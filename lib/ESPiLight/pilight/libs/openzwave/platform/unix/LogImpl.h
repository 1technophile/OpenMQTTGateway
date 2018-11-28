//-----------------------------------------------------------------------------
//
//	LogImpl.h
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
#ifndef _LogImpl_H
#define _LogImpl_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <list>
#include "../Log.h"

namespace OpenZWave
{
	class LogImpl : public i_LogImpl
	{
	private:
		friend class Log;

		LogImpl( string const& _filename, bool const _bAppendLog, bool const _bConsoleOutput, LogLevel const _saveLevel, LogLevel const _queueLevel, LogLevel const _dumpTrigger );
		~LogImpl();

		void Write( LogLevel _level, uint8 const _nodeId, char const* _format, va_list _args );
		void Queue( char const* _buffer );
		void QueueDump();
		void QueueClear();
		void SetLoggingState( LogLevel _saveLevel, LogLevel _queueLevel, LogLevel _dumpTrigger );
		void SetLogFileName( const string &_filename );

		string GetTimeStampString();
		string GetNodeString( uint8 const _nodeId );
		string GetThreadId();
		string GetLogLevelString(LogLevel _level);

		string m_filename;						/**< filename specified by user (default is ozw_log.txt) */
		bool m_bConsoleOutput;					/**< if true, send log output to console as well as to the file */
		bool m_bAppendLog;						/**< if true, the log file should be appended to any with the same name */
		list<string> m_logQueue;				/**< list of queued log messages */
		LogLevel m_saveLevel;
		LogLevel m_queueLevel;
		LogLevel m_dumpTrigger;
		FILE* pFile;
	};

} // namespace OpenZWave

#endif //_LogImpl_H

