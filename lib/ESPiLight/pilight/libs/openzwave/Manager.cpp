//-----------------------------------------------------------------------------
//
//	Manager.cpp
//
//	The main public interface to OpenZWave.
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

#include <algorithm>
#include <string>
#include <sstream>

#include "Defs.h"
#include "Manager.h"
#include "Driver.h"
#include "Node.h"
#include "Notification.h"
#include "Options.h"
#include "Scene.h"

#include "platform/Mutex.h"
#include "platform/Event.h"
#include "platform/Log.h"

#include "command_classes/CommandClasses.h"
#include "command_classes/CommandClass.h"
#include "command_classes/WakeUp.h"

#include "value_classes/ValueID.h"
#include "value_classes/ValueBool.h"
#include "value_classes/ValueButton.h"
#include "value_classes/ValueByte.h"
#include "value_classes/ValueDecimal.h"
#include "value_classes/ValueInt.h"
#include "value_classes/ValueList.h"
#include "value_classes/ValueRaw.h"
#include "value_classes/ValueSchedule.h"
#include "value_classes/ValueShort.h"
#include "value_classes/ValueString.h"

using namespace OpenZWave;

Manager* Manager::s_instance = NULL;
extern uint16_t ozw_vers_major;
extern uint16_t ozw_vers_minor;
extern uint16_t ozw_vers_revision;

//-----------------------------------------------------------------------------
//	Construction
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	<Manager::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
Manager* Manager::Create
(
)
{
	if( Options::Get() && Options::Get()->AreLocked() )
	{
		if( NULL == s_instance )
		{
			s_instance = new Manager();
		}
		return s_instance;
	}

	// Options have not been created and locked.
	Log::Create( "", false, true, LogLevel_Debug, LogLevel_Debug, LogLevel_None );
	Log::Write( LogLevel_Error, "Options have not been created and locked. Exiting..." );
	exit(1);
	return NULL;
}

//-----------------------------------------------------------------------------
//	<Manager::Destroy>
//	Static method to destroy the singleton.
//-----------------------------------------------------------------------------
void Manager::Destroy
(
)
{
	delete s_instance;
	s_instance = NULL;
}

//-----------------------------------------------------------------------------
//	<Manager::getVersion>
//	Static method to get the Version of OZW as a string.
//-----------------------------------------------------------------------------
std::string Manager::getVersionAsString() {
	std::ostringstream versionstream;
	versionstream << ozw_vers_major << "." << ozw_vers_minor << "." << ozw_vers_revision;
	return versionstream.str();
}
//-----------------------------------------------------------------------------
//	<Manager::getVersion>
//	Static method to get the Version of OZW.
//-----------------------------------------------------------------------------
ozwversion Manager::getVersion() {
	return version(ozw_vers_major, ozw_vers_minor);
}

//-----------------------------------------------------------------------------
// <Manager::Manager>
// Constructor
//-----------------------------------------------------------------------------
Manager::Manager
(
):
	m_notificationMutex( new Mutex() )
{
	// Ensure the singleton instance is set
	s_instance = this;

	// Create the log file (if enabled)
	bool logging = false;
	Options::Get()->GetOptionAsBool( "Logging", &logging );

	string userPath = "";
	Options::Get()->GetOptionAsString( "UserPath", &userPath );

	string logFileNameBase = "OZW_Log.txt";
	Options::Get()->GetOptionAsString( "LogFileName", &logFileNameBase );

	bool bAppend = false;
	Options::Get()->GetOptionAsBool( "AppendLogFile", &bAppend );

	bool bConsoleOutput = true;
	Options::Get()->GetOptionAsBool( "ConsoleOutput", &bConsoleOutput );

	int nSaveLogLevel = (int) LogLevel_Detail;
	Options::Get()->GetOptionAsInt( "SaveLogLevel", &nSaveLogLevel );

	int nQueueLogLevel = (int) LogLevel_Debug;
	Options::Get()->GetOptionAsInt( "QueueLogLevel", &nQueueLogLevel );

	int nDumpTrigger = (int) LogLevel_Warning;
	Options::Get()->GetOptionAsInt( "DumpTriggerLevel", &nDumpTrigger );

	string logFilename = userPath + logFileNameBase;
	Log::Create( logFilename, bAppend, bConsoleOutput, (LogLevel) nSaveLogLevel, (LogLevel) nQueueLogLevel, (LogLevel) nDumpTrigger );
	Log::SetLoggingState( logging );

	CommandClasses::RegisterCommandClasses();
	Scene::ReadScenes();
	Log::Write(LogLevel_Always, "OpenZwave Version %s Starting Up", getVersionAsString().c_str());
}

//-----------------------------------------------------------------------------
// <Manager::Manager>
// Destructor
//-----------------------------------------------------------------------------
Manager::~Manager
(
)
{
	// Clear the pending list
	while( !m_pendingDrivers.empty() )
	{
		list<Driver*>::iterator it = m_pendingDrivers.begin();
		delete *it;
		m_pendingDrivers.erase( it );
	}

	// Clear the ready map
	while( !m_readyDrivers.empty() )
	{
		map<uint32,Driver*>::iterator it = m_readyDrivers.begin();
		delete it->second;
		m_readyDrivers.erase( it );
	}

	m_notificationMutex->Release();

	// Clear the watchers list
	while( !m_watchers.empty() )
	{
		list<Watcher*>::iterator it = m_watchers.begin();
		delete *it;
		m_watchers.erase( it );
	}

	// Clear the generic device class list
	while( !Node::s_genericDeviceClasses.empty() )
	{
		map<uint8,Node::GenericDeviceClass*>::iterator git = Node::s_genericDeviceClasses.begin();
		delete git->second;
		Node::s_genericDeviceClasses.erase( git );
	}

	Log::Destroy();
}

//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::WriteConfig>
// Save the configuration of a driver to a file
//-----------------------------------------------------------------------------
void Manager::WriteConfig
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->WriteConfig();
		Log::Write( LogLevel_Info, "mgr,     Manager::WriteConfig completed for driver with home ID of 0x%.8x", _homeId );
	}
	else
	{
		Log::Write( LogLevel_Info, "mgr,     Manager::WriteConfig failed - _homeId %d not found", _homeId );
	}
	Scene::WriteXML( "zwscene.xml" );
}

//-----------------------------------------------------------------------------
//	Drivers
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::AddDriver>
// Add a new Z-Wave PC Interface
//-----------------------------------------------------------------------------
bool Manager::AddDriver
(
	string const& _controllerPath,
	Driver::ControllerInterface const& _interface
)
{
	// Make sure we don't already have a driver for this controller

	// Search the pending list
	for( list<Driver*>::iterator pit = m_pendingDrivers.begin(); pit != m_pendingDrivers.end(); ++pit )
	{
		if( _controllerPath == (*pit)->GetControllerPath() )
		{
			Log::Write( LogLevel_Info, "mgr,     Cannot add driver for controller %s - driver already exists", _controllerPath.c_str() );
			return false;
		}
	}

	// Search the ready map
	for( map<uint32,Driver*>::iterator rit = m_readyDrivers.begin(); rit != m_readyDrivers.end(); ++rit )
	{
		if( _controllerPath == rit->second->GetControllerPath() )
		{
			Log::Write( LogLevel_Info, "mgr,     Cannot add driver for controller %s - driver already exists", _controllerPath.c_str() );
			return false;
		}
	}

	Driver* driver = new Driver( _controllerPath, _interface );
	m_pendingDrivers.push_back( driver );
	driver->Start();

	Log::Write( LogLevel_Info, "mgr,     Added driver for controller %s", _controllerPath.c_str() );
	return true;
}

