/* $Id: showEngineering.c,v 1.7 2008/09/24 01:15:49 mrippa Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * showEngineering.c
 * 
 * PURPOSE
 * -------
 * Read diagnostic data from the M2 system on the reflective memory and
 * package for display on EPICS records
 * 
 * FUNCTION NAME(S)
 * ----------------
 * readM2Diagnostics    - Write diagnostic data from fg osp structure to gensub
 *            outputs for display
 * gensubFanDoubles - receive array of doubles on port A, write elements to
 *            individual output ports
 * 
 * DEPENDENCIES
 * ------------
 *
 * LIMITATIONS
 * -----------
 * 
 * AUTHOR
 * ------
 * Sean Prior (srp@roe.ac.uk)
 * 
 * HISTORY
 * -------
 * 
 * 28-Jan-1999: Original (srp)
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include "showEngineering.h"
#include "m2ParseMsg.h"
#include "control.h"            /* For writeCommand, simLevel, scsBase,
                                               m2Ptr */
#include "xycom.h"              /* For PORT_7_ADDR define */
#include "m2Log.h"              /* For m2LogActive */
#include "utilities.h"          /* For tilt2act */

#include <cad.h>
#include <logLib.h>             /* For logMsg */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sysLib.h>             /* For sysClkRateGet */
#include <taskLib.h>            /* Spawning and deleting tasks */


/* specify constant definitions */

#define M2_DIAGNOSTIC_PAGE_ADDRESS (0xf0a00840)
#define CEM_ON           47
#define CEM_OFF          48
#define TTL_SERVO_ON     49
#define TTL_SERVO_OFF    50
#define TOGGLE_CEM_POWER 51
#define OPEN_LOG         52
#define START_LOG        53
#define STOP_LOG         54
#define CLOSE_LOG        55

#define M2POWERON   0x1 /* 00000001 */
#define M2POWEROFF  0xfe    /* 11111110 */
#define M2SERVOON   0x2 /* 00000010 */
#define M2SERVOOFF  0xfd    /* 11111101 */

#define IN_POSITION_LIMIT 100.0 /* actuator error within this range (microns), OK to turn on servos */

typedef struct
{
    double  item1;
    double  item2;
    double  item3;
    double  item4;
    double  item5;
    double  item6;
    double  item7;
    double  item8;
} engData;

static char *primitiveName[] =
{
    "FAST_ONLY",
    "POSITION",
    "CHOP_ON",
    "CHOP_OFF",
    "SYNC_SOURCE_SCS",
    "SYNC_SOURCE_M2",
    "ACT_PWR_ON",
    "ACT_PWR_OFF",
    "CMD_INIT",
    "CMD_RESET",
    "CMD_TEST",
    "MSTART",
    "MEND",
    "VIBSTART",
    "VEND",
    "MOFFLON",
    "MOFFLOFF",
    "DECS_ON",
    "DECS_OFF",
    "DECS_PAUSE",
    "DECS_CONTINUE",
    "DECS_FREEZE",
    "DECS_UNFREEZE",
    "DECS_ZERO",
    "TILT_SPACE",
    "ACTUATOR_SPACE",
    "BAFFLE_CHANGE",
    "DECS_CHANGE",
    "BANDWIDTH_CHANGE",
    "TOLERANCE_CHANGE",
    "CHOP_CHANGE",
    "DIAGNOSTICS_REQUEST",
    "UPDATE_XY_RANGE",
    "UPDATE_XPYP_RANGE",
    "VSTARTDRVFOL",
    "VDRVFOL",
    "VSTOPDRVFOL",
    "MSTARTDRVOFL",
    "MDRVOFL",
    "MSTOPDRVOFL",
    "DBDRV",
    "CBDRV",
    "XYDRV",
    "VDECS_ON",
    "VDECS_OFF",
    "LOGGING_ON",
    "LOGGING_OFF",              /* 46 */
    "CEM_ON",
    "CEM_OFF",
    "TTL_SERVO_ON",
    "TTL_SERVO_OFF",
    "TOGGLE_CEM_POWER",
    "OPEN_LOG",
    "START_LOG",
    "STOP_LOG",
    "CLOSE_LOG",
    "SEQUENCE1",
    "SEQUENCE2",
    "SEQUENCE3",
    "CMD_XYINIT",
    "XY_DEADBAND_CHANGE",
    "SCS_TIME_UPDATE",
    NULL
};


