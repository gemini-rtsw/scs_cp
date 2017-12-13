/* $Id: dmDrive.c,v 1.3 2006/09/28 16:00:08 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * dmDrive.c
 * 
 * PURPOSE
 * -------
 * The function reads the contents of the tilt system reflective memory
 * status buffer and make the items available on the gensub output ports
 * where they can be monitored for display purposes
 * 
 * FUNCTION NAME(S)
 * ----------------
 * realDrive    - read tilt system data buffer and write to gensub ports
 * displayScs   - read scs system data buffer and write to gensub ports
 * statusDrive  - read tilt system status word and write to gensub ports
 * real2Drive   - read more tilt system values and write to gensub ports
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
 * 17-Oct-1997: Original (srp)
 * 29-Oct-1997: Add displayScs routines
 * 07-May-1999: Added RCS id
 * 15-Dec-1999: Added real2Drive
 * 06-Dec-2017: Begin EPICS OSI conversion (mdw)
 *
 */
/* INDENT ON */
/* ===================================================================== */
#include <timeLib.h>        /* For timeNow */
#include "archive.h"        /* For refMemFree */
#include "chop.h"           /* For chopIsOn, getSyncMask */
#include "control.h"        /* For simLevel, scsPtr, scsBase, m2Ptr, 
                                       m2MemFree */
