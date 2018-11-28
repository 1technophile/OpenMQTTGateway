/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _DATETIME_H_
#define _DATETIME_H_

int datetime_gc(void);
char *coord2tz(double, double);
time_t datetime2ts(int, int, int, int, int, int);
int tzoffset(char *, char *, double *);
int isdst(time_t, char *);
void datefix(int *, int *, int *, int *, int *, int *, int *);
void datetime_init(void);
int localtime_l(time_t, struct tm *, char *);

#endif
