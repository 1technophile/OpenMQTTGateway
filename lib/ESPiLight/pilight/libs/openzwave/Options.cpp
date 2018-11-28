//-----------------------------------------------------------------------------
//
//	Options.h
//
//	Program options read from XML files or the command line.
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
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

#include <algorithm>
#include <string>

#include "Defs.h"
#include "Options.h"
#include "Utils.h"
#include "Manager.h"
#include "platform/Log.h"
#include "platform/FileOps.h"
#include "tinyxml.h"

using namespace OpenZWave;

Options* Options::s_instance = NULL;

//-----------------------------------------------------------------------------
// <Options::Create>
// Static method to create an Options object
//-----------------------------------------------------------------------------
Options* Options::Create
(
	string const& _configPath,
	string const& _userPath,
	string const& _commandLine
)
{
	if( s_instance == NULL )
	{
		string configPath = _configPath;
		string userPath = _userPath;

		// Make sure a trailing path delimiter is present
		if( configPath.size() > 0 && configPath[configPath.size() - 1] != '/' )
		{
			configPath += "/";
		}
		if( userPath.size() > 0 && userPath[userPath.size() - 1] != '/' )
		{
			userPath += "/";
		}

		FileOps::Create();
		if( !FileOps::FolderExists( configPath ) )
		{
			Log::Create( "", false, true, LogLevel_Debug, LogLevel_Debug, LogLevel_None );
			/* Try some default directories */
			if ( FileOps::FolderExists( "config/" ) )
			{
				Log::Write( LogLevel_Error, "Cannot find a path to the configuration files at %s, Using config/ instead...", configPath.c_str() );
				configPath = "config/";
			} else if (FileOps::FolderExists("/etc/openzwave/" ) )
			{
				Log::Write( LogLevel_Error, "Cannot find a path to the configuration files at %s, Using /etc/openzwave/ instead...", configPath.c_str() );
				configPath = "/etc/openzwave/";
#ifdef SYSCONFDIR
			} else if ( FileOps::FolderExists(SYSCONFDIR ) )
			{
				Log::Write( LogLevel_Error, "Cannot find a path to the configuration files at %s, Using %s instead...", configPath.c_str(), SYSCONFDIR);
				configPath = SYSCONFDIR;
#endif
			} else {
				Log::Write( LogLevel_Error, "Cannot find a path to the configuration files at %s. Exiting...", configPath.c_str() );
				exit( 1 );
			}
		}
		FileOps::Destroy();
		s_instance = new Options( configPath, userPath, _commandLine );

		// Add the default options
		s_instance->AddOptionString(	"ConfigPath",				configPath,	false );	// Path to the OpenZWave config folder.
		s_instance->AddOptionString(	"UserPath",					userPath,		false );	// Path to the user's data folder.

		s_instance->AddOptionBool(		"Logging",					true );						// Enable logging of library activity.
		s_instance->AddOptionString(	"LogFileName",				"OZW_Log.txt",	false );	// Name of the log file (can be changed via Log::SetLogFileName)
		s_instance->AddOptionBool(		"AppendLogFile",			false );					// Append new session logs to existing log file (false = overwrite)
		s_instance->AddOptionBool(		"ConsoleOutput",			true );						// Display log information on console (as well as save to disk)
		s_instance->AddOptionInt(		"SaveLogLevel",				LogLevel_Detail );			// Save (to file) log messages equal to or above LogLevel_Detail
		s_instance->AddOptionInt(		"QueueLogLevel",			LogLevel_Debug );			// Save (in RAM) log messages equal to or above LogLevel_Debug
		s_instance->AddOptionInt(		"DumpTriggerLevel",			LogLevel_None );			// Default is to never dump RAM-stored log messages

		s_instance->AddOptionBool(		"Associate",				true );						// Enable automatic association of the controller with group one of every device.
		s_instance->AddOptionString(	"Exclude",					string(""),		true );		// Remove support for the listed command classes.
		s_instance->AddOptionString(	"Include",					string(""),		true );		// Only handle the specified command classes.  The Exclude option is ignored if anything is listed here.
		s_instance->AddOptionBool(		"NotifyTransactions",		false );					// Notifications when transaction complete is reported.
		s_instance->AddOptionString(	"Interface",				string(""),		true );		// Identify the serial port to be accessed (TODO: change the code so more than one serial port can be specified and HID)
		s_instance->AddOptionBool(		"SaveConfiguration",		true );						// Save the XML configuration upon driver close.
		s_instance->AddOptionInt(		"DriverMaxAttempts",		0);

		s_instance->AddOptionInt(		"PollInterval",				30000);						// 30 seconds (can easily poll 30 values in this time; ~120 values is the effective limit for 30 seconds)
		s_instance->AddOptionBool(		"IntervalBetweenPolls",		false );					// if false, try to execute the entire poll list within the PollInterval time frame
																								// if true, wait for PollInterval milliseconds between polls
		s_instance->AddOptionBool(		"SuppressValueRefresh",		false );					// if true, notifications for refreshed (but unchanged) values will not be sent
		s_instance->AddOptionBool(		"PerformReturnRoutes",		true );					// if true, return routes will be updated
		s_instance->AddOptionString(	"NetworkKey", 				string(""), 			false);
		s_instance->AddOptionBool(		"RefreshAllUserCodes",		false ); 					// if true, during startup, we refresh all the UserCodes the device reports it supports. If False, we stop after we get the first "Available" slot (Some devices have 250+ usercode slots! - That makes our Session Stage Very Long )
		s_instance->AddOptionInt( 		"RetryTimeout", 			RETRY_TIMEOUT);				// How long do we wait to timeout messages sent
		s_instance->AddOptionBool( 		"EnableSIS", 				true);						// Automatically become a SUC if there is no SUC on the network.
		s_instance->AddOptionBool( 		"AssumeAwake", 				true);						// Assume Devices that Support the Wakeup CC are awake when we first query them....
		s_instance->AddOptionBool(		"NotifyOnDriverUnload",		false);						// Should we send the Node/Value Notifications on Driver Unloading - Read comments in Driver::~Driver() method about possible race conditions
	}

	return s_instance;
}

