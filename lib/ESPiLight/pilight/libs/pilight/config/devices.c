/*
	Copyright (C) 2013 CurlyMo

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
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#ifndef _WIN32
	#include <regex.h>
#endif
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>

#include "../core/threads.h"
#include "../core/common.h"
#include "../core/log.h"
#include "../core/options.h"
#include "../core/config.h"
#include "../core/ssdp.h"
#include "../core/firmware.h"
#include "../core/datetime.h"

#include "../protocols/protocol.h"

#include "defines.h"
#include "devices.h"
#include "gui.h"

static pthread_mutex_t mutex_lock;
static pthread_mutexattr_t mutex_attr;

struct config_t *config_devices;

/* Struct to store the locations */
static struct devices_t *devices = NULL;

int devices_update(char *protoname, JsonNode *json, enum origin_t origin, JsonNode **out) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	/* The pointer to the devices devices */
	struct devices_t *dptr = devices;
	/* The pointer to the device settings */
	struct devices_settings_t *sptr = NULL;
	/* The pointer to the device settings */
	struct devices_values_t *vptr = NULL;
	/* The pointer to the registered protocols */
	struct protocol_t *protocol = NULL;
	/* The pointer to the protocol options */
	struct options_t *opt = NULL;
	/* Get the message part of the sended code */
	JsonNode *message = json_find_member(json, "message");
	/* Get the settings part of the sended code */
	JsonNode *settings = json_find_member(json, "settings");
	/* The return JSON object will all updated devices */
	JsonNode *rroot = json_mkobject();
	JsonNode *rdev = json_mkarray();
	JsonNode *rval = json_mkobject();

	/* Temporarily char pointer */
	char *stmp = NULL;
	/* Temporarily int */
	double itmp;
	/* Do we need to update the devices file */
	unsigned short update = 0;
	/* The new state value */
	char vstring_[255];
	double vnumber_ = -1;
	int vdecimals_ = 0;
	char sstring_[255];
	double snumber_ = -1;
	int sdecimals_ = 0;
	int stateType = 0;
	int valueType = 0;
	/* The UUID of this device */
	char *uuid = NULL;

	/* Make sure the character pointers are empty */
	memset(sstring_, '\0', sizeof(sstring_));
	memset(vstring_, '\0', sizeof(vstring_));

	/* Check if the found settings matches the send code */
	unsigned int match = 0, match1 = 0, match2 = 0;

	/* Is is a valid new state / value */
	int is_valid = 1;

	/* Retrieve the used protocol */
	struct protocols_t *pnode = protocols;
	while(pnode) {
		protocol = pnode->listener;
		if(strcmp(protocol->id, protoname) == 0) {
			break;
		}
		pnode = pnode->next;
	}

	time_t timenow = time(NULL);
	struct tm gmt;
	memset(&gmt, '\0', sizeof(struct tm));
#ifdef _WIN32
	struct tm *tm;
	tm = gmtime(&timenow);
	memcpy(&gmt, tm, sizeof(struct tm));
#else
	gmtime_r(&timenow, &gmt);
