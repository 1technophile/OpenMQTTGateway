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
#include <errno.h>

#include "../action.h"
#include "../events.h"
#include "../../core/options.h"
#include "../../config/devices.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "switch.h"

static struct units_t {
	char name[255];
	int id;
} units[] = {
	{ "MILLISECOND", 	1 },
	{ "SECOND", 2 },
	{ "MINUTE", 3 },
	{ "HOUR", 4 },
	{ "DAY", 5 }
};

static int checkArguments(struct rules_actions_t *obj) {
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jto = NULL;
	struct JsonNode *jfor = NULL;
	struct JsonNode *jafter = NULL;
	struct JsonNode *javalues = NULL;
	struct JsonNode *jbvalues = NULL;
	struct JsonNode *jcvalues = NULL;
	struct JsonNode *jdvalues = NULL;
	struct JsonNode *jachild = NULL;
	struct JsonNode *jbchild = NULL;
	struct JsonNode *jcchild = NULL;
	struct JsonNode *jdchild = NULL;
	char *state = NULL, **array = NULL;
	double nr1 = 0.0, nr2 = 0.0, nr3 = 0.0, nr4 = 0.0;
	int nrvalues = 0, l = 0, i = 0, match = 0;
	int	nrunits = (sizeof(units)/sizeof(units[0]));

	jdevice = json_find_member(obj->arguments, "DEVICE");
	jto = json_find_member(obj->arguments, "TO");
	jfor = json_find_member(obj->arguments, "FOR");
	jafter = json_find_member(obj->arguments, "AFTER");

	if(jdevice == NULL) {
		logprintf(LOG_ERR, "switch action is missing a \"DEVICE\" statement");
		return -1;
	}

	if(jto == NULL) {
		logprintf(LOG_ERR, "switch action is missing a \"TO ...\" statement");
		return -1;
	}

	json_find_number(jdevice, "order", &nr1);
	json_find_number(jto, "order", &nr2);

	if(jfor != NULL) {
		json_find_number(jfor, "order", &nr3);
		if(nr3 < nr2) {
			logprintf(LOG_ERR, "switch actions are formatted as \"switch DEVICE ... TO ... FOR ...\"");
			return -1;
		}
	}

	if(jafter != NULL) {
		json_find_number(jafter, "order", &nr4);
		if(nr4 < nr2) {
			logprintf(LOG_ERR, "switch actions are formatted as \"switch DEVICE ... TO ... AFTER ...\"");
			return -1;
		}
	}

	if((int)nr1 != 1 || (int)nr2 != 2) {
		logprintf(LOG_ERR, "switch actions are formatted as \"switch DEVICE ... TO ...\"");
		return -1;
	}

	nrvalues = 0;
	if((javalues = json_find_member(jto, "value")) != NULL) {
		jachild = json_first_child(javalues);
		while(jachild) {
			nrvalues++;
			jachild = jachild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "switch action \"TO\" only takes one argument");
		return -1;
	}

	nrvalues = 0;
	if(jfor != NULL) {
		if((jcvalues = json_find_member(jfor, "value")) != NULL) {
			jcchild = json_first_child(jcvalues);
			while(jcchild) {
				nrvalues++;
				if(jcchild->tag == JSON_STRING) {
					l = explode(jcchild->string_, " ", &array);
					if(l == 2) {
						match = 0;
						for(i=0;i<nrunits;i++) {
							if(strcmp(array[1], units[i].name) == 0) {
								match = 1;
								if(isNumeric(array[0]) != 0 && atoi(array[0]) <= 0) {
									logprintf(LOG_ERR, "switch action \"FOR\" requires a positive number and a unit e.g. \"1 MINUTE\"");
									array_free(&array, l);
									return -1;
								}
								break;
							}
						}
						if(match == 0) {
							logprintf(LOG_ERR, "switch action \"%s\" is not a valid unit", array[1]);
							array_free(&array, l);
							return -1;
						}
					} else {
						logprintf(LOG_ERR, "switch action \"FOR\" requires a positive number and a unit e.g. \"1 MINUTE\"");
						if(l > 0) {
							array_free(&array, l);
						}
						return -1;
					}
				}
				jcchild = jcchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "switch action \"FOR\" only takes one argument");
			return -1;
		}
	}

	nrvalues = 0;
	if(jafter != NULL) {
		if((jdvalues = json_find_member(jafter, "value")) != NULL) {
			jdchild = json_first_child(jdvalues);
			while(jdchild) {
				nrvalues++;
				if(jdchild->tag == JSON_STRING) {
					l = explode(jdchild->string_, " ", &array);
					if(l == 2) {
						match = 0;
						for(i=0;i<nrunits;i++) {
							if(strcmp(array[1], units[i].name) == 0) {
								match = 1;
								if(isNumeric(array[0]) != 0 && atoi(array[0]) <= 0) {
									logprintf(LOG_ERR, "switch action \"AFTER\" requires a positive number and a unit e.g. \"1 MINUTE\"");
									array_free(&array, l);
									return -1;
								}
								break;
							}
						}
						if(match == 0) {
							logprintf(LOG_ERR, "switch action \"%s\" is not a valid unit", array[1]);
							array_free(&array, l);
							return -1;
						}
					} else {
						logprintf(LOG_ERR, "switch action \"AFTER\" requires a positive number and a unit e.g. \"1 MINUTE\"");
						if(l > 0) {
							array_free(&array, l);
						}
						return -1;
					}
					array_free(&array, l);
				}
				jdchild = jdchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "switch action \"AFTER\" only takes one argument");
			return -1;
		}
	}

	if((jbvalues = json_find_member(jdevice, "value")) != NULL) {
		jbchild = json_first_child(jbvalues);
		while(jbchild) {
			if(jbchild->tag == JSON_STRING) {
				struct devices_t *dev = NULL;
				if(devices_get(jbchild->string_, &dev) == 0) {
					if((javalues = json_find_member(jto, "value")) != NULL) {
						jachild = json_first_child(javalues);
						while(jachild) {
							if(jachild->tag == JSON_STRING) {
								state = jachild->string_;
								struct protocols_t *tmp = dev->protocols;
								int match1 = 0;
								while(tmp) {
									struct options_t *opt = tmp->listener->options;
									while(opt) {
										if(opt->conftype == DEVICES_STATE) {
											if(strcmp(opt->name, state) == 0) {
												match1 = 1;
												break;
											}
										}
										opt = opt->next;
									}
									tmp = tmp->next;
								}
								if(match1 == 0) {
									logprintf(LOG_ERR, "device \"%s\" can't be set to state \"%s\"", jbchild->string_, state);
									return -1;
								}
							} else {
								return -1;
							}
							jachild = jachild->next;
						}
					} else {
						return -1;
					}
				} else {
					logprintf(LOG_ERR, "device \"%s\" doesn't exists", jbchild->string_);
					return -1;
				}
			} else {
				return -1;
			}
			jbchild = jbchild->next;
		}
	}
	return 0;
}

