//-----------------------------------------------------------------------------
//
//	Powerlevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_POWERLEVEL
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

#include <vector>

#include "CommandClasses.h"
#include "Powerlevel.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueShort.h"
#include "../value_classes/ValueList.h"
#include "../value_classes/ValueButton.h"

using namespace OpenZWave;

enum PowerlevelCmd
{
	PowerlevelCmd_Set			= 0x01,
	PowerlevelCmd_Get			= 0x02,
	PowerlevelCmd_Report			= 0x03,
	PowerlevelCmd_TestNodeSet		= 0x04,
	PowerlevelCmd_TestNodeGet		= 0x05,
	PowerlevelCmd_TestNodeReport		= 0x06
};

enum
{
	PowerlevelIndex_Powerlevel = 0,
	PowerlevelIndex_Timeout,
	PowerlevelIndex_Set,
	PowerlevelIndex_TestNode,
	PowerlevelIndex_TestPowerlevel,
	PowerlevelIndex_TestFrames,
	PowerlevelIndex_Test,
	PowerlevelIndex_Report,
	PowerlevelIndex_TestStatus,
	PowerlevelIndex_TestAckFrames
};

static char const* c_powerLevelNames[] =
{
	"Normal",
	"-1dB",
	"-2dB",
	"-3dB",
	"-4dB",
	"-5dB",
	"-6dB",
	"-7dB",
	"-8dB",
	"-9dB",
	"Unknown"
};

static char const* c_powerLevelStatusNames[] =
{
	"Failed",
	"Success",
	"In Progress",
	"Unknown"
};


