/*
	Copyright (C) 2015 CurlyMoo & dominikkarall

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "quigg_gt9000.h"

#define PULSE_QUIGG_SHORT	500
#define PULSE_QUIGG_LONG	1100
#define PULSE_QUIGG_FOOTER1	3000
#define PULSE_QUIGG_FOOTER2	7000

#define NORMAL_REPEATS		4
#define AVG_PULSE_LENGTH	750
#define RAW_LENGTH		50

/* Encoding details:
	Bit
	0-3      First part of systemcode
	4-19     Encrypted systemcode
	16-19    ON/OFF statecodes (in encoded state, is also used decoded for systemcode)
		 example based on 1110 firt part systemcode
			ON2/OFF4        - 0000
			OFF1/ON3/ONALL  - 1000
			OFF2/ON4        - 0100
			OFF2/ON4        - 1100
			OFF1/ON3/ONALL  - 0010
			ON1/OFF3/OFFALL - 1010
			ON1/OFF3/OFFALL - 0110
			ON2/OFF4        - 1110
			ON1/OFF3/OFFALL - 0001
			OFF2/ON4        - 1001
			ON1/OFF3/OFFALL - 0101
			OFF2/ON4        - 1101
			ON2/OFF4        - 0011
			OFF1/ON3/ONALL  - 1011
			OFF1/ON3/ONALL  - 0111
			ON2/OFF4        - 1111
	20-24    Unit
	25       Footer (3000 7000)
*/

char gt9000_unit_offon_map[16][2][4] = {{{8,2,11,7},{10,6,1,5}},   //unit 0 (untested)
         {{8,2,11,7},{10,6,1,5}},   //unit 1 (untested)
         {{8,2,11,7},{10,6,1,5}},   //unit 2 (working)
         {{8,2,11,7},{10,6,1,5}},   //unit 3
         {{10,6,1,5},{8,2,11,7}},   //unit 4 (ALL-working)
         {{10,6,1,5},{8,2,11,7}},   //unit 5 (working)
         {{10,6,1,5},{8,2,11,7}},   //unit 6 (working)
         {{10,6,1,5},{8,2,11,7}},   //unit 7
         {{4,9,12,13},{0,3,14,15}}, //unit 8 (working)
         {{4,9,12,13},{0,3,14,15}}, //unit 9 (untested)
         {{4,9,12,13},{0,3,14,15}}, //unit 10
         {{4,9,12,13},{0,3,14,15}}, //unit 11 (untested)
         {{0,3,14,15},{4,9,12,13}}, //unit 12 (untested)
         {{0,3,14,15},{4,9,12,13}}, //unit 13 (untested)
         {{0,3,14,15},{4,9,12,13}}, //unit 14
         {{0,3,14,15},{4,9,12,13}}};//unit 15 (untested)
char gt9000_unit_offon_map2[16][2][4] = {{{1,2,9,10},{3,4,7,11}},  //unit 0
         {{1,2,9,10},{3,4,7,11}},   //unit 1 (untested)
         {{1,2,9,10},{3,4,7,11}},   //unit 2
         {{1,2,9,10},{3,4,7,11}},   //unit 3
         {{3,4,7,11},{1,2,9,10}},   //unit 4
         {{3,4,7,11},{1,2,9,10}},   //unit 5 (ALL-working)
         {{3,4,7,11},{1,2,9,10}},   //unit 6 (untested)
         {{3,4,7,11},{1,2,9,10}},   //unit 7 (untested)
         {{-1,-1,-1,-1},{-1,-1,-1,-1}},
         {{-1,-1,-1,-1},{-1,-1,-1,-1}},
         {{-1,-1,-1,-1},{-1,-1,-1,-1}},
         {{-1,-1,-1,-1},{-1,-1,-1,-1}},
         {{-1,-1,-1,-1},{-1,-1,-1,-1}},
         {{-1,-1,-1,-1},{-1,-1,-1,-1}},
         {{-1,-1,-1,-1},{-1,-1,-1,-1}},
         {{-1,-1,-1,-1},{-1,-1,-1,-1}}};

