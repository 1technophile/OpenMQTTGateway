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

/* $Id: ser_win32.c 1294 2014-03-12 23:03:18Z joerg_wunsch $ */

/*
* Native Win32 serial interface for avrdude.
*/

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <ctype.h>

#include "../pilight/core/log.h"
#include "../pilight/core/mem.h"
#include "serial.h"

long serial_recv_timeout = 5000; /* ms */

#define W32SERBUFSIZE 1024

static BOOL serial_w32SetTimeOut(HANDLE hComPort, DWORD timeout) {
	COMMTIMEOUTS ctmo;
	ZeroMemory (&ctmo, sizeof(COMMTIMEOUTS));
	ctmo.ReadIntervalTimeout = timeout;
	ctmo.ReadTotalTimeoutMultiplier = timeout;
	ctmo.ReadTotalTimeoutConstant = timeout;

	return SetCommTimeouts(hComPort, &ctmo);
}

static int ser_setspeed(union filedescriptor *fd, long baud) {
	DCB dcb;
	HANDLE hComPort = (HANDLE)fd->pfd;

	ZeroMemory (&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = baud;
	dcb.fBinary = 1;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	if(!SetCommState(hComPort, &dcb)) {
		return -1;
	}

	return 0;
}


static int ser_open(char * port, union pinfo pinfo, union filedescriptor *fdp) {
	LPVOID lpMsgBuf;
	HANDLE hComPort=INVALID_HANDLE_VALUE;
	char *newname = 0;

	if(strncasecmp(port, "com", strlen("com")) == 0) {
		// prepend "\\\\.\\" to name, required for port # >= 10
		newname = MALLOC(strlen("\\\\.\\") + strlen(port) + 1);

		if(newname == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(newname, "\\\\.\\");
		strcat(newname, port);

		port = newname;
	}

	hComPort = CreateFile(port, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hComPort == INVALID_HANDLE_VALUE) {
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			logprintf(LOG_ERR, "ser_open(): can't open device \"%s\": %s", port, (char*)lpMsgBuf);
			if(newname != NULL) {
				FREE(newname);
			}
			LocalFree(lpMsgBuf);
			return -1;
	}

	if(!SetupComm(hComPort, W32SERBUFSIZE, W32SERBUFSIZE)) {
		CloseHandle(hComPort);
		logprintf(LOG_ERR, "ser_open(): can't set buffers for \"%s\"", port);
		if(newname != NULL) {
			FREE(newname);
		}
		return -1;
	}

	fdp->pfd = (void *)hComPort;
	if(ser_setspeed(fdp, pinfo.baud) != 0) {
		CloseHandle(hComPort);
		logprintf(LOG_ERR, "ser_open(): can't set com-state for \"%s\"", port);
		if(newname != NULL) {
			FREE(newname);
		}
		return -1;
	}

	if(!serial_w32SetTimeOut(hComPort, 0)) {
		CloseHandle(hComPort);
		logprintf(LOG_ERR, "ser_open(): can't set initial timeout for \"%s\"", port);
		if(newname != NULL) {
			FREE(newname);
		}
		return -1;
	}

	if(newname != NULL) {
		FREE(newname);
	}
	return 0;
}


static void ser_close(union filedescriptor *fd) {
	HANDLE hComPort=(HANDLE)fd->pfd;
	if(hComPort != INVALID_HANDLE_VALUE) {
		CloseHandle (hComPort);
	}

	hComPort = INVALID_HANDLE_VALUE;
}

static int ser_set_dtr_rts(union filedescriptor *fd, int is_on) {
	HANDLE hComPort=(HANDLE)fd->pfd;

	if(is_on > 0) {
		EscapeCommFunction(hComPort, SETDTR);
		EscapeCommFunction(hComPort, SETRTS);
	} else {
		EscapeCommFunction(hComPort, CLRDTR);
		EscapeCommFunction(hComPort, CLRRTS);
	}
	return 0;
}


static int ser_send(union filedescriptor *fd, unsigned char *buf, size_t buflen) {
	size_t len = buflen;
	DWORD written;

	HANDLE hComPort = (HANDLE)fd->pfd;

	if(hComPort == INVALID_HANDLE_VALUE) {
		logprintf(LOG_ERR, "ser_send(): port not open");
		return -1;
	}

	if(len <= 0) {
		return 0;
	}

	serial_w32SetTimeOut(hComPort,500);

	if(!WriteFile(hComPort, buf, buflen, &written, NULL)) {
		logprintf(LOG_ERR, "ser_send(): write error"); // TODO
		exit(1);
	}

	if(written != buflen) {
		logprintf(LOG_ERR, "ser_send(): size/send mismatch");
		return -1;
	}

	return 0;
}


static int ser_recv(union filedescriptor *fd, unsigned char *buf, size_t buflen) {
	DWORD read;

	HANDLE hComPort = (HANDLE)fd->pfd;

	if(hComPort == INVALID_HANDLE_VALUE) {
		logprintf(LOG_ERR, "ser_read(): port not open");
		return -1;
	}

	serial_w32SetTimeOut(hComPort, serial_recv_timeout);

	if(!ReadFile(hComPort, buf, buflen, &read, NULL)) {
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		logprintf(LOG_ERR, "ser_recv(): read error: %s", (char*)lpMsgBuf);
		LocalFree(lpMsgBuf);
		return -1;
	}

	/* time out detected */
	if(read == 0) {
		return -1;
	}

	return 0;
}


static int ser_drain(union filedescriptor *fd, int display) {
	unsigned char buf[10];
	BOOL readres;
	DWORD read;

	HANDLE hComPort=(HANDLE)fd->pfd;

	if(hComPort == INVALID_HANDLE_VALUE) {
		logprintf(LOG_ERR, "%s: ser_drain(): port not open");
		return -1;
	}

	serial_w32SetTimeOut(hComPort,250);

	while(1) {
		readres=ReadFile(hComPort, buf, 1, &read, NULL);
		if(!readres) {
			LPVOID lpMsgBuf;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			logprintf(LOG_ERR, "ser_drain(): read error: %s", (char*)lpMsgBuf);
			LocalFree(lpMsgBuf);
			return -1;
		}

		if(read <= 0) {
			break;
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

#endif
