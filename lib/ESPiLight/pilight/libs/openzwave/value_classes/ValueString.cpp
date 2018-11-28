//-----------------------------------------------------------------------------
//
//	ValueStore.cpp
//
//	Represents a string value
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
#include "ValueString.h"
#include "../Msg.h"
#include "../platform/Log.h"
#include "../Manager.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueString::ValueString>
// Constructor
//-----------------------------------------------------------------------------
ValueString::ValueString
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
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_String, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
	m_value( _value ),
	m_valueCheck( "" ),
	m_newValue( "" )
{
}

//-----------------------------------------------------------------------------
// <ValueString::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueString::ReadXML
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
		Log::Write( LogLevel_Alert, "Missing default string value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
	}
}

//-----------------------------------------------------------------------------
// <ValueString::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueString::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );
	_valueElement->SetAttribute( "value", m_value.c_str() );
}

//-----------------------------------------------------------------------------
// <ValueString::Set>
// Set a new value in the device and queue a "RequestValue" to confirm it worked
//-----------------------------------------------------------------------------
bool ValueString::Set
(
	string const& _value
)
{
	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueString* tempValue = new ValueString( *this );
	tempValue->m_value = _value;

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}

//-----------------------------------------------------------------------------
// <ValueString::OnValueRefreshed>
// A value in a device has been refreshed
//-----------------------------------------------------------------------------
void ValueString::OnValueRefreshed
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
