//-----------------------------------------------------------------------------
//
//	ValueSchedule.h
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

#ifndef _ValueSchedule_H
#define _ValueSchedule_H

#include <string>
#include "../Defs.h"
#include "Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;

	/** \brief Schedule sent to/received from a node.
	 */
	class ValueSchedule: public Value
	{
	public:
		ValueSchedule( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, uint8 const _pollIntensity );
		ValueSchedule();
		virtual ~ValueSchedule(){}

		bool SetSwitchPoint( uint8 const _hours, uint8 const _minutes, int8 const _setback );
		bool RemoveSwitchPoint( uint8 const _idx );
		void ClearSwitchPoints(){ m_numSwitchPoints = 0; }
		bool GetSwitchPoint( uint8 const _idx, uint8* o_hours, uint8* o_minutes, int8* o_setback )const;
		bool FindSwitchPoint( uint8 const _hours, uint8 const _minutes, uint8* o_idx )const;
		uint8 GetNumSwitchPoints()const{ return m_numSwitchPoints; }

		bool Set();
		void OnValueRefreshed();

		// From Value
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

	private:
		struct SwitchPoint
		{
			uint8	m_hours;
			uint8	m_minutes;
			int8	m_setback;
		};

		SwitchPoint		m_switchPoints[9];
		uint8			m_numSwitchPoints;
	};

} // namespace OpenZWave

#endif
