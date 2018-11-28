//-----------------------------------------------------------------------------
//
//	DoorLock.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_DOOR_LOCK
//
//	Copyright (c) 2014 Justin Hammond <justin@dynam.ac>
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
#include "DoorLock.h"
#include "WakeUp.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueBool.h"
#include "../value_classes/ValueByte.h"
#include "../value_classes/ValueInt.h"

#include "../tinyxml.h"

using namespace OpenZWave;

enum DoorLockCmd
{
	DoorLockCmd_Set					= 0x01,
	DoorLockCmd_Get					= 0x02,
	DoorLockCmd_Report				= 0x03,
	DoorLockCmd_Configuration_Set	= 0x04,
	DoorLockCmd_Configuration_Get	= 0x05,
	DoorLockCmd_Configuration_Report= 0x06
};

enum TimeOutMode
{
	DoorLockConfig_NoTimeout		= 0x01,
	DoorLockConfig_Timeout			= 0x02
};

static char const* c_TimeOutModeNames[] =
{
	"No Timeout",
	"Secure Lock after Timeout"
};

enum DoorLockControlState
{
	DoorLockControlState_Handle1			= 0x01,
	DoorLockControlState_Handle2			= 0x02,
	DoorLockControlState_Handle3			= 0x04,
	DoorLockControlState_Handle4			= 0x08
};

enum ValueIDSystemIndexes
{
	Value_Lock							= 0x00,		/* Simple On/Off Mode for Lock */
	Value_Lock_Mode						= 0x01,		/* To Set more Complex Lock Modes (Such as timeouts etc) */
	Value_System_Config_Mode			= 0x02,		/* To Set/Unset if Locks should return to Secured Mode after a timeout */
	Value_System_Config_Minutes			= 0x03,		/* If Timeouts are enabled, how many minutes before a Lock "AutoLocks" */
	Value_System_Config_Seconds			= 0x04,		/* If Timeouts are enabled, how many seconds beofre a Lock "Autolocks" */
	Value_System_Config_OutsideHandles 	= 0x05,		/* What Outside Handles are controlled via Z-Wave (BitMask 1-4) */
	Value_System_Config_InsideHandles	= 0x06,		/* What inside Handles are control via ZWave (BitMask 1-4) */
};

enum DoorLockState
{
	DoorLockState_Unsecured					= 0x00,
	DoorLockState_Unsecured_Timeout 		= 0x01,
	DoorLockState_Inside_Unsecured			= 0x10,
	DoorLockState_Inside_Unsecured_Timeout	= 0x11,
	DoorLockState_Outside_Unsecured			= 0x20,
	DoorLockState_Outside_Unsecured_Timeout = 0x21,
	DoorLockState_Secured					= 0xFF
};


static char const* c_LockStateNames[] =
{
		"Unsecure",
		"Unsecured with Timeout",
		"Inside Handle Unsecured",
		"Inside Handle Unsecured with Timeout",
		"Outside Handle Unsecured",
		"Outside Handle Unsecured with Timeout",
		"Secured",
		"Invalid"
};


//-----------------------------------------------------------------------------
// <DoorLock::DoorLock>
// Constructor
//-----------------------------------------------------------------------------
DoorLock::DoorLock
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),
	m_timeoutsupported(0),
	m_insidehandlemode(0),
	m_outsidehandlemode(0),
	m_timeoutmins(0xFE),
	m_timeoutsecs(0xFE)
{
	SetStaticRequest( StaticRequest_Values );
}

//-----------------------------------------------------------------------------
// <UserCode::ReadXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void DoorLock::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	int32 intVal;

	CommandClass::ReadXML( _ccElement );
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "m_timeoutsupported", &intVal ) )
	{
		m_timeoutsupported = intVal;
	}
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "m_insidehandlemode", &intVal ) )
	{
		m_insidehandlemode = intVal;
	}
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "m_outsidehandlemode", &intVal ) )
	{
		m_outsidehandlemode = intVal;
	}
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "m_timeoutmins", &intVal ) )
	{
		m_timeoutmins = intVal;
	}
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "m_timeoutsecs", &intVal ) )
	{
		m_timeoutsecs = intVal;
	}
}

