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

#include "CommandClasses.h"
#include "MultiCmd.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <MultiCmd::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool MultiCmd::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( MultiCmdCmd_Encap == (MultiCmdCmd)_data[0] )
	{
		Log::Write( LogLevel_Info, GetNodeId(), "Received encapsulated multi-command from node %d", GetNodeId() );

		if( Node const* node = GetNodeUnsafe() )
		{
			// Iterate over commands
			uint8 base = 2;
			for( uint8 i=0; i<_data[1]; ++i )
			{
				uint8 length = _data[base];
				uint8 commandClassId = _data[base+1];

				if( CommandClass* pCommandClass = node->GetCommandClass( commandClassId ) )
				{
					pCommandClass->HandleMsg( &_data[base+2], length-1 );
				}

				base += (length + 1);
			}
		}

		Log::Write( LogLevel_Info, GetNodeId(), "End of encapsulated multi-command from node %d", GetNodeId() );
		return true;
	}
	return false;
}

