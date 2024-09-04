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

#include "TheengsUtils.h"

String TheengsUtils::toString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

/*
* @brief Convert the spaces of the certificate into new lines
*/
std::string TheengsUtils::processCert(const char* cert) {
  std::string certStr(cert);
  size_t pos = 0;
  while ((pos = certStr.find(' ', pos)) != std::string::npos) {
    if (pos < 4 || (pos >= 27 && pos <= certStr.length() - 25) || pos >= certStr.length() - 4) {
      certStr.replace(pos, 1, "\n");
    }
    pos++;
  }
  return certStr;
}

std::string TheengsUtils::generateHash(const std::string& input) {
  // Implementation depends on your hash function
  // This is a placeholder
  return "hash_placeholder";
}

bool TheengsUtils::cmpToMainTopic(const char* topicOri, const char* toAdd) {
  // Implementation depends on mqtt_topic and gateway_name
  // You might need to pass these as parameters or make them class members
  return false;
}

unsigned long TheengsUtils::uptime() {
  static unsigned long lastUptime = 0;
  static unsigned long uptimeAdd = 0;
  unsigned long uptime = millis() / 1000 + uptimeAdd;
  if (uptime < lastUptime) {
    uptime += 4294967;
    uptimeAdd += 4294967;
  }
  lastUptime = uptime;
  return uptime;
}

void TheengsUtils::syncNTP() {
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    now = time(nullptr);
  }
}

int TheengsUtils::unixtimestamp() {
  return time(nullptr);
}

String TheengsUtils::UTCtimestamp() {
  time_t now;
  time(&now);
  char buffer[sizeof "yyyy-MM-ddThh:mm:ssZ"];
  strftime(buffer, sizeof buffer, "%FT%TZ", gmtime(&now));
  return buffer;
}

void TheengsUtils::revert_hex_data(const char* in, char* out, int l) {
  //reverting array 2 by 2 to get the data in good order
  int i = l - 2, j = 0;
  while (i != -2) {
    if (i % 2 == 0)
      out[j] = in[i + 1];
    else
      out[j] = in[i - 1];
    j++;
    i--;
  }
  out[l - 1] = '\0';
}

/**
 * Retrieve an unsigned long value from a char array extract representing hexadecimal data, reversed or not,
 * This value can represent a negative value if canBeNegative is set to true
 */
long TheengsUtils::value_from_hex_data(const char* service_data, int offset, int data_length, bool reverse, bool canBeNegative) {
  char data[data_length + 1];
  memcpy(data, &service_data[offset], data_length);
  data[data_length] = '\0';
  long value;
  if (reverse) {
    // reverse data order
    char rev_data[data_length + 1];
    revert_hex_data(data, rev_data, data_length + 1);
    value = strtol(rev_data, NULL, 16);
  } else {
    value = strtol(data, NULL, 16);
  }
  if (value > 65000 && data_length <= 4 && canBeNegative)
    value = value - 65535;
  return value;
}

/*
 rounds a number to 2 decimal places
 example: round(3.14159) -> 3.14
*/
double TheengsUtils::round2(float value) {
  return (int)(value * 100 + 0.5) / 100.0;
}

/*
From a byte array to an hexa char array ("A220EE...", double the size)
 */
bool TheengsUtils::_rawToHex(byte* in, char* out, int rawSize) {
  for (unsigned char p = 0; p < rawSize; p++) {
    sprintf_P(&out[p * 2], PSTR("%02X\r"), in[p]);
  }
  return true;
}

/*
From an hexa char array ("A220EE...") to a byte array (half the size)
 */
bool TheengsUtils::_hexToRaw(const char* in, byte* out, int rawSize) {
  if (strlen(in) != rawSize * 2)
    return false;
  char tmp[3] = {0};
  for (unsigned char p = 0; p < rawSize; p++) {
    memcpy(tmp, &in[p * 2], 2);
    out[p] = strtol(tmp, NULL, 16);
  }
  return true;
}

char* TheengsUtils::ip2CharArray(IPAddress ip) {
  static char ipChar[16];
  sprintf(ipChar, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return ipChar;
}

bool TheengsUtils::to_bool(String const& s) {
  return s != "0" && s != "false" && s != "False" && s != "FALSE";
}