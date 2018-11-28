//-----------------------------------------------------------------------------
//
//	SwitchAll.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SWITCH_ALL
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
#include "SwitchAll.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueList.h"

using namespace OpenZWave;

enum SwitchAllCmd
{
	SwitchAllCmd_Set	= 0x01,
	SwitchAllCmd_Get	= 0x02,
	SwitchAllCmd_Report	= 0x03,
	SwitchAllCmd_On		= 0x04,
	SwitchAllCmd_Off	= 0x05
};

static char const* c_switchAllStateName[] =
{
	"Disabled",
	"Off Enabled",
	"On Enabled",
	"On and Off Enabled"
};

//-----------------------------------------------------------------------------
// <SwitchAll::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool SwitchAll::RequestState
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
// <SwitchAll::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool SwitchAll::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _dummy1,	// = 0 (not used)
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "SwitchAllCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchAllCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "SwitchAllCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <SwitchAll::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool SwitchAll::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if (SwitchAllCmd_Report == (SwitchAllCmd)_data[0])
	{
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, 0 ) ) )
		{
			value->OnValueRefreshed( (int32)_data[1] );
			value->Release();
			Log::Write( LogLevel_Info, GetNodeId(), "Received SwitchAll report from node %d: %s", GetNodeId(), value->GetItem().m_label.c_str() );
		}
 		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchAll::SetValue>
// Set the device's response to SWITCH_ALL commands
//-----------------------------------------------------------------------------
bool SwitchAll::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_List == _value.GetID().GetType() )
	{
		ValueList const* value = static_cast<ValueList const*>(&_value);
		ValueList::Item const& item = value->GetItem();

		Log::Write( LogLevel_Info, GetNodeId(), "SwitchAll::Set - %s on node %d", item.m_label.c_str(), GetNodeId() );
		Msg* msg = new Msg( "SwitchAllCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( SwitchAllCmd_Set );
		msg->Append( (uint8)item.m_value );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <SwitchAll::Off>
// Send a command to switch all devices off
//-----------------------------------------------------------------------------
void SwitchAll::Off
(
	Driver* _driver,
	uint8 const _nodeId
)
{
	Log::Write( LogLevel_Info, _nodeId, "SwitchAll::Off (Node=%d)", _nodeId );
	Msg* msg = new Msg( "SwitchAllCmd_Off", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( _nodeId );
	msg->Append( 2 );
	msg->Append( StaticGetCommandClassId() );
	msg->Append( SwitchAllCmd_Off );
	msg->Append( _driver->GetTransmitOptions() );
	_driver->SendMsg( msg, Driver::MsgQueue_Send );
}

//-----------------------------------------------------------------------------
// <SwitchAll::On>
// Send a command to switch all devices on
//-----------------------------------------------------------------------------
void SwitchAll::On
(
	Driver* _driver,
	uint8 const _nodeId
)
{
	Log::Write( LogLevel_Info, _nodeId, "SwitchAll::On (Node=%d)", _nodeId );
	Msg* msg = new Msg( "SwitchAllCmd_On", _nodeId, REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	msg->Append( _nodeId );
	msg->Append( 2 );
	msg->Append( StaticGetCommandClassId() );
	msg->Append( SwitchAllCmd_On );
	msg->Append( _driver->GetTransmitOptions() );
	_driver->SendMsg( msg, Driver::MsgQueue_Send );
}

//-----------------------------------------------------------------------------
// <SwitchAll::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void SwitchAll::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		vector<ValueList::Item> items;
		for( int i=0; i<4; ++i )
		{
			ValueList::Item item;
			item.m_label = c_switchAllStateName[i];
			item.m_value = (i==3) ? 0x000000ff : i;
			items.push_back( item );
		}

		node->CreateValueList(  ValueID::ValueGenre_System, GetCommandClassId(), _instance, 0, "Switch All", "", false, false, 1, items, 0, 0 );
	}
}



