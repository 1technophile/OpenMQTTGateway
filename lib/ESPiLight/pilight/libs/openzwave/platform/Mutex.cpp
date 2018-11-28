//-----------------------------------------------------------------------------
//
//	Mutex.cpp
//
//	Cross-platform mutex
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
#include "Mutex.h"

#ifdef _WIN32
#include "windows/MutexImpl.h"	// Platform-specific implementation of a mutex
#else
#include "unix/MutexImpl.h"	// Platform-specific implementation of a mutex
#endif


using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<Mutex::Mutex>
//	Constructor
//-----------------------------------------------------------------------------
Mutex::Mutex
(
):
	m_pImpl( new MutexImpl() )
{
}

//-----------------------------------------------------------------------------
//	<Mutex::~Mutex>
//	Destructor
//-----------------------------------------------------------------------------
Mutex::~Mutex
(
)
{
	delete m_pImpl;
}

//-----------------------------------------------------------------------------
//	<Mutex::Lock>
//	Lock the mutex
//-----------------------------------------------------------------------------
bool Mutex::Lock
(
	bool const _bWait // = true;
)
{
	return m_pImpl->Lock( _bWait );
}

//-----------------------------------------------------------------------------
//	<Mutex::Unlock>
//	Release our lock on the mutex
//-----------------------------------------------------------------------------
void Mutex::Unlock
(
)
{
	m_pImpl->Unlock();

	if( IsSignalled() )
	{
		// The mutex has no owners, so notify the watchers
		Notify();
	}
}

//-----------------------------------------------------------------------------
//	<Mutex::IsSignalled>
//	Test whether the event is set
//-----------------------------------------------------------------------------
bool Mutex::IsSignalled
(
)
{
	return m_pImpl->IsSignalled();
}

