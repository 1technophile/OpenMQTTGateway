//
// SerialControllerImpl.h
//
// POSIX implementation of a cross-platform serial port
//
// Copyright (c) 2010, Greg Satz <satz@iranger.com>
// All rights reserved.
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

#ifndef _SerialControllerImpl_H
#define _SerialControllerImpl_H

#include <strings.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "../../Defs.h"
#include "../SerialController.h"

namespace OpenZWave
{
	class SerialControllerImpl
	{
	public:
		void ReadThreadProc( Event* _exitEvent );

	private:
		friend class SerialController;

		SerialControllerImpl( SerialController* _owner );
		~SerialControllerImpl();

		bool Open();
		void Close();

		uint32 Write( uint8* _buffer, uint32 _length );

		bool Init( uint32 const _attempts );
		void Read();

		SerialController*	m_owner;
		int			m_hSerialController;
		Thread*			m_pThread;

		static void SerialReadThreadEntryPoint( Event* _exitEvent, void* _content );
	};

} // namespace OpenZWave

#endif //_SerialControllerImpl_H

