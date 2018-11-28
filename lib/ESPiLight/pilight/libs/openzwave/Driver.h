//-----------------------------------------------------------------------------
//
//	Driver.h
//
//	Communicates with a Z-Wave network
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

#ifndef _Driver_H
#define _Driver_H

#include <string>
#include <map>
#include <list>

#include "Defs.h"
#include "value_classes/ValueID.h"
#include "Node.h"
#include "platform/Event.h"
#include "platform/Mutex.h"
#include "platform/TimeStamp.h"

namespace OpenZWave
{
	class Msg;
	class Value;
	class Event;
	class Mutex;
	class Controller;
	class Thread;
	class ControllerReplication;
	class Notification;

	/** \brief The Driver class handles communication between OpenZWave
	 *  and a device attached via a serial port (typically a controller).
	 */
	class OPENZWAVE_EXPORT Driver
	{
		friend class Manager;
		friend class Node;
		friend class Group;
		friend class CommandClass;
		friend class ControllerReplication;
		friend class Value;
		friend class ValueStore;
		friend class ValueButton;
		friend class Association;
		friend class Basic;
		friend class ManufacturerSpecific;
		friend class NodeNaming;
		friend class NoOperation;
		friend class SceneActivation;
		friend class WakeUp;
		friend class Security;

	//-----------------------------------------------------------------------------
	//	Controller Interfaces
	//-----------------------------------------------------------------------------
	public:
		enum ControllerInterface
		{
			ControllerInterface_Unknown = 0,
			ControllerInterface_Serial,
			ControllerInterface_Hid
		};

	//-----------------------------------------------------------------------------
	// Construction / Destruction
	//-----------------------------------------------------------------------------
	private:
		/**
		 *  Creates threads, events and initializes member variables and the node array.
		 */
		Driver( string const& _controllerPath, ControllerInterface const& _interface );
		/** Sets "exit" flags and stops the three background threads (pollThread, serialThread
		 *  and driverThread).  Then clears out the send queue and node array.  Notifies
		 *  watchers and exits.
		 */
		virtual ~Driver();

		/**
		 *  Start the driverThread
		 */
		void Start();
		/**
		 *  Entry point for driverThread
		 */
		static void DriverThreadEntryPoint( Event* _exitEvent, void* _context );
		/**
		 *  ThreadProc for driverThread.  This is where all the "action" takes place.
		 *  <p>
		 *  First, the thread is initialized by calling Init().  If Init() fails, it will be retried
		 *  every 5 seconds for the first two minutes and every 30 seconds thereafter.
		 *  <p>
		 *  After the thread is successfully initialized, the thread enters a loop with the
		 *  following elements:
		 *  - Confirm that m_exit is still false (or exit from the thread if it is true)
		 *  - Call ReadMsg() to consume any available messages from the controller
		 *  - Call NotifyWatchers() to send any pending notifications
		 *  - If the thread is not waiting for an ACK, a callback or a message reply, send [any][the next] queued message[s]
		 *  - If there was no message read or sent (workDone=false), sleep for 5 seconds.  If nothing happened
		 *  within this time frame and something was expected (ACK, callback or reply), retrieve the
		 *  last message from the send queue and examine GetSendAttempts().  If greater than 2, give up
		 *  and remove the message from the queue.  Otherwise, resend the message.
		 *  - If something did happen [reset m_wakeEvent]
		 */
		void DriverThreadProc( Event* _exitEvent );
		/**
		 *  Initialize the controller.  Open the specified serial port, start the serialThread
		 *  and pollThread, then send a NAK to the device [presumably to flush it].
		 *  <p>
		 *  Then queue the commands to retrieve the Z-Wave interface:
		 *  - Get version
		 *  - Get home and node IDs
		 *  - Get controller capabilities
		 *  - Get serial API capabilties
		 *  - [Get SUC node ID]
		 *  - Get init data [identifying the nodes on the network]
		 *  Init() will return false if the serial port could not be opened.
		 */
		bool Init( uint32 _attempts );

		/**
		 * Remove any messages to a node on the queues
		 * Used when deleting a node.
		 */
		void RemoveQueues( uint8 const _nodeId );

		Thread*					m_driverThread;			/**< Thread for reading from the Z-Wave controller, and for creating and managing the other threads for sending, polling etc. */
		bool					m_exit;					/**< Flag that is set when the application is exiting. */
		bool					m_init;					/**< Set to true once the driver has been initialised */
		bool					m_awakeNodesQueried;	/**< Set to true once the driver has polled all awake nodes */
		bool					m_allNodesQueried;		/**< Set to true once the driver has polled all nodes */
		bool					m_notifytransactions;
		TimeStamp				m_startTime;			/**< Time this driver started (for log report purposes) */

	//-----------------------------------------------------------------------------
	//	Configuration
	//-----------------------------------------------------------------------------
	private:
		void RequestConfig();							// Get the network configuration from the Z-Wave network
		bool ReadConfig();								// Read the configuration from a file
		void WriteConfig();								// Save the configuration to a file

