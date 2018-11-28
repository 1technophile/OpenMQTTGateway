//-----------------------------------------------------------------------------
//
//	Lock.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_LOCK
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
#include "Lock.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueBool.h"

using namespace OpenZWave;

enum LockCmd
{
	LockCmd_Set		= 0x01,
	LockCmd_Get		= 0x02,
	LockCmd_Report	= 0x03
};

//-----------------------------------------------------------------------------
// <Lock::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Lock::RequestState
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
// <Lock::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Lock::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "LockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( LockCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "LockCmd_Get Not Supported on this node");
	}
	return false;
}


//-----------------------------------------------------------------------------
// <Lock::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Lock::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( LockCmd_Report == (LockCmd)_data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received Lock report: Lock is %s", _data[1] ? "Locked" : "Unlocked" );

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
// <Lock::SetValue>
// Set the lock's state
//-----------------------------------------------------------------------------
bool Lock::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Bool == _value.GetID().GetType() )
	{
		ValueBool const* value = static_cast<ValueBool const*>(&_value);

		Log::Write( LogLevel_Info, GetNodeId(), "Lock::Set - Requesting lock to be %s", value->GetValue() ? "Locked" : "Unlocked" );
		Msg* msg = new Msg( "LockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( LockCmd_Set );
		msg->Append( value->GetValue() ? 0x01:0x00 );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Lock::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Lock::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueBool( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Locked", "", false, false, false, 0 );
	}
}