//-----------------------------------------------------------------------------
// <Manager::RemoveDriver>
// Remove a Z-Wave PC Interface
//-----------------------------------------------------------------------------
bool Manager::RemoveDriver
(
	string const& _controllerPath
)
{
	// Search the pending list
	for( list<Driver*>::iterator pit = m_pendingDrivers.begin(); pit != m_pendingDrivers.end(); ++pit )
	{
		if( _controllerPath == (*pit)->GetControllerPath() )
		{
			delete *pit;
			m_pendingDrivers.erase( pit );
			Log::Write( LogLevel_Info, "mgr,     Driver for controller %s removed", _controllerPath.c_str() );
			return true;
		}
	}

	// Search the ready map
	for( map<uint32,Driver*>::iterator rit = m_readyDrivers.begin(); rit != m_readyDrivers.end(); ++rit )
	{
		if( _controllerPath == rit->second->GetControllerPath() )
		{
			/* data race right here:
			 * Before, we were deleting the Driver Class direct from the Map... this was causing a datarace:
			 * 1) Driver::~Driver destructor starts deleting everything....
			 * 2) This Triggers Notifications such as ValueDeleted etc
			 * 3) Notifications are delivered to applications, and applications start calling
			 *    Manager Functions which require the Driver (such as IsPolled(valueid))
			 * 4) Manager looks up the Driver in the m_readyDriver and returns a pointer to the Driver Class
			 *    which is currently being destructed.
			 * 5) All Hell Breaks loose and we crash and burn.
			 *
			 * But we can't change this, as the Driver Destructor triggers internal GetDriver calls... which
			 * will crash and burn if they can't get a valid Driver back...
			 */
			Log::Write( LogLevel_Info, "mgr,     Driver for controller %s pending removal", _controllerPath.c_str() );
			delete rit->second;
			m_readyDrivers.erase( rit );
			Log::Write( LogLevel_Info, "mgr,     Driver for controller %s removed", _controllerPath.c_str() );
			return true;
		}
	}

	Log::Write( LogLevel_Info, "mgr,     Failed to remove driver for controller %s", _controllerPath.c_str() );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::GetDriver>
// Get a pointer to the driver for a Z-Wave PC Interface
//-----------------------------------------------------------------------------
Driver* Manager::GetDriver
(
	uint32 const _homeId
)
{
	map<uint32,Driver*>::iterator it = m_readyDrivers.find( _homeId );
	if( it != m_readyDrivers.end() )
	{
		return it->second;
	}

	Log::Write( LogLevel_Error, "mgr,     Manager::GetDriver failed - Home ID 0x%.8x is unknown", _homeId );
	//assert(0); << Don't assert as this might be a valid condition when we call RemoveDriver. See comments above.
	return NULL;
}

//-----------------------------------------------------------------------------
// <Manager::SetDriverReady>
// Move a driver from pending to ready, and notify any watchers
//-----------------------------------------------------------------------------
void Manager::SetDriverReady
(
	Driver* _driver,
	bool success
)
{
	// Search the pending list
	bool found = false;
	for( list<Driver*>::iterator it = m_pendingDrivers.begin(); it != m_pendingDrivers.end(); ++it )
	{
		if( (*it) == _driver )
		{
			// Remove the driver from the pending list
			m_pendingDrivers.erase( it );
			found = true;
			break;
		}
	}

	if( found )
	{
		if (success) {
			Log::Write( LogLevel_Info, "mgr,     Driver with Home ID of 0x%.8x is now ready.", _driver->GetHomeId() );
			Log::Write( LogLevel_Info, "" );
		}

		// Add the driver to the ready map
		m_readyDrivers[_driver->GetHomeId()] = _driver;

		// Notify the watchers
		Notification* notification = new Notification(success ? Notification::Type_DriverReady : Notification::Type_DriverFailed );
		notification->SetHomeAndNodeIds( _driver->GetHomeId(), _driver->GetNodeId() );
		_driver->QueueNotification( notification );
	}
}

//-----------------------------------------------------------------------------
// <Manager::GetControllerNodeId>
//
//-----------------------------------------------------------------------------
uint8 Manager::GetControllerNodeId
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeId();
	}

	Log::Write( LogLevel_Info, "mgr,     GetControllerNodeId() failed - _homeId %d not found", _homeId );
	return 0xff;
}

//-----------------------------------------------------------------------------
// <Manager::GetSUCNodeId>
//
//-----------------------------------------------------------------------------
uint8 Manager::GetSUCNodeId
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetSUCNodeId();
	}

	Log::Write( LogLevel_Info, "mgr,     GetSUCNodeId() failed - _homeId %d not found", _homeId );
	return 0xff;
}

//-----------------------------------------------------------------------------
// <Manager::IsPrimaryController>
//
//-----------------------------------------------------------------------------
bool Manager::IsPrimaryController
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->IsPrimaryController();
	}

	Log::Write( LogLevel_Info, "mgr,     IsPrimaryController() failed - _homeId %d not found", _homeId );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::IsStaticUpdateController>
//
//-----------------------------------------------------------------------------
bool Manager::IsStaticUpdateController
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->IsStaticUpdateController();
	}

	Log::Write( LogLevel_Info, "mgr,     IsStaticUpdateController() failed - _homeId %d not found", _homeId );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::IsBridgeController>
//
//-----------------------------------------------------------------------------
bool Manager::IsBridgeController
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->IsBridgeController();
	}

	Log::Write( LogLevel_Info, "mgr,     IsBridgeController() failed - _homeId %d not found", _homeId );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::GetLibraryVersion>
//
//-----------------------------------------------------------------------------
string Manager::GetLibraryVersion
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetLibraryVersion();
	}

	Log::Write( LogLevel_Info, "mgr,     GetLibraryVersion() failed - _homeId %d not found", _homeId );
	return "";
}

//-----------------------------------------------------------------------------
// <Manager::GetLibraryTypeName>
//
//-----------------------------------------------------------------------------
string Manager::GetLibraryTypeName
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetLibraryTypeName();
	}

	Log::Write( LogLevel_Info, "mgr,     GetLibraryTypeName() failed - _homeId %d not found", _homeId );
	return "";
}

//-----------------------------------------------------------------------------
// <Manager::GetSendQueueCount>
//
//-----------------------------------------------------------------------------
int32 Manager::GetSendQueueCount
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetSendQueueCount();
	}

	Log::Write( LogLevel_Info, "mgr,     GetSendQueueCount() failed - _homeId %d not found", _homeId );
	return -1;
}

//-----------------------------------------------------------------------------
// <Manager::LogDriverStatistics>
// Send driver statistics to the log file
//-----------------------------------------------------------------------------
void Manager::LogDriverStatistics
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->LogDriverStatistics();
	}

	Log::Write( LogLevel_Warning, "mgr,     LogDriverStatistics() failed - _homeId %d not found", _homeId );
}

//-----------------------------------------------------------------------------
// <Manager::GetControllerInterfaceType>
// Retrieve controller interface type
//-----------------------------------------------------------------------------
Driver::ControllerInterface Manager::GetControllerInterfaceType
(
	uint32 const _homeId
)
{
	Driver::ControllerInterface ifType = Driver::ControllerInterface_Unknown;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		ifType = driver->GetControllerInterfaceType();
	}
	return ifType;
}

//-----------------------------------------------------------------------------
// <Manager::GetControllerPath>
// Retrieve controller interface path
//-----------------------------------------------------------------------------
string Manager::GetControllerPath
(
	uint32 const _homeId
)
{
	string path = "";
	if( Driver* driver = GetDriver( _homeId ) )
	{
		path = driver->GetControllerPath();
	}
	return path;
}
//-----------------------------------------------------------------------------
//	Polling Z-Wave values
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::GetPollInterval>
// Return the polling interval
//-----------------------------------------------------------------------------
int32 Manager::GetPollInterval
(
)
{
	for( map<uint32,Driver*>::iterator rit = m_readyDrivers.begin(); rit != m_readyDrivers.end(); ++rit )
	{
		return rit->second->GetPollInterval();
	}
	for( list<Driver*>::iterator pit = m_pendingDrivers.begin(); pit != m_pendingDrivers.end(); ++pit )
	{
		return (*pit)->GetPollInterval();
	}
	return 0;

}

//-----------------------------------------------------------------------------
// <Manager::SetPollInterval>
// Set the polling interval on all drivers
//-----------------------------------------------------------------------------
void Manager::SetPollInterval
(
	int32 _milliseconds,
	bool _bIntervalBetweenPolls
)
{
	for( list<Driver*>::iterator pit = m_pendingDrivers.begin(); pit != m_pendingDrivers.end(); ++pit )
	{
		(*pit)->SetPollInterval( _milliseconds, _bIntervalBetweenPolls );
	}

	for( map<uint32,Driver*>::iterator rit = m_readyDrivers.begin(); rit != m_readyDrivers.end(); ++rit )
	{
		rit->second->SetPollInterval( _milliseconds, _bIntervalBetweenPolls );
	}
}

