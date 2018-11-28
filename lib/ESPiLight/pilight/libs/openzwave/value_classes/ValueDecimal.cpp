//-----------------------------------------------------------------------------
//
//	ValueDecimal.cpp
//
//	Represents a value that may have a fractional component
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
#include "ValueDecimal.h"
#include "../Msg.h"
#include "../platform/Log.h"
#include "../Manager.h"
#include <ctime>

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueDecimal::ValueDecimal>
// Constructor
//-----------------------------------------------------------------------------
ValueDecimal::ValueDecimal
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
	string const& _value,
	uint8 const _pollIntensity
):
  	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Decimal, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
	m_value( _value ),
	m_valueCheck( "" ),
	m_newValue( "" ),
	m_precision( 0 )
{
}

//-----------------------------------------------------------------------------
// <ValueDecimal::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueDecimal::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value::ReadXML( _homeId, _nodeId, _commandClassId, _valueElement );

	char const* str = _valueElement->Attribute( "value" );
	if( str )
	{
		m_value = str;
	}
	else
	{
		Log::Write( LogLevel_Info, "Missing default decimal value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
	}
}

//-----------------------------------------------------------------------------
// <ValueDecimal::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueDecimal::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );
	_valueElement->SetAttribute( "value", m_value.c_str() );
}

//-----------------------------------------------------------------------------
// <ValueDecimal::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueDecimal::Set
(
	string const& _value
)
{
	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueDecimal* tempValue = new ValueDecimal( *this );
	tempValue->m_value = _value;

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}

//-----------------------------------------------------------------------------
// <ValueDecimal::OnValueRefreshed>
// A value in a device has been refreshed
//-----------------------------------------------------------------------------
void ValueDecimal::OnValueRefreshed
(
	string const& _value
)
{
	switch( VerifyRefreshedValue( (void*) &m_value, (void*) &m_valueCheck, (void*) &_value, 1) )
	{
	case 0:		// value hasn't changed, nothing to do
		break;
	case 1:		// value has changed (not confirmed yet), save _value in m_valueCheck
		m_valueCheck = _value;
		break;
	case 2:		// value has changed (confirmed), save _value in m_value
		m_value = _value;
		break;
	case 3:		// all three values are different, so wait for next refresh to try again
		break;
	}
}
