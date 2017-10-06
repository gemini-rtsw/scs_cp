/* $Id: interlock.c,v 1.1 2002/02/05 13:19:49 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * interlock.c
 * 
 * PURPOSE
 * -------
 * Functions to handle the receipt of an interlock from the Gemini
 * interlock system
 * 
 * FUNCTION NAME(S)
 * ----------------
 * initInterlock    - dummy INAM routine
 * lockMonitor      - read current interlock status. If active, clear
 *            the message queue
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
 * 
 * 19-Jul-1997: Original (srp)
 * 07-May-1999: Added RCS id
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include "archive.h"        /* For refMemFree */
#include "xycom.h"          /* For PORT_3_ADDR define */
#include "control.h"        /* For scsPtr, interlockFlag, commandQId */
#include "interlock.h"
#include "utilities.h"      /* For reportHealth */

#include <logLib.h>

/* Define interlock port masks */

#define INTERLOCKMASK   3
#define DEMANDCLEAR     2
#define DEMANDSET       1

/* Declare external variables */

location lockPosition;
long scsState = SYSTEMIDLE;
int eventConnect = OFF;

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * initInterlock
 * lockMonitor
 * 
 * Purpose:
 * At the scan rate of the record the interlock flag is polled. If it is set
 * a global flag is set to lock out further commands. Any commands which may
 * be currently queued are then cleared out. If an interlock demand is not
 * present the global flag is set to not active
 * If the xycom card interfacing to the interlock system is present a global
 * flag 'eventConnect' is set to one. If this is NOT overridden the interlock
 * state is then read from the card. If interlocks are overriden then the port
 * A setting is used.
 * 
 * Invocation:
 * 
 * Parameters in:
 *      psub->a long    interlock state request (1 = active, 0 = not active)
 *      psub->b long    interlock override (1 = ignore xycom card, 0 read xycom card)
 *  
 * Parameters out:
 * 
 * Return value:
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      eventConnect    >   int if set to 1, read from event system
 *      port3addr       !   char*   pointer to flag register on xycom card
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 26-Jan-1998: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

/*
long dummyInitPsub (struct subRecord * psub)
{
    return (OK);
}
*/

long    initInterlock (struct subRecord * psub)
{
    return (OK);
}

long    lockMonitor (struct subRecord * psub)
{
    long    received;
    static int Qcleared = 0;
    long    interlockStatus = OFF;
    long    interlockOverride = OFF;

    /* read current interlock status from the event system   */
    /* for test purposes make this input port A      */

    interlockStatus = (long) psub->a;
    interlockOverride = (long)psub->b;

    if(interlockOverride == OFF)
    {
        if(eventConnect == ON)
        {
        if (( (*(unsigned char *) PORT_3_ADDR) & INTERLOCKMASK) == DEMANDCLEAR)
            interlockStatus = OFF;
        else if (( (*(unsigned char *) PORT_3_ADDR) & INTERLOCKMASK) 
		== DEMANDSET)
            interlockStatus = ON;
        else
        {
            interlockStatus = ON;
            reportHealth(WARNING, "interlock undefined - default to active");
        }
        }
    }

    if (interlockStatus == ON)
    {
        /* set interlockFlag to lockout further commands */

        interlockFlag = ON;

        if(semTake(refMemFree, SEM_TIMEOUT) == OK)
        {
            /* record current mirror position */

            lockPosition.xTilt = scsPtr->page1.xTilt;
            lockPosition.yTilt = scsPtr->page1.yTilt;
            lockPosition.zFocus = scsPtr->page1.zFocus;
            lockPosition.xPos = scsPtr->page1.xPosition;
            lockPosition.yPos = scsPtr->page1.yPosition;

            semGive(refMemFree);
        }
        else
        {
            logMsg("interlock monitor - refMemFree timeout\n", 0, 0, 0, 0, 0, 0);
        }

        /* clear command message queues */

        if (Qcleared == 0)
        {
            logMsg("interlock detected - begin clearing message queue", 0, 0, 0, 0, 0, 0);

            /* read out and discard messages until none left */

            while (msgQReceive (commandQId, (char *) &received, sizeof (long), NO_WAIT) != ERROR)
                ;

            Qcleared = 1;

            logMsg ("message queue cleared\n", 0, 0, 0, 0, 0, 0);
        }
    }
    else
    {
        /* set interlockFlag to enable commands again */

        Qcleared = 0;
        interlockFlag = OFF;
    }

    psub->val = (double)interlockFlag;

    return (OK);
}



















