/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2003-2004  Brian S. Dean <bsd@bsdhome.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* $Id: avrpart.h 916 2010-01-15 16:36:13Z joerg_wunsch $ */

#ifndef avrpart_h
#define avrpart_h

#include <limits.h>

/*
 * AVR serial programming instructions
 */
enum {
  AVR_OP_READ,
  AVR_OP_WRITE,
  AVR_OP_READ_LO,
  AVR_OP_READ_HI,
  AVR_OP_WRITE_LO,
  AVR_OP_WRITE_HI,
  AVR_OP_LOADPAGE_LO,
  AVR_OP_LOADPAGE_HI,
  AVR_OP_LOAD_EXT_ADDR,
  AVR_OP_WRITEPAGE,
  AVR_OP_CHIP_ERASE,
  AVR_OP_PGM_ENABLE,
  AVR_OP_MAX
};


enum {
  AVR_CMDBIT_IGNORE,  /* bit is ignored on input and output */
  AVR_CMDBIT_VALUE,   /* bit is set to 0 or 1 for input or output */
  AVR_CMDBIT_ADDRESS, /* this bit represents an input address bit */
  AVR_CMDBIT_INPUT,   /* this bit is an input bit */
  AVR_CMDBIT_OUTPUT   /* this bit is an output bit */
};

enum { /* these are assigned to reset_disposition of AVRPART */
  RESET_DEDICATED,    /* reset pin is dedicated */
  RESET_IO            /* reset pin might be configured as an I/O pin */
};

enum ctl_stack_t {
  CTL_STACK_NONE,     /* no control stack defined */
  CTL_STACK_PP,	      /* parallel programming control stack */
  CTL_STACK_HVSP      /* high voltage serial programming control stack */
};

/*
 * serial programming instruction bit specifications
 */
typedef struct cmdbit {
  int          type;  /* AVR_CMDBIT_* */
  int          bitno; /* which input bit to use for this command bit */
  int          value; /* bit value if type == AVR_CMDBIT_VALUD */
} CMDBIT;

typedef struct opcode {
  CMDBIT        bit[32]; /* opcode bit specs */
} OPCODE;


#define AVRPART_SERIALOK       0x0001  /* part supports serial programming */
#define AVRPART_PARALLELOK     0x0002  /* part supports parallel programming */
#define AVRPART_PSEUDOPARALLEL 0x0004  /* part has pseudo parallel support */
#define AVRPART_HAS_JTAG       0x0008  /* part has a JTAG i/f */
#define AVRPART_ALLOWFULLPAGEBITSTREAM 0x0010 /* JTAG ICE mkII param. */
#define AVRPART_ENABLEPAGEPROGRAMMING 0x0020 /* JTAG ICE mkII param. */
#define AVRPART_HAS_DW         0x0040  /* part has a debugWire i/f */
#define AVRPART_HAS_PDI        0x0080  /* part has PDI i/f rather than ISP (ATxmega) */
#define AVRPART_AVR32          0x0100  /* part is in AVR32 family */
#define AVRPART_INIT_SMC       0x0200  /* part will undergo chip erase */
#define AVRPART_WRITE          0x0400  /* at least one write operation specified */
#define AVRPART_HAS_TPI        0x0800  /* part has TPI i/f rather than ISP (ATtiny4/5/9/10) */

#define AVR_DESCLEN 64
#define AVR_IDLEN   32
#define CTL_STACK_SIZE 32
#define FLASH_INSTR_SIZE 3
#define EEPROM_INSTR_SIZE 20

#define TAG_ALLOCATED          1    /* memory byte is allocated */

#define AVR_MEMDESCLEN 64
typedef struct avrmem {
  char desc[AVR_MEMDESCLEN];  /* memory description ("flash", "eeprom", etc) */
  int paged;                  /* page addressed (e.g. ATmega flash) */
  int size;                   /* total memory size in bytes */
  int page_size;              /* size of memory page (if page addressed) */
  int num_pages;              /* number of pages (if page addressed) */
  unsigned int offset;        /* offset in IO memory (ATxmega) */
  int min_write_delay;        /* microseconds */
  int max_write_delay;        /* microseconds */
  int pwroff_after_write;     /* after this memory type is written to,
                                 the device must be powered off and
                                 back on, see errata
                                 http://www.atmel.com/atmel/acrobat/doc1280.pdf */
  unsigned char readback[2];  /* polled read-back values */

  int mode;                   /* stk500 v2 xml file parameter */
  int delay;                  /* stk500 v2 xml file parameter */
  int blocksize;              /* stk500 v2 xml file parameter */
  int readsize;               /* stk500 v2 xml file parameter */
  int pollindex;              /* stk500 v2 xml file parameter */

  unsigned char * buf;        /* pointer to memory buffer */
  unsigned char tags[1024*1024];       /* allocation tags */
  OPCODE * op[AVR_OP_MAX];    /* opcodes */
} AVRMEM;

