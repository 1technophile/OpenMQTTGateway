//-----------------------------------------------------------------------------
//
//	ControllerReplication.h
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

#ifndef _ControllerReplication_H
#define _ControllerReplication_H

#include "CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_CONTROLLER_REPLICATION (0x21), a Z-Wave device command class.
	 */
	class ControllerReplication: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new ControllerReplication( _homeId, _nodeId ); }
		virtual ~ControllerReplication(){}

		static uint8 const StaticGetCommandClassId(){ return 0x21; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_CONTROLLER_REPLICATION"; }

		// From CommandClass
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		virtual bool SetValue( Value const& _value );

		void SendNextData();

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		ControllerReplication( uint32 const _homeId, uint8 const _nodeId );
		bool StartReplication( uint8 const _instance );


		bool	m_busy;
		uint8	m_targetNodeId;
		uint8	m_funcId;
		int	m_nodeId;
		int	m_groupCount;
		int	m_groupIdx;
		string	m_groupName;
	};

} // namespace OpenZWave

#endif

