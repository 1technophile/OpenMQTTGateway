//-----------------------------------------------------------------------------
//
//	SensorAlarm.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_ALARM
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
#include "SensorAlarm.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueByte.h"

using namespace OpenZWave;

enum SensorAlarmCmd
{
	SensorAlarmCmd_Get				= 0x01,
	SensorAlarmCmd_Report			= 0x02,
	SensorAlarmCmd_SupportedGet		= 0x03,
	SensorAlarmCmd_SupportedReport	= 0x04
};

static char const* c_alarmTypeName[] =
{
	"General",
	"Smoke",
	"Carbon Monoxide",
	"Carbon Dioxide",
	"Heat",
	"Flood"
};


//-----------------------------------------------------------------------------
// <SensorAlarm::SensorAlarm>
// Constructor
//-----------------------------------------------------------------------------
SensorAlarm::SensorAlarm
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId )
{
	SetStaticRequest( StaticRequest_Values );
}

//-----------------------------------------------------------------------------
// <SensorAlarm::RequestState>
// Get the static thermostat setpoint details from the device
//-----------------------------------------------------------------------------
bool SensorAlarm::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		requests = RequestValue( _requestFlags, 0xff, _instance, _queue );
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		for( uint8 i=0; i<SensorAlarm_Count; ++i )
		{
			Value* value = GetValue( 1, i );
			if( value != NULL )
			{
				value->Release();
				// There is a value for this alarm type, so request it
				requests |= RequestValue( _requestFlags, i, _instance, _queue );
			}
		}
	}

	return requests;
}

//-----------------------------------------------------------------------------
// <SensorAlarm::RequestValue>
// Get the sensor alarm details from the device
//-----------------------------------------------------------------------------
bool SensorAlarm::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _alarmType,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _alarmType == 0xff )
	{
		// Request the supported alarm types
		Msg* msg = new Msg( "SensorAlarmCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SensorAlarmCmd_SupportedGet );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	}
	else
	{
		// Request the alarm state
		if ( IsGetSupported() )
		{
			Msg* msg = new Msg( "SensorAlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( SensorAlarmCmd_Get );
			msg->Append( _alarmType );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		} else {
			Log::Write(  LogLevel_Info, GetNodeId(), "SensorAlarmCmd_Get Not Supported on this node");
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SensorAlarm::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SensorAlarm::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( SensorAlarmCmd_Report == (SensorAlarmCmd)_data[0] )
	{
		// We have received an alarm state report from the Z-Wave device
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, _data[2] ) ) )
		{
			uint8 sourceNodeId = _data[1];
			uint8 state = _data[3];
			// uint16 time = (((uint16)_data[4])<<8) | (uint16)_data[5];  Don't know what to do with this yet.

			value->OnValueRefreshed( state );
			value->Release();
			Log::Write( LogLevel_Info, GetNodeId(), "Received alarm state report from node %d: %s = %d", sourceNodeId, value->GetLabel().c_str(), state );
		}

		return true;
	}

	if( SensorAlarmCmd_SupportedReport == (SensorAlarmCmd)_data[0] )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			// We have received the supported alarm types from the Z-Wave device
			Log::Write( LogLevel_Info, GetNodeId(), "Received supported alarm types" );

			// Parse the data for the supported alarm types
			uint8 numBytes = _data[1];
			for( uint32 i=0; i<numBytes; ++i )
			{
				for( int32 bit=0; bit<8; ++bit )
				{
					if( ( _data[i+2] & (1<<bit) ) != 0 )
					{
						// Add supported setpoint
						int32 index = (int32)(i<<3) + bit;
						if( index < SensorAlarm_Count )
						{
						  	node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, index, c_alarmTypeName[index], "", true, false, 0, 0 );
							Log::Write( LogLevel_Info, GetNodeId(), "    Added alarm type: %s", c_alarmTypeName[index] );
						}
					}
				}
			}
		}

		ClearStaticRequest( StaticRequest_Values );
		return true;
	}

	return false;
}

