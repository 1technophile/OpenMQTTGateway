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

/* $Id: config.h 725 2007-01-30 13:41:54Z joerg_wunsch $ */

#ifndef avrconfig_h
#define avrconfig_h

#include "avrpart.h"
#include "pindefs.h"
#include "avr.h"
#include "pgm.h"

#define MAX_STR_CONST 1024

enum { V_NONE, V_NUM, V_STR };
typedef struct value_t {
  int      type;
  double   number;
  char   * string;
} VALUE;


typedef struct token_t {
  int primary;
  VALUE value;
} TOKEN;
typedef struct token_t *token_p;


extern FILE       * in;
extern PROGRAMMER * current_prog;
extern AVRPART    * current_part;
extern AVRMEM     * current_mem;
extern int          lineno;
extern const char * infile;
extern char         default_programmer[];
extern char         default_parallel[];
extern char         default_serial[];

enum tokentype {
	K_READ = 258,
	K_WRITE = 259,
	K_READ_LO = 260,
	K_READ_HI = 261,
	K_WRITE_LO = 262,
	K_WRITE_HI = 263,
	K_LOADPAGE_LO = 264,
	K_LOADPAGE_HI = 265,
	K_LOAD_EXT_ADDR = 266,
	K_WRITEPAGE = 267,
	K_CHIP_ERASE = 268,
	K_PGM_ENABLE = 269,
	K_MEMORY = 270,
	K_PAGE_SIZE = 271,
	K_PAGED = 272,
	K_ARDUINO = 273,
	K_BAUDRATE = 274,
	K_BS2 = 275,
	K_BUFF = 276,
	K_BUSPIRATE = 277,
	K_CHIP_ERASE_DELAY = 278,
	K_DEDICATED = 279,
	K_DEFAULT_PARALLEL = 280,
	K_DEFAULT_PROGRAMMER = 281,
	K_DEFAULT_SERIAL = 282,
	K_DESC = 283,
	K_DEVICECODE = 284,
	K_DRAGON_DW = 285,
	K_DRAGON_HVSP = 286,
	K_DRAGON_ISP = 287,
	K_DRAGON_JTAG = 288,
	K_DRAGON_PDI = 289,
	K_DRAGON_PP = 290,
	K_GPIO = 291,
	K_STK500_DEVCODE = 292,
	K_AVR910_DEVCODE = 293,
	K_EEPROM = 294,
	K_ERRLED = 295,
	K_FLASH = 296,
	K_ID = 297,
	K_IO = 298,
	K_JTAG_MKI = 299,
	K_JTAG_MKII = 300,
	K_JTAG_MKII_AVR32 = 301,
	K_JTAG_MKII_DW = 302,
	K_JTAG_MKII_ISP = 303,
	K_JTAG_MKII_PDI = 304,
	K_LOADPAGE = 305,
	K_MAX_WRITE_DELAY = 306,
	K_MIN_WRITE_DELAY = 307,
	K_MISO = 308,
	K_MOSI = 309,
	K_NUM_PAGES = 310,
	K_NVM_BASE = 311,
	K_OFFSET = 312,
	K_PAGEL = 313,
	K_PAR = 314,
	K_PARALLEL = 315,
	K_PART = 316,
	K_PGMLED = 317,
	K_PROGRAMMER = 318,
	K_PSEUDO = 319,
	K_PWROFF_AFTER_WRITE = 320,
	K_RDYLED = 321,
	K_READBACK_P1 = 322,
	K_READBACK_P2 = 323,
	K_READMEM = 324,
	K_RESET = 325,
	K_RETRY_PULSE = 326,
	K_SERBB = 327,
	K_SERIAL = 328,
	K_SCK = 329,
	K_SIGNATURE = 330,
	K_SIZE = 331,
	K_STK500 = 332,
	K_STK500HVSP = 333,
	K_STK500PP = 334,
	K_STK500V2 = 335,
	K_STK500GENERIC = 336,
	K_STK600 = 337,
	K_STK600HVSP = 338,
	K_STK600PP = 339,
	K_AVR910 = 340,
	K_USBASP = 341,
	K_USBTINY = 342,
	K_BUTTERFLY = 343,
	K_TYPE = 344,
	K_VCC = 345,
	K_VFYLED = 346,
	K_NO = 347,
	K_YES = 348,
	K_TIMEOUT = 349,
	K_STABDELAY = 350,
	K_CMDEXEDELAY = 351,
	K_HVSPCMDEXEDELAY = 352,
	K_SYNCHLOOPS = 353,
	K_BYTEDELAY = 354,
	K_POLLVALUE = 355,
	K_POLLINDEX = 356,
	K_PREDELAY = 357,
	K_POSTDELAY = 358,
	K_POLLMETHOD = 359,
	K_MODE = 360,
	K_DELAY = 361,
	K_BLOCKSIZE = 362,
	K_READSIZE = 363,
	K_HVENTERSTABDELAY = 364,
	K_PROGMODEDELAY = 365,
	K_LATCHCYCLES = 366,
	K_TOGGLEVTG = 367,
	K_POWEROFFDELAY = 368,
	K_RESETDELAYMS = 369,
	K_RESETDELAYUS = 370,
	K_HVLEAVESTABDELAY = 371,
	K_RESETDELAY = 372,
	K_SYNCHCYCLES = 373,
	K_HVCMDEXEDELAY = 374,
	K_CHIPERASEPULSEWIDTH = 375,
	K_CHIPERASEPOLLTIMEOUT = 376,
	K_CHIPERASETIME = 377,
	K_PROGRAMFUSEPULSEWIDTH = 378,
	K_PROGRAMFUSEPOLLTIMEOUT = 379,
	K_PROGRAMLOCKPULSEWIDTH = 380,
	K_PROGRAMLOCKPOLLTIMEOUT = 381,
	K_PP_CONTROLSTACK = 382,
	K_HVSP_CONTROLSTACK = 383,
	K_ALLOWFULLPAGEBITSTREAM = 384,
	K_ENABLEPAGEPROGRAMMING = 385,
	K_HAS_JTAG = 386,
	K_HAS_DW = 387,
	K_HAS_PDI = 388,
	K_HAS_TPI = 389,
	K_IDR = 390,
	K_IS_AVR32 = 391,
	K_RAMPZ = 392,
	K_SPMCR = 393,
	K_EECR = 394,
	K_FLASH_INSTR = 395,
	K_EEPROM_INSTR = 396,
	TKN_COMMA = 397,
	TKN_EQUAL = 398,
	TKN_SEMI = 399,
	TKN_TILDE = 400,
	TKN_NUMBER = 401,
	TKN_STRING = 402,
	TKN_ID = 403
};

char default_programmer[MAX_STR_CONST];
char default_parallel[4096];
char default_serial[4096];

char string_buf[MAX_STR_CONST];
char *string_buf_ptr;

PROGRAMMER * current_prog;
AVRPART    * current_part;

int    lineno;
const char * infile;
extern char string_buf[MAX_STR_CONST];
extern char *string_buf_ptr;

#ifdef __cplusplus
extern "C" {
#endif

int init_config(void);
TOKEN * new_token(int primary);
void free_token(TOKEN * tkn);
void free_tokens(int n, ...);
TOKEN * number(char * text);
TOKEN * hexnumber(char * text);
TOKEN * string(char * text);

#ifdef __cplusplus
}
#endif

#endif
