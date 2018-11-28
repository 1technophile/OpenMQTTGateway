//-----------------------------------------------------------------------------
//
//	Msg.cpp
//
//	Represents a Z-Wave message
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "platform/Log.h"
#include "command_classes/MultiInstance.h"

using namespace OpenZWave;

uint8 Msg::s_nextCallbackId = 1;


//-----------------------------------------------------------------------------
// <Msg::Msg>
// Constructor
//-----------------------------------------------------------------------------
Msg::Msg
( 
	string const& _logText,
	uint8 _targetNodeId,
	uint8 const _msgType,
	uint8 const _function,
	bool const _bCallbackRequired,
	bool const _bReplyRequired,			// = true
	uint8 const _expectedReply,			// = 0
	uint8 const _expectedCommandClassId	// = 0
):
	m_logText( _logText ),
	m_bFinal( false ),
	m_bCallbackRequired( _bCallbackRequired ),
	m_callbackId( 0 ),
	m_expectedReply( 0 ),
	m_expectedCommandClassId( _expectedCommandClassId ),
	m_length( 4 ),
	m_targetNodeId( _targetNodeId ),
	m_sendAttempts( 0 ),
	m_maxSendAttempts( MAX_TRIES ),
	m_instance( 1 ),
	m_endPoint( 0 ),
	m_flags( 0 )
{
	if( _bReplyRequired )
	{
		// Wait for this message before considering the transaction complete 
		m_expectedReply = _expectedReply ? _expectedReply : _function;
	}

	m_buffer[0] = SOF;
	m_buffer[1] = 0;					// Length of the following data, filled in during Finalize.
	m_buffer[2] = _msgType;
	m_buffer[3] = _function;
}

//-----------------------------------------------------------------------------
// <Msg::SetInstance>
// Used to enable wrapping with MultiInstance/MultiChannel during finalize.
//-----------------------------------------------------------------------------
void Msg::SetInstance
(
	CommandClass* _cc,
	uint8 const _instance
)
{
	// Determine whether we should encapsulate the message in MultiInstance or MultiCommand
	if( Node* node = _cc->GetNodeUnsafe() )
	{
		MultiInstance* micc = static_cast<MultiInstance*>( node->GetCommandClass( MultiInstance::StaticGetCommandClassId() ) );
		m_instance = _instance;
		if( micc )
		{
			if( micc->GetVersion() > 1 )
			{
				m_endPoint = _cc->GetEndPoint( _instance );
				if( m_endPoint != 0 )
				{
					// Set the flag bit to indicate MultiChannel rather than MultiInstance
					m_flags |= m_MultiChannel;
					m_expectedCommandClassId = MultiInstance::StaticGetCommandClassId();
				}
			}
			else if( m_instance > 1 )
			{
				// Set the flag bit to indicate MultiInstance rather than MultiChannel
				m_flags |= m_MultiInstance;
				m_expectedCommandClassId = MultiInstance::StaticGetCommandClassId();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Msg::Append>
// Add a byte to the message
//-----------------------------------------------------------------------------
void Msg::Append
(
	uint8 const _data
)
{
	m_buffer[m_length++] = _data;
}

//-----------------------------------------------------------------------------
// <Msg::Finalize>
// Fill in the length and checksum values for the message
//-----------------------------------------------------------------------------
void Msg::Finalize()
{
	if( m_bFinal )
	{
		// Already finalized
		return;
	}

	// Deal with Multi-Channel/Instance encapsulation
	if( ( m_flags & ( m_MultiChannel | m_MultiInstance ) ) != 0 )
	{
		MultiEncap();
	}

	// Add the callback id
	if( m_bCallbackRequired )
	{
		// Set the length byte
		m_buffer[1] = m_length;		// Length of following data

		if( 0 == s_nextCallbackId )
		{
			s_nextCallbackId = 1;
		}

		m_buffer[m_length++] = s_nextCallbackId;	
		m_callbackId = s_nextCallbackId++;
	}
	else
	{
		// Set the length byte
		m_buffer[1] = m_length - 1;		// Length of following data
	}

	// Calculate the checksum
	uint8 checksum = 0xff;
	for( uint32 i=1; i<m_length; ++i ) 
	{
		checksum ^= m_buffer[i];
	}
	m_buffer[m_length++] = checksum;

	m_bFinal = true;
}


//-----------------------------------------------------------------------------
// <Msg::UpdateCallbackId>
// If this message has a callback ID, increment it and recalculate the checksum
//-----------------------------------------------------------------------------
void Msg::UpdateCallbackId()
{
	if( m_bCallbackRequired )
	{
		// update the callback ID
		m_buffer[m_length-2] = s_nextCallbackId;
		m_callbackId = s_nextCallbackId++;

		// Recalculate the checksum
		uint8 checksum = 0xff;
		for( int32 i=1; i<m_length-1; ++i ) 
		{
			checksum ^= m_buffer[i];
		}
		m_buffer[m_length-1] = checksum;
	}
}


//-----------------------------------------------------------------------------
// <Msg::GetAsString>
// Create a string containing the raw data
//-----------------------------------------------------------------------------
string Msg::GetAsString()
{
	string str = m_logText;

	char byteStr[16];
	if( m_targetNodeId != 0xff )
	{
		snprintf( byteStr, sizeof(byteStr), " (Node=%d)", m_targetNodeId );
		str += byteStr;
	}

	str += ": ";

	for( uint32 i=0; i<m_length; ++i ) 
	{
		if( i )
		{
			str += ", ";
		}

		snprintf( byteStr, sizeof(byteStr), "0x%.2x", m_buffer[i] );
		str += byteStr;
	}

	return str;
}

//-----------------------------------------------------------------------------
// <Msg::MultiEncap>
// Encapsulate the data inside a MultiInstance/Multicommand message
//-----------------------------------------------------------------------------
void Msg::MultiEncap
(
)
{
	char str[256];
	if( m_buffer[3]	!= FUNC_ID_ZW_SEND_DATA )
	{
		return;
	}

	// Insert the encap header
	if( ( m_flags & m_MultiChannel ) != 0 )
	{
		// MultiChannel
		for( uint32 i=m_length-1; i>=6; --i )
		{
			m_buffer[i+4] = m_buffer[i];
		}

		m_buffer[5] += 4;
		m_buffer[6] = MultiInstance::StaticGetCommandClassId();
		m_buffer[7] = MultiInstance::MultiChannelCmd_Encap;
		m_buffer[8] = 1;
		m_buffer[9] = m_endPoint;
		m_length += 4;

		snprintf( str, sizeof(str), "MultiChannel Encapsulated (instance=%d): %s", m_instance, m_logText.c_str() );
		m_logText = str;
	}
	else
	{
		// MultiInstance
		for( uint32 i=m_length-1; i>=6; --i )
		{
			m_buffer[i+3] = m_buffer[i];
		}

		m_buffer[5] += 3;
		m_buffer[6] = MultiInstance::StaticGetCommandClassId();
		m_buffer[7] = MultiInstance::MultiInstanceCmd_Encap;
		m_buffer[8] = m_instance;
		m_length += 3;

		snprintf( str, sizeof(str), "MultiInstance Encapsulated (instance=%d): %s", m_instance, m_logText.c_str() );
		m_logText = str;
	}
}
