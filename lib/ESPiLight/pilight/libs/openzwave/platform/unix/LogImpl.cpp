//-----------------------------------------------------------------------------
//
//	LogImpl.cpp
//
//  Unix implementation of message and error logging
//
//	Copyright (c) 2010, Greg Satz <satz@iranger.com>
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
#ifdef _WIN32
	#include "../../libs/pthreadw32/pthread.h"
#else
	#include <pthread.h>
#endif
#include <iostream>
#include "../../Defs.h"
#include "LogImpl.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<LogImpl::LogImpl>
//	Constructor
//-----------------------------------------------------------------------------
LogImpl::LogImpl
(
	string const& _filename,
	bool const _bAppendLog,
	bool const _bConsoleOutput,
	LogLevel const _saveLevel,
	LogLevel const _queueLevel,
	LogLevel const _dumpTrigger
):
	m_filename( _filename ),					// name of log file
	m_bConsoleOutput( _bConsoleOutput ),		// true to provide a copy of output to console
	m_bAppendLog( _bAppendLog ),				// true to append (and not overwrite) any existing log
	m_saveLevel( _saveLevel ),					// level of messages to log to file
	m_queueLevel( _queueLevel ),				// level of messages to log to queue
	m_dumpTrigger( _dumpTrigger )				// dump queued messages when this level is seen
{
	if ( !m_bAppendLog )
	{
		this->pFile = fopen( m_filename.c_str(), "w" );
	} else {
		this->pFile = fopen( m_filename.c_str(), "a" );
	}
	if( this->pFile == NULL )
	{
		std::cerr << "Could Not Open OZW Log File." << std::endl;
	}
	setlinebuf(stdout);	// To prevent buffering and lock contention issues
}

//-----------------------------------------------------------------------------
//	<LogImpl::~LogImpl>
//	Destructor
//-----------------------------------------------------------------------------
LogImpl::~LogImpl
(
)
{
	fclose( this->pFile );
}

//-----------------------------------------------------------------------------
//	<LogImpl::Write>
//	Write to the log
//-----------------------------------------------------------------------------
void LogImpl::Write
(
	LogLevel _logLevel,
	uint8 const _nodeId,
	char const* _format,
	va_list _args
)
{
	// create a timestamp string
	string timeStr = GetTimeStampString();
	string nodeStr = GetNodeString( _nodeId );
	string loglevelStr = GetLogLevelString(_logLevel);

	// handle this message
	if( (_logLevel <= m_queueLevel) || (_logLevel == LogLevel_Internal) )	// we're going to do something with this message...
	{
		char lineBuf[1024] = {0};
		//int lineLen = 0;
		if( _format != NULL && _format[0] != '\0' )
		{
			va_list saveargs;
			va_copy( saveargs, _args );

			vsnprintf( lineBuf, sizeof(lineBuf), _format, _args );
			va_end( saveargs );
		}

		// should this message be saved to file (and possibly written to console?)
		if( (_logLevel <= m_saveLevel) || (_logLevel == LogLevel_Internal) )
		{
			std::string outBuf;

			if ( this->pFile != NULL || m_bConsoleOutput )
			{
				if( _logLevel != LogLevel_Internal )						// don't add a second timestamp to display of queued messages
				{
					outBuf.append(timeStr);
					outBuf.append(loglevelStr);
					outBuf.append(nodeStr);
					outBuf.append(lineBuf);
					outBuf.append("\n");

				}

				// print message to file (and possibly screen)
				if( this->pFile != NULL )
				{
					fputs( outBuf.c_str(), pFile );
				}
				if( m_bConsoleOutput )
				{
					fputs( outBuf.c_str(), stdout );
				}
			}
		}

		if( _logLevel != LogLevel_Internal )
		{
			char queueBuf[1024];
			string threadStr = GetThreadId();
			snprintf( queueBuf, sizeof(queueBuf), "%s%s%s", timeStr.c_str(), threadStr.c_str(), lineBuf );
			Queue( queueBuf );
		}
	}

	// now check to see if the _dumpTrigger has been hit
	if( (_logLevel <= m_dumpTrigger) && (_logLevel != LogLevel_Internal) && (_logLevel != LogLevel_Always) )
		QueueDump();
}

//-----------------------------------------------------------------------------
//	<LogImpl::Queue>
//	Write to the log queue
//-----------------------------------------------------------------------------
void LogImpl::Queue
(
	char const* _buffer
)
{
	string bufStr = _buffer;
	m_logQueue.push_back( bufStr );

	// rudimentary queue size management
	if( m_logQueue.size() > 500 )
	{
		m_logQueue.pop_front();
	}
}

