/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/mem.h"
#include "stack.h"

void dt_stack_free(struct stack_dt *stack, void (*func)(void *node)) {
	int i = 0;
	for(i=0;i<stack->top;i++) {
		if(func != NULL) {
			func(stack->list[i]);
		}
	}
	if(stack->size > 0) {
		FREE(stack->list);
	}
	FREE(stack);
}

void dt_stack_push(struct stack_dt *stack, size_t size, void *item) {
	if(stack->size == 0 || stack->size <= stack->top) {
		if((stack->list = REALLOC(stack->list, size*(stack->size+12))) == NULL) {
			OUT_OF_MEMORY
		}
		stack->size += 12;
	}
	stack->list[stack->top++] = item;
}

int dt_stack_top(struct stack_dt *stack) {
	return stack->top;
}

void dt_stack_insert(struct stack_dt *stack, size_t size, int pos, void *item) {
	int i = stack->top;
	while(i >= pos) {
		stack->list[i] = stack->list[i-1];
		i--;
	}
	if(stack->size == 0 || stack->size <= stack->top) {
		if((stack->list = REALLOC(stack->list, size*(stack->size+12))) == NULL) {
			printf("out of memory\n");
			exit(-1);
		}
		stack->size += 12;
	}
	stack->top++;
	stack->list[pos] = item;
}

void *dt_stack_pop(struct stack_dt *stack, int i) {
	if(stack->top <= 0) {
		return NULL;
	} else if(i == -1) {
		return stack->list[--stack->top];
	} else if(i < stack->top) {
		int x = 0;
		char *out = stack->list[i];
		for(x=i;x<stack->top-1;x++) {
			stack->list[x] = stack->list[x+1];
		}
		stack->top--;
		return out;
	}
	return NULL;
}

void *dt_stack_peek(struct stack_dt *stack, int i) {
	if(stack->top <= 0) {
		return NULL;
	} else if(i == -1) {
		return stack->list[stack->top-1];
	} else if(i < stack->top) {
		return stack->list[i];
	} else {
		return NULL;
	}
}