//-----------------------------------------------------------------------------
//
//	Value.cpp
//
//	Base class for all OpenZWave Value Classes
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

#include "../tinyxml.h"
#include "../Manager.h"
#include "../Driver.h"
#include "../Node.h"
#include "../Notification.h"
#include "../Msg.h"
#include "Value.h"
#include "../platform/Log.h"
#include "../command_classes/CommandClass.h"
#include <ctime>
#include "../Options.h"

using namespace OpenZWave;

static char const* c_genreName[] =
{
	"basic",
	"user",
	"config",
	"system",
	"invalid"
};

static char const* c_typeName[] =
{
	"bool",
	"byte",
	"decimal",
	"int",
	"list",
	"schedule",
	"short",
	"string",
	"button",
	"raw"
};

//-----------------------------------------------------------------------------
// <Value::Value>
// Constructor
//-----------------------------------------------------------------------------
Value::Value
(
	uint32 const _homeId,
	uint8 const _nodeId,
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	ValueID::ValueType const _type,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	bool const _isSet,
	uint8 const _pollIntensity
):
	m_min( 0 ),
	m_max( 0 ),
	m_refreshTime(0),
	m_verifyChanges( false ),
	m_id( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, _type ),
	m_label( _label ),
	m_units( _units ),
	m_help( "" ),
	m_readOnly( _readOnly ),
	m_writeOnly( _writeOnly ),
	m_isSet( _isSet ),
	m_affectsLength( 0 ),
	m_affects(),
	m_affectsAll( false ),
	m_checkChange( false ),
	m_pollIntensity( _pollIntensity )
{
}

//-----------------------------------------------------------------------------
// <Value::Value>
// Constructor (from XML)
//-----------------------------------------------------------------------------
Value::Value
(
):
	m_min( 0 ),
	m_max( 0 ),
	m_refreshTime(0),
	m_verifyChanges( false ),
	m_readOnly( false ),
	m_writeOnly( false ),
	m_isSet( false ),
	m_affectsLength( 0 ),
	m_affects(),
	m_affectsAll( false ),
	m_checkChange( false ),
	m_pollIntensity( 0 )
{
}

//-----------------------------------------------------------------------------
// <Value::~Value>
// Destructor
//-----------------------------------------------------------------------------
Value::~Value
(
)
{
	if( m_affectsLength > 0 )
	{
		delete [] m_affects;
	}
}

