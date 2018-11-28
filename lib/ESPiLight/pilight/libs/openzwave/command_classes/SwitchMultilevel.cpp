//-----------------------------------------------------------------------------
//
//	SwitchMultilevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_MULTILEVEL
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
#include "SwitchMultilevel.h"
#include "WakeUp.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../platform/Log.h"

#include "../value_classes/ValueBool.h"
#include "../value_classes/ValueButton.h"
#include "../value_classes/ValueByte.h"

using namespace OpenZWave;

enum SwitchMultilevelCmd
{
	SwitchMultilevelCmd_Set						= 0x01,
	SwitchMultilevelCmd_Get						= 0x02,
	SwitchMultilevelCmd_Report					= 0x03,
	SwitchMultilevelCmd_StartLevelChange				= 0x04,
	SwitchMultilevelCmd_StopLevelChange				= 0x05,
	SwitchMultilevelCmd_SupportedGet				= 0x06,
	SwitchMultilevelCmd_SupportedReport				= 0x07
};

enum
{
	SwitchMultilevelIndex_Level = 0,
	SwitchMultilevelIndex_Bright,
	SwitchMultilevelIndex_Dim,
	SwitchMultilevelIndex_IgnoreStartLevel,
	SwitchMultilevelIndex_StartLevel,
	SwitchMultilevelIndex_Duration,
	SwitchMultilevelIndex_Step,
	SwitchMultilevelIndex_Inc,
	SwitchMultilevelIndex_Dec
};

static uint8 c_directionParams[] =
{
	0x18,
	0x58,
	0xc0,
	0xc8
};

static char const* c_directionDebugLabels[] =
{
	"Up",
	"Down",
	"Inc",
	"Dec"
};

static char const* c_switchLabelsPos[] =
{
	"Undefined",
	"On",
	"Up",
	"Open",
	"Clockwise",
	"Right",
	"Forward",
	"Push"
};

static char const* c_switchLabelsNeg[] =
{
	"Undefined",
	"Off",
	"Down",
	"Close",
	"Counter-Clockwise",
	"Left",
	"Reverse",
	"Pull"
};

