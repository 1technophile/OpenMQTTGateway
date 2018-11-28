//-----------------------------------------------------------------------------
//
//	MultiInstanceAssociation.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_MULTI_INSTANCE_ASSOCIATION
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

#include "CommandClasses.h"
#include "MultiInstanceAssociation.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../platform/Log.h"

using namespace OpenZWave;

enum MultiInstanceAssociationCmd
{
	MultiInstanceAssociationCmd_Set				= 0x01,
	MultiInstanceAssociationCmd_Get				= 0x02,
	MultiInstanceAssociationCmd_Report			= 0x03,
	MultiInstanceAssociationCmd_Remove			= 0x04,
	MultiInstanceAssociationCmd_GroupingsGet	= 0x05,
	MultiInstanceAssociationCmd_GroupingsReport	= 0x06
};


//-----------------------------------------------------------------------------
// <MultiInstanceAssociation::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MultiInstanceAssociation::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	return false;
}