#endif
	time_t utct = datetime2ts(gmt.tm_year+1900, gmt.tm_mon+1, gmt.tm_mday, gmt.tm_hour, gmt.tm_min, gmt.tm_sec);
	json_append_member(rval, "timestamp", json_mknumber((double)utct, 0));

	json_find_string(json, "uuid", &uuid);

	if((opt = protocol->options)) {
		/* Loop through all devices */

		while(dptr) {
			/*
			 * uuid 				= The UUID of the pilight instance that received the specific information.
			 * pilight_uuid	= The UUID of the currently running pilight instance this function was called on.
			 * dev_uuid 		= The UUID of the device set by the user or the UUID of pilight instance that read the config.
			 * ori_uuid			= The UUID of the pilight instance that parsed the config (mostly the master).
			 * cst_uuid			= The UUID manually set the UUID of these devices
			 *
			 */
			int uuidmatch = 0;
			if(uuid != NULL && dptr->dev_uuid != NULL && dptr->ori_uuid != NULL && strlen(pilight_uuid) > 0) {
				if(dptr->cst_uuid == 1) {
					/* If the user forced the device UUID and it matches the UUID of the recieved code */
					if(strcmp(dptr->dev_uuid, uuid) == 0) {
						uuidmatch = 1;
					}
				} else {
					uuidmatch = 1;
				}
			} else if(uuid == NULL || strlen(pilight_uuid) == 0) {
				uuidmatch = 1;
			}
			if(uuidmatch == 1) {
				struct protocols_t *tmp_protocols = dptr->protocols;
				match = 0;
				while(tmp_protocols) {
					if(protocol_device_exists(protocol, tmp_protocols->name) == 0) {
						match = 1;
						break;
					}
					tmp_protocols = tmp_protocols->next;
				}

				if(match == 1) {
					sptr = dptr->settings;
					/* Loop through all settings */
					while(sptr) {
						match1 = 0; match2 = 0;

						/* Check how many id's we need to match */
						opt = protocol->options;
						while(opt) {
							if(opt->conftype == DEVICES_ID) {
								JsonNode *jtmp = json_first_child(message);
								while(jtmp) {
									if(strcmp(jtmp->key, opt->name) == 0) {
										match1++;
									}
									jtmp = jtmp->next;
								}
							}
							opt = opt->next;
						}

						/* Loop through all protocol options */
						opt = protocol->options;
						while(opt) {
							if(opt->conftype == DEVICES_ID && strcmp(sptr->name, "id") == 0) {
								/* Check the devices id's to match a device */
								vptr = sptr->values;
								while(vptr) {
									if(strcmp(vptr->name, opt->name) == 0) {
										if(json_find_string(message, opt->name, &stmp) == 0 &&
										   vptr->type == JSON_STRING &&
										   strcmp(stmp, vptr->string_) == 0) {
											match2++;
										}
										if(json_find_number(message, opt->name, &itmp) == 0 &&
										   vptr->type == JSON_NUMBER &&
										   fabs(vptr->number_-itmp) < EPSILON) {
											match2++;
										}
									}
									vptr = vptr->next;
								}
							}

							/* Retrieve the new device state */
							if(opt->conftype == DEVICES_STATE) {
								if(opt->argtype == OPTION_NO_VALUE) {
									if(json_find_string(message, "state", &stmp) == 0) {
										strcpy(sstring_, stmp);
										stateType = JSON_STRING;
									}
									if(json_find_number(message, "state", &itmp) == 0) {
										snumber_ = itmp;
										stateType = JSON_NUMBER;
									}
								} else if(opt->argtype == OPTION_HAS_VALUE) {
									if(json_find_string(message, opt->name, &stmp) == 0) {
										strcpy(sstring_, stmp);
										stateType = JSON_STRING;
									}
									struct JsonNode *jtmp = NULL;
									if((jtmp = json_find_member(message, opt->name)) != NULL &&
									    jtmp->tag == JSON_NUMBER) {
										snumber_ = jtmp->number_;
										sdecimals_ = jtmp->decimals_;
										stateType = JSON_NUMBER;
									}
								}
							}
							opt = opt->next;
						}
						if(match1 > 0 && match2 > 0 && match1 == match2) {
							break;
						}
						sptr = sptr->next;
					}
					is_valid = 1;

					/* If we matched a device, update it's state */
					if(match1 > 0 && match2 > 0 && match1 == match2) {
						if(protocol->checkValues) {
							is_valid = 0;
							JsonNode *jcode = json_mkobject();
							sptr = dptr->settings;
							while(sptr) {
								opt = protocol->options;
								/* Loop through all protocol options */
								while(opt) {
									/* Check if there are values that can be updated */
									if(strcmp(sptr->name, opt->name) == 0
									   && (opt->conftype == DEVICES_VALUE || opt->conftype == DEVICES_OPTIONAL)
									   && opt->argtype == OPTION_HAS_VALUE) {
										memset(vstring_, '\0', sizeof(vstring_));
										vnumber_ = -1;
										if(json_find_string(message, opt->name, &stmp) == 0) {
											strcpy(vstring_, stmp);
											valueType = JSON_STRING;
											is_valid = 1;
										}
										struct JsonNode *jtmp = NULL;
										if((jtmp = json_find_member(message, opt->name)) != NULL &&
										    jtmp->tag == JSON_NUMBER) {
											vnumber_ = jtmp->number_;
											vdecimals_ = jtmp->decimals_;
											valueType = JSON_NUMBER;
											is_valid = 1;
										}

										/* Check if the protocol settings of this device are valid to
										   make sure no errors occur in the config.json. */
										JsonNode *jsettings = json_first_child(settings);
										while(jsettings) {
											if(jsettings->tag == JSON_NUMBER) {
												json_append_member(jcode, jsettings->key, json_mknumber(jsettings->number_, jsettings->decimals_));
											} else if(jsettings->tag == JSON_STRING) {
												json_append_member(jcode, jsettings->key, json_mkstring(jsettings->string_));
											}
											jsettings = jsettings->next;
										}
										if(valueType == JSON_STRING) {
											json_append_member(jcode, opt->name, json_mkstring(vstring_));
										} else {
											json_append_member(jcode, opt->name, json_mknumber(vnumber_, vdecimals_));
										}
									}
									opt = opt->next;
								}
								sptr = sptr->next;
							}
							if(protocol->checkValues(jcode) != 0) {
								is_valid = 0;
							}
							json_delete(jcode);
						}

						sptr = dptr->settings;
						while(sptr) {
							opt = protocol->options;
							/* Loop through all protocol options */
							while(opt) {
								/* Check if there are values that can be updated */
								if(strcmp(sptr->name, opt->name) == 0
								   && (opt->conftype == DEVICES_VALUE || opt->conftype == DEVICES_OPTIONAL)
								   && opt->argtype == OPTION_HAS_VALUE) {
									int upd_value = 1;
									memset(vstring_, '\0', sizeof(vstring_));
									vnumber_ = -1;
									vdecimals_ = 0;
									struct JsonNode *jtmp = NULL;
									if(json_find_string(message, opt->name, &stmp) == 0) {
										strcpy(vstring_, stmp);
										valueType = JSON_STRING;
									} else if((jtmp = json_find_member(message, opt->name)) != NULL &&
									           jtmp->tag == JSON_NUMBER) {
										vnumber_ = jtmp->number_;
										vdecimals_ = jtmp->decimals_;
										valueType = JSON_NUMBER;
									} else {
										upd_value = 0;
									}

									if(is_valid == 1 && upd_value == 1) {
										if(valueType == JSON_STRING &&
										   strlen(vstring_) > 0 &&
										   sptr->values->type == JSON_STRING &&
										   strcmp(sptr->values->string_, vstring_) != 0) {
											if((sptr->values->string_ = REALLOC(sptr->values->string_, strlen(vstring_)+1)) == NULL) {
												fprintf(stderr, "out of memory\n");
												exit(EXIT_FAILURE);
											}
											strcpy(sptr->values->string_, vstring_);
											sptr->values->type = JSON_STRING;
										} else if(valueType == JSON_NUMBER &&
												  sptr->values->type == JSON_NUMBER &&
												  fabs(sptr->values->number_-vnumber_) >= EPSILON) {
											sptr->values->number_ = vnumber_;
											sptr->values->decimals = vdecimals_;
											sptr->values->type = JSON_NUMBER;
										}
										if(sptr->values->type == JSON_STRING && json_find_string(rval, sptr->name, &stmp) != 0) {
											json_append_member(rval, sptr->name, json_mkstring(sptr->values->string_));
											update = 1;
										} else if(sptr->values->type == JSON_NUMBER && json_find_number(rval, sptr->name, &itmp) != 0) {
											json_append_member(rval, sptr->name, json_mknumber(sptr->values->number_, sptr->values->decimals));
											update = 1;
										}
										dptr->timestamp = utct;
									}
									//break;
								}
								opt = opt->next;
							}

							/* Check if we need to update the state */
							if(strcmp(sptr->name, "state") == 0) {
								if((stateType == JSON_STRING &&
									sptr->values->type == JSON_STRING &&
									strcmp(sptr->values->string_, sstring_) != 0)) {
									if((sptr->values->string_ = REALLOC(sptr->values->string_, strlen(sstring_)+1)) == NULL) {
										fprintf(stderr, "out of memory\n");
										exit(EXIT_FAILURE);
									}
									strcpy(sptr->values->string_, sstring_);
									sptr->values->type = JSON_STRING;
									dptr->timestamp = utct;
									update = 1;
								} else if((stateType == JSON_NUMBER &&
										   sptr->values->type == JSON_NUMBER &&
										   fabs(sptr->values->number_-snumber_) < EPSILON)) {
									sptr->values->number_ = snumber_;
									sptr->values->decimals = sdecimals_;
									sptr->values->type = JSON_NUMBER;
									dptr->timestamp = utct;
									update = 1;
								}
								if(sptr->values->type == JSON_STRING && json_find_string(rval, sptr->name, &stmp) != 0) {
									json_append_member(rval, sptr->name, json_mkstring(sptr->values->string_));
								} else if(sptr->values->type == JSON_NUMBER && json_find_number(rval, sptr->name, &itmp) != 0) {
									json_append_member(rval, sptr->name, json_mknumber(sptr->values->number_, sptr->values->decimals));
								}
								//break;
							}
							if(update == 1) {
								match = 0;
								struct JsonNode *jchild = json_first_child(rdev);
								while(jchild) {
									if(jchild->tag == JSON_STRING && strcmp(dptr->id, jchild->string_) == 0) {
										match = 1;
										break;
									}
									jchild = jchild->next;
								}
								if(match == 0) {
									dptr->prevorigin = dptr->lastorigin;
									dptr->lastorigin = origin;
#ifdef EVENTS
									/*
									* If the action itself it not triggering a device update, something
									* else is. We therefor need to abort the running action to let
									* the new state persist.
									*/
									if(dptr->action_thread->running == 1 && origin != ACTION) {
										/*
										 * In case of Z-Wave, the ACTION is always followed by a RECEIVER origin due to
										 * its feedback feature. We do not want to abort or action in these cases.
										 */
										if(!((dptr->protocols->listener->hwtype == ZWAVE) && dptr->lastorigin == RECEIVER && dptr->prevorigin == ACTION) || 
										   dptr->protocols->listener->hwtype != ZWAVE) {
											event_action_thread_stop(dptr);
										}
									}

									/*
									* We store the rule number that triggered the device change.
									* The eventing library can then check if the same rule is
									* triggered again so infinite loops can be prevented.
									*/
									if(origin == ACTION) {
										if(dptr->action_thread->obj != NULL) {
											dptr->prevrule = dptr->lastrule;
											dptr->lastrule = dptr->action_thread->obj->rule->nr;
										}
									} else {
										dptr->lastrule = -1;
										dptr->prevrule = -1;
									}
#endif
									json_append_element(rdev, json_mkstring(dptr->id));
								}
							}
							sptr = sptr->next;
						}
					}
				}
			}
			dptr = dptr->next;
		}
	}

	if(update == 1) {
		json_append_member(rroot, "origin", json_mkstring("update"));
		json_append_member(rroot, "type",  json_mknumber((int)protocol->devtype, 0));
		//if(strlen(pilight_uuid) > 0 && (protocol->hwtype == SENSOR || protocol->hwtype == HWRELAY)) {
			json_append_member(rroot, "uuid",  json_mkstring(pilight_uuid));
		//}
		json_append_member(rroot, "devices", rdev);
		json_append_member(rroot, "values", rval);

		*out = rroot;
	} else {
		json_delete(rdev);
		json_delete(rval);
		json_delete(rroot);
	}

	return (update == 1) ? 0 : -1;
}

