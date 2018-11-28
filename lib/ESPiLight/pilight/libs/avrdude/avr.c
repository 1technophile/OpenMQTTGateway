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

/* $Id: avr.c 907 2010-01-12 15:42:40Z joerg_wunsch $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "avrdude.h"

#include "../pilight/core/log.h"
#include "../pilight/core/common.h"
#include "avr.h"
#include "pindefs.h"
#include "safemode.h"

FP_UpdateProgress avr_update_progress;

void avr_set_update_progress(FP_UpdateProgress callback) {
	avr_update_progress = callback;
}

int parse_cmdbits(OPCODE * op, char *t) {
	int bitno;
	unsigned int n = 0, i = 0;
	char ch;
	char *e = NULL;
	char *q = NULL;
	int len;
	char **array = NULL;

	bitno = 32;
	n = explode(t, " ", &array);
	for(i=0;i<n;i++) {
		bitno--;
		len = strlen(array[i]);
		ch = array[i][0];

		if(len == 1) {
			switch(ch) {
				case '1':
					op->bit[bitno].type  = AVR_CMDBIT_VALUE;
					op->bit[bitno].value = 1;
					op->bit[bitno].bitno = bitno % 8;
				break;
				case '0':
					op->bit[bitno].type  = AVR_CMDBIT_VALUE;
					op->bit[bitno].value = 0;
					op->bit[bitno].bitno = bitno % 8;
				break;
				case 'x':
					op->bit[bitno].type  = AVR_CMDBIT_IGNORE;
					op->bit[bitno].value = 0;
					op->bit[bitno].bitno = bitno % 8;
				break;
				case 'a':
					op->bit[bitno].type  = AVR_CMDBIT_ADDRESS;
					op->bit[bitno].value = 0;
					op->bit[bitno].bitno = 8*(bitno/8) + bitno % 8;
				break;
				case 'i':
					op->bit[bitno].type  = AVR_CMDBIT_INPUT;
					op->bit[bitno].value = 0;
					op->bit[bitno].bitno = bitno % 8;
				break;
				case 'o':
					op->bit[bitno].type  = AVR_CMDBIT_OUTPUT;
					op->bit[bitno].value = 0;
					op->bit[bitno].bitno = bitno % 8;
				break;
				default :
				break;
			}
		} else {
			if(ch == 'a') {
				q = &array[i][1];
				op->bit[bitno].bitno = strtol(q, &e, 0);
				op->bit[bitno].type = AVR_CMDBIT_ADDRESS;
				op->bit[bitno].value = 0;
			}
		}
	}
	array_free(&array, n);
	return 0;
}

int avr_read_byte_default(PROGRAMMER * pgm, AVRPART * p, AVRMEM * mem,
                          unsigned long addr, unsigned char * value)
{
  unsigned char cmd[4];
  unsigned char res[4];
  unsigned char data;
  OPCODE * readop, * lext;

  if (pgm->cmd == NULL) {
    logprintf(LOG_ERR, "%s programmer uses avr_read_byte_default() but does not provide a cmd() method", pgm->type);
    return -1;
  }

  /*
   * figure out what opcode to use
   */
  if (mem->op[AVR_OP_READ_LO]) {
    if (addr & 0x00000001)
      readop = mem->op[AVR_OP_READ_HI];
    else
      readop = mem->op[AVR_OP_READ_LO];
    addr = addr / 2;
  }
  else {
    readop = mem->op[AVR_OP_READ];
  }

  if (readop == NULL) {
    return -1;
  }

  /*
   * If this device has a "load extended address" command, issue it.
   */
  lext = mem->op[AVR_OP_LOAD_EXT_ADDR];

  if (lext != NULL) {
    memset(cmd, 0, sizeof(cmd));

    avr_set_bits(lext, cmd);
    avr_set_addr(lext, cmd, addr);
    pgm->cmd(pgm, cmd, res);
  }

  memset(cmd, 0, sizeof(cmd));

  avr_set_bits(readop, cmd);
  avr_set_addr(readop, cmd, addr);
  pgm->cmd(pgm, cmd, res);
  data = 0;
  avr_get_output(readop, res, &data);

  *value = data;

  return 0;
}


