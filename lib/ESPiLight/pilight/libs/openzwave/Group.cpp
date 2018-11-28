//-----------------------------------------------------------------------------
//
//	Group.cpp
//
//	A set of associations in a Z-Wave device.
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

#include <cstring>
#include "Group.h"
#include "Manager.h"
#include "Driver.h"
#include "Node.h"
#include "Notification.h"
#include "Options.h"
#include "command_classes/Association.h"
#include "command_classes/AssociationCommandConfiguration.h"

#include "tinyxml.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <Group::Group>
// Constructor
//-----------------------------------------------------------------------------
Group::Group
( 
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _groupIdx,
	uint8 const _maxAssociations
):
	m_homeId( _homeId ),
	m_nodeId( _nodeId ),
	m_groupIdx( _groupIdx ),
	m_maxAssociations( _maxAssociations ),
	m_auto( false )
{
	char str[16];
	snprintf( str, sizeof(str), "Group %d", m_groupIdx );
	m_label = str;

	// Auto-association by default is with group 1 or 255, with group 1 taking precedence.
	// Group 255 is always created first, so if this is group 1, we need to turn off the
	// auto flag in group 255.  All this messing about is to support the various behaviours
	// of certain Cooper devices.
	if( _groupIdx == 255 )
	{
		m_auto = true;
	}
	else if( _groupIdx == 1 )
	{
		m_auto = true;

		// Clear the flag from Group 255, if it exists.
		if( Driver* driver = Manager::Get()->GetDriver( m_homeId ) )
		{
			if( Node* node = driver->GetNodeUnsafe( m_nodeId ) )
			{
				if( Group* group = node->GetGroup( 255 ) )
				{
					group->SetAuto( false );
				}
			}
		}	
	}
}

//-----------------------------------------------------------------------------
// <Group::Group>
// Constructor (from XML)
//-----------------------------------------------------------------------------
Group::Group
(
	uint32 const _homeId,
	uint8 const _nodeId,
	TiXmlElement const* _groupElement
):
	m_homeId( _homeId ),
	m_nodeId( _nodeId ),
	m_groupIdx( 0 ),
	m_maxAssociations( 0 ),
	m_auto( false )
{
	int intVal;
	char const* str;
	vector<uint8> pending;

	if( TIXML_SUCCESS == _groupElement->QueryIntAttribute( "index", &intVal ) )
	{
		m_groupIdx = (uint8)intVal;
	}

	if( TIXML_SUCCESS == _groupElement->QueryIntAttribute( "max_associations", &intVal ) )
	{
		m_maxAssociations = (uint8)intVal;
	}

	str = _groupElement->Attribute( "auto" );
	if( str )
	{
		m_auto = !strcmp( str, "true" );
	}

	str = _groupElement->Attribute( "label" );
	if( str )
	{
		m_label = str;
	}

	// Read the associations for this group
	TiXmlElement const* associationElement = _groupElement->FirstChildElement();
	while( associationElement )
	{
		char const* elementName = associationElement->Value();
		if( elementName && !strcmp( elementName, "Node" ) )
		{
			if (associationElement->QueryIntAttribute( "id", &intVal ) == TIXML_SUCCESS) 
				pending.push_back( (uint8)intVal );
		}

		associationElement = associationElement->NextSiblingElement();
	}

	// Group must be added before OnGroupChanged is called so UpdateNodeRoutes can find it.
	// Since we do not want to update return routes UpdateNodeRoutes won't find the group
	// so nothing will go out from here. The not sending of return routes information
	// only works by a side effect of not finding the group.
	OnGroupChanged( pending );
}

//-----------------------------------------------------------------------------
// <Group::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void Group::WriteXML
(
	TiXmlElement* _groupElement
)
{
	char str[16];

	snprintf( str, 16, "%d", m_groupIdx );
	_groupElement->SetAttribute( "index", str );

	snprintf( str, 16, "%d", m_maxAssociations );
	_groupElement->SetAttribute( "max_associations", str );

	_groupElement->SetAttribute( "label", m_label.c_str() );
	_groupElement->SetAttribute( "auto", m_auto ? "true" : "false" );

	for( map<uint8,AssociationCommandVec>::iterator it = m_associations.begin(); it != m_associations.end(); ++it )
	{
		TiXmlElement* associationElement = new TiXmlElement( "Node" );
		
		snprintf( str, 16, "%d", it->first );
		associationElement->SetAttribute( "id", str );

		_groupElement->LinkEndChild( associationElement );
	}
}

//-----------------------------------------------------------------------------
// <Group::Contains>
// Whether a group contains a particular node
//-----------------------------------------------------------------------------
bool Group::Contains
(
	uint8 const _nodeId
)
{
	map<uint8,AssociationCommandVec>::iterator it = m_associations.find( _nodeId );
	return( it != m_associations.end() );
}

