//-----------------------------------------------------------------------------
//
//	Language.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_LANGUAGE
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
#include "Language.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueString.h"

using namespace OpenZWave;

enum LanguageCmd
{
	LanguageCmd_Set		= 0x01,
	LanguageCmd_Get		= 0x02,
	LanguageCmd_Report	= 0x03
};

enum
{
	LanguageIndex_Language	= 0,
	LanguageIndex_Country
};

//-----------------------------------------------------------------------------
// <Language::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Language::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Language::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Language::RequestValue
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
	if ( IsGetSupported() )
	{
		Msg* msg = new Msg( "LanguageCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( LanguageCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "LanguageCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Language::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Language::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( LanguageCmd_Report == (LanguageCmd)_data[0] )
	{
		char language[4];
		char country[3];

		language[0] = _data[1];
		language[1] = _data[2];
		language[2] = _data[3];
		language[3] = 0;

		country[0] = _data[4];
		country[1] = _data[5];
		country[2] = 0;

		Log::Write( LogLevel_Info, GetNodeId(), "Received Language report: Language=%s, Country=%s", language, country );
		ClearStaticRequest( StaticRequest_Values );

		if( ValueString* languageValue = static_cast<ValueString*>( GetValue( _instance, LanguageIndex_Language ) ) )
		{
			languageValue->OnValueRefreshed( language );
			languageValue->Release();
		}
		if( ValueString* countryValue = static_cast<ValueString*>( GetValue( _instance, LanguageIndex_Country ) ) )
		{
			countryValue->OnValueRefreshed( country );
			countryValue->Release();
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Language::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Language::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueString( ValueID::ValueGenre_System, GetCommandClassId(), _instance, (uint8)LanguageIndex_Language, "Language", "", false, false, "", 0 );
		node->CreateValueString( ValueID::ValueGenre_System, GetCommandClassId(), _instance, (uint8)LanguageIndex_Country, "Country", "", false, false, "", 0 );
	}
}

