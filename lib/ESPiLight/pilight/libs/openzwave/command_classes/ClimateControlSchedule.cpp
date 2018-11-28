//-----------------------------------------------------------------------------
//
//	ClimateControlSchedule.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CLIMATE_CONTROL_SCHEDULE
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
#include "ClimateControlSchedule.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../platform/Log.h"
#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueList.h"
#include "../value_classes/ValueSchedule.h"

#include "../tinyxml.h"

using namespace OpenZWave;

enum ClimateControlScheduleCmd
{
	ClimateControlScheduleCmd_Set = 0x01,
	ClimateControlScheduleCmd_Get,
	ClimateControlScheduleCmd_Report,
	ClimateControlScheduleCmd_ChangedGet,
	ClimateControlScheduleCmd_ChangedReport,
	ClimateControlScheduleCmd_OverrideSet,
	ClimateControlScheduleCmd_OverrideGet,
	ClimateControlScheduleCmd_OverrideReport
};

enum
{
	ClimateControlScheduleIndex_OverrideState = 8,
	ClimateControlScheduleIndex_OverrideSetback = 9
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

static char const* c_overrideStateNames[] =
{
	"None",
	"Temporary",
	"Permanent",
	"Invalid"
};


//-----------------------------------------------------------------------------
// <ClimateControlSchedule::ReadXML>
// Read the saved change-counter value
//-----------------------------------------------------------------------------
void ClimateControlSchedule::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );

	int intVal;
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "change_counter", &intVal ) )
	{
		m_changeCounter = (uint8)intVal;
	}
}

//-----------------------------------------------------------------------------
// <ClimateControlSchedule::WriteXML>
// Write the change-counter value
//-----------------------------------------------------------------------------
void ClimateControlSchedule::WriteXML
(
	TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML( _ccElement );

	char str[8];
	snprintf( str, 8, "%d", m_changeCounter );
	_ccElement->SetAttribute( "change_counter", str );
}

