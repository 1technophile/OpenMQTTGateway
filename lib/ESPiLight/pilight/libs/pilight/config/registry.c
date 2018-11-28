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

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/json.h"
#include "../core/log.h"
#include "registry.h"

struct JsonNode *registry = NULL;

static pthread_mutex_t mutex_lock;
static pthread_mutexattr_t mutex_attr;

static int registry_get_value_recursive(struct JsonNode *root, const char *key, void **value, void **decimals, int type) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *sub = strstr(key, ".");
	char *buff = MALLOC(strlen(key)+1);
	strcpy(buff, key);
	if(sub != NULL) {
		int pos = sub-key;
		buff[pos] = '\0';
	}
	struct JsonNode *member = json_find_member(root, buff);
	if(member != NULL) {
		if(member->tag == type) {
			if(type == JSON_NUMBER) {
				*value = (void *)&member->number_;
				*decimals = (void *)&member->decimals_;
			} else if(type == JSON_STRING) {
				*value = (void *)member->string_;
			}
			FREE(buff);
			return 0;
		} else if(member->tag == JSON_OBJECT) {
			if(sub != NULL) {
				int pos = sub-key;
				strcpy(buff, &key[pos+1]);
			}
			int ret = registry_get_value_recursive(member, buff, value, decimals, type);
			FREE(buff);
			return ret;
		}
	}
	FREE(buff);
	return -1;
}

static int registry_set_value_recursive(struct JsonNode *root, const char *key, void *value, int decimals, int type) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *sub = strstr(key, ".");
	char *buff = MALLOC(strlen(key)+1);
	strcpy(buff, key);
	if(sub != NULL) {
		int pos = sub-key;
		buff[pos] = '\0';
	}
	struct JsonNode *member = json_find_member(root, buff);
	if(member != NULL) {
		if(member->tag == type) {
			if(type == JSON_NUMBER) {
				member->number_ = *(double *)value;
				member->decimals_ = decimals;
			} else if(type == JSON_STRING) {
				member->string_ = REALLOC(member->string_, strlen(value)+1);
				strcpy(member->string_, (char *)value);
			}
			FREE(buff);
			return 0;
		} else if(member->tag == JSON_OBJECT) {
			if(sub != NULL) {
				int pos = sub-key;
				strcpy(buff, &key[pos+1]);
			}
			int ret = registry_set_value_recursive(member, buff, value, decimals, type);
			FREE(buff);
			return ret;
		}
	} else if(sub != NULL) {
		member = json_mkobject();
		json_append_member(root, buff, member);
		int pos = sub-key;
		strcpy(buff, &key[pos+1]);
		int ret = registry_set_value_recursive(member, buff, value, decimals, type);
		FREE(buff);
		return ret;
	} else {
		if(type == JSON_NUMBER) {
			json_append_member(root, buff, json_mknumber(*(double *)value, decimals));
		} else if(type == JSON_STRING) {
			json_append_member(root, buff, json_mkstring(value));
		}
		FREE(buff);
		return 0;
	}
	FREE(buff);
	return -1;
}

static void registry_remove_empty_parent(struct JsonNode *root) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct JsonNode *parent = root->parent;
	if(json_first_child(root) == NULL) {
		if(parent != NULL) {
			json_remove_from_parent(root);
			registry_remove_empty_parent(parent);
			json_delete(root);
		}
	}
}

static int registry_remove_value_recursive(struct JsonNode *root, const char *key) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *sub = strstr(key, ".");
	char *buff = MALLOC(strlen(key)+1);
	strcpy(buff, key);
	if(sub != NULL) {
		int pos = sub-key;
		buff[pos] = '\0';
	}
	struct JsonNode *member = json_find_member(root, buff);
	if(member != NULL) {
		if(sub == NULL) {
			json_remove_from_parent(member);
			json_delete(member);
			registry_remove_empty_parent(root);
			FREE(buff);
			return 0;
		}
		if(member->tag == JSON_OBJECT) {
			if(sub != NULL) {
				int pos = sub-key;
				strcpy(buff, &key[pos+1]);
			}
			int ret = registry_remove_value_recursive(member, buff);
			FREE(buff);
			return ret;
		}
	}
	FREE(buff);
	return -1;
}

int registry_get_string(const char *key, char **value) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(registry == NULL) {
		return -1;
	}
	return registry_get_value_recursive(registry, key, (void *)value, NULL, JSON_STRING);
}

int registry_get_number(const char *key, double *value, int *decimals) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(registry == NULL) {
		return -1;
	}
	void *p = NULL;
	void *q = NULL;
	int ret = registry_get_value_recursive(registry, key, &p, &q, JSON_NUMBER);
	if(ret == 0) {
		*value = *(double *)p;
		*decimals = *(int *)q;
	}
	return ret;
}

int registry_set_string(const char *key, char *value) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(registry == NULL) {
		registry = json_mkobject();
	}
	return registry_set_value_recursive(registry, key, (void *)value, 0, JSON_STRING);
}

int registry_set_number(const char *key, double value, int decimals) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(registry == NULL) {
		registry = json_mkobject();
	}
	void *p = (void *)&value;
	return registry_set_value_recursive(registry, key, p, decimals, JSON_NUMBER);
}

int registry_remove_value(const char *key) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(registry == NULL) {
		return -1;
	}
	return registry_remove_value_recursive(registry, key);
}

static int registry_parse(JsonNode *root) {
	if(root->tag == JSON_OBJECT) {
		char *content = json_stringify(root, NULL);
		registry = json_decode(content);
		json_free(content);
	} else {
		logprintf(LOG_ERR, "config registry should be of an object type");
		return -1;
	}
	return 0;
}

static JsonNode *registry_sync(int level, const char *display) {
	if(registry != NULL) {
		char *content = json_stringify(registry, NULL);
		struct JsonNode *jret = json_decode(content);
		json_free(content);
		return jret;
	} else {
		return NULL;
	}
}

int registry_gc(void) {
	pthread_mutex_lock(&mutex_lock);
	if(registry != NULL) {
		json_delete(registry);
	}
	registry = NULL;
	pthread_mutex_unlock(&mutex_lock);

	logprintf(LOG_DEBUG, "garbage collected config registry library");
	return 1;
}

void registry_init(void) {
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex_lock, &mutex_attr);

	/* Request settings json object in main configuration */
	config_register(&config_registry, "registry");
	config_registry->readorder = 5;
	config_registry->writeorder = 5;
	config_registry->parse=&registry_parse;
	config_registry->sync=&registry_sync;
	config_registry->gc=&registry_gc;
}
