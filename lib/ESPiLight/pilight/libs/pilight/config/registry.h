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

#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#include "../core/json.h"
#include "../core/config.h"

struct config_t *config_registry;

void registry_init(void);
int registry_gc(void);
int registry_get_string(const char *key, char **value);
int registry_get_number(const char *key, double *value, int *decimals);
int registry_set_string(const char *key, char *value);
int registry_set_number(const char *key, double value, int decimals);
int registry_remove_value(const char *key);

#endif
