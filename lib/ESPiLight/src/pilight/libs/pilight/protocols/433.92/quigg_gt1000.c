/*
	Copyright (C) 2015 CurlyMo & wo_rasp & RvW

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
/* Protocol quigg_gt1000 is an implementation of a protocol for the Quigg GT-FSI-08 switches
   with GT-1000 remote. It is believed that they are compatible with the Lidl and Brennenstuhl outlets
   RvW, jan 2015
   This implementation mimics the GT-1000 remote control with extensions
   These switches listen to the next command string of 24 bit:
   |-|0-----------4|5---------20|21------24|
   |S| group-id[4] | random[16] |unit-id[4]|
   |-|-------------|------------|----------|
   where:
   S=Start, a short high pulse of 350 us followed by a low of 2350 us
   group-id[4]= 4 logic bits for the group-id code, range 0..15
   unit-id[4]= 4 bits for the unit code for switches 1..4. Two different coding schemes are used
   random[16]= a 16 bit sequence where no bits seems to have a specific function. Each action
   for each switch uses 4 codes (40 in total for one group-id) that are transmitted in a rolling sequence by the RC
   Although codes are reused within a single group, for the sake of code simplicity and readability
   this implementation will not use this fact. So we will implement a table with 16*40 = 640 "random" codes

   A logic bit is encoded by the position of the high to low transition in the 1540 us bit window.
   A logic 1 therefore is encoded by a mark(high) of 1100 us followed by a space(low) of 440 us.
   A logic 0 is encoded by a mark of 330 us followed by a space of 1210 us

   The total message consists of 24 bit + start pulse, and has therefore a duration 39660 us
   There is no space between the repetition of a message, so the next message immediately follows the previous message

   To program a switch the RC uses a second type of start pulse with a mark(high) of 3000us and a space(low) of 7300us
   In order to be able to program switches from pilight, this implementation mimics this behavior by
   transmitting two normal code sequences followed by a program sequence with this long start pulse.
   This implies that the complete message in fact consists of 75 bits and a short footer.
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
#include "quigg_gt1000.h"

/* define the basic protocol Mark-Space pulse width and other values*/
#define START_MARK	350
#define START_SPACE	2340
#define PROG_MARK	3000
#define PROG_SPACE	7300
#define SHORT_MARK	330
#define LONG_SPACE	1210
#define LONG_MARK	1100
#define SHORT_SPACE	440
#define FOOTER_MARK	200

