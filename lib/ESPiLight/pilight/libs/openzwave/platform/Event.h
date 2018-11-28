//-----------------------------------------------------------------------------
//
//	Event.h
//
//	Cross-platform manual-reset event
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
#ifndef _Event_H
#define _Event_H

#include "Wait.h"

namespace OpenZWave
{
	class EventImpl;

	/** \brief Platform-independent definition of event objects.
	 */
	class Event: public Wait
	{
		friend class SerialControllerImpl;
		friend class Wait;

	public:
		/**
		 * Constructor.
		 * Creates a cross-platform event object equivalent to the Windows manual-reset event
		 */
		Event();

		/**
		 * Set the event to signalled.
		 * \see Reset, Wait
		 */
		void Set();

		/**
		 * Set the event to not signalled.
		 * \see Set, Wait
		 */
		void Reset();

	protected:
		/**
		 * Used by the Wait class to test whether the event is set.
		 */
		virtual bool IsSignalled();

		/**
		 * Used by the Wait::Multiple method.
		 * returns true if the event signalled, false if it timed out
		 */
		bool Wait( int32 _timeout );

		/**
		 * Destructor.
		 * Destroys the event object.
		 */
		~Event();

	private:
		Event( Event const&	);					// prevent copy
		Event& operator = ( Event const& );		// prevent assignment

		EventImpl*	m_pImpl;	// Pointer to an object that encapsulates the platform-specific implementation of a event.
	};

} // namespace OpenZWave

#endif //_Event_H

