//-----------------------------------------------------------------------------
//
//	EventImpl.cpp
//
//	Windows implementation of a cross-platform event
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
#include <windows.h>

#include "../../Defs.h"
#include "EventImpl.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
//	<EventImpl::EventImpl>
//	Constructor
//-----------------------------------------------------------------------------
EventImpl::EventImpl
(
)
{
	// Create a manual reset event
	m_hEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL );
}

//-----------------------------------------------------------------------------
//	<EventImpl::~EventImpl>
//	Destructor
//-----------------------------------------------------------------------------
EventImpl::~EventImpl
(
)
{
	::CloseHandle( m_hEvent );
}

//-----------------------------------------------------------------------------
//	<EventImpl::Set>
//	Set the event to signalled
//-----------------------------------------------------------------------------
void EventImpl::Set
(
)
{
	::SetEvent( m_hEvent );
}

//-----------------------------------------------------------------------------
//	<EventImpl::Reset>
//	Set the event to not signalled
//-----------------------------------------------------------------------------
void EventImpl::Reset
(
)
{
	::ResetEvent( m_hEvent );
}

//-----------------------------------------------------------------------------
//	<EventImpl::IsSignalled>
//	Test whether the event is set
//-----------------------------------------------------------------------------
bool EventImpl::IsSignalled
(
)
{
	return( WAIT_OBJECT_0 == WaitForSingleObject( m_hEvent, 0 ) );
}

//-----------------------------------------------------------------------------
//	<EventImpl::Wait>
//	Wait for the event to become signalled
//-----------------------------------------------------------------------------
bool EventImpl::Wait
(
	int32 const _timeout
)
{
	return( WAIT_TIMEOUT != ::WaitForSingleObject( m_hEvent, (DWORD)_timeout ) );
}
