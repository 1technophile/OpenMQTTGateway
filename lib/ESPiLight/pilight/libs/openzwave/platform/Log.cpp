//-----------------------------------------------------------------------------
//
//	Log.cpp
//
//	Cross-platform message and error logging
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
#include <stdarg.h>

#include "../Defs.h"
#include "Mutex.h"
#include "Log.h"

#ifdef _WIN32
#include "windows/LogImpl.h"	// Platform-specific implementation of a log
#else
#include "unix/LogImpl.h"	// Platform-specific implementation of a log
#endif


using namespace OpenZWave;

char const *OpenZWave::LogLevelString[] =
{
		"None", 	/**< LogLevel_None Disable all logging */
		"Always",   /**< LogLevel_Always These messages should always be shown */
		"Fatal",	/**< LogLevel_Fatal A likely fatal issue in the library */
		"Error", 	/**< LogLevel_Error A serious issue with the library or the network */
		"Warning",  /**< LogLevel_Warning A minor issue from which the library should be able to recover */
		"Alert",    /**< LogLevel_Alert Something unexpected by the library about which the controlling application should be aware */
		"Info", 	/**< LogLevel_Info Everything's working fine...these messages provide streamlined feedback on each message */
		"Detail", 	/**< LogLevel_Detail Detailed information on the progress of each message */
		"Debug", 	/**< LogLevel_Debug Very detailed information on progress that will create a huge log file quickly
									But this level (as others) can be queued and sent to the log only on an error or warning */
		"StreamDetail", 	/**< LogLevel_StreamDetail Will include low-level byte transfers from controller to buffer to application and back */
		"Internal" 		/**< LogLevel_Internal Used only within the log class (uses existing timestamp, etc.) */
};



Log* Log::s_instance = NULL;
i_LogImpl* Log::m_pImpl = NULL;
static bool s_dologging;

//-----------------------------------------------------------------------------
//	<Log::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
Log* Log::Create
(
	string const& _filename,
	bool const _bAppend,
	bool const _bConsoleOutput,
	LogLevel const _saveLevel,
	LogLevel const _queueLevel,
	LogLevel const _dumpTrigger
)
{
	if( NULL == s_instance )
	{
		s_instance = new Log( _filename, _bAppend, _bConsoleOutput, _saveLevel, _queueLevel, _dumpTrigger );
		s_dologging = true; // default logging to true so no change to what people experience now
	}

	return s_instance;
}

//-----------------------------------------------------------------------------
//	<Log::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
/* 	It isn't clear this is ever called or used.  If no one complains, consider
	deleting this code in April 2012.
Log* Log::Create
(
	i_LogImpl *LogClass
)
{
	if (NULL == s_instance )
	{
		s_instance = new Log( "" );
		s_dologging = true;
	}
	SetLoggingClass( LogClass );
	return s_instance;
}
*/

//-----------------------------------------------------------------------------
//	<Log::Destroy>
//	Static method to destroy the logging singleton.
//-----------------------------------------------------------------------------
void Log::Destroy
(
)
{
	delete s_instance;
	s_instance = NULL;
}

//-----------------------------------------------------------------------------
//	<Log::SetLoggingClass>
//	Set log class
//-----------------------------------------------------------------------------
bool Log::SetLoggingClass
(
	i_LogImpl *LogClass
)
{
	delete m_pImpl;
	m_pImpl = LogClass;
	return true;
}

//-----------------------------------------------------------------------------
//	<Log::SetLoggingState>
//	Set flag to actually write to log or skip it (legacy version)
//	If logging is enabled, the default log detail settings will be used
//	Write to file/screen		LogLevel_Detail
//	Save in queue for errors	LogLevel_Debug
//	Trigger for dumping queue	LogLevel_Warning
//	Console output?				Yes
//	Append to an existing log?	No (overwrite)
//-----------------------------------------------------------------------------
void Log::SetLoggingState
(
	bool _dologging
)
{
	bool prevLogging = s_dologging;
	s_dologging = _dologging;
	
	if (!prevLogging && s_dologging) Log::Write(LogLevel_Always, "Logging started\n\n");
}