//-----------------------------------------------------------------------------
// <Manager::EnablePoll>
// Enable polling of a value
//-----------------------------------------------------------------------------
bool Manager::EnablePoll
(
	ValueID const &_valueId,
	uint8 const _intensity
)
{
	if( Driver* driver = GetDriver( _valueId.GetHomeId() ) )
	{
		return( driver->EnablePoll( _valueId, _intensity ) );
	}

	Log::Write( LogLevel_Info, "mgr,     EnablePoll failed - Driver with Home ID 0x%.8x is not available", _valueId.GetHomeId() );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::DisablePoll>
// Disable polling of a value
//-----------------------------------------------------------------------------
bool Manager::DisablePoll
(
	ValueID const &_valueId
)
{
	if( Driver* driver = GetDriver( _valueId.GetHomeId() ) )
	{
		return( driver->DisablePoll( _valueId ) );
	}

	Log::Write( LogLevel_Info, "mgr,     DisablePoll failed - Driver with Home ID 0x%.8x is not available", _valueId.GetHomeId() );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::isPolled>
// Check polling status of a value
//-----------------------------------------------------------------------------
bool Manager::isPolled
(
	ValueID const &_valueId
)
{
	if( Driver* driver = GetDriver( _valueId.GetHomeId() ) )
	{
		return( driver->isPolled( _valueId ) );
	}

	Log::Write( LogLevel_Info, "mgr,     isPolled failed - Driver with Home ID 0x%.8x is not available", _valueId.GetHomeId() );
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetPollIntensity>
// Change the intensity with which this value is polled
//-----------------------------------------------------------------------------
void Manager::SetPollIntensity
(
	ValueID const &_valueId,
	uint8 const _intensity
)
{
	if( Driver* driver = GetDriver( _valueId.GetHomeId() ) )
	{
		return( driver->SetPollIntensity( _valueId, _intensity ) );
	}

	Log::Write( LogLevel_Error, "mgr,     SetPollIntensity failed - Driver with Home ID 0x%.8x is not available", _valueId.GetHomeId() );
}

//-----------------------------------------------------------------------------
// <Manager::GetPollIntensity>
// Change the intensity with which this value is polled
//-----------------------------------------------------------------------------
uint8 Manager::GetPollIntensity
(
	ValueID const &_valueId
)
{
	uint8 intensity = 0;
	if( Driver* driver = GetDriver( _valueId.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _valueId ) )
		{
			intensity = value->GetPollIntensity();
			value->Release();
		}
		driver->ReleaseNodes();
	}

 	return intensity;
}

//-----------------------------------------------------------------------------
//	Retrieving Node information
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::RefreshNodeInfo>
// Fetch the data for a node from the Z-Wave network
//-----------------------------------------------------------------------------
bool Manager::RefreshNodeInfo
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		// Cause the node's data to be obtained from the Z-Wave network
		// in the same way as if it had just been added.
		Node* node = driver->GetNode( _nodeId );
		if( node )
		{
			node->SetQueryStage( Node::QueryStage_ProtocolInfo );
			driver->ReleaseNodes();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Manager::RequestNodeState>
// Fetch the command class data for a node from the Z-Wave network
//-----------------------------------------------------------------------------
bool Manager::RequestNodeState
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		// Retreive the Node's session and dynamic data
		Node* node = driver->GetNode( _nodeId );
		if( node )
		{
			node->SetQueryStage( Node::QueryStage_Associations );
			driver->ReleaseNodes();
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::RequestNodeDynamic>
// Fetch only the dynamic command class data for a node from the Z-Wave network
//-----------------------------------------------------------------------------
bool Manager::RequestNodeDynamic
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		// Retreive the Node's dynamic data
		Node* node = driver->GetNode( _nodeId );
		if( node )
		{
			node->SetQueryStage( Node::QueryStage_Dynamic );
			driver->ReleaseNodes();
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::IsNodeListeningDevice>
// Get whether the node is a listening device that does not go to sleep
//-----------------------------------------------------------------------------
bool Manager::IsNodeListeningDevice
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	bool res = false;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		res = driver->IsNodeListeningDevice( _nodeId );
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::IsNodeFrequentListeningDevice>
// Get whether the node is a listening device that does not go to sleep
//-----------------------------------------------------------------------------
bool Manager::IsNodeFrequentListeningDevice
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	bool res = false;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		res = driver->IsNodeFrequentListeningDevice( _nodeId );
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::IsNodeBeamingDevice>
// Get whether the node is a beam capable device.
//-----------------------------------------------------------------------------
bool Manager::IsNodeBeamingDevice
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	bool res = false;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		res = driver->IsNodeBeamingDevice( _nodeId );
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::IsNodeRoutingDevice>
// Get whether the node is a routing device that passes messages to other nodes
//-----------------------------------------------------------------------------
bool Manager::IsNodeRoutingDevice
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	bool res = false;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		res = driver->IsNodeRoutingDevice( _nodeId );
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::IsNodeSecurityDevice>
// Get the security attribute for a node.
//-----------------------------------------------------------------------------
bool Manager::IsNodeSecurityDevice
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	bool security = 0;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		security = driver->IsNodeSecurityDevice( _nodeId );
	}

	return security;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeMaxBaudRate>
// Get the maximum baud rate of a node's communications
//-----------------------------------------------------------------------------
uint32 Manager::GetNodeMaxBaudRate
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	uint32 baud = 0;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		baud = driver->GetNodeMaxBaudRate( _nodeId );
	}

	return baud;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeVersion>
// Get the version number of a node
//-----------------------------------------------------------------------------
uint8 Manager::GetNodeVersion
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	uint8 version = 0;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		version = driver->GetNodeVersion( _nodeId );
	}

	return version;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeSecurity>
// Get the security byte of a node
//-----------------------------------------------------------------------------
uint8 Manager::GetNodeSecurity
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	uint8 version = 0;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		version = driver->GetNodeSecurity( _nodeId );
	}

	return version;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeBasic>
// Get the basic type of a node
//-----------------------------------------------------------------------------
uint8 Manager::GetNodeBasic
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	uint8 basic = 0;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		basic = driver->GetNodeBasic( _nodeId );
	}

	return basic;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeGeneric>
// Get the generic type of a node
//-----------------------------------------------------------------------------
uint8 Manager::GetNodeGeneric
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	uint8 genericType = 0;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		genericType = driver->GetNodeGeneric( _nodeId );
	}

	return genericType;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeSpecific>
// Get the specific type of a node
//-----------------------------------------------------------------------------
uint8 Manager::GetNodeSpecific
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	uint8 specific = 0;
	if( Driver* driver = GetDriver( _homeId ) )
	{
		specific = driver->GetNodeSpecific( _nodeId );
	}

	return specific;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeType>
// Get a string describing the type of a node
//-----------------------------------------------------------------------------
string Manager::GetNodeType
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeType( _nodeId );
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeNeighbors>
// Get the bitmap of this node's neighbors
//-----------------------------------------------------------------------------
uint32 Manager::GetNodeNeighbors
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8** o_neighbors
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeNeighbors( _nodeId, o_neighbors );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeManufacturerName>
// Get the manufacturer name of a node
//-----------------------------------------------------------------------------
string Manager::GetNodeManufacturerName
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeManufacturerName( _nodeId );
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeProductName>
// Get the product name of a node
//-----------------------------------------------------------------------------
string Manager::GetNodeProductName
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeProductName( _nodeId );
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeName>
// Get the user-editable name of a node
//-----------------------------------------------------------------------------
string Manager::GetNodeName
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeName( _nodeId );
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeLocation>
// Get the location of a node
//-----------------------------------------------------------------------------
string Manager::GetNodeLocation
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeLocation( _nodeId );
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Manager::SetNodeManufacturerName>
// Set the manufacturer name a node
//-----------------------------------------------------------------------------
void Manager::SetNodeManufacturerName
(
	uint32 const _homeId,
	uint8 const _nodeId,
	string const& _manufacturerName
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->SetNodeManufacturerName( _nodeId, _manufacturerName );
	}
}

//-----------------------------------------------------------------------------
// <Manager::SetNodeProductName>
// Set the product name of a node
//-----------------------------------------------------------------------------
void Manager::SetNodeProductName
(
	uint32 const _homeId,
	uint8 const _nodeId,
	string const& _productName
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->SetNodeProductName( _nodeId, _productName );
	}
}

//-----------------------------------------------------------------------------
// <Manager::SetNodeName>
// Set the node name value with the specified ID
//-----------------------------------------------------------------------------
void Manager::SetNodeName
(
	uint32 const _homeId,
	uint8 const _nodeId,
	string const& _nodeName
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->SetNodeName( _nodeId, _nodeName );
	}
}

//-----------------------------------------------------------------------------
// <Manager::SetNodeLocation>
// Set a string describing the location of a node
//-----------------------------------------------------------------------------
void Manager::SetNodeLocation
(
	uint32 const _homeId,
	uint8 const _nodeId,
	string const& _location

)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->SetNodeLocation( _nodeId, _location );
	}
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeManufacturerId>
// Get the manufacturer ID value of a node
//-----------------------------------------------------------------------------
string Manager::GetNodeManufacturerId
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeManufacturerId( _nodeId );
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeProductType>
// Get the product type value of a node
//-----------------------------------------------------------------------------
string Manager::GetNodeProductType
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeProductType( _nodeId );
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeProductId>
// Get the product Id value with the specified ID
//-----------------------------------------------------------------------------
string Manager::GetNodeProductId
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNodeProductId( _nodeId );
	}

	return "Unknown";
}

//-----------------------------------------------------------------------------
// <Manager::SetNodeOn>
// Helper method to turn a node on
//-----------------------------------------------------------------------------
void Manager::SetNodeOn
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->SetNodeOn( _nodeId );
	}
}

//-----------------------------------------------------------------------------
// <Manager::SetNodeOff>
// Helper method to turn a node off
//-----------------------------------------------------------------------------
void Manager::SetNodeOff
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->SetNodeOff( _nodeId );
	}
}

