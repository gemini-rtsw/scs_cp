/* $Id: m2Log.h,v 1.1 2002/02/05 13:19:49 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * m2Log.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for m2Log.c.  How many
 * damn log methods does this thing need?
 * 
 * FUNCTION NAME(S)
 * ----------------
 * 
 * DEPENDENCIES
 * ------------
 *
 * LIMITATIONS
 * -----------
 * 
 * AUTHOR
 * ------
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * HISTORY
 * -------
 * 17-Nov-1999: Created new header files. KG
 * 16-Nov-1999: Added global variables
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_M2LOG_H
#define _INCLUDED_M2LOG_H

#include "control.h"      /* For memMap */
#include "utilities.h"    /* For FALSE */

/* Public functions */

long m2LogInit(void);

long m2LogClose(void);

long m2LoggerTask(memMap *ptr);

long m2CircBufferTask(memMap *ptr);

/* Global variables */

extern int m2LogActive;

#endif



