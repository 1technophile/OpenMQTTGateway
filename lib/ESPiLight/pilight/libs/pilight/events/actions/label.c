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
#include "label.h"

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
	struct JsonNode *jcolor = NULL;
	struct JsonNode *javalues = NULL;
	struct JsonNode *jbvalues = NULL;
	struct JsonNode *jcvalues = NULL;
	struct JsonNode *jdvalues = NULL;
	struct JsonNode *jevalues = NULL;
	struct JsonNode *jachild = NULL;
	struct JsonNode *jbchild = NULL;
	struct JsonNode *jcchild = NULL;
	struct JsonNode *jdchild = NULL;
	struct JsonNode *jechild = NULL;
	char **array = NULL;
	double nr1 = 0.0, nr2 = 0.0, nr3 = 0.0, nr4 = 0.0, nr5 = 0.0;
	int nrvalues = 0, l = 0, i = 0, match = 0;
	int	nrunits = (sizeof(units)/sizeof(units[0]));

	if(obj == NULL) {
		/* Internal error */
		return -1;
	}

	if(obj->arguments == NULL) {
		/* Internal error */
		return -1;
	}

	jdevice = json_find_member(obj->arguments, "DEVICE");
	jto = json_find_member(obj->arguments, "TO");
	jfor = json_find_member(obj->arguments, "FOR");
	jafter = json_find_member(obj->arguments, "AFTER");
	jcolor = json_find_member(obj->arguments, "COLOR");

	if(jdevice == NULL) {
		logprintf(LOG_ERR, "label action is missing a \"DEVICE\"");
		return -1;
	}

	if(jto == NULL) {
		logprintf(LOG_ERR, "label action is missing a \"TO ...\" statement");
		return -1;
	}

	json_find_number(jdevice, "order", &nr1);
	json_find_number(jto, "order", &nr2);

	if(jcolor != NULL) {
		json_find_number(jcolor, "order", &nr5);
	}

	if(jfor != NULL) {
		json_find_number(jfor, "order", &nr3);
		if(nr3 < nr2) {
			logprintf(LOG_ERR, "label actions are formatted as \"label DEVICE ... TO ... FOR ...\"");
			return -1;
		} else if(nr5 > 0 && nr3 < nr5) {
			logprintf(LOG_ERR, "label actions are formatted as \"label DEVICE ... TO ... COLOR ... FOR ...\"");
			return -1;
		}
	}

	if(jafter != NULL) {
		json_find_number(jafter, "order", &nr4);
		if(nr4 < nr2) {
			logprintf(LOG_ERR, "label actions are formatted as \"label DEVICE ... TO ... AFTER ...\"");
			return -1;
		} else if(nr5 > 0 && nr4 < nr5) {
			logprintf(LOG_ERR, "label actions are formatted as \"label DEVICE ... TO ... COLOR ... AFTER ...\"");
			return -1;
		}
	}

	if((int)nr1 != 1 || (int)nr2 != 2) {
		logprintf(LOG_ERR, "label actions are formatted as \"label DEVICE ... TO ...\"");
		return -1;
	}
	if(nr5 > 0 && nr5 != 3) {
		logprintf(LOG_ERR, "label actions are formatted as \"label DEVICE ... TO ... COLOR ...\"");
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
		logprintf(LOG_ERR, "label action \"TO\" only takes one argument");
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
						array_free(&array, l);
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
			logprintf(LOG_ERR, "label action \"FOR\" only takes one argument");
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
								if(isNumeric(array[0]) != 0) {
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
						array_free(&array, l);
					} else {
						logprintf(LOG_ERR, "switch action \"AFTER\" requires a positive number and a unit e.g. \"1 MINUTE\"");
						if(l > 0) {
							array_free(&array, l);
						}
						return -1;
					}
				}
				jdchild = jdchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "label action \"AFTER\" only takes one argument");
			return -1;
		}
	}

	nrvalues = 0;
	if(jcolor != NULL) {
		if((jevalues = json_find_member(jcolor, "value")) != NULL) {
			jechild = json_first_child(jevalues);
			while(jechild) {
				nrvalues++;
				jechild = jechild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "label action \"COLOR\" only takes one argument");
			return -1;
		}
	}

	if((jbvalues = json_find_member(jdevice, "value")) != NULL) {
		jbchild = json_first_child(jbvalues);
		while(jbchild) {
			if(jbchild->tag == JSON_STRING) {
				struct devices_t *dev = NULL;
				if(devices_get(jbchild->string_, &dev) == 0) {
					struct protocols_t *protocols = dev->protocols;
					int match = 0;
					while(protocols) {
						if(protocols->listener->devtype == LABEL) {
							match = 1;
							break;
						}
						protocols = protocols->next;
					}
					if(match == 0) {
						logprintf(LOG_ERR, "the label action only works with the label devices");
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
	struct JsonNode *jcolor = NULL;
	struct JsonNode *javalues = NULL;
	struct JsonNode *jcvalues = NULL;
	struct JsonNode *jdvalues = NULL;
	struct JsonNode *jevalues = NULL;
	struct JsonNode *jlabel = NULL;
	struct JsonNode *jaseconds = NULL;
	struct JsonNode *jvalues = NULL;
	char *new_label = NULL, *old_label = NULL, *label = NULL, **array = NULL;
	char *new_color = NULL, *old_color = NULL, *color = NULL;
	int seconds_after = 0, type_after = 0, free_label = 0;
	int	l = 0, i = 0, nrunits = (sizeof(units)/sizeof(units[0]));
	int seconds_for = 0, type_for = 0, timer = 0;

	event_action_started(pth);

	if((jcolor = json_find_member(json, "COLOR")) != NULL) {
		if((jevalues = json_find_member(jcolor, "value")) != NULL) {
			jcolor = json_find_element(jevalues, 0);
			if(jcolor != NULL && jcolor->tag == JSON_STRING) {
				color = jcolor->string_;
				if((new_color = MALLOC(strlen(color)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(new_color, color);
			}
		}
	}

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
								seconds_after = atoi(array[1]);
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

	/* Store current label */
	struct devices_t *tmp = pth->device;
	int match1 = 0, match2 = 0;
	while(tmp) {
		struct devices_settings_t *opt = tmp->settings;
		while(opt) {
			if(strcmp(opt->name, "label") == 0) {
				if(opt->values->type == JSON_STRING) {
					if((old_label = MALLOC(strlen(opt->values->string_)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(old_label, opt->values->string_);
					match1 = 1;
				}
			}
			if(strcmp(opt->name, "color") == 0) {
				if(opt->values->type == JSON_STRING) {
					if((old_color = MALLOC(strlen(opt->values->string_)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(old_color, opt->values->string_);
					match2 = 1;
				}
			}
			opt = opt->next;
		}
		if(match1 == 1 && match2 == 1) {
			break;
		}
		tmp = tmp->next;
	}
	if(match1 == 0) {
		logprintf(LOG_NOTICE, "could not store old label of \"%s\"", pth->device->id);
	}
	if(match2 == 0) {
		logprintf(LOG_NOTICE, "could not store old color of \"%s\"", pth->device->id);
	}

	timer = 0;
	while(pth->loop == 1) {
		if(timer == seconds_after) {
			if((jto = json_find_member(json, "TO")) != NULL) {
				if((javalues = json_find_member(jto, "value")) != NULL) {
					jlabel = json_find_element(javalues, 0);
					if(jlabel != NULL) {
						if(jlabel->tag == JSON_STRING) {
							label = jlabel->string_;
						} else if(jlabel->tag == JSON_NUMBER) {
							int l = snprintf(NULL, 0, "%.*f", jlabel->decimals_, jlabel->number_);
							if((label = MALLOC(l+1)) == NULL) {
								fprintf(stderr, "out of memory\n");
								exit(EXIT_FAILURE);
							}
							memset(label, '\0', l);
							free_label = 1;
							snprintf(label, l + 1, "%.*f", jlabel->decimals_, jlabel->number_);
							label[l] = '\0';
						}
						if((new_label = MALLOC(strlen(label)+1)) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						strcpy(new_label, label);
						/*
						 * We're not switching when current label or is the same as
						 * the old label or old color.
						 */
						if(old_label == NULL || strcmp(old_label, new_label) != 0 ||
							(old_color != NULL && new_color != NULL && strcmp(old_color, new_color) != 0)) {
							if(pilight.control != NULL) {
								jvalues = json_mkobject();
								if(color != NULL) {
									json_append_member(jvalues, "color", json_mkstring(color));
								}
								json_append_member(jvalues, "label", json_mkstring(label));
								pilight.control(pth->device, NULL, json_first_child(jvalues), ACTION);
								json_delete(jvalues);
								break;
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
	 * We only need to restore the label if it was actually changed
	 */
	if(seconds_for > 0 && ((old_label != NULL && new_label != NULL && strcmp(old_label, new_label) != 0) ||
	   (old_color != NULL && new_color != NULL && strcmp(old_color, new_color) != 0))) {
		timer = 0;
		while(pth->loop == 1) {
			if(seconds_for == timer) {
				if(pilight.control != NULL) {
					jvalues = json_mkobject();
					if(old_color != NULL) {
						json_append_member(jvalues, "color", json_mkstring(old_color));
					}
					json_append_member(jvalues, "label", json_mkstring(old_label));
					pilight.control(pth->device, NULL, json_first_child(jvalues), ACTION);
					json_delete(jvalues);
					break;
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

	if(free_label == 1) {
		FREE(label);
	}

	if(old_label != NULL) {
		FREE(old_label);
	}

	if(new_label != NULL) {
		FREE(new_label);
	}

	if(old_color != NULL) {
		FREE(old_color);
	}

	if(new_color != NULL) {
		FREE(new_color);
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
						event_action_thread_start(dev, action_label->name, thread, obj);
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
void actionLabelInit(void) {
	event_action_register(&action_label, "label");

	options_add(&action_label->options, 'a', "DEVICE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_label->options, 'b', "TO", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING | JSON_NUMBER, NULL, NULL);
	options_add(&action_label->options, 'c', "AFTER", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_label->options, 'd', "FOR", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_label->options, 'e', "COLOR", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	action_label->run = &run;
	action_label->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "label";
	module->version = "2.2";
	module->reqversion = "6.0";
	module->reqcommit = "152";
}

void init(void) {
	actionLabelInit();
}
#endif
