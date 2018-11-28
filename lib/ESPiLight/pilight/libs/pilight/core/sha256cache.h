/*
	Copyright (C) 2015 CurlyMo

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
#include <fcntl.h>
#include <sys/stat.h>

typedef struct sha256cache_t {
	char *name;
	char hash[65];
	struct sha256cache_t *next;
} sha256cache_t;

struct sha256cache_t *sha256cache;

int sha256cache_gc(void);
void sha256cache_remove_node(struct sha256cache_t **sha256cache, char *name);
int sha256cache_rm(char *name);
int sha256cache_add(char *name);
char *sha256cache_get_hash(char *name);
