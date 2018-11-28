//-----------------------------------------------------------------------------
//
//	Alarm.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_ALARM
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

#ifndef _Alarm_H
#define _Alarm_H

#include "CommandClass.h"

namespace OpenZWave
{
	class ValueByte;

	/** \brief Implements COMMAND_CLASS_ALARM (0x71), a Z-Wave device command class.
	 */
	class Alarm: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new Alarm( _homeId, _nodeId ); }
		virtual ~Alarm(){}

		/** \brief Get command class ID (1 byte) identifying this command class. */
		static uint8 const StaticGetCommandClassId(){ return 0x71; }
		/** \brief Get a string containing the name of this command class. */
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_ALARM"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		/** \brief Get command class ID (1 byte) identifying this command class. (Inherited from CommandClass) */
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		/** \brief Get a string containing the name of this command class. (Inherited from CommandClass) */
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		/** \brief Handle a response to a message associated with this command class. (Inherited from CommandClass) */
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

		virtual uint8 GetMaxVersion(){ return 2; }

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		Alarm( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ){}
	};

} // namespace OpenZWave

#endif
