/*
	Copyright (C) 2014 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _NTP_H_
#define _NTP_H_

#include "../../libuv/uv.h"

typedef struct ntp_servers_t {
	struct {
		char host[255];
		int port;
	} server[10];
	int nrservers;
	void (*callback)(int, time_t);
} ntp_servers_t;

struct ntp_servers_t ntp_servers;

void ntpsync(void);
void ntp_gc(void);
int getntpdiff(void);
int isntpsynced(void);

#endif
