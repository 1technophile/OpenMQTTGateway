//-----------------------------------------------------------------------------
//
//	Thread.cpp
//
//	Cross-platform threads
//
//	Copyright (c) 2010 Mal Lansell <mal@lansell.org>
//	All rights reserved.
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
#include "Event.h"
#include "Thread.h"

#ifdef _WIN32
#include "windows/ThreadImpl.h"	// Platform-specific implementation of a thread
#else
#include "unix/ThreadImpl.h"	// Platform-specific implementation of a thread
#endif

using namespace OpenZWave;


//-----------------------------------------------------------------------------
//	<Thread::Thread>
//	Constructor
//-----------------------------------------------------------------------------
Thread::Thread
(
	string const& _name
)
{
	m_exitEvent = new Event();
	m_pImpl = new ThreadImpl( this, _name );
}

//-----------------------------------------------------------------------------
//	<Thread::~Thread>
//	Destructor
//-----------------------------------------------------------------------------
Thread::~Thread
(
)
{
	delete m_pImpl;
	m_exitEvent->Release();
}

//-----------------------------------------------------------------------------
//	<Thread::Start>
//	Start a function running on this thread
//-----------------------------------------------------------------------------
bool Thread::Start
(
	pfnThreadProc_t _pfnThreadProc, 
	void* _context 
)
{
	return( m_pImpl->Start( _pfnThreadProc, m_exitEvent, _context ) );
}

//-----------------------------------------------------------------------------
//	<Thread::Stop>
//	Stop a function running on this thread
//-----------------------------------------------------------------------------
bool Thread::Stop
(
)
{
	int32 timeout = 2000;	// Give the thread 2 seconds to exit
	m_exitEvent->Set();
	
	if( Wait::Single( this, timeout ) < 0 )
	{
		// Timed out
	        m_pImpl->Terminate();
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//	<Thread::Sleep>
//	Causes the thread to sleep for the specified number of milliseconds.
//-----------------------------------------------------------------------------
void Thread::Sleep
(
	uint32 _milliseconds
) 
{
	return( m_pImpl->Sleep( _milliseconds ) );
}

//-----------------------------------------------------------------------------
//	<Thread::IsSignalled>
//	Test whether the event is set
//-----------------------------------------------------------------------------
bool Thread::IsSignalled
(
)
{
	return m_pImpl->IsSignalled();
}


