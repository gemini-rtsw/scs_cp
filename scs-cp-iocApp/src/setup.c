/* $Id: setup.c,v 1.7 2015/05/01 00:04:02 mrippa Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * setup.c
 * 
 * PURPOSE
 * -------
 * 
 * 
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
 * 
 * 31-Mar-1998: Original (srp)
 * 23-Jun-1998: Create message queue for health reporting
 * 07-May-1999: Added RCS id
 * 05-Dec-2017: Begin conversion to EPICS OSI (mdw)
 *
 * oi
 */
/* INDENT ON */
/* ===================================================================== */

#include <stdlib.h>     /* for malloc() */
#include <string.h>
#include <stdio.h>

#include <dbAccess.h>   /* For dbNameToAddr */
#include <devSup.h>     /* for S_dev_???   */
#include <vmi5588.h>    /* FOr reflective memory card routines */

#include "setup.h"
#include "utilities.h"  /* For errorLog, loadInitFiles, compileStatus,
                           statusCompiled, doPvLoad, pvLoadComplete */
#include "guide.h"      /* For createFilter, setPointFree */
#include "archive.h"    /* For loggerTask, refMemFree, logCAddr */
#include "control.h"    /* For fireLoops, slowTransmit, scsReceive, scsPtr, 
                           scsBase, m2Ptr, m2MemFree, slowUpdate, wfsFree
                           diagnosticsAvailable, scsDataAvailable,
                           scsReceiveNow, commandQId, receiveQId 
                           SYSTEM_CLOCK_RATE */


#define TOP "m2:"

/* #define TRUESCSBASE     0xf0A00040      / * Base address of node 0 */
#define TRUESCSBASE     0xfAA00040      /* Base address of node 0 
                                           reflective memory card (scs)  */
/* #define TRUEM2BASE      0xf0600040      / * Base address of node 1 */
#define TRUEM2BASE      0xfA600040      /* Base address of node 1 
                                           reflective memory card (m2 )  */

/* Declare externals */

epicsMessageQueueId  healthQId = NULL;

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * scsInit
 * 
 * Purpose:
 * Create all semaphores and spawn all tasks for the SCS system except
 * for those related to the xycom and chopping, which are done in 
 * chopInit.
 * 
 * Invocation:
 * scsInit
 * 
 * Parameters in:
 * None
 *
 * Parameters out:
 * None
 * 
 * Return value:
 *  < status    int OK or ERROR
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      pwfs1Update
 *      pwfs1DataAvailable
 *      pwfs2Update
 *      pwfs2DataAvailable
 *      oiwfsUpdate
 *      oiwfsDataAvailable
 *      gaosUpdate
 *      gaosDataAvailable
 *      gyroUpdate
 *      gyroDataAvailable
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(srp)
 * 10-Feb-1998: Incorporate spawning of guide handling tasks
 * 05-Dec-2017: Removed scsReady semaphore creation code since the semaphore
 *              wasn't being used anywhere. (mdw)
 */

/* INDENT ON */
/* ===================================================================== */

