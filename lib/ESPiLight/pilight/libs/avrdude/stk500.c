/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2002-2004 Brian S. Dean <bsd@bsdhome.com>
 * Copyright (C) 2008,2014 Joerg Wunsch
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

/* $Id: stk500.c 1294 2014-03-12 23:03:18Z joerg_wunsch $ */

/*
 * avrdude interface for Atmel STK500 programmer
 *
 * Note: most commands use the "universal command" feature of the
 * programmer in a "pass through" mode, exceptions are "program
 * enable", "paged read", and "paged write".
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "../pilight/core/log.h"
#include "avrdude.h"
#include "avr.h"
#include "pgm.h"
#include "stk500.h"
#include "stk500_private.h"
#include "serial.h"

#define STK500_XTAL 7372800U
#define MAX_SYNC_ATTEMPTS 1

struct pdata {
	unsigned char ext_addr_byte; /* Record ext-addr byte set in the target device (if used) */
};

#define PDATA(pgm) ((struct pdata *)(pgm->cookie))

static int stk500_send(PROGRAMMER *pgm, unsigned char *buf, size_t len) {
	return serial_send(&pgm->fd, buf, len);
}

static int stk500_recv(PROGRAMMER *pgm, unsigned char *buf, size_t len) {
	if(serial_recv(&pgm->fd, buf, len) < 0) {
		logprintf(LOG_ERR, "stk500_recv(): programmer is not responding");
		return -1;
	}
	return 0;
}


int stk500_drain(PROGRAMMER *pgm, int display) {
	return serial_drain(&pgm->fd, display);
}

int stk500_getsync(PROGRAMMER * pgm) {
	unsigned char buf[32], resp[32];
	int attempt = 0;

	/*
	* get in sync */
	buf[0] = Cmnd_STK_GET_SYNC;
	buf[1] = Sync_CRC_EOP;

	/*
	* First send and drain a few times to get rid of line noise
	*/
	stk500_send(pgm, buf, 2);
	stk500_drain(pgm, 0);
	stk500_send(pgm, buf, 2);
	stk500_drain(pgm, 0);

	for(attempt = 0; attempt < MAX_SYNC_ATTEMPTS; attempt++) {
		stk500_send(pgm, buf, 2);
		stk500_recv(pgm, resp, 1);
		if(resp[0] == Resp_STK_INSYNC){
			break;
		}
		logprintf(LOG_ERR, "stk500_getsync() attempt %d of %d: not in sync: resp=0x%02x",
												attempt + 1, MAX_SYNC_ATTEMPTS, resp[0]);
	}
	if(attempt == MAX_SYNC_ATTEMPTS) {
		stk500_drain(pgm, 0);
		return -1;
	}

	if(stk500_recv(pgm, resp, 1) < 0) {
		return -1;
	}
	if(resp[0] != Resp_STK_OK) {
		logprintf(LOG_ERR, "stk500_getsync(): can't communicate with device: resp=0x%02x", resp[0]);
		return -1;
	}

	return 0;
}


/*
* transmit an AVR device command and return the results; 'cmd' and
* 'res' must point to at least a 4 byte data buffer
*/
static int stk500_cmd(PROGRAMMER *pgm, const unsigned char *cmd, unsigned char *res) {
	unsigned char buf[32];

	buf[0] = Cmnd_STK_UNIVERSAL;
	buf[1] = cmd[0];
	buf[2] = cmd[1];
	buf[3] = cmd[2];
	buf[4] = cmd[3];
	buf[5] = Sync_CRC_EOP;

	stk500_send(pgm, buf, 6);

	if(stk500_recv(pgm, buf, 1) < 0) {
		return -1;
	}
	if(buf[0] != Resp_STK_INSYNC) {
		logprintf(LOG_ERR, "stk500_cmd(): programmer is out of sync");
		return -1;
	}

	res[0] = cmd[1];
	res[1] = cmd[2];
	res[2] = cmd[3];
	if(stk500_recv(pgm, &res[3], 1) < 0) {
		return -1;
	}

	if(stk500_recv(pgm, buf, 1) < 0) {
		return -1;
	}
	if(buf[0] != Resp_STK_OK) {
		logprintf(LOG_ERR, "stk500_cmd(): protocol error");
		return -1;
	}

	return 0;
}



