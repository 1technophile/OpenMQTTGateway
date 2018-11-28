//-----------------------------------------------------------------------------
//
//	WakeUp.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_WAKE_UP
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
#include "WakeUp.h"
#include "MultiCmd.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../Notification.h"
#include "../Options.h"
#include "../platform/Log.h"
#include "../platform/Mutex.h"
#include "../value_classes/ValueInt.h"

using namespace OpenZWave;

enum WakeUpCmd
{
	WakeUpCmd_IntervalSet		= 0x04,
	WakeUpCmd_IntervalGet		= 0x05,
	WakeUpCmd_IntervalReport	= 0x06,
	WakeUpCmd_Notification		= 0x07,
	WakeUpCmd_NoMoreInformation	= 0x08,
	WakeUpCmd_IntervalCapabilitiesGet = 0x09,
	WakeUpCmd_IntervalCapabilitiesReport = 0x0A
};


//-----------------------------------------------------------------------------
// <WakeUp::WakeUp>
// Constructor
//-----------------------------------------------------------------------------
WakeUp::WakeUp
( 
	uint32 const _homeId,
	uint8 const _nodeId 
):
	CommandClass( _homeId, _nodeId ), 
	m_mutex( new Mutex() ),
	m_pollRequired( false ),
	m_notification( false )
{
        m_awake = true;
        Options::Get()->GetOptionAsBool("AssumeAwake", &m_awake);

	SetStaticRequest( StaticRequest_Values );
}

//-----------------------------------------------------------------------------
// <WakeUp::~WakeUp>
// Destructor
//-----------------------------------------------------------------------------
WakeUp::~WakeUp
( 
)
{
	m_mutex->Release();
	while( !m_pendingQueue.empty() )
	{
		Driver::MsgQueueItem const& item = m_pendingQueue.front();
		if( Driver::MsgQueueCmd_SendMsg == item.m_command )
		{
			delete item.m_msg;
		}
		else if( Driver::MsgQueueCmd_Controller == item.m_command )
		{
			delete item.m_cci;
		}
		m_pendingQueue.pop_front();
	}
}

//-----------------------------------------------------------------------------
// <WakeUp::Init>
// Starts the process of requesting node state from a sleeping device
//-----------------------------------------------------------------------------
void WakeUp::Init
( 
)
{
	// Request the wake up interval.  When we receive the response, we
	// can send a set interval message with the same interval, but with
	// the target node id set to that of the controller.  This will ensure
	// that the controller will receive the wake-up notifications from
	// the device.  Once this is done, we can request the rest of the node
	// state.
	RequestState( CommandClass::RequestFlag_Session, 1, Driver::MsgQueue_WakeUp );
}

//-----------------------------------------------------------------------------
// <WakeUp::RequestState>
// Nothing to do for wakeup
//-----------------------------------------------------------------------------
bool WakeUp::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		if( GetVersion() > 1 )
		{
			requests |= RequestValue( _requestFlags, WakeUpCmd_IntervalCapabilitiesGet, _instance, _queue );
		}
	}
	if( _requestFlags & RequestFlag_Session )
	{
		Node* node = GetNodeUnsafe();
		if( node != NULL && !node->IsController() )
		{
			requests |= RequestValue( _requestFlags, 0, _instance, _queue );
		}
	}

	return requests;
}

