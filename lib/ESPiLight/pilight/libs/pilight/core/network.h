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

#ifndef _NETWORK_H_
#define _NETWORK_H_

#ifndef _WIN32
	#include <sys/types.h>
	#include <ifaddrs.h>
	#include <sys/socket.h>
#endif
#include <pthread.h>
#include <stdint.h>

#include "pilight.h"

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifdef _WIN32
#define sa_family_t uint16_t
int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, int cnt);
#endif

int inetdevs(char ***array);
int dev2mac(char *ifname, char **mac);
#ifdef __FreeBSD__
int dev2ip(char *dev, char **ip, __sa_family_t type);
#else
int dev2ip(char *dev, char **ip, sa_family_t type);
#endif
int host2ip(char *host, char **ip);
int whitelist_check(char *ip);
void whitelist_free(void);

#ifdef __FreeBSD__
struct sockaddr *sockaddr_dup(struct sockaddr *sa);
int rep_getifaddrs(struct ifaddrs **ifap);
void rep_freeifaddrs(struct ifaddrs *ifap);
#endif

#endif
