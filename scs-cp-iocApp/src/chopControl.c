/* $Id: chopControl.c,v 1.1 2002/02/05 13:19:47 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * chopControl.c
 * 
 * PURPOSE
 * -------
 * Functions to initialise chop for the Gemini secondary control system
 * 
 * FUNCTION NAME(S)
 * ----------------
 * chopInit             -- initialise chop control
 * 
 * DEPENDENCIES
 * ------------
 *
 * LIMITATIONS
 * -----------
 * 
 * AUTHOR
 * ------
 * Magnus Paterson      (mjp@roe.ac.uk)
 * 
 * HISTORY
 * -------
 * 
 * 19-Jul-1997: Original (mjp)
 * 02-Feb-1998: Convert error reporting from logMsg to errorLog function (srp)
 * 30-Jun-1998: Check for presence of xycom card with vxMemProbe (srp)
 * 07-May-1999: Added RCS id 
 * 13-Jun-2000: Gutted and redone
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include "chopControl.h"
#include "interlock.h"  /* For eventConnect */
#include "utilities.h"  /* For errorLog, debugLevel */
#include "xycom.h"      /* For eventHandler */

#include <taskLib.h>    /* For spawning and deleting tasks */
#include <sysLib.h>
#include <stdio.h>

/* Declare external variables */

SEM_ID chopEventSem = NULL;

/* Semaphore given in scs_st.stpp in startInit and startChopControl states */

SEM_ID eventSem = NULL; 

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * chopInit
 * 
 * 
 * Purpose:
 * Initialises variables associated with chopping.
 * 
 * Invocation:
 * r chopInit()
 * 
 * Parameters in:
 * 
 * Parameters out:
 * 
 * Return value:
 * 
 * Globals: 
 *
 * External functions:
 * 
 * External variables:
 * eventConnect
 * 
 * Requirements:
 * 
 * Author:
 * Magnus Paterson    (mjp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(mjp)
 * 30-Jun-1998: Check for presence of xycom card with vxmemprobe (srp)
 * 12-Jun-2000: Take out Xycom setup and do in xycom.c 
 * 13-Jun-2000: Do new stuff here...
 */

/* INDENT ON */
/* ===================================================================== */


int    chopInit (void)
{
    int tid;

    /* Check for presence of Xycom, set up I/O and intr addresses and
     * set up semaphore to keep track of interrupts on the Xycom      */

    if (xyInit() == OK)
    {
        eventConnect = ON;
    }
    else
    {
	eventConnect = OFF;
	printf ("unable to init the Xycom\n");
	return (ERROR);
    }

    /* Create the event semaphore which keeps track of when eventConfig
     * changes */

    if ((eventSem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
    {
        printf ("unable to create eventSem sem\n");
	return (ERROR);
    } 

    /* Spawn the task to watch for the event semaphore to be given */
    if ((tid = taskSpawn ("tWatchEvents", 90, 0, 5000, (FUNCPTR)eventHandler,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) == ERROR) 
    {
        printf ("Cannot watch for evemts\n");
        return ERROR;
    }

    /* Create semaphore to trigger tasks */

    if ((chopEventSem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
        {
            printf ("unable to create chopEvent sem\n");
	    return (ERROR);
	}
    return (OK);
}

