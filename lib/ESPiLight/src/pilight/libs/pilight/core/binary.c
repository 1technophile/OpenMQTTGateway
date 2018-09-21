/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>

#include "binary.h"

/*
 * Macros to help convertion of a value from and to bits.
 * The value-variable passed to VALUE_TO_BITS_MSB_FIRST()
 * and VALUE_TO_BITS_LSB_FIRST() MUST be an unsigned type!
 * Macros have side effects to almost all variables passed!
 */

#define BITS_LSB_FIRST_TO_VALUE(bits, s, e, result)	\
	unsigned long long mask = 1;			\
	result = 0;					\
	for(; s<=e; mask <<= 1)				\
		if(bits[s++] != 0)			\
			result |= mask

#define BITS_MSB_FIRST_TO_VALUE(bits, s, e, result)	\
	unsigned long long mask = 1;			\
	result = 0;					\
	e++;						\
	for(; e > 0 && s<e; mask <<= 1)			\
		if(bits[--e] != 0)			\
			result |= mask

#define VALUE_TO_BITS_MSB_FIRST(value, bits, length)	\
	unsigned long long mask = value;			\
	do bits++; while (mask >>= 1);			\
	int *start = bits;				\
	do *--start = value & 1; while (value >>= 1);	\
	length = bits - start


#define VALUE_TO_BITS_LSB_FIRST(value, bits, length)	\
	int *start = bits;				\
	do *bits++ = value & 1; while (value >>= 1);	\
	length = bits - start

/*
 * Invocation of above macros for various types:
 * Again:
 * The value-variable passed to VALUE_TO_BITS_MSB_FIRST()
 * and VALUE_TO_BITS_LSB_FIRST() MUST be an unsigned type!
 */
int binToDecRev(const int *binary, int s, int e) { //  0<=s<=e, binary[s(msb) .. e(lsb)]
	int result;
	BITS_MSB_FIRST_TO_VALUE(binary, s, e, result);
	return result;
}

int binToDec(const int *binary, int s, int e) { //  0<=s<=e, binary[s(lsb) .. e(msb)]
	int result;
	BITS_LSB_FIRST_TO_VALUE(binary, s, e, result);
	return result;
}

int decToBin(int dec, int *binary) {  // stores dec as binary[msb .. lsb] and return index of lsb
	unsigned int n = (unsigned int) dec;
	int len;
	VALUE_TO_BITS_MSB_FIRST(n, binary, len);
	return len - 1; // return index, not count.
}

int decToBinRev(int dec, int *binary) { // stores dec as binary[lsb .. msb] and return index of msb
	unsigned int n = (unsigned int) dec;
	int len;
	VALUE_TO_BITS_LSB_FIRST(n, binary, len);
	return len - 1; // return index, not count.
}

unsigned long long binToDecRevUl(const int *binary, unsigned int s, unsigned int e) {
	unsigned long long result = 0;
	BITS_MSB_FIRST_TO_VALUE(binary, s, e, result);
	return result;
}

unsigned long long binToDecUl(const int *binary, unsigned int s, unsigned int e) {
	unsigned long long result;
	BITS_LSB_FIRST_TO_VALUE(binary, s, e, result);
	return result;
}

int decToBinUl(unsigned long long n, int *binary) {
	int len;
	VALUE_TO_BITS_MSB_FIRST(n, binary, len);
	return len - 1; // return index, not count.
}

int decToBinRevUl(unsigned long long n, int *binary) {
	int len;
	VALUE_TO_BITS_LSB_FIRST(n, binary, len);
	return len - 1; // return index, not count.
}

int binToSignedRev(const int *binary, int s, int e) { //  0<=s<=e, binary[s(msb) .. e(lsb)]
	int result = binToDecRev(binary, s, e);
	if (binary[s]) {
		result -= 1<<(e-s+1);
	}
	return result;
}

int binToSigned(const int *binary, int s, int e) { //  0<=s<=e, binary[s(lsb) .. e(msb)]
	int result = binToDec(binary, s, e);
	if (binary[e]) {
		result -= 1<<(e-s+1);
	}
	return result;
}