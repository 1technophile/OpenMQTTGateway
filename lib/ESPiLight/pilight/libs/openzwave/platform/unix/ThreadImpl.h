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
#ifndef _ThreadImpl_H
#define _ThreadImpl_H

#include <stdio.h>
#include <signal.h>
#ifdef _WIN32
	#include "../../libs/pthreadw32/pthread.h"
#else
	#include <pthread.h>
#endif
#include <string>

namespace OpenZWave
{
    class Thread;
    class Event;

    class ThreadImpl
    {
    private:
        friend class Thread;

        ThreadImpl( Thread* _owner, string const& _tname );
        ~ThreadImpl();

        bool Start( Thread::pfnThreadProc_t _pfnThreadProc, Event* _exitEvent, void* _context );
        void Sleep( uint32 _millisecs );
        bool IsSignalled();
        bool Terminate();

        void Run();
        static void* ThreadProc( void *parg);

        Thread*                 m_owner;
        Event*                  m_exitEvent;
        pthread_t               m_hThread;
        Thread::pfnThreadProc_t	m_pfnThreadProc;
        void*                   m_pContext;
        bool                    m_bIsRunning;
        string                  m_name;
    };
} // namespace OpenZWave

#endif //_ThreadImpl_H