//-----------------------------------------------------------------------------
// <ClimateControlSchedule::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool ClimateControlSchedule::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Session )
	{
		// See if the schedule has changed since last time
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <ClimateControlSchedule::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool ClimateControlSchedule::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	// See if the schedule has changed since last time
	Msg* msg = new Msg( "ClimateControlScheduleCmd_ChangedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( ClimateControlScheduleCmd_ChangedGet );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <ClimateControlSchedule::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ClimateControlSchedule::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( ClimateControlScheduleCmd_Report == (ClimateControlScheduleCmd)_data[0] )
	{
		uint8 day = _data[1] & 0x07;

		if (day > 7) /* size of c_dayNames */
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "Day Value was greater than range. Setting to Invalid");
			day = 0;
		}

		Log::Write( LogLevel_Info, GetNodeId(), "Received climate control schedule report for %s", c_dayNames[day] );

		if( ValueSchedule* value = static_cast<ValueSchedule*>( GetValue( _instance, day ) ) )
		{
			// Remove any existing data
			value->ClearSwitchPoints();

			// Parse the switch point data
			for( uint8 i=2; i<29; i+=3 )
			{
				uint8 setback = _data[i+2];
				if( setback == 0x7f )
				{
					// Switch point is unused, so we stop parsing here
					break;
				}

				uint8 hours = _data[i] & 0x1f;
				uint8 minutes = _data[i+1] & 0x3f;

				if( setback == 0x79 )
				{
					Log::Write( LogLevel_Info, GetNodeId(), "  Switch point at %02d:%02d, Frost Protection Mode", hours, minutes, c_dayNames[day] );
				}
				else if( setback == 0x7a )
				{
					Log::Write( LogLevel_Info, GetNodeId(), "  Switch point at %02d:%02d, Energy Saving Mode", hours, minutes, c_dayNames[day] );
				}
				else
				{
					Log::Write( LogLevel_Info, GetNodeId(), "  Switch point at %02d:%02d, Setback %+.1fC", hours, minutes, ((float)setback)*0.1f );
				}

				value->SetSwitchPoint( hours, minutes, setback );
			}

			if( !value->GetNumSwitchPoints() )
			{
				Log::Write( LogLevel_Info, GetNodeId(), "  No Switch points have been set" );
			}

			// Notify the user
			value->OnValueRefreshed();
			value->Release();
		}

		return true;
	}

	if( ClimateControlScheduleCmd_ChangedReport == (ClimateControlScheduleCmd)_data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received climate control schedule changed report:" );

		if( _data[1] )
		{
			if( _data[1] != m_changeCounter )
			{
				m_changeCounter = _data[1];

				// The schedule has changed and is not in override mode, so request reports for each day
				for( int i=1; i<=7; ++i )
				{
					char str[64];
					snprintf( str, 64, "Get climate control schedule for %s", c_dayNames[i] );

					Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
					msg->Append( GetNodeId() );
					msg->Append( 3 );
					msg->Append( GetCommandClassId() );
					msg->Append( ClimateControlScheduleCmd_Get );
					msg->Append( i );
					msg->Append( GetDriver()->GetTransmitOptions() );
					GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
				}
			}
		}
		else
		{
			// Device is in override mode, so we request details of that instead
			Msg* msg = new Msg( "Get climate control schedule override state", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( ClimateControlScheduleCmd_OverrideGet );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		}
		return true;
	}

	if( ClimateControlScheduleCmd_OverrideReport == (ClimateControlScheduleCmd)_data[0] )
	{
		uint8 overrideState = _data[1] & 0x03;
		if (overrideState > 3) /* size of c_overrideStateNames */
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "overrideState Value was greater than range. Setting to Invalid");
			overrideState = 3;
		}

		Log::Write( LogLevel_Info, GetNodeId(), "Received climate control schedule override report:" );
		Log::Write( LogLevel_Info, GetNodeId(), "  Override State: %s:", c_overrideStateNames[overrideState] );

		if( ValueList* valueList = static_cast<ValueList*>( GetValue( _instance, ClimateControlScheduleIndex_OverrideState ) ) )
		{
			valueList->OnValueRefreshed( (int)overrideState );
			valueList->Release();
		}

		uint8 setback = _data[2];
		if( overrideState )
		{
			if( setback == 0x79 )
			{
				Log::Write( LogLevel_Info, GetNodeId(), "  Override Setback: Frost Protection Mode" );
			}
			else if( setback == 0x7a )
			{
				Log::Write( LogLevel_Info, GetNodeId(), "  Override Setback: Energy Saving Mode" );
			}
			else
			{
				Log::Write( LogLevel_Info, GetNodeId(), "  Override Setback: %+.1fC", ((float)setback)*0.1f );
			}
		}

		if( ValueByte* valueByte = static_cast<ValueByte*>( GetValue( _instance, ClimateControlScheduleIndex_OverrideSetback ) ) )
		{
			valueByte->OnValueRefreshed( setback );
			valueByte->Release();
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <ClimateControlSchedule::SetValue>
// Set a value in the device
//-----------------------------------------------------------------------------
bool ClimateControlSchedule::SetValue
(
	Value const& _value
)
{
//	bool res = false;
	uint8 instance = _value.GetID().GetInstance();
	uint8 idx = _value.GetID().GetIndex();

	if( idx < 8 )
	{
		// Set a schedule
		ValueSchedule const* value = static_cast<ValueSchedule const*>(&_value);

		char str[64];
		snprintf( str, 64, "Set the climate control schedule for %s on node %d", c_dayNames[idx], GetNodeId() );

		Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, instance );
		msg->Append( GetNodeId() );
		msg->Append( 30 );
		msg->Append( GetCommandClassId() );
		msg->Append( ClimateControlScheduleCmd_Set );
		msg->Append( idx );	// Day of week

		for( uint8 i=0; i<9; ++i )
		{
			uint8 hours;
			uint8 minutes;
			int8 setback;
			if( value->GetSwitchPoint( i, &hours, &minutes, &setback ) )
			{
				msg->Append( hours );
				msg->Append( minutes );
				msg->Append( setback );
			}
			else
			{
				// Unused switch point
				msg->Append( 0 );
				msg->Append( 0 );
				msg->Append( 0x7f );
			}
		}

		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	}
	else
	{
		// Set an override
		ValueList* state = static_cast<ValueList*>( GetValue( instance, ClimateControlScheduleIndex_OverrideState ) );
		ValueByte* setback = static_cast<ValueByte*>( GetValue( instance, ClimateControlScheduleIndex_OverrideSetback ) );

		if( state && setback )
		{
			ValueList::Item const& item = state->GetItem();

			Msg* msg = new Msg( "Set climate control schedule override state", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, instance );
			msg->Append( GetNodeId() );
			msg->Append( 4 );
			msg->Append( GetCommandClassId() );
			msg->Append( ClimateControlScheduleCmd_OverrideSet );
			msg->Append( (uint8)item.m_value );
			msg->Append( setback->GetValue() );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <ClimateControlSchedule::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void ClimateControlSchedule::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		// Add a ValueSchedule for each day of the week.
		for( uint8 i=0; i<7; ++i )
		{
		  	node->CreateValueSchedule( ValueID::ValueGenre_User, GetCommandClassId(), _instance, i+1, c_dayNames[i+1], "", false, false, 0 );
		}

		// Add values for the override state and setback
		vector<ValueList::Item> items;

		ValueList::Item item;
		for( uint8 i=0; i<3; ++i )
		{
			item.m_label = c_overrideStateNames[i];
			item.m_value = i;
			items.push_back( item );
		}

		node->CreateValueList(  ValueID::ValueGenre_User, GetCommandClassId(), _instance, ClimateControlScheduleIndex_OverrideState, "Override State", "", false, false, 1, items, 0, 0 );
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, ClimateControlScheduleIndex_OverrideSetback, "Override Setback", "", false, false, 0, 0  );
	}
}

