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

#ifndef _SSDP_H_
#define _SSDP_H_

#ifndef _WIN32
	#include <ifaddrs.h>
#endif

typedef struct ssdp_list_t {
	char ip[17];
	unsigned short port;
	struct ssdp_list_t *next;
} ssdp_list_t;

int ssdp_gc(void);
int ssdp_start(void);
int ssdp_seek(struct ssdp_list_t **ssdp_list);
void ssdp_free(struct ssdp_list_t *ssdp_list);
void *ssdp_wait(void* param);
void ssdp_close(int ssdp_socket);

#endif
