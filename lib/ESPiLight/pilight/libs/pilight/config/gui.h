/*
	Copyright (C) 2013 - 2014 CurlyMo

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

#ifndef _GUI_H_
#define _GUI_H_

#include "../core/json.h"
#include "../core/config.h"

struct gui_values_t {
	union {
		char *string_;
		double number_;
	};
	int decimals;
	char *name;
	int type;
	struct gui_values_t *next;
};

struct gui_settings_t {
	char *name;
	struct gui_values_t *values;
	struct gui_settings_t *next;
};

struct gui_elements_t {
	char *id;
	struct devices_t *device;
	struct gui_settings_t *settings;
	struct gui_elements_t *next;
};

struct config_t *config_gui;

struct gui_values_t *gui_media(char *name);
void gui_init(void);
int gui_gc(void);

#endif
