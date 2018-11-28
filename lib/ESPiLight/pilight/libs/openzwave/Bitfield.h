//-----------------------------------------------------------------------------
//
//	Bitfield.h
//
//	Variable length bitfield implementation
//
//	Copyright (c) 2011 Mal Lansell <openzwave@lansell.org>
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

#ifndef _Bitfield_H
#define _Bitfield_H

#include <vector>
#include "Defs.h"

namespace OpenZWave
{
	class OPENZWAVE_EXPORT Bitfield
	{
		friend class Iterator;

	public:
		Bitfield():m_numSetBits(0){}
		~Bitfield(){}

		void Set( uint32 _idx )
		{
			if( !IsSet(_idx) )
			{
				uint32 newSize = (_idx>>5)+1;
				if( newSize > m_bits.size() )
				{
					m_bits.resize( newSize, 0 );
				}
				m_bits[_idx>>5] |= (1<<(_idx&0x1f));
				++m_numSetBits;
			}
		}

		void Clear( uint32 _idx )
		{
			if( IsSet(_idx) )
			{
				m_bits[_idx>>5] &= ~(1<<(_idx&0x1f));
				--m_numSetBits;
			}
		}

		bool IsSet( uint32 _idx )const
		{
			if( (_idx>>5) < m_bits.size() )
			{
				return( ( m_bits[_idx>>5] & (1<<(_idx&0x1f)) ) !=0 );
			}
			return false;
		}

		uint32 GetNumSetBits()const{ return m_numSetBits; }

		class Iterator
		{
			friend class Bitfield;

		public:
			uint32 operator *() const
			{
				return m_idx;
			}

			Iterator& operator++()
			{
				// Search forward to next set bit
				NextSetBit();
			  	return *this;
			}

			Iterator operator++(int)
			{
				Iterator tmp = *this;
				++tmp;
				return tmp;
			}

			bool operator ==(const Iterator &rhs)
			{
				return m_idx == rhs.m_idx;
			}

			bool operator !=(const Iterator &rhs)
			{
				return m_idx != rhs.m_idx;
			}

		private:
			Iterator( Bitfield const* _bitfield, uint32 _idx ): m_idx( _idx ), m_bitfield( _bitfield )
			{
				// If this is a begin iterator, move it to the first set bit
				if( ( _idx == 0 ) && ( !m_bitfield->IsSet(0) ) )
				{
					NextSetBit();
				}
			}

			void NextSetBit()
			{
				while( ((++m_idx)>>5)<m_bitfield->m_bits.size() )
				{
					// See if there are any bits left to find in the current uint32
					if( ( m_bitfield->m_bits[m_idx>>5] & ~((1<<(m_idx&0x1f))-1) ) == 0 )
					{
						// No more bits - move on to next uint32 (or rather one less than
						// the next uint32 because of the preincrement in the while statement)
						m_idx = (m_idx&0xffffffe0)+31;
					}
					else
					{
						if( (m_bitfield->m_bits[m_idx>>5] & (1<<(m_idx&0x1f))) !=0 )
						{
							// This bit is set
							return;
						}
					}
				}
			}

			uint32				m_idx;
			Bitfield const*		m_bitfield;
		};

		Iterator Begin()const{ return Iterator( this, 0 ); }
		Iterator End()const{ return Iterator( this, (uint32) m_bits.size()<<5 ); }

	private:
OPENZWAVE_EXPORT_WARNINGS_OFF
		vector<uint32>	m_bits;
OPENZWAVE_EXPORT_WARNINGS_ON
		uint32			m_numSetBits;
	};
} // namespace OpenZWave

#endif



