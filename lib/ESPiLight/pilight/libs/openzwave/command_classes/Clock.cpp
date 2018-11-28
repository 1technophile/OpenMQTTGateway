//-----------------------------------------------------------------------------
//
//	Clock.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CLOCK
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
#include "Clock.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueList.h"

using namespace OpenZWave;

enum ClockCmd
{
	ClockCmd_Set	= 0x04,
	ClockCmd_Get	= 0x05,
	ClockCmd_Report	= 0x06
};

enum
{
	ClockIndex_Day = 0,
	ClockIndex_Hour,
	ClockIndex_Minute
};

static char const* c_dayNames[] =
{
	"Invalid",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday"
};

//-----------------------------------------------------------------------------
// <Clock::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Clock::RequestState
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
// <Clock::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Clock::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "ClockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ClockCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "ClockCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Clock::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Clock::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (ClockCmd_Report == (ClockCmd)_data[0])
	{
		uint8 day = _data[1] >> 5;
		uint8 hour = _data[1] & 0x1f;
		uint8 minute = _data[2];

		if (day > 7) /* size of c_dayNames */
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "Day Value was greater than range. Setting to Invalid");
			day = 0;
		}

		Log::Write( LogLevel_Info, GetNodeId(), "Received Clock report: %s %.2d:%.2d", c_dayNames[day], hour, minute );


		if( ValueList* dayValue = static_cast<ValueList*>( GetValue( _instance, ClockIndex_Day ) ) )
		{
			dayValue->OnValueRefreshed( day );
			dayValue->Release();
		}
		if( ValueByte* hourValue = static_cast<ValueByte*>( GetValue( _instance, ClockIndex_Hour ) ) )
		{
			hourValue->OnValueRefreshed( hour );
			hourValue->Release();
		}
		if( ValueByte* minuteValue = static_cast<ValueByte*>( GetValue( _instance, ClockIndex_Minute ) ) )
		{
			minuteValue->OnValueRefreshed( minute );
			minuteValue->Release();
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Clock::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
bool Clock::SetValue
(
	Value const& _value
)
{
	bool ret = false;

	uint8 instance = _value.GetID().GetInstance();

	ValueList* dayValue = static_cast<ValueList*>( GetValue( instance, ClockIndex_Day ) );
	ValueByte* hourValue = static_cast<ValueByte*>( GetValue( instance, ClockIndex_Hour ) );
	ValueByte* minuteValue = static_cast<ValueByte*>( GetValue( instance, ClockIndex_Minute ) );

	if( dayValue && hourValue && minuteValue )
	{
		uint8 day = dayValue->GetItem().m_value;
		uint8 hour = hourValue->GetValue();
		uint8 minute = minuteValue->GetValue();

		Msg* msg = new Msg( "ClockCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, instance );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( ClockCmd_Set );
		msg->Append( ( day << 5 ) | hour );
		msg->Append( minute );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		ret = true;
	}

	if( dayValue != NULL )
	{
		dayValue->Release();
	}
	if( hourValue != NULL )
	{
		hourValue->Release();
	}
	if( minuteValue != NULL )
	{
		minuteValue->Release();
	}
	return ret;
}

//-----------------------------------------------------------------------------
// <Clock::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Clock::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		vector<ValueList::Item> items;
		for( int i=1; i<=7; ++i )
		{
			ValueList::Item item;
			item.m_label = c_dayNames[i];
			item.m_value = i;
			items.push_back( item );
		}

		node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ClockIndex_Day, "Day", "", false, false, 1, items, 0, 0 );
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ClockIndex_Hour, "Hour", "", false, false, 12, 0 );
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ClockIndex_Minute, "Minute", "", false, false, 0, 0 );
	}
}
