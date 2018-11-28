//-----------------------------------------------------------------------------
//
//	Manager.h
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

#ifndef _Manager_H
#define _Manager_H

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <list>
#include <deque>

#include "Defs.h"
#include "Driver.h"
#include "value_classes/ValueID.h"

namespace OpenZWave
{
	class Options;
	class Node;
	class Msg;
	class Value;
	class Event;
	class Mutex;
	class SerialPort;
	class Thread;
	class Notification;
	class ValueBool;
	class ValueByte;
	class ValueDecimal;
	class ValueInt;
	class ValueList;
	class ValueShort;
	class ValueString;
	class ValueRaw;

	/** \brief
	 *   The main public interface to OpenZWave.
	 *
	 *	 \nosubgrouping
	 *   A singleton class providing the main public interface to OpenZWave.
	 *   The Manager class exposes all the functionality required to add
	 *   Z-Wave support to an application.  It handles the sending and receiving
	 *   of Z-Wave messages as well as the configuration of a Z-Wave network
	 *   and its devices, freeing the library user from the burden of learning
	 *   the low-level details of the Z-Wave protocol.
	 *   <p>
	 *   All Z-Wave functionality is accessed via the Manager class.  While this
	 *   does not make for the most efficient code structure, it does enable
	 *   the library to handle potentially complex and hard-to-debug issues
	 *   such as multi-threading and object lifespans behind the scenes.
	 *   Application development is therefore simplified and less prone to bugs.
	 *   <p>
	 *   There can be only one instance of the Manager class, and all applications
	 *   will start by calling Manager::Create static method to create that instance.
	 *   From then on, a call to the Manager::Get static method will return the
	 *   pointer to the Manager object.  On application exit, Manager::Destroy
	 *   should be called to allow OpenZWave to clean up and delete any other
	 *   objects it has created.
	 *   <p>
	 *   Once the Manager has been created, a call should be made to Manager::AddWatcher
	 *   to install a notification callback handler.  This handler will receive
	 *   notifications of Z-Wave network changes and updates to device values, and is
	 *   an essential element of OpenZWave.
 	 *   <p>
	 *   Next, a call should be made to Manager::AddDriver for each Z-Wave controller
	 *   attached to the PC.  Each Driver will handle the sending and receiving of
	 *   messages for all the devices in its controller's Z-Wave network.  The Driver
	 *   will read any previously saved configuration and then query the Z-Wave controller
	 *   for any missing information.  Once that process is complete, a DriverReady
	 *   notification callback will be sent containing the Home ID of the controller,
	 *   which is required by most of the other Manager class methods.
	 *	 <p>
	 *	 [After the DriverReady notification is sent, the Driver will poll each node on
	 *   the network to update information about each node.  After all "awake" nodes
	 *   have been polled, an "AllAwakeNodesQueried" notification is sent.  This is when
	 *   a client application can expect all of the node information (both static
	 *   information, like the physical device's capabilities, session information
	 *   (like [associations and/or names] and dynamic information (like temperature or
	 *   on/off state) to be available.  Finally, after all nodes (whether listening or
	 *   sleeping) have been polled, an "AllNodesQueried" notification is sent.]
	 */
	class OPENZWAVE_EXPORT Manager
	{
		friend class Driver;
		friend class CommandClass;
		friend class Group;
		friend class Node;
		friend class Value;
		friend class ValueStore;
		friend class ValueButton;

	public:
		typedef void (*pfnOnNotification_t)( Notification const* _pNotification, void* _context );

	//-----------------------------------------------------------------------------
	// Construction
	//-----------------------------------------------------------------------------
	/** \name Construction
	 *  For creating and destroying the Manager singleton.
	 */
	/*@{*/
	public:
   		/**
		 * \brief Creates the Manager singleton object.
		 * The Manager provides the public interface to OpenZWave, exposing all the functionality required
		 * to add Z-Wave support to an application. There can be only one Manager in an OpenZWave application.
		 * An Options object must be created and Locked first, otherwise the call to Manager::Create will
		 * fail. Once the Manager has been created, call AddWatcher to install a notification callback handler,
		 * and then call the AddDriver method for each attached PC Z-Wave controller in turn.
		 * \param _options a locked Options object containing all the application's configurable option values.
		 * \return a pointer to the newly created Manager object, or NULL if creation failed.
		 * \see Options, Get, Destroy, AddWatcher, AddDriver
		 */
		static Manager* Create();

		/**
		 * \brief Gets a pointer to the Manager object.
		 * \return pointer to the Manager object, or NULL if Create has not yet been called.
		 * \see Create, Destroy
		 */
		static Manager* Get(){ return s_instance; }

		/**
		 * \brief Deletes the Manager and cleans up any associated objects.
		 * \see Create, Get
		 */
		static void Destroy();

		/**
		 * \brief Get the Version Number of OZW as a string
		 * \return a String representing the version number as MAJOR.MINOR.REVISION
		 */
		static std::string getVersionAsString();

		/**
		 * \brief Get the Version Number as the Version Struct (Only Major/Minor returned)
		 * \return the version struct representing the version
		 */
		static ozwversion getVersion();
	/*@}*/

	private:
		Manager();															// Constructor, to be called only via the static Create method.
		virtual ~Manager();													// Destructor, to be called only via the static Destroy method.

		bool					m_exit;										// Flag indicating that program exit is in progress.
		static Manager*			s_instance;									// Pointer to the instance of the Manager singleton.

	//-----------------------------------------------------------------------------
	// Configuration
	//-----------------------------------------------------------------------------
	/** \name Configuration
	 *  For saving the Z-Wave network configuration so that the entire network does not need to be
	 *  polled every time the application starts.
	 */
	/*@{*/
	public:
		/**
		 * \brief Saves the configuration of a PC Controller's Z-Wave network to the application's user data folder.
		 * This method does not normally need to be called, since OpenZWave will save the state automatically
		 * during the shutdown process.  It is provided here only as an aid to development.
		 * The configuration of each PC Controller's Z-Wave network is stored in a separate file.  The filename
		 * consists of the 8 digit hexadecimal version of the controller's Home ID, prefixed with the string 'zwcfg_'.
		 * This convention allows OpenZWave to find the correct configuration file for a controller, even if it is
		 * attached to a different serial port, USB device path, etc.
		 * \param _homeId The Home ID of the Z-Wave controller to save.
		 */
		void WriteConfig( uint32 const _homeId );

		/**
		 * \brief Gets a pointer to the locked Options object.
		 * \return pointer to the Options object.
		 * \see Create
		 */
		Options* GetOptions()const{ return m_options; }
	/*@}*/

	private:
		Options*	m_options;			// Pointer to the locked Options object that was passed in during creation.

	//-----------------------------------------------------------------------------
	//	Drivers
	//-----------------------------------------------------------------------------
	/** \name Drivers
	 *  Methods for adding and removing drivers and obtaining basic controller information.
	 */
	/*@{*/
	public:
		/**
		 * \brief Creates a new driver for a Z-Wave controller.
		 * This method creates a Driver object for handling communications with a single Z-Wave controller.  In the background, the
		 * driver first tries to read configuration data saved during a previous run.  It then queries the controller directly for any
		 * missing information, and a refresh of the list of nodes that it controls.  Once this information
		 * has been received, a DriverReady notification callback is sent, containing the Home ID of the controller.  This Home ID is
		 * required by most of the OpenZWave Manager class methods.
		 * @param _controllerPath The string used to open the controller.  On Windows this might be something like
		 * "\\.\COM3", or on Linux "/dev/ttyUSB0".
		 * \return True if a new driver was created, false if a driver for the controller already exists.
		 * \see Create, Get, RemoveDriver
		 */
		bool AddDriver( string const& _controllerPath, Driver::ControllerInterface const& _interface = Driver::ControllerInterface_Serial);

		/**
		 * \brief Removes the driver for a Z-Wave controller, and closes the controller.
		 * Drivers do not need to be explicitly removed before calling Destroy - this is handled automatically.
		 * \warning You should NOT call any Manager methods that require the Driver Reference (eg, in response to
		 * Notifications recieved about NodeRemoved etc) once you call this, as your application will most likely
		 * break
		 * @param _controllerPath The same string as was passed in the original call to AddDriver.
		 * @returns True if the driver was removed, false if it could not be found.
		 * @see Destroy, AddDriver
		 */
		bool RemoveDriver( string const& _controllerPath );

		/**
		 * \brief Get the node ID of the Z-Wave controller.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return the node ID of the Z-Wave controller.
		 */
		uint8 GetControllerNodeId( uint32 const _homeId );