/*
 * Return the number of "interesting" bytes in a memory buffer,
 * "interesting" being defined as up to the last non-0xff data
 * value. This is useful for determining where to stop when dealing
 * with "flash" memory, since writing 0xff to flash is typically a
 * no-op. Always return an even number since flash is word addressed.
 */
int avr_mem_hiaddr(AVRMEM * mem)
{
  int i, n;

  /* return the highest non-0xff address regardless of how much
     memory was read */
  for (i=mem->size-1; i>0; i--) {
    if (mem->buf[i] != 0xff) {
      n = i+1;
      if (n & 0x01)
        return n+1;
      else
        return n;
    }
  }

  return 0;
}


/*
 * Read the entirety of the specified memory type into the
 * corresponding buffer of the avrpart pointed to by 'p'.  If size =
 * 0, read the entire contents, otherwise, read 'size' bytes.
 *
 * Return the number of bytes read, or < 0 if an error occurs.
 */
int avr_read(PROGRAMMER * pgm, AVRPART * p, char * memtype, int size)
{
  int              wsize;
  unsigned char    rbyte;
  unsigned long    i;
  unsigned char  * buf;
  AVRMEM * mem = NULL;
  int rc;

  /* NEW */
  if(strcmp(memtype, "signature") == 0) {
		mem = p->sigmem;
  }	else if(strcmp(memtype, "flash") == 0) {
		mem = p->flashmem;
  } else if(strcmp(memtype, "lfuse") == 0) {
		mem = p->lfusemem;
  } else if(strcmp(memtype, "hfuse") == 0) {
		mem = p->hfusemem;
  }
  // mem = avr_locate_mem(p, memtype);
  if (mem == NULL) {
    logprintf(LOG_ERR, "No \"%s\" memory for part %s", memtype, p->desc);
    return -1;
  }

	wsize = mem->size;
  if (size < wsize) {
    wsize = size;
  }

  buf  = mem->buf;
  if (size == 0) {
    size = mem->size;
  }

  /*
   * start with all 0xff
   */
  memset(buf, 0xff, size);

  if (pgm->paged_load != NULL && mem->page_size != 0) {
     /*
     * the programmer supports a paged mode read
     */
    int need_read, failure;
    unsigned int pageaddr;
    unsigned int npages, nread;

    /* quickly scan number of pages to be written to first */
    for (pageaddr = 0, npages = 0;
         pageaddr < wsize;
         pageaddr += mem->page_size) {
      /* check whether this page must be read */
      for (i = pageaddr;
           i < pageaddr + mem->page_size;
           i++)
        if (i < mem->size) {
          npages++;
          break;
        }
    }

    for (pageaddr = 0, failure = 0, nread = 0;
         !failure && pageaddr < wsize;
         pageaddr += mem->page_size) {
      /* check whether this page must be read */
      for (i = pageaddr, need_read = 0;
           i < pageaddr + mem->page_size;
           i++)
        if (i < mem->size) {
          need_read = 1;
          break;
        }
      if (need_read) {
        rc = pgm->paged_load(pgm, p, mem, mem->page_size,
                            pageaddr, mem->page_size);
        if (rc < 0)
          /* paged load failed, fall back to byte-at-a-time read below */
          failure = 1;
      }
      nread++;
      report_progress(nread, npages, NULL);
    }
    if (!failure) {
      if (strcasecmp(mem->desc, "flash") == 0 ||
          strcasecmp(mem->desc, "application") == 0 ||
          strcasecmp(mem->desc, "apptable") == 0 ||
          strcasecmp(mem->desc, "boot") == 0)
        return avr_mem_hiaddr(mem);
      else
        return mem->size;
    }
  }

  if (strcmp(mem->desc, "signature") == 0) {
    if (pgm->read_sig_bytes) {
      return pgm->read_sig_bytes(pgm, p, mem);
    }
  }

  for (i=0; i<size; i++) {
    rc = pgm->read_byte(pgm, p, mem, i, &rbyte);
    if (rc != 0) {
      logprintf(LOG_ERR, "avr_read(): error reading address 0x%04lx", i);
      if (rc == -1)
        logprintf(LOG_ERR, "read operation not supported for memory \"%s\"", memtype);
      return -2;
    }
    buf[i] = rbyte;
    report_progress(i, size, NULL);
  }

  if (strcasecmp(mem->desc, "flash") == 0)
    return avr_mem_hiaddr(mem);
  else
    return i;
}


