//-----------------------------------------------------------------------------
//
//	CommandClasses.cpp
//
//	Singleton holding methods to create each command class object
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

#include <string.h>

#include "CommandClasses.h"

using namespace OpenZWave;

#include "Alarm.h"
#include "ApplicationStatus.h"
#include "Association.h"
#include "AssociationCommandConfiguration.h"
#include "Basic.h"
#include "BasicWindowCovering.h"
#include "Battery.h"
#include "ClimateControlSchedule.h"
#include "Clock.h"
#include "Configuration.h"
#include "ControllerReplication.h"
#include "CRC16Encap.h"
#include "DoorLock.h"
#include "DoorLockLogging.h"
#include "EnergyProduction.h"
#include "Hail.h"
#include "Indicator.h"
#include "Language.h"
#include "Lock.h"
#include "ManufacturerSpecific.h"
#include "Meter.h"
#include "MeterPulse.h"
#include "MultiCmd.h"
#include "MultiInstance.h"
#include "MultiInstanceAssociation.h"
#include "NodeNaming.h"
#include "NoOperation.h"
#include "Powerlevel.h"
#include "Proprietary.h"
#include "Protection.h"
#include "SceneActivation.h"
#include "Security.h"
#include "SensorAlarm.h"
#include "SensorBinary.h"
#include "SensorMultilevel.h"
#include "SwitchAll.h"
#include "SwitchBinary.h"
#include "SwitchMultilevel.h"
#include "SwitchToggleBinary.h"
#include "SwitchToggleMultilevel.h"
#include "TimeParameters.h"
#include "ThermostatFanMode.h"
#include "ThermostatFanState.h"
#include "ThermostatMode.h"
#include "ThermostatOperatingState.h"
#include "ThermostatSetpoint.h"
#include "UserCode.h"
#include "Version.h"
#include "WakeUp.h"

#include "../value_classes/ValueBool.h"
#include "../value_classes/ValueButton.h"
#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueDecimal.h"
#include "../value_classes/ValueInt.h"
#include "../value_classes/ValueList.h"
#include "../value_classes/ValueSchedule.h"
#include "../value_classes/ValueShort.h"
#include "../value_classes/ValueString.h"

#include "../Manager.h"
#include "../Options.h"
#include "../Utils.h"

//-----------------------------------------------------------------------------
//	<CommandClasses::CommandClasses>
//	Constructor
//-----------------------------------------------------------------------------
CommandClasses::CommandClasses
(
)
{
	memset( m_commandClassCreators, 0, sizeof(pfnCreateCommandClass_t)*256 );
	memset( m_supportedCommandClasses, 0, sizeof(uint32)*8 );
}

//-----------------------------------------------------------------------------
//	<CommandClasses::IsSupported>
//	Static method to determine whether a command class is supported
//-----------------------------------------------------------------------------
bool CommandClasses::IsSupported
(
	uint8 const _commandClassId
)
{
	// Test the bit representing the command class
	return( (Get().m_supportedCommandClasses[_commandClassId>>5] & (1u<<(_commandClassId&0x1f))) != 0 );
}
string CommandClasses::GetName(
	uint8 const _commandClassId
)
{
	for (std::map<string,uint8>::iterator it = Get().m_namesToIDs.begin(); it != Get().m_namesToIDs.end(); it++) {
		if (it->second == _commandClassId)
			return it->first;
	}
	return string("Unknown");
}
//-----------------------------------------------------------------------------
//	<CommandClasses::Register>
//	Static method to register a command class creator method
//-----------------------------------------------------------------------------
void CommandClasses::Register
(
	uint8 const _commandClassId,
	string const& _commandClassName,
	pfnCreateCommandClass_t _creator
)
{
	m_commandClassCreators[_commandClassId] = _creator;

	// Set the bit representing the command class
	Get().m_supportedCommandClasses[_commandClassId>>5] |= (1u<<(_commandClassId&0x1f));

	m_namesToIDs[_commandClassName] = _commandClassId;
}

//-----------------------------------------------------------------------------
//	<CommandClasses::CreateCommandClass>
//	Create a command class object using the registered method
//-----------------------------------------------------------------------------
CommandClass* CommandClasses::CreateCommandClass
(
	uint8 const _commandClassId,
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	// Get a pointer to the required CommandClass's Create method
	pfnCreateCommandClass_t creator = Get().m_commandClassCreators[_commandClassId];
	if( NULL == creator )
	{
		return NULL;
	}

	// Create an instance of the command class
	return creator( _homeId, _nodeId );
}