//-----------------------------------------------------------------------------
// <WakeUp::RequestValue>
// Nothing to do for wakeup
//-----------------------------------------------------------------------------
bool WakeUp::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _getTypeEnum,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}

	if( _getTypeEnum == WakeUpCmd_IntervalCapabilitiesGet )
	{
		Msg* msg = new Msg( "WakeUpCmd_IntervalCapabilityGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( WakeUpCmd_IntervalCapabilitiesGet );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
	}


	if( _getTypeEnum == 0 )
	{
		// We won't get a response until the device next wakes up
		Msg* msg = new Msg( "WakeUpCmd_IntervalGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( WakeUpCmd_IntervalGet );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <WakeUp::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool WakeUp::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( WakeUpCmd_IntervalReport == (WakeUpCmd)_data[0] )
	{	
		if( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, 0 ) ) )
		{
			// some interval reports received are validly formatted (proper checksum, etc.) but only have length
			// of 3 (0x84 (classid), 0x06 (IntervalReport), 0x00).  Not sure what this means
			if( _length < 6 )
			{
				Log::Write( LogLevel_Warning, "" );
				Log::Write( LogLevel_Warning, GetNodeId(), "Unusual response: WakeUpCmd_IntervalReport with len = %d.  Ignored.", _length );
				value->Release();
				return false;
			}

			uint32 interval = ((uint32)_data[1]) << 16;
			interval |= (((uint32)_data[2]) << 8);
			interval |= (uint32)_data[3];

			uint8 targetNodeId = _data[4];

			Log::Write( LogLevel_Info, GetNodeId(), "Received Wakeup Interval report from node %d: Interval=%d, Target Node=%d", GetNodeId(), interval, targetNodeId );

			value->OnValueRefreshed( (int32)interval );
		
			// Ensure that the target node for wake-up notifications is the controller
			// but only if node is not a listening device. Hybrid devices that can be
			// powered by other then batteries shouldn't do this.
			Node *node = GetNodeUnsafe();
			if( GetDriver()->GetNodeId() != targetNodeId && ((node) && (!node->IsListeningDevice())) )
			{
				SetValue( *value );	
			}
			value->Release();
 		}
		return true;
	}
	else if( WakeUpCmd_Notification == (WakeUpCmd)_data[0] )
	{	
		// The device is awake.
		Log::Write( LogLevel_Info, GetNodeId(), "Received Wakeup Notification from node %d", GetNodeId() );
		m_notification = true;
		SetAwake( true );				
		return true;
	}
	else if( WakeUpCmd_IntervalCapabilitiesReport == (WakeUpCmd)_data[0] )
	{	
		uint32 mininterval = (((uint32)_data[1]) << 16) | (((uint32)_data[2]) << 8) | ((uint32)_data[3]);
		uint32 maxinterval = (((uint32)_data[4]) << 16) | (((uint32)_data[5]) << 8) | ((uint32)_data[6]);
		uint32 definterval = (((uint32)_data[7]) << 16) | (((uint32)_data[8]) << 8) | ((uint32)_data[9]);
		uint32 stepinterval = (((uint32)_data[10]) << 16) | (((uint32)_data[11]) << 8) | ((uint32)_data[12]);
		Log::Write( LogLevel_Info, GetNodeId(), "Received Wakeup Interval Capability report from node %d: Min Interval=%d, Max Interval=%d, Default Interval=%d, Interval Step=%d", GetNodeId(), mininterval, maxinterval, definterval, stepinterval );
		if( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, 1 ) ) )
		{
			value->OnValueRefreshed( (int32)mininterval );
			value->Release();
		}
		if( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, 2 ) ) )
		{
			value->OnValueRefreshed( (int32)maxinterval );
			value->Release();
		}
		if( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, 3 ) ) )
		{
			value->OnValueRefreshed( (int32)definterval );
			value->Release();
		}
		if( ValueInt* value = static_cast<ValueInt*>( GetValue( _instance, 4 ) ) )
		{
			value->OnValueRefreshed( (int32)stepinterval );
			value->Release();
		}
		ClearStaticRequest( StaticRequest_Values );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <WakeUp::SetValue>
