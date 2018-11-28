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
	#include <libgen.h>
	#include <dirent.h>
	#include <unistd.h>
#endif

#include "lua.h"
#include "lualibrary.h"

#include "../core/log.h"
#include "../core/json.h"
#include "../core/mem.h"
#include "../core/common.h"

#define NRLUASTATES	5

static int init = 0;
struct lua_state_t lua_state[NRLUASTATES];
struct plua_module_t *modules = NULL;

/* LCOV_EXCL_START */
void plua_stack_dump(lua_State *L) {
	int i = 0;
	int top = lua_gettop(L);
	for(i = 1; i <= top; i++) {  /* repeat for each level */
		int t = lua_type(L, i);
		switch(t) {
			case LUA_TSTRING:  /* strings */
				printf("%d: '%s'", i, lua_tostring(L, i));
			break;
			case LUA_TBOOLEAN:  /* booleans */
				printf("%d: %s", i, lua_toboolean(L, i) ? "true" : "false");
			break;
			case LUA_TNUMBER:  /* numbers */
				printf("%d: %g", i, lua_tonumber(L, i));
			break;
			default:  /* other values */
				printf("%d: %s", i, lua_typename(L, t));
			break;
		}
		printf("  ");  /* put a separator */
	}
	printf("\n");  /* end the listing */
}
/* LCOV_EXCL_STOP */

static void plua_metatable_free(struct plua_metatable_t *table) {
	int x = 0;
	for(x=0;x<table->nrvar;x++) {
		if(table->table[x].val.type_ == LUA_TSTRING) {
			FREE(table->table[x].val.string_);
		}
		if(table->table[x].key.type_ == LUA_TSTRING) {
			FREE(table->table[x].key.string_);
		}
	}
	if(table->nrvar > 0) {
		FREE(table->table);
	}
	FREE(table);
}

void plua_metatable_clone(struct plua_metatable_t **src, struct plua_metatable_t **dst) {
	int i = 0;
	struct plua_metatable_t *a = *src;

	if((*dst) != NULL) {
		plua_metatable_free((*dst));
	}
	if(((*dst) = MALLOC(sizeof(struct plua_metatable_t))) == NULL) {
		OUT_OF_MEMORY
	}
	memset((*dst), 0, sizeof(struct plua_metatable_t));
	if(((*dst)->table = MALLOC(sizeof(*a->table)*(a->nrvar))) == NULL) {
		OUT_OF_MEMORY
	}
	memset((*dst)->table, 0, sizeof(*a->table)*(a->nrvar));
	for(i=0;i<a->nrvar;i++) {
		(*dst)->table[i].key.type_ = a->table[i].key.type_;
		(*dst)->table[i].val.type_ = a->table[i].val.type_;

		if(a->table[i].key.type_ == LUA_TSTRING) {
			if(((*dst)->table[i].key.string_ = STRDUP(a->table[i].key.string_)) == NULL) {
				OUT_OF_MEMORY
			}
		}
		if(a->table[i].key.type_ == LUA_TNUMBER) {
			(*dst)->table[i].key.number_ = a->table[i].key.number_;
		}

		if(a->table[i].val.type_ == LUA_TSTRING) {
			if(((*dst)->table[i].val.string_ = STRDUP(a->table[i].val.string_)) == NULL) {
				OUT_OF_MEMORY
			}
		}
		if(a->table[i].val.type_ == LUA_TNUMBER) {
			(*dst)->table[i].val.number_ = a->table[i].val.number_;
		}
	}
	(*dst)->nrvar = a->nrvar;
}

int plua_metatable_pairs(lua_State *L) {
	logprintf(LOG_NOTICE, "pilight lua metatables do not support pairs");
  return 0;
}

int plua_metatable_get(lua_State *L) {
	struct plua_metatable_t *node = NULL;
	char buf[128] = { '\0' }, *p = buf;
	char *error = "string or number expected, got %s";
	int x = 0;

	sprintf(p, error, lua_typename(L, lua_type(L, -1)));

	luaL_argcheck(L,
		(lua_isstring(L, -1) || lua_isnumber(L, -1)),
		1, buf);

	struct lua_state_t *state = plua_get_current_state(L);

	node = state->table;
	for(x=0;x<node->nrvar;x++) {
		if((lua_isnumber(L, -1) == 1 && node->table[x].key.type_ == LUA_TNUMBER && node->table[x].key.number_ == (int)lua_tonumber(L, -1)) ||
		   (lua_isstring(L, -1) == 1 && node->table[x].key.type_ == LUA_TSTRING && strcmp(node->table[x].key.string_, lua_tostring(L, -1)) == 0)) {
			if(node->table[x].val.type_ == LUA_TNUMBER) {
				lua_pushnumber(L, node->table[x].val.number_);
			}
			if(node->table[x].val.type_ == LUA_TSTRING) {
				lua_pushstring(L, node->table[x].val.string_);
			}
			return 1;
		}
	}

	return 0;
}

