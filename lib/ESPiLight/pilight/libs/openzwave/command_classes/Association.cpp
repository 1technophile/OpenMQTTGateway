//-----------------------------------------------------------------------------
//
//	Association.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ASSOCIATION
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
#include "Association.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../Group.h"
#include "../platform/Log.h"

using namespace OpenZWave;

enum AssociationCmd
{
	AssociationCmd_Set				= 0x01,
	AssociationCmd_Get				= 0x02,
	AssociationCmd_Report			= 0x03,
	AssociationCmd_Remove			= 0x04,
	AssociationCmd_GroupingsGet		= 0x05,
	AssociationCmd_GroupingsReport	= 0x06
};


//-----------------------------------------------------------------------------
// <Association::Association>
// Constructor
//-----------------------------------------------------------------------------
Association::Association
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),
	m_queryAll(false),
	m_numGroups(0)
{
	SetStaticRequest( StaticRequest_Values );
}

//-----------------------------------------------------------------------------
// <Association::ReadXML>
// Read the saved association data
//-----------------------------------------------------------------------------
void Association::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );

	TiXmlElement const* associationsElement = _ccElement->FirstChildElement();
	while( associationsElement )
	{
		char const* str = associationsElement->Value();
		if( str && !strcmp( str, "Associations" ) )
		{
			int intVal;
			if( TIXML_SUCCESS == associationsElement->QueryIntAttribute( "num_groups", &intVal ) )
			{
				m_numGroups = (uint8)intVal;
			}

			TiXmlElement const* groupElement = associationsElement->FirstChildElement();
			while( groupElement )
			{
				if( Node* node = GetNodeUnsafe() )
				{
					Group* group = new Group( GetHomeId(), GetNodeId(), groupElement );
					node->AddGroup( group );
				}

				groupElement = groupElement->NextSiblingElement();
			}

			break;
		}

		associationsElement = associationsElement->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Association::WriteXML>
// Save the association data
//-----------------------------------------------------------------------------
void Association::WriteXML
(
	TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML( _ccElement );

	if( Node* node = GetNodeUnsafe() )
	{
		TiXmlElement* associationsElement = new TiXmlElement( "Associations" );

		char str[8];
		snprintf( str, 8, "%d", m_numGroups );
		associationsElement->SetAttribute( "num_groups", str );

		_ccElement->LinkEndChild( associationsElement );
		node->WriteGroups( associationsElement );
	}
}

//-----------------------------------------------------------------------------
// <Association::RequestState>
// Nothing to do for Association
//-----------------------------------------------------------------------------
bool Association::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		// Request the supported group info
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Association::RequestValue>
// Nothing to do for Association
//-----------------------------------------------------------------------------
bool Association::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}
	// Request the supported group info
	Msg* msg = new Msg( "Get Association Groupings", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_GroupingsGet );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <Association::RequestAllGroups>
// Request the contents of each group in turn
//-----------------------------------------------------------------------------
void Association::RequestAllGroups
(
	uint32 const _requestFlags
)
{
	m_queryAll = true;

	// Request the contents of the individual groups in turn.
	if( m_numGroups == 0xff )
	{
		// We start with group 255, and will then move to group 1, 2 etc and stop when we find a group with a maxAssociations of zero.
		Log::Write( LogLevel_Info, GetNodeId(), "Number of association groups reported for node %d is 255, which requires special case handling.", GetNodeId() );
		QueryGroup( 0xff, _requestFlags );
	}
	else
	{
		// We start with group 1, and will then move to group 2, 3 etc and stop when the group index is greater than m_numGroups.
		Log::Write( LogLevel_Info, GetNodeId(), "Number of association groups reported for node %d is %d.", GetNodeId(), m_numGroups );
		QueryGroup( 1, _requestFlags );
	}
}

