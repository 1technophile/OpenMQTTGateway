//-----------------------------------------------------------------------------
//
//	ThermostatMode.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_THERMOSTAT_MODE
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
#include "ThermostatMode.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueList.h"

#include "../tinyxml.h"

using namespace OpenZWave;

enum ThermostatModeCmd
{
	ThermostatModeCmd_Set				= 0x01,
	ThermostatModeCmd_Get				= 0x02,
	ThermostatModeCmd_Report			= 0x03,
	ThermostatModeCmd_SupportedGet		= 0x04,
	ThermostatModeCmd_SupportedReport	= 0x05
};

static char const* c_modeName[] =
{
	"Off",
	"Heat",
	"Cool",
	"Auto",
	"Aux Heat",
	"Resume",
	"Fan Only",
	"Furnace",
	"Dry Air",
	"Moist Air",
	"Auto Changeover",
	"Heat Econ",
	"Cool Econ",
	"Away",
	"Unknown"
};

//-----------------------------------------------------------------------------
// <ThermostatMode::ReadXML>
// Read the supported modes
//-----------------------------------------------------------------------------
void ThermostatMode::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );

	if( GetNodeUnsafe() )
	{
		vector<ValueList::Item>	supportedModes;

		TiXmlElement const* supportedModesElement = _ccElement->FirstChildElement( "SupportedModes" );
		if( supportedModesElement )
		{
			TiXmlElement const* modeElement = supportedModesElement->FirstChildElement();
			while( modeElement )
			{
				char const* str = modeElement->Value();
				if( str && !strcmp( str, "Mode" ) )
				{
					int index;
					if( TIXML_SUCCESS == modeElement->QueryIntAttribute( "index", &index ) )
					{
						if (index > 13) /* size of c_modeName minus Invalid */
						{
							Log::Write (LogLevel_Warning, GetNodeId(), "index Value in XML was greater than range. Setting to Invalid");
							index = 14;
						}
						ValueList::Item item;
						item.m_value = index;
						item.m_label = c_modeName[index];
						supportedModes.push_back( item );
					}
				}

				modeElement = modeElement->NextSiblingElement();
			}
		}

		if( !supportedModes.empty() )
		{
			m_supportedModes = supportedModes;
			ClearStaticRequest( StaticRequest_Values );
			CreateVars( 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// <ThermostatMode::WriteXML>
// Save the supported modes
//-----------------------------------------------------------------------------
void ThermostatMode::WriteXML
(
	TiXmlElement* _ccElement
)
{
	if( m_supportedModes.empty() )
	{
		return;
	}

	CommandClass::WriteXML( _ccElement );

	if( GetNodeUnsafe() )
	{
		TiXmlElement* supportedModesElement = new TiXmlElement( "SupportedModes" );
		_ccElement->LinkEndChild( supportedModesElement );

		for( vector<ValueList::Item>::iterator it = m_supportedModes.begin(); it != m_supportedModes.end(); ++it )
		{
			ValueList::Item const& item = *it;

			TiXmlElement* modeElement = new TiXmlElement( "Mode" );
			supportedModesElement->LinkEndChild( modeElement );

			char str[8];
			snprintf( str, 8, "%d", item.m_value );
			modeElement->SetAttribute( "index", str );
			modeElement->SetAttribute( "label", item.m_label.c_str() );
		}
	}
}

//-----------------------------------------------------------------------------
// <ThermostatMode::RequestState>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
bool ThermostatMode::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		// request supported mode list
		requests |= RequestValue( _requestFlags, ThermostatModeCmd_SupportedGet, _instance, _queue );
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		// Request the current mode
		requests |= RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return requests;
}

//-----------------------------------------------------------------------------
// <ThermostatMode::RequestValue>
// Get the static thermostat mode details from the device
//-----------------------------------------------------------------------------
bool ThermostatMode::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _getTypeEnum,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _getTypeEnum == ThermostatModeCmd_SupportedGet )
	{
		// Request the supported modes
		Msg* msg = new Msg( "ThermostatModeCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatModeCmd_SupportedGet );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	}

	if( _getTypeEnum == 0 )		// get current mode
	{
		if ( IsGetSupported() )
		{
			// Request the current mode
			Msg* msg = new Msg( "ThermostatModeCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( ThermostatModeCmd_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		} else {
			Log::Write(  LogLevel_Info, GetNodeId(), "ThermostatModeCmd_Get Not Supported on this node");

		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <ThermostatMode::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ThermostatMode::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( ThermostatModeCmd_Report == (ThermostatModeCmd)_data[0] )
	{
		uint8 mode = _data[1]&0x1f;
		bool validMode = false;
		for (vector<ValueList::Item>::iterator it = m_supportedModes.begin(); it != m_supportedModes.end(); ++it )
		{
                	ValueList::Item const& item = *it;

                	if (item.m_value == mode) {
				validMode = true;
				break;
			}
		}

		if( validMode )
		{
			// We have received the thermostat mode from the Z-Wave device
			if( ValueList* valueList = static_cast<ValueList*>( GetValue( _instance, 0 ) ) )
			{
				valueList->OnValueRefreshed( mode );
				Log::Write( LogLevel_Info, GetNodeId(), "Received thermostat mode: %s", valueList->GetItem().m_label.c_str() );
				valueList->Release();
			}
			else
			{
				Log::Write( LogLevel_Info, GetNodeId(), "Received thermostat mode: index %d", mode );
			}
		}
		else
		{
			 Log::Write( LogLevel_Info, GetNodeId(), "Received unknown thermostat mode: index %d", mode );
		}
		return true;
	}

	if( ThermostatModeCmd_SupportedReport == (ThermostatModeCmd)_data[0] )
	{
		// We have received the supported thermostat modes from the Z-Wave device
		// these values are used to populate m_supportedModes which, in turn, is used to "seed" the values
		// for each m_modes instance
		Log::Write( LogLevel_Info, GetNodeId(), "Received supported thermostat modes" );

		m_supportedModes.clear();
		for( uint32 i=1; i<_length-1; ++i )
		{
			for( int32 bit=0; bit<8; ++bit )
			{
				if( ( _data[i] & (1<<bit) ) != 0 )
				{
					ValueList::Item item;
					item.m_value = (int32)((i-1)<<3) + bit;
					/* minus 1 in the sizeof calc here, as the Unknown entry is our addition */
					if ((size_t)item.m_value >= (sizeof(c_modeName)/sizeof(*c_modeName) -1))
					{
						Log::Write( LogLevel_Info, GetNodeId(), "Received unknown thermostat mode: 0x%x", item.m_value);
					}
					else
					{
						item.m_label = c_modeName[item.m_value];
						m_supportedModes.push_back( item );

						Log::Write( LogLevel_Info, GetNodeId(), "    Added mode: %s", c_modeName[item.m_value] );
					}
				}
			}
		}

		ClearStaticRequest( StaticRequest_Values );
		CreateVars( _instance );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <ThermostatMode::SetValue>
// Set the device's thermostat mode
//-----------------------------------------------------------------------------
bool ThermostatMode::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_List == _value.GetID().GetType() )
	{
		ValueList const* value = static_cast<ValueList const*>(&_value);
		uint8 state = (uint8)value->GetItem().m_value;

		Msg* msg = new Msg( "ThermostatModeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( ThermostatModeCmd_Set );
		msg->Append( state );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <ThermostatMode::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void ThermostatMode::CreateVars
(
	uint8 const _instance
)
{
	// There are three ways to get here...each needs to be handled differently:
	//	QueryStage_ProtocolInfo:
	//		Don't know what's supported yet, so do nothing
	//	QueryStage_NodeInfo:
	//		Need to create the instance so the values can be read from the xml file
	//	QueryStage_Static:
	//		Need to create the instance (processing SupportedReport) if it doesn't exist
	//		If it does, populate with the appropriate values

	if( Node* node = GetNodeUnsafe() )
	{
		Node::QueryStage qs = node->GetCurrentQueryStage();
		if( qs == Node::QueryStage_ProtocolInfo )
		{
			// this call is from QueryStage_ProtocolInfo,
			// so just return (don't know which modes are supported yet)
			return;
		}

		// identify the lowest supported mode as the "default" (or default to 0 if no supported modes identified yet)
		int32 defaultValue = 0;
		if( !m_supportedModes.empty() )
		{
			defaultValue = m_supportedModes[0].m_value;
		}

		if( qs == Node::QueryStage_Static )
		{
			// This instance might already have been created (in NodeInfo, in preparation for loading the values
			// from zwcfg xml file).  So, if the instance already exists, we delete its value and add a new one below
			if( ValueList* valueList = static_cast<ValueList*>( GetValue( _instance, 0 ) ) )
			{
				node->RemoveValueList( valueList );
				valueList->Release();
			}
		}

		node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Mode", "", false, false, 1, m_supportedModes, defaultValue, 0 );
	}
}
