
/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2000-2004  Brian S. Dean <bsd@bsdhome.com>
 * Copyright (C) 2006 Joerg Wunsch <j@uriah.heep.sax.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* $Id: avrpart.c 772 2008-06-07 21:03:41Z joerg_wunsch $ */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../pilight/core/log.h"
#include "../pilight/core/mem.h"
#include "avrdude.h"
#include "avrpart.h"
#include "pindefs.h"

/***
 *** Elementary functions dealing with OPCODE structures
 ***/

OPCODE * avr_new_opcode(void)
{
  OPCODE * m;

  m = (OPCODE *)MALLOC(sizeof(*m));
  if (m == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }

  memset(m, 0, sizeof(*m));

  return m;
}


/*
 * avr_set_bits()
 *
 * Set instruction bits in the specified command based on the opcode.
 */
int avr_set_bits(OPCODE * op, unsigned char * cmd)
{
  int i, j, bit;
  unsigned char mask;

  for (i=0; i<32; i++) {
    if (op->bit[i].type == AVR_CMDBIT_VALUE) {
      j = 3 - i / 8;
      bit = i % 8;
      mask = 1 << bit;
      if (op->bit[i].value)
        cmd[j] = cmd[j] | mask;
      else
        cmd[j] = cmd[j] & ~mask;
    }
  }

  return 0;
}


/*
 * avr_set_addr()
 *
 * Set address bits in the specified command based on the opcode, and
 * the address.
 */
int avr_set_addr(OPCODE * op, unsigned char * cmd, unsigned long addr)
{
  int i, j, bit;
  unsigned long value;
  unsigned char mask;

  for (i=0; i<32; i++) {
    if (op->bit[i].type == AVR_CMDBIT_ADDRESS) {
      j = 3 - i / 8;
      bit = i % 8;
      mask = 1 << bit;
      value = addr >> op->bit[i].bitno & 0x01;
      if (value)
        cmd[j] = cmd[j] | mask;
      else
        cmd[j] = cmd[j] & ~mask;
    }
  }

  return 0;
}


/*
 * avr_set_input()
 *
 * Set input data bits in the specified command based on the opcode,
 * and the data byte.
 */
int avr_set_input(OPCODE * op, unsigned char * cmd, unsigned char data)
{
  int i, j, bit;
  unsigned char value;
  unsigned char mask;

  for (i=0; i<32; i++) {
    if (op->bit[i].type == AVR_CMDBIT_INPUT) {
      j = 3 - i / 8;
      bit = i % 8;
      mask = 1 << bit;
      value = data >> op->bit[i].bitno & 0x01;
      if (value)
        cmd[j] = cmd[j] | mask;
      else
        cmd[j] = cmd[j] & ~mask;
    }
  }

  return 0;
}


/*
 * avr_get_output()
 *
 * Retreive output data bits from the command results based on the
 * opcode data.
 */
int avr_get_output(OPCODE * op, unsigned char * res, unsigned char * data)
{
  int i, j, bit;
  unsigned char value;
  unsigned char mask;

  for (i=0; i<32; i++) {
    if (op->bit[i].type == AVR_CMDBIT_OUTPUT) {
      j = 3 - i / 8;
      bit = i % 8;
      mask = 1 << bit;
      value = ((res[j] & mask) >> bit) & 0x01;
      value = value << op->bit[i].bitno;
      if (value)
        *data = *data | value;
      else
        *data = *data & ~value;
    }
  }

  return 0;
}

/***
 *** Elementary functions dealing with AVRMEM structures
 ***/

AVRMEM * avr_new_memtype(void)
{
  AVRMEM * m;

  m = (AVRMEM *)MALLOC(sizeof(*m));
  if (m == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }

  memset(m, 0, sizeof(*m));

  return m;
}


AVRMEM * avr_dup_mem(AVRMEM * m)
{
  AVRMEM * n;

  n = avr_new_memtype();

  *n = *m;

  n->buf = (unsigned char *)MALLOC(n->size);
  if (n->buf == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  memset(n->buf, 0, n->size);

  memcpy(n->tags, m->tags, n->size);

  return n;
}

/*
 * Elementary functions dealing with AVRPART structures
 */


AVRPART * avr_new_part(void)
{
  AVRPART * p;

  p = (AVRPART *)MALLOC(sizeof(AVRPART));
  if (p == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }

  memset(p, 0, sizeof(*p));

  p->id[0]   = 0;
  p->desc[0] = 0;
  p->reset_disposition = RESET_DEDICATED;
  p->retry_pulse = PIN_AVR_SCK;
  p->flags = AVRPART_SERIALOK | AVRPART_PARALLELOK | AVRPART_ENABLEPAGEPROGRAMMING;
  p->lineno = 0;
  memset(p->signature, 0xFF, 3);
  p->ctl_stack_type = CTL_STACK_NONE;

  return p;
}


AVRPART * avr_dup_part(AVRPART * d)
{
  AVRPART * p;

  p = avr_new_part();
  *p = *d;

  /* NEW */
  p->sigmem = avr_dup_mem(p->sigmem);
  p->flashmem = avr_dup_mem(p->flashmem);
  p->lfusemem = avr_dup_mem(p->lfusemem);
  p->hfusemem = avr_dup_mem(p->hfusemem);

  return p;
}
