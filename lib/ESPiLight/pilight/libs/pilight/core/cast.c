/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
	#include <unistd.h>
#endif

#include "common.h"
#include "../events/events.h" /* rewrite */

int cast2bool(struct varcont_t **a) {
	struct varcont_t b;
	memset(&b, 0, sizeof(struct varcont_t));

	b.type_ = JSON_BOOL;

	if((*a)->type_ == JSON_NUMBER) {
		if((int)(*a)->number_ != 0) {
			b.bool_ = 1;
		}
	}
	if((*a)->type_ == JSON_STRING) {
		if(strcmp((*a)->string_, "0") == 0) {
			b.bool_ = 0;
		} else if(strlen((*a)->string_) > 0) {
			b.bool_ = 1;
		}
		if((*a)->free_ == 1) {
			FREE((*a)->string_);
			(*a)->free_ = 0;
		}
	}
	if((*a)->type_ == JSON_BOOL) {
		b.bool_ = (*a)->bool_;
	}

	memcpy(*a, &b, sizeof(struct varcont_t));
	return 0;
}

int cast2int(struct varcont_t **a) {
	struct varcont_t b;
	memset(&b, 0, sizeof(struct varcont_t));

	b.type_ = JSON_NUMBER;

	if((*a)->type_ == JSON_STRING) {
		b.number_  = atof((*a)->string_);
		if(b.number_ != 0) {
			b.decimals_ = nrDecimals((*a)->string_);
		}
		if((*a)->free_ == 1) {
			FREE((*a)->string_);
			(*a)->free_ = 0;
		}
	}
	if((*a)->type_ == JSON_BOOL) {
		if((*a)->bool_ == 0) {
			b.number_ = 0;
		} else {
			b.number_ = 1;
		}
	}
	if((*a)->type_ == JSON_NUMBER) {
		b.number_ = (*a)->number_;
		b.decimals_ = (*a)->decimals_;
	}

	memcpy(*a, &b, sizeof(struct varcont_t));
	return 0;
}

int cast2str(struct varcont_t **a) {
	struct varcont_t b;
	memset(&b, 0, sizeof(struct varcont_t));

	b.type_ = JSON_STRING;

	if((*a)->type_ == JSON_BOOL) {
		struct varcont_t bb, *bbb = &bb;
		memcpy(&bb, *a, sizeof(struct varcont_t));
		cast2int(&bbb);
		memcpy(*a, &bb, sizeof(struct varcont_t));
	}
	if((*a)->type_ == JSON_NUMBER) {
		int len = snprintf(NULL, 0, "%.*f", (*a)->decimals_, (*a)->number_);
		if((b.string_ = MALLOC(len+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		memset(b.string_, 0, len+1);
		sprintf(b.string_, "%.*f", (*a)->decimals_, (*a)->number_);
	}
	if((*a)->type_ == JSON_STRING) {
		b.string_ = STRDUP((*a)->string_);
		if((*a)->free_ == 1) {
			FREE((*a)->string_);
			(*a)->free_ = 0;
		}
	}

	memcpy(*a, &b, sizeof(struct varcont_t));
	return 0;
}
