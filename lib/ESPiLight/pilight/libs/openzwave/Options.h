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

#ifndef _Options_H
#define _Options_H

#include <string>
#include <cstring>
#include <map>

#include "Defs.h"

namespace OpenZWave
{
	/** \brief Manages library options read from XML files or the command line.
	 *
	 * A class that manages program options read from XML files or the command line.
	 * The OpenZWave Manager class requires a complete and locked Options
	 * object when created.  The Options are therefore one of the first things that
	 * any OpenZWave application must deal with.
	 * Options are first read from an XML file called options.xml located in the
	 * User data folder (the path to which is supplied to the Options::Create method).
	 * This is the same folder that will be used by the Manager to save the state of
	 * each controller in the Z-Wave network, to avoid querying them for their entire
	 * state every time the application starts up.
	 * The second source of program options is a string, which will normally be the
	 * command line used to launch the application.
	 * In this way, common options can be specified in the XML, but over-ridden if
	 * necessary by the command line.
	 * The Options process is as follows:
	 * 1) Create an Options object, providing paths to the OpenZWave config folder,
	 * the User data folder and any command line string containing program options.
	 * 2) Call Options::AddOptionBool, Options::AddOptionInt or Options::AddOptionString
	 * to add any application-specific configurable options.  In this way, the Options
	 * class can be used for non-OpenZWave options as well.  (OpenZWave options must
	 * be specified in step #1 above (either via xml file or a command line string).
	 * 3) Call Options::Lock.  This will cause the option values to be read from
	 * the options.xml file and the command line string, and will lock the options
	 * so that no more calls aside from GetOptionAs may be made.
	 * 4) Create the OpenZWave Manager object.
	 */
	class OPENZWAVE_EXPORT Options
	{
	public:
		enum OptionType
		{
			OptionType_Invalid = 0,
			OptionType_Bool,
			OptionType_Int,
			OptionType_String
		};

   		/**
		 * Creates an object to manage the program options.
		 * \param _configPath a string containing the path to the OpenZWave library config
		 * folder, which contains XML descriptions of Z-Wave manufacturers and products.
		 * \param _userPath a string containing the path to the application's user data
		 * folder where the OpenZWave should store the Z-Wave network configuration and state.
		 * The _userPath is also the folder where OpenZWave will look for the file Options.xml
		 * which contains program option values.  The file should be in the form outlined below,
		 * with one or more Option elements containing a name and value attribute.  Multiple
		 * values with the same option name should be listed separately. Note that option names
		 * are case insensitive.
		 * \code
		 * <?xml version="1.0" encoding="utf-8"?>
		 * <Options>
		 *   <Option name="logging" value="true" />
		 *   <Option name="ignore" value="COMMAND_CLASS_BASIC" />
		 *   <Option name="ignore" value="COMMAND_CLASS_VERSION" />
		 * </Options>
		 * \endcode
		 * \param _commandLine a string containing the program's command line options.
		 * Command line options are parsed after the options.xml file, and so take precedence.
		 * Options are identified by a leading -- (two minus signs). The following items
		 * in the string are treated as values for this option, until the next -- is
		 * reached. For boolean options only, it is possible to omit the value, in which case
		 * the value is assumed to be "true".  Note that option names are case insensitive, and
		 * that option values should be separated by a space.
		 * \return Pointer to the newly created Options object.
		 * \see Get, Destroy, AddOption, GetOptionAs, Lock
		 */
		static Options* Create( string const& _configPath, string const& _userPath, string const& _commandLine );

		/**
		 * Deletes the Options and cleans up any associated objects.
		 * The application is responsible for destroying the Options object,
		 * but this must not be done until after the Manager object has been
		 * destroyed.
		 * \param _options Pointer to the Options object to be destroyed.
		 * \return true if the Options object was destroyed.  If the manager
		 * object still exists, this call will return false.
		 * \see Create, Get
		 */
		static bool Destroy();

		/**
		 * Gets a pointer to the Options singleton object.
		 * \return a pointer to the Options singleton object.
		 * \see Create, Destroy
		 */
		static Options* Get(){ return s_instance; }

		/**
		 * Locks the options.
		 * Reads in option values from  the XML options file and command line string and
		 * marks the options as locked.  Once locked, no more calls to AddOption
		 * can be made.
		 * The options must be locked before the Manager::Create method is called.
		 * \see AddOption
		 */
		bool Lock();