//-----------------------------------------------------------------------------
// <UserCode::WriteXML>
// Class specific configuration
//-----------------------------------------------------------------------------
void DoorLock::WriteXML
(
	TiXmlElement* _ccElement
)
{
	char str[32];

	CommandClass::WriteXML( _ccElement );
	snprintf( str, sizeof(str), "%d", m_timeoutsupported );
	_ccElement->SetAttribute( "m_timeoutsupported", str);

	snprintf( str, sizeof(str), "%d", m_insidehandlemode );
	_ccElement->SetAttribute( "m_insidehandlemode", str);

	snprintf( str, sizeof(str), "%d", m_outsidehandlemode );
	_ccElement->SetAttribute( "m_outsidehandlemode", str);

	snprintf( str, sizeof(str), "%d", m_timeoutmins );
	_ccElement->SetAttribute( "m_timeoutmins", str);

	snprintf( str, sizeof(str), "%d", m_timeoutsecs );
	_ccElement->SetAttribute( "m_timeoutsecs", str);

}



//-----------------------------------------------------------------------------
// <DoorLock::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool DoorLock::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		requests = RequestValue( _requestFlags, Value_System_Config_Mode, _instance, _queue );
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		requests |= RequestValue( _requestFlags, Value_Lock, _instance, _queue );
	}

	return requests;
}




//-----------------------------------------------------------------------------
// <DoorLock::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool DoorLock::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _what,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if ( _what >= Value_System_Config_Mode) {
		Msg* msg = new Msg( "DoorLockCmd_Configuration_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _instance );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( DoorLockCmd_Configuration_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;

	} else if ((_what == Value_Lock) || (_what == Value_Lock_Mode)) {

		if ( IsGetSupported() )
		{
			Msg* msg = new Msg( "DoorLockCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( DoorLockCmd_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		} else {
			Log::Write(  LogLevel_Info, GetNodeId(), "DoorLockCmd_Get Not Supported on this node");
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// <DoorLock::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool DoorLock::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{

	if( DoorLockCmd_Report == (DoorLockCmd)_data[0] )
	{
		uint8 lockState = (_data[1] == 0xFF) ? 6 : _data[1];
		if (lockState > 6) /* size of c_LockStateNames minus Invalid Entry */
		{
			Log::Write (LogLevel_Warning, GetNodeId(), "LockState Value was greater than range. Setting to Invalid");
			lockState = 7;
		}

		Log::Write( LogLevel_Info, GetNodeId(), "Received DoorLock report: DoorLock is %s", c_LockStateNames[lockState] );

		if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, Value_Lock ) ) )
		{
			value->OnValueRefreshed( lockState == 0xFF );
			value->Release();
		}
		if( ValueList* value = static_cast<ValueList*>( GetValue( _instance, Value_Lock_Mode ) ) )
		{
			value->OnValueRefreshed( lockState);
			value->Release();
		}
		return true;
	} else if (DoorLockCmd_Configuration_Report == (DoorLockCmd)_data[0] )
	{
		switch (_data[1]) {
			case DoorLockConfig_NoTimeout:
				m_timeoutsupported = DoorLockConfig_NoTimeout;
				RemoveValue(_instance, Value_System_Config_Minutes);
				RemoveValue(_instance, Value_System_Config_Seconds);
			  	m_timeoutmins = 0xFE;
			  	m_timeoutsecs = 0xFE;
				break;
			case DoorLockConfig_Timeout:
				/* if we have a timeout, then create the Values for the timeout config */
				if( Node* node = GetNodeUnsafe() )
				{
					node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, Value_System_Config_Minutes, "Timeout Minutes", "Mins", false, false, _data[3], 0 );
					node->CreateValueInt( ValueID::ValueGenre_System, GetCommandClassId(), _instance, Value_System_Config_Seconds, "Timeout Seconds", "Secs", false, false, _data[4], 0 );
				}
			  	m_timeoutsupported = DoorLockConfig_Timeout;
			  	m_timeoutmins = _data[3];
			  	m_timeoutsecs = _data[4];
				break;
			default:
				Log::Write(LogLevel_Warning, GetNodeId(), "Recieved a Unsupported Door Lock Config Report %d", _data[1]);
		}

		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, Value_System_Config_OutsideHandles ) ) )
		{
			value->OnValueRefreshed( ((_data[2] & 0xF0)>>4) );
			value->Release();
			m_outsidehandlemode = ((_data[2] & 0xF0)>>4);
		}
		if( ValueByte* value = static_cast<ValueByte*>( GetValue( _instance, Value_System_Config_InsideHandles ) ) )
		{
			value->OnValueRefreshed( (_data[2] & 0x0F) );
			value->Release();
			m_insidehandlemode = (_data[2] & 0x0F);
		}


		ClearStaticRequest( StaticRequest_Values );

	}
	return false;
}

