/* $Id: showEngineering.h,v 1.1 2002/02/05 13:19:51 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * showEngineering.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for showEngineering.c.
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
#ifndef _INCLUDED_SHOWENGINEERING_H
#define _INCLUDED_SHOWENGINEERING_H

#ifndef _INCLUDED_CADRECORD_H
#define _INCLUDED_CADRECORD_H
#include <cadRecord.h>
#endif

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif

/* Public functions */

long readM2Diagnostics(struct genSubRecord* pgsub);

long gensubFanDoubles(struct genSubRecord* pgsub);

long issueM2Primitive(struct cadRecord* pcad);

long fillDiagnostics(double seed);

#endif
