//-----------------------------------------------------------------------------
//
//	UserCode.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_USER_CODE
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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

#include "../tinyxml.h"
#include "CommandClasses.h"
#include "UserCode.h"
#include "../Node.h"
#include "../Options.h"
#include "../platform/Log.h"

#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueRaw.h"

using namespace OpenZWave;

enum UserCodeCmd
{
	UserCodeCmd_Set			= 0x01,
	UserCodeCmd_Get			= 0x02,
	UserCodeCmd_Report		= 0x03,
	UserNumberCmd_Get		= 0x04,
	UserNumberCmd_Report		= 0x05
};

enum
{
	UserCodeIndex_Refresh	= 254,
	UserCodeIndex_Count		= 255
};

const uint8 UserCodeLength = 10;

//-----------------------------------------------------------------------------
// <UserCode::UserCode>
// Constructor
//-----------------------------------------------------------------------------
UserCode::UserCode
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),
	m_queryAll( false ),
	m_currentCode( 0 ),
	m_userCodeCount( 0 ),
	m_refreshUserCodes(false)
{
	SetStaticRequest( StaticRequest_Values );
	memset( m_userCodesStatus, 0xff, sizeof(m_userCodesStatus) );
	Options::Get()->GetOptionAsBool("RefreshAllUserCodes", &m_refreshUserCodes );

}

//-----------------------------------------------------------------------------
// <UserCode::ReadXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void UserCode::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	int32 intVal;

	CommandClass::ReadXML( _ccElement );
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "codes", &intVal ) )
	{
		m_userCodeCount = intVal;
	}
}

//-----------------------------------------------------------------------------
// <UserCode::WriteXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void UserCode::WriteXML
(
	TiXmlElement* _ccElement
)
{
	char str[32];

	CommandClass::WriteXML( _ccElement );
	snprintf( str, sizeof(str), "%d", m_userCodeCount );
	_ccElement->SetAttribute( "codes", str);
}

//-----------------------------------------------------------------------------
// <UserCode::RequestState>
// Nothing to do for UserCode
//-----------------------------------------------------------------------------
bool UserCode::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		requests |= RequestValue( _requestFlags, UserCodeIndex_Count, _instance, _queue );
	}

	if( _requestFlags & RequestFlag_Session )
	{
		if( m_userCodeCount > 0 )
		{
			m_queryAll = true;
			m_currentCode = 1;
			requests |= RequestValue( _requestFlags, m_currentCode, _instance, _queue );
		}
	}

	return requests;
}

