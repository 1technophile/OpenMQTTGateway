/*
  Copyright (C) 2017 - 2019 by CurlyMo, Meloen, zeltom, mwka

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
#include "kerui_d026.h"


#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	260
#define MAX_PULSE_LENGTH	310
#define AVG_PULSE_LENGTH	280
#define RAW_LENGTH		50

static int validate(void) {
	if(kerui_D026->rawlen == RAW_LENGTH) {
		if(kerui_D026->raw[kerui_D026->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   kerui_D026->raw[kerui_D026->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int unitcode, int state, int state2, int state3, int state4) {
	kerui_D026->message = json_mkobject();
	json_append_member(kerui_D026->message, "unitcode", json_mknumber(unitcode, 0));

	if(state4 == 0) {
		json_append_member(kerui_D026->message, "state", json_mkstring("opened"));
	}
        else if(state == 0) {
                json_append_member(kerui_D026->message, "state", json_mkstring("closed"));
        }
        else if(state2 == 0) {
                json_append_member(kerui_D026->message, "state", json_mkstring("tamper"));
        }
        else if(state3 == 0) {
                json_append_member(kerui_D026->message, "state", json_mkstring("not used"));
        }
	else{
                json_append_member(kerui_D026->message, "battery", json_mkstring("low"));
	}

}

static void parseCode(void) {
	int binary[RAW_LENGTH/2], x = 0, i = 0;

	for(x=0;x<kerui_D026->rawlen-2;x+=2) {
		if(kerui_D026->raw[x] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	int unitcode = binToDec(binary, 0, 19);
	int state = binary[20];
        int state2 = binary[21];
        int state3 = binary[22];
        int state4 = binary[23];
        createMessage(unitcode, state, state2, state3, state4);
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void keruiD026Init(void) {

	protocol_register(&kerui_D026);
	protocol_set_id(kerui_D026, "kerui_D026");
	protocol_device_add(kerui_D026, "kerui_D026", "KERUI D026 Door sensor");
	kerui_D026->devtype = CONTACT;
	kerui_D026->hwtype = RF433;
	kerui_D026->minrawlen = RAW_LENGTH;
	kerui_D026->maxrawlen = RAW_LENGTH;
	kerui_D026->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	kerui_D026->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&kerui_D026->options, "u", "unitcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, NULL);
	options_add(&kerui_D026->options, "t", "opened", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&kerui_D026->options, "f", "closed", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&kerui_D026->options, "a", "tamper", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	kerui_D026->parseCode=&parseCode;
	kerui_D026->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "kerui_D026";
	module->version = "2.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	keruiD026Init();
}
#endif