int gt9000_hash[16] = { 0x0, 0x9, 0xF, 0x4, 0xA, 0xD, 0x5, 0xB,
			0x3, 0x2, 0x1, 0x7, 0xE, 0x6, 0xC, 0x8 };
int gt9000_hash2[16] = { 0x0, 0x9, 0x5, 0xF, 0x3, 0x6, 0xC, 0x7,
			 0xE, 0xD, 0x1, 0xB, 0x2, 0xA, 0x4, 0x8 };

static int isSyscodeType1(int syscodetype) {
	if(syscodetype & 0x13)
		return 1;

	return 0;
}

static int validate(void) {
	if(quigg_gt9000->rawlen == RAW_LENGTH) {
		if(quigg_gt9000->raw[quigg_gt9000->rawlen-1] >= (int)(PULSE_QUIGG_FOOTER2*0.9) &&
		   quigg_gt9000->raw[quigg_gt9000->rawlen-1] <= (int)(PULSE_QUIGG_FOOTER2*1.1) &&
		   quigg_gt9000->raw[quigg_gt9000->rawlen-2] >= (int)(PULSE_QUIGG_FOOTER1*0.9) &&
		   quigg_gt9000->raw[quigg_gt9000->rawlen-2] <= (int)(PULSE_QUIGG_FOOTER1*1.1)) {
			return 0;
		}
	}
	return -1;
}

static void createMessage(int *binary, int systemcode, int state, int unit) {
	int i = 0;
	char binaryCh[RAW_LENGTH/2];
	quigg_gt9000->message = json_mkobject();
	if(binary != NULL) {
        	for(i=0;i<RAW_LENGTH/2;i++) {
                	if(binary[i] == 0) {
                		binaryCh[i] = '0';
                	} else {
                		binaryCh[i] = '1';
                	}
        	}
        	binaryCh[RAW_LENGTH/2-1] = '\0';
        	json_append_member(quigg_gt9000->message, "binary", json_mkstring(binaryCh));
        }
	json_append_member(quigg_gt9000->message, "id", json_mknumber(systemcode, 0));
	json_append_member(quigg_gt9000->message, "unit", json_mknumber(unit, 0));
	if(state == 1) {
		json_append_member(quigg_gt9000->message, "state", json_mkstring("on"));
	} else {
		json_append_member(quigg_gt9000->message, "state", json_mkstring("off"));
	}
}

static int decodePayload(int payload, int index, int syscodetype) {
	int ret = -1;

	if(isSyscodeType1(syscodetype))
		ret = payload^gt9000_hash[index];
	else
		ret = payload^gt9000_hash2[index];

	return ret;
}

static int parseSystemcode(int *binary) {
	int systemcode1dec = binToDecRev(binary, 0, 3);
	int systemcode2enc = binToDecRev(binary, 4, 7);
	int systemcode2dec = 0; //calculate all codes with base syscode2 = 0
	int systemcode3enc = binToDecRev(binary, 8, 11);
	int systemcode3dec = decodePayload(systemcode3enc, systemcode2enc, systemcode1dec);
	int systemcode4enc = binToDecRev(binary, 12, 15);
	int systemcode4dec = decodePayload(systemcode4enc, systemcode3enc, systemcode1dec);
	int systemcode5enc = binToDecRev(binary, 16, 19);
	int systemcode5dec = decodePayload(systemcode5enc, systemcode4enc, systemcode1dec);
	int systemcode = (systemcode1dec<<16) + (systemcode2dec<<12) + (systemcode3dec<<8) + (systemcode4dec<<4) + systemcode5dec;

	return systemcode;
}

static void pulseToBinary(int *binary) {
	int x = 0;
	for(x=0; x<quigg_gt9000->rawlen-1; x+=2) {
		if(quigg_gt9000->raw[x+1] > AVG_PULSE_LENGTH) {
  			binary[x/2] = 0;
		} else {
  			binary[x/2] = 1;
		}
	}
}

