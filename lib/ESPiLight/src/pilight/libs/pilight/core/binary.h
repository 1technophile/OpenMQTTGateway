/*
	Copyright (C) 2013 - 2014 CurlyMo

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#ifndef _BINARY_H_
#define _BINARY_H_

/*
 * Convert "bits" to the corresponding integer value.
 * The difference between binToDecRev[Ul]() and binToDec[Ul]() is where the most and where the least
 * significant bit is located (at index position s or e).
 * @param binary The bits to convert, each represented by an int in a buffer (non-zero = "1").
 * @param s, e Start+End index in the buffer of "bits". 0<=s<=e. e-s < sizeof(int)*8
 * @return int The converted value.
 */
int binToDecRev(const int *binary, int s, int e);	// 0<=s<=e, binary[s(msb) .. e(lsb)]
int binToDec(const int *binary, int s, int e);		// 0<=s<=e, binary[s(lsb) .. e(msb)]

/*
 * Convert an integer value to its bits, stored in a buffer of ints.
 * The difference of decToBinRev() and decToBin() is if Most or Least Significant bit is generated first.
 * IMPORTANT: binToDec() devToBin() use the "Rev"/"non-Rev" in opposit meaning: A buffer generated
 * using the "Rev" function must be read by the "non-Rev" function to produce the original value. :-(
 * @param dec The number to convert.
 * @param binary The buffer of int where to store the "bits" (0 and 1).
 * @return int The index of the last bit generated (0 if one bit was generated).
 */
int decToBinRev(int dec, int *binary);	// stores dec as binary[lsb .. msb] and return index of msb
int decToBin(int dec, int *binary);	// stores dec as binary[msb .. lsb] and return index of lsb

/*
 * Dito for unsigned long long values.
 */
unsigned long long binToDecRevUl(const int *binary, unsigned int s, unsigned int e);
unsigned long long binToDecUl(const int *binary, unsigned int s, unsigned int e);
int decToBinUl(unsigned long long n, int *binary);
int decToBinRevUl(unsigned long long n, int *binary);

/*
 * Convert "bits" to the corresponding signed integer value.
 * The difference between binToSignedRev() and binToSigned() is where the most and where the least
 * significant bit is located (at index position s or e).
 * @param binary The bits to convert, each represented by an int in a buffer (non-zero = "1").
 * @param s, e Start+End index in the buffer of "bits". 0<=s<=e. e-s < sizeof(int)*8
 * @return signed int The converted value.
 *
 * Examples:
 *  binToSignedRev((int[]){0,1,1,1, 1,1,1,1}, 0, 7) == 127
 *  binToSignedRev((int[]){0,1,1,1, 1,1,1,0}, 0, 7) == 126
 *  binToSignedRev((int[]){0,0,0,0, 0,0,1,0}, 0, 7) == 2
 *  binToSignedRev((int[]){0,0,0,0, 0,0,0,1}, 0, 7) == 1
 *  binToSignedRev((int[]){0,0,0,0, 0,0,0,0}, 0, 7) == 0
 *  binToSignedRev((int[]){1,1,1,1, 1,1,1,1}, 0, 7) == -1
 *  binToSignedRev((int[]){1,1,1,1, 1,1,1,0}, 0, 7) == -2
 *  binToSignedRev((int[]){1,0,0,0, 0,0,1,0}, 0, 7) == -126
 *  binToSignedRev((int[]){1,0,0,0, 0,0,0,1}, 0, 7) == -127
 *  binToSignedRev((int[]){1,0,0,0, 0,0,0,0}, 0, 7) == -128
 *  binToSignedRev((int[]){1,0, 0,1,1,1, 0,0}, 2, 5) == 7
 *  binToSignedRev((int[]){1,0, 1,1,1,0, 0,0}, 2, 5) == -2
 *
 *  binToSigned((int[]){1,1,1,1, 1,1,1,0}, 0, 7) == 127
 *  binToSigned((int[]){0,1,1,1, 1,1,1,0}, 0, 7) == 126
 *  binToSigned((int[]){0,1,0,0, 0,0,0,0}, 0, 7) == 2
 *  binToSigned((int[]){1,0,0,0, 0,0,0,0}, 0, 7) == 1
 *  binToSigned((int[]){0,0,0,0, 0,0,0,0}, 0, 7) == 0
 *  binToSigned((int[]){1,1,1,1, 1,1,1,1}, 0, 7) == -1
 *  binToSigned((int[]){0,1,1,1, 1,1,1,1}, 0, 7) == -2
 *  binToSigned((int[]){0,1,0,0, 0,0,0,1}, 0, 7) == -126
 *  binToSigned((int[]){1,0,0,0, 0,0,0,1}, 0, 7) == -127
 *  binToSigned((int[]){0,0,0,0, 0,0,0,1}, 0, 7) == -128
 *  binToSigned((int[]){1,0, 1,1,1,0, 0,0}, 2, 5) == 7
 *  binToSigned((int[]){1,0, 0,1,1,1, 0,0}, 2, 5) == -2
 */
int binToSignedRev(const int *binary, int s, int e);    // 0<=s<=e, binary[s(msb) .. e(lsb)]
int binToSigned(const int *binary, int s, int e);       // 0<=s<=e, binary[s(lsb) .. e(msb)]

#endif
