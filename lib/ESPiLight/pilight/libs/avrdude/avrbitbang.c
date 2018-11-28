/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2000, 2001, 2002, 2003  Brian S. Dean <bsd@bsdhome.com>
 * Copyright (C) 2005 Michael Holzt <kju-avr@fqdn.org>
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
/* $Id: bitbang.c 898 2010-01-08 20:02:35Z joerg_wunsch $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/time.h>

#include "../pilight/core/log.h"
#include "../pilight/core/mem.h"
#include "avrdude.h"
#include "avr.h"
#include "pindefs.h"
#include "pgm.h"

#ifdef _WIN32
struct itimerval {
	struct timeval it_value, it_interval;
};
#define SIGALRM 14
#define ITIMER_REAL 0
#endif

static int delay_decrement;
static volatile int done;

typedef void (*mysighandler_t)(int);
static mysighandler_t saved_alarmhandler;

static void alarmhandler(int signo)
{
  done = 1;
  signal(SIGALRM, saved_alarmhandler);
}

/*
 * Calibrate the microsecond delay loop below.
 */
static void bitbang_calibrate_delay(void)
{
  struct itimerval itv;
  volatile int i;

  i = 0;
  done = 0;
  saved_alarmhandler = signal(SIGALRM, alarmhandler);
  /*
   * Set ITIMER_REAL to 100 ms.  All known systems have a timer
   * granularity of 10 ms or better, so counting the delay cycles
   * accumulating over 100 ms should give us a rather realistic
   * picture, without annoying the user by a lengthy startup time (as
   * an alarm(1) would do).  Of course, if heavy system activity
   * happens just during calibration but stops before the remaining
   * part of AVRDUDE runs, this will yield wrong values.  There's not
   * much we can do about this.
   */
  itv.it_value.tv_sec = 0;
  itv.it_value.tv_usec = 100000;
  itv.it_interval.tv_sec = itv.it_interval.tv_usec = 0;
#ifndef _WIN32
  setitimer(ITIMER_REAL, &itv, 0);
#endif
  while (!done)
    i--;
  itv.it_value.tv_sec = itv.it_value.tv_usec = 0;
#ifndef _WIN32
  setitimer(ITIMER_REAL, &itv, 0);
#endif
  /*
   * Calculate back from 100 ms to 1 us.
   */
  delay_decrement = -i / 100000;
}

/*
 * Delay for approximately the number of microseconds specified.
 * usleep()'s granularity is usually like 1 ms or 10 ms, so it's not
 * really suitable for short delays in bit-bang algorithms.
 */
void bitbang_delay(int us)
{
  volatile int del = us * delay_decrement;

  while (del > 0)
    del--;
}

/*
 * transmit and receive a byte of data to/from the AVR device
 */
static unsigned char bitbang_txrx(PROGRAMMER * pgm, unsigned char byte)
{
  int i;
  unsigned char r, b, rbyte;

  rbyte = 0;
  for (i=7; i>=0; i--) {
    /*
     * Write and read one bit on SPI.
     * Some notes on timing: Let T be the time it takes to do
     * one pgm->setpin()-call resp. par clrpin()-call, then
     * - SCK is high for 2T
     * - SCK is low for 2T
     * - MOSI setuptime is 1T
     * - MOSI holdtime is 3T
     * - SCK low to MISO read is 2T to 3T
     * So we are within programming specs (expect for AT90S1200),
     * if and only if T>t_CLCL (t_CLCL=clock period of target system).
     *
     * Due to the delay introduced by "IN" and "OUT"-commands,
     * T is greater than 1us (more like 2us) on x86-architectures.
     * So programming works safely down to 1MHz target clock.
    */

    b = (byte >> i) & 0x01;

    /* set the data input line as desired */
    pgm->setpin(pgm, pgm->pinno[PIN_AVR_MOSI], b);

    pgm->setpin(pgm, pgm->pinno[PIN_AVR_SCK], 1);

    /*
     * read the result bit (it is either valid from a previous falling
     * edge or it is ignored in the current context)
     */
    r = pgm->getpin(pgm, pgm->pinno[PIN_AVR_MISO]);

    pgm->setpin(pgm, pgm->pinno[PIN_AVR_SCK], 0);

    rbyte |= r << i;
  }

  return rbyte;
}


int bitbang_rdy_led(PROGRAMMER * pgm, int value)
{
  // pgm->setpin(pgm, pgm->pinno[PIN_LED_RDY], !value);
  return 0;
}

int bitbang_err_led(PROGRAMMER * pgm, int value)
{
  // pgm->setpin(pgm, pgm->pinno[PIN_LED_ERR], !value);
  return 0;
}

int bitbang_pgm_led(PROGRAMMER * pgm, int value)
{
  // pgm->setpin(pgm, pgm->pinno[PIN_LED_PGM], !value);
  return 0;
}