static void parseCode(void) {
	int binary[RAW_LENGTH/2], state = 0;
  	int i = 0;

	pulseToBinary(binary);

  	int syscodetype = binToDecRev(binary, 0, 3);
	int systemcode = parseSystemcode(binary);
	int statecode = binToDecRev(binary, 16, 19);
	int unit = binToDec(binary, 20, 23);

	//validate unit & statecode
	if(isSyscodeType1(syscodetype)) {
		for(i=0;i<4;i++) {
			if(statecode == gt9000_unit_offon_map[unit][1][i]) {
				state = 1;
			}
		}
	} else {
		for(i=0;i<4;i++) {
			if(statecode == gt9000_unit_offon_map2[unit][1][i]) {
				state = 1;
			}
		}
	}

	createMessage(binary, systemcode, state, unit);
}

static void createZero(int s, int e) {
	int i;
	for(i=s;i<=e;i+=2) {
		quigg_gt9000->raw[i] = PULSE_QUIGG_SHORT;
		quigg_gt9000->raw[i+1] = PULSE_QUIGG_LONG;
	}
}

static void createOne(int s, int e) {
	int i;
	for(i=s;i<=e;i+=2) {
		quigg_gt9000->raw[i] = PULSE_QUIGG_LONG;
		quigg_gt9000->raw[i+1] = PULSE_QUIGG_SHORT;
	}
}

static void createFooter(void) {
	quigg_gt9000->raw[quigg_gt9000->rawlen-2] = PULSE_QUIGG_FOOTER1;
	quigg_gt9000->raw[quigg_gt9000->rawlen-1] = PULSE_QUIGG_FOOTER2;
}

static void clearCode(void) {
	createZero(0, quigg_gt9000->rawlen-3);
}

static void createEncryptedData(int encrypteddata) {
	int binary[20], length = 0, i = 0, x = 0;

	length = decToBin(encrypteddata, binary);
	for(i=0;i<=length;i++) {
		x = (i+19-length)*2;
		if(binary[i] == 1) {
			createOne(x, x+1);
		}
	}
}

static void createUnit(int unit) {
	int binary[4], length = 0, i = 0, x = 20;

	length = decToBinRev(unit, binary);
	for(i=0;i<=length;i++) {
		x = i*2 + 20*2;
		if(binary[i] == 1) {
			createOne(x, x+1);
		}
	}
}

static void initAllCodes(int systemcode, int allcodes[16]) {
	int i = 0, syscodetype = 0;
	int systemcode1enc = 0, systemcode2enc = 0, systemcode3enc = 0, systemcode4enc = 0, systemcode5enc = 0;
	int systemcode1dec = 0, systemcode3dec = 0, systemcode4dec = 0, systemcode5dec = 0;

	syscodetype = (systemcode >> 16) & 0xF;
	systemcode1dec = (systemcode >> 16) & 0xF;
	//systemcode2dec is always 0, therefore it is not needed
	//systemcode2dec = (systemcode >> 12) & 0xF;
	systemcode3dec = (systemcode >> 8) & 0xF;
	systemcode4dec = (systemcode >> 4) & 0xF;
	systemcode5dec = systemcode & 0xF;

	//first 4 bits are not encrypted
	systemcode1enc = systemcode1dec;

	//encrypt systemcode
	for(i=0;i<16;i++) {
		systemcode2enc = i;
		if(isSyscodeType1(syscodetype)) {
			systemcode3enc = gt9000_hash[systemcode2enc]^systemcode3dec;
			systemcode4enc = gt9000_hash[systemcode3enc]^systemcode4dec;
			systemcode5enc = gt9000_hash[systemcode4enc]^systemcode5dec;
		} else { //if(systemcodetype == 13 || systemcodetype == 12)
			systemcode3enc = gt9000_hash2[systemcode2enc]^systemcode3dec;
			systemcode4enc = gt9000_hash2[systemcode3enc]^systemcode4dec;
			systemcode5enc = gt9000_hash2[systemcode4enc]^systemcode5dec;
		}
		allcodes[systemcode5enc] = (systemcode1enc<<16) + (systemcode2enc<<12) + (systemcode3enc<<8) + (systemcode4enc<<4) + systemcode5enc;
	}
}

