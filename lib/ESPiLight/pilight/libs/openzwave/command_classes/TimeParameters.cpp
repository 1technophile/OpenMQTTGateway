//-----------------------------------------------------------------------------
//
//	TimeParameters.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_TIME_PARAMETERS
//
//	Copyright (c) 2014 Justin Hammond <Justin@dynam.ac>
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

#include <time.h>
#include "CommandClasses.h"
#include "TimeParameters.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueButton.h"
#include "../value_classes/ValueString.h"

using namespace OpenZWave;

enum TimeParametersCmd
{
	TimeParametersCmd_Set		= 0x01,
	TimeParametersCmd_Get		= 0x02,
	TimeParametersCmd_Report	= 0x03
};

enum
{
	TimeParametersIndex_Date = 0,
	TimeParametersIndex_Time,
	TimeParametersIndex_Set,
	TimeParametersIndex_Refresh
};


//-----------------------------------------------------------------------------
// <DoorLockLogging::DoorLockLogging>
// Constructor
//-----------------------------------------------------------------------------
TimeParameters::TimeParameters
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId )
{
	SetStaticRequest( StaticRequest_Values );
}


//-----------------------------------------------------------------------------
// <TimeParameters::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool TimeParameters::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <TimeParameters::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool TimeParameters::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "TimeParametersCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( TimeParametersCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "TimeParametersCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <TimeParameters::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool TimeParameters::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (TimeParametersCmd_Report == (TimeParametersCmd)_data[0])
	{
		uint16 year = (_data[1] << 8) + (_data[2] & 0xFF);
		uint8 month = (_data[3] & 0x0F);
		uint8 day = (_data[4] & 0x1F);
		uint8 hour = (_data[5] & 0x1F);
		uint8 minute = (_data[6] & 0x3F);
		uint8 second = (_data[7] & 0x3F);

		Log::Write( LogLevel_Info, GetNodeId(), "Received TimeParameters report: %02d/%02d/%04d %02d:%02d:%02d", (int)day, (int)month, (int)year, (int)hour, (int)minute, (int)second);
		if( ValueString* value = static_cast<ValueString*>( GetValue( _instance, TimeParametersIndex_Date ) ) )
		{
			char msg[512];
			snprintf(msg, sizeof(msg), "%02d/%02d/%04d", (int)day, (int)month, (int)year);
			value->OnValueRefreshed( msg );
			value->Release();
		}
		if( ValueString* value = static_cast<ValueString*>( GetValue( _instance, TimeParametersIndex_Time ) ) )
		{
			char msg[512];
			snprintf(msg, sizeof(msg), "%02d:%02d:%02d", (int)hour, (int)minute, (int)second);
			value->OnValueRefreshed( msg );
			value->Release();
		}
		ClearStaticRequest( StaticRequest_Values );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <TimeParameters::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
bool TimeParameters::SetValue
(
	Value const& _value
)
{
	bool ret = false;

	uint8 instance = _value.GetID().GetInstance();

	if ( (ValueID::ValueType_Button == _value.GetID().GetType()) && (_value.GetID().GetIndex() == TimeParametersIndex_Set) )
	{
		time_t rawtime;
		struct tm *timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		Msg* msg = new Msg( "TimeParametersCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, instance );
		msg->Append( GetNodeId() );
		msg->Append( 9 );
		msg->Append( GetCommandClassId() );
		msg->Append( TimeParametersCmd_Set );
		/* Year 1 */
		msg->Append( ((timeinfo->tm_year + 1900)>> 8) & 0xFF);
		/* Year 2 */
		msg->Append( ((timeinfo->tm_year + 1900) & 0xFF));
		/* Month */
		msg->Append( (timeinfo->tm_mon & 0x0F)+1);
		/* Day */
		msg->Append( (timeinfo->tm_mday & 0x1F));
		/* Hour */
		msg->Append( (timeinfo->tm_hour & 0x1F));
		/* Minute */
		msg->Append( (timeinfo->tm_min & 0x3F));
		/* Second */
		msg->Append( (timeinfo->tm_sec & 0x3F));
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );


		/* Refresh after we send updated date/time */
		SetStaticRequest( StaticRequest_Values );
		ret = RequestValue( RequestFlag_Static, 0, instance, Driver::MsgQueue_Query );
	}
	if ( (ValueID::ValueType_Button == _value.GetID().GetType()) && (_value.GetID().GetIndex() == TimeParametersIndex_Refresh) )
	{
		SetStaticRequest( StaticRequest_Values );
		ret = RequestValue( RequestFlag_Static, 0, instance, Driver::MsgQueue_Query );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// <TimeParameters::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void TimeParameters::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueString( ValueID::ValueGenre_System, GetCommandClassId(), _instance, TimeParametersIndex_Date, "Date", "", true, false, "", 0 );
		node->CreateValueString( ValueID::ValueGenre_System, GetCommandClassId(), _instance, TimeParametersIndex_Time, "Time", "", true, false, "", 0 );
		node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, TimeParametersIndex_Set, "Set Date/Time", 0);
		node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, TimeParametersIndex_Refresh, "Refresh Date/Time", 0);

	}
}
