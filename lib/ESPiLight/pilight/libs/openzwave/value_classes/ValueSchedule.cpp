//-----------------------------------------------------------------------------
//
//	ValueSchedule.cpp
//
//	A one day schedule for the Climate Control Schedule command class
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

#include <sstream>
#include <limits.h>
#include "../tinyxml.h"
#include "ValueSchedule.h"
#include "../Msg.h"
#include "../platform/Log.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// <ValueSchedule::ValueSchedule>
// Constructor
//-----------------------------------------------------------------------------
ValueSchedule::ValueSchedule
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
	uint8 const _pollIntensity
):
	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Byte, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
	m_numSwitchPoints( 0 )
	
{
}

//-----------------------------------------------------------------------------
// <ValueSchedule::ValueSchedule>
// Constructor
//-----------------------------------------------------------------------------
ValueSchedule::ValueSchedule
(
):
	Value(),
	m_numSwitchPoints( 0 )
	
{
}

//-----------------------------------------------------------------------------
// <ValueSchedule::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueSchedule::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value::ReadXML( _homeId, _nodeId, _commandClassId, _valueElement );

	// Read in the switch points
	TiXmlElement const* child = _valueElement->FirstChildElement();
	while( child )
	{
		char const* str = child->Value();
		if( str )
		{
			if( !strcmp( str, "SwitchPoint" ) )
			{
				int intVal;
				
				uint8 hours = 0;
				if( TIXML_SUCCESS == child->QueryIntAttribute( "hours", &intVal ) )
				{
					hours = (uint8)intVal;
				}

				uint8 minutes = 0;
				if( TIXML_SUCCESS == child->QueryIntAttribute( "minutes", &intVal ) )
				{
					minutes = (uint8)intVal;
				}

				int8 setback = 0;
				if( TIXML_SUCCESS == child->QueryIntAttribute( "setback", &intVal ) )
				{
					setback = (int8)intVal;
				}

				SetSwitchPoint( hours, minutes, setback );
			}
		}

		child = child->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <ValueSchedule::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueSchedule::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );

	for( uint8 i=0; i<GetNumSwitchPoints(); ++i )
	{
		uint8 hours;
		uint8 minutes;
		int8 setback;
		if( GetSwitchPoint( i, &hours, &minutes, &setback ) )
		{
			char str[8];

			TiXmlElement* switchPointElement = new TiXmlElement( "SwitchPoint" );
			_valueElement->LinkEndChild( switchPointElement );

			snprintf( str, sizeof(str), "%d", hours );
			switchPointElement->SetAttribute( "hours", str );

			snprintf( str, sizeof(str), "%d", minutes );
			switchPointElement->SetAttribute( "minutes", str );

			snprintf( str, sizeof(str), "%d", setback );
			switchPointElement->SetAttribute( "setback", str );
		}
	}
}

//-----------------------------------------------------------------------------
// <ValueSchedule::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueSchedule::Set
(
)
{
	// Set the value in the device.
	// TODO this needs to be checked to make sure it works as intended
	return Value::Set();
}

//-----------------------------------------------------------------------------
// <ValueSchedule::OnValueRefreshed>
// A value in a device has been refreshed
//-----------------------------------------------------------------------------
void ValueSchedule::OnValueRefreshed
(
)
{
	// TODO:  do schedules ever report spurious values and need rechecking like other value types?
	// See, for example, ValueShort::OnValueRefreshed
	Value::OnValueChanged();
}

//-----------------------------------------------------------------------------
// <ValueSchedule::SetSwitchPoint>
// A value in a device has changed
//-----------------------------------------------------------------------------
bool ValueSchedule::SetSwitchPoint
(
	uint8 const _hours, uint8 const _minutes, int8 const _setback
)
{
	// Find where to insert this switch point.  They must be sorted by ascending time value.
	uint8 i;
	uint8 insertAt = 0;
	
	for( i=0; i<m_numSwitchPoints; ++i )
	{
		if( m_switchPoints[i].m_hours == _hours )
		{
			if( m_switchPoints[i].m_minutes == _minutes )
			{
				// There is already a switch point with this time, so we
				// just update its setback value
				m_switchPoints[i].m_setback = _setback;
				return true;
			}

			if( m_switchPoints[i].m_minutes > _minutes )
			{
				break;
			}
		}
		else if( m_switchPoints[i].m_hours > _hours )
		{
			break;
		}

		++insertAt;
	}

	if( m_numSwitchPoints >= 9 )
	{
		// The schedule is full
		return false;
	}

	// Shuffle any later switch points out of the way
	for( i=m_numSwitchPoints; i>insertAt; --i )
	{
		m_switchPoints[i].m_hours   = m_switchPoints[i-1].m_hours;
		m_switchPoints[i].m_minutes = m_switchPoints[i-1].m_minutes;
		m_switchPoints[i].m_setback = m_switchPoints[i-1].m_setback;
	}

	// Insert the new switch point
	m_switchPoints[insertAt].m_hours   = _hours;
	m_switchPoints[insertAt].m_minutes = _minutes;
	m_switchPoints[insertAt].m_setback = _setback;

	++m_numSwitchPoints;
	return true;
}

//-----------------------------------------------------------------------------
// <ValueSchedule::RemoveSwitchPoint>
// A value in a device has changed
//-----------------------------------------------------------------------------
bool ValueSchedule::RemoveSwitchPoint
(
	uint8 const _idx
)
{
	if( _idx >= m_numSwitchPoints )
	{
		// _idx is out of range
		return false;
	}

	// Shuffle any later switch points down to fill the gap
	for( uint8 i=_idx; i<(m_numSwitchPoints-1); ++i )
	{
		m_switchPoints[i].m_hours   = m_switchPoints[i+1].m_hours;
		m_switchPoints[i].m_minutes = m_switchPoints[i+1].m_minutes;
		m_switchPoints[i].m_setback = m_switchPoints[i+1].m_setback;
	}

	--m_numSwitchPoints;
	return true;
}

//-----------------------------------------------------------------------------
// <ValueSchedule::GetSwitchPoint>
// Get the values of a switch point
//-----------------------------------------------------------------------------
bool ValueSchedule::GetSwitchPoint
( 
	uint8 const _idx,
	uint8* o_hours,
	uint8* o_minutes,
	int8* o_setback
)const
{
	if( _idx >= m_numSwitchPoints )
	{
		// _idx is out of range
		return false;
	}

	if( o_hours )
	{
		*o_hours   = m_switchPoints[_idx].m_hours;
	}

	if( o_minutes )
	{
		*o_minutes = m_switchPoints[_idx].m_minutes;
	}

	if( o_setback )
	{
		*o_setback = m_switchPoints[_idx].m_setback;
	}

	return true;
}

//-----------------------------------------------------------------------------
// <ValueSchedule::FindSwitchPoint>
// Get the index of the switch point at the specified time
//-----------------------------------------------------------------------------
bool ValueSchedule::FindSwitchPoint
(
	uint8 const _hours,
	uint8 const _minutes,
	uint8* o_idx
)const
{
	for( uint8 i=0; i<m_numSwitchPoints; ++i )
	{
		if( m_switchPoints[i].m_hours == _hours )
		{
			if( m_switchPoints[i].m_minutes == _minutes )
			{
				// Found a match
				if( o_idx )
				{
					*o_idx = i;
				}
				return true;
			}

			if( m_switchPoints[i].m_minutes > _minutes )
			{
				// Gone past any possible match
				return false;
			}
		}
		else if( m_switchPoints[i].m_hours > _hours )
		{
			// Gone past any possible match
			return false;
		}
	}

	// No match found
	return false;
}