//-----------------------------------------------------------------------------
// <Manager::IsNodeInfoReceived>
// Helper method to return whether a particular class is available in a node
//-----------------------------------------------------------------------------
bool Manager::IsNodeInfoReceived
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	bool result = false;

	if( Driver* driver = GetDriver( _homeId ) )
	{
		Node *node;

		// Need to lock and unlock nodes to check this information
		driver->LockNodes();

		if( (node = driver->GetNodeUnsafe( _nodeId ) ) != NULL)
		{
			result = node->NodeInfoReceived();
		}

		driver->ReleaseNodes();
	}

	return result;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeClassAvailable>
// Helper method to return whether a particular class is available in a node
//-----------------------------------------------------------------------------
bool Manager::GetNodeClassInformation
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	string *_className,
	uint8 *_classVersion
)
{
	bool result = false;

	if( Driver* driver = GetDriver( _homeId ) )
	{
	        Node *node;

	        // Need to lock and unlock nodes to check this information
	        driver->LockNodes();

	        if( ( node = driver->GetNodeUnsafe( _nodeId ) ) != NULL )
	        {
			CommandClass *cc;
			if( node->NodeInfoReceived() && ( ( cc = node->GetCommandClass( _commandClassId ) ) != NULL ) )
			{
				if( _className )
				{
					*_className = cc->GetCommandClassName();
				}
				if( _classVersion )
				{
					*_classVersion = cc->GetVersion();
				}
				// Positive result
				result = true;
			}
		}

		driver->ReleaseNodes();
	}

	return result;
}

//-----------------------------------------------------------------------------
// <Manager::IsNodeAwake>
// Helper method to return whether a node is awake or sleeping
//-----------------------------------------------------------------------------
bool Manager::IsNodeAwake
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( IsNodeListeningDevice( _homeId, _nodeId ) )
	{
		return true;				// if listening then always awake
	}
	bool result = true;
	if( Driver* driver = GetDriver( _homeId ) )
	{
	        // Need to lock and unlock nodes to check this information
	        driver->LockNodes();

	        if( Node* node = driver->GetNodeUnsafe( _nodeId ) )
	        {
			if( WakeUp* wcc = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
			{
				result = wcc->IsAwake();
			}
		}
		driver->ReleaseNodes();
	}
	return result;
}

