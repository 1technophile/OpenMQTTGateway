//-----------------------------------------------------------------------------
//
//	ValueButton.h
//
//	Represents a write-only value that triggers activity in a device
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

#ifndef _ValueButton_H
#define _ValueButton_H

#include <string>
#include "../Defs.h"
#include "Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;
	class CommandClass;

	/** \brief Button value ???.
	 */
	class ValueButton: public Value
	{
	public:
		ValueButton( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, uint8 const _pollIntensity );
		ValueButton(): m_pressed( false ){}
		virtual ~ValueButton(){}

		bool PressButton();
		bool ReleaseButton();

		// From Value
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		bool IsPressed()const{ return m_pressed; }

	private:
		bool	m_pressed;
	};

} // namespace OpenZWave

#endif