//-----------------------------------------------------------------------------
// <Options::Destroy>
// Static method to destroy an Options object
//-----------------------------------------------------------------------------
bool Options::Destroy
(
)
{
	if( Manager::Get() )
	{
		// Cannot delete Options because Manager object still exists
		assert(0);
		return false;
	}

	delete s_instance;
	s_instance = NULL;

	return true;
}

//-----------------------------------------------------------------------------
// <Options::Options>
// Constructor
//-----------------------------------------------------------------------------
Options::Options
(
	string const& _configPath,
	string const& _userPath,
	string const& _commandLine
):
	m_xml ("options.xml"),
	m_commandLine( _commandLine ),
	m_SystemPath (_configPath),
	m_LocalPath (_userPath),
	m_locked( false )
{
}

//-----------------------------------------------------------------------------
// <Options::~Options>
// Destructor
//-----------------------------------------------------------------------------
Options::~Options
(
)
{
	// Clear the options map
	while( !m_options.empty() )
	{
		map<string,Option*>::iterator it = m_options.begin();
		delete it->second;
		m_options.erase( it );
	}
}

//-----------------------------------------------------------------------------
// <Options::AddOptionBool>
// Add a boolean option.
//-----------------------------------------------------------------------------
bool Options::AddOptionBool
(
	string const& _name,
	bool const _value
)
{
	// get (or create) option
	Option* option = AddOption( _name );

	if (option == NULL) return false;

	// set unique option members
	option->m_type = Options::OptionType_Bool;
	option->m_valueBool = _value;

	// save in m_options map
	string lowerName = ToLower( _name );
	m_options[lowerName] = option;
	return true;
}

//-----------------------------------------------------------------------------
// <Options::AddOptionInt>
// Add an integer option.
//-----------------------------------------------------------------------------
bool Options::AddOptionInt
(
	string const& _name,
	int32 const _value
)
{
	// get (or create) option
	Option* option = AddOption( _name );

	if (option == NULL) return false;

	// set unique option members
	option->m_type = Options::OptionType_Int;
	option->m_valueInt = _value;

	// save in m_options map
	string lowerName = ToLower( _name );
	m_options[lowerName] = option;
	return true;
}

