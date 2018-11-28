//-----------------------------------------------------------------------------
//
//	Node.cpp
//
//	A node in the Z-Wave network.
//
//	Copyright (c) 2009 Mal Lansell <xpl@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
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

#include "Node.h"
#include "Defs.h"
#include "Group.h"
#include "Options.h"
#include "Manager.h"
#include "Driver.h"
#include "Notification.h"
#include "Msg.h"
#include "platform/Log.h"
#include "platform/Mutex.h"

#include "tinyxml.h"

#include "command_classes/CommandClasses.h"
#include "command_classes/CommandClass.h"
#include "command_classes/Association.h"
#include "command_classes/Basic.h"
#include "command_classes/Configuration.h"
#include "command_classes/ControllerReplication.h"
#include "command_classes/ManufacturerSpecific.h"
#include "command_classes/MultiInstance.h"
#include "command_classes/Security.h"
#include "command_classes/WakeUp.h"
#include "command_classes/NodeNaming.h"
#include "command_classes/NoOperation.h"
#include "command_classes/Version.h"
#include "command_classes/SwitchAll.h"

#include "Scene.h"

#include "value_classes/ValueID.h"
#include "value_classes/Value.h"
#include "value_classes/ValueBool.h"
#include "value_classes/ValueButton.h"
#include "value_classes/ValueByte.h"
#include "value_classes/ValueDecimal.h"
#include "value_classes/ValueInt.h"
#include "value_classes/ValueRaw.h"
#include "value_classes/ValueList.h"
#include "value_classes/ValueSchedule.h"
#include "value_classes/ValueShort.h"
#include "value_classes/ValueString.h"
#include "value_classes/ValueStore.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// Statics
//-----------------------------------------------------------------------------
bool Node::s_deviceClassesLoaded = false;
map<uint8,string> Node::s_basicDeviceClasses;
map<uint8,Node::GenericDeviceClass*> Node::s_genericDeviceClasses;

static char const* c_queryStageNames[] =
{
	"ProtocolInfo",
	"Probe",
	"WakeUp",
	"ManufacturerSpecific1",
	"NodeInfo",
	"SecurityReport",
	"ManufacturerSpecific2",
	"Versions",
	"Instances",
	"Static",
	"Probe1",
	"Associations",
	"Neighbors",
	"Session",
	"Dynamic",
	"Configuration",
	"Complete",
	"None"
};

//-----------------------------------------------------------------------------
// <Node::Node>
// Constructor
//-----------------------------------------------------------------------------
Node::Node
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	m_queryStage( QueryStage_None ),
	m_queryPending( false ),
	m_queryConfiguration( false ),
	m_queryRetries( 0 ),
	m_protocolInfoReceived( false ),
	m_nodeInfoReceived( false ),
	m_manufacturerSpecificClassReceived( false ),
	m_nodeInfoSupported( true ),
	m_nodeAlive( true ),	// assome live node
	m_listening( true ),	// assume we start out listening
	m_frequentListening( false ),
	m_beaming( false ),
	m_routing( false ),
	m_maxBaudRate( 0 ),
	m_version( 0 ),
	m_security( false ),
	m_homeId( _homeId ),
	m_nodeId( _nodeId ),
	m_basic( 0 ),
	m_generic( 0 ),
	m_specific( 0 ),
	m_type( "" ),
	m_numRouteNodes( 0 ),
	m_addingNode( false ),
	m_manufacturerName( "" ),
	m_productName( "" ),
	m_nodeName( "" ),
	m_location( "" ),
	m_manufacturerId( "" ),
	m_productType( "" ),
	m_productId( "" ),
	m_values( new ValueStore() ),
	m_sentCnt( 0 ),
	m_sentFailed( 0 ),
	m_retries( 0 ),
	m_receivedCnt( 0 ),
	m_receivedDups( 0 ),
	m_receivedUnsolicited( 0 ),
	m_lastRequestRTT( 0 ),
	m_lastResponseRTT( 0 ),
	m_averageRequestRTT( 0 ),
	m_averageResponseRTT( 0 ),
	m_quality( 0 ),
	m_lastReceivedMessage(),
	m_errors( 0 )
{
	memset( m_neighbors, 0, sizeof(m_neighbors) );
	memset( m_routeNodes, 0, sizeof(m_routeNodes) );
	AddCommandClass( 0 );
}

//-----------------------------------------------------------------------------
// <Node::~Node>
// Destructor
//-----------------------------------------------------------------------------
Node::~Node
(
)
{
	// Remove any messages from queues
	GetDriver()->RemoveQueues( m_nodeId );

	// Remove the values from the poll list
	for( ValueStore::Iterator it = m_values->Begin(); it != m_values->End(); ++it )
	{
		ValueID const& valueId = it->second->GetID();
		if( GetDriver()->isPolled( valueId ) )
		{
			GetDriver()->DisablePoll( valueId );
		}
	}

	Scene::RemoveValues( m_homeId, m_nodeId );

	// Delete the values
	delete m_values;

	// Delete the command classes
	while( !m_commandClassMap.empty() )
	{
		map<uint8,CommandClass*>::iterator it = m_commandClassMap.begin();
		delete it->second;
		m_commandClassMap.erase( it );
	}

	// Delete the groups
	while( !m_groups.empty() )
	{
		map<uint8,Group*>::iterator it = m_groups.begin();
		delete it->second;
		m_groups.erase( it );
	}

	// Delete the button map
	while( !m_buttonMap.empty() )
	{
		map<uint8,uint8>::iterator it = m_buttonMap.begin();
		m_buttonMap.erase( it );
	}
}