int plua_metatable_set(lua_State *L) {
	struct plua_metatable_t *node = NULL;
	char buf[128] = { '\0' }, *p = buf;
	char *error = "string or number expected, got %s";
	int match = 0, x = 0;

	sprintf(p, error, lua_typename(L, lua_type(L, -1)));

	luaL_argcheck(L,
		(lua_isstring(L, -1) || lua_isnumber(L, -1)),
		1, buf);

	sprintf(p, error, lua_typename(L, lua_type(L, -1)));

	luaL_argcheck(L,
		(lua_isstring(L, -2) || lua_isnumber(L, -2)),
		1, buf);

	struct lua_state_t *state = plua_get_current_state(L);

	node = state->table;
	for(x=0;x<node->nrvar;x++) {
		if((lua_isnumber(L, -2) == 1 && node->table[x].key.type_ == LUA_TNUMBER && node->table[x].key.number_ == (int)lua_tonumber(L, -1)) ||
		   (lua_isstring(L, -2) == 1 && node->table[x].key.type_ == LUA_TSTRING && strcmp(node->table[x].key.string_, lua_tostring(L, -1)) == 0)) {
			match = 1;
			if(lua_isnumber(L, -1) == 1) {
				node->table[x].val.number_ = lua_tonumber(L, -1);
				node->table[x].val.type_ = LUA_TNUMBER;
			}
			if(lua_isstring(L, -1) == 1) {
				node->table[x].val.string_ = STRDUP((char *)lua_tostring(L, -1));
				node->table[x].val.type_ = LUA_TSTRING;
			}
			break;
		}
	}

	if(node != NULL) {
		if(match == 0) {
			int idx = node->nrvar;
			if((node->table = REALLOC(node->table, sizeof(*node->table)*(idx+1))) == NULL) {
				OUT_OF_MEMORY
			}
			if(lua_isnumber(L, -1) == 1) {
				node->table[idx].val.number_ = lua_tonumber(L, -1);
				node->table[idx].val.type_ = LUA_TNUMBER;
			}
			if(lua_isstring(L, -1) == 1) {
				node->table[idx].val.string_ = STRDUP((char *)lua_tostring(L, -1));
				node->table[idx].val.type_ = LUA_TSTRING;
			}
			if(lua_isnumber(L, -2) == 1) {
				node->table[idx].key.number_ = lua_tonumber(L, -2);
				node->table[idx].key.type_ = LUA_TNUMBER;
			}
			if(lua_isstring(L, -2) == 1) {
				node->table[idx].key.string_ = STRDUP((char *)lua_tostring(L, -2));
				node->table[idx].key.type_ = LUA_TSTRING;
			}
			node->nrvar++;
		}
	}
	return 1;
}

int plua_metatable_gc(lua_State *L){
	return 1;
}

struct lua_state_t *plua_get_free_state(void) {
	int i = 0;
	for(i=0;i<NRLUASTATES;i++) {
		if(uv_mutex_trylock(&lua_state[i].lock) == 0) {
			if(lua_state[i].table != NULL) {
				plua_metatable_free(lua_state[i].table);
				if((lua_state[i].table = MALLOC(sizeof(struct plua_metatable_t))) == NULL) {
					OUT_OF_MEMORY
				}
				memset(lua_state[i].table, 0, sizeof(struct plua_metatable_t));
			}
			return &lua_state[i];
		}
	}
	return NULL;
}

struct lua_state_t *plua_get_current_state(lua_State *L) {
	int i = 0;
	for(i=0;i<NRLUASTATES;i++) {
		if(lua_state[i].L == L) {
			return &lua_state[i];
		}
	}
	return NULL;
}

