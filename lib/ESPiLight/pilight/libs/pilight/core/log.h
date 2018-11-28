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

#ifndef _LOG_H_
#define _LOG_H_

#ifdef _WIN32
	#define	LOG_EMERG	0
	#define	LOG_ALERT	1
	#define	LOG_CRIT	2
	#define	LOG_ERR		3
	#define	LOG_WARNING	4
	#define	LOG_NOTICE	5
	#define	LOG_INFO	6
	#define	LOG_DEBUG	7
#else
	#include <syslog.h>
#endif

#define LOG_STACK		255

void logprintf1(int prio, char *file, int line, const char *format_str, ...);
void logprintf(int prio, const char *format_str, ...);
void logperror(int prio, const char *s);
void *logloop(void *param);
void log_file_enable(void);
void log_file_disable(void);
void log_shell_enable(void);
void log_shell_disable(void);
int log_file_set(char *file);
void log_level_set(int level);
int log_level_get(void);
int log_gc(void);
void logerror(const char *format_str, ...);

#endif
