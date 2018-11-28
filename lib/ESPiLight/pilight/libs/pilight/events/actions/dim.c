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

#include "../../core/pilight.h"
#include "../../core/options.h"
#include "../../config/devices.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../action.h"
#include "../events.h"
#include "dim.h"

#define DECREASING 0
#define INCREASING 1

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
	struct JsonNode *jfrom = NULL;
	struct JsonNode *jin = NULL;
	struct JsonNode *javalues = NULL;
	struct JsonNode *jbvalues = NULL;
	struct JsonNode *jcvalues = NULL;
	struct JsonNode *jdvalues = NULL;
	struct JsonNode *jevalues = NULL;
	struct JsonNode *jfvalues = NULL;
	struct JsonNode *jachild = NULL;
	struct JsonNode *jbchild = NULL;
	struct JsonNode *jcchild = NULL;
	struct JsonNode *jdchild = NULL;
	struct JsonNode *jechild = NULL;
	struct JsonNode *jfchild = NULL;
	char **array = NULL;
	double nr1 = 0.0, nr2 = 0.0, nr3 = 0.0, nr4 = 0.0, nr5 = 0.0, nr6 = 0.0;
	double dimfrom = 0.0, dimto = 0.0;
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
	jin = json_find_member(obj->arguments, "IN");
	jafter = json_find_member(obj->arguments, "AFTER");
	jfrom = json_find_member(obj->arguments, "FROM");

	if(jdevice == NULL) {
		logprintf(LOG_ERR, "dim action is missing a \"DEVICE ...\" statement");
		return -1;
	}

	if(jto == NULL) {
		logprintf(LOG_ERR, "dim action is missing a \"TO ...\" statement");
		return -1;
	}

	json_find_number(jdevice, "order", &nr1);
	json_find_number(jto, "order", &nr2);
	if((int)nr1 != 1 || (int)nr2 != 2) {
		logprintf(LOG_ERR, "dim actions are formatted as \"dim DEVICE ... TO ...\"");
		return -1;
	}

	if(jfor != NULL) {
		json_find_number(jfor, "order", &nr3);
		if(nr3 < nr2) {
			logprintf(LOG_ERR, "dim actions are formatted as \"dim DEVICE ... TO ... FOR ...\"");
			return -1;
		}
	}

	if(jafter != NULL) {
		json_find_number(jafter, "order", &nr4);
		if(nr4 < nr2) {
			logprintf(LOG_ERR, "dim actions are formatted as \"dim DEVICE ... TO ... AFTER ...\"");
			return -1;
		}
	}

	if(jin != NULL) {
		json_find_number(jin, "order", &nr6);
		if(nr6 < nr2) {
			logprintf(LOG_ERR, "dim actions are formatted as \"dim DEVICE ... TO ... FROM ... IN ...\"");
			return -1;
		}
	}

	if(jfrom != NULL) {
		json_find_number(jfrom, "order", &nr5);
		if(nr5 < nr2) {
			logprintf(LOG_ERR, "dim actions are formatted as \"dim DEVICE ... TO ... FROM ... IN ...\"");
			return -1;
		}
		if(jin == NULL) {
			logprintf(LOG_ERR, "dim actions are formatted as \"dim DEVICE ... TO ... FROM ... IN ...\"");
			return -1;
		}
	} else if(jin != NULL) {
		logprintf(LOG_ERR, "dim actions are formatted as \"dim DEVICE ... TO ... FROM ... IN ...\"");
		return -1;
	}

	if((javalues = json_find_member(jto, "value")) != NULL) {
		jachild = json_first_child(javalues);
		while(jachild) {
			nrvalues++;
			jachild = jachild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "dim actions are formatted as \"dim DEVICE ... TO ...\"");
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
			logprintf(LOG_ERR, "dim action \"FOR\" only takes one argument");
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
			logprintf(LOG_ERR, "dim action \"AFTER\" only takes one argument");
			return -1;
		}
	}

	nrvalues = 0;
	if(jfrom != NULL) {
		if((jevalues = json_find_member(jfrom, "value")) != NULL) {
			jechild = json_first_child(jevalues);
			while(jechild) {
				nrvalues++;
				jechild = jechild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "dim action \"FROM\" only takes one argument");
			return -1;
		}
	}

	nrvalues = 0;
	if(jin != NULL) {
		if((jfvalues = json_find_member(jin, "value")) != NULL) {
			jfchild = json_first_child(jfvalues);
			while(jfchild) {
				nrvalues++;
				if(jfchild->tag == JSON_STRING) {
					l = explode(jfchild->string_, " ", &array);
					if(l == 2) {
						match = 0;
						for(i=0;i<nrunits;i++) {
							if(strcmp(array[1], units[i].name) == 0) {
								match = 1;
								if(isNumeric(array[0]) != 0 && atoi(array[0]) <= 0) {
									logprintf(LOG_ERR, "switch action \"IN\" requires a positive number and a unit e.g. \"1 MINUTE\"");
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
						logprintf(LOG_ERR, "switch action \"IN\" requires a positive number and a unit e.g. \"1 MINUTE\"");
						if(l > 0) {
							array_free(&array, l);
						}
						return -1;
					}
				}
				jfchild = jfchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "dim action \"IN\" only takes one argument");
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
							match = 0;
							if(jachild->tag == JSON_NUMBER) {
								dimto = jachild->number_;
								match = 1;
							}
							if(match == 1) {
								struct protocols_t *tmp_protocols = dev->protocols;
								if(tmp_protocols->listener->devtype == DIMMER) {
									struct devices_settings_t *tmp_settings = dev->settings;
									int match1 = 0, match2 = 0;
									while(tmp_settings) {
										if(strcmp(tmp_settings->name, "dimlevel-maximum") == 0) {
											if(tmp_settings->values->type == JSON_NUMBER &&
												(int)tmp_settings->values->number_ < (int)dimto) {
												logprintf(LOG_ERR, "device \"%s\" can't be set to dimlevel \"%d\"", jbchild->string_, (int)dimto);
												return -1;
											}
											match1 = 1;
										}
										if(strcmp(tmp_settings->name, "dimlevel-minimum") == 0) {
											if(tmp_settings->values->type == JSON_NUMBER &&
												(int)tmp_settings->values->number_ > (int)dimto) {
												logprintf(LOG_ERR, "device \"%s\" can't be set to dimlevel \"%d\"", jbchild->string_, (int)dimto);
												return -1;
											}
											match2 = 1;
										}
										tmp_settings = tmp_settings->next;
									}
									if(match1 == 0 || match2 == 0) {
										while(tmp_protocols) {
											struct options_t *opt = tmp_protocols->listener->options;
											while(opt) {
												if(match1 == 0 && strcmp(opt->name, "dimlevel-maximum") == 0 &&
													opt->vartype == JSON_NUMBER && (int)(intptr_t)opt->def < (int)dimto) {
													logprintf(LOG_ERR, "device \"%s\" can't be set to dimlevel \"%d\"", jbchild->string_, (int)dimto);
													return -1;
												}
												if(match2 == 0 && strcmp(opt->name, "dimlevel-minimum") == 0 &&
													opt->vartype == JSON_NUMBER && (int)(intptr_t)opt->def > (int)dimto) {
													logprintf(LOG_ERR, "device \"%s\" can't be set to dimlevel \"%d\"", jbchild->string_, (int)dimto);
													return -1;
												}
												opt = opt->next;
											}
											tmp_protocols = tmp_protocols->next;
										}
									}
								} else {
									logprintf(LOG_ERR, "device \"%s\" doesn't support dimming", jbchild->string_);
									return -1;
								}
							} else {
								logprintf(LOG_ERR, "internal error 1 in dim action", jbchild->string_);
								return -1;
							}
						jachild = jachild->next;
						}
					} else {
						logprintf(LOG_ERR, "internal error 2 in dim action", jbchild->string_);
						return -1;
					}

					if((jevalues = json_find_member(jfrom, "value")) != NULL) {
						jechild = json_first_child(jevalues);
						while(jechild) {
							match = 0;
							if(jechild->tag == JSON_NUMBER) {
								dimfrom = jechild->number_;
								match = 1;
							}
							if(match == 1) {
								struct protocols_t *tmp_protocols = dev->protocols;
								if(tmp_protocols->listener->devtype == DIMMER) {
									struct devices_settings_t *tmp_settings = dev->settings;
									int match1 = 0, match2 = 0;
									while(tmp_settings) {
										if(strcmp(tmp_settings->name, "dimlevel-maximum") == 0) {
											if(tmp_settings->values->type == JSON_NUMBER &&
												(int)tmp_settings->values->number_ < (int)dimfrom) {
												logprintf(LOG_ERR, "device \"%s\" dimlevel can't be set to \"%d\"", jbchild->string_, (int)dimfrom);
												return -1;
											}
											match1 = 1;
										}
										if(strcmp(tmp_settings->name, "dimlevel-minimum") == 0) {
											if(tmp_settings->values->type == JSON_NUMBER &&
												(int)tmp_settings->values->number_ > (int)dimfrom) {
												logprintf(LOG_ERR, "device \"%s\" dimlevel can't be set to \"%d\"", jbchild->string_, (int)dimfrom);
												return -1;
											}
											match2 = 1;
										}
										tmp_settings = tmp_settings->next;
									}
									if(match1 == 0 || match2 == 0) {
										while(tmp_protocols) {
											struct options_t *opt = tmp_protocols->listener->options;
											while(opt) {
												if(match1 == 0 && strcmp(opt->name, "dimlevel-maximum") == 0 &&
													opt->vartype == JSON_NUMBER && (int)(intptr_t)opt->def < (int)dimfrom) {
													logprintf(LOG_ERR, "device \"%s\" dimlevel can't be set to \"%d\"", jbchild->string_, (int)dimfrom);
													return -1;
												}
												if(match2 == 0 && strcmp(opt->name, "dimlevel-minimum") == 0 &&
													opt->vartype == JSON_NUMBER && (int)(intptr_t)opt->def > (int)dimfrom) {
													logprintf(LOG_ERR, "device \"%s\" dimlevel can't be set to \"%d\"", jbchild->string_, (int)dimfrom);
													return -1;
												}
												opt = opt->next;
											}
											tmp_protocols = tmp_protocols->next;
										}
									}
								} else {
									logprintf(LOG_ERR, "device \"%s\" doesn't support dimming", jbchild->string_);
									return -1;
								}
							} else {
								logprintf(LOG_ERR, "device \"%s\" doesn't exists", jbchild->string_);
								return -1;
							}
						jechild = jechild->next;
						}
					}
				} else {
					logprintf(LOG_ERR, "internal error 3 in dim action", jbchild->string_);
					return -1;
				}
			} else {
				logprintf(LOG_ERR, "internal error 4 in dim action", jbchild->string_);
				return -1;
			}
			jbchild = jbchild->next;
		}
	} else {
		logprintf(LOG_ERR, "internal error 5 in dim action", jbchild->string_);
		return -1;
	}
	return 0;
}