int devices_get(char *sid, struct devices_t **dev) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct devices_t *dptr = NULL;

	dptr = devices;
	while(dptr) {
		if(strcmp(dptr->id, sid) == 0) {
			if(dev != NULL) {
				*dev = dptr;
			}
			return 0;
		}
		dptr = dptr->next;
	}

	return 1;
}

int devices_valid_state(char *sid, char *state) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct devices_t *dptr = NULL;
	struct protocol_t *protocol = NULL;
	struct options_t *options = NULL;
	struct protocols_t *tmp_protocol = NULL;

	if(devices_get(sid, &dptr) == 0) {
		tmp_protocol = dptr->protocols;
		while(tmp_protocol) {
			protocol = tmp_protocol->listener;
			if(protocol->options) {
				options = protocol->options;
				while(options) {
					if(strcmp(options->name, state) == 0 && options->conftype == DEVICES_STATE) {
						return 0;
						break;
					}
					options = options->next;
				}
			}
			tmp_protocol = tmp_protocol->next;
		}
	}
	return 1;
}

int devices_valid_value(char *sid, char *name, char *value) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct devices_t *dptr = NULL;
	struct options_t *opt = NULL;
	struct protocols_t *tmp_protocol = NULL;
#if !defined(__FreeBSD__) && !defined(_WIN32)
	regex_t regex;
	int reti = 0;
	memset(&regex, '\0', sizeof(regex));
#endif

	if(devices_get(sid, &dptr) == 0) {
		tmp_protocol = dptr->protocols;
		while(tmp_protocol) {
			opt = tmp_protocol->listener->options;
			while(opt) {
				if((opt->conftype == DEVICES_VALUE || opt->conftype == DEVICES_OPTIONAL) && strcmp(name, opt->name) == 0) {
#if !defined(__FreeBSD__) && !defined(_WIN32)
					if(opt->mask != NULL) {
						reti = regcomp(&regex, opt->mask, REG_EXTENDED);
						if(reti) {
							logprintf(LOG_ERR, "%s: could not compile %s regex", tmp_protocol->listener->id, opt->name);
							exit(EXIT_FAILURE);
						}
						reti = regexec(&regex, value, 0, NULL, 0);
						if(reti == REG_NOMATCH || reti != 0) {
							regfree(&regex);
							return 1;
						}
						regfree(&regex);
					}
#endif
					return 0;
				}
				opt = opt->next;
			}
			tmp_protocol = tmp_protocol->next;
		}
	}
	return 1;
}

struct JsonNode *devices_values(const char *media) {
	/* Temporary pointer to the different structure */
	struct devices_t *tmp_devices = NULL;
	struct devices_settings_t *tmp_settings = NULL;
	struct devices_values_t *tmp_values = NULL;
	struct gui_values_t *gui_values = NULL;

	/* Pointers to the newly created JSON object */
	struct JsonNode *jroot = json_mkarray();
	struct JsonNode *jelement = NULL;
	struct JsonNode *jvalues = NULL;
	struct JsonNode *jdevices = NULL;
	struct options_t *opt = NULL;

	int match = 0;

	tmp_devices = devices;

	while(tmp_devices) {
		match = 0;
		if((gui_values = gui_media(tmp_devices->id)) != NULL) {
			while(gui_values) {
				if(gui_values->type == JSON_STRING) {
					if(strcmp(gui_values->string_, media) == 0 ||
						 strcmp(gui_values->string_, "all") == 0 ||
						 strcmp(media, "all") == 0) {
							match = 1;
					}
				}
				gui_values = gui_values->next;
			}
		} else {
			match = 1;
		}
		if(strcmp(media, "all") == 0) {
			match = 1;
		}
		if(match == 1) {
			jelement = json_mkobject();
			jdevices = json_mkarray();
			jvalues = json_mkobject();

			struct protocols_t *tmp_protocols = tmp_devices->protocols;
			json_append_member(jelement, "type", json_mknumber(tmp_protocols->listener->devtype, 0));
			json_append_element(jdevices, json_mkstring(tmp_devices->id));
			json_append_member(jelement, "devices", jdevices);

			json_append_member(jvalues, "timestamp", json_mknumber(tmp_devices->timestamp, 0));

			tmp_settings = tmp_devices->settings;
			while(tmp_settings) {
				if(strcmp(tmp_settings->name, "state") == 0) {
					tmp_values = tmp_settings->values;
					if(tmp_values->type == JSON_NUMBER) {
						json_append_member(jvalues, tmp_settings->name, json_mknumber(tmp_values->number_, tmp_values->decimals));
					} else if(tmp_values->type == JSON_STRING) {
						json_append_member(jvalues, tmp_settings->name, json_mkstring(tmp_values->string_));
					}
				}
				tmp_settings = tmp_settings->next;
			}

			while(tmp_protocols) {
				opt = tmp_protocols->listener->options;
				while(opt) {
					if(opt->conftype == DEVICES_VALUE || opt->conftype == DEVICES_OPTIONAL) {
						tmp_settings = tmp_devices->settings;
						while(tmp_settings) {
							if(strcmp(tmp_settings->name, opt->name) == 0) {
								tmp_values = tmp_settings->values;
								if(tmp_values->type == JSON_NUMBER) {
									json_append_member(jvalues, tmp_settings->name, json_mknumber(tmp_values->number_, tmp_values->decimals));
								} else if(tmp_values->type == JSON_STRING) {
									json_append_member(jvalues, tmp_settings->name, json_mkstring(tmp_values->string_));
								}
							}
							tmp_settings = tmp_settings->next;
						}
					}
					opt = opt->next;
				}
				tmp_protocols = tmp_protocols->next;
			}
			json_append_member(jelement, "values", jvalues);
			json_append_element(jroot, jelement);
		}
		tmp_devices = tmp_devices->next;
	}

	return jroot;
}

struct JsonNode *devices_sync(int level, const char *media) {
	/* Temporary pointer to the different structure */
	struct devices_t *tmp_devices = NULL;
	struct devices_settings_t *tmp_settings = NULL;
	struct devices_values_t *tmp_values = NULL;
	struct gui_values_t *gui_values = NULL;
	int match = 0;

	/* Pointers to the newly created JSON object */
	struct JsonNode *jroot = json_mkobject();
	struct JsonNode *jdevice = NULL;
	struct JsonNode *joptions = NULL;
	struct JsonNode *jid = NULL;
	struct options_t *tmp_options = NULL;

