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

#include "../tinyxml.h"
#include "ValueButton.h"
#include "../Manager.h"
#include "../Driver.h"
#include "../Node.h"
#include "../platform/Log.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueButton::ValueButton>
// Constructor
//-----------------------------------------------------------------------------
ValueButton::ValueButton
(
	uint32 const _homeId,
	uint8 const _nodeId,
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint8 const _index,
	string const& _label,
	uint8 const _pollIntensity
):
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Button, _label, "", false, true, true, _pollIntensity ),
	m_pressed( false )
{
}

//-----------------------------------------------------------------------------
// <ValueButton::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueButton::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value::ReadXML( _homeId, _nodeId, _commandClassId, _valueElement );
}

//-----------------------------------------------------------------------------
// <ValueButton::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueButton::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );
}

//-----------------------------------------------------------------------------
// <ValueButton::PressButton>
// Start an activity in a device
//-----------------------------------------------------------------------------
bool ValueButton::PressButton
(
)
{
	// Set the value in the device.
	m_pressed = true;
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueButton::ReleaseButton>
// Stop an activity in a device
//-----------------------------------------------------------------------------
bool ValueButton::ReleaseButton
(
)
{
	// Set the value in the device.
	m_pressed = false;
	bool res = Value::Set();
	if( Driver* driver = Manager::Get()->GetDriver( GetID().GetHomeId() ) )
	{
		if( Node* node = driver->GetNodeUnsafe( GetID().GetNodeId() ) )
		{
			node->RequestDynamicValues();
		}
	}
	return res;
}



