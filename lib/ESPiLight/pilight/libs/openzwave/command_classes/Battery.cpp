//-----------------------------------------------------------------------------
//
//	Battery.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BATTERY
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
#include "Battery.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueByte.h"

using namespace OpenZWave;

enum BatteryCmd
{
	BatteryCmd_Get		= 0x02,
	BatteryCmd_Report	= 0x03
};

//-----------------------------------------------------------------------------
// <Battery::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Battery::RequestState
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
// <Battery::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Battery::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "BatteryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( BatteryCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "BatteryCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Battery::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Battery::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (BatteryCmd_Report == (BatteryCmd)_data[0])
	{
		// We have received a battery level report from the Z-Wave device.
		// Devices send 0xff instead of zero for a low battery warning.
		uint8 batteryLevel = _data[1];
		if( batteryLevel == 0xff )
		{
			batteryLevel = 0;
		}

		Log::Write( LogLevel_Info, GetNodeId(), "Received Battery report from node %d: level=%d", GetNodeId(), batteryLevel );

		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, 0 ) ) )
		{
			value->OnValueRefreshed( batteryLevel );
			value->Release();
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Battery::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Battery::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Battery Level", "%", true, false, 100, 0 );
	}
}