static void *thread(void *param) {
	struct event_action_thread_t *pth = (struct event_action_thread_t *)param;
	struct JsonNode *json = pth->obj->arguments;
	struct JsonNode *jedimlevel = NULL;
	struct JsonNode *jsdimlevel = NULL;
	struct JsonNode *jto = NULL;
	struct JsonNode *jafter = NULL;
	struct JsonNode *jfor = NULL;
	struct JsonNode *jfrom = NULL;
	struct JsonNode *jin = NULL;
	struct JsonNode *javalues = NULL;
	struct JsonNode *jcvalues = NULL;
	struct JsonNode *jdvalues = NULL;
	struct JsonNode *jevalues = NULL;
	struct JsonNode *jfvalues = NULL;
	struct JsonNode *jaseconds = NULL;
	struct JsonNode *jiseconds = NULL;
	struct JsonNode *jvalues = NULL;
	char *old_state = NULL, **array = NULL;
	double dimlevel = 0.0, old_dimlevel = 0.0, new_dimlevel = 0.0, cur_dimlevel = 0.0;
	int seconds_after = 0, seconds_for = 0, seconds_in = 0, has_in = 0, dimdiff = 0;
	int type_for = 0, type_after = 0, type_in = 0;
	int direction = 0, interval = 0, timer = 0;
	int	l = 0, i = 0, nrunits = (sizeof(units)/sizeof(units[0]));
	char state[3];

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

	if((jin = json_find_member(json, "IN")) != NULL) {
		if((jfvalues = json_find_member(jin, "value")) != NULL) {
			jiseconds = json_find_element(jfvalues, 0);
			if(jiseconds != NULL) {
				if(jiseconds->tag == JSON_STRING) {
					l = explode(jiseconds->string_, " ", &array);
					if(l == 2) {
						for(i=0;i<nrunits;i++) {
							if(strcmp(array[1], units[i].name) == 0) {
								seconds_in = atoi(array[0]);
								type_in = units[i].id;
								has_in = 1;
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

	switch(type_in) {
		case 3:
			seconds_in *= 60;
		break;
		case 4:
			seconds_in *= (60*60);
		break;
		case 5:
			seconds_in *= (60*60*24);
		break;
	}

	if(pilight.debuglevel == 1) {
		fprintf(stderr, "action dim: for %d:%d, after %d:%d, in: %d:%d\n", seconds_for, type_for, seconds_after, type_after, seconds_in, type_in);
	}
	/* Store current state and dimlevel */
	struct devices_t *tmp = pth->device;
	int match1 = 0, match2 = 0;
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
					match1 = 1;
				}
			}
			if(strcmp(opt->name, "dimlevel") == 0) {
				if(opt->values->type == JSON_NUMBER) {
					cur_dimlevel = opt->values->number_;
					match2 = 1;
				}
			}
			if(match1 == 1 && match2 == 1) {
				break;
			}
			opt = opt->next;
		}
		if(match1 == 1 && match2 == 1) {
			break;
		}
		tmp = tmp->next;
	}
	if(match1 == 0) {
		logprintf(LOG_NOTICE, "could not store old state of \"%s\"", pth->device->id);
	}
	if(match2 == 0) {
		logprintf(LOG_NOTICE, "could not store old dimlevel of \"%s\"", pth->device->id);
	}

	if((jfrom = json_find_member(json, "FROM")) != NULL) {
		if((jevalues = json_find_member(jfrom, "value")) != NULL) {
			jsdimlevel = json_find_element(jevalues, 0);
			if(jsdimlevel != NULL) {
				if(jsdimlevel->tag == JSON_NUMBER) {
					old_dimlevel = (int)jsdimlevel->number_;
				}
			}
		}
	}

	if((jto = json_find_member(json, "TO")) != NULL) {
		if((javalues = json_find_member(jto, "value")) != NULL) {
			jedimlevel = json_find_element(javalues, 0);
			if(jedimlevel != NULL) {
				if(jedimlevel->tag == JSON_NUMBER) {
					new_dimlevel = (int)jedimlevel->number_;
					dimlevel = (int)jedimlevel->number_;
					strcpy(state,  "on");
				}
			}
		}
	}

	if(pilight.debuglevel == 1) {
		fprintf(stderr, "action dim: old %d, new %d, direction %d\n", (int)old_dimlevel, (int)new_dimlevel, direction);
	}

	if(has_in == 1) {
		if(old_dimlevel < new_dimlevel) {
			direction = INCREASING;
			dimdiff = (int)(new_dimlevel - old_dimlevel);
			interval = (int)(seconds_in / dimdiff);
		} else if(old_dimlevel > new_dimlevel) {
			direction = DECREASING;
			dimdiff = (int)(old_dimlevel - new_dimlevel);
			interval = (int)(seconds_in / dimdiff);
		} else {
			dimdiff = 0;
			interval = 0;
		}
	}

	/*
	 * We'll switch from first dimlevel to second dimlevel after X seconds
	 * and switch back after X seconds.
	 */
	if(has_in == 0) {
		if(old_state == NULL || ((strcmp(old_state, "on") != 0 || (int)cur_dimlevel != (int)new_dimlevel))) {
			timer = 0;
			while(pth->loop == 1) {
				if(timer == seconds_after) {
					jvalues = json_mkobject();
					json_append_member(jvalues, "dimlevel", json_mknumber(dimlevel, 0));
					if(pilight.control != NULL) {
						pilight.control(pth->device, state, json_first_child(jvalues), ACTION);
					}
					json_delete(jvalues);
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
			if(seconds_for > 0 && old_state != NULL && (strcmp(old_state, "on") != 0 || (int)cur_dimlevel != (int)new_dimlevel)) {
				timer = 0;
				while(pth->loop == 1) {
					if(seconds_for == timer) {
						jvalues = json_mkobject();
						json_append_member(jvalues, "dimlevel", json_mknumber(cur_dimlevel, 0));
						if(pilight.control != NULL) {
							if(strcmp(old_state, "off") == 0) {
								pilight.control(pth->device, state, json_first_child(jvalues), ACTION);
								pilight.control(pth->device, old_state, NULL, ACTION);
							} else {
								pilight.control(pth->device, old_state, json_first_child(jvalues), ACTION);
							}
						}
						json_delete(jvalues);
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
		}
	/* We'll gently start increasing / decreasing the dimlevel after X seconds in X seconds. */
	} else if(interval > 0) {
		timer = 0;
		while(pth->loop == 1) {
			if(timer == seconds_after) {
				break;
			}
			timer++;
			if(type_after > 1) {
				sleep(1);
			} else {
				usleep(1000);
			}
		}
		if(direction == INCREASING) {
			for(i=(int)old_dimlevel;i<=(int)new_dimlevel;i++) {
				timer = 0;
				while(pth->loop == 1) {
					if(interval == timer || i == (int)old_dimlevel) {
						jvalues = json_mkobject();
						json_append_member(jvalues, "dimlevel", json_mknumber(i, 0));
						if(pilight.control != NULL) {
							pilight.control(pth->device, state, json_first_child(jvalues), ACTION);
						}
						json_delete(jvalues);
						break;
					}
					timer++;
					if(type_in > 1) {
						sleep(1);
					} else {
						usleep(1000);
					}
				}
			}
		} else {
			for(i=(int)old_dimlevel;i>=(int)new_dimlevel;i--) {
				timer = 0;
				while(pth->loop == 1) {
					if(interval == timer || i == (int)old_dimlevel) {
						jvalues = json_mkobject();
						json_append_member(jvalues, "dimlevel", json_mknumber(i, 0));
						if(pilight.control != NULL) {
							pilight.control(pth->device, state, json_first_child(jvalues), ACTION);
						}
						json_delete(jvalues);
						break;
					}
					timer++;
					if(type_in > 1) {
						sleep(1);
					} else {
						usleep(1000);
					}
				}
			}
		}
		/*
		 * We only need to restore the state if it was actually changed
		 */
		if(seconds_for > 0 && old_state != NULL &&
			 (strcmp(old_state, "on") != 0 || (int)cur_dimlevel != (int)new_dimlevel)) {
			timer = 0;
			while(pth->loop == 1) {
				if(timer == seconds_for) {
					jvalues = json_mkobject();
					json_append_member(jvalues, "dimlevel", json_mknumber(cur_dimlevel, 0));
					if(pilight.control != NULL) {
						if(strcmp(old_state, "off") == 0) {
							pilight.control(pth->device, state, json_first_child(jvalues), ACTION);
							pilight.control(pth->device, old_state, NULL, ACTION);
						} else {
							pilight.control(pth->device, old_state, json_first_child(jvalues), ACTION);
						}
					}
					json_delete(jvalues);
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
	}

	if(old_state != NULL) {
		FREE(old_state);
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
						event_action_thread_start(dev, action_dim->name, thread, obj);
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
void actionDimInit(void) {
	event_action_register(&action_dim, "dim");

	options_add(&action_dim->options, 'a', "DEVICE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_dim->options, 'b', "TO", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&action_dim->options, 'c', "FROM", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&action_dim->options, 'd', "FOR", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_dim->options, 'e', "AFTER", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_dim->options, 'f', "IN", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	action_dim->run = &run;
	action_dim->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "dim";
	module->version = "3.5";
	module->reqversion = "6.0";
	module->reqcommit = "152";
}

void init(void) {
	actionDimInit();
}
#endif