//-----------------------------------------------------------------------------
// <Value::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void Value::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	int intVal;

	ValueID::ValueGenre genre = Value::GetGenreEnumFromName( _valueElement->Attribute( "genre" ) );
	ValueID::ValueType type = Value::GetTypeEnumFromName( _valueElement->Attribute( "type" ) );

	uint8 instance = 1;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "instance", &intVal ) )
	{
		instance = (uint8)intVal;
	}

	uint8 index = 0;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "index", &intVal ) )
	{
		index = (uint8)intVal;
	}

	m_id = ValueID( _homeId, _nodeId, genre, _commandClassId, instance, index, type );

	char const* label = _valueElement->Attribute( "label" );
	if( label )
	{
		m_label = label;
	}

	char const* units = _valueElement->Attribute( "units" );
	if( units )
	{
		m_units = units;
	}

	char const* readOnly = _valueElement->Attribute( "read_only" );
	if( readOnly )
	{
		m_readOnly = !strcmp( readOnly, "true" );
	}

	char const* writeOnly = _valueElement->Attribute( "write_only" );
	if( writeOnly )
	{
		m_writeOnly = !strcmp( writeOnly, "true" );
	}

	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "poll_intensity", &intVal ) )
	{
		m_pollIntensity = (uint8)intVal;
	}

	char const* affects = _valueElement->Attribute( "affects" );
	if( affects )
	{
		if ( m_affectsLength != 0 )
		{
			delete [] m_affects;
		}
		m_affectsLength = 0;
		if( !strcmp( affects, "all" ) )
		{
			m_affectsAll = true;
		}
		else
		{
			size_t len = strlen( affects );
			if( len > 0 )
			{
				for( size_t i = 0; i < len; i++ )
				{
					if( affects[i] == ',' )
					{
						m_affectsLength++;
					}
					else if(affects[i] < '0' || affects[i] > '9')
					{
						Log::Write( LogLevel_Info, "Improperly formatted affects data: \"%s\"", affects);
						break;
					}
				}
				m_affectsLength++;
				m_affects = new uint8[m_affectsLength];
				unsigned int j = 0;
				for( int i = 0; i < m_affectsLength; i++ )
				{
					m_affects[i] = atoi( &affects[j] );
					while( j < len && affects[j] != ',' )
					{
						j++;
					}
					j++;
				}
			}
		}
	}

	char const* verifyChanges = _valueElement->Attribute( "verify_changes" );
	if( verifyChanges )
	{
		m_verifyChanges = !strcmp( verifyChanges, "true" );
	}

	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "min", &intVal ) )
	{
		m_min = intVal;
	}

	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "max", &intVal ) )
	{
		m_max = intVal;
	}

	TiXmlElement const* helpElement = _valueElement->FirstChildElement();
	while( helpElement )
	{
		char const* str = helpElement->Value();
		if( str && !strcmp( str, "Help" ) )
		{
			str = helpElement->GetText();
			if( str )
			{
				m_help = str;
			}
			break;
		}

		helpElement = helpElement->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <Value::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void Value::WriteXML
(
	TiXmlElement* _valueElement
)
{
	char str[16];

	_valueElement->SetAttribute( "type", GetTypeNameFromEnum(m_id.GetType()) );
	_valueElement->SetAttribute( "genre", GetGenreNameFromEnum(m_id.GetGenre()) );

	snprintf( str, sizeof(str), "%d", m_id.GetInstance() );
	_valueElement->SetAttribute( "instance", str );

	snprintf( str, sizeof(str), "%d", m_id.GetIndex() );
	_valueElement->SetAttribute( "index", str );

	_valueElement->SetAttribute( "label", m_label.c_str() );
	_valueElement->SetAttribute( "units", m_units.c_str() );
	_valueElement->SetAttribute( "read_only", m_readOnly ? "true" : "false" );
	_valueElement->SetAttribute( "write_only", m_writeOnly ? "true" : "false" );
	_valueElement->SetAttribute( "verify_changes", m_verifyChanges ? "true" : "false" );

	snprintf( str, sizeof(str), "%d", m_pollIntensity );
	_valueElement->SetAttribute( "poll_intensity", str );

	snprintf( str, sizeof(str), "%d", m_min );
	_valueElement->SetAttribute( "min", str );

	snprintf( str, sizeof(str), "%d", m_max );
	_valueElement->SetAttribute( "max", str );

	if( m_affectsAll )
	{
		_valueElement->SetAttribute( "affects", "all" );
	}
	else if( m_affectsLength > 0 )
	{
		string s;
		for( int i = 0; i < m_affectsLength; i++ )
		{
			snprintf( str, sizeof(str), "%d", m_affects[i] );
		  	s = s + str;
			if( i + 1 < m_affectsLength )
			{
				s = s + ",";
			}

		}
		_valueElement->SetAttribute( "affects", s.c_str() );
	}

	if( m_help.length() > 0 )
	{
		TiXmlElement* helpElement = new TiXmlElement( "Help" );
		_valueElement->LinkEndChild( helpElement );

		TiXmlText* textElement = new TiXmlText( m_help.c_str() );
		helpElement->LinkEndChild( textElement );
	}
}

//-----------------------------------------------------------------------------
// <Value::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool Value::Set
(
)
{
	// nothing to do if this is a read-only value (return false to indicate an error)
	if( IsReadOnly() )
	{
		return false;
	}

	// retrieve the driver, node and commandclass object for this value
	bool res = false;
	Node* node = NULL;
	if( Driver* driver = Manager::Get()->GetDriver( m_id.GetHomeId() ) )
	{
		node = driver->GetNodeUnsafe( m_id.GetNodeId() );
		if( node != NULL )
		{
			if( CommandClass* cc = node->GetCommandClass( m_id.GetCommandClassId() ) )
			{
				// flag value as set and queue a "Set Value" message for transmission to the device
				res = cc->SetValue( *this );

				if( res )
				{
					if( !IsWriteOnly() )
					{
						// queue a "RequestValue" message to update the value
						cc->RequestValue( 0, m_id.GetIndex(), m_id.GetInstance(), Driver::MsgQueue_Send );
					}
					else
					{
						// There is a "bug" here in that write only values
						// never send a notification about the value changing.
						// For sleeping devices it may not change until the
						// device wakes up at some point in the future.
						// So when is the right time to change it?
						if( m_affectsAll )
						{
							node->RequestAllConfigParams( 0 );
						}
						else if( m_affectsLength > 0 )
						{
							for( int i = 0; i < m_affectsLength; i++ )
							{
								node->RequestConfigParam( m_affects[i] );
							}
						}
					}
				}
			}
		}
	}

	return res;
}

//-----------------------------------------------------------------------------
// <Value::OnValueRefreshed>
// A value in a device has been refreshed
//-----------------------------------------------------------------------------
void Value::OnValueRefreshed
(
)
{
	if( IsWriteOnly() )
	{
		return;
	}

	if( Driver* driver = Manager::Get()->GetDriver( m_id.GetHomeId() ) )
	{
		m_isSet = true;

		bool bSuppress;
		Options::Get()->GetOptionAsBool( "SuppressValueRefresh", &bSuppress );
		if( !bSuppress )
		{
			// Notify the watchers
			Notification* notification = new Notification( Notification::Type_ValueRefreshed );
			notification->SetValueId( m_id );
			driver->QueueNotification( notification );
		}
	}
}


//-----------------------------------------------------------------------------
// <Value::OnValueChanged>
// A value in a device has changed
//-----------------------------------------------------------------------------
void Value::OnValueChanged
(
)
{
	if( IsWriteOnly() )
	{
		return;
	}

	if( Driver* driver = Manager::Get()->GetDriver( m_id.GetHomeId() ) )
	{
		m_isSet = true;

		// Notify the watchers
		Notification* notification = new Notification( Notification::Type_ValueChanged );
		notification->SetValueId( m_id );
		driver->QueueNotification( notification );
	}
	/* Call Back to the Command Class that this Value has changed, so we can search the
	 * TriggerRefreshValue vector to see if we should request any other values to be
	 * refreshed.
	 */
	Node* node = NULL;
	if( Driver* driver = Manager::Get()->GetDriver( m_id.GetHomeId() ) )
	{
		node = driver->GetNodeUnsafe( m_id.GetNodeId() );
		if( node != NULL )
		{
			if( CommandClass* cc = node->GetCommandClass( m_id.GetCommandClassId() ) )
			{
				cc->CheckForRefreshValues(this);
			}
		}
	}

}

//-----------------------------------------------------------------------------
// <Value::GetGenreEnumFromName>
// Static helper to get a genre enum from a string
//-----------------------------------------------------------------------------
ValueID::ValueGenre Value::GetGenreEnumFromName
(
	char const* _name
)
{
	ValueID::ValueGenre genre = ValueID::ValueGenre_System;
	if( _name )
	{
		for( int i=0; i<(int)ValueID::ValueGenre_Count; ++i )
		{
			if( !strcmp( _name, c_genreName[i] ) )
			{
				genre = (ValueID::ValueGenre)i;
				break;
			}
		}
	}

	return genre;
}

//-----------------------------------------------------------------------------
// <Value::GetGenreNameFromEnum>
// Static helper to get a genre enum as a string
//-----------------------------------------------------------------------------
char const* Value::GetGenreNameFromEnum
(
	ValueID::ValueGenre _genre
)
{
	return c_genreName[_genre];
}

//-----------------------------------------------------------------------------
// <Value::GetTypeEnumFromName>
// Static helper to get a type enum from a string
//-----------------------------------------------------------------------------
ValueID::ValueType Value::GetTypeEnumFromName
(
	char const* _name
)
{
	ValueID::ValueType type = ValueID::ValueType_Bool;
	if( _name )
	{
		for( int i=0; i<=(int)ValueID::ValueType_Max; ++i )
		{
			if( !strcmp( _name, c_typeName[i] ) )
			{
				type = (ValueID::ValueType)i;
				break;
			}
		}
	}

	return type;
}

//-----------------------------------------------------------------------------
// <Value::GetTypeNameFromEnum>
// Static helper to get a type enum as a string
//-----------------------------------------------------------------------------
char const* Value::GetTypeNameFromEnum
(
	ValueID::ValueType _type
)
{
	return c_typeName[_type];
}

//-----------------------------------------------------------------------------
// <Value::VerifyRefreshedValue>
// Check a refreshed value
//-----------------------------------------------------------------------------
int Value::VerifyRefreshedValue
(
	void* _originalValue,
	void* _checkValue,
	void* _newValue,
	int _type,
	int _length	// = 0
)
{
	// TODO: this is pretty rough code, but it's reused by each value type.  It would be
	// better if the actions were taken (m_value = _value, etc.) in this code rather than
	// in the calling routine as a result of the return value.  In particular, it's messy
	// to be setting these values after the refesh or notification is sent.  With some
	// focus on the actual variable storage, we should be able to accomplish this with
	// memory functions.  It's really the strings that make things complicated(?).
	// if this is the first read of a value, assume it is valid (and notify as a change)
	if( !IsSet() )
	{
		Log::Write( LogLevel_Detail, m_id.GetNodeId(), "Initial read of value" );
		Value::OnValueChanged();
		return 2;		// confirmed change of value
	}
	else
	{
		switch( _type )
		{
			case 1:			// string
			{
				Log::Write( LogLevel_Detail, m_id.GetNodeId(), "Refreshed Value: old value=%s, new value=%s, type=%s", ((string*)_originalValue)->c_str(), ((string*)_newValue)->c_str(), "string" );
				break;
			}
			case 2:			// short
			{
				Log::Write( LogLevel_Detail, m_id.GetNodeId(), "Refreshed Value: old value=%d, new value=%d, type=%s", *((short*)_originalValue), *((short*)_newValue), "short");
				break;
			}
			case 3:			// int32
			{
				Log::Write( LogLevel_Detail, m_id.GetNodeId(), "Refreshed Value: old value=%d, new value=%d, type=%s", *((int32*)_originalValue), *((int32*)_newValue), "int32" );
				break;
			}
			case 4:			// uint8
			{
				Log::Write( LogLevel_Detail, m_id.GetNodeId(), "Refreshed Value: old value=%d, new value=%d, type=%s", *((uint8*)_originalValue), *((uint8*)_newValue), "uint8" );
				break;
			}
			case 5:			// bool
			{
				Log::Write( LogLevel_Detail, m_id.GetNodeId(), "Refreshed Value: old value=%s, new value=%s, type=%s", *((bool*)_originalValue)?"true":"false", *((uint8*)_newValue)?"true":"false", "bool" );
				break;
			}
			case 6:			// raw
			{
				Log::Write( LogLevel_Detail, m_id.GetNodeId(), "Refreshed Value: old value=%x, new value=%x, type=raw", _originalValue, _newValue );
				break;
			}
			default:
			{
				break;
			}
		}
	}
	m_refreshTime = time( NULL );	// update value refresh time

	// check whether changes in this value should be verified (since some devices will report values that always
	// change, where confirming changes is difficult or impossible)
	Log::Write( LogLevel_Detail, m_id.GetNodeId(), "Changes to this value are %sverified", m_verifyChanges ? "" : "not " );

	if( !m_verifyChanges )
	{
		// since we're not checking changes in this value, notify ValueChanged (to be on the safe side)
		Value::OnValueChanged();
		return 2;				// confirmed change of value
	}

	// see if the value has changed (result is used whether checking change or not)
	bool bOriginalEqual = false;
	switch( _type )
	{
	case 1:			// string
		bOriginalEqual = ( strcmp( ((string*)_originalValue)->c_str(), ((string*)_newValue)->c_str() ) == 0 );
		break;
	case 2:			// short
		bOriginalEqual = ( *((short*)_originalValue) == *((short*)_newValue) );
		break;
	case 3:			// int32
		bOriginalEqual = ( *((int32*)_originalValue) == *((int32*)_newValue) );
		break;
	case 4:			// uint8
		bOriginalEqual = ( *((uint8*)_originalValue) == *((uint8*)_newValue) );
		break;
	case 5:			// bool
		bOriginalEqual = ( *((bool*)_originalValue) == *((bool*)_newValue) );
		break;
	case 6:			// raw
		bOriginalEqual = ( memcmp( _originalValue, _newValue, _length ) == 0 );
		break;
	}

		// if this is the first refresh of the value, test to see if the value has changed
	if( !IsCheckingChange() )
	{
		if( bOriginalEqual )
		{
			// values are the same, so signal a refresh and return
			Value::OnValueRefreshed();
			return 0;			// value hasn't changed
		}

		// values are different, so flag this as a verification refresh and queue it
		Log::Write( LogLevel_Info, m_id.GetNodeId(), "Changed value (possible)--rechecking" );
		SetCheckingChange( true );
		Manager::Get()->RefreshValue( GetID() );
		return 1;				// value has changed (to be confirmed)
	}
	else		// IsCheckingChange is true if this is the second read of a potentially changed value
	{
		// if the second read is the same as the first read, the value really changed
		bool bCheckEqual = false;
		switch( _type )
		{
		case 1:			// string
			bCheckEqual = ( strcmp( ((string*)_checkValue)->c_str(), ((string*)_newValue)->c_str() ) == 0 );
			break;
		case 2:			// short
			bCheckEqual = ( *((short*)_checkValue) == *((short*)_newValue) );
			break;
		case 3:			// int32
			bCheckEqual = ( *((int32*)_checkValue) == *((int32*)_newValue) );
			break;
		case 4:			// uint8
			bCheckEqual = ( *((uint8*)_checkValue) == *((uint8*)_newValue) );
			break;
		case 5:			// bool
			bCheckEqual = ( *((bool*)_checkValue) == *((bool*)_newValue) );
			break;
		case 6:
			bCheckEqual = ( memcmp( _checkValue, _newValue, _length ) == 0 );
			break;
		}
		if( bCheckEqual )
		{
			Log::Write( LogLevel_Info, m_id.GetNodeId(), "Changed value--confirmed" );
			SetCheckingChange( false );

			// update the saved value and send notification
			Value::OnValueChanged();
			return 2;
		}

		// if the second read is the same as the original value, the first read is assumed to have been in error
		// log this situation, but don't change the value or send a ValueChanged Notification
		if( bOriginalEqual )
		{
			Log::Write( LogLevel_Info, m_id.GetNodeId(), "Spurious value change was noted." );
			SetCheckingChange( false );
			Value::OnValueRefreshed();
			return 0;
		}

		// the second read is different than both the original value and the checked value...retry
		// keep trying until we get the same value twice
		Log::Write( LogLevel_Info, m_id.GetNodeId(), "Changed value (changed again)--rechecking" );
		SetCheckingChange( true );

		// save a temporary copy of value and re-read value from device
		Manager::Get()->RefreshValue( GetID() );
		return 1;
	}
}
