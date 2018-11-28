//-----------------------------------------------------------------------------
//
//	Wait.h
//
//	Cross-platform abstract base class for objects we
//	want to be able to wait for.
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
#ifndef _Wait_H
#define _Wait_H

#include <list>
#include "Ref.h"

namespace OpenZWave
{
	class WaitImpl;

	/** \brief Platform-independent definition of Wait objects.
	 */
	class Wait: public Ref
	{
		friend class WaitImpl;
		friend class ThreadImpl;

	public:
		enum
		{
			Timeout_Immediate = 0,
			Timeout_Infinite = -1
		};

		typedef void (*pfnWaitNotification_t)( void* _context );

		/**
		 * Add a watcher to our object.  The watcher will be triggered
		 * by the derived class, when it enters a certain state.
		 * \param _callback pointer to the function that will be called when the wait is over.
		 * \param _context pointer to custom data that will be sent with the callback.
		 */
		void AddWatcher( pfnWaitNotification_t _callback, void* _context );

		/**
		 * Remove a watcher from our object.  Both the _callback and _context pointers
		 * must match those used in a previous call to AddWatcher.
		 * \param _callback pointer to the function that will be called when the wait is over.
		 * \param _context pointer to custom data that will be sent with the callback.
		 */
		void RemoveWatcher( pfnWaitNotification_t _callback, void* _context );

		/**
		 * Wait for a single object to become signalled.
		 * \param _object pointer to the object to wait on.
		 * \param _timeout optional maximum time to wait.  Defaults to -1, which means wait forever.
		 * \return zero if the object was signalled, -1 if the wait timed out.
		 */
		static int32 Single( Wait* _object, int32 _timeout = -1 ){ return Multiple( &_object, 1, _timeout ); }

		/**
		 * Wait for one of multiple objects to become signalled.  If more than one object is in
		 * a signalled state, the lowest array index will be returned.
		 * \param _objects array of pointers to objects to wait on.
		 * \param _numObjects number of objects in the array.
		 * \param _timeout optional maximum time to wait.  Defaults to -1, which means wait forever.
		 * \return index into the array of the object that was signalled, -1 if the wait timed out.
		 */
		static int32 Multiple( Wait** _objects, uint32 _numObjects, int32 _timeout = -1 );

	protected:
		Wait();
		virtual ~Wait();

		/**
		 * Notify the watchers that the object is signalled.
		 */
		void Notify();

		/**
		 * Test whether an object is signalled.
		 */
		virtual bool IsSignalled() = 0;

	private:
		Wait( Wait const&	);					// prevent copy
		Wait& operator = ( Wait const& );		// prevent assignment

		WaitImpl*	m_pImpl;					// Pointer to an object that encapsulates the platform-specific implementation of a Wait object.
	};

} // namespace OpenZWave

#endif //_Wait_H