   		/**
		 * Add a boolean option to the program.
		 * Adds an option to the program whose value can then be read from a file or command line.
		 * All calls to AddOptionInt must be made before Lock.
		 * \param _name the name of the option.  Option names are case insensitive and must be unique.
		 * \param _default the default value for this option.
		 * \see GetOptionAsBool
		 */
		bool AddOptionBool( string const& _name, bool const _default );

		/**
		 * Add an integer option to the program.
		 * Adds an option to the program whose value can then be read from a file or command line.
		 * All calls to AddOptionInt must be made before Lock.
		 * \param _name the name of the option.  Option names are case insensitive and must be unique.
		 * \param _default the default value for this option.
		 * \see GetOptionAsInt
		 */
		bool AddOptionInt( string const& _name, int32 const _default );

		/**
		 * Add a string option to the program.
		 * Adds an option to the program whose value can then be read from a file or command line.
		 * All calls to AddOptionString must be made before Lock.
		 * \param _name the name of the option.  Option names are case insensitive and must be unique.
		 * \param _default the default value for this option.
		 * \param _append Setting append to true will cause values read from the command line
		 * or XML file to be concatenated into a comma delimited list.  If _append is false,
		 * newer values will overwrite older ones.
		 * \see GetOptionAsString
		 */
		bool AddOptionString( string const& _name, string const& _default, bool const _append );

		/**
		 * Get the value of a boolean option.
		 * \param _name the name of the option.  Option names are case insensitive.
		 * \param o_value a pointer to the item that will be filled with the option value.
		 * \return true if the option value was fetched successfully, false if the
		 * option does not exist, or does not contain a boolean value
		 * \see AddOptionBool, GetOptionType
		 */
		bool GetOptionAsBool( string const& _name, bool* o_value );

		/**
		 * Get the value of an integer option.
		 * \param _name the name of the option.  Option names are case insensitive.
		 * \param o_value a pointer to the item that will be filled with the option value.
		 * \return true if the option value was fetched successfully, false if the
		 * option does not exist, or does not contain an integer value
		 * \see AddOptionInt, GetOptionType
		 */
		bool GetOptionAsInt( string const& _name, int32* o_value );

		/**
		 * Get the value of a string option.
		 * \param _name the name of the option.  Option names are case insensitive.
		 * \param o_value a pointer to the item that will be filled with the option value.
		 * \return true if the option value was fetched successfully, false if the
		 * option does not exist, or does not contain a string value
		 * \see AddOptionString, GetOptionType
		 */
		bool GetOptionAsString( string const& _name, string* o_value );

		/**
		 * Get the type of value stored in an option.
		 * \param _name the name of the option.  Option names are case insensitive.
		 * \return An enum value representing the type of the option value.  If the
		 * option does not exist, OptionType_Invalid is returned.
		 * \see GetOptionAsBool, GetOptionAsInt, GetOptionAsString
		 */
		OptionType GetOptionType( string const& _name );

		/**
		 * Test whether the options have been locked.
		 * \return true if the options have been locked.
		 * \see Lock
		 */
		bool AreLocked()const{ return m_locked; }


	private:
		class Option
		{
			friend class Options;

		public:
			Option( string const& _name ):  m_name( _name ), m_append( false ){}
			bool SetValueFromString( string const& _value );

			Options::OptionType	m_type;
			string				m_name;
			bool				m_valueBool;
			int32				m_valueInt;
			string				m_valueString;
			bool				m_append;
		};

		Options( string const& _configPath, string const& _userPath, string const& _commandLine );	// Constructor, to be called only via the static Create method.
		~Options();																					// Destructor, to be called only via the static Destroy method.

		bool ParseOptionsString( string const& _options );					// Parse a string containing program options, such as a command line.
		bool ParseOptionsXML( string const& _filename );					// Parse an XML file containing program options.
		Option* AddOption( string const& _name );							// check lock and create (or open existing) option
		Option* Find( string const& _name );

OPENZWAVE_EXPORT_WARNINGS_OFF
		map<string,Option*>	m_options;										// Map of option names to values.
OPENZWAVE_EXPORT_WARNINGS_ON
		string				m_xml;											// Path to XML options file.
		string				m_commandLine;									// String containing command line options.
		string				m_SystemPath;
		string				m_LocalPath;
		bool				m_locked;										// If true, the options are final and AddOption can no longer be called.
		static Options*		s_instance;
	};
} // namespace OpenZWave

#endif // _Options_H