//-----------------------------------------------------------------------------
//	<LogImpl::QueueDump>
//	Dump the LogQueue to output device
//-----------------------------------------------------------------------------
void LogImpl::QueueDump
(
)
{
	Log::Write( LogLevel_Always, "" );
	Log::Write( LogLevel_Always, "Dumping queued log messages");
	Log::Write( LogLevel_Always, "" );
	list<string>::iterator it = m_logQueue.begin();
	while( it != m_logQueue.end() )
	{
		string strTemp = *it;
		Log::Write( LogLevel_Internal, strTemp.c_str() );
		it++;
	}
	m_logQueue.clear();
	Log::Write( LogLevel_Always, "" );
	Log::Write( LogLevel_Always, "End of queued log message dump");
	Log::Write( LogLevel_Always, "" );
}

//-----------------------------------------------------------------------------
//	<LogImpl::Clear>
//	Clear the LogQueue
//-----------------------------------------------------------------------------
void LogImpl::QueueClear
(
)
{
	m_logQueue.clear();
}

//-----------------------------------------------------------------------------
//	<LogImpl::SetLoggingState>
//	Sets the various log state variables
//-----------------------------------------------------------------------------
void LogImpl::SetLoggingState
(
	LogLevel _saveLevel,
	LogLevel _queueLevel,
	LogLevel _dumpTrigger
)
{
	m_saveLevel = _saveLevel;
	m_queueLevel = _queueLevel;
	m_dumpTrigger = _dumpTrigger;
}

//-----------------------------------------------------------------------------
//	<LogImpl::GetTimeStampString>
//	Generate a string with formatted current time
//-----------------------------------------------------------------------------
string LogImpl::GetTimeStampString
(
)
{
	struct timeval tv;
	struct tm tm;
	char fmt[64], buf[64], line[256];
	gettimeofday(&tv, NULL);
#ifdef _WIN32
	struct tm *tm1;
	if((tm1 = gmtime(&tv.tv_sec)) != 0) {
		memcpy(&tm, tm1, sizeof(struct tm));
#else
	if((gmtime_r(&tv.tv_sec, &tm)) != 0) {
#endif
		strftime(fmt, sizeof(fmt), "%b %d %H:%M:%S", &tm);
		snprintf(buf, sizeof(buf), "%s:%03u", fmt, (unsigned int)tv.tv_usec);
	}
	sprintf(line, "[%22.22s] %s: [Z-Wave]: ", buf, "pilight-daemon");

	string str = line;
	return str;
}

//-----------------------------------------------------------------------------
//	<LogImpl::GetNodeString>
//	Generate a string with formatted node id
//-----------------------------------------------------------------------------
string LogImpl::GetNodeString
(
	uint8 const _nodeId
)
{
	if( _nodeId == 0 )
	{
		return "";
	}
	else
		if( _nodeId == 255 ) // should make distinction between broadcast and controller better for SwitchAll broadcast
		{
			return "contrlr, ";
		}
		else
		{
			char buf[20];
			snprintf( buf, sizeof(buf), "Node%03d, ", _nodeId );
			return buf;
		}
}

//-----------------------------------------------------------------------------
//	<LogImpl::GetThreadId>
//	Generate a string with formatted thread id
//-----------------------------------------------------------------------------
string LogImpl::GetThreadId
(
)
{
	char buf[20];
	snprintf( buf, sizeof(buf), "%08lx ", (long unsigned int)pthread_self() );
	string str = buf;
	return str;
}

//-----------------------------------------------------------------------------
//	<LogImpl::SetLogFileName>
//	Provide a new log file name (applicable to future writes)
//-----------------------------------------------------------------------------
void LogImpl::SetLogFileName
(
	const string &_filename
)
{
	m_filename = _filename;
}


//-----------------------------------------------------------------------------
//	<LogImpl::GetLogLevelString>
//	Provide a new log file name (applicable to future writes)
//-----------------------------------------------------------------------------
string LogImpl::GetLogLevelString
(
		LogLevel _level
)
{
	if ((_level >= LogLevel_None) && (_level <= LogLevel_Internal)) {
		char buf[20];
		snprintf( buf, sizeof(buf), "%s, ", LogLevelString[_level] );
		return buf;
	}
	else
		return "Unknown, ";
}