int scsInit (void)
{
   int source = PWFS1;
   char pRecordName[MAX_STRING_SIZE];


   /* create semaphores to control pvload of initialisation files */
   doPvLoad = epicsEventMustCreate(epicsEventEmpty);
   pvLoadComplete = epicsEventMustCreate(epicsEventEmpty);

   /* get address of logging CAR record */

   strncpy (pRecordName, TOP "logC.IVAL", MAX_STRING_SIZE);

   if (dbNameToAddr (pRecordName, &logCAddr) != 0)
   {
      errlogPrintf("scsLogInit - unable to fetch address of logC CAR\n");
      return (ERROR);
   }

   /* mutex semaphore to prevent multiple access to guide data */
   for (source = PWFS1; source <= GYRO; source++)
   {
      wfsFree[source] = epicsMutexMustCreate();
   }

   /* create dummy filters for each of the guide sources */
   for (source = PWFS1; source <= GYRO; source++)
   {
      createFilter (source, OFF, 200.0, 20.0, 20.0, -2.0, -2.0, -2.0);
   }

   /* create mutex semaphore to protect the set point values */
   setPointFree = epicsMutexCreate();

   /* the vxWorks code had stacksize for all threads set to 20000, but epicsThreadStackBig is only 11000.
    * are such large stack sizes really needed? */

   /* spawn task to pvload initialisation data */
   epicsThreadMustCreate("tloadInit", epicsThreadPriorityMedium, 
                     epicsThreadGetStackSize(epicsThreadStackBig),
                     (EPICSTHREADFUNC)loadInitFiles, (void *)NULL);

   /* spawn control loop task */
   epicsThreadMustCreate("tslowTx",epicsThreadPriorityMedium,
                    epicsThreadGetStackSize(epicsThreadStackBig),
                    (EPICSTHREADFUNC)slowTransmit, (void *)NULL);

   /* Do in startup in order that xycom has already been initted.
    * Then, if TIME_LATENCY needed, everything w.r.t. port addresses will
    * have been defined */

   epicsThreadMustCreate("tprocGuides", epicsThreadPriorityHigh,
                    epicsThreadGetStackSize(epicsThreadStackBig),
                    (EPICSTHREADFUNC)processGuides, (void *)NULL);

#if 0
/* convert this to a thread that runs periodically */
   /* set up aux clock to and connect ISR */
   sysAuxClkRateSet (SYSTEM_CLOCK_RATE);
   sysAuxClkConnect ((FUNCPTR) fireLoops, 0);
   sysAuxClkEnable ();
#endif
   
   epicsThreadMustCreate("tfireLoops", epicsThreadPriorityHigh,
                   epicsThreadGetStackSize(epicsThreadStackSmall),
                   (EPICSTHREADFUNC)fireLoops, (void *)NULL);


   refMemFree = epicsMutexMustCreate();
   eventDataSem = epicsMutexMustCreate();
   m2MemFree = epicsMutexMustCreate();

   scsDataAvailable = epicsEventMustCreate(epicsEventEmpty);
   slowUpdate = epicsEventMustCreate(epicsEventEmpty);
   scsReceiveNow = epicsEventMustCreate(epicsEventEmpty);
   guideUpdateNow = epicsEventMustCreate(epicsEventEmpty);
   diagnosticsAvailable = epicsEventMustCreate(epicsEventEmpty);



/* These semaphores don't seem to be used anywhere  -- get rid of them. 20171019 MDW */
   // compileStatus = epicsEventMustCreate(epicsEventEmpty);
   // statusCompiled = epicsEventMustCreate(epicsEventEmpty);


   /* M2 simulation pointer always points to the buffer area */
   if ((m2Ptr = (memMap *) malloc (sizeof (memMap))) == NULL)
   {
      printf ("malloc fail on creation of m2Ptr buffer\n");
      return (ERROR);
   }

   if ((scsPtr = (memMap *) malloc (sizeof (memMap))) == NULL)
   {
      printf ("malloc fail on creation of scsPtr buffer\n");
      return (ERROR);
   }


   /* if no reflective memory card is present, create an equivalent buffer */
   if(rmStatus(0) != S_dev_NoInit) {
      scsBase = (memMap *) rmPageMemBase();
      printf("Reflective memory available\n");
   }
   else
   {
      printf("Reflective memory card not available\n");

      if ((scsBase = (memMap *) malloc (sizeof (memMap))) == NULL)    
      {
         errlogMessage("malloc fail on creation of scsBase buffer\n");
         return (ERROR);
      }
   }


   printf ("initRefMem, scsPtr = 0x%p, scsBase = 0x%p\n",  scsPtr, scsBase); 

   /* create command message queue */

   if ((commandQId = epicsMessageQueueCreate(100, sizeof (long))) == NULL)
   {
      errorLog ("initRefMem(): error in creation of commandQId message queue", 1, ON);
   }
   else
   {
      printf ("iinitRefMem(): commandQId message queue created successfully\n");
   }

   /* create command receiving queue for the simulation */

   if ((receiveQId = epicsMessageQueueCreate(100, sizeof (long))) == NULL)
   {
      errorLog ("initRefMem():  error in creation of receiveQId message queue", 1, ON);
   }
   else
   {
      printf ("initRefMem(): receiveQId message queue created successfully\n");
   }

   /* create health message queue */

   if ((healthQId = epicsMessageQueueCreate(100, sizeof (healthReport))) == NULL)
   {
      errorLog ("initRefMem(): error in creation of healthiQId  message queue", 1, ON);
   }
   else
   {
      printf ("initRefMem(): healthiQId  message queue created successfully\n");
   }

   /* clear buffers */

   epicsMutexLock(refMemFree);
   memset ((void *) scsPtr, 0, sizeof (memMap));
   epicsMutexUnlock(refMemFree);

   epicsMutexLock(m2MemFree);
   memset ((void *) m2Ptr, 0, sizeof (memMap));
   epicsMutexUnlock(m2MemFree);

   memset ((void *) scsBase, 0, sizeof (memMap));

   /* spawn communication tasks */  
   /* these had been medium priority for GS */
   epicsThreadMustCreate("tscsRx", epicsThreadPriorityHigh,
                  epicsThreadGetStackSize(epicsThreadStackBig),
                  (EPICSTHREADFUNC)scsReceive, (void *)NULL);

   epicsThreadMustCreate("ttiltRx", epicsThreadPriorityHigh,
                  epicsThreadGetStackSize(epicsThreadStackBig),
                  (EPICSTHREADFUNC)tiltReceive, (void *)NULL);

/*
   if ((taskSpawn ("cemTimerS", 101, VX_FP_TASK, 20000, (FUNCPTR) cemTimerStart, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) == ERROR)
   {
      printf ("taskSpawn fail - cemTimerS\n");
      return (ERROR);
   }

   if ((taskSpawn ("cemTimerE", 101, VX_FP_TASK, 20000, (FUNCPTR) cemTimerEnd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) == ERROR)
   {
      printf ("taskSpawn fail - cemTimerE\n");
      return (ERROR);
   }
*/

   return (OK);

}




