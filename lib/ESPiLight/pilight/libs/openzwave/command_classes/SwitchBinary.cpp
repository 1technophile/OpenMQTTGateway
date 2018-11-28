//-----------------------------------------------------------------------------
//
//	SwitchBinary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_BINARY
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

#include "CommandClasses.h"
#include "SwitchBinary.h"
#include "WakeUp.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../platform/Log.h"

#include "../value_classes/ValueBool.h"

using namespace OpenZWave;

enum SwitchBinaryCmd
{
	SwitchBinaryCmd_Set		= 0x01,
	SwitchBinaryCmd_Get		= 0x02,
	SwitchBinaryCmd_Report	= 0x03
};

//-----------------------------------------------------------------------------
// <SwitchBinary::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool SwitchBinary::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchBinary::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool SwitchBinary::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "SwitchBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchBinaryCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "SwitchBinaryCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SwitchBinary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchBinary::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (SwitchBinaryCmd_Report == (SwitchBinaryCmd)_data[0])
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received SwitchBinary report from node %d: level=%s", GetNodeId(), _data[1] ? "On" : "Off" );

		if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, 0 ) ) )
		{
			value->OnValueRefreshed( _data[1] != 0 );
			value->Release();
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchBinary::SetValue>
// Set the state of the switch
//-----------------------------------------------------------------------------
bool SwitchBinary::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Bool == _value.GetID().GetType() )
	{
		ValueBool const* value = static_cast<ValueBool const*>(&_value);

		Log::Write( LogLevel_Info, GetNodeId(), "SwitchBinary::Set - Setting node %d to %s", GetNodeId(), value->GetValue() ? "On" : "Off" );
		Msg* msg = new Msg( "SwitchBinary Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchBinaryCmd_Set );
		msg->Append( value->GetValue() ? 0xff : 0x00 );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchBinary::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
void SwitchBinary::SetValueBasic
(
	uint8 const _instance,
	uint8 const _value
)
{
	// Send a request for new value to synchronize it with the BASIC set/report.
	// In case the device is sleeping, we set the value anyway so the BASIC set/report
	// stays in sync with it. We must be careful mapping the uint8 BASIC value
	// into a class specific value.
	// When the device wakes up, the real requested value will be retrieved.
	RequestValue( 0, 0, _instance, Driver::MsgQueue_Send );
	if( Node* node = GetNodeUnsafe() )
	{
		if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
		{
			if( !wakeUp->IsAwake() )
			{
				if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, 0 ) ) )
				{
					value->OnValueRefreshed( _value != 0 );
					value->Release();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <SwitchBinary::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchBinary::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueBool( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Switch", "", false, false, false, 0 );
	}
}
