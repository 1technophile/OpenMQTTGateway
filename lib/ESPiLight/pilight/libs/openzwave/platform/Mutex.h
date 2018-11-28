//-----------------------------------------------------------------------------
//
//	Mutex.h
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
#ifndef _Mutex_H
#define _Mutex_H

#include "Wait.h"

namespace OpenZWave
{
	class MutexImpl;

	/** \brief Implements a platform-independent mutex--for serializing access to a shared resource.
	 */
	class Mutex: public Wait
	{
	public:
		/**
		 * Constructor.
		 * Creates a mutex object that can be used to serialize access to a shared resource.
		 */
		Mutex();

		/**
		 * Lock the mutex.
		 * Attempts to lock the mutex.
		 * There must be a matching call to Release for every call to Lock.
		 * \param _bWait Defaults to true.  Set this argument to false if the method should return
		 * immediately, even if the lock is not available.
		 * \return True if the lock was obtained.
		 * \see Release
		 */
		bool Lock( bool const _bWait = true );

		/**
		 * Releases the lock on the mutex.
		 * There must be a matching call to Release for every call to Lock.
		 * \see Lock
		 */
		void Unlock();

	protected:
		/**
		 * Used by the Wait class to test whether the mutex is free.
		 */
		virtual bool IsSignalled();

		/**
		 * Destructor.
		 * Destroys the mutex object.
		 */
		~Mutex();

	private:
		Mutex( Mutex const&	);					// prevent copy
		Mutex& operator = ( Mutex const& );		// prevent assignment

		MutexImpl*	m_pImpl;					// Pointer to an object that encapsulates the platform-specific implementation of a mutex.
	};

} // namespace OpenZWave

#endif //_Mutex_H

