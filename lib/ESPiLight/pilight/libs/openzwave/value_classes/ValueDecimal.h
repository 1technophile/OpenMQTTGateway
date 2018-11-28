//-----------------------------------------------------------------------------
//
//	ValueDecimal.h
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

#ifndef _ValueDecimal_H
#define _ValueDecimal_H

#include <string>
#include "../Defs.h"
#include "Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;

	/** \brief Decimal value sent to/received from a node.
	 */
	class ValueDecimal: public Value
	{
		friend class EnergyProduction;
		friend class Meter;
		friend class SensorMultilevel;
		friend class ThermostatSetpoint;

	public:
		ValueDecimal( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, string const& _value, uint8 const _pollIntensity );
		ValueDecimal(): m_precision( 0 ){}
		virtual ~ValueDecimal(){}

		bool Set( string const& _value );
		void OnValueRefreshed( string const& _value );

		// From Value
		virtual string const GetAsString() const { return GetValue(); }
		virtual bool SetFromString( string const& _value ) { return Set( _value ); }
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		string GetValue()const{ return m_value; }
		uint8 GetPrecision()const{ return m_precision; }

	private:
		void SetPrecision( uint8 _precision ){ m_precision = _precision; }

		string	m_value;				// the current value
		string	m_valueCheck;			// the previous value (used for double-checking spurious value reads)
		string	m_newValue;				// a new value to be set on the appropriate device
	        uint8	m_precision;
	};

} // namespace OpenZWave

#endif



