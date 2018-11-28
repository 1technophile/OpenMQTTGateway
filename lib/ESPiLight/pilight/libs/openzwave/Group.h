//-----------------------------------------------------------------------------
//
//	Group.h
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

#ifndef _Group_H
#define _Group_H

#include <string>
#include <vector>
#include <map>
#include "Defs.h"

class TiXmlElement;

namespace OpenZWave
{
	class Node;

	/** \brief Manages a group of devices (various nodes associated with each other).
	 */
	class Group
	{
		friend class Node;
		friend class Association;

	//-----------------------------------------------------------------------------
	// Construction
	//-----------------------------------------------------------------------------
	public:
		Group( uint32 const _homeId, uint8 const _nodeId, uint8 const _groupIdx, uint8 const _maxAssociations );
		Group( uint32 const _homeId, uint8 const _nodeId, TiXmlElement const* _valueElement );
		~Group(){}

		void WriteXML( TiXmlElement* _groupElement );

	//-----------------------------------------------------------------------------
	// Association methods	(COMMAND_CLASS_ASSOCIATION)
	//-----------------------------------------------------------------------------
	public:
		string const& GetLabel()const{ return m_label; }
		uint32 GetAssociations( uint8** o_associations );
		uint8 GetMaxAssociations()const{ return m_maxAssociations; }
		uint8 GetIdx()const{ return m_groupIdx; }
		bool Contains( uint8 const _nodeId );

	private:
		bool IsAuto()const{ return m_auto; }
		void SetAuto( bool const _state ){ m_auto = _state; }

		void AddAssociation( uint8 const _nodeId );
		void RemoveAssociation( uint8 const _nodeId );
		void OnGroupChanged( vector<uint8> const& _associations );

	//-----------------------------------------------------------------------------
	// Command methods (COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION)
	//-----------------------------------------------------------------------------
	public:
		bool ClearCommands( uint8 const _nodeId );
		bool AddCommand( uint8 const _nodeId, uint8 const _length, uint8 const* _data );

	private:
		class AssociationCommand
		{
		public:
			AssociationCommand( uint8 const _length, uint8 const* _data );
			~AssociationCommand();

		private:
			uint8	m_length;
			uint8*	m_data;
		};

		typedef vector<AssociationCommand>	AssociationCommandVec;

	//-----------------------------------------------------------------------------
	// Member variables
	//-----------------------------------------------------------------------------
	private:
		string								m_label;
		uint32								m_homeId;
		uint8								m_nodeId;
		uint8								m_groupIdx;
		uint8								m_maxAssociations;
		bool								m_auto;				// If true, the controller will automatically be associated with the group
		map<uint8,AssociationCommandVec>	m_associations;
	};

} //namespace OpenZWave

#endif //_Group_H

