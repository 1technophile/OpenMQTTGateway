/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2000-2004  Brian S. Dean <bsd@bsdhome.com>
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

/* $Id: config.c 725 2007-01-30 13:41:54Z joerg_wunsch $ */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../pilight/core/log.h"
#include "../pilight/core/mem.h"
#include "avrconfig.h"

TOKEN * new_token(int primary)
{
  TOKEN * tkn;

  tkn = (TOKEN *)MALLOC(sizeof(TOKEN));
  if (tkn == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }

  memset(tkn, 0, sizeof(TOKEN));

  tkn->primary = primary;

  return tkn;
}

void free_token(TOKEN * tkn)
{
  if (tkn) {
    switch (tkn->primary) {
      case TKN_STRING:
      case TKN_ID:
        if (tkn->value.string)
          FREE(tkn->value.string);
        tkn->value.string = NULL;
        break;
    }

    FREE(tkn);
  }
}

TOKEN * number(char * text)
{
  struct token_t * tkn;

  tkn = new_token(TKN_NUMBER);
  tkn->value.type   = V_NUM;
  tkn->value.number = atof(text);

  return tkn;
}

TOKEN * hexnumber(char * text)
{
  struct token_t * tkn;
  char * e;

  tkn = new_token(TKN_NUMBER);
  tkn->value.type   = V_NUM;
  tkn->value.number = strtoul(text, &e, 16);
  if ((e == text) || (*e != 0)) {
    logprintf(LOG_ERR, "%s:%d: can't scan hex number \"%s\"", infile, lineno, text);
		return NULL;
  }

  return tkn;
}

TOKEN * string(char * text)
{
  struct token_t * tkn;
  int len;

  tkn = new_token(TKN_STRING);

  len = strlen(text);

  tkn->value.type   = V_STR;
  tkn->value.string = (char *) MALLOC(len+1);
  if (tkn->value.string == NULL) {
    fprintf(stderr, "out of memory\n");
		return NULL;
  }
  strcpy(tkn->value.string, text);

  return tkn;
}
