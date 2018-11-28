/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _LUA_H_
#define _LUA_H_

#include <luajit-2.0/lua.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>

#include "../libs/pilight/core/common.h"

#define OPERATOR	1
#define FUNCTION	2

typedef struct plua_metatable_t {
	struct {
		struct varcont_t val;
		struct varcont_t key;
	} *table;
	int nrvar;
	int idx;
} plua_metatable_t;

typedef struct plua_module_t {
	char name[255];
	char file[PATH_MAX];
	char version[12];
	char reqversion[12];
	char reqcommit[12];
	void *bytecode;
	int size;
	int type;

	struct plua_module_t *next;
} plua_module_t;

typedef struct lua_state_t {
	lua_State *L;
	uv_mutex_t lock;
	struct plua_module_t *module;
	struct plua_metatable_t *table;
	int idx;
} lua_state_t;

int plua_metatable_set(lua_State *L);
int plua_metatable_get(lua_State *L);
int plua_metatable_gc(lua_State *L);
int plua_metatable_pairs(lua_State *L);
void plua_stack_dump(lua_State *L);
void plua_module_load(char *, int);
int plua_module_exists(char *, int);
void plua_metatable_clone(struct plua_metatable_t **, struct plua_metatable_t **);
struct lua_state_t *plua_get_free_state(void);
struct lua_state_t *plua_get_current_state(lua_State *L);
struct plua_module_t *plua_get_modules(void);
void plua_init(void);
int plua_gc(void);

#endif