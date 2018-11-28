/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _EVENT_OPERATOR_H_
#define _EVENT_OPERATOR_H_

#include "../core/common.h"

#include "events.h" /* rewrite */

void event_operator_init(void);
int event_operator_callback(char *, struct varcont_t *, struct varcont_t *, struct varcont_t *v);
int event_operator_associativity(char *, int *);
int event_operator_precedence(char *, int *);
int event_operator_exists(char *);
int event_operator_gc(void);

#endif