	//-----------------------------------------------------------------------------
	//	Controller
	//-----------------------------------------------------------------------------
	private:
		// Controller Capabilities (return in FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES)
		enum
		{
			ControllerCaps_Secondary	= 0x01,		/**< The controller is a secondary. */
			ControllerCaps_OnOtherNetwork	= 0x02,		/**< The controller is not using its default HomeID. */
			ControllerCaps_SIS		= 0x04,		/**< There is a SUC ID Server on the network. */
			ControllerCaps_RealPrimary	= 0x08,		/**< Controller was the primary before the SIS was added. */
			ControllerCaps_SUC		= 0x10		/**< Controller is a static update controller. */
		};

		// Init Capabilities (return in FUNC_ID_SERIAL_API_GET_INIT_DATA)
		enum
		{
			InitCaps_Slave					= 0x01,		/**<  */
			InitCaps_TimerSupport				= 0x02,		/**< Controller supports timers. */
			InitCaps_Secondary				= 0x04,		/**< Controller is a secondary. */
			InitCaps_SUC					= 0x08		/**< Controller is a static update controller. */
		};

		bool IsPrimaryController()const{ return ((m_initCaps & InitCaps_Secondary) == 0); }
		bool IsStaticUpdateController()const{ return ((m_initCaps & InitCaps_SUC) != 0); }
		bool IsBridgeController()const{ return (m_libraryType == 7); }
		bool IsInclusionController()const{ return ((m_controllerCaps & ControllerCaps_SIS) != 0); }


		uint32 GetHomeId()const{ return m_homeId; }
		uint8 GetNodeId()const{ return m_nodeId; }
		uint8 GetSUCNodeId()const{ return m_SUCNodeId; }
		uint16 GetManufacturerId()const{ return m_manufacturerId; }
		uint16 GetProductType()const{ return m_productType; }
		uint16 GetProductId()const{ return m_productId; }
		string GetControllerPath()const{ return m_controllerPath; }
		ControllerInterface GetControllerInterfaceType()const{ return m_controllerInterfaceType; }
		string GetLibraryVersion()const{ return m_libraryVersion; }
		string GetLibraryTypeName()const{ return m_libraryTypeName; }
		int32 GetSendQueueCount()const
		{
			int32 count = 0;
			for( int32 i=0; i<MsgQueue_Count; ++i )
			{
				count += (int32) (m_msgQueue[i].size());
			}
			return count;
		}

		/**
		 *  A version of GetNode that does not have the protective "lock" and "release" requirement.
		 *  This function can be used within driverThread, which "knows" that the node will not be
		 *  changed or deleted while it is being used.
		 *  \param _nodeId The nodeId (index into the node array) identifying the node to be returned
		 *  \return
		 *  A pointer to the specified node (if it exists) or NULL if not.
		 *  \see GetNode
		 */
		Node* GetNodeUnsafe( uint8 _nodeId );
		/**
		 *  Locks the node array and returns the specified node (if it exists).  If a node is returned,
		 *  the lock must be released after the node has been processed via a call to ReleaseNodes().
		 *  If the node specified by _nodeId does not exist, the lock is released and NULL is returned.
		 *  \param _nodeId The nodeId (index into the node array) identifying the node to be returned
		 *  \return
		 *  A pointer to the specified node (if it exists) or NULL if not.
		 *  \see LockNodes, ReleaseNodes
		 */
		Node* GetNode( uint8 _nodeId );
		/**
		 *  Lock the nodes so no other thread can modify them.
		 */
		void LockNodes();
		/**
		 *  Release the lock on the nodes so other threads can modify them.
		 */
		void ReleaseNodes();

		ControllerInterface			m_controllerInterfaceType;						// Specifies the controller's hardware interface
		string					m_controllerPath;							// name or path used to open the controller hardware.
		Controller*				m_controller;								// Handles communications with the controller hardware.
		uint32					m_homeId;									// Home ID of the Z-Wave controller.  Not valid until the DriverReady notification has been received.

		string					m_libraryVersion;							// Verison of the Z-Wave Library used by the controller.
		string					m_libraryTypeName;							// Name describing the library type.
		uint8					m_libraryType;								// Type of library used by the controller.

		uint8					m_serialAPIVersion[2];
		uint16					m_manufacturerId;
		uint16					m_productType;
		uint16					m_productId;
		uint8					m_apiMask[32];

		uint8					m_initVersion;								// Version of the Serial API used by the controller.
		uint8					m_initCaps;									// Set of flags indicating the serial API capabilities (See IsSlave, HasTimerSupport, IsPrimaryController and IsStaticUpdateController above).
		uint8					m_controllerCaps;							// Set of flags indicating the controller's capabilities (See IsInclusionController above).
		uint8					m_nodeId;									// Z-Wave Controller's own node ID.
		Node*					m_nodes[256];								// Array containing all the node objects.
		Mutex*					m_nodeMutex;								// Serializes access to node data

