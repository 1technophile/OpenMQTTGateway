//-----------------------------------------------------------------------------
//
//	WaitImpl.h
//
//	Windows implementation of a base class for objects we
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
#ifndef _WaitImpl_H
#define _WaitImpl_H

#include <windows.h>
#include <list>
#include "../../Defs.h"
#include "../Ref.h"
#include "../Wait.h"

namespace OpenZWave
{
	/** \brief Windows specific implementation of Wait objects.
	 */
	class WaitImpl
	{
	private:
		friend class Wait;

		WaitImpl( Wait* _owner );
		virtual ~WaitImpl();

		void AddWatcher( Wait::pfnWaitNotification_t _callback, void* _context );
		bool RemoveWatcher( Wait::pfnWaitNotification_t _callback, void* _context );
		void Notify();

		static int32 Multiple( Wait** _objects, uint32 _numObjects, int32 _timeout = -1 );

		WaitImpl( Wait const&	);					// prevent copy
		WaitImpl& operator = ( WaitImpl const& );	// prevent assignment

		struct Watcher
		{
			Wait::pfnWaitNotification_t		m_callback;
			void*							m_context;
		};

		list<Watcher>		m_watchers;
		Wait*				m_owner;
		CRITICAL_SECTION	m_criticalSection;
	};

} // namespace OpenZWave

#endif //_WaitImpl_H