typedef struct avrpart {
  char          desc[AVR_DESCLEN];  /* long part name */
  char          id[AVR_IDLEN];      /* short part name */
  int           stk500_devcode;     /* stk500 device code */
  int           avr910_devcode;     /* avr910 device code */
  int           chip_erase_delay;   /* microseconds */
  unsigned char pagel;              /* for parallel programming */
  unsigned char bs2;                /* for parallel programming */
  unsigned char signature[3];       /* expected value of signature bytes */
  int           reset_disposition;  /* see RESET_ enums */
  int           retry_pulse;        /* retry program enable by pulsing
                                       this pin (PIN_AVR_*) */
  unsigned      flags;              /* see AVRPART_ masks */

  int           timeout;            /* stk500 v2 xml file parameter */
  int           stabdelay;          /* stk500 v2 xml file parameter */
  int           cmdexedelay;        /* stk500 v2 xml file parameter */
  int           synchloops;         /* stk500 v2 xml file parameter */
  int           bytedelay;          /* stk500 v2 xml file parameter */
  int           pollindex;          /* stk500 v2 xml file parameter */
  unsigned char pollvalue;          /* stk500 v2 xml file parameter */
  int           predelay;           /* stk500 v2 xml file parameter */
  int           postdelay;          /* stk500 v2 xml file parameter */
  int           pollmethod;         /* stk500 v2 xml file parameter */

  enum ctl_stack_t ctl_stack_type;  /* what to use the ctl stack for */
  unsigned char controlstack[CTL_STACK_SIZE]; /* stk500v2 PP/HVSP ctl stack */
  unsigned char flash_instr[FLASH_INSTR_SIZE]; /* flash instructions (debugWire, JTAG) */
  unsigned char eeprom_instr[EEPROM_INSTR_SIZE]; /* EEPROM instructions (debugWire, JTAG) */

  int           hventerstabdelay;   /* stk500 v2 hv mode parameter */
  int           progmodedelay;      /* stk500 v2 hv mode parameter */
  int           latchcycles;        /* stk500 v2 hv mode parameter */
  int           togglevtg;          /* stk500 v2 hv mode parameter */
  int           poweroffdelay;      /* stk500 v2 hv mode parameter */
  int           resetdelayms;       /* stk500 v2 hv mode parameter */
  int           resetdelayus;       /* stk500 v2 hv mode parameter */
  int           hvleavestabdelay;   /* stk500 v2 hv mode parameter */
  int           resetdelay;         /* stk500 v2 hv mode parameter */
  int           chiperasepulsewidth; /* stk500 v2 hv mode parameter */
  int           chiperasepolltimeout; /* stk500 v2 hv mode parameter */
  int           chiperasetime;      /* stk500 v2 hv mode parameter */
  int           programfusepulsewidth; /* stk500 v2 hv mode parameter */
  int           programfusepolltimeout; /* stk500 v2 hv mode parameter */
  int           programlockpulsewidth; /* stk500 v2 hv mode parameter */
  int           programlockpolltimeout; /* stk500 v2 hv mode parameter */
  int           synchcycles;        /* stk500 v2 hv mode parameter */
  int           hvspcmdexedelay;    /* stk500 v2 xml file parameter */

  unsigned char idr;                /* JTAG ICE mkII XML file parameter */
  unsigned char rampz;              /* JTAG ICE mkII XML file parameter */
  unsigned char spmcr;              /* JTAG ICE mkII XML file parameter */
  unsigned short eecr;              /* JTAC ICE mkII XML file parameter */
  unsigned int nvm_base;            /* Base address of NVM controller in ATxmega devices */

  OPCODE      * op[AVR_OP_MAX];     /* opcodes */

  AVRMEM		*sigmem;
  AVRMEM		*flashmem;
  AVRMEM		*lfusemem;
  AVRMEM		*hfusemem;
  AVRMEM		*fusesmem;
	AVRMEM		*lockmem;
  char          config_file[4096]; /* config file where defined */
  int           lineno;                /* config file line number */
} AVRPART;

#ifdef __cplusplus
extern "C" {
#endif

/* Functions for OPCODE structures */
OPCODE * avr_new_opcode(void);
int avr_set_bits(OPCODE * op, unsigned char * cmd);
int avr_set_addr(OPCODE * op, unsigned char * cmd, unsigned long addr);
int avr_set_input(OPCODE * op, unsigned char * cmd, unsigned char data);
int avr_get_output(OPCODE * op, unsigned char * res, unsigned char * data);

/* Functions for AVRMEM structures */
AVRMEM * avr_new_memtype(void);
int avr_initmem(AVRPART * p);
AVRMEM * avr_dup_mem(AVRMEM * m);
AVRMEM * avr_locate_mem(AVRPART * p, char * desc);

/* Functions for AVRPART structures */
AVRPART * avr_new_part(void);
AVRPART * avr_dup_part(AVRPART * d);
typedef void (*walk_avrparts_cb)(const char *name, const char *desc,
                                 const char *cfgname, int cfglineno,
                                 void *cookie);

#ifdef __cplusplus
}
#endif

#endif /* avrpart_h */
