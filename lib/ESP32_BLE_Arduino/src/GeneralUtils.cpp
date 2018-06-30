/*
 * GeneralUtils.cpp
 *
 *  Created on: May 20, 2017
 *      Author: kolban
 */

#include "GeneralUtils.h"
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <FreeRTOS.h>
#include <esp_err.h>
#include <nvs.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

static const char* LOG_TAG = "GeneralUtils";

static const char kBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static int base64EncodedLength(size_t length) {
	return (length + 2 - ((length + 2) % 3)) / 3 * 4;
} // base64EncodedLength


static int base64EncodedLength(const std::string &in) {
	return base64EncodedLength(in.length());
} // base64EncodedLength


static void a3_to_a4(unsigned char * a4, unsigned char * a3) {
	a4[0] = (a3[0] & 0xfc) >> 2;
	a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
	a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
	a4[3] = (a3[2] & 0x3f);
} // a3_to_a4


static void a4_to_a3(unsigned char * a3, unsigned char * a4) {
	a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
	a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
	a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
} // a4_to_a3


/**
 * @brief Encode a string into base 64.
 * @param [in] in
 * @param [out] out
 */
bool GeneralUtils::base64Encode(const std::string &in, std::string *out) {
	int i = 0, j = 0;
	size_t enc_len = 0;
	unsigned char a3[3];
	unsigned char a4[4];

	out->resize(base64EncodedLength(in));

	int input_len = in.size();
	std::string::const_iterator input = in.begin();

	while (input_len--) {
		a3[i++] = *(input++);
		if (i == 3) {
			a3_to_a4(a4, a3);

			for (i = 0; i < 4; i++) {
				(*out)[enc_len++] = kBase64Alphabet[a4[i]];
			}

			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 3; j++) {
			a3[j] = '\0';
		}

		a3_to_a4(a4, a3);

		for (j = 0; j < i + 1; j++) {
			(*out)[enc_len++] = kBase64Alphabet[a4[j]];
		}

		while ((i++ < 3)) {
			(*out)[enc_len++] = '=';
		}
	}

	return (enc_len == out->size());
} // base64Encode


/**
 * @brief Dump general info to the log.
 * Data includes:
 * * Amount of free RAM
 */
void GeneralUtils::dumpInfo() {
	size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	esp_chip_info_t chipInfo;
	esp_chip_info(&chipInfo);
	ESP_LOGD(LOG_TAG, "--- dumpInfo ---");
	ESP_LOGD(LOG_TAG, "Free heap: %d", freeHeap);
	ESP_LOGD(LOG_TAG, "Chip Info: Model: %d, cores: %d, revision: %d", chipInfo.model, chipInfo.cores, chipInfo.revision);
	ESP_LOGD(LOG_TAG, "ESP-IDF version: %s", esp_get_idf_version());
	ESP_LOGD(LOG_TAG, "---");
} // dumpInfo


/**
 * @brief Does the string end with a specific character?
 * @param [in] str The string to examine.
 * @param [in] c The character to look form.
 * @return True if the string ends with the given character.
 */
bool GeneralUtils::endsWith(std::string str, char c) {
	if (str.empty()) {
		return false;
	}
	if (str.at(str.length()-1) == c) {
		return true;
	}
	return false;
} // endsWidth


static int DecodedLength(const std::string &in) {
	int numEq = 0;
	int n = in.size();

	for (std::string::const_reverse_iterator it = in.rbegin(); *it == '='; ++it) {
		++numEq;
	}
	return ((6 * n) / 8) - numEq;
} // DecodedLength


static unsigned char b64_lookup(unsigned char c) {
	if(c >='A' && c <='Z') return c - 'A';
	if(c >='a' && c <='z') return c - 71;
	if(c >='0' && c <='9') return c + 4;
	if(c == '+') return 62;
	if(c == '/') return 63;
	return 255;
}; // b64_lookup


/**
 * @brief Decode a chunk of data that is base64 encoded.
 * @param [in] in The string to be decoded.
 * @param [out] out The resulting data.
 */
bool GeneralUtils::base64Decode(const std::string &in, std::string *out) {
	int i = 0, j = 0;
	size_t dec_len = 0;
	unsigned char a3[3];
	unsigned char a4[4];

	int input_len = in.size();
	std::string::const_iterator input = in.begin();

	out->resize(DecodedLength(in));

	while (input_len--) {
		if (*input == '=') {
			break;
		}

		a4[i++] = *(input++);
		if (i == 4) {
			for (i = 0; i <4; i++) {
				a4[i] = b64_lookup(a4[i]);
			}

			a4_to_a3(a3,a4);

			for (i = 0; i < 3; i++) {
				(*out)[dec_len++] = a3[i];
			}

			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++) {
			a4[j] = '\0';
		}

		for (j = 0; j < 4; j++) {
			a4[j] = b64_lookup(a4[j]);
		}

		a4_to_a3(a3,a4);

		for (j = 0; j < i - 1; j++) {
			(*out)[dec_len++] = a3[j];
		}
	}

	return (dec_len == out->size());
 } // base64Decode