#define LEARN_REPEATS			40
#define NORMAL_REPEATS		10
#define PULSE_MULTIPLIER	2
#define MIN_PULSE_LENGTH	320
#define MAX_PULSE_LENGTH	340
#define AVG_PULSE_LENGTH	330
#define RAW_LENGTH				151
#define BIN_LENGTH				24
/*
// Support Rx
static int validate(void) {
	if(quigg_gt1000->rawlen == RAW_LENGTH) {
		if(quigg_gt1000->raw[0] >= (int)(START_MARK*0.9) &&
		   quigg_gt1000->raw[0] <= (int)(START_MARK*1.1) &&
		   quigg_gt1000->raw[1] >= (int)(START_SPACE*0.9) &&
		   quigg_gt1000->raw[1] <= (int)(START_SPACE*1.1)) {
			return 0;
		}
	}
	return -1;
}
*/
int codetab[16][40] = {
	/* the next table contains the random codes (bits 4..19) of the 4 code sequences for
	 * each off-on action for each switch 0..3 in each group-id 0..15.
	 * For each group-id a separate array is created where de codes are stored in the next order:
	 * off-codes 0..3 for switch 0, on-codes 0..3 for switch 0,
	 * off-codes 0..3 for switch 1, on-codes 0..3 for switch 1,
	 * off-codes 0..3 for switch 2, on-codes 0..3 for switch 2,
	 * off-codes 0..3 for switch 3, on-codes 0..3 for switch 3,
	 * off-codes 0..3 for all, on-codes 0..3 for all
	 * If a code is unknown 0 is substituted, f.e. for group-id 0 no codes are known, so all 40 values are 0
	 */
	/* code 0 */
	{ 0xBB42,0x4301,0x00F9,0x8EBA, 0xE4C3,0x1924,0x6CDB,0x9D57,
	  0x00F9,0x4301,0x00F9,0x8EBA, 0x9D57,0xE4C3,0x6CDB,0x1924,
	  0x8EBA,0x00F9,0x4301,0xBB42, 0x1924,0xE4C3,0x6CDB,0x9D57,
	  0xE4C3,0x1924,0x9D57,0x6CDB, 0x8EBA,0xBB42,0x4301,0x00F9,
	  0xE4C3,0x1924,0x9D57,0x6CDB, 0x4301,0x00F9,0x8EBA,0xBB42 },
	/* code 1 */
	{ 0x616A,0xCF71,0x7AE2,0xB639, 0xF59B,0xE923,0x14C4,0x32A7,
	  0x616A,0xB639,0xCF71,0x7AE2, 0x14C4,0x32A7,0xF59B,0xE923,
	  0xCF71,0x616A,0x7AE2,0xB639, 0xE923,0xF59B,0x14C4,0x32A7,
	  0x14C4,0xE923,0xF59B,0x32A7, 0x7AE2,0x616A,0xCF71,0xB639,
	  0x14C4,0xE923,0xF59B,0x32A7, 0x7AE2,0x616A,0xB639,0xCF71 },
	/* code 2 */
	{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
	/* code 3 */
	{ 0x32D7,0x7D4B,0x9482,0xC818, 0x067A,0x2901,0xB1B6,0xEA35,
	  0x5B5C,0x6364,0x85F9,0xFEED, 0x1FA0,0x4CCF,0xA793,0xD02E,
	  0x067A,0x2901,0xB1B6,0xEA35, 0x32D7,0x7D4B,0x9482,0xC818,
	  0x1FA0,0x4CCF,0xA793,0xD02E, 0x5B5C,0x6364,0x85F9,0xFEED,
	  0x067A,0x2901,0xB1B6,0xEA35, 0x32D7,0x7D4B,0x9482,0xC818 },
	/* code 4 */
	{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
	/* code 5 */
	{ 0x23C2,0x71A1,0x88DA,0xDC19, 0x1FBB,0x39E4,0xA743,0xFE77,
	  0x23C2,0x71A1,0x88DA,0xDC19, 0x1FBB,0x39E4,0xA743,0xFE77,
	  0x23C2,0x71A1,0x88DA,0xDC19, 0x1FBB,0x39E4,0xA743,0xFE77,
	  0x1FBB,0x39E4,0xA743,0xFE77, 0x23C2,0x71A1,0x88DA,0xDC19,
	  0x1FBB,0x39E4,0xA743,0xFE77, 0x23C2,0x71A1,0x88DA,0xDC19 },
	/* code 6 */
	{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
	/* code 7 */
	{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
	/* code 8 */
	{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
	/* code 9 */
	{ 0x1B1A,0x27D9,0x75C1,0xC0A2, 0x02FB,0x3D03,0x6EE7,0xB974,
	  0x1B1A,0x27D9,0x75C1,0xC0A2, 0x02FB,0x3D03,0x6EE7,0xB974,
	  0x1B1A,0x27D9,0x75C1,0xC0A2, 0x02FB,0x3D03,0x6EE7,0xB974,
	  0x02FB,0x3D03,0x6EE7,0xB974, 0x1B1A,0x27D9,0x75C1,0xC0A2,
	  0x02FB,0x3D03,0x6EE7,0xB974, 0x1B1A,0x27D9,0x75C1,0xC0A2 },
	/* code 10 */
	{ 0x9E98,0xC2AB,0xE057,0xF4F2, 0x460A,0x6971,0x77E6,0xBB25,
	  0x0CBD,0x1589,0x51C4,0x8FDC, 0x2313,0x386F,0xAD3E,0xDA40,
	  0x460A,0x6971,0x77E6,0xBB25, 0x9E98,0xC2AB,0xE057,0xF4F2,
	  0x2313,0x386F,0xAD3E,0xDA40, 0x0CBD,0x1589,0x51C4,0x8FDC,
	  0x460A,0x6971,0x77E6,0xBB25, 0x9E98,0xC2AB,0xE057,0xF4F2 },
	/* code 11 */
	{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
	/* code 12 */
	{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
	/* code 13 */
	{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
	/* code 14 */
	{ 0xE692,0x98F8,0x577B,0xABB7, 0x0AD6,0xBDA1,0xC465,0x6F4A,
	  0xF234,0x89EC,0x2519,0x715D, 0x1383,0xDC2F,0x40CE,0x3E00,
	  0x0AD6,0xBDA1,0xC465,0x6F4A, 0xE692,0x98F8,0x577B,0xABB7,
	  0x1383,0xDC2F,0x40CE,0x3E00, 0xF234,0x89EC,0x2519,0x715D,
	  0x0AD6,0xBDA1,0xC465,0x6F4A, 0xE692,0x98F8,0x577B,0xABB7 },
	/* code 15 */
	{ 0x1067,0x3D02,0x9B1B,0, 0x2636,0x6C81,0xE5B5,0xF1FA,
	  0x432D,0x54CC,0x8A79,0xC7D4, 0x7290,0xA85F,0xDFEE,0,
	  0x2636,0x6C81,0xE5B5,0xF1FA, 0x1067,0x3D02,0x9B1B,0,
	  0x7290,0xA85F,0xDFEE,0, 0x432D,0x54CC,0x8A79,0xC7D4,
	  0x2636,0x6C81,0xE5B5,0xF1FA, 0x1067,0x3D02,0x9B1B,0 }
};

int unittab[2][5] = {
	/* This table defines the coding of the unit-id or switch-id. Two coding schemes are used
	 * I call them first and second generation switches. They encode the unit-id in  a different way.
	 */
	/* first gen group-id = 1, 5, 9, 12 */
	{ 0x0, 0x4, 0xC, 0x2, 0xA }, /* these are 1st gen switch-codes for switch 0..3 and all */
	/* second gen all other */
	{ 0xC, 0x5, 0xE, 0x7, 0x2 }	/* 2nd gen switch-codes */
};

int gentab[16] =
	/* This table defines the switch generation for each group-id
	 * A 0 at location x means group-id x belongs to first generation, and a 1 means second generation
	 */
	 /* for now only group-id 1,5,9 and 12 are first generation */
	{0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1};

#define NRSUPERMASK 0x07
#define NRSUPERCODES (NRSUPERMASK+1)	/* we will use 8 on and 8 off supercodes */
/* make sure the number of Supercodes is always a power of 2, i.e. 2,4,8,16 ... */
int supercodes[2*NRSUPERCODES] = {
	/* since with supercodes group-id's are irrelevant, the codes consist of the first 20 bits of the code sequence */
	/* OFF codes */
	0x0F000,0x1F006,0x4F00E,0x5F016,0x8F01E,0x9F026,0xCF02E,0xDF02F,
	/* ON codes */
	0x0F005,0x1F008,0x4F015,0x5F018,0x8F025,0x9F028,0xCF02C,0xDF03C };

char bincode[BIN_LENGTH+1];

static void createMessage(int id, int unit, int state, int seq, int learn) {
	int i = 0;

	for(i=0;i<BIN_LENGTH;i++) {
		bincode[i] = bincode[i] | 0x30; /* convert bin 0,1 into char 0,1 */
	}

	bincode[BIN_LENGTH] = '\0'; /* end of string */

	quigg_gt1000->message = json_mkobject();
	json_append_member(quigg_gt1000->message, "id", json_mknumber(id, 0));
	json_append_member(quigg_gt1000->message, "unit", json_mknumber(unit, 0));
	json_append_member(quigg_gt1000->message, "seq", json_mknumber(seq, 0));
	if(state == 1) {
		json_append_member(quigg_gt1000->message, "state", json_mkstring("on"));
	} else {
		json_append_member(quigg_gt1000->message, "state", json_mkstring("off"));
	}
	json_append_member(quigg_gt1000->message, "code", json_mkstring(bincode));
}

static int fillLow(int idx) {
	/* fill in the mark-space code for a logic Low = short pulse*/
	quigg_gt1000->raw[idx++] = SHORT_MARK;
	quigg_gt1000->raw[idx++] = LONG_SPACE;
	return idx;
}

static int fillHigh(int idx) {
	/* fill in the mark-space code for a logic High = long pulse*/
	quigg_gt1000->raw[idx++] = LONG_MARK;
	quigg_gt1000->raw[idx++] = SHORT_SPACE;
	return idx;
}

static void numtoBin(int idx, int num, int len) {
	/* fill in bits idx to idx+len-1 for number, msb first */
	for(len--;len>=0;len--) {
		bincode[idx+len] = (num & 1); /* value is lsb num */
		num = num>>1; /* shift number 1 place to right, so 2th bit becomes lsb */
	}
}

static int fillRawCode(void) {
	/* convert binary code in bincode[] to raw Mark-Space combis for this protocol */
	/* the complete RawCode consist of <startpulse><bincode><startpulse><bincode><progpulse><bincode><footer> */

	int idx = 0; /* the index into the raw matrix, starting with 0 */
	int cnt = 0;

	quigg_gt1000->raw[idx++] = START_MARK; /* always start with start pulse */
	quigg_gt1000->raw[idx++] = START_SPACE;
	for(cnt=0;cnt<BIN_LENGTH; cnt++) {
		idx = (bincode[cnt]==1) ? fillHigh(idx) : fillLow(idx);
	}

	quigg_gt1000->raw[idx++] = START_MARK; /* start second sequence*/
	quigg_gt1000->raw[idx++] = START_SPACE;
	for(cnt=0;cnt<BIN_LENGTH; cnt++) {
		idx = (bincode[cnt]==1) ? fillHigh(idx) : fillLow(idx);
	}

	quigg_gt1000->raw[idx++] = PROG_MARK; /* program sequence */
	quigg_gt1000->raw[idx++] = PROG_SPACE;
	for(cnt=0;cnt<BIN_LENGTH;cnt++) {
		idx = (bincode[cnt]==1) ? fillHigh(idx) : fillLow(idx);
	}

	quigg_gt1000->raw[idx++] = FOOTER_MARK;
	return idx;
}

static int fillBinCode(int id, int unit, int state, int codeseq) {
	int genindex = 0, rcodeindex = 0;

	/* encode id */
	numtoBin(0, id, 4); /* first 4 bits [0..3] is id-code */

	/* encode the right random code, depends on id, state and unit, state=0,1 */
	rcodeindex= 8*unit + 4*state; /*calculate second index into codetab*/
	if(codetab[id][rcodeindex + codeseq] == 0) {	/* no random code available? */
		/* then try first index */
		if(codetab[id][rcodeindex] == 0) { /* and no first code available? */
			logprintf(LOG_ERR, "quiggGT1000: no random code available for this switch action");
			return EXIT_FAILURE;
		} else {
			logprintf(LOG_WARNING, "quiggGT1000: seq#%d not available for this switch action, using #0 instead", codeseq);
			codeseq = 0;
		}
	}
	/* now we have an index for a valid random code, so put it in our codetab  starting at position 4 */
	numtoBin(4, codetab[id][rcodeindex + codeseq], 16);

	/* encode the unit part of the command */
	/* key1..4 represents unit0..3, encoding depends on generation */
	/* master key results in unit=4, encoding depends on generation */
	genindex = gentab[id];	/* get 0 or 1 depending on generation of group-id */
	numtoBin(20, unittab[genindex][unit], 4); /* get unit-id from tab and encode starting at pos 20 */
	return EXIT_SUCCESS;
}

static void fillSuperBinCode(int state, int codeseq) {
	/* encode the right random supercode, depends on state=0,1 and codeseq*/
	numtoBin(0, supercodes[NRSUPERCODES * state + codeseq], 20);

	/* encode the unit part of the command, is always 1000 or 8 */
	numtoBin(20, 8, 4);
}


static int createCode(JsonNode *code) { // function to create the raw code
	int id = -1;
	int unit = -1;
	int all = 0;
	int super = 0;
	int state = -1;
	int seq = -1;
	double itmp = -1;

	if(json_find_number(code, "num", &itmp) == 0)
		seq = (int)round(itmp);
	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if(json_find_number(code, "all", &itmp) == 0)
		all = (int)round(itmp);
	if(json_find_number(code, "super", &itmp) == 0)
		super = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(seq == -1) {/* no seqnr was given, a random seqnr will be used */
		srand((unsigned int)time (0));	/* seed the random generator with current time */
		seq = rand() & ((super == 1)? NRSUPERMASK:0x03); /* use last bits of random number */
	}

	if((id == -1 && super==0) || (unit == -1 && all==0 && super==0) || state == -1) {
		logprintf(LOG_ERR, "quigg_gt1000: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if((id >15 || id < 0) && super == 0) {
		logprintf(LOG_ERR, "quigg_gt1000: invalid id range");
		return EXIT_FAILURE;
	} else if((unit > 3 || unit < 0) && all == 0 && super == 0){
		logprintf(LOG_ERR, "quigg_gt1000: invalid unit range");
		return EXIT_FAILURE;
	} else if(((seq > 3 || seq < 0) && super == 0) || ((seq >= NRSUPERCODES || seq < 0) && super != 0)) {
		logprintf(LOG_ERR, "quigg_gt1000: invalid optional seq number");
		return EXIT_FAILURE;
	} else {
		if(all == 1) { /* all overrides unit selection */
		    unit = 4;
		}
		 /* first create binary command string */
		if(super == 1) { /* do we want supercodes?  */
			/* for supercodes we use a non-existend id and unit */
			id = 16; /* this does only influence the CreateMessage and not the bin code generation */
			unit = 5;
			fillSuperBinCode(state, seq); /*create binary supercode string */
		} else if(fillBinCode(id, unit, state, seq) != EXIT_SUCCESS) {
			return EXIT_FAILURE; /* failure because no codeseq was available */
		}

		/* and now convert binary to Mark-Space combis */
		if(fillRawCode() != RAW_LENGTH) {
			/* this Error should never occur. It indicates a wrong raw protocol length or misaligned fill */
			logprintf(LOG_ERR, "quigg_gt1000: raw index not correct %d %d",quigg_gt1000->rawlen,fillRawCode);
			return EXIT_FAILURE;
		}
		quigg_gt1000->rawlen = RAW_LENGTH;
		createMessage(id, unit, state, seq, 0);
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) { // print help for this protocol (send -p <name> -H)
	printf("\t -t --on\tsend an on signal\n");
	printf("\t -f --off\tsend an off signal\n");
	printf("\t -u --unit=unit\tcontrol a device with this unit code [0..3]\n");
	printf("\t -i --id=id\tcontrol a device with this id [0..15] \n");
	printf("\t -a --all\tcontrol all devices with this id\n");
	printf("\t -s --super\tcontrol all devices regardless of id\n");
	printf("\t -n --num\toptional use random (super)code sequence num 0..3 (%d)\n", NRSUPERCODES-1);

}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void quiggGT1000Init(void) {
	protocol_register(&quigg_gt1000);
	protocol_set_id(quigg_gt1000, "quigg_gt1000");
	protocol_device_add(quigg_gt1000, "quigg_gt1000", "Quigg GT-1000 protocol");
	quigg_gt1000->devtype = SWITCH;
	quigg_gt1000->hwtype = RF433;
	quigg_gt1000->txrpt = NORMAL_REPEATS;
	quigg_gt1000->minrawlen = RAW_LENGTH;
	quigg_gt1000->maxrawlen = RAW_LENGTH;
	quigg_gt1000->maxgaplen = (int)PROG_SPACE*1.1;
	quigg_gt1000->mingaplen = (int)PROG_SPACE*0.9;

	options_add(&quigg_gt1000->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&quigg_gt1000->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&quigg_gt1000->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-5])$");
	options_add(&quigg_gt1000->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(1[0-6]|[0-9])$");
	options_add(&quigg_gt1000->options, "a", "all", OPTION_NO_VALUE, DEVICES_SETTING, JSON_NUMBER, NULL, NULL);
	options_add(&quigg_gt1000->options, "s", "super", OPTION_NO_VALUE, DEVICES_SETTING, JSON_NUMBER, NULL, NULL);
	options_add(&quigg_gt1000->options, "n", "num", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, NULL, "^([0-3])$");

	options_add(&quigg_gt1000->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&quigg_gt1000->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	quigg_gt1000->printHelp = &printHelp;
	quigg_gt1000->createCode = &createCode;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "quigg_gt1000";
	module->version = "1.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	quiggGT1000Init();
}
#endif
