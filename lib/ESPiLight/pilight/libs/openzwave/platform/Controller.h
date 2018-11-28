//-----------------------------------------------------------------------------
//
//	Controller.h
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

#ifndef _Controller_H
#define _Controller_H

#include <string>
#include <list>
#include "../Defs.h"
#include "../Driver.h"
#include "Stream.h"

namespace OpenZWave
{
	class Driver;

	class Controller: public Stream
	{
		// Controller is derived from Stream rather than containing one, so that
		// we can use its Wait abilities without having to duplicate them here.
		// The stream is used for input.  Buffering of output is handled by the OS.

	public:
		/**
		 * Consructor.
		 * Creates the controller object.
		 */
		Controller():Stream( 2048 ){}

		/**
		 * Destructor.
		 * Destroys the controller object.
		 */
		virtual ~Controller(){}

		/**
		 * Queues a set of Z-Wave messages in the correct order needed to initialize the Controller implementation.
		 * @param Pointer to the driver object that will handle the messages.
		 * @see Driver::Init
		 */
		void PlayInitSequence( Driver* _driver );

		/**
		 * Open a controller.
		 * Attempts to open a controller and initialize it with the specified paramters.
		 * @param _controllerName The name of the port to open.  For example, ttyS1 on Linux, or \\.\COM2 in Windows.
		 * @see Close, Read, Write
		 */
		virtual bool Open( string const& _controllerName ) = 0;

		/**
		 * Close a controller.
		 * Closes the controller.
		 * @return True if the controller was closed successfully, or false if the controller was already closed, or an error occurred.
		 * @see Open
		 */
		virtual bool Close() = 0;

		/**
		 * Write to a controller.
		 * Attempts to write data to an open controller.
		 * @param _buffer Pointer to a block of memory containing the data to be written.
		 * @param _length Length in bytes of the data.
		 * @return The number of bytes written.
		 * @see Read, Open, Close
		 */
		virtual uint32 Write( uint8* _buffer, uint32 _length ) = 0;

		/**
		 * Read from a controller.
		 * Attempts to read data from an open controller.
		 * @param _buffer Pointer to a block of memory large enough to hold the requested data.
		 * @param _length Length in bytes of the data to be read.
		 * @return The number of bytes read.
		 * @see Write, Open, Close
		 */
		uint32 Read( uint8* _buffer, uint32 _length );
	};

} // namespace OpenZWave

#endif //_Controller_H

