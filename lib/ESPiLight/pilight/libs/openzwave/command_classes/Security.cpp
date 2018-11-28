//-----------------------------------------------------------------------------
//
//	Security.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_Security
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

#include <ctime>


#include "CommandClasses.h"
#include "Security.h"
#include "../Defs.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../platform/Log.h"

#include "../value_classes/ValueBool.h"


using namespace OpenZWave;


#define UNUSED(x) (void)(x)


/* in order to communicate with a Secure Device, we need to send the Network Key to the
 * Device with a SecurityCmd_NetworkKeySet packet containing our Network Key.
 * Fairly simple right?
 *
 * BUT
 *
 * The NetworkKeySet should be done during a inclusion of the device (there is technically a timeout)
 * and not at any random time. This means that when including Secure Devices, we must use the OZW AddNode
 * Controller Commands, and then once the Inclusion is complete, initiate a NetworkKeySet. Including a
 * Secure Device by say using the button on a Z-Stick is technically not possible, as OZW needs to send
 * the Network Key right away after negotiating the SecurityScheme (before the timeout).
 *
 */

enum SecurityCmd
{
	SecurityCmd_SupportedGet			= 0x02,
	SecurityCmd_SupportedReport			= 0x03,
	SecurityCmd_SchemeGet				= 0x04,
	SecurityCmd_SchemeReport			= 0x05,
	SecurityCmd_NetworkKeySet			= 0x06,
	SecurityCmd_NetworkKeyVerify		= 0x07,
	SecurityCmd_SchemeInherit			= 0x08,
	SecurityCmd_NonceGet				= 0x40,
	SecurityCmd_NonceReport				= 0x80,
	SecurityCmd_MessageEncap			= 0x81,
	SecurityCmd_MessageEncapNonceGet	= 0xc1
};

enum SecurityScheme
{
	SecurityScheme_Zero					= 0x00,
};


uint8_t SecuritySchemes[1][16] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

uint8_t EncryptPassword[16] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
uint8_t AuthPassword[16] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};

void PrintHex(std::string prefix, uint8_t const *data, uint32 const length) {
	char byteStr[16];
	std::string str;
	for( uint32 i=0; i<length; ++i )
	{
		if( i )
		{
			str += ", ";
		}

		snprintf( byteStr, sizeof(byteStr), "0x%.2x", data[i] );
		str += byteStr;
	}
	Log::Write(LogLevel_Info, "%s Packet: %s", prefix.c_str(), str.c_str());
}

Security::Security
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),

	m_queueMutex( new Mutex() ),
	m_waitingForNonce(false),
	m_sequenceCounter(0),
	m_networkkeyset(false),
	m_schemeagreed(false),
	m_secured(false)

{
	/* We don't want the Driver to route "Security" messages back to us for Encryption,
	 * so disable SecureSupport for the Security Command Class
	 * (This stops this Command Class getting Marked as as IsSecured() if its listed
	 * in the SecurityCmd_SupportedReport from the device - Which some devices do)
	 */
	ClearSecureSupport();

	/* seed our Random Number Generator for NONCE Generation
	 * It doesn't matter we might seed it multiple times with each device that
	 * supports this class, it just adds more "randomness" to the NONCE
	 * although I'm sure its no way cryptographically secure :) */
	srand((unsigned)time(0));
	SetupNetworkKey();
}

Security::~Security
(
)
{
	m_queueMutex->Release();
	delete this->AuthKey;
	delete this->EncryptKey;
}

//-----------------------------------------------------------------------------
// <Version::ReadXML>
// Read configuration.
//-----------------------------------------------------------------------------
void Security::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	CommandClass::ReadXML( _ccElement );
}

//-----------------------------------------------------------------------------
// <Version::WriteXML>
// Save changed configuration
//-----------------------------------------------------------------------------
void Security::WriteXML
(
	TiXmlElement* _ccElement
)
{
	CommandClass::WriteXML( _ccElement );
}



