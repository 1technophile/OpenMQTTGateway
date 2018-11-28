/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * avrdude is Copyright (C) 2000-2004  Brian S. Dean <bsd@bsdhome.com>
 *
 * This file: Copyright (C) 2005-2007 Colin O'Flynn <coflynn@newae.com>
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

#include "../pilight/core/log.h"
#include "avrdude.h"
#include "avr.h"
#include "pgm.h"
#include "safemode.h"

/* This value from ac_cfg.h */
/*
 * Writes the specified fuse in fusename (can be "lfuse", "hfuse", or
 * "efuse") and verifies it. Will try up to tries amount of times
 * before giving up
 */
int safemode_writefuse (unsigned char fuse, const char * fusename, PROGRAMMER * pgm,
                        AVRPART * p, int tries)
{
  AVRMEM * m = NULL;
  unsigned char fuseread;
  int returnvalue = -1;

  // m = avr_locate_mem(p, fusename);
  if (m == NULL) {
    return -1;
    }

  /* Keep trying to write then read back the fuse values */
  while (tries > 0) {
    if (avr_write_byte(pgm, p, m, 0, fuse) != 0) {
        continue;
    }
    if (pgm->read_byte(pgm, p, m, 0, &fuseread) != 0) {
        continue;
    }

    /* Report information to user if needed */
      logprintf(LOG_ERR, "AVR wrote %s to %x, read as %x. %d attempts left", fusename, fuse, fuseread, tries-1);

    /* If fuse wrote OK, no need to keep going */
    if (fuse == fuseread) {
       tries = 0;
       returnvalue = 0;
    }
    tries--;
  }

  return returnvalue;
}

/*
 * Reads the fuses three times, checking that all readings are the
 * same. This will ensure that the before values aren't in error!
 */
