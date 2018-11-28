//-----------------------------------------------------------------------------
//
//	Wait.cpp
//
//	Base class for objects we want to be able to wait for.
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
#include <stdio.h>
#include "../Defs.h"
#include "Wait.h"
#include "Event.h"
#include "Log.h"

#ifdef _WIN32
#include "windows/WaitImpl.h"	// Platform-specific implementation of a Wait object
#else
#include "unix/WaitImpl.h"	// Platform-specific implementation of a Wait object
#endif

using namespace OpenZWave;

void WaitMultipleCallback( void* _context );

//-----------------------------------------------------------------------------
//	<Wait::Wait>
//	Constructor
//-----------------------------------------------------------------------------
Wait::Wait
(
)
{
	m_pImpl = new WaitImpl( this );
}

//-----------------------------------------------------------------------------
//	<Wait::~Wait>
//	Destructor
//-----------------------------------------------------------------------------
Wait::~Wait
(
)
{
	delete m_pImpl;
}

//-----------------------------------------------------------------------------
//	<Wait::AddWatcher>
//	Add a watcher to our object.
//-----------------------------------------------------------------------------
void Wait::AddWatcher
( 
	pfnWaitNotification_t _callback, 
	void* _context
)
{
	if( !_callback )
	{
		assert(0);
		return;
	}

	// Add a ref so our object cannot disappear while being watched
	AddRef();

	// Add the watcher (platform specific code required here for thread safety)
	m_pImpl->AddWatcher( _callback, _context );
}

//-----------------------------------------------------------------------------
//	<Wait::RemoveWatcher>
//	Remove a watcher from our object.
//-----------------------------------------------------------------------------
void Wait::RemoveWatcher
( 
	pfnWaitNotification_t _callback, 
	void* _context
)
{
	if( m_pImpl->RemoveWatcher( _callback, _context ) )
	{
		Release();
	}
}

//-----------------------------------------------------------------------------
//	<Wait::Notify>
//	Notify all the watchers that the object has become signalled
//-----------------------------------------------------------------------------
void Wait::Notify
(
)
{
	m_pImpl->Notify();
}

//-----------------------------------------------------------------------------
//	<Wait::Multiple>
//	Wait for one of multiple objects to become signalled.
//-----------------------------------------------------------------------------
int32 Wait::Multiple
(
	Wait** _objects,
	uint32 _numObjects,
	int32 _timeout // = -1
)
{
	uint32 i;

	// Create an event that will be set when any of the objects in the list becomes signalled.
	Event* waitEvent = new Event();
 
	// Add a watcher to each object in the list, passing in the event as the context.
	for( i=0; i<_numObjects; ++i )
	{
		_objects[i]->AddWatcher( WaitMultipleCallback, waitEvent ); 
	}

	int32 res = -1;	// Default to timeout result
	string str = "";
	if( waitEvent->Wait( _timeout ) )
	{
		// An object was signalled.  Run through the list 
		// and see which one it was.
		for( i=0; i<_numObjects; ++i )
		{
			if( _objects[i]->IsSignalled() )
			{
				if( res == -1 )
					res = (int32)i;
				char buf[15];
				snprintf(buf, sizeof(buf), "%d, ", i);
				str += buf;
			}
		}
	}
	//Log::Write( LogLevel_Debug, "Wait::Multiple res=%d num=%d >%s", res, _numObjects, str.c_str() );

	// Remove the watchers
	for( i=0; i<_numObjects; ++i )
	{
		_objects[i]->RemoveWatcher( WaitMultipleCallback, waitEvent ); 
	}

	// We're done with the event now
	waitEvent->Release();
	return res;
}


//-----------------------------------------------------------------------------
//	<WaitMultipleCallback>
//	Callback handler for the watchers added during WaitImpl::Multiple
//-----------------------------------------------------------------------------
void WaitMultipleCallback
(
	void* _context
)
{
	Event* waitEvent = (Event*)_context;
	waitEvent->Set();
}

