/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * avrdude is Copyright (C) 2000-2004  Brian S. Dean <bsd@bsdhome.com>
 *
 * This file: Copyright (C) 2005 Colin O'Flynn <coflynn@newae.com>
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


#ifndef safemode_h
#define safemode_h

#ifdef __cplusplus
extern "C" {
#endif

/* Writes the specified fuse in fusename (can be "lfuse", "hfuse", or "efuse") and verifies it. Will try up to tries
amount of times before giving up */
int safemode_writefuse (unsigned char fuse, const char * fusename, PROGRAMMER * pgm, AVRPART * p, int tries);

/* Reads the fuses three times, checking that all readings are the same. This will ensure that the before values aren't in error! */
int safemode_readfuses (unsigned char * lfuse, unsigned char * hfuse, unsigned char * efuse, unsigned char * fuse, PROGRAMMER * pgm, AVRPART * p);

/* This routine will store the current values pointed to by lfuse, hfuse, and efuse into an internal buffer in this routine
when save is set to 1. When save is 0 (or not 1 really) it will copy the values from the internal buffer into the locations
pointed to be lfuse, hfuse, and efuse. This allows you to change the fuse bits if needed from another routine (ie: have it so
if user requests fuse bits are changed, the requested value is now verified */
int safemode_memfuses (int save, unsigned char * lfuse, unsigned char * hfuse, unsigned char * efuse, unsigned char * fuse);

#ifdef __cplusplus
}
#endif

#endif /* safemode_h */
