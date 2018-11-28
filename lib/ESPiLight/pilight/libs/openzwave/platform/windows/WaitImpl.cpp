//-----------------------------------------------------------------------------
//
//	WaitImpl.cpp
//
//	Windows implementation of an abstract base class for objects we 
//  want to be able to wait for.
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
#include "../Log.h"

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
	InitializeCriticalSection( &m_criticalSection );
}

//-----------------------------------------------------------------------------
//	<WaitImpl::~WaitImpl>
//	Destructor
//-----------------------------------------------------------------------------
WaitImpl::~WaitImpl
(
)
{
	DeleteCriticalSection( &m_criticalSection );
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
	
	EnterCriticalSection( &m_criticalSection );

	m_watchers.push_back( watcher );

	LeaveCriticalSection( &m_criticalSection );

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
	EnterCriticalSection( &m_criticalSection );

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

	LeaveCriticalSection( &m_criticalSection );
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
	EnterCriticalSection( &m_criticalSection );

	for( list<Watcher>::iterator it=m_watchers.begin(); it!=m_watchers.end(); ++it )
	{
		Watcher const& watcher = *it;
		watcher.m_callback( watcher.m_context );
	}

	LeaveCriticalSection( &m_criticalSection );
}