		ControllerReplication*	m_controllerReplication;					// Controller replication is handled separately from the other command classes, due to older hand-held controllers using invalid node IDs.

		uint8					m_transmitOptions;

	//-----------------------------------------------------------------------------
	//	Receiving Z-Wave messages
	//-----------------------------------------------------------------------------
	private:
		bool ReadMsg();
		void ProcessMsg( uint8* _data );

		void HandleGetVersionResponse( uint8* _data );
		void HandleGetRandomResponse( uint8* _data );
		void HandleGetControllerCapabilitiesResponse( uint8* _data );
		void HandleGetSerialAPICapabilitiesResponse( uint8* _data );
		void HandleSerialAPISoftResetResponse( uint8* _data );
		void HandleEnableSUCResponse( uint8* _data );
		void HandleSetSUCNodeIdResponse( uint8* _data );
		void HandleGetSUCNodeIdResponse( uint8* _data );
		void HandleMemoryGetIdResponse( uint8* _data );
		/**
		 *  Process a response to a FUNC_ID_SERIAL_API_GET_INIT_DATA request.
		 *  <p>
		 *  The response message contains a bitmap identifying which of the 232 possible nodes
		 *  in the network are actually present.  These bitmap values are compared with the
		 *  node map (read in from zwcfg_0x[homeid].xml) to see if the node has already been registered
		 *  by the OpenZWave library.  If it has (the log will show it as "Known") and this is
		 *  the first time this message was sent (m_init is false), then AddNodeQuery() is called
		 *  to retrieve its current state.  If this is a "New" node to OpenZWave, then InitNode()
		 *  is called.
		 *  \see AddNodeQuery, InitNode, GetNode, ReleaseNodes
		 */
		void HandleSerialAPIGetInitDataResponse( uint8* _data );
		void HandleGetNodeProtocolInfoResponse( uint8* _data );
		bool HandleRemoveFailedNodeResponse( uint8* _data );
		void HandleIsFailedNodeResponse( uint8* _data );
		bool HandleReplaceFailedNodeResponse( uint8* _data );
		bool HandleAssignReturnRouteResponse( uint8* _data );
		bool HandleDeleteReturnRouteResponse( uint8* _data );
		void HandleSendNodeInformationRequest( uint8* _data );
		void HandleSendDataResponse( uint8* _data, bool _replication );
		bool HandleNetworkUpdateResponse( uint8* _data );
		void HandleGetRoutingInfoResponse( uint8* _data );

		void HandleSendDataRequest( uint8* _data, bool _replication );
		void HandleAddNodeToNetworkRequest( uint8* _data );
		void HandleCreateNewPrimaryRequest( uint8* _data );
		void HandleControllerChangeRequest( uint8* _data );
		void HandleSetLearnModeRequest( uint8* _data );
		void HandleRemoveFailedNodeRequest( uint8* _data );
		void HandleReplaceFailedNodeRequest( uint8* _data );
		void HandleRemoveNodeFromNetworkRequest( uint8* _data );
		void HandleApplicationCommandHandlerRequest( uint8* _data );
		void HandlePromiscuousApplicationCommandHandlerRequest( uint8* _data );
		void HandleAssignReturnRouteRequest( uint8* _data );
		void HandleDeleteReturnRouteRequest( uint8* _data );
		void HandleNodeNeighborUpdateRequest( uint8* _data );
		void HandleNetworkUpdateRequest( uint8* _data );
		bool HandleApplicationUpdateRequest( uint8* _data );
		bool HandleRfPowerLevelSetResponse( uint8* _data );
		bool HandleSerialApiSetTimeoutsResponse( uint8* _data );
		bool HandleMemoryGetByteResponse( uint8* _data );
		bool HandleReadMemoryResponse( uint8* _data );
		void HandleGetVirtualNodesResponse( uint8* _data );
		bool HandleSetSlaveLearnModeResponse( uint8* _data );
		void HandleSetSlaveLearnModeRequest( uint8* _data );
		bool HandleSendSlaveNodeInfoResponse( uint8* _data );
		void HandleSendSlaveNodeInfoRequest( uint8* _data );
		void HandleApplicationSlaveCommandRequest( uint8* _data );
		void HandleSerialAPIResetRequest( uint8* _data );

		void CommonAddNodeStatusRequestHandler( uint8 _funcId, uint8* _data );

		bool					m_waitingForAck;							// True when we are waiting for an ACK from the dongle
		uint8					m_expectedCallbackId;						// If non-zero, we wait for a message with this callback Id
		uint8					m_expectedReply;							// If non-zero, we wait for a message with this function Id
		uint8					m_expectedCommandClassId;					// If the expected reply is FUNC_ID_APPLICATION_COMMAND_HANDLER, this value stores the command class we're waiting to hear from
		uint8					m_expectedNodeId;							// If we are waiting for a FUNC_ID_APPLICATION_COMMAND_HANDLER, make sure we only accept it from this node.

