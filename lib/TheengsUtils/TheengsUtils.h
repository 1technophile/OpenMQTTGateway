/*
  Theengs - IoT Interoperability

  Copyright: (c)Florian ROBERT

    This file is part of Theengs products.

    Theengs is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Theengs is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef THEENGS_UTIL_H
#define THEENGS_UTIL_H

#include <Arduino.h>
#include <time.h>
#include <IPAddress.h>
#include <string>

class TheengsUtils {
public:
    static String toString(uint64_t input);
    static std::string processCert(const char* cert);
    static std::string generateHash(const std::string& input);
    static unsigned long uptime();
    static void syncNTP();
    static int unixtimestamp();
    static String UTCtimestamp();
    static void revert_hex_data(const char* in, char* out, int l);
    static long value_from_hex_data(const char* service_data, int offset, int data_length, bool reverse, bool canBeNegative = true);
    static double round2(float value);
    static bool _rawToHex(byte* in, char* out, int rawSize);
    static bool _hexToRaw(const char* in, byte* out, int rawSize);
    static char* ip2CharArray(IPAddress ip);
    static bool to_bool(String const& s);
};

#endif