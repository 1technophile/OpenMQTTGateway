/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2000-2005  Brian S. Dean <bsd@bsdhome.com>
 * Copyright (C) 2007 Joerg Wunsch
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

/* $Id: AVRUPD.c 819 2009-04-28 18:35:14Z joerg_wunsch $ */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "../pilight/core/log.h"
#include "../pilight/core/mem.h"
#include "avrdude.h"
#include "avr.h"
#include "avrconfig.h"
#include "fileio.h"
#include "avrupd.h"

AVRUPD * parse_op(char * s)
{
  char buf[1024];
  char * p, * cp, c;
  AVRUPD * upd;
  int i;
  size_t fnlen;

  upd = (AVRUPD *)MALLOC(sizeof(AVRUPD));
  if (upd == NULL) {
		fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }

  i = 0;
  p = s;
  while ((i < (sizeof(buf)-1) && *p && (*p != ':')))
    buf[i++] = *p++;
  buf[i] = 0;

  if (*p != ':') {
    upd->memtype = (char *)MALLOC(strlen("flash")+1);
    if (upd->memtype == NULL) {
      outofmem:
			fprintf(stderr, "out of memory\n");
      exit(EXIT_FAILURE);
    }
    strcpy(upd->memtype, "flash");
    upd->op = DEVICE_WRITE;
    upd->filename = (char *)MALLOC(strlen(buf) + 1);
    if (upd->filename == NULL)
      goto outofmem;
    strcpy(upd->filename, buf);
    upd->format = FMT_AUTO;
    return upd;
  }

  upd->memtype = (char *)MALLOC(strlen(buf)+1);
  if (upd->memtype == NULL) {
		fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  strcpy(upd->memtype, buf);

  p++;
  if (*p == 'r') {
    upd->op = DEVICE_READ;
  }
  else if (*p == 'w') {
    upd->op = DEVICE_WRITE;
  }
  else if (*p == 'v') {
    upd->op = DEVICE_VERIFY;
  }
  else {
    logprintf(LOG_ERR, "invalid I/O mode '%c' in update specification", *p);
    logprintf(LOG_ERR,
            "  allowed values are:\n"
            "    r = read device\n"
            "    w = write device\n"
            "    v = verify device");
    FREE(upd->memtype);
    FREE(upd);
    return NULL;
  }

  p++;

  if (*p != ':') {
    logprintf(LOG_ERR, "invalid update specification");
    FREE(upd->memtype);
    FREE(upd);
    return NULL;
  }

  p++;

  /*
   * Now, parse the filename component.  Instead of looking for the
   * leftmost possible colon delimiter, we look for the rightmost one.
   * If we found one, we do have a trailing :format specifier, and
   * process it.  Otherwise, the remainder of the string is our file
   * name component.  That way, the file name itself is allowed to
   * contain a colon itself (e. g. C:/some/file.hex), except the
   * optional format specifier becomes mandatory then.
   */
  cp = p;
  p = strrchr(cp, ':');
  if (p == NULL) {
    upd->format = FMT_AUTO;
    fnlen = strlen(cp);
    upd->filename = (char *)MALLOC(fnlen + 1);
  } else {
    fnlen = p - cp;
    upd->filename = (char *)MALLOC(fnlen +1);
    c = *++p;
    if (c && p[1])
      /* More than one char - force failure below. */
      c = '?';
    switch (c) {
      case 'a': upd->format = FMT_AUTO; break;
      case 's': upd->format = FMT_SREC; break;
      case 'i': upd->format = FMT_IHEX; break;
      case 'r': upd->format = FMT_RBIN; break;
      case 'm': upd->format = FMT_IMM; break;
      case 'b': upd->format = FMT_BIN; break;
      case 'd': upd->format = FMT_DEC; break;
      case 'h': upd->format = FMT_HEX; break;
      case 'o': upd->format = FMT_OCT; break;
      default:
        logprintf(LOG_ERR, "invalid file format '%s' in update specifier", p);
        FREE(upd->memtype);
        FREE(upd);
        return NULL;
    }
  }

  if (upd->filename == NULL) {
		fprintf(stderr, "out of memory\n");
    FREE(upd->memtype);
    FREE(upd);
    return NULL;
  }
  memcpy(upd->filename, cp, fnlen);
  upd->filename[fnlen] = 0;

  return upd;
}

AVRUPD * dup_AVRUPD(AVRUPD * upd)
{
  AVRUPD * u;

  u = (AVRUPD *)MALLOC(sizeof(AVRUPD));
  if(u == NULL) {
		fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }

  memcpy(u, upd, sizeof(AVRUPD));

  if((u->memtype = MALLOC(strlen(upd->memtype)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
	strcpy(u->memtype, upd->memtype);
  if((u->filename = MALLOC(strlen(upd->filename)+1)) == NULL) {
		fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
	}
	strcpy(u->filename, upd->filename);

  return u;
}

AVRUPD * new_AVRUPD(int op, char * memtype, int filefmt, char * filename)
{
  AVRUPD * u;

  u = (AVRUPD *)MALLOC(sizeof(AVRUPD));
  if (u == NULL) {
		fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
	}

  u->memtype = MALLOC(strlen(memtype)+1);
	strcpy(u->memtype, memtype);
  u->filename = MALLOC(strlen(filename)+1);
	strcpy(u->filename, filename);
  u->op = op;
  u->format = filefmt;

  return u;
}

int do_op(PROGRAMMER * pgm, struct avrpart * p, AVRUPD * upd, int nowrite,
          int verify)
{
  struct avrpart * v;
  AVRMEM * mem = NULL;
  int size, vsize;
  int rc;

  /* NEW */
  if(strcmp(upd->memtype, "flash") == 0) {
		mem = p->flashmem;
  } else if(strcmp(upd->memtype, "lfuse") == 0) {
		mem = p->lfusemem;
  } else if(strcmp(upd->memtype, "hfuse") == 0) {
		mem = p->hfusemem;
  }

  if (mem == NULL) {
    logprintf(LOG_ERR, "\"%s\" memory type not defined for part \"%s\"", upd->memtype, p->desc);
    return -1;
  }

  if (upd->op == DEVICE_READ) {
    /*
     * read out the specified device memory and write it to a file
     */

    report_progress(0,1,"Reading");
    rc = avr_read(pgm, p, upd->memtype, 0);
    if (rc < 0) {
      logprintf(LOG_ERR, "failed to read all of %s memory, rc=%d", mem->desc, rc);
      return -1;
    }
    report_progress(1,1,NULL);
    size = rc;

      logprintf(LOG_INFO,
            "writing output file \"%s\"",
            strcmp(upd->filename, "-")==0 ? "<stdout>" : upd->filename);

    rc = fileio(FIO_WRITE, upd->filename, upd->format, p, upd->memtype, size);
    if (rc < 0) {
      logprintf(LOG_ERR, "write to file '%s' failed", upd->filename);
      return -1;
    }
  }
  else if (upd->op == DEVICE_WRITE) {
    /*
     * write the selected device memory using data from a file; first
     * read the data from the specified file
     */

 	 logprintf(LOG_INFO,
            "reading input file \"%s\"",
            strcmp(upd->filename, "-")==0 ? "<stdin>" : upd->filename);

    rc = fileio(FIO_READ, upd->filename, upd->format, p, upd->memtype, -1);
    if (rc < 0) {
      logprintf(LOG_INFO, "write to file '%s' failed", upd->filename);
      return -1;
    }
    size = rc;

    /*
     * write the buffer contents to the selected memory type
     */
      logprintf(LOG_INFO, "writing %s (%d bytes)", mem->desc, size);

    if (!nowrite) {
      report_progress(0,1,"Writing");
      rc = avr_write(pgm, p, upd->memtype, size);
      report_progress(1,1,NULL);
    }
    else {
      /*
       * test mode, don't actually write to the chip, output the buffer
       * to stdout in intel hex instead
       */
      rc = fileio(FIO_WRITE, "-", FMT_IHEX, p, upd->memtype, size);
    }

    if (rc < 0) {
      logprintf(LOG_ERR, "failed to write %s memory, rc=%d", mem->desc, rc);
      return -1;
    }

    vsize = rc;

	logprintf(LOG_INFO, "%d bytes of %s written", vsize, mem->desc);

  }
  else if (upd->op == DEVICE_VERIFY) {
    /*
     * verify that the in memory file (p->mem[AVR_M_FLASH|AVR_M_EEPROM])
     * is the same as what is on the chip
     */
    v = avr_dup_part(p);

      logprintf(LOG_INFO, "verifying %s memory against %s:", mem->desc, upd->filename);
      logprintf(LOG_INFO, "load data %s data from input file %s:", mem->desc, upd->filename);

    rc = fileio(FIO_READ, upd->filename, upd->format, p, upd->memtype, -1);
    if (rc < 0) {
      logprintf(LOG_ERR, "read from file '%s' failed", upd->filename);
      return -1;
    }
    size = rc;

      logprintf(LOG_INFO, "input file %s contains %d bytes", upd->filename, size);

    report_progress (0,1,"Reading");
    rc = avr_read(pgm, v, upd->memtype, size);
    if (rc < 0) {
      logprintf(LOG_ERR, "failed to read all of %s memory, rc=%d", mem->desc, rc);
      return -1;
    }
    report_progress (1,1,NULL);

      logprintf(LOG_INFO, "verifying ...");

    rc = avr_verify(p, v, upd->memtype, size);
    if (rc < 0) {
      logprintf(LOG_ERR, "verification error; content mismatch");
      return -1;
    }

      logprintf(LOG_INFO, "%d bytes of %s verified", rc, mem->desc);

	FREE(v->sigmem->buf);
	FREE(v->sigmem);
	FREE(v->flashmem->buf);
	FREE(v->flashmem);
	FREE(v->hfusemem->buf);
	FREE(v->hfusemem);
	FREE(v->lfusemem->buf);
	FREE(v->lfusemem);
	FREE(v);
  }
  else {
    logprintf(LOG_ERR, "invalid update operation (%d) requested", upd->op);
    return -1;
  }

  return 0;
}

