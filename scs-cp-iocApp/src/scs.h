/* $Id: scs.h,v 1.1 2002/02/05 13:19:50 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * scs.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for scs.c
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
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_SCS_H
#define _INCLUDED_SCS_H

#ifndef _INCLUDED_CADRECORD_H
#define _INCLUDED_CADRECORD_H
#include <cadRecord.h>
#endif

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif

/* Public functions */

long initFollowGenSub(struct genSubRecord* pgsub);

long dummyInitGenSub(struct genSubRecord* pgsub);

long receiveTcsDemand(struct genSubRecord* pgsub);

long ticker(struct genSubRecord* pgsub);

long CADstop(struct cadRecord* pcad);

long CADmove(struct cadRecord* pcad);

long CADfollow(struct cadRecord* pcad);

long CADactuators(struct cadRecord* pcad);

long CADpark(struct cadRecord* pcad);

void resetFirstFollowDemand(void);

#endif

