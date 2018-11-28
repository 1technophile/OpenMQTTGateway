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
/* $Id: bitbang.h 797 2009-02-17 15:31:27Z joerg_wunsch $ */

#ifndef avrbitbang_h
#define avrbitbang_h

#ifdef __cplusplus
extern "C" {
#endif

int bitbang_setpin(int fd, int pin, int value);
int bitbang_getpin(int fd, int pin);
int bitbang_highpulsepin(int fd, int pin);
void bitbang_delay(unsigned int us);

void bitbang_check_prerequisites(PROGRAMMER *pgm);

int  bitbang_rdy_led        (PROGRAMMER * pgm, int value);
int  bitbang_err_led        (PROGRAMMER * pgm, int value);
int  bitbang_pgm_led        (PROGRAMMER * pgm, int value);
int  bitbang_vfy_led        (PROGRAMMER * pgm, int value);
int  bitbang_cmd            (PROGRAMMER * pgm, unsigned char cmd[4],
                                unsigned char res[4]);
int  bitbang_spi            (PROGRAMMER * pgm, unsigned char cmd[],
                                unsigned char res[], int count);
int  bitbang_chip_erase     (PROGRAMMER * pgm, AVRPART * p);
int  bitbang_program_enable (PROGRAMMER * pgm, AVRPART * p);
void bitbang_powerup        (PROGRAMMER * pgm);
void bitbang_powerdown      (PROGRAMMER * pgm);
int  bitbang_initialize     (PROGRAMMER * pgm, AVRPART * p);
void bitbang_disable        (PROGRAMMER * pgm);
void bitbang_enable         (PROGRAMMER * pgm);

#ifdef __cplusplus
}
#endif

#endif