//-----------------------------------------------------------------------------
// <UserCode::RequestValue>
// Nothing to do for UserCode
//-----------------------------------------------------------------------------
bool UserCode::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _userCodeIdx,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}
	if ( !IsGetSupported() )
	{
		Log::Write(  LogLevel_Info, GetNodeId(), "UserNumberCmd_Get Not Supported on this node");
		return false;
	}
	if( _userCodeIdx == UserCodeIndex_Count )
	{
		// Get number of supported user codes.
		Msg* msg = new Msg( "UserNumberCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( UserNumberCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	}
	if (_userCodeIdx == 0)
	{
		Log::Write( LogLevel_Warning, GetNodeId(), "UserCodeCmd_Get with Index 0 not Supported");
		return false;
	}
	Msg* msg = new Msg( "UserCodeCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( UserCodeCmd_Get );
	msg->Append( _userCodeIdx );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <UserCode::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool UserCode::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( UserNumberCmd_Report == (UserCodeCmd)_data[0] )
	{
		m_userCodeCount = _data[1];
		if( m_userCodeCount > 254 )
		{
			// Make space for code count.
			m_userCodeCount = 254;
		}
		ClearStaticRequest( StaticRequest_Values );
		if( m_userCodeCount == 0 )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received User Number report from node %d: Not supported", GetNodeId() );
		}
		else
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received User Number report from node %d: Supported Codes %d (%d)", GetNodeId(), m_userCodeCount, _data[1] );
		}

		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, UserCodeIndex_Count ) ) )
		{
			value->OnValueRefreshed( m_userCodeCount );
			value->Release();
		}

		if( Node* node = GetNodeUnsafe() )
		{
			uint8 data[UserCodeLength];

			memset( data, 0, UserCodeLength );
			for( uint8 i = 0; i <= m_userCodeCount; i++ )
			{
				char str[16];
				if (i == 0)
				{
					snprintf( str, sizeof(str), "Enrollment Code");
					node->CreateValueRaw( ValueID::ValueGenre_User, GetCommandClassId(), _instance, i, str, "", true, false, data, UserCodeLength, 0 );
				}
				else
				{
					snprintf( str, sizeof(str), "Code %d:", i);
					node->CreateValueRaw( ValueID::ValueGenre_User, GetCommandClassId(), _instance, i, str, "", false, false, data, UserCodeLength, 0 );
				}
			}
		}
		return true;
	}
	else if( UserCodeCmd_Report == (UserCodeCmd)_data[0] )
	{
		int i = _data[1];
		if( ValueRaw* value = static_cast<ValueRaw*>( GetValue( _instance, i ) ) )
		{
			uint8 data[UserCodeLength];
			uint8 size = _length - 4;
			if( size > UserCodeLength )
			{
				Log::Write( LogLevel_Warning, GetNodeId(), "User Code length %d is larger then maximum %d", size, UserCodeLength );
				size = UserCodeLength;
			}
			m_userCodesStatus[i] = _data[2];
			memcpy( data, &_data[3], size );
			value->OnValueRefreshed( data, size );
			value->Release();
		}
		Log::Write( LogLevel_Info, GetNodeId(), "Received User Code Report from node %d for User Code %d (%s)", GetNodeId(), i, CodeStatus( _data[2] ).c_str() );
		if( m_queryAll && i == m_currentCode )
		{

			if (m_refreshUserCodes && (_data[2] != UserCode_Available)) {
				if( ++i <= m_userCodeCount )
				{
					m_currentCode = i;
					RequestValue( 0, m_currentCode, _instance, Driver::MsgQueue_Query );
				}
				else
				{
					m_queryAll = false;
					/* we might have reset this as part of the RefreshValues Button Value */
					Options::Get()->GetOptionAsBool("RefreshAllUserCodes", &m_refreshUserCodes );
				}
			} else {
				Log::Write( LogLevel_Info, GetNodeId(), "Not Requesting additional UserCode Slots as RefreshAllUserCodes is false, and slot %d is available", i);
				m_queryAll = false;
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <UserCode::SetValue>
// Set a User Code value
//-----------------------------------------------------------------------------
bool UserCode::SetValue
(
	Value const& _value
)
{
	if( (ValueID::ValueType_Raw == _value.GetID().GetType()) && (_value.GetID().GetIndex() < UserCodeIndex_Refresh) )
	{
		ValueRaw const* value = static_cast<ValueRaw const*>(&_value);
		uint8* s = value->GetValue();
		uint8 len = value->GetLength();

		if( len > UserCodeLength )
		{
			return false;
		}
		m_userCodesStatus[value->GetID().GetIndex()] = UserCode_Occupied;
		Msg* msg = new Msg( "UserCodeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 4 + len );
		msg->Append( GetCommandClassId() );
		msg->Append( UserCodeCmd_Set );
		msg->Append( value->GetID().GetIndex() );
		msg->Append( UserCode_Occupied );
		for( uint8 i = 0; i < len; i++ )
		{
			msg->Append( s[i] );
		}
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}
	if ( (ValueID::ValueType_Button == _value.GetID().GetType()) && (_value.GetID().GetIndex() == UserCodeIndex_Refresh) )
	{
		m_refreshUserCodes = true;
		m_currentCode = 1;
		m_queryAll = true;
		RequestValue( 0, m_currentCode, _value.GetID().GetInstance(), Driver::MsgQueue_Query );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <UserCode::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void UserCode::CreateVars
(
	uint8 const _instance

)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, UserCodeIndex_Count, "Code Count", "", true, false, 0, 0 );
		node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, UserCodeIndex_Refresh, "Refresh All UserCodes", 0);
	}
}
