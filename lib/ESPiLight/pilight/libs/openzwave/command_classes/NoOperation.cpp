//-----------------------------------------------------------------------------
//
//	NoOperation.cpp
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

#include "CommandClasses.h"
#include "NoOperation.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// <NoOperation::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool NoOperation::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	// We have received a no operation from the Z-Wave device.
	Log::Write( LogLevel_Info, GetNodeId(), "Received NoOperation command from node %d", GetNodeId() );
	return true;
}

//-----------------------------------------------------------------------------
// <NoOperation::Set>
// Send a No Operation message class.
//-----------------------------------------------------------------------------
void NoOperation::Set
(
	bool const _route,
	Driver::MsgQueue _queue		// = Driver::MsgQueue_NoOp
)
{
	Log::Write( LogLevel_Info, GetNodeId(), "NoOperation::Set - Routing=%s", _route ? "true" : "false" );

	Msg* msg = new Msg( "NoOperation_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( 0 );
	if( _route )
		msg->Append( GetDriver()->GetTransmitOptions() );
	else
		msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE );
	GetDriver()->SendMsg( msg, _queue );
}
