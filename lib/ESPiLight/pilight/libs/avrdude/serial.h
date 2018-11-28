/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2003-2004  Theodore A. Roth  <troth@openavr.org>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* $Id: serial.h 1294 2014-03-12 23:03:18Z joerg_wunsch $ */

/* This is the API for the generic serial interface. The implementations are
   actually provided by the target dependant files:

   ser_posix.c : posix serial interface.
   ser_win32.c : native win32 serial interface.

   The target file will be selected at configure time. */

#ifndef serial_h
#define serial_h

extern long serial_recv_timeout;
union filedescriptor
{
  int ifd;
  void *pfd;
  struct
  {
    void *handle;
    int rep;                    /* bulk read endpoint */
    int wep;                    /* bulk write endpoint */
    int eep;                    /* event read endpoint */
    int max_xfer;               /* max transfer size */
    int use_interrupt_xfer;     /* device uses interrupt transfers */
  } usb;
};

union pinfo
{
  long baud;
  struct
  {
    unsigned short vid;
    unsigned short pid;
    unsigned short flags;
#define PINFO_FL_USEHID         0x0001
#define PINFO_FL_SILENT         0x0002  /* don't complain if not found */
  } usbinfo;
};


struct serial_device
{
  // open should return -1 on error, other values on success
  int (*open)(char * port, union pinfo pinfo, union filedescriptor *fd);
  int (*setspeed)(union filedescriptor *fd, long baud);
  void (*close)(union filedescriptor *fd);

  int (*send)(union filedescriptor *fd, unsigned char * buf, size_t buflen);
  int (*recv)(union filedescriptor *fd, unsigned char * buf, size_t buflen);
  int (*drain)(union filedescriptor *fd, int display);

  int (*set_dtr_rts)(union filedescriptor *fd, int is_on);

  int flags;
#define SERDEV_FL_NONE         0x0000 /* no flags */
#define SERDEV_FL_CANSETSPEED  0x0001 /* device can change speed */
};

extern struct serial_device *serdev;
extern struct serial_device serial_serdev;
extern struct serial_device usb_serdev;
extern struct serial_device usb_serdev_frame;
extern struct serial_device avrdoper_serdev;

#define serial_open (serdev->open)
#define serial_setspeed (serdev->setspeed)
#define serial_close (serdev->close)
#define serial_send (serdev->send)
#define serial_recv (serdev->recv)
#define serial_drain (serdev->drain)
#define serial_set_dtr_rts (serdev->set_dtr_rts)

#endif /* serial_h */
