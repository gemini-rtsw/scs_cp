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
 *
 */
/* INDENT ON */
/* ===================================================================== */

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

#include <string.h>
#include <stdio.h>

#include <logLib.h> /* For logMsg */
#include <dbAccess.h>   /* For dbNameToAddr */

#include <taskLib.h>
#include <sysLib.h> /* For sysXXX */
#include <vxLib.h>  /* For vxMemProbe */

#define TOP "m2:"

/* #define TRUESCSBASE     0xf0A00040      / * Base address of node 0 */
#define TRUESCSBASE     0xfAA00040      /* Base address of node 0 
                                           reflective memory card (scs)  */
/* #define TRUEM2BASE      0xf0600040      / * Base address of node 1 */
#define TRUEM2BASE      0xfA600040      /* Base address of node 1 
                                           reflective memory card (m2 )  */

/* Declare externals */

MSG_Q_ID healthQId = NULL;
SEM_ID scsReady = NULL;

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
 * 
 */

/* INDENT ON */
/* ===================================================================== */

int scsInit (void)
{
   int source = PWFS1;
   char pRecordName[MAX_STRING_SIZE];
   char test = 0;

   /* create holding semaphore to prevent execution before intialisation */

   if ((scsReady = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create scsReady sem\n");
      return (ERROR);
   }

   /* create semaphores to control pvload of initialisation files */

   if ((doPvLoad = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create doPvload sem\n");
      return (ERROR);
   }

   if ((pvLoadComplete = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create pvLoadComplete sem\n");
      return (ERROR);
   }

   /* create semaphore to control data logging */

   if ((logNow = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("scsLogInit - unable to create logNow sem\n");
      return (ERROR);
   }

   /* get address of logging CAR record */

   strncpy (pRecordName, TOP "logC.IVAL", MAX_STRING_SIZE);

   if (dbNameToAddr (pRecordName, &logCAddr) != 0)
   {
      printf ("scsLogInit - unable to fetch address of logC CAR\n");
      return (ERROR);
   }

   /* mutex semaphore to prevent multiple access to guide data */

   for (source = PWFS1; source <= GYRO; source++)
   {
      wfsFree[source] = epicsMutexMustCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
   }

   /* create dummy filters for each of the guide sources */

   for (source = PWFS1; source <= GYRO; source++)
   {
      createFilter (source, OFF, 200.0, 20.0, 20.0, -2.0, -2.0, -2.0);
   }

   /* create mutex semaphore to protect the set point values */

   if ((setPointFree = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) == NULL)
   {
      printf ("unable to create setPointFree sem\n");
      return (ERROR);
   }

   /* spawn task to pvload initialisation data */

   if (taskSpawn ("tloadInit", 150, VX_FP_TASK, 20000, (FUNCPTR) loadInitFiles, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
   {
      logMsg ("unable to spawn load init files task\n", 0, 0, 0, 0, 0, 0);
      return (ERROR);
   }

   /* spawn logging task */

   if (taskSpawn ("loggerTask", 44, VX_FP_TASK, 20000, (FUNCPTR) loggerTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
   {
      printf ("unable to spawn loggerTask\n");
      return (ERROR);
   }

   /* spawn control loop task */

   if (taskSpawn ("tslowTx", 100, VX_FP_TASK, 30000, (FUNCPTR) slowTransmit, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
   {
      logMsg ("unable to spawn slowTransmit task\n", 0, 0, 0, 0, 0, 0);
      return (ERROR);
   }

   /* Do in startup in order that xycom has already been initted.
    * Then, if TIME_LATENCY needed, everything w.r.t. port addresses will
    * have been defined */
   if (taskSpawn ("tprocGuides", 7, VX_FP_TASK, 20000, (FUNCPTR) processGuides, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
   {
      logMsg ("unable to spawn processGuides task\n", 0, 0, 0, 0, 0, 0);
      return (ERROR);
   }

   /* set up aux clock to and connect ISR */

   sysAuxClkRateSet (SYSTEM_CLOCK_RATE);
   sysAuxClkConnect ((FUNCPTR) fireLoops, 0);
   sysAuxClkEnable ();

   /*
    * call with parameter zero for no simulation, any other value for full
    * simulation
    */

   if ((refMemFree = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) == NULL)
   {
      printf ("unable to create refMemFree\n");
      return (ERROR);
   }

   if ((eventDataSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) == NULL)
   {
      printf ("unable to create eventDataSem\n");
      return (ERROR);
   }

   if ((m2MemFree = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) == NULL)
   {
      printf ("unable to create m2MemFree\n");
      return (ERROR);
   }

   if ((scsDataAvailable = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create scsDataAvailable\n");
      return (ERROR);
   }

   if ((slowUpdate = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create slowUpdate\n");
      return (ERROR);
   }

   if ((scsReceiveNow = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create scsReceiveNow\n");
      return (ERROR);
   }

   if ((guideUpdateNow = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create guideUpdateNow\n");
      return (ERROR);
   }


/* This semaphore doesn't seem to be used anywhere  -- get rid of it. 20171019 MDW */
//   if ((compileStatus = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
//   {
//      printf ("unable to create compileStatus\n");
//      return (ERROR);
//   }

   if ((statusCompiled = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create statusCompiled\n");
      return (ERROR);
   }

   if ((diagnosticsAvailable = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL)
   {
      printf ("unable to create diagnosticsAvailable\n");
      return (ERROR);
   }

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

   if (vxMemProbe((void *)TRUESCSBASE, READ, sizeof(char), &test) == OK)
   {
      printf("Reflective memory card detected\n");
      scsBase = (memMap *) TRUESCSBASE;
   }
   else
   {
      printf("Reflective memory card not detected\n");

      if ((scsBase = (memMap *) malloc (sizeof (memMap))) == NULL)    
      {
         printf ("malloc fail on creation of scsBase buffer\n");
         return (ERROR);
      }
   }

   printf ("initRefMem, scsPtr = %x, scsBase = %x\n", (unsigned int) scsPtr, (unsigned int) scsBase); 

   /* create command message queue */

   if ((commandQId = msgQCreate (100, sizeof (long), MSG_Q_FIFO)) == NULL)
   {
      errorLog ("initRefMem - error in creation of command message queue", 1, ON);
   }
   else
   {
      printf ("message queue created successfully\n");
   }

   /* create command receiving queue for the simulation */

   if ((receiveQId = msgQCreate (100, sizeof (long), MSG_Q_FIFO)) == NULL)
   {
      errorLog ("initRefMem - error in creation of receive message queue", 1, ON);
   }
   else
   {
      printf ("receive message queue created successfully\n");
   }

   /* create health message queue */

   if ((healthQId = msgQCreate (100, sizeof (healthReport), MSG_Q_FIFO)) == NULL)
   {
      errorLog ("error in creation of health message queue", 1, ON);
   }
   else
   {
      printf ("health message queue created successfully\n");
   }

   /* clear buffers */

   if (semTake (refMemFree, SEM_TIMEOUT) == OK)
   {
      memset ((void *) scsPtr, 0, sizeof (memMap));
      semGive (refMemFree);
   }
   else
   {
      errorLog ("initRefMem - refMemFree timeout", 1, ON);
   }

   if (semTake (m2MemFree, SEM_TIMEOUT) == OK)
   {
      memset ((void *) m2Ptr, 0, sizeof (memMap));
      semGive (m2MemFree);
   }
   else
   {
      errorLog ("initRefMem - m2MemFree timeout", 1, ON);
   }

   memset ((void *) scsBase, 0, sizeof (memMap));

   /* spawn communication tasks */

   if (taskSpawn ("tscsRx", 7, VX_FP_TASK, 20000, (FUNCPTR) scsReceive, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
   {
      printf ("taskSpawn fail - scsReceive\n");
      return (ERROR);
   }

   if ((taskSpawn ("ttiltRx", 8, VX_FP_TASK, 20000, (FUNCPTR) tiltReceive, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) == ERROR)
   {
      printf ("taskSpawn fail - tiltReceive\n");
      return (ERROR);
   }

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




