//-----------------------------------------------------------------------------
//
//	ControllerReplication.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_CONTROLLER_REPLICATION
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
#include "ControllerReplication.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../platform/Log.h"

#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueList.h"
#include "../value_classes/ValueButton.h"

using namespace OpenZWave;

enum ControllerReplicationCmd
{
	ControllerReplicationCmd_TransferGroup		= 0x31,
	ControllerReplicationCmd_TransferGroupName	= 0x32,
	ControllerReplicationCmd_TransferScene		= 0x33,
	ControllerReplicationCmd_TransferSceneName	= 0x34
};

enum
{
	ControllerReplicationIndex_NodeId = 0,
	ControllerReplicationIndex_Function,
	ControllerReplicationIndex_Replicate
};

static char const* c_controllerReplicationFunctionNames[] = 
{
	"Groups",
	"Group Names",
	"Scenes",
	"Scene Names",
};

//-----------------------------------------------------------------------------
// <ControllerReplication::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
ControllerReplication::ControllerReplication
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),
	m_busy( false ),
	m_targetNodeId( 0 ),
	m_funcId( 0 ),
	m_nodeId( -1 ),
	m_groupCount( -1 ),
	m_groupIdx( -1 )
{
}

//-----------------------------------------------------------------------------
// <ControllerReplication::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ControllerReplication::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	// When creating replication messages, use FUNC_ID_ZW_SEND_REPLICATION_DATA instead of FUNC_ID_ZW_SEND_DATA
	// e.g. Msg* msg = new Msg( "TransferGroup", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_REPLICATION_DATA, true, false );

	switch( _data[0] )
	{
		case ControllerReplicationCmd_TransferGroup:
		{
			break;
		}
		case ControllerReplicationCmd_TransferGroupName:
		{
			break;
		}
		case  ControllerReplicationCmd_TransferScene:
		{
			break;
		}
		case ControllerReplicationCmd_TransferSceneName:
		{
			break;
		}
	}

	Msg* msg = new Msg( "ControllerReplication Command Complete", GetNodeId(), REQUEST, FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE, false, false );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Command );
	return true;
}

//-----------------------------------------------------------------------------
// <ControllerReplication::SetValue>
// Set a value on the Z-Wave device
//-----------------------------------------------------------------------------
bool ControllerReplication::SetValue
(
	Value const& _value
)
{
	bool res = false;
	uint8 instance = _value.GetID().GetInstance();

	switch( _value.GetID().GetIndex() )
	{
		case ControllerReplicationIndex_NodeId:
		{
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, ControllerReplicationIndex_NodeId ) ) )
			{
				value->OnValueRefreshed( (static_cast<ValueByte const*>( &_value))->GetValue() );
				value->Release();
				res = true;
			}
			break;
		}
		case ControllerReplicationIndex_Function:
		{
			if( ValueList* value = static_cast<ValueList*>( GetValue( instance, ControllerReplicationIndex_Function ) ) )
			{
				ValueList::Item const& item = (static_cast<ValueList const*>( &_value))->GetItem();
				value->OnValueRefreshed( item.m_value );
				value->Release();
				res = true;
			}
			break;
		}
		case ControllerReplicationIndex_Replicate:
		{
			if( ValueButton* button = static_cast<ValueButton*>( GetValue( instance, ControllerReplicationIndex_Replicate ) ) )
			{
				if( button->IsPressed() )
				{
					res = StartReplication( instance );
				}
				button->Release();
			}
			break;
		}
	}
	return res;
}

//-----------------------------------------------------------------------------
// <ControllerReplication::StartReplication>
// Set up the group and scene data to be sent to the other controller
//-----------------------------------------------------------------------------
bool ControllerReplication::StartReplication
(
	uint8 const _instance
)
{
	if( m_busy )
	{
		return false;
	}

	if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, ControllerReplicationIndex_NodeId ) ) )
	{
		m_targetNodeId = value->GetValue();
		value->Release();
	}
	else
	{
		return false;
	}

	if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, ControllerReplicationIndex_Function ) ) )
	{
		ValueList::Item const& item = value->GetItem();
		m_funcId = item.m_value;
		value->Release();
	}
	else
	{
		return false;
	}

	// Store the Z-Wave command we should use when replication has completed.
	m_nodeId = -1;
	m_groupCount = -1;
	m_groupIdx = -1;
	m_busy = true;

	// Set up the groups and scenes to be sent
	SendNextData();
	return true;
}

//-----------------------------------------------------------------------------
// <ControllerReplication::SendNextData>
// Send the next block of replication data
//-----------------------------------------------------------------------------
void ControllerReplication::SendNextData
(
)
{
	uint16 i = 255;

	if( !m_busy )
	{
		return;
	}

	while( 1 )
	{
		if( m_groupIdx != -1 )
		{
			m_groupIdx++;
			if( m_groupIdx <= m_groupCount )
			{
				break;
			}
		}
		i = m_nodeId == -1 ? 0 : m_nodeId+1;
		GetDriver()->LockNodes();
		while( i < 256 )
		{
			if( GetDriver()->m_nodes[i] )
			{
				m_groupCount = GetDriver()->m_nodes[i]->GetNumGroups();
				if( m_groupCount != 0 )
				{
					m_groupName = GetDriver()->m_nodes[i]->GetGroupLabel( m_groupIdx );
					m_groupIdx = m_groupName.length() > 0 ? 0 : 1;
					break;
				}
			}
			i++;
		}
		GetDriver()->ReleaseNodes();
		m_nodeId = i;
		break;
	}
	if( i < 255 )
	{
		Msg* msg = new Msg( "Replication Send", m_targetNodeId, REQUEST, FUNC_ID_ZW_REPLICATION_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( m_targetNodeId );
		if( m_groupName.length() > 0 )
		{		
			msg->Append((uint8) (m_groupName.length() + 4 ));
			msg->Append( GetCommandClassId() );
			msg->Append( ControllerReplicationCmd_TransferGroupName );
			msg->Append( 0 );
			msg->Append( m_groupIdx );
			for( uint8 j = 0; j < m_groupName.length(); j++ )
			{
				msg->Append( m_groupName[j] );
			}
			m_groupName = "";
		}
		else
		{
			msg->Append( 5 );
			msg->Append( GetCommandClassId() );
			msg->Append( ControllerReplicationCmd_TransferGroup );
			msg->Append( 0 );
			msg->Append( m_groupIdx );
			msg->Append( m_nodeId );
		}
		msg->Append( TRANSMIT_OPTION_ACK );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Command );
	}
	else
	{
		GetDriver()->AddNodeStop( m_funcId );
		m_busy = false;
	}
}

//-----------------------------------------------------------------------------
// <ControllerReplication::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void ControllerReplication::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ControllerReplicationIndex_NodeId, "Node", "", false, false, 0, 0 );
		vector<ValueList::Item> items;

		ValueList::Item item;
		for( uint8 i=0; i<4; ++i )
		{
			item.m_label = c_controllerReplicationFunctionNames[i];
			item.m_value = ControllerReplicationCmd_TransferGroup + i;
			items.push_back( item ); 
		}

		node->CreateValueList( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ControllerReplicationIndex_Function, "Functions", "", false, false, 1, items, 0, 0 );
		node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, ControllerReplicationIndex_Replicate, "Replicate", 0 );
	}
}