//-----------------------------------------------------------------------------
// <Node::AdvanceQueries>
// Proceed through the initialisation process
//-----------------------------------------------------------------------------
void Node::AdvanceQueries
(
)
{
	// For OpenZWave to discover everything about a node, we have to follow a certain
	// order of queries, because the results of one stage may affect what is requested
	// in the next stage.  The stage is saved with the node data, so that any incomplete
	// queries can be restarted the next time the application runs.
	// The individual command classes also store some state as to whether they have
	// had a response to certain queries.  This state is initilized by the SetStaticRequests
	// call in QueryStage_None.  It is also saved, so we do not need to request state
	// from every command class if some have previously responded.
	//
	// Each stage must generate all the messages for its particular	stage as
	// assumptions are made in later code (RemoveMsg) that this is the case. This means
	// each stage is only visited once.

	Log::Write( LogLevel_Detail, m_nodeId, "AdvanceQueries queryPending=%d queryRetries=%d queryStage=%s live=%d", m_queryPending, m_queryRetries, c_queryStageNames[m_queryStage], m_nodeAlive );
	bool addQSC = false;			// We only want to add a query stage complete if we did some work.
	while( !m_queryPending && m_nodeAlive )
	{
		switch( m_queryStage )
		{
			case QueryStage_None:
			{
				// Init the node query process
				m_queryStage = QueryStage_ProtocolInfo;
				m_queryRetries = 0;
				break;
			}
			case QueryStage_ProtocolInfo:
			{
				// determines, among other things, whether this node is a listener, its maximum baud rate and its device classes
				if( !ProtocolInfoReceived() )
				{
					Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_ProtocolInfo" );
					Msg* msg = new Msg( "Get Node Protocol Info", m_nodeId, REQUEST, FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO, false );
					msg->Append( m_nodeId );
					GetDriver()->SendMsg( msg, Driver::MsgQueue_Query );
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					// This stage has been done already, so move to the Neighbours stage
					m_queryStage = QueryStage_Probe;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Probe:
			{
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Probe" );
				//
				// Send a NoOperation message to see if the node is awake
				// and alive. Based on the response or lack of response
				// will determine next step.
				//
				NoOperation* noop = static_cast<NoOperation*>( GetCommandClass( NoOperation::StaticGetCommandClassId() ) );
				if( GetDriver()->GetNodeId() != m_nodeId )
				{
					noop->Set( true );
				      	m_queryPending = true;
					addQSC = true;
				}
				else
				{
					m_queryStage = QueryStage_WakeUp;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_WakeUp:
			{
				// For sleeping devices other than controllers, we need to defer the usual requests until
				// we have told the device to send it's wake-up notifications to the PC controller.
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_WakeUp" );

				WakeUp* wakeUp = static_cast<WakeUp*>( GetCommandClass( WakeUp::StaticGetCommandClassId() ) );

				// if this device is a "sleeping device" and not a controller and not a
				// FLiRS device. FLiRS will wake up when you send them something and they
				// don't seem to support Wakeup
				if( wakeUp && !IsController() && !IsFrequentListeningDevice() )
				{
					// start the process of requesting node state from this sleeping device
					wakeUp->Init();
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					// this is not a sleeping device, so move to the ManufacturerSpecific1 stage
					m_queryStage = QueryStage_ManufacturerSpecific1;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_ManufacturerSpecific1:
			{
				// Obtain manufacturer, product type and product ID code from the node device
				// Manufacturer Specific data is requested before the other command class data so
				// that we can modify the supported command classes list through the product XML files.
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_ManufacturerSpecific1" );
				if( GetDriver()->GetNodeId() == m_nodeId )
				{
					string configPath = ManufacturerSpecific::SetProductDetails( this, GetDriver()->GetManufacturerId(), GetDriver()->GetProductType(), GetDriver()->GetProductId() );
					if( configPath.length() > 0 )
					{
						ManufacturerSpecific::LoadConfigXML( this, configPath );
					}
					m_queryStage = QueryStage_NodeInfo;
					m_queryRetries = 0;
				}
				else
				{
					ManufacturerSpecific* cc = static_cast<ManufacturerSpecific*>( GetCommandClass( ManufacturerSpecific::StaticGetCommandClassId() ) );
					if( cc  )
					{
						m_queryPending = cc->RequestState( CommandClass::RequestFlag_Static, 1, Driver::MsgQueue_Query );
						addQSC = m_queryPending;
					}
					if( !m_queryPending )
					{
						m_queryStage = QueryStage_NodeInfo;
						m_queryRetries = 0;
					}
				}
				break;
			}
			case QueryStage_NodeInfo:
			{
				if( !NodeInfoReceived() && m_nodeInfoSupported )
				{
					// obtain from the node a list of command classes that it 1) supports and 2) controls (separated by a mark in the buffer)
					Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_NodeInfo" );
					Msg* msg = new Msg( "Request Node Info", m_nodeId, REQUEST, FUNC_ID_ZW_REQUEST_NODE_INFO, false, true, FUNC_ID_ZW_APPLICATION_UPDATE );
					msg->Append( m_nodeId );
					GetDriver()->SendMsg( msg, Driver::MsgQueue_Query );
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					// This stage has been done already, so move to the Manufacturer Specific stage
					m_queryStage = QueryStage_SecurityReport;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_SecurityReport:
			{
				/* For Devices that Support the Security Class, we have to request a list of
				 * Command Classes that Require Security.
				 */
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_SecurityReport" );

				Security* seccc = static_cast<Security*>( GetCommandClass( Security::StaticGetCommandClassId() ) );

				if( seccc )
				{
					// start the process of requesting node state from this sleeping device
					m_queryPending = seccc->Init();
					/* Dont add a Notification Callback here, as this is a multipacket exchange.
					 * the Security Command Class will automatically advance the Query Stage
					 * when we recieve a SecurityCmd_SupportedReport
					 */
					addQSC = false;
				}
				else
				{
					// this is not a Security Device, so move onto the next querystage
					m_queryStage = QueryStage_ManufacturerSpecific2;
					m_queryRetries = 0;
				}

				break;
			}
			case QueryStage_ManufacturerSpecific2:
			{
				if( !m_manufacturerSpecificClassReceived )
				{
					// Obtain manufacturer, product type and product ID code from the node device
					// Manufacturer Specific data is requested before the other command class data so
					// that we can modify the supported command classes list through the product XML files.
					Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_ManufacturerSpecific2" );
					ManufacturerSpecific* cc = static_cast<ManufacturerSpecific*>( GetCommandClass( ManufacturerSpecific::StaticGetCommandClassId() ) );
					if( cc  )
					{
						m_queryPending = cc->RequestState( CommandClass::RequestFlag_Static, 1, Driver::MsgQueue_Query );
						addQSC = m_queryPending;
					}
					if( !m_queryPending )
					{
						m_queryStage = QueryStage_Versions;
						m_queryRetries = 0;
					}
				}
				else
				{
					ManufacturerSpecific* cc = static_cast<ManufacturerSpecific*>( GetCommandClass( ManufacturerSpecific::StaticGetCommandClassId() ) );
					if( cc  )
					{
						cc->ReLoadConfigXML();
					}
					m_queryStage = QueryStage_Versions;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Versions:
			{
				// Get the version information (if the device supports COMMAND_CLASS_VERSION
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Versions" );
				Version* vcc = static_cast<Version*>( GetCommandClass( Version::StaticGetCommandClassId() ) );
				if( vcc )
				{
					for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
					{
						CommandClass* cc = it->second;
						if( cc->GetMaxVersion() > 1 )
						{
							// Get the version for each supported command class that
							// we have implemented at greater than version one.
							m_queryPending |= vcc->RequestCommandClassVersion( it->second );
						}
					}
					addQSC = m_queryPending;
				}
				// advance to Instances stage when finished
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Instances;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Instances:
			{
				// if the device at this node supports multiple instances, obtain a list of these instances
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Instances" );
				MultiInstance* micc = static_cast<MultiInstance*>( GetCommandClass( MultiInstance::StaticGetCommandClassId() ) );
				if( micc )
				{
					m_queryPending = micc->RequestInstances();
					addQSC = m_queryPending;
				}

				// when done, advance to the Static stage
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Static;
					m_queryRetries = 0;

					Log::Write( LogLevel_Info, m_nodeId, "Essential node queries are complete" );
					Notification* notification = new Notification( Notification::Type_EssentialNodeQueriesComplete );
					notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
					GetDriver()->QueueNotification( notification );
				}
				break;
			}
			case QueryStage_Static:
			{
				// Request any other static values associated with each command class supported by this node
				// examples are supported thermostat operating modes, setpoints and fan modes
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Static" );
				for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
				{
					if( !it->second->IsAfterMark() )
					{
						m_queryPending |= it->second->RequestStateForAllInstances( CommandClass::RequestFlag_Static, Driver::MsgQueue_Query );
					}
				}
				addQSC = m_queryPending;

				if( !m_queryPending )
				{
					// when all (if any) static information has been retrieved, advance to the Associations stage
					m_queryStage = QueryStage_Associations;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Probe1:
			{
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Probe1" );
				//
				// Send a NoOperation message to see if the node is awake
				// and alive. Based on the response or lack of response
				// will determine next step. Called here when configuration exists.
				//
				NoOperation* noop = static_cast<NoOperation*>( GetCommandClass( NoOperation::StaticGetCommandClassId() ) );
				if( GetDriver()->GetNodeId() != m_nodeId )
				{
					noop->Set( true );
				      	m_queryPending = true;
					addQSC = true;
				}
				else
				{
					m_queryStage = QueryStage_Associations;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Associations:
			{
				// if this device supports COMMAND_CLASS_ASSOCIATION, determine to which groups this node belong
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Associations" );
				Association* acc = static_cast<Association*>( GetCommandClass( Association::StaticGetCommandClassId() ) );
				if( acc )
				{
					acc->RequestAllGroups( 0 );
					m_queryPending = true;
					addQSC = true;
				}
				else
				{
					// if this device doesn't support Associations, move to retrieve Session information
					m_queryStage = QueryStage_Neighbors;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Neighbors:
			{
				// retrieves this node's neighbors and stores the neighbor bitmap in the node object
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Neighbors" );
				GetDriver()->RequestNodeNeighbors( m_nodeId, 0 );
				m_queryPending = true;
				addQSC = true;
				break;
			}
			case QueryStage_Session:
			{
				// Request the session values from the command classes in turn
				// examples of Session information are: current thermostat setpoints, node names and climate control schedules
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Session" );
				for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
				{
					if( !it->second->IsAfterMark() )
					{
						m_queryPending |= it->second->RequestStateForAllInstances( CommandClass::RequestFlag_Session, Driver::MsgQueue_Query );
					}
				}
				addQSC = m_queryPending;
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Dynamic;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Dynamic:
			{
				// Request the dynamic values from the node, that can change at any time
				// Examples include on/off state, heating mode, temperature, etc.
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Dynamic" );
				m_queryPending = RequestDynamicValues();
				addQSC = m_queryPending;

				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Configuration;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Configuration:
			{
				// Request the configurable parameter values from the node.
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Configuration" );
				if( m_queryConfiguration )
				{
					if( RequestAllConfigParams( 0 ) )
					{
						m_queryPending = true;
						addQSC = true;
					}
					m_queryConfiguration = false;
				}
				if( !m_queryPending )
				{
					m_queryStage = QueryStage_Complete;
					m_queryRetries = 0;
				}
				break;
			}
			case QueryStage_Complete:
			{
				ClearAddingNode();
				// Notify the watchers that the queries are complete for this node
				Log::Write( LogLevel_Detail, m_nodeId, "QueryStage_Complete" );
				Notification* notification = new Notification( Notification::Type_NodeQueriesComplete );
				notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
				GetDriver()->QueueNotification( notification );

				// Check whether all nodes are now complete
				GetDriver()->CheckCompletedNodeQueries();
				return;
			}
			default:
			{
				break;
			}
		}
	}

	if( addQSC && m_nodeAlive )
	{
		// Add a marker to the query queue so this advance method
		// gets called again once this stage has completed.
		GetDriver()->SendQueryStageComplete( m_nodeId, m_queryStage );
	}
}

//-----------------------------------------------------------------------------
// <Node::QueryStageComplete>
// We are done with a stage in the query process
//-----------------------------------------------------------------------------
void Node::QueryStageComplete
(
	QueryStage const _stage
)
{
	// Check that we are actually on the specified stage
	if( _stage != m_queryStage )
	{
		return;
	}

	if( m_queryStage != QueryStage_Complete )
	{
		// Move to the next stage
		m_queryPending = false;
		m_queryStage = (QueryStage)( (uint32)m_queryStage + 1 );
		if( m_queryStage == QueryStage_Probe1 )
		{
			m_queryStage = (QueryStage)( (uint32)m_queryStage + 1 );
		}
		m_queryRetries = 0;
	}
}

//-----------------------------------------------------------------------------
// <Node::QueryStageRetry>
// Retry a stage up to the specified maximum
//-----------------------------------------------------------------------------
void Node::QueryStageRetry
(
	QueryStage const _stage,
	uint8 const _maxAttempts // = 0
)
{
	Log::Write( LogLevel_Info, m_nodeId, "QueryStageRetry stage %s requested stage %s max %d retries %d pending %d", c_queryStageNames[_stage], c_queryStageNames[m_queryStage], _maxAttempts, m_queryRetries, m_queryPending);

	// Check that we are actually on the specified stage
	if( _stage != m_queryStage )
	{
		return;
	}

	m_queryPending = false;
	if( _maxAttempts && ( ++m_queryRetries >= _maxAttempts ) )
	{
		m_queryRetries = 0;
		// We've retried too many times. Move to the next stage but only if
		// we aren't in any of the probe stages.
		if( m_queryStage != QueryStage_Probe && m_queryStage != QueryStage_Probe1 )
		{
			m_queryStage = (Node::QueryStage)( (uint32)(m_queryStage + 1) );
		}
	}
	// Repeat the current query stage
	GetDriver()->RetryQueryStageComplete( m_nodeId, m_queryStage );
}

//-----------------------------------------------------------------------------
// <Node::SetQueryStage>
// Set the query stage (but only to an earlier stage)
//-----------------------------------------------------------------------------
void Node::SetQueryStage
(
	QueryStage const _stage,
	bool const _advance	// = true
)
{
	if( (int)_stage < (int)m_queryStage )
	{
		m_queryStage = _stage;
		m_queryPending = false;

		if( QueryStage_Configuration == _stage )
		{
			m_queryConfiguration = true;
		}
	}

	if( _advance )
	{
		AdvanceQueries();
	}
}

//-----------------------------------------------------------------------------
// <Node::GetQueryStageName>
// Gets the query stage name
//-----------------------------------------------------------------------------
string Node::GetQueryStageName
(
	QueryStage const _stage
)
{
	return c_queryStageNames[_stage];
}

//-----------------------------------------------------------------------------
// <Node::GetNeighbors>
// Gets the neighbors of a node
//-----------------------------------------------------------------------------
uint32 Node::GetNeighbors
(
	uint8** o_neighbors
)
{
	// determine how many neighbors there are
	int i;
	uint32 numNeighbors = 0;
	if( m_queryStage < QueryStage_Session )
	{
		*o_neighbors = NULL;
		return 0;
	}
	for( i = 0; i < 29; i++ )
	{
		for( unsigned char mask = 0x80; mask != 0; mask >>= 1 )
		  if( ( m_neighbors[i] & mask ) != 0 )
				numNeighbors++;
	}

	// handle the possibility that no neighbors are reported
	if( !numNeighbors )
	{
		*o_neighbors = NULL;
		return 0;
	}

	// create and populate an array with neighbor node ids
	uint8* neighbors = new uint8[numNeighbors];
	uint32 index = 0;
	for( int by=0; by<29; by++ )
	{
		for( int bi=0; bi<8; bi++ )
		{
			if( (m_neighbors[by] & ( 0x01<<bi ) ) )
				neighbors[index++] = ( ( by<<3 ) + bi + 1 );
		}
	}

	*o_neighbors = neighbors;
	return numNeighbors;
}

//-----------------------------------------------------------------------------
// <Node::ReadXML>
// Read the node config from XML
//-----------------------------------------------------------------------------
void Node::ReadXML
(
	TiXmlElement const* _node
)
{
	char const* str;
	int intVal;

	str = _node->Attribute( "query_stage" );
	if( str )
	{
		// After restoring state from a file, we need to at least refresh the association, session and dynamic values.
		QueryStage queryStage = QueryStage_Associations;
		for( uint32 i=0; i<(uint32)QueryStage_Associations; ++i )
		{
			if( !strcmp( str, c_queryStageNames[i] ) )
			{
				queryStage = (QueryStage)i;
				break;
			}
		}

		SetQueryStage( queryStage, false );
	}

	if( m_queryStage != QueryStage_None )
	{
		if( m_queryStage > QueryStage_ProtocolInfo )
		{
			// Notify the watchers of the protocol info.
			// We do the notification here so that it gets into the queue ahead of
			// any other notifications generated by adding command classes etc.
			m_protocolInfoReceived = true;
			Notification* notification = new Notification( Notification::Type_NodeProtocolInfo );
			notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
			GetDriver()->QueueNotification( notification );
		}

		if( m_queryStage > QueryStage_NodeInfo )
		{
			m_nodeInfoReceived = true;
		}

		if( m_queryStage > QueryStage_Instances )
		{
			Notification* notification = new Notification( Notification::Type_EssentialNodeQueriesComplete );
			notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
			GetDriver()->QueueNotification( notification );
		}
	}

	str = _node->Attribute( "name" );
	if( str )
	{
		m_nodeName = str;
	}

	str = _node->Attribute( "location" );
	if( str )
	{
		m_location = str;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "basic", &intVal ) )
	{
		m_basic = (uint8)intVal;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "generic", &intVal ) )
	{
		m_generic = (uint8)intVal;
	}

	if( TIXML_SUCCESS == _node->QueryIntAttribute( "specific", &intVal ) )
	{
		m_specific = (uint8)intVal;
	}

	str = _node->Attribute( "type" );
	if( str )
	{
		m_type = str;
	}

	m_listening = true;
	str = _node->Attribute( "listening" );
	if( str )
	{
		m_listening = !strcmp( str, "true" );
	}

	m_frequentListening = false;
	str = _node->Attribute( "frequentListening" );
	if( str )
	{
		m_frequentListening = !strcmp( str, "true" );
	}

	m_beaming = false;
	str = _node->Attribute( "beaming" );
	if( str )
	{
		m_beaming = !strcmp( str, "true" );
	}

	m_routing = true;
	str = _node->Attribute( "routing" );
	if( str )
	{
		m_routing = !strcmp( str, "true" );
	}

	m_maxBaudRate = 0;
	if( TIXML_SUCCESS == _node->QueryIntAttribute( "max_baud_rate", &intVal ) )
	{
		m_maxBaudRate = (uint32)intVal;
	}

	m_version = 0;
	if( TIXML_SUCCESS == _node->QueryIntAttribute( "version", &intVal ) )
	{
		m_version = (uint8)intVal;
	}

	m_security = false;
	str = _node->Attribute( "security" );
	if( str )
	{
		m_security = !strcmp( str, "true" );
	}

	m_nodeInfoSupported = true;
	str = _node->Attribute( "nodeinfosupported" );
	if( str )
	{
		m_nodeInfoSupported = !strcmp( str, "true" );
	}

	// Read the manufacturer info and create the command classes
	TiXmlElement const* child = _node->FirstChildElement();
	while( child )
	{
		str = child->Value();
		if( str )
		{
			if( !strcmp( str, "CommandClasses" ) )
			{
				ReadCommandClassesXML( child );
			}
			else if( !strcmp( str, "Manufacturer" ) )
			{
				str = child->Attribute( "id" );
				if( str )
				{
					m_manufacturerId = str;
				}

				str = child->Attribute( "name" );
				if( str )
				{
					m_manufacturerName = str;
				}

				TiXmlElement const* product = child->FirstChildElement();
				if( !strcmp( product->Value(), "Product" ) )
				{
					str = product->Attribute( "type" );
					if( str )
					{
						m_productType = str;
					}

					str = product->Attribute( "id" );
					if( str )
					{
						m_productId = str;
					}

					str = product->Attribute( "name" );
					if( str )
					{
						m_productName = str;
					}
				}
			}
		}

		// Move to the next child node
		child = child->NextSiblingElement();
	}

	if( m_nodeName.length() > 0 || m_location.length() > 0 || m_manufacturerId.length() > 0 )
	{
		// Notify the watchers of the name changes
		Notification* notification = new Notification( Notification::Type_NodeNaming );
		notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
		GetDriver()->QueueNotification( notification );
	}
}

//-----------------------------------------------------------------------------
// <Node::ReadDeviceProtocolXML>
// Read the device's protocol configuration from XML
//-----------------------------------------------------------------------------
void Node::ReadDeviceProtocolXML
(
	TiXmlElement const* _ccsElement
)
{
	TiXmlElement const* ccElement = _ccsElement->FirstChildElement();
	while( ccElement )
	{
		char const* str = ccElement->Value();
		if( str && !strcmp( str, "Protocol" ) )
		{
			str = ccElement->Attribute( "nodeinfosupported" );
			if( str )
			{
				m_nodeInfoSupported = !strcmp( str, "true" );
			}

			// Some controllers support API calls that aren't advertised in their returned data.
			// So provide a way to manipulate the returned data to reflect reality.
			TiXmlElement const* childElement = _ccsElement->FirstChildElement();
			while( childElement )
			{
				str = childElement->Value();
				if( str && !strcmp( str, "APIcall" ) )
				{
					char const* funcStr = childElement->Attribute( "function" );
					char *p;
					uint8 func = (uint8)strtol( funcStr, &p, 16 );
					if( p != funcStr )
					{
						char const* presStr = ccElement->Attribute( "present" );
						GetDriver()->SetAPICall( func, !strcmp( presStr, "true" ) );
					}
				}
				childElement = childElement->NextSiblingElement();
			}
			return;
		}
		ccElement = ccElement->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::ReadCommandClassesXML>
// Read the command classes from XML
//-----------------------------------------------------------------------------
void Node::ReadCommandClassesXML
(
	TiXmlElement const* _ccsElement
)
{
	char const* str;
	int32 intVal;

	TiXmlElement const* ccElement = _ccsElement->FirstChildElement();
	while( ccElement )
	{
		str = ccElement->Value();
		if( str && !strcmp( str, "CommandClass" ) )
		{
			if( TIXML_SUCCESS == ccElement->QueryIntAttribute( "id", &intVal ) )
			{
				uint8 id = (uint8)intVal;

				// Check whether this command class is to be removed (product XMLs might
				// request this if a class is not implemented properly by the device)
				bool remove = false;
				char const* action = ccElement->Attribute( "action" );
				if( action && !strcasecmp( action, "remove" ) )
				{
					remove = true;
				}

				CommandClass* cc = GetCommandClass( id );
				if( remove )
				{
					// Remove support for the command class
					RemoveCommandClass( id );
				}
				else
				{
					if( NULL == cc )
					{
						// Command class support does not exist yet, so we create it
						cc = AddCommandClass( id );
					}

					if( NULL != cc )
					{
						cc->ReadXML( ccElement );
					}
				}
			}
		}

		ccElement = ccElement->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::WriteXML>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void Node::WriteXML
(
	TiXmlElement* _driverElement
)
{
	char str[32];

	TiXmlElement* nodeElement = new TiXmlElement( "Node" );
	_driverElement->LinkEndChild( nodeElement );

	snprintf( str, 32, "%d", m_nodeId );
	nodeElement->SetAttribute( "id", str );

	nodeElement->SetAttribute( "name", m_nodeName.c_str() );
	nodeElement->SetAttribute( "location", m_location.c_str() );

	snprintf( str, 32, "%d", m_basic );
	nodeElement->SetAttribute( "basic", str );

	snprintf( str, 32, "%d", m_generic );
	nodeElement->SetAttribute( "generic", str );

	snprintf( str, 32, "%d", m_specific );
	nodeElement->SetAttribute( "specific", str );

	nodeElement->SetAttribute( "type", m_type.c_str() );

	nodeElement->SetAttribute( "listening", m_listening ? "true" : "false" );
	nodeElement->SetAttribute( "frequentListening", m_frequentListening ? "true" : "false" );
	nodeElement->SetAttribute( "beaming", m_beaming ? "true" : "false" );
	nodeElement->SetAttribute( "routing", m_routing ? "true" : "false" );

	snprintf( str, 32, "%d", m_maxBaudRate );
	nodeElement->SetAttribute( "max_baud_rate", str );

	snprintf( str, 32, "%d", m_version );
	nodeElement->SetAttribute( "version", str );

	if( m_security )
        {
		nodeElement->SetAttribute( "security", "true" );
	}

	if( !m_nodeInfoSupported )
	{
		nodeElement->SetAttribute( "nodeinfosupported", "false" );
	}

	nodeElement->SetAttribute( "query_stage", c_queryStageNames[m_queryStage] );

	// Write the manufacturer and product data in the same format
	// as used in the ManyfacturerSpecfic.xml file.  This will
	// allow new devices to be added via a simple cut and paste.
	TiXmlElement* manufacturerElement = new TiXmlElement( "Manufacturer" );
	nodeElement->LinkEndChild( manufacturerElement );

	manufacturerElement->SetAttribute( "id", m_manufacturerId.c_str() );
	manufacturerElement->SetAttribute( "name", m_manufacturerName.c_str() );

	TiXmlElement* productElement = new TiXmlElement( "Product" );
	manufacturerElement->LinkEndChild( productElement );

	productElement->SetAttribute( "type", m_productType.c_str() );
	productElement->SetAttribute( "id", m_productId.c_str() );
	productElement->SetAttribute( "name", m_productName.c_str() );

	// Write the command classes
	TiXmlElement* ccsElement = new TiXmlElement( "CommandClasses" );
	nodeElement->LinkEndChild( ccsElement );

	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		if( it->second->GetCommandClassId() == NoOperation::StaticGetCommandClassId() ) // don't output NoOperation
		{
			continue;
		}
		TiXmlElement* ccElement = new TiXmlElement( "CommandClass" );
		ccsElement->LinkEndChild( ccElement );
		it->second->WriteXML( ccElement );
	}
}

//-----------------------------------------------------------------------------
// <Node::UpdateProtocolInfo>
// Handle the FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO response
//-----------------------------------------------------------------------------
void Node::UpdateProtocolInfo
(
	uint8 const* _data
)
{
	if( ProtocolInfoReceived() )
	{
		// We already have this info
		return;
	}

	if( _data[4] == 0 )
	{
		// Node doesn't exist if Generic class is zero.
		Log::Write( LogLevel_Info, m_nodeId, "  Protocol Info for Node %d reports node nonexistent", m_nodeId );
		SetNodeAlive( false );
		return;
	}

	// Notify the watchers of the protocol info.
	// We do the notification here so that it gets into the queue ahead of
	// any other notifications generated by adding command classes etc.
	Notification* notification = new Notification( Notification::Type_NodeProtocolInfo );
	notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
	GetDriver()->QueueNotification( notification );

	// Capabilities
	m_listening = ( ( _data[0] & 0x80 ) != 0 );
	m_routing = ( ( _data[0] & 0x40 ) != 0 );

	m_maxBaudRate = 9600;
	if( ( _data[0] & 0x38 ) == 0x10 )
	{
		m_maxBaudRate = 40000;
	}

	m_version = ( _data[0] & 0x07 ) + 1;

	m_frequentListening = ( ( _data[1] & ( SecurityFlag_Sensor250ms | SecurityFlag_Sensor1000ms ) ) != 0 );
	m_beaming = ( ( _data[1] & SecurityFlag_BeamCapability ) != 0 );

	// Security
	m_security = ( ( _data[1] & SecurityFlag_Security ) != 0 );

	// Optional flag is true if the device reports optional command classes.
	// NOTE: We stopped using this because not all devices report it properly,
	// and now just request the optional classes regardless.
	// bool optional = (( _data[1] & 0x80 ) != 0 );

	Log::Write( LogLevel_Info, m_nodeId, "  Protocol Info for Node %d:", m_nodeId );
	if( m_listening )
		Log::Write( LogLevel_Info, m_nodeId, "    Listening     = true" );
	else
	{
		Log::Write( LogLevel_Info, m_nodeId, "    Listening     = false" );
		Log::Write( LogLevel_Info, m_nodeId, "    Frequent      = %s", m_frequentListening ? "true" : "false" );
	}
	Log::Write( LogLevel_Info, m_nodeId, "    Beaming       = %s", m_beaming ? "true" : "false" );
	Log::Write( LogLevel_Info, m_nodeId, "    Routing       = %s", m_routing ? "true" : "false" );
	Log::Write( LogLevel_Info, m_nodeId, "    Max Baud Rate = %d", m_maxBaudRate );
	Log::Write( LogLevel_Info, m_nodeId, "    Version       = %d", m_version );
	Log::Write( LogLevel_Info, m_nodeId, "    Security      = %s", m_security ? "true" : "false" );

	// Set up the device class based data for the node, including mandatory command classes
	SetDeviceClasses( _data[3], _data[4], _data[5] );
	// Do this for every controller. A little extra work but it won't be a large file.
	if( IsController() )
	{
		GetDriver()->ReadButtons( m_nodeId );
	}
	m_protocolInfoReceived = true;
}

void Node::SetSecuredClasses
(
		uint8 const* _data,
		uint8 const _length
)
{
	uint32 i;
	Log::Write( LogLevel_Info, m_nodeId, "  Secured command classes for node %d:", m_nodeId );

	bool afterMark = false;
	for( i=0; i<_length; ++i )
	{
		if( _data[i] == 0xef )
		{
			// COMMAND_CLASS_MARK.
			// Marks the end of the list of supported command classes.  The remaining classes
			// are those that can be controlled by the device.  These classes are created
			// without values.  Messages received cause notification events instead.
			afterMark = true;
			continue;
		}
		/* Check if this is a CC that is already registered with the node */
		if (CommandClass *pCommandClass = GetCommandClass(_data[i]))
		{
			if (pCommandClass->IsSecureSupported()) {
				pCommandClass->SetSecured();
				Log::Write( LogLevel_Info, m_nodeId, "    %s (Secured)", pCommandClass->GetCommandClassName().c_str() );
			} else {
				Log::Write( LogLevel_Info, m_nodeId, "    %s (Downgraded)", pCommandClass->GetCommandClassName().c_str() );
			}
		}
		/* it might be a new CC we havn't seen as part of the NIF */
		else if( CommandClasses::IsSupported( _data[i] ) )
		{
			if( CommandClass* pCommandClass = AddCommandClass( _data[i] ) )
			{
				// If this class came after the COMMAND_CLASS_MARK, then we do not create values.
				if( afterMark )
				{
					pCommandClass->SetAfterMark();
				}
				if (pCommandClass->IsSecureSupported()) {
					pCommandClass->SetSecured();
					Log::Write( LogLevel_Info, m_nodeId, "    %s (Secured)", pCommandClass->GetCommandClassName().c_str() );
				} else {
					Log::Write( LogLevel_Info, m_nodeId, "    %s (Downgraded)", pCommandClass->GetCommandClassName().c_str() );
				}
				// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
				// then some command class instance counts will increase once the responses to the RequestState
				// call at the end of this method have been processed.
				pCommandClass->SetInstance( 1 );

			}
		}
		else
		{
			Log::Write( LogLevel_Info, m_nodeId, "    Secure CommandClass 0x%.2x - NOT SUPPORTED", _data[i] );
		}
	}
	Log::Write( LogLevel_Info, m_nodeId, "  UnSecured command classes for node %d:", m_nodeId );
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		if (!it->second->IsSecured())
			Log::Write( LogLevel_Info, m_nodeId, "    %s (Unsecured)", it->second->GetCommandClassName().c_str() );
	}


}
//-----------------------------------------------------------------------------
// <Node::UpdateNodeInfo>
// Set up the command classes from the node info frame
//-----------------------------------------------------------------------------
void Node::UpdateNodeInfo
(
	uint8 const* _data,
	uint8 const _length
)
{
	if( !NodeInfoReceived() )
	{
		// Add the command classes specified by the device
		Log::Write( LogLevel_Info, m_nodeId, "  Optional command classes for node %d:", m_nodeId );

		bool newCommandClasses = false;
		uint32 i;

		bool afterMark = false;
		for( i=0; i<_length; ++i )
		{
			if( _data[i] == 0xef )
			{
				// COMMAND_CLASS_MARK.
				// Marks the end of the list of supported command classes.  The remaining classes
				// are those that can be controlled by the device.  These classes are created
				// without values.  Messages received cause notification events instead.
				afterMark = true;

				if( !newCommandClasses )
				{
					Log::Write( LogLevel_Info, m_nodeId, "    None" );
				}
				Log::Write( LogLevel_Info, m_nodeId, "  Optional command classes controlled by node %d:", m_nodeId );
				newCommandClasses = false;
				continue;
			}

			if( CommandClasses::IsSupported( _data[i] ) )
			{
				if( CommandClass* pCommandClass = AddCommandClass( _data[i] ) )
				{
					// If this class came after the COMMAND_CLASS_MARK, then we do not create values.
					if( afterMark )
					{
						pCommandClass->SetAfterMark();
					}

					// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
					// then some command class instance counts will increase once the responses to the RequestState
					// call at the end of this method have been processed.
					pCommandClass->SetInstance( 1 );
					newCommandClasses = true;
					Log::Write( LogLevel_Info, m_nodeId, "    %s", pCommandClass->GetCommandClassName().c_str() );
				}
			}
			else
			{
				Log::Write( LogLevel_Info, m_nodeId, "  CommandClass 0x%.2x - NOT REQUIRED", _data[i] );
			}
		}

		if( !newCommandClasses )
		{
			// No additional command classes over the mandatory ones.
			Log::Write( LogLevel_Info, m_nodeId, "    None" );
		}

		SetStaticRequests();
		m_nodeInfoReceived = true;
	}
	else
	{
		// We probably only need to do the dynamic stuff
		SetQueryStage( QueryStage_Dynamic );
	}

	// Treat the node info frame as a sign that the node is awake
	if( WakeUp* wakeUp = static_cast<WakeUp*>( GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
	{
		wakeUp->SetAwake( true );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetNodeAlive>
// Track alive state of a node for dead node detection.
//-----------------------------------------------------------------------------
void Node::SetNodeAlive
(
	bool const _isAlive
)
{
	Notification* notification;

	if( _isAlive )
	{
		Log::Write( LogLevel_Error, m_nodeId, "WARNING: node revived" );
		m_nodeAlive = true;
		m_errors = 0;
		if( m_queryStage != Node::QueryStage_Complete )
		{
			m_queryRetries = 0; // restart at last stage
			AdvanceQueries();
		}
		notification = new Notification( Notification::Type_Notification );
		notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
		notification->SetNotification( Notification::Code_Alive );
	}
	else
	{
		Log::Write( LogLevel_Error, m_nodeId, "ERROR: node presumed dead" );
		m_nodeAlive = false;
		if( m_queryStage != Node::QueryStage_Complete )
		{
			// Check whether all nodes are now complete
			GetDriver()->CheckCompletedNodeQueries();
		}
		notification = new Notification( Notification::Type_Notification );
		notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
		notification->SetNotification( Notification::Code_Dead );
	}
	GetDriver()->QueueNotification( notification );
}

//-----------------------------------------------------------------------------
// <Node::SetStaticRequests>
// The first time we hear from a node, we set flags to indicate the
// need to request certain static data from the device.  This is so that
// we can track which data has been received, and which has not.
//-----------------------------------------------------------------------------
void Node::SetStaticRequests
(
)
{
	uint8 request = 0;

	if( GetCommandClass( MultiInstance::StaticGetCommandClassId() ) )
	{
		// Request instances
		request |= (uint8)CommandClass::StaticRequest_Instances;
	}

	if( GetCommandClass( Version::StaticGetCommandClassId() ) )
	{
		// Request versions
		request |= (uint8)CommandClass::StaticRequest_Version;
	}

	if( request )
	{
		for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
		{
			it->second->SetStaticRequest( request );
		}
		SetQueryStage( QueryStage_ManufacturerSpecific2 );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetNodeName>
// Set the name of the node
//-----------------------------------------------------------------------------
void Node::SetNodeName
(
	string const& _nodeName
)
{
	m_nodeName = _nodeName;
	// Notify the watchers of the name changes
	Notification* notification = new Notification( Notification::Type_NodeNaming );
	notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
	GetDriver()->QueueNotification( notification );
	if( NodeNaming* cc = static_cast<NodeNaming*>( GetCommandClass( NodeNaming::StaticGetCommandClassId() ) ) )
	{
		// The node supports naming, so we try to write the name into the device
		cc->SetName( _nodeName );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetLocation>
// Set the location of the node
//-----------------------------------------------------------------------------
void Node::SetLocation
(
	string const& _location
)
{
	m_location = _location;
	// Notify the watchers of the name changes
	Notification* notification = new Notification( Notification::Type_NodeNaming );
	notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
	GetDriver()->QueueNotification( notification );
	if( NodeNaming* cc = static_cast<NodeNaming*>( GetCommandClass( NodeNaming::StaticGetCommandClassId() ) ) )
	{
		// The node supports naming, so we try to write the location into the device
		cc->SetLocation( _location );
	}
}

//-----------------------------------------------------------------------------
// <Node::ApplicationCommandHandler>
// Handle a command class message
//-----------------------------------------------------------------------------
void Node::ApplicationCommandHandler
(
	uint8 const* _data
)
{
	if( CommandClass* pCommandClass = GetCommandClass( _data[5] ) )
	{
		pCommandClass->ReceivedCntIncr();
		pCommandClass->HandleMsg( &_data[6], _data[4] );
	}
	else
	{
		if( _data[5] == ControllerReplication::StaticGetCommandClassId() )
		{
			// This is a controller replication message, and we do not support it.
			// We have to at least acknowledge the message to avoid locking the sending device.
			Log::Write( LogLevel_Info, m_nodeId, "ApplicationCommandHandler - Default acknowledgement of controller replication data" );

			Msg* msg = new Msg( "Replication Command Complete", m_nodeId, REQUEST, FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE, false );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Command );
		}
		else
		{
			Log::Write( LogLevel_Info, m_nodeId, "ApplicationCommandHandler - Unhandled Command Class 0x%.2x", _data[5] );
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::GetCommandClass>
// Get the specified command class object if supported, otherwise NULL
//-----------------------------------------------------------------------------
CommandClass* Node::GetCommandClass
(
	uint8 const _commandClassId
)const
{
	map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.find( _commandClassId );
	if( it != m_commandClassMap.end() )
	{
		return it->second;
	}

	// Not found
	return NULL;
}

//-----------------------------------------------------------------------------
// <Node::AddCommandClass>
// Add a command class to the node
//-----------------------------------------------------------------------------
CommandClass* Node::AddCommandClass
(
	uint8 const _commandClassId
)
{
	if( GetCommandClass( _commandClassId ) )
	{
		// Class and instance have already been added
		return NULL;
	}

	// Create the command class object and add it to our map
	if( CommandClass* pCommandClass = CommandClasses::CreateCommandClass( _commandClassId, m_homeId, m_nodeId ) )
	{
		m_commandClassMap[_commandClassId] = pCommandClass;
		return pCommandClass;
	}
	else
	{
		Log::Write( LogLevel_Info, m_nodeId, "AddCommandClass - Unsupported Command Class 0x%.2x", _commandClassId );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Node::RemoveCommandClass>
// Remove a command class from the node
//-----------------------------------------------------------------------------
void Node::RemoveCommandClass
(
	uint8 const _commandClassId
)
{
	map<uint8,CommandClass*>::iterator it = m_commandClassMap.find( _commandClassId );
	if( it == m_commandClassMap.end() )
	{
		// Class is not found
		return;
	}

	// Remove all the values associated with this class
	if( ValueStore* store = GetValueStore() )
	{
		store->RemoveCommandClassValues( _commandClassId );
	}

	// Destroy the command class object and remove it from our map
	Log::Write( LogLevel_Info, m_nodeId, "RemoveCommandClass - Removed support for %s", it->second->GetCommandClassName().c_str() );

	delete it->second;
	m_commandClassMap.erase( it );
}

//-----------------------------------------------------------------------------
// <Node::SetConfigParam>
// Set a configuration parameter in a device
//-----------------------------------------------------------------------------
bool Node::SetConfigParam
(
	uint8 const _param,
	int32 _value,
	uint8 const _size
)
{
	if( Configuration* cc = static_cast<Configuration*>( GetCommandClass( Configuration::StaticGetCommandClassId() ) ) )
	{
		// First try to find an existing value representing the parameter, and set that.
		if( Value* value = cc->GetValue( 1, _param ) )
		{
			switch( value->GetID().GetType() )
			{
				case ValueID::ValueType_Bool:
				{
					ValueBool* valueBool = static_cast<ValueBool*>( value );
					valueBool->Set( _value != 0 );
					break;
				}
				case ValueID::ValueType_Byte:
				{
					ValueByte* valueByte = static_cast<ValueByte*>( value );
					valueByte->Set( (uint8)_value );
					break;
				}
				case ValueID::ValueType_Short:
				{
					ValueShort* valueShort = static_cast<ValueShort*>( value );
					valueShort->Set( (uint16)_value );
					break;
				}
				case ValueID::ValueType_Int:
				{
					ValueInt* valueInt = static_cast<ValueInt*>( value );
					valueInt->Set( _value );
					break;
				}
				case ValueID::ValueType_List:
				{
					ValueList* valueList = static_cast<ValueList*>( value );
					valueList->SetByValue( _value );
					break;
				}
				default:
				{
				}
			}

			return true;
		}

		// Failed to find an existing value object representing this
		// configuration parameter, so we try using the default or
		// included size through the Configuration command class.
		cc->Set( _param, _value, _size );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Node::RequestConfigParam>
// Request the value of a configuration parameter from the device
//-----------------------------------------------------------------------------
void Node::RequestConfigParam
(
	uint8 const _param
)
{
	if( Configuration* cc = static_cast<Configuration*>( GetCommandClass( Configuration::StaticGetCommandClassId() ) ) )
	{
		cc->RequestValue( 0, _param, 1, Driver::MsgQueue_Send );
	}
}

//-----------------------------------------------------------------------------
// <Node::RequestAllConfigParams>
// Request the values of all known configuration parameters from the device
//-----------------------------------------------------------------------------
bool Node::RequestAllConfigParams
(
	uint32 const _requestFlags
)
{
	bool res = false;
	if( Configuration* cc = static_cast<Configuration*>( GetCommandClass( Configuration::StaticGetCommandClassId() ) ) )
	{
		// Go through all the values in the value store, and request all those which are in the Configuration command class
		for( ValueStore::Iterator it = m_values->Begin(); it != m_values->End(); ++it )
		{
			Value* value = it->second;
			if( value->GetID().GetCommandClassId() == Configuration::StaticGetCommandClassId() && !value->IsWriteOnly() )
			{
				res |= cc->RequestValue( _requestFlags, value->GetID().GetIndex(), 1, Driver::MsgQueue_Send );
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Node::RequestDynamicValues>
// Request an update of all known dynamic values from the device
//-----------------------------------------------------------------------------
bool Node::RequestDynamicValues
(
)
{
	bool res = false;
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		if( !it->second->IsAfterMark() )
		{
			res |= it->second->RequestStateForAllInstances( CommandClass::RequestFlag_Dynamic, Driver::MsgQueue_Send );
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Node::SetLevel>
// Helper method to set a device's basic level
//-----------------------------------------------------------------------------
void Node::SetLevel
(
	uint8 const _level
)
{
	// Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
	uint8 adjustedLevel = _level;
	if( ( _level > 99 ) && ( _level < 255 ) )
	{
		adjustedLevel = 99;
	}

	if( Basic* cc = static_cast<Basic*>( GetCommandClass( Basic::StaticGetCommandClassId() ) ) )
	{
		cc->Set( adjustedLevel );
	}
}

//-----------------------------------------------------------------------------
// <Node::SetNodeOn>
// Helper method to set a device to be on
//-----------------------------------------------------------------------------
void Node::SetNodeOn
(
)
{
    // Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
    if( Basic* cc = static_cast<Basic*>( GetCommandClass( Basic::StaticGetCommandClassId() ) ) )
    {
        cc->Set( 255 );
    }
}

//-----------------------------------------------------------------------------
// <Node::SetNodeOff>
// Helper method to set a device to be off
//-----------------------------------------------------------------------------
void Node::SetNodeOff
(
)
{
    // Level is 0-99, with 0 = off and 99 = fully on. 255 = turn on at last level.
    if( Basic* cc = static_cast<Basic*>( GetCommandClass( Basic::StaticGetCommandClassId() ) ) )
    {
        cc->Set( 0 );
    }
}

//-----------------------------------------------------------------------------
// <Node::CreateValueID>
// Helper to create a ValueID
//-----------------------------------------------------------------------------
ValueID Node::CreateValueID
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	ValueID::ValueType const _type
)
{
	return ValueID( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _type );
}

//-----------------------------------------------------------------------------
// <Node::CreateValueBool>
// Helper to create a new bool value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueBool
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	bool const _default,
	uint8 const _pollIntensity
)
{
  	ValueBool* value = new ValueBool( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueButton>
// Helper to create a new trigger value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueButton
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	uint8 const _pollIntensity
)
{
	ValueButton* value = new ValueButton( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueByte>
// Helper to create a new byte value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueByte
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	uint8 const _default,
	uint8 const _pollIntensity
)
{
  	ValueByte* value = new ValueByte( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueDecimal>
// Helper to create a new decimal value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueDecimal
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	string const& _default,
	uint8 const _pollIntensity
)
{
  	ValueDecimal* value = new ValueDecimal( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueInt>
// Helper to create a new int value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueInt
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	int32 const _default,
	uint8 const _pollIntensity
)
{
  	ValueInt* value = new ValueInt( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueList>
// Helper to create a new list value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueList
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	uint8 const _size,
	vector<ValueList::Item> const& _items,
	int32 const _default,
	uint8 const _pollIntensity
)
{
	ValueList* value = new ValueList( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _items, _default, _pollIntensity, _size );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueRaw>
// Helper to create a new raw value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueRaw
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	uint8 const* _default,
	uint8 const _length,
	uint8 const _pollIntensity
)
{
	ValueRaw* value = new ValueRaw( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _length, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueSchedule>
// Helper to create a new schedule value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueSchedule
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	uint8 const _pollIntensity
)
{
	ValueSchedule* value = new ValueSchedule( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueShort>
// Helper to create a new short value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueShort
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	int16 const _default,
	uint8 const _pollIntensity
)
{
  	ValueShort* value = new ValueShort( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::CreateValueString>
// Helper to create a new string value and add it to the value store
//-----------------------------------------------------------------------------
bool Node::CreateValueString
(
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	string const& _default,
	uint8 const _pollIntensity
)
{
  	ValueString* value = new ValueString( m_homeId, m_nodeId, _genre, _commandClassId, _instance, _valueIndex, _label, _units, _readOnly, _writeOnly, _default, _pollIntensity );
	ValueStore* store = GetValueStore();
	if( store->AddValue( value ) )
	{
		value->Release();
		return true;
	}

	value->Release();
	return false;
}

//-----------------------------------------------------------------------------
// <Node::RemoveValueList>
// Helper to remove an existing list value from the value store
//-----------------------------------------------------------------------------
void Node::RemoveValueList
(
	ValueList* _value
)
{
	ValueStore* store = GetValueStore();
	store->RemoveValue( _value->GetID().GetValueStoreKey() );
}

//-----------------------------------------------------------------------------
// <Node::CreateValueFromXML>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
bool Node::CreateValueFromXML
(
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value* value = NULL;

	// Create the value
	ValueID::ValueType type = Value::GetTypeEnumFromName( _valueElement->Attribute( "type" ) );

	switch( type )
	{
		case ValueID::ValueType_Bool:		{	value = new ValueBool();		break;	}
		case ValueID::ValueType_Byte:		{	value = new ValueByte();		break;	}
		case ValueID::ValueType_Decimal:	{	value = new ValueDecimal();		break;	}
		case ValueID::ValueType_Int:		{	value = new ValueInt();			break;	}
		case ValueID::ValueType_List:		{	value = new ValueList();		break;	}
		case ValueID::ValueType_Schedule:	{	value = new ValueSchedule();		break;	}
		case ValueID::ValueType_Short:		{	value = new ValueShort();		break;	}
		case ValueID::ValueType_String:		{	value = new ValueString();		break;	}
		case ValueID::ValueType_Button:		{	value = new ValueButton();		break;	}
		case ValueID::ValueType_Raw:		{	value = new ValueRaw();			break;  }
		default:				{	Log::Write( LogLevel_Info, m_nodeId, "Unknown ValueType in XML: %s", _valueElement->Attribute( "type" ) ); break; }
	}

	if( value )
	{
		value->ReadXML( m_homeId, m_nodeId, _commandClassId, _valueElement );

		ValueStore* store = GetValueStore();
		if( store->AddValue( value ) )
		{
			value->Release();
			return true;
		}

		value->Release();
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Node::ReadValueFromXML>
// Apply XML differences to a value
//-----------------------------------------------------------------------------
void Node::ReadValueFromXML
(
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	int32 intVal;

	ValueID::ValueGenre genre = Value::GetGenreEnumFromName( _valueElement->Attribute( "genre" ) );
	ValueID::ValueType type = Value::GetTypeEnumFromName( _valueElement->Attribute( "type" ) );

	uint8 instance = 0;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "instance", &intVal ) )
	{
		instance = (uint8)intVal;
	}

	uint8 index = 0;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "index", &intVal ) )
	{
		index = (uint8)intVal;
	}

	ValueID id = ValueID( m_homeId, m_nodeId, genre, _commandClassId, instance, index, type );

	// Try to get the value from the ValueStore (everything except configuration parameters
	// should already have been created when the command class instance count was read in).
	// Create it if it doesn't already exist.
	if( ValueStore* store = GetValueStore() )
	{
		if( Value* value = store->GetValue( id.GetValueStoreKey() ) )
		{
			value->ReadXML( m_homeId, m_nodeId, _commandClassId, _valueElement );
			value->Release();
		}
		else
		{
			CreateValueFromXML( _commandClassId, _valueElement );
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::GetValue>
// Get the value object with the specified ID
//-----------------------------------------------------------------------------
Value* Node::GetValue
(
	ValueID const& _id
)
{
	// This increments the value's reference count
	return GetValueStore()->GetValue( _id.GetValueStoreKey() );
}

//-----------------------------------------------------------------------------
// <Node::GetValue>
// Get the value object with the specified settings
//-----------------------------------------------------------------------------
Value* Node::GetValue
(
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	Value* value = NULL;
	ValueStore* store = GetValueStore();
	// This increments the value's reference count
	value = store->GetValue( ValueID::GetValueStoreKey( _commandClassId, _instance, _valueIndex ) );
	return value;
}

//-----------------------------------------------------------------------------
// <Node::RemoveValue>
// Remove the value object with the specified settings
//-----------------------------------------------------------------------------
bool Node::RemoveValue
(
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _valueIndex
)
{
	ValueStore* store = GetValueStore();
	return store->RemoveValue( ValueID::GetValueStoreKey( _commandClassId, _instance, _valueIndex ) );
}

//-----------------------------------------------------------------------------
// <Node::GetGroup>
// Get a Group from the node's map
//-----------------------------------------------------------------------------
Group* Node::GetGroup
(
	uint8 const _groupIdx
)
{
	map<uint8,Group*>::iterator it = m_groups.find( _groupIdx );
	if( it == m_groups.end() )
	{
		return NULL;
	}

	return it->second;
}

//-----------------------------------------------------------------------------
// <Node::AddGroup>
// Add a group into the node's map
//-----------------------------------------------------------------------------
void Node::AddGroup
(
	Group* _group
)
{
	map<uint8,Group*>::iterator it = m_groups.find( _group->GetIdx() );
	if( it != m_groups.end() )
	{
		// There is already a group with this id.  We will replace it.
		delete it->second;
		m_groups.erase( it );
	}

	m_groups[_group->GetIdx()] = _group;
}

//-----------------------------------------------------------------------------
// <Node::WriteGroups>
// Save the group data
//-----------------------------------------------------------------------------
void Node::WriteGroups
(
	TiXmlElement* _associationsElement
)
{
	for( map<uint8,Group*>::iterator it = m_groups.begin(); it != m_groups.end(); ++it )
	{
		Group* group = it->second;

		TiXmlElement* groupElement = new TiXmlElement( "Group" );
		_associationsElement->LinkEndChild( groupElement );
		group->WriteXML( groupElement );
	}
}

//-----------------------------------------------------------------------------
// <Node::GetNumGroups>
// Gets the number of association groups reported by this node
//-----------------------------------------------------------------------------
uint8 Node::GetNumGroups
(
)
{
	return (uint8) m_groups.size();
}

//-----------------------------------------------------------------------------
// <Node::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 Node::GetAssociations
(
	uint8 const _groupIdx,
	uint8** o_associations
)
{
	uint32 numAssociations = 0;
	if( Group* group = GetGroup( _groupIdx ) )
	{
		numAssociations = group->GetAssociations( o_associations );
	}

	return numAssociations;
}

//-----------------------------------------------------------------------------
// <Node::GetMaxAssociations>
// Gets the maximum number of associations for a group
//-----------------------------------------------------------------------------
uint8 Node::GetMaxAssociations
(
	uint8 const _groupIdx
)
{
	uint8 maxAssociations = 0;
	if( Group* group = GetGroup( _groupIdx ) )
	{
		maxAssociations = group->GetMaxAssociations();
	}

	return maxAssociations;
}

//-----------------------------------------------------------------------------
// <Node::GetGroupLabel>
// Gets the label for a particular group
//-----------------------------------------------------------------------------
string Node::GetGroupLabel
(
	uint8 const _groupIdx
)
{
	string label = "";
	if( Group* group = GetGroup( _groupIdx ) )
	{
		label = group->GetLabel();
	}

	return label;
}

//-----------------------------------------------------------------------------
// <Node::AddAssociation>
// Adds a node to an association group
//-----------------------------------------------------------------------------
void Node::AddAssociation
(
	uint8 const _groupIdx,
	uint8 const _targetNodeId
)
{
	if( Group* group = GetGroup( _groupIdx ) )
	{
		group->AddAssociation( _targetNodeId );
	}
}

//-----------------------------------------------------------------------------
// <Node::RemoveAssociation>
// Removes a node from an association group
//-----------------------------------------------------------------------------
void Node::RemoveAssociation
(
	uint8 const _groupIdx,
	uint8 const _targetNodeId
)
{
	if( Group* group = GetGroup( _groupIdx ) )
	{
		group->RemoveAssociation( _targetNodeId );
	}
}

//-----------------------------------------------------------------------------
// <Node::AutoAssociate>
// Automatically associate the controller with certain groups
//-----------------------------------------------------------------------------
void Node::AutoAssociate
(
)
{
	bool autoAssociate = false;
	Options::Get()->GetOptionAsBool( "Associate", &autoAssociate );
	if( autoAssociate )
	{
		// Try to automatically associate with any groups that have been flagged.
		uint8 controllerNodeId = GetDriver()->GetNodeId();

		for( map<uint8,Group*>::iterator it = m_groups.begin(); it != m_groups.end(); ++it )
		{
			Group* group = it->second;
			if( group->IsAuto() && !group->Contains( controllerNodeId ) )
			{
				// Associate the controller into the group
				Log::Write( LogLevel_Info, m_nodeId, "Adding the controller to group %d (%s) of node %d", group->GetIdx(), group->GetLabel().c_str(), GetNodeId() );
				group->AddAssociation( controllerNodeId );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Node::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
Driver* Node::GetDriver
(
)const
{
	return( Manager::Get()->GetDriver( m_homeId ) );
}

//-----------------------------------------------------------------------------
// Device Classes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Node::GetEndPointDeviceClassLabel>
// Use the device class data to get a label for a MultiChannel EndPoint.
//-----------------------------------------------------------------------------
string Node::GetEndPointDeviceClassLabel
(
	uint8 const _generic,
	uint8 const _specific
)
{
	char str[32];
	string label;

	snprintf( str, sizeof(str), "Generic 0x%.2x Specific 0x%.2x", _generic, _specific );
	label = str;

	// Read in the device class data if it has not been read already.
	if( !s_deviceClassesLoaded )
	{
		ReadDeviceClasses();
	}

	// Get the Generic device class label
	map<uint8,GenericDeviceClass*>::iterator git = s_genericDeviceClasses.find( _generic );
	if( git != s_genericDeviceClasses.end() )
	{
		GenericDeviceClass* genericDeviceClass = git->second;
		label = genericDeviceClass->GetLabel();

		// Override with any specific device class label
		if( DeviceClass* specificDeviceClass = genericDeviceClass->GetSpecificDeviceClass( _specific ) )
		{
			label = specificDeviceClass->GetLabel();
		}
	}

	return label;
}

//-----------------------------------------------------------------------------
// <Node::SetDeviceClasses>
// Set the device class data for the node
//-----------------------------------------------------------------------------
bool Node::SetDeviceClasses
(
	uint8 const _basic,
	uint8 const _generic,
	uint8 const _specific
)
{
	m_basic = _basic;
	m_generic = _generic;
	m_specific = _specific;

	// Read in the device class data if it has not been read already.
	if( !s_deviceClassesLoaded )
	{
		ReadDeviceClasses();
	}

	// Get the basic device class label
	map<uint8,string>::iterator bit = s_basicDeviceClasses.find( _basic );
	if( bit != s_basicDeviceClasses.end() )
	{
		m_type = bit->second;
		Log::Write( LogLevel_Info, m_nodeId, "  Basic device class    (0x%.2x) - %s", m_basic, m_type.c_str() );
	}
	else
	{
		Log::Write( LogLevel_Info, m_nodeId, "  Basic device class unknown" );
	}

	// Apply any Generic device class data
	uint8 basicMapping = 0;
	map<uint8,GenericDeviceClass*>::iterator git = s_genericDeviceClasses.find( _generic );
	if( git != s_genericDeviceClasses.end() )
	{
		GenericDeviceClass* genericDeviceClass = git->second;
		m_type = genericDeviceClass->GetLabel();

		Log::Write( LogLevel_Info, m_nodeId, "  Generic device Class  (0x%.2x) - %s", m_generic, m_type.c_str() );

		// Add the mandatory command classes for this generic class type
		AddMandatoryCommandClasses( genericDeviceClass->GetMandatoryCommandClasses() );

		// Get the command class that COMMAND_CLASS_BASIC maps to.
		basicMapping = genericDeviceClass->GetBasicMapping();

		// Apply any Specific device class data
		if( DeviceClass* specificDeviceClass = genericDeviceClass->GetSpecificDeviceClass( _specific ) )
		{
			m_type = specificDeviceClass->GetLabel();

			Log::Write( LogLevel_Info, m_nodeId, "  Specific device class (0x%.2x) - %s", m_specific, m_type.c_str() );

			// Add the mandatory command classes for this specific class type
			AddMandatoryCommandClasses( specificDeviceClass->GetMandatoryCommandClasses() );

			if( specificDeviceClass->GetBasicMapping() )
			{
				// Override the generic device class basic mapping with the specific device class one.
				basicMapping = specificDeviceClass->GetBasicMapping();
			}
		}
		else
		{
			Log::Write( LogLevel_Info, m_nodeId, "  No specific device class defined" );
		}
	}
	else
	{
		Log::Write( LogLevel_Info, m_nodeId, "  No generic or specific device classes defined" );
	}

	// Deal with sleeping devices
	if( !m_listening && !IsFrequentListeningDevice())
	{
		// Device does not always listen, so we need the WakeUp handler.  We can't
		// wait for the command class list because the request for the command
		// classes may need to go in the wakeup queue itself!
		if( CommandClass* pCommandClass = AddCommandClass( WakeUp::StaticGetCommandClassId() ) )
		{
			pCommandClass->SetInstance( 1 );
		}
	}

	// Apply any COMMAND_CLASS_BASIC remapping
	if( Basic* cc = static_cast<Basic*>( GetCommandClass( Basic::StaticGetCommandClassId() ) ) )
	{
		cc->SetMapping( basicMapping );
	}

	// Write the mandatory command classes to the log
	if( !m_commandClassMap.empty() )
	{
		map<uint8,CommandClass*>::const_iterator cit;

		Log::Write( LogLevel_Info, m_nodeId, "  Mandatory Command Classes for Node %d:", m_nodeId );
		bool reportedClasses = false;
		for( cit = m_commandClassMap.begin(); cit != m_commandClassMap.end(); ++cit )
		{
			if( !cit->second->IsAfterMark() && cit->second->GetCommandClassId() != NoOperation::StaticGetCommandClassId() )
			{
				Log::Write( LogLevel_Info, m_nodeId, "    %s", cit->second->GetCommandClassName().c_str() );
				reportedClasses = true;
			}
		}
		if( !reportedClasses )
		{
			Log::Write( LogLevel_Info, m_nodeId, "    None" );
		}

		Log::Write( LogLevel_Info, m_nodeId, "  Mandatory Command Classes controlled by Node %d:", m_nodeId );
		reportedClasses = false;
		for( cit = m_commandClassMap.begin(); cit != m_commandClassMap.end(); ++cit )
		{
			if( cit->second->IsAfterMark() )
			{
				Log::Write( LogLevel_Info, m_nodeId, "    %s", cit->second->GetCommandClassName().c_str() );
				reportedClasses = true;
			}
		}
		if( !reportedClasses )
		{
			Log::Write( LogLevel_Info, m_nodeId, "    None" );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Node::AddMandatoryCommandClasses>
// Add mandatory command classes to the node
//-----------------------------------------------------------------------------
bool Node::AddMandatoryCommandClasses
(
	uint8 const* _commandClasses
)
{
	if( NULL == _commandClasses )
	{
		// No command classes to add
		return false;
	}

	int i=0;
	bool afterMark = false;
	while( uint8 cc = _commandClasses[i++] )
	{
		if( cc == 0xef )
		{
			// COMMAND_CLASS_MARK.
			// Marks the end of the list of supported command classes.  The remaining classes
			// are those that can be controlled by this device, which we can ignore.
			afterMark = true;
			continue;
		}

		if( CommandClasses::IsSupported( cc ) )
        {
			if( CommandClass* commandClass = AddCommandClass( cc ) )
			{
				// If this class came after the COMMAND_CLASS_MARK, then we do not create values.
				if( afterMark )
				{
					commandClass->SetAfterMark();
				}

				// Start with an instance count of one.  If the device supports COMMMAND_CLASS_MULTI_INSTANCE
				// then some command class instance counts will increase.
				commandClass->SetInstance( 1 );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Node::ReadDeviceClasses>
// Read the static device class data from the device_classes.xml file
//-----------------------------------------------------------------------------
void Node::ReadDeviceClasses
(
)
{
	// Load the XML document that contains the device class information
	string configPath;
	Options::Get()->GetOptionAsString( "ConfigPath", &configPath );

	string filename =  configPath + string("device_classes.xml");

	TiXmlDocument doc;
	if( !doc.LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		Log::Write( LogLevel_Info, "Failed to load device_classes.xml" );
		Log::Write( LogLevel_Info, "Check that the config path provided when creating the Manager points to the correct location." );
		return;
	}

	TiXmlElement const* deviceClassesElement = doc.RootElement();

	// Read the basic and generic device classes
	TiXmlElement const* child = deviceClassesElement->FirstChildElement();
	while( child )
	{
		char const* str = child->Value();
		if( str )
		{
			char const* keyStr = child->Attribute( "key" );
			if( keyStr )
			{
				char* pStop;
				uint8 key = (uint8)strtol( keyStr, &pStop, 16 );

				if( !strcmp( str, "Generic" ) )
				{
					s_genericDeviceClasses[key] = new GenericDeviceClass( child );
				}
				else if( !strcmp( str, "Basic" ) )
				{
					char const* label = child->Attribute( "label" );
					if( label )
					{
						s_basicDeviceClasses[key] = label;
					}
				}
			}
		}

		child = child->NextSiblingElement();
	}

	s_deviceClassesLoaded = true;
}

//-----------------------------------------------------------------------------
// <Node::GetNoderStatistics>
// Return driver statistics
//-----------------------------------------------------------------------------
void Node::GetNodeStatistics
(
	NodeData* _data
)
{
	_data->m_sentCnt = m_sentCnt;
	_data->m_sentFailed = m_sentFailed;
	_data->m_retries = m_retries;
	_data->m_receivedCnt = m_receivedCnt;
	_data->m_receivedDups = m_receivedDups;
	_data->m_receivedUnsolicited = m_receivedUnsolicited;
	_data->m_lastRequestRTT = m_lastRequestRTT;
	_data->m_lastResponseRTT = m_lastResponseRTT;
	_data->m_sentTS = m_sentTS.GetAsString();
	_data->m_receivedTS = m_receivedTS.GetAsString();
	_data->m_averageRequestRTT = m_averageRequestRTT;
	_data->m_averageResponseRTT = m_averageResponseRTT;
	_data->m_quality = m_quality;
	memcpy( _data->m_lastReceivedMessage, m_lastReceivedMessage, sizeof(m_lastReceivedMessage) );
	for( map<uint8,CommandClass*>::const_iterator it = m_commandClassMap.begin(); it != m_commandClassMap.end(); ++it )
	{
		CommandClassData ccData;
		ccData.m_commandClassId = it->second->GetCommandClassId();
		ccData.m_sentCnt = it->second->GetSentCnt();
		ccData.m_receivedCnt = it->second->GetReceivedCnt();
		_data->m_ccData.push_back( ccData );
	}
}

//-----------------------------------------------------------------------------
// <DeviceClass::DeviceClass>
// Constructor
//-----------------------------------------------------------------------------
Node::DeviceClass::DeviceClass
(
	TiXmlElement const* _el
):
	m_mandatoryCommandClasses(NULL),
	m_basicMapping(0)
{
	char const* str = _el->Attribute( "label" );
	if( str )
	{
		m_label = str;
	}

	str = _el->Attribute( "command_classes" );
	if( str )
	{
		// Parse the comma delimted command class
		// list into a temporary vector.
		vector<uint8> ccs;
		char* pos = const_cast<char*>(str);
		while( *pos )
		{
			ccs.push_back( (uint8)strtol( pos, &pos, 16 ) );
			if( (*pos) == ',' )
			{
				++pos;
			}
		}

		// Copy the vector contents into an array.
		size_t numCCs = ccs.size();
		m_mandatoryCommandClasses = new uint8[numCCs+1];
		m_mandatoryCommandClasses[numCCs] = 0;	// Zero terminator

		for( uint32 i=0; i<numCCs; ++i )
		{
			m_mandatoryCommandClasses[i] = ccs[i];
		}
	}

	str = _el->Attribute( "basic" );
	if( str )
	{
		char* pStop;
		m_basicMapping = (uint8)strtol( str, &pStop, 16 );
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::GenericDeviceClass>
// Constructor
//-----------------------------------------------------------------------------
Node::GenericDeviceClass::GenericDeviceClass
(
	TiXmlElement const* _el
):
	DeviceClass( _el )
{
	// Add any specific device classes
	TiXmlElement const* child = _el->FirstChildElement();
	while( child )
	{
		char const* str = child->Value();
		if( str && !strcmp( str, "Specific" ) )
		{
			char const* keyStr = child->Attribute( "key" );
			if( keyStr )
			{
				char* pStop;
				uint8 key = (uint8)strtol( keyStr, &pStop, 16 );

				m_specificDeviceClasses[key] = new DeviceClass( child );
			}
		}

		child = child->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::~GenericDeviceClass>
// Destructor
//-----------------------------------------------------------------------------
Node::GenericDeviceClass::~GenericDeviceClass
(
)
{
	while( !m_specificDeviceClasses.empty() )
	{
		map<uint8,DeviceClass*>::iterator it = m_specificDeviceClasses.begin();
		delete it->second;
		m_specificDeviceClasses.erase( it );
	}
}

//-----------------------------------------------------------------------------
// <Node::GenericDeviceClass::GetSpecificDeviceClass>
// Get a specific device class object
//-----------------------------------------------------------------------------
Node::DeviceClass* Node::GenericDeviceClass::GetSpecificDeviceClass
(
	uint8 const& _specific
)
{
	map<uint8,DeviceClass*>::iterator it = m_specificDeviceClasses.find( _specific );
	if( it != m_specificDeviceClasses.end() )
	{
		return it->second;
	}

	return NULL;
}
