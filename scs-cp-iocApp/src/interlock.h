/* $Id: interlock.h,v 1.1 2002/02/05 13:19:49 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * interlock.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for interlock.c
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
 * 17-Nov-1999: Created new header files.
 * 16-Dec-1999: Added global variables
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_INTERLOCK_H
#define _INCLUDED_INTERLOCK_H

#ifndef _INCLUDED_SUBRECORD_H
#define _INCLUDED_SUBRECORD_H
#include <subRecord.h>
#endif

#include "utilities.h"

/* Public functions */

long initInterlock (struct subRecord * psub);

long lockMonitor (struct subRecord * psub);

/* Global variables */

extern location lockPosition;
extern long scsState;
extern int eventConnect;

#endif



