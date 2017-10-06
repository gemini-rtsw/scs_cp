/* $Id: dmDrive.h,v 1.1 2002/02/05 13:19:48 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * dmDrive.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for dmDrive.c.
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
 * 15-Dec-1999: Added real2Drive  KDK
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_DMDRIVE_H
#define _INCLUDED_DMDRIVE_H

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif

/* Public functions */

long realDrive(struct genSubRecord* pgsub);

long real2Drive(struct genSubRecord* pgsub);

long displayScs(struct genSubRecord* pgsub);

long statusDrive(struct genSubRecord* pgsub);

#endif

