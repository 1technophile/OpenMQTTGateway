//-----------------------------------------------------------------------------
//
//	MultiInstance.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_MULTI_INSTANCE
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

#include "../tinyxml.h"
#include "CommandClasses.h"
#include "Basic.h"
#include "MultiInstance.h"
#include "NoOperation.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../platform/Log.h"

using namespace OpenZWave;

// Reduced set of Generic Device classes sorted to reduce
// the likely number of calls to MultiChannelCmd_EndPointFind.
uint8 const c_genericClass[] =
{
		0x21,		// Multilevel Sensor
		0x20,		// Binary Sensor
		0x31,		// Meter
		0x08,		// Thermostat
		0x11,		// Multilevel Switch
		0x10,		// Binary Switch
		0x12,		// Remote Switch
		0xa1,		// Alarm Sensor
		0x16,		// Ventilation
		0x30,		// Pulse Meter
		0x40,		// Entry Control
		0x13,		// Toggle Switch
		0x03,		// AV Control Point
		0x04,		// Display
		0x00		// End of list
};

char const* c_genericClassName[] =
{
		"Multilevel Sensor",
		"Binary Sensor",
		"Meter",
		"Thermostat",
		"Multilevel Switch",
		"Binary Switch",
		"Remote Switch",
		"Alarm Sensor",
		"Ventilation",
		"Pulse Meter",
		"Entry Control",
		"Toggle Switch",
		"AV Control Point",
		"Display",
		"Unknown"
};

//-----------------------------------------------------------------------------
// <MultiInstance::MultiInstance>
// Constructor
//-----------------------------------------------------------------------------
MultiInstance::MultiInstance
(
		uint32 const _homeId,
		uint8 const _nodeId
):
CommandClass( _homeId, _nodeId ),
m_numEndPoints( 0 ),
m_numEndPointsHint( 0 ),
m_endPointMap( MultiInstanceMapAll ),
m_endPointFindSupported( false )
{
}

