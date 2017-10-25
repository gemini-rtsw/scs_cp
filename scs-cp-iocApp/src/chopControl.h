/* $Id: chopControl.h,v 1.1 2002/02/05 13:19:47 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * chopControl.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for chopControl.c
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
 * 16-Dec-1999: Added global variables.
 * 04-Jan-2000: Imported declaration of flagOutReg here from utilities.h
 * 06-Jan-2000: Deleted chopModeChange declaration since never used
 * 12-Jun-2000: All Xycom setup from this file now done in xycom.h
 * 19-Oct-2017: Begin conversion to EPICS OSI (mdw)
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_CHOPCONTROL_H
#define _INCLUDED_CHOPCONTROL_H

#include "utilities.h"     /* for epicsEventId */

/* Define constants for event system configuration */ 

#define LMMS    0
#define INST    1

/* define chop configuration structure for setting up the event system */

typedef struct
{
    int     change;       /* set to 1 for change                          */
    int     onChange;     /* set to 1 to flag change in "on"              */
    int     on;           /* { OFF | ON }                                 */
    int     profile;      /* { TWOPOINT | THREEPOINT | TRIANGLE }         */
    int     drive;        /* { LMMS | INST }                              */
    int     source;       /* controlling instrument { SCS | .. | ICS4 }   */
    int     capability;   /* { STROBE | ONELINE | TWOLINE | ONESHOT }     */
    int     m2Power;      /* { OFF | ON }                                 */
    int     m2Servo;      /* { OFF | ON }                                 */
}configStructure;

enum
{
    STROBE = 0,
    ONELINE,
    TWOLINE,
    ONESHOT
};
 
enum                    
{
    SCS = 0,
    ICS0,
    ICS1,
    ICS2,
    ICS3,
    ICS4
};

/* Define chop profile index */

enum
{
    TWOPOINT = 0,
    THREEPOINT,
    TRIANGLE
};

/* Define chop beam position indices */

enum
{
    BEAMA = 0,
    BEAMB,
    BEAMC,
    A2BRAMP,
    B2ARAMP
};


/* Global variables */

extern epicsEventId chopEventSem;

/* Semaphore given in scs_st.stpp */
extern epicsEventId eventSem;

/* Event system configuration params */
extern configStructure eventConfig;

#endif

