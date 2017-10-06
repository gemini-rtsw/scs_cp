/* $Id: house.h,v 1.1 2002/02/05 13:19:49 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * house.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for house.c
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
#ifndef _INCLUDED_HOUSE_H
#define _INCLUDED_HOUSE_H

#ifndef _INCLUDED_CADRECORD_H
#define _INCLUDED_CADRECORD_H
#include <cadRecord.h>
#endif

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif

long CADtest(struct cadRecord * pcad);

long CADinit(struct cadRecord * pcad);

long CADreboot(struct cadRecord * pcad);

long CADdatum(struct cadRecord * pcad);

long CADsimulate(struct cadRecord * pcad);

long carDrive1(struct genSubRecord * pgsub);

long carDrive2(struct genSubRecord * pgsub);

#endif

