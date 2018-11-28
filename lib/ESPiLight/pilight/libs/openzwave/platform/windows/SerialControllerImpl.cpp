//-----------------------------------------------------------------------------
//
//	SerialControllerImpl.cpp
//
//	Windows Implementation of the cross-platform serial port
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

#include "../../Defs.h"
#include "SerialControllerImpl.h"

#include "../Log.h"

using namespace OpenZWave;

DWORD WINAPI SerialReadThreadEntryPoint( void* _context );

//-----------------------------------------------------------------------------
// <SerialControllerImpl::SerialControllerImpl>
// Constructor
//-----------------------------------------------------------------------------
SerialControllerImpl::SerialControllerImpl
(
	SerialController* _owner
):
	m_owner( _owner )
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
	CloseHandle( m_hSerialController );
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

	// Create an event to trigger exiting the read thread
	m_hExit = ::CreateEvent( NULL, TRUE, FALSE, NULL );

	// Start the read thread
	m_hThread = ::CreateThread( NULL, 0, SerialReadThreadEntryPoint, this, CREATE_SUSPENDED, NULL );
	::ResumeThread( m_hThread );

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
	::SetEvent( m_hExit );
	::WaitForSingleObject( m_hThread, INFINITE );

	CloseHandle( m_hThread );
	m_hThread = INVALID_HANDLE_VALUE;

	CloseHandle( m_hExit );
	m_hExit = INVALID_HANDLE_VALUE;

	CloseHandle( m_hSerialController );
	m_hSerialController = INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// <SerialReadThreadEntryPoint>
