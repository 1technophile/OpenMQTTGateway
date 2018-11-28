//-----------------------------------------------------------------------------
//
//	EventImpl.h
//
//	Windows implementation of a cross-platform event
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
#ifndef _EventImpl_H
#define _EventImpl_H

#include <windows.h>
#include "../../Defs.h"

namespace OpenZWave
{
	/** \brief Windows-specific implementation of the Event class.
	 */
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

		HANDLE	m_hEvent;
	};

} // namespace OpenZWave

#endif //_EventImpl_H

