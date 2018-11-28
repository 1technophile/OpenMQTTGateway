//-----------------------------------------------------------------------------
//
//	Log.h
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
#ifndef _Log_H
#define _Log_H

#include <stdarg.h>
#include <string>
#include "../Defs.h"



namespace OpenZWave
{
	class Mutex;
	extern char const *LogLevelString[];
	enum LogLevel
	{
		LogLevel_None,		/**< Disable all logging */
		LogLevel_Always,	/**< These messages should always be shown */
		LogLevel_Fatal,		/**< A likely fatal issue in the library */
		LogLevel_Error,		/**< A serious issue with the library or the network */
		LogLevel_Warning,	/**< A minor issue from which the library should be able to recover */
		LogLevel_Alert,		/**< Something unexpected by the library about which the controlling application should be aware */
		LogLevel_Info,		/**< Everything's working fine...these messages provide streamlined feedback on each message */
		LogLevel_Detail,	/**< Detailed information on the progress of each message */
		LogLevel_Debug,		/**< Very detailed information on progress that will create a huge log file quickly
									But this level (as others) can be queued and sent to the log only on an error or warning */
		LogLevel_StreamDetail, /**< Will include low-level byte transfers from controller to buffer to application and back */
		LogLevel_Internal	/**< Used only within the log class (uses existing timestamp, etc.) */
	};

	class i_LogImpl
	{
	public:
		i_LogImpl() { } ;
		virtual ~i_LogImpl() { } ;
		virtual void Write( LogLevel _level, uint8 const _nodeId, char const* _format, va_list _args ) = 0;
		virtual void QueueDump() = 0;
		virtual void QueueClear() = 0;
		virtual void SetLoggingState( LogLevel _saveLevel, LogLevel _queueLevel, LogLevel _dumpTrigger ) = 0;
		virtual void SetLogFileName( const string &_filename ) = 0;
	};

	/** \brief Implements a platform-independent log...written to the console and, optionally, a file.
	 */
	class Log
	{
	public:
		/**
		 * Create a log.
		 * Creates the cross-platform logging singleton.
		 * Any previous log will be cleared.
		 * \return a pointer to the logging object.
		 * \see Destroy, Write
		 */
		static Log* Create( string const& _filename, bool const _bAppend, bool const _bConsoleOutput, LogLevel const _saveLevel, LogLevel const _queueLevel, LogLevel const _dumpTrigger );

		/**
		 * Create a log.
		 * Creates the cross-platform logging singleton.
		 * Any previous log will be cleared.
		 * \param LogClass a Logging Class that inherits the i_LogImpl Class to use to Log
		 * \return a pointer to the logging object.
		 * \see Destroy, Write
		 */

		static Log* Create( i_LogImpl *LogClass );

		/**
		 * Destroys the log.
		 * Destroys the logging singleton.  The log can no longer
		 * be written to without another call to Create.
		 * \see Create, Write
		 */
		static void Destroy();

		/**
		 * \brief Set the Logging Implmentation Class to replace the standard File/Console Loggin
		 * \param LogClass A Logging Class that inherits the i_LogImpl Class used to Log to
		 * \return Bool Value indicating success or failure
		 */
		static bool SetLoggingClass(i_LogImpl *LogClass );

		/**
		 * \brief Enable or disable library logging (retained for backward compatibility)
		 * \param _dologging  If true, logging is enabled; if false, disabled
		*/
		static void SetLoggingState(bool _dologging);

		/**
		 * \brief Enable or disable library logging.  To disable, set _saveLevel and _queueLevel
		 * to LogLevel_None.
		 * \param _saveLevel	LogLevel of messages to write in real-time
		 * \param _queueLevel	LogLevel of messages to queue to be dumped in case of an error
		 * \param _dumpTrigger	LogLevel of message that triggers a queue dump (probably LogLevel_Error or LogLevel_Warning)
		*/
		static void SetLoggingState( LogLevel _saveLevel, LogLevel _queueLevel, LogLevel _dumpTrigger );

		/**
		 * \brief Determine whether logging is enabled or not (retained for backward compatibility)
		 * \param _dologging  If true, logging is enabled; if false, disabled
		*/
		static bool GetLoggingState();

		/**
		 * \brief Obtain the various logging levels.
		 * \param _saveLevel	LogLevel of messages to write in real-time
		 * \param _queueLevel	LogLevel of messages to queue to be dumped in case of an error
		 * \param _dumpTrigger	LogLevel of message that triggers a queue dump (probably LogLevel_Error or LogLevel_Warning)
		*/
		static void GetLoggingState( LogLevel* _saveLevel, LogLevel* _queueLevel, LogLevel* _dumpTrigger );

		/**
		 * \brief Change the log file name.  This will start a new log file (or potentially start appending
		 * information to an existing one.  Developers might want to use this function, together with a timer
		 * in the controlling application, to create timestamped log file names.
		 * \param _filename Name of the new (or existing) file to use for log output.
		*/
		static void SetLogFileName( const string &_filename );

		/**
		 * Write an entry to the log.
		 * Writes a formatted string to the log.
		 * \param _level	Specifies the type of log message (Error, Warning, Debug, etc.)
		 * \param _format.  A string formatted in the same manner as used with printf etc.
		 * \param ... a variable number of arguments, to be included in the formatted string.
		 * \see Create, Destroy
		 */
		static void Write( LogLevel _level, char const* _format, ... );

		/**
		 * Write an entry to the log.
		 * Writes a formatted string to the log.
		 * \param _level	Specifies the type of log message (Error, Warning, Debug, etc.)
		 * \param _nodeId	Node Id this entry is about.
		 * \param _format.  A string formatted in the same manner as used with printf etc.
		 * \param ... a variable number of arguments, to be included in the formatted string.
		 * \see Create, Destroy
		 */
		static void Write( LogLevel _level, uint8 const _nodeId, char const* _format, ... );

		/**
		 * Send the queued log messages to the log output.
		 */
		static void QueueDump();

		/**
		 * Clear the log message queue
		 */
		static void QueueClear();

	private:
		Log( string const& _filename, bool const _bAppend, bool const _bConsoleOutput, LogLevel _saveLevel, LogLevel _queueLevel, LogLevel _dumpTrigger );
		~Log();

		static i_LogImpl*	m_pImpl;		/**< Pointer to an object that encapsulates the platform-specific logging implementation. */
		static Log*	s_instance;
		Mutex*		m_logMutex;
	};
} // namespace OpenZWave

#endif //_Log_H
