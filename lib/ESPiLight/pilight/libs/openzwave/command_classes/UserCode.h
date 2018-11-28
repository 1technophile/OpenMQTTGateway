//-----------------------------------------------------------------------------
//
//	UserCode.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_USER_CODE
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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

#ifndef _UserCode_H
#define _UserCode_H

#include "CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_USER_CODE (0x63), a Z-Wave device command class.
	 */
	class UserCode: public CommandClass
	{
	public:
		enum UserCodeStatus
		{
			UserCode_Available		= 0x00,
			UserCode_Occupied		= 0x01,
			UserCode_Reserved		= 0x02,
			UserCode_NotAvailable		= 0xfe,
			UserCode_Unset			= 0xff
		};

		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new UserCode( _homeId, _nodeId ); }
		virtual ~UserCode(){}

		static uint8 const StaticGetCommandClassId(){ return 0x63; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_USER_CODE"; }

		// From CommandClass
		virtual void ReadXML( TiXmlElement const* _ccElement );
		virtual void WriteXML( TiXmlElement* _ccElement );
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		virtual bool SetValue( Value const& _value );

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		UserCode( uint32 const _homeId, uint8 const _nodeId );

		string CodeStatus( uint8 const _byte )
		{
			switch( _byte )
			{
				case UserCode_Available:
				{
					return "Available";
				}
				case UserCode_Occupied:
				{
					return "Occupied";
				}
				case UserCode_Reserved:
				{
					return "Reserved";
				}
				case UserCode_NotAvailable:
				{
					return "Not Available";
				}
				case UserCode_Unset:
				{
					return "Unset";
				}
				default:
				{
					return "Unknown";
				}
			}
		}

		bool		m_queryAll;				// True while we are requesting all the user codes.
		uint8		m_currentCode;
		uint8		m_userCodeCount;
		uint8		m_userCodesStatus[256];
		bool		m_refreshUserCodes;
	};

} // namespace OpenZWave

#endif