/*
void GeneralUtils::hexDump(uint8_t* pData, uint32_t length) {
	uint32_t index=0;
	std::stringstream ascii;
	std::stringstream hex;
	char asciiBuf[80];
	char hexBuf[80];
	hex.str("");
	ascii.str("");
	while(index < length) {
		hex << std::setfill('0') << std::setw(2) << std::hex << (int)pData[index] << ' ';
		if (std::isprint(pData[index])) {
			ascii << pData[index];
		} else {
			ascii << '.';
		}
		index++;
		if (index % 16 == 0) {
			strcpy(hexBuf, hex.str().c_str());
			strcpy(asciiBuf, ascii.str().c_str());
			ESP_LOGD(tag, "%s %s", hexBuf, asciiBuf);
			hex.str("");
			ascii.str("");
		}
	}
	if (index %16 != 0) {
		while(index % 16 != 0) {
			hex << "   ";
			index++;
		}
		strcpy(hexBuf, hex.str().c_str());
		strcpy(asciiBuf, ascii.str().c_str());
		ESP_LOGD(tag, "%s %s", hexBuf, asciiBuf);
		//ESP_LOGD(tag, "%s %s", hex.str().c_str(), ascii.str().c_str());
	}
	FreeRTOS::sleep(1000);
}
*/

/*
void GeneralUtils::hexDump(uint8_t* pData, uint32_t length) {
	uint32_t index=0;
	static std::stringstream ascii;
	static std::stringstream hex;
	hex.str("");
	ascii.str("");
	while(index < length) {
		hex << std::setfill('0') << std::setw(2) << std::hex << (int)pData[index] << ' ';
		if (std::isprint(pData[index])) {
			ascii << pData[index];
		} else {
			ascii << '.';
		}
		index++;
		if (index % 16 == 0) {
			ESP_LOGD(tag, "%s %s", hex.str().c_str(), ascii.str().c_str());
			hex.str("");
			ascii.str("");
		}
	}
	if (index %16 != 0) {
		while(index % 16 != 0) {
			hex << "   ";
			index++;
		}
		ESP_LOGD(tag, "%s %s", hex.str().c_str(), ascii.str().c_str());
	}
	FreeRTOS::sleep(1000);
}
*/


/**
 * @brief Dump a representation of binary data to the console.
 *
 * @param [in] pData Pointer to the start of data to be logged.
 * @param [in] length Length of the data (in bytes) to be logged.
 * @return N/A.
 */
void GeneralUtils::hexDump(const uint8_t* pData, uint32_t length) {
	char ascii[80];
	char hex[80];
	char tempBuf[80];
	uint32_t lineNumber = 0;

	ESP_LOGD(LOG_TAG, "     00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f  ----------------");
	strcpy(ascii, "");
	strcpy(hex, "");
	uint32_t index=0;
	while(index < length) {
		sprintf(tempBuf, "%.2x ", pData[index]);
		strcat(hex, tempBuf);
		if (isprint(pData[index])) {
			sprintf(tempBuf, "%c", pData[index]);
		} else {
			sprintf(tempBuf, ".");
		}
		strcat(ascii, tempBuf);
		index++;
		if (index % 16 == 0) {
			ESP_LOGD(LOG_TAG, "%.4x %s %s", lineNumber*16, hex, ascii);
			strcpy(ascii, "");
			strcpy(hex, "");
			lineNumber++;
		}
	}
	if (index %16 != 0) {
		while(index % 16 != 0) {
			strcat(hex, "   ");
			index++;
		}
		ESP_LOGD(LOG_TAG, "%.4x %s %s", lineNumber*16, hex, ascii);
	}
} // hexDump


/**
 * @brief Convert an IP address to string.
 * @param ip The 4 byte IP address.
 * @return A string representation of the IP address.
 */
std::string GeneralUtils::ipToString(uint8_t *ip) {
	std::stringstream s;
	s << (int)ip[0] << '.' << (int)ip[1] << '.' << (int)ip[2] << '.' << (int)ip[3];
	return s.str();
} // ipToString


/**
 * @brief Split a string into parts based on a delimiter.
 * @param [in] source The source string to split.
 * @param [in] delimiter The delimiter characters.
 * @return A vector of strings that are the split of the input.
 */
