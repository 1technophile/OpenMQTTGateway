/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2003-2004  Theodore A. Roth  <troth@openavr.org>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* $Id: ser_posix.c 1294 2014-03-12 23:03:18Z joerg_wunsch $ */

/*
 * Posix serial interface for avrdude.
 */

#ifndef _WIN32

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "../pilight/core/log.h"
#include "../pilight/core/mem.h"
#include "avrdude.h"
#include "serial.h"

long serial_recv_timeout = 5000; /* ms */

static struct termios original_termios;
static int saved_original_termios;

static int ser_setspeed(union filedescriptor *fd, long baud) {
  int rc = 0;
  struct termios termios;
  speed_t speed = baud;

  if(!isatty(fd->ifd)) {
    return -ENOTTY;
	}

  /*
   * initialize terminal modes
   */
  if(tcgetattr(fd->ifd, &termios) < 0) {
    logprintf(LOG_ERR, "ser_setspeed(): tcgetattr() failed");
    return -errno;
  }

  /*
   * copy termios for ser_close if we haven't already
   */
  if(!saved_original_termios++) {
    original_termios = termios;
  }

  termios.c_iflag = IGNBRK;
  termios.c_oflag = 0;
  termios.c_lflag = 0;
  termios.c_cflag = (CS8 | CREAD | CLOCAL);
  termios.c_cc[VMIN]  = 1;
  termios.c_cc[VTIME] = 0;

  cfsetospeed(&termios, speed);
  cfsetispeed(&termios, speed);

  if(tcsetattr(fd->ifd, TCSANOW, &termios) < 0) {
    logprintf(LOG_ERR, "%s: ser_setspeed(): tcsetattr() failed");
    return -errno;
  }

  /*
   * Everything is now set up for a local line without modem control
   * or flow control, so clear O_NONBLOCK again.
   */
  rc = fcntl(fd->ifd, F_GETFL, 0);
  if(rc != -1) {
    fcntl(fd->ifd, F_SETFL, rc & ~O_NONBLOCK);
	}

  return 0;
}

static int ser_set_dtr_rts(union filedescriptor *fdp, int is_on) {
  unsigned int ctl = 0;

  if(ioctl(fdp->ifd, TIOCMGET, &ctl) < 0) {
    // perror("ioctl(\"TIOCMGET\")");
    return -1;
  }

  if(is_on > 0) {
    /* Set DTR and RTS */
    ctl |= (TIOCM_DTR | TIOCM_RTS);
  } else {
    /* Clear DTR and RTS */
    ctl &= ~(TIOCM_DTR | TIOCM_RTS);
  }

  if(ioctl(fdp->ifd, TIOCMSET, &ctl) < 0) {
    // perror("ioctl(\"TIOCMSET\")");
    return -1;
  }

  return 0;
}

static int ser_open(char * port, union pinfo pinfo, union filedescriptor *fdp) {
  int rc = 0, fd = 0;

  /*
   * open the serial port
   */
  if((fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
    logprintf(LOG_ERR, "ser_open(): can't open device \"%s\": %s", port, strerror(errno));
    return -1;
  }

  fdp->ifd = fd;

  /*
   * set serial line attributes
   */
  if((rc = ser_setspeed(fdp, pinfo.baud)) > 0) {
    logprintf(LOG_ERR, "ser_open(): can't set attributes for device \"%s\": %s", port, strerror(-rc));
    close(fd);
    return -1;
  }
  return 0;
}


static void ser_close(union filedescriptor *fd) {
  /*
   * restore original termios settings from ser_open
   */
  if(saved_original_termios > 0) {
    if(tcsetattr(fd->ifd, TCSANOW | TCSADRAIN, &original_termios) > 0) {
      logprintf(LOG_ERR, "ser_close(): can't reset attributes for device: %s", strerror(errno));
    }
    saved_original_termios = 0;
  }

  close(fd->ifd);
}


static int ser_send(union filedescriptor *fd, unsigned char * buf, size_t buflen) {
  int rc = 0;
  unsigned char *p = buf;
  size_t len = buflen;

  if(len <= 0)
    return 0;

  while(len) {
    rc = write(fd->ifd, p, (len > 1024) ? 1024 : len);
    if(rc < 0) {
      logprintf(LOG_ERR, "ser_send(): write error: %s", strerror(errno));
      exit(1);
    }
    p += rc;
    len -= rc;
  }

  return 0;
}


static int ser_recv(union filedescriptor *fd, unsigned char * buf, size_t buflen) {
  struct timeval timeout, to2;
  fd_set rfds;
  int nfds = 0, rc = 0;
  unsigned char *p = buf;
  size_t len = 0;

  timeout.tv_sec = serial_recv_timeout / 1000L;
  timeout.tv_usec = (serial_recv_timeout % 1000L) * 1000;
  to2 = timeout;

  while(len < buflen) {
  reselect:
    FD_ZERO(&rfds);
    FD_SET(fd->ifd, &rfds);

    nfds = select(fd->ifd + 1, &rfds, NULL, NULL, &to2);
    if(nfds == 0) {
			logprintf(LOG_ERR, "ser_recv(): programmer is not responding");
			return -1;
    } else if(nfds == -1) {
      if(errno == EINTR || errno == EAGAIN) {
				logprintf(LOG_ERR, "ser_recv(): programmer is not responding, reselecting");
        goto reselect;
      } else {
        logprintf(LOG_ERR, "ser_recv(): select(): %s", strerror(errno));
        return -1;
      }
    }

    rc = read(fd->ifd, p, (buflen - len > 1024) ? 1024 : buflen - len);
    if(rc < 0) {
      logprintf(LOG_ERR, "ser_recv(): read error: %s", strerror(errno));
      return -1;
    }
    p += rc;
    len += rc;
  }

  p = buf;

  return 0;
}


static int ser_drain(union filedescriptor *fd, int display) {
  struct timeval timeout;
  fd_set rfds;
  int nfds = 0, rc = 0;
  unsigned char buf;

  timeout.tv_sec = 0;
  timeout.tv_usec = 250000;

  while(1) {
    FD_ZERO(&rfds);
    FD_SET(fd->ifd, &rfds);

  reselect:
    nfds = select(fd->ifd + 1, &rfds, NULL, NULL, &timeout);
    if(nfds == 0) {
      break;
    } else if(nfds == -1) {
      if(errno == EINTR) {
        goto reselect;
      } else {
        logprintf(LOG_ERR, "ser_drain(): select(): %s", strerror(errno));
        return -1;
      }
    }

    rc = read(fd->ifd, &buf, 1);
    if(rc < 0) {
      logprintf(LOG_ERR, "ser_drain(): read error: %s", strerror(errno));
      return -1;
    }
  }

  return 0;
}

struct serial_device serial_serdev = {
  .open = ser_open,
  .setspeed = ser_setspeed,
  .close = ser_close,
  .send = ser_send,
  .recv = ser_recv,
  .drain = ser_drain,
  .set_dtr_rts = ser_set_dtr_rts,
  .flags = SERDEV_FL_CANSETSPEED,
};

struct serial_device *serdev = &serial_serdev;

#endif  /* WIN32NATIVE */
