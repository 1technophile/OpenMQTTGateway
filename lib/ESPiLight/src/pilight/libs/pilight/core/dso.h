/*
	Copyright (C) 2013 CurlyMo

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

#ifndef _DSO_H_
#define _DSO_H_

typedef struct dso_t {
	void *handle;
	char *name;
	ssize_t size;
	struct dso_t *next;
} dso_t;

typedef struct module_t {
	const char *name;
	const char *version;
	const char *reqversion;
	const char *reqcommit;
} module_t;

struct dso_t *dso;

void *dso_load(char *object);
int dso_gc(void);
void *dso_function(void *handle, const char *name);

#endif