//-----------------------------------------------------------------------------
// <DoorLock::SetValue>
// Set the lock's state
//-----------------------------------------------------------------------------
bool DoorLock::SetValue
(
	Value const& _value
)
{
	uint8 instance = _value.GetID().GetInstance();
	if( (Value_Lock == _value.GetID().GetIndex()) && ValueID::ValueType_Bool == _value.GetID().GetType() )
	{
		ValueBool const* value = static_cast<ValueBool const*>(&_value);

		Log::Write( LogLevel_Info, GetNodeId(), "Value_Lock::Set - Requesting lock to be %s", value->GetValue() ? "Locked" : "Unlocked" );
		Msg* msg = new Msg( "DoorLockCmd_Get",  GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( DoorLockCmd_Set );
		msg->Append( value->GetValue() ? 0xFF:0x00 );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}
	if ( (Value_Lock_Mode == _value.GetID().GetIndex()) && (ValueID::ValueType_List == _value.GetID().GetType()) )
	{
		ValueList const* value = static_cast<ValueList const*>(&_value);
		ValueList::Item const& item = value->GetItem();


		Log::Write( LogLevel_Info, GetNodeId(), "Value_Lock_Mode::Set - Requesting lock to be %s", item.m_label.c_str() );
		Msg* msg = new Msg( "DoorLockCmd_Get",  GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( DoorLockCmd_Set );
		msg->Append( item.m_value );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		return true;
	}
	/* if its any of our System Messages.... */
	if ( Value_System_Config_Mode >= _value.GetID().GetIndex() )
	{
		bool sendmsg = true;
		switch (_value.GetID().GetIndex()) {
			/* this is a List */
			case Value_System_Config_Mode:
				if (ValueID::ValueType_List != _value.GetID().GetType()) {
					sendmsg = false;
					break;
				}
				if( ValueList* value = static_cast<ValueList*>( GetValue( instance, _value.GetID().GetIndex() ) ) )
				{
					ValueList::Item const& item = (static_cast<ValueList const*>( &_value))->GetItem();
					value->OnValueRefreshed( item.m_value );
					value->Release();
				}

				break;

			/* these are a int */
			case Value_System_Config_Minutes:
			case Value_System_Config_Seconds:
				if (ValueID::ValueType_Int != _value.GetID().GetType()) {
					sendmsg = false;
					break;
				}
				if( ValueInt* value = static_cast<ValueInt*>( GetValue( instance, _value.GetID().GetIndex() ) ) )
				{
					value->OnValueRefreshed( (static_cast<ValueInt const*>( &_value))->GetValue() );
					value->Release();
				}
				break;
			/* these are a byte */
			case Value_System_Config_OutsideHandles:
			case Value_System_Config_InsideHandles:
				if (ValueID::ValueType_Byte != _value.GetID().GetType()) {
					sendmsg = false;
					break;
				}
				if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, _value.GetID().GetIndex() ) ) )
				{
					value->OnValueRefreshed( (static_cast<ValueByte const*>( &_value))->GetValue() );
					value->Release();
				}
				break;
			default:
				Log::Write(LogLevel_Warning, GetNodeId(), "DoorLock::SetValue - Unhandled System_Config Variable %d", _value.GetID().GetIndex());
				sendmsg = false;
				break;
		}

		if (sendmsg)
		{
			bool ok = true;
			if( ValueList* value = static_cast<ValueList*>( GetValue( instance, Value_System_Config_Mode ) ) ) {
				ValueList::Item const& item = value->GetItem();
				m_timeoutsupported = item.m_value;
			} else {
				ok = false;
				Log::Write(LogLevel_Warning, GetNodeId(), "Failed To Retrieve Value_System_Config_Mode For SetValue");
			}
			uint8 control = 0;
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, Value_System_Config_OutsideHandles ) ) )
			{
				control = (value->GetValue() << 4);
				m_insidehandlemode = control;
			} else {
				ok = false;
				Log::Write(LogLevel_Warning, GetNodeId(), "Failed To Retrieve Value_System_Config_OutsideHandles For SetValue");
			}
			if( ValueByte* value = static_cast<ValueByte*>( GetValue( instance, Value_System_Config_InsideHandles ) ) )
			{
				control += (value->GetValue() & 0x0F);
				m_outsidehandlemode = (value->GetValue() & 0x0F);
			} else {
				ok = false;
				Log::Write(LogLevel_Warning, GetNodeId(), "Failed To Retrieve Value_System_Config_InsideHandles For SetValue");
			}
			if( ValueInt* value = static_cast<ValueInt*>( GetValue( instance, Value_System_Config_Minutes ) ) )
			{
				m_timeoutmins = value->GetValue();
			} else {
				/* Minutes and Seconds Might Not Exist, this is fine. Set to 0xFE */
				m_timeoutmins = 0xFE;
			}
			if( ValueInt* value = static_cast<ValueInt*>( GetValue( instance, Value_System_Config_Seconds ) ) )
			{
				m_timeoutsecs = value->GetValue();
			} else {
				/* Minutes and Seconds Might Not Exist, this is fine. Set to 0xFE */
				m_timeoutsecs = 0xFE;
			}
			if (ok) {
				Msg* msg = new Msg( "DoorLockCmd_Configuration_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
				msg->SetInstance( this, _value.GetID().GetInstance() );
				msg->Append( GetNodeId() );
				msg->Append( 6 );
				msg->Append( GetCommandClassId() );
				msg->Append( DoorLockCmd_Configuration_Set );
				msg->Append( m_timeoutsupported );
				msg->Append( control );
				msg->Append( m_timeoutmins );
				msg->Append( m_timeoutsecs );
				msg->Append( GetDriver()->GetTransmitOptions() );
				GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
				return true;
			}
			return false;
		}

	}
	return false;
}