// Entry point of the thread for receiving data from the serial port
//-----------------------------------------------------------------------------
DWORD WINAPI SerialReadThreadEntryPoint
(
	void* _context
)
{
	SerialControllerImpl* impl = (SerialControllerImpl*)_context;
	if( impl )
	{
		impl->ReadThreadProc();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// <SerialControllerImpl::ReadThreadProc>
// Handle receiving data
//-----------------------------------------------------------------------------
void SerialControllerImpl::ReadThreadProc
(
)
{  
	uint32 attempts = 0;
	while( true )
	{
		// Init must have been called successfully during Open, so we
		// don't do it again until the end of the loop
		if( INVALID_HANDLE_VALUE != m_hSerialController )
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
			if( WAIT_OBJECT_0 == ::WaitForSingleObject( m_hExit, 5000 ) )
			{
				// Exit signalled.
				break;
			}
		}
		else
		{
			// ...retry every 30 seconds after that
			if( WAIT_OBJECT_0 == ::WaitForSingleObject( m_hExit, 30000 ) )
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
	Log::Write( LogLevel_Info, "    Trying to open serial port %s (Attempt %d)", m_owner->m_serialControllerName.c_str(), _attempts );

	m_hSerialController = CreateFileA( m_owner->m_serialControllerName.c_str(), 
							 GENERIC_READ | GENERIC_WRITE, 
							 0, 
							 NULL,
							 OPEN_EXISTING,
						 	 FILE_FLAG_OVERLAPPED, 
							 NULL );

	if( INVALID_HANDLE_VALUE == m_hSerialController )
	{
		//Error
		Log::Write( LogLevel_Error, "ERROR: Cannot open serial port %s. Error code %d\n", m_owner->m_serialControllerName.c_str(), GetLastError() );
		goto SerialOpenFailure;
	}

	// Configure the serial device parameters
	// Build on the current configuration
	DCB dcb;
	if( !GetCommState( m_hSerialController, &dcb ) )
	{
		//Error.  Clean up and exit
		Log::Write( LogLevel_Error, "ERROR: Failed to read serial port state" );
		goto SerialOpenFailure;
	}

	// Fill in the Device Control Block
	dcb.BaudRate = (DWORD)m_owner->m_baud;		
	dcb.ByteSize = 8;			
	dcb.Parity = (BYTE)m_owner->m_parity;		
	dcb.StopBits = (BYTE)m_owner->m_stopBits;	
	
	if( !SetCommState( m_hSerialController, &dcb) )
	{
		//Error. Clean up and exit
		Log::Write( LogLevel_Error, "ERROR: Failed to set serial port state" );
		goto SerialOpenFailure;
	}

	// Set the timeouts for the serial port
	COMMTIMEOUTS commTimeouts;
	commTimeouts.ReadIntervalTimeout = MAXDWORD;
	commTimeouts.ReadTotalTimeoutConstant = 0;
	commTimeouts.ReadTotalTimeoutMultiplier = 0;
	commTimeouts.WriteTotalTimeoutConstant = 0;
	commTimeouts.WriteTotalTimeoutMultiplier = 0;
	if( !SetCommTimeouts( m_hSerialController, &commTimeouts ) )
	{
		// Error.  Clean up and exit
		Log::Write( LogLevel_Error, "ERROR: Failed to set serial port timeouts" );
		goto SerialOpenFailure;
	}

	// Set the serial port to signal when data is received
	if( !SetCommMask( m_hSerialController, EV_RXCHAR ) )
	{
		//Error.  Clean up and exit
		Log::Write( LogLevel_Info, "ERROR: Failed to set serial port mask" );
		goto SerialOpenFailure;
	}

	// Clear any residual data from the serial port
	PurgeComm( m_hSerialController, PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR );

	// Open successful
 	Log::Write( LogLevel_Info, "    Serial port %s opened (attempt %d)", m_owner->m_serialControllerName.c_str(), _attempts );
	return true;

SerialOpenFailure:
 	Log::Write( LogLevel_Info, "ERROR: Failed to open serial port %s (attempt %d)", m_owner->m_serialControllerName.c_str(), _attempts );
	CloseHandle( m_hSerialController );
	m_hSerialController = INVALID_HANDLE_VALUE;
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

	OVERLAPPED overlapped;
	memset( &overlapped, 0, sizeof(overlapped) );
	overlapped.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
   
	while( true )
	{
		// Try to read all available data from the serial port
		DWORD bytesRead = 0;
		do
		{
			if( ::ReadFile( m_hSerialController, buffer, 256, NULL, &overlapped ) )
			{
				// Read completed
				GetOverlappedResult( m_hSerialController, &overlapped, &bytesRead, TRUE );

				// Copy to the stream buffer
				if( bytesRead > 0 )
					m_owner->Put( buffer, bytesRead );
			}
			else
			{
				//Wait for the read to complete
				if( ERROR_IO_PENDING == GetLastError() )
				{
					// Wait for the read to complete or the
					// signal that this thread should exit.
					HANDLE handles[2];
					handles[0] = overlapped.hEvent;
					handles[1] = m_hExit;
					DWORD res = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
			
					if( (WAIT_OBJECT_0+1) == res )
					{
						// Exit signalled.  Cancel the read.
						goto exitRead;
					}

					if( WAIT_TIMEOUT == res )
					{
						// Timed out - should never happen
						goto exitRead;
					}

					// Read completed
					GetOverlappedResult( m_hSerialController, &overlapped, &bytesRead, TRUE );

					// Copy to the stream buffer
					if( bytesRead > 0 )
						m_owner->Put( buffer, bytesRead );
				}
				else
				{
					// An error has occurred
					goto exitRead;
				}
			}
		}
		while( bytesRead > 0 );
		
		// Clear the event
		ResetEvent( overlapped.hEvent );

		// Nothing available to read, so wait for the next rx char event
		DWORD dwEvtMask;
		if( !WaitCommEvent( m_hSerialController, &dwEvtMask, &overlapped ) )
		{
			if( ERROR_IO_PENDING == GetLastError() )
			{
				// Wait for either some data to arrive or 
				// the signal that this thread should exit.
				HANDLE handles[2];
				handles[0] = overlapped.hEvent;
				handles[1] = m_hExit;
				
				DWORD res = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
			
				if( (WAIT_OBJECT_0+1) == res )
				{
					// Exit signalled.  Prevent WaitCommEvent from corrupting the 
					// stack by forcing it to exit.
					goto exitRead;
				}

				if( WAIT_TIMEOUT == res )
				{
					// Timed out - should never happen
					goto exitRead;
				}

				GetOverlappedResult( m_hSerialController, &overlapped, &bytesRead, TRUE );
			}
		}

		// Clear the event
		ResetEvent( overlapped.hEvent );

		// Loop back to the top to read the waiting data
	}

exitRead:
	// Exit event has been signalled, or an error has occurred
	SetCommMask( m_hSerialController, 0 );
	CancelIo( m_hSerialController );
	CloseHandle( overlapped.hEvent );
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
	if( INVALID_HANDLE_VALUE == m_hSerialController )
	{
		//Error
		Log::Write( LogLevel_Error, "ERROR: Serial port must be opened before writing\n" );
		return 0;
	}

	// Write the data
	OVERLAPPED overlapped;
	memset( &overlapped, 0, sizeof(overlapped) );
	overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	DWORD bytesWritten;
	if( !::WriteFile( m_hSerialController, _buffer, _length, &bytesWritten, &overlapped ) )
	{
		//Wait for the write to complete
		if( ERROR_IO_PENDING == GetLastError() )
		{
			GetOverlappedResult( m_hSerialController, &overlapped, &bytesWritten, TRUE );
		}
		else
		{
			Log::Write( LogLevel_Error, "ERROR: Serial port write (0x%.8x)", GetLastError() );
		}
	}

	CloseHandle( overlapped.hEvent );
	return (uint32)bytesWritten;
}