//-----------------------------------------------------------------------------
//	<CommandClasses::RegisterCommandClasses>
//	Register all our implemented command classes
//-----------------------------------------------------------------------------
void CommandClasses::RegisterCommandClasses
(
)
{
	CommandClasses& cc = Get();
	cc.Register( Alarm::StaticGetCommandClassId(), Alarm::StaticGetCommandClassName(), Alarm::Create );
	cc.Register( ApplicationStatus::StaticGetCommandClassId(), ApplicationStatus::StaticGetCommandClassName(), ApplicationStatus::Create );
	cc.Register( Association::StaticGetCommandClassId(), Association::StaticGetCommandClassName(), Association::Create );
	cc.Register( AssociationCommandConfiguration::StaticGetCommandClassId(), AssociationCommandConfiguration::StaticGetCommandClassName(), AssociationCommandConfiguration::Create );
	cc.Register( Basic::StaticGetCommandClassId(), Basic::StaticGetCommandClassName(), Basic::Create );
	cc.Register( BasicWindowCovering::StaticGetCommandClassId(), BasicWindowCovering::StaticGetCommandClassName(), BasicWindowCovering::Create );
	cc.Register( Battery::StaticGetCommandClassId(), Battery::StaticGetCommandClassName(), Battery::Create );
	cc.Register( ClimateControlSchedule::StaticGetCommandClassId(), ClimateControlSchedule::StaticGetCommandClassName(), ClimateControlSchedule::Create );
	cc.Register( Clock::StaticGetCommandClassId(), Clock::StaticGetCommandClassName(), Clock::Create );
	cc.Register( Configuration::StaticGetCommandClassId(), Configuration::StaticGetCommandClassName(), Configuration::Create );
	cc.Register( ControllerReplication::StaticGetCommandClassId(), ControllerReplication::StaticGetCommandClassName(), ControllerReplication::Create );
	cc.Register( CRC16Encap::StaticGetCommandClassId(), CRC16Encap::StaticGetCommandClassName(), CRC16Encap::Create );
	cc.Register( DoorLock::StaticGetCommandClassId(), DoorLock::StaticGetCommandClassName(), DoorLock::Create );
	cc.Register( DoorLockLogging::StaticGetCommandClassId(), DoorLockLogging::StaticGetCommandClassName(), DoorLockLogging::Create);
	cc.Register( EnergyProduction::StaticGetCommandClassId(), EnergyProduction::StaticGetCommandClassName(), EnergyProduction::Create );
	cc.Register( Hail::StaticGetCommandClassId(), Hail::StaticGetCommandClassName(), Hail::Create );
	cc.Register( Indicator::StaticGetCommandClassId(), Indicator::StaticGetCommandClassName(), Indicator::Create );
	cc.Register( Language::StaticGetCommandClassId(), Language::StaticGetCommandClassName(), Language::Create );
	cc.Register( Lock::StaticGetCommandClassId(), Lock::StaticGetCommandClassName(), Lock::Create );
	cc.Register( ManufacturerSpecific::StaticGetCommandClassId(), ManufacturerSpecific::StaticGetCommandClassName(), ManufacturerSpecific::Create );
	cc.Register( Meter::StaticGetCommandClassId(), Meter::StaticGetCommandClassName(), Meter::Create );
	cc.Register( MeterPulse::StaticGetCommandClassId(), MeterPulse::StaticGetCommandClassName(), MeterPulse::Create );
	cc.Register( MultiCmd::StaticGetCommandClassId(), MultiCmd::StaticGetCommandClassName(), MultiCmd::Create );
	cc.Register( MultiInstance::StaticGetCommandClassId(), MultiInstance::StaticGetCommandClassName(), MultiInstance::Create );
	cc.Register( MultiInstanceAssociation::StaticGetCommandClassId(), MultiInstanceAssociation::StaticGetCommandClassName(), MultiInstanceAssociation::Create );
	cc.Register( NodeNaming::StaticGetCommandClassId(), NodeNaming::StaticGetCommandClassName(), NodeNaming::Create );
	cc.Register( NoOperation::StaticGetCommandClassId(), NoOperation::StaticGetCommandClassName(), NoOperation::Create );
	cc.Register( Powerlevel::StaticGetCommandClassId(), Powerlevel::StaticGetCommandClassName(), Powerlevel::Create );
	cc.Register( Proprietary::StaticGetCommandClassId(), Proprietary::StaticGetCommandClassName(), Proprietary::Create );
	cc.Register( Protection::StaticGetCommandClassId(), Protection::StaticGetCommandClassName(), Protection::Create );
	cc.Register( SceneActivation::StaticGetCommandClassId(), SceneActivation::StaticGetCommandClassName(), SceneActivation::Create );
	cc.Register( Security::StaticGetCommandClassId(), Security::StaticGetCommandClassName(), Security::Create);
	cc.Register( SensorAlarm::StaticGetCommandClassId(), SensorAlarm::StaticGetCommandClassName(), SensorAlarm::Create );
	cc.Register( SensorBinary::StaticGetCommandClassId(), SensorBinary::StaticGetCommandClassName(), SensorBinary::Create );
	cc.Register( SensorMultilevel::StaticGetCommandClassId(), SensorMultilevel::StaticGetCommandClassName(), SensorMultilevel::Create );
	cc.Register( SwitchAll::StaticGetCommandClassId(), SwitchAll::StaticGetCommandClassName(), SwitchAll::Create );
	cc.Register( SwitchBinary::StaticGetCommandClassId(), SwitchBinary::StaticGetCommandClassName(), SwitchBinary::Create );
	cc.Register( SwitchMultilevel::StaticGetCommandClassId(), SwitchMultilevel::StaticGetCommandClassName(), SwitchMultilevel::Create );
	cc.Register( SwitchToggleBinary::StaticGetCommandClassId(), SwitchToggleBinary::StaticGetCommandClassName(), SwitchToggleBinary::Create );
	cc.Register( SwitchToggleMultilevel::StaticGetCommandClassId(), SwitchToggleMultilevel::StaticGetCommandClassName(), SwitchToggleMultilevel::Create );
	cc.Register( TimeParameters::StaticGetCommandClassId(), TimeParameters::StaticGetCommandClassName(), TimeParameters::Create);
	cc.Register( ThermostatFanMode::StaticGetCommandClassId(), ThermostatFanMode::StaticGetCommandClassName(), ThermostatFanMode::Create );
	cc.Register( ThermostatFanState::StaticGetCommandClassId(), ThermostatFanState::StaticGetCommandClassName(), ThermostatFanState::Create );
	cc.Register( ThermostatMode::StaticGetCommandClassId(), ThermostatMode::StaticGetCommandClassName(), ThermostatMode::Create );
	cc.Register( ThermostatOperatingState::StaticGetCommandClassId(), ThermostatOperatingState::StaticGetCommandClassName(), ThermostatOperatingState::Create );
	cc.Register( ThermostatSetpoint::StaticGetCommandClassId(), ThermostatSetpoint::StaticGetCommandClassName(), ThermostatSetpoint::Create );
	cc.Register( UserCode::StaticGetCommandClassId(), UserCode::StaticGetCommandClassName(), UserCode::Create );
	cc.Register( Version::StaticGetCommandClassId(), Version::StaticGetCommandClassName(), Version::Create );
	cc.Register( WakeUp::StaticGetCommandClassId(), WakeUp::StaticGetCommandClassName(), WakeUp::Create );

	// Now all the command classes have been registered, we can modify the
	// supported command classes array according to the program options.
	string str;
	Options::Get()->GetOptionAsString( "Include", &str );
	if( str != "" )
	{
		// The include list has entries, so we assume that it is a
		// complete list of what should be supported.
		// Any existing support is cleared first.
		memset( cc.m_supportedCommandClasses, 0, sizeof(uint32)*8 );
		cc.ParseCommandClassOption( str, true );
	}

	// Apply the excluded command class option
	Options::Get()->GetOptionAsString( "Exclude", &str );
	if( str != "" )
	{
		cc.ParseCommandClassOption( str, false );
	}
}

