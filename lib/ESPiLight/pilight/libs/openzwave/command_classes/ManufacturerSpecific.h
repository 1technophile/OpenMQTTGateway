//-----------------------------------------------------------------------------
//
//	ManufacturerSpecific.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_MANUFACTURER_SPECIFIC
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

#ifndef _ManufacturerSpecific_H
#define _ManufacturerSpecific_H

#include <map>
#include "CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_MANUFACTURER_SPECIFIC (0x72), a Z-Wave device command class.
	 */
	class ManufacturerSpecific: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new ManufacturerSpecific( _homeId, _nodeId ); }
		virtual ~ManufacturerSpecific(){ UnloadProductXML(); }

		static uint8 const StaticGetCommandClassId(){ return 0x72; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_MANUFACTURER_SPECIFIC"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );

		static string SetProductDetails( Node *_node, uint16 _manufacturerId, uint16 _productType, uint16 _productId );
		static bool LoadConfigXML( Node* _node, string const& _configXML );

		void ReLoadConfigXML();

	private:
		ManufacturerSpecific( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ){ SetStaticRequest( StaticRequest_Values ); }
		static bool LoadProductXML();
		static void UnloadProductXML();

		class Product
		{
		public:
			Product
			(
				uint16 _manufacturerId,
				uint16 _productType,
				uint16 _productId,
				string const& _productName,
				string const& _configPath
			):
				m_manufacturerId( _manufacturerId ),
				m_productType( _productType ),
				m_productId( _productId ),
				m_productName( _productName ),
				m_configPath( _configPath )
			{
			}

			int64 GetKey()const
			{
				return( GetKey( m_manufacturerId, m_productType, m_productId ) );
			}

			static int64 GetKey( uint16 _manufacturerId, uint16 _productType, uint16 _productId )
			{
				int64 key = (((int64)_manufacturerId)<<32) | (((int64)_productType)<<16) | (int64)_productId;
				return key;
			}

			uint16 GetManufacturerId()const{ return m_manufacturerId; }
			uint16 GetProductType()const{ return m_productType; }
			uint16 GetProductId()const{ return m_productId; }
			string GetProductName()const{ return m_productName; }
			string GetConfigPath()const{ return m_configPath; }

		private:
			uint16	m_manufacturerId;
			uint16	m_productType;
			uint16	m_productId;
			string	m_productName;
			string	m_configPath;
		};

		static map<uint16,string>	s_manufacturerMap;
		static map<int64,Product*>	s_productMap;
		static bool					s_bXmlLoaded;
	};

} // namespace OpenZWave

#endif