	tmp_devices = devices;

	while(tmp_devices) {
		match = 0;
		if((gui_values = gui_media(tmp_devices->id)) != NULL) {
			while(gui_values) {
				if(gui_values->type == JSON_STRING) {
					if(strcmp(gui_values->string_, media) == 0 ||
						 strcmp(gui_values->string_, "all") == 0 ||
						 strcmp(media, "all") == 0) {
							match = 1;
					}
				}
				gui_values = gui_values->next;
			}
		} else {
			match = 1;
		}
		if(strcmp(media, "all") == 0) {
			match = 1;
		}
		if(match == 1) {
			jdevice = json_mkobject();

			struct protocols_t *tmp_protocols = tmp_devices->protocols;
			struct JsonNode *jprotocols = json_mkarray();

			if(level == CONFIG_INTERNAL || (strlen(pilight_uuid) > 0 &&
				(strcmp(tmp_devices->ori_uuid, pilight_uuid) == 0) &&
				(strcmp(tmp_devices->dev_uuid, pilight_uuid) != 0))
				|| tmp_devices->cst_uuid == 1) {
				json_append_member(jdevice, "uuid", json_mkstring(tmp_devices->dev_uuid));
			}
			if(level == CONFIG_INTERNAL || level == CONFIG_INTERNAL) {
				json_append_member(jdevice, "origin", json_mkstring(tmp_devices->ori_uuid));
				json_append_member(jdevice, "timestamp", json_mknumber((double)tmp_devices->timestamp, 0));
			}

			while(tmp_protocols) {
				json_append_element(jprotocols, json_mkstring(tmp_protocols->name));
				tmp_protocols = tmp_protocols->next;
			}
			json_append_member(jdevice, "protocol", jprotocols);
			json_append_member(jdevice, "id", json_mkarray());

			tmp_settings = tmp_devices->settings;
			while(tmp_settings) {
				tmp_values = tmp_settings->values;
				if(strcmp(tmp_settings->name, "id") == 0) {
					jid = json_find_member(jdevice, tmp_settings->name);
					JsonNode *jnid = json_mkobject();
					while(tmp_values) {
						if(tmp_values->type == JSON_NUMBER) {
							json_append_member(jnid, tmp_values->name, json_mknumber(tmp_values->number_, tmp_values->decimals));
						} else if(tmp_values->type == JSON_STRING) {
							json_append_member(jnid, tmp_values->name, json_mkstring(tmp_values->string_));
						}
						tmp_values = tmp_values->next;
					}
					json_append_element(jid, jnid);
				} else if(!tmp_values->next) {
					if(tmp_values->type == JSON_NUMBER) {
						json_append_member(jdevice, tmp_settings->name, json_mknumber(tmp_values->number_, tmp_values->decimals));
					} else if(tmp_values->type == JSON_STRING) {
						json_append_member(jdevice, tmp_settings->name, json_mkstring(tmp_values->string_));
					}
				} else {
					joptions = json_mkarray();
					while(tmp_values) {
						if(tmp_values->type == JSON_NUMBER) {
							json_append_element(joptions, json_mknumber(tmp_values->number_, tmp_values->decimals));
						} else if(tmp_values->type == JSON_STRING) {
							json_append_element(joptions, json_mkstring(tmp_values->string_));
						}
						tmp_values = tmp_values->next;
					}
					json_append_member(jdevice, tmp_settings->name, joptions);
				}
				tmp_settings = tmp_settings->next;
			}

			tmp_protocols = tmp_devices->protocols;
			while(tmp_protocols) {
				tmp_options = tmp_protocols->listener->options;
				if(tmp_options) {
					while(tmp_options) {
						if((level == CONFIG_INTERNAL || level == CONFIG_INTERNAL) && (tmp_options->conftype == DEVICES_SETTING)
						&& json_find_member(jdevice, tmp_options->name) == NULL) {
							if(tmp_options->vartype == JSON_NUMBER) {
								json_append_member(jdevice, tmp_options->name, json_mknumber((int)(intptr_t)tmp_options->def, 0));
							} else if(tmp_options->vartype == JSON_STRING) {
								json_append_member(jdevice, tmp_options->name, json_mkstring((char *)tmp_options->def));
							}
						}
						tmp_options = tmp_options->next;
					}
				}
				tmp_protocols = tmp_protocols->next;
			}
			json_append_member(jroot, tmp_devices->id, jdevice);
		}
		tmp_devices = tmp_devices->next;
	}

	return jroot;
}

