/*
	Copyright (C) 2014 CurlyMo & wo_rasp & RvW

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
/* Protocol quigg_gt1000 is an implementation of a protocol for the Quigg GT-FSI-08 switches
   with GT-1000 remote. It is believed that they are compatible with the Lidl and Brennenstuhl outlets
   RvW, jan 2015
*/

#ifndef _PROTOCOL_QUIGG_GT1000_H_
#define _PROTOCOL_QUIGG_GT1000_H_

#include "../protocol.h"

struct protocol_t *quigg_gt1000;
void quiggGT1000Init(void);

#endif
