//-----------------------------------------------------------------------------
//
//	SensorAlarm.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_ALARM
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

#ifndef _SensorAlarm_H
#define _SensorAlarm_H

#include <vector>
#include <string>
#include "CommandClass.h"

namespace OpenZWave
{
	class ValueByte;

	/** \brief Implements COMMAND_CLASS_SENSOR_ALARM (0x9c), a Z-Wave device command class.
	 */
	class SensorAlarm: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new SensorAlarm( _homeId, _nodeId ); }
		virtual ~SensorAlarm(){}

		static uint8 const StaticGetCommandClassId(){ return 0x9c; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_SENSOR_ALARM"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _alarmType, uint8 const _dummy, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

	private:
		SensorAlarm( uint32 const _homeId, uint8 const _nodeId );

		enum
		{
			SensorAlarm_General = 0,
			SensorAlarm_Smoke,
			SensorAlarm_CarbonMonoxide,
			SensorAlarm_CarbonDioxide,
			SensorAlarm_Heat,
			SensorAlarm_Flood,
			SensorAlarm_Count
		};
	};

} // namespace OpenZWave

#endif



