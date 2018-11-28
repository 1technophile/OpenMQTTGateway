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

#include "../../core/threads.h"
#include "../action.h"
#include "../../core/options.h"
#include "../../config/devices.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "../../core/http.h"
#include "../../core/common.h"
#include "pushover.h"

static int checkArguments(struct rules_actions_t *obj) {
	struct JsonNode *jtitle = NULL;
	struct JsonNode *jmessage = NULL;
	struct JsonNode *juser = NULL;
	struct JsonNode *jtoken = NULL;
	struct JsonNode *jvalues = NULL;
	struct JsonNode *jchild = NULL;
	int nrvalues = 0;

	jtitle = json_find_member(obj->arguments, "TITLE");
	jmessage = json_find_member(obj->arguments, "MESSAGE");
	jtoken = json_find_member(obj->arguments, "TOKEN");
	juser = json_find_member(obj->arguments, "USER");

	if(jtitle == NULL) {
		logprintf(LOG_ERR, "pushover action is missing a \"TITLE\"");
		return -1;
	}
	if(jmessage == NULL) {
		logprintf(LOG_ERR, "pushover action is missing a \"MESSAGE\"");
		return -1;
	}
	if(juser == NULL) {
		logprintf(LOG_ERR, "pushover action is missing a \"USER\"");
		return -1;
	}
	if(jtoken == NULL) {
		logprintf(LOG_ERR, "pushover action is missing a \"TOKEN\"");
		return -1;
	}
	nrvalues = 0;
	if((jvalues = json_find_member(jtitle, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "pushover action \"TITLE\" only takes one argument");
		return -1;
	}
	nrvalues = 0;
	if((jvalues = json_find_member(jmessage, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "pushover action \"MESSAGE\" only takes one argument");
		return -1;
	}
	nrvalues = 0;
	if((jvalues = json_find_member(jtoken, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "pushover action \"TOKEN\" only takes one argument");
		return -1;
	}
	nrvalues = 0;
	if((jvalues = json_find_member(juser, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "pushover action \"USER\" only takes one argument");
		return -1;
	}
	return 0;
}

static void callback(int code, char *data, int size, char *type, void *userdata) {
	if(code == 200) {
		logprintf(LOG_DEBUG, "pushover action succeeded with message: %s", data);
	} else {
		logprintf(LOG_NOTICE, "pushover action failed (%d) with message: %s", code, data);
	}
}

static void *thread(void *param) {
	struct rules_actions_t *pth = (struct rules_actions_t *)param;
	// struct rules_t *obj = pth->obj;
	struct JsonNode *arguments = pth->arguments;
	struct JsonNode *jtitle = NULL;
	struct JsonNode *jmessage = NULL;
	struct JsonNode *juser = NULL;
	struct JsonNode *jtoken = NULL;
	struct JsonNode *jvalues1 = NULL;
	struct JsonNode *jvalues2 = NULL;
	struct JsonNode *jvalues3 = NULL;
	struct JsonNode *jvalues4 = NULL;
	struct JsonNode *jval1 = NULL;
	struct JsonNode *jval2 = NULL;
	struct JsonNode *jval3 = NULL;
	struct JsonNode *jval4 = NULL;

	action_pushover->nrthreads++;

	char url[1024];

	jtitle = json_find_member(arguments, "TITLE");
	jmessage = json_find_member(arguments, "MESSAGE");
	jtoken = json_find_member(arguments, "TOKEN");
	juser = json_find_member(arguments, "USER");

	if(jtitle != NULL && jmessage != NULL && jtoken != NULL && juser != NULL) {
		jvalues1 = json_find_member(jtitle, "value");
		jvalues2 = json_find_member(jmessage, "value");
		jvalues3 = json_find_member(jtoken, "value");
		jvalues4 = json_find_member(juser, "value");
		if(jvalues1 != NULL && jvalues2 != NULL && jvalues3 != NULL && jvalues4 != NULL) {
			jval1 = json_find_element(jvalues1, 0);
			jval2 = json_find_element(jvalues2, 0);
			jval3 = json_find_element(jvalues3, 0);
			jval4 = json_find_element(jvalues4, 0);
			if(jval1 != NULL && jval2 != NULL && jval3 != NULL && jval4 != NULL &&
			 jval1->tag == JSON_STRING && jval2->tag == JSON_STRING &&
			 jval3->tag == JSON_STRING && jval4->tag == JSON_STRING) {
				strcpy(url, "https://api.pushover.net/1/messages.json");
				char *message = urlencode(jval2->string_);
				char *token = urlencode(jval3->string_);
				char *user = urlencode(jval4->string_);
				char *title = urlencode(jval1->string_);
				size_t l = strlen(message)+strlen(token);
				l += strlen(user)+strlen(title);
				l += strlen("token=")+strlen("&user=");
				l += strlen("&message=")+strlen("&title=");
				char content[l+2];
				sprintf(content, "token=%s&user=%s&title=%s&message=%s", token, user, title, message);

				http_post_content(url, "application/x-www-form-urlencoded", content, callback, NULL);

				FREE(message);
				FREE(token);
				FREE(user);
				FREE(title);
			}
		}
	}

	action_pushover->nrthreads--;

	return (void *)NULL;
}

static int run(struct rules_actions_t *obj) {
	pthread_t pth;
	threads_create(&pth, NULL, thread, (void *)obj);
	pthread_detach(pth);
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void actionPushoverInit(void) {
	event_action_register(&action_pushover, "pushover");

	options_add(&action_pushover->options, 'a', "TITLE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_pushover->options, 'b', "MESSAGE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_pushover->options, 'c', "TOKEN", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_pushover->options, 'd', "USER", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	action_pushover->run = &run;
	action_pushover->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "pushover";
	module->version = "2.3";
	module->reqversion = "5.0";
	module->reqcommit = "87";
}

void init(void) {
	actionPushoverInit();
}
#endif
