/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "../core/common.h"

#include "events.h" /* rewrite */

struct event_function_args_t {
	struct varcont_t var;
	struct event_function_args_t *next;
} event_function_args_t;

void event_function_init(void);
int event_function_callback(char *, struct event_function_args_t *, struct varcont_t *v);
struct event_function_args_t *event_function_add_argument(struct varcont_t *, struct event_function_args_t *);
void event_function_free_argument(struct event_function_args_t *);
int event_function_exists(char *);
int event_function_gc(void);

#endif