//-----------------------------------------------------------------------------
// <Powerlevel::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Powerlevel::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Session )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Powerlevel::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Powerlevel::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _index,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _index == 0 )
	{
		if ( IsGetSupported() )
		{
			Msg* msg = new Msg( "Powerlevel_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( PowerlevelCmd_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		} else {
			Log::Write(  LogLevel_Info, GetNodeId(), "Powerlevel_Get Not Supported on this node");
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Powerlevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Powerlevel::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( PowerlevelCmd_Report == (PowerlevelCmd)_data[0] )
	{
		PowerLevelEnum powerLevel = (PowerLevelEnum)_data[1];
		if (powerLevel > 9) /* size of c_powerLevelNames minus Unknown*/
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "powerLevel Value was greater than range. Setting to Invalid");
			powerLevel = (PowerLevelEnum)10;
		}
		uint8 timeout = _data[2];

		Log::Write( LogLevel_Info, GetNodeId(), "Received a PowerLevel report: PowerLevel=%s, Timeout=%d", c_powerLevelNames[powerLevel], timeout );
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, PowerlevelIndex_Powerlevel ) ) )
		{
			value->OnValueRefreshed( (int)powerLevel );
			value->Release();
		}
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, PowerlevelIndex_Timeout ) ) )
		{
			value->OnValueRefreshed( timeout );
			value->Release();
		}
		return true;
	}

	if( PowerlevelCmd_TestNodeReport == (PowerlevelCmd)_data[0] )
	{
		uint8 testNode = _data[1];
		PowerLevelStatusEnum status = (PowerLevelStatusEnum)_data[2];
		uint16 ackCount = (((uint16)_data[3])<<8) | (uint16)_data[4];

		if (status > 2) /* size of c_powerLevelStatusNames minus Unknown */
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "status Value was greater than range. Setting to Unknown");
			status = (PowerLevelStatusEnum)3;
		}

		Log::Write( LogLevel_Info, GetNodeId(), "Received a PowerLevel Test Node report: Test Node=%d, Status=%s, Test Frame ACK Count=%d", testNode, c_powerLevelStatusNames[status], ackCount );
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, PowerlevelIndex_TestNode ) ) )
		{
			value->OnValueRefreshed( testNode );
			value->Release();
		}
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, PowerlevelIndex_TestStatus ) ) )
		{
			value->OnValueRefreshed( (int)status );
			value->Release();
		}
		if( ValueShort* value = static_cast<ValueShort*>( GetValue( _instance, PowerlevelIndex_TestAckFrames ) ) )
		{
			value->OnValueRefreshed( (short)ackCount );
			value->Release();
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Powerlevel::SetValue>
// Set a value on the Z-Wave device
//-----------------------------------------------------------------------------
bool Powerlevel::SetValue
(
	Value const& _value
)
{
	bool res = false;
	uint8 instance = _value.GetID().GetInstance();

	switch( _value.GetID().GetIndex() )
	{
		case PowerlevelIndex_Powerlevel:
		{
			if( ValueList* value = static_cast<ValueList*>( GetValue( instance, PowerlevelIndex_Powerlevel ) ) )
			{
				ValueList::Item const& item = (static_cast<ValueList const*>( &_value))->GetItem();
				value->OnValueRefreshed( item.m_value );
				value->Release();
			}
			res = true;
			break;
		}
		case PowerlevelIndex_Timeout:
		{
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, PowerlevelIndex_Timeout ) ) )
			{
				value->OnValueRefreshed( (static_cast<ValueByte const*>( &_value))->GetValue() );
				value->Release();
			}
			res = true;
			break;
		}
		case PowerlevelIndex_Set:
		{
			// Set
			if( ValueButton* button = static_cast<ValueButton*>( GetValue( instance, PowerlevelIndex_Set ) ) )
			{
				if( button->IsPressed() )
				{
					res = Set( instance );
				}
				button->Release();
			}
			break;
		}
		case PowerlevelIndex_TestNode:
		{
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, PowerlevelIndex_TestNode ) ) )
			{
				value->OnValueRefreshed( (static_cast<ValueByte const*>( &_value))->GetValue() );
				value->Release();
			}
			res = true;
			break;
		}
		case PowerlevelIndex_TestPowerlevel:
		{
			if( ValueList* value = static_cast<ValueList*>( GetValue( instance, PowerlevelIndex_TestPowerlevel ) ) )
			{
				ValueList::Item const& item = (static_cast<ValueList const*>( &_value))->GetItem();
				value->OnValueRefreshed( item.m_value );
				value->Release();
			}
			res = true;
			break;
		}
		case PowerlevelIndex_TestFrames:
		{
			if( ValueShort* value = static_cast<ValueShort*>( GetValue( instance, PowerlevelIndex_TestFrames ) ) )
			{
				value->OnValueRefreshed( (static_cast<ValueByte const*>( &_value))->GetValue() );
				value->Release();
			}
			res = true;
			break;
		}
		case PowerlevelIndex_Test:
		{
			// Test
			if( ValueButton* button = static_cast<ValueButton*>( GetValue( instance, PowerlevelIndex_Test ) ) )
			{
				if( button->IsPressed() )
				{
					res = Test( instance );
				}
				button->Release();
			}
			break;
		}
		case PowerlevelIndex_Report:
		{
			// Test
			if( ValueButton* button = static_cast<ValueButton*>( GetValue( instance, PowerlevelIndex_Report ) ) )
			{
				if( button->IsPressed() )
				{
					res = Report( instance );
				}
				button->Release();
			}
			break;
		}
	}
	return res;
}

//-----------------------------------------------------------------------------
// <Powerlevel::Set>
// Set the transmit power of a node for a specified time
//-----------------------------------------------------------------------------
bool Powerlevel::Set
(
	uint8 const _instance
)
{
	PowerLevelEnum powerLevel;
	uint8 timeout;

	if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, PowerlevelIndex_Powerlevel ) ) )
	{
		ValueList::Item const& item = value->GetItem();
		powerLevel = (PowerLevelEnum)item.m_value;
		value->Release();
	}
	else
	{
		return false;
	}

	if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, PowerlevelIndex_Timeout ) ) )
	{
		timeout = value->GetValue();
		value->Release();
	}
	else
	{
		return false;
	}
	if (powerLevel > 9) /* size of c_powerLevelNames minus Unknown */
	{
		Log::Write (LogLevel_Warning, GetNodeId(), "powerLevel Value was greater than range. Dropping");
		/* Drop it */
		return false;
	}


	Log::Write( LogLevel_Info, GetNodeId(), "Setting the power level to %s for %d seconds", c_powerLevelNames[powerLevel], timeout );
	Msg* msg = new Msg( "PowerlevelCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( PowerlevelCmd_Set );
	msg->Append( (uint8)powerLevel );
	msg->Append( timeout );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	return true;
}

