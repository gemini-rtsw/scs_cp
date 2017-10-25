/* ===================================================================== */
/* INDENT OFF */
/*
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
 * 24-Oct-2017: Begin conversion to EPICS OSI (mdw)
 *
 */
/* ===================================================================== */
#include <stdio.h>

#include <epicsThread.h>

#include "chopControl.h"
#include "interlock.h"  /* For eventConnect */
#include "utilities.h"  /* For errorLog, debugLevel */
//#include "xycom.h"      /* For eventHandler */


/* Declare external variables */

epicsEventId chopEventSem;

/* Semaphore given in scs_st.stpp in startInit and startChopControl states */
epicsEventId eventSem; 

/* ===================================================================== */
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

/* ===================================================================== */


int chopInit (void)
{
   epicsThreadId tid;

   /* Check for presence of Xycom, set up I/O and intr addresses and
    * set up semaphore to keep track of interrupts on the Xycom      */

   if (xyInit() == OK)
   {
       eventConnect = ON;
   }
   else
   {
      eventConnect = OFF;
      errlogMessage ("unable to init the Xycom\n");
      return (ERROR);
   }

   /* Create the event semaphore which keeps track of when eventConfig
    * changes */

   eventSem = epicsEventMustCreate(epicsEventEmpty);

   /* Spawn the task to watch for the event semaphore to be given */
   if (!(tid = epicsThreadCreate("tWatchEvents", epicsThreadPriorityMedium,
                           epicsThreadGetStackSize(epicsThreadStackMedium), 
                           eventHandler, NULL)))
   {
      errlogMessage("Cannot watch for events\n");
      return ERROR;
   }

   /* Create semaphore to trigger tasks */
   chopEventSem = epicsEventMustCreate(epicsEventEmpty)

   return (OK);
}

