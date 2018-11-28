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
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include <dirent.h>
#ifndef _WIN32
	#include <libgen.h>
	#include <unistd.h>
#endif

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/dso.h"
#include "../core/log.h"
#include "../config/settings.h"
#include "../lua/lua.h"

#include "operator.h"

static int init = 0;

void event_operator_init(void) {
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
	char *operator_root = OPERATOR_ROOT;

	if(f == NULL) {
		OUT_OF_MEMORY
	}

#ifdef PILIGHT_REWRITE
	settings_select_string(ORIGIN_MASTER, "operators-root", &operator_root);
#else
	settings_find_string("operators-root", &operator_root);
#endif

	if((d = opendir(operator_root))) {
		while((file = readdir(d)) != NULL) {
			memset(path, '\0', PATH_MAX);
			sprintf(path, "%s%s", operator_root, file->d_name);
			if(stat(path, &s) == 0) {
				/* Check if file */
				if(S_ISREG(s.st_mode)) {
					if(strstr(file->d_name, ".lua") != NULL) {
						plua_module_load(path, OPERATOR);
					}
				}
			}
		}
	}
	closedir(d);
	FREE(f);
}

static int plua_operator_precedence_run(struct lua_State *L, char *file, int *ret) {
#if LUA_VERSION_NUM <= 502
	lua_getfield(L, -1, "precedence");
	if(strcmp(lua_typename(L, lua_type(L, -1)), "function") != 0) {
#else
	if(lua_getfield(L, -1, "precedence") == 0) {
#endif
		logprintf(LOG_ERR, "%s: precedence function missing", file);
		return 0;
	}

	if(lua_pcall(L, 0, 1, 0) == LUA_ERRRUN) {
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

	if(lua_isnumber(L, -1) == 0) {
		logprintf(LOG_ERR, "%s: the precedence function returned %s, number expected", file, lua_typename(L, lua_type(L, -1)));
		return 0;
	}

	*ret = lua_tonumber(L, -1);

	lua_pop(L, 1);

	return 1;
}

static int plua_operator_associativity_run(struct lua_State *L, char *file, int *ret) {
#if LUA_VERSION_NUM <= 502
	lua_getfield(L, -1, "associativity");
	if(strcmp(lua_typename(L, lua_type(L, -1)), "function") != 0) {
#else
	if(lua_getfield(L, -1, "associativity") == 0) {
#endif
		logprintf(LOG_ERR, "%s: associativity function missing", file);
		return 0;
	}

	if(lua_pcall(L, 0, 1, 0) == LUA_ERRRUN) {
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

	if(lua_isnumber(L, -1) == 0) {
		logprintf(LOG_ERR, "%s: the associativity function returned %s, number expected", file, lua_typename(L, lua_type(L, -1)));
		return 0;
	}

	*ret = lua_tonumber(L, -1);

	lua_pop(L, 1);

	return 1;
}

static int plua_operator_module_run(struct lua_State *L, char *file, struct varcont_t *a, struct varcont_t *b, struct varcont_t *v) {
#if LUA_VERSION_NUM <= 502
	lua_getfield(L, -1, "run");
	if(strcmp(lua_typename(L, lua_type(L, -1)), "function") != 0) {
#else
	if(lua_getfield(L, -1, "run") == 0) {
#endif
		logprintf(LOG_ERR, "%s: run function missing", file);
		return 0;
	}

	switch(a->type_) {
		case JSON_NUMBER: {
			lua_pushnumber(L, a->number_);
		} break;
		case JSON_STRING:
			lua_pushstring(L, a->string_);
		break;
		case JSON_BOOL:
			lua_pushboolean(L, a->bool_);
		break;
	}
	switch(b->type_) {
		case JSON_NUMBER: {
			lua_pushnumber(L, b->number_);
		} break;
		case JSON_STRING:
			lua_pushstring(L, b->string_);
		break;
		case JSON_BOOL:
			lua_pushboolean(L, b->bool_);
		break;
	}

	if(lua_pcall(L, 2, 1, 0) == LUA_ERRRUN) {
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

int event_operator_exists(char *module) {
	return plua_module_exists(module, OPERATOR);
}

int event_operator_precedence(char *module, int *ret) {
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

	sprintf(p, "operator.%s", module);

	lua_getglobal(L, name);
	if(lua_isnil(L, -1) != 0) {
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
		if(plua_operator_precedence_run(L, file, ret) == 0) {
			lua_pop(L, -1);
			uv_mutex_unlock(&state->lock);
			return -1;
		}
	}
	lua_pop(L, -1);

	uv_mutex_unlock(&state->lock);

	return 0;
}

int event_operator_associativity(char *module, int *ret) {
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

	sprintf(p, "operator.%s", module);

	lua_getglobal(L, name);
	if(lua_isnil(L, -1) != 0) {
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
		if(plua_operator_associativity_run(L, file, ret) == 0) {
			lua_pop(L, -1);
			uv_mutex_unlock(&state->lock);
			return -1;
		}
	}
	lua_pop(L, -1);

	uv_mutex_unlock(&state->lock);

	return 0;
}

int event_operator_callback(char *module, struct varcont_t *a, struct varcont_t *b, struct varcont_t *v) {
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

	sprintf(p, "operator.%s", module);

	lua_getglobal(L, name);
	if(lua_isnil(L, -1) != 0) {
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
		if(plua_operator_module_run(L, file, a, b, v) == 0) {
			lua_pop(L, -1);
			uv_mutex_unlock(&state->lock);
			return -1;
		}
	}
	lua_pop(L, -1);

	uv_mutex_unlock(&state->lock);

	return 0;
}

int event_operator_gc(void) {
	init = 0;
	logprintf(LOG_DEBUG, "garbage collected event operator library");
	return 0;
}