//-----------------------------------------------------------------------------
// <Powerlevel::Test>
// Test node to node communications
//-----------------------------------------------------------------------------
bool Powerlevel::Test
(
	uint8 const _instance
)
{
	uint8 testNodeId;
	PowerLevelEnum powerLevel;
	uint16 numFrames;

	if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, PowerlevelIndex_TestNode ) ) )
	{
		testNodeId = value->GetValue();
		value->Release();
	}
	else
	{
		return false;
	}

	if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, PowerlevelIndex_TestPowerlevel ) ) )
	{
		ValueList::Item const& item = value->GetItem();
		powerLevel = (PowerLevelEnum)item.m_value;
		value->Release();
	}
	else
	{
		return false;
	}

	if( ValueShort* value = static_cast<ValueShort*>( GetValue( _instance, PowerlevelIndex_TestFrames ) ) )
	{
		numFrames = value->GetValue();
		value->Release();
	}
	else
	{
		return false;
	}
	if (powerLevel > 9) /* size of c_powerLevelNames minus Unknown */
	{
		Log::Write (LogLevel_Warning, GetNodeId(), "powerLevel Value was greater than range. Dropping");
		return false;
	}


	Log::Write( LogLevel_Info, GetNodeId(), "Running a Power Level Test: Target Node = %d, Power Level = %s, Number of Frames = %d", testNodeId, c_powerLevelNames[powerLevel], numFrames );
	Msg* msg = new Msg( "PowerlevelCmd_TestNodeSet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
	msg->Append( 6 );
	msg->Append( GetCommandClassId() );
	msg->Append( PowerlevelCmd_TestNodeSet );
	msg->Append( testNodeId );
	msg->Append( (uint8)powerLevel );
	msg->Append( (uint8)(numFrames >> 8) );
	msg->Append( (uint8)(numFrames & 0x00ff) );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	return true;
}

//-----------------------------------------------------------------------------
// <Powerlevel::Report>
// Request test report
//-----------------------------------------------------------------------------
bool Powerlevel::Report
(
	uint8 const _instance
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "Power Level Report" );
	Msg* msg = new Msg( "PowerlevelCmd_TestNodeGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
	msg->Append( 6 );
	msg->Append( GetCommandClassId() );
	msg->Append( PowerlevelCmd_TestNodeGet );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	return true;
}

//-----------------------------------------------------------------------------
// <Powerlevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Powerlevel::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		vector<ValueList::Item> items;

		ValueList::Item item;
		for( uint8 i=0; i<10; ++i )
		{
			item.m_label = c_powerLevelNames[i];
			item.m_value = i;
			items.push_back( item );
		}

		node->CreateValueList( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_Powerlevel, "Powerlevel", "dB", false, false, 1, items, 0, 0 );
		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_Timeout, "Timeout", "seconds", false, false, 0, 0 );
		node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_Set, "Set Powerlevel", 0 );
		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_TestNode, "Test Node", "", false, false, 0, 0 );
		node->CreateValueList( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_TestPowerlevel, "Test Powerlevel", "dB", false, false, 1, items, 0, 0 );
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_TestFrames, "Frame Count", "", false, false, 0, 0 );
		node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_Test, "Test", 0 );
		node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_Report, "Report", 0 );

		items.clear();
		for( uint8 i=0; i<3; ++i )
		{
			item.m_label = c_powerLevelStatusNames[i];
			item.m_value = i;
			items.push_back( item );
		}
		node->CreateValueList( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_TestStatus, "Test Status", "", true, false, 1, items, 0, 0 );
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, PowerlevelIndex_TestAckFrames, "Acked Frames", "", true, false, 0, 0 );
	}
}
