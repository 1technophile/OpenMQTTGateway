//-----------------------------------------------------------------------------
//
//	MultiCmd.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_MULTI_CMD
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

#ifndef _MultiCmd_H
#define _MultiCmd_H

#include "CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_MULTI_CMD (0x8f), a Z-Wave device command class.
	 */
	class MultiCmd: public CommandClass
	{
	public:
		enum MultiCmdCmd
		{
			MultiCmdCmd_Encap = 0x01
		};

		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new MultiCmd( _homeId, _nodeId ); }
		virtual ~MultiCmd(){}

		static uint8 const StaticGetCommandClassId(){ return 0x8f; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_MULTI_CMD"; }

		// From CommandClass
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

	private:
		MultiCmd( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ){}
	};

} // namespace OpenZWave

#endif

