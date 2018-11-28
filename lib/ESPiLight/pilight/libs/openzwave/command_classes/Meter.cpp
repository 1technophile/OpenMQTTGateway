//-----------------------------------------------------------------------------
//
//	Meter.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_METER
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
#include "Meter.h"
#include "MultiInstance.h"
#include "../Defs.h"
#include "../Bitfield.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueDecimal.h"
#include "../value_classes/ValueList.h"
#include "../value_classes/ValueButton.h"
#include "../value_classes/ValueInt.h"
#include "../value_classes/ValueBool.h"

using namespace OpenZWave;

enum MeterCmd
{
	MeterCmd_Get				= 0x01,
	MeterCmd_Report				= 0x02,
	// Version 2
	MeterCmd_SupportedGet			= 0x03,
	MeterCmd_SupportedReport		= 0x04,
	MeterCmd_Reset				= 0x05
};

enum MeterType
{
	MeterType_Electric = 1,
	MeterType_Gas,
	MeterType_Water
};

enum
{
	MeterIndex_Exporting = 32,
	MeterIndex_Reset
};


static char const* c_meterTypes[] =
{
	"Unknown",
	"Electric",
	"Gas",
	"Water"
};

static char const* c_electricityUnits[] =
{
	"kWh",
	"kVAh",
	"W",
	"pulses",
	"V",
	"A",
	"Power Factor",
	""
};

static char const* c_gasUnits[] =
{
	"cubic meters",
	"cubic feet",
	"",
	"pulses",
	"",
	"",
	"",
	""
};

static char const* c_waterUnits[] =
{
	"cubic meters",
	"cubic feet",
	"US gallons",
	"pulses",
	"",
	"",
	"",
	""
};

static char const* c_electricityLabels[] =
{
	"Energy",
	"Energy",
	"Power",
	"Count",
	"Voltage",
	"Current",
	"Power Factor",
	"Unknown"
};

//-----------------------------------------------------------------------------
// <Meter::Meter>
// Constructor
//-----------------------------------------------------------------------------
Meter::Meter
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId )
{
	SetStaticRequest( StaticRequest_Values );
}