std::vector<std::string> GeneralUtils::split(std::string source, char delimiter) {
	// See also: https://stackoverflow.com/questions/5167625/splitting-a-c-stdstring-using-tokens-e-g
	std::vector<std::string> strings;
	std::istringstream iss(source);
	std::string s;
	while(std::getline(iss, s, delimiter)) {
		strings.push_back(trim(s));
	}
	return strings;
} // split


/**
 * @brief Convert an ESP error code to a string.
 * @param [in] errCode The errCode to be converted.
 * @return A string representation of the error code.
 */
const char* GeneralUtils::errorToString(esp_err_t errCode) {
	switch(errCode) {
		case ESP_OK:
			return "OK";
		case ESP_FAIL:
			return "Fail";
		case ESP_ERR_NO_MEM:
			return "No memory";
		case ESP_ERR_INVALID_ARG:
			return "Invalid argument";
		case ESP_ERR_INVALID_SIZE:
			return "Invalid state";
		case ESP_ERR_INVALID_STATE:
			return "Invalid state";
		case ESP_ERR_NOT_FOUND:
			return "Not found";
		case ESP_ERR_NOT_SUPPORTED:
			return "Not supported";
		case ESP_ERR_TIMEOUT:
			return "Timeout";
		case ESP_ERR_NVS_NOT_INITIALIZED:
			return "ESP_ERR_NVS_NOT_INITIALIZED";
		case ESP_ERR_NVS_NOT_FOUND:
			return "ESP_ERR_NVS_NOT_FOUND";
		case ESP_ERR_NVS_TYPE_MISMATCH:
			return "ESP_ERR_NVS_TYPE_MISMATCH";
		case ESP_ERR_NVS_READ_ONLY:
			return "ESP_ERR_NVS_READ_ONLY";
		case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
			return "ESP_ERR_NVS_NOT_ENOUGH_SPACE";
		case ESP_ERR_NVS_INVALID_NAME:
			return "ESP_ERR_NVS_INVALID_NAME";
		case ESP_ERR_NVS_INVALID_HANDLE:
			return "ESP_ERR_NVS_INVALID_HANDLE";
		case ESP_ERR_NVS_REMOVE_FAILED:
			return "ESP_ERR_NVS_REMOVE_FAILED";
		case ESP_ERR_NVS_KEY_TOO_LONG:
			return "ESP_ERR_NVS_KEY_TOO_LONG";
		case ESP_ERR_NVS_PAGE_FULL:
			return "ESP_ERR_NVS_PAGE_FULL";
		case ESP_ERR_NVS_INVALID_STATE:
			return "ESP_ERR_NVS_INVALID_STATE";
		case ESP_ERR_NVS_INVALID_LENGTH:
			return "ESP_ERR_NVS_INVALID_LENGTH";
		case ESP_ERR_WIFI_NOT_INIT:
			return "ESP_ERR_WIFI_NOT_INIT";
		//case ESP_ERR_WIFI_NOT_START:
		//	return "ESP_ERR_WIFI_NOT_START";
		case ESP_ERR_WIFI_IF:
			return "ESP_ERR_WIFI_IF";
		case ESP_ERR_WIFI_MODE:
			return "ESP_ERR_WIFI_MODE";
		case ESP_ERR_WIFI_STATE:
			return "ESP_ERR_WIFI_STATE";
		case ESP_ERR_WIFI_CONN:
			return "ESP_ERR_WIFI_CONN";
		case ESP_ERR_WIFI_NVS:
			return "ESP_ERR_WIFI_NVS";
		case ESP_ERR_WIFI_MAC:
			return "ESP_ERR_WIFI_MAC";
		case ESP_ERR_WIFI_SSID:
			return "ESP_ERR_WIFI_SSID";
		case ESP_ERR_WIFI_PASSWORD:
			return "ESP_ERR_WIFI_PASSWORD";
		case ESP_ERR_WIFI_TIMEOUT:
			return "ESP_ERR_WIFI_TIMEOUT";
		case ESP_ERR_WIFI_WAKE_FAIL:
			return "ESP_ERR_WIFI_WAKE_FAIL";
		}
	return "Unknown ESP_ERR error";
} // errorToString


/**
 * @brief Convert a string to lower case.
 * @param [in] value The string to convert to lower case.
 * @return A lower case representation of the string.
 */
std::string GeneralUtils::toLower(std::string& value) {
	// Question: Could this be improved with a signature of:
	// std::string& GeneralUtils::toLower(std::string& value)
	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
	return value;
} // toLower


/**
 * @brief Remove white space from a string.
 */
std::string GeneralUtils::trim(const std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
} // trim