//-----------------------------------------------------------------------------
//	<Log::SetLoggingState>
//	Set flag to actually write to log or skip it
//-----------------------------------------------------------------------------
void Log::SetLoggingState
( 
	LogLevel _saveLevel, 
	LogLevel _queueLevel, 
	LogLevel _dumpTrigger 
)
{
	// parameter checking:
	//  _queueLevel cannot be less than or equal to _saveLevel (where lower ordinals are more severe conditions)
	//  _dumpTrigger cannot be greater than or equal to _queueLevel
	if( _queueLevel <= _saveLevel )
		Log::Write( LogLevel_Warning, "Only lower priority messages may be queued for error-driven display." );
	if( _dumpTrigger >= _queueLevel )
		Log::Write( LogLevel_Warning, "The trigger for dumping queued messages must be a higher-priority message than the level that is queued." );

	bool prevLogging = s_dologging;
	// s_dologging is true if any messages are to be saved in file or queue
	if( (_saveLevel > LogLevel_Always) ||
		(_queueLevel > LogLevel_Always) )
	{
		s_dologging = true;
	}
	else
	{
		s_dologging = false;
	}

	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
	  	s_instance->m_logMutex->Lock();
		s_instance->m_pImpl->SetLoggingState( _saveLevel, _queueLevel, _dumpTrigger );
		s_instance->m_logMutex->Unlock();
	}
	
	if (!prevLogging && s_dologging) Log::Write(LogLevel_Always, "Logging started\n\n");
}

//-----------------------------------------------------------------------------
//	<Log::GetLoggingState>
//	Return a flag to indicate whether logging is enabled
//-----------------------------------------------------------------------------
bool Log::GetLoggingState
(
)
{
	return s_dologging;
}

//-----------------------------------------------------------------------------
//	<Log::Write>
//	Write to the log
//-----------------------------------------------------------------------------
void Log::Write
(
	LogLevel _level,
	char const* _format,
	...
)
{
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
		s_instance->m_logMutex->Lock(); // double locks if recursive
		va_list args;
		va_start( args, _format );
		s_instance->m_pImpl->Write( _level, 0, _format, args );
		va_end( args );
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::Write>
//	Write to the log
//-----------------------------------------------------------------------------
void Log::Write
(
	LogLevel _level,
	uint8 const _nodeId,
	char const* _format,
	...
)
{
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
		if( _level != LogLevel_Internal )
		  	s_instance->m_logMutex->Lock();
		va_list args;
		va_start( args, _format );
		s_instance->m_pImpl->Write( _level, _nodeId, _format, args );
		va_end( args );
		if( _level != LogLevel_Internal )
			s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::QueueDump>
//	Send queued messages to the log (and empty the queue)
//-----------------------------------------------------------------------------
void Log::QueueDump
(
)
{
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
	  	s_instance->m_logMutex->Lock();
		s_instance->m_pImpl->QueueDump();
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::QueueClear>
//	Empty the queued message queue
//-----------------------------------------------------------------------------
void Log::QueueClear
(
)
{
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
	  	s_instance->m_logMutex->Lock();
		s_instance->m_pImpl->QueueClear();
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::SetLogFileName>
//	Change the name of the log file (will start writing a new file)
//-----------------------------------------------------------------------------
void Log::SetLogFileName
(
	const string &_filename
)
{
	if( s_instance && s_dologging && s_instance->m_pImpl )
	{
	  	s_instance->m_logMutex->Lock();
		s_instance->m_pImpl->SetLogFileName( _filename );
		s_instance->m_logMutex->Unlock();
	}
}

//-----------------------------------------------------------------------------
//	<Log::Log>
//	Constructor
//-----------------------------------------------------------------------------
Log::Log
(
	string const& _filename,
	bool const _bAppend,
	bool const _bConsoleOutput,
	LogLevel const _saveLevel,
	LogLevel const _queueLevel,
	LogLevel const _dumpTrigger
):
	m_logMutex( new Mutex() )
{
        if (NULL == m_pImpl) 
        	m_pImpl = new LogImpl( _filename, _bAppend, _bConsoleOutput, _saveLevel, _queueLevel, _dumpTrigger );
}

//-----------------------------------------------------------------------------
//	<Log::~Log>
//	Destructor
//-----------------------------------------------------------------------------
Log::~Log
(
)
{
	m_logMutex->Release();
	delete m_pImpl;
	m_pImpl = NULL;
}
