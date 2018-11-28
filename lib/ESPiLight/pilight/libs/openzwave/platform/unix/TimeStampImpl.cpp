//-----------------------------------------------------------------------------
//
//	TimeStampImpl.cpp
//
//	OSX implementation of a TimeStamp
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
#include <string>
#include <cstring>
#include "../../Defs.h"
#include "TimeStampImpl.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<TimeStampImpl::TimeStampImpl>
//	Constructor
//-----------------------------------------------------------------------------
TimeStampImpl::TimeStampImpl
(
)
{
	SetTime(0);
}

//-----------------------------------------------------------------------------
//	<TimeStampImpl::~TimeStampImpl>
//	Destructor
//-----------------------------------------------------------------------------
TimeStampImpl::~TimeStampImpl
(
)
{
}

//-----------------------------------------------------------------------------
//	<TimeStampImpl::SetTime>
//	Sets the timestamp to now, plus an offset in milliseconds
//-----------------------------------------------------------------------------
void TimeStampImpl::SetTime
(
	int32 _milliseconds	// = 0
)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    
    m_stamp.tv_sec = now.tv_sec + (_milliseconds / 1000);
    
    // Now add the remainder of our timeout to the microseconds part of 'now'
    now.tv_usec += ((_milliseconds % 1000) * 1000);
    
    // Careful now! Did it wrap?
    if(now.tv_usec >= 1000000)
    {
        // Yes it did so bump our seconds and modulo
        now.tv_usec %= 1000000;
        m_stamp.tv_sec++;
    }
    
    m_stamp.tv_nsec = now.tv_usec * 1000;
}

//-----------------------------------------------------------------------------
//	<TimeStampImpl::TimeRemaining>
//	Gets the difference between now and the timestamp time in milliseconds
//-----------------------------------------------------------------------------
int32 TimeStampImpl::TimeRemaining
(
)
{
    int32 diff;
    
    struct timeval now;   
    gettimeofday(&now, NULL);
    
    // Seconds
    diff = (int32)((m_stamp.tv_sec - now.tv_sec)*1000);
    
    // Milliseconds
    diff += (((m_stamp.tv_nsec/1000)-now.tv_usec)/1000);
    
    return diff;
}

//-----------------------------------------------------------------------------
//	<TimeStampImpl::GetAsString>
//	Return a string representation
//-----------------------------------------------------------------------------
string TimeStampImpl::GetAsString
(
)
{
	char str[100];
	struct tm *tm;
	tm = localtime( &m_stamp.tv_sec );

	snprintf( str, sizeof(str), "%04d-%02d-%02d %02d:%02d:%02d:%03d ", 
		  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		  tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(m_stamp.tv_nsec / (1000*1000)) );
	return str;
}

//-----------------------------------------------------------------------------
//	<TimeStampImpl::operator->
//	Overload the subtract operator to get the difference between two 
//	timestamps in milliseconds
//-----------------------------------------------------------------------------
int32 TimeStampImpl::operator- 
(
	TimeStampImpl const& _other
)
{
    // Seconds
    int32 diff = (int32)((m_stamp.tv_sec-_other.m_stamp.tv_sec)*1000);
    
    // Milliseconds
    diff += ((m_stamp.tv_nsec - _other.m_stamp.tv_nsec)/1000000);
    
    return diff;  
}