		/**
		 * \brief Get the node ID of the Static Update Controller.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return the node ID of the Z-Wave controller.
		 */
		uint8 GetSUCNodeId( uint32 const _homeId );

		/**
		 * \brief Query if the controller is a primary controller.
		 * The primary controller is the main device used to configure and control a Z-Wave network.
		 * There can only be one primary controller - all other controllers are secondary controllers.
		 * <p>
		 * The only difference between a primary and secondary controller is that the primary is the
		 * only one that can be used to add or remove other devices.  For this reason, it is usually
		 * better for the promary controller to be portable, since most devices must be added when
		 * installed in their final location.
		 * <p>
		 * Calls to BeginControllerCommand will fail if the controller is not the primary.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return true if it is a primary controller, false if not.
		 */
		bool IsPrimaryController( uint32 const _homeId );

		/**
		 * \brief Query if the controller is a static update controller.
		 * A Static Update Controller (SUC) is a controller that must never be moved in normal operation
		 * and which can be used by other nodes to receive information about network changes.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return true if it is a static update controller, false if not.
		 */
		bool IsStaticUpdateController( uint32 const _homeId );

		/**
		 * \brief Query if the controller is using the bridge controller library.
		 * A bridge controller is able to create virtual nodes that can be associated
		 * with other controllers to enable events to be passed on.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return true if it is a bridge controller, false if not.
		 */
		bool IsBridgeController( uint32 const _homeId );

		/**
		 * \brief Get the version of the Z-Wave API library used by a controller.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return a string containing the library version. For example, "Z-Wave 2.48".
		 */
		string GetLibraryVersion( uint32 const _homeId );

		/**
		 * \brief Get a string containing the Z-Wave API library type used by a controller.
		 * The possible library types are:
		 * - Static Controller
		 * - Controller
		 * - Enhanced Slave
		 * - Slave
		 * - Installer
		 * - Routing Slave
		 * - Bridge Controller
		 * - Device Under Test
		 * The controller should never return a slave library type.
		 * For a more efficient test of whether a controller is a Bridge Controller, use
		 * the IsBridgeController method.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return a string containing the library type.
		 * \see GetLibraryVersion, IsBridgeController
		 */
		string GetLibraryTypeName( uint32 const _homeId );

		/**
		 * \brief Get count of messages in the outgoing send queue.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return a integer message count
		 */
		int32 GetSendQueueCount( uint32 const _homeId );

		/**
		 * \brief Send current driver statistics to the log file
		 * \param _homeId The Home ID of the Z-Wave controller.
		 */
		void LogDriverStatistics( uint32 const _homeId );

		/**
		 * \brief Obtain controller interface type
		 * \param _homeId The Home ID of the Z-Wave controller.
		 */
		Driver::ControllerInterface GetControllerInterfaceType( uint32 const _homeId );

		/**
		 * \brief Obtain controller interface name
		 * \param _homeId The Home ID of the Z-Wave controller.
		 */
		string GetControllerPath( uint32 const _homeId );
	/*@}*/

	private:
		Driver* GetDriver( uint32 const _homeId );	/**< Get a pointer to a Driver object from the HomeID.  Only to be used by OpenZWave. */
		void SetDriverReady( Driver* _driver, bool success );		/**< Indicate that the Driver is ready to be used, and send the notification callback. */

OPENZWAVE_EXPORT_WARNINGS_OFF
		list<Driver*>		m_pendingDrivers;		/**< Drivers that are in the process of reading saved data and querying their Z-Wave network for basic information. */
		map<uint32,Driver*>	m_readyDrivers;			/**< Drivers that are ready to be used by the application. */
OPENZWAVE_EXPORT_WARNINGS_ON

	//-----------------------------------------------------------------------------
	//	Polling Z-Wave devices
	//-----------------------------------------------------------------------------
	/** \name Polling Z-Wave devices
	 *  Methods for controlling the polling of Z-Wave devices.  Modern devices will not
	 *  require polling.  Some old devices need to be polled as the only way to detect
	 *  status changes.
	 */
	/*@{*/
	public:
		/**
		 * \brief Get the time period between polls of a node's state.
		 */
		int32 GetPollInterval();

		/**
		 * \brief Set the time period between polls of a node's state.
		 * Due to patent concerns, some devices do not report state changes automatically to the controller.
		 * These devices need to have their state polled at regular intervals.  The length of the interval
		 * is the same for all devices.  To even out the Z-Wave network traffic generated by polling, OpenZWave
		 * divides the polling interval by the number of devices that have polling enabled, and polls each
		 * in turn.  It is recommended that if possible, the interval should not be set shorter than the
		 * number of polled devices in seconds (so that the network does not have to cope with more than one
		 * poll per second).
		 * \param _seconds The length of the polling interval in seconds.
		 */
		void SetPollInterval( int32 _milliseconds, bool _bIntervalBetweenPolls );

		/**
		 * \brief Enable the polling of a device's state.
		 * \param _valueId The ID of the value to start polling.
		 * \return True if polling was enabled.
		 */
		bool EnablePoll( ValueID const &_valueId, uint8 const _intensity = 1 );

		/**
		 * \brief Disable the polling of a device's state.
		 * \param _valueId The ID of the value to stop polling.
		 * \return True if polling was disabled.
		 */
		bool DisablePoll( ValueID const &_valueId );

		/**
		 * \brief Determine the polling of a device's state.
		 * \param _valueId The ID of the value to check polling.
		 * \return True if polling is active.
		 */
		bool isPolled( ValueID const &_valueId );

		/**
		 * \brief Set the frequency of polling (0=none, 1=every time through the list, 2-every other time, etc)
		 * \param _valueId The ID of the value whose intensity should be set
		 */
		void SetPollIntensity( ValueID const &_valueId, uint8 const _intensity );

		/**
		 * \brief Get the polling intensity of a device's state.
		 * \param _valueId The ID of the value to check polling.
		 * \return Intensity, number of polling for one polling interval.
		 */
		uint8 GetPollIntensity( ValueID const &_valueId );

	/*@}*/

