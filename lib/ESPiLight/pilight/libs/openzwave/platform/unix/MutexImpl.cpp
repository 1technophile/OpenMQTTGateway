//----------------------------------------------------------------------------
//
//  MutexImpl.cpp
//
//  POSIX implementation of the cross-platform mutex
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

#include "../../Defs.h"
#include "../Log.h"
#include "MutexImpl.h"

#include <stdio.h>
#include <errno.h>

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
	pthread_mutexattr_t ma;

	pthread_mutexattr_init ( &ma );
	pthread_mutexattr_settype( &ma, PTHREAD_MUTEX_RECURSIVE );
	int err = pthread_mutex_init( &m_criticalSection, &ma );
	if( err != 0 )
	{
		fprintf(stderr, "MutexImpl::MutexImpl error %d (%d)\n", errno, err );
	}
	pthread_mutexattr_destroy( &ma );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::~MutexImpl>
//	Destructor
//-----------------------------------------------------------------------------
MutexImpl::~MutexImpl
(
)
{
	pthread_mutex_destroy( &m_criticalSection );
}

//-----------------------------------------------------------------------------
//	<MutexImpl::Lock>
//	Lock the mutex
//-----------------------------------------------------------------------------
bool MutexImpl::Lock
(
	bool const _bWait
)
{
	if( _bWait )
	{
		// We will wait for the lock
		int err = pthread_mutex_lock( &m_criticalSection );
		if( err == 0 )
		{
			++m_lockCount;
			return true;
		}
		fprintf(stderr, "MutexImpl::Lock error %d (%d)\n", errno, err);
		return false;
	}

	// Returns immediately, even if the lock was not available.
	if( pthread_mutex_trylock( &m_criticalSection ) )
	{
		return false;
	}
	
	++m_lockCount;
	return true;
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
		int err = pthread_mutex_unlock( &m_criticalSection );
		if( err != 0 )
		{
			fprintf(stderr, "MutexImpl::Unlock error %d (%d)\n", errno, err);
		}
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