//-----------------------------------------------------------------------------
// <Group::AddAssociation>
// Associate a node with this group
//-----------------------------------------------------------------------------
void Group::AddAssociation
(
	uint8 const _nodeId
)
{
	if( Driver* driver = Manager::Get()->GetDriver( m_homeId ) )
	{
		if( Node* node = driver->GetNodeUnsafe( m_nodeId ) )
		{
			if( Association* cc = static_cast<Association*>( node->GetCommandClass( Association::StaticGetCommandClassId() ) ) )
			{
				cc->Set( m_groupIdx, _nodeId );
				cc->QueryGroup( m_groupIdx, 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Group:RemoveAssociation>
// Remove a node from this group
//-----------------------------------------------------------------------------
void Group::RemoveAssociation
(
	uint8 const _nodeId
)
{
	if( Driver* driver = Manager::Get()->GetDriver( m_homeId ) )
	{
		if( Node* node = driver->GetNodeUnsafe( m_nodeId ) )
		{
			if( Association* cc = static_cast<Association*>( node->GetCommandClass( Association::StaticGetCommandClassId() ) ) )
			{
				cc->Remove( m_groupIdx, _nodeId );
				cc->QueryGroup( m_groupIdx, 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Group::OnGroupChanged>
// Change the group contents and notify the watchers
//-----------------------------------------------------------------------------
void Group::OnGroupChanged
(
	vector<uint8> const& _associations
)
{
	bool notify = false;

	// If the number of associations is different, we'll save 
	// ourselves some work and clear the old set now.
	if( _associations.size() != m_associations.size() )
	{
		m_associations.clear();
		notify = true;
	}
	else
	{
		// Handle initial group creation case
		if ( _associations.size() == 0 && m_associations.size() == 0 )
		{
			notify = true;
		}
	}

	// Add the new associations. 
	uint8 oldSize = (uint8)m_associations.size();

	uint8 i;
	for( i=0; i<_associations.size(); ++i )
	{
		m_associations[_associations[i]] = AssociationCommandVec();
	}

	if( (!notify) && ( oldSize != m_associations.size() ) )
	{
		// The number of nodes in the original and new groups is the same, but
		// the number of associations has grown. There must be different nodes 
		// in the original and new sets of nodes in the group.  The easiest way
		// to sort this out is to clear the associations and add the new nodes again.
		m_associations.clear();
		for( i=0; i<_associations.size(); ++i )
		{
			m_associations[_associations[i]] = AssociationCommandVec();
		}
		notify = true;
	}

	if( notify )
	{
		// If the node supports COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION, we need to request the command data.
		if( Driver* driver = Manager::Get()->GetDriver( m_homeId ) )
		{
			if( Node* node = driver->GetNodeUnsafe( m_nodeId ) )
			{
				if( AssociationCommandConfiguration* cc = static_cast<AssociationCommandConfiguration*>( node->GetCommandClass( AssociationCommandConfiguration::StaticGetCommandClassId() ) ) )
				{
					for( map<uint8,AssociationCommandVec>::iterator it = m_associations.begin(); it != m_associations.end(); ++it )
					{
						cc->RequestCommands( m_groupIdx, it->first );
					}
				}
			}
		}

		// Send notification that the group contents have changed
		Notification* notification = new Notification( Notification::Type_Group );
		notification->SetHomeAndNodeIds( m_homeId, m_nodeId );
		notification->SetGroupIdx( m_groupIdx );
		Manager::Get()->GetDriver( m_homeId )->QueueNotification( notification ); 
		// Update routes on remote node if necessary
		bool update = false;
		Options::Get()->GetOptionAsBool( "PerformReturnRoutes", &update );
		if( update )
		{
			Driver *drv = Manager::Get()->GetDriver( m_homeId );
			if (drv)
				drv->UpdateNodeRoutes( m_nodeId );
		}
	}
}

//-----------------------------------------------------------------------------
// <Group::GetAssociations>
// Get a list of associations for this group
//-----------------------------------------------------------------------------
uint32 Group::GetAssociations
( 
	uint8** o_associations 
)
{
	size_t numNodes = m_associations.size();
	if( !numNodes )
	{
		*o_associations = NULL;
		return 0;
	}

	uint8* associations = new uint8[numNodes];
	uint32 i = 0;
	for( map<uint8,AssociationCommandVec>::iterator it = m_associations.begin(); it != m_associations.end(); ++it )
	{
		associations[i++] = it->first;
	}

	*o_associations = associations;
	return (uint32) numNodes;
}

//-----------------------------------------------------------------------------
// Command methods (COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// <Group::ClearCommands>
// Clear all the commands for the specified node
//-----------------------------------------------------------------------------
bool Group::ClearCommands
( 
	uint8 const _nodeId
)
{
	map<uint8,AssociationCommandVec>::iterator it = m_associations.find( _nodeId );
	if( it != m_associations.end() )
	{
		it->second.clear();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Group::AddCommand>
// Add a command to the list for the specified node
//-----------------------------------------------------------------------------
bool Group::AddCommand
(
	uint8 const _nodeId,
	uint8 const _length,
	uint8 const* _data
)
{
	map<uint8,AssociationCommandVec>::iterator it = m_associations.find( _nodeId );
	if( it != m_associations.end() )
	{
		it->second.push_back( AssociationCommand( _length, _data ) );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Group::AssociationCommand::AssociationCommand>
// Constructor
//-----------------------------------------------------------------------------
Group::AssociationCommand::AssociationCommand
(
	uint8 const _length,
	uint8 const* _data
):
	m_length( _length )
{
	m_data = new uint8[_length];
	memcpy( m_data, _data, _length );
}

//-----------------------------------------------------------------------------
// <Group::AssociationCommand::AssociationCommand>
// Destructor
//-----------------------------------------------------------------------------
Group::AssociationCommand::~AssociationCommand
(
)
{
	delete [] m_data;
}