/*
 * write a page data at the specified address
 */
int avr_write_page(PROGRAMMER * pgm, AVRPART * p, AVRMEM * mem,
                   unsigned long addr)
{
  unsigned char cmd[4];
  unsigned char res[4];
  OPCODE * wp, * lext;

  if (pgm->cmd == NULL) {
    logprintf(LOG_ERR, "%s programmer uses avr_write_page() but does not provide a cmd() method", pgm->type);
    return -1;
  }

  wp = mem->op[AVR_OP_WRITEPAGE];
  if (wp == NULL) {
    logprintf(LOG_ERR, "avr_write_page(): memory \"%s\" not configured for page writes", mem->desc);
    return -1;
  }

  /*
   * if this memory is word-addressable, adjust the address
   * accordingly
   */
  if ((mem->op[AVR_OP_LOADPAGE_LO]) || (mem->op[AVR_OP_READ_LO]))
    addr = addr / 2;

  /*
   * If this device has a "load extended address" command, issue it.
   */
  lext = mem->op[AVR_OP_LOAD_EXT_ADDR];
  if (lext != NULL) {
    memset(cmd, 0, sizeof(cmd));

    avr_set_bits(lext, cmd);
    avr_set_addr(lext, cmd, addr);
    pgm->cmd(pgm, cmd, res);
  }

  memset(cmd, 0, sizeof(cmd));

  avr_set_bits(wp, cmd);
  avr_set_addr(wp, cmd, addr);
  pgm->cmd(pgm, cmd, res);

  /*
   * since we don't know what voltage the target AVR is powered by, be
   * conservative and delay the max amount the spec says to wait
   */
  usleep(mem->max_write_delay);

  return 0;
}


int avr_write_byte_default(PROGRAMMER * pgm, AVRPART * p, AVRMEM * mem,
                   unsigned long addr, unsigned char data)
{
  unsigned char cmd[4];
  unsigned char res[4];
  unsigned char r;
  int ready;
  int tries;
  unsigned long start_time;
  unsigned long prog_time;
  unsigned char b;
  unsigned short caddr;
  OPCODE * writeop;
  int rc;
  int readok=0;
  struct timeval tv;

  if (pgm->cmd == NULL) {
    logprintf(LOG_ERR, "%s programmer uses avr_write_byte_default() but does not provide a cmd() method.", pgm->type);
    return -1;
  }

  if (!mem->paged) {
    /*
     * check to see if the write is necessary by reading the existing
     * value and only write if we are changing the value; we can't
     * use this optimization for paged addressing.
     */
    rc = pgm->read_byte(pgm, p, mem, addr, &b);
    if (rc != 0) {
      if (rc != -1) {
        return -2;
      }
      /*
       * the read operation is not support on this memory type
       */
    }
    else {
      readok = 1;
      if (b == data) {
        return 0;
      }
    }
  }

  /*
   * determine which memory opcode to use
   */
  if (mem->op[AVR_OP_WRITE_LO]) {
    if (addr & 0x01)
      writeop = mem->op[AVR_OP_WRITE_HI];
    else
      writeop = mem->op[AVR_OP_WRITE_LO];
    caddr = addr / 2;
  }
  else if (mem->paged && mem->op[AVR_OP_LOADPAGE_LO]) {
    if (addr & 0x01)
      writeop = mem->op[AVR_OP_LOADPAGE_HI];
    else
      writeop = mem->op[AVR_OP_LOADPAGE_LO];
    caddr = addr / 2;
  }
  else {
    writeop = mem->op[AVR_OP_WRITE];
    caddr = addr;
  }

  if (writeop == NULL) {
    return -1;
  }

  memset(cmd, 0, sizeof(cmd));

  avr_set_bits(writeop, cmd);
  avr_set_addr(writeop, cmd, caddr);
  avr_set_input(writeop, cmd, data);
  pgm->cmd(pgm, cmd, res);

  if (mem->paged) {
    /*
     * in paged addressing, single bytes to be written to the memory
     * page complete immediately, we only need to delay when we commit
     * the whole page via the avr_write_page() routine.
     */
    return 0;
  }

  if (readok == 0) {
    /*
     * read operation not supported for this memory type, just wait
     * the max programming time and then return
     */
    usleep(mem->max_write_delay); /* maximum write delay */
    return 0;
  }

  tries = 0;
  ready = 0;
  while (!ready) {

    if ((data == mem->readback[0]) ||
        (data == mem->readback[1])) {
      /*
       * use an extra long delay when we happen to be writing values
       * used for polled data read-back.  In this case, polling
       * doesn't work, and we need to delay the worst case write time
       * specified for the chip.
       */
      usleep(mem->max_write_delay);
      rc = pgm->read_byte(pgm, p, mem, addr, &r);
      if (rc != 0) {
        return -5;
      }
    }
    else {
      gettimeofday (&tv, NULL);
      start_time = (tv.tv_sec * 1000000) + tv.tv_usec;
      do {
        /*
         * Do polling, but timeout after max_write_delay.
	 */
        rc = pgm->read_byte(pgm, p, mem, addr, &r);
        if (rc != 0) {
          return -4;
        }
        gettimeofday (&tv, NULL);
        prog_time = (tv.tv_sec * 1000000) + tv.tv_usec;
      } while ((r != data) &&
               ((prog_time-start_time) < mem->max_write_delay));
    }

    /*
     * At this point we either have a valid readback or the
     * max_write_delay is expired.
     */

    if (r == data) {
      ready = 1;
    }
    else if (mem->pwroff_after_write) {
      /*
       * The device has been flagged as power-off after write to this
       * memory type.  The reason we don't just blindly follow the
       * flag is that the power-off advice may only apply to some
       * memory bits but not all.  We only actually power-off the
       * device if the data read back does not match what we wrote.
       */
      logprintf(LOG_NOTICE, "this device must be powered off and back on to continue");
      if (pgm->pinno[PPI_AVR_VCC]) {
        logprintf(LOG_NOTICE, "attempting to do this now ...");
        pgm->powerdown(pgm);
        usleep(250000);
        rc = pgm->initialize(pgm, p);
        if (rc < 0) {
          logprintf(LOG_ERR, "initialization failed, rc=%d", rc);
          logprintf(LOG_ERR, "can't re-initialize device after programming the %s bits", mem->desc);
          return -3;
        }

        logprintf(LOG_NOTICE, "device was successfully re-initialized");
        return 0;
      }
    }

    tries++;
    if (!ready && tries > 5) {
      /*
       * we wrote the data, but after waiting for what should have
       * been plenty of time, the memory cell still doesn't match what
       * we wrote.  Indicate a write error.
       */
      return -6;
    }
  }

  return 0;
}


