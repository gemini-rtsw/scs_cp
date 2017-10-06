/* $Id: tiltSim.h,v 1.1 2002/02/05 13:19:51 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * tiltSim.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for tiltSim.c.
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
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_TILTSIM_H
#define _INCLUDED_TILTSIM_H

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif

/* Define chop synchronisation internal and external constants */

#define INTERNAL        0
#define EXTERNAL        1

/* Public functions */

long mechSim(struct genSubRecord* pgsub);

#endif