void Security::SetupNetworkKey
(
)
{
//#define TESTENC 1

#if TESTENC
//	uint8_t iv[16] = {  0x81, 0x42, 0xd1, 0x51, 0xf1, 0x59, 0x3d, 0x70, 0xd5, 0xe3, 0x6c, 0xcb, 0x02, 0xd0, 0x3f, 0x5c,  /* */  };
	uint8_t iv[16] = {  0xee, 0x2c, 0x1c, 0x0e, 0xe1, 0x54, 0xe7, 0xfa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa  /* */  };
//	uint8_t pck[] = {  0x25, 0x68, 0x06, 0xc5, 0xb3, 0xee, 0x2c, 0x17, 0x26, 0x7e, 0xf0, 0x84, 0xd4, 0xc3, 0xba, 0xed, 0xe5, 0xb9, 0x55};
	uint8_t pck[] = {  0x65, 0xce, 0x5e, 0x75, 0xec, 0xe6, 0x09, 0x9b};
	/* 				    1     2     3     4     5     6     7     8     9     10    11    12    13    14    15    16    17    18    19 */
	uint8_t out[50];
	memset(&out[0], 0, 50);
	this->nk = SecuritySchemes[0];

#endif
#if 0

	this->nk = GetDriver->GetNetworkKey();
#endif
	if ((GetNodeUnsafe()->IsAddingNode() == true) && (m_networkkeyset == false)) {
		Log::Write(LogLevel_Info, GetNodeId(), "  Using Scheme0 Network Key for Key Exchange (AddingNode: %s KeySet: %s)", GetNodeUnsafe()->IsAddingNode() ? "true" : "false", m_networkkeyset ? "true" : "false" );
		this->nk = SecuritySchemes[0];
	} else {
		Log::Write(LogLevel_Info, GetNodeId(), "  Using Configured Network Key (AddingNode: %s KeySet: %s)", GetNodeUnsafe()->IsAddingNode() ? "true" : "false", m_networkkeyset ? "true" : "false" );
		this->nk = GetDriver()->GetNetworkKey();
	}
#ifdef DEBUG
	PrintHex("Network Key", this->nk, 16);
#endif
	this->AuthKey = new aes_encrypt_ctx;
	this->EncryptKey = new aes_encrypt_ctx;

	if (aes_init1() == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Init AES Engine");
		return;
	}

	if (aes_encrypt_key128(this->nk, this->EncryptKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Set Initial Network Key for Encryption");
		return;
	}

	if (aes_encrypt_key128(this->nk, this->AuthKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Set Initial Network Key for Authentication");
		return;
	}

	uint8 tmpEncKey[32];
	uint8 tmpAuthKey[32];
	aes_mode_reset(this->EncryptKey);
	aes_mode_reset(this->AuthKey);

	if (aes_ecb_encrypt(EncryptPassword, tmpEncKey, 16, this->EncryptKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Generate Encrypted Network Key for Encryption");
		return;
	}
	if (aes_ecb_encrypt(AuthPassword, tmpAuthKey, 16, this->AuthKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Generate Encrypted Network Key for Authentication");
		return;
	}


	aes_mode_reset(this->EncryptKey);
	aes_mode_reset(this->AuthKey);
	if (aes_encrypt_key128(tmpEncKey, this->EncryptKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to set Encrypted Network Key for Encryption");
		return;
	}
	if (aes_encrypt_key128(tmpAuthKey, this->AuthKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to set Encrypted Network Key for Authentication");
		return;
	}
	aes_mode_reset(this->EncryptKey);
	aes_mode_reset(this->AuthKey);
#if TESTENC
	PrintHex("Key", this->nk, 16);
	PrintHex("Packet Encryption Key", tmpEncKey, 16);
	PrintHex("IV", iv, 16);
	PrintHex("input", pck, 19);
	if (aes_ofb_decrypt(pck, out, 19, iv, this->EncryptKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Decrypt Packet");
		return;
	}
	PrintHex("Pck", out, 19);
	//exit(-1);

#endif
	//uint8 tmpiv[8] = {  0x16, 0xc2, 0x96, 0x60, 0x46, 0x33, 0x69, 0x04 };
	/* expected output = 0x06, 0xcc, 0xab, 0x1a, 0x86, 0x91, 0x39, 0xfd
	 * expected Auth = 0x64, 0x42, 0x26, 0x59, 0xf3, 0x04, 0x72, 0x16
	 * plaintext packet = 0x62, 0x03, 0x00, 0x10, 0x02, 0xfe, 0xfe
	 */
	//this->EncryptMessage(tmpiv);

}
bool Security::Init
(
)
{
	/* if we are adding this node, then instead to a SchemeGet Command instead - This
	 * will start the Network Key Exchange
	 */
	if (GetNodeUnsafe()->IsAddingNode()) {
		Msg * msg = new Msg ("SecurityCmd_SchemeGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 3 );
		msg->Append( GetCommandClassId() );
		msg->Append( SecurityCmd_SchemeGet );
		msg->Append( 0 );
		msg->Append( GetDriver()->GetTransmitOptions() );
		/* SchemeGet is unencrypted */
		GetDriver()->SendMsg(msg, Driver::MsgQueue_Security);
	} else {
		Msg* msg = new Msg( "SecurityCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( SecurityCmd_SupportedGet );
		msg->Append( GetDriver()->GetTransmitOptions() );
		this->SendMsg( msg);
	}

	return true;
}
//-----------------------------------------------------------------------------
// <Security::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Security::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
#if 0
	if( _requestFlags & RequestFlag_Static )
	{

	}
	return false;
#endif
	return true;
}

//-----------------------------------------------------------------------------
// <Security::RequestValue>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Security::RequestValue
(
	uint32 const _requestFlags,
	uint8 const _index,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	Log::Write(LogLevel_Info, GetNodeId(), "Got a RequestValue Call");
	return true;
}


bool Security::HandleSupportedReport
(
	uint8 const* _data,
	uint32 const _length
)
{

#ifdef DEBUG
	PrintHex("Security Classes", _data, _length);
#endif
	GetNodeUnsafe()->SetSecuredClasses(_data, _length);
	/* advance the Query Stage */
	GetNodeUnsafe()->QueryStageComplete(Node::QueryStage_SecurityReport);
	GetNodeUnsafe()->AdvanceQueries();
	return true;
}

//-----------------------------------------------------------------------------
// <Security::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Security::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	switch( (SecurityCmd)_data[0] )
	{
		case SecurityCmd_SupportedReport:
		{
			/* this is a list of CommandClasses that should be Encrypted.
			 * and it might contain new command classes that were not present in the NodeInfoFrame
			 * so we have to run through, mark existing Command Classes as SetSecured (so SendMsg in the Driver
			 * class will route the unecrypted messages to our SendMsg) and for New Command
			 * Classes, create them, and of course, also do a SetSecured on them.
			 *
			 * This means we must do a SecurityCmd_SupportedGet request ASAP so we dont have
			 * Command Classes created after the Discovery Phase is completed!
			 */
			Log::Write(LogLevel_Info, GetNodeId(), "Received SecurityCmd_SupportedReport from node %d", GetNodeId() );
			HandleSupportedReport(&_data[2], _length-2);
			break;
		}
		case SecurityCmd_SchemeReport:
		{
			Log::Write(LogLevel_Info, GetNodeId(), "Received SecurityCmd_SchemeReport from node %d: %d", GetNodeId(), _data[1]);
			uint8 schemes = _data[1];
			if (m_schemeagreed == true) {
				Log::Write(LogLevel_Warning, GetNodeId(), "   Already Received a SecurityCmd_SchemeReport from the node. Ignoring");
				break;
			}
			if( schemes == SecurityScheme_Zero )
			{
				/* We're good to go.  We now should send our NetworkKey to the device if this is the first
				 * time we have seen it
				 */
				Log::Write(LogLevel_Info, GetNodeId(), "    Security scheme agreed." );
				/* create the NetworkKey Packet. EncryptMessage will encrypt it for us (And request the NONCE) */
				Msg * msg = new Msg ("SecurityCmd_NetworkKeySet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
				msg->Append( GetNodeId() );
				msg->Append( 18 );
				msg->Append( GetCommandClassId() );
				msg->Append( SecurityCmd_NetworkKeySet );
				for (int i = 0; i < 16; i++)
					msg->Append(GetDriver()->GetNetworkKey()[i]);
				msg->Append( GetDriver()->GetTransmitOptions() );
				this->SendMsg( msg);
				m_schemeagreed = true;
			}
			else
			{
				/* No common security scheme.  The device should continue as an unsecured node.
				 * but Some Command Classes might not be present...
				 */
				Log::Write(LogLevel_Warning,  GetNodeId(), "    No common security scheme.  The device will continue as an unsecured node." );
			}
			break;
		}
		case SecurityCmd_NetworkKeySet:
		{
			/* we shouldn't get a NetworkKeySet from a node if we are the controller
			 * as we send it out to the Devices
			 */
			Log::Write(LogLevel_Info,  GetNodeId(), "Received SecurityCmd_NetworkKeySet from node %d", GetNodeId() );
			break;
		}
		case SecurityCmd_NetworkKeyVerify:
		{
			/* if we can decrypt this packet, then we are assured that our NetworkKeySet is successfull
			 * and thus should set the Flag referenced in SecurityCmd_SchemeReport
			 */
			Log::Write(LogLevel_Info,  GetNodeId(), "Received SecurityCmd_NetworkKeyVerify from node %d", GetNodeId() );
			/* now as for our SupportedGet */
			Msg* msg = new Msg( "SecurityCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( SecurityCmd_SupportedGet );
			msg->Append( GetDriver()->GetTransmitOptions() );
			this->SendMsg( msg);

			break;
		}
		case SecurityCmd_SchemeInherit:
		{
			/* only used in a Controller Replication Type enviroment.
			 *
			 */
			Log::Write(LogLevel_Info,  GetNodeId(), "Received SecurityCmd_SchemeInherit from node %d", GetNodeId() );
			break;
		}
		case SecurityCmd_NonceGet:
		{
			/* the Device wants to send us a Encrypted Packet, and thus requesting for our latest NONCE
			 *
			 */
			Log::Write(LogLevel_Info,  GetNodeId(), "Received SecurityCmd_NonceGet from node %d", GetNodeId() );
			SendNonceReport();
			break;
		}
		case SecurityCmd_NonceReport:
		{
			/* we recieved a NONCE from a device, so assume that there is something in a queue to send
			 * out
			 */
			Log::Write(LogLevel_Info,  GetNodeId(), "Received SecurityCmd_NonceReport from node %d", GetNodeId() );
			EncryptMessage( &_data[1] );
			m_waitingForNonce = false;
			break;
		}
		case SecurityCmd_MessageEncap:
		{
			/* We recieved a Encrypted single packet from the Device. Decrypt it.
			 *
			 */
			Log::Write(LogLevel_Info,  GetNodeId(), "Received SecurityCmd_MessageEncap from node %d", GetNodeId() );
			DecryptMessage( _data, _length );
			break;
		}
		case SecurityCmd_MessageEncapNonceGet:
		{
			/* we recieved a encrypted packet from the device, and the device is also asking us to send a
			 * new NONCE to it, hence there must be multiple packets.
			 */
			Log::Write(LogLevel_Info,  GetNodeId(), "Received SecurityCmd_MessageEncapNonceGet from node %d", GetNodeId() );
			DecryptMessage( _data, _length );
			/* Regardless of the success/failure of Decrypting, send a new NONCE */
			SendNonceReport();
			break;
		}
		default:
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// <Security::SendMsg>
// Queue a message to be securely sent by the Z-Wave PC Interface
//-----------------------------------------------------------------------------
void Security::SendMsg
(
	Msg* _msg
)
{
	_msg->Finalize();

	uint8* buffer = _msg->GetBuffer();
	if( _msg->GetLength() < 7 )
	{
		// Message too short
		assert(0);
		return;
	}

	if( buffer[3] != FUNC_ID_ZW_SEND_DATA )
	{
		// Invalid message type
		assert(0);
		return;
	}

	uint8 length = buffer[5];
	if( length > 28 )
	{
		// Message must be split into two parts
		SecurityPayload *payload1 = new SecurityPayload();
		payload1->m_length = 28;
		payload1->m_part = 1;
		memcpy( payload1->m_data, &buffer[6], payload1->m_length );
		payload1->logmsg = _msg->GetLogText();
		QueuePayload( payload1 );

		SecurityPayload *payload2 = new SecurityPayload();
		payload2->m_length = length-28;
		payload2->m_part = 2;
		memcpy( payload2->m_data, &buffer[34], payload2->m_length );
		payload2->logmsg = _msg->GetLogText();
		QueuePayload( payload2 );
	}
	else
	{
		// The entire message can be encapsulated as one
		SecurityPayload *payload = new SecurityPayload();
		payload->m_length = length;
		payload->m_part = 0;				// Zero means not split into separate messages
		memcpy( payload->m_data, &buffer[6], payload->m_length );
		payload->logmsg = _msg->GetLogText();
		QueuePayload( payload );
	}
	delete _msg;
}

//-----------------------------------------------------------------------------
// <Security::QueuePayload>
// Queue data to be encapsulated by the Security Command Class, on
// receipt of a nonce value from the remote node.
//-----------------------------------------------------------------------------
void Security::QueuePayload
(
	SecurityPayload *_payload
)
{
	m_queueMutex->Lock();
	m_queue.push_back( _payload );

	if( !m_waitingForNonce )
	{
		// Request a nonce from the node.  Its arrival
		// will trigger the sending of the first payload
		RequestNonce();
	}

	m_queueMutex->Unlock();
}


//-----------------------------------------------------------------------------
// <Security::EncryptMessage>
// Encrypt and send a Z-Wave message securely.
//-----------------------------------------------------------------------------
bool Security::EncryptMessage
(
	uint8 const* _nonce
)
{
#if 1
	if( m_nonceTimer.GetMilliseconds() > 10000 )
	{
		// The nonce was  not received within 10 seconds
		// of us sending the nonce request.  Send it again
		RequestNonce();
		return false;
	}

	// Fetch the next payload from the queue and encapsulate it
	m_queueMutex->Lock();
	if( m_queue.empty() )
	{
		// Nothing to do
		m_queueMutex->Release();
		return false;
	}

	SecurityPayload * payload = m_queue.front();
	m_queue.pop_front();
	//uint32 queueSize = m_queue.size();
	m_queueMutex->Unlock();
#else
	uint32 queueSize = m_queue.size();
	struct SecurityPayload payload;
	payload.m_length = 7;
	payload.m_part = 0;
	uint8 tmpdata[7] = {0x62, 0x03, 0x00, 0x10, 0x02, 0xfe, 0xfe};
	for (int i = 0; i < payload.m_length; i++)
		payload.m_data[i] = tmpdata[i];
#endif
	// Encapsulate the message fragment
	/* MessageEncapNonceGet doesn't seem to work */
	//Msg* msg = new Msg( (queueSize>1) ? "SecurityCmd_MessageEncapNonceGet" : "SecurityCmd_MessageEncap", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
	string LogMessage("SecurityCmd_MessageEncap (");
	LogMessage.append(payload->logmsg);
	LogMessage.append(")");
	Msg* msg = new Msg( LogMessage, GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( payload->m_length + 20 );
	msg->Append( GetCommandClassId() );
	//msg->Append( (queueSize>1) ? SecurityCmd_MessageEncapNonceGet : SecurityCmd_MessageEncap );
	msg->Append( SecurityCmd_MessageEncap );
	/* create the iv
	 *
	 */
	uint8 initializationVector[16];
	/* the first 8 bytes of a outgoing IV are random */
	for (int i = 0; i < 8; i++) {
		//initializationVector[i] = (rand()%0xFF)+1;
		initializationVector[i] = 0xAA;
	}
	/* the remaining 8 bytes are the NONCE we got from the device */
	for (int i = 0; i < 8; i++) {
		initializationVector[8+i] = _nonce[i];
	}

	/* Append the first 8 bytes of the initialization vector
	 * to the message. The remaining 8 bytes are the NONCE we recieved from
	 * the node, and is ommitted from sending back to the Node. But we use the full 16 bytes to
	 * as the IV to encrypt out message.
	 */
	for(int i=0; i<8; ++i )
	{
		msg->Append( initializationVector[i] );
	}

	// Append the sequence data
	uint8 sequence = 0;
	if( payload->m_part == 0 )
	{
		sequence = 0;
	}
	else if( payload->m_part == 1 )
	{
		sequence = (++m_sequenceCounter) & 0x0f;
		sequence |= 0x10;		// Sequenced, first frame
	}
	if( payload->m_part == 2 )
	{
		sequence = m_sequenceCounter & 0x0f;
		sequence |= 0x30;		// Sequenced, second frame
	}
	/* at most, the payload will be 28 bytes + 1 byte for the Sequence. */
	uint8 plaintextmsg[32];
	plaintextmsg[0] = sequence;
	for (int i = 0; i < payload->m_length; i++)
		plaintextmsg[i+1] = payload->m_data[i];

	/* Append the message payload after encrypting it with AES-OFB (key is EncryptPassword,
	 * full IV (16 bytes - 8 Random and 8 NONCE) and payload.m_data
	 */
	PrintHex("Input Packet:", plaintextmsg, payload->m_length+1);
#ifdef DEBUG
	PrintHex("IV:", initializationVector, 16);
#endif
	uint8 encryptedpayload[30];
	aes_mode_reset(this->EncryptKey);
	if (aes_ofb_encrypt(plaintextmsg, encryptedpayload, payload->m_length+1, initializationVector, this->EncryptKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Encrypt Packet");
		delete msg;
		return false;
	}
#ifdef DEBUG
	PrintHex("Encrypted Output", encryptedpayload, payload->m_length+1);

	/* The Following Code attempts to Decrypt the Packet and Verify */

	/* the first 8 bytes of a outgoing IV are random */
	for (int i = 0; i < 8; i++) {
		//initializationVector[i] = (rand()%0xFF)+1;
		initializationVector[i] = 0xAA;
	}
	/* the remaining 8 bytes are the NONCE we got from the device */
	for (int i = 0; i < 8; i++) {
		initializationVector[8+i] = _nonce[i];
	}
	aes_mode_reset(this->EncryptKey);
	uint8 tmpoutput[16];
	if (aes_ofb_encrypt(encryptedpayload, tmpoutput, payload->m_length+1, initializationVector, this->EncryptKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Encrypt Packet");
		delete msg;
		return false;
	}

	PrintHex("Decrypted output", tmpoutput, payload->m_length+1);
#endif
	for(int i=0; i<payload->m_length+1; ++i )
	{
		msg->Append( encryptedpayload[i] );
	}

	// Append the nonce identifier :)
	msg->Append(_nonce[0]);

	/* Append space for the authentication data Set with AES-CBCMAC (key is AuthPassword,
	 * Full IV (16 bytes - 8 random and 8 NONCE) and sequence|SrcNode|DstNode|payload.m_length|payload.m_data
	 *
	 */
	/* Regenerate IV */
	/* the first 8 bytes of a outgoing IV are random */
	for (int i = 0; i < 8; i++) {
		//initializationVector[i] = (rand()%0xFF)+1;
		initializationVector[i] = 0xAA;
	}
	/* the remaining 8 bytes are the NONCE we got from the device */
	for (int i = 0; i < 8; i++) {
		initializationVector[8+i] = _nonce[i];
	}
	uint8 mac[8];
	this->GenerateAuthentication(&msg->GetBuffer()[7], msg->GetLength()+2, GetDriver()->GetNodeId(), GetNodeId(), initializationVector, mac);
	for(int i=0; i<8; ++i )
	{
		msg->Append( mac[i] );
	}
#ifdef DEBUG
	PrintHex("Auth", mac, 8);
#endif
	msg->Append( GetDriver()->GetTransmitOptions() );
#ifdef DEBUG
	PrintHex("Outgoing", msg->GetBuffer(), msg->GetLength());
#endif
	GetDriver()->SendMsg(msg, Driver::MsgQueue_Security);

	/* finally, if the message we just sent is a NetworkKeySet, then we need to reset our Network Key here
	 * as the reply we will get back will be encrypted with the new Network key
	 */
	if ((this->m_networkkeyset == false) && (payload->m_data[0] == 0x98) && (payload->m_data[1] == 0x06)) {
		Log::Write(LogLevel_Info, GetNodeId(), "Reseting Network Key after Inclusion");
		this->m_networkkeyset = true;
		SetupNetworkKey();
	}

	delete payload;
	return true;
}

bool Security::createIVFromPacket_inbound(uint8 const* _data, uint8 *iv) {

	for (int i = 0; i < 8; i++) {
		iv[i] = _data[1+i];
	}
	for (int i = 0; i < 8; i++) {
		iv[8+i] = this->currentNonce[i];
	}
	return true;
}
bool Security::createIVFromPacket_outbound(uint8 const* _data, uint8 *iv) {


	for (int i = 0; i < 8; i++) {
		iv[i] = this->currentNonce[i];
	}
	for (int i = 0; i < 8; i++) {
		iv[8+i] = _data[1+i];
	}
	return true;
}



//-----------------------------------------------------------------------------
// <Security::DecryptMessage>
// Decrypt a security-encapsulated message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Security::DecryptMessage
(
	uint8 const* _data,
	uint32 const _length
)
{
	if( m_nonceTimer.GetMilliseconds() > 10000 )
	{
		// The message was not received within 10 seconds
		// of us sending the nonce report.

		// TBD - clear any partial message that has been stored.

		return false;
	}

	uint8 iv[17];
	createIVFromPacket_inbound(_data, iv); /* first 8 bytes of Packet are the Random Value generated by the Device
									* 2nd 8 bytes of the IV are our nonce we sent previously
									*/
	uint8 decryptpacket[32];
	memset(&decryptpacket[0], 0, 32);
	uint32 encryptedpacketsize = _length-11-8;
	uint8 encyptedpacket[32];

	for (uint32 i = 0; i < 32; i++) {
		if (i >= encryptedpacketsize) {
			/* pad the remaining fields */
			encyptedpacket[i] = 0;
		} else {
			encyptedpacket[i] = _data[9+i];
		}
	}
#ifdef DEBUG
	Log::Write(LogLevel_Debug, GetNodeId(), "Encrypted Packet Sizes: %d (Total) %d (Payload)", _length, encryptedpacketsize);
	PrintHex("IV", iv, 16);
	PrintHex("Encrypted", encyptedpacket, 16);
	/* 8 - IV - 2 - Command Header */
	PrintHex("Auth", &_data[8+encryptedpacketsize+2], 8);
#endif
	aes_mode_reset(this->EncryptKey);
#if 0
	uint8_t iv[16] = {  0x81, 0x42, 0xd1, 0x51, 0xf1, 0x59, 0x3d, 0x70, 0xd5, 0xe3, 0x6c, 0xcb, 0x02, 0xd0, 0x3f, 0x5c,  /* */  };
   	uint8_t pck[] = {  0x25, 0x68, 0x06, 0xc5, 0xb3, 0xee, 0x2c, 0x17, 0x26, 0x7e, 0xf0, 0x84, 0xd4, 0xc3, 0xba, 0xed, 0xe5, 0xb9, 0x55};
	if (aes_ofb_decrypt(pck, decryptpacket, 19, iv, this->EncryptKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Decrypt Packet");
		return false;
	}
	PrintHex("Pck", decryptpacket, 19);
#else
	if (aes_ofb_decrypt(encyptedpacket, decryptpacket, encryptedpacketsize, iv, this->EncryptKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to Decrypt Packet");
		if (m_queue.size() > 1)
			RequestNonce();
		return false;
	}
	PrintHex("Decrypted", decryptpacket, encryptedpacketsize);
#endif
	uint8 mac[32];
	/* we have to regenerate the IV as the ofb decryption routine will alter it. */
	createIVFromPacket_inbound(_data, iv);

	this->GenerateAuthentication(_data, _length, GetNodeId(), GetDriver()->GetNodeId(), iv, mac);
	if (memcmp(&_data[8+encryptedpacketsize+2], mac, 8) != 0) {
		Log::Write(LogLevel_Warning, GetNodeId(), "MAC Authentication of Packet Failed. Dropping");
		if (m_queue.size() > 1)
			RequestNonce();
		return false;
	}
	/* XXX TODO: Check the Sequence Header Frame to see if this is the first part of a
	 * message, or 2nd part, or a entire message.
	 *
	 * I havn't actually seen a Z-Wave Message thats too big to fit in a encrypted message
	 * yet, so we will look at this if such a message actually exists!
	 */

	/* if the command class is us, send it back to our HandleMessage */
	if (decryptpacket[1] == this->StaticGetCommandClassId()) {
		/* drop the sequence header, and the command class when we pass through to our handler */
		this->HandleMsg(&decryptpacket[2], encryptedpacketsize-2);
	} else {
		/* send to the Command Class for processing.... */
		if( Node* node = GetNodeUnsafe() )
		{
			uint8 commandClassId = decryptpacket[1];

			if( CommandClass* pCommandClass = node->GetCommandClass( commandClassId ) )
			{
				Log::Write( LogLevel_Info, GetNodeId(), "Received a SecurityCmd_MessageEncap from node %d for Command Class %s", GetNodeId(), pCommandClass->GetCommandClassName().c_str() );
				pCommandClass->ReceivedCntIncr();
				pCommandClass->HandleMsg( &decryptpacket[2], encryptedpacketsize-2);
			} else {
				Log::Write( LogLevel_Info, GetNodeId(), "ApplicationCommandHandler - Unhandled Command Class 0x%.2x", decryptpacket[1] );
			}
		}
	}
	if (m_queue.size() > 0)
		RequestNonce();
	if (m_secured == false) {
		if( ValueBool* value = static_cast<ValueBool*>( GetValue( 1, 0 ) ) )
		{
			value->OnValueRefreshed( true );
			value->Release();
		}
		m_secured = true;
	}
	return true;

}

//-----------------------------------------------------------------------------
// <Security::GenerateAuthentication>
// Generate authentication data from a security-encrypted message
//-----------------------------------------------------------------------------
bool Security::GenerateAuthentication
(
	uint8 const* _data,				// Starting from the command class command
	uint32 const _length,
	uint8 const _sendingNode,
	uint8 const _receivingNode,
	uint8 *iv,
	uint8* _authentication			// 8-byte buffer that will be filled with the authentication data
)
{
	// Build a buffer containing a 4-byte header and the encrypted
	// message data, padded with zeros to a 16-byte boundary.
	uint8 buffer[256];
	uint8 tmpauth[16];
	memset(buffer, 0, 256);
	memset(tmpauth, 0, 16);
	buffer[0] = _data[0];							// Security command class command
	buffer[1] = _sendingNode;
	buffer[2] = _receivingNode;
	buffer[3] = _length - 19; // Subtract 19 to account for the 9 security command class bytes that come before and after the encrypted data
	memcpy( &buffer[4], &_data[9], _length-19 );	// Encrypted message

	uint8 bufsize = _length - 19 + 4; /* the size of buffer */
#ifdef DEBUG
	PrintHex("Raw Auth (minus IV)", buffer, bufsize);
	Log::Write(LogLevel_Debug, GetNodeId(), "Raw Auth (Minus IV) Size: %d (%d)", bufsize, bufsize+16);
#endif

	aes_mode_reset(this->AuthKey);
	/* encrypt the IV with ecb */
	if (aes_ecb_encrypt(iv, tmpauth, 16, this->AuthKey) == EXIT_FAILURE) {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed Initial ECB Encrypt of Auth Packet");
		return false;
	}

	/* our temporary holding var */
	uint8 encpck[16];

	int block = 0;
	/* reset our encpck temp var */
	memset(encpck, 0, 16);
	/* now xor the buffer with our encrypted IV */
	for (int i = 0; i < bufsize; i++) {
		encpck[block] = buffer[i];
		block++;
		/* if we hit a blocksize, then encrypt */
		if (block == 16) {
			for (int j = 0; j < 16; j++) {
				/* here we do our xor */
				tmpauth[j] = encpck[j] ^ tmpauth[j];
				/* and reset encpck for good measure */
				encpck[j] = 0;
			}
			/* reset our block counter back to 0 */
			block = 0;
			aes_mode_reset(this->AuthKey);
			if (aes_ecb_encrypt(tmpauth, tmpauth, 16, this->AuthKey) == EXIT_FAILURE) {
				Log::Write(LogLevel_Warning, GetNodeId(), "Failed Subsequent (%d) ECB Encrypt of Auth Packet", i);
				return false;
			}
		}
	}
	/* any left over data that isn't a full block size*/
	if (block > 0) {
		for (int i= 0; i < 16; i++) {
			/* encpck from block to 16 is already gauranteed to be 0
			 * so its safe to xor it with out tmpmac */
			tmpauth[i] = encpck[i] ^ tmpauth[i];
		}
		aes_mode_reset(this->AuthKey);
		if (aes_ecb_encrypt(tmpauth, tmpauth, 16, this->AuthKey) == EXIT_FAILURE) {
			Log::Write(LogLevel_Warning, GetNodeId(), "Failed Final ECB Encrypt of Auth Packet");
			return false;
		}
	}
	/* we only care about the first 8 bytes of tmpauth as the mac */
#ifdef DEBUG
	PrintHex("Computed Auth", tmpauth, 8);
#endif
	/* so only copy 8 bytes to the _authentication var */
	memcpy(_authentication, tmpauth, 8);
	return true;
}

//-----------------------------------------------------------------------------
// <Security::RequestNonce>
// Request a nonce from the node
//-----------------------------------------------------------------------------
void Security::RequestNonce
(
)
{
	if (m_waitingForNonce == true)
		return;
	m_waitingForNonce = true;

	Msg* msg = new Msg( "SecurityCmd_NonceGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( SecurityCmd_NonceGet );
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Security);

	// Reset the nonce timer.  The nonce report
	// must be received within 10 seconds.
	m_nonceTimer.Reset();
}

//-----------------------------------------------------------------------------
// <Security::SendNonceReport>
// Send a nonce to the node
//-----------------------------------------------------------------------------
void Security::SendNonceReport
(
)
{
	//uint8 publicNonce[8];

	/* this should be pretty random */
	for (int i = 0; i < 8; i++) {
		//this->currentNonce[i] = (rand()%0xFF)+1;
		this->currentNonce[i] = 0xAA;
	}


	Msg* msg = new Msg( "SecurityCmd_NonceReport", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 10 );
	msg->Append( GetCommandClassId() );
	msg->Append( SecurityCmd_NonceReport );
	for( int i=0; i<8; ++i )
	{
		msg->Append( this->currentNonce[i] );
	}
	msg->Append( TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE );
	GetDriver()->SendMsg( msg, Driver::MsgQueue_Security);

	// Reset the nonce timer.  The encapsulated message
	// must be received within 10 seconds.
	m_nonceTimer.Reset();
}

//-----------------------------------------------------------------------------
// <Battery::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Security::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
	  	node->CreateValueBool( ValueID::ValueGenre_System, GetCommandClassId(), _instance, 0, "Secured", "", true, false, false, 0 );
	}
}