//-----------------------------------------------------------------------------
// <Manager::IsNodeFailed>
// Helper method to return whether a node is on the network or not
//-----------------------------------------------------------------------------
bool Manager::IsNodeFailed
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	bool result = false;
	if( Driver* driver = GetDriver( _homeId ) )
	{
	        if( Node* node = driver->GetNode( _nodeId ) )
	        {
			result = !node->IsNodeAlive();
        		driver->ReleaseNodes();
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
// <Manager::GetNodeQueryStage>
// Helper method to return whether a node's query stage
//-----------------------------------------------------------------------------
string Manager::GetNodeQueryStage
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	string result = "Unknown";
	if( Driver* driver = GetDriver( _homeId ) )
	{
	        if( Node* node = driver->GetNode( _nodeId ) )
	        {
			result = node->GetQueryStageName( node->GetCurrentQueryStage() );
        		driver->ReleaseNodes();
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
// <Manager::SetNodeLevel>
// Helper method to set the basic level of a node
//-----------------------------------------------------------------------------
void Manager::SetNodeLevel
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _level
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->SetNodeLevel( _nodeId, _level );
	}
}

//-----------------------------------------------------------------------------
//	Values
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::GetValueLabel>
// Gets the user-friendly label for the value
//-----------------------------------------------------------------------------
string Manager::GetValueLabel
(
	ValueID const& _id
)
{
	string label;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			label = value->GetLabel();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return label;
}

//-----------------------------------------------------------------------------
// <Manager::SetValueLabel>
// Sets the user-friendly label for the value
//-----------------------------------------------------------------------------
void Manager::SetValueLabel
(
	ValueID const& _id,
	string const& _value
)
{
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			value->SetLabel( _value );
			value->Release();
		}
		driver->ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Manager::GetValueUnits>
// Gets the units that the value is measured in
//-----------------------------------------------------------------------------
string Manager::GetValueUnits
(
	ValueID const& _id
)
{
	string units;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			units = value->GetUnits();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return units;
}

//-----------------------------------------------------------------------------
// <Manager::SetValueUnits>
// Sets the units that the value is measured in
//-----------------------------------------------------------------------------
void Manager::SetValueUnits
(
 	ValueID const& _id,
	string const& _value
)
{
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			value->SetUnits( _value );
			value->Release();
		}
		driver->ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Manager::GetValueHelp>
// Gets a help string describing the value's purpose and usage
//-----------------------------------------------------------------------------
string Manager::GetValueHelp
(
	ValueID const& _id
)
{
	string help;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			help = value->GetHelp();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return help;
}

//-----------------------------------------------------------------------------
// <Manager::SetValueHelp>
// Sets a help string describing the value's purpose and usage
//-----------------------------------------------------------------------------
void Manager::SetValueHelp
(
	ValueID const& _id,
	string const& _value
)
{
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			value->SetHelp( _value );
			value->Release();
		}
		driver->ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Manager::GetValueMin>
// Gets the minimum for a value
//-----------------------------------------------------------------------------
int32 Manager::GetValueMin
(
	ValueID const& _id
)
{
	int32 limit = 0;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			limit = value->GetMin();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return limit;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueMax>
// Gets the maximum for a value
//-----------------------------------------------------------------------------
int32 Manager::GetValueMax
(
	ValueID const& _id
)
{
	int32 limit = 0;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			limit = value->GetMax();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return limit;
}

//-----------------------------------------------------------------------------
// <Manager::IsValueReadOnly>
// Test whether the value is read-only
//-----------------------------------------------------------------------------
bool Manager::IsValueReadOnly
(
	ValueID const& _id
)
{
	bool res = false;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			res = value->IsReadOnly();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::IsValueWriteOnly>
// Test whether the value is write-only
//-----------------------------------------------------------------------------
bool Manager::IsValueWriteOnly
(
	ValueID const& _id
)
{
	bool res = false;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			res = value->IsWriteOnly();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::IsValueSet>
// Test whether the value has been set by a status message from the device
//-----------------------------------------------------------------------------
bool Manager::IsValueSet
(
	ValueID const& _id
)
{
	bool res = false;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			res = value->IsSet();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::IsValuePolled>
// Test whether the value is currently being polled
//-----------------------------------------------------------------------------
bool Manager::IsValuePolled
(
	ValueID const& _id
)
{
	bool res = false;
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			res = value->IsPolled();
			value->Release();
		}
		driver->ReleaseNodes();
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueAsBool>
// Gets a value as a bool
//-----------------------------------------------------------------------------
bool Manager::GetValueAsBool
(
	ValueID const& _id,
	bool* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_Bool == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueBool* value = static_cast<ValueBool*>( driver->GetValue( _id ) ) )
				{
					*o_value = value->GetValue();
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
		else if( ValueID::ValueType_Button == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
			    	driver->LockNodes();
				if( ValueButton* value = static_cast<ValueButton*>( driver->GetValue( _id ) ) )
				{
					*o_value = value->IsPressed();
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueAsByte>
// Gets a value as an 8-bit unsigned integer
//-----------------------------------------------------------------------------
bool Manager::GetValueAsByte
(
	ValueID const& _id,
	uint8* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_Byte == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueByte* value = static_cast<ValueByte*>( driver->GetValue( _id ) ) )
				{
					*o_value = value->GetValue();
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueAsFloat>
// Gets a value as a floating point number
//-----------------------------------------------------------------------------
bool Manager::GetValueAsFloat
(
	ValueID const& _id,
	float* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_Decimal == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueDecimal* value = static_cast<ValueDecimal*>( driver->GetValue( _id ) ) )
				{
					string str = value->GetValue();
					*o_value = (float)atof( str.c_str() );
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueAsInt>
// Gets a value as a 32-bit signed integer
//-----------------------------------------------------------------------------
bool Manager::GetValueAsInt
(
	ValueID const& _id,
	int32* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_Int == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueInt* value = static_cast<ValueInt*>( driver->GetValue( _id ) ) )
				{
					*o_value = value->GetValue();
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueAsRaw>
// Gets a value as a collection of bytes
//-----------------------------------------------------------------------------
bool Manager::GetValueAsRaw
(
	ValueID const& _id,
	uint8** o_value,
	uint8* o_length
)
{
	bool res = false;

	if( o_value && o_length )
	{
		if( ValueID::ValueType_Raw == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueRaw* value = static_cast<ValueRaw*>( driver->GetValue( _id ) ) )
				{
					*o_length = value->GetLength();
					*o_value = new uint8[*o_length];
					memcpy( *o_value, value->GetValue(), *o_length );
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueAsShort>
// Gets a value as a 16-bit signed integer
//-----------------------------------------------------------------------------
bool Manager::GetValueAsShort
(
	ValueID const& _id,
	int16* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_Short == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueShort* value = static_cast<ValueShort*>( driver->GetValue( _id ) ) )
				{
					*o_value = value->GetValue();
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueAsString>
// Creates a string representation of the value, regardless of type
//-----------------------------------------------------------------------------
bool Manager::GetValueAsString
(
	ValueID const& _id,
	string* o_value
)
{
	bool res = false;
	char str[256] = {0};

	if( o_value )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			driver->LockNodes();

			switch( _id.GetType() )
			{
				case ValueID::ValueType_Bool:
				{
					if( ValueBool* value = static_cast<ValueBool*>( driver->GetValue( _id ) ) )
					{
						*o_value = value->GetValue() ? "True" : "False";
						value->Release();
						res = true;
					}
					break;
				}
				case ValueID::ValueType_Byte:
				{
					if( ValueByte* value = static_cast<ValueByte*>( driver->GetValue( _id ) ) )
					{
						snprintf( str, sizeof(str), "%u", value->GetValue() );
						*o_value = str;
						value->Release();
						res = true;
					}
					break;
				}
				case ValueID::ValueType_Decimal:
				{
					if( ValueDecimal* value = static_cast<ValueDecimal*>( driver->GetValue( _id ) ) )
					{
						*o_value = value->GetValue();
						value->Release();
						res = true;
					}
					break;
				}
				case ValueID::ValueType_Int:
				{
					if( ValueInt* value = static_cast<ValueInt*>( driver->GetValue( _id ) ) )
					{
						snprintf( str, sizeof(str), "%d", value->GetValue() );
						*o_value = str;
						value->Release();
						res = true;
					}
					break;
				}
				case ValueID::ValueType_List:
				{
					if( ValueList* value = static_cast<ValueList*>( driver->GetValue( _id ) ) )
					{
						ValueList::Item const& item = value->GetItem();
						*o_value = item.m_label;
						value->Release();
						res = true;
					}
					break;
				}
				case ValueID::ValueType_Raw:
				{
					if( ValueRaw* value = static_cast<ValueRaw*>( driver->GetValue( _id ) ) )
					{
						*o_value = value->GetAsString();
						value->Release();
						res = true;
					}
					break;
				}
				case ValueID::ValueType_Short:
				{
					if( ValueShort* value = static_cast<ValueShort*>( driver->GetValue( _id ) ) )
					{
						snprintf( str, sizeof(str), "%d", value->GetValue() );
						*o_value = str;
						value->Release();
						res = true;
					}
					break;
				}
				case ValueID::ValueType_String:
				{
					if( ValueString* value = static_cast<ValueString*>( driver->GetValue( _id ) ) )
					{
						*o_value = value->GetValue();
						value->Release();
						res = true;
					}
					break;
				}
				case ValueID::ValueType_Button:
				{
					if( ValueButton* value = static_cast<ValueButton*>( driver->GetValue( _id ) ) )
					{
						*o_value = value->IsPressed() ? "True" : "False";
						value->Release();
						res = true;
					}
					break;
				}
				default:
				{
					// To keep GCC happy
					break;
				}
			}

			driver->ReleaseNodes();
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueListSelection>
// Gets the selected item from a list value (returning a string)
//-----------------------------------------------------------------------------
bool Manager::GetValueListSelection
(
	ValueID const& _id,
	string* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_List == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueList* value = static_cast<ValueList*>( driver->GetValue( _id ) ) )
				{
					ValueList::Item const& item = value->GetItem();
					if( &item != NULL )
					{
						*o_value = item.m_label;
						res = true;
					}
					value->Release();
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueListSelection>
// Gets the selected item from a list value (returning the index)
//-----------------------------------------------------------------------------
bool Manager::GetValueListSelection
(
	ValueID const& _id,
	int32* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_List == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueList* value = static_cast<ValueList*>( driver->GetValue( _id ) ) )
				{
					ValueList::Item const& item = value->GetItem();
					*o_value = item.m_value;
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueListItems>
// Gets the list of items from a list value
//-----------------------------------------------------------------------------
bool Manager::GetValueListItems
(
	ValueID const& _id,
	vector<string>* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_List == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueList* value = static_cast<ValueList*>( driver->GetValue( _id ) ) )
				{
					res = value->GetItemLabels( o_value );
					value->Release();
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::GetValueFloatPrecision>
// Gets a value's scale as a uint8
//-----------------------------------------------------------------------------
bool Manager::GetValueFloatPrecision
(
	ValueID const& _id,
	uint8* o_value
)
{
	bool res = false;

	if( o_value )
	{
		if( ValueID::ValueType_Decimal == _id.GetType() )
		{
			if( Driver* driver = GetDriver( _id.GetHomeId() ) )
			{
				driver->LockNodes();
				if( ValueDecimal* value = static_cast<ValueDecimal*>( driver->GetValue( _id ) ) )
				{
					*o_value = value->GetPrecision();
					value->Release();
					res = true;
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::SetValue>
// Sets the value from a bool
//-----------------------------------------------------------------------------
bool Manager::SetValue
(
	ValueID const& _id,
	bool const _value
)
{
	bool res = false;

	if( ValueID::ValueType_Bool == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			if( _id.GetNodeId() != driver->GetNodeId() )
			{
				driver->LockNodes();
				if( ValueBool* value = static_cast<ValueBool*>( driver->GetValue( _id ) ) )
				{
					res = value->Set( _value );
					value->Release();
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::SetValue>
// Sets the value from a byte
//-----------------------------------------------------------------------------
bool Manager::SetValue
(
	ValueID const& _id,
	uint8 const _value
)
{
	bool res = false;

	if( ValueID::ValueType_Byte == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			if( _id.GetNodeId() != driver->GetNodeId() )
			{
				driver->LockNodes();
				if( ValueByte* value = static_cast<ValueByte*>( driver->GetValue( _id ) ) )
				{
					res = value->Set( _value );
					value->Release();
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::SetValue>
// Sets the value from a floating point number
//-----------------------------------------------------------------------------
bool Manager::SetValue
(
	ValueID const& _id,
	float const _value
)
{
	bool res = false;

	if( ValueID::ValueType_Decimal == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			if( _id.GetNodeId() != driver->GetNodeId() )
			{
				driver->LockNodes();
				if( ValueDecimal* value = static_cast<ValueDecimal*>( driver->GetValue( _id ) ) )
				{
					char str[256];
					snprintf( str, sizeof(str), "%f", _value );

					// remove trailing zeros (and the decimal point, if present)
					// TODO: better way of figuring out which locale is being used ('.' or ',' to separate decimals)
					size_t nLen;
					if( ( strchr( str, '.' ) != NULL) || (strchr( str, ',' ) != NULL ) )
					{
						for( nLen = strlen( str ) - 1; nLen > 0; nLen-- )
						{
							if( str[nLen] == '0' )
								str[nLen] = 0;
							else
								break;
						}
						if( (str[nLen] == '.') || (str[nLen] == ',') )
							str[nLen] = 0;
					}

					// now set the value
					res = value->Set( str );
					value->Release();
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::SetValue>
// Sets the value from a 32-bit signed integer
//-----------------------------------------------------------------------------
bool Manager::SetValue
(
	ValueID const& _id,
	int32 const _value
)
{
	bool res = false;

	if( ValueID::ValueType_Int == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			if( _id.GetNodeId() != driver->GetNodeId() )
			{
				driver->LockNodes();
				if( ValueInt* value = static_cast<ValueInt*>( driver->GetValue( _id ) ) )
				{
					res = value->Set( _value );
					value->Release();
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::SetValue>
// Sets the value from a collection of bytes
//-----------------------------------------------------------------------------
bool Manager::SetValue
(
	ValueID const& _id,
	uint8 const* _value,
	uint8 const _length
)
{
	bool res = false;

	if( ValueID::ValueType_Raw == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			if( _id.GetNodeId() != driver->GetNodeId() )
			{
				driver->LockNodes();
				if( ValueRaw* value = static_cast<ValueRaw*>( driver->GetValue( _id ) ) )
				{
					res = value->Set( _value, _length );
					value->Release();
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::SetValue>
// Sets the value from a 16-bit signed integer
//-----------------------------------------------------------------------------
bool Manager::SetValue
(
	ValueID const& _id,
	int16 const _value
)
{
	bool res = false;

	if( ValueID::ValueType_Short == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			if( _id.GetNodeId() != driver->GetNodeId() )
			{
				driver->LockNodes();
				if( ValueShort* value = static_cast<ValueShort*>( driver->GetValue( _id ) ) )
				{
					res = value->Set( _value );
					value->Release();
				}
				driver->ReleaseNodes();
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::SetValueListSelection>
// Sets the selected item in a list by value
//-----------------------------------------------------------------------------
bool Manager::SetValueListSelection
(
	ValueID const& _id,
	string const& _selectedItem
)
{
	bool res = false;

	if( ValueID::ValueType_List == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			if( _id.GetNodeId() != driver->GetNodeId() )
			{
				driver->LockNodes();
				if( ValueList* value = static_cast<ValueList*>( driver->GetValue( _id ) ) )
				{
					res = value->SetByLabel( _selectedItem );
					value->Release();
				}
				driver->ReleaseNodes();
			}

		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::SetValue>
// Sets the value from a string
//-----------------------------------------------------------------------------
bool Manager::SetValue
(
	ValueID const& _id,
	string const& _value
)
{
	bool res = false;

	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		if( _id.GetNodeId() != driver->GetNodeId() )
		{
			driver->LockNodes();

			switch( _id.GetType() )
			{
				case ValueID::ValueType_Bool:
				{
					if( ValueBool* value = static_cast<ValueBool*>( driver->GetValue( _id ) ) )
					{
						if( !strcasecmp( "true", _value.c_str() ) )
						{
							res = value->Set( true );
						}
						else if( !strcasecmp( "false", _value.c_str() ) )
						{
							res = value->Set( false );
						}
						value->Release();
					}
					break;
				}
				case ValueID::ValueType_Byte:
				{
					if( ValueByte* value = static_cast<ValueByte*>( driver->GetValue( _id ) ) )
					{
						uint32 val = (uint32)atoi( _value.c_str() );
						if( val < 256 )
						{
							res = value->Set( (uint8)val );
						}
						value->Release();
					}
					break;
				}
				case ValueID::ValueType_Decimal:
				{
					if( ValueDecimal* value = static_cast<ValueDecimal*>( driver->GetValue( _id ) ) )
					{
						res = value->Set( _value );
						value->Release();
					}
					break;
				}
				case ValueID::ValueType_Int:
				{
					if( ValueInt* value = static_cast<ValueInt*>( driver->GetValue( _id ) ) )
					{
						int32 val = atoi( _value.c_str() );
						res = value->Set( val );
						value->Release();
					}
				break;
				}
				case ValueID::ValueType_List:
				{
					if( ValueList* value = static_cast<ValueList*>( driver->GetValue( _id ) ) )
					{
						res = value->SetByLabel( _value );
						value->Release();
					}
					break;
				}
				case ValueID::ValueType_Short:
				{
					if( ValueShort* value = static_cast<ValueShort*>( driver->GetValue( _id ) ) )
					{
						int32 val = (uint32)atoi( _value.c_str() );
						if( ( val < 32768 ) && ( val >= -32768 ) )
						{
							res = value->Set( (int16)val );
						}
						value->Release();
					}
					break;
				}
				case ValueID::ValueType_String:
				{
					if( ValueString* value = static_cast<ValueString*>( driver->GetValue( _id ) ) )
					{
						res = value->Set( _value );
						value->Release();
					}
					break;
				}
				case ValueID::ValueType_Raw:
				{
					if( ValueRaw* value = static_cast<ValueRaw*>( driver->GetValue( _id ) ) )
					{
						res = value->SetFromString( _value );
						value->Release();
					}
					break;
				}
				default:
				{
					// To keep GCC happy
					break;
				}
			}
			driver->ReleaseNodes();
		}
	}
	return res;
}

//-----------------------------------------------------------------------------
// <Manager::RefreshValue>
// Instruct the driver to refresh this value by sending a message to the device
//-----------------------------------------------------------------------------
bool Manager::RefreshValue
(
	ValueID const& _id
)
{
	bool bRet = false;	// return value

	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
	    Node *node;

	    // Need to lock and unlock nodes to check this information
	    driver->LockNodes();

	    if( (node = driver->GetNodeUnsafe( _id.GetNodeId() ) ) != NULL)
	    {
			CommandClass* cc = node->GetCommandClass( _id.GetCommandClassId() );
			if (cc) {
        			uint8 index = _id.GetIndex();
	        		uint8 instance = _id.GetInstance();
		        	Log::Write( LogLevel_Info, "mgr,     Refreshing node %d: %s index = %d instance = %d (to confirm a reported change)", node->m_nodeId, cc->GetCommandClassName().c_str(), index, instance );
		        	cc->RequestValue( 0, index, instance, Driver::MsgQueue_Send );
        			bRet = true;
                        } else {
                                bRet = false;
                        }
		}
		driver->ReleaseNodes();
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// <Manager::SetChangeVerified>
// Set the verify changes flag for the specified value
//-----------------------------------------------------------------------------
void Manager::SetChangeVerified
(
	ValueID const& _id,
	bool _verify
)
{
	if( Driver* driver = GetDriver( _id.GetHomeId() ) )
	{
		driver->LockNodes();
		if( Value* value = driver->GetValue( _id ) )
		{
			value->SetChangeVerified( _verify );
			value->Release();
		}
		driver->ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Manager::PressButton>
// Starts an activity in a device.
//-----------------------------------------------------------------------------
bool Manager::PressButton
(
	ValueID const& _id
)
{
	bool res = false;

	if( ValueID::ValueType_Button == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			driver->LockNodes();
			if( ValueButton* value = static_cast<ValueButton*>( driver->GetValue( _id ) ) )
			{
				res = value->PressButton();
				value->Release();
			}
			driver->ReleaseNodes();
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::ReleaseButton>
// Stops an activity in a device.
//-----------------------------------------------------------------------------
bool Manager::ReleaseButton
(
	ValueID const& _id
)
{
	bool res = false;

	if( ValueID::ValueType_Button == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			driver->LockNodes();
			if( ValueButton* value = static_cast<ValueButton*>( driver->GetValue( _id ) ) )
			{
				res = value->ReleaseButton();
				value->Release();
			}
			driver->ReleaseNodes();
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// Climate Control Schedules
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::GetNumSwitchPoints>
// Get the number of switch points defined in a schedule
//-----------------------------------------------------------------------------
uint8 Manager::GetNumSwitchPoints
(
	ValueID const& _id
)
{
//	bool res = false;

	uint8 numSwitchPoints = 0;
	if( ValueID::ValueType_Schedule == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			driver->LockNodes();
			if( ValueSchedule* value = static_cast<ValueSchedule*>( driver->GetValue( _id ) ) )
			{
				numSwitchPoints = value->GetNumSwitchPoints();
				value->Release();
			}
			driver->ReleaseNodes();
		}
	}

	return numSwitchPoints;
}

//-----------------------------------------------------------------------------
// <Manager::SetSwitchPoint>
// Set a switch point in the schedule
//-----------------------------------------------------------------------------
bool Manager::SetSwitchPoint
(
	ValueID const& _id,
	uint8 const _hours,
	uint8 const _minutes,
	int8 const _setback
)
{
	bool res = false;

	if( ValueID::ValueType_Schedule == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			driver->LockNodes();
			if( ValueSchedule* value = static_cast<ValueSchedule*>( driver->GetValue( _id ) ) )
			{
				res = value->SetSwitchPoint( _hours, _minutes, _setback );
				value->Release();
			}
			driver->ReleaseNodes();
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::RemoveSwitchPoint>
// Remove a switch point from the schedule
//-----------------------------------------------------------------------------
bool Manager::RemoveSwitchPoint
(
	ValueID const& _id,
	uint8 const _hours,
	uint8 const _minutes
)
{
	bool res = false;

	if( ValueID::ValueType_Schedule == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			driver->LockNodes();
			if( ValueSchedule* value = static_cast<ValueSchedule*>( driver->GetValue( _id ) ) )
			{
				uint8 idx;
				res = value->FindSwitchPoint( _hours, _minutes, &idx );

				if( res )
				{
					res = value->RemoveSwitchPoint( idx );
				}
				value->Release();
			}
			driver->ReleaseNodes();
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Manager::ClearSwitchPoints>
// Clears all switch points from the schedule
//-----------------------------------------------------------------------------
void Manager::ClearSwitchPoints
(
	ValueID const& _id
)
{
	if( ValueID::ValueType_Schedule == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			driver->LockNodes();
			if( ValueSchedule* value = static_cast<ValueSchedule*>( driver->GetValue( _id ) ) )
			{
				value->ClearSwitchPoints();
				value->Release();
			}
			driver->ReleaseNodes();
		}
	}
}

//-----------------------------------------------------------------------------
// <Manager::GetSwitchPoint>
// Gets switch point data from the schedule
//-----------------------------------------------------------------------------
bool Manager::GetSwitchPoint
(
	ValueID const& _id,
	uint8 const _idx,
	uint8* o_hours,
	uint8* o_minutes,
	int8* o_setback
)
{
	bool res = false;

	if( ValueID::ValueType_Schedule == _id.GetType() )
	{
		if( Driver* driver = GetDriver( _id.GetHomeId() ) )
		{
			driver->LockNodes();
			if( ValueSchedule* value = static_cast<ValueSchedule*>( driver->GetValue( _id ) ) )
			{
				res = value->GetSwitchPoint( _idx, o_hours, o_minutes, o_setback );
				value->Release();
			}
			driver->ReleaseNodes();
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
//	SwitchAll
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::SwitchAllOn>
// All devices that support the SwitchAll command class will be turned on
//-----------------------------------------------------------------------------
void Manager::SwitchAllOn
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->SwitchAllOn();
	}
}

//-----------------------------------------------------------------------------
// <Manager::SwitchAllOff>
// All devices that support the SwitchAll command class will be turned off
//-----------------------------------------------------------------------------
void Manager::SwitchAllOff
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->SwitchAllOff();
	}
}

//-----------------------------------------------------------------------------
//	Configuration Parameters
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::SetConfigParam>
// Set the value of one of the configuration parameters of a device
//-----------------------------------------------------------------------------
bool Manager::SetConfigParam
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _param,
	int32 _value,
	uint8 const _size
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->SetConfigParam( _nodeId, _param, _value, _size );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Manager::RequestConfigParam>
// Request the value of one of the configuration parameters of a device
//-----------------------------------------------------------------------------
void Manager::RequestConfigParam
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _param
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->RequestConfigParam( _nodeId, _param );
	}
}

//-----------------------------------------------------------------------------
// <Manager::RequestAllConfigParams>
// Request the values of all of the known configuration parameters of a device
//-----------------------------------------------------------------------------
void Manager::RequestAllConfigParams
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		Node* node = driver->GetNode( _nodeId );
		if( node )
		{
			node->SetQueryStage( Node::QueryStage_Configuration );
			driver->ReleaseNodes();
		}
	}
}

//-----------------------------------------------------------------------------
//	Groups
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::GetNumGroups>
// Gets the number of association groups reported by this node
//-----------------------------------------------------------------------------
uint8 Manager::GetNumGroups
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetNumGroups( _nodeId );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// <Manager::GetAssociations>
// Gets the associations for a group
//-----------------------------------------------------------------------------
uint32 Manager::GetAssociations
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _groupIdx,
	uint8** o_associations
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetAssociations( _nodeId, _groupIdx, o_associations );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// <Manager::GetMaxAssociations>
// Gets the maximum number of associations for a group
//-----------------------------------------------------------------------------
uint8 Manager::GetMaxAssociations
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _groupIdx
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetMaxAssociations( _nodeId, _groupIdx );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// <Manager::GetGroupLabel>
// Gets the label for a particular group
//-----------------------------------------------------------------------------
string Manager::GetGroupLabel
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _groupIdx
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->GetGroupLabel( _nodeId, _groupIdx );
	}

	return "";
}

//-----------------------------------------------------------------------------
// <Manager::AddAssociation>
// Adds a node to an association group
//-----------------------------------------------------------------------------
void Manager::AddAssociation
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _groupIdx,
	uint8 const _targetNodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->AddAssociation( _nodeId, _groupIdx, _targetNodeId );
	}
}

//-----------------------------------------------------------------------------
// <Manager::RemoveAssociation>
// Removes a node from an association group
//-----------------------------------------------------------------------------
void Manager::RemoveAssociation
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _groupIdx,
	uint8 const _targetNodeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->RemoveAssociation( _nodeId, _groupIdx, _targetNodeId );
	}
}


//-----------------------------------------------------------------------------
//	Notifications
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::AddWatcher>
// Add a watcher to the list
//-----------------------------------------------------------------------------
bool Manager::AddWatcher
(
	pfnOnNotification_t _watcher,
	void* _context
)
{
	// Ensure this watcher is not already on the list
	m_notificationMutex->Lock();
	for( list<Watcher*>::iterator it = m_watchers.begin(); it != m_watchers.end(); ++it )
	{
		if( ((*it)->m_callback == _watcher ) && ( (*it)->m_context == _context ) )
		{
			// Already in the list
			m_notificationMutex->Unlock();
			return false;
		}
	}

	m_watchers.push_back( new Watcher( _watcher, _context ) );
	m_notificationMutex->Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// <Manager::RemoveWatcher>
// Remove a watcher from the list
//-----------------------------------------------------------------------------
bool Manager::RemoveWatcher
(
	pfnOnNotification_t _watcher,
	void* _context
)
{
	m_notificationMutex->Lock();
	list<Watcher*>::iterator it = m_watchers.begin();
	while( it != m_watchers.end() )
	{
		if( ((*it)->m_callback == _watcher ) && ( (*it)->m_context == _context ) )
		{
			delete (*it);
			m_watchers.erase( it );
			m_notificationMutex->Unlock();
			return true;
		}
		++it;
	}

	m_notificationMutex->Unlock();
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::NotifyWatchers>
// Notify any watching objects of a value change
//-----------------------------------------------------------------------------
void Manager::NotifyWatchers
(
	Notification* _notification
)
{
	m_notificationMutex->Lock();
	for( list<Watcher*>::iterator it = m_watchers.begin(); it != m_watchers.end(); ++it )
	{
		Watcher* pWatcher = *it;
		pWatcher->m_callback( _notification, pWatcher->m_context );
	}
	m_notificationMutex->Unlock();
}

//-----------------------------------------------------------------------------
//	Controller commands
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Manager::ResetController>
// Reset controller and erase all node information
//-----------------------------------------------------------------------------
void Manager::ResetController
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		Event *event = new Event();
		driver->ResetController( event );
		Wait::Single( event );
		event->Release();
		string path = driver->GetControllerPath();
		Driver::ControllerInterface intf = driver->GetControllerInterfaceType();
		RemoveDriver( path );
		AddDriver( path, intf );
		Wait::Multiple( NULL, 0, 500 );
	}
	RemoveAllScenes( _homeId );
}

//-----------------------------------------------------------------------------
// <Manager::SoftReset>
// Soft-reset the Z-Wave controller chip
//-----------------------------------------------------------------------------
void Manager::SoftReset
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->SoftReset();
	}
}

//-----------------------------------------------------------------------------
// <Manager::BeginControllerCommand>
// Start the controller performing one of its network management functions
//-----------------------------------------------------------------------------
bool Manager::BeginControllerCommand
(
	uint32 const _homeId,
	Driver::ControllerCommand _command,
	Driver::pfnControllerCallback_t _callback,				// = NULL
	void* _context,								// = NULL
	bool _highPower,							// = false
	uint8 _nodeId,								// = 0xff
	uint8 _arg								// = 0
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return driver->BeginControllerCommand( _command, _callback, _context, _highPower, _nodeId, _arg );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Manager::CancelControllerCommand>
// Stop the current controller function
//-----------------------------------------------------------------------------
bool Manager::CancelControllerCommand
(
	uint32 const _homeId
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		return( driver->CancelControllerCommand() );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Manager::TestNetworkNode>
// Send a number of test messages to a node and record results.
//-----------------------------------------------------------------------------
void Manager::TestNetworkNode
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint32 const _count
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->TestNetwork( _nodeId, _count );
	}
}

//-----------------------------------------------------------------------------
// <Manager::TestNetwork>
// Send a number of test messages to every node and record results.
//-----------------------------------------------------------------------------
void Manager::TestNetwork
(
	uint32 const _homeId,
	uint32 const _count
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->TestNetwork( 0, _count );
	}
}

//-----------------------------------------------------------------------------
// <Manager::HealNetworkNode>
// Heal a single node in the network
//-----------------------------------------------------------------------------
void Manager::HealNetworkNode
(
	uint32 const _homeId,
	uint8 const _nodeId,
	bool _doRR
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		Node* node = driver->GetNode( _nodeId );
		if( node )
		{
			driver->BeginControllerCommand( Driver::ControllerCommand_RequestNodeNeighborUpdate, NULL, NULL, true, _nodeId, 0 );
			if( _doRR )
			{
				driver->UpdateNodeRoutes( _nodeId, true );
			}
			driver->ReleaseNodes();
		}
	}
}

//-----------------------------------------------------------------------------
// <Manager::HealNetwork>
// Heal the Z-Wave network one node at a time.
//-----------------------------------------------------------------------------
void Manager::HealNetwork
(
	uint32 const _homeId,
	bool _doRR
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->LockNodes();
		for( uint8 i=0; i<255; i++ )
		{
			if( driver->m_nodes[i] != NULL )
			{
			  driver->BeginControllerCommand( Driver::ControllerCommand_RequestNodeNeighborUpdate, NULL, NULL, true, i, 0 );
				if( _doRR )
				{
					driver->UpdateNodeRoutes( i, true );
				}
			}
		}
		driver->ReleaseNodes();
	}
}

//-----------------------------------------------------------------------------
// <Manager::GetNumScenes>
// Return the number of defined scenes.
//-----------------------------------------------------------------------------
uint8 Manager::GetNumScenes
(
)
{
	return Scene::s_sceneCnt;
}

//-----------------------------------------------------------------------------
// <Manager::GetAllScenes>
// Return an array of all Scene Ids
//-----------------------------------------------------------------------------
uint8 Manager::GetAllScenes
(
	uint8** _sceneIds
)
{
	*_sceneIds = NULL;
	return Scene::GetAllScenes( _sceneIds );
}

//-----------------------------------------------------------------------------
// <Manager::RemoveAllScenes>
// Remove every scene id
//-----------------------------------------------------------------------------
void Manager::RemoveAllScenes
(
	uint32 const _homeId
)
{
	for( int i = 1; i < 256; i++ )
	{
		if( _homeId == 0 )	// remove every device from every scene
		{
			RemoveScene( i );
		}
		else
		{
			Scene *scene = Scene::Get( i );
			if( scene != NULL )
			{
				scene->RemoveValues( _homeId );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Manager::CreateScene>
// Create a new scene and return new Scene ID.
//-----------------------------------------------------------------------------
uint8 Manager::CreateScene
(
)
{
	for( int i = 1; i < 256; i++ )
	{
		Scene* scene = Scene::Get( i );
		if( scene != NULL )
		{
			continue;
		}
		new Scene( i );
		return i;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// <Manager::RemoveScene>
// Remove scene and delete its contents
//-----------------------------------------------------------------------------
bool Manager::RemoveScene
(
	uint8 const _sceneId
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		delete scene;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddSceneValue>
// Add a bool ValueID/value pair to the scene
//-----------------------------------------------------------------------------
bool Manager::AddSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	bool const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->AddValue( _valueId, _value ? "True" : "False");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddSceneValue>
// Add a byte ValueID/value pair to the scene
//-----------------------------------------------------------------------------
bool Manager::AddSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	uint8 const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%d", _value );
		return scene->AddValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddSceneValue>
// Add a decimal ValueID/value pair to the scene
//-----------------------------------------------------------------------------
bool Manager::AddSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	float const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%f", _value );
		return scene->AddValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddSceneValue>
// Add an integer ValueID/value pair to the scene
//-----------------------------------------------------------------------------
bool Manager::AddSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int32 const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%d", _value );
		return scene->AddValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddSceneValue>
// Add a short ValueID/value pair to the scene
//-----------------------------------------------------------------------------
bool Manager::AddSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int16 const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%d", _value );
		return scene->AddValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddSceneValue>
// Add a string ValueID/value pair to the scene
//-----------------------------------------------------------------------------
bool Manager::AddSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	string const& _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->AddValue( _valueId, _value );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddSceneValueListSelection>
// Add a list selection item ValueID/value pair to the scene (as string)
//-----------------------------------------------------------------------------
bool Manager::AddSceneValueListSelection
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	string const& _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->AddValue( _valueId, _value );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::AddSceneValueListSelection>
// Add a list selection item ValueID/value pair to the scene (as integer)
//-----------------------------------------------------------------------------
bool Manager::AddSceneValueListSelection
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int32 const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%d", _value );
		return scene->AddValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::RemoveSceneValue>
// Remove a ValueID/value pair from the scene
//-----------------------------------------------------------------------------
bool Manager::RemoveSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->RemoveValue( _valueId );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValues>
// Return a scene's Value ID
//-----------------------------------------------------------------------------
int Manager::SceneGetValues
(
	uint8 const _sceneId,
	vector<ValueID>* o_value
)
{
	o_value->clear();
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->GetValues( o_value );
	}
	return 0;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValueAsBool>
// Return a scene's Value ID bool value
//-----------------------------------------------------------------------------
bool Manager::SceneGetValueAsBool
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	bool* o_value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		string str;
		if( scene->GetValue( _valueId, &str ) )
		{
			*o_value = !strcasecmp( "true", str.c_str() );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValueAsByte>
// Return a scene's Value ID byte value
//-----------------------------------------------------------------------------
bool Manager::SceneGetValueAsByte
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	uint8* o_value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		string str;
		if( scene->GetValue( _valueId, &str ) )
		{
			*o_value = (uint8)atoi( str.c_str() );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValueAsFloat>
// Return a scene's Value ID float value
//-----------------------------------------------------------------------------
bool Manager::SceneGetValueAsFloat
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	float* o_value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		string str;
		if( scene->GetValue( _valueId, &str ) )
		{
			*o_value = (float)atof( str.c_str() );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValueAsInt>
// Return a scene's Value ID integer value
//-----------------------------------------------------------------------------
bool Manager::SceneGetValueAsInt
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int32* o_value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		string str;
		if( scene->GetValue( _valueId, &str ) )
		{
			*o_value = (int32)atoi( str.c_str() );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValueAsShort>
// Return a scene's Value ID short value
//-----------------------------------------------------------------------------
bool Manager::SceneGetValueAsShort
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int16* o_value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		string str;
		if( scene->GetValue( _valueId, &str ) )
		{
			*o_value = (int16)atoi( str.c_str() );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValueAsString>
// Return a scene's Value ID string value
//-----------------------------------------------------------------------------
bool Manager::SceneGetValueAsString
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	string* o_value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		if( scene->GetValue( _valueId, o_value ) )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValueListSelection>
// Return a scene's Value ID list selection (as string) value
//-----------------------------------------------------------------------------
bool Manager::SceneGetValueListSelection
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	string* o_value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		if( scene->GetValue( _valueId, o_value ) )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SceneGetValueListSelection>
// Return a scene's Value ID list selection (as integer) value
//-----------------------------------------------------------------------------
bool Manager::SceneGetValueListSelection
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int32* o_value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		string str;
		if( scene->GetValue( _valueId, &str ) )
		{
			*o_value = (int32)atoi( str.c_str() );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneValue>
// Set a scene's ValueID bool value.
//-----------------------------------------------------------------------------
bool Manager::SetSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	bool const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
	  return scene->SetValue( _valueId, _value ? "True" : "False" );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneValue>
// Set a scene's ValueID byte value.
//-----------------------------------------------------------------------------
bool Manager::SetSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	uint8 const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%d", _value );
		return scene->SetValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneValue>
// Set a scene's ValueID float value.
//-----------------------------------------------------------------------------
bool Manager::SetSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	float const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%f", _value );
		return scene->SetValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneValue>
// Set a scene's ValueID integer value.
//-----------------------------------------------------------------------------
bool Manager::SetSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int32 const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%d", _value );
		return scene->SetValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneValue>
// Set a scene's ValueID short value.
//-----------------------------------------------------------------------------
bool Manager::SetSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int16 const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%d", _value );
		return scene->SetValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneValue>
// Set a scene's ValueID string value.
//-----------------------------------------------------------------------------
bool Manager::SetSceneValue
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	string const& _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->SetValue( _valueId, _value );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneValueListSelection>
// Set a scene's ValueID list item value (as string).
//-----------------------------------------------------------------------------
bool Manager::SetSceneValueListSelection
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	string const& _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->SetValue( _valueId, _value );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneValueListSelection>
// Set a scene's ValueID list item value (as integer).
//-----------------------------------------------------------------------------
bool Manager::SetSceneValueListSelection
(
	uint8 const _sceneId,
	ValueID const& _valueId,
	int32 const _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		char str[16];
		snprintf( str, sizeof(str), "%d", _value );
		return scene->SetValue( _valueId, str );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::GetSceneLabel>
// Return a scene's label
//-----------------------------------------------------------------------------
string Manager::GetSceneLabel
(
	uint8 const _sceneId
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->GetLabel();
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// <Manager::SetSceneLabel>
// Set a scene's label
//-----------------------------------------------------------------------------
void Manager::SetSceneLabel
(
	uint8 const _sceneId,
	string const& _value
)
{
	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		scene->SetLabel( _value );
	}
}

//-----------------------------------------------------------------------------
// <Manager::SceneExists>
// Check if a Scene ID exists
//-----------------------------------------------------------------------------
bool Manager::SceneExists
(
	uint8 const _sceneId
)
{
	return Scene::Get( _sceneId ) != NULL;
}

//-----------------------------------------------------------------------------
// <Manager::ActivateScene>
// Perform all the settings for the given Scene ID
//-----------------------------------------------------------------------------
bool Manager::ActivateScene
(
	uint8 const _sceneId
)
{
  	Scene *scene = Scene::Get( _sceneId );
	if( scene != NULL )
	{
		return scene->Activate();
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Manager::GetDriverStatistics>
// Retrieve driver based counters.
//-----------------------------------------------------------------------------
void Manager::GetDriverStatistics
(
	uint32 const _homeId,
	Driver::DriverData* _data
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->GetDriverStatistics( _data );
	}

}

//-----------------------------------------------------------------------------
// <Manager::GetNodeStatistics>
// Retrieve driver based counters.
//-----------------------------------------------------------------------------
void Manager::GetNodeStatistics
(
	uint32 const _homeId,
	uint8 const _nodeId,
	Node::NodeData* _data
)
{
	if( Driver* driver = GetDriver( _homeId ) )
	{
		driver->GetNodeStatistics( _nodeId, _data );
	}

}
