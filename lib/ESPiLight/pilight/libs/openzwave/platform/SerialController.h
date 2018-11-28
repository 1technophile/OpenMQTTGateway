//-----------------------------------------------------------------------------
//
//	SerialController.h
//
//	Cross-platform serial port handler
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

#ifndef _SerialController_H
#define _SerialController_H

#include <string>
#include "../Defs.h"
#include "Controller.h"

namespace OpenZWave
{
	class Driver;
	class Msg;
	class SerialControllerImpl;

	class SerialController: public Controller
	{
		friend class SerialControllerImpl;

	public:
		enum Parity
		{
			Parity_None = 0,
			Parity_Odd,
			Parity_Even,
			Parity_Mark,
			Parity_Space
		};

		enum StopBits
		{
			StopBits_One = 0,
			StopBits_OneAndAHalf = 1,
			StopBits_Two = 2
		};

		/**
		 * Constructor.
		 * Creates an object that represents a serial port.
		 */
		SerialController();

		/**
		 * Destructor.
		 * Destroys the serial port object.
		 */
		virtual ~SerialController();

		/**
		 * Set the serial port baud rate.  The serial port must be closed for the setting to be accepted.
		 * @param _baud Integer containing the expected baud-rate of the serial connection.  Most Z-Wave interfaces run at 115200 baud.
		 * @return True if the baud value was accepted.
		 * @see Open, Close
		 */
		bool SetBaud( uint32 const _baud );

		/**
		 * Set the serial port parity.  The serial port must be closed for the setting to be accepted.
		 * @param _parity Parity enum value indicating the serial data's expected type of parity bits, if any.
		 * @return True if the parity value was accepted.
		 * @see Open, Close
		 */
		bool SetParity( Parity const _parity );

		/**
		 * Set the serial port stop bits.  The serial port must be closed for the setting to be accepted.
 		 * @param _stopBits StopBits enum value indicating the serial data's expected number of stop-bits.
		 * @return True if the stop bits value was accepted.
		 * @see Open, Close
		 */
		bool SetStopBits( StopBits const _stopBits );

		/**
		 * Open a serial port.
		 * Attempts to open a serial port and initialize it with the specified paramters.
		 * @param _SerialControllerName The name of the port to open.  For example, ttyS1 on Linux, or \\.\COM2 in Windows.
		 * @return True if the port was opened and configured successfully.
		 * @see Close, Read, Write
		 */
		bool Open( string const& _SerialControllerName );

		/**
		 * Close a serial port.
		 * Closes the serial port.
		 * @return True if the port was closed successfully, or false if the port was already closed, or an error occurred.
		 * @see Open
		 */
		bool Close();

		/**
		 * Write to a serial port.
		 * Attempts to write data to an open serial port.
		 * @param _buffer Pointer to a block of memory containing the data to be written.
		 * @param _length Length in bytes of the data.
		 * @return The number of bytes written.
		 * @see Read, Open, Close
		 */
		uint32 Write( uint8* _buffer, uint32 _length );

   	private:
        uint32                      m_baud;
        SerialController::Parity    m_parity;
        SerialController::StopBits  m_stopBits;
		string						m_serialControllerName;

		SerialControllerImpl*	    m_pImpl;	// Pointer to an object that encapsulates the platform-specific implementation of the serial port.
		bool			            m_bOpen;
	};

} // namespace OpenZWave

#endif //_SerialController_H

