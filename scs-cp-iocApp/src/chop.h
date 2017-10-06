/* $Id: chop.h,v 1.1 2002/02/05 13:19:47 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * chop.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for chop.c.  Methods
 * used in CAD records.
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
 * 12-Jun-2000: Move chopIsOn here from chopControl.h
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_CHOP_H
#define _INCLUDED_CHOP_H

#ifndef _INCLUDED_CADRECORD_H
#define _INCLUDED_CADRECORD_H
#include <cadRecord.h>
#endif

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif

#ifndef _INCLUDED_SUBRECORD_H
#define _INCLUDED_SUBRECORD_H
#include <subRecord.h>
#endif

#include "chopControl.h"

#define MAX_CHOP_CONTROLLERS    6    /* five instruments plus the SCS itself */

/* define structure to hold instrument configurations */

typedef struct
{
    char name[MAX_STRING_SIZE];       /* Name of the instrument to appear on 
                                         screens                             */
    int  port;                        /* Event bus port to which it is 
                                         connected (0 - 16)                  */
    char synchro[MAX_STRING_SIZE];    /* Is it connected to the synchro bus  */
    int  page;                        /* If on the synchro bus, which page is 
                                         it using                            */
    char capability[MAX_STRING_SIZE]; /* What chop capabilities does it have */
}instStructure;

/* Public functions */

long CADchopConfig (struct cadRecord * pcad);

long CADchopControl (struct cadRecord * pcad);

long CADbeamJog (struct cadRecord * pcad);

long calcEnvelope (struct genSubRecord * pgsub);

double percentCalc (struct subRecord * psub);

double getChopDuty (void);

double getChopFreq (void);

/* Global variables */

extern int chopIsOn;
extern int m2ChopResponseOK;
extern int chopIsPending;
extern int decsIsPending;
extern int jogBeam;
extern instStructure instruments[MAX_CHOP_CONTROLLERS];

#endif



