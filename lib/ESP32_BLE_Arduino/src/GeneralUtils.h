/*
 * GeneralUtils.h
 *
 *  Created on: May 20, 2017
 *      Author: kolban
 */

#ifndef COMPONENTS_CPP_UTILS_GENERALUTILS_H_
#define COMPONENTS_CPP_UTILS_GENERALUTILS_H_
#include <stdint.h>
#include <string>
#include <esp_err.h>

/**
 * @brief General utilities.
 */
class GeneralUtils {
public:
	static bool        base64Decode(const std::string& in, std::string* out);
	static bool        base64Encode(const std::string& in, std::string* out);
	static bool        endsWith(std::string str, char c);
	static const char* errorToString(esp_err_t errCode);
	static void        hexDump(const uint8_t* pData, uint32_t length);
	static std::string ipToString(uint8_t* ip);
	static void        restart();
};

#endif /* COMPONENTS_CPP_UTILS_GENERALUTILS_H_ */
