//-----------------------------------------------------------------------------
//
// SerialControllerImpl.cpp
//
// POSIX implementation of a cross-platform serial port
//
// Copyright (c) 2010, Greg Satz <satz@iranger.com>
// All rights reserved.
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
#include <unistd.h>
#ifdef _WIN32
	#include "../../libs/pthreadw32/pthread.h"
#else
	#include <pthread.h>
#endif
#include "../../Defs.h"
#include "../Thread.h"
#include "../Event.h"
#include "SerialControllerImpl.h"
#include "../Log.h"

#ifdef __linux__
#include "../../libudev.h"
#endif

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// <SerialControllerImpl::SerialControllerImpl>
// Constructor
//-----------------------------------------------------------------------------
SerialControllerImpl::SerialControllerImpl
(
	SerialController* _owner
):
	m_owner( _owner ),
	m_hSerialController( -1 )
{
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::~SerialControllerImpl>
// Destructor
//-----------------------------------------------------------------------------
SerialControllerImpl::~SerialControllerImpl
(
)
{
	flock(m_hSerialController, LOCK_UN);
	if(m_hSerialController >= 0)
		close( m_hSerialController );
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Open>
// Open the serial port 
//-----------------------------------------------------------------------------
bool SerialControllerImpl::Open
( 
)
{
	// Try to init the serial port
	if( !Init( 1 ) )
	{
		// Failed.  We bail to allow the app a chance to take over, rather than retry
		// automatically.  Automatic retries only occur after a successful init.
		return false;
	}

	// Start the read thread
	m_pThread = new Thread( "SerialController" );
	m_pThread->Start( SerialReadThreadEntryPoint, this );

	return true;
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Close>
// Close the serial port 
//-----------------------------------------------------------------------------
void SerialControllerImpl::Close
( 
)
{
	if( m_pThread )
	{
		m_pThread->Stop();
		m_pThread->Release();
		m_pThread = NULL;
	}
	close( m_hSerialController );
	m_hSerialController = -1;
}

//-----------------------------------------------------------------------------
// <SerialReadThreadEntryPoint>
// Entry point of the thread for receiving data from the serial port
//-----------------------------------------------------------------------------
void SerialControllerImpl::SerialReadThreadEntryPoint
(
	Event* _exitEvent,
	void* _context
)
{
	SerialControllerImpl* impl = (SerialControllerImpl*)_context;
	if( impl )
	{
		impl->ReadThreadProc( _exitEvent );
	}
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::ReadThreadProc>
// Handle receiving data
//-----------------------------------------------------------------------------
void SerialControllerImpl::ReadThreadProc
(
	Event* _exitEvent
)
{  
	uint32 attempts = 0;
	while( true )
	{
		// Init must have been called successfully during Open, so we
		// don't do it again until the end of the loop
		if( -1 != m_hSerialController )
		{
			// Enter read loop.  Call will only return if
			// an exit is requested or an error occurs
			Read();

			// Reset the attempts, so we get a rapid retry for temporary errors
			attempts = 0;
		}

		if( attempts < 25 )		
		{
			// Retry every 5 seconds for the first two minutes...
			if( Wait::Single( _exitEvent, 5000 ) >= 0 )
			{
				// Exit signalled.
				break;
			}
		}
		else
		{
			// ...retry every 30 seconds after that
			if( Wait::Single( _exitEvent, 30000 ) >= 0 )
			{
				// Exit signalled.
				break;
			}
		}

		Init( ++attempts );
	}
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Init>
// Initialize the serial port
//-----------------------------------------------------------------------------
bool SerialControllerImpl::Init
(
	uint32 const _attempts
)
{  

	string device = m_owner->m_serialControllerName;
	
	Log::Write( LogLevel_Info, "Trying to open serial port %s (attempt %d)", device.c_str(), _attempts );
	
	m_hSerialController = open( device.c_str(), O_RDWR | O_NOCTTY, 0 );

	if( -1 == m_hSerialController )
	{
		//Error
		Log::Write( LogLevel_Error, "ERROR: Cannot open serial port %s. Error code %d", device.c_str(), errno );
		goto SerialOpenFailure;
	}

	if( flock( m_hSerialController, LOCK_EX | LOCK_NB) == -1 )
	{
		Log::Write( LogLevel_Error, "ERROR: Cannot get exclusive lock for serial port %s. Error code %d", device.c_str(), errno );
	}

	int bits;
	bits = 0;
	ioctl( m_hSerialController, TIOCMSET, &bits );

	// Configure the serial device parameters
	// Build on the current configuration
	struct termios tios;

	bzero( &tios, sizeof(tios) );
	tcgetattr( m_hSerialController, &tios );
	switch( m_owner->m_parity )
	{
		case SerialController::Parity_None:
			tios.c_iflag = IGNPAR;
			break;
		case SerialController::Parity_Odd:
			tios.c_iflag = INPCK;
			tios.c_cflag = PARENB | PARODD;
			break;
		default:
			Log::Write( LogLevel_Error, "ERROR: Parity not supported" );
			goto SerialOpenFailure;
	}
	switch( m_owner->m_stopBits )
	{
		case SerialController::StopBits_One:
			break;		// default
		case SerialController::StopBits_Two:
			tios.c_cflag |= CSTOPB;
			break;
		default:
			Log::Write( LogLevel_Error, "ERROR: Stopbits not supported" );
			goto SerialOpenFailure;
	}
	tios.c_iflag |= IGNBRK;
	tios.c_cflag |= CS8 | CREAD | CLOCAL;
	tios.c_oflag = 0;
	tios.c_lflag = 0;
	for( int i = 0; i < NCCS; i++ )
		tios.c_cc[i] = 0;
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 1;
	switch( m_owner->m_baud )
	{
		case 300:
			cfsetspeed( &tios, B300 );
			break;
		case 1200:
			cfsetspeed( &tios, B1200 );
			break;
		case 2400:
			cfsetspeed( &tios, B2400 );
			break;
		case 4800:
			cfsetspeed( &tios, B4800 );
			break;
		case 9600:
			cfsetspeed( &tios, B9600 );
			break;
		case 19200:
			cfsetspeed( &tios, B19200 );
			break;
		case 38400:
			cfsetspeed( &tios, B38400 );
			break;
		case 57600:
			cfsetspeed( &tios, B57600 );
			break;
#ifdef DARWIN
		case 76800:
			cfsetspeed( &tios, B76800 );
			break;
#endif
		case 115200:
			cfsetspeed( &tios, B115200 );
			break;
		case 230400:
			cfsetspeed( &tios, B230400 );
			break;
		default:
			Log::Write( LogLevel_Error, "Baud rate not supported" );
			goto SerialOpenFailure;
	}
	if ( tcsetattr( m_hSerialController, TCSANOW, &tios ) == -1 )
	{
		// Error.  Clean up and exit
		Log::Write( LogLevel_Error, "ERROR: Failed to set serial port parameters" );
		goto SerialOpenFailure;
	}

	tcflush( m_hSerialController, TCIOFLUSH );

	// Open successful
 	Log::Write( LogLevel_Info, "Serial port %s opened (attempt %d)", device.c_str(), _attempts );
	return true;

SerialOpenFailure:
 	Log::Write( LogLevel_Error, "ERROR: Failed to open serial port %s", device.c_str() );
	if(m_hSerialController >= 0)
	{
		close( m_hSerialController );
		m_hSerialController = -1;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Read>
// Read data from the serial port
//-----------------------------------------------------------------------------
void SerialControllerImpl::Read
(
)
{
	uint8 buffer[256];

	while( 1 )
        {
		int32 bytesRead;
		int err;

		do
		{
			bytesRead = read( m_hSerialController, buffer, sizeof(buffer) );
			if( bytesRead > 0 )
				m_owner->Put( buffer, bytesRead );
		} while( bytesRead > 0 );

		do
		{
			struct timeval *whenp;
			fd_set rds, eds;
			int oldstate;

			FD_ZERO( &rds );
			FD_SET( m_hSerialController, &rds );
			FD_ZERO( &eds );
			FD_SET( m_hSerialController, &eds );
			whenp = NULL;

			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
			err = select( m_hSerialController + 1, &rds, NULL, &eds, whenp );
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
		} while( err <= 0 );
	}
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::Write>
// Send data to the serial port
//-----------------------------------------------------------------------------
uint32 SerialControllerImpl::Write
(
	uint8* _buffer,
	uint32 _length
)
{
	if( -1 == m_hSerialController )
	{
		//Error
		Log::Write( LogLevel_Error, "ERROR: Serial port must be opened before writing" );
		return 0;
	}

	// Write the data
	uint32 bytesWritten;
	bytesWritten = write( m_hSerialController, _buffer, _length);
	return bytesWritten;
}