	//-----------------------------------------------------------------------------
	//	Node information
	//-----------------------------------------------------------------------------
	/** \name Node information
	 *  Methods for accessing information on indivdual nodes.
	 */
	/*@{*/
	public:
		/**
		 * \brief Trigger the fetching of fixed data about a node.
		 * Causes the node's data to be obtained from the Z-Wave network in the same way as if it had just been added.
		 * This method would normally be called automatically by OpenZWave, but if you know that a node has been
		 * changed, calling this method will force a refresh of all of the data held by the library.  This can be especially
		 * useful for devices that were asleep when the application was first run. This is the
		 * same as the query state starting from the beginning.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the request was sent successfully.
		 */
		bool RefreshNodeInfo( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Trigger the fetching of dynamic value data for a node.
		 * Causes the node's values to be requested from the Z-Wave network. This is the
		 * same as the query state starting from the associations state.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the request was sent successfully.
		 */
		bool RequestNodeState( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Trigger the fetching of just the dynamic value data for a node.
		 * Causes the node's values to be requested from the Z-Wave network. This is the
		 * same as the query state starting from the dynamic state.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the request was sent successfully.
		 */
		bool RequestNodeDynamic( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get whether the node is a listening device that does not go to sleep
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if it is a listening node.
		 */
		bool IsNodeListeningDevice( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get whether the node is a frequent listening device that goes to sleep but
		 * can be woken up by a beam. Useful to determine node and controller consistency.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if it is a frequent listening node.
		 */
		bool IsNodeFrequentListeningDevice( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get whether the node is a beam capable device.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if it is a beam capable node.
		 */
		bool IsNodeBeamingDevice( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get whether the node is a routing device that passes messages to other nodes
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the node is a routing device
		 */
		bool IsNodeRoutingDevice( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the security attribute for a node. True if node supports security features.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return true if security features implemented.
		 */
		bool IsNodeSecurityDevice( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the maximum baud rate of a node's communications
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return the baud rate in bits per second.
		 */
		uint32 GetNodeMaxBaudRate( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the version number of a node
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return the node's version number
		 */
		uint8 GetNodeVersion( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the security byte of a node
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return the node's security byte
		 */
		uint8 GetNodeSecurity( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the basic type of a node.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return the node's basic type.
		 */
		uint8 GetNodeBasic( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the generic type of a node.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return the node's generic type.
		 */
		uint8 GetNodeGeneric( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the specific type of a node.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return the node's specific type.
		 */
		uint8 GetNodeSpecific( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get a human-readable label describing the node
		 * The label is taken from the Z-Wave specific, generic or basic type, depending on which of those values are specified by the node.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the label text.
		 */
		string GetNodeType( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the bitmap of this node's neighbors
		 *
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _nodeNeighbors An array of 29 uint8s to hold the neighbor bitmap
		 */
		uint32 GetNodeNeighbors( uint32 const _homeId, uint8 const _nodeId, uint8** _nodeNeighbors );

		/**
		 * \brief Get the manufacturer name of a device
		 * The manufacturer name would normally be handled by the Manufacturer Specific commmand class,
		 * taking the manufacturer ID reported by the device and using it to look up the name from the
		 * manufacturer_specific.xml file in the OpenZWave config folder.
		 * However, there are some devices that do not support the command class, so to enable the user
		 * to manually set the name, it is stored with the node data and accessed via this method rather
		 * than being reported via a command class Value object.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's manufacturer name.
		 * \see SetNodeManufacturerName, GetNodeProductName, SetNodeProductName
		 */
		string GetNodeManufacturerName( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the product name of a device
		 * The product name would normally be handled by the Manufacturer Specific commmand class,
		 * taking the product Type and ID reported by the device and using it to look up the name from the
		 * manufacturer_specific.xml file in the OpenZWave config folder.
		 * However, there are some devices that do not support the command class, so to enable the user
		 * to manually set the name, it is stored with the node data and accessed via this method rather
		 * than being reported via a command class Value object.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's product name.
		 * \see SetNodeProductName, GetNodeManufacturerName, SetNodeManufacturerName
		 */
		string GetNodeProductName( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the name of a node
		 * The node name is a user-editable label for the node that would normally be handled by the
		 * Node Naming commmand class, but many devices do not support it.  So that a node can always
		 * be named, OpenZWave stores it with the node data, and provides access through this method
		 * and SetNodeName, rather than reporting it via a command class Value object.
		 * The maximum length of a node name is 16 characters.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's name.
		 * \see SetNodeName, GetNodeLocation, SetNodeLocation
		 */
		string GetNodeName( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the location of a node
		 * The node location is a user-editable string that would normally be handled by the Node Naming
		 * commmand class, but many devices do not support it.  So that a node can always report its
		 * location, OpenZWave stores it with the node data, and provides access through this method
		 * and SetNodeLocation, rather than reporting it via a command class Value object.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's location.
		 * \see SetNodeLocation, GetNodeName, SetNodeName
		 */
		string GetNodeLocation( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the manufacturer ID of a device
		 * The manufacturer ID is a four digit hex code and would normally be handled by the Manufacturer
		 * Specific commmand class, but not all devices support it.  Although the value reported by this
		 * method will be an empty string if the command class is not supported and cannot be set by the
		 * user, the manufacturer ID is still stored with the node data (rather than being reported via a
		 * command class Value object) to retain a consistent approach with the other manufacturer specific data.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's manufacturer ID, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * \see GetNodeProductType, GetNodeProductId, GetNodeManufacturerName, GetNodeProductName
		 */
		string GetNodeManufacturerId( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the product type of a device
		 * The product type is a four digit hex code and would normally be handled by the Manufacturer Specific
		 * commmand class, but not all devices support it.  Although the value reported by this method will
		 * be an empty string if the command class is not supported and cannot be set by the user, the product
		 * type is still stored with the node data (rather than being reported via a command class Value object)
		 * to retain a consistent approach with the other manufacturer specific data.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's product type, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * \see GetNodeManufacturerId, GetNodeProductId, GetNodeManufacturerName, GetNodeProductName
		 */
		string GetNodeProductType( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get the product ID of a device
		 * The product ID is a four digit hex code and would normally be handled by the Manufacturer Specific
		 * commmand class, but not all devices support it.  Although the value reported by this method will
		 * be an empty string if the command class is not supported and cannot be set by the user, the product
		 * ID is still stored with the node data (rather than being reported via a command class Value object)
		 * to retain a consistent approach with the other manufacturer specific data.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return A string containing the node's product ID, or an empty string if the manufactuer
		 * specific command class is not supported by the device.
		 * \see GetNodeManufacturerId, GetNodeProductType, GetNodeManufacturerName, GetNodeProductName
		 */
		string GetNodeProductId( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Set the manufacturer name of a device
		 * The manufacturer name would normally be handled by the Manufacturer Specific commmand class,
		 * taking the manufacturer ID reported by the device and using it to look up the name from the
		 * manufacturer_specific.xml file in the OpenZWave config folder.
		 * However, there are some devices that do not support the command class, so to enable the user
		 * to manually set the name, it is stored with the node data and accessed via this method rather
		 * than being reported via a command class Value object.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _manufacturerName	A string containing the node's manufacturer name.
		 * \see GetNodeManufacturerName, GetNodeProductName, SetNodeProductName
		 */
		void SetNodeManufacturerName( uint32 const _homeId, uint8 const _nodeId, string const& _manufacturerName );

		/**
		 * \brief Set the product name of a device
		 * The product name would normally be handled by the Manufacturer Specific commmand class,
		 * taking the product Type and ID reported by the device and using it to look up the name from the
		 * manufacturer_specific.xml file in the OpenZWave config folder.
		 * However, there are some devices that do not support the command class, so to enable the user
		 * to manually set the name, it is stored with the node data and accessed via this method rather
		 * than being reported via a command class Value object.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _productName A string containing the node's product name.
		 * \see GetNodeProductName, GetNodeManufacturerName, SetNodeManufacturerName
		 */
		void SetNodeProductName( uint32 const _homeId, uint8 const _nodeId, string const& _productName );

		/**
		 * \brief Set the name of a node
		 * The node name is a user-editable label for the node that would normally be handled by the
		 * Node Naming commmand class, but many devices do not support it.  So that a node can always
		 * be named, OpenZWave stores it with the node data, and provides access through this method
		 * and GetNodeName, rather than reporting it via a command class Value object.
		 * If the device does support the Node Naming command class, the new name will be sent to the node.
		 * The maximum length of a node name is 16 characters.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _nodeName A string containing the node's name.
		 * \see GetNodeName, GetNodeLocation, SetNodeLocation
		 */
		void SetNodeName( uint32 const _homeId, uint8 const _nodeId, string const& _nodeName );

		/**
		 * \brief Set the location of a node
		 * The node location is a user-editable string that would normally be handled by the Node Naming
		 * commmand class, but many devices do not support it.  So that a node can always report its
		 * location, OpenZWave stores it with the node data, and provides access through this method
		 * and GetNodeLocation, rather than reporting it via a command class Value object.
		 * If the device does support the Node Naming command class, the new location will be sent to the node.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _location A string containing the node's location.
		 * \see GetNodeLocation, GetNodeName, SetNodeName
		 */
		void SetNodeLocation( uint32 const _homeId, uint8 const _nodeId, string const& _location );

		/**
		 * \brief Turns a node on
		 * This is a helper method to simplify basic control of a node.  It is the equivalent of
		 * changing the level reported by the node's Basic command class to 255, and will generate a
		 * ValueChanged notification from that class.  This command will turn on the device at its
		 * last known level, if supported by the device, otherwise it will turn	it on at 100%.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to be changed.
		 * \see SetNodeOff, SetNodeLevel
		 */
		void SetNodeOn( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Turns a node off
		 * This is a helper method to simplify basic control of a node.  It is the equivalent of
		 * changing the level reported by the node's Basic command class to zero, and will generate
		 * a ValueChanged notification from that class.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to be changed.
		 * \see SetNodeOn, SetNodeLevel
		 */
		void SetNodeOff( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Sets the basic level of a node
		 * This is a helper method to simplify basic control of a node.  It is the equivalent of
		 * changing the value reported by the node's Basic command class and will generate a
		 * ValueChanged notification from that class.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to be changed.
		 * \param _level The level to set the node.  Valid values are 0-99 and 255.  Zero is off and
		 * 99 is fully on.  255 will turn on the device at its last known level (if supported).
		 * \see SetNodeOn, SetNodeOff
		 */
		void SetNodeLevel( uint32 const _homeId, uint8 const _nodeId, uint8 const _level );

		/**
		 * \brief Get whether the node information has been received
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the node information has been received yet
		 */
		bool IsNodeInfoReceived( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get whether the node has the defined class available or not
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \param _commandClassId Id of the class to test for
		 * \return True if the node does have the class instantiated, will return name & version
		 */
		bool GetNodeClassInformation( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId,
					      string *_className = NULL, uint8 *_classVersion = NULL);
		/**
		 * \brief Get whether the node is awake or asleep
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the node is awake
		 */
		bool IsNodeAwake( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get whether the node is working or has failed
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return True if the node has failed and is no longer part of the network
		 */
		bool IsNodeFailed( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Get whether the node's query stage as a string
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to query.
		 * \return name of current query stage as a string.
		 */
		string GetNodeQueryStage( uint32 const _homeId, uint8 const _nodeId );

	/*@}*/

	//-----------------------------------------------------------------------------
	// Values
	//-----------------------------------------------------------------------------
	/** \name Values
	 *  Methods for accessing device values.  All the methods require a ValueID, which will have been provided
	 *  in the ValueAdded Notification callback when the the value was first discovered by OpenZWave.
	 */
	/*@{*/
	public:
		/**
		 * \brief Gets the user-friendly label for the value.
		 * \param _id The unique identifier of the value.
		 * \return The value label.
		 * \see ValueID
		 */
		string GetValueLabel( ValueID const& _id );

		/**
		 * \brief Sets the user-friendly label for the value.
		 * \param _id The unique identifier of the value.
		 * \param _value The new value of the label.
		 * \see ValueID
		 */
		void SetValueLabel( ValueID const& _id, string const& _value );

		/**
		 * \brief Gets the units that the value is measured in.
		 * \param _id The unique identifier of the value.
		 * \return The value units.
		 * \see ValueID
		 */
		string GetValueUnits( ValueID const& _id );

		/**
		 * \brief Sets the units that the value is measured in.
		 * \param _id The unique identifier of the value.
		 * \param _value The new value of the units.
		 * \see ValueID
		 */
		void SetValueUnits( ValueID const& _id, string const& _value );

		/**
		 * \brief Gets a help string describing the value's purpose and usage.
		 * \param _id The unique identifier of the value.
		 * \return The value help text.
		 * \see ValueID
		 */
		string GetValueHelp( ValueID const& _id );

		/**
		 * \brief Sets a help string describing the value's purpose and usage.
		 * \param _id The unique identifier of the value.
		 * \param _value The new value of the help text.
		 * \see ValueID
		 */
		void SetValueHelp( ValueID const& _id, string const& _value );

		/**
		 * \brief Gets the minimum that this value may contain.
		 * \param _id The unique identifier of the value.
		 * \return The value minimum.
		 * \see ValueID
		 */
		int32 GetValueMin( ValueID const& _id );

		/**
		 * \brief Gets the maximum that this value may contain.
		 * \param _id The unique identifier of the value.
		 * \return The value maximum.
		 * \see ValueID
		 */
		int32 GetValueMax( ValueID const& _id );

		/**
		 * \brief Test whether the value is read-only.
		 * \param _id The unique identifier of the value.
		 * \return true if the value cannot be changed by the user.
		 * \see ValueID
		 */
		bool IsValueReadOnly( ValueID const& _id );

		/**
		 * \brief Test whether the value is write-only.
		 * \param _id The unique identifier of the value.
		 * \return true if the value can only be written to and not read.
		 * \see ValueID
		 */
		bool IsValueWriteOnly( ValueID const& _id );

		/**
		 * \brief Test whether the value has been set.
		 * \param _id The unique identifier of the value.
		 * \return true if the value has actually been set by a status message from the device, rather than simply being the default.
		 * \see ValueID
		 */
		bool IsValueSet( ValueID const& _id );

		/**
		 * \brief Test whether the value is currently being polled.
		 * \param _id The unique identifier of the value.
		 * \return true if the value is being polled, otherwise false.
		 * \see ValueID
		 */
		bool IsValuePolled( ValueID const& _id );

		/**
		 * \brief Gets a value as a bool.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to a bool that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Bool. The type can be tested with a call to ValueID::GetType.
		 * \see ValueID::GetType, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueAsBool( ValueID const& _id, bool* o_value );

		/**
		 * \brief Gets a value as an 8-bit unsigned integer.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to a uint8 that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Byte. The type can be tested with a call to ValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueAsByte( ValueID const& _id, uint8* o_value );

		/**
		 * \brief Gets a value as a float.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to a float that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Decimal. The type can be tested with a call to ValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueAsFloat( ValueID const& _id, float* o_value );

		/**
		 * \brief Gets a value as a 32-bit signed integer.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to an int32 that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Int. The type can be tested with a call to ValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueAsInt( ValueID const& _id, int32* o_value );

		/**
		 * \brief Gets a value as a 16-bit signed integer.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to an int16 that will be filled with the value.
		 * \return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Short. The type can be tested with a call to ValueID::GetType.
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsString, GetValueListSelection, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueAsShort( ValueID const& _id, int16* o_value );

		/**
		 * \brief Gets a value as a string.
		 * Creates a string representation of a value, regardless of type.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to a string that will be filled with the value.
		 * \return true if the value was obtained.</returns>
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueListSelection, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueAsString( ValueID const& _id, string* o_value );

		/**
		 * \brief Gets a value as a collection of bytes.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to a uint8* that will be filled with the value. This return value will need to be freed as it was dynamically allocated.
		 * \param o_length Pointer to a uint8 that will be fill with the data length.
		 * \return true if the value was obtained. Returns false if the value is not a ValueID::ValueType_Raw. The type can be tested with a call to ValueID::GetType.
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueListSelection, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueAsRaw( ValueID const& _id, uint8** o_value, uint8* o_length );

		/**
		 * \brief Gets the selected item from a list (as a string).
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to a string that will be filled with the selected item.
		 * \return True if the value was obtained.  Returns false if the value is not a ValueID::ValueType_List. The type can be tested with a call to ValueID::GetType.
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueListSelection( ValueID const& _id, string* o_value );

		/**
		 * \brief Gets the selected item from a list (as an integer).
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to an integer that will be filled with the selected item.
		 * \return True if the value was obtained.  Returns false if the value is not a ValueID::ValueType_List. The type can be tested with a call to ValueID::GetType.
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListItems, GetValueAsRaw
		 */
		bool GetValueListSelection( ValueID const& _id, int32* o_value );

		/**
		 * \brief Gets the list of items from a list value.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to a vector of strings that will be filled with list items. The vector will be cleared before the items are added.
		 * \return true if the list items were obtained.  Returns false if the value is not a ValueID::ValueType_List. The type can be tested with a call to ValueID::GetType.
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsFloat, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueAsRaw
		 */
		bool GetValueListItems( ValueID const& _id, vector<string>* o_value );

		/**
		 * \brief Gets a float value's precision.
		 * \param _id The unique identifier of the value.
		 * \param o_value Pointer to a uint8 that will be filled with the precision value.
		 * \return true if the value was obtained.  Returns false if the value is not a ValueID::ValueType_Decimal. The type can be tested with a call to ValueID::GetType
		 * \see ValueID::GetType, GetValueAsBool, GetValueAsByte, GetValueAsInt, GetValueAsShort, GetValueAsString, GetValueListSelection, GetValueListItems
		 */
		bool GetValueFloatPrecision( ValueID const& _id, uint8* o_value );

		/**
		 * \brief Sets the state of a bool.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _id The unique identifier of the bool value.
		 * \param _value The new value of the bool.
		 * \return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Bool. The type can be tested with a call to ValueID::GetType.
		 */
		bool SetValue( ValueID const& _id, bool const _value );

		/**
		 * \brief Sets the value of a byte.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _id The unique identifier of the byte value.
		 * \param _value The new value of the byte.
		 * \return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Byte. The type can be tested with a call to ValueID::GetType.
		 */
		bool SetValue( ValueID const& _id, uint8 const _value );

		/**
		 * \brief Sets the value of a decimal.
		 * It is usually better to handle decimal values using strings rather than floats, to avoid floating point accuracy issues.
		 * Due to the possibility of a device being asleep, the command is assumed to succeed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _id The unique identifier of the decimal value.
		 * \param _value The new value of the decimal.
		 * \return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Decimal. The type can be tested with a call to ValueID::GetType.
		 */
		bool SetValue( ValueID const& _id, float const _value );

		/**
		 * \brief Sets the value of a 32-bit signed integer.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _id The unique identifier of the integer value.
		 * \param _value The new value of the integer.
		 * \return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Int. The type can be tested with a call to ValueID::GetType.
		 */
		bool SetValue( ValueID const& _id, int32 const _value );

		/**
		 * \brief Sets the value of a 16-bit signed integer.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _id The unique identifier of the integer value.
		 * \param _value The new value of the integer.
		 * \return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Short. The type can be tested with a call to ValueID::GetType.
		 */
		bool SetValue( ValueID const& _id, int16 const _value );

		/**
		 * \brief Sets the value of a collection of bytes.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _id The unique identifier of the raw value.
		 * \param _value The new collection of bytes.
		 * \return true if the value was set.  Returns false if the value is not a ValueID::ValueType_Raw. The type can be tested with a call to ValueID::GetType.
		 */
		bool SetValue( ValueID const& _id, uint8 const* _value, uint8 const _length );

		/**
		 * \brief Sets the value from a string, regardless of type.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _id The unique identifier of the integer value.
		 * \param _value The new value of the string.
		 * \return true if the value was set.  Returns false if the value could not be parsed into the correct type for the value.
		 */
		bool SetValue( ValueID const& _id, string const& _value );

		/**
		 * \brief Sets the selected item in a list.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the value
		 * held by the node is updated directly.  This will be reverted by a future status message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _id The unique identifier of the list value.
		 * \param _selectedItem A string matching the new selected item in the list.
		 * \return true if the value was set.  Returns false if the selection is not in the list, or if the value is not a ValueID::ValueType_List.
		 * The type can be tested with a call to ValueID::GetType
		 */
		bool SetValueListSelection( ValueID const& _id, string const& _selectedItem );

		/**
		 * \brief Refreshes the specified value from the Z-Wave network.
		 * A call to this function causes the library to send a message to the network to retrieve the current value
		 * of the specified ValueID (just like a poll, except only one-time, not recurring).
		 * \param _id The unique identifier of the value to be refreshed.
		 * \return true if the driver and node were found; false otherwise
		 */
		bool RefreshValue( ValueID const& _id);

		/**
		 * \brief Sets a flag indicating whether value changes noted upon a refresh should be verified.  If so, the
		 * library will immediately refresh the value a second time whenever a change is observed.  This helps to filter
		 * out spurious data reported occasionally by some devices.
		 * \param _id The unique identifier of the value whose changes should or should not be verified.
		 * \param _verify if true, verify changes; if false, don't verify changes.
		 */
		void SetChangeVerified( ValueID const& _id, bool _verify );

		/**
		 * \brief Starts an activity in a device.
		 * Since buttons are write-only values that do not report a state, no notification callbacks are sent.
		 * \param _id The unique identifier of the integer value.
		 * \return true if the activity was started.  Returns false if the value is not a ValueID::ValueType_Button. The type can be tested with a call to ValueID::GetType.
		 */
		bool PressButton( ValueID const& _id );

		/**
		 * \brief Stops an activity in a device.
		 * Since buttons are write-only values that do not report a state, no notification callbacks are sent.
		 * \param _id The unique identifier of the integer value.
		 * \return true if the activity was stopped.  Returns false if the value is not a ValueID::ValueType_Button. The type can be tested with a call to ValueID::GetType.
		 */
		bool ReleaseButton( ValueID const& _id );
	/*@}*/

	//-----------------------------------------------------------------------------
	// Climate Control Schedules
	//-----------------------------------------------------------------------------
	/** \name Climate Control Schedules
	 *  Methods for accessing schedule values.  All the methods require a ValueID, which will have been provided
	 *  in the ValueAdded Notification callback when the the value was first discovered by OpenZWave.
	 *  <p>The ValueType_Schedule is a specialized Value used to simplify access to the switch point schedule
	 *  information held by a setback thermostat that supports the Climate Control Schedule command class.
	 *  Each schedule contains up to nine switch points for a single day, consisting of a time in
	 *  hours and minutes (24 hour clock) and a setback in tenths of a degree Celsius.  The setback value can
	 *  range from -128 (-12.8C) to 120 (12.0C).  There are two special setback values - 121 is used to set
	 *  Frost Protection mode, and 122 is used to set Energy Saving mode.
	 *  <p>The switch point methods only modify OpenZWave's copy of the schedule information.  Once all changes
	 *  have been made, they are sent to the device by calling SetSchedule.
	 */
	/*@{*/

		/**
		 * \brief Get the number of switch points defined in a schedule.
		 * \param _id The unique identifier of the schedule value.
		 * \return the number of switch points defined in this schedule.  Returns zero if the value is not a ValueID::ValueType_Schedule. The type can be tested with a call to ValueID::GetType.
		 */
		uint8 GetNumSwitchPoints( ValueID const& _id );

		/**
		 * \brief Set a switch point in the schedule.
		 * Inserts a new switch point into the schedule, unless a switch point already exists at the specified
		 * time in which case that switch point is updated with the new setback value instead.
		 * A maximum of nine switch points can be set in the schedule.
		 * \param _id The unique identifier of the schedule value.
		 * \param _hours The hours part of the time when the switch point will trigger.  The time is set using
		 * the 24-hour clock, so this value must be between 0 and 23.
		 * \param _minutes The minutes part of the time when the switch point will trigger.  This value must be
		 * between 0 and 59.
		 * \param _setback The setback in tenths of a degree Celsius.  The setback value can range from -128 (-12.8C)
		 * to 120 (12.0C).  There are two special setback values - 121 is used to set Frost Protection mode, and
		 * 122 is used to set Energy Saving mode.
		 * \return true if successful.  Returns false if the value is not a ValueID::ValueType_Schedule. The type can be tested with a call to ValueID::GetType.
		 * \see GetNumSwitchPoints, RemoveSwitchPoint, ClearSwitchPoints
		 */
		bool SetSwitchPoint( ValueID const& _id, uint8 const _hours, uint8 const _minutes, int8 const _setback );

		/**
		 * \brief Remove a switch point from the schedule.
		 * Removes the switch point at the specified time from the schedule.
		 * \param _id The unique identifier of the schedule value.
		 * \param _hours The hours part of the time when the switch point will trigger.  The time is set using
		 * the 24-hour clock, so this value must be between 0 and 23.
		 * \param _minutes The minutes part of the time when the switch point will trigger.  This value must be
		 * between 0 and 59.
		 * \return true if successful.  Returns false if the value is not a ValueID::ValueType_Schedule or if there
		 * is not switch point with the specified time values. The type can be tested with a call to ValueID::GetType.
		 * \see GetNumSwitchPoints, SetSwitchPoint, ClearSwitchPoints
		 */
		bool RemoveSwitchPoint( ValueID const& _id, uint8 const _hours, uint8 const _minutes );

		/**
		 * \brief Clears all switch points from the schedule.
		 * \param _id The unique identifier of the schedule value.
		 * \see GetNumSwitchPoints, SetSwitchPoint, RemoveSwitchPoint
		 */
		void ClearSwitchPoints( ValueID const& _id );

		/**
		 * \brief Gets switch point data from the schedule.
		 * Retrieves the time and setback values from a switch point in the schedule.
		 * \param _id The unique identifier of the schedule value.
		 * \param _idx The index of the switch point, between zero and one less than the value
		 * returned by GetNumSwitchPoints.
		 * \param o_hours a pointer to a uint8 that will be filled with the hours part of the switch point data.
		 * \param o_minutes a pointer to a uint8 that will be filled with the minutes part of the switch point data.
		 * \param o_setback a pointer to an int8 that will be filled with the setback value.  This can range from -128
		 * (-12.8C)to 120 (12.0C).  There are two special setback values - 121 is used to set Frost Protection mode, and
		 * 122 is used to set Energy Saving mode.
		 * \return true if successful.  Returns false if the value is not a ValueID::ValueType_Schedule. The type can be tested with a call to ValueID::GetType.
		 * \see GetNumSwitchPoints
		 */
		bool GetSwitchPoint( ValueID const& _id, uint8 const _idx, uint8* o_hours, uint8* o_minutes, int8* o_setback );

	/*@}*/

	//-----------------------------------------------------------------------------
	// SwitchAll
	//-----------------------------------------------------------------------------
	/** \name SwitchAll
	 *  Methods for switching all devices on or off together.  The devices must support
	 *	the SwitchAll command class.  The command is first broadcast to all nodes, and
	 *	then followed up with individual commands to each node (because broadcasts are
	 *	not routed, the message might not otherwise reach all the nodes).
	 */
	/*@{*/

		/**
		 * \brief Switch all devices on.
		 * All devices that support the SwitchAll command class will be turned on.
		 */
		void SwitchAllOn( uint32 const _homeId );

		/**
		 * \brief Switch all devices off.
		 * All devices that support the SwitchAll command class will be turned off.
		 */
		void SwitchAllOff( uint32 const _homeId );

	/*@}*/

	//-----------------------------------------------------------------------------
	// Configuration Parameters
	//-----------------------------------------------------------------------------
	/** \name Configuration Parameters
	 *  Methods for accessing device configuration parameters.
	 *  Configuration parameters are values that are managed by the Configuration command class.
	 *	The values are device-specific and are not reported by the devices. Information on parameters
	 *  is provided only in the device user manual.
	 *  <p>An ongoing task for the OpenZWave project is to create XML files describing the available
	 *  parameters for every Z-Wave.  See the config folder in the project source code for examples.
	 */
	/*@{*/
	public:
		/**
		 * \brief Set the value of a configurable parameter in a device.
		 * Some devices have various parameters that can be configured to control the device behaviour.
		 * These are not reported by the device over the Z-Wave network, but can usually be found in
		 * the device's user manual.
		 * This method returns immediately, without waiting for confirmation from the device that the
		 * change has been made.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to configure.
		 * \param _param The index of the parameter.
		 * \param _value The value to which the parameter should be set.
		 * \param _size Is an optional number of bytes to be sent for the paramter _value. Defaults to 2.
		 * \return true if the a message setting the value was sent to the device.
		 * \see RequestConfigParam
		 */
		bool SetConfigParam( uint32 const _homeId, uint8 const _nodeId, uint8 const _param, int32 _value, uint8 const _size = 2 );

		/**
		 * \brief Request the value of a configurable parameter from a device.
		 * Some devices have various parameters that can be configured to control the device behaviour.
		 * These are not reported by the device over the Z-Wave network, but can usually be found in
		 * the device's user manual.
		 * This method requests the value of a parameter from the device, and then returns immediately,
		 * without waiting for a response.  If the parameter index is valid for this device, and the
		 * device is awake, the value will eventually be reported via a ValueChanged notification callback.
		 * The ValueID reported in the callback will have an index set the same as _param and a command class
		 * set to the same value as returned by a call to Configuration::StaticGetCommandClassId.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to configure.
		 * \param _param The index of the parameter.
		 * \see SetConfigParam, ValueID, Notification
		 */
		void RequestConfigParam( uint32 const _homeId, uint8 const _nodeId, uint8 const _param );

		/**
		 * \brief Request the values of all known configurable parameters from a device.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node to configure.
		 * \see SetConfigParam, ValueID, Notification
		 */
		void RequestAllConfigParams( uint32 const _homeId, uint8 const _nodeId );
	/*@}*/

	//-----------------------------------------------------------------------------
	// Groups (wrappers for the Node methods)
	//-----------------------------------------------------------------------------
	/** \name Groups
	 *  Methods for accessing device association groups.
	 */
	/*@{*/
	public:
		/**
		 * \brief Gets the number of association groups reported by this node
		 * In Z-Wave, groups are numbered starting from one.  For example, if a call to GetNumGroups returns 4, the _groupIdx
		 * value to use in calls to GetAssociations, AddAssociation and RemoveAssociation will be a number between 1 and 4.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose groups we are interested in.
		 * \return The number of groups.
		 * \see GetAssociations, GetMaxAssociations, AddAssociation, RemoveAssociation
		 */
		uint8 GetNumGroups( uint32 const _homeId, uint8 const _nodeId );

		/**
		 * \brief Gets the associations for a group.
		 * Makes a copy of the list of associated nodes in the group, and returns it in an array of uint8's.
		 * The caller is responsible for freeing the array memory with a call to delete [].
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations we are interested in.
		 * \param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \param o_associations If the number of associations returned is greater than zero, o_associations will be set to point to an array containing the IDs of the associated nodes.
		 * \return The number of nodes in the associations array.  If zero, the array will point to NULL, and does not need to be deleted.
		 * \see GetNumGroups, AddAssociation, RemoveAssociation, GetMaxAssociations
		 */
		uint32 GetAssociations( uint32 const _homeId, uint8 const _nodeId, uint8 const _groupIdx, uint8** o_associations );

		/**
		 * \brief Gets the maximum number of associations for a group.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations we are interested in.
		 * \param _groupIdx one-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \return The maximum number of nodes that can be associated into the group.
		 * \see GetNumGroups, AddAssociation, RemoveAssociation, GetAssociations
		 */
		uint8 GetMaxAssociations( uint32 const _homeId, uint8 const _nodeId, uint8 const _groupIdx );

		/**
		 * \brief Returns a label for the particular group of a node.
		 * This label is populated by the device specific configuration files.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations are to be changed.
		 * \param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \see GetNumGroups, GetAssociations, GetMaxAssociations, AddAssociation
		 */
		string GetGroupLabel( uint32 const _homeId, uint8 const _nodeId, uint8 const _groupIdx );

		/**
		 * \brief Adds a node to an association group.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the association data
		 * held in this class is updated directly.  This will be reverted by a future Association message from the device
		 * if the Z-Wave message actually failed to get through.  Notification callbacks will be sent in both cases.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations are to be changed.
		 * \param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \param _targetNodeId Identifier for the node that will be added to the association group.
		 * \see GetNumGroups, GetAssociations, GetMaxAssociations, RemoveAssociation
		 */
		void AddAssociation( uint32 const _homeId, uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId );

		/**
		 * \brief Removes a node from an association group.
		 * Due to the possibility of a device being asleep, the command is assumed to suceed, and the association data
		 * held in this class is updated directly.  This will be reverted by a future Association message from the device
		 * if the Z-Wave message actually failed to get through.   Notification callbacks will be sent in both cases.
		 * \param _homeId The Home ID of the Z-Wave controller that manages the node.
		 * \param _nodeId The ID of the node whose associations are to be changed.
		 * \param _groupIdx One-based index of the group (because Z-Wave product manuals use one-based group numbering).
		 * \param _targetNodeId Identifier for the node that will be removed from the association group.
		 * \see GetNumGroups, GetAssociations, GetMaxAssociations, AddAssociation
		 */
		void RemoveAssociation( uint32 const _homeId, uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId );

	/*@}*/

	//-----------------------------------------------------------------------------
	//	Notifications
	//-----------------------------------------------------------------------------
	/** \name Notifications
	 *  For notification of changes to the Z-Wave network or device values and associations.
	 */
	/*@{*/
	public:
		/**
		 * \brief Add a notification watcher.
		 * In OpenZWave, all feedback from the Z-Wave network is sent to the application via callbacks.
		 * This method allows the application to add a notification callback handler, known as a "watcher" to OpenZWave.
		 * An application needs only add a single watcher - all notifications will be reported to it.
		 * \param _watcher pointer to a function that will be called by the notification system.
		 * \param _context pointer to user defined data that will be passed to the watcher function with each notification.
		 * \return true if the watcher was successfully added.
		 * \see RemoveWatcher, Notification
		 */
		bool AddWatcher( pfnOnNotification_t _watcher, void* _context );

		/**
		 * \brief Remove a notification watcher.
		 * \param _watcher pointer to a function that must match that passed to a previous call to AddWatcher
		 * \param _context pointer to user defined data that must match the one passed in that same previous call to AddWatcher.
		 * \return true if the watcher was successfully removed.
		 * \see AddWatcher, Notification
		 */
		bool RemoveWatcher( pfnOnNotification_t _watcher, void* _context );
	/*@}*/

	private:
		void NotifyWatchers( Notification* _notification );					// Passes the notifications to all the registered watcher callbacks in turn.

		struct Watcher
		{
			pfnOnNotification_t	m_callback;
			void*				m_context;

			Watcher
			(
				pfnOnNotification_t _callback,
				void* _context
			):
				m_callback( _callback ),
				m_context( _context )
			{
			}
		};

OPENZWAVE_EXPORT_WARNINGS_OFF
		list<Watcher*>		m_watchers;										// List of all the registered watchers.
OPENZWAVE_EXPORT_WARNINGS_ON
		Mutex*				m_notificationMutex;

	//-----------------------------------------------------------------------------
	// Controller commands
	//-----------------------------------------------------------------------------
	/** \name Controller Commands
	 *  Commands for Z-Wave network management using the PC Controller.
	 */
	/*@{*/
	public:
		/**
		 * \brief Hard Reset a PC Z-Wave Controller.
		 * Resets a controller and erases its network configuration settings.  The controller becomes a primary controller ready to add devices to a new network.
		 * \param _homeId The Home ID of the Z-Wave controller to be reset.
		 * \see SoftReset
		 */
		void ResetController( uint32 const _homeId );

		/**
		 * \brief Soft Reset a PC Z-Wave Controller.
		 * Resets a controller without erasing its network configuration settings.
		 * \param _homeId The Home ID of the Z-Wave controller to be reset.
		 * \see SoftReset
		 */
		void SoftReset( uint32 const _homeId );

		/**
		 * \brief Start a controller command process.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \param _command The command to be sent to the controller.
		 * \param _callback pointer to a function that will be called at various stages during the command process
		 * to notify the user of progress or to request actions on the user's part.  Defaults to NULL.
		 * \param _context pointer to user defined data that will be passed into to the callback function.  Defaults to NULL.
		 * \param _highPower used only with the AddDevice, AddController, RemoveDevice and RemoveController commands.
		 * Usually when adding or removing devices, the controller operates at low power so that the controller must
		 * be physically close to the device for security reasons.  If _highPower is true, the controller will
		 * operate at normal power levels instead.  Defaults to false.
		 * \param _nodeId is the node ID used by the command if necessary.
		 * \param _arg is an optional argument, usually another node ID, that is used by the command.
		 * \return true if the command was accepted and has queued to be executed.
		 * \see CancelControllerCommand, HasNodeFailed, RemoveFailedNode, Driver::ControllerCommand, Driver::pfnControllerCallback_t,
		 * <p> Commands
		 * - Driver::ControllerCommand_AddDevice - Add a new device or controller to the Z-Wave network.
		 * - Driver::ControllerCommand_CreateNewPrimary - Create a new primary controller when old primary fails. Requires SUC.
		 * - Driver::ControllerCommand_ReceiveConfiguration - Receive network configuration information from primary controller. Requires secondary.
		 * - Driver::ControllerCommand_RemoveDevice - Remove a device or controller from the Z-Wave network.
 		 * - Driver::ControllerCommand_RemoveFailedNode - Remove a node from the network. The node must not be responding
		 * and be on the controller's failed node list.
		 * - Driver::ControllerCommand_HasNodeFailed - Check whether a node is in the controller's failed nodes list.
		 * - Driver::ControllerCommand_ReplaceFailedNode - Replace a failed device with another. If the node is not in
		 * the controller's failed nodes list, or the node responds, this command will fail.
		 * - Driver:: ControllerCommand_TransferPrimaryRole - Add a new controller to the network and
		 * make it the primary.  The existing primary will become a secondary controller.
		 * - Driver::ControllerCommand_RequestNetworkUpdate - Update the controller with network information from the SUC/SIS.
		 * - Driver::ControllerCommand_RequestNodeNeighborUpdate - Get a node to rebuild its neighbour list.  This method also does RequestNodeNeighbors afterwards.
		 * - Driver::ControllerCommand_AssignReturnRoute - Assign a network return route to a device.
		 * - Driver::ControllerCommand_DeleteAllReturnRoutes - Delete all network return routes from a device.
		 * - Driver::ControllerCommand_SendNodeInformation - Send a node information frame.
		 * - Driver::ControllerCommand_ReplicationSend - Send information from primary to secondary
		 * - Driver::ControllerCommand_CreateButton - Create a handheld button id.
		 * - Driver::ControllerCommand_DeleteButton - Delete a handheld button id.
		 * <p> Callbacks
		 * - Driver::ControllerState_Starting, the controller command has begun
		 * - Driver::ControllerState_Waiting, the controller is waiting for a user action.  A notice should be displayed
		 * to the user at this point, telling them what to do next.
		 * For the add, remove, replace and transfer primary role commands, the user needs to be told to press the
		 * inclusion button on the device that  is going to be added or removed.  For ControllerCommand_ReceiveConfiguration,
		 * they must set their other controller to send its data, and for ControllerCommand_CreateNewPrimary, set the other
		 * controller to learn new data.
		 * - Driver::ControllerState_InProgress - the controller is in the process of adding or removing the chosen node.  It is now too late to cancel the command.
		 * - Driver::ControllerState_Complete - the controller has finished adding or removing the node, and the command is complete.
		 * - Driver::ControllerState_Failed - will be sent if the command fails for any reason.
		 */
		bool BeginControllerCommand( uint32 const _homeId, Driver::ControllerCommand _command, Driver::pfnControllerCallback_t _callback = NULL, void* _context = NULL, bool _highPower = false, uint8 _nodeId = 0xff, uint8 _arg = 0 );

		/**
		 * \brief Cancels any in-progress command running on a controller.
		 * \param _homeId The Home ID of the Z-Wave controller.
		 * \return true if a command was running and was cancelled.
		 * \see BeginControllerCommand
		 */
		bool CancelControllerCommand( uint32 const _homeId );
	/*@}*/

	//-----------------------------------------------------------------------------
	// Network commands
	//-----------------------------------------------------------------------------
	/** \name Network Commands
	 *  Commands for Z-Wave network for testing, routing and other internal
	 *  operations.
	 */
	/*@{*/
	public:
		/**
		 * \brief Test network node.
		 * Sends a series of messages to a network node for testing network reliability.
		 * \param _homeId The Home ID of the Z-Wave controller to be reset.
		 * \param _count This is the number of test messages to send.
		 * \see TestNetwork
		 */
		void TestNetworkNode( uint32 const _homeId, uint8 const _nodeId, uint32 const _count );

		/**
		 * \brief Test network.
		 * Sends a series of messages to every node on the network for testing network reliability.
		 * \param _homeId The Home ID of the Z-Wave controller to be reset.
		 * \param _count This is the number of test messages to send.
		 * \see TestNetwork
		 */
		void TestNetwork( uint32 const _homeId, uint32 const _count );

 		/**
		 * \brief Heal network node by requesting the node rediscover their neighbors.
		 * Sends a ControllerCommand_RequestNodeNeighborUpdate to the node.
		 * \param _homeId The Home ID of the Z-Wave network to be healed.
		 * \param _nodeId The node to heal.
		 * \param _doRR Whether to perform return routes initialization.
		 */
		void HealNetworkNode( uint32 const _homeId, uint8 const _nodeId, bool _doRR );

 		/**
		 * \brief Heal network by requesting node's rediscover their neighbors.
		 * Sends a ControllerCommand_RequestNodeNeighborUpdate to every node.
		 * Can take a while on larger networks.
		 * \param _homeId The Home ID of the Z-Wave network to be healed.
		 * \param _doRR Whether to perform return routes initialization.
		 */
		void HealNetwork( uint32 const _homeId, bool _doRR );

	/*@}*/

	//-----------------------------------------------------------------------------
	// Scene commands
	//-----------------------------------------------------------------------------
	/** \name Scene Commands
	 *  Commands for Z-Wave scene interface.
	 */
	/*@{*/
	public:
		/**
		 * \brief Gets the number of scenes that have been defined.
		 * \return The number of scenes.
		 * \see GetAllScenes, RemoveAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		uint8 GetNumScenes( );

		/**
		 * \brief Gets a list of all the SceneIds.
		 * \param _sceneIds is a pointer to an array of integers.
		 * \return The number of scenes. If zero, _sceneIds will be NULL and doesn't need to be freed.
		 * \see GetNumScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		uint8 GetAllScenes( uint8** _sceneIds );

		/**
		 * \brief Remove all the SceneIds.
		 * \param _homeId The Home ID of the Z-Wave controller. 0 for all devices from all scenes.
		 * \see GetAllScenes, GetNumScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		void RemoveAllScenes( uint32 const _homeId );

		/**
		 * \brief Create a new Scene passing in Scene ID
		 * \return uint8 Scene ID used to reference the scene. 0 is failure result.
		 * \see GetNumScenes, GetAllScenes, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene

		 */
		uint8 CreateScene();

		/**
		 * \brief Remove an existing Scene.
		 * \param _sceneId is an integer representing the unique Scene ID to be removed.
		 * \return true if scene was removed.
		 * \see GetNumScenes, GetAllScenes, CreateScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool RemoveScene( uint8 const _sceneId );

		/**
		 * \brief Add a bool Value ID to an existing scene.
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the bool value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool AddSceneValue( uint8 const _sceneId, ValueID const& _valueId, bool const _value );

		/**
		 * \brief Add a byte Value ID to an existing scene.
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the byte value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool AddSceneValue( uint8 const _sceneId, ValueID const& _valueId, uint8 const _value );

		/**
		 * \brief Add a decimal Value ID to an existing scene.
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the float value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool AddSceneValue( uint8 const _sceneId, ValueID const& _valueId, float const _value );

		/**
		 * \brief Add a 32-bit signed integer Value ID to an existing scene.
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the int32 value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool AddSceneValue( uint8 const _sceneId, ValueID const& _valueId, int32 const _value );

		/**
		 * \brief Add a 16-bit signed integer Value ID to an existing scene.
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the int16 value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool AddSceneValue( uint8 const _sceneId, ValueID const& _valueId, int16 const _value );

		/**
		 * \brief Add a string Value ID to an existing scene.
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the string value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool AddSceneValue( uint8 const _sceneId, ValueID const& _valueId, string const& _value );

		/**
		 * \brief Add the selected item list Value ID to an existing scene (as a string).
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the string value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool AddSceneValueListSelection( uint8 const _sceneId, ValueID const& _valueId, string const& _value );

		/**
		 * \brief Add the selected item list Value ID to an existing scene (as a integer).
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the integer value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool AddSceneValueListSelection( uint8 const _sceneId, ValueID const& _valueId, int32 const _value );

		/**
		 * \brief Remove the Value ID from an existing scene.
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be removed.
		 * \return true if Value ID was removed.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool RemoveSceneValue( uint8 const _sceneId, ValueID const& _valueId );

		/**
		 * \brief Retrieves the scene's list of values.
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param o_value Pointer to an array of ValueIDs if return is non-zero.
		 * \return The number of nodes in the o_value array. If zero, the array will point to NULL and does not need to be deleted.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		int SceneGetValues( uint8 const _sceneId, vector<ValueID>* o_value );

		/**
		 * \brief Retrieves a scene's value as a bool.
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param _valueId The Value ID of the value to retrieve.
		 * \param o_value Pointer to a bool that will be filled with the returned value.
		 * \return true if the value was obtained.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SceneGetValueAsBool( uint8 const _sceneId, ValueID const& _valueId, bool* o_value );

		/**
		 * \brief Retrieves a scene's value as an 8-bit unsigned integer.
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param _valueId The Value ID of the value to retrieve.
		 * \param o_value Pointer to a uint8 that will be filled with the returned value.
		 * \return true if the value was obtained.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SceneGetValueAsByte( uint8 const _sceneId, ValueID const& _valueId, uint8* o_value );

		/**
		 * \brief Retrieves a scene's value as a float.
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param _valueId The Value ID of the value to retrieve.
		 * \param o_value Pointer to a float that will be filled with the returned value.
		 * \return true if the value was obtained.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SceneGetValueAsFloat( uint8 const _sceneId, ValueID const& _valueId, float* o_value );

		/**
		 * \brief Retrieves a scene's value as a 32-bit signed integer.
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param _valueId The Value ID of the value to retrieve.
		 * \param o_value Pointer to a int32 that will be filled with the returned value.
		 * \return true if the value was obtained.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SceneGetValueAsInt( uint8 const _sceneId, ValueID const& _valueId, int32* o_value );

		/**
		 * \brief Retrieves a scene's value as a 16-bit signed integer.
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param _valueId The Value ID of the value to retrieve.
		 * \param o_value Pointer to a int16 that will be filled with the returned value.
		 * \return true if the value was obtained.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SceneGetValueAsShort( uint8 const _sceneId, ValueID const& _valueId, int16* o_value );

		/**
		 * \brief Retrieves a scene's value as a string.
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param _valueId The Value ID of the value to retrieve.
		 * \param o_value Pointer to a string that will be filled with the returned value.
		 * \return true if the value was obtained.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SceneGetValueAsString( uint8 const _sceneId, ValueID const& _valueId, string* o_value );

		/**
		 * \brief Retrieves a scene's value as a list (as a string).
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param _valueId The Value ID of the value to retrieve.
		 * \param o_value Pointer to a string that will be filled with the returned value.
		 * \return true if the value was obtained.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SceneGetValueListSelection( uint8 const _sceneId, ValueID const& _valueId, string* o_value );

		/**
		 * \brief Retrieves a scene's value as a list (as a integer).
		 * \param _sceneId The Scene ID of the scene to retrieve the value from.
		 * \param _valueId The Value ID of the value to retrieve.
		 * \param o_value Pointer to a integer that will be filled with the returned value.
		 * \return true if the value was obtained.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SceneGetValueListSelection( uint8 const _sceneId, ValueID const& _valueId, int32* o_value );

		/**
		 * \brief Set a bool Value ID to an existing scene's ValueID
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the bool value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SetSceneValue( uint8 const _sceneId, ValueID const& _valueId, bool const _value );

		/**
		 * \brief Set a byte Value ID to an existing scene's ValueID
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the byte value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SetSceneValue( uint8 const _sceneId, ValueID const& _valueId, uint8 const _value );

		/**
		 * \brief Set a decimal Value ID to an existing scene's ValueID
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the float value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SetSceneValue( uint8 const _sceneId, ValueID const& _valueId, float const _value );

		/**
		 * \brief Set a 32-bit signed integer Value ID to an existing scene's ValueID
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the int32 value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SetSceneValue( uint8 const _sceneId, ValueID const& _valueId, int32 const _value );

		/**
		 * \brief Set a 16-bit integer Value ID to an existing scene's ValueID
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the int16 value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SetSceneValue( uint8 const _sceneId, ValueID const& _valueId, int16 const _value );

		/**
		 * \brief Set a string Value ID to an existing scene's ValueID
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the string value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SetSceneValue( uint8 const _sceneId, ValueID const& _valueId, string const& _value );

		/**
		 * \brief Set the list selected item Value ID to an existing scene's ValueID (as a string).
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the string value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SetSceneValueListSelection( uint8 const _sceneId, ValueID const& _valueId, string const& _value );

		/**
		 * \brief Set the list selected item Value ID to an existing scene's ValueID (as a integer).
		 * \param _sceneId is an integer representing the unique Scene ID.
		 * \param _valueId is the Value ID to be added.
		 * \param _value is the integer value to be saved.
		 * \return true if Value ID was added.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists, ActivateScene
		 */
		bool SetSceneValueListSelection( uint8 const _sceneId, ValueID const& _valueId, int32 const _value );

		/**
		 * \brief Returns a label for the particular scene.
		 * \param _sceneId The Scene ID
		 * \return The label string.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, SetSceneLabel, SceneExists, ActivateScene
		 */
		string GetSceneLabel( uint8 const _sceneId );

		/**
		 * \brief Sets a label for the particular scene.
		 * \param _sceneId The Scene ID
		 * \param _value The new value of the label.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SceneExists, ActivateScene
		 */
		void SetSceneLabel( uint8 const _sceneId, string const& _value );

		/**
		 * \brief Check if a Scene ID is defined.
		 * \param _sceneId The Scene ID.
		 * \return true if Scene ID exists.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, ActivateScene
		 */
		bool SceneExists( uint8 const _sceneId );

		/**
		 * \brief Activate given scene to perform all its actions.
		 * \param _sceneId The Scene ID.
		 * \return true if it is successful.
		 * \see GetNumScenes, GetAllScenes, CreateScene, RemoveScene, AddSceneValue, RemoveSceneValue, SceneGetValues, SceneGetValueAsBool, SceneGetValueAsByte, SceneGetValueAsFloat, SceneGetValueAsInt, SceneGetValueAsShort, SceneGetValueAsString, SetSceneValue, GetSceneLabel, SetSceneLabel, SceneExists
		 */
		bool ActivateScene( uint8 const _sceneId );

	/*@}*/

	//-----------------------------------------------------------------------------
	// Statistics interface
	//-----------------------------------------------------------------------------
	/** \name Statistics retrieval interface
	 *  Commands for Z-Wave statistics interface.
	 */
	/*@{*/
	public:
		/**
		 * \brief Retrieve statistics from driver
		 * \param _homeId The Home ID of the driver to obtain counters
		 * \param _data Pointer to structure DriverData to return values
		 */
		void GetDriverStatistics( uint32 const _homeId, Driver::DriverData* _data );

		/**
		 * \brief Retrieve statistics per node
		 * \param _homeId The Home ID of the driver for the node
		 * \param _nodeId The node number
		 * \param _data Pointer to structure NodeData to return values
		 */
		void GetNodeStatistics( uint32 const _homeId, uint8 const _nodeId, Node::NodeData* _data );

	};
	/*@}*/
} // namespace OpenZWave

#endif // _Manager_H
