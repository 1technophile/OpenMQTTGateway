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

#ifndef _COMMON_H_
#define _COMMON_H_

#ifndef _WIN32
	#include <sys/types.h>
	#include <ifaddrs.h>
	#include <sys/socket.h>
#endif
#include <pthread.h>
#include <stdint.h>

typedef struct varcont_t {
	union {
		char *string_;
		double number_;
		int bool_;
	};
	int decimals_;
	int type_;
	int free_;
} varcont_t;

#include "pilight.h"

extern char *progname;

#ifdef _WIN32
#define sleep(a) Sleep(a*1000)
int check_instances(const wchar_t *prog);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
int isrunning(const char *program);
#endif

void array_free(char ***array, int len);
int isrunning(const char *program);
void atomicinit(void);
void atomiclock(void);
void atomicunlock(void);
unsigned int explode(const char *str, const char *delimiter, char ***output);
int isNumeric(char *str);
int nrDecimals(char *str);
int name2uid(char const *name);
int which(const char *program);
int ishex(int x);
const char *rstrstr(const char* haystack, const char* needle);
void alpha_random(char *s, const int len);
int urldecode(const char *s, char *dec);
char *urlencode(char *str);
char *base64encode(char *src, size_t len);
char *base64decode(char *src, size_t len, size_t *decsize);
char *hostname(void);
char *distroname(void);
void rmsubstr(char *s, const char *r);
char *genuuid(char *ifname);
int file_exists(char *fil);
int path_exists(char *fil);
char *uniq_space(char *str);

#ifdef __FreeBSD__
int findproc(char *name, char *args, int loosely);
#else
pid_t findproc(char *name, char *args, int loosely);
#endif

int vercmp(char *val, char *ref);
int str_replace(char *search, char *replace, char **str);
#ifndef _WIN32
int stricmp(char const *a, char const *b);
int strnicmp(char const *a, char const *b, size_t len);
#endif
int file_get_contents(char *file, char **content);
int check_email_addr(const char *addr, int allow_lists, int check_domain_can_mail);

#endif
