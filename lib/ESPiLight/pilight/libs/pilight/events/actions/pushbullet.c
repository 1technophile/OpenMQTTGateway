/*
	Copyright (C) 2013 - 2015 CurlyMo

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
#include "pushbullet.h"

static int checkArguments(struct rules_actions_t *obj) {
	struct JsonNode *jtitle = NULL;
	struct JsonNode *jbody = NULL;
	struct JsonNode *jtype = NULL;
	struct JsonNode *jtoken = NULL;
	struct JsonNode *jvalues = NULL;
	struct JsonNode *jchild = NULL;
	int nrvalues = 0;

	jtitle = json_find_member(obj->arguments, "TITLE");
	jbody = json_find_member(obj->arguments, "BODY");
	jtoken = json_find_member(obj->arguments, "TOKEN");
	jtype = json_find_member(obj->arguments, "TYPE");

	if(jtitle == NULL) {
		logprintf(LOG_ERR, "pushbullet action is missing a \"TITLE\"");
		return -1;
	}
	if(jbody == NULL) {
		logprintf(LOG_ERR, "pushbullet action is missing a \"BODY\"");
		return -1;
	}
	if(jtype == NULL) {
		logprintf(LOG_ERR, "pushbullet action is missing a \"TYPE\"");
		return -1;
	}
	if(jtoken == NULL) {
		logprintf(LOG_ERR, "pushbullet action is missing a \"TOKEN\"");
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
		logprintf(LOG_ERR, "pushbullet action \"TITLE\" only takes one argument");
		return -1;
	}
	nrvalues = 0;
	if((jvalues = json_find_member(jbody, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "pushbullet action \"BODY\" only takes one argument");
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
		logprintf(LOG_ERR, "pushbullet action \"TOKEN\" only takes one argument");
		return -1;
	}
	nrvalues = 0;
	if((jvalues = json_find_member(jtype, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "pushbullet action \"TYPE\" only takes one argument");
		return -1;
	}
	return 0;
}

static void callback(int code, char *data, int size, char *type, void *userdata) {
	if(code == 200) {
		logprintf(LOG_DEBUG, "pushbullet action succeeded with message: %s", data);
	} else {
		logprintf(LOG_NOTICE, "pushbullet action failed (%d) with message: %s", code, data);
	}
}

static void *thread(void *param) {
	struct rules_actions_t *pth = (struct rules_actions_t *)param;
	// struct rules_t *obj = pth->obj;
	struct JsonNode *arguments = pth->arguments;
	struct JsonNode *jtitle = NULL;
	struct JsonNode *jbody = NULL;
	struct JsonNode *jtype = NULL;
	struct JsonNode *jtoken = NULL;
	struct JsonNode *jvalues1 = NULL;
	struct JsonNode *jvalues2 = NULL;
	struct JsonNode *jvalues3 = NULL;
	struct JsonNode *jvalues4 = NULL;
	struct JsonNode *jval1 = NULL;
	struct JsonNode *jval2 = NULL;
	struct JsonNode *jval3 = NULL;
	struct JsonNode *jval4 = NULL;

	char url[1024];

	action_pushbullet->nrthreads++;

	jtitle = json_find_member(arguments, "TITLE");
	jbody = json_find_member(arguments, "BODY");
	jtoken = json_find_member(arguments, "TOKEN");
	jtype = json_find_member(arguments, "TYPE");

	if(jtitle != NULL && jbody != NULL && jtoken != NULL && jtype != NULL) {
		jvalues1 = json_find_member(jtitle, "value");
		jvalues2 = json_find_member(jbody, "value");
		jvalues3 = json_find_member(jtoken, "value");
		jvalues4 = json_find_member(jtype, "value");
		if(jvalues1 != NULL && jvalues2 != NULL && jvalues3 != NULL && jvalues4 != NULL) {
			jval1 = json_find_element(jvalues1, 0);
			jval2 = json_find_element(jvalues2, 0);
			jval3 = json_find_element(jvalues3, 0);
			jval4 = json_find_element(jvalues4, 0);
			if(jval1 != NULL && jval2 != NULL && jval3 != NULL && jval4 != NULL &&
			 jval1->tag == JSON_STRING && jval2->tag == JSON_STRING &&
			 jval3->tag == JSON_STRING && jval4->tag == JSON_STRING) {
				snprintf(url, 1024, "https://%s@api.pushbullet.com/v2/pushes", jval3->string_);

				struct JsonNode *code = json_mkobject();
				json_append_member(code, "type", json_mkstring(jval4->string_));
				json_append_member(code, "title", json_mkstring(jval1->string_));
				json_append_member(code, "body", json_mkstring(jval2->string_));

				char *content = json_stringify(code, "\t");
				json_delete(code);

				http_post_content(url, "application/json", content, callback, NULL);

				json_free(content);
			}
		}
	}


	action_pushbullet->nrthreads--;

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
void actionPushbulletInit(void) {
	event_action_register(&action_pushbullet, "pushbullet");

	options_add(&action_pushbullet->options, 'a', "TITLE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_pushbullet->options, 'b', "BODY", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_pushbullet->options, 'c', "TOKEN", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_pushbullet->options, 'd', "TYPE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	action_pushbullet->run = &run;
	action_pushbullet->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "pushbullet";
	module->version = "2.2";
	module->reqversion = "5.0";
	module->reqcommit = "87";
}

void init(void) {
	actionPushbulletInit();
}
#endif