//-----------------------------------------------------------------------------
// <Options::AddOptionString>
// Add a string option.
//-----------------------------------------------------------------------------
bool Options::AddOptionString
(
	string const& _name,
	string const& _value,
	bool const _append
)
{
	// get (or create) option
	Option* option = AddOption( _name );

	if (option == NULL) return false;

	// set unique option members
	option->m_type = Options::OptionType_String;
	option->m_valueString = _value;
	option->m_append = _append;

	// save in m_options map
	string lowerName = ToLower( _name );
	m_options[lowerName] = option;
	return true;
}

//-----------------------------------------------------------------------------
// <Options::GetOptionAsBool>
// Get the value of a boolean option.
//-----------------------------------------------------------------------------
bool Options::GetOptionAsBool
(
	string const& _name,
	bool* o_value
)
{
	Option* option = Find( _name );
	if( o_value && option && ( OptionType_Bool == option->m_type ) )
	{
		*o_value = option->m_valueBool;
		return true;
	}

	Log::Write( LogLevel_Warning, "Specified option [%s] was not found.", _name.c_str() );
	return false;
}

//-----------------------------------------------------------------------------
// <Options::GetOptionAsInt>
// Get the value of an integer option.
//-----------------------------------------------------------------------------
bool Options::GetOptionAsInt
(
	string const& _name,
	int32* o_value
)
{
	Option* option = Find( _name );
	if( o_value && option && ( OptionType_Int == option->m_type ) )
	{
		*o_value = option->m_valueInt;
		return true;
	}

	Log::Write( LogLevel_Warning, "Specified option [%s] was not found.", _name.c_str() );
	return false;
}

//-----------------------------------------------------------------------------
// <Options::GetOptionAsString>
// Get the value of a string option.
//-----------------------------------------------------------------------------
bool Options::GetOptionAsString
(
	string const& _name,
	string* o_value
)
{
	Option* option = Find( _name );
	if( o_value && option && ( OptionType_String == option->m_type ) )
	{
		*o_value = option->m_valueString;
		return true;
	}

	Log::Write( LogLevel_Warning, "Specified option [%s] was not found.", _name.c_str() );
	return false;
}

//-----------------------------------------------------------------------------
// <Options::GetOptionType>
// Get the type of value stored in an option.
//-----------------------------------------------------------------------------
Options::OptionType Options::GetOptionType
(
	string const& _name
)
{
	Option* option = Find( _name );
	if( option )
	{
		return option->m_type;
	}

	// Option not found
	Log::Write( LogLevel_Warning, "Specified option [%s] was not found.", _name.c_str() );
	return OptionType_Invalid;
}

//-----------------------------------------------------------------------------
// <Options::Lock>
// Read all the option XMLs and Command Lines, and lock their values.
//-----------------------------------------------------------------------------
bool Options::Lock
(
)
{
	if( m_locked )
	{
		Log::Write( LogLevel_Error, "Options are already final (locked)." );
		return false;
	}

	ParseOptionsXML( m_SystemPath + m_xml );
	ParseOptionsXML( m_LocalPath + m_xml);
	ParseOptionsString( m_commandLine );
	m_locked = true;

	return true;
}

