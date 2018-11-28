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
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include <ctype.h>

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/json.h"
#include "../core/log.h"
#include "../events/events.h"
#include "../events/operator.h"
#include "../events/action.h"
#include "../events/function.h"
#include "rules.h"
#include "gui.h"

static struct rules_t *rules = NULL;

static pthread_mutex_t mutex_lock;
static pthread_mutexattr_t mutex_attr;

static int rules_parse(JsonNode *root) {
	int have_error = 0, match = 0, x = 0;
	unsigned int i = 0;
	struct JsonNode *jrules = NULL;
	char *rule = NULL;
	double active = 1.0;

	event_function_init();
	event_operator_init();

	if(root->tag == JSON_OBJECT) {
		jrules = json_first_child(root);
		while(jrules) {
			i++;
			if(jrules->tag == JSON_OBJECT) {
				if(json_find_string(jrules, "rule", &rule) != 0) {
					logprintf(LOG_ERR, "config rule #%d \"%s\", missing \"rule\"", i, jrules->key);
					have_error = 1;
					break;
				} else {
					active = 1.0;
					json_find_number(jrules, "active", &active);

					struct rules_t *tmp = rules;
					match = 0;
					while(tmp) {
						if(strcmp(tmp->name, jrules->key) == 0) {
							match = 1;
							break;
						}
						tmp = tmp->next;
					}
					if(match == 1) {
						logprintf(LOG_ERR, "config rule #%d \"%s\" already exists", i, jrules->key);
						have_error = 1;
						break;
					}
					for(x=0;x<strlen(jrules->key);x++) {
						if(!isalnum(jrules->key[x]) && jrules->key[x] != '-' && jrules->key[x] != '_') {
							logprintf(LOG_ERR, "config rule #%d \"%s\", not alphanumeric", i, jrules->key);
							have_error = 1;
							break;
						}
					}

					struct rules_t *node = MALLOC(sizeof(struct rules_t));
					if(node == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					node->next = NULL;
					node->values = NULL;
					node->jtrigger = NULL;
					node->nrdevices = 0;
					node->status = 0;
					node->devices = NULL;
					node->actions = NULL;
					node->nr = i;
					if((node->name = MALLOC(strlen(jrules->key)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(node->name, jrules->key);
#ifndef WIN32
					clock_gettime(CLOCK_MONOTONIC, &node->timestamp.first);
#endif
					if(event_parse_rule(rule, node, 0, 1) == -1) {
						have_error = 1;
					}
#ifndef WIN32
					clock_gettime(CLOCK_MONOTONIC, &node->timestamp.second);
					logprintf(LOG_INFO, "rule #%d %s was parsed in %.6f seconds", node->nr, node->name,
						((double)node->timestamp.second.tv_sec + 1.0e-9*node->timestamp.second.tv_nsec) -
						((double)node->timestamp.first.tv_sec + 1.0e-9*node->timestamp.first.tv_nsec));
#endif
					node->status = 0;
					if((node->rule = MALLOC(strlen(rule)+1)) == NULL) {
						fprintf(stderr, "out of memory\n");
						exit(EXIT_FAILURE);
					}
					strcpy(node->rule, rule);
					node->active = (unsigned short)active;

					tmp = rules;
					if(tmp) {
						while(tmp->next != NULL) {
							tmp = tmp->next;
						}
						tmp->next = node;
					} else {
						node->next = rules;
						rules = node;
					}
					/*
					 * In case of an error, we do want to
					 * save a pointer to our faulty rule
					 * so it can be properly garbage collected.
					 */
					if(have_error == 1) {
						break;
					}
				}
			}
			jrules = jrules->next;
		}
	} else {
		logprintf(LOG_ERR, "config rules should be placed in an object");
		have_error = 1;
	}

	return have_error;
}

static JsonNode *rules_sync(int level, const char *media) {
	struct JsonNode *root = json_mkobject();
	struct JsonNode *rule = NULL;
	struct rules_t *tmp = NULL;
	struct gui_values_t *gui_values = NULL;
	int match = 0, i = 0;

	tmp = rules;

	while(tmp) {
		match = 0;
		for(i=0;i<tmp->nrdevices;i++) {
			if((gui_values = gui_media(tmp->devices[i])) != NULL) {
				while(gui_values) {
					if(gui_values->type == JSON_STRING) {
						if(strcmp(gui_values->string_, media) == 0 ||
							 strcmp(gui_values->string_, "all") == 0 ||
							 strcmp(media, "all") == 0) {
								match++;
						}
					}
					gui_values = gui_values->next;
				}
			}
		}
		if(strcmp(media, "all") == 0) {
			match = tmp->nrdevices;
		}
		if(match == tmp->nrdevices) {
			rule = json_mkobject();
			json_append_member(rule, "rule", json_mkstring(tmp->rule));
			json_append_member(rule, "active", json_mknumber((double)tmp->active, 0));
			json_append_member(root, tmp->name, rule);
		}
		tmp = tmp->next;
	}
	return root;
}

struct rules_t *rules_get(void) {
	return rules;
}

int rules_gc(void) {
	struct rules_t *tmp_rules = NULL;
	struct rules_values_t *tmp_values = NULL;
	struct rules_actions_t *tmp_actions = NULL;
	int i = 0;

	pthread_mutex_lock(&mutex_lock);
	while(rules) {
		tmp_rules = rules;
		FREE(tmp_rules->name);
		FREE(tmp_rules->rule);
		events_tree_gc(tmp_rules->tree);
		for(i=0;i<tmp_rules->nrdevices;i++) {
			FREE(tmp_rules->devices[i]);
		}
		while(tmp_rules->values) {
			tmp_values = tmp_rules->values;
			FREE(tmp_values->name);
			FREE(tmp_values->device);
			tmp_rules->values = tmp_rules->values->next;
			FREE(tmp_values);
		}
		if(tmp_rules->values != NULL) {
			FREE(tmp_rules->values);
		}
		while(tmp_rules->actions) {
			tmp_actions = tmp_rules->actions;
			if(tmp_actions->arguments != NULL) {
				json_delete(tmp_actions->arguments);
			}
			tmp_rules->actions = tmp_rules->actions->next;
			if(tmp_actions != NULL) {
				FREE(tmp_actions);
			}
		}
		if(tmp_rules->actions != NULL) {
			FREE(tmp_rules->actions);
		}
		if(tmp_rules->jtrigger != NULL) {
			json_delete(tmp_rules->jtrigger);
		}
		if(tmp_rules->devices != NULL) {
			FREE(tmp_rules->devices);
		}
		rules = rules->next;
		FREE(tmp_rules);
	}
	if(rules != NULL) {
		FREE(rules);
	}
	rules = NULL;
	pthread_mutex_unlock(&mutex_lock);

	logprintf(LOG_DEBUG, "garbage collected config rules library");
	return 1;
}

void rules_init(void) {
	event_action_init();

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex_lock, &mutex_attr);

	/* Request rules json object in main configuration */
	config_register(&config_rules, "rules");
	config_rules->readorder = 2;
	config_rules->writeorder = 1;
	config_rules->parse=&rules_parse;
	config_rules->sync=&rules_sync;
	config_rules->gc=&rules_gc;
}