#include "eventBus.h"       /* for getSyncMask() */
#include "dmDrive.h"
#include "utilities.h"      /* For errorLog, debugLevel */

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * realDrive
 * 
 * Purpose:
 * Read status from the tilt system reflective memory and make it available
 * to the dm screens through epics records
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = realDrive(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 * 
 * Parameters out:
 *
 *      < pgsub->vala   long    checksum
 *      < pgsub->valb   long    NR
 *      < pgsub->valc   float   xTilt
 *      < pgsub->vald   float   yTilt
 *      < pgsub->vale   float   zFocus
 *      < pgsub->valf   float   actuator1
 *      < pgsub->valg   float   actuator2
 *      < pgsub->valh   float   actuator3
 *      < pgsub->vali   long    inPosition
 *      < pgsub->valj   long    chopTransition
 *      < pgsub->valk   long    statusWord
 *      < pgsub->vall   long    heartbeat
 *      < pgsub->valm   long    beamPosition
 *      < pgsub->valn   float   xPosition
 *      < pgsub->valo   float   yPosition
 *      < pgsub->valp   long    deployBaffle
 *      < pgsub->valq   long    centralBaffle
 *      < pgsub->valr   float   baffleEncoderA
 *      < pgsub->vals   float   baffleEncoderB
 *      < pgsub->valt   float   baffleEncoderC
 *      < pgsub->valu   long    topEnd
 * 
 * Return value:
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      ! refMemFree
 *
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 22-Aug-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */
long    realDrive (struct genSubRecord * pgsub)
{

    /* note sense reversal for in position, on ref mem 0 = in position */

    epicsMutexLock(refMemFree);

    *(long *) pgsub->vala   = scsPtr->page1.checksum;
    *(long *) pgsub->valb   = scsPtr->page1.NR;
    *(double *) pgsub->valc = scsPtr->page1.xTilt;
    *(double *) pgsub->vald = scsPtr->page1.yTilt;
    *(double *) pgsub->vale = scsPtr->page1.zFocus;
    *(double *) pgsub->valf = scsPtr->page1.actuator1;
    *(double *) pgsub->valg = scsPtr->page1.actuator2;
    *(double *) pgsub->valh = scsPtr->page1.actuator3;
    *(long *) pgsub->vali   = scsPtr->page1.inPosition;

    if (scsPtr->page1.chopTransition)
       *(long *) pgsub->valj   = 0;
    else
       *(long *) pgsub->valj   = 1;

    *(long *) pgsub->valk   = (long) scsPtr->page1.statusWord.all;
    *(long *) pgsub->vall   = scsPtr->page1.heartbeat;
    *(long *) pgsub->valm   = scsPtr->page1.beamPosition;
    *(double *) pgsub->valn = scsPtr->page1.xPosition;
    *(double *) pgsub->valo = scsPtr->page1.yPosition;
    *(long *) pgsub->valp   = scsPtr->page1.deployBaffle;
    *(long *) pgsub->valq   = scsPtr->page1.centralBaffle;
    *(double *) pgsub->valr = scsPtr->page1.baffleEncoderA;
    *(double *) pgsub->vals = scsPtr->page1.baffleEncoderB;
    *(double *) pgsub->valt = scsPtr->page1.baffleEncoderC;
    *(long *) pgsub->valu   = scsPtr->page1.topEnd;

    epicsMutexUnlock(refMemFree);

    return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * real2Drive
 * 
 * Purpose:
 * Read more status from the tilt system reflective memory and make it available
 * to the dm screens through epics records
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = real2Drive(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 * 
 * Parameters out:
 *
 *              < pgsub->vala   float   upperBearingAngle
 *              < pgsub->valb   float   lowerBearingAngle
 * 
 * Return value:
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      ! refMemFree
 *
 * 
 * Requirements:
 * 
 * Author:
 * Dayle Kotturi
 * 
 * History:
 * 15-Dec-1999: Original(kdk)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    real2Drive (struct genSubRecord * pgsub)
{

    epicsMutexLock(refMemFree);
    *(double *) pgsub->vala = scsPtr->page1.upperBearingAngle;
    *(double *) pgsub->valb = scsPtr->page1.lowerBearingAngle;
    epicsMutexUnlock(refMemFree);

    return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * displayScs
 * 
 * Purpose:
 * Read scs > tilt system data from the reflective memory and make it available
 * to the dm screens through epics records
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = displayScs(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 * 
 * Parameters out:
 *
 *      < pgsub->vala   float   xTiltGuide
 *      < pgsub->valb   float   yTiltGuide
 *      < pgsub->valc   float   zFocusGuide
 *      < pgsub->vald   float   AxTilt
 *      < pgsub->vale   float   AyTilt
 *      < pgsub->valf   float   BxTilt
 *      < pgsub->valg   float   ByTilt
 *      < pgsub->valh   float   CxTilt
 *      < pgsub->vali   float   CyTilt
 *      < pgsub->valj   float   actuator1
 *      < pgsub->valk   float   actuator2
 *      < pgsub->vall   float   actuator3
 *      < pgsub->valm   long    heartbeat
 *      < pgsub->valn   long    xDemand
 *      < pgsub->valo   long    yDemand
 *      < pgsub->valp   long    centralBaffle
 *      < pgsub->valq   long    deployBaffle
 *      < pgsub->valr   float   chopProfile
 *      < pgsub->vals   float   chopFrequency
 *      < pgsub->valt   long    chopDutyCycle
 *      < pgsub->valu   long    frame sequence number NS
 * 
 * Return value:
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      ! refMemFree
 *
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 29-Oct-1997: Original(srp)
 * 03-Jun-1998: Add frame sequence number to output U
 */

/* INDENT ON */
/* ===================================================================== */

long    displayScs (struct genSubRecord * pgsub)
{
    if(simLevel != 0)
    {
        epicsMutexLock(m2MemFree);  
        *(double *) pgsub->vala = m2Ptr->page0.xTiltGuide;
        *(double *) pgsub->valb = m2Ptr->page0.yTiltGuide;
        *(double *) pgsub->valc = m2Ptr->page0.zFocusGuide;
        *(double *) pgsub->vald = m2Ptr->page0.AxTilt;
        *(double *) pgsub->vale = m2Ptr->page0.AyTilt;
        *(double *) pgsub->valf = m2Ptr->page0.BxTilt;
        *(double *) pgsub->valg = m2Ptr->page0.ByTilt;
        *(double *) pgsub->valh = m2Ptr->page0.CxTilt;
        *(double *) pgsub->vali = m2Ptr->page0.CyTilt;
        *(double *) pgsub->valj = m2Ptr->page0.actuator1;
        *(double *) pgsub->valk = m2Ptr->page0.actuator2;
        *(double *) pgsub->vall = m2Ptr->page0.actuator3;
        *(long *) pgsub->valm = m2Ptr->page0.heartbeat;
        *(double *) pgsub->valn = m2Ptr->page0.xDemand;
        *(double *) pgsub->valo = m2Ptr->page0.yDemand;
        *(long *) pgsub->valp = m2Ptr->page0.centralBaffle;
        *(long *) pgsub->valq = m2Ptr->page0.deployBaffle;
        *(long *) pgsub->valr = m2Ptr->page0.chopProfile;
        *(double *) pgsub->vals = m2Ptr->page0.chopFrequency;
        *(double *) pgsub->valt = m2Ptr->page0.chopDutyCycle;
        *(long *) pgsub->valu = m2Ptr->page0.NS;
        epicsMutexUnlock(m2MemFree);
    }
    else
    {
        /* using real reflective memory, reference to scsBase pointer */
        if(scsBase != NULL)
        {
            *(double *) pgsub->vala = scsBase->page0.xTiltGuide;
            *(double *) pgsub->valb = scsBase->page0.yTiltGuide;
            *(double *) pgsub->valc = scsBase->page0.zFocusGuide;
            *(double *) pgsub->vald = scsBase->page0.AxTilt;
            *(double *) pgsub->vale = scsBase->page0.AyTilt;
            *(double *) pgsub->valf = scsBase->page0.BxTilt;
            *(double *) pgsub->valg = scsBase->page0.ByTilt;
            *(double *) pgsub->valh = scsBase->page0.CxTilt;
            *(double *) pgsub->vali = scsBase->page0.CyTilt;
            *(double *) pgsub->valj = scsBase->page0.actuator1;
            *(double *) pgsub->valk = scsBase->page0.actuator2;
            *(double *) pgsub->vall = scsBase->page0.actuator3;
            *(long *) pgsub->valm = scsBase->page0.heartbeat;
            *(double *) pgsub->valn = scsBase->page0.xDemand;
            *(double *) pgsub->valo = scsBase->page0.yDemand;
            *(long *) pgsub->valp = scsBase->page0.centralBaffle;
            *(long *) pgsub->valq = scsBase->page0.deployBaffle;
            *(long *) pgsub->valr = scsBase->page0.chopProfile;
            *(double *) pgsub->vals = scsBase->page0.chopFrequency;
            *(double *) pgsub->valt = scsBase->page0.chopDutyCycle;
            *(long *) pgsub->valu = scsBase->page0.NS;
        }
    }
    return(OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * displayScs2
 * 
 * Purpose:
 * Read scs > tilt system data from the reflective memory and make it available
 * to the dm screens through epics records
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = displayScs2(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 * 
 * Parameters out:
 *
 *      < pgsub->vala   float   zFocus
 *      < pgsub->valb   float   zGuide
 *      < pgsub->valc   float   rawXGuide
 *      < pgsub->vald   float   rawYGuide
 *      < pgsub->vale   float   rawZGuide
 *      < pgsub->valf   float   xGrossTiltDmd
 *      < pgsub->valg   float   yGrossTiltDmd
 *      < pgsub->valh   float   zGrossTiltDmd
 *      < pgsub->vali   int     chopInPosition
 *      < pgsub->valj   int     currentBeam
 *      < pgsub->valk   float   xTilt
 *      < pgsub->vall   float   yTilt
 *      < pgsub->valm   float   zFocus
 *      < pgsub->valn   float   xPosition
 *      < pgsub->valo   float   yPosition
 *      < pgsub->valp   float   time
 * 
 * Return value:
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      ! refMemFree
 *
 * 
 * Requirements:
 * 
 * Author:
 * Dayle Kotturi  (dayle@gemini.edu)
 * 
 * History:
 * 16-Feb-2000: Original(kdk)
 */

/* INDENT ON */
/* ===================================================================== */

long    displayScs2 (struct genSubRecord * pgsub)
{
#ifdef NEVER
    double timeStamp;
    double timeLogged;
#endif

    /* static int firstTime = 1;  display xycom registers once only */

    if(simLevel != 0)
    {
	epicsMutexLock(m2MemFree);  
	*(double *) pgsub->vala = m2Ptr->page0.zFocus;
	*(double *) pgsub->valb = m2Ptr->page0.zGuide; 
	*(double *) pgsub->valc = m2Ptr->page0.rawXGuide;
	*(double *) pgsub->vald = m2Ptr->page0.rawYGuide;
	*(double *) pgsub->vale = m2Ptr->page0.rawZGuide;
        *(double *) pgsub->valf = m2Ptr->page0.xGrossTiltDmd;
        *(double *) pgsub->valg = m2Ptr->page0.yGrossTiltDmd;
	epicsMutexUnlock(m2MemFree);
    }
    else
    {
	/* using real reflective memory, reference to scsBase pointer */
	if(scsBase != NULL)
	{
	    *(double *) pgsub->vala = scsBase->page0.zFocus;
	    *(double *) pgsub->valb = scsBase->page0.zGuide; 
	    *(double *) pgsub->valc = scsBase->page0.rawXGuide;
	    *(double *) pgsub->vald = scsBase->page0.rawYGuide;
	    *(double *) pgsub->vale = scsBase->page0.rawZGuide;
            *(double *) pgsub->valf = scsBase->page0.xGrossTiltDmd;
            *(double *) pgsub->valg = scsBase->page0.yGrossTiltDmd;

	}
	else
	{
	    errorLog ("displayScs2 - NULL pointer to scsBase\n", 1, ON);
	    return (ERROR);
	}
    }

    /* Either way, simulating or not, put eventData values on display */
    epicsMutexLock(eventDataSem);

    /* event bus stuff */
    *(long   *) pgsub->valh = eventData.inPosition;
    *(long   *) pgsub->vali = eventData.currentBeam;
            /*
            *(double *) pgsub->valj = eventData.xTilt;
            *(double *) pgsub->valk = eventData.yTilt;
            *(double *) pgsub->vall = eventData.zFocus;
            *(double *) pgsub->valm = eventData.xPosition;
            *(double *) pgsub->valn = eventData.yPosition;
            *(double *) pgsub->valo = eventData.time; 
	    timeLogged = eventData.time;*/
    epicsMutexUnlock(eventDataSem);
        
    *(long *) pgsub->valp = getSyncMask();

    return(OK);
}
/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * statusDrive
 * 
 * Purpose:
 * Take the 32 bit tilt systsem status word and split into
 * four chars. Write these to the gensub output ports where
 * they can be picked up by epics records
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = statusDrive(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 * 
 * Parameters out:
 *      < pgsub->vala   char    statusWord.byte[3];
 *      < pgsub->valb   char    statusWord.byte[2];
 *      < pgsub->valc   char    statusWord.byte[1];
 *      < pgsub->vald   char    statusWord.byte[0];
 * 
 * Return value:
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
 * 22-Aug-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */
long    statusDrive (struct genSubRecord * pgsub)
{

    epicsMutexLock(refMemFree);

    /* read the m2 status word as bytes and put out to ports */
    *(long *) pgsub->vala = (char) scsPtr->page1.statusWord.byte[3];
    *(long *) pgsub->valb = (char) scsPtr->page1.statusWord.byte[2];
    *(long *) pgsub->valc = (char) scsPtr->page1.statusWord.byte[1];
    *(long *) pgsub->vald = (char) scsPtr->page1.statusWord.byte[0];

    /* read enclosure temperature */
    *(double *) pgsub->vale = scsPtr->page1.enclosureTemp;

    epicsMutexUnlock(refMemFree);

    return (OK);
}