static void *thread(void *param) {
	struct event_action_thread_t *pth = (struct event_action_thread_t *)param;
	struct JsonNode *json = pth->obj->arguments;
	struct JsonNode *jto = NULL;
	struct JsonNode *jafter = NULL;
	struct JsonNode *jfor = NULL;
	struct JsonNode *javalues = NULL;
	struct JsonNode *jcvalues = NULL;
	struct JsonNode *jdvalues = NULL;
	struct JsonNode *jstate = NULL;
	struct JsonNode *jaseconds = NULL;
	char *new_state = NULL, *old_state = NULL, *state = NULL, **array = NULL;
	int seconds_after = 0, type_after = 0;
	int	l = 0, i = 0, nrunits = (sizeof(units)/sizeof(units[0]));
	int seconds_for = 0, type_for = 0, timer = 0;

	event_action_started(pth);

	if((jfor = json_find_member(json, "FOR")) != NULL) {
		if((jcvalues = json_find_member(jfor, "value")) != NULL) {
			jaseconds = json_find_element(jcvalues, 0);
			if(jaseconds != NULL) {
				if(jaseconds->tag == JSON_STRING) {
					l = explode(jaseconds->string_, " ", &array);
					if(l == 2) {
						for(i=0;i<nrunits;i++) {
							if(strcmp(array[1], units[i].name) == 0) {
								seconds_for = atoi(array[0]);
								type_for = units[i].id;
								break;
							}
						}
					}
					if(l > 0) {
						array_free(&array, l);
					}
				}
			}
		}
	}

	if((jafter = json_find_member(json, "AFTER")) != NULL) {
		if((jdvalues = json_find_member(jafter, "value")) != NULL) {
			jaseconds = json_find_element(jdvalues, 0);
			if(jaseconds != NULL) {
				if(jaseconds->tag == JSON_STRING) {
					l = explode(jaseconds->string_, " ", &array);
					if(l == 2) {
						for(i=0;i<nrunits;i++) {
							if(strcmp(array[1], units[i].name) == 0) {
								seconds_after = atoi(array[0]);
								type_after = units[i].id;
								break;
							}
						}
					}
					if(l > 0) {
						array_free(&array, l);
					}
				}
			}
		}
	}

	switch(type_for) {
		case 3:
			seconds_for *= 60;
		break;
		case 4:
			seconds_for *= (60*60);
		break;
		case 5:
			seconds_for *= (60*60*24);
		break;
	}

	switch(type_after) {
		case 3:
			seconds_after *= 60;
		break;
		case 4:
			seconds_after *= (60*60);
		break;
		case 5:
			seconds_after *= (60*60*24);
		break;
	}

	/* Store current state */
	struct devices_t *tmp = pth->device;
	int match = 0;
	while(tmp) {
		struct devices_settings_t *opt = tmp->settings;
		while(opt) {
			if(strcmp(opt->name, "state") == 0) {
				if(opt->values->type == JSON_STRING) {
					if((old_state = MALLOC(strlen(opt->values->string_)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(old_state, opt->values->string_);
					match = 1;
				}
				break;
			}
			opt = opt->next;
		}
		if(match == 1) {
			break;
		}
		tmp = tmp->next;
	}
	if(match == 0) {
		logprintf(LOG_NOTICE, "could not store old state of \"%s\"\n", pth->device->id);
	}

	timer = 0;
	while(pth->loop == 1) {
		if(timer == seconds_after) {
			if((jto = json_find_member(json, "TO")) != NULL) {
				if((javalues = json_find_member(jto, "value")) != NULL) {
					jstate = json_find_element(javalues, 0);
					if(jstate != NULL && jstate->tag == JSON_STRING) {
						state = jstate->string_;
						if((new_state = MALLOC(strlen(state)+1)) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						strcpy(new_state, state);
						/*
						 * We're not switching when current state is the same as
						 * the old state.
						 */
						if(old_state == NULL || strcmp(old_state, new_state) != 0) {
							if(pilight.control != NULL) {
								pilight.control(pth->device, new_state, NULL, ACTION);
							}
						}
					}
				}
			}
			break;
		}
		timer++;
		if(type_after > 1) {
			sleep(1);
		} else {
			usleep(1000);
		}
	}

	/*
	 * We only need to restore the state if it was actually changed
	 */
	if(seconds_for > 0 && old_state != NULL && new_state != NULL && strcmp(old_state, new_state) != 0) {
		timer = 0;
		while(pth->loop == 1) {
			if(seconds_for == timer) {
				if(pilight.control != NULL) {
					pilight.control(pth->device, old_state, NULL, ACTION);
				}
				break;
			}
			timer++;
			if(type_for > 1) {
				sleep(1);
			} else {
				usleep(1000);
			}
		}
	}

	if(old_state != NULL) {
		FREE(old_state);
	}

	if(new_state != NULL) {
		FREE(new_state);
	}

	event_action_stopped(pth);

	return (void *)NULL;
}

static int run(struct rules_actions_t *obj) {
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jto = NULL;
	struct JsonNode *jbvalues = NULL;
	struct JsonNode *jbchild = NULL;

	if((jdevice = json_find_member(obj->arguments, "DEVICE")) != NULL &&
		 (jto = json_find_member(obj->arguments, "TO")) != NULL) {
		if((jbvalues = json_find_member(jdevice, "value")) != NULL) {
			jbchild = json_first_child(jbvalues);
			while(jbchild) {
				if(jbchild->tag == JSON_STRING) {
					struct devices_t *dev = NULL;
					if(devices_get(jbchild->string_, &dev) == 0) {
						event_action_thread_start(dev, action_switch->name, thread, obj);
					}
				}
				jbchild = jbchild->next;
			}
		}
	}
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void actionSwitchInit(void) {
	event_action_register(&action_switch, "switch");

	options_add(&action_switch->options, 'a', "DEVICE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_switch->options, 'b', "TO", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_switch->options, 'c', "AFTER", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_switch->options, 'd', "FOR", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	action_switch->run = &run;
	action_switch->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "switch";
	module->version = "3.2";
	module->reqversion = "6.0";
	module->reqcommit = "152";
}

void init(void) {
	actionSwitchInit();
}
#endif
