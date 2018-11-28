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
#include "Stream.h"
#include "Mutex.h"
#include "Log.h"

#include <string.h>

#include <cstdio>

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<Stream::Stream>
//	Constructor
//-----------------------------------------------------------------------------
Stream::Stream
(
	uint32 _bufferSize
):
	m_bufferSize( _bufferSize ),
	m_signalSize(1),
	m_dataSize(0),
	m_head(0),
	m_tail(0),
	m_mutex( new Mutex() )
{
	m_buffer = new uint8[m_bufferSize];
}

//-----------------------------------------------------------------------------
//	<Stream::~Stream>
//	Destructor
//-----------------------------------------------------------------------------
Stream::~Stream
(
)
{
	m_mutex->Release();
	delete [] m_buffer;
}

//-----------------------------------------------------------------------------
//	<Stream::SetSignalThreshold>
//	Set the amount of data that must be in the buffer for to to be signalled
//-----------------------------------------------------------------------------
void Stream::SetSignalThreshold
(
	uint32 _size 
)
{
	m_signalSize = _size;
	if( IsSignalled() )
	{
		// We have more data than we are waiting for, so notify the watchers
		Notify();
	}
}

//-----------------------------------------------------------------------------
//	<Stream::Get>
//	Remove data from the buffer
//-----------------------------------------------------------------------------
bool Stream::Get
(
	uint8* _buffer,
	uint32 _size
)
{
	if( m_dataSize < _size )
	{
		// There is not enough data in the buffer to fulfill the request
		Log::Write( LogLevel_Error, "ERROR: Not enough data in stream buffer");
		return false;
	}

	m_mutex->Lock();
	if( (m_tail + _size) > m_bufferSize )
	{
		// We will have to wrap around
		uint32 block1 = m_bufferSize - m_tail;
		uint32 block2 = _size - block1;

		memcpy( _buffer, &m_buffer[m_tail], block1 );
		memcpy( &_buffer[block1], m_buffer, block2 );
		m_tail = block2;
	}
	else
	{
		// Requested data is in a contiguous block
		memcpy( _buffer, &m_buffer[m_tail], _size );
		m_tail += _size;
	}

	LogData( _buffer, _size, "      Read (buffer->application): ");

	m_dataSize -= _size;
	m_mutex->Unlock();
	return true;
}

//-----------------------------------------------------------------------------
//	<Stream::Put>
//	Add data to the buffer
//-----------------------------------------------------------------------------
bool Stream::Put
(
	uint8* _buffer,
	uint32 _size
)
{
	if( (m_bufferSize-m_dataSize) < _size )
	{
		// There is not enough space left in the buffer for the data
		Log::Write( LogLevel_Error, "ERROR: Not enough space in stream buffer");
		return false;
	}

	m_mutex->Lock();
	if( (m_head + _size) > m_bufferSize )
	{
		// We will have to wrap around
		uint32 block1 = m_bufferSize - m_head;
		uint32 block2 = _size - block1;

		memcpy( &m_buffer[m_head], _buffer, block1 );
		memcpy( m_buffer, &_buffer[block1], block2 );
		m_head = block2;
		LogData( m_buffer + m_head - block1, block1, "      Read (controller->buffer):  ");
		LogData( m_buffer, block2, "      Read (controller->buffer):  ");
	}
	else
	{
		// There is enough space before we reach the end of the buffer
		memcpy( &m_buffer[m_head], _buffer, _size );
		m_head += _size;
		LogData(m_buffer+m_head-_size, _size, "      Read (controller->buffer):  ");
	}

	m_dataSize += _size;

	if( IsSignalled() )
	{
		// We now have more data than we are waiting for, so notify the watchers
		Notify();
	}

	m_mutex->Unlock();
	return true;
}

//-----------------------------------------------------------------------------
//	<Stream::Purge>
//	Empty the data buffer
//-----------------------------------------------------------------------------
void Stream::Purge
(
)
{
	m_tail = 0;
	m_head = 0;
	m_dataSize = 0;
}

//-----------------------------------------------------------------------------
//	<Stream::IsSignalled>
//	Test whether there is enough data to be signalled
//-----------------------------------------------------------------------------
bool Stream::IsSignalled
(
)
{
	return( m_dataSize >= m_signalSize );
}

//-----------------------------------------------------------------------------
//	<Stream::LogData>
//	Format the stream buffer data for log output
//-----------------------------------------------------------------------------
void Stream::LogData
(
	uint8* _buffer,
	uint32 _length,
	const string &_function
)
{
	if( !_length ) return;

	string str = "";
	for( uint32 i=0; i<_length; ++i ) 
	{
		if( i )
		{
			str += ", ";
		}
			
		char byteStr[8];
		snprintf( byteStr, sizeof(byteStr), "0x%.2x", _buffer[i] );
		str += byteStr;
	}
	Log::Write( LogLevel_StreamDetail, "%s%s", _function.c_str(), str.c_str() );
}
