//-----------------------------------------------------------------------------
//
//	ValueStore.h
//
//	Container for Value objects
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

#ifndef _ValueStore_H
#define _ValueStore_H

#include <map>
#include "../Defs.h"
#include "ValueID.h"

class TiXmlElement;

namespace OpenZWave
{
	class Value;

	/** \brief Container that holds all of the values associated with a given node.
	 */
	class ValueStore
	{
	public:

		typedef map<uint32,Value*>::const_iterator Iterator;

		Iterator Begin(){ return m_values.begin(); }
		Iterator End(){ return m_values.end(); }

		ValueStore(){}
		~ValueStore();

		bool AddValue( Value* _value );
		bool RemoveValue( uint32 const& _key );
		Value* GetValue( uint32 const& _key )const;

		void RemoveCommandClassValues( uint8 const _commandClassId );		// Remove all the values associated with a command class

	private:
		map<uint32,Value*>	m_values;
	};

} // namespace OpenZWave

#endif



