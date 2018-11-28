//-----------------------------------------------------------------------------
//
//	MutexImpl.cpp
//
//	Windows Implementation of the cross-platform mutex
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
#include "MutexImpl.h"


using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<MutexImpl::MutexImpl>
//	Constructor
//-----------------------------------------------------------------------------
MutexImpl::MutexImpl
(
):
	m_lockCount( 0 )
{
	InitializeCriticalSection( &m_criticalSection );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::~MutexImpl>
//	Destructor
//-----------------------------------------------------------------------------
MutexImpl::~MutexImpl
(
)
{
	DeleteCriticalSection( &m_criticalSection );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::Lock>
//	Lock the mutex
//-----------------------------------------------------------------------------
bool MutexImpl::Lock
(
	bool const _bWait // = true;
)
{
	if( _bWait )
	{
		// We will wait for the lock
		EnterCriticalSection( &m_criticalSection );
		++m_lockCount;
		return true;
	}

	// Returns immediately, even if the lock was not available.
	if( TryEnterCriticalSection( &m_criticalSection ) )
	{
		++m_lockCount;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//	<MutexImpl::Unlock>
//	Release our lock on the mutex
//-----------------------------------------------------------------------------
void MutexImpl::Unlock
(
)
{
	if( !m_lockCount )
	{
		// No locks - we have a mismatched lock/release pair
		assert(0);
	}
	else
	{
		--m_lockCount;
		LeaveCriticalSection( &m_criticalSection );
	}
}

//-----------------------------------------------------------------------------
//	<MutexImpl::IsSignalled>
//	Test whether the mutex is free
//-----------------------------------------------------------------------------
bool MutexImpl::IsSignalled
(
)
{
	return( 0 == m_lockCount );
}

