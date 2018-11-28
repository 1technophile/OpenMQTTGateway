//-----------------------------------------------------------------------------
//
//	LogImpl.cpp
//
//	Windows implementation of message and error logging
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
#include <windows.h>
#include <list>
#include <sys/time.h>
#include <time.h>

#include "../../Defs.h"
#include "LogImpl.h"

#ifdef _WIN32

#define vsprintf_s vsnprintf
#define sprintf_s snprintf

#define strcpy_s(DEST, NUM, SOURCE) strncpy(DEST, SOURCE, NUM)

errno_t fopen_s(FILE** pFile, const char *filename, const char *mode)
{
    if (!pFile)
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    *pFile = fopen(filename, mode);

    if (!*pFile)
    {
        return errno;
    }

    return 0;
}

#endif



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
	m_bAppendLog( _bAppendLog ),				// true to append (and not overwrite) any existing log
	m_bConsoleOutput( _bConsoleOutput ),		// true to provide a copy of output to console
	m_saveLevel( _saveLevel ),					// level of messages to log to file
	m_queueLevel( _queueLevel ),				// level of messages to log to queue
	m_dumpTrigger( _dumpTrigger )				// dump queued messages when this level is seen
{
	string accessType;

	// create an adjusted file name and timestamp string
	string timeStr = GetTimeStampString();

	if ( m_bAppendLog )
	{
		accessType = "a";
	}
	else
	{
		accessType = "w";
	}

	FILE* pFile;
	if( !fopen_s( &pFile, _filename.c_str(), accessType.c_str() ) )
	{
		fprintf( pFile, "\nLogging started %s\n\n", timeStr.c_str() );
		fclose( pFile );
	}
}

//-----------------------------------------------------------------------------
//	<LogImpl::~LogImpl>
//	Destructor
//-----------------------------------------------------------------------------
LogImpl::~LogImpl
(
)
{
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
	string logLevelStr = GetLogLevelString(_logLevel);

	// handle this message
	if( (_logLevel <= m_queueLevel) || (_logLevel == LogLevel_Internal) )	// we're going to do something with this message...
	{
		char lineBuf[1024];
		if( !_format || ( _format[0] == 0 ) )
		{
			strcpy_s( lineBuf, 1024, "" );
		}
		else
		{
			vsprintf_s( lineBuf, sizeof(lineBuf), _format, _args );
		}

		// should this message be saved to file (and possibly written to console?)
		if( (_logLevel <= m_saveLevel) || (_logLevel == LogLevel_Internal) )
		{
			// save to file
			FILE* pFile = NULL;
			if( !fopen_s( &pFile, m_filename.c_str(), "a" ) || m_bConsoleOutput )
			{
				if( _logLevel != LogLevel_Internal )						// don't add a second timestamp to display of queued messages
				{
					if( pFile != NULL )
					{
						fprintf( pFile, "%s%s%s", timeStr.c_str(), logLevelStr.c_str(), nodeStr.c_str() );
					}
					if( m_bConsoleOutput )
					{
						printf( "%s%s%s", timeStr.c_str(), logLevelStr.c_str(), nodeStr.c_str() );
					}
				}

				// print message to file (and possibly screen)
				if( pFile != NULL )
				{
					fprintf( pFile, "%s", lineBuf );
					fprintf( pFile, "\n" );
					fclose( pFile );
				}
				if( m_bConsoleOutput )
				{
					printf( "%s", lineBuf );
					printf( "\n" );
				}

			}
		}

		if( _logLevel != LogLevel_Internal )
		{
			char queueBuf[1024];
			string threadStr = GetThreadId();
			sprintf_s( queueBuf, sizeof(queueBuf), "%s%s%s", timeStr.c_str(), threadStr.c_str(), lineBuf );
			Queue( queueBuf );
		}
	}

	// now check to see if the _dumpTrigger has been hit
	if( (_logLevel <= m_dumpTrigger) && (_logLevel != LogLevel_Internal) && (_logLevel != LogLevel_Always) )
	{
		QueueDump();
	}
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
	Log::Write( LogLevel_Internal, "\n\nDumping queued log messages\n");
	list<string>::iterator it = m_logQueue.begin();
	while( it != m_logQueue.end() )
	{
		string strTemp = *it;
		Log::Write( LogLevel_Internal, strTemp.c_str() );
		++it;
	}
	m_logQueue.clear();
	Log::Write( LogLevel_Internal, "\nEnd of queued log message dump\n\n");
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
//	<LogImpl::GetTimeStampAndThreadId>
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
	DWORD dwThread = ::GetCurrentThreadId();
	sprintf_s( buf, sizeof(buf), "%04d ", dwThread );
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
