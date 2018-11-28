//-----------------------------------------------------------------------------
//
//	WaitImpl.cpp
//
//	POSIX implementation of an abstract base class for objects we 
//	want to be able to wait for.
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
#include "../Wait.h"
#include "WaitImpl.h"

#include <stdio.h>
#include <errno.h>

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<WaitImpl::WaitImpl>
//	Constructor
//-----------------------------------------------------------------------------
WaitImpl::WaitImpl
(	
	Wait* _owner
):
	m_owner( _owner )
{
	pthread_mutexattr_t ma;
	pthread_mutexattr_init ( &ma );
	pthread_mutexattr_settype( &ma, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutex_init( &m_criticalSection, &ma );
	pthread_mutexattr_destroy( &ma );
}

//-----------------------------------------------------------------------------
//	<WaitImpl::~WaitImpl>
//	Destructor
//-----------------------------------------------------------------------------
WaitImpl::~WaitImpl
(
)
{
	pthread_mutex_destroy( &m_criticalSection );
}

//-----------------------------------------------------------------------------
//	<WaitImpl::AddWatcher>
//	Add a watcher to our object.
//-----------------------------------------------------------------------------
void WaitImpl::AddWatcher
( 
	Wait::pfnWaitNotification_t _callback, 
	void* _context
)
{
	// Add the watcher to our list
	Watcher watcher;
	watcher.m_callback = _callback;
	watcher.m_context = _context;
	
	if( pthread_mutex_lock( &m_criticalSection ) != 0 )
	{
		fprintf(stderr, "WaitImpl::AddWatcher lock error %d\n", errno );
		assert( 0 );
	}
	m_watchers.push_back( watcher );
	if( pthread_mutex_unlock( &m_criticalSection ) != 0 )
	{
		fprintf(stderr, "WaitImpl::AddWatcher unlock error %d\n", errno );
		assert( 0 );
	}

	// If the object is already in a signalled state, notify the watcher immediately
	if( m_owner->IsSignalled() )
	{
		_callback( _context );
	}
}

//-----------------------------------------------------------------------------
//	<WaitImpl::RemoveWatcher>
//	Remove a watcher from our object.
//-----------------------------------------------------------------------------
bool WaitImpl::RemoveWatcher
( 
	Wait::pfnWaitNotification_t _callback, 
	void* _context
)
{
	bool res = false;

	if( pthread_mutex_lock( &m_criticalSection ) != 0 )
	{
		fprintf(stderr, "WaitImpl::RemoveWatcher lock error %d\n", errno );
		assert( 0 );
	}

	for( list<Watcher>::iterator it=m_watchers.begin(); it!=m_watchers.end(); ++it )
	{
		Watcher const& watcher = *it;
		if( ( watcher.m_callback == _callback ) && ( watcher.m_context == _context ) )
		{
			m_watchers.erase( it );
			res = true;
			break;
		}
	}

	if( pthread_mutex_unlock( &m_criticalSection ) != 0 )
	{
		fprintf(stderr, "WaitImpl::RemoveWatcher unlock error %d\n", errno );
		assert( 0 );
	}
	return res;
}

//-----------------------------------------------------------------------------
//	<WaitImpl::Notify>
//	Notify all the watchers that the object has become signalled
//-----------------------------------------------------------------------------
void WaitImpl::Notify
(
)
{
	if( pthread_mutex_lock( &m_criticalSection ) != 0 )
	{
		fprintf(stderr, "WaitImpl::Notify lock error %d\n", errno );
		assert( 0 );
	}
	for( list<Watcher>::iterator it=m_watchers.begin(); it!=m_watchers.end(); ++it )
	{
		Watcher const& watcher = *it;
		watcher.m_callback( watcher.m_context );
	}
	if( pthread_mutex_unlock( &m_criticalSection ) != 0 )
	{
		fprintf(stderr, "WaitImpl::Notify unlock error %d\n", errno );
		assert( 0 );
	}
}
