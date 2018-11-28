//-----------------------------------------------------------------------------
//
//	AssociationCommandConfiguration.h
//
//	Implementation of the COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION
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

#ifndef _AssociationCommandConfiguration_H
#define _AssociationCommandConfiguration_H

#include "CommandClass.h"
#include "../value_classes/ValueBool.h"
#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueShort.h"


namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION (0x9b), a Z-Wave device command class.
	 */

	class AssociationCommandConfiguration: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new AssociationCommandConfiguration( _homeId, _nodeId ); }
		virtual ~AssociationCommandConfiguration(){}

		static uint8 const StaticGetCommandClassId(){ return 0x9b; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION"; }

		void RequestCommands( uint8 const _groupIdx, uint8 const _nodeId );
		void SetCommand( uint8 const _groupIdx, uint8 const _nodeId, uint8 const _length, uint8 const* _data );

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		AssociationCommandConfiguration( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ){}
	};

} // namespace OpenZWave

#endif
