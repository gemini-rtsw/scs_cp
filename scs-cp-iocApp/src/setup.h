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
 * 05-Dec-2017: Begin EPICS OSI conversion (mdw)
 *              Removed #include <elgLib.h> (mdw)
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_SETUP_H
#define _INCLUDED_SETUP_H

#include "utilities.h"

/* Public functions */
int scsInit(void);

/* Global variables */

#endif
