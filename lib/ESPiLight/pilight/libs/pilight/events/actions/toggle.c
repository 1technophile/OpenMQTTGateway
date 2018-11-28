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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../action.h"
#include "../../core/options.h"
#include "../../config/devices.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "toggle.h"

static int checkArguments(struct rules_actions_t *obj) {
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jbetween = NULL;
	struct JsonNode *jsvalues = NULL;
	struct JsonNode *jdvalues = NULL;
	struct JsonNode *jschild = NULL;
	struct JsonNode *jdchild = NULL;
	double nr1 = 0.0, nr2 = 0.0;
	int nrvalues = 0;
	jdevice = json_find_member(obj->arguments, "DEVICE");
	jbetween = json_find_member(obj->arguments, "BETWEEN");

	if(jdevice == NULL) {
		logprintf(LOG_ERR, "toggle action is missing a \"DEVICE\"");
		return -1;
	}
	if(jbetween == NULL && jbetween->tag != JSON_ARRAY) {
		logprintf(LOG_ERR, "toggle action is missing a \"BETWEEN ... AND ...\" statement");
		return -1;
	}
	json_find_number(jdevice, "order", &nr1);
	json_find_number(jbetween, "order", &nr2);
	if((int)nr1 != 1 || (int)nr2 != 2) {
		logprintf(LOG_ERR, "toggle actions are formatted as \"toggle DEVICE ... BETWEEN ... AND ...\"");
		return -1;
	}
	if((jsvalues = json_find_member(jbetween, "value")) != NULL) {
		jschild = json_first_child(jsvalues);
		while(jschild) {
			nrvalues++;
			jschild = jschild->next;
		}
	}
	if(nrvalues != 2) {
		logprintf(LOG_ERR, "toggle actions are formatted as \"toggle DEVICE ... BETWEEN ... AND ...\"");
		return -1;
	}
	if((jdvalues = json_find_member(jdevice, "value")) != NULL) {
		jdchild = json_first_child(jdvalues);
		while(jdchild) {
			if(jdchild->tag == JSON_STRING) {
				struct devices_t *dev = NULL;
				if(devices_get(jdchild->string_, &dev) == 0) {
					if((jsvalues = json_find_member(jbetween, "value")) != NULL) {
						jschild = json_first_child(jsvalues);
						while(jschild) {
							if(jschild->tag == JSON_STRING) {
									struct protocols_t *tmp = dev->protocols;
									int match1 = 0;
									while(tmp) {
										struct options_t *opt = tmp->listener->options;
										while(opt) {
											if(opt->conftype == DEVICES_STATE) {
												if(strcmp(opt->name, jschild->string_) == 0) {
													match1 = 1;
													break;
												}
											}
											opt = opt->next;
										}
										tmp = tmp->next;
									}
									if(match1 == 0) {
										logprintf(LOG_ERR, "device \"%s\" can't be set to state \"%s\"", jdchild->string_, jschild->string_);
										return -1;
									}
								} else {
									return -1;
								}
							jschild = jschild->next;
						}
					}
				} else {
					logprintf(LOG_ERR, "device \"%s\" doesn't exists", jdchild->string_);
					return -1;
				}
			} else {
				return -1;
			}
			jdchild = jdchild->next;
		}
	} else {
		return -1;
	}
	return 0;
}

static void *thread(void *param) {
	struct event_action_thread_t *pth = (struct event_action_thread_t *)param;
	// struct rules_t *obj = pth->obj;
	struct JsonNode *json = pth->obj->arguments;
	struct devices_settings_t *tmp_settings = pth->device->settings;
	struct JsonNode *jbetween = NULL;
	struct JsonNode *jsvalues = NULL;
	struct JsonNode *jstate1 = NULL;
	struct JsonNode *jstate2 = NULL;
	char *cstate = NULL, *state1 = NULL, *state2 = NULL;

	event_action_started(pth);

	while(tmp_settings) {
		if(strcmp(tmp_settings->name, "state") == 0) {
			if(tmp_settings->values->type == JSON_STRING) {
				cstate = tmp_settings->values->string_;
				break;
			}
		}
		tmp_settings = tmp_settings->next;
	}

	if((jbetween = json_find_member(json, "BETWEEN")) != NULL) {
			if((jsvalues = json_find_member(jbetween, "value")) != NULL) {
			jstate1 = json_find_element(jsvalues, 0);
			jstate2 = json_find_element(jsvalues, 1);
			if(jstate1 != NULL && jstate2 != NULL &&
				jstate1->tag == JSON_STRING && jstate2->tag == JSON_STRING) {
				state1 = jstate1->string_;
				state2 = jstate2->string_;

				if(pilight.control != NULL) {
					if(strcmp(state1, cstate) == 0) {
						pilight.control(pth->device, state2, NULL, ACTION);
					} else if(strcmp(state2, cstate) == 0) {
						pilight.control(pth->device, state1, NULL, ACTION);
					}
				}
			}
		}
	}

	event_action_stopped(pth);

	return (void *)NULL;
}

static int run(struct rules_actions_t *obj) {
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jbetween = NULL;
	struct JsonNode *jdvalues = NULL;
	struct JsonNode *jdchild = NULL;

	if((jdevice = json_find_member(obj->arguments, "DEVICE")) != NULL &&
		 (jbetween = json_find_member(obj->arguments, "BETWEEN")) != NULL) {
		if((jdvalues = json_find_member(jdevice, "value")) != NULL) {
			jdchild = json_first_child(jdvalues);
			while(jdchild) {
				if(jdchild->tag == JSON_STRING) {
					struct devices_t *dev = NULL;
					if(devices_get(jdchild->string_, &dev) == 0) {
						event_action_thread_start(dev, action_toggle->name, thread, obj);
					}
				}
				jdchild = jdchild->next;
			}
		}
	}
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void actionToggleInit(void) {
	event_action_register(&action_toggle, "toggle");

	options_add(&action_toggle->options, 'a', "DEVICE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_toggle->options, 'b', "BETWEEN", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	action_toggle->run = &run;
	action_toggle->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "toggle";
	module->version = "2.1";
	module->reqversion = "6.0";
	module->reqcommit = "58";
}

void init(void) {
	actionToggleInit();
}
#endif