/*
* issue the 'chip erase' command to the AVR device
*/
static int stk500_chip_erase(PROGRAMMER *pgm, AVRPART *p) {
	unsigned char cmd[4];
	unsigned char res[4];

	if(pgm->cmd == NULL) {
		logprintf(LOG_ERR, "Error: %s programmer uses stk500_chip_erase() but does not provide a cmd() method.", pgm->type);
		return -1;
	}

	if(p->op[AVR_OP_CHIP_ERASE] == NULL) {
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
static int stk500_program_enable(PROGRAMMER *pgm, AVRPART *p) {
	unsigned char buf[16];
	int tries=0;

	retry:

		tries++;

		buf[0] = Cmnd_STK_ENTER_PROGMODE;
		buf[1] = Sync_CRC_EOP;

		stk500_send(pgm, buf, 2);
		if(stk500_recv(pgm, buf, 1) < 0) {
			return -1;
		}
		if(buf[0] == Resp_STK_NOSYNC) {
			if(tries > 33) {
				logprintf(LOG_ERR, "stk500_program_enable(): can't get into sync");
				return -1;
			}
			if(stk500_getsync(pgm) < 0) {
				return -1;
			}
			goto retry;
		} else if(buf[0] != Resp_STK_INSYNC) {
			logprintf(LOG_ERR, "%s: stk500_program_enable(): protocol error, expect=0x%02x, resp=0x%02x\n", Resp_STK_INSYNC, buf[0]);
			return -1;
		}

		if(stk500_recv(pgm, buf, 1) < 0)
			return -1;
		if(buf[0] == Resp_STK_OK) {
			return 0;
		} else if(buf[0] == Resp_STK_NODEVICE) {
			logprintf(LOG_ERR, "stk500_program_enable(): no device");
			return -1;
		}

		if(buf[0] == Resp_STK_FAILED) {
			logprintf(LOG_ERR, "stk500_program_enable(): failed to enter programming mode");
			return -1;
		}


	logprintf(LOG_ERR, "stk500_program_enable(): unknown response=0x%02x", buf[0]);

	return -1;
}

/*
* initialize the AVR device and prepare it to accept commands
*/
static int stk500_initialize(PROGRAMMER *pgm, AVRPART *p) {
	unsigned char buf[32];
	AVRMEM *m = NULL;
	int tries = 0;

	tries = 0;

retry:
	tries++;

	memset(buf, 0, sizeof(buf));

	/*
	* set device programming parameters
	*/
	buf[0] = Cmnd_STK_SET_DEVICE;
	buf[1] = p->stk500_devcode;
	buf[2] = 0; /* device revision */

	if((p->flags & AVRPART_SERIALOK) && (p->flags & AVRPART_PARALLELOK)) {
		buf[3] = 0; /* device supports parallel and serial programming */
	} else {
		buf[3] = 1; /* device supports parallel only */
	}

	if(p->flags & AVRPART_PARALLELOK) {
		if(p->flags & AVRPART_PSEUDOPARALLEL) {
			buf[4] = 0; /* pseudo parallel interface */
		} else {
			buf[4] = 1; /* full parallel interface */
		}
	}

	buf[5] = 1; /* polling supported - XXX need this in config file */
	buf[6] = 1; /* programming is self-timed - XXX need in config file */
	buf[7] = 0;
	buf[8] = 0;
	buf[8] += p->lfusemem->size;
	buf[8] += p->hfusemem->size;

	m = p->flashmem;
	if(m != NULL) {
		buf[9] = m->readback[0];
		buf[10] = m->readback[1];
		if(m->paged == 1) {
			buf[13] = (m->page_size >> 8) & 0x00ff;
			buf[14] = m->page_size & 0x00ff;
		}
		buf[17] = (m->size >> 24) & 0xff;
		buf[18] = (m->size >> 16) & 0xff;
		buf[19] = (m->size >> 8) & 0xff;
		buf[20] = m->size & 0xff;
	}

	buf[11] = 0xff;
	buf[12] = 0xff;
	buf[15] = 0;
	buf[16] = 0;
	buf[21] = Sync_CRC_EOP;

	stk500_send(pgm, buf, 22);
	if(stk500_recv(pgm, buf, 1) < 0) {
		return -1;
	}
	if(buf[0] == Resp_STK_NOSYNC) {
		logprintf(LOG_ERR, "stk500_initialize(): programmer not in sync, resp=0x%02x", buf[0]);
		if(tries > 33) {
			return -1;
		}
		if(stk500_getsync(pgm) < 0) {
			return -1;
		}
		goto retry;
	} else if(buf[0] != Resp_STK_INSYNC) {
		logprintf(LOG_ERR, "stk500_initialize(): (a) protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_INSYNC, buf[0]);
		return -1;
	}

	if(stk500_recv(pgm, buf, 1) < 0) {
		return -1;
	}
	if(buf[0] != Resp_STK_OK) {
		logprintf(LOG_ERR, "stk500_initialize(): (b) protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_OK, buf[0]);
		return -1;
	}

	return pgm->program_enable(pgm, p);
}

static int stk500_loadaddr(PROGRAMMER *pgm, AVRMEM *mem, unsigned int addr) {
	unsigned char buf[16];
	int tries = 0;
	unsigned char ext_byte;
	OPCODE * lext;

	tries = 0;

retry:
	tries++;

	/* To support flash > 64K words the correct Extended Address Byte is needed */
	lext = mem->op[AVR_OP_LOAD_EXT_ADDR];
	if(lext != NULL) {
		ext_byte = (addr >> 16) & 0xff;
		if(ext_byte != PDATA(pgm)->ext_addr_byte) {
			/* Either this is the first addr load, or a 64K word boundary is
			* crossed, so set the ext addr byte */
			avr_set_bits(lext, buf);
			avr_set_addr(lext, buf, addr);
			stk500_cmd(pgm, buf, buf);
			PDATA(pgm)->ext_addr_byte = ext_byte;
		}
	}

	buf[0] = Cmnd_STK_LOAD_ADDRESS;
	buf[1] = addr & 0xff;
	buf[2] = (addr >> 8) & 0xff;
	buf[3] = Sync_CRC_EOP;

	stk500_send(pgm, buf, 4);

	if(stk500_recv(pgm, buf, 1) < 0) {
		return -1;
	}
	if(buf[0] == Resp_STK_NOSYNC) {
		if(tries > 33) {
			logprintf(LOG_ERR, "stk500_loadaddr(): can't get into sync");
			return -1;
		}
		if(stk500_getsync(pgm) < 0) {
			return -1;
		}
		goto retry;
	} else if(buf[0] != Resp_STK_INSYNC) {
		logprintf(LOG_ERR, "stk500_loadaddr(): (a) protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_INSYNC, buf[0]);
		return -1;
	}

	if(stk500_recv(pgm, buf, 1) < 0) {
		return -1;
	}
	if(buf[0] == Resp_STK_OK) {
		return 0;
	}

	logprintf(LOG_ERR, "loadaddr(): (b) protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_INSYNC, buf[0]);

	return -1;
}


static int stk500_paged_write(PROGRAMMER *pgm, AVRPART *p, AVRMEM *m,
															unsigned int page_size,
															unsigned int addr, unsigned int n_bytes) {

	unsigned char buf[page_size + 16];
	int memtype = 0, a_div = 0, block_size = 0, tries = 0;
	unsigned int n = 0, i = 0;

	if(strcmp(m->desc, "flash") == 0) {
		memtype = 'F';
	} else if(strcmp(m->desc, "eeprom") == 0) {
		memtype = 'E';
	} else {
		return -2;
	}

	if((m->op[AVR_OP_LOADPAGE_LO]) || (m->op[AVR_OP_READ_LO])) {
		a_div = 2;
	} else {
		a_div = 1;
	}

	n = addr + n_bytes;

	for(; addr < n; addr += block_size) {
		if(n - addr < page_size) {
			block_size = n - addr;
		} else {
			block_size = page_size;
		}
		tries = 0;
	retry:
		tries++;
		stk500_loadaddr(pgm, m, addr/a_div);

		/* build command block and avoid multiple send commands as it leads to a crash
				of the silabs usb serial driver on mac os x */
		i = 0;
		buf[i++] = Cmnd_STK_PROG_PAGE;
		buf[i++] = (block_size >> 8) & 0xff;
		buf[i++] = block_size & 0xff;
		buf[i++] = memtype;

		memcpy(&buf[i], &m->buf[addr], block_size);

		i += block_size;
		buf[i++] = Sync_CRC_EOP;
		stk500_send(pgm, buf, i);

		if(stk500_recv(pgm, buf, 1) < 0) {
			return -1;
		}
		if(buf[0] == Resp_STK_NOSYNC) {
			if(tries > 33) {
				logprintf(LOG_ERR, "stk500_paged_write(): can't get into sync", progname);
				return -3;
			}
			if(stk500_getsync(pgm) < 0) {
				return -1;
			}
			goto retry;
		} else if(buf[0] != Resp_STK_INSYNC) {
			logprintf(LOG_ERR, "stk500_paged_write(): (a) protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_INSYNC, buf[0]);
			return -4;
		}

		if(stk500_recv(pgm, buf, 1) < 0) {
			return -1;
		}
		if(buf[0] != Resp_STK_OK) {
			logprintf(LOG_ERR, "stk500_paged_write(): (a) protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_INSYNC, buf[0]);
			return -5;
		}
	}

	return n_bytes;
}

static int stk500_paged_load(PROGRAMMER *pgm, AVRPART *p, AVRMEM *m,
														unsigned int page_size,
														unsigned int addr, unsigned int n_bytes) {
	unsigned char buf[16];
	int memtype = 0, a_div = 0, tries = 0, block_size = 0;
	unsigned int n = 0;

	if(strcmp(m->desc, "flash") == 0) {
		memtype = 'F';
	} else if(strcmp(m->desc, "eeprom") == 0) {
		memtype = 'E';
	} else {
		return -2;
	}

	if((m->op[AVR_OP_LOADPAGE_LO]) || (m->op[AVR_OP_READ_LO])) {
		a_div = 2;
	} else {
		a_div = 1;
	}

	n = addr + n_bytes;
	for(; addr < n; addr += block_size) {
		if(n - addr < page_size) {
			block_size = n - addr;
		}	else {
			block_size = page_size;
		}

		tries = 0;
	retry:
		tries++;
		stk500_loadaddr(pgm, m, addr/a_div);
		buf[0] = Cmnd_STK_READ_PAGE;
		buf[1] = (block_size >> 8) & 0xff;
		buf[2] = block_size & 0xff;
		buf[3] = memtype;
		buf[4] = Sync_CRC_EOP;
		stk500_send(pgm, buf, 5);

		if(stk500_recv(pgm, buf, 1) < 0) {
			return -1;
		}
		if(buf[0] == Resp_STK_NOSYNC) {
			if(tries > 33) {
				logprintf(LOG_ERR, "stk500_paged_load(): can't get into sync");
				return -3;
			}
			if(stk500_getsync(pgm) < 0) {
				return -1;
				goto retry;
			}
		} else if(buf[0] != Resp_STK_INSYNC) {
			logprintf(LOG_ERR, "stk500_paged_load(): (a) protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_INSYNC, buf[0]);
			return -4;
		}

		if(stk500_recv(pgm, &m->buf[addr], block_size) < 0) {
			return -1;
		}

		if(stk500_recv(pgm, buf, 1) < 0) {
			return -1;
		}

		if(buf[0] != Resp_STK_OK) {
			logprintf(LOG_ERR, "stk500_paged_load(): (a) protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_OK, buf[0]);
			return -5;
		}
	}

	return n_bytes;
}


static void stk500_display(PROGRAMMER * pgm, const char * p) {
}

static void stk500_disable(PROGRAMMER * pgm) {
	unsigned char buf[16];
	int tries=0;

retry:
	tries++;
	buf[0] = Cmnd_STK_LEAVE_PROGMODE;
	buf[1] = Sync_CRC_EOP;
	stk500_send(pgm, buf, 2);

	if(stk500_recv(pgm, buf, 1) < 0) {
		return;
	}
	if(buf[0] == Resp_STK_NOSYNC) {
		if(tries > 33) {
			logprintf(LOG_ERR, "stk500_disable(): can't get into sync");
			return;
		}
		if(stk500_getsync(pgm) < 0) {
			return;
		}
		goto retry;
	} else if(buf[0] != Resp_STK_INSYNC) {
		logprintf(LOG_ERR, "stk500_disable(): protocol error, expect=0x%02x, resp=0x%02x", Resp_STK_INSYNC, buf[0]);
		return;
	}
	if(stk500_recv(pgm, buf, 1) < 0) {
		return;
	}
	if(buf[0] == Resp_STK_OK) {
		return;
	} else if(buf[0] == Resp_STK_NODEVICE) {
		logprintf(LOG_ERR, "stk500_disable(): no device");
		return;
	}
	logprintf(LOG_ERR, "stk500_disable(): unknown response=0x%02x", buf[0]);
	return;
}

static void stk500_enable(PROGRAMMER *pgm) {
	return;
}

static void stk500_close(PROGRAMMER *pgm) {
	serial_close(&pgm->fd);
	pgm->fd.ifd = -1;
}

const char stk500_desc[] = "Atmel STK500 Version 1.x firmware";

void stk500_initpgm(PROGRAMMER * pgm) {
	strcpy(pgm->type, "STK500");

	/*
	* mandatory functions
	*/
	pgm->initialize     = stk500_initialize;
	pgm->display        = stk500_display;
	pgm->enable					= stk500_enable;
	pgm->disable				= stk500_disable;
	pgm->program_enable = stk500_program_enable;
	pgm->chip_erase     = stk500_chip_erase;
	pgm->cmd            = stk500_cmd;
	pgm->close					= stk500_close;
	pgm->read_byte      = avr_read_byte_default;
	pgm->write_byte     = avr_write_byte_default;

	/*
	* optional functions
	*/
	pgm->paged_write    = stk500_paged_write;
	pgm->paged_load     = stk500_paged_load;
	pgm->page_size      = 256;
}
