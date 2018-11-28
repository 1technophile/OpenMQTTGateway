//-----------------------------------------------------------------------------
//
//	Stream.h
//
//	Cross-platform circular buffer with signalling
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
#ifndef _Stream_H
#define _Stream_H

#include "../Defs.h"
#include "Wait.h"

#include <string>

namespace OpenZWave
{
	class Mutex;

	/** \brief Platform-independent definition of a circular buffer.
	 */
	class Stream: public Wait
	{
		friend class Wait;

	public:
		/**
		 * Constructor.
		 * Creates a cross-platform ring buffer object
		 */
		Stream( uint32 _bufferSize );

		/**
		 * Set the number of bytes the buffer must contain before it becomes signalled.
		 * Once the threshold is set, the application can use Wait::Single or Wait::Multiple
		 * to wait until the buffer has been filled with the desired amount of data.
		 * \param _size the amoutn of data in bytes that the buffer must contain for it to become signalled.
		 * \see Wait::Single, Wait::Multiple
		 */
		void SetSignalThreshold( uint32 _size );


		/**
		 * Copies the requested amount of data from the stream, removing it from the stream as it does so.
		 * If there is insufficient data available, the method returns false, and no data is transferred.
		 * \param _buffer pointer to a block of memory that will be filled with the stream data.
		 * \param _size the amount of data in bytes to copy from the stream.
		 * \return true if all the requested data has been copied.  False if there was not enough data in
		 * the stream.
		 * \see GetDataSize, Put
		 */
		bool Get( uint8* _buffer, uint32 _size );

		/**
		 * Copies the requested amount of data from the buffer into the stream.
		 * If there is insufficient room available in the stream's circular buffer, and no data is transferred.
		 * \param _buffer pointer to a block of memory that will be copied into the stream.
		 * \param _size the amount of data in bytes to copy to the stream.
		 * \return true if all the requested data has been copied.  False if there was not enough space in
		 * the stream's circular buffer.
		 * \see Get, GetDataSize
		 */
		bool Put( uint8* _buffer, uint32 _size );

 		/**
		 * Returns the amount of data in bytes that is stored in the stream.
		 * \return the number of bytes of data in the stream.
		 * \see Get, GetDataSize
		 */
		uint32 GetDataSize()const{ return m_dataSize; }

 		/**
		 * Empties the stream bytes held in the buffer.
		 * This is called when the library gets out of sync with the controller and sends a "NAK"
		 * to the controller.
		 */
		void Purge();

	protected:
		/**
		 * Formats stream buffer data for output to the log.
		 * \param _buffer pointer to the stream buffer "head" location
		 * \param _size number of valid bytes currently in the buffer
		 * \param _function string containing text to display before the data
		 */
		void LogData( uint8* _buffer, uint32 _size, const string &_function );

		/**
		 * Used by the Wait class to test whether the buffer contains sufficient data.
		 */
		virtual bool IsSignalled();

		/**
		 * Destructor.
		 * Destroys the ring buffer object.
		 */
		~Stream();

	private:
		Stream( Stream const&	);					// prevent copy
		Stream& operator = ( Stream const& );		// prevent assignment

		uint8*	m_buffer;
		uint32	m_bufferSize;
		uint32	m_signalSize;
		uint32	m_dataSize;
		uint32	m_head;
		uint32	m_tail;
 		Mutex*	m_mutex;
	};

} // namespace OpenZWave

#endif //_Event_H