//-----------------------------------------------------------------------------
// <SwitchMultilevel::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool SwitchMultilevel::RequestState
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
// <SwitchMultilevel::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool SwitchMultilevel::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _index,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _index == SwitchMultilevelIndex_Level )
	{
		if ( IsGetSupported() )
		{
			Msg* msg = new Msg( "SwitchMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( SwitchMultilevelCmd_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		} else {
			Log::Write(  LogLevel_Info, GetNodeId(), "SwitchMultilevelCmd_Get Not Supported on this node");
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchMultilevel::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( SwitchMultilevelCmd_Report == (SwitchMultilevelCmd)_data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received SwitchMultiLevel report: level=%d", _data[1] );

		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, SwitchMultilevelIndex_Level ) ) )
		{
			value->OnValueRefreshed( _data[1] );
			value->Release();
		}
		return true;
	}

	if( SwitchMultilevelCmd_SupportedReport == (SwitchMultilevelCmd)_data[0] )
	{
		uint8 switchType1 = _data[1] & 0x1f;
		uint8 switchType2 = _data[2] & 0x1f;
		uint8 switchtype1label = switchType1;
		uint8 switchtype2label = switchType2;
		if (switchtype1label > 7) /* size of c_switchLabelsPos, c_switchLabelsNeg */
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "switchtype1label Value was greater than range. Setting to Invalid");
			switchtype1label = 0;
		}
		if (switchtype2label > 7) /* sizeof c_switchLabelsPos, c_switchLabelsNeg */
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "switchtype2label Value was greater than range. Setting to Invalid");
			switchtype2label = 0;
		}


		Log::Write( LogLevel_Info, GetNodeId(), "Received SwitchMultiLevel supported report: Switch1=%s/%s, Switch2=%s/%s", c_switchLabelsPos[switchtype1label], c_switchLabelsNeg[switchtype1label], c_switchLabelsPos[switchtype2label], c_switchLabelsNeg[switchtype2label] );
		ClearStaticRequest( StaticRequest_Version );

		// Set the labels on the values
		ValueButton* button;

		if( switchType1 )
		{
			if( NULL != ( button = static_cast<ValueButton*>( GetValue( _instance, SwitchMultilevelIndex_Bright ) ) ) )
			{
				button->SetLabel( c_switchLabelsPos[switchtype1label] );
				button->Release();
			}
			if( NULL != ( button = static_cast<ValueButton*>( GetValue( _instance, SwitchMultilevelIndex_Dim ) ) ) )
			{
				button->SetLabel( c_switchLabelsNeg[switchtype1label] );
				button->Release();
			}
		}

		if( switchType2 )
		{
			if( NULL != ( button = static_cast<ValueButton*>( GetValue( _instance, SwitchMultilevelIndex_Inc ) ) ) )
			{
				button->SetLabel( c_switchLabelsPos[switchtype2label] );
				button->Release();
			}
			if( NULL != ( button = static_cast<ValueButton*>( GetValue( _instance, SwitchMultilevelIndex_Dec ) ) ) )
			{
				button->SetLabel( c_switchLabelsNeg[switchtype2label] );
				button->Release();
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetVersion>
// Set the command class version
//-----------------------------------------------------------------------------
void SwitchMultilevel::SetVersion
(
	uint8 const _version
)
{
	CommandClass::SetVersion( _version );

	if( _version == 3 )
	{
		// Request the supported switch types
		Msg* msg = new Msg( "SwitchMultilevelCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchMultilevelCmd_SupportedGet );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );

		// Set the request flag again - it will be cleared when we get a
		// response to the SwitchMultilevelCmd_SupportedGet message.
		SetStaticRequest( StaticRequest_Version );
	}
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetValue>
// Set the level on a device
//-----------------------------------------------------------------------------
bool SwitchMultilevel::SetValue
(
	Value const& _value
)
{
	bool res = false;
	uint8 instance = _value.GetID().GetInstance();

	switch( _value.GetID().GetIndex() )
	{
		case SwitchMultilevelIndex_Level:
		{
			// Level
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, SwitchMultilevelIndex_Level ) ) )
			{
				res = SetLevel( instance, (static_cast<ValueByte const*>(&_value))->GetValue() );
				value->Release();
			}
			break;
		}
		case SwitchMultilevelIndex_Bright:
		{
			// Bright
			if( ValueButton* button = static_cast<ValueButton*>( GetValue( instance, SwitchMultilevelIndex_Bright ) ) )
			{
				if( button->IsPressed() )
				{
					res = StartLevelChange( instance, SwitchMultilevelDirection_Up );
				}
				else
				{
					res = StopLevelChange( instance );
				}
				button->Release();
			}
			break;
		}
		case SwitchMultilevelIndex_Dim:
		{
			// Dim
			if( ValueButton* button = static_cast<ValueButton*>( GetValue( instance, SwitchMultilevelIndex_Dim ) ) )
			{
				if( button->IsPressed() )
				{
					res = StartLevelChange( instance, SwitchMultilevelDirection_Down );
				}
				else
				{
					res = StopLevelChange( instance );
				}
				button->Release();
			}
			break;
		}
		case SwitchMultilevelIndex_IgnoreStartLevel:
		{
			if( ValueBool* value = static_cast<ValueBool*>( GetValue( instance, SwitchMultilevelIndex_IgnoreStartLevel ) ) )
			{
				value->OnValueRefreshed( (static_cast<ValueBool const*>( &_value))->GetValue() );
				value->Release();
			}
			res = true;
			break;
		}
		case SwitchMultilevelIndex_StartLevel:
		{
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, SwitchMultilevelIndex_StartLevel ) ) )
			{
				value->OnValueRefreshed( (static_cast<ValueByte const*>( &_value))->GetValue() );
				value->Release();
			}
			res = true;
			break;
		}
		case SwitchMultilevelIndex_Duration:
		{
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, SwitchMultilevelIndex_Duration ) ) )
			{
				value->OnValueRefreshed( (static_cast<ValueByte const*>( &_value))->GetValue() );
				value->Release();
			}
			res = true;
			break;
		}
		case SwitchMultilevelIndex_Step:
		{
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, SwitchMultilevelIndex_Step ) ) )
			{
				value->OnValueRefreshed( (static_cast<ValueByte const*>( &_value))->GetValue() );
				value->Release();
			}
			res = true;
			break;
		}
		case SwitchMultilevelIndex_Inc:
		{
			// Inc
			if( ValueButton* button = static_cast<ValueButton*>( GetValue( instance, SwitchMultilevelIndex_Inc ) ) )
			{
				if( button->IsPressed() )
				{
					res = StartLevelChange( instance, SwitchMultilevelDirection_Inc );
				}
				else
				{
					res = StopLevelChange( instance );
				}
				button->Release();
			}
			break;
		}
		case SwitchMultilevelIndex_Dec:
		{
			// Dec
			if( ValueButton* button = static_cast<ValueButton*>( GetValue( instance, SwitchMultilevelIndex_Dec ) ) )
			{
				if( button->IsPressed() )
				{
					res = StartLevelChange( instance, SwitchMultilevelDirection_Dec );
				}
				else
				{
					res = StopLevelChange( instance );
				}
				button->Release();
			}
			break;
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
void SwitchMultilevel::SetValueBasic
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
				if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, SwitchMultilevelIndex_Level ) ) )
				{
					value->OnValueRefreshed( _value != 0 );
					value->Release();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SetLevel>
// Set a new level for the switch
//-----------------------------------------------------------------------------
bool SwitchMultilevel::SetLevel
(
	uint8 const _instance,
	uint8 const _level
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "SwitchMultilevel::Set - Setting to level %d", _level );
	Msg* msg = new Msg( "SwitchMultiLevel Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );

	if( ValueByte* durationValue = static_cast<ValueByte*>( GetValue( _instance, SwitchMultilevelIndex_Duration ) ) )
	{
		uint8 duration = durationValue->GetValue();
		durationValue->Release();
		if( duration == 0xff )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "  Duration: Default" );
		}
		else if( duration >= 0x80 )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "  Duration: %d minutes", duration - 0x7f );
		}
		else
		{
			Log::Write( LogLevel_Info, GetNodeId(), "  Duration: %d seconds", duration );
		}

		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchMultilevelCmd_Set );
		msg->Append( _level );
		msg->Append( duration );
	}
	else
	{
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchMultilevelCmd_Set );
		msg->Append( _level );
	}

	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	return true;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::SwitchMultilevelCmd_StartLevelChange>
