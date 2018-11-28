/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _DATATYPES_STACK_T_
#define _DATATYPES_STACK_T_

typedef struct stack_dt {
	void **list;
	int size;
	int top;
	int type;
} stack_dt;

void dt_stack_free(struct stack_dt *, void (*)(void *));
void dt_stack_push(struct stack_dt *, size_t, void *);
int dt_stack_top(struct stack_dt *);
void dt_stack_insert(struct stack_dt *, size_t, int, void *);
void *dt_stack_pop(struct stack_dt *, int);
void *dt_stack_peek(struct stack_dt *, int);

#endif