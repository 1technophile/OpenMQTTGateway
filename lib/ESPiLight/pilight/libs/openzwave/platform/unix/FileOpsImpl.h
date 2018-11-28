//-----------------------------------------------------------------------------
//
//	FileOpsImpl.h
//
//	Unix implementation of file operations
//
//	Copyright (c) 2012, Greg Satz <satz@iranger.com>
//	All rights reserved.
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
#ifndef _FileOpsImpl_H
#define _FileOpsImpl_H

#include <stdarg.h>
#include <string>
#include "../../Defs.h"
#include "../FileOps.h"

namespace OpenZWave
{
	class FileOpsImpl
	{
		friend class FileOps;

	private:
		FileOpsImpl();
		~FileOpsImpl();

		bool FolderExists( string _filename );
	};

} // namespace OpenZWave

#endif //_FileOpsImpl_H

