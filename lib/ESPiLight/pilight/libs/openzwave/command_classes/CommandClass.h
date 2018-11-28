//-----------------------------------------------------------------------------
//
//	CommandClass.h
//
//	Base class for all Z-Wave Command Classes
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

#ifndef _CommandClass_H
#define _CommandClass_H

#include <string>
#include <vector>
#include <map>
#include "../Defs.h"
#include "../Bitfield.h"
#include "../Driver.h"

namespace OpenZWave
{
	class Msg;
	class Node;
	class Value;

	/** \brief Base class for all Z-Wave command classes.
	 */
	class OPENZWAVE_EXPORT CommandClass
	{

	public:
		enum
		{
			RequestFlag_Static		= 0x00000001,	/**< Values that never change. */
			RequestFlag_Session		= 0x00000002,	/**< Values that change infrequently, and so only need to be requested at start up, or via a manual refresh. */
			RequestFlag_Dynamic		= 0x00000004,	/**< Values that change and will be requested if polling is enabled on the node. */
		};

		CommandClass( uint32 const _homeId, uint8 const _nodeId );
		virtual ~CommandClass();

		virtual void ReadXML( TiXmlElement const* _ccElement );
		virtual void WriteXML( TiXmlElement* _ccElement );
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue ){ return false; }
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue ) { return false; }

		virtual uint8 const GetCommandClassId()const = 0;
		virtual string const GetCommandClassName()const = 0;
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) = 0;
		virtual bool SetValue( Value const& _value ){ return false; }
		virtual void SetValueBasic( uint8 const _instance, uint8 const _level ){}		// Class specific handling of BASIC value mapping
		virtual void SetVersion( uint8 const _version ){ m_version = _version; }

		bool RequestStateForAllInstances( uint32 const _requestFlags, Driver::MsgQueue const _queue );
		bool CheckForRefreshValues(Value const* _value );

		// The highest version number of the command class implemented by OpenZWave.  We only need
		// to do version gets on command classes that override this with a number greater than one.
		virtual uint8 GetMaxVersion(){ return 1; }

		uint8 GetVersion()const{ return m_version; }
		Bitfield const* GetInstances()const{ return &m_instances; }
		uint32 GetHomeId()const{ return m_homeId; }
		uint8 GetNodeId()const{ return m_nodeId; }
		Driver* GetDriver()const;
		Node* GetNodeUnsafe()const;
		Value* GetValue( uint8 const _instance, uint8 const _index );
		bool RemoveValue( uint8 const _instance, uint8 const _index );
		uint8 GetEndPoint( uint8 const _instance )
		{
			map<uint8,uint8>::iterator it = m_endPointMap.find( _instance );
			return( it == m_endPointMap.end() ? 0 : it->second );
		}
		uint8 GetInstance( uint8 const _endPoint )
		{
			for( map<uint8,uint8>::iterator it = m_endPointMap.begin(); it != m_endPointMap.end(); ++it )
			{
				if( _endPoint == it->second )
				{
					return it->first;
				}
			}
			return 0;
		}

		void SetInstances( uint8 const _instances );
		void SetInstance( uint8 const _endPoint );
		void SetAfterMark(){ m_afterMark = true; }
		void SetEndPoint( uint8 const _instance, uint8 const _endpoint){ m_endPointMap[_instance] = _endpoint; }
		bool IsAfterMark()const{ return m_afterMark; }
		bool IsCreateVars()const{ return m_createVars; }
		bool IsGetSupported()const{ return m_getSupported; }
		bool IsSecured()const{ return m_isSecured; }
		void SetSecured(){ m_isSecured = true; }
		bool IsSecureSupported()const { return m_SecureSupport; }
		void ClearSecureSupport() { m_SecureSupport = false; }
		void SetSecureSupport() { m_SecureSupport = true; }

		// Helper methods
		string ExtractValue( uint8 const* _data, uint8* _scale, uint8* _precision, uint8 _valueOffset = 1 )const;

		/**
		 *  Append a floating-point value to a message.
		 *  \param _msg The message to which the value should be appended.
		 *  \param _value A string containing a decimal number to be appended.
		 *  \param _scale A byte indicating the scale corresponding to this value (e.g., 1=F and 0=C for temperatures).
		 *  \see Msg
		 */
		void AppendValue( Msg* _msg, string const& _value, uint8 const _scale )const;
		uint8 const GetAppendValueSize( string const& _value )const;
		int32 ValueToInteger( string const& _value, uint8* o_precision, uint8* o_size )const;

		void UpdateMappedClass( uint8 const _instance, uint8 const _classId, uint8 const _value );		// Update mapped class's value from BASIC class

		typedef struct RefreshValue {
			uint8 cc;
			uint8 genre;
			uint8 instance;
			uint8 index;
			std::vector<RefreshValue *> RefreshClasses;
		} RefreshValue;

	protected:
		virtual void CreateVars( uint8 const _instance ){}
		void ReadValueRefreshXML ( TiXmlElement const* _ccElement );

	public:
		virtual void CreateVars( uint8 const _instance, uint8 const _index ){}

	private:
		uint32		m_homeId;
		uint8		m_nodeId;
		uint8		m_version;
		Bitfield	m_instances;
OPENZWAVE_EXPORT_WARNINGS_OFF
		map<uint8,uint8> m_endPointMap;
OPENZWAVE_EXPORT_WARNINGS_ON
		bool		m_afterMark;		// Set to true if the command class is listed after COMMAND_CLASS_MARK, and should not create any values.
		bool		m_createVars;		// Do we want to create variables
		int8		m_overridePrecision;	// Override precision when writing values if >=0
		bool		m_getSupported;	    	// Get operation supported
		bool		m_isSecured;		// is this command class secured with the Security Command Class
		bool		m_SecureSupport; 	// Does this commandclass support secure encryption (eg, the Security CC doesn't encrypt itself, so it doesn't support encryption)
		std::vector<RefreshValue *> m_RefreshClassValues; // what Command Class Values should we refresh ?

	//-----------------------------------------------------------------------------
	// Record which items of static data have been read from the device
	//-----------------------------------------------------------------------------
	public:
		enum StaticRequest
		{
			StaticRequest_Instances		= 0x01,
			StaticRequest_Values		= 0x02,
			StaticRequest_Version		= 0x04
		};

		bool HasStaticRequest( uint8 _request )const{ return( (m_staticRequests & _request) != 0 ); }
		void SetStaticRequest( uint8 _request ){ m_staticRequests |= _request; }
		void ClearStaticRequest( uint8 _request );

	private:
		uint8   m_staticRequests;

	//-----------------------------------------------------------------------------
	//	Statistics
	//-----------------------------------------------------------------------------
	public:
		uint32 GetSentCnt()const{ return m_sentCnt; }
		uint32 GetReceivedCnt()const{ return m_receivedCnt; }
		void SentCntIncr(){ m_sentCnt++; }
		void ReceivedCntIncr(){ m_receivedCnt++; }

	private:
		uint32 m_sentCnt;				// Number of messages sent from this command class.
		uint32 m_receivedCnt;				// Number of messages received from this commandclass.
	};

} // namespace OpenZWave

#endif