static int plua_get_table_string_by_key(struct lua_State *L, const char *key, const char **ret) {
	/*
	 * Push the key we want to retrieve on the stack
	 *
	 * stack now contains: -1 => key -2 => table
	 */
	lua_pushstring(L, key);

	if(lua_istable(L, -2) == 0) {
		/*
		 * Remove the key from the stack again
		 *
		 * stack now contains: -1 => table
		 */
		lua_pop(L, 1);
		return 0;
	}

	/*
	 * Replace the key at -1 with it value in table -2
	 *
	 * stack now contains: -1 => value -2 => table
	 */
	lua_gettable(L, -2);

	/*
	 * Check if the first element is a number
	 */
	if(lua_isstring(L, -1) == 0) {
		/*
		 * Remove the value from the stack again
		 *
		 * stack now contains: -1 => table
		 */
		lua_pop(L, 1);
		return 0;
	}

	*ret = lua_tostring(L, -1);

	/*
	 * stack now contains: -1 => table
	 */
	lua_pop(L, 1);
	return 1;
}

// static int plua_get_table_double_by_key(struct lua_State *L, const char *key, double *ret) {
	// /*
	 // * Push the key we want to retrieve on the stack
	 // *
	 // * stack now contains: -1 => key -2 => table
	 // */
	// lua_pushstring(L, key);

	// /*
	 // * Replace the key at -1 with it value in table -2
	 // *
	 // * stack now contains: -1 => value -2 => table
	 // */
	// if(lua_istable(L, -2) == 0) {
		// /*
		 // * Remove the key from the stack again
		 // *
		 // * stack now contains: -1 => table
		 // */
		// lua_pop(L, 1);
		// return 0;
	// }
	// /*
	 // * Replace the key at -1 with it value in table -2
	 // *
	 // * stack now contains: -1 => value -2 => table
	 // */
	// lua_gettable(L, -2);

	// /*
	 // * Check if the first element is a number
	 // */
	// if(lua_isnumber(L, -1) == 0) {
		// /*
		 // * Remove the value from the stack again
		 // *
		 // * stack now contains: -1 => table
		 // */
		// lua_pop(L, 1);
		// return 0;
	// }

	// *ret = lua_tonumber(L, -1);

	// /*
	 // * stack now contains: -1 => table
	 // */
	// lua_pop(L, 1);

	// return 1;
// }

static int plua_table_has_keys(lua_State *L, char **keys, int number) {
	if(lua_istable(L, -1) == 0) {
		return 0;
	}

	int i = 0;
	/*
	 * Push another reference to the table on top of the stack (so we know
	 * where it is), and this function can work for negative, positive and
	 * pseudo indices.
	 *
	 * stack now contains: -1 => table -2 => table
	 */
	lua_pushvalue(L, -1);

	/*
	 * stack now contains: -1 => nil -2 => table -3 => table
	 */
	lua_pushnil(L);

	int match = 0, nrkeys = 0;
	while(lua_next(L, -2)) {
		nrkeys++;

		/*
		 * stack now contains: -1 => value -2 => key -3 => table
		 *
		 * copy the key so that lua_tostring does not modify the original
		 *
		 * stack now contains: -1 => key -2 => value; -3 => key -4 => table -5 => table
		 */
		lua_pushvalue(L, -2);

		const char *k = lua_tostring(L, -1); // key

		for(i=0;i<number;i++) {
			if(strcmp(keys[i], k) == 0) {
				match++;
				break;
			}
		}

		/*
		 * pop value + copy of key, leaving original key
		 *
		 * stack now contains: -1 => key -2 => table -3 => table
		 */
		lua_pop(L, 2);
	}
	/*
	 * After the last lua_next call stack now contains:
	 * -1 => table -2 => table
	 */
	if(match != number || nrkeys != number) {
		return 0;
	}

	/*
	 * Remove duplicated table from stack
	 *
	 * stack now contains -1 => table
	 */
	lua_pop(L, 1);

	return 1;
}

