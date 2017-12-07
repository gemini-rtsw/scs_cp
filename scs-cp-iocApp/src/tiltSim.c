/* $Id: tiltSim.c,v 1.1 2002/02/05 13:19:51 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * tiltSim.c
 * 
 * PURPOSE
 * -------
 * Functions to support simulation of the tilt and positioning systems
 * 
 * FUNCTION NAME(S)
 * ----------------
 * mechSim      - simulation of the mechanisms
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
 * 06-Nov-1997: Original (srp)
 * 11-Feb-1998: Add semaphore timeout handling
 * 24-Feb-1998: typo on enable port for mechanisms should be b not d
 * 15-Mar-1998: combine individual mechanism simulations into this one routine
 * 07-May-1999: Added RCS id
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include "tiltSim.h"
#include "control.h"        /* For m2Ptr, m2MemFree */
#include "utilities.h"      /* For tilt2act, errorLog */
#include "chopControl.h"    /* For chopEventSem */


#define CHOP_DRIVE_SCAN_RATE    20

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * mechSim
 * 
 * Purpose:
 * Simulate tilt system mechanical axes
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = mechSim(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 *      > pgsub->a  long    mechanism enable { ON | OFF}
 *      > pgsub->b  long    chop on { ON | OFF }
 *      > pgsub->c  long    chop profile { TWOPOINT | THREEPOINT | TRIANGLE }
 *      > pgsub->d  double  chop frequency (Hz)
 *      > pgsub->e  long    sync source { INTERNAL | EXTERNAL }
 * 
 * Parameters out:
 *      < pgsub->vala   long    in position indicator { ON | OFF }
 *      < pgsub->valb   double  x tilt position
 *      < pgsub->valc   double  x tilt position
 *      < pgsub->vald   double  z focus position
 *      < pgsub->vale   double  x position
 *      < pgsub->valf   double  y position
 *      < pgsub->valg   double  actuator 1 position
 *      < pgsub->valh   double  actuator 2 position
 *      < pgsub->vali   double  actuator 3 position
 *
 * Return value:
 *      < status    long    OK or ERROR
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * None
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 28-Oct-1997: Original(srp)
 * 24-Feb-1998: typo on enable port for mechanisms should be b not d
 * 15-Mar-1998: combine individual mechanism simulations into this one routine
 * 
 */

/* INDENT ON */
/* ===================================================================== */
#include <stdio.h>
#include <math.h>
long mechSim (struct genSubRecord * pgsub)
{
    long coincidence;
    location position;
    double checkFreq;
    static int beam = BEAMA;
    static long tickThreshold, profile, syncSource, remainingTicks, doChop;
    static double xTiltCustom = 0, yTiltCustom = 0;

    static int stateCount = 0, ticker = 0;
    char msg[81];

    /* define filter constants */

    struct
    {
    double a1;
    double a2;
    double b1;
    double b2;
    }   tiltFilter =
    {
    0.55874,
    0.25735,
    -0.29240,
    0.10849
    };

    int sequence[3][4] = { {BEAMA, BEAMB, BEAMA, BEAMB}, { BEAMA, BEAMB, BEAMA, BEAMC}, {A2BRAMP, B2ARAMP, A2BRAMP, B2ARAMP}};

    /* define history arrays for the filters */

    static double xin[3] = {0.0, 0.0, 0.0}, xout[3] = {0.0, 0.0, 0.0};
    static double yin[3] = {0.0, 0.0, 0.0}, yout[3] = {0.0, 0.0, 0.0};
    static double zin[3] = {0.0, 0.0, 0.0}, zout[3] = {0.0, 0.0, 0.0};
    static double xpin[3] = {0.0, 0.0, 0.0}, xpout[3] = {0.0, 0.0, 0.0};
    static double ypin[3] = {0.0, 0.0, 0.0}, ypout[3] = {0.0, 0.0, 0.0};

    /* define structure to capture reflective memory demands */

    Demands tiltDemand;

    double tolerance[5] = {10.0, 10.0, 10.0, 10.0, 10.0};

    /* check that the mechanism is enabled */

    if (*(long *) pgsub->a == OFF)
    {
    /* get demands from reflective memory */

    epicsMutexLock(m2MemFree);
    tiltDemand.xTiltA = m2Ptr->page0.AxTilt;
    tiltDemand.xTiltB = m2Ptr->page0.BxTilt;
    tiltDemand.xTiltC = m2Ptr->page0.CxTilt;

    tiltDemand.yTiltA = m2Ptr->page0.AyTilt;
    tiltDemand.yTiltB = m2Ptr->page0.ByTilt;
    tiltDemand.yTiltC = m2Ptr->page0.CyTilt;

    tiltDemand.xGuide = m2Ptr->page0.xTiltGuide;
    tiltDemand.yGuide = m2Ptr->page0.yTiltGuide;

    tiltDemand.zFocus = m2Ptr->page0.zFocusGuide;
    tiltDemand.xPosition = m2Ptr->page0.xDemand;
    tiltDemand.yPosition = m2Ptr->page0.yDemand;

    tolerance[XTILT] = m2Ptr->page0.xTiltTolerance;
    tolerance[YTILT] = m2Ptr->page0.yTiltTolerance;
    tolerance[FOCUS] = m2Ptr->page0.zFocusTolerance;
    tolerance[XPOSITION] = m2Ptr->page0.xPositionTolerance;
    tolerance[YPOSITION] = m2Ptr->page0.yPositionTolerance;

    epicsMutexUnlock(m2MemFree);

    /* look at driveChop requirements */

    doChop = *(long *) pgsub->b;

    /* if chopping is active */

    if (doChop == ON)
    {
        profile = *(long *) pgsub->c;
        checkFreq = *(double *) pgsub->d;
        syncSource = *(long *) pgsub->e;

        /* check index limits then look up current beam */

        if (profile < TWOPOINT || profile > TRIANGLE)
        {
        errorLog ("mechanism - profile out of range", 1, ON);
        return (ERROR);
        }
        else if (stateCount < 0 || stateCount > 3)
        {
        errorLog ("mechanism - statecount out of range", 1, ON);
        return (ERROR);
        }
        else if (checkFreq < 0.1 || checkFreq > 10.0)
        {
        sprintf (msg, "chop freq error, pgsub->c = %f\n", checkFreq);
        errlogPrintf("%s", msg);
        return (ERROR);
        }

        beam = sequence[profile][stateCount];

        /* calculate threshold limits */

        if (profile == THREEPOINT)
        tickThreshold = (long) (0.25 * CHOP_DRIVE_SCAN_RATE / checkFreq);
        else
        tickThreshold = (long) (0.5 * CHOP_DRIVE_SCAN_RATE / checkFreq);


        /* if the chop threshold has been exceeded, force a transition */

        if ( ((syncSource == INTERNAL) && (ticker > tickThreshold)) || 
             ((syncSource == EXTERNAL) && (epicsEventTryWait(chopEventSem))))
        {
           ticker = 0;
           stateCount++;

           /*
            * select count as modulo 3 for three point otherwise modulo
            * 2
            */

           if (profile == THREEPOINT)
           {
               if (stateCount > 3)
               stateCount = 0;
           }
           else
           {
               if (stateCount > 1)
               stateCount = 0;
           }
        }
        else
        {
           /* no chop transition on this tick */

           ticker++;

           /* calculate triangular chop demands for x and y tilts */

           if (profile == TRIANGLE)
           {
               if ((remainingTicks = (tickThreshold - ticker)) > 0)
               {
                  if (stateCount == 0)
                  {
                      xTiltCustom = xTiltCustom + (tiltDemand.xTiltB - xTiltCustom) / remainingTicks;
                      yTiltCustom = yTiltCustom + (tiltDemand.yTiltB - yTiltCustom) / remainingTicks;
                  }
                  else
                  {
                      xTiltCustom = xTiltCustom + (tiltDemand.xTiltA - xTiltCustom) / remainingTicks;
                      yTiltCustom = yTiltCustom + (tiltDemand.yTiltA - yTiltCustom) / remainingTicks;
                  }
               }
           }
        }
    }
    else
    {
        /* if chop off then zero the outputs and reset beam position */

        stateCount = 0;
        ticker = 0;
        beam = BEAMA;
        xTiltCustom = 0;
        yTiltCustom = 0;
    }

    /*
     * once demands and chop state have been established, apply these
     * demands to the mechanisms
     */

    /*
     * read current x and y tilt demand to filter arrays according to
     * current beam
     */

    switch (beam)
    {
    case BEAMA:
        xin[0] = tiltDemand.xTiltA;
        yin[0] = tiltDemand.yTiltA;
        break;

    case BEAMB:
        xin[0] = tiltDemand.xTiltB;
        yin[0] = tiltDemand.yTiltB;
        break;

    case BEAMC:
        xin[0] = tiltDemand.xTiltC;
        yin[0] = tiltDemand.yTiltC;
        break;

    case A2BRAMP:
    case B2ARAMP:
        xin[0] = xTiltCustom;
        yin[0] = yTiltCustom;
        break;

    default:
        errorLog ("mechSim - current beam out of range", 1, ON);
        return (ERROR);
    }

    /* focus, xposition and y position are not beam dependent */

    zin[0] = tiltDemand.zFocus;
    xpin[0] = tiltDemand.xPosition;
    ypin[0] = tiltDemand.yPosition;

    /* add guide components to the tilt axes */

    xin[0] += tiltDemand.xGuide;
    yin[0] += tiltDemand.yGuide;

    /* filter the demands for each axis */

    xout[0] = tiltFilter.a1 * xin[1] + tiltFilter.a2 * xin[2] - tiltFilter.b1 * xout[1] - tiltFilter.b2 * xout[2];
    yout[0] = tiltFilter.a1 * yin[1] + tiltFilter.a2 * yin[2] - tiltFilter.b1 * yout[1] - tiltFilter.b2 * yout[2];
    zout[0] = tiltFilter.a1 * zin[1] + tiltFilter.a2 * zin[2] - tiltFilter.b1 * zout[1] - tiltFilter.b2 * zout[2];
    xpout[0] = tiltFilter.a1 * xpin[1] + tiltFilter.a2 * xpin[2] - tiltFilter.b1 * xpout[1] - tiltFilter.b2 * xpout[2];
    ypout[0] = tiltFilter.a1 * ypin[1] + tiltFilter.a2 * ypin[2] - tiltFilter.b1 * ypout[1] - tiltFilter.b2 * ypout[2];

    /* calculate in position indication */

    coincidence = (long) ON;

    if ((fabs (xin[0] - xout[0])) > tolerance[XTILT])
        coincidence = (long) OFF;
    else if ((fabs (yin[0] - yout[0])) > tolerance[YTILT])
        coincidence = (long) OFF;
    else if ((fabs (zin[0] - zout[0])) > tolerance[FOCUS])
        coincidence = (long) OFF;
    else if ((fabs (xpin[0] - xpout[0])) > tolerance[XPOSITION])
        coincidence = (long) OFF;
    else if ((fabs (ypin[0] - ypout[0])) > tolerance[YPOSITION])
        coincidence = (long) OFF;

    /* calculate actuator positions corresponding to tilt and focus */

    position.xTilt = xout[0];
    position.yTilt = yout[0];
    position.zFocus = zout[0];

    tilt2act (&position);

    /* write results back to reflective memory */

    epicsMutexLock(m2MemFree);
    m2Ptr->page1.xTilt = (float) xout[0];
    m2Ptr->page1.yTilt = (float) yout[0];
    m2Ptr->page1.zFocus = (float) zout[0];
    m2Ptr->page1.xPosition = (float) xpout[0];
    m2Ptr->page1.yPosition = (float) ypout[0];

    m2Ptr->page1.actuator1 = (float) position.actuator1;
    m2Ptr->page1.actuator2 = (float) position.actuator2;
    m2Ptr->page1.actuator3 = (float) position.actuator3;

    /* m2Ptr->page1.inPosition = !coincidence;*/
    m2Ptr->page1.inPosition = coincidence;
    m2Ptr->page1.beamPosition = beam;

    epicsMutexUnlock(m2MemFree);

    /* write calculated position to ouput ports */

    *(long *) pgsub->vala = coincidence;
    *(double *) pgsub->valb = xout[0];
    *(double *) pgsub->valc = yout[0];
    *(double *) pgsub->vald = zout[0];
    *(double *) pgsub->vale = xpout[0];
    *(double *) pgsub->valf = ypout[0];
    *(double *) pgsub->valg = position.actuator1;
    *(double *) pgsub->valh = position.actuator2;
    *(double *) pgsub->vali = position.actuator3;

    /* ripple filter sample histories */

    xin[2] = xin[1]; xin[1] = xin[0]; xout[2] = xout[1]; xout[1] = xout[0];
    yin[2] = yin[1]; yin[1] = yin[0]; yout[2] = yout[1]; yout[1] = yout[0];
    zin[2] = zin[1]; zin[1] = zin[0]; zout[2] = zout[1]; zout[1] = zout[0];
    xpin[2] = xpin[1]; xpin[1] = xpin[0]; xpout[2] = xpout[1]; xpout[1] = xpout[0];
    ypin[2] = ypin[1]; ypin[1] = ypin[0]; ypout[2] = ypout[1]; ypout[1] = ypout[0];
    }

    return (OK);
}

