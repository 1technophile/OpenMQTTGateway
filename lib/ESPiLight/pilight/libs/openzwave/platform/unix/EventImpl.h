//-----------------------------------------------------------------------------
//
//	EventImpl.h
//
//	POSIX implementation of a cross-platform event
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
#ifndef _EventImpl_H
#define _EventImpl_H

#ifdef _WIN32
	#include "../../libs/pthreadw32/pthread.h"
#else
	#include <pthread.h>
#endif
#include <errno.h>

namespace OpenZWave
{
	class EventImpl
	{
	private:
		friend class Event;
		friend class SocketImpl;
		friend class Wait;

		EventImpl();
		~EventImpl();

		void Set();
		void Reset();

		bool Wait( int32 _timeout );	// The wait method is to be used only by the Wait::Multiple method
		bool IsSignalled();

		pthread_mutex_t		m_lock;
		pthread_cond_t		m_condition;
		bool			m_manualReset;
		bool			m_isSignaled;
		unsigned int		m_waitingThreads;
	};

} // namespace OpenZWave

#endif //_EventImpl_H