//-----------------------------------------------------------------------------
// <DoorLock::SetValueBasic>
// Update class values based in BASIC mapping
//-----------------------------------------------------------------------------
void DoorLock::SetValueBasic
(
	uint8 const _instance,
	uint8 const _value
)
{



	// Send a request for new value to synchronize it with the BASIC set/report.
	// In case the device is sleeping, we set the value anyway so the BASIC set/report
	// stays in sync with it. We must be careful mapping the uint8 BASIC value
	// into a class specific value.
	// When the device wakes up, the real requested value will be retrieved.
	RequestValue( 0, DoorLockCmd_Get, _instance, Driver::MsgQueue_Send );
	if( Node* node = GetNodeUnsafe() )
	{
		if( WakeUp* wakeUp = static_cast<WakeUp*>( node->GetCommandClass( WakeUp::StaticGetCommandClassId() ) ) )
		{
			if( !wakeUp->IsAwake() )
			{
				if( ValueBool* value = static_cast<ValueBool*>( GetValue( _instance, Value_Lock ) ) )
				{
					value->OnValueRefreshed( _value != 0 );
					value->Release();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <DoorLock::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void DoorLock::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueBool( ValueID::ValueGenre_User, GetCommandClassId(), _instance, Value_Lock, "Locked", "", false, false, false, 0 );

	  	/* Complex Lock Option */
	  	{
	  		vector<ValueList::Item> items;
	  		ValueList::Item item;
	  		for( uint8 i=0; i<8; ++i )
	  		{
	  			item.m_label = c_LockStateNames[i];
	  			item.m_value = (i < 6) ? i : 0xFF;
	  			items.push_back( item );
	  		}
	  		node->CreateValueList( ValueID::ValueGenre_User, GetCommandClassId(), _instance, Value_Lock_Mode, "Locked (Advanced)", "", false, false, 1, items, 0, 0 );


	  	}

	  	/* Timeout mode for Locks that support it */
	  	{
	  		vector<ValueList::Item> items;
	  		ValueList::Item item;
	  		for( uint8 i=0; i<2; ++i )
	  		{
	  			item.m_label = c_TimeOutModeNames[i];
	  			item.m_value = i+1;
	  			items.push_back( item );
	  		}
	  		node->CreateValueList( ValueID::ValueGenre_System, GetCommandClassId(), _instance, Value_System_Config_Mode, "Timeout Mode", "", false, false, 1, items, 0, 0 );
	  	}
  		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, Value_System_Config_OutsideHandles, "Outside Handle Control", "", false, false, 0x0F, 0 );
  		node->CreateValueByte( ValueID::ValueGenre_System, GetCommandClassId(), _instance, Value_System_Config_InsideHandles, "Inside Handle Control", "", false, false, 0x0F, 0 );

	}
}


