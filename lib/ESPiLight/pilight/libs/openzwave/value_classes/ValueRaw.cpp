//-----------------------------------------------------------------------------
//
//	ValueRaw.cpp
//
//	Represents a collection of 8-bit values
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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
#include "ValueRaw.h"
#include "../Msg.h"
#include "../platform/Log.h"
#include "../Manager.h"
#include <ctime>

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueRaw::ValueRaw>
// Constructor
//-----------------------------------------------------------------------------
ValueRaw::ValueRaw
(
	uint32 const _homeId,
	uint8 const _nodeId,
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	uint8 const* _value,
	uint8 const _length,
	uint8 const _pollIntensity
):
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Raw, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
	m_value( NULL ),
	m_valueLength( _length ),
	m_valueCheck ( NULL ),
	m_newValue ( NULL )
{
	m_value = new uint8[_length];
	memcpy( m_value, _value, _length );
	m_min = 0;
	m_max = 0;
}

//-----------------------------------------------------------------------------
// <ValueRaw::ValueRaw>
// Constructor (from XML)
//-----------------------------------------------------------------------------
ValueRaw::ValueRaw
(
): 
        m_value( NULL ),
        m_valueCheck ( NULL ),
        m_newValue ( NULL )
{
	m_valueLength = 0;
	m_min = 0;
	m_max = 0;
}

//-----------------------------------------------------------------------------
// <ValueRaw::~ValueRaw>
// Destructor
//-----------------------------------------------------------------------------
ValueRaw::~ValueRaw
(
)
{
	delete [] m_value;
}

string const ValueRaw::GetAsString
(
) const
{
	string str = "";
	char bstr[10];

	for( uint32 i=0; i<m_valueLength; ++i ) 
	{
		if( i )
		{
			str += " ";
		}
		snprintf( bstr, sizeof(bstr), "0x%.2x", m_value[i] );
		str += bstr;
	}

	return str;
}

bool ValueRaw::SetFromString
(
	string const& _value
)
{
	char const* p = _value.c_str();
	uint8 index = 0;
	uint8* value = new uint8[m_valueLength];
	while( 1 )
	{
		char *ep = NULL;
		uint32 val = (uint32)strtol( p, &ep, 16 );
		if( p == ep || val >= 256 )
		{
			break;
		}
		if( index < m_valueLength )
		{
			value[index] = (uint8)val;
		}
		index++;
		if( ep != NULL && *ep == '\0' )
		{
			break;
		}
		p = ep + 1;
	}

	bool bRet = false;

	if ( index <= m_valueLength )
	{
		bRet = Set( value, index );
	}

	delete [] value;
	return bRet;
}

//-----------------------------------------------------------------------------
// <ValueRaw::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueRaw::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value::ReadXML( _homeId, _nodeId, _commandClassId, _valueElement );

	int intVal;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "length", &intVal ) )
	{
		m_valueLength = (uint8)intVal;
	}
	m_value = new uint8[m_valueLength];
	char const* str = _valueElement->Attribute( "value" );
	if( str )
	{
		uint8 index = 0;
		while( 1 )
		{
			char *ep = NULL;
			uint32 val = (uint32)strtol( str, &ep, 16 );
			if( str == ep || val >= 256 )
			{
				break;
			}
			if( index < m_valueLength )
			{
				m_value[index] = (uint8)val;
			}
			index++;
			if( ep != NULL && *ep == '\0' )
			{
				break;
			}
			str = ep + 1;
		}
		if( index > m_valueLength )
		{
			Log::Write( LogLevel_Info, "Data length mismatch for raw data. Got %d but expected %d.", index, m_valueLength );
		}
	}
	else
	{
		Log::Write( LogLevel_Info, "Missing default raw value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
	}
}

//-----------------------------------------------------------------------------
// <ValueRaw::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueRaw::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );

	_valueElement->SetAttribute( "value", GetAsString().c_str() );
	char str[8];
	snprintf( str, sizeof(str), "%d", GetLength() );
	_valueElement->SetAttribute( "length", str );
}

//-----------------------------------------------------------------------------
// <ValueRaw::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueRaw::Set
(
	uint8 const* _value,
	uint8 const _length
)
{
	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueRaw* tempValue = new ValueRaw( *this );
	tempValue->m_value = new uint8[_length];
	memcpy( tempValue->m_value, _value, _length );
	tempValue->m_valueLength = _length;

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}

//-----------------------------------------------------------------------------
// <ValueRaw::OnValueRefreshed>
// A value in a device has been refreshed
//-----------------------------------------------------------------------------
void ValueRaw::OnValueRefreshed
(
	uint8 const* _value,
	uint8 const _length
)
{
	switch( VerifyRefreshedValue( (void*)m_value, (void*)m_valueCheck, (void*)_value, 6, _length ) )
	{
	case 0:		// value hasn't changed, nothing to do
		break;
	case 1:		// value has changed (not confirmed yet), save _value in m_valueCheck
		if( m_valueCheck != NULL )
		{
			delete [] m_valueCheck;
		}
		m_valueCheck = new uint8[_length];
		memcpy( m_valueCheck, _value, _length );
		break;
	case 2:		// value has changed (confirmed), save _value in m_value
		if( m_value != NULL )
		{
			delete [] m_value;
		}
		m_value = new uint8[_length];
		memcpy( m_value, _value, _length );
		break;
	case 3:		// all three values are different, so wait for next refresh to try again
		break;
	}
}
