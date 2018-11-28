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

#ifndef _WIN32
	#include <unistd.h>
	#include <sys/time.h>
	#include <libgen.h>
	#include <dirent.h>
	#include <unistd.h>
#endif

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/strptime.h"
#include "../core/datetime.h"
#include "../core/dso.h"
#include "../core/log.h"
#include "../core/cast.h"
#include "../protocols/protocol.h"
#ifdef PILIGHT_REWRITE
#include "../storage/storage.h"
#else
#include "../config/devices.h"
#endif
#include "lua.h"

typedef struct lua_thread_t {
	lua_state_t *state;
	char *callback;
} lua_thread_t;

static int plua_toboolean(struct lua_State *L) {
	char buf[128] = { '\0' }, *p = buf;
	char *error = "string, number, or boolean expected, got %s";
	sprintf(p, error, lua_typename(L, lua_type(L, 1)));

	luaL_argcheck(L,
		(lua_isstring(L, 1) || lua_isnumber(L, 1) || lua_isboolean(L, 1)),
		1, buf);

	switch(lua_type(L, 1)) {
		case LUA_TSTRING: {
			const char *str = lua_tostring(L, 1);
			if(strcmp(str, "0") == 0 || strlen(str) == 0) {
				lua_pop(L, -1);
				lua_pushboolean(L, 0);
			} else {
				lua_pop(L, -1);
				lua_pushboolean(L, 1);
			}
		} break;
		case LUA_TNUMBER: {
			int num = (int)lua_tonumber(L, 1);
			lua_pop(L, -1);
			lua_pushboolean(L, num);
		} break;
		case LUA_TBOOLEAN: {
			// int b = lua_toboolean(L, 1);
			// lua_pop(L, -1);
			// lua_pushboolean(L, b);
		} break;
	}

	return 1;
}

static int plua_tonumber(struct lua_State *L) {
	char buf[128] = { '\0' }, *p = buf;
	char *error = "string, number, or boolean expected, got %s";
	sprintf(p, error, lua_typename(L, lua_type(L, 1)));

	luaL_argcheck(L,
		(lua_isstring(L, 1) || lua_isnumber(L, 1) || lua_isboolean(L, 1)),
		1, buf);

	switch(lua_type(L, 1)) {
		case LUA_TSTRING: {
			const char *str = lua_tostring(L, 1);
			float num = atof(str);
			lua_pop(L, -1);
			lua_pushnumber(L, num);
		} break;
		case LUA_TNUMBER: {
			// float num = lua_tonumber(L, 1);
			// lua_pop(L, -1);
			// lua_pushnumber(L, num);
		} break;
		case LUA_TBOOLEAN: {
			int b = lua_toboolean(L, 1);
			lua_pop(L, -1);
			if(b == 0) {
				lua_pushnumber(L, 0);
			} else {
				lua_pushnumber(L, 1);
			}
		} break;
	}

	return 1;
}

static int plua_tostring(struct lua_State *L) {
	char buf[128] = { '\0' }, *p = buf;
	char *error = "string, number, or boolean expected, got %s";
	sprintf(p, error, lua_typename(L, lua_type(L, 1)));

	luaL_argcheck(L,
		(lua_isstring(L, 1) || lua_isnumber(L, 1) || lua_isboolean(L, 1)),
		1, buf);

	switch(lua_type(L, 1)) {
		case LUA_TSTRING: {
			// const char *str = lua_tostring(L, 1);
			// lua_pop(L, -1);
			// lua_pushstring(L, str);
		} break;
		case LUA_TNUMBER: {
			float num = lua_tonumber(L, 1);
			lua_pop(L, -1);

			char *tmp = NULL;
			int len = snprintf(NULL, 0, "%.6f", num);
			if((tmp = MALLOC(len+1)) == NULL) {
				OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
			}
			memset(tmp, 0, len+1);
			sprintf(tmp, "%.6f", num);
			lua_pushstring(L, tmp);
			FREE(tmp);
		} break;
		case LUA_TBOOLEAN: {
			int b = lua_toboolean(L, 1);
			lua_pop(L, -1);
			if(b == 0) {
				lua_pushstring(L, "0");
			} else {
				lua_pushstring(L, "1");
			}
		} break;
	}

	return 1;
}