/*
 * write a byte of data at the specified address
 */
int avr_write_byte(PROGRAMMER * pgm, AVRPART * p, AVRMEM * mem,
                   unsigned long addr, unsigned char data)
{

  unsigned char safemode_lfuse;
  unsigned char safemode_hfuse;
  unsigned char safemode_efuse;
  unsigned char safemode_fuse;

  /* If we write the fuses, then we need to tell safemode that they *should* change */
  safemode_memfuses(0, &safemode_lfuse, &safemode_hfuse, &safemode_efuse, &safemode_fuse);

  if (strcmp(mem->desc, "fuse")==0) {
      safemode_fuse = data;
  }
  if (strcmp(mem->desc, "lfuse")==0) {
      safemode_lfuse = data;
  }
  if (strcmp(mem->desc, "hfuse")==0) {
      safemode_hfuse = data;
  }
  if (strcmp(mem->desc, "efuse")==0) {
      safemode_efuse = data;
  }

  safemode_memfuses(1, &safemode_lfuse, &safemode_hfuse, &safemode_efuse, &safemode_fuse);

  return pgm->write_byte(pgm, p, mem, addr, data);
}


/*
 * Write the whole memory region of the specified memory from the
 * corresponding buffer of the avrpart pointed to by 'p'.  Write up to
 * 'size' bytes from the buffer.  Data is only written if the new data
 * value is different from the existing data value.  Data beyond
 * 'size' bytes is not affected.
 *
 * Return the number of bytes written, or -1 if an error occurs.
 */
int avr_write(PROGRAMMER * pgm, AVRPART * p, char * memtype, int size)
{
  int              rc;
  int              wsize;
  long             i;
  unsigned char    data;
  AVRMEM         * m = NULL;

  /* NEW */
  if(strcmp(memtype, "flash") == 0) {
		m = p->flashmem;
  } else if(strcmp(memtype, "lfuse") == 0) {
		m = p->lfusemem;
  } else if(strcmp(memtype, "hfuse") == 0) {
		m = p->hfusemem;
  }

  if (m == NULL) {
    logprintf(LOG_ERR, "No \"%s\" memory for part %s", memtype, p->desc);
    return -1;
  }

  wsize = m->size;
  if (size < wsize) {
    wsize = size;
  }
  else if (size > wsize) {
    logprintf(LOG_ERR, "%d bytes requested, but memory region is only %d bytes", size, wsize);
		logprintf(LOG_ERR, "Only %d bytes will actually be written", wsize);
  }

 if (pgm->paged_write != NULL && m->page_size != 0) {
    /*
     * the programmer supports a paged mode write
     */
    int need_write, failure;
    unsigned int pageaddr;
    unsigned int npages, nwritten;

    /* quickly scan number of pages to be written to first */
    for (pageaddr = 0, npages = 0;
         pageaddr < wsize;
         pageaddr += m->page_size) {
      /* check whether this page must be written to */
      for (i = pageaddr;
           i < pageaddr + m->page_size;
           i++)
        if (i < m->size) {
          npages++;
          break;
        }
    }

    for (pageaddr = 0, failure = 0, nwritten = 0;
         !failure && pageaddr < wsize;
         pageaddr += m->page_size) {
      /* check whether this page must be written to */
      for (i = pageaddr, need_write = 0;
           i < pageaddr + m->page_size;
           i++)
        if (i < m->size) {
          need_write = 1;
          break;
        }
      if (need_write) {
        rc = 0;
        if (rc >= 0)
          rc = pgm->paged_write(pgm, p, m, m->page_size, pageaddr, m->page_size);
        if (rc < 0)
          /* paged write failed, fall back to byte-at-a-time write below */
          failure = 1;
      }
      nwritten++;
      report_progress(nwritten, npages, NULL);
    }
    if (!failure)
      return wsize;
    /* else: fall back to byte-at-a-time write, for historical reasons */
  }

  if (pgm->write_setup) {
      pgm->write_setup(pgm, p, m);
  }

  for (i=0; i<wsize; i++) {
    data = m->buf[i];
    report_progress(i, wsize, NULL);

    rc = avr_write_byte(pgm, p, m, i, data);
    if (rc) {
      logprintf(LOG_ERR, "***failed");
    }

    if (m->paged) {
      /*
       * check to see if it is time to flush the page with a page
       * write
       */
      if (((i % m->page_size) == m->page_size-1) ||
          (i == wsize-1)) {
        rc = avr_write_page(pgm, p, m, i);
        if (rc) {
          logprintf(LOG_ERR, " *** page %ld (addresses 0x%04lx - 0x%04lx) failed to write",
					i % m->page_size,
					i - m->page_size + 1, i);
        }
      }
    }
  }

  return i;
}



/*
 * read the AVR device's signature bytes
 */
int avr_signature(PROGRAMMER * pgm, AVRPART * p)
{
  int rc;

  report_progress (0,1,"Reading");
  rc = avr_read(pgm, p, "signature", 0);
  if (rc < 0) {
    logprintf(LOG_ERR, "error reading signature data for part \"%s\", rc=%d", p->desc, rc);
    return -1;
  }
  report_progress (1,1,NULL);

  return 0;
}


/*
 * Verify the memory buffer of p with that of v.  The byte range of v,
 * may be a subset of p.  The byte range of p should cover the whole
 * chip's memory size.
 *
 * Return the number of bytes verified, or -1 if they don't match.
 */
int avr_verify(AVRPART * p, AVRPART * v, char * memtype, int size)
{
  int i;
  unsigned char * buf1, * buf2;
  int vsize;
  AVRMEM * a = NULL, * b = NULL;

  /* NEW */
  if(strcmp(memtype, "flash") == 0) {
	a = p->flashmem;
  } else if(strcmp(memtype, "lfuse") == 0) {
	a = p->lfusemem;
  } else if(strcmp(memtype, "hfuse") == 0) {
	a = p->hfusemem;
  }
  if (a == NULL) {
    logprintf(LOG_ERR, "No \"%s\" memory for part %s", memtype, p->desc);
    return -1;
  }

  /* NEW */
  if(strcmp(memtype, "flash") == 0) {
	b = v->flashmem;
  } else if(strcmp(memtype, "lfuse") == 0) {
	b = v->lfusemem;
  } else if(strcmp(memtype, "hfuse") == 0) {
	b = v->hfusemem;
  }
  if (b == NULL) {
    logprintf(LOG_ERR, "No \"%s\" memory for part %s", memtype, p->desc);
    return -1;
  }

  buf1  = a->buf;
  buf2  = b->buf;
  vsize = a->size;

  if (vsize < size) {
    logprintf(LOG_ERR, "requested verification for %d bytes", size);
		logprintf(LOG_ERR, "%s memory region only contains %d bytes", memtype, vsize);
		logprintf(LOG_ERR, "Only %d bytes will be verified.", vsize);
    size = vsize;
  }

  for (i=0; i<size; i++) {
    if (buf1[i] != buf2[i]) {
      logprintf(LOG_ERR, "verification error, first mismatch at byte 0x%04x", i);
			logprintf(LOG_ERR, "0x%02x != 0x%02x", buf1[i], buf2[i]);
      return -1;
    }
  }

  return size;
}


int avr_get_cycle_count(PROGRAMMER * pgm, AVRPART * p, int * cycles)
{
  AVRMEM * a = NULL;
  unsigned int cycle_count = 0;
  unsigned char v1;
  int rc;
  int i;

  // a = avr_locate_mem(p, "eeprom");
  if (a == NULL) {
    return -1;
  }

  for (i=4; i>0; i--) {
    rc = pgm->read_byte(pgm, p, a, a->size-i, &v1);
  if (rc < 0) {
    logprintf(LOG_ERR, "can't read memory for cycle count, rc=%d", rc);
    return -1;
  }
    cycle_count = (cycle_count << 8) | v1;
  }

   /*
   * If the EEPROM is erased, the cycle count reads 0xffffffff.
   * In this case we return a cycle_count of zero.
   * So, the calling function don't have to care about whether or not
   * the cycle count was initialized.
   */
  if (cycle_count == 0xffffffff) {
    cycle_count = 0;
  }

  *cycles = (int) cycle_count;

  return 0;
}


int avr_put_cycle_count(PROGRAMMER * pgm, AVRPART * p, int cycles)
{
  AVRMEM * a = NULL;
  unsigned char v1;
  int rc;
  int i;

  // a = avr_locate_mem(p, "eeprom");
  if (a == NULL) {
    return -1;
  }

  for (i=1; i<=4; i++) {
    v1 = cycles & 0xff;
    cycles = cycles >> 8;

    rc = avr_write_byte(pgm, p, a, a->size-i, v1);
    if (rc < 0) {
      logprintf(LOG_ERR, "can't write memory for cycle count, rc=%d", rc);
      return -1;
    }
  }

  return 0;
  }

int avr_chip_erase(PROGRAMMER * pgm, AVRPART * p)
{
  int rc;

  rc = pgm->chip_erase(pgm, p);

  return rc;
}

/*
 * Report the progress of a read or write operation from/to the
 * device.
 *
 * The first call of report_progress() should look like this (for a write op):
 *
 * report_progress (0, 1, "Writing");
 *
 * Then hdr should be passed NULL on subsequent calls while the
 * operation is progressing. Once the operation is complete, a final
 * call should be made as such to ensure proper termination of the
 * progress report:
 *
 * report_progress (1, 1, NULL);
 *
 * It would be nice if we could reduce the usage to one and only one
 * call for each of start, during and end cases. As things stand now,
 * that is not possible and makes maintenance a bit more work.
 */
void report_progress (int completed, int total, char *hdr)
{
  static int last = 0;
  static double start_time;
  int percent = (completed * 100) / total;
  struct timeval tv;
  double t;

  if (avr_update_progress == NULL)
    return;

  gettimeofday(&tv, NULL);
  t = tv.tv_sec + ((double)tv.tv_usec)/1000000;

  if (hdr) {
    last = 0;
    start_time = t;
    avr_update_progress (percent, t - start_time, hdr);
  }

  if (percent > 100)
    percent = 100;

  if (percent > last) {
    last = percent;
    avr_update_progress (percent, t - start_time, hdr);
  }

  if (percent == 100)
    last = 0;                   /* Get ready for next time. */
}
