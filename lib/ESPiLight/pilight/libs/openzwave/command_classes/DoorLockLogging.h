//-----------------------------------------------------------------------------
//
//	DoorLockLogging.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_DOOR_LOCK_LOGGING
//
//	Copyright (c) 2014 Justin Hammond <justin@dynam.ac>
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

#ifndef _DoorLockLogging_H
#define _DoorLockLogging_H

#include "CommandClass.h"

namespace OpenZWave
{
	class ValueBool;

	/** \brief Implements COMMAND_CLASS_DOOR_LOCK_LOGGING (0x4C), a Z-Wave device command class.
	 */
	class DoorLockLogging: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new DoorLockLogging( _homeId, _nodeId ); }
		virtual ~DoorLockLogging(){}

		static uint8 const StaticGetCommandClassId(){ return 0x4c; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_DOOR_LOCK_LOGGING"; }

		// From CommandClass
		virtual void ReadXML( TiXmlElement const* _ccElement );
		virtual void WriteXML( TiXmlElement* _ccElement );
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		virtual bool SetValue( Value const& _value );
	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		DoorLockLogging( uint32 const _homeId, uint8 const _nodeId );
		uint8 m_MaxRecords;
		uint8 m_CurRecord;
	};

} // namespace OpenZWave

#endif

