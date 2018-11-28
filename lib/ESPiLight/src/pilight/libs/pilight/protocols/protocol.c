/*
  ESPiLight - pilight 433.92 MHz protocols library for Arduino
  Copyright (c) 2016 Puuu.  All right reserved.

  Project home: https://github.com/puuu/espilight/
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with library. If not, see <http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/pilight.h"
#include "../core/log.h"
#include "protocol.h"

#include "protocol_header.h"

struct protocols_t *pilight_protocols = NULL;

void protocol_init(void) {
  #include "protocol_init.h"
}

void protocol_register(protocol_t **proto) {
  if((*proto = MALLOC(sizeof(struct protocol_t))) == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  //(*proto)->options = NULL;
  //(*proto)->devices = NULL;

  (*proto)->rawlen = 0;
  (*proto)->minrawlen = 0;
  (*proto)->maxrawlen = 0;
  (*proto)->mingaplen = 0;
  (*proto)->maxgaplen = 0;
  (*proto)->txrpt = 10;
  (*proto)->rxrpt = 1;
  (*proto)->hwtype = NONE;
  //(*proto)->multipleId = 1;
  //(*proto)->config = 1;
  //(*proto)->masterOnly = 0;
  (*proto)->parseCode = NULL;
  (*proto)->createCode = NULL;
  (*proto)->checkValues = NULL;
  //(*proto)->initDev = NULL;
  (*proto)->printHelp = NULL;
  //(*proto)->threadGC = NULL;
  (*proto)->gc = NULL;
  (*proto)->message = NULL;
  //(*proto)->threads = NULL;

  (*proto)->repeats = 0;
  (*proto)->first = 0;
  (*proto)->second = 0;

  (*proto)->raw = NULL;

  /* Arduino special, compare repeated messages*/
  (*proto)->old_content = NULL;

  struct protocols_t *pnode = MALLOC(sizeof(struct protocols_t));
  if(pnode == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  pnode->listener = *proto;
  pnode->next = pilight_protocols;
  pilight_protocols = pnode;
}

void protocol_set_id(protocol_t *proto, char *id) {
  proto->id = id;
}