//-----------------------------------------------------------------------------
// <Association::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Association::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	bool handled = false;
	uint32 i;

	if( Node* node = GetNodeUnsafe() )
	{
		if( AssociationCmd_GroupingsReport == (AssociationCmd)_data[0] )
		{
			// Retrieve the number of groups this device supports.
			// The groups will be queried with the session data.
			m_numGroups = _data[1];
			Log::Write( LogLevel_Info, GetNodeId(), "Received Association Groupings report from node %d. Number of groups is %d", GetNodeId(), m_numGroups );
			ClearStaticRequest( StaticRequest_Values );
			handled = true;
		}
		else if( AssociationCmd_Report == (AssociationCmd)_data[0] )
		{
			// Get the group info
			uint8 groupIdx = _data[1];
			uint8 maxAssociations = _data[2];		// If the maxAssociations is zero, this is not a supported group.
			uint8 numReportsToFollow = _data[3];	// If a device supports a lot of associations, they may come in more than one message.

			if( maxAssociations )
			{
				if( _length >= 5 )
				{
					uint8 numAssociations = _length - 5;

					Log::Write( LogLevel_Info, GetNodeId(), "Received Association report from node %d, group %d, containing %d associations", GetNodeId(), groupIdx, numAssociations );
					if( numAssociations )
					{
						Log::Write( LogLevel_Info, GetNodeId(), "  The group contains:" );
						for( i=0; i<numAssociations; ++i )
						{
							Log::Write( LogLevel_Info, GetNodeId(), "    Node %d",  _data[i+4] );
							m_pendingMembers.push_back( _data[i+4] );
						}
					}
				}

				if( numReportsToFollow )
				{
					// We're expecting more reports for this group
					Log::Write( LogLevel_Info, GetNodeId(), "%d more association reports expected for node %d, group %d", numReportsToFollow, GetNodeId(), groupIdx );
					return true;
				}
				else
				{
					// No more reports to come for this group, so we can apply the pending list
					Group* group = node->GetGroup( groupIdx );
					if( NULL == group )
					{
						// Group has not been created yet
						group = new Group( GetHomeId(), GetNodeId(), groupIdx, maxAssociations );
						node->AddGroup( group );
					}

					// Update the group with its new contents
					group->OnGroupChanged( m_pendingMembers );
					m_pendingMembers.clear();
				}
			}
			else
			{
				// maxAssociations is zero, so we've reached the end of the query process
				Log::Write( LogLevel_Info, GetNodeId(), "Max associations for node %d, group %d is zero.  Querying associations for this node is complete.", GetNodeId(), groupIdx );
				node->AutoAssociate();
				m_queryAll = false;
			}

			if( m_queryAll )
			{
				// Work out which is the next group we will query.
				// If we are currently on group 255, the next group will be 1.
				uint8 nextGroup = groupIdx + 1;
				if( !nextGroup )
				{
					nextGroup = 1;
				}

				if( nextGroup <= m_numGroups )
				{
					// Query the next group
					QueryGroup( nextGroup, 0 );
				}
				else
				{
					// We're all done
					Log::Write( LogLevel_Info, GetNodeId(), "Querying associations for node %d is complete.", GetNodeId() );
					node->AutoAssociate();
					m_queryAll = false;
				}
			}

			handled = true;
		}
	}

	return handled;
}

//-----------------------------------------------------------------------------
// <Association::QueryGroup>
// Request details of an association group
//-----------------------------------------------------------------------------
void Association::QueryGroup
(
	uint8 _groupIdx,
	uint32 const _requestFlags
)
{
	if ( IsGetSupported() )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Get Associations for group %d of node %d", _groupIdx, GetNodeId() );
		Msg* msg = new Msg( "Get Associations", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( AssociationCmd_Get );
		msg->Append( _groupIdx );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "AssociationCmd_Get Not Supported on this node");
	}
	return;
}
//-----------------------------------------------------------------------------
// <Association::Set>
// Add an association between devices
//-----------------------------------------------------------------------------
void Association::Set
(
	uint8 _groupIdx,
	uint8 _targetNodeId
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "Association::Set - Adding node %d to group %d of node %d", _targetNodeId, _groupIdx, GetNodeId() );

	Msg* msg = new Msg( "Association Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_Set );
	msg->Append( _groupIdx );
	msg->Append( _targetNodeId );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
}

//-----------------------------------------------------------------------------
// <Association::Remove>
// Remove an association between devices
//-----------------------------------------------------------------------------
void Association::Remove
(
	uint8 _groupIdx,
	uint8 _targetNodeId
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "Association::Remove - Removing node %d from group %d of node %d", _targetNodeId, _groupIdx, GetNodeId() );

	Msg* msg = new Msg( "Association Remove", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 4 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCmd_Remove );
	msg->Append( _groupIdx );
	msg->Append( _targetNodeId );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
}

