//-----------------------------------------------------------------------------
//
//	SwitchToggleBinary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_TOGGLE_BINARY
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
#include "SwitchToggleBinary.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../platform/Log.h"

#include "../value_classes/ValueBool.h"

using namespace OpenZWave;

enum SwitchToggleBinaryCmd
{
	SwitchToggleBinaryCmd_Set		= 0x01,
	SwitchToggleBinaryCmd_Get		= 0x02,
	SwitchToggleBinaryCmd_Report	= 0x03
};

//-----------------------------------------------------------------------------
// <SwitchToggleBinary::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool SwitchToggleBinary::RequestState
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
// <SwitchToggleBinary::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool SwitchToggleBinary::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "SwitchToggleBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchToggleBinaryCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "SwitchToggleBinaryCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SwitchToggleBinary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchToggleBinary::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( SwitchToggleBinaryCmd_Report == (SwitchToggleBinaryCmd)_data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received SwitchToggleBinary report: %s", _data[1] ? "On" : "Off" );

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
// <SwitchToggleBinary::SetValue>
// Toggle the state of the switch
//-----------------------------------------------------------------------------
bool SwitchToggleBinary::SetValue
(
	Value const& _value
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "SwitchToggleBinary::Set - Toggling the state" );
	Msg* msg = new Msg( "SwitchToggleBinary Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->SetInstance( this, _value.GetID().GetInstance() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchToggleBinaryCmd_Set );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	return true;
}

//-----------------------------------------------------------------------------
// <SwitchToggleBinary::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchToggleBinary::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueBool( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Toggle Switch", "", false, false, false, 0 );
	}
}


