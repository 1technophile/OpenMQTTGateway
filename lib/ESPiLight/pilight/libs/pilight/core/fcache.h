/*
Copyright 2013 CurlyMo <curlymoo1@gmail.com>

This file is part of Splash - Linux GUI Framebuffer Splash.

Splash is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Splash is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along
with Splash. If not, see <http://www.gnu.org/licenses/>
*/

#ifndef _FCACHE_H_
#define _FCACHE_H_

typedef struct fcache_t {
	char *name;
	int size;
	unsigned char *bytes;
	struct fcache_t *next;
} fcaches_t;

struct fcache_t *fcache;

int fcache_gc(void);
int fcache_add(char *filename);
int fcache_rm(char *filename);
short fcache_get_size(char *filename, int *out);
unsigned char *fcache_get_bytes(char *filename);

#endif
