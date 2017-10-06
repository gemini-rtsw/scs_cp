/* $Id: config.h,v 1.2 2005/07/12 20:03:56 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * config.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for config.c. 
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
 * 18-Jan-2000: Added CADdriveFollower 
 * 06-Mar-2000: Added other CADdrive* functions
 * 06-Mar-2000: Added CADtilt- and CADfocus- PidControl
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_CONFIG_H
#define _INCLUDED_CONFIG_H

#ifndef _INCLUDED_CADRECORD_H
#define _INCLUDED_CADRECORD_H
#include <cadRecord.h>
#endif

/* Public functions */
/* 06-Mar-00: I don't see any of these used outside of config.c
   Do they need to be global because they are part of record support?
*/
long CADmoveBaffle(struct cadRecord* pcad);
long CADmovePeriscope(struct cadRecord* pcad);
long CADservoBandwidth(struct cadRecord* pcad);
long dummyInit(struct cadRecord* pcad);
long CADcontroller(struct cadRecord* pcad);
long CADtolerance(struct cadRecord* pcad);
long CADdebug(struct cadRecord* pcad);
long CADdriveFollower(struct cadRecord* pcad);
long CADdriveOffloader(struct cadRecord* pcad);
long CADdriveDB(struct cadRecord* pcad);
long CADdriveCB(struct cadRecord* pcad);
long CADdriveXY(struct cadRecord* pcad);
long CADtiltPidControl(struct cadRecord* pcad);
long CADfocusPidControl(struct cadRecord* pcad);

/* Global variables */
extern char *periscopeOption[];
extern char *cenBaffle[];
extern char *depBaffle[];

#endif
