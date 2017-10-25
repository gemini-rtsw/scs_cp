/* $Id: utilities.c,v 1.5 2015/04/30 23:58:17 mrippa Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * utilties.c
 * 
 * PURPOSE
 * -------
 * Library functions for coordinate conversion
 * 
 * FUNCTION NAME(S)
 * ----------------
 * act2tilt - conversion actuator space to tilt space
 * tilt2act - conversion tilt space to actuator space
 * checkSum - checksum over specified block
 * weight2string- convert guide weighting to string
 * errorLog - write error information to file and screen
 * tcs2m2   - convert tip,tilt,focus,xPos,yPos from TCS coords to M2
 * m22tcs   - convert tip,tilt,focus,xPos,yPos from M2 coords to TCS
 * control      - perform PID algorithm with anti-windup and rate limit
 * setPid       - adjust PID parameters using values from engineering screens
 * stateInit
 * stateMonitor
 * scsStateStringConvert
 * scsStateStringInit
 * snlStateInit
 * snlStateMonitor
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
 * 28-Aug-1997: Original(srp)
 * 29-Aug-1997: Add functions for tilt2curve and curve2tilt conversion
 * 16-Jan-1998: Add weight2string function
 * 02-Feb-1998: add errorLog functions
 * 25-Feb-1998: remove all references to moveCurve
 * 23-Jun-1998: Add functions to read and report health, remove setHealth
 * 10-May-1999: Added RCS id
 */
/* INDENT ON */
/* ===================================================================== */

#include "utilities.h"
#include "chopControl.h"     /* For chopEventSem */
#include "control.h"         /* For controller */
#include "elgLib.h"          /* For MSG_Q_ID definition, XTILT, YTILT, ZFOCUS */
#include "guide.h"           /* For guideOn, weight */
#include "interlock.h"       /* For scsState */
#include "setup.h"           /* For healthQId */

#ifndef _INCLUDED_SUBRECORD_H
#define _INCLUDED_SUBRECORD_H
#include <subRecord.h>
#endif
#include <stdio.h>          /* For atoi */
#include <stdlib.h>         /* For atoi */
#include <string.h>
#include <math.h>           /* For sin, cos */
#include <time.h>           /* For date2secs */
#include <timeLib.h>        /* For timeNow */

#define SCSTOP "top = m2:"
#define INSTTOP "I = m2:inst:"

static int loggingEnable = ON;

/* Define function prototypes */

int pvload(char *file, char *subset, int flags, int noAbort);

int debugLevel = DEBUG_NONE;
long inPosition = 0;
frameChange *ag2m2[MAX_SOURCES];

/* not used anywhere. 20171019 MDW */
//SEM_ID compileStatus = NULL; 
//SEM_ID statusCompiled = NULL;


// SEM_ID doPvLoad = NULL;
// SEM_ID pvLoadComplete = NULL;
epicsEventId doPvLoad;
epicsEventId pvLoadComplete;