// Start the level changing
//-----------------------------------------------------------------------------
bool SwitchMultilevel::StartLevelChange
(
	uint8 const _instance,
	SwitchMultilevelDirection const _direction
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "SwitchMultilevel::StartLevelChange - Starting a level change" );

	uint8 length = 4;
	if (_direction > 3) /* size of  c_directionParams, c_directionDebugLabels */
	{
		Log::Write (LogLevel_Warning, GetNodeId(), "_direction Value was greater than range. Dropping");
		return false;
	}
	uint8 direction = c_directionParams[_direction];
	Log::Write( LogLevel_Info, GetNodeId(), "  Direction:          %s", c_directionDebugLabels[_direction] );

	if( ValueBool* ignoreStartLevel = static_cast<ValueBool*>( GetValue( _instance, SwitchMultilevelIndex_IgnoreStartLevel ) ) )
	{
		if( ignoreStartLevel->GetValue() )
		{
			ignoreStartLevel->Release();
			// Set the ignore start level flag
			direction |= 0x20;
		}
	}
	Log::Write( LogLevel_Info, GetNodeId(), "  Ignore Start Level: %s", (direction & 0x20) ? "True" : "False" );

	uint8 startLevel = 0;
	if( ValueByte* startLevelValue = static_cast<ValueByte*>( GetValue( _instance, SwitchMultilevelIndex_StartLevel ) ) )
	{
		startLevel = startLevelValue->GetValue();
		startLevelValue->Release();
	}
	Log::Write( LogLevel_Info, GetNodeId(), "  Start Level:        %d", startLevel );

	uint8 duration = 0;
	if( ValueByte* durationValue = static_cast<ValueByte*>( GetValue( _instance, SwitchMultilevelIndex_Duration ) ) )
	{
		length = 5;
		duration = durationValue->GetValue();
		durationValue->Release();
		Log::Write( LogLevel_Info, GetNodeId(), "  Duration:           %d", duration );
	}

	uint8 step = 0;
	if( ( SwitchMultilevelDirection_Inc == _direction ) || ( SwitchMultilevelDirection_Dec == _direction ) )
	{
		if( ValueByte* stepValue = static_cast<ValueByte*>( GetValue( _instance, SwitchMultilevelIndex_Step ) ) )
		{
			length = 6;
			step = stepValue->GetValue();
			stepValue->Release();
			Log::Write( LogLevel_Info, GetNodeId(), "  Step Size:          %d", step );
		}
	}

	Msg* msg = new Msg( "SwitchMultilevel StartLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
	msg->Append( length );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchMultilevelCmd_StartLevelChange );
	msg->Append( direction );
	msg->Append( startLevel );

	if( length >= 5 )
	{
		msg->Append( duration );
	}

	if( length == 6 )
	{
		msg->Append( step );
	}

	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	return true;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::StopLevelChange>
// Stop the level changing
//-----------------------------------------------------------------------------
bool SwitchMultilevel::StopLevelChange
(
	uint8 const _instance
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "SwitchMultilevel::StopLevelChange - Stopping the level change" );
	Msg* msg = new Msg( "SwitchMultilevel StopLevelChange", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SwitchMultilevelCmd_StopLevelChange );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	return true;
}

//-----------------------------------------------------------------------------
// <SwitchMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchMultilevel::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		switch( GetVersion() )
		{
			case 3:
			{
			  	node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, SwitchMultilevelIndex_Step, "Step Size", "", false, false, 0, 0 );
				node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, SwitchMultilevelIndex_Inc, "Inc", 0 );
				node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, SwitchMultilevelIndex_Dec, "Dec", 0 );
				// Fall through to version 2
			}
			case 2:
			{
			  	node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, SwitchMultilevelIndex_Duration, "Dimming Duration", "", false, false, 0xff, 0 );
				// Fall through to version 1
			}
			case 1:
			{
			  	node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, SwitchMultilevelIndex_Level, "Level", "", false, false, 0, 0 );
				node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, SwitchMultilevelIndex_Bright, "Bright", 0 );
				node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, SwitchMultilevelIndex_Dim, "Dim", 0 );
				node->CreateValueBool( ValueID::ValueGenre_System, GetCommandClassId(), _instance, SwitchMultilevelIndex_IgnoreStartLevel, "Ignore Start Level", "", false, false, true, 0 );
				node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, SwitchMultilevelIndex_StartLevel, "Start Level", "", false, false, 0, 0 );
				break;
			}
		}
	}
}



