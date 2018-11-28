/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _HTTP_H_
#define _HTTP_H_

char *http_post_content(char *url, const char *contype, char *post, void (*callback)(int, char *, int, char *, void *), void *userdata);
char *http_get_content(char *url, void (*callback)(int, char *, int, char *, void *), void *userdata);
int http_gc(void);

#endif
