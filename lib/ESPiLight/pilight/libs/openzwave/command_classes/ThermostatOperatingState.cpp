//-----------------------------------------------------------------------------
//
//	ThermostatOperatingState.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_OPERATING_STATE
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
#include "ThermostatOperatingState.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueString.h"

using namespace OpenZWave;

enum ThermostatOperatingStateCmd
{
	ThermostatOperatingStateCmd_Get				= 0x02,
	ThermostatOperatingStateCmd_Report			= 0x03
};

static char const* c_stateName[] =
{
	"Idle",
	"Heating",
	"Cooling",
	"Fan Only",
	"Pending Heat",
	"Pending Cool",
	"Vent / Economizer",
	"State 07",				// Undefined states.  May be used in the future.
	"State 08",
	"State 09",
	"State 10",
	"State 11",
	"State 12",
	"State 13",
	"State 14",
	"State 15"
};

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::RequestState>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
bool ThermostatOperatingState::RequestState
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
// <ThermostatOperatingState::RequestValue>
// Get a thermostat mode value from the device
//-----------------------------------------------------------------------------
bool ThermostatOperatingState::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "ThermostatOperatingStateCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatOperatingStateCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "ThermostatOperatingStateCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatOperatingState::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( ThermostatOperatingStateCmd_Report == (ThermostatOperatingStateCmd)_data[0] )
	{
		// We have received the thermostat operating state from the Z-Wave device
		if( ValueString* valueString = static_cast<ValueString*>( GetValue( _instance, 0 ) ) )
		{
			/* no need bounds checking on c_stateName here, as it can only be 1 Byte anyway */
			valueString->OnValueRefreshed( c_stateName[_data[1]&0x0f] );
			valueString->Release();
			Log::Write( LogLevel_Info, GetNodeId(), "Received thermostat operating state: %s", valueString->GetValue().c_str() );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <ThermostatOperatingState::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void ThermostatOperatingState::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Operating State", "", true, false, c_stateName[0], 0 );
	}
}