	//-----------------------------------------------------------------------------
	//	Polling Z-Wave devices
	//-----------------------------------------------------------------------------
	private:
		int32 GetPollInterval(){ return m_pollInterval ; }
		void SetPollInterval( int32 _milliseconds, bool _bIntervalBetweenPolls ){ m_pollInterval = _milliseconds; m_bIntervalBetweenPolls = _bIntervalBetweenPolls; }
		bool EnablePoll( const ValueID &_valueId, uint8 _intensity = 1 );
		bool DisablePoll( const ValueID &_valueId );
		bool isPolled( const ValueID &_valueId );
		void SetPollIntensity( const ValueID &_valueId, uint8 _intensity );
		static void PollThreadEntryPoint( Event* _exitEvent, void* _context );
		void PollThreadProc( Event* _exitEvent );

		Thread*					m_pollThread;								// Thread for polling devices on the Z-Wave network
		struct PollEntry
		{
			ValueID	m_id;
			uint8	m_pollCounter;
		};
OPENZWAVE_EXPORT_WARNINGS_OFF
		list<PollEntry>			m_pollList;									// List of nodes that need to be polled
OPENZWAVE_EXPORT_WARNINGS_ON
		Mutex*					m_pollMutex;								// Serialize access to the polling list
		int32					m_pollInterval;								// Time interval during which all nodes must be polled
		bool					m_bIntervalBetweenPolls;					// if true, the library intersperses m_pollInterval between polls; if false, the library attempts to complete all polls within m_pollInterval

	//-----------------------------------------------------------------------------
	//	Retrieving Node information
	//-----------------------------------------------------------------------------
	public:
		uint8 GetNodeNumber( Msg const* _msg )const{ return  ( _msg == NULL ? 0 : _msg->GetTargetNodeId() ); }

	private:
		/**
		 *  Creates a new Node object (deleting any previous Node object with the same nodeId) and
		 *  queues a full query of the node's parameters (starting at the beginning of the query
		 *  stages--Node::QueryStage_None).  This function will send Notification::Type_NodeAdded
		 *  and Notification::Type_NodeRemoved messages to identify these modifications.
		 *  \param _nodeId The node ID of the node to create and query.
		 *  \see Notification::Type_NodeAdded, Notification::Type_NodeRemoved, Node::QueryStage_None,
		 */
		void InitNode( uint8 const _nodeId, bool newNode = false );

		void InitAllNodes();												// Delete all nodes and fetch the data from the Z-Wave network again.

		bool IsNodeListeningDevice( uint8 const _nodeId );
		bool IsNodeFrequentListeningDevice( uint8 const _nodeId );
		bool IsNodeBeamingDevice( uint8 const _nodeId );
		bool IsNodeRoutingDevice( uint8 const _nodeId );
		bool IsNodeSecurityDevice( uint8 const _nodeId );
		uint32 GetNodeMaxBaudRate( uint8 const _nodeId );
		uint8 GetNodeVersion( uint8 const _nodeId );
		uint8 GetNodeSecurity( uint8 const _nodeId );
		uint8 GetNodeBasic( uint8 const _nodeId );
		uint8 GetNodeGeneric( uint8 const _nodeId );
		uint8 GetNodeSpecific( uint8 const _nodeId );
		string GetNodeType( uint8 const _nodeId );
		uint32 GetNodeNeighbors( uint8 const _nodeId, uint8** o_neighbors );

		string GetNodeManufacturerName( uint8 const _nodeId );
		string GetNodeProductName( uint8 const _nodeId );
		string GetNodeName( uint8 const _nodeId );
		string GetNodeLocation( uint8 const _nodeId );

		string GetNodeManufacturerId( uint8 const _nodeId );
		string GetNodeProductType( uint8 const _nodeId );
		string GetNodeProductId( uint8 const _nodeId );
		void SetNodeManufacturerName( uint8 const _nodeId, string const& _manufacturerName );
		void SetNodeProductName( uint8 const _nodeId, string const& _productName );
		void SetNodeName( uint8 const _nodeId, string const& _nodeName );
		void SetNodeLocation( uint8 const _nodeId, string const& _location );
		void SetNodeLevel( uint8 const _nodeId, uint8 const _level );
		void SetNodeOn( uint8 const _nodeId );
		void SetNodeOff( uint8 const _nodeId );

		Value* GetValue( ValueID const& _id );

		bool IsAPICallSupported( uint8 const _apinum )const{ return (( m_apiMask[( _apinum - 1 ) >> 3] & ( 1 << (( _apinum - 1 ) & 0x07 ))) != 0 ); }
		void SetAPICall( uint8 const _apinum, bool _toSet )
		{
			if( _toSet )
			{
				m_apiMask[( _apinum - 1 ) >> 3] |= ( 1 << (( _apinum - 1 ) & 0x07 ));
			}
			else
			{
				m_apiMask[( _apinum - 1 ) >> 3] &= ~( 1 << (( _apinum - 1 ) & 0x07 ));
			}
		}
		uint8 NodeFromMessage( uint8 const* buffer );

