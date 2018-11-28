/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Support for bitbanging GPIO pins using the /sys/class/gpio interface
 *
 * Copyright (C) 2010 Radoslav Kolev <radoslav@kolev.info>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <wiringx.h>

#include "avrdude.h"
#include "avr.h"
#include "pindefs.h"
#include "pgm.h"
#include "avrbitbang.h"
#include "defines.h"
#include "../pilight/core/log.h"
#include "../pilight/config/settings.h"

/*
 * GPIO user space helpers
 *
 * Copyright 2009 Analog Devices Inc.
 * Michael Hennerich (hennerich@blackfin.uclinux.org)
 *
 * Licensed under the GPL-2 or later
 */

#define GPIO_DIR_IN	0
#define GPIO_DIR_OUT	1
#define N_GPIO 256

/*
* an array which holds open FDs to /sys/class/gpio/gpioXX/value for all needed pins
*/
static int gpio_fds[N_GPIO];

static int gpio_setpin(PROGRAMMER * pgm, int pin, int value) {
	if(gpio_fds[pin] != PINMODE_OUTPUT) {
		return -1;
	}

	/* Small delay for too fast computers */
	delayMicroseconds(1);

  if(value == 1) {
		digitalWrite(pin, HIGH);
  } else {
		digitalWrite(pin, LOW);
	}

  if(pgm->ispdelay > 1) {
    bitbang_delay(pgm->ispdelay);
	}

  return 0;
}

static int gpio_getpin(PROGRAMMER * pgm, int pin) {
	if(gpio_fds[pin] == PINMODE_INPUT) {
		return digitalRead(pin);
	} else {
		return -1;
	}
}

static int gpio_highpulsepin(PROGRAMMER * pgm, int pin) {

	if(gpio_fds[pin] == PINMODE_OUTPUT) {
		digitalWrite(pin, HIGH);
		digitalWrite(pin, LOW);
		return 0;
	} else {
		return -1;
	}
}

static void gpio_display(PROGRAMMER *pgm, const char *p) {
  /* MAYBE */
}

static void gpio_enable(PROGRAMMER *pgm) {
  /* nothing */
}

static void gpio_disable(PROGRAMMER *pgm) {
  /* nothing */
}

static void gpio_powerup(PROGRAMMER *pgm) {
  /* nothing */
}

static void gpio_powerdown(PROGRAMMER *pgm) {
  /* nothing */
}

static int gpio_open(PROGRAMMER *pgm, char *port) {
	int i = 0;

	bitbang_check_prerequisites(pgm);

	for(i=0;i<N_PINS;i++) {
		if(pgm->pinno[i] != 0) {
			if(i == PIN_AVR_MISO) {
				gpio_fds[pgm->pinno[i]] = PINMODE_INPUT;
				pinMode(pgm->pinno[i], PINMODE_INPUT);
			} else {
				gpio_fds[pgm->pinno[i]] = PINMODE_OUTPUT;;
				pinMode(pgm->pinno[i], PINMODE_OUTPUT);
			}
		}
	}

 return(0);
}

static void gpio_close(PROGRAMMER *pgm) {
	if(gpio_fds[pgm->pinno[PIN_AVR_RESET]] == PINMODE_OUTPUT) {
		digitalWrite(pgm->pinno[PIN_AVR_RESET], HIGH);
		// digitalWrite(pgm->pinno[PIN_AVR_RESET], LOW);
	}

  return;
}

void gpio_initpgm(PROGRAMMER *pgm)
{
  strcpy(pgm->type, "GPIO");
	char *platform = GPIO_PLATFORM;
	if(settings_find_string("gpio-platform", &platform) != 0 || strcmp(platform, "none") == 0) {
		logprintf(LOG_ERR, "gpio_switch: no gpio-platform configured");
		exit(EXIT_FAILURE);
	}
	if(wiringXSetup(platform, logprintf1) < 0) {
		exit(EXIT_FAILURE);
	}
  pgm->rdy_led        = bitbang_rdy_led;
  pgm->err_led        = bitbang_err_led;
  pgm->pgm_led        = bitbang_pgm_led;
  pgm->vfy_led        = bitbang_vfy_led;
  pgm->initialize     = bitbang_initialize;
  pgm->display        = gpio_display;
  pgm->enable         = gpio_enable;
  pgm->disable        = gpio_disable;
  pgm->powerup        = gpio_powerup;
  pgm->powerdown      = gpio_powerdown;
  pgm->program_enable = bitbang_program_enable;
  pgm->chip_erase     = bitbang_chip_erase;
  pgm->cmd            = bitbang_cmd;
  pgm->open           = gpio_open;
  pgm->close          = gpio_close;
  pgm->setpin         = gpio_setpin;
  pgm->getpin         = gpio_getpin;
  pgm->highpulsepin   = gpio_highpulsepin;
  pgm->read_byte      = avr_read_byte_default;
  pgm->write_byte     = avr_write_byte_default;
}