static int createCode(JsonNode *code) {
	int syscodetype = 0;
	double itmp = -1;
	int unit = -1, systemcode = -1, verifysyscode = -1, state = -1, all = 0, statecode = -1;
	int allcodes[16], binary[RAW_LENGTH/2];

	if(json_find_number(code, "id", &itmp) == 0)
		systemcode = (int)round(itmp);
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if((systemcode == -1) || (unit == -1 && all == 0)) {
		logprintf(LOG_ERR, "quigg_gt9000: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(systemcode == -1) {
		logprintf(LOG_ERR, "quigg_gt9000: invalid id range");
		return EXIT_FAILURE;
	} else if(unit < 0) {
		logprintf(LOG_ERR, "quigg_gt9000: invalid unit code range");
		return EXIT_FAILURE;
	} else {
		quigg_gt9000->rawlen = RAW_LENGTH;
		//create all 16 codes used by the remote
		initAllCodes(systemcode, allcodes);
		//it is possible to use 4 codes per state
		//we stick to code number 1 in the on/off array
		syscodetype = (systemcode >> 16) & 0xF;
		if(isSyscodeType1(syscodetype))
			statecode = gt9000_unit_offon_map[unit][state][1];
		else
			statecode = gt9000_unit_offon_map2[unit][state][1];

		if(statecode==-1) {
			logprintf(LOG_ERR, "quigg_gt9000: unit %d not supported, try 0-15.", unit);
			return EXIT_FAILURE;
		}

		int encrypteddata = allcodes[statecode];

		clearCode();
		createEncryptedData(encrypteddata);
		createUnit(unit);
		createFooter();

		pulseToBinary(binary);
		verifysyscode = parseSystemcode(binary);
		if(verifysyscode != systemcode) {
			logprintf(LOG_ERR, "quigg_gt9000: invalid id, try %d", verifysyscode);
			return EXIT_FAILURE;
		}

		createMessage(NULL, systemcode, state, unit);
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -u --unit=unit\t\t\tcontrol the device unit with this code\n");
	printf("\t -t --on\t\t\tsend an on signal to device\n");
	printf("\t -f --off\t\t\tsend an off signal to device\n");
	printf("\t -i --id=id\t\t\tcontrol one or multiple devices with this id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void quiggGT9000Init(void) {

	protocol_register(&quigg_gt9000);
	protocol_set_id(quigg_gt9000, "quigg_gt9000");
	protocol_device_add(quigg_gt9000, "quigg_gt9000", "Quigg GT-9000 remote with GT-FSi-06 switches");
	quigg_gt9000->devtype = SWITCH;
	quigg_gt9000->hwtype = RF433;
	quigg_gt9000->txrpt = NORMAL_REPEATS;
	quigg_gt9000->minrawlen = RAW_LENGTH;
	quigg_gt9000->maxrawlen = RAW_LENGTH;
	quigg_gt9000->maxgaplen = (int)(PULSE_QUIGG_FOOTER2*1.1);
	quigg_gt9000->mingaplen = (int)(PULSE_QUIGG_FOOTER2*0.9);

	options_add(&quigg_gt9000->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&quigg_gt9000->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&quigg_gt9000->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);
	options_add(&quigg_gt9000->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);

	quigg_gt9000->parseCode=&parseCode;
	quigg_gt9000->createCode=&createCode;
	quigg_gt9000->printHelp=&printHelp;
	quigg_gt9000->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "quigg_gt9000";
	module->version = "1.3";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	quiggGT9000Init();
}
#endif