// Set the device's wakeup interval
//-----------------------------------------------------------------------------
bool WakeUp::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Int == _value.GetID().GetType() )
	{
		ValueInt const* value = static_cast<ValueInt const*>(&_value);
	
		Msg* msg = new Msg( "Wakeup Interval Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		
		if( GetNodeUnsafe()->GetCommandClass( MultiCmd::StaticGetCommandClassId() ) )
		{
			msg->Append( 10 );
			msg->Append( MultiCmd::StaticGetCommandClassId() );
			msg->Append( MultiCmd::MultiCmdCmd_Encap );
			msg->Append( 1 );
		}

		int32 interval = value->GetValue();

		msg->Append( 6 );	// length of command bytes following
		msg->Append( GetCommandClassId() );
		msg->Append( WakeUpCmd_IntervalSet );
		msg->Append( (uint8)(( interval >> 16 ) & 0xff) ); 
		msg->Append( (uint8)(( interval >> 8 ) & 0xff) );	 
		msg->Append( (uint8)( interval & 0xff ) );		
		msg->Append( GetDriver()->GetNodeId() );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_WakeUp );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <WakeUp::SetVersion>
// Set the command class version
//-----------------------------------------------------------------------------
void WakeUp::SetVersion
(
	uint8 const _version
)
{
	CommandClass::SetVersion( _version );
	CreateVars( 1 );
}

//-----------------------------------------------------------------------------
// <WakeUp::SetAwake>
// Set whether the device is likely to be awake
//-----------------------------------------------------------------------------
void WakeUp::SetAwake
(
	bool _state
)
{
	if( m_awake != _state )
	{
		m_awake = _state;
		Log::Write( LogLevel_Info, GetNodeId(), "  Node %d has been marked as %s", GetNodeId(), m_awake ? "awake" : "asleep" );
		Notification* notification = new Notification( Notification::Type_Notification );
		notification->SetHomeAndNodeIds( GetHomeId(), GetNodeId() );
		notification->SetNotification( m_awake ? Notification::Code_Awake : Notification::Code_Sleep );
		GetDriver()->QueueNotification( notification );

	}

	if( m_awake )
	{
		// If the device is marked for polling, request the current state
		Node* node = GetNodeUnsafe();
		if( m_pollRequired )
		{
			if( node != NULL )
			{
				node->SetQueryStage( Node::QueryStage_Dynamic );
			}
			m_pollRequired = false;
		}
			
		// Send all pending messages
		SendPending();
	}
}

//-----------------------------------------------------------------------------
// <WakeUp::QueueMsg>
// Add a Z-Wave message to the queue
//-----------------------------------------------------------------------------
void WakeUp::QueueMsg
(
	Driver::MsgQueueItem const& _item
)
{
	m_mutex->Lock();

	// See if there is already a copy of this message in the queue.  If so, 
	// we delete it.  This is to prevent duplicates building up if the 
	// device does not wake up very often.  Deleting the original and
	// adding the copy to the end avoids problems with the order of
	// commands such as on and off.
	list<Driver::MsgQueueItem>::iterator it = m_pendingQueue.begin();
	while( it != m_pendingQueue.end() )
	{
		Driver::MsgQueueItem const& item = *it;
		if( item == _item )
		{
			// Duplicate found
			if( Driver::MsgQueueCmd_SendMsg == item.m_command )
			{
				delete item.m_msg;
			}
			else if( Driver::MsgQueueCmd_Controller == item.m_command )
			{
				delete item.m_cci;
			}
			m_pendingQueue.erase( it++ );
		}
		else
		{
			++it;
		}
	}
	m_pendingQueue.push_back( _item );
	m_mutex->Unlock();
}

//-----------------------------------------------------------------------------
// <WakeUp::SendPending>
// The device is awake, so send all the pending messages
//-----------------------------------------------------------------------------
void WakeUp::SendPending
(
)
{
	m_awake = true;

	m_mutex->Lock();
	list<Driver::MsgQueueItem>::iterator it = m_pendingQueue.begin();
	while( it != m_pendingQueue.end() )
	{	
		Driver::MsgQueueItem const& item = *it;
		if( Driver::MsgQueueCmd_SendMsg == item.m_command )
		{
			GetDriver()->SendMsg( item.m_msg, Driver::MsgQueue_WakeUp );
		}
		else if( Driver::MsgQueueCmd_QueryStageComplete == item.m_command )
		{
			GetDriver()->SendQueryStageComplete( item.m_nodeId, item.m_queryStage );
		} else if( Driver::MsgQueueCmd_Controller == item.m_command )
		{
			GetDriver()->BeginControllerCommand( item.m_cci->m_controllerCommand, item.m_cci->m_controllerCallback, item.m_cci->m_controllerCallbackContext, item.m_cci->m_highPower, item.m_cci->m_controllerCommandNode, item.m_cci->m_controllerCommandArg );
			delete item.m_cci;
		}
		it = m_pendingQueue.erase( it );
	}
	m_mutex->Unlock();

	// Send the device back to sleep, unless we have outstanding queries.
	bool sendToSleep = m_notification;
	Node* node = GetNodeUnsafe();
	if( node != NULL )
	{
		if( !node->AllQueriesCompleted() )
		{
			sendToSleep = false;
		}
	}

	if( sendToSleep )
	{
		m_notification = false;
		Msg* msg = new Msg( "Wakeup - No More Information (send to sleep)", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( WakeUpCmd_NoMoreInformation );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_WakeUp );
	}
}

//-----------------------------------------------------------------------------
// <WakeUp::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void WakeUp::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		if( !node->IsController() )	// We don't add the interval value for controllers, because they don't appear to ever wake up on their own.
		{
			switch( GetVersion() )
			{
				case 1:
				{
				  	node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 0, "Wake-up Interval", "Seconds", false, false, 3600, 0 );
					break;
				}
				case 2:
				{
				  	node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 1, "Minimum Wake-up Interval", "Seconds", true, false, 0, 0 );
					node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 2, "Maximum Wake-up Interval", "Seconds", true, false, 0, 0 );
					node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 3, "Default Wake-up Interval", "Seconds", true, false, 0, 0 );
					node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 4, "Wake-up Interval Step", "Seconds", true, false, 0, 0 );
					break;
				}
			}
		}
	}
}
