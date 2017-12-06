/*
 *
 * FILENAME
 * -------- 
 * scs.c
 * 
 * PURPOSE
 * -------
 * This file contains the c subroutines for the CAD records of the SCS
 * 
 * FUNCTION NAME(S)
 * ----------------
 * CADmove              - move to specified coordinates and hold
 * CADstop              - halt and hold at current position
 * CADfollow            - move to specified coordinates and track
 * receiveTcsDemand     - genSub record routine to receive tcs demand positions
 * CADmovebaffle        - move baffle to specified position
 * CADactuator          - move M2 in tilt and focus by direct actuator commands
 * CADpark              - move M2 to a predefined position and shut down
 * tcsTimeout           - Handling routine when 20Hz follow demand not received
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
 * 24-Jul-1996: Original (srp)
 * 02-Aug-1996: bug fix, remove '\n' from strings written to pcad->mess
 * 21-Aug-1996: Remove message strings from successful CAD routines
 * 23-Aug-1996: Modify debug operation and tidy up
 * 13-Nov-1996: Add calls to filter design routines in guideconfig
 * 26-Nov-1996: Add follow command subroutine. Remove stop restrictions
 * 02-Dec-1996: Add code for gensub routine receiving TCS position demands
 * 03-Jul-1997: Modify receiveTcsDemands such that TCS demands are received
 *              on port A and engineering screen demands on port B. A switch
 *              is incorporated such that the TCS demands are locked out when
 *              eng. screen demands are active - else they would be overwritten.
 * 27-Oct-1997: Add watchdog timer to receiveTcsDemands with handling routine
 *              named 'tcsTimeout'
 * 02-Jul-1998: Reject parameters with extraneous characters e.g. 3.1xxx
 * 07-May-1999: Added RCS id
 * 19-May-1999: Check for > Y_POSITION_LIMIT_MAX and < Y_POSITION_LIMIT_MIN
 * 21-May-1999: Include interp.h instead of defining external prototype for
 *              tcsInterpolate
 *              Take out bounds checking on XY positioner
 *              Add cjm's technique to limit demands on fine control servos
 * 02-Jun-1999: Expanded tabs to blanks
 *
 * 06-Oct-2017: Conversion to EPICS OSI started. (MDW)
 *
 */

/* ===================================================================== */

#include <math.h>
#include <string.h>
#include <stdlib.h>         /* For calloc */
#include <stdio.h>         /* For calloc */

#include <cad.h>
#include <car.h>
#include <timeLib.h>        /* For timeNow */

#include "utilities.h"      /* For act2tilt, errorLog, modifyFrame,
                                       debugLevel, ag2m2 */
#include "scs.h"
#include "archive.h"        /* For cadDirLog */
#include "chop.h"           /* For chopIsOn */
#include "control.h"        /* For scsPtr, interlockFlag, followOn, scsBase */
#include "guide.h"          /* For enum define of instrument indices */
#include "interlock.h"      /* For scsState */
#include "interp.h"         /* For tcsInterpolate */

/* Define default tilt and focus scaling */
#define DEFAULT_TILT_SCALE    3.917
#define DEFAULT_FOCUS_SCALE -59.359

/* -define FOLLOW_TIMEOUT       SYSTEM_CLOCK_RATE  for system clock at 
                                                  SYSTEM_CLOCK_RATE ticks 
                                                  per sec,
                                                  allow 1 second timeout */
#define FOLLOW_TIMEOUT       400 /* same definition as MOVE */
#define FOLLOW_ARRAY_SIZE     14

#define TCS_COMMAND_TIMEOUT  SYSTEM_CLOCK_RATE

#define ALWAYS 1

