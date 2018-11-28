//-----------------------------------------------------------------------------
//
//	SensorBinary.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_BINARY
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
#include "SensorBinary.h"
#include "WakeUp.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"
#include "../value_classes/ValueBool.h"
#include "../tinyxml.h"

using namespace OpenZWave;

enum SensorBinaryCmd
{
	SensorBinaryCmd_Get		= 0x02,
	SensorBinaryCmd_Report		= 0x03
};

//-----------------------------------------------------------------------------
// <SensorBinary::ReadXML>
// Read node configuration data
//-----------------------------------------------------------------------------
void SensorBinary::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );

	TiXmlElement const* child = _ccElement->FirstChildElement();

	char const* str; int index; int type;

	while( child )
	{
		str = child->Value();

		if( str )
		{
			if( !strcmp( str, "SensorMap" ) )
			{
				if( TIXML_SUCCESS == child->QueryIntAttribute( "index", &index ) &&
					TIXML_SUCCESS == child->QueryIntAttribute( "type", &type ) )
				{
					m_sensorsMap[(uint8)type] = (uint8)index;
				}
			}
		}

		child = child->NextSiblingElement();
	}
}
//-----------------------------------------------------------------------------
// <SensorBinary::WriteXML>
// Write node configuration data
//-----------------------------------------------------------------------------
void SensorBinary::WriteXML
(
	TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML( _ccElement );

	char str[8];

	for( map<uint8,uint8>::iterator it = m_sensorsMap.begin(); it != m_sensorsMap.end(); it++ )
	{
		TiXmlElement* sensorMapElement = new TiXmlElement( "SensorMap" );
		_ccElement->LinkEndChild( sensorMapElement );

		snprintf( str, 8, "%d", it->second );
		sensorMapElement->SetAttribute( "index", str );

		snprintf( str, 8, "%d", it->first );
		sensorMapElement->SetAttribute( "type", str );
	}
}
//-----------------------------------------------------------------------------
// <SensorBinary::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool SensorBinary::RequestState
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
// <SensorBinary::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool SensorBinary::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "SensorBinaryCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SensorBinaryCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "SensorBinaryCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SensorBinary::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SensorBinary::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (SensorBinaryCmd_Report == (SensorBinaryCmd)_data[0])
	{
	    if( _length > 2 )
	    {
	        uint8 index = m_sensorsMap[_data[2]];

            Log::Write( LogLevel_Info, GetNodeId(), "Received SensorBinary report: Sensor:%d State=%s", _data[2], _data[1] ? "On" : "Off" );

            if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, index ) ) )
            {
                value->OnValueRefreshed( _data[1] != 0 );
                value->Release();
            }

            return true;
	    }
	    else
	    {
            Log::Write( LogLevel_Info, GetNodeId(), "Received SensorBinary report: State=%s", _data[1] ? "On" : "Off" );

            if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, 0 ) ) )
            {
                value->OnValueRefreshed( _data[1] != 0 );
                value->Release();
            }

            return true;
	    }
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SensorBinary::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
void SensorBinary::SetValueBasic
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
				if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, 0 ) ) )
				{
					value->OnValueRefreshed( _value != 0 );
					value->Release();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <SensorBinary::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SensorBinary::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueBool(  ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "Sensor", "", true, false, false, 0 );
	}
}