//-----------------------------------------------------------------------------
// <MultiInstance::ReadXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void MultiInstance::ReadXML
(
		TiXmlElement const* _ccElement
)
{
	int32 intVal;
	char const* str;

	CommandClass::ReadXML( _ccElement );

	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "endpoints", &intVal ) )
	{
		m_numEndPointsHint = (uint8)intVal;
	}

	str = _ccElement->Attribute("mapping");
	if( str )
	{
		if( strcmp( str, "all") == 0 )
		{
			m_endPointMap = MultiInstanceMapAll;
		}
		else if( strcmp( str, "endpoints") == 0 )
		{
			m_endPointMap = MultiInstanceMapEndPoints;
		}
		else
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Bad value for mapping: %s", str);
		}
	}

	str = _ccElement->Attribute("findsupport");
	if( str )
	{
		m_endPointFindSupported = !strcmp( str, "true");
	}
	str = _ccElement->Attribute("ignoreUnsolicitedMultiChnCapReport");
	if( str )
	{
		m_ignoreUnsolicitedMultiChannelCapabilityReport = !strcmp( str, "true");
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::WriteXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void MultiInstance::WriteXML
(
		TiXmlElement* _ccElement
)
{
	char str[32];

	CommandClass::WriteXML( _ccElement );
	if( m_numEndPointsHint != 0 )
	{
		snprintf( str, sizeof(str), "%d", m_numEndPointsHint );
		_ccElement->SetAttribute( "endpoints", str);
	}

	if( m_endPointMap == MultiInstanceMapEndPoints )
	{
		_ccElement->SetAttribute( "mapping", "endpoints" );
	}

	if( m_endPointFindSupported )
	{
		_ccElement->SetAttribute( "findsupport", "true" );
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::RequestInstances>
// Request number of instances of the specified command class from the device
//-----------------------------------------------------------------------------
bool MultiInstance::RequestInstances
(
)
{
	bool res = false;

	if( GetVersion() == 1 )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			// MULTI_INSTANCE
			char str[128];
			for( map<uint8,CommandClass*>::const_iterator it = node->m_commandClassMap.begin(); it != node->m_commandClassMap.end(); ++it )
			{
				CommandClass* cc = it->second;
				if( cc->GetCommandClassId() == NoOperation::StaticGetCommandClassId() )
				{
					continue;
				}
				if( cc->HasStaticRequest( StaticRequest_Instances ) )
				{
					snprintf( str, sizeof( str ), "MultiInstanceCmd_Get for %s", cc->GetCommandClassName().c_str() );

					Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
					msg->Append( GetNodeId() );
					msg->Append( 3 );
					msg->Append( GetCommandClassId() );
					msg->Append( MultiInstanceCmd_Get );
					msg->Append( cc->GetCommandClassId() );
					msg->Append( GetDriver()->GetTransmitOptions() );
					GetDriver()->SendMsg( msg, Driver::MsgQueue_Query );
					res = true;
				}
			}
		}
	}
	else
	{
		// MULTI_CHANNEL
		char str[128];
		snprintf( str, sizeof( str ), "MultiChannelCmd_EndPointGet for node %d", GetNodeId() );

		Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelCmd_EndPointGet );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Query );
		res = true;
	}

	return res;
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MultiInstance::HandleMsg
(
		uint8 const* _data,
		uint32 const _length,
		uint32 const _instance	// = 1
)
{
	bool handled = false;
	Node* node = GetNodeUnsafe();
	if( node != NULL )
	{
		handled = true;
		switch( (MultiInstanceCmd)_data[0] )
		{
			case MultiInstanceCmd_Report:
			{
				HandleMultiInstanceReport( _data, _length );
				break;
			}
			case MultiInstanceCmd_Encap:
			{
				HandleMultiInstanceEncap( _data, _length );
				break;
			}
			case MultiChannelCmd_EndPointReport:
			{
				HandleMultiChannelEndPointReport( _data, _length );
				break;
			}
			case MultiChannelCmd_CapabilityReport:
			{
				HandleMultiChannelCapabilityReport( _data, _length );
				break;
			}
			case MultiChannelCmd_EndPointFindReport:
			{
				HandleMultiChannelEndPointFindReport( _data, _length );
				break;
			}
			case MultiChannelCmd_Encap:
			{
				HandleMultiChannelEncap( _data, _length );
				break;
			}
			default:
			{
				handled = false;
				break;
			}
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiInstanceReport>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiInstanceReport
(
		uint8 const* _data,
		uint32 const _length
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		uint8 commandClassId = _data[1];
		uint8 instances = _data[2];

		if( CommandClass* pCommandClass = node->GetCommandClass( commandClassId ) )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received MultiInstanceReport from node %d for %s: Number of instances = %d", GetNodeId(), pCommandClass->GetCommandClassName().c_str(), instances );
			pCommandClass->SetInstances( instances );
			pCommandClass->ClearStaticRequest( StaticRequest_Instances );
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiInstanceEncap>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiInstanceEncap
(
		uint8 const* _data,
		uint32 const _length
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		uint8 instance = _data[1];
		if( GetVersion() > 1 )
		{
			instance &= 0x7f;
		}
		uint8 commandClassId = _data[2];

		if( CommandClass* pCommandClass = node->GetCommandClass( commandClassId ) )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received a MultiInstanceEncap from node %d, instance %d, for Command Class %s", GetNodeId(), instance, pCommandClass->GetCommandClassName().c_str() );
			pCommandClass->ReceivedCntIncr();
			pCommandClass->HandleMsg( &_data[3], _length-3, instance );
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiChannelEndPointReport>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiChannelEndPointReport
(
		uint8 const* _data,
		uint32 const _length
)
{
	int len;

	if( m_numEndPoints != 0 )
	{
		return;
	}

	m_numEndPointsCanChange = (( _data[1] & 0x80 ) != 0 );	// Number of endpoints can change.
	m_endPointsAreSameClass = (( _data[1] & 0x40 ) != 0 );	// All endpoints are the same command class.
	m_numEndPoints = _data[2] & 0x7f;
	if( m_numEndPointsHint != 0 )
	{
		m_numEndPoints = m_numEndPointsHint;		// don't use device's number
	}

	len = m_numEndPoints;
	if( m_endPointsAreSameClass ) // only need to check single end point
	{
		len = 1;
		Log::Write( LogLevel_Info, GetNodeId(), "Received MultiChannelEndPointReport from node %d. All %d endpoints are the same.", GetNodeId(), m_numEndPoints );
	}
	else
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received MultiChannelEndPointReport from node %d. %d endpoints are not all the same.", GetNodeId(), m_numEndPoints );
	}

	// This code assumes the endpoints are all in numeric sequential order.
	// Since the end point finds do not appear to work this is the best estimate.
	for( uint8 i = 1; i <= len; i++ )
	{

		// Send a single capability request to each endpoint
		char str[128];
		snprintf( str, sizeof( str ), "MultiChannelCmd_CapabilityGet for endpoint %d", i );
		Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( MultiChannelCmd_CapabilityGet );
		msg->Append( i );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiChannelCapabilityReport>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiChannelCapabilityReport
(
		uint8 const* _data,
		uint32 const _length
)
{

	bool dynamic = ((_data[1] & 0x80)!=0);


	if( Node* node = GetNodeUnsafe() )
	{
		/* if you having problems with Dynamic Devices not correctly
		 * updating the commandclasses, see this email thread:
		 * https://groups.google.com/d/topic/openzwave/IwepxScRAVo/discussion
		 */
		if ((m_ignoreUnsolicitedMultiChannelCapabilityReport && (node->GetCurrentQueryStage() != Node::QueryStage_Instances))
				&& !dynamic && m_endPointCommandClasses.size() > 0) {
			Log::Write(LogLevel_Error, GetNodeId(), "Recieved a Unsolicited MultiChannelEncap when we are not in QueryState_Instances");
			return;
		}

		uint8 endPoint = _data[1] & 0x7f;

		Log::Write( LogLevel_Info, GetNodeId(), "Received MultiChannelCapabilityReport from node %d for endpoint %d", GetNodeId(), endPoint );
		Log::Write( LogLevel_Info, GetNodeId(), "    Endpoint is%sdynamic, and is a %s", dynamic ? " " : " not ", node->GetEndPointDeviceClassLabel( _data[2], _data[3] ).c_str() );
		Log::Write( LogLevel_Info, GetNodeId(), "    Command classes supported by the endpoint are:" );

		// Store the command classes for later use
		bool afterMark = false;
		m_endPointCommandClasses.clear();
		uint8 numCommandClasses = _length - 5;
		for( uint8 i = 0; i < numCommandClasses; ++i )
		{
			uint8 commandClassId = _data[i+4];
			if( commandClassId == 0xef )
			{
				afterMark = true;
				continue;
			}

			m_endPointCommandClasses.insert( commandClassId );

			// Ensure the node supports this command class
			CommandClass* cc = node->GetCommandClass( commandClassId );
			if( !cc )
			{
				cc = node->AddCommandClass( commandClassId );
				if( cc && afterMark )
				{
					cc->SetAfterMark();
				}
			}
			if( cc )
			{
				Log::Write( LogLevel_Info, GetNodeId(), "        %s", cc->GetCommandClassName().c_str() );
			}
		}

		// Create internal library instances for each command class in the list
		// Also set up mapping from intances to endpoints for encapsulation
		Basic* basic = static_cast<Basic*>( node->GetCommandClass( Basic::StaticGetCommandClassId() ) );
		if( m_endPointsAreSameClass )				// Create all the same instances here
		{
			int len;

			if( m_endPointMap == MultiInstanceMapAll )	// Include the non-endpoint instance
			{
				endPoint = 0;
				len = m_numEndPoints + 1;
			}
			else
			{
				endPoint = 1;
				len = m_numEndPoints;
			}

			// Create all the command classes for all the endpoints
			for( uint8 i = 1; i <= len; i++ )
			{
				for( set<uint8>::iterator it = m_endPointCommandClasses.begin(); it != m_endPointCommandClasses.end(); ++it )
				{
					uint8 commandClassId = *it;
					CommandClass* cc = node->GetCommandClass( commandClassId );
					if( cc )
					{
						cc->SetInstance( i );
						if( m_endPointMap != MultiInstanceMapAll || i != 1 )
						{
							cc->SetEndPoint( i, endPoint );
						}
						// If we support the BASIC command class and it is mapped to a command class
						// assigned to this end point, make sure the BASIC command class is also associated
						// with this end point.
						if( basic != NULL && basic->GetMapping() == commandClassId )
						{
							basic->SetInstance( i );
							if( m_endPointMap != MultiInstanceMapAll || i != 1 )
							{
								basic->SetEndPoint( i, endPoint );
							}
						}
					}
				}
				endPoint++;
			}
		}
		else							// Endpoints are different
		{
			for( set<uint8>::iterator it = m_endPointCommandClasses.begin(); it != m_endPointCommandClasses.end(); ++it )
			{
				uint8 commandClassId = *it;
				CommandClass* cc = node->GetCommandClass( commandClassId );
				if( cc )
				{
					uint8 i;
					// Find the next free instance of this class
					for( i = 1; i <= 127; i++ )
					{
						if( m_endPointMap == MultiInstanceMapAll ) // Include the non-endpoint instance
						{
							if( !cc->GetInstances()->IsSet( i ) )
							{
								break;
							}
						}
						// Reuse non-endpoint instances first time we see it
						else if( i == 1 && cc->GetInstances()->IsSet( i ) && cc->GetEndPoint( i ) == 0 )
						{
							break;
						}
						// Find the next free instance
						else if( !cc->GetInstances()->IsSet( i ) )
						{
							break;
						}
					}
					cc->SetInstance( i );
					cc->SetEndPoint( i, endPoint );
					// If we support the BASIC command class and it is mapped to a command class
					// assigned to this end point, make sure the BASIC command class is also associated
					// with this end point.
					if( basic != NULL && basic->GetMapping() == commandClassId )
					{
						basic->SetInstance( i );
						basic->SetEndPoint( i, endPoint );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiChannelEndPointFindReport>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiChannelEndPointFindReport
(
		uint8 const* _data,
		uint32 const _length
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "Received MultiChannelEndPointFindReport from node %d", GetNodeId() );
	uint8 numEndPoints = _length - 5;
	for( uint8 i=0; i<numEndPoints; ++i )
	{
		uint8 endPoint = _data[i+4] & 0x7f;

		if( m_endPointsAreSameClass )
		{
			// Use the stored command class list to set up the endpoint.
			if( Node* node = GetNodeUnsafe() )
			{
				for( set<uint8>::iterator it=m_endPointCommandClasses.begin(); it!=m_endPointCommandClasses.end(); ++it )
				{
					uint8 commandClassId = *it;
					CommandClass* cc = node->GetCommandClass( commandClassId );
					if( cc )
					{
						Log::Write( LogLevel_Info, GetNodeId(), "    Endpoint %d: Adding %s", endPoint, cc->GetCommandClassName().c_str() );
						cc->SetInstance( endPoint );
					}
				}
			}
		}
		else
		{
			// Endpoints are different, so request the capabilities
			char str[128];
			snprintf( str, 128, "MultiChannelCmd_CapabilityGet for node %d, endpoint %d", GetNodeId(), endPoint );
			Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( MultiChannelCmd_CapabilityGet );
			msg->Append( endPoint );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		}
	}

	m_numEndPointsFound += numEndPoints;
	if( !m_endPointsAreSameClass )
	{
		if( _data[1] == 0 )
		{
			// No more reports to follow this one, so we can continue the search.
			if( m_numEndPointsFound < numEndPoints )
			{
				// We have not yet found all the endpoints, so move to the next generic class request
				++m_endPointFindIndex;
				if (m_endPointFindIndex <= 13) /* we are finished */
				{
					if( c_genericClass[m_endPointFindIndex] > 0 )
					{
						if (m_endPointFindIndex > 13) /* size of c_genericClassName minus Unknown Entry */
						{
							Log::Write (LogLevel_Warning, GetNodeId(), "m_endPointFindIndex Value was greater than range. Setting to Unknown");
							m_endPointFindIndex = 14;
						}

						char str[128];
						snprintf( str, 128, "MultiChannelCmd_EndPointFind for generic device class 0x%.2x (%s)", c_genericClass[m_endPointFindIndex], c_genericClassName[m_endPointFindIndex] );
						Msg* msg = new Msg( str, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
						msg->Append( GetNodeId() );
						msg->Append( 4 );
						msg->Append( GetCommandClassId() );
						msg->Append( MultiChannelCmd_EndPointFind );
						msg->Append( c_genericClass[m_endPointFindIndex] );		// Generic device class
						msg->Append( 0xff );									// Any specific device class
						msg->Append( GetDriver()->GetTransmitOptions() );
						GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
					}
				} else {
					Log::Write (LogLevel_Warning, GetNodeId(), "m_endPointFindIndex is higher than range. Not Sending MultiChannelCmd_EndPointFind message");
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <MultiInstance::HandleMultiChannelEncap>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
void MultiInstance::HandleMultiChannelEncap
(
		uint8 const* _data,
		uint32 const _length
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		uint8 endPoint = _data[1] & 0x7f;
		uint8 commandClassId = _data[3];
		if( CommandClass* pCommandClass = node->GetCommandClass( commandClassId ) )
		{
			uint8 instance = pCommandClass->GetInstance( endPoint );
			if( instance == 0 )
			{
				Log::Write( LogLevel_Error, GetNodeId(), "Cannot find endpoint map to instance for Command Class %s endpoint %d", pCommandClass->GetCommandClassName().c_str(), endPoint );
			}
			else
			{
				Log::Write( LogLevel_Info, GetNodeId(), "Received a MultiChannelEncap from node %d, endpoint %d for Command Class %s", GetNodeId(), endPoint, pCommandClass->GetCommandClassName().c_str() );
				pCommandClass->HandleMsg( &_data[4], _length-4, instance );
			}
		} else {
			Log::Write(LogLevel_Error, GetNodeId(), "Recieved a MultiChannelEncap for endpoint %d for Command Class %d, which we can't find", endPoint, commandClassId);
		}
	}
}
