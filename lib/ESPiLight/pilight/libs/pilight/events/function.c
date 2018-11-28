/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <libgen.h>
#include <dirent.h>
#ifndef _WIN32
	#include <dlfcn.h>
#endif

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/options.h"
#include "../core/dso.h"
#include "../core/log.h"
#include "../lua/lua.h"
#include "../config/settings.h"

#include "function.h"

static int init = 0;

void event_function_init(void) {
	if(init == 1) {
		return;
	}
	init = 1;
	plua_init();

	char path[PATH_MAX];
	char *f = STRDUP(__FILE__);
	struct dirent *file = NULL;
	DIR *d = NULL;
	struct stat s;
	char *functions_root = FUNCTION_ROOT;

	if(f == NULL) {
		OUT_OF_MEMORY
	}

#ifdef PILIGHT_REWRITE
	settings_select_string(ORIGIN_MASTER, "functions-root", &functions_root);
#else
	settings_find_string("functions-root", &functions_root);
#endif

	if((d = opendir(functions_root))) {
		while((file = readdir(d)) != NULL) {
			memset(path, '\0', PATH_MAX);
			sprintf(path, "%s%s", functions_root, file->d_name);
			if(stat(path, &s) == 0) {
				/* Check if file */
				if(S_ISREG(s.st_mode)) {
					if(strstr(file->d_name, ".lua") != NULL) {
						plua_module_load(path, FUNCTION);
					}
				}
			}
		}
	}
	closedir(d);
	FREE(f);
}

static int plua_function_module_run(struct lua_State *L, char *file, struct event_function_args_t *args, struct varcont_t *v) {
#if LUA_VERSION_NUM <= 502
	lua_getfield(L, -1, "run");
	if(strcmp(lua_typename(L, lua_type(L, -1)), "function") != 0) {
#else
	if(lua_getfield(L, -1, "run") == 0) {
#endif
		logprintf(LOG_ERR, "%s: run function missing", file);
		return 0;
	}

	int nrargs = 0;
	struct event_function_args_t *tmp1 = NULL;
	while(args) {
		tmp1 = args;
		nrargs++;
		switch(tmp1->var.type_) {
			case JSON_NUMBER: {
				char *tmp = NULL;
				int len = snprintf(NULL, 0, "%.*f", tmp1->var.decimals_, tmp1->var.number_);
				if((tmp = MALLOC(len+1)) == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				memset(tmp, 0, len+1);
				sprintf(tmp, "%.*f", tmp1->var.decimals_, tmp1->var.number_);
				lua_pushstring(L, tmp);
				FREE(tmp);
			} break;
			case JSON_STRING:
				lua_pushstring(L, tmp1->var.string_);
				FREE(tmp1->var.string_);
			break;
			case JSON_BOOL:
				lua_pushboolean(L, tmp1->var.bool_);
			break;
		}
		args = args->next;
		FREE(tmp1);
	}
	args = NULL;

	if(lua_pcall(L, nrargs, 1, 0) == LUA_ERRRUN) {
		if(strcmp(lua_typename(L, lua_type(L, -1)), "nil") == 0) {
			logprintf(LOG_ERR, "%s: syntax error", file);
			return 0;
		}
		if(strcmp(lua_typename(L, lua_type(L, -1)), "string") == 0) {
			logprintf(LOG_ERR, "%s", lua_tostring(L,  -1));
			lua_pop(L, 1);
			return 0;
		}
	}

	if(lua_isstring(L, -1) == 0 &&
		lua_isnumber(L, -1) == 0 &&
		lua_isboolean(L, -1) == 0) {
		logprintf(LOG_ERR, "%s: the run function returned %s, string, number or boolean expected", file, lua_typename(L, lua_type(L, -1)));
		return 0;
	}

	if(lua_isnumber(L, -1) == 1) {
		char *p = (char *)lua_tostring(L, -1);
		v->number_ = atof(p);
		v->decimals_ = nrDecimals(p);
		v->type_ = JSON_NUMBER;
	} else if(lua_isstring(L, -1) == 1) {
		int l = strlen(lua_tostring(L, -1));
		if((v->string_ = REALLOC(v->string_, l+1)) == NULL) {
			OUT_OF_MEMORY
		}
		strcpy(v->string_, lua_tostring(L, -1));
		v->type_ = JSON_STRING;
		v->free_ = 1;
	} else if(lua_isboolean(L, -1) == 1) {
		v->bool_ = (int)lua_toboolean(L, -1);
		v->type_ = JSON_BOOL;
	}

	lua_pop(L, 1);

	return 1;
}

int event_function_exists(char *module) {
	return plua_module_exists(module, FUNCTION);
}

struct event_function_args_t *event_function_add_argument(struct varcont_t *var, struct event_function_args_t *head) {
	struct event_function_args_t *node = MALLOC(sizeof(struct event_function_args_t));
	if(node == NULL) {
		OUT_OF_MEMORY
	}
	memset(node, 0, sizeof(struct event_function_args_t));
	switch(var->type_) {
		case JSON_NUMBER: {
			node->var.type_ = JSON_NUMBER;
			node->var.number_ = var->number_;
			node->var.decimals_ = var->decimals_;
		} break;
		case JSON_STRING: {
			node->var.type_ = JSON_STRING;
			if((node->var.string_ = STRDUP(var->string_)) == NULL) {
				OUT_OF_MEMORY
			}
		} break;
		case JSON_BOOL: {
			node->var.type_ = JSON_BOOL;
			node->var.bool_ = var->bool_;
		} break;
	}
	if(head != NULL) {
		struct event_function_args_t *tmp = head;
		while(tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = node;
		node = tmp;
	} else {
		node->next = head;
		head = node;
	}
	return head;
}

void event_function_free_argument(struct event_function_args_t *args) {
	struct event_function_args_t *tmp1 = NULL;
	while(args) {
		tmp1 = args;
		switch(tmp1->var.type_) {
			case JSON_STRING:
				FREE(tmp1->var.string_);
			break;
		}
		args = args->next;
		FREE(tmp1);
	}
}

int event_function_callback(char *module, struct event_function_args_t *args, struct varcont_t *v) {
	struct lua_state_t *state = plua_get_free_state();
	struct lua_State *L = NULL;

	if(state == NULL) {
		return -1;
	}
	if((L = state->L) == NULL) {
		uv_mutex_unlock(&state->lock);
		return -1;
	}

	char name[255], *p = name;
	memset(name, '\0', 255);

	sprintf(p, "function.%s", module);

	lua_getglobal(L, name);
	if(lua_isnil(L, -1) != 0) {
		event_function_free_argument(args);
		uv_mutex_unlock(&state->lock);
		return -1;
	}
	if(lua_istable(L, -1) != 0) {
		char *file = NULL;
		struct plua_module_t *tmp = plua_get_modules();
		while(tmp) {
			if(strcmp(module, tmp->name) == 0) {
				file = tmp->file;
				state->module = tmp;
				break;
			}
			tmp = tmp->next;
		}
		if(file != NULL) {
			if(plua_function_module_run(L, file, args, v) == 0) {
				lua_pop(L, -1);
				uv_mutex_unlock(&state->lock);
				return -1;
			}
		} else {
			event_function_free_argument(args);
			uv_mutex_unlock(&state->lock);
			return -1;
		}
	}
	lua_pop(L, -1);

	uv_mutex_unlock(&state->lock);

	return 0;
}

int event_function_gc(void) {
	init = 0;
	logprintf(LOG_DEBUG, "garbage collected event function library");
	return 0;
}