static void datetime2table(struct lua_State *L, char *device) {
	struct varcont_t val;
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));
#ifdef PILIGHT_REWRITE
	char *setting = NULL;
	int i = 0;
	while(devices_select_settings(ORIGIN_RULE, device, i++, &setting, &val) == 0) {
		if(strcmp(setting, "year") == 0) {
			tm.tm_year = val.number_-1900;
		}
		if(strcmp(setting, "month") == 0) {
			tm.tm_mon = val.number_-1;
		}
		if(strcmp(setting, "day") == 0) {
			tm.tm_mday = val.number_;
		}
		if(strcmp(setting, "hour") == 0) {
			tm.tm_hour = val.number_;
		}
		if(strcmp(setting, "minute") == 0) {
			tm.tm_min = val.number_;
		}
		if(strcmp(setting, "second") == 0) {
			tm.tm_sec = val.number_;
		}
		if(strcmp(setting, "weekday") == 0) {
			tm.tm_wday = val.number_-1;
		}
		if(strcmp(setting, "dst") == 0) {
			tm.tm_isdst = val.number_;
		}
	}
#else
	struct devices_t *dev = NULL;
	if(devices_get(device, &dev) == 0) {
		struct devices_settings_t *tmp_settings = dev->settings;
		while(tmp_settings) {
			val.type_ = tmp_settings->values->type;
			if(val.type_ == JSON_STRING) {
				val.string_ = tmp_settings->values->string_;
			} else if(val.type_ == JSON_NUMBER) {
				val.number_ = tmp_settings->values->number_;
				val.decimals_ = tmp_settings->values->decimals;
			}
			if(strcmp(tmp_settings->name, "year") == 0) {
				tm.tm_year = val.number_-1900;
			}
			if(strcmp(tmp_settings->name, "month") == 0) {
				tm.tm_mon = val.number_-1;
			}
			if(strcmp(tmp_settings->name, "day") == 0) {
				tm.tm_mday = val.number_;
			}
			if(strcmp(tmp_settings->name, "hour") == 0) {
				tm.tm_hour = val.number_;
			}
			if(strcmp(tmp_settings->name, "minute") == 0) {
				tm.tm_min = val.number_;
			}
			if(strcmp(tmp_settings->name, "second") == 0) {
				tm.tm_sec = val.number_;
			}
			if(strcmp(tmp_settings->name, "weekday") == 0) {
				tm.tm_wday = val.number_-1;
			}
			if(strcmp(tmp_settings->name, "dst") == 0) {
				tm.tm_isdst = val.number_;
			}
			tmp_settings = tmp_settings->next;
		}
	}
#endif
int year = tm.tm_year+1900;
	int month = tm.tm_mon+1;
	int day = tm.tm_mday;
	int hour = tm.tm_hour;
	int minute = tm.tm_min;
	int second = tm.tm_sec;
	int weekday = tm.tm_wday;

	datefix(&year, &month, &day, &hour, &minute, &second, &weekday);

	lua_createtable(L, 0, 2);

	lua_pushstring(L, "type");
	lua_pushnumber(L, DATETIME);
	lua_settable(L, -3);

	lua_pushstring(L, "year");
	lua_pushnumber(L, year);
	lua_settable(L, -3);

	lua_pushstring(L, "month");
	lua_pushnumber(L, month);
	lua_settable(L, -3);

	lua_pushstring(L, "day");
	lua_pushnumber(L, day);
	lua_settable(L, -3);

	lua_pushstring(L, "hour");
	lua_pushnumber(L, hour);
	lua_settable(L, -3);

	lua_pushstring(L, "min");
	lua_pushnumber(L, minute);
	lua_settable(L, -3);

	lua_pushstring(L, "sec");
	lua_pushnumber(L, second);
	lua_settable(L, -3);

	lua_pushstring(L, "weekday");
	lua_pushnumber(L, weekday);
	lua_settable(L, -3);
}