/* Save the device settings to the device struct */
static void devices_save_setting(int i, struct JsonNode *jsetting, struct devices_t *device) {
	/* Struct to store the values */
	struct devices_values_t *vnode = NULL;
	struct devices_settings_t *snode = NULL;
	struct devices_settings_t *tmp_settings = NULL;
	struct devices_values_t *tmp_values = NULL;
	/* Temporary JSON pointer */
	struct JsonNode *jtmp;

	/* Variable holder for casting settings */
	char *stmp = NULL;

	/* If the JSON tag is an array, then it should be a values or id array */
	if(jsetting->tag == JSON_ARRAY) {
		if(strcmp(jsetting->key, "id") == 0) {
			/* Loop through the values of this values array */
			jtmp = json_first_child(jsetting);
			while(jtmp) {
				if((snode = MALLOC(sizeof(struct devices_settings_t))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if((snode->name = MALLOC(strlen(jsetting->key)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(snode->name, jsetting->key);
				snode->values = NULL;
				snode->next = NULL;
				if(jtmp->tag == JSON_OBJECT) {
					JsonNode *jtmp1 = json_first_child(jtmp);
					while(jtmp1) {
						if((vnode = MALLOC(sizeof(struct devices_values_t))) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						if((vnode->name = MALLOC(strlen(jtmp1->key)+1)) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						strcpy(vnode->name, jtmp1->key);
						vnode->next = NULL;
						if(jtmp1->tag == JSON_STRING) {
							if((vnode->string_ = MALLOC(strlen(jtmp1->string_)+1)) == NULL) {
								fprintf(stderr, "out of memory\n");
								exit(EXIT_FAILURE);
							}
							strcpy(vnode->string_, jtmp1->string_);
							vnode->type = JSON_STRING;
						} else if(jtmp1->tag == JSON_NUMBER) {
							vnode->number_ = jtmp1->number_;
							vnode->decimals = jtmp1->decimals_;
							vnode->type = JSON_NUMBER;
						}
						if(jtmp1->tag == JSON_NUMBER || jtmp1->tag == JSON_STRING) {
							tmp_values = snode->values;
							if(tmp_values) {
								while(tmp_values->next != NULL) {
									tmp_values = tmp_values->next;
								}
								tmp_values->next = vnode;
							} else {
								vnode->next = snode->values;
								snode->values = vnode;
							}
						}
						jtmp1 = jtmp1->next;
					}
				}
				jtmp = jtmp->next;

				tmp_settings = device->settings;
				if(tmp_settings) {
					while(tmp_settings->next != NULL) {
						tmp_settings = tmp_settings->next;
					}
					tmp_settings->next = snode;
				} else {
					snode->next = device->settings;
					device->settings = snode;
				}
			}
		}
	} else if(jsetting->tag == JSON_OBJECT) {
		if((snode = MALLOC(sizeof(struct devices_settings_t))) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		if((snode->name = MALLOC(strlen(jsetting->key)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(snode->name, jsetting->key);
		snode->values = NULL;
		snode->next = NULL;

		jtmp = json_first_child(jsetting);
		while(jtmp) {
			if(jtmp->tag == JSON_STRING) {
				if((vnode = MALLOC(sizeof(struct devices_values_t))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if((vnode->name = MALLOC(strlen(jtmp->key)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(vnode->name, jtmp->key);
				if((vnode->string_ = MALLOC(strlen(jtmp->string_)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(vnode->string_, jtmp->string_);
				vnode->type = JSON_STRING;
				vnode->next = NULL;
			} else if(jtmp->tag == JSON_NUMBER) {
				if((vnode = MALLOC(sizeof(struct devices_values_t))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if((vnode->name = MALLOC(strlen(jtmp->key)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(vnode->name, jtmp->key);
				vnode->number_ = jtmp->number_;
				vnode->decimals = jtmp->decimals_;
				vnode->type = JSON_NUMBER;
				vnode->next = NULL;
			}
			if(jtmp->tag == JSON_NUMBER || jtmp->tag == JSON_STRING) {
				tmp_values = snode->values;
				if(tmp_values) {
					while(tmp_values->next != NULL) {
						tmp_values = tmp_values->next;
					}
					tmp_values->next = vnode;
				} else {
					vnode->next = snode->values;
					snode->values = vnode;
				}
			}
			jtmp = jtmp->next;
		}

		tmp_settings = device->settings;
		if(tmp_settings) {
			while(tmp_settings->next != NULL) {
				tmp_settings = tmp_settings->next;
			}
			tmp_settings->next = snode;
		} else {
			snode->next = device->settings;
			device->settings = snode;
		}

	} else {
		/* New device settings node */
		if((snode = MALLOC(sizeof(struct devices_settings_t))) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		if((snode->name = MALLOC(strlen(jsetting->key)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(snode->name, jsetting->key);
		snode->values = NULL;
		snode->next = NULL;

		if((vnode = MALLOC(sizeof(struct devices_values_t))) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		int valid = 0;
		/* Cast and store the new value */
		if(jsetting->tag == JSON_STRING && json_find_string(jsetting->parent, jsetting->key, &stmp) == 0) {
			if((vnode->string_ = MALLOC(strlen(stmp)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			vnode->name = NULL;
			strcpy(vnode->string_, stmp);
			vnode->type = JSON_STRING;
			valid = 1;
		} else if(jsetting->tag == JSON_NUMBER &&
		         (jtmp = json_find_member(jsetting->parent, jsetting->key)) != NULL &&
				 jtmp->tag == JSON_NUMBER) {
			vnode->name = NULL;
			vnode->number_ = jtmp->number_;
			vnode->decimals = jtmp->decimals_;
			vnode->type = JSON_NUMBER;
			valid = 1;
		}
		if(valid) {
			tmp_values = snode->values;
			if(tmp_values) {
				while(tmp_values->next != NULL) {
					tmp_values = tmp_values->next;
				}
				tmp_values->next = vnode;
			} else {
				vnode->next = snode->values;
				snode->values = vnode;
			}
		}

		tmp_settings = device->settings;
		if(tmp_settings) {
			while(tmp_settings->next != NULL) {
				tmp_settings = tmp_settings->next;
			}
			tmp_settings->next = snode;
		} else {
			snode->next = device->settings;
			device->settings = snode;
		}
	}
}

static int devices_check_id(int i, JsonNode *jsetting, struct devices_t *device) {
	/* Temporary options pointer */
	struct options_t *tmp_options = NULL;
	/* Temporary ID array pointer */
	struct JsonNode *jid = NULL;
	/* Temporary ID values pointer */
	struct JsonNode *jvalues = NULL;
	/* Temporary protocols pointer */
	struct protocols_t *tmp_protocols = NULL;

	int match1 = 0, match2 = 0, match3 = 0, has_id = 0;
	int valid_values = 0, nrprotocols = 0, nrids1 = 0;
	unsigned short nrid = 0;
	int nrids2 = 0, have_error = 0, etype = 0;

	/* Variable holders for casting */
	char ctmp[256];

	tmp_protocols = device->protocols;
	while(tmp_protocols) {
		jid = json_first_child(jsetting);
		has_id = 0;
		nrid = 0;
		match3 = 0;
		nrprotocols++;
		while(jid) {
			nrid++;
			match2 = 0; match1 = 0; nrids1 = 0; nrids2 = 0;
			jvalues = json_first_child(jid);
			while(jvalues) {
				nrids1++;
				jvalues = jvalues->next;
			}
			if((tmp_options = tmp_protocols->listener->options)) {
				while(tmp_options) {
					if(tmp_options->conftype == DEVICES_ID) {
						nrids2++;
					}
					tmp_options = tmp_options->next;
				}
			}
			if(nrids1 == nrids2) {
				has_id = 1;
				jvalues = json_first_child(jid);
				while(jvalues) {
					match1++;
					if((tmp_options = tmp_protocols->listener->options)) {
						while(tmp_options) {
							if(tmp_options->conftype == DEVICES_ID && tmp_options->vartype == jvalues->tag) {
								if(strcmp(tmp_options->name, jvalues->key) == 0) {
									match2++;
									if(jvalues->tag == JSON_NUMBER) {
										sprintf(ctmp, "%.*f", jvalues->decimals_, jvalues->number_);
									} else if(jvalues->tag == JSON_STRING) {
										strcpy(ctmp, jvalues->string_);
									}

									if(tmp_options->mask != NULL && strlen(tmp_options->mask) > 0) {
#if !defined(__FreeBSD__) && !defined(_WIN32)
										regex_t regex;
										memset(&regex, '\0', sizeof(regex));
										int reti = regcomp(&regex, tmp_options->mask, REG_EXTENDED);
										if(reti) {
											logprintf(LOG_ERR, "%s: could not compile %s regex", tmp_protocols->listener->id, tmp_options->name);
										} else {
											reti = regexec(&regex, ctmp, 0, NULL, 0);
											if(reti == REG_NOMATCH || reti != 0) {
												match2--;
											}
											regfree(&regex);
										}
#endif
									}
								}
							}
							tmp_options = tmp_options->next;
						}
					}
					jvalues = jvalues->next;
				}
			}
			jid = jid->next;
			if(match2 > 0 && match1 == match2) {
				match3 = 1;
			}
		}
		if(!has_id) {
			valid_values--;
		} else if(tmp_protocols->listener->multipleId == 0 && nrid > 1) {
			valid_values--;
			etype = 1;
			break;
		} else if(match3) {
			valid_values++;
		} else {
			valid_values--;
		}
		tmp_protocols = tmp_protocols->next;
	}
	if(nrprotocols != valid_values) {
		if(etype == 1) {
			logprintf(LOG_ERR, "protocol \"%s\" used in \"%s\" doesn't support multiple id's", tmp_protocols->listener->id, device->id);
		} else {
			logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, "id", device->id);
		}
		have_error = 1;
	}
	return have_error;
}

static int devices_validate_settings(void) {
	/* Temporary pointer to the different structure */
	struct devices_t *tmp_devices = NULL;
	struct devices_settings_t *tmp_settings = NULL;
	struct devices_values_t *tmp_values = NULL;

	/* Pointers to the newly created JSON object */
	struct JsonNode *jdevice = NULL;
	struct JsonNode *joptions = NULL;

	int have_error = 0;
	int dorder = 0;

	tmp_devices = devices;
	while(tmp_devices) {

		jdevice = json_mkobject();
		struct protocols_t *tmp_protocols = tmp_devices->protocols;
		while(tmp_protocols) {
			/* Only continue if protocol specific settings can be validated */
			if(tmp_protocols->listener->checkValues) {
				tmp_settings = tmp_devices->settings;
				dorder++;
				while(tmp_settings) {
					tmp_values = tmp_settings->values;
					/* Retrieve all protocol specific settings for this device. Also add all
					   device values and states so it can be validated by the protocol */
					if(strcmp(tmp_settings->name, "id") == 0) {
						JsonNode *jid = json_find_member(jdevice, "id");
						if(!jid) {
							jid = json_mkarray();
							json_append_member(jdevice, tmp_settings->name, jid);
						}
						JsonNode *jnid = json_mkobject();
						while(tmp_values) {
							if(tmp_values->type == JSON_NUMBER) {
								json_append_member(jnid, tmp_values->name, json_mknumber(tmp_values->number_, tmp_values->decimals));
							} else if(tmp_values->type == JSON_STRING) {
								json_append_member(jnid, tmp_values->name, json_mkstring(tmp_values->string_));
							}
							tmp_values = tmp_values->next;
						}
						json_append_element(jid, jnid);
					} else {
						if(!tmp_values->next) {
							if(tmp_values->type == JSON_STRING) {
								json_append_member(jdevice, tmp_settings->name, json_mkstring(tmp_values->string_));
							} else if(tmp_values->type == JSON_NUMBER) {
								json_append_member(jdevice, tmp_settings->name, json_mknumber(tmp_values->number_, tmp_values->decimals));
							}
						} else {
							joptions = json_mkarray();
							while(tmp_values) {
								if(tmp_values->type == JSON_STRING) {
									json_append_element(joptions, json_mkstring(tmp_values->string_));
								} else if(tmp_values->type == JSON_NUMBER) {
									json_append_element(joptions, json_mknumber(tmp_values->number_, tmp_values->decimals));
								}
								tmp_values = tmp_values->next;
							}
							json_append_member(jdevice, tmp_settings->name, joptions);
						}
					}
					tmp_settings = tmp_settings->next;
				}

				/* Let the settings and values be validated against each other */
				if(tmp_protocols->listener->checkValues(jdevice) != 0) {
					logprintf(LOG_ERR, "config device #%d, invalid", dorder);
					have_error = 1;
					goto clear;
				}
			}
			tmp_protocols = tmp_protocols->next;
		}
		tmp_devices = tmp_devices->next;
		json_delete(jdevice);
		jdevice=NULL;
	}

clear:
	if(jdevice) {
		json_delete(jdevice);
	}
	return have_error;
}

static int devices_check_state(int i, JsonNode *jsetting, struct devices_t *device) {
	/* Temporary options pointer */
	struct options_t *tmp_options;

	int valid_state = 0, have_error = 0;

	/* Variable holders for casting */
	double itmp = 0;
	char ctmp[256];
	char *stmp = NULL;

#if !defined(__FreeBSD__) && !defined(_WIN32)
	/* Regex variables */
	regex_t regex;
	int reti = 0;
	memset(&regex, '\0', sizeof(regex));
#endif

	/* Cast the different values */
	if(jsetting->tag == JSON_NUMBER && json_find_number(jsetting->parent, jsetting->key, &itmp) == 0) {
		sprintf(ctmp, "%.0f", itmp);
	} else if(jsetting->tag == JSON_STRING && json_find_string(jsetting->parent, jsetting->key, &stmp) == 0) {
		strcpy(ctmp, stmp);
	}

	struct protocols_t *tmp_protocols = device->protocols;
	while(tmp_protocols) {
		/* Check if the specific device contains any options */
		if(tmp_protocols->listener->options) {

			tmp_options = tmp_protocols->listener->options;

			while(tmp_options) {
				/* We are only interested in the DEVICES_STATE options */
				if(tmp_options->conftype == DEVICES_STATE && tmp_options->vartype == jsetting->tag) {
					/* If an option requires an argument, then check if the
					   argument state values and values array are of the right
					   type. This is done by checking the regex mask */
					if(tmp_options->argtype == OPTION_HAS_VALUE) {
						if(tmp_options->mask != NULL && strlen(tmp_options->mask) > 0) {
#if !defined(__FreeBSD__) && !defined(_WIN32)
							reti = regcomp(&regex, tmp_options->mask, REG_EXTENDED);
							if(reti) {
								logprintf(LOG_ERR, "%s: could not compile %s regex", tmp_protocols->listener->id, tmp_options->name);
								have_error = 1;
								goto clear;
							}
							reti = regexec(&regex, ctmp, 0, NULL, 0);

							if(reti == REG_NOMATCH || reti != 0) {
								logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, jsetting->key, device->id);
								have_error = 1;
								regfree(&regex);
								goto clear;
							}
							regfree(&regex);
#endif
						}
					} else {
						/* If a protocol has DEVICES_STATE arguments, than these define
						   the states a protocol can take. Check if the state value
						   match, these protocol states */

						if(strcmp(tmp_options->name, ctmp) == 0 && valid_state == 0) {
							valid_state = 1;
						}
					}
				}
				tmp_options = tmp_options->next;
			}
		}
		tmp_protocols = tmp_protocols->next;
	}

	if(valid_state == 0) {
		logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, jsetting->key, device->id);
		have_error = 1;
		goto clear;
	}

clear:
	return have_error;
}

static int devices_parse_elements(JsonNode *jdevices, struct devices_t *device) {
	/* Temporary settings holder */
	struct devices_settings_t *tmp_settings = NULL;
	/* JSON devices iterator */
	JsonNode *jsettings = NULL;
	/* Temporarily options pointer */
	struct options_t *tmp_options = NULL;
	/* Temporarily protocols pointer */
	struct protocols_t *tmp_protocols = NULL;

	int i = 0, have_error = 0, valid_setting = 0;
	int	match = 0, has_state = 0;
	/* Check for any duplicate fields */
	int nrprotocol = 0, nrstate = 0;
	int nruuid = 0, nrorigin = 0, nrtimestamps = 0;

	jsettings = json_first_child(jdevices);

	while(jsettings) {
		i++;
		/* Check for any duplicate fields */
		if(strcmp(jsettings->key, "uuid") == 0) {
			nruuid++;
		}
		if(strcmp(jsettings->key, "origin") == 0) {
			nrorigin++;
		}
		if(strcmp(jsettings->key, "protocol") == 0) {
			nrprotocol++;
		}
		if(strcmp(jsettings->key, "state") == 0) {
			nrstate++;
		}
		if(strcmp(jsettings->key, "timestamp") == 0) {
			nrtimestamps++;
		}
		if(nrstate > 1 || nrprotocol > 1 || nruuid > 1 || nrtimestamps > 1) {
			logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", duplicate", i, jsettings->key, device->id);
			have_error = 1;
			goto clear;
		}

		/* Parse the state setting separately from the other settings. */
		if(strcmp(jsettings->key, "state") == 0) {
			if(jsettings->tag == JSON_STRING || jsettings->tag == JSON_NUMBER) {
				if(devices_check_state(i, jsettings, device) != 0) {
					have_error = 1;
					goto clear;
				}
				devices_save_setting(i, jsettings, device);
			} else {
				logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, jsettings->key, device->id);
				have_error = 1;
				goto clear;
			}
		} else if(strcmp(jsettings->key, "id") == 0) {
			if(jsettings->tag == JSON_ARRAY) {
				if(devices_check_id(i, jsettings, device) == EXIT_FAILURE) {
					have_error = 1;
					goto clear;
				}
				devices_save_setting(i, jsettings, device);
			} else {
				logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, jsettings->key, device->id);
				have_error = 1;
				goto clear;
			}
		} else if(strcmp(jsettings->key, "uuid") == 0) {
			if(jsettings->tag == JSON_STRING) {
				strcpy(device->dev_uuid, jsettings->string_);
				device->cst_uuid = 1;
			} else {
				logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, jsettings->key, device->id);
				have_error = 1;
				goto clear;
			}
		} else if(strcmp(jsettings->key, "timestamp") == 0) {
			if(jsettings->tag == JSON_NUMBER) {
				device->timestamp = (int)jsettings->number_;
			} else {
				logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, jsettings->key, device->id);
				have_error = 1;
				goto clear;
			}
		} else if(strcmp(jsettings->key, "origin") == 0) {
			if(jsettings->tag == JSON_STRING) {
				strcpy(device->ori_uuid, jsettings->string_);
			} else {
				logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, jsettings->key, device->id);
				have_error = 1;
				goto clear;
			}
		/* The protocol and name settings are already saved in the device struct */
		} else if(!((strcmp(jsettings->key, "protocol") == 0 && jsettings->tag == JSON_ARRAY)
			|| (strcmp(jsettings->key, "uuid") == 0 && jsettings->tag == JSON_STRING)
			|| (strcmp(jsettings->key, "timestamp") == 0 && jsettings->tag == JSON_NUMBER))
			|| ((strcmp(jsettings->key, "id") == 0) && jsettings->tag == JSON_ARRAY)) {

			/* Check for duplicate settings */
			tmp_settings = device->settings;
			while(tmp_settings) {
				if(strcmp(tmp_settings->name, jsettings->key) == 0) {
					logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", duplicate", i, jsettings->key, device->id);
					have_error = 1;
					goto clear;
				}
				tmp_settings = tmp_settings->next;
			}

			tmp_protocols = device->protocols;
			valid_setting = 0;
			while(tmp_protocols) {
				/* Check if the settings are required by the protocol */
				if(tmp_protocols->listener->options) {
					tmp_options = tmp_protocols->listener->options;
					while(tmp_options) {
						if(strcmp(jsettings->key, tmp_options->name) == 0
						   && (tmp_options->conftype == DEVICES_ID
						       || tmp_options->conftype == DEVICES_VALUE
							   || tmp_options->conftype == DEVICES_SETTING
							   || tmp_options->conftype == DEVICES_OPTIONAL)
						   && (tmp_options->vartype == jsettings->tag
							     || tmp_options->vartype == (JSON_STRING | JSON_NUMBER))) {
							valid_setting = 1;
							break;
						}
						tmp_options = tmp_options->next;
					}
				}
				tmp_protocols = tmp_protocols->next;
			}
			/* If the settings are valid, store them */
			if(valid_setting == 1) {
				devices_save_setting(i, jsettings, device);
			} else {
				logprintf(LOG_ERR, "config device setting #%d \"%s\" of \"%s\", invalid", i, jsettings->key, device->id);
				have_error = 1;
				goto clear;
			}
		}
		jsettings = jsettings->next;
	}

	/* Check if required options by this protocol are missing
	   in the devices file */
	tmp_protocols = device->protocols;
	while(tmp_protocols) {
		if(tmp_protocols->listener->options) {
			tmp_options = tmp_protocols->listener->options;
			while(tmp_options) {
				match = 0;
				jsettings = json_first_child(jdevices);
				while(jsettings) {
					if(strcmp(jsettings->key, tmp_options->name) == 0) {
						match = 1;
						break;
					}
					jsettings = jsettings->next;
				}
				if(match == 0 && tmp_options->conftype == DEVICES_VALUE) {
					logprintf(LOG_ERR, "config device setting \"%s\" of \"%s\", missing", tmp_options->name, device->id);
					have_error = 1;
					goto clear;
				}
			tmp_options = tmp_options->next;
			}

		}
		tmp_protocols = tmp_protocols->next;
	}
	match = 0;
	jsettings = json_first_child(jdevices);
	while(jsettings) {
		if(strcmp(jsettings->key, "id") == 0) {
			match = 1;
			break;
		}
		jsettings = jsettings->next;
	}
	if(match == 0) {
		logprintf(LOG_ERR, "config device setting \"id\" of \"%s\", missing", device->id);
		have_error = 1;
		goto clear;
	}

	/* Check if this protocol requires a state setting */
	if(nrstate == 0) {
		tmp_protocols = device->protocols;
		while(tmp_protocols) {
			if(tmp_protocols->listener->options) {
				tmp_options = tmp_protocols->listener->options;
				while(tmp_options) {
					if(tmp_options->conftype == DEVICES_STATE) {
						has_state = 1;
						break;
					}
					tmp_options = tmp_options->next;
				}
			}
			if(has_state == 1) {
				if(nrstate == 0) {
					logprintf(LOG_ERR, "config device setting \"%s\" of \"%s\", missing", "state", device->id);
					have_error = 1;
					goto clear;
				}
			}
			tmp_protocols = tmp_protocols->next;
		}
	}

	if(strlen(pilight_uuid) > 0 && strcmp(device->dev_uuid, pilight_uuid) == 0) {
		tmp_protocols = device->protocols;
		while(tmp_protocols) {
			if(tmp_protocols->listener->initDev && (tmp_protocols->listener->masterOnly == 0 || pilight.runmode == STANDALONE)) {
				struct threadqueue_t *tmp = tmp_protocols->listener->initDev(jdevices);
				if(tmp != NULL) {
					device->protocol_threads = REALLOC(device->protocol_threads, (sizeof(struct threadqueue_t *)*(size_t)(device->nrthreads+1)));
					device->protocol_threads[device->nrthreads] = tmp;
					device->nrthreads++;
				}
			}
			tmp_protocols = tmp_protocols->next;
		}
	}

clear:
	return have_error;
}

static int devices_parse(JsonNode *root) {
	/* Struct to store the devices */
	struct devices_t *dnode = NULL;
	/* Temporary JSON devices  */
	struct devices_t *tmp_devices = NULL;
	/* Temporary protocol JSON array */
	JsonNode *jprotocol = NULL;
	JsonNode *jprotocols = NULL;
	/* JSON devices iterator */
	JsonNode *jdevices = NULL;

	int i = 0, have_error = 0, match = 0, x = 0;

	jdevices = json_first_child(root);
	while(jdevices) {
		i++;
		/* Check if all fields of the devices are of the right type */
		if(jdevices->tag != JSON_OBJECT) {
			logprintf(LOG_ERR, "config device device #%d \"%s\", invalid field(s)", i, jdevices->key);
			have_error = 1;
			goto clear;
		} else {
			if((jprotocols = json_find_member(jdevices, "protocol")) == NULL || jprotocols->tag != JSON_ARRAY) {
				logprintf(LOG_ERR, "config device #%d \"%s\", missing protocol", i, jdevices->key);
				have_error = 1;
				goto clear;
			 } else {
				for(x=0;x<strlen(jdevices->key);x++) {
					if(!isalnum(jdevices->key[x]) && jdevices->key[x] != '-' && jdevices->key[x] != '_') {
						logprintf(LOG_ERR, "config device #%d \"%s\", not alphanumeric", i, jdevices->key);
						have_error = 1;
						goto clear;
					}
					struct protocols_t *tmp_protocols = protocols;
					while(tmp_protocols) {
						struct protocol_t *protocol = tmp_protocols->listener;
						if(strcmp(protocol->id, jdevices->key) == 0) {
							logprintf(LOG_ERR, "config device #%d \"%s\", protocol names are reserved words", i, jdevices->key);
							have_error = 1;
							goto clear;
						}
						tmp_protocols = tmp_protocols->next;
					}
				}
				/* Check for duplicate fields */
				tmp_devices = devices;
				while(tmp_devices) {
					if(strcmp(tmp_devices->id, jdevices->key) == 0) {
						logprintf(LOG_ERR, "config device #%d \"%s\", duplicate", i, jdevices->key);
						have_error = 1;
					}
					tmp_devices = tmp_devices->next;
				}

				if((dnode = MALLOC(sizeof(struct devices_t))) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				if(strlen(pilight_uuid) > 0) {
					strcpy(dnode->dev_uuid, pilight_uuid);
					strcpy(dnode->ori_uuid, pilight_uuid);
				}
				dnode->cst_uuid = 0;
				if((dnode->id = MALLOC(strlen(jdevices->key)+1)) == NULL) {
					fprintf(stderr, "out of memory\n");
					exit(EXIT_FAILURE);
				}
				strcpy(dnode->id, jdevices->key);
				dnode->nrthreads = 0;
				dnode->timestamp = 0;
				dnode->protocol_threads = NULL;
				dnode->settings = NULL;
				dnode->next = NULL;
				dnode->protocols = NULL;

#ifdef EVENTS
				event_action_thread_init(dnode);
#endif

				int ptype = -1;
				/* Save both the protocol pointer and the protocol name */
				jprotocol = json_first_child(jprotocols);
				while(jprotocol) {
					match = 0;
					struct protocols_t *tmp_protocols = protocols;
					/* Pointer to the match protocol */
					struct protocol_t *protocol = NULL;
					while(tmp_protocols) {
						protocol = tmp_protocols->listener;
						if(protocol_device_exists(protocol, jprotocol->string_) == 0 && match == 0
						   && protocol->config == 1) {
							if(ptype == -1) {
								ptype = protocol->hwtype;
								match = 1;
							}
							if(ptype > -1 && protocol->hwtype == ptype) {
								match = 1;
							}
							match = 1;
							break;
						}
						tmp_protocols = tmp_protocols->next;
					}
					if(match == 1 && ptype != protocol->hwtype) {
						logprintf(LOG_ERR, "config device #%d \"%s\", cannot combine protocols of different hardware types", i, jdevices->key);
						have_error = 1;
					}
					if(match == 0) {
						logprintf(LOG_ERR, "config device #%d \"%s\", invalid protocol", i, jdevices->key);
						have_error = 1;
					} else {
						struct protocols_t *pnode = MALLOC(sizeof(struct protocols_t));
						if(pnode == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						if((pnode->listener = MALLOC(sizeof(struct protocol_t))) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						memcpy(pnode->listener, protocol, sizeof(struct protocol_t));
						if((pnode->name = MALLOC(strlen(jprotocol->string_)+1)) == NULL) {
							fprintf(stderr, "out of memory\n");
							exit(EXIT_FAILURE);
						}
						strcpy(pnode->name, jprotocol->string_);
						pnode->next = NULL;
						tmp_protocols = dnode->protocols;
						if(tmp_protocols) {
							while(tmp_protocols->next != NULL) {
								tmp_protocols = tmp_protocols->next;
							}
							tmp_protocols->next = pnode;
						} else {
							pnode->next = dnode->protocols;
							dnode->protocols = pnode;
						}
					}
					jprotocol = jprotocol->next;
				}

				if(have_error == 0 && devices_parse_elements(jdevices, dnode) != 0) {
					have_error = 1;
				}

				tmp_devices = devices;
				if(tmp_devices) {
					while(tmp_devices->next != NULL) {
						tmp_devices = tmp_devices->next;
					}
					tmp_devices->next = dnode;
				} else {
					dnode->next = devices;
					devices = dnode;
				}

				if(have_error) {
					goto clear;
				}
			}
		}

		jdevices = jdevices->next;
	}

clear:
	return have_error;
}

int devices_gc(void) {
	int i = 0;
	struct devices_t *dtmp = NULL;
	struct devices_settings_t *stmp = NULL;
	struct devices_values_t *vtmp = NULL;
	struct protocols_t *ptmp = NULL;

	pthread_mutex_lock(&mutex_lock);
	/* Free devices structure */
	while(devices) {
		dtmp = devices;

#ifdef EVENTS
		event_action_thread_free(dtmp);
#endif

		while(dtmp->settings) {
			stmp = dtmp->settings;
			while(stmp->values) {
				vtmp = stmp->values;
				if(vtmp->type == JSON_STRING && vtmp->string_ != NULL) {
					FREE(vtmp->string_);
				}
				if(vtmp->name) {
					FREE(vtmp->name);
				}
				stmp->values = stmp->values->next;
				FREE(vtmp);
			}
			if(stmp->values != NULL) {
				FREE(stmp->values);
			}
			if(stmp->name) {
				FREE(stmp->name);
			}
			dtmp->settings = dtmp->settings->next;
			FREE(stmp);
		}
		while(dtmp->protocols) {
			ptmp = dtmp->protocols;
			if(ptmp->listener != NULL && ptmp->listener->threadGC != NULL) {
				ptmp->listener->threadGC();
			}
			if(ptmp->name != NULL) {
				FREE(ptmp->name);
			}
			if(ptmp->listener != NULL) {
				FREE(ptmp->listener);
			}
			dtmp->protocols = dtmp->protocols->next;
			FREE(ptmp);
		}
		if(dtmp->nrthreads > 0) {
			for(i=0;i<dtmp->nrthreads;i++) {
				thread_stop(dtmp->protocol_threads[i]->id);
			}
		}
		if(dtmp->protocols != NULL) {
			FREE(dtmp->protocols);
		}
		if(dtmp->settings != NULL) {
			FREE(dtmp->settings);
		}
		if(dtmp->id != NULL) {
			FREE(dtmp->id);
		}
		if(dtmp->protocol_threads != NULL) {
			FREE(dtmp->protocol_threads);
		}
		devices = devices->next;
		FREE(dtmp);
	}
	if(devices != NULL) {
		FREE(devices);
	}
	devices = NULL;

	pthread_mutex_unlock(&mutex_lock);
	logprintf(LOG_DEBUG, "garbage collected config devices library");

	return EXIT_SUCCESS;
}

static int devices_read(JsonNode *root) {
	if(devices_parse(root) == 0 && devices_validate_settings() == 0) {
		return 0;
	} else {
		return 1;
	}
}

void devices_init(void) {
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex_lock, &mutex_attr);

	/* Request hardware json object in main configuration */
	config_register(&config_devices, "devices");
	config_devices->readorder = 1;
	config_devices->writeorder = 0;
	config_devices->parse=&devices_read;
	config_devices->sync=&devices_sync;
	config_devices->gc=&devices_gc;
}
