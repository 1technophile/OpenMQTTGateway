//-----------------------------------------------------------------------------
//
//	Controller.cpp
//
//	Cross-platform, hardware-abstracted controller data interface
//
//	Copyright (c) 2010 Jason Frazier <frazierjason@gmail.com>
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
#include "../Defs.h"
#include "../Driver.h"
#include "Controller.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// <Controller::PlayInitSequence>
//  Queues up the controller's initialization commands.
//-----------------------------------------------------------------------------
void Controller::PlayInitSequence
(
	Driver* _driver
)
{
	_driver->SendMsg( new Msg( "FUNC_ID_ZW_GET_VERSION", 0xff, REQUEST, FUNC_ID_ZW_GET_VERSION, false ), Driver::MsgQueue_Command );
	_driver->SendMsg( new Msg( "FUNC_ID_ZW_MEMORY_GET_ID", 0xff, REQUEST, FUNC_ID_ZW_MEMORY_GET_ID, false ), Driver::MsgQueue_Command );
	_driver->SendMsg( new Msg( "FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES", 0xff, REQUEST, FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES, false ), Driver::MsgQueue_Command );
	_driver->SendMsg( new Msg( "FUNC_ID_SERIAL_API_GET_CAPABILITIES", 0xff, REQUEST, FUNC_ID_SERIAL_API_GET_CAPABILITIES, false ), Driver::MsgQueue_Command );
	_driver->SendMsg( new Msg( "FUNC_ID_ZW_GET_SUC_NODE_ID", 0xff, REQUEST, FUNC_ID_ZW_GET_SUC_NODE_ID, false ), Driver::MsgQueue_Command );
	// FUNC_ID_ZW_GET_VIRTUAL_NODES & FUNC_ID_SERIAL_API_GET_INIT_DATA has moved into the handler for FUNC_ID_SERIAL_API_GET_CAPABILITIES
}

//-----------------------------------------------------------------------------
//	<Controller::Read>
//	Read from a controller
//-----------------------------------------------------------------------------
uint32 Controller::Read
(
	uint8* _buffer,
	uint32 _length
)
{
	// Fetch the data from the ring buffer (which is an all or nothing read)
	if( Get( _buffer, _length ) )
	{
		return _length;
	}

	return 0;
}