//-----------------------------------------------------------------------------
//	<CommandClasses::ParseCommandClassOption>
//	Parse a comma delimited list of included/excluded command classes
//-----------------------------------------------------------------------------
void CommandClasses::ParseCommandClassOption
(
	string const& _optionStr,
	bool const _include
)
{
	size_t pos = 0;
    size_t start = 0;
	bool parsing = true;
	while( parsing )
	{
		string ccStr;

		pos = _optionStr.find_first_of( ",", start );
		if( string::npos == pos )
		{
			ccStr = _optionStr.substr( start );
			parsing = false;
		}
		else
		{
			ccStr = _optionStr.substr( start, pos-start );
			start = pos + 1;
		}

		if( ccStr != "" )
		{
			uint8 ccIdx = GetCommandClassId( ccStr );
			if( _include )
			{
				m_supportedCommandClasses[ccIdx>>5] |= (1u<<(ccIdx&0x1f));
			}
			else
			{
				m_supportedCommandClasses[ccIdx>>5] &= ~(1u<<(ccIdx&0x1f));
			}
		}
	}
}

//-----------------------------------------------------------------------------
//	<CommandClasses::GetCommandClassId>
//	Convert a command class name (e.g COMMAND_CLASS_BASIC) into its 8-bit ID
//-----------------------------------------------------------------------------
uint8 CommandClasses::GetCommandClassId
(
	string const& _name
)
{
	string upperName = ToUpper( _name );
	map<string,uint8>::iterator it = m_namesToIDs.find( upperName );
	if( it != m_namesToIDs.end() )
	{
		return it->second;
	}

	return 0xff;
}