/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * readM2Diagnostics
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long status;
 *
 * long    gensub(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Read data from m2 diagnostics page and package for display by EPICS records
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Jan-1999  Original version  (srp)
 *-
 */

long    readM2Diagnostics (struct genSubRecord * pgsub)
{ 
    char junk;
    int index = 0;
    double act1[21], act2[21], act3[21], sys[21];
    location position;
    long servoInPosition; 
    memMap *ptr;
    char msg[80];
    long errorSystem, errorCode;
    static long lastErrorSystem, lastErrorCode;  

    if(simLevel == 0)
      ptr = scsBase;
    else
      ptr = m2Ptr;

    for(index = 0; index < 21; index++)
      {
        act1[index] = 0.0;
        act2[index] = 0.0;
        act3[index] = 0.0;
        sys[index] = 0.0;
      }

    /* check 5588 synchro card memory location, exit if not found */

    if (vxMemProbe ((void *)ptr, VX_READ, 1, &junk) != OK)    
    {
      logMsg("readM2Diagnostics - synchro card not detected at address %p\n", (int)ptr, 0, 0, 0, 0, 0);
        return(ERROR);
    }

    /* read data from m2 diagnostic page into local arrays */

    act1[0] = ptr->page1.actuator1;
    act1[1] = ptr->m2Eng.follow1;
    act1[2] = ptr->m2Eng.current1;
    act1[3] = ptr->m2Eng.kaman1;
    act1[4] = ptr->m2Eng.integ1;

    act2[0] = ptr->page1.actuator2;
    act2[1] = ptr->m2Eng.follow2;
    act2[2] = ptr->m2Eng.current2;
    act2[3] = ptr->m2Eng.kaman2;
    act2[4] = ptr->m2Eng.integ2;

    act3[0] = ptr->page1.actuator3;
    act3[1] = ptr->m2Eng.follow3;
    act3[2] = ptr->m2Eng.current3;
    act3[3] = ptr->m2Eng.kaman3;
    act3[4] = ptr->m2Eng.integ3;

    /* Tracking down the corrupted RM values. The following 
       are copies of the MCDSP real values, the scale factors 
       and the computed values of xTilt, yTilt and zFocus */

    act1[5] = ptr->m2Eng.TMS2realXTilt;
    act1[6] = ptr->m2Eng.TMS2realYTilt;
    act1[7] = ptr->m2Eng.TMS2realZFocus;

    act1[8] = ptr->m2Eng.xTilt;
    act1[9] = ptr->m2Eng.yTilt;
    act1[10] = ptr->m2Eng.zFocus;

    act1[11] = ptr->m2Eng.rad2arcsec;
    act1[12] = ptr->m2Eng.mm2um;


    /* write packaged arrays to val fields */

    memcpy ((double *)pgsub->vala, act3, 21*sizeof (double));
    memcpy ((double *)pgsub->valb, act2, 21*sizeof (double));
    memcpy ((double *)pgsub->valc, act1, 21*sizeof (double));
    memcpy ((double *)pgsub->vald, sys,  21*sizeof (double));

    /* calculate whether position error is small enough to */
    /* turn on fine control */

    /* fetch set point values from reflective memory*/
    /* calculate actuator positions corresponding to tilt and focus */

    position.xTilt = ptr->page0.AxTilt;
    position.yTilt = ptr->page0.AyTilt;
    position.zFocus = ptr->page0.zFocusGuide;

    tilt2act (&position);

    if((fabs(position.actuator1-ptr->page1.actuator1) < (IN_POSITION_LIMIT+7)) &&
       (fabs(position.actuator2-ptr->page1.actuator2) < (IN_POSITION_LIMIT+7)) &&
       (fabs(position.actuator3-ptr->page1.actuator3) < (IN_POSITION_LIMIT+7))   )
    {
        servoInPosition = 1;
    }
    else
    {
        servoInPosition = 0;
    }

    /* write servoInPosition to output port e */

    *(long *)pgsub->vale = servoInPosition;

        /* Tracking down the corrupted RM values. The following 
           are copies of the MCDSP raw values and the frame number */
        *(long *) pgsub->valf = ptr->m2Eng.rawXTilt;
        *(long *) pgsub->valg = ptr->m2Eng.rawYTilt;
        *(long *) pgsub->valh = ptr->m2Eng.rawZFocus;
        *(long *) pgsub->vali = ptr->m2Eng.NR;

        /* write progress of initialization to output port j */
        *(long *) pgsub->valj = ptr->m2Eng.initState;

        /* read last recorded M2 error from RM */
        errorSystem = (long)(ptr->m2Eng.errorSystem);
        errorCode   = (long)(ptr->m2Eng.errorCode);

        /* based on these values, determine the error msg */
	/* note: errorSystem and errorCode must be > 0    */
        if ((errorSystem > 0) && (errorCode > 0))	
	{
            strcpy(msg, parseM2Msg(errorSystem, errorCode));

            /* if requested, display the error on the crate's display */
            if ((debugLevel > DEBUG_MIN) && (debugLevel <= DEBUG_MED))
            {
                printf("Last reported M2 error message: %s Sys=%ld; Code=%ld\n",
                    msg, errorSystem, errorCode);
            }

            /* Also log it once, whenever it changes */
            if ((errorSystem != lastErrorSystem) & (errorCode != lastErrorCode))
            {
                printf("M2 error: %s Sys=%ld; Code=%ld\n", 
                    msg, errorSystem, errorCode); 
		if ( (errorSystem == ERRLOG_XY ) ||
		     (errorSystem == ERRLOG_DBAF ) ||
		     (errorSystem == ERRLOG_CBAF ) ||
		     (errorSystem == ERRLOG_VCD && errorCode == VC_INIT_FOLLOW_TIMEOUT ) ||
		     (errorSystem == ERRLOG_MCD && errorCode == MC_INIT_SENSOR_SWITCH_TIMEOUT ) ||
		     (errorSystem == ERRLOG_MCD && errorCode == MC_INIT_OFFLOAD_TIMEOUT ) ||
		     (errorSystem == ERRLOG_MISC && errorCode == NO_DSP_DATA )) 
		   {
		   reportHealth(WARNING,msg); 
		   }
		else
		   {
		   reportHealth(BAD,msg); 
		   }
                lastErrorSystem = errorSystem;
                lastErrorCode = errorCode;
            }
	}

    return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubFanDoubles
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long status;
 *
 * long    gensubFanDoubles(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Receive array of double on port A, copy individual elements to output
 * ports. Primarily intended for display to dm screens.
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 23-Jan-1999  Original version    Sean Prior
 *-
 */

long    gensubFanDoubles (struct genSubRecord * pgsub)
{
    int index = 0;
    double    localArray[21];
    double    *ptr = NULL;

    ptr = (double *) pgsub->a;

    /* read in the array from port A */

    for (index = 0; index < 21; index++)
    {
        localArray[index] = *(ptr++);
    }

    /* write values to gensub outputs for screen display */

    *(double *)pgsub->vala = localArray[0];
    *(double *)pgsub->valb = localArray[1];
    *(double *)pgsub->valc = localArray[2];
    *(double *)pgsub->vald = localArray[3];
    *(double *)pgsub->vale = localArray[4];
    *(double *)pgsub->valf = localArray[5];
    *(double *)pgsub->valg = localArray[6];
    *(double *)pgsub->valh = localArray[7];
    *(double *)pgsub->vali = localArray[8];
    *(double *)pgsub->valj = localArray[9];
    *(double *)pgsub->valk = localArray[10];
    *(double *)pgsub->vall = localArray[11];
    *(double *)pgsub->valm = localArray[12];
    *(double *)pgsub->valn = localArray[13];
    *(double *)pgsub->valo = localArray[14];
    *(double *)pgsub->valp = localArray[15];
    *(double *)pgsub->valq = localArray[16];
    *(double *)pgsub->valr = localArray[17];
    *(double *)pgsub->vals = localArray[18];
    *(double *)pgsub->valt = localArray[19];
    *(double *)pgsub->valu = localArray[20];

    return (OK);
}

long    fillDiagnostics (double seed)
{ 
    char junk;
    engData *ptr;

    ptr = (engData *)M2_DIAGNOSTIC_PAGE_ADDRESS;

    /* check 5588 synchro card memory location, exit if not found */

    if (vxMemProbe ((void *)ptr, VX_READ, 1, &junk) != OK)    
    {
        logMsg("readM2Diagnostics - synchro card not detected at address %p\n", (int)ptr, 0, 0, 0, 0, 0);
        return(ERROR);
    }

    /* write data to m2 diagnostic page */

    ptr->item1 = seed;
    ptr->item2 = seed + 1.0;
    ptr->item3 = seed + 2.0;
    ptr->item4 = seed + 3.0;
    ptr->item5 = seed + 4.0;
    ptr->item6 = seed + 5.0;
    ptr->item7 = seed + 6.0;
    ptr->item8 = seed + 7.0;

    return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * issueM2Primitive
 * 
 * Purpose:
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = issueM2Primitive(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 *      > pcad->a   *string desired command name
 * 
 * Parameters out:
 *      < pcd->mess *string status message
 *      < pcad->vala    long command code number
 *      < pcad->valb    string command name
 *
 * 
 * Return value:
 *      < status    long
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 * 
 * Requirements:
 * None
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 29-Jan-1999: Original(srp)
 * 07-May-1999: Added RCS id
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    issueM2Primitive (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    static  long commandCode;
    char    test;
    volatile unsigned char    *portPtr;

    portPtr = (unsigned char *)PORT_7_ADDR;

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* search through recognised commands to identify command string */

        for(commandCode = FAST_ONLY; commandCode <= CLOSE_LOG; commandCode++)
        {
            if(!strcmp((char *)pcad->a, primitiveName[commandCode]))
               break;
        }
        
        /* check that command is in range */

        if (commandCode < FAST_ONLY || commandCode > CLOSE_LOG)
        {
            printf("command code = %ld\n", commandCode);
            strncpy (pcad->mess, "cmd not recognised", (MAX_STRING_SIZE - 1));
            status = CAD_REJECT;
        }
        else
        {
            strncpy (pcad->mess, "", (MAX_STRING_SIZE - 1));
        }

        /* if command uses the xycom card, check that it's there */

        if (commandCode >= CEM_ON && commandCode <= TOGGLE_CEM_POWER)
        {
            if (vxMemProbe ((void *) portPtr, READ, sizeof (char), &test) != OK)
            {
                strncpy (pcad->mess, "no xycom detected", (MAX_STRING_SIZE - 1));
                status = CAD_REJECT;
            }
        }

        break;

    case menuDirectiveSTART:

        /* write command number and name to output ports */

        *(long *)pcad->vala = (long)commandCode;
        strncpy(pcad->valb, primitiveName[commandCode], (MAX_STRING_SIZE - 1));

        if(commandCode == CEM_ON)
        {
            *portPtr = *portPtr | M2POWERON;
        }
        else if(commandCode == CEM_OFF)
        {
            *portPtr = *portPtr & M2POWEROFF;           
        }
        else if(commandCode == TTL_SERVO_ON)
        {
            *portPtr = *portPtr | M2SERVOON;
        }
        else if(commandCode == TTL_SERVO_OFF)
        {
            *portPtr = *portPtr & M2SERVOOFF;           
        }
        else if(commandCode == TOGGLE_CEM_POWER)
        {
            /* off */
            printf("Shutting off CEM power\n");
            *portPtr = *portPtr & M2SERVOOFF;  
            *portPtr = *portPtr & M2POWEROFF;

            /* ensure task is delayed for 2 seconds, instead
               of a time which depends on the internal clock rate */
            taskDelay(sysClkRateGet()*2);

            /* and on */
            printf("Turning CEM power back on\n");
            *portPtr = *portPtr | M2POWERON;
            *portPtr = *portPtr | M2SERVOON;
        }
        else if(commandCode == OPEN_LOG)
        {
            m2LogInit();
        }
        else if(commandCode == START_LOG)
        {
            m2LogActive = 1;
            printf("logging m2 active\n");
        }
        else if(commandCode == STOP_LOG)
        {
            m2LogActive = 0;
            printf("logging m2 stopped\n");
        }
        else if(commandCode == CLOSE_LOG)
        {
            m2LogActive = 0;
            m2LogClose();
        }       
        else if(commandCode == MSTART)
        {
            writeCommand(VIBSTART);
            writeCommand(MSTART);
        }
        else
        {
            writeCommand(commandCode);
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

