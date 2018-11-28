//-----------------------------------------------------------------------------
//
//	ApplicationStatus.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_APPLICATION_STATUS
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
#include "ApplicationStatus.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../platform/Log.h"

using namespace OpenZWave;

enum ApplicationStatusCmd
{
	ApplicationStatusCmd_Busy				= 0x01,
	ApplicationStatusCmd_RejectedRequest	= 0x02
};


//-----------------------------------------------------------------------------
// <ApplicationStatus::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool ApplicationStatus::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( ApplicationStatusCmd_Busy == (ApplicationStatusCmd)_data[0] )
	{
		char msg[64];
		switch( _data[1] )
		{
			case 0:
			{
				snprintf( msg, sizeof(msg), "Try again later" );
				break;
			}
			case 1:
			{
				snprintf( msg, sizeof(msg), "Try again in %d seconds", _data[2] );
				break;
			}
			case 2:
			{
				snprintf( msg, sizeof(msg), "Request queued, will be executed later" );
				break;
			}
			default:
			{
				// Invalid status
				snprintf( msg, sizeof(msg), "Unknown status %d", _data[1] );
			}
		}
		Log::Write( LogLevel_Info, GetNodeId(), "Received Application Status Busy: %s", msg );
		return true;
	}

	if( ApplicationStatusCmd_RejectedRequest == (ApplicationStatusCmd)_data[0] )
	{
		Log::Write( LogLevel_Info, "Received Application Rejected Request: Status=%d", _data[1] );
		return true;
	}

	return false;
}

