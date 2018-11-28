//-----------------------------------------------------------------------------
//
//	FileOpsImpl.cpp
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

#include <dirent.h>
#include "FileOpsImpl.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
//	<FileOpsImpl::FileOpsImpl>
//	Constructor
//-----------------------------------------------------------------------------
FileOpsImpl::FileOpsImpl
(
)
{
}

//-----------------------------------------------------------------------------
//	<FileOpsImpl::~FileOpsImpl>
//	Destructor
//-----------------------------------------------------------------------------
FileOpsImpl::~FileOpsImpl
(
)
{
}

//-----------------------------------------------------------------------------
//	<FileOpsImpl::FolderExists>
//	Determine if a folder exists
//-----------------------------------------------------------------------------
bool FileOpsImpl::FolderExists
( 
	string _folderName
)
{
	DIR *dirp = opendir( _folderName.c_str() );
	if( dirp != NULL )
	{
		closedir( dirp );
		return true;
	}
	else
		return false;
}
