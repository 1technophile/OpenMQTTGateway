//-----------------------------------------------------------------------------
//
//	ThreadImpl.cpp
//
//	Windows implementation of a cross-platform thread
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
#include "../../Defs.h"
#include "../Event.h"
#include "../Thread.h"
#include "ThreadImpl.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
//	<ThreadImpl::ThreadImpl>
//	Constructor
//-----------------------------------------------------------------------------
ThreadImpl::ThreadImpl
(
	Thread* _owner,
	string const& _name
):
	m_owner( _owner ),
	m_hThread( INVALID_HANDLE_VALUE ),
	m_bIsRunning( false ),
	m_name( _name )
{

}

//-----------------------------------------------------------------------------
//	<ThreadImpl::~ThreadImpl>
//	Destructor
//-----------------------------------------------------------------------------
ThreadImpl::~ThreadImpl
(
)
{
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Start>
//	Start a function running on this thread
//-----------------------------------------------------------------------------
bool ThreadImpl::Start
(
	Thread::pfnThreadProc_t _pfnThreadProc,
	Event* _exitEvent,
	void* _context 
)
{
	// Create a thread to run the specified function
	m_pfnThreadProc = _pfnThreadProc;
	m_context = _context;
	m_exitEvent = _exitEvent;
	m_exitEvent->Reset();

	HANDLE hThread = ::CreateThread( NULL, 0, ThreadImpl::ThreadProc, this, CREATE_SUSPENDED, NULL );
	m_hThread = hThread;

	::ResumeThread( hThread );
	return true;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Sleep>
//	Cause thread to sleep for the specified number of milliseconds
//-----------------------------------------------------------------------------
void ThreadImpl::Sleep
(
	uint32 _millisecs
)
{
	::Sleep(_millisecs);
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Terminate>
//	Force the thread to stop
//-----------------------------------------------------------------------------
bool ThreadImpl::Terminate
(
)
{
	if( !m_bIsRunning )
	{
		return false;
	}

	// This can cause all sorts of trouble if the thread is holding a lock.
	TerminateThread( m_hThread, 0 );
	m_hThread = INVALID_HANDLE_VALUE;
	return true;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::IsSignalled>
//	Test whether the thread has completed
//-----------------------------------------------------------------------------
bool ThreadImpl::IsSignalled
(
)
{
	return !m_bIsRunning;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::ThreadProc>
//	Entry point for running a function on this thread
//-----------------------------------------------------------------------------
DWORD WINAPI ThreadImpl::ThreadProc
( 
	void* _pArg 
)
{
	ThreadImpl* pImpl = (ThreadImpl*)_pArg;
	pImpl->Run();
	return 0;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Run>
//	Entry point for running a function on this thread
//-----------------------------------------------------------------------------
void ThreadImpl::Run
( 
)
{
	m_bIsRunning = true;
	m_pfnThreadProc( m_exitEvent, m_context );
	m_bIsRunning = false;
	
	// Let any watchers know that the thread has finished running.
	m_owner->Notify();
}

