/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _CAST_H_
#define _CAST_H_

#include "common.h"
#include "../events/events.h" /* rewrite */

int cast2bool(struct varcont_t **);
int cast2int(struct varcont_t **);
int cast2str(struct varcont_t **);

#endif