#if 0    // FIX THIS
/* Declare in file globals */
//static WDOG_ID timeoutId;
epicsTimerQueueId tqid = epicsTimerQueueAllocate(1, epicsThreadPriorityScanLow);
epicsTimerId timeoutId = epicsTimerQueueCreateTimer(tqid, 
#endif


static int first = TRUE; 
int badBeamCount = 0;    /* accessible from VxWorks prompt */

/* In file prototypes */
// static void tcsTimeout (void);

/* ===================================================================== */
/*
 * Function name:
 * receiveTcsDemand
 * 
 * Purpose:
 * Receipt of array from TCS containing position demands and scaling factors
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = receiveTcsDemand(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 *              > pcad->a       array of 14 doubles as follows:
 *
 *              [time sent, time to be applied, track ID,
 *               chop A x tilt, chop A y tilt,
 *               chop B x tilt, chop B y tilt,          
 *               chop C x tilt, chop C y tilt,
 *               X position, Y position, zFocus
 *               tiltScale, focusScale]
 * 
 * Parameters out:
 *              None
 *
 * Return value:
 *              < status        long
 * 
 * Globals: 
 *      External functions:
 *      None
 * 
 *      External variables:
 *      None
 * 
 * Requirements:
 * None
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 02-Dec-1996: Original(srp)
 * 10-Dec-1996: Modify to use global array of demands
 * 04-Jul-1997: Route engineering screen demands to port B and select with port C
 * 11-Jul-1997: Add interpolation call, remove trace statements
 * 25-Jul-1997: Stop using ports B and C, it is a Gemini requirement that the TCS
 *              and engineering screens should use the same interface
 * 23-Oct-1997: Modify to detect followOn flag rather than FOLLOWING state
 * 29-Jan-1998: Input port for follow must be port J to work over channel access
 * 24-Nov-1998: Add two extra array elements for tilt scaling and focus scaling
 *              modify processing so array is read even if not following
 * 
 */

/* ===================================================================== */

long initFollowGenSub (struct genSubRecord * pgsub)
{
    int source = 0;

    /* create conversion array if required */

    for (source = PWFS1; source <= GYRO; source++)
    {
        if (ag2m2[source] == NULL)
        {
            if ((ag2m2[source] = (frameChange *) calloc 
            (1, sizeof (frameChange))) == NULL)
            {
                errlogPrintf("Unable to calloc frameChange for source %d\n", source);
                return (ERROR);
            }
            else
            {
                ag2m2[source]->access = epicsMutexMustCreate(); 

                 /* initialise structure with default values */
                 modifyFrame (ag2m2[source], 0.0, DEFAULT_TILT_SCALE, 
                       DEFAULT_TILT_SCALE, DEFAULT_FOCUS_SCALE, 0.0, 0.0);
            }
        }
    }



// Why has the following been commented out? (MDW)
//
    /* create and start watchdog timer for follow updates */

    /* if ((timeoutId = wdCreate ()) == NULL)
    {
        printf ("unable to create watchdog\n");
        return (ERROR);
    }
    else
    {
        printf ("created watchdog %p\n", timeoutId);
    } */

    /* if (wdStart (timeoutId, FOLLOW_TIMEOUT, (FUNCPTR) tcsTimeout, 0) != OK)
    {
        printf ("error creating follow watchdog\n");
        return (ERROR);
    }
    else
    {
        printf ("started watchdog\n");
    } */

    return (OK);
}

long dummyInitGenSub (struct genSubRecord * pgsub)
{
    /* dummy init routine only */

    return (OK);
}

long receiveTcsDemand (struct genSubRecord * pgsub)
{
    int    index, arrayS = 0;
    long   activeC = CAR_IDLE;
    static location position;
    double tcsUpdate[FOLLOW_ARRAY_SIZE];
           /*= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22.22, 33.33};*/
    double *ptr, scsTimeNow;


    static double lastDmdX;
    static double lastDmdY;
    static double lastDmdZ;

    double currFocus;                     /* Current focus */
    double currXtilt;                     /* Current Xtilt */
    double currYtilt;                     /* Current Ytilt */
    static double prevFocus;              /* Previous focus */
    static double prevXtilt[3];           /* Previous Xtilt for each beam */
    static double prevYtilt[3];           /* Previous Ytilt for each beam */
    static int badFrames;                 /* Count of bad synchro frames */

    #ifdef M2_SENDING_CORRUPT_FRAMES 
    double dSyncFocus;                    
    double dSyncXtilt;
    double dSyncYtilt;
    #endif

    double dzDemand, dxDemand, dyDemand;  /* Change in demand */

    double zStep;                         /* Max change in z demand (mm) */
    double tiltStep;                      /* Max change in tilt demand */
    double tmp;                  /* temp value to hold z guide */
    int    beam;              /* Current beam as M2 sees */
    //int    beamCompare;              /* Current beam as SCS sees */
    int    currFrame;

    static int beamDiscrepancy = FALSE;   /* Flag that M2 beam != SCS beam */

// Why has the following been commented out? (MDW)
    /* Restart watchdog timeout when routine called */

    /* wdStart (timeoutId, FOLLOW_TIMEOUT, (FUNCPTR) tcsTimeout, 0);  */

    ptr = (double *) pgsub->j;

    /* Read these values in from channel access for now */

    zStep    = *(double *) pgsub->h ;
    tiltStep = *(double *) pgsub->i ;

    /* Read in the demand array */

    for (index = 0; index < FOLLOW_ARRAY_SIZE; index++)
    {
        tcsUpdate[index] = *(ptr++);
    }

    /* Update scaling factors */

    if (scsState == MOVING && followOn == ON && interlockFlag != 1)
    {
      /* get current SCS time */

      if (timeNow (&scsTimeNow) != OK)
        {
/*
            errlogMessage("receiveTcsDemand - error reading timestamp\n");
            return (ERROR); 
*/
        }

      /* compare time sent for late arrival and sanity check all demands */

      if ((scsTimeNow - tcsUpdate[0]) > TCS_COMMAND_TIMEOUT)
        {
      arrayS = 2;
        }
      else if (fabs (tcsUpdate[3]) > X_TILT_LIMIT)    /* BEAM A */
        {
      arrayS = 1;
        }
      else if (fabs (tcsUpdate[4]) > Y_TILT_LIMIT)
        {
      arrayS = 1;
        }
      else if (fabs (tcsUpdate[5]) > X_TILT_LIMIT)    /* BEAM B */
        {
      arrayS = 1;
        }
      else if (fabs (tcsUpdate[6]) > Y_TILT_LIMIT)
        {
      arrayS = 1;
        }
      else if (fabs (tcsUpdate[7]) > X_TILT_LIMIT)    /* BEAM C */
        {
      arrayS = 1;
        }
      else if (fabs (tcsUpdate[8]) > Y_TILT_LIMIT)
        {
      arrayS = 1;
        }
      else if (fabs (tcsUpdate[11]) > Z_FOCUS_LIMIT)
        {
      arrayS = 1;
        }
      /* let it be an unbounded box
     else if (fabs (tcsUpdate[9]) > X_POSITION_LIMIT)
     {
     arrayS = 1;
     }
     else if (tcsUpdate[10] > Y_POSITION_LIMIT_MAX)
     {
     arrayS = 1;
     }
     else if (tcsUpdate[10] < Y_POSITION_LIMIT_MIN)
     {
     arrayS = 1;
     }*/


      /* if the command was sent recently and values are in absolute range */

      if (arrayS == 0) {

    beam = (int)(scsPtr->page1.beamPosition);

    /* Inspect "beam" */
    if ((beam != BEAMA) && (beam != BEAMB) && (beam != BEAMC)) {
      errlogPrintf("receiveTcsDemand - unknown beam = %d\n",beam);
      beam = BEAMA; 
    } /***/

    else {


      beamDiscrepancy = FALSE;

      currFrame = (int) scsPtr->page1.NR;
      currFocus = scsPtr->page1.zFocus;
      currXtilt = scsPtr->page1.xTilt;
      currYtilt = scsPtr->page1.yTilt;


      /* "first" gets reset to TRUE each time follow is turned ON */
      if (first) {
        lastDmdX = tcsUpdate[3];
        lastDmdY = tcsUpdate[4];
        lastDmdZ = tcsUpdate[11];

        prevFocus = currFocus;
        prevXtilt[beam] = currXtilt;
        prevYtilt[beam] = currYtilt;
        badFrames = 0;
        first = FALSE;
        printf ("receiveTcsDemand - current pos beam %c at start of follow (frame %d) = %f %f %f\n", 
            ((beam == 0) ? 'A':'B'), currFrame, currXtilt, currYtilt, currFocus);
        printf("receiveTcsDemand - setting lastDmds to %f %f %f\n",
           lastDmdX, lastDmdY, lastDmdZ);
      }
          
      /* If neccessary, rate limit the demands. Note there is no
       * protection on reading the reflected memory as it is not
       * critical that focus, tip and tilt are all read at the
       * same time.
       *
       * Note: currFocus includes the guide component; tcsUpdate
       * does not. So need to add the current guide term in before
       * looking at the difference between demand and actual.
       * 
       * Use scsBase to read the demanded guide */

      if (scsBase != NULL)
        tmp = (double)(scsBase->page0.zGuide);
      else
        { 
          tmp = 0;
          printf("receiveTcsDemand - scsBase ptr is NULL");
        }

      /* Note 10jul00: As an alternative to rate limiting,
       * could check for big diff between demand and actual
       * and turn off servos, otherwise turn on - but this
       * may cause implications with guiding/chopping 
       * simultaneously. */


      dzDemand = (tcsUpdate[11] + tmp) - currFocus;
      if (dzDemand > zStep)
        tcsUpdate[11] = currFocus + zStep;
      else if (dzDemand < -zStep)
        tcsUpdate[11] = currFocus - zStep;

      if (debugLevel == DEBUG_RESERVED1)
        {
          /*     printf("receiveTcsD: dzD=%f zS=%f tcsUpd=%f tmp=%f currF=%f\n",dzDemand,zStep,tcsUpdate[11],tmp,currFocus); */
        }

      /* NOT_VALID_FOR_CHOPPING
       * 19-jul-00: assume valid for chopping.
       *            because now the limit is 150 arcsec 
       * later: add checks of beam amplitude and forbid
       * chops of > 150 or 200 or so.
       if (chopIsOn == 0)
       {*/
      dxDemand = tcsUpdate[3] - lastDmdX;     /* BEAM A */
      if (dxDemand > tiltStep)
        tcsUpdate[3] = lastDmdX + tiltStep;
      else if (dxDemand < -tiltStep)
        tcsUpdate[3] = lastDmdX - tiltStep;

      dyDemand = tcsUpdate[4] - lastDmdY;
      if (dyDemand > tiltStep)
        tcsUpdate[4] = lastDmdY + tiltStep;
      else if (dyDemand < -tiltStep)
        tcsUpdate[4] = lastDmdY - tiltStep;
      /*
        }
      */


      /* calculate the interpolation coefficients */

      tcs.timeSent = tcsUpdate[0];
      tcs.timeApply = tcsUpdate[1];
      tcs.trackId = (long) tcsUpdate[2];

      /* perform coordinate conversion for each beam */

      position.xTilt = tcsUpdate[3];
      position.yTilt = tcsUpdate[4];
      position.zFocus = tcsUpdate[11];
      position.xPos = tcsUpdate[9];
      position.yPos = tcsUpdate[10];


      if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
        {
          printf("before: xy = %f %f;", position.xPos, position.yPos);
        }

      tcs2m2 (&position);
            
      if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
        {
          printf("after: xy = %f %f\n", 
             position.xPosNew, position.yPosNew);
        }

      tcs.xTiltA = position.xTiltNew;
      tcs.yTiltA = position.yTiltNew;
      tcs.zFocus = position.zFocusNew;
      tcs.xPosition = position.xPosNew;
      tcs.yPosition = position.yPosNew;

      /* NEW: rate limits for beam B */
      dxDemand = tcsUpdate[5] - currXtilt;       /* BEAM B */
      if (dxDemand > tiltStep)
        tcsUpdate[5] = currXtilt + tiltStep;
      else if (dxDemand < -tiltStep)
        tcsUpdate[5] = currXtilt - tiltStep;

      dyDemand = tcsUpdate[6] - currYtilt;
      if (dyDemand > tiltStep)
        tcsUpdate[6] = currYtilt + tiltStep;
      else if (dyDemand < -tiltStep)
        tcsUpdate[6] = currYtilt - tiltStep;
             
      position.xTilt = tcsUpdate[5];
      position.yTilt = tcsUpdate[6];

      tcs2m2 (&position);

      tcs.xTiltB = position.xTiltNew;
      tcs.yTiltB = position.yTiltNew;

      dxDemand = tcsUpdate[7] - currXtilt;       /* BEAM C */
      if (dxDemand > tiltStep)
        tcsUpdate[7] = currXtilt + tiltStep;
      else if (dxDemand < -tiltStep)
        tcsUpdate[7] = currXtilt - tiltStep;

      dyDemand = tcsUpdate[8] - currYtilt;
      if (dyDemand > tiltStep)
        tcsUpdate[8] = currYtilt + tiltStep;
      else if (dyDemand < -tiltStep)
        tcsUpdate[8] = currYtilt - tiltStep;

      position.xTilt = tcsUpdate[7];
      position.yTilt = tcsUpdate[8];

      tcs2m2 (&position);

      tcs.xTiltC = position.xTiltNew;
      tcs.yTiltC = position.yTiltNew;

      tcsInterpolate (tcs);

      /* too much time to print - don't use
         if (beamDiscrepancy == TRUE)
         {
         printf("%d m2 beam=%c, scs beam=%c tcsUpdate  IS ", currFrame,
         ((beam == 0) ? 'A' : 'B'), ((beamCompare == 0) ? 'A' : 'B'));
         printf("xA%f (last=%f)", tcsUpdate[3], lastDmdX);
         printf("yA%f (last=%f)", tcsUpdate[4], lastDmdY);
         printf("z%f (act=%f)", tcsUpdate[11], currFocus);
         printf("\n"); 
         }
      */
      lastDmdX = tcsUpdate[3];
      lastDmdY = tcsUpdate[4];
      lastDmdZ = tcsUpdate[11];
    }
      } 
    
      else {
    errorLog ("receiveTcsDemand - follow demands failed sanity checks", 1, ON);
      }
    }

    /* write indicators to genSub ouputs */

    *(long *) pgsub->vala = activeC;
    *(long *) pgsub->valb = arrayS;
    *(double *) pgsub->valc = scsTimeNow;
    *(double *) pgsub->vald = tcsUpdate[2];
    *(double *) pgsub->vale = tcsUpdate[12];    /* tilt scale factor */
    *(double *) pgsub->valf = tcsUpdate[13];    /* focus scale factor */
    *(long *)pgsub->valg = badFrames ;

    return (OK);
}

void resetFirstFollowDemand( void )
{
    first = TRUE;
}

/* ===================================================================== */
/*
 * Function name:
 * CADstop
 * 
 * Purpose:
 * The stop command shall cause the tilt and positioning mechanisms to
 * hold at the current position and ignore updates in demanded position.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADstop(pcad)
 * 
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 * 
 * Parameters out:
 *              < pcd->mess     *string status message
 * 
 * Return value:
 *              < status        long
 * 
 * Globals: 
 *      External functions:
 *      None
 * 
 *      External variables:
 *              > scsState      long    current scs state
 * 
 * Requirements:
 * None
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 24-Jul-1996: Original(srp)
 * 26-Nov-1996: Accept stop command even if already stopped
 * 11-Jul-1997: Remove requirement for stop to be rejected if not following
 * 
 */

/* ===================================================================== */

long CADstop (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;

    cadDirLog ("stop", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* trigger the forward link */
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/*
 * Function name:
 * CADmove
 * 
 * Purpose:
 * Cause the M2 mechanism to move to a position demand and remain
 * there ignoring any further demand updates (c.f. follow command)
 * The demands are transformed from the TCS frame of reference to M2
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADmove(pcad)
 * 
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 *
 *              > pcad->a       string beam A x tilt
 *              > pcad->b       string beam A y tilt
 *              > pcad->c       string beam B x tilt
 *              > pcad->d       string beam B y tilt
 *              > pcad->e       string beam C x tilt
 *              > pcad->f       string beam C y tilt
 *              > pcad->g       string x posiiton
 *              > pcad->h       string y position
 *              > pcad->i       string focus
 * 
 * Parameters out:
 *              < pcad->vala    double beam A x tilt
 *              < pcad->valb    double beam A y tilt
 *              < pcad->valc    double beam B x tilt
 *              < pcad->vald    double beam B y tilt
 *              < pcad->vale    double beam C x tilt
 *              < pcad->valf    double beam C y tilt
 *              < pcad->valg    double focus
 *              < pcad->valh    double x posiiton
 *              < pcad->vali    double y position
 *
 *              < pcd->mess     *string status message
 * 
 * Return value:
 *              < status        long
 * 
 * Globals: 
 *      External functions:
 *      tcs2m2
 * 
 *      External variables:
 *              > scsState      current state of the SCS
 * 
 * Requirements:
 * None
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 24-Jul-1996: Original(srp)
 * 21-Jan-1996: remove "stop accepted" message. Change rejection scsStates to INIT, TEST, RESET
 *              add sanity checks to demanded values
 * 11-Aug-1997: Modify inputs to accept all beam positions.
 * 10-Nov-1997: Add coordinate conversion from tcs to m2 frames
 * 13-Jan-1998: Change order of x,y position and z focus
 * 21-May-1999: Take out the bounds on the XY positioner
 */

/* ===================================================================== */

long CADmove (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;
    char dumpString[MAX_STRING_SIZE];
    static location position;
    static double AxTilt, AyTilt, BxTilt, ByTilt, CxTilt, CyTilt, zFocus,
        xPos, yPos;

    cadDirLog ("move", pcad->dir, 9, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:

        printf ("CAD clear with scsState = %ld\n",scsState);
        strncpy (pcad->mess, "Clear the CAD Move state", MAX_STRING_SIZE - 1);
        status = CAD_ACCEPT;
        break;

    case menuDirectivePRESET:

        /* convert the input strings to numbers for checking */

        if (sscanf (pcad->a, "%lf%s", &AxTilt, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail axtilt conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if (fabs (AxTilt) > X_TILT_LIMIT)
        {
            printf ("axtilt out of range %f AxTilt %f \n",X_TILT_LIMIT,fabs(AxTilt));
            strncpy (pcad->mess, "axtilt out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->b, "%lf%s", &AyTilt, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail aytilt conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if (fabs (AyTilt) > X_TILT_LIMIT)
        {
            strncpy (pcad->mess, "aytilt out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->c, "%lf%s", &BxTilt, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail bxtilt conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if (fabs (BxTilt) > X_TILT_LIMIT)
        {
            strncpy (pcad->mess, "bxtilt out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->d, "%lf%s", &ByTilt, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail bytilt conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if (fabs (ByTilt) > X_TILT_LIMIT)
        {
            strncpy (pcad->mess, "bytilt out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->e, "%lf%s", &CxTilt, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail cxtilt conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if (fabs (CxTilt) > X_TILT_LIMIT)
        {
            strncpy (pcad->mess, "cxtilt out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->f, "%lf%s", &CyTilt, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail cytilt conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if (fabs (CyTilt) > X_TILT_LIMIT)
        {
            strncpy (pcad->mess, "cytilt out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->g, "%lf%s", &xPos, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail xpos conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        /*if (fabs (xPos) > X_POSITION_LIMIT)
        {
            strncpy (pcad->mess, "xpos out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }*/

        if (sscanf (pcad->h, "%lf%s", &yPos, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail ypos conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        /*
        if (fabs (yPos) > Y_POSITION_LIMIT)
        {
            strncpy (pcad->mess, "ypos out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }*/

        if (sscanf (pcad->i, "%lf%s", &zFocus, dumpString) != 1)
        {
            strncpy (pcad->mess, "fail focus conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if (fabs (zFocus) > Z_FOCUS_LIMIT)
        {
            strncpy (pcad->mess, "focus out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* read current state of SCS to determine if move is appropriate */

            if ((scsState == INITIALISING) || (scsState == TESTING) || (scsState == REBOOTING))
            {
               printf ("INIT IN PROGRESS with scsState = %ld\n",scsState);
               strncpy (pcad->mess, "init or test in progress", MAX_STRING_SIZE - 1);
               status = CAD_REJECT;
            }
            else
            {
                /* convert values to M2 frame of reference */

                position.xTilt = AxTilt;
                position.yTilt = AyTilt;
                position.zFocus = zFocus;
                position.xPos = xPos;
                position.yPos = yPos;

                tcs2m2 (&position);

                /* copy values to CAD outputs */

                *(double *) pcad->vala = position.xTiltNew;
                *(double *) pcad->valb = position.yTiltNew;

        /* A wierdness: valg holds the focus while "g" holds xPos */
        /* A wierdness: valh holds the xPos while "h" holds yPos  */
        /* A wierdness: vali holds the yPos while "i" holds focus */

                *(double *) pcad->valg = position.zFocusNew;
                *(double *) pcad->valh = position.xPosNew;
                *(double *) pcad->vali = position.yPosNew;

                /* repeat for B and C beams */

                position.xTilt = BxTilt;
                position.yTilt = ByTilt;

                tcs2m2 (&position);

                *(double *) pcad->valc = position.xTiltNew;
                *(double *) pcad->vald = position.yTiltNew;

                position.xTilt = CxTilt;
                position.yTilt = CyTilt;

                tcs2m2 (&position);

                *(double *) pcad->vale = position.xTiltNew;
                *(double *) pcad->valf = position.yTiltNew;
            }
        }
        else
        {
            printf ("interlocks active with scsState = %ld\n",scsState);
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/*
 * Function name:
 * CADfollow
 * 
 * Purpose:
 * The follow command cuases the tilt and positioning systems to move to
 * the demanded position and subsequent demanded positions
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADfollow(pcad)
 * 
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 * 
 * Parameters out:
 *              < pcd->mess     *string status message
 * 
 * Return value:
 *              < status        long
 * 
 * Globals: 
 *      External functions:
 *      None
 * 
 *      External variables:
 *              > scsState      current state of the SCS
 * 
 * Requirements:
 * None
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 26-Nov-1996: Original. Adapted from previous move command (srp)
 * 
 */

/* ===================================================================== */

long CADfollow (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;

    cadDirLog ("follow", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {

            /* read current state of SCS to determine if move is appropriate */

            if (scsState == TESTING || scsState == INITIALISING || scsState == REBOOTING)
            {
                strncpy (pcad->mess, "Inappropriate SCS state", MAX_STRING_SIZE - 1);
                status = CAD_REJECT;
            }
            else
            {
                strncpy (pcad->mess, "", MAX_STRING_SIZE - 1);
            }
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/*
 * Function name:
 * CADactuator
 * 
 * Purpose:
 * The preset directive causes the three actuator demands to be
 * converted to x tilt, y tilt and z focus demands which are
 * copied to the CAD outputs.  When start is activated the action
 * is as for the move command
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADactuatormove(pcad)
 * 
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 *              > pcad->a       *string actuator A1 displacement
 *              > pcad->b       *string actuator A2 displacement
 *              > pcad->c       *string actuator A2 displacement
 * 
 * Parameters out:
 *              < pcd->mess     *string status message
 *              < pcad->vala    double x tilt equivalence
 *              < pcad->valb    double y tilt equivalence
 *              < pcad->valc    double z focus equivalence
 *              < pcad->vald    double x tilt
 *              < pcad->vale    double y tilt
 *              < pcad->valf    double z focus

 * 
 * Return value:
 *              < status        long
 * 
 * Globals: 
 *      External functions:
 *      None
 * 
 *      External variables:
 *              > scsState      current state of the SCS
 * 
 * Requirements:
 * None
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 24-Jul-1996: Original(srp)
 * 27-Nov-1996: Modify to output tilts and focus as well as A1,2,3
 * 21-Jan-1997: Add sanity checks to entered data
 * 11-Jul-1997: Error in units, actuator displacement now entered in microns, focus output in microns
 * 
 */

/* ===================================================================== */

long CADactuators (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;
    static location position;
    char dumpString[MAX_STRING_SIZE];

    cadDirLog ("actuators", pcad->dir, 3, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        if (sscanf (pcad->a, "%lf%s", &position.actuator1, dumpString) != 1)
        {
            strncpy (pcad->mess, "act1 failed conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->b, "%lf%s", &position.actuator2, dumpString) != 1)
        {
            strncpy (pcad->mess, "act2 failed conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->c, "%lf%s", &position.actuator3, dumpString) != 1)
        {
            strncpy (pcad->mess, "act3 failed conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        /* if all is well perform the conversion  */

        act2tilt (&position);

        /* if resulting values are within range, copy to CAD outputs */

        if (fabs (position.xTilt) > X_TILT_LIMIT || fabs (position.yTilt) > Y_TILT_LIMIT || fabs (position.zFocus) > Z_FOCUS_LIMIT)
        {
            strncpy (pcad->mess, "arguments out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            *(double *) pcad->vala = position.actuator1;
            *(double *) pcad->valb = position.actuator2;
            *(double *) pcad->valc = position.actuator3;
            *(double *) pcad->vald = position.xTilt;
            *(double *) pcad->vale = position.yTilt;
            *(double *) pcad->valf = position.zFocus;
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/*
 * Function name:
 * CADpark
 * 
 * Purpose:
 * Verify that the current state of the scs is appropriate for parking
 * to be initiated.  Provide a trigger to the parking SNL code
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADpark(pcad)
 * 
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 * 
 * Parameters out:
 *              < pcd->mess     *string status message
 * 
 * Return value:
 *              < status        long
 * 
 * Globals: 
 *      External functions:
 *      None
 * 
 *      External variables:
 *      None
 * 
 * Requirements:
 * None
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 24-Jul-1996: Original(srp)
 * 03-Jul-1997: Remove state restriction of START directive.
 * 
 */

/* ===================================================================== */

long CADpark (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;

    cadDirLog ("park", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* trigger the forward link */
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "park - inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/*
 * Function name:
 * ticker
 * 
 * Purpose:
 * read current time from the timeServer and make available on to the
 * outputs of the genSub record so that it can be read in EPICS
 * 
 * Invocation:
 * struct genSubRecord *pgsub
 * status = ticker(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 * None
 * 
 * Parameters out:
 *              > pgsub->a      double  current time
 *              > pgsub->b      string  current time as a string
 *              > pgsub->c      long    incrementing integer count
 *              > pgsub->d      long    oscillating heartbeat
 *
 * Return value:
 *              < status        long
 * 
 * Globals: 
 *      External functions:
 *      timeNow
 * 
 *      External variables:
 *      None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 24-Jul-1996: Original(srp)
 * 23-Jan-1997; add ticker function to supply current time
 * 11-Mar-1998: add oscillating heartbeat ouput to port d
 * 
 */

/* ===================================================================== */

long ticker (struct genSubRecord * pgsub)
{
    int j, c[7];
    static long count = 0;
    char timeString[MAX_STRING_SIZE];
    double scsTimeNow;

    j = timeNowC (TAI, 3, c);

    if (j == 0)
    {
        sprintf (timeString, "%d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d", c[0], c[1], c[2], c[3], c[4], c[5]);
    }
    else
    {
        strncpy (timeString, "error reading local time", MAX_STRING_SIZE - 1);
    }

    if (timeNow (&scsTimeNow) != OK)
    {
        if ((debugLevel > DEBUG_NONE) & (debugLevel <= DEBUG_MED))
            errlogMessage("ticker - error reading timestamp\n");
        return (ERROR);
    }
    

    *(double *) pgsub->vala = scsTimeNow;
    strncpy (pgsub->valb, timeString, MAX_STRING_SIZE - 1);

    *(long *) pgsub->valc = count++;
    *(long *) pgsub->vald = count % 2;

    return (OK);
}

#if 0
static void tcsTimeout (void)
{
    /* restart watchdog timer */

    /* wdStart (timeoutId, FOLLOW_TIMEOUT, (FUNCPTR) tcsTimeout, 0); */
    if (debugLevel == DEBUG_MED)
      errlogMessage("tcsTimeout - tcs 20Hz follow demand missed for > 1 sec\n");
}
#endif