	//-----------------------------------------------------------------------------
	// Controller commands
	//-----------------------------------------------------------------------------
	public:
		/**
		 * Controller Commands.
		 * Commands to be used with the BeginControllerCommand method.
		 * \see Manager::BeginControllerCommand
		 */
		enum ControllerCommand
		{
			ControllerCommand_None = 0,					/**< No command. */
			ControllerCommand_AddDevice,					/**< Add a new device or controller to the Z-Wave network. */
			ControllerCommand_CreateNewPrimary,				/**< Add a new controller to the Z-Wave network. Used when old primary fails. Requires SUC. */
			ControllerCommand_ReceiveConfiguration,				/**< Receive Z-Wave network configuration information from another controller. */
			ControllerCommand_RemoveDevice,					/**< Remove a device or controller from the Z-Wave network. */
			ControllerCommand_RemoveFailedNode,				/**< Move a node to the controller's failed nodes list. This command will only work if the node cannot respond. */
			ControllerCommand_HasNodeFailed,				/**< Check whether a node is in the controller's failed nodes list. */
			ControllerCommand_ReplaceFailedNode,				/**< Replace a non-responding node with another. The node must be in the controller's list of failed nodes for this command to succeed. */
			ControllerCommand_TransferPrimaryRole,				/**< Make a different controller the primary. */
			ControllerCommand_RequestNetworkUpdate,				/**< Request network information from the SUC/SIS. */
			ControllerCommand_RequestNodeNeighborUpdate,			/**< Get a node to rebuild its neighbour list.  This method also does RequestNodeNeighbors */
			ControllerCommand_AssignReturnRoute,				/**< Assign a network return routes to a device. */
			ControllerCommand_DeleteAllReturnRoutes,			/**< Delete all return routes from a device. */
			ControllerCommand_SendNodeInformation,				/**< Send a node information frame */
			ControllerCommand_ReplicationSend,				/**< Send information from primary to secondary */
			ControllerCommand_CreateButton,					/**< Create an id that tracks handheld button presses */
			ControllerCommand_DeleteButton					/**< Delete id that tracks handheld button presses */
		};

		/**
		 * Controller States.
		 * States reported via the callback handler passed into the BeginControllerCommand method.
		 * \see Manager::BeginControllerCommand
	     */
		enum ControllerState
		{
			ControllerState_Normal = 0,				/**< No command in progress. */
			ControllerState_Starting,				/**< The command is starting. */
			ControllerState_Cancel,					/**< The command was cancelled. */
			ControllerState_Error,					/**< Command invocation had error(s) and was aborted */
			ControllerState_Waiting,				/**< Controller is waiting for a user action. */
			ControllerState_Sleeping,				/**< Controller command is on a sleep queue wait for device. */
			ControllerState_InProgress,				/**< The controller is communicating with the other device to carry out the command. */
			ControllerState_Completed,			    	/**< The command has completed successfully. */
			ControllerState_Failed,					/**< The command has failed. */
			ControllerState_NodeOK,					/**< Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node is OK. */
			ControllerState_NodeFailed				/**< Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node has failed. */
		};

		/**
		 * Controller Errors
		 * Provide some more information about controller failures.
		 */
		enum ControllerError
		{
			ControllerError_None = 0,
			ControllerError_ButtonNotFound,					/**< Button */
			ControllerError_NodeNotFound,					/**< Button */
			ControllerError_NotBridge,					/**< Button */
			ControllerError_NotSUC,						/**< CreateNewPrimary */
			ControllerError_NotSecondary,					/**< CreateNewPrimary */
			ControllerError_NotPrimary,					/**< RemoveFailedNode, AddNodeToNetwork */
			ControllerError_IsPrimary,					/**< ReceiveConfiguration */
			ControllerError_NotFound,					/**< RemoveFailedNode */
			ControllerError_Busy,						/**< RemoveFailedNode, RequestNetworkUpdate */
			ControllerError_Failed,						/**< RemoveFailedNode, RequestNetworkUpdate */
			ControllerError_Disabled,					/**< RequestNetworkUpdate error */
			ControllerError_Overflow					/**< RequestNetworkUpdate error */
		};

		typedef void (*pfnControllerCallback_t)( ControllerState _state, ControllerError _err, void* _context );

	private:
		// The public interface is provided via the wrappers in the Manager class
		void ResetController( Event* _evt );
		void SoftReset();
		void RequestNodeNeighbors( uint8 const _nodeId, uint32 const _requestFlags );

		bool BeginControllerCommand( ControllerCommand _command, pfnControllerCallback_t _callback, void* _context, bool _highPower, uint8 _nodeId, uint8 _arg );
		bool CancelControllerCommand();
		void AddNodeStop( uint8 const _funcId );					// Handle different controller behaviors

