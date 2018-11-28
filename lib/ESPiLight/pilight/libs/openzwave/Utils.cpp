//-----------------------------------------------------------------------------
//
//	Utils.h
//
//	Miscellaneous helper functions
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


#include "Defs.h"
#include "Utils.h"

using namespace OpenZWave;

//-----------------------------------------------------------------------------
// <OpenZWave::ToUpper>
// Convert a string to all upper-case.
//-----------------------------------------------------------------------------
string OpenZWave::ToUpper
(
	string const& _str
)
{
	string upper = _str;
	transform( upper.begin(), upper.end(), upper.begin(), ::toupper );
	return upper;
}

//-----------------------------------------------------------------------------
// <OpenZWave::ToLower>
// Convert a string to all lower-case.
//-----------------------------------------------------------------------------
string OpenZWave::ToLower
(
	string const& _str
)
{
	string lower = _str;
	transform( lower.begin(), lower.end(), lower.begin(), ::tolower );
	return lower;
}

//-----------------------------------------------------------------------------
// <OpenZWave::trim>
// Remove WhiteSpaces from the begining and end of a string
//-----------------------------------------------------------------------------

std::string &OpenZWave::trim
(
		std::string &s
)
{
    if(s.size() == 0)
    {
        return s;
    }

    int val = 0;
    for (size_t cur = 0; cur < s.size(); cur++)
    {
        if(s[cur] != ' ' && isalnum(s[cur]))
        {
            s[val] = s[cur];
            val++;
        }
    }
    s.resize(val);
    return s;
}

//-----------------------------------------------------------------------------
// <OpenZWave::split>
// Split a String into a vector, seperated by anything specified in seperators.
//-----------------------------------------------------------------------------
void OpenZWave::split
(
		std::vector<std::string>& lst,
		const std::string& input,
		const std::string& separators,
		bool remove_empty
)
{
    std::ostringstream word;
    for (size_t n = 0; n < input.size(); ++n)
    {
        if (std::string::npos == separators.find(input[n]))
            word << input[n];
        else
        {
            if (!word.str().empty() || !remove_empty)
                lst.push_back(word.str());
            word.str("");
        }
    }
    if (!word.str().empty() || !remove_empty)
        lst.push_back(word.str());
}
