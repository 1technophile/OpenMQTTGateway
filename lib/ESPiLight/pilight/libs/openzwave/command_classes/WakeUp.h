//-----------------------------------------------------------------------------
//
//	WakeUp.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_WAKE_UP
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

#ifndef _WakeUp_H
#define _WakeUp_H

#include <list>
#include "CommandClass.h"
#include "../Driver.h"

namespace OpenZWave
{
	class Msg;
	class ValueInt;
	class Mutex;

	/** \brief Implements COMMAND_CLASS_WAKE_UP (0x84), a Z-Wave device command class.
	 */
	class WakeUp: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new WakeUp( _homeId, _nodeId ); }
		virtual ~WakeUp();

		static uint8 const StaticGetCommandClassId(){ return 0x84; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_WAKE_UP"; }

		void Init();	// Starts the process of requesting node state from a sleeping device.
		void QueueMsg( Driver::MsgQueueItem const& _item );
		void SendPending();
		bool IsAwake()const{ return m_awake; }
		void SetAwake( bool _state );
		void SetPollRequired(){ m_pollRequired = true; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue );
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		virtual bool SetValue( Value const& _value );
		virtual void SetVersion( uint8 const _version );

		virtual uint8 GetMaxVersion(){ return 2; }

	protected:
		virtual void CreateVars( uint8 const _instance );

	private:
		WakeUp( uint32 const _homeId, uint8 const _nodeId );

		Mutex*						m_mutex;			// Serialize access to the pending queue
		list<Driver::MsgQueueItem>	m_pendingQueue;		// Messages waiting to be sent when the device wakes up
		bool						m_awake;
		bool						m_pollRequired;
		bool						m_notification;
	};

} // namespace OpenZWave

#endif


