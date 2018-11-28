//-----------------------------------------------------------------------------
//
//	BasicWindowCovering.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_BASIC_WINDOW_COVERING
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
#include "BasicWindowCovering.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Driver.h"
#include "../Node.h"
#include "../platform/Log.h"
#include "../value_classes/ValueButton.h"

using namespace OpenZWave;

enum BasicWindowCoveringCmd
{
	BasicWindowCoveringCmd_StartLevelChange	= 0x01,
	BasicWindowCoveringCmd_StopLevelChange	= 0x02
};

enum
{
	BasicWindowCoveringIndex_Open = 0,
	BasicWindowCoveringIndex_Close
};

//-----------------------------------------------------------------------------
// <BasicWindowCovering::SetValue>
// Set a value on the Z-Wave device
//-----------------------------------------------------------------------------
bool BasicWindowCovering::SetValue
(
	Value const& _value
)
{
	if( ValueID::ValueType_Button == _value.GetID().GetType() )
	{
		ValueButton const* button = static_cast<ValueButton const*>(&_value);

		uint8 action = 0x40;
		if( button->GetID().GetIndex() )	// Open is index zero, so non-zero is close.
		{
			// Close
			action = 0;
		}

		if( button && button->IsPressed() )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "BasicWindowCovering - Start Level Change (%s)", action ? "Open" : "Close" );
			Msg* msg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
			msg->SetInstance( this, _value.GetID().GetInstance() );
			msg->Append( GetNodeId() );
			msg->Append( 3 );
			msg->Append( GetCommandClassId() );
			msg->Append( BasicWindowCoveringCmd_StartLevelChange );
			msg->Append( action );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
			return true;
		}
		else
		{
			Log::Write( LogLevel_Info, GetNodeId(), "BasicWindowCovering - Stop Level Change" );
			Msg* msg = new Msg( "Basic Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );		
			msg->SetInstance( this, _value.GetID().GetInstance() );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( BasicWindowCoveringCmd_StopLevelChange );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// <BasicWindowCovering::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void BasicWindowCovering::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, BasicWindowCoveringIndex_Open, "Open", 0 );
		node->CreateValueButton( ValueID::ValueGenre_User, GetCommandClassId(), _instance, BasicWindowCoveringIndex_Close, "Close", 0 );
	}
}