int bitbang_vfy_led(PROGRAMMER * pgm, int value)
{
  // pgm->setpin(pgm, pgm->pinno[PIN_LED_VFY], !value);
  return 0;
}


/*
 * transmit an AVR device command and return the results; 'cmd' and
 * 'res' must point to at least a 4 byte data buffer
 */
int bitbang_cmd(PROGRAMMER * pgm, unsigned char cmd[4],
                   unsigned char res[4])
{
  int i;

  for (i=0; i<4; i++) {
    res[i] = bitbang_txrx(pgm, cmd[i]);
  }

  return 0;
}

/*
 * transmit bytes via SPI and return the results; 'cmd' and
 * 'res' must point to data buffers
 */
int bitbang_spi(PROGRAMMER * pgm, unsigned char cmd[],
                   unsigned char res[], int count)
{
  int i;

  pgm->setpin(pgm, pgm->pinno[PIN_LED_PGM], 0);

  for (i=0; i<count; i++) {
    res[i] = bitbang_txrx(pgm, cmd[i]);
  }

  pgm->setpin(pgm, pgm->pinno[PIN_LED_PGM], 1);

  return 0;
}


/*
 * issue the 'chip erase' command to the AVR device
 */
int bitbang_chip_erase(PROGRAMMER * pgm, AVRPART * p)
{
  unsigned char cmd[4];
  unsigned char res[4];

  if (p->op[AVR_OP_CHIP_ERASE] == NULL) {
    logprintf(LOG_ERR, "chip erase instruction not defined for part \"%s\"", p->desc);
    return -1;
  }

  pgm->pgm_led(pgm, ON);

  memset(cmd, 0, sizeof(cmd));

  avr_set_bits(p->op[AVR_OP_CHIP_ERASE], cmd);
  pgm->cmd(pgm, cmd, res);
  usleep(p->chip_erase_delay);
  pgm->initialize(pgm, p);

  pgm->pgm_led(pgm, OFF);

  return 0;
}

/*
 * issue the 'program enable' command to the AVR device
 */
int bitbang_program_enable(PROGRAMMER * pgm, AVRPART * p)
{
  unsigned char cmd[4];
  unsigned char res[4];

  if (p->op[AVR_OP_PGM_ENABLE] == NULL) {
    logprintf(LOG_ERR, "program enable instruction not defined for part \"%s\"", p->desc);
    return -1;
  }

  memset(cmd, 0, sizeof(cmd));
  avr_set_bits(p->op[AVR_OP_PGM_ENABLE], cmd);
  pgm->cmd(pgm, cmd, res);

  if (res[2] != cmd[1])
    return -2;

  return 0;
}

/*
 * initialize the AVR device and prepare it to accept commands
 */
int bitbang_initialize(PROGRAMMER * pgm, AVRPART * p)
{
  int rc;
  int tries;

  bitbang_calibrate_delay();

  pgm->powerup(pgm);
  usleep(20000);

  pgm->setpin(pgm, pgm->pinno[PIN_AVR_SCK], 0);
  pgm->setpin(pgm, pgm->pinno[PIN_AVR_RESET], 0);
  usleep(20000);

  pgm->highpulsepin(pgm, pgm->pinno[PIN_AVR_RESET]);

  usleep(20000); /* 20 ms XXX should be a per-chip parameter */

  /*
   * Enable programming mode.  If we are programming an AT90S1200, we
   * can only issue the command and hope it worked.  If we are using
   * one of the other chips, the chip will echo 0x53 when issuing the
   * third byte of the command.  In this case, try up to 32 times in
   * order to possibly get back into sync with the chip if we are out
   * of sync.
   */

  if (strcmp(p->desc, "AT90S1200")==0) {
    pgm->program_enable(pgm, p);
  }
  else {
    tries = 0;
    do {
      rc = pgm->program_enable(pgm, p);
      if ((rc == 0)||(rc == -1))
        break;
      pgm->highpulsepin(pgm, pgm->pinno[p->retry_pulse/*PIN_AVR_SCK*/]);
      tries++;
    } while (tries < 65);

    /*
     * can't sync with the device, maybe it's not attached?
     */
    if (rc) {
      logprintf(LOG_ERR, "AVR device not responding");
      return -1;
    }
  }

  return 0;
}

static void verify_pin_assigned(PROGRAMMER * pgm, int pin, char * desc)
{
}


/*
 * Verify all prerequisites for a bit-bang programmer are present.
 */
void bitbang_check_prerequisites(PROGRAMMER *pgm)
{
  verify_pin_assigned(pgm, PIN_AVR_RESET, "AVR RESET");
  verify_pin_assigned(pgm, PIN_AVR_SCK,   "AVR SCK");
  verify_pin_assigned(pgm, PIN_AVR_MISO,  "AVR MISO");
  verify_pin_assigned(pgm, PIN_AVR_MOSI,  "AVR MOSI");
}