		struct ControllerCommandItem
		{
			ControllerState				m_controllerState;
			bool					m_controllerStateChanged;
			bool					m_controllerCommandDone;
			ControllerCommand			m_controllerCommand;
			pfnControllerCallback_t			m_controllerCallback;
			ControllerError				m_controllerReturnError;
			void*					m_controllerCallbackContext;
			bool					m_highPower;
			bool					m_controllerAdded;
			uint8					m_controllerCommandNode;
			uint8					m_controllerCommandArg;
		};

		ControllerCommandItem*			m_currentControllerCommand;

		void DoControllerCommand();
		void UpdateControllerState( ControllerState const _state, ControllerError const _error = ControllerError_None )
		{
			if( m_currentControllerCommand != NULL )
			{
				if( _state != m_currentControllerCommand->m_controllerState )
				{
					m_currentControllerCommand->m_controllerStateChanged = true;
					m_currentControllerCommand->m_controllerState = _state;
					switch( _state )
					{
						case ControllerState_Error:
						case ControllerState_Cancel:
						case ControllerState_Failed:
						case ControllerState_Sleeping:
						case ControllerState_NodeFailed:
						case ControllerState_NodeOK:
						case ControllerState_Completed:
						{
							m_currentControllerCommand->m_controllerCommandDone = true;
							m_sendMutex->Lock();
							m_queueEvent[MsgQueue_Controller]->Set();
							m_sendMutex->Unlock();
							break;
						}
						default:
						{
							break;
						}
					}

				}
				if( _error != ControllerError_None )
				{
					m_currentControllerCommand->m_controllerReturnError = _error;
				}
			}
		}

		uint8					m_SUCNodeId;

		void UpdateNodeRoutes( uint8 const_nodeId, bool _doUpdate = false );

		Event*					m_controllerResetEvent;

	//-----------------------------------------------------------------------------
	//	Sending Z-Wave messages
	//-----------------------------------------------------------------------------
	public:
		enum MsgQueue
		{
			MsgQueue_Command = 0,
			MsgQueue_Security,
			MsgQueue_NoOp,
			MsgQueue_Controller,
			MsgQueue_WakeUp,
			MsgQueue_Send,
			MsgQueue_Query,
			MsgQueue_Poll,
			MsgQueue_Count		// Number of message queues
		};

		void SendMsg( Msg* _msg, MsgQueue const _queue );

		/**
		 * Fetch the transmit options
		 */
		uint8 GetTransmitOptions()const{ return m_transmitOptions; }

	private:
		/**
		 *  If there are messages in the send queue (m_sendQueue), gets the next message in the
		 *  queue and writes it to the serial port.  In sending the message, SendMsg also initializes
		 *  variables tracking the message's callback ID (m_expectedCallbackId), expected reply
		 *  (m_expectedReply) and expected command class ID (m_expectedCommandClassId).  It also
		 *  sets m_waitingForAck to true and increments the message's send attempts counter.
		 *  <p>
		 *  If there are no messages in the send queue, then SendMsg checks the query queue to
		 *  see if there are any outstanding queries that can be processed (target node not asleep).
		 *  If so, it retrieves the Node object that needs to be queried and calls that node's
		 *  AdvanceQueries member function.  If this call results in all of the node's queries to be
		 *  completed, SendMsg will remove the node query item from the query queue.
		 *  \return TRUE if data was written, FALSE if not
		 *  \see Msg, m_sendQueue, m_expectedCallbackId, m_expectedReply, m_expectedCommandClassId,
		 *  m_waitingForAck, Msg::GetSendAttempts, Node::AdvanceQueries, GetCurrentNodeQuery,
		 *  RemoveNodeQuery, Node::AllQueriesCompleted
		 */
		bool WriteNextMsg( MsgQueue const _queue );							// Extracts the first message from the queue, and makes it the current one.
		bool WriteMsg( string const &str);									// Sends the current message to the Z-Wave network
		void RemoveCurrentMsg();											// Deletes the current message and cleans up the callback etc states
		bool MoveMessagesToWakeUpQueue(	uint8 const _targetNodeId, bool const _move );		// If a node does not respond, and is of a type that can sleep, this method is used to move all its pending messages to another queue ready for when it mext wakes up.
		bool HandleErrorResponse( uint8 const _error, uint8 const _nodeId, char const* _funcStr, bool _sleepCheck = false );									    // Handle data errors and process consistently. If message is moved to wake-up queue, return true.
		bool IsExpectedReply( uint8 const _nodeId );						// Determine if reply message is the one we are expecting
		void SendQueryStageComplete( uint8 const _nodeId, Node::QueryStage const _stage );
		void RetryQueryStageComplete( uint8 const _nodeId, Node::QueryStage const _stage );
		void CheckCompletedNodeQueries();									// Send notifications if all awake and/or sleeping nodes have completed their queries