//-----------------------------------------------------------------------------
// <Options::ParseOptionsString>
// Parse a string containing program options, such as a command line
//-----------------------------------------------------------------------------
bool Options::ParseOptionsString
(
	string const& _commandLine
)
{
	bool res = true;

	size_t pos = 0;
	size_t start = 0;
	while( 1 )
	{
		// find start of first option name
		pos = _commandLine.find_first_of( "--", start );
		if( string::npos == pos )
		{
			break;
		}
		start = pos + 2;

		// found an option.  Get the name.
		string optionName;
		pos = _commandLine.find( " ", start );
		if( string::npos == pos )
		{
			optionName = _commandLine.substr( start );
			start = pos;
		}
		else
		{
			optionName = _commandLine.substr( start, pos-start );
			start = pos + 1;
		}

		// Find the matching option object
		Option* option = Find( optionName );
		if( option )
		{
			// Read the values
			int numValues = 0;
			bool parsing = true;
			while( parsing )
			{
				string value;
				size_t back = start;
				pos = _commandLine.find( " ", start );
				if( string::npos == pos )
				{
					// Last value in string
					value = _commandLine.substr( start );
					parsing = false;
					start = pos;
				}
				else
				{
					value = _commandLine.substr( start, pos-start );
					start = pos+1;
				}

				if( !value.compare( 0, 2, "--" ) )
				{
					// Value is actually the next option.
					if( !numValues )
					{
						// No values were read for this option
						// This is ok only for bool options, where we assume no value means "true".
						if( OptionType_Bool == option->m_type )
						{
							option->m_valueBool = true;
						}
						else
						{
							res = false;
						}
					}
					start = back;		// back up to the beginning of the next option
					break;
				}
				else if( value.size() > 0 )
				{
					// Set the value
					option->SetValueFromString( value );
					numValues++;
				}
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Options::ParseOptionsXML>
// Parse an XML file containing program options
//-----------------------------------------------------------------------------
bool Options::ParseOptionsXML
(
	string const& _filename
)
{
	TiXmlDocument doc;
	if( !doc.LoadFile( _filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		Log::Write(LogLevel_Warning, "Failed to Parse %s: %s", _filename.c_str(), doc.ErrorDesc());
		return false;
	}
	Log::Write(LogLevel_Info, "Reading %s for Options", _filename.c_str());

	TiXmlElement const* optionsElement = doc.RootElement();

	// Read the options
	TiXmlElement const* optionElement = optionsElement->FirstChildElement();
	while( optionElement )
	{
		char const* str = optionElement->Value();
		if( str && !strcmp( str, "Option" ) )
		{
			char const* name = optionElement->Attribute( "name" );
			if( name )
			{
				Option* option = Find( name );
				if( option )
				{
					char const* value = optionElement->Attribute( "value" );
					if( value )
					{
						// Set the value
						option->SetValueFromString( value );
					}
				}
			}
		}

		optionElement = optionElement->NextSiblingElement();
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Options::AddOption>
// General setup for adding a specific option
//-----------------------------------------------------------------------------
Options::Option* Options::AddOption
(
	string const& _name
)
{
	if( m_locked )
	{
		Log::Write( LogLevel_Error, "Options have been locked.  No more may be added." );
		return NULL;
	}

	// get a pointer to the option (and create a new Option if it doesn't already exist)
	Option* option = Find( _name );
	if( option == NULL )
	{
		option = new Option( _name );
	}

	return option;
}

//-----------------------------------------------------------------------------
// <Options::Find>
// Find an option by name
//-----------------------------------------------------------------------------
Options::Option* Options::Find
(
	string const& _name
)
{
	string lowername = ToLower( _name );
	map<string,Option*>::iterator it = m_options.find( lowername );
	if( it != m_options.end() )
	{
		return it->second;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// <Options::Option::SetValueFromString>
// Find an option by name
//-----------------------------------------------------------------------------
bool Options::Option::SetValueFromString
(
	string const& _value
)
{
	if( OptionType_Bool == m_type )
	{
		string lowerValue = ToLower( _value );
		if( ( lowerValue == "true" ) || ( lowerValue == "1" ) )
		{
			m_valueBool = true;
			return true;
		}

		if( ( lowerValue == "false" ) || ( lowerValue == "0" ) )
		{
			m_valueBool = false;
			return true;
		}

		return false;
	}

	if( OptionType_Int == m_type )
	{
		m_valueInt = (int32)atol( _value.c_str() );
		return true;
	}

	if( OptionType_String == m_type )
	{
		if( m_append && ( m_valueString.size() > 0 ) )
		{
			m_valueString += ( string(",") + _value );
		}
		else
		{
			m_valueString = _value;
		}
		return true;
	}

	return false;
}
