//-----------------------------------------------------------------------------
//
//	Notification.h
//
//	Contains details of a Z-Wave event reported to the user
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

#ifndef _Notification_H
#define _Notification_H

#include "Defs.h"
#include "value_classes/ValueID.h"

namespace OpenZWave
{
	/** \brief Provides a container for data sent via the notification callback
	 *    handler installed by a call to Manager::AddWatcher.
	 *
	 *    A notification object is only ever created or deleted internally by
	 *    OpenZWave.
	 */
	class OPENZWAVE_EXPORT Notification
	{
		friend class Manager;
		friend class Driver;
		friend class Node;
		friend class Group;
		friend class Value;
		friend class ValueStore;
		friend class Basic;
		friend class ManufacturerSpecific;
		friend class NodeNaming;
		friend class NoOperation;
		friend class SceneActivation;
		friend class WakeUp;

	public:
		/**
		 * Notification types.
		 * Notifications of various Z-Wave events sent to the watchers
		 * registered with the Manager::AddWatcher method.
		 * \see Manager::AddWatcher
	     */
		enum NotificationType
		{
			Type_ValueAdded = 0,					/**< A new node value has been added to OpenZWave's list. These notifications occur after a node has been discovered, and details of its command classes have been received.  Each command class may generate one or more values depending on the complexity of the item being represented.  */
			Type_ValueRemoved,					/**< A node value has been removed from OpenZWave's list.  This only occurs when a node is removed. */
			Type_ValueChanged,					/**< A node value has been updated from the Z-Wave network and it is different from the previous value. */
			Type_ValueRefreshed,					/**< A node value has been updated from the Z-Wave network. */
			Type_Group,						/**< The associations for the node have changed. The application should rebuild any group information it holds about the node. */
			Type_NodeNew,						/**< A new node has been found (not already stored in zwcfg*.xml file) */
			Type_NodeAdded,						/**< A new node has been added to OpenZWave's list.  This may be due to a device being added to the Z-Wave network, or because the application is initializing itself. */
			Type_NodeRemoved,					/**< A node has been removed from OpenZWave's list.  This may be due to a device being removed from the Z-Wave network, or because the application is closing. */
			Type_NodeProtocolInfo,					/**< Basic node information has been receievd, such as whether the node is a listening device, a routing device and its baud rate and basic, generic and specific types. It is after this notification that you can call Manager::GetNodeType to obtain a label containing the device description. */
			Type_NodeNaming,					/**< One of the node names has changed (name, manufacturer, product). */
			Type_NodeEvent,						/**< A node has triggered an event.  This is commonly caused when a node sends a Basic_Set command to the controller.  The event value is stored in the notification. */
			Type_PollingDisabled,					/**< Polling of a node has been successfully turned off by a call to Manager::DisablePoll */
			Type_PollingEnabled,					/**< Polling of a node has been successfully turned on by a call to Manager::EnablePoll */
			Type_SceneEvent,					/**< Scene Activation Set received */
			Type_CreateButton,					/**< Handheld controller button event created */
			Type_DeleteButton,					/**< Handheld controller button event deleted */
			Type_ButtonOn,						/**< Handheld controller button on pressed event */
			Type_ButtonOff,						/**< Handheld controller button off pressed event */
			Type_DriverReady,					/**< A driver for a PC Z-Wave controller has been added and is ready to use.  The notification will contain the controller's Home ID, which is needed to call most of the Manager methods. */
			Type_DriverFailed,					/**< Driver failed to load */
			Type_DriverReset,					/**< All nodes and values for this driver have been removed.  This is sent instead of potentially hundreds of individual node and value notifications. */
			Type_EssentialNodeQueriesComplete,			/**< The queries on a node that are essential to its operation have been completed. The node can now handle incoming messages. */
			Type_NodeQueriesComplete,				/**< All the initialisation queries on a node have been completed. */
			Type_AwakeNodesQueried,					/**< All awake nodes have been queried, so client application can expected complete data for these nodes. */
			Type_AllNodesQueriedSomeDead,				/**< All nodes have been queried but some dead nodes found. */
			Type_AllNodesQueried,					/**< All nodes have been queried, so client application can expected complete data. */
			Type_Notification,					/**< An error has occured that we need to report. */
			Type_DriverRemoved					/**< The Driver is being removed. (either due to Error or by request) Do Not Call Any Driver Related Methods after recieving this call */
		};