int safemode_readfuses (unsigned char * lfuse, unsigned char * hfuse,
                        unsigned char * efuse, unsigned char * fuse,
                        PROGRAMMER * pgm, AVRPART * p)
{

  unsigned char value;
  unsigned char fusegood = 0;
  unsigned char allowfuseread = 1;
  unsigned char safemode_lfuse;
  unsigned char safemode_hfuse;
  unsigned char safemode_efuse;
  unsigned char safemode_fuse;
  AVRMEM * m = NULL;

  safemode_lfuse = *lfuse;
  safemode_hfuse = *hfuse;
  safemode_efuse = *efuse;
  safemode_fuse  = *fuse;


  /* Read fuse three times */
  // fusegood = 2; /* If AVR device doesn't support this fuse, don't want
                   // to generate a verify error */
  // m = avr_locate_mem(p, "fuse");
  // if (m != NULL) {
    // fusegood = 0; /* By default fuse is a failure */
    // if(pgm->read_byte(pgm, p, m, 0, &safemode_fuse) != 0)
        // {
        // allowfuseread = 0;
        // }
    // if(pgm->read_byte(pgm, p, m, 0, &value) != 0)
        // {
        // allowfuseread = 0;
        // }
    // if (value == safemode_fuse) {
        // if (pgm->read_byte(pgm, p, m, 0, &value) != 0)
            // {
            // allowfuseread = 0;
            // }
        // if (value == safemode_fuse)
            // {
            // fusegood = 1; /* Fuse read OK three times */
            // }
    // }
  // }

	// //Programmer does not allow fuse reading.... no point trying anymore
    // if (allowfuseread == 0)
		// {
		// return -5;
		// }

    // if (fusegood == 0)	 {
        // logprintf(LOG_ERR, "AVR verify error, unable to read fuse properly.");
		// logprintf(LOG_ERR, "Programmer may not be reliable.");
        // return -1;
    // }
    // else if ((fusegood == 1)) {
		// logprintf(LOG_INFO, "AVR fuse reads as %X", safemode_lfuse);
    // }


  /* Read lfuse three times */
  fusegood = 2; /* If AVR device doesn't support this fuse, don't want
                   to generate a verify error */
  m = p->lfusemem;
  if (m != NULL) {
    fusegood = 0; /* By default fuse is a failure */
    if (pgm->read_byte(pgm, p, m, 0, &safemode_lfuse) != 0) {
        allowfuseread = 0;
	}
    if (pgm->read_byte(pgm, p, m, 0, &value) != 0) {
        allowfuseread = 0;
	}
    if (value == safemode_lfuse) {
        if (pgm->read_byte(pgm, p, m, 0, &value) != 0) {
            allowfuseread = 0;
		}
        if (value == safemode_lfuse) {
        fusegood = 1; /* Fuse read OK three times */
        }
    }
  }

	//Programmer does not allow fuse reading.... no point trying anymore
    if (allowfuseread == 0) {
		return -5;
	}


    if (fusegood == 0)	 {
        logprintf(LOG_ERR, "AVR verify error, unable to read lfuse properly.");
		logprintf(LOG_ERR, "Programmer may not be reliable.");
        return -1;
    }
    else if (fusegood == 1) {
		logprintf(LOG_INFO, "AVR lfuse reads as %X", safemode_lfuse);
    }

  /* Read hfuse three times */
  fusegood = 2; /* If AVR device doesn't support this fuse, don't want
                   to generate a verify error */
  m = p->hfusemem;
  if (m != NULL) {
    fusegood = 0; /* By default fuse is a failure */
    if (pgm->read_byte(pgm, p, m, 0, &safemode_hfuse) != 0) {
        allowfuseread = 0;
	}
    if (pgm->read_byte(pgm, p, m, 0, &value) != 0) {
        allowfuseread = 0;
	}
    if (value == safemode_hfuse) {
        if (pgm->read_byte(pgm, p, m, 0, &value) != 0) {
            allowfuseread = 0;
		}
        if (value == safemode_hfuse) {
             fusegood = 1; /* Fuse read OK three times */
        }
    }
  }

	//Programmer does not allow fuse reading.... no point trying anymore
    if (allowfuseread == 0) {
		return -5;
	}

    if (fusegood == 0)	 {
        logprintf(LOG_ERR, "AVR verify error, unable to read hfuse properly.");
		logprintf(LOG_ERR, "Programmer may not be reliable.");
        return -1;
    }
    else if (fusegood == 1) {
		logprintf(LOG_INFO, "AVR hfuse reads as %X", safemode_lfuse);
    }

  /* Read efuse three times */
  fusegood = 2; /* If AVR device doesn't support this fuse, don't want
                   to generate a verify error */
  // m = avr_locate_mem(p, "efuse");
  // if (m != NULL) {
    // fusegood = 0; /* By default fuse is a failure */
    // if (pgm->read_byte(pgm, p, m, 0, &safemode_efuse) != 0)
        // {
        // allowfuseread = 0;
        // }
    // if (pgm->read_byte(pgm, p, m, 0, &value) != 0)
        // {
        // allowfuseread = 0;
        // }
    // if (value == safemode_efuse) {
        // if (pgm->read_byte(pgm, p, m, 0, &value) != 0)
            // {
            // allowfuseread = 0;
            // }
        // if (value == safemode_efuse){
             // fusegood = 1; /* Fuse read OK three times */
        // }
    // }
  // }

	// //Programmer does not allow fuse reading.... no point trying anymore
    // if (allowfuseread == 0)
		// {
		// return -5;
		// }

    // if (fusegood == 0)	 {
        // logprintf(LOG_ERR, "AVR verify error, unable to read efuse properly.");
		// logprintf(LOG_ERR, "Programmer may not be reliable.");
        // return -1;
    // }
    // else if ((fusegood == 1)) {
		// logprintf(LOG_INFO, "AVR efuse reads as %X", safemode_lfuse);
    // }

  *lfuse = safemode_lfuse;
  *hfuse = safemode_hfuse;
  *efuse = safemode_efuse;
  *fuse  = safemode_fuse;

  return 0;
}


/*
 * This routine will store the current values pointed to by lfuse,
 * hfuse, and efuse into an internal buffer in this routine when save
 * is set to 1. When save is 0 (or not 1 really) it will copy the
 * values from the internal buffer into the locations pointed to be
 * lfuse, hfuse, and efuse. This allows you to change the fuse bits if
 * needed from another routine (ie: have it so if user requests fuse
 * bits are changed, the requested value is now verified
 */
int safemode_memfuses (int save, unsigned char * lfuse, unsigned char * hfuse,
                       unsigned char * efuse, unsigned char * fuse)
{
  static unsigned char safemode_lfuse = 0xff;
  static unsigned char safemode_hfuse = 0xff;
  static unsigned char safemode_efuse = 0xff;
  static unsigned char safemode_fuse = 0xff;

  switch (save) {

    /* Save the fuses as safemode setting */
    case 1:
        safemode_lfuse = *lfuse;
        safemode_hfuse = *hfuse;
        safemode_efuse = *efuse;
        safemode_fuse  = *fuse;

        break;
    /* Read back the fuses */
    default:
        *lfuse = safemode_lfuse;
        *hfuse = safemode_hfuse;
        *efuse = safemode_efuse;
        *fuse  = safemode_fuse;
        break;
  }

  return 0;
}
