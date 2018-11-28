//-----------------------------------------------------------------------------
//
//	ValueRaw.h
//
//	Represents a collection of 8-bit values
//
//	Copyright (c) 2013 Greg Satz <satz@iranger.com>
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

#ifndef _ValueRaw_H
#define _ValueRaw_H

#include <string>
#include "../Defs.h"
#include "Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;

	/** \brief A collection of bytes sent to/received from a node.
	 */
	class ValueRaw: public Value
	{
	public:
		ValueRaw( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, uint8 const* _value, uint8 const _length, uint8 const _pollIntensity );
		ValueRaw();
		virtual ~ValueRaw();

		bool Set( uint8 const* _value, uint8 const _length );
		void OnValueRefreshed( uint8 const* _value, uint8 const _length );

		// From Value
		virtual string const GetAsString() const;
		virtual bool SetFromString( string const& _value );
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		uint8* GetValue()const{ return m_value; }
		uint8 GetLength()const{ return m_valueLength; }

	private:
		uint8*	m_value;				// the current value
		uint8	m_valueLength;				// fixed length for this instance
		uint8*	m_valueCheck;				// the previous value (used for double-checking spurious value reads)
		uint8*	m_newValue;				// a new value to be set on the appropriate device
	};

} // namespace OpenZWave

#endif



