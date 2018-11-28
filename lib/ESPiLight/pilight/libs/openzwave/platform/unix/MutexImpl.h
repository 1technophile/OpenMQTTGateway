//----------------------------------------------------------------------------
//
//  MutexImpl.h
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
#ifndef _MutexImpl_H
#define _MutexImpl_H

#include <stdio.h>
#ifdef _WIN32
	#include "../../libs/pthreadw32/pthread.h"
#else
	#include <pthread.h>
#endif

namespace OpenZWave
{
	class MutexImpl
	{
	private:
		friend class Mutex;

		MutexImpl();
		~MutexImpl();

		bool Lock( bool const _bWait = true );
		void Unlock();

		bool IsSignalled();

		uint32				m_lockCount;				// Keep track of the locks (there can be more than one if they occur on the same thread.
		pthread_mutex_t		m_criticalSection;
	};

} // namespace OpenZWave

#endif //_MutexIF_H