static int plua_getdevice(struct lua_State *L) {
	if(lua_gettop(L) != 1) {
		luaL_error(L, "pilight.plua_getdevice requires 1 arguments, %d given", lua_gettop(L));
		return 0;
	}
	char buf[128] = { '\0' }, *p = buf;
	char *error = "string expected, got %s";
	sprintf(p, error, lua_typename(L, lua_type(L, 1)));

	luaL_argcheck(L,
		(lua_isstring(L, 1)),
		1, buf);

	const char *device = NULL;
	if(lua_type(L, -1) == LUA_TSTRING) {
		device = lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	char *d = (char *)device;

#ifdef PILIGHT_REWRITE
	if(devices_select(ORIGIN_RULE, d, NULL) == 0) {
		struct protocol_t *protocol = NULL;
		if(devices_select_protocol(ORIGIN_RULE, d, 0, &protocol) == 0) {
			if(protocol->devtype == DATETIME) {
				datetime2table(L, d);
			}
		}
#else
	struct devices_t *dev = NULL;
	if(devices_get(d, &dev) == 0) {	
		struct protocols_t *tmp = dev->protocols;
		while(tmp) {
			if(tmp->listener->devtype == DATETIME) {
				datetime2table(L, d);
			}
			tmp = tmp->next;
		}
#endif
	} else {
		// luaL_error(L, "device \"%s\" does not exist", d);
		return 0;
	}

	return 1;
}

static int plua_strptime(struct lua_State *L) {
	if(lua_gettop(L) != 2) {
		luaL_error(L, "pilight.strptime requires 2 arguments, %d given", lua_gettop(L));
		return 0;
	}
	char buf[128] = { '\0' }, *p = buf;
	char *error = "string expected, got %s";
	sprintf(p, error, lua_typename(L, lua_type(L, 1)));

	luaL_argcheck(L,
		(lua_isstring(L, 1)),
		1, buf);

	sprintf(p, error, lua_typename(L, lua_type(L, 2)));
	luaL_argcheck(L,
		(lua_isstring(L, 2)),
		2, buf);

	const char *datetime = NULL;
	const char *format = NULL;
	if(lua_type(L, -1) == LUA_TSTRING) {
		datetime = lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	if(lua_type(L, -1) == LUA_TSTRING) {
		format = lua_tostring(L, -1);
		lua_pop(L, 1);
	}

	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));
	if(strptime(datetime, format, &tm) == NULL) {
		luaL_error(L, "pilight.strptime is unable to parse \"%s\" as \"%s\" ", datetime, format);
		return 0;
	}

	int year = tm.tm_year+1900;
	int month = tm.tm_mon+1;
	int day = tm.tm_mday;
	int hour = tm.tm_hour;
	int minute = tm.tm_min;
	int second = tm.tm_sec;
	int weekday = tm.tm_wday;

	datefix(&year, &month, &day, &hour, &minute, &second, &weekday);

	lua_createtable(L, 0, 2);

	lua_pushstring(L, "year");
	lua_pushnumber(L, year);
	lua_settable(L, -3);

	lua_pushstring(L, "month");
	lua_pushnumber(L, month);
	lua_settable(L, -3);

	lua_pushstring(L, "day");
	lua_pushnumber(L, day);
	lua_settable(L, -3);

	lua_pushstring(L, "hour");
	lua_pushnumber(L, hour);
	lua_settable(L, -3);

	lua_pushstring(L, "min");
	lua_pushnumber(L, minute);
	lua_settable(L, -3);

	lua_pushstring(L, "sec");
	lua_pushnumber(L, second);
	lua_settable(L, -3);

	lua_pushstring(L, "weekday");
	lua_pushnumber(L, weekday);
	lua_settable(L, -3);

	return 1;
}

static int plua_random(struct lua_State *L) {
	if(lua_gettop(L) != 2) {
		luaL_error(L, "pilight.random requires 2 arguments, %d given", lua_gettop(L));
		return 0;
	}
	char buf[128] = { '\0' }, *p = buf;
	char *error = "number expected, got %s";
	sprintf(p, error, lua_typename(L, lua_type(L, 1)));

	luaL_argcheck(L,
		(lua_isnumber(L, 1)),
		1, buf);

	sprintf(p, error, lua_typename(L, lua_type(L, 2)));
	luaL_argcheck(L,
		(lua_isnumber(L, 2)),
		2, buf);

	int min = 0, max = 0;
	if(lua_type(L, -1) == LUA_TNUMBER || lua_type(L, -1) == LUA_TSTRING) {
		max = (int)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	if(lua_type(L, -1) == LUA_TNUMBER || lua_type(L, -1) == LUA_TSTRING) {
		min = (int)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	struct timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);

	int r = rand() / (RAND_MAX + 1.0) * (max - min + 1) + min;

	lua_pushnumber(L, r);

	return 1;
}

static void thread_free(uv_work_t *req, int status) {
	struct lua_thread_t *thread = req->data;
	FREE(thread->callback);
	FREE(req->data);
	FREE(req);
}

static void thread_callback(uv_work_t *req) {
	struct lua_thread_t *thread = req->data;
	struct lua_state_t *state = thread->state;

	char name[255], *p = name;
	memset(name, '\0', 255);

	switch(state->module->type) {
		case FUNCTION: {
			sprintf(p, "function.%s", state->module->name);
		} break;
		case OPERATOR: {
			sprintf(p, "operator.%s", state->module->name);
		} break;
	}

	lua_getglobal(state->L, name);
	if(lua_isnil(state->L, -1) != 0) {
		logprintf(LOG_ERR, "cannot find %s lua module", name);
		return;
	}

#if LUA_VERSION_NUM <= 502
	lua_getfield(state->L, -1, thread->callback);
	if(strcmp(lua_typename(state->L, lua_type(state->L, -1)), "function") != 0) {
#else
	if(lua_getfield(state->L, -1, thread->callback) == 0) {
#endif
		logprintf(LOG_ERR, "%s: thread callback %s does not exist", state->module->file, thread->callback);
		return;
	}

	lua_newtable(state->L);
	lua_createtable(state->L, 0, 1);
	lua_pushcfunction(state->L, plua_metatable_get);
	lua_setfield(state->L, -2, "__index");
	lua_pushcfunction(state->L, plua_metatable_set);
	lua_setfield(state->L, -2, "__newindex");
	lua_pushcfunction(state->L, plua_metatable_gc);
	lua_setfield(state->L, -2, "__gc");
	lua_pushcfunction(state->L, plua_metatable_pairs);
	lua_setfield(state->L, -2, "__pairs");
	lua_setmetatable(state->L, -2);

	if(lua_pcall(state->L, 1, 0, 0) == LUA_ERRRUN) {
		if(strcmp(lua_typename(state->L, lua_type(state->L, -1)), "string") == 0) {
			logprintf(LOG_ERR, "%s", lua_tostring(state->L,  -1));
			lua_pop(state->L, 1);
			return;
		}
	}

	uv_mutex_unlock(&state->lock);
}

static int plua_thread(struct lua_State *L) {
	if(lua_gettop(L) != 1) {
		luaL_error(L, "pilight.devices_select_string_setting requires at least 1 argument, %d given", lua_gettop(L));
		return 0;
	}

	const char *func = NULL;
	char buf[128] = { '\0' }, *p = buf;
	char *error = "string expected, got %s";

	sprintf(p, error, lua_typename(L, lua_type(L, 1)));

	luaL_argcheck(L,
		(lua_isstring(L, 1)),
		1, buf);

	if(lua_type(L, -1) == LUA_TSTRING) {
		func = lua_tostring(L, -1);
		lua_pop(L, 1);
	}

	char name[255];
	memset(name, '\0', 255);

	p = name;

	struct lua_state_t *state = plua_get_current_state(L);
	if(state == NULL) {
		return 1;
	}

	switch(state->module->type) {
		case FUNCTION: {
			sprintf(p, "function.%s", state->module->name);
		} break;
		case OPERATOR: {
			sprintf(p, "operator.%s", state->module->name);
		} break;
	}

	lua_getglobal(L, name);
	if(lua_isnil(L, -1) != 0) {
		logprintf(LOG_ERR, "cannot find %s lua module");
		return 1;
	}

#if LUA_VERSION_NUM <= 502
	lua_getfield(L, -1, func);
	if(strcmp(lua_typename(L, lua_type(L, -1)), "function") != 0) {
#else
	if(lua_getfield(L, -1, func) == 0) {
#endif
		logprintf(LOG_ERR, "%s: thread callback %s does not exist", state->module->file, func);
		return 1;
	}

	struct lua_state_t *new_state = plua_get_free_state();
	new_state->module = state->module;

	plua_metatable_clone(&state->table, &new_state->table);

	uv_work_t *tp_work_req = MALLOC(sizeof(uv_work_t));
	if(tp_work_req == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	struct lua_thread_t *lua_thread = MALLOC(sizeof(struct lua_thread_t));
	if(lua_thread == NULL) {
		OUT_OF_MEMORY
	}
	lua_thread->state = new_state;
	if((lua_thread->callback = STRDUP((char *)func)) == NULL) {
		OUT_OF_MEMORY
	}
	tp_work_req->data = lua_thread;
	char *pthname = "lua thread on state #%d";
	memset(buf, 0, sizeof(buf));
	p = buf;
	sprintf(p, pthname, new_state->idx);

	if(uv_queue_work(uv_default_loop(), tp_work_req, buf, thread_callback, thread_free) < 0) {
		FREE(lua_thread->callback);
		FREE(lua_thread);
		FREE(tp_work_req);

		uv_mutex_unlock(&new_state->lock);
	}

	return 1;
}

static struct lua_libs {
	char *name;
	int (*func)(struct lua_State *);
} pilightlib[] = {
	{"toboolean", plua_toboolean},
	{"tonumber", plua_tonumber},
	{"tostring", plua_tostring},
	{"strptime", plua_strptime},
	{"getdevice", plua_getdevice},
	{"random", plua_random},
	{"thread", plua_thread},
	{NULL, NULL}
};

void plua_register_library(struct lua_State *L) {
	int i = 0;
	/*
	 * Register pilight lua library
	 */
	lua_newtable(L);
	while(pilightlib[i].name != NULL) {
		lua_pushcfunction(L, pilightlib[i].func);
		lua_setfield(L, -2, pilightlib[i].name);
		i++;
	}
	lua_pushvalue(L, -1);
	lua_setglobal(L, "pilight");
}