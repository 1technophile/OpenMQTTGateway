/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2002-2004  Brian S. Dean <bsd@bsdhome.com>
 * Copyright 2007 Joerg Wunsch <j@uriah.heep.sax.de>
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

/* $Id: pgm.h 797 2009-02-17 15:31:27Z joerg_wunsch $ */

#ifndef pgm_h
#define pgm_h

#include <limits.h>

#include "avrpart.h"
#include "serial.h"
#include "pindefs.h"

#define ON  1
#define OFF 0

#define PGM_DESCLEN 80
#define PGM_PORTLEN 4096
#define PGM_TYPELEN 32

typedef enum {
  EXIT_VCC_UNSPEC,
  EXIT_VCC_ENABLED,
  EXIT_VCC_DISABLED
} exit_vcc_t;

typedef enum {
  EXIT_RESET_UNSPEC,
  EXIT_RESET_ENABLED,
  EXIT_RESET_DISABLED
} exit_reset_t;

typedef struct programmer_t {
  char desc[PGM_DESCLEN];
  char type[PGM_TYPELEN];
  char port[PGM_PORTLEN];
  unsigned int pinno[N_PINS];
  exit_vcc_t exit_vcc;
  exit_reset_t exit_reset;
  int ppidata;
  int ppictrl;
  int baudrate;
  double bitclock;    /* JTAG ICE clock period in microseconds */
  int ispdelay;    /* ISP clock delay */
  union filedescriptor fd;
  int  page_size;  /* page size if the programmer supports paged write/load */
  int  (*rdy_led)        (struct programmer_t * pgm, int value);
  int  (*err_led)        (struct programmer_t * pgm, int value);
  int  (*pgm_led)        (struct programmer_t * pgm, int value);
  int  (*vfy_led)        (struct programmer_t * pgm, int value);
  int  (*initialize)     (struct programmer_t * pgm, AVRPART * p);
  void (*display)        (struct programmer_t * pgm, const char * p);
  void (*enable)         (struct programmer_t * pgm);
  void (*disable)        (struct programmer_t * pgm);
  void (*powerup)        (struct programmer_t * pgm);
  void (*powerdown)      (struct programmer_t * pgm);
  int  (*program_enable) (struct programmer_t * pgm, AVRPART * p);
  int  (*chip_erase)     (struct programmer_t * pgm, AVRPART * p);
  int  (*cmd)            (struct programmer_t * pgm, unsigned char cmd[4],
                          unsigned char res[4]);
  int  (*spi)            (struct programmer_t * pgm, unsigned char cmd[],
                          unsigned char res[], int count);
  int  (*open)           (struct programmer_t * pgm, char * port);
  void (*close)          (struct programmer_t * pgm);
  int  (*paged_write)    (struct programmer_t * pgm, AVRPART * p, AVRMEM * m,
                          unsigned int page_size, unsigned int baseaddr,
                          unsigned int n_bytes);
  int  (*paged_load)     (struct programmer_t * pgm, AVRPART * p, AVRMEM * m,
                          unsigned int page_size, unsigned int baseaddr,
                          unsigned int n_bytes);
  void (*write_setup)    (struct programmer_t * pgm, AVRPART * p, AVRMEM * m);
  int  (*write_byte)     (struct programmer_t * pgm, AVRPART * p, AVRMEM * m,
                          unsigned long addr, unsigned char value);
  int  (*read_byte)      (struct programmer_t * pgm, AVRPART * p, AVRMEM * m,
                          unsigned long addr, unsigned char * value);
  int  (*read_sig_bytes) (struct programmer_t * pgm, AVRPART * p, AVRMEM * m);
  void (*print_parms)    (struct programmer_t * pgm);
  int  (*set_vtarget)    (struct programmer_t * pgm, double v);
  int  (*set_varef)      (struct programmer_t * pgm, unsigned int chan, double v);
  int  (*set_fosc)       (struct programmer_t * pgm, double v);
  int  (*set_sck_period) (struct programmer_t * pgm, double v);
  int  (*setpin)         (struct programmer_t * pgm, int pin, int value);
  int  (*getpin)         (struct programmer_t * pgm, int pin);
  int  (*highpulsepin)   (struct programmer_t * pgm, int pin);
  int  (*parseexitspecs) (struct programmer_t * pgm, char *s);
  int  (*perform_osccal) (struct programmer_t * pgm);
  void (*setup)          (struct programmer_t * pgm);
  void (*teardown)       (struct programmer_t * pgm);
  char config_file[4096]; /* config file where defined */
  int  lineno;                /* config file line number */
  void *cookie;		      /* for private use by the programmer */
  char flag;		      /* for private use of the programmer */
} PROGRAMMER;

#ifdef __cplusplus
extern "C" {
#endif

PROGRAMMER * pgm_new(void);

void programmer_display(PROGRAMMER * pgm, const char * p);

typedef void (*walk_programmers_cb)(const char *name, const char *desc,
                                    const char *cfgname, int cfglineno,
                                    void *cookie);

#ifdef __cplusplus
}
#endif

#endif
