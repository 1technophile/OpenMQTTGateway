/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _MAIL_H_
#define _MAIL_H_

typedef struct mail_t {
	char *from;
	char *to;
	char *subject;
	char *message;
} mail_t;

int sendmail(char *, char *, char *, unsigned short, int, struct mail_t *, void (*)(int, struct mail_t *));

#endif
