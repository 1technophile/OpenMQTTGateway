//-----------------------------------------------------------------------------
//
//	SerialControllerImpl.h
//
//	Windows Implementation of the cross-platform serial port
//
//	Copyright (c) 2010 Jason Frazier <frazierjason@gmail.com>
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

#include <windows.h>

#include "../../Defs.h"
#include "../SerialController.h"

namespace OpenZWave
{
	class SerialControllerImpl
	{
	public:
		void ReadThreadProc();

	private:
		friend class SerialController;

		SerialControllerImpl( SerialController* _owner );
		~SerialControllerImpl();

		bool Open();
		void Close();

		uint32 Write( uint8* _buffer, uint32 _length );

		bool Init( uint32 const _attempts );
		void Read();

		SerialController*			m_owner;
		HANDLE						m_hThread;
		HANDLE						m_hExit;
		HANDLE						m_hSerialController;
	};

} // namespace OpenZWave

#endif //_SerialControllerImpl_H

