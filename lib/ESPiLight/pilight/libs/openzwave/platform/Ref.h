//-----------------------------------------------------------------------------
//
//  Ref.h
//
//  Reference counting for objects.
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
#ifndef _Ref_H
#define _Ref_H

#pragma once

#include "../Defs.h"

namespace OpenZWave
{
	/**
	 * Provides reference counting for objects.
	 * Any class wishing to include reference counting should be derived from Ref.
	 * Derived classes must declare their destructor as protected virtual.
	 * On construction, the reference count is set to one.  Calls to AddRef increment
	 * the count.  Calls to Release decrement the count.  When the count reaches
	 * zero, the object is deleted.
	 */
	class Ref
	{
	public:
		/**
		 * Initializes the RefCount to one.  The object
		 * can only be deleted through a call to Release.
		 * \see AddRef, Release
		 */
		Ref(){ m_refs = 1; }

		/**
		 * Increases the reference count of the object.
		 * Every call to AddRef requires a matching call
		 * to Release before the object will be deleted.
		 * \see Release
		 */
		void AddRef(){ ++m_refs; }

		/**
		 * Removes a reference to an object.
		 * If this was the last reference to the message, the
		 * object is deleted.
		 * \see AddRef
		 */
		int32 Release()
		{
			if( 0 >= ( --m_refs ) )
			{
				delete this;
				return 0;
			}
			return m_refs;
		}

	protected:
		virtual ~Ref(){}

	private:
		// Reference counting
		int32	m_refs;

	}; // class Ref

} // namespace OpenZWave

#endif // _Ref_H

