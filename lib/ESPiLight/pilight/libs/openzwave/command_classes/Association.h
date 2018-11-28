//-----------------------------------------------------------------------------
//
//	Association.h
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

#ifndef _Association_H
#define _Association_H

#include <vector>
#include "CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_ASSOCIATION (0x85), a Z-Wave device command class.
	 */
	class Association: public CommandClass
	{
		friend class Group;

	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new Association( _homeId, _nodeId ); }
		virtual ~Association(){}

		static uint8 const StaticGetCommandClassId(){ return 0x85; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_ASSOCIATION"; }

		// From CommandClass
		virtual void ReadXML( TiXmlElement const* _ccElement );
		virtual void WriteXML( TiXmlElement* _ccElement );
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

		void RequestAllGroups( uint32 const _requestFlags );
		void Set( uint8 const _group, uint8 const _nodeId );
		void Remove( uint8 const _group, uint8 const _nodeId );

	private:
		Association( uint32 const _homeId, uint8 const _nodeId );
		void QueryGroup( uint8 _groupIdx, uint32 const _requestFlags );
		void AutoAssociate();

		bool			m_queryAll;			// When true, once a group has been queried, we request the next one.
		uint8			m_numGroups;		// Number of groups supported by the device.  255 is reported by certain manufacturers and requires special handling.
		vector<uint8>	m_pendingMembers;	// Used to build a list of group members from multiple reports
	};

} // namespace OpenZWave

#endif

