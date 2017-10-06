/* $Id: setup.h,v 1.1 2002/02/05 13:19:51 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * setup.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for setup.c
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
#ifndef _INCLUDED_SETUP_H
#define _INCLUDED_SETUP_H

#include "elgLib.h"

#ifndef _INCLUDED_SEMLIB_H
#define _INCLUDED_SEM_LIB_H
#include <semLib.h>
#endif

/* Public functions */

int scsInit(void);

/* Global variables */

extern MSG_Q_ID healthQId;
extern SEM_ID scsReady;

#endif
