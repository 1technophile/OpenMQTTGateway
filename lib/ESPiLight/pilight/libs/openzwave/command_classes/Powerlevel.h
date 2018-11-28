//-----------------------------------------------------------------------------
//
//	Powerlevel.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_POWERLEVEL
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

#ifndef _Powerlevel_H
#define _Powerlevel_H

#include "CommandClass.h"

namespace OpenZWave
{
	class ValueList;

	/** \brief Implements COMMAND_CLASS_POWERLEVEL (0x73), a Z-Wave device command class.
	 */
	class Powerlevel: public CommandClass
	{
	public:
		enum PowerLevelEnum
		{
			PowerLevel_Normal = 0,
			PowerLevel_Minus1dB,
			PowerLevel_Minus2dB,
			PowerLevel_Minus3dB,
			PowerLevel_Minus4dB,
			PowerLevel_Minus5dB,
			PowerLevel_Minus6dB,
			PowerLevel_Minus7dB,
			PowerLevel_Minus8dB,
			PowerLevel_Minus9dB
		};

		enum PowerLevelStatusEnum
		{
			PowerLevelStatus_Failed = 0,
			PowerLevelStatus_Success,
			PowerLevelStatus_InProgress
		};

		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new Powerlevel( _homeId, _nodeId ); }
		virtual ~Powerlevel(){}

		static uint8 const StaticGetCommandClassId(){ return 0x73; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_POWERLEVEL"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		virtual bool SetValue( Value const& _value );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		Powerlevel( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ){}

		bool Set( uint8 const _instance );
		bool Test( uint8 const _instance );
		bool Report( uint8 const _instance );
	};

} // namespace OpenZWave

#endif

