//-----------------------------------------------------------------------------
//
//	Thread.h
//
//	Cross-platform threads
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
#ifndef _Thread_H
#define _Thread_H

#include <string>
#include "../Defs.h"
#include "Wait.h"

namespace OpenZWave
{
	class ThreadImpl;
	class Event;

	/** \brief Implements a platform-independent thread management class.
	 */
	class Thread: public Wait
	{
	public:
		typedef void (*pfnThreadProc_t)( Event* _exitEvent, void* _context );

		/**
		 * Constructor.
		 * Creates a thread object that can be used to serialize access to a shared resource.
		 */
		Thread( string const& _name );

		/**
		 * Start running a function on this thread.
		 * Attempts to start a function running on this thread.  The call will fail if another
		 * function is already running.
		 * \param _pThreadProc pointer to the function to be run.  The function must take a
		 * single void pointer as its only argument, and return void.  On entry, the pointer
		 * will be set to the context provided to this Start method.
		 * \param _context pointer allowing any relevant data to be passed to the thread function.
		 * \return True if the function was successfully started.
		 * \see Stop, IsRunning
		 */
		bool Start( pfnThreadProc_t _pfnThreadProc, void* _context );

		/**
		 * Stop a function running on this thread.
		 * Attempts to stop a function running on this thread.  The call will fail if no
		 * function is running.
		 * \return True if the function was successfully stopped.
		 * \see Start, IsRunning
		 */
		bool Stop();

		/**
		 * Causes the thread to sleep for the specified number of milliseconds.
		 * \param _millisecs Number of milliseconds to sleep.
		 */
		void Sleep( uint32 _millisecs );

	protected:
		/**
		 * Used by the Wait class to test whether the thread has been completed.
		 */
		virtual bool IsSignalled();

		/**
		 * Destructor.
		 * Destroys the Thread object.
		 */
		virtual ~Thread();

	private:
		ThreadImpl*	m_pImpl;	// Pointer to an object that encapsulates the platform-specific implementation of a thread.
		Event*		m_exitEvent;
	};

} // namespace OpenZWave

#endif //_Thread_H

