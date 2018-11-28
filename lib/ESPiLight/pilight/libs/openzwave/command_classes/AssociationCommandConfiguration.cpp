//-----------------------------------------------------------------------------
//
//	AssociationCommandConfiguration.cpp
//
//	Implementation of the COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION
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

#include "CommandClasses.h"
#include "AssociationCommandConfiguration.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Group.h"
#include "../Driver.h"
#include "../platform/Log.h"
#include "../value_classes/ValueBool.h"
#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueShort.h"

using namespace OpenZWave;

enum AssociationCommandConfigurationCmd
{
	AssociationCommandConfigurationCmd_SupportedRecordsGet		= 0x01,
	AssociationCommandConfigurationCmd_SupportedRecordsReport	= 0x02,
	AssociationCommandConfigurationCmd_Set						= 0x03,
	AssociationCommandConfigurationCmd_Get						= 0x04,
	AssociationCommandConfigurationCmd_Report					= 0x05
};

enum
{
	AssociationCommandConfigurationIndex_MaxCommandLength = 0,
	AssociationCommandConfigurationIndex_CommandsAreValues,
	AssociationCommandConfigurationIndex_CommandsAreConfigurable,
	AssociationCommandConfigurationIndex_NumFreeCommands,
	AssociationCommandConfigurationIndex_MaxCommands
};


//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool AssociationCommandConfiguration::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _requestFlags & RequestFlag_Session )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool AssociationCommandConfiguration::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}

	Msg* msg = new Msg( "AssociationCommandConfigurationCmd_SupportedRecordsGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCommandConfigurationCmd_SupportedRecordsGet );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::RequestCommands>
// Request the command data
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::RequestCommands
(
	uint8 const _groupIdx,
	uint8 const _nodeId
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "AssociationCommandConfigurationCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 4 );
		msg->Append( GetCommandClassId() );
		msg->Append( AssociationCommandConfigurationCmd_Get );
		msg->Append( _groupIdx );
		msg->Append( _nodeId );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "AssociationCommandConfigurationCmd_Get Not Supported on this node");
	}
}
//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool AssociationCommandConfiguration::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (AssociationCommandConfigurationCmd_SupportedRecordsReport == (AssociationCommandConfigurationCmd)_data[0])
	{
		uint8 maxCommandLength			=	 _data[1] >> 2;
		bool commandsAreValues			= ( (_data[1] & 0x02) != 0 );
		bool commandsAreConfigurable	= ( (_data[1] & 0x01) != 0 );
		int16 numFreeCommands			= (((int16)_data[2])<<16) | (int16)_data[3];
		int16 maxCommands				= (((int16)_data[4])<<16) | (int16)_data[5];

		Log::Write( LogLevel_Info, GetNodeId(), "Received AssociationCommandConfiguration Supported Records Report:" );
		Log::Write( LogLevel_Info, GetNodeId(), "    Maximum command length = %d bytes", maxCommandLength );
		Log::Write( LogLevel_Info, GetNodeId(), "    Maximum number of commands = %d", maxCommands );
		Log::Write( LogLevel_Info, GetNodeId(), "    Number of free commands = %d", numFreeCommands );
		Log::Write( LogLevel_Info, GetNodeId(), "    Commands are %s and are %s", commandsAreValues ? "values" : "not values", commandsAreConfigurable ? "configurable" : "not configurable" );

		ValueBool* valueBool;
		ValueByte* valueByte;
		ValueShort* valueShort;

		if( (valueByte = static_cast<ValueByte*>( GetValue( _instance, AssociationCommandConfigurationIndex_MaxCommandLength ) )) )
		{
			valueByte->OnValueRefreshed( maxCommandLength );
			valueByte->Release();
		}

		if( (valueBool = static_cast<ValueBool*>( GetValue( _instance, AssociationCommandConfigurationIndex_CommandsAreValues ) )) )
		{
			valueBool->OnValueRefreshed( commandsAreValues );
			valueBool->Release();
		}

		if( (valueBool = static_cast<ValueBool*>( GetValue( _instance, AssociationCommandConfigurationIndex_CommandsAreConfigurable ) )) )
		{
			valueBool->OnValueRefreshed( commandsAreConfigurable );
			valueBool->Release();
		}

		if( (valueShort = static_cast<ValueShort*>( GetValue( _instance, AssociationCommandConfigurationIndex_NumFreeCommands ) )) )
		{
			valueShort->OnValueRefreshed( numFreeCommands );
			valueShort->Release();
		}

		if( (valueShort = static_cast<ValueShort*>( GetValue( _instance, AssociationCommandConfigurationIndex_MaxCommands ) )) )
		{
			valueShort->OnValueRefreshed( maxCommands );
			valueShort->Release();
		}
		return true;
	}

	if (AssociationCommandConfigurationCmd_Report == (AssociationCommandConfigurationCmd)_data[0])
	{
		uint8 groupIdx		= _data[1];
		uint8 nodeIdx		= _data[2];
		bool  firstReports	= ( ( _data[3] & 0x80 ) != 0 );		// True if this is the first message containing commands for this group and node.
		uint8 numReports	= _data[3] & 0x0f;

		Log::Write( LogLevel_Info, GetNodeId(), "Received AssociationCommandConfiguration Report from:" );
		Log::Write( LogLevel_Info, GetNodeId(), "    Commands for node %d in group %d,", nodeIdx, groupIdx );

		if( Node* node = GetNodeUnsafe() )
		{
			Group* group = node->GetGroup( groupIdx );
			if( NULL == group )
			{
				if( firstReports )
				{
					// This is the first report message, so we should clear any existing command data
					group->ClearCommands( nodeIdx );
				}

				uint8 const* start = &_data[4];

				for( uint8 i=0; i<numReports; ++i )
				{
					uint8 length = start[0];
					group->AddCommand( nodeIdx, length, start+1 );
					start += length;
				}
			}
		}

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueByte (	ValueID::ValueGenre_System, GetCommandClassId(), _instance, AssociationCommandConfigurationIndex_MaxCommandLength,			"Max Command Length",			"", true, false, 0, 0 );
		node->CreateValueBool ( ValueID::ValueGenre_System, GetCommandClassId(), _instance, AssociationCommandConfigurationIndex_CommandsAreValues,			"Commands are Values",			"", true, false, false, 0 );
		node->CreateValueBool ( ValueID::ValueGenre_System, GetCommandClassId(), _instance, AssociationCommandConfigurationIndex_CommandsAreConfigurable,	"Commands are Configurable",	"", true, false, false, 0 );
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, AssociationCommandConfigurationIndex_NumFreeCommands,			"Free Commands",				"", true, false, 0, 0 );
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, AssociationCommandConfigurationIndex_MaxCommands,				"Max Commands",					"", true, false, 0, 0 );
	}
}

//-----------------------------------------------------------------------------
// <AssociationCommandConfiguration::SetCommand>
// Set a command for the association
//-----------------------------------------------------------------------------
void AssociationCommandConfiguration::SetCommand
(
	uint8 const _groupIdx,
	uint8 const _nodeId,
	uint8 const _length,
	uint8 const* _data
)
{
	Msg* msg = new Msg( "AssociationCommandConfigurationCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( _length + 5 );
	msg->Append( GetCommandClassId() );
	msg->Append( AssociationCommandConfigurationCmd_Set );
	msg->Append( _groupIdx );
	msg->Append( _nodeId );
	msg->Append( _length );

	for( uint8 i=0; i<_length; ++i )
	{
		msg->Append( _data[i] );
	}

	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
}