		/**
		 * Notification codes.
		 * Notifications of the type Type_Notification convey some
		 * extra information defined here.
		 */
		enum NotificationCode
		{
			Code_MsgComplete = 0,					/**< Completed messages */
			Code_Timeout,						/**< Messages that timeout will send a Notification with this code. */
			Code_NoOperation,					/**< Report on NoOperation message sent completion  */
			Code_Awake,						/**< Report when a sleeping node wakes up */
			Code_Sleep,						/**< Report when a node goes to sleep */
			Code_Dead,						/**< Report when a node is presumed dead */
			Code_Alive						/**< Report when a node is revived */
		};

		/**
		 * Get the type of this notification.
		 * \return the notification type.
		 * \see NotificationType
	     */
		NotificationType GetType()const{ return m_type; }

		/**
		 * Get the Home ID of the driver sending this notification.
		 * \return the driver Home ID
	     */
		uint32 GetHomeId()const{ return m_valueId.GetHomeId(); }

		/**
		 * Get the ID of any node involved in this notification.
		 * \return the node's ID
	     */
		uint8 GetNodeId()const{ return m_valueId.GetNodeId(); }

		/**
		 * Get the unique ValueID of any value involved in this notification.
		 * \return the value's ValueID
		 */
		ValueID const& GetValueID()const{ return m_valueId; }

		/**
		 * Get the index of the association group that has been changed.  Only valid in NotificationType::Type_Group notifications.
		 * \return the group index.
		 */
		uint8 GetGroupIdx()const{ assert(Type_Group==m_type); return m_byte; }

		/**
		 * Get the event value of a notification.  Only valid in NotificationType::Type_NodeEvent notifications.
		 * \return the event value.
		 */
		uint8 GetEvent()const{ assert(Type_NodeEvent==m_type); return m_byte; }

		/**
		 * Get the button id of a notification.  Only valid in NotificationType::Type_CreateButton, DeleteButton,
		 * ButtonOn and ButtonOff notifications.
		 * \return the button id.
		 */
		uint8 GetButtonId()const{ assert(Type_CreateButton==m_type || Type_DeleteButton==m_type || Type_ButtonOn==m_type || Type_ButtonOff==m_type); return m_byte; }

		/**
		 * Get the scene Id of a notification.  Only valid in NotificationType::Type_SceneEvent notifications.
		 * \return the event value.
		 */
		uint8 GetSceneId()const{ assert(Type_SceneEvent==m_type); return m_byte; }

		/**
		 * Get the notification code from a notification. Only valid for NotificationType::Type_Notification notifications.
		 * \return the notification code.
		 */
		uint8 GetNotification()const{ assert(Type_Notification==m_type); return m_byte; }

		/**
		 * Helper function to simplify wrapping the notification class.  Should not normally need to be called.
		 * \return the internal byte value of the notification.
		 */
		uint8 GetByte()const{ return m_byte; }

	private:
		Notification( NotificationType _type ): m_type( _type ), m_byte(0){}
		~Notification(){}

		void SetHomeAndNodeIds( uint32 const _homeId, uint8 const _nodeId ){ m_valueId = ValueID( _homeId, _nodeId ); }
		void SetHomeNodeIdAndInstance ( uint32 const _homeId, uint8 const _nodeId, uint32 const _instance ){ m_valueId = ValueID( _homeId, _nodeId, _instance ); }
		void SetValueId( ValueID const& _valueId ){ m_valueId = _valueId; }
		void SetGroupIdx( uint8 const _groupIdx ){ assert(Type_Group==m_type); m_byte = _groupIdx; }
		void SetEvent( uint8 const _event ){ assert(Type_NodeEvent==m_type); m_byte = _event; }
		void SetSceneId( uint8 const _sceneId ){ assert(Type_SceneEvent==m_type); m_byte = _sceneId; }
		void SetButtonId( uint8 const _buttonId ){ assert(Type_CreateButton==m_type||Type_DeleteButton==m_type||Type_ButtonOn==m_type||Type_ButtonOff==m_type); m_byte = _buttonId; }
		void SetNotification( uint8 const _noteId ){ assert(Type_Notification==m_type); m_byte = _noteId; }

		NotificationType		m_type;
		ValueID				m_valueId;
		uint8				m_byte;
	};

} //namespace OpenZWave

#endif //_Notification_H

