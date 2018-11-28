//-----------------------------------------------------------------------------
//
//	TimeStamp.h
//
//	Cross-platform TimeStamp
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
#ifndef _TimeStamp_H
#define _TimeStamp_H

#include "../Defs.h"

namespace OpenZWave
{
	class TimeStampImpl;

	/** \brief Implements a platform-independent TimeStamp.
	 */
	class OPENZWAVE_EXPORT TimeStamp
	{
	public:
		/**
		 * Constructor.
		 * Creates a TimeStamp object.
		 */
		TimeStamp();

		/**
		 * Destructor.
		 * Destroys the TimeStamp object.
		 */
		~TimeStamp();

		/**
		 * SetTime.  Sets the timestamp to now, plus the offset in milliseconds.
		 * \param _milliseconds optional positive or negative offset from
		 * now in milliseconds.  Defaults to zero.
		 */
		void SetTime( int32 _milliseconds = 0 );

		/**
		 * TimeRemaining.  Gets the difference between now and the timestamp
		 * time in milliseconds.
		 * \return milliseconds remaining until we reach the timestamp.  The
		 * return value is negative if the timestamp is in the past.
		 */
		int32 TimeRemaining();

		/**
		 * Return as a string for output.
		 * \return string
		 */
		string GetAsString();

		/**
		 * Overload the subtract operator to get the difference between
		 * two timestamps in milliseconds.
		 */
		int32 operator- ( TimeStamp const& _other );

	private:
		TimeStamp( TimeStamp const& );					// prevent copy
		TimeStamp& operator = ( TimeStamp const& );			// prevent assignment

		TimeStampImpl*	m_pImpl;					// Pointer to an object that encapsulates the platform-specific implementation of the TimeStamp.
	};

} // namespace OpenZWave

#endif //_TimeStamp_H