		// Requests to be sent to nodes are assigned to one of five queues.
		// From highest to lowest priority, these are
		//
		// 0)   The security queue, for handling encrypted messages.  This is the
		//              highest priority send queue, because the security process inserts
		//              messages to handle the encryption process that must be sent before
		//              a new message can be wrapped.
		//
		// 1)	The command queue, for controller commands.  This is the 2nd highest
		//		priority send queue, because the controller command processes are not
		//		permitted to be interupted by other requests.
		//
		// 2)	The controller queue exists to handle multi-step controller commands. These
		//		typically require user interaction or affect the network in some way.
		//
		// 3)	The No Operation command class queue. This is used for device probing
		//		at startup as well as network diagostics.
		//
		// 4)	The wakeup queue.  This holds messages that have been held for a
		//		sleeping device that has now woken up.  These get a high priority
		//		because such devices do not stay awake for very long.
		//
		// 5)	The send queue.  This is for normal messages, usually triggered by
		//		a user interaction with the application.
		//
		// 6)	The query queue.  For node query messages sent when a new node is
		//		discovered.  The query process generates a large number of requests,
		//		so the query queue has a low priority to avoid making the system
		//		unresponsive.
		//
		// 7)   The poll queue.  Requests to devices that need their state polling
		//		at regular intervals.  These are of the lowest priority, and are only
		//		sent when nothing else is going on
		//
		enum MsgQueueCmd
		{
			MsgQueueCmd_SendMsg = 0,
			MsgQueueCmd_QueryStageComplete,
			MsgQueueCmd_Controller
		};

		class MsgQueueItem
		{
		public:
			MsgQueueItem() :
				m_msg(NULL),
				m_nodeId(0),
				m_queryStage(Node::QueryStage_None),
				m_retry(false),
				m_cci(NULL)
		  	{}

			bool operator == ( MsgQueueItem const& _other )const
			{
				if( _other.m_command == m_command )
				{
					if( m_command == MsgQueueCmd_SendMsg )
					{
						return( (*_other.m_msg) == (*m_msg) );
					}
					else if( m_command == MsgQueueCmd_QueryStageComplete )
					{
						return( (_other.m_nodeId == m_nodeId) && (_other.m_queryStage == m_queryStage) );
					}
					else if( m_command == MsgQueueCmd_Controller )
					{
						return( (_other.m_cci->m_controllerCommand == m_cci->m_controllerCommand) && (_other.m_cci->m_controllerCallback == m_cci->m_controllerCallback) );
					}
				}

				return false;
			}

			MsgQueueCmd			m_command;
			Msg*				m_msg;
			uint8				m_nodeId;
			Node::QueryStage		m_queryStage;
			bool				m_retry;
			ControllerCommandItem*		m_cci;
		};

OPENZWAVE_EXPORT_WARNINGS_OFF
		list<MsgQueueItem>			m_msgQueue[MsgQueue_Count];
OPENZWAVE_EXPORT_WARNINGS_ON
		Event*					m_queueEvent[MsgQueue_Count];				// Events for each queue, which are signalled when the queue is not empty
		Mutex*					m_sendMutex;						// Serialize access to the queues
		Msg*					m_currentMsg;
		MsgQueue				m_currentMsgQueueSource;			// identifies which queue held m_currentMsg
		TimeStamp				m_resendTimeStamp;

	//-----------------------------------------------------------------------------
	// Network functions
	//-----------------------------------------------------------------------------
	private:
		void TestNetwork( uint8 const _nodeId, uint32 const _count );

	//-----------------------------------------------------------------------------
	// Virtual Node commands
	//-----------------------------------------------------------------------------
	public:
		/**
		 * Virtual Node Commands.
		 * Commands to be used with virtual nodes.
		 */
	private:
		uint32 GetVirtualNeighbors( uint8** o_neighbors );
		void RequestVirtualNeighbors( MsgQueue const _queue );
		bool IsVirtualNode( uint8 const _nodeId )const{  return (( m_virtualNeighbors[( _nodeId - 1 ) >> 3] & 1 << (( _nodeId - 1 ) & 0x07 )) != 0 ); }
		void SendVirtualNodeInfo( uint8 const _fromNodeId, uint8 const _ToNodeId );
		void SendSlaveLearnModeOff();
		void SaveButtons();
		void ReadButtons( uint8 const _nodeId );

		bool		m_virtualNeighborsReceived;
		uint8		m_virtualNeighbors[NUM_NODE_BITFIELD_BYTES];		// Bitmask containing virtual neighbors

	//-----------------------------------------------------------------------------
	// SwitchAll
	//-----------------------------------------------------------------------------
	private:
		// The public interface is provided via the wrappers in the Manager class
		void SwitchAllOn();
		void SwitchAllOff();

	//-----------------------------------------------------------------------------
	// Configuration Parameters	(wrappers for the Node methods)
	//-----------------------------------------------------------------------------
	private:
		// The public interface is provided via the wrappers in the Manager class
		bool SetConfigParam( uint8 const _nodeId, uint8 const _param, int32 _value, uint8 const _size );
		void RequestConfigParam( uint8 const _nodeId, uint8 const _param );

