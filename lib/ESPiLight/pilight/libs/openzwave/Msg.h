//-----------------------------------------------------------------------------
//
//	Msg.h
//
//	Represents a Z-Wave message
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

#ifndef _Msg_H
#define _Msg_H

#include <cstdio>
#include <string>
#include <string.h>
#include "Defs.h"

namespace OpenZWave
{
	class CommandClass;

	/** \brief Message object to be passed to and from devices on the Z-Wave network.
	 */
	class OPENZWAVE_EXPORT Msg
	{
	public:
		enum MessageFlags
		{
			m_MultiChannel			= 0x01,		// Indicate MultiChannel encapsulation
			m_MultiInstance			= 0x02,		// Indicate MultiInstance encapsulation
		};

		Msg( string const& _logtext, uint8 _targetNodeId, uint8 const _msgType, uint8 const _function, bool const _bCallbackRequired, bool const _bReplyRequired = true, uint8 const _expectedReply = 0, uint8 const _expectedCommandClassId = 0 );
		~Msg(){}

		void SetInstance( CommandClass* _cc, uint8 const _instance );	// Used to enable wrapping with MultiInstance/MultiChannel during finalize.

		void Append( uint8 const _data );
		void Finalize();
		void UpdateCallbackId();

		/**
		 * \brief Identifies the Node ID of the "target" node (if any) for this function.
		 * \return Node ID of the target.
		 */
		uint8 GetTargetNodeId()const{ return m_targetNodeId; }

		/**
		 * \brief Identifies the Callback ID (if any) for this message.  Callback ID is a value (OpenZWave uses sequential IDs) that
		 * helps the application associate message responses with the original message request.
		 * \return Callback ID for this message.
		 */
		uint8 GetCallbackId()const{ return m_callbackId; }

		/**
		 * \brief Identifies the expected reply type (if any) for this message. The expected reply is a function code...one
		 * of the FUNC_ID... values defined in Defs.h.  Many Z-Wave functions generate responses with the same function code
		 * (for example, a FUNC_ID_ZW_GET_VERSION message generates a FUNC_ID_ZW_GET_VERSION response.  But other functions
		 * generate a different response. FUNC_ID_ZW_SEND_DATA triggers several responses, but ultimately, a "Get" sent with
		 * this function should result in a FUNC_ID_APPLICATION_COMMAND_HANDLER response.
		 * \return Expected reply (function code) for this message.
		 */
		uint8 GetExpectedReply()const{ return m_expectedReply; }

		/**
		 * \brief Identifies the expected Command Class ID (if any) for this message.
		 * \return Expected command class ID for this message.
		 */
		uint8 GetExpectedCommandClassId()const{ return m_expectedCommandClassId; }

		/**
		 * \brief For messages that request a Report for a specified command class, identifies the expected Instance
		 * for the variable being obtained in the report.
		 * \return Expected Instance value for this message.
		 */
		uint8 GetExpectedInstance()const{ return m_instance; }

		/**
		 * \brief For messages that request a Report for a specified command class, identifies the expected Index
		 * for the variable being obtained in the report.
		 * \return Expected Index value for this message.
		 */
//		uint8 GetExpectedIndex()const{ return m_expectedIndex; }
		/**
		 * \brief get the LogText Associated with this message
		 * \return the LogText used during the constructor
		 */
		string GetLogText()const{ return m_logText; }

		uint32 GetLength()const{ return m_length; }
		uint8* GetBuffer(){ return m_buffer; }
		string GetAsString();

		uint8 GetSendAttempts()const{ return m_sendAttempts; }
		void SetSendAttempts( uint8 _count ){ m_sendAttempts = _count; }

		uint8 GetMaxSendAttempts()const{ return m_maxSendAttempts; }
		void SetMaxSendAttempts( uint8 _count ){ if( _count < MAX_MAX_TRIES ) m_maxSendAttempts = _count; }

		bool IsWakeUpNoMoreInformationCommand()
		{
			return( m_bFinal && (m_length==11) && (m_buffer[3]==0x13) && (m_buffer[6]==0x84) && (m_buffer[7]==0x08) );
		}
		bool IsNoOperation()
		{
			return( m_bFinal && (m_length==11) && (m_buffer[3]==0x13) && (m_buffer[6]==0x00) && (m_buffer[7]==0x00) );
		}

		bool operator == ( Msg const& _other )const
		{
			if( m_bFinal && _other.m_bFinal )
			{
				// Do not include the callback Id or checksum in the comparison.
				uint8 length = m_length - (m_bCallbackRequired ? 2: 1 );
				return( !memcmp( m_buffer, _other.m_buffer, length ) );
			}

			return false;
		}
		uint8 GetSendingCommandClass() {
			if (m_buffer[3] == 0x13) {
				return m_buffer[6];
			}
			return 0;

		}

	private:
		void MultiEncap();					// Encapsulate the data inside a MultiInstance/Multicommand message

		string			m_logText;
		bool			m_bFinal;
		bool			m_bCallbackRequired;

		uint8			m_callbackId;
		uint8			m_expectedReply;
		uint8			m_expectedCommandClassId;
		uint8			m_length;
		uint8			m_buffer[256];

		uint8			m_targetNodeId;
		uint8			m_sendAttempts;
		uint8			m_maxSendAttempts;

		uint8			m_instance;
		uint8			m_endPoint;			// Endpoint to use if the message must be wrapped in a multiInstance or multiChannel command class
		uint8			m_flags;

		static uint8		s_nextCallbackId;		// counter to get a unique callback id
	};

} // namespace OpenZWave

#endif //_Msg_H

