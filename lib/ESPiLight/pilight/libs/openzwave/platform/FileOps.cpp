//-----------------------------------------------------------------------------
//
//	FileOps.cpp
//
//	Cross-platform File Operations
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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
#include <string>
#include "FileOps.h"

#ifdef _WIN32
#include "windows/FileOpsImpl.h"	// Platform-specific implementation of a File Operations
#else
#include "unix/FileOpsImpl.h"	// Platform-specific implementation of a File Operations
#endif

using namespace OpenZWave;

FileOps* FileOps::s_instance = NULL;
FileOpsImpl* FileOps::m_pImpl = NULL;

//-----------------------------------------------------------------------------
//	<FileOps::Create>
//	Static creation of the singleton
//-----------------------------------------------------------------------------
FileOps* FileOps::Create
(
)
{
	if( s_instance == NULL )
	{
		s_instance = new FileOps();
	}
	return s_instance;
}

//-----------------------------------------------------------------------------
//	<FileOps::Destroy>
//	Static method to destroy the fileops singleton.
//-----------------------------------------------------------------------------
void FileOps::Destroy
(
)
{
	delete s_instance;
	s_instance = NULL;
}

//-----------------------------------------------------------------------------
//	<FileOps::FolderExists>
//	Static method to check for existance of a folder
//-----------------------------------------------------------------------------
bool FileOps::FolderExists
(
	const string &_folderName
)
{
	if( s_instance != NULL )
	{
		return s_instance->m_pImpl->FolderExists( _folderName );
	}
	return false;
}

//-----------------------------------------------------------------------------
//	<FileOps::FileOps>
//	Constructor
//-----------------------------------------------------------------------------
FileOps::FileOps
(
)
{
	m_pImpl = new FileOpsImpl();
}

//-----------------------------------------------------------------------------
//	<FileOps::~FileOps>
//	Destructor
//-----------------------------------------------------------------------------
FileOps::~FileOps
(
)
{
	delete m_pImpl;
}