//-----------------------------------------------------------------------------
// <Meter::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Meter::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool res = false;
	if( GetVersion() > 1 )
	{
		if( _requestFlags & RequestFlag_Static )
		{
 			Msg* msg = new Msg( "MeterCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( MeterCmd_SupportedGet );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			res = true;
		}
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		res |= RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Meter::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Meter::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool res = false;
	if ( !IsGetSupported())
	{
		Log::Write(  LogLevel_Info, GetNodeId(), "MeterCmd_Get Not Supported on this node");
		return false;
	}
	for( uint8 i=0; i<8; ++i )
	{
		uint8 baseIndex = i<<2;

		Value* value = GetValue( _instance, baseIndex );
		if( value != NULL )
		{
			value->Release();
			Msg* msg = new Msg( "MeterCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( MeterCmd_Get );
			msg->Append( (uint8)( i << 3 ) );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			res |= true;
		}
	}
	return res;
}

//-----------------------------------------------------------------------------
// <Meter::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Meter::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	bool handled = false;
	if (MeterCmd_SupportedReport == (MeterCmd)_data[0])
	{
		handled = HandleSupportedReport( _data, _length, _instance );
	}
	else if (MeterCmd_Report == (MeterCmd)_data[0])
	{
		handled = HandleReport( _data, _length, _instance );
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <Meter::HandleSupportedReport>
// Create the values for this command class based on the reported parameters
//-----------------------------------------------------------------------------
bool Meter::HandleSupportedReport
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance
)
{
	bool canReset = ((_data[1] & 0x80) != 0);
	int8 meterType = (MeterType)(_data[1] & 0x1f);
	if (meterType > 4) /* size of c_meterTypes */
	{
		Log::Write (LogLevel_Warning, GetNodeId(), "meterType Value was greater than range. Dropping Message");
		return false;
	}


	ClearStaticRequest( StaticRequest_Version );
	if( Node* node = GetNodeUnsafe() )
	{
		string msg;
		msg = c_meterTypes[meterType];
		msg += ": ";
		// Create the list of supported scales
		uint8 scaleSupported = _data[2];
		if( GetVersion() == 2 )
		{
			// Only four scales are allowed in version 2
			scaleSupported &= 0x0f;
		}

		for( uint8 i=0; i<8; ++i )
		{
			if( scaleSupported & (1<<i) )
			{
				uint8 baseIndex = i<<2;		// We leave space between the value indices to insert previous and time delta values if we need to later on.
				switch( meterType )
				{
					case MeterType_Electric:
					{
						if( ValueDecimal* value = static_cast<ValueDecimal*>( GetValue( _instance, baseIndex ) ) )
						{
							value->SetLabel( c_electricityLabels[i] );
							value->SetUnits( c_electricityUnits[i] );
							value->Release();
						}
						else
						{
							node->CreateValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, baseIndex, c_electricityLabels[i], c_electricityUnits[i], true, false, "0.0", 0 );
						}
						if( i != 0 )
							msg += ", ";
						msg += c_electricityUnits[i];
						break;
					}
					case MeterType_Gas:
					{
						if( ValueDecimal* value = static_cast<ValueDecimal*>( GetValue( _instance, baseIndex ) ) )
						{
							value->SetLabel( c_meterTypes[MeterType_Gas] );
							value->SetUnits( c_gasUnits[i] );
							value->Release();
						}
						else
						{
							node->CreateValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, baseIndex, c_meterTypes[meterType], c_gasUnits[i], true, false, "0.0", 0 );
						}
						if( i != 0 )
							msg += ", ";
						msg += c_gasUnits[i];
						break;
					}
					case MeterType_Water:
					{
						if( ValueDecimal* value = static_cast<ValueDecimal*>( GetValue( _instance, baseIndex ) ) )
						{
							value->SetLabel( c_meterTypes[MeterType_Water] );
							value->SetUnits( c_waterUnits[i] );
							value->Release();
						}
						else
						{
							node->CreateValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, baseIndex, c_meterTypes[meterType], c_waterUnits[i], true, false, "0.0", 0 );
						}
						if( i != 0 )
							msg += ", ";
						msg += c_waterUnits[i];
						break;
					}
					default:
					{
						break;
					}
				}
			}
		}

		// Create the export flag
		node->CreateValueBool( ValueID::ValueGenre_User, GetCommandClassId(), _instance, MeterIndex_Exporting, "Exporting", "", true, false, false, 0 );

		// Create the reset button
		if( canReset )
		{
			node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, MeterIndex_Reset, "Reset", 0 );
		}

		Log::Write( LogLevel_Info, GetNodeId(), "Received Meter supported report from node %d, %s", GetNodeId(), msg.c_str() );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Meter::HandleReport>