	//-----------------------------------------------------------------------------
	// Groups (wrappers for the Node methods)
	//-----------------------------------------------------------------------------
	private:
		// The public interface is provided via the wrappers in the Manager class
		uint8 GetNumGroups( uint8 const _nodeId );
		uint32 GetAssociations( uint8 const _nodeId, uint8 const _groupIdx, uint8** o_associations );
		uint8 GetMaxAssociations( uint8 const _nodeId, uint8 const _groupIdx );
		string GetGroupLabel( uint8 const _nodeId, uint8 const _groupIdx );
		void AddAssociation( uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId );
		void RemoveAssociation( uint8 const _nodeId, uint8 const _groupIdx, uint8 const _targetNodeId );

	//-----------------------------------------------------------------------------
	//	Notifications
	//-----------------------------------------------------------------------------
	private:
		void QueueNotification( Notification* _notification );				// Adds a notification to the list.  Notifications are queued until a point in the thread where we know we do not have any nodes locked.
		void NotifyWatchers();												// Passes the notifications to all the registered watcher callbacks in turn.

OPENZWAVE_EXPORT_WARNINGS_OFF
		list<Notification*>		m_notifications;
OPENZWAVE_EXPORT_WARNINGS_ON
		Event*				m_notificationsEvent;

	//-----------------------------------------------------------------------------
	//	Statistics
	//-----------------------------------------------------------------------------
	public:
		struct DriverData
		{
			uint32 m_SOFCnt;			// Number of SOF bytes received
			uint32 m_ACKWaiting;			// Number of unsolicited messages while waiting for an ACK
			uint32 m_readAborts;			// Number of times read were aborted due to timeouts
			uint32 m_badChecksum;			// Number of bad checksums
			uint32 m_readCnt;			// Number of messages successfully read
			uint32 m_writeCnt;			// Number of messages successfully sent
			uint32 m_CANCnt;			// Number of CAN bytes received
			uint32 m_NAKCnt;			// Number of NAK bytes received
			uint32 m_ACKCnt;			// Number of ACK bytes received
			uint32 m_OOFCnt;			// Number of bytes out of framing
			uint32 m_dropped;			// Number of messages dropped & not delivered
			uint32 m_retries;			// Number of messages retransmitted
			uint32 m_callbacks;			// Number of unexpected callbacks
			uint32 m_badroutes;			// Number of failed messages due to bad route response
			uint32 m_noack;				// Number of no ACK returned errors
			uint32 m_netbusy;			// Number of network busy/failure messages
			uint32 m_notidle;
			uint32 m_nondelivery;			// Number of messages not delivered to network
			uint32 m_routedbusy;			// Number of messages received with routed busy status
			uint32 m_broadcastReadCnt;		// Number of broadcasts read
			uint32 m_broadcastWriteCnt;		// Number of broadcasts sent
		};

		void LogDriverStatistics();

	private:
		void GetDriverStatistics( DriverData* _data );
		void GetNodeStatistics( uint8 const _nodeId, Node::NodeData* _data );

		uint32 m_SOFCnt;			// Number of SOF bytes received
		uint32 m_ACKWaiting;			// Number of unsolcited messages while waiting for an ACK
		uint32 m_readAborts;			// Number of times read were aborted due to timeouts
		uint32 m_badChecksum;			// Number of bad checksums
		uint32 m_readCnt;			// Number of messages successfully read
		uint32 m_writeCnt;			// Number of messages successfully sent
		uint32 m_CANCnt;			// Number of CAN bytes received
		uint32 m_NAKCnt;			// Number of NAK bytes received
		uint32 m_ACKCnt;			// Number of ACK bytes received
		uint32 m_OOFCnt;			// Number of bytes out of framing
		uint32 m_dropped;			// Number of messages dropped & not delivered
		uint32 m_retries;			// Number of retransmitted messages
		uint32 m_callbacks;			// Number of unexpected callbacks
		uint32 m_badroutes;			// Number of failed messages due to bad route response
		uint32 m_noack;				// Number of no ACK returned errors
		uint32 m_netbusy;			// Number of network busy/failure messages
		uint32 m_notidle;			// Number of not idle messages
		uint32 m_nondelivery;			// Number of messages not delivered to network
		uint32 m_routedbusy;			// Number of messages received with routed busy status
		uint32 m_broadcastReadCnt;		// Number of broadcasts read
		uint32 m_broadcastWriteCnt;		// Number of broadcasts sent
		//time_t m_commandStart;	// Start time of last command
		//time_t m_timeoutLost;		// Cumulative time lost to timeouts


	//-----------------------------------------------------------------------------
	//	Security Command Class Related (Version 1.1)
	//-----------------------------------------------------------------------------
	private:
		uint8 *GetNetworkKey();
	};

} // namespace OpenZWave

#endif // _Driver_H
