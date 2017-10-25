/* $Id: archive.h,v 1.1 2002/02/05 13:19:46 gemvx Exp $ */
/*+
 *
 * FILENAME
 * -------- 
 * archive.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for archive.c
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
 * 16-Dec-1999: Added global variables.
 *
 */
/* ===================================================================== */
#ifndef _INCLUDED_ARCHIVE_H
#define _INCLUDED_ARCHIVE_H

#ifndef _INCLUDED_CADRECORD_H
#define _INCLUDED_CADRECORD_H
#include <cadRecord.h>
#endif

#ifndef _INCLUDED_DBACCESS_H
#define _INCLUDED_DBACCESS_H
#include <dbAccess.h>   /* For DBADDR */
#endif

#include "control.h"    /* For m2History */

/* Public functions */

int readArchive (m2History * archivePtr, double targetTime);

/* Returns true if logging is armed, otherwise false. */
int isLoggingArmed(void);

long CADlog (struct cadRecord * pcad);

void showArchive (void);

void loggerTask (void);

int cadDirLog (char *cadName, int directive, int argc, struct cadRecord * pcad);

long scsLogDestruct(void);


/* Global variables */

extern int loggingNow;
extern int logThreshold;
extern epicsMutexId refMemFree;
extern epicsEventId logNow;
extern DBADDR logCAddr;

#endif /* _INCLUDED_ARCHIVE_H */