// Read the reported meter value
//-----------------------------------------------------------------------------
bool Meter::HandleReport
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance
)
{
	// Import or Export (only valid in version > 1)
	bool exporting = false;
	if( GetVersion() > 1 )
	{
		exporting = ((_data[1] & 0x60) == 0x40 );
		if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, MeterIndex_Exporting ) ) )
		{
			value->OnValueRefreshed( exporting );
			value->Release();
		}
	}

	// Get the value and scale
	uint8 scale;
	uint8 precision = 0;
	string valueStr = ExtractValue( &_data[2], &scale, &precision );

	if (scale > 7) /* size of c_electricityLabels, c_electricityUnits, c_gasUnits, c_waterUnits */
	{
		Log::Write (LogLevel_Warning, GetNodeId(), "Scale was greater than range. Setting to Invalid");
		scale = 7;
	}


	if( GetVersion() == 1 )
	{
		// In version 1, we don't know the scale until we get the first value report
		string label;
		string units;
		int8 meterType = (MeterType)(_data[1] & 0x1f);
		if (meterType > 4) /* size of c_meterTypes */
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "meterType Value was greater than range. Dropping Message");
			return false;
		}

		switch( (MeterType)(_data[1] & 0x1f) )
		{
			case MeterType_Electric:
			{
				// Electricity Meter
				label = c_electricityLabels[scale];
				units = c_electricityUnits[scale];
				break;
			}
			case MeterType_Gas:
			{
				// Gas Meter
				label = c_meterTypes[MeterType_Gas];
				units = c_gasUnits[scale];
				break;
			}
			case MeterType_Water:
			{
				// Water Meter
				label = c_meterTypes[MeterType_Water];
				units = c_waterUnits[scale];
				break;
			}
			default:
			{
				break;
			}
		}

		if( ValueDecimal* value = static_cast<ValueDecimal*>( GetValue( _instance, 0 ) ) )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Meter report from node %d: %s=%s%s", GetNodeId(), label.c_str(), valueStr.c_str(), units.c_str() );
			value->SetLabel( label );
			value->SetUnits( units );
			value->OnValueRefreshed( valueStr );
			if( value->GetPrecision() != precision )
			{
				value->SetPrecision( precision );
			}
			value->Release();
		}
	}
	else
	{
		// Version 2 and above
		uint8 baseIndex = scale << 2;
		if( GetVersion() > 2 )
		{
			// In version 3, an extra scale bit is stored in the meter type byte.
			scale |= ((_data[1]&0x80)>>5);
			baseIndex = scale << 2;
		}

		if( ValueDecimal* value = static_cast<ValueDecimal*>( GetValue( _instance, baseIndex ) ) )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Meter report from node %d: %s%s=%s%s", GetNodeId(), exporting ? "Exporting ": "", value->GetLabel().c_str(), valueStr.c_str(), value->GetUnits().c_str() );
			value->OnValueRefreshed( valueStr );
			if( value->GetPrecision() != precision )
			{
				value->SetPrecision( precision );
			}
			value->Release();

			// Read any previous value and time delta
			uint8 size = _data[2] & 0x07;
			uint16 delta = (uint16)( (_data[3+size]<<8) | _data[4+size]);

			if( delta )
			{
				// There is only a previous value if the time delta is non-zero
				ValueDecimal* previous = static_cast<ValueDecimal*>( GetValue( _instance, baseIndex+1 ) );
				if( NULL == previous )
				{
					// We need to create a value to hold the previous
					if( Node* node = GetNodeUnsafe() )
					{
						node->CreateValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, baseIndex+1, "Previous Reading", value->GetUnits().c_str(), true, false, "0.0", 0 );
						previous = static_cast<ValueDecimal*>( GetValue( _instance, baseIndex+1 ) );
					}
				}
				if( previous )
				{
					precision = 0;
					valueStr = ExtractValue( &_data[2], &scale, &precision, 3+size );
					Log::Write( LogLevel_Info, GetNodeId(), "    Previous value was %s%s, received %d seconds ago.", valueStr.c_str(), previous->GetUnits().c_str(), delta );
					previous->OnValueRefreshed( valueStr );
					if( previous->GetPrecision() != precision )
					{
						previous->SetPrecision( precision );
					}
					previous->Release();
				}

				// Time delta
				ValueInt* interval = static_cast<ValueInt*>( GetValue( _instance, baseIndex+2 ) );
				if( NULL == interval )
				{
					// We need to create a value to hold the time delta
					if( Node* node = GetNodeUnsafe() )
					{
						node->CreateValueInt( ValueID::ValueGenre_User, GetCommandClassId(), _instance, baseIndex+2, "Interval", "seconds", true, false, 0, 0 );
						interval = static_cast<ValueInt*>( GetValue( _instance, baseIndex+2 ) );
					}
				}
				if( interval )
				{
		 			interval->OnValueRefreshed( (int32)delta );
					interval->Release();
				}
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Meter::SetValue>
// Set the device's scale, or reset its accumulated values.
//-----------------------------------------------------------------------------
bool Meter::SetValue
(
	Value const& _value
)
{
	if( MeterIndex_Reset == _value.GetID().GetIndex() )
	{
		ValueButton const* button = static_cast<ValueButton const*>(&_value);
		if( button->IsPressed() )
		{
			Msg* msg = new Msg( "MeterCmd_Reset", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
			msg->SetInstance( this, _value.GetID().GetInstance() );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( MeterCmd_Reset );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Meter::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Meter::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueDecimal( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Unknown", "", true, false, "0.0", 0 );
	}
}
