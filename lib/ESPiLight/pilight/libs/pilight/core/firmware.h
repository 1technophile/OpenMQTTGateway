/*
	Copyright (C) 2014 CurlyMo

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

#ifndef _FIRMWARE_H_
#define _FIRMWARE_H_

typedef struct firmware_t {
	double lpf;
	double hpf;
	double version;
} firmware_t;
struct firmware_t firmware;

typedef enum {
	FW_PROG_OP_FAIL = 1,
	FW_INIT_FAIL,
	FW_RD_SIG_FAIL,
	FW_INV_SIG_FAIL,
	FW_MATCH_SIG_FAIL,
	FW_ERASE_FAIL,
	FW_WRITE_FAIL,
	FW_VERIFY_FAIL,
	FW_RD_FUSE_FAIL,
	FW_INV_FUSE_FAIL
} exitrc_t;

typedef enum {
	FW_MP_UNKNOWN,
	FW_MP_ATTINY25,
	FW_MP_ATTINY45,
	FW_MP_ATTINY85,
	FW_MP_ATMEL328P,
	FW_MP_ATMEL32U4
} mptype_t;

int firmware_update(char *fwfile, char *comport);
int firmware_getmp(char *comport);

#endif