static int plua_module_init(struct lua_State *L, char *file, struct plua_module_t *mod) {
	/*
	 * Function info is at top of stack
	 *
	 * stack now contains -1 => function
	 */
#if LUA_VERSION_NUM <= 502
	lua_getfield(L, -1, "info");
	if(strcmp(lua_typename(L, lua_type(L, -1)), "function") != 0) {
#else
	if(lua_getfield(L, -1, "info") == 0) {
#endif
		logprintf(LOG_ERR, "%s: info function missing", file);
		return 0;
	}

	char *type = NULL;
	switch(mod->type) {
		case FUNCTION: {
			type = STRDUP("event function");
		} break;
		case OPERATOR: {
			type = STRDUP("event operator");
		} break;
	}
	if(type == NULL) {
		OUT_OF_MEMORY
	}

	/*
	 * Returned table (first argument) is at top of stack
	 *
	 * stack now contains -1 => function
	 */
	if(lua_pcall(L, 0, 1, 0) == LUA_ERRRUN) {
		if(strcmp(lua_typename(L, lua_type(L, -1)), "string") == 0) {
			logprintf(LOG_ERR, "%s", lua_tostring(L,  -1));
			lua_pop(L, 1);
			return 0;
		}
	}

	if(lua_istable(L, -1) == 0) {
		logprintf(LOG_ERR, "%s: the info function returned %s, table expected", file, lua_typename(L, lua_type(L, -1)));
		FREE(type);
		return 0;
	}

	char *keys[12] = {"name", "version", "reqversion", "reqcommit"};
	if(plua_table_has_keys(L, keys, 4) == 0) {
		logprintf(LOG_ERR, "%s: the info table has invalid keys", file);
		FREE(type);
		return 0;
	}

	const char *name = NULL, *version = NULL, *reqversion = NULL, *reqcommit = NULL;
	if(plua_get_table_string_by_key(L, "name", &name) == 0) {
		logprintf(LOG_ERR, "%s: the info table 'name' key is missing or invalid", file);
		FREE(type);
		return 0;
	}

	if(plua_get_table_string_by_key(L, "version", &version) == 0) {
		logprintf(LOG_ERR, "%s: the info table 'version' key is missing or invalid", file);
		FREE(type);
		return 0;
	}

	if(plua_get_table_string_by_key(L, "reqversion", &reqversion) == 0) {
		logprintf(LOG_ERR, "%s: the info table 'reqversion' key is missing or invalid", file);
		FREE(type);
		return 0;
	}
	if(plua_get_table_string_by_key(L, "reqcommit", &reqcommit) == 0) {
		logprintf(LOG_ERR, "%s: the info table 'reqcommit' key is missing or invalid", file);
		FREE(type);
		return 0;
	}

	strcpy(mod->name, name);
	strcpy(mod->version, version);
	strcpy(mod->reqversion, reqversion);
	strcpy(mod->reqcommit, reqcommit);

	char pilight_version[strlen(PILIGHT_VERSION)+1];
	char pilight_commit[3], *v = (char *)reqversion, *r = (char *)reqcommit;
	int valid = 1, check = 1;

	memset(&pilight_commit, 0, sizeof(pilight_commit));

	strcpy(pilight_version, PILIGHT_VERSION);

	if((check = vercmp(v, pilight_version)) > 0) {
		valid = 0;
	}

	if(check == 0 && strlen(mod->reqcommit) > 0) {
		sscanf(HASH, "v%*[0-9].%*[0-9]-%[0-9]-%*[0-9a-zA-Z\n\r]", pilight_commit);

		if(strlen(pilight_commit) > 0 && (vercmp(r, pilight_commit)) > 0) {
			valid = 0;
		}
	}
	if(valid == 1) {
		logprintf(LOG_DEBUG, "loaded %s %s v%s", type, file, version);
	} else {
		if(strlen(mod->reqcommit) > 0) {
			logprintf(LOG_ERR, "%s %s requires at least pilight v%s (commit %s)", type, file, mod->reqversion, mod->reqcommit);
		} else {
			logprintf(LOG_ERR, "%s %s requires at least pilight v%s", type, file, mod->reqversion);
		}
		/*
		 * Pop function from stack
		 *
		 * The stack now contains: nothing
		 */
		lua_pop(L, 1);
		FREE(type);
		return 0;
	}

	/*
	 * Pop function from stack
	 *
	 * The stack now contains: nothing
	 */
	lua_pop(L, 1);
	FREE(type);
	return 1;
}

static int plua_writer(lua_State *L, const void* p, size_t sz, void* ud) {
	struct plua_module_t *module = ud;
	if((module->bytecode = REALLOC(module->bytecode, module->size+sz)) == NULL) {
		OUT_OF_MEMORY
	}
	memcpy(module->bytecode, p, sz);
	module->size += sz;
	return 0;
}

void plua_module_load(char *file, int type) {
	struct plua_module_t *module = MALLOC(sizeof(struct plua_module_t));
	lua_State *L = lua_state[0].L;
	char name[255] = { '\0' }, *p = name;
	int i = 0;
	if(module == NULL) {
		OUT_OF_MEMORY
	}
	memset(module, 0, sizeof(struct plua_module_t));

	if(luaL_loadfile(L, file) != 0) {
		logprintf(LOG_ERR, "cannot load lua file: %s", file);
		lua_pop(L, 1);
		FREE(module);
		return;
	}
	strcpy(module->file, file);
	if(lua_dump(L, plua_writer, module) != 0) {
		logprintf(LOG_ERR, "cannot dump lua file: %s", file);
		lua_pop(L, 1);
		FREE(module->bytecode);
		FREE(module);
		return;
	}

	lua_pcall(L, 0, LUA_MULTRET, 0);

	if(lua_istable(L, -1) == 0) {
		logprintf(LOG_ERR, "%s: does not return a table", file);
		lua_pop(L, 1);
		FREE(module->bytecode);
		FREE(module);
		return;
	}

	module->type = type;
	strcpy(module->file, file);
	if(plua_module_init(L, file, module) != 0) {
		memset(p, '\0', sizeof(name));
		switch(module->type) {
			case OPERATOR:
				sprintf(p, "operator.%s", module->name);
			break;
			case FUNCTION:
				sprintf(p, "function.%s", module->name);
			break;
		}

		module->next = modules;
		modules = module;
	} else {
		FREE(module->bytecode);
		FREE(module);
		return;
	}
	lua_setglobal(L, name);

	for(i=1;i<NRLUASTATES;i++) {
		L = lua_state[i].L;
		luaL_loadbuffer(L, module->bytecode, module->size, module->name);
	}
	lua_pcall(L, 0, LUA_MULTRET, 0);
	lua_setglobal(L, name);
}

struct plua_module_t *plua_get_modules(void) {
	return modules;
}

// static void *plua_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
	// if(nsize == 0) {
		// if(ptr != NULL) {
			// FREE(ptr);
		// }
		// return 0;
	// } else {
		// return REALLOC(ptr, nsize);
	// }
// }

void plua_init(void) {
	if(init == 1) {
		return;
	}
	init = 1;

	int i = 0;
	for(i=0;i<NRLUASTATES;i++) {
		memset(&lua_state[i], 0, sizeof(struct lua_state_t));
		uv_mutex_init(&lua_state[i].lock);

		if((lua_state[i].table = MALLOC(sizeof(struct plua_metatable_t))) == NULL) {
			OUT_OF_MEMORY
		}
		memset(lua_state[i].table, 0, sizeof(struct plua_metatable_t));
		lua_State *L = luaL_newstate();

		luaL_openlibs(L);
		plua_register_library(L);
		lua_state[i].L = L;
		lua_state[i].idx = i;
	}
}

int plua_module_exists(char *module, int type) {
	struct lua_state_t *state = plua_get_free_state();
	struct lua_State *L = NULL;

	if(state == NULL) {
		return 1;
	}
	if((L = state->L) == NULL) {
		uv_mutex_unlock(&state->lock);
		return 1;
	}

	char name[255], *p = name;
	memset(name, '\0', 255);

	switch(type) {
		case OPERATOR: {
			sprintf(p, "operator.%s", module);
		} break;
		case FUNCTION:
			sprintf(p, "function.%s", module);
		break;
	}

	lua_getglobal(L, name);
	if(lua_isnil(L, -1) != 0) {
		lua_pop(L, -1);
		return -1;
	}
	if(lua_istable(L, -1) == 0) {
		lua_pop(L, -1);
		return -1;
	}
	lua_pop(L, -1);

	uv_mutex_unlock(&state->lock);
	return 0;
}

int plua_gc(void) {
	struct plua_module_t *tmp = NULL;
	while(modules) {
		tmp = modules;
		FREE(tmp->bytecode);
		modules = modules->next;
		FREE(tmp);
	}
	int i = 0;
	for(i=0;i<NRLUASTATES;i++) {
		if(lua_state[i].L != NULL) {
			plua_metatable_free(lua_state[i].table);
			lua_close(lua_state[i].L);
			lua_state[i].L = NULL;
		}
	}
	init = 0;
	logprintf(LOG_DEBUG, "garbage collected lua library");
	return 0;
}
