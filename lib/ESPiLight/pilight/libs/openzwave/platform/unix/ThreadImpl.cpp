//----------------------------------------------------------------------------
//
//  ThreadImpl.h
//
//	POSIX implementation of a cross-platform thread
//
//	Copyright (c) 2010, Greg Satz <satz@iranger.com>
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
#include <unistd.h>
#include "../../Defs.h"
#include "../Event.h"
#include "../Thread.h"
#include "ThreadImpl.h"

#ifdef DARWIN
#define pthread_yield pthread_yield_np
#endif

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<ThreadImpl::ThreadImpl>
//	Constructor
//-----------------------------------------------------------------------------
ThreadImpl::ThreadImpl
(
	Thread* _owner,
	string const& _tname
):
	m_owner( _owner ),
//	m_hThread( NULL ),  /* p_thread_t isn't a pointer in Linux, so can't do this */
	m_bIsRunning( false ),
	m_name( _tname )
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
	void* _pContext 
)
{
	pthread_attr_t ta;

	pthread_attr_init( &ta );
	pthread_attr_setstacksize ( &ta, 0 );
	pthread_attr_setdetachstate ( &ta, PTHREAD_CREATE_JOINABLE );

	// Create a thread to run the specified function
	m_pfnThreadProc = _pfnThreadProc;
	m_pContext = _pContext;
	m_exitEvent = _exitEvent;
	m_exitEvent->Reset();

	pthread_create ( &m_hThread, &ta, ThreadImpl::ThreadProc, this );
	pthread_detach( m_hThread );
	//fprintf(stderr, "thread %s starting %08x\n", m_name.c_str(), m_hThread);
	//fflush(stderr);

	pthread_attr_destroy ( &ta );
	return true;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::Terminate>
//	End this thread
//-----------------------------------------------------------------------------
bool ThreadImpl::Terminate
(
)
{
	// void* data = NULL;

	//fprintf(stderr, "thread %s stopping %08x running %d\n", m_name.c_str(), m_hThread, m_bIsRunning );
	//fflush(stderr);
	if( !m_bIsRunning )
	{
		return false;
	}

	// This will kill an app that doesn't catch and ignore it.
	// We need to find another way to interrupt select.
	// thread_kill( m_hThread, SIGALRM );

	//m_hThread = NULL;
	m_bIsRunning = false;

	// pthread_cancel( m_hThread );
	// pthread_join( m_hThread, &data );

	return true;
}

//-----------------------------------------------------------------------------
//	<ThreadImpl::msSleep>
//	Cause thread to sleep for the specified number of milliseconds
//-----------------------------------------------------------------------------
void ThreadImpl::Sleep
(
	uint32 _millisecs
)
{
	usleep( _millisecs*1000 );
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
void *ThreadImpl::ThreadProc
( 
	void* _pArg 
)
{
	ThreadImpl* pImpl = (ThreadImpl*)_pArg;
	//fprintf(stderr, "thread %s run begin %08x running %d\n", pImpl->m_name.c_str(), pImpl->m_hThread, pImpl->m_bIsRunning );
	//fflush(stderr);
	pImpl->Run();
	//fprintf(stderr, "thread %s run end %08x running %d\n", pImpl->m_name.c_str(), pImpl->m_hThread, pImpl->m_bIsRunning );
	//fflush(stderr);
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
	m_pfnThreadProc( m_exitEvent, m_pContext );
	m_bIsRunning = false;
    
	// Let any watchers know that the thread has finished running
	m_owner->Notify();
}