frameOfReference frame =
{
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0
};
long cadProcessorSnlState;  /* Initialize these states? */
long followDemandSnlState;
long monitorSadSnlState;
long monitorProcessSnlState;
long rebootScsSnlState;
long moveBaffleSnlState;

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * act2tilt
 * 
 * Purpose:
 * Convert position representation from actuator positions to tilt/focus
 * positions
 *
 * Invocation:
 * int act2tilt(location *position)
 *
 * Parameters in:
 *      > position  location pointer
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < int       OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(srp)
 * 23-Dec-1997: mod to pass pointer to structure of positions rather than positions themselves (srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

int act2tilt (location *position)
{
    /* function converts the mirror position represented in actuator     */
    /* positions to tilt and focus coordinates               */
    /* actuator positions are supplied individually in units of microns  */
    /* a pointer is returned to a structure containing the tilts or a NULL   */
    /* pointer if the results are out of range               */

    /* convert input units (microns) to metres */

    position->actuator1 /= 1e6;
    position->actuator2 /= 1e6;
    position->actuator3 /= 1e6;

    /* convert actuator demands to tilt space - units degrees and metres */

    position->xTilt = RADS2DEGS * 
        (2 * position->actuator1 - position->actuator2 - position->actuator3) / 
        (3 * ACTUATOR_RADIUS);
    position->yTilt = RADS2DEGS * (position->actuator2 - position->actuator3) /
        (ACTUATOR_RADIUS * sqrt (3.0));
    position->zFocus = 
        (position->actuator1 + position->actuator2 + position->actuator3) / 3;

    /* convert degrees and metres to arcseconds and microns */

    position->xTilt *= DEGS2ASECS;
    position->yTilt *= DEGS2ASECS;
    position->zFocus *= METRES2MICRONS;

    /* if resulting values are within range return pointer to values else
     * error */

    if (fabs (position->xTilt) > X_TILT_LIMIT || 
        fabs (position->yTilt) > Y_TILT_LIMIT || 
        fabs (position->zFocus) > Z_FOCUS_LIMIT)
    {
        return (ERROR);
    }
    else
    {
        return (OK);
    }
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * tilt2act
 * 
 * Purpose:
 * Convert position representation from tilt/focus positions to actuator
 * positions
 *
 * Invocation:
 * int tilt2act(location *position)
 *
 * Parameters in:
 *      > position  location pointer
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < int       OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(srp)
 * 23-Dec-1997: mod to pass pointer to structure of positions rather than positions themselves (srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

int tilt2act (location *position)
{
    /* function converts the mirror position represented in tilt and     */
    /* focus coordinates to actuator positions               */

    /* tilts supplied in arcsecs, focus in microns, convert to rads and
     * metres respectively */

    position->xTilt *= DEGS2RADS / DEGS2ASECS;
    position->yTilt *= DEGS2RADS / DEGS2ASECS;
    position->zFocus /= METRES2MICRONS;

    position->actuator1 = position->zFocus + 
        (ACTUATOR_RADIUS * position->xTilt);
    position->actuator2 = position->zFocus - 
        (ACTUATOR_RADIUS * position->xTilt / 2) + 
        (sqrt (3.0) * ACTUATOR_RADIUS * position->yTilt / 2);
    position->actuator3 = position->zFocus - 
        (ACTUATOR_RADIUS * position->xTilt / 2) - 
        (sqrt (3.0) * ACTUATOR_RADIUS * position->yTilt / 2);

    /* convert input units (microns) to metres */

    position->actuator1 *= METRES2MICRONS;
    position->actuator2 *= METRES2MICRONS;
    position->actuator3 *= METRES2MICRONS;

    return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * tcs2m2
 * 
 * Purpose:
 * Convert position representation from tcs frame of reference to M2 frame
 *
 * Invocation:
 * status = tcs2m2(location *position)
 *
 * Parameters in:
 *      > position  location*   pointer to structure of positions
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int     OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      > frame     struct  structure of skew angles and offsets
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 07-Nov-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

int tcs2m2 (location *position)
{
    /* function converts the mirror position represented in tcs frame    */
    /* to m2 frame of reference                                          */
    /* tilts supplied in arcsecs, focus and translations in microns, 
       convert to rads and metres respectively */
    double xPosTemp;

    /* if debugging, dump some values */

    if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
    { 
        printf ("debugLevel is %d\n", debugLevel);

        printf ("frame: tiltCosTheta=%f, tiltSinTheta=%f, tiltOffsetX=%f, tiltOffsetY=%f\n",  
                frame.tiltCosTheta, frame.tiltSinTheta, frame.tiltOffsetX, frame.tiltOffsetY);

        printf ("frame: posCosTheta=%f, posSinTheta=%f, posOffsetX=%f, posOffsetY=%f\n",  
                frame.posCosTheta, frame.posSinTheta, frame.posOffsetX, frame.posOffsetY);
    }

    /* convert tilt axes */

    position->xTiltNew = frame.tiltCosTheta * position->xTilt - 
        frame.tiltSinTheta * position->yTilt + 
        frame.tiltOffsetX;
    position->yTiltNew = frame.tiltSinTheta * position->xTilt + 
        frame.tiltCosTheta * position->yTilt + 
        frame.tiltOffsetY;

    /* convert translation axes */

    xPosTemp          = -position->xPos; 

    position->xPosNew = frame.posCosTheta*xPosTemp - 
        frame.posSinTheta* position->yPos + 
        frame.posOffsetX;
    position->yPosNew = frame.posSinTheta*xPosTemp + 
        frame.posCosTheta* position->yPos + 
        frame.posOffsetY;

    /* focus is unchanged */

    position->zFocusNew = position->zFocus;

    return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * m22tcs
 * 
 * Purpose:
 * Convert position representation from m2 frame of reference to tcs frame
 *
 * Invocation:
 * int = m22tcs(location *position)
 *
 * Parameters in:
 *      > position  location*   pointer to structure of positions
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int     OK or ERROR
 *
 * Globals: 
 *
 * External functions:
 * None
 * 
 * External variables:
 *      > frame     struct  structure of skew angles and offsets
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 07-Nov-1997: Original(srp)
 * 24-Feb-1998: Modify to accept location pointer
 * 
 */

/* INDENT ON */
/* ===================================================================== */

int m22tcs (location *position)
{
    /* function converts the mirror position represented in m2 frame
       to tcs frame of reference tilts supplied in arcsecs, focus and 
       translations in microns, convert to rads and metres respectively */

    /* if debugging, dump some values */

    if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
    { 
        printf ("debugLevel is %d\n", debugLevel);

        printf ("frame: tiltCosTheta=%f, tiltSinTheta=%f, tiltOffsetX=%f, tiltOffsetY=%f\n",  
                frame.tiltCosTheta, frame.tiltSinTheta, frame.tiltOffsetX, frame.tiltOffsetY);

        printf ("frame: posCosTheta=%f, posSinTheta=%f, posOffsetX=%f, posOffsetY=%f\n",  
                frame.posCosTheta, frame.posSinTheta, frame.posOffsetX, frame.posOffsetY);
    }

    /* convert tilt axes */

    position->xTilt = frame.tiltCosTheta * 
        (position->xTilt - frame.tiltOffsetX) + 
        frame.tiltSinTheta * (position->yTilt - 
        frame.tiltOffsetY);
    position->yTilt = -frame.tiltSinTheta *
        (position->xTilt - frame.tiltOffsetX) + 
        frame.tiltCosTheta * 
        (position->yTilt - frame.tiltOffsetY);

    /* convert translation axes */

    position->xPosNew = -(frame.posCosTheta * 
        (position->xPos - frame.posOffsetX) + 
        frame.posSinTheta * 
        (position->yPos - frame.posOffsetY));
    position->yPosNew = -frame.posSinTheta * 
        (position->xPos - frame.posOffsetX) + 
        frame.posCosTheta * 
        (position->yPos - frame.posOffsetY);

    /* focus is unchanged */

    position->zFocus = position->zFocus;

    return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * gaos2m2
 * 
 * Purpose:
 * Convert position representation from gaos frame of reference to M2 frame
 *
 * Invocation:
 * int = gaos2m2(location *position)
 *
 * Parameters in:
 *      > position  *location   structure of positions
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int     OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      > frame     struct  structure of skew angles and offsets
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 11-Nov-1997: Original(srp)
 * 29-Jan-1998: Calling function passes structure pointer. 
 */

/* INDENT ON */
/* ===================================================================== */

int gaos2m2 (location *position)
{
    /* function converts the mirror position represented in gaos frame   */
    /* to m2 frame of reference                      */

    /* tilts supplied in arcsecs, focus and translations in microns, 
       convert to rads and metres respectively */

    /* convert tilt axes */

    position->xTiltNew = frame.gaosCosTheta*position->xTilt - 
        frame.gaosSinTheta* position->yTilt + frame.gaosOffsetX;
    position->yTiltNew = frame.gaosSinTheta*position->xTilt + 
        frame.gaosCosTheta* position->yTilt + frame.gaosOffsetY;

    /* focus scaling adjustment */

    position->zFocusNew = frame.focusScaling * position->zFocus;

    return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * gyro2m2
 * 
 * Purpose:
 * Convert position representation from gyro frame of reference to M2 frame
 *
 * Invocation:
 * int = gyro2m2(location *position)
 *
 * Parameters in:
 *      > position  location*   structure of positions
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int     OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      > frame     struct  structure of skew angles and offsets
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 11-Nov-1997: Original(srp)
 * 29-Jan-1998: pass pointer to structure of positions
 */

/* INDENT ON */
/* ===================================================================== */

int gyro2m2 (location *position)
{
    /* function converts the mirror position represented in gyro frame   */
    /* to m2 frame of reference                      */

    /* convert tilt axes */

    position->xTiltNew = frame.gyroCosTheta*position->xTilt - 
        frame.gyroSinTheta* position->yTilt + frame.gyroOffsetX;
    position->yTiltNew = frame.gyroSinTheta*position->xTilt + 
        frame.gyroCosTheta* position->yTilt + frame.gyroOffsetY;

    position->zFocusNew = frame.focusScaling * position->zFocus;

    return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * checkSum
 * 
 * Purpose:
 * Calculates the checksum over the given number of bytes assuming the
 * block to consist entirely of type long
 *
 * Invocation:
 * value = checkSum(*start, numLongs)
 *
 * Parameters in:
 *      > start     *void   start address
 *      > numLongs  int number of long words to sum
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < value     long    sum
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    checkSum (void *ptr, int numLongs)
{
    long   *checkPtr = (long *) ptr;
    long    sum = 0;
    int     n;

    for (n = 0; n < numLongs; n++)
    {
        sum += *checkPtr++;
    }

    return (sum);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * date2secs
 * 
 * Purpose:
 * Converts a data and time string to seconds (UTC). The date string must
 * be of the form "year/month/day hour:minute:second" e.g. "1997/12/02 16:23:11"
 * 
 *
 * Invocation:
 * value = date2secs(char * dateString);
 *
 * Parameters in:
 *      > dateString    string  "1997/12/02 16:23:11"
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < value     int seconds or -1 if error
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 02_Dec-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

int date2secs(char * dateString)
{
    struct tm t;
    char    *token;

    token = strtok(dateString, "/");
    if(token)
        t.tm_year = atoi(token) - 1900;

    token = strtok(NULL, "/");
    if(token)
        t.tm_mon = atoi(token) - 1;

    token = strtok(NULL, " ");
    if(token)
        t.tm_mday = atoi(token);

    token = strtok(NULL, ":");
    if(token)
        t.tm_hour = atoi(token);

    token = strtok(NULL, ":");
    if(token)
        t.tm_min = atoi(token);

    token = strtok(NULL, ":");
    if(token)
        t.tm_sec = atoi(token);

    return ( (int) mktime(&t) );
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * weight2string
 * 
 * Purpose:
 * Convert the guide weight to a string for display on engineering screens
 *
 * Invocation:
 * char* = weight2string(double weight)
 *
 * Parameters in:
 *      > weight    double  -2 = don't use
 *                  -1 = use auto weighting
 *                  0 .. 100 as given
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < value     char*   weight converted to string
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 16-Jan-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

char*   weight2string(double weightA, double weightB, double weightC)
{
    static char weightString[30];
    char    aString[10], bString[10], cString[10];

    /*strncpy(weightString, '       ", 29);*/
    weightString[0] = 0;

    if(weightA < -1)
        strncpy(aString, "A-OFF  ", 9);
    else if(weightA < 0)
        strncpy(aString, "A-AUTO ", 9);
    else
        sprintf(aString, "A-%4.1f ", weightA);

    if(weightB < -1)
        strncpy(bString, "B-OFF  ", 9);
    else if(weightB < 0)
        strncpy(bString, "B-AUTO ", 9);
    else
        sprintf(bString, "B-%4.1f ", weightB);

    if(weightC < -1)
        strncpy(cString, "C-OFF  ", 9);
    else if(weightC < 0)
        strncpy(cString, "C-AUTO ", 9);
    else
        sprintf(cString, "C-%4.1f ", weightC);

    strncat(weightString, aString, 9);
    strncat(weightString, bString, 9);
    strncat(weightString, cString, 9);

    return(weightString);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * errorLog
 * 
 * Purpose:
 * Write error messages to console and log file
 *
 * Invocation:
 * int = errorLog((char *) errorString, int debugLevel, int fileLog);
 *
 * Parameters in:
 *      > errorString   string  string describing error
 *      > debugLevel    int debug level of error message
 *      > fileLog   int log to file if set to 1
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 01-Feb-1998: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

int errorLog (char *errorString, int debugLevelRqst, int fileLog)
{
    double  timeStamp;
    char    errMsg[81], localString[55];

    if(loggingEnable != ON) {
      printf("--------CONSOLE PRINTING DISABLED--------\n"); 
      return(OK);
    }
    if(timeNow(&timeStamp) != OK)
    {
        if(debugLevel >= debugLevelRqst)
        {
        logMsg("errorLog - error reading timeStamp\n", 0, 0, 0, 0, 0, 0);
        return(ERROR);
        }
    }

    strncpy(localString, errorString, 54);

    /* add timestamp to error message */

    sprintf(errMsg, "%16.6f - %-.54s", timeStamp, localString);

    /* display error to screen */

    if(debugLevel >= debugLevelRqst)
    {
        logMsg("%s\n", (int)errMsg, 0, 0, 0, 0, 0);
    }

    return(OK);
}
/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * reportHealth
 * 
 * Purpose:
 * Set the system health status. The requested health status and message
 * are sanity checked and written to the health message queue
 *
 * Invocation:
 * int = reportHealth(int severity, char *message)
 *
 * Parameters in:
 *      > severity  int {GOOD | WARNING | BAD}
 *      > message   char*   pointer to message text
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 *
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 15-Jun-1998: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */
int reportHealth(int severity, char *message)
{
    healthReport    newReport;

    /* sanity check severity and string length */

    if(severity < GOOD || severity > BAD)
        return(ERROR);
    else
        newReport.severity = severity;

    strncpy(newReport.message, message, MAX_STRING_SIZE - 1);
    newReport.message[MAX_STRING_SIZE - 1] = NULL;

    /* write structure to message queue */

    if (msgQSend (healthQId, (char *) &newReport, sizeof (healthReport), SEM_TIMEOUT, MSG_PRI_NORMAL) == ERROR)
    {
        printf ("failed to append health severity %d, message >  %s to health queue\n", newReport.severity, newReport.message);
        return (ERROR);
    }
    return (OK);
}   

/* ===================================================================== */

long readHealthInit(struct genSubRecord *pgsub)
{
    strcpy((char *)pgsub->vala, "GOOD");
    strcpy((char *)pgsub->valb, "");

    return(OK);
}


long readHealth(struct genSubRecord *pgsub)
{
    healthReport    health;

    /* fetch command from health queue */

    if(msgQReceive (healthQId, (char *) &health, sizeof (healthReport), NO_WAIT) != ERROR)
    {
        switch(health.severity)
        {
        case WARNING:
            strcpy((char *)pgsub->vala, "WARNING");
            break;
        case BAD:
            strcpy((char *)pgsub->vala, "BAD");
            break;
        default:
            strcpy((char *)pgsub->vala, "GOOD");
        }
        strcpy((char *)pgsub->valb, health.message);
    }
    return(OK);
}   

int loadInitFiles(void)
{
   for(;;)
   {
      epicsEventMustWait(dpPvLoad);

      errlogPrintf("pvload initialisation data\n");

      if(pvload("./data/SCSinit.dat", SCSTOP, 0, 0) != OK)
         errlogPrintf("pvload error SCSinit.dat\n");
      else
         errlogPrintf("pvload SCSinit.dat\n");
                
      if(pvload("./data/xforms.dat", SCSTOP, 0, 0) != OK)
         errlogPrintf("pvload error xforms.dat\n");
      else
         errlogPrintf("pvload xforms.dat\n");

      if(pvload("./data/limits.dat", SCSTOP, 0, 0) != OK)
         errlogPrintf("pvload error limits.dat\n");
      else
         errlogPrintf("pvload limits.dat\n");

      if(pvload("./data/instConfig.dat", INSTTOP, 0, 0) != OK)
         errlogPrintf("pvload error instConfig.dat\n");
      else
         errlogPrintf("pvload instConfig.dat\n");

      epicsEventSignal(pvLoadComplete);
    }
}


/* allow chop transition to be forced from engineering screens for testing */
long  driveEvent (struct subRecord * psub)
{
    if(psub->a > 0)
        semGive(chopEventSem);

    return(OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * stateMonitor
 * stateInit
 * 
 * Purpose:
 * Make variable on the Capfast diagrams available globally to all of 
 * the CAD subroutines. The subroutine record inputs a and b are read 
 * and written to the global variables scsState
 * 
 * Invocation:
 * struct subRecord *psub
 * status = stateMonitor(psub)
 * status = stateInit(psub)
 * 
 * Parameters in:
 *      > psub->a   long scsState
 *      > psub->c   long inPosition
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    long
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 24-Jul-1996: Original(srp)
 * 23-Jan-1997; add ticker function to supply current time
 * 23-Oct-1997: remove references to guide state, guideOn shall be used instead
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    stateInit (struct subRecord * psub)
{
    /* this is a dummy initialisation routine to satisfy requirements of
     * the subroutine record */

    return (OK);
}

long    stateMonitor (struct subRecord * psub)
{
    scsState = (long) psub->a;
    inPosition = (long) psub->c;

    return (OK);
}


/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * scsStateStringConvert
 * scsStateStringInit
 * 
 * Purpose:
 * Convert the scsState (longout record) to a string for display
 * on dm screens.
 * 
 * Invocation:
 * struct genSubRecord *pgsub
 * status = scsStateStringConvert(pgsub)
 * status = scsStateStringInit(pgsub)
 * 
 * Parameters in:
 *      > pgsub->a  long scsState
 * 
 * Parameters out:
 *          < pgsub->vala   string scsStateString   
 * 
 * Return value:
 *      < status    long
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Dayle Kotturi
 * 
 * History:
 * 14-Oct-1999: Original(kdk)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    scsStateStringInit (struct genSubRecord * pgsub)
{
    /* this is a dummy initialisation routine to satisfy requirements of
     * the subroutine record */

    strcpy((char *)pgsub->vala, "NEVER PROCESSED"); 
    return (OK);
}

long    scsStateStringConvert (struct genSubRecord * pgsub)
{
    scsState = *((long *)pgsub->a);

        switch (scsState)
    {
    case SYSTEMIDLE:
            strcpy((char *)pgsub->vala, "SYSTEM IDLE");
        break;
    case INITIALISING:
            strcpy((char *)pgsub->vala, "INITIALISING");
        break;
    case REBOOTING:
            strcpy((char *)pgsub->vala, "REBOOTING");
        break;
    case TESTING:
            strcpy((char *)pgsub->vala, "TESTING");
        break;
    case MOVING:
            strcpy((char *)pgsub->vala, "MOVING");
        break;
    case CHOPPING:
            strcpy((char *)pgsub->vala, "CHOPPING");
        break;
    case PARKED:
            strcpy((char *)pgsub->vala, "PARKED");
        break;
    case FOLLOWING:
            strcpy((char *)pgsub->vala, "FOLLOWING");
        break;
    case INTERLOCKED:
            strcpy((char *)pgsub->vala, "INTERLOCKED");
        break;
    default:
        strcpy((char *)pgsub->vala, "UNKNOWN CASE");
    }

    return (OK);
}


/* ===================================================================== */

double confine (double value, double upper, double lower)
{
    char message[MAX_STRING_SIZE];

    if (upper < lower) 
    {
        sprintf(message, "confine - Upper (%f) < lower (%f)",
            upper, lower);
        logMsg ("%s\n", (int)message, 0, 0, 0, 0, 0);
    };
	
    if (value > upper)
    return (upper);
    else if (value < lower)
    return (lower);
    else
    return (value);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * control
 * 
 * Purpose:
 * Implements a single axis PID algorithm with anti-windup and rate limiting
 * 
 * Invocation:
 * u = control(axis, demanded position, current position)
 * 
 * Parameters in:
 *      > axis      int XTILT or YTILT or FOCUS
 *      > input     double  desired position
 *      > systemOutput  double  current mechanism position
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < u     double  calculated control value
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

double control (int axis, double input, double systemOutput)
{
    double error, u;

    /* check axis is within bounds */

    if (axis < 0 || axis > 2)
    {
        logMsg ("control - axis out of range\n", 0, 0, 0, 0, 0, 0);
        return (0.0);
    }
    else
    {
        /* calculate current error value */

        error = input - systemOutput;

        /* calculate integral term */

        controller[axis].sum += error;

        /* so many clamps... take this one out 23feb2000 for focus 
           to see what happens (don't want to break the integrator
           term currently working for x,y tilt                     

           10-mar-00 take this out for all axes

        if (axis != 2)
        {
        controller[axis].sum = confine (controller[axis].sum, 
            (double) controller[axis].windUpLimit, 
            (double) -controller[axis].windUpLimit);
        }
        */

	/* an ugly patch to throw away corrupted sum values 
	 * They look to be overwritten by a timestamp */

	if (controller[axis].sum > 10000.)
	{
            printf("control - found huge integral sum = %f. repl with %f\n", 
	        controller[axis].sum, controller[axis].oldSum);
	    controller[axis].sum = controller[axis].oldSum;
	}

        /* calculate output */

        u = (controller[axis].P * error)
            + (controller[axis].D * (error - controller[axis].oldError))
            + (controller[axis].I * controller[axis].sum);

        /*
         * apply rate limit by limiting the maximum excursion between samples
         */

        /* so many clamps... take this one out 23feb2000 for focus 
           to see what happens (don't want to break the term currently 
           working for x,y tilt                     

           10-mar-00 take this out for all axes

        if (axis != 2)
        {
        if (fabs (u - controller[axis].oldOutput) > controller[axis].rateLimit)
        {
            if (u > controller[axis].oldOutput)
            u = controller[axis].oldOutput + controller[axis].rateLimit;
            else
            u = controller[axis].oldOutput - controller[axis].rateLimit;
        }
        }
        */

        controller[axis].oldOutput = u;
        controller[axis].oldError = error;
	controller[axis].oldSum = controller[axis].sum;

        return (u);
    }
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * setPid
 * 
 * Purpose:
 * The controller CAD takes parameters for the PID controllers from the
 * engineering screens then calls this function to write the values to
 * the control structures
 * 
 * Invocation:
 * status = setPid(axis, P, I, D, windUpLimit, rateLimit)
 * 
 * Parameters in:
 *      > axis      int XTILT or YTILT or FOCUS
 *      > P     double  proportional gain
 *      > I     double  integral gain
 *      > D     double  derivative gain
 *      > windUpLimit   double  Maximum value for integral term
 *      > rateLimit double  maximum change in output between calls
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(srp)
 * 24-Feb-1998: add array index check
 * 
 */

/* INDENT ON */
/* ===================================================================== */

int setPid (int axis, double P, double I, double D, double windUpLimit, 
            double rateLimit)
{

    if (axis < 0 || axis > 2)
    {
        printf ("setPid axis out of range\n");
        return (ERROR);
    }
    else
    {
        /* set PID controller settings */

        controller[axis].P = P;
        controller[axis].I = I;
        controller[axis].D = D;
        controller[axis].windUpLimit = windUpLimit;
        controller[axis].rateLimit = rateLimit;

        /* controller[axis].sum = 0.0;
        controller[axis].oldError = 0.0;
        controller[axis].oldOutput = 0.0; */
    }
    return (OK);
}


/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * modifyFrame
 * 
 * Purpose:
 * Update frame conversion data
 *
 * Invocation:
 * STATUS   modifyFrame (
 *              frameChange *f, 
 *              const double theta,
 *              const double scaleX,
 *              const double scaleY,
 *              const double scaleZ,
 *              const double offsetX,
 *              const double offsetY
 *              )
 *
 * Parameters in:
 *      > frame *f      pointer to structure of skew angles
 *      > double theta      frame rotation angle (degrees)
 *      > double scaleX     scale factor for X axis
 *      > double scaleY     scale factor for Y axis
 *      > double scaleZ     scale factor for Z axis
 *      > double offsetX    offset for X axis
 *      > double offsetY    offset for Y axis
 *
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int     OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 * 
 * Requirements:
 * 
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 20-Nov-1998: Original(srp)
 * 28-Nov-1998: Adapted for SCS usage(srp)
 *
 */

/* INDENT ON */

/* ===================================================================== */

int  modifyFrame
    (
    frameChange *f,
    const double theta,
    const double scaleX,
    const double scaleY,
    const double scaleZ,
    const double offsetX,
    const double offsetY
    )
{
    /* check that frame structure has been initialised */

    if (f == NULL)
    {
        logMsg("Conversion frame not initialised\n", 0, 0, 0, 0, 0 ,0);
            return(ERROR);
    }
        
    /* access frame */

    if(semTake(f->access, WFS_TIMEOUT) == OK)
    {
        /* update the structure */

        f->theta    = theta*DEGS2RADS;
        f->sinTheta = sin(theta);
        f->cosTheta = cos(theta);
        f->offsetX  = offsetX;
        f->offsetY  = offsetY;
        f->scaleX   = scaleX;
        f->scaleY   = scaleY;
        f->scaleZ   = scaleZ;

        semGive(f->access);

        return (OK);
    }
    else
    {
        logMsg("Modify frame - unable to get mutex for conversion frame\n", 0, 0, 0, 0 ,0 ,0);
        return(ERROR);
    }
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * showFrame
 * 
 * Purpose:
 * Print frame conversion data to screen
 *
 * Invocation:
 * STATUS   showFrame (int source)
 *
 * Parameters in:
 *      > int source        index number of wfs source
 *
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    int     OK or ERROR
 *
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 * 
 * Requirements:
 * 
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 25-Nov-1998: Original(srp)
 *
 */

/* INDENT ON */

/* ===================================================================== */

int   showFrame (int source)
{
    frameChange grab, *f;

    if(source < PWFS1 || source > GYRO)
    {
        printf("source %d out of range\n", source);
        return(ERROR);
    }

    f = ag2m2[source];

    /* access frame */

    if(semTake(f->access, WFS_TIMEOUT) == OK)
    {
        /* copy the frame contents */

        grab = *f;
        semGive(f->access);
    }
    else
    {
        logMsg("showFrame - unable to get mutex for conversion frame\n", 0, 0, 0, 0 ,0 ,0);
        return(ERROR);
    }

    printf("conversion data for source %d\n", source);
    printf("angle (rads)            = %f\n", grab.theta);
    printf("angle (degrees)         = %f\n", (grab.theta/DEGS2RADS));
    printf("sin theta               = %f\n", grab.sinTheta);
    printf("cos theta               = %f\n", grab.cosTheta);
    printf("offsetX                 = %f\n", grab.offsetX);
    printf("offsetY                 = %f\n", grab.offsetY);
    printf("scaleX                  = %f\n", grab.scaleX);
    printf("scaleY                  = %f\n", grab.scaleY);
    printf("scaleZ                  = %f\n", grab.scaleZ);

    return (OK);
}

double vtkscale = 1.0;

int vtkControl (Vtk *vtk, double guideError) {

    long status = OK;
    double rotatorAngle;
    double C, S;
    double R[2][2]= {{0.0, 0.0}, {0.0, 0.0}}; /*Transpose of my primary Rotator*/
    static long guide_paused = 0;
    static double lastGuideError = 0.0;


    /*Scale the guideError to deal with any WFS gain*/
    guideError = vtkscale * guideError;

    /* If the guideError is zero, they've paused for an offset.
     *
     * We don't want to advance the oscillator when we're paused and
     * we can resume once again only when:
     *
     *   1. Non-constant guideError signals are coming in.
     *   2. The new frequency calculations are close to the previous converged values.
     * */
    if ( guideError == lastGuideError ) {

        guide_paused = 1;
        vtk->counter = 0;
        return OK; 
    }
    lastGuideError = guideError;

    rotatorAngle = 2*PI*vtk->frequency.currentValue / vtk->Fs;
    C=cos(rotatorAngle), 
    S=sin(rotatorAngle);
    R[0][0] = C;
    R[0][1] = -S;
    R[1][0] = S;
    R[1][1] = C;
   
    MatMult(R, vtk->oscillator.Sold, vtk->oscillator.Snew);   /* Advance the oscillator */
    memcpy(vtk->oscillator.Sold, vtk->oscillator.Snew, 2*1*sizeof(double));
    
    vtk->integral[0][0] = vtk->integral[0][0] + vtk->gain.phase * vtk->oscillator.Snew[0][0] * guideError;
    vtk->integral[1][0] = vtk->integral[1][0] + vtk->gain.phase * vtk->oscillator.Snew[1][0] * guideError;

    /* Prepare M2 command injection*/
    /*   Localoscillator = vtk->Scale * vtk->Oscillator * vtk->Rotator';
     */
    MatMult(vtk->Rotator, vtk->oscillator.Snew, vtk->localOscillator.Snew);        /* Advance the rotation */
    ScalerMult(vtk->scale, vtk->localOscillator.Snew);                             /* Modifies vtk->localOscillator.Snew */

    /* Apply the M2 command injection */
    vtk->command = 2*(vtk->integral[0][0] * vtk->localOscillator.Snew[0][0] +
                     vtk->integral[1][0] * vtk->localOscillator.Snew[1][0] );
 
    /* Saturate the out put for now... but this should Return 
     * an error as something is wrong if this occurs.*/

    if ( abs(vtk->command) > vtk->maxAmplitude) {
        status = ERROR;
        return (status);
    }
             
    /* Measure the phase */
    /* move the last measured phase to the first position */
    vtk->phaseOld = vtk->phase;
    /* compute the new phase */
    vtk->phase = atan2(vtk->integral[1][0], vtk->integral[0][0]);

    /* Initially we ignore Frequency tracking  */
    if (vtk->counter < 100) {
        /* wait for the phase estiamtor to converge */
        vtk->counter++;
    }

    else {
        /* unwrap the phases (make their difference less than pi) */
        vtk->phaseOld = vtk->phaseOld - 2*PI*myround1( ((vtk->phaseOld - vtk->phase)/((double)(2.0*PI)) ));
        vtk->deltaPhase = vtk->phase - vtk->phaseOld;
        vtk->frequency.error = vtk->deltaPhase/(2*PI) * vtk->Fs;
        vtk->frequency.currentValue = vtk->frequency.currentValue - vtk->gain.frequency * vtk->frequency.error;
    
        /* Check for frequency tolerance*/
        if (vtk->frequency.currentValue > ( vtk->frequency.initialValue + vtk->frequency.tolerance) )
           vtk->frequency.currentValue = vtk->frequency.initialValue + vtk->frequency.tolerance; 

        if (vtk->frequency.currentValue < ( vtk->frequency.initialValue - vtk->frequency.tolerance) )
           vtk->frequency.currentValue = vtk->frequency.initialValue - vtk->frequency.tolerance; 
    }

    guide_paused = 0;

    return (status);

}

/*
 * @fn double myround( double x, int precision) 
 *
 * @brief Round a double to the specified precision.
 *
 * @param[in] x (double) Variable to round
 * @param[in] precision (int) Number of digits to round to
 *
 * @return: (double) Returns rounded value as a double.
 *
 * @Description
 * The PPC doesn't implement round(), so we roll a simple one 
 * favoring rounding up. Works ONLY for positive inputs.
 *
 *
 * Learn here that many math routines are machine dependent:
 * http://www.vxdev.com/docs/vx55man/vxworks/ppc/powerpc.html
 *
 * So, round is not available 'off-the-shelf'
 *
 *
 * Round here good to 'precision' decimals and only positive numbers, so good 
 * for TAI timestamps.
 *
 * mrippa
 */
double myround( double x, int precision) {

    double fac;

    fac = pow(10, precision);
    return ((int)(x*fac + 0.5))/fac;

}


double myround1(double x) {

    double c,delta;

    c = ceil(x);
    delta = c - x;

    if (delta > 0.5)
        c -= 1;

    return c;

}

int myround_nearest10(int n) {

    int tmp, delta;
    
    tmp = (10* floor((n/10)+0.5));
    delta = n - tmp;

    if (delta >=5 )
        return tmp + 10;

    return tmp; 
    
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * snlStateMonitor
 * snlStateInit
 * 
 * Purpose:
 * Make variable on the Capfast diagrams available globally to all of 
 * the CAD subroutines. The subroutine record inputs a - f are read 
 * and written to the global variables scsState.
 *
 * I still have something to learn about Process Variables, it seems,
 * because I don't understand why they are not global via pvGet.
 * 
 * Invocation:
 * struct subRecord *psub
 * status = snlStateMonitor(psub)
 * status = snlStateInit(psub)
 * 
 * Parameters in:
 *      > psub->a   long cadProcessorSnlState
 *      > psub->b   long followDemandSnlState
 *      > psub->c   long monitorSadSnlState
 *      > psub->d   long monitorProcessSnlState
 *      > psub->e   long rebootScsSnlState
 *      > psub->f   long moveBaffleSnlState
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < status    long
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Dayle Kotturi (dayle@gemini.edu)
 * 
 * History:
 * 28-Jan-2000: Original(kdk)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    snlStateInit (struct subRecord * psub)
{
    /* this is a dummy initialisation routine to satisfy requirements of
     * the subroutine record */

    return (OK);
}

long    snlStateMonitor (struct subRecord * psub)
{
    cadProcessorSnlState   = (long) psub->a;
    followDemandSnlState   = (long) psub->b;
    monitorSadSnlState     = (long) psub->c;
    monitorProcessSnlState = (long) psub->d;
    rebootScsSnlState      = (long) psub->e;
    moveBaffleSnlState     = (long) psub->f;

    return (OK);
}


/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * snlStateStringConvert
 * snlStateStringInit
 * 
 * Purpose:
 * Convert the SNL state set states (longout records) to a string for display
 * on dm screens.
 * 
 * Invocation:
 * struct genSubRecord *pgsub
 * status = snlStateStringConvert(pgsub)
 * status = snlStateStringInit(pgsub)
 * 
 * Parameters in:
 *      > pgsub->a  long cadProcessorState
 *      > pgsub->b  long followDemandState
 *      > pgsub->c  long monitorSadState
 *      > pgsub->d  long monitorProcessState
 *      > pgsub->e  long rebootScsState
 *      > pgsub->f  long moveBaffleState
 * 
 * Parameters out:
 *          < pgsub->vala   string snlStateString   
 *          < pgsub->valb   string snlStateString   
 *          < pgsub->valc   string snlStateString   
 *          < pgsub->vald   string snlStateString   
 *          < pgsub->vale   string snlStateString   
 *          < pgsub->valf   string snlStateString   
 * 
 * Return value:
 *      < status    long
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Dayle Kotturi
 * 
 * History:
 * 27-Jan-2000: Original(kdk)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    snlStateStringInit (struct genSubRecord * pgsub)
{
    /* this is a dummy initialisation routine to satisfy requirements of
     * the subroutine record */

    strcpy((char *)pgsub->vala, "NEVER PROCESSED"); 
    strcpy((char *)pgsub->valb, "NEVER PROCESSED"); 
    strcpy((char *)pgsub->valc, "NEVER PROCESSED"); 
    strcpy((char *)pgsub->vald, "NEVER PROCESSED"); 
    strcpy((char *)pgsub->vale, "NEVER PROCESSED"); 
    strcpy((char *)pgsub->valf, "NEVER PROCESSED"); 
    return (OK);
}

long    snlStateStringConvert (struct genSubRecord * pgsub)
{
    cadProcessorSnlState = *((long *)pgsub->a);

    switch (cadProcessorSnlState)
    {
    case CADPROCENTRY:
        strcpy((char *)pgsub->vala, "ENTRY");
        break;
    case CADPROCIDLE:
        strcpy((char *)pgsub->vala, "IDLE");
        break;
    case STARTTESTHANDSHAKE:
        strcpy((char *)pgsub->vala, "START TEST HANDSHAKE");
        break;
    case READAGAIN:
        strcpy((char *)pgsub->vala, "READ AGAIN");
        break;
    case STARTINIT:
        strcpy((char *)pgsub->vala, "START INIT");
        break;
    case LOADFILES:
        strcpy((char *)pgsub->vala, "LOAD FILES");
        break;
    case WAITINITRESPONSE:
        strcpy((char *)pgsub->vala, "WAIT INIT RESPONSE");
        break;
    case WAITINITCOMPLETE:
        strcpy((char *)pgsub->vala, "WAIT INIT COMPLETE");
        break;
    case INITCOMPLETETIMEOUT:
        strcpy((char *)pgsub->vala, "INIT COMPLETE TIMEOUT");
        break;
    case STARTMOVE:
        strcpy((char *)pgsub->vala, "START MOVE");
        break;
    case PROCEEDWITHMOVE:
        strcpy((char *)pgsub->vala, "PROCEED WITH MOVE");
        break;
    case SERVOOFFTIMEOUT:
        strcpy((char *)pgsub->vala, "SERVO OFF TIMEOUT");
        break;
    case MOVEWAITFORCOINCIDENCE:
        strcpy((char *)pgsub->vala, "MOVE WAIT COINCIDENCE");
        break;
    case MOVETIMEOUTFORCOINCIDENCE:
        strcpy((char *)pgsub->vala, "MOVE TIMEOUT COINCIDENCE");
        break;
    case STARTACTUATOR:
        strcpy((char *)pgsub->vala, "START ACTUATOR");
        break;
    case PROCEEDWITHACTMOVE:
        strcpy((char *)pgsub->vala, "PROCEED ACTMOVE");
        break;
    case ACTUATORSERVOOFFTIMEOUT:
        strcpy((char *)pgsub->vala, "ACT SERVO OFF TIMEOUT");
        break;
    case ACTUATORWAITFORCOINCIDENCE:
        strcpy((char *)pgsub->vala, "ACT WAIT COINCIDENCE");
        break;
    case STARTFOLLOW:
        strcpy((char *)pgsub->vala, "START FOLLOW");
        break;
    case PROCEEDWITHFOLLOW:
        strcpy((char *)pgsub->vala, "PROCEED WITH FOLLOW");
        break;
    case FOLLOWSERVOTIMEOUT:
        strcpy((char *)pgsub->vala, "FOLLOW SERVO TIMEOUT");
        break;
    case FOLLOWWAITFORCOINCIDENCE:
        strcpy((char *)pgsub->vala, "FOLLOW WAIT COINCIDENCE");
        break;
    case FOLLOWTIMEOUT:
        strcpy((char *)pgsub->vala, "FOLLOW TIMEOUT");
        break;
    case STARTSTOP:
        strcpy((char *)pgsub->vala, "START STOP");
        break;
    case STARTPARK:
        strcpy((char *)pgsub->vala, "START PARK");
        break;
    case WAITFORPARK:
        strcpy((char *)pgsub->vala, "WAIT FOR PARK");
        break;
    case TIMEOUTWAITFORPARK:
        strcpy((char *)pgsub->vala, "TIMEOUT WAIT PARK");
        break;
    case STARTCHOPCONFIG:
        strcpy((char *)pgsub->vala, "START CHOP CONFIG");
        break;
    case STARTCHOPCONTROL:
        strcpy((char *)pgsub->vala, "START CHOP CONTROL");
        break;
    case WAITFORCHOPRESPONSE:
        strcpy((char *)pgsub->vala, "WAIT FOR CHOP RESPONSE");
        break;
    case TIMEOUTWAITFORCHOPRESPONSE:
        strcpy((char *)pgsub->vala, "TIMEOUT WAIT CHOP RESPONSE");
        break;
    case STARTBANDWITH:
        strcpy((char *)pgsub->vala, "START BANDWITH");
        break;
    case STARTTOLERANCE:
        strcpy((char *)pgsub->vala, "START TOLERANCE");
        break;
    case STARTDRIVEFOLLOWER:
        strcpy((char *)pgsub->vala, "START DRIVE FOLLOWER");
        break;
    case PARKWAITFORCOINCIDENCE:
        strcpy((char *)pgsub->vala, "PARK WAIT COINCIDENCE");
        break;
    default:
        strcpy((char *)pgsub->vala, "UNKNOWN CASE");
    }

    followDemandSnlState = *((long *)pgsub->b);

        switch (followDemandSnlState)
        {
        case FOLDEMANDENTRY:
            strcpy((char *)pgsub->valb, "ENTRY");
            break;
        case WAITFORDEMAND:
            strcpy((char *)pgsub->valb, "WAIT FOR DEMAND");
            break;
        case WAITFORNEXTCOINCIDENCE:
            strcpy((char *)pgsub->valb, "WAIT NEXT COINCIDENCE");
            break;
        case TIMEOUTWAITFORNEXTCOINCIDENCE:
            strcpy((char *)pgsub->valb, "TIMEOUT WAIT NEXT COINCIDENCE");
            break;
        default:
        strcpy((char *)pgsub->valb, "UNKNOWN CASE");
        }

    monitorSadSnlState = *((long *)pgsub->c);

        switch (monitorSadSnlState)
        {
        case MONSADENTRY:
            strcpy((char *)pgsub->valc, "ENTRY");
            break;
        case UPDATESAD:
            strcpy((char *)pgsub->valc, "UPDATE SAD");
            break;
        default:
        strcpy((char *)pgsub->valc, "UNKNOWN CASE");
        }

    monitorProcessSnlState = *((long *)pgsub->d);

        switch (monitorProcessSnlState)
        {
        case UPDATEPARAMETERS:
            strcpy((char *)pgsub->vald, "UPDATE PARAMETERS");
            break;
        default:
        strcpy((char *)pgsub->vald, "UNKNOWN CASE");
        }

    rebootScsSnlState = *((long *)pgsub->e);

        switch (rebootScsSnlState)
        {
        case INITREBOOT:
            strcpy((char *)pgsub->vale, "INIT REBOOT");
            break;
        case WAITFORREBOOTCMD:
            strcpy((char *)pgsub->vale, "WAIT REBOOT CMD");
            break;
        case STARTREBOOT:
            strcpy((char *)pgsub->vale, "START REBOOT");
            break;
        case WAITFORPARKREBOOT:
            strcpy((char *)pgsub->vale, "WAIT PARK REBOOT");
            break;
        case TIMEOUTWAITFORPARKREBOOT:
            strcpy((char *)pgsub->vale, "TIMEOUT WAIT PARK REBOOT");
            break;
        case REBOOTNOW:
            strcpy((char *)pgsub->vale, "REBOOT NOW");
            break;
        default:
        strcpy((char *)pgsub->vale, "UNKNOWN CASE");
        }

    moveBaffleSnlState = *((long *)pgsub->f);

        switch (moveBaffleSnlState)
        {
        case INITMOVEBAFFLE:
            strcpy((char *)pgsub->valf, "INIT MOVE BAFFLE");
            break;
        case WAITFORMOVEBAFFLECMD:
            strcpy((char *)pgsub->valf, "WAIT MOVE BAFFLE CMD");
            break;
        case STARTMOVEBAFFLE:
            strcpy((char *)pgsub->valf, "START MOVE BAFFLE");
            break;
        case WAITFORMOVEBAFFLE:
            strcpy((char *)pgsub->valf, "WAIT MOVE BAFFLE");
            break;
        case TIMEOUTWAITFORMOVEBAFFLE:
            strcpy((char *)pgsub->valf, "TIMEOUT WAIT MOVE BAFFLE");
            break;
        default:
        strcpy((char *)pgsub->valf, "UNKNOWN CASE");
        }

    return (OK);
}

/* ===================================================================== */
