//-----------------------------------------------------------------------------
//
//	Event.cpp
//
//	Cross-platform event
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

#ifdef _WIN32
#include "windows/EventImpl.h"	// Platform-specific implementation of an event
#else
#include "unix/EventImpl.h"	// Platform-specific implementation of an event
#endif

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<Event::Event>
//	Constructor
//-----------------------------------------------------------------------------
Event::Event
(
):
	m_pImpl( new EventImpl() )
{
}

//-----------------------------------------------------------------------------
//	<Event::~Event>
//	Destructor
//-----------------------------------------------------------------------------
Event::~Event
(
)
{
	delete m_pImpl;
}

//-----------------------------------------------------------------------------
//	<Event::Set>
//	Set the event to signalled
//-----------------------------------------------------------------------------
void Event::Set
(
)
{
	m_pImpl->Set();
	Notify();			// Notify any watchers that the event is now set
}

//-----------------------------------------------------------------------------
//	<Event::Reset>
//	Set the event to not signalled
//-----------------------------------------------------------------------------
void Event::Reset
(
)
{
	m_pImpl->Reset();
}

//-----------------------------------------------------------------------------
//	<Event::IsSignalled>
//	Test whether the event is set
//-----------------------------------------------------------------------------
bool Event::IsSignalled
(
)
{
	return m_pImpl->IsSignalled();
}

//-----------------------------------------------------------------------------
//	<Event::Wait>
//	Wait for the event to become signalled
//-----------------------------------------------------------------------------
bool Event::Wait
(
	int32 const _timeout
)
{
	return m_pImpl->Wait( _timeout );
}


