//-----------------------------------------------------------------------------
//
//	NoOperation.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_NO_OPERATION
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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

#ifndef _NoOperation_H
#define _NoOperation_H

#include "CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_NO_OPERATION (0x00), a Z-Wave device command class.
	 */
	class NoOperation: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new NoOperation( _homeId, _nodeId ); }
		virtual ~NoOperation(){}

		static uint8 const StaticGetCommandClassId(){ return 0x00; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_NO_OPERATION"; }

		void Set( bool const _route, Driver::MsgQueue const _queue = Driver::MsgQueue_NoOp );

		// From CommandClass
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

	private:
		NoOperation( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ){}
	};

} // namespace OpenZWave

#endif

