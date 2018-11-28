//-----------------------------------------------------------------------------
//
//	Security.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_Security
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

#ifndef _Security_H
#define _Security_H

#include <ctime>
#include "../aes/aescpp.h"
#include "CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_SECURITY (0x98), a Z-Wave device command class.
	 */

	typedef struct SecurityPayload {
		uint8 m_length;
		uint8 m_part;
		uint8 m_data[32];
		string logmsg;
	} SecurityPayload;

	/* This should probably go into its own file, but its so simple... and only the Security Command Class uses it currently
	 */

	class Timer {
	public:
		Timer() {
			this->Reset();
		};
		virtual ~Timer() {};
		void Reset() {
			start = clock();
		}
		inline uint64 GetMilliseconds() {
            return (uint64 )(((clock() - start) / (double)CLOCKS_PER_SEC) / 1000);
		}
	private:
		clock_t start;
	};

	class Security: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new Security( _homeId, _nodeId ); }
		virtual ~Security();

		static uint8 const StaticGetCommandClassId(){ return 0x98; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_SECURITY"; }
		bool Init();
		// From CommandClass
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		void ReadXML(TiXmlElement const* _ccElement);
		void WriteXML(TiXmlElement* _ccElement);
		void SendMsg( Msg* _msg );

	protected:
		void CreateVars( uint8 const _instance );

	private:
		Security( uint32 const _homeId, uint8 const _nodeId );
		bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue);
		bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue);
		bool HandleSupportedReport(uint8 const* _data, uint32 const _length);
		void SendNonceReport();
		void RequestNonce();
		bool GenerateAuthentication( uint8 const* _data, uint32 const _length, uint8 const _sendingNode, uint8 const _receivingNode, uint8 *iv, uint8* _authentication);
		bool DecryptMessage( uint8 const* _data, uint32 const _length );
		bool EncryptMessage( uint8 const* _nonce );
		void QueuePayload( SecurityPayload * _payload );
		bool createIVFromPacket_inbound(uint8 const* _data, uint8 *iv);
		bool createIVFromPacket_outbound(uint8 const* _data, uint8 *iv);
		void SetupNetworkKey();

		Mutex *m_queueMutex;
		list<SecurityPayload *>      m_queue;         // Messages waiting to be sent when the device wakes up
		bool m_waitingForNonce;
		uint8 m_sequenceCounter;
		Timer m_nonceTimer;
		uint8 currentNonce[8];
		bool m_networkkeyset;

		aes_encrypt_ctx *AuthKey;
		aes_encrypt_ctx *EncryptKey;
		uint8 *nk;
		bool m_schemeagreed;
		bool m_secured;





	};

} // namespace OpenZWave

#endif

