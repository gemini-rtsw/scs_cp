/* $Id: m2Log.c,v 1.1 2002/02/05 13:19:49 gemvx Exp $ */
/* INDENT OFF */
/*+
 * FILENAME
 * -------- 
 * m2log.c
 * 
 * PURPOSE
 * -------
 * Set of functions for logging engineering data from the m2 system
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
 * 04-Feb-1999: Original (srp)
 * 07-May-1999: Added RCS id
 */

/* INDENT ON */
/* ===================================================================== */
#include "m2Log.h"
#include "elgLib.h"

#include <string.h>
#include <stdio.h>

#include <tickLib.h>        /* For tickGet */
#include <timeLib.h>        /* For timeNow */
#include <logLib.h>         /* For logMsg */


#define MAX_FILENAME_SIZE 100
#define M2_DATA_SIZE      81
#define CB_RECORD_NB 	  5000

/* Define global variables */

static int logInterval = 5; /* every 10 is not enough, but 1 will hang crate */

/* Circular buffers for debugging what is going on in M2 */
static int cbM2Counter = 0;
static double cbTime[CB_RECORD_NB];
static unsigned long cbTick[CB_RECORD_NB];
static float cbXTilt[CB_RECORD_NB]; 
static float cbYTilt[CB_RECORD_NB];
static float cbZfocus[CB_RECORD_NB];
static float cbAct1[CB_RECORD_NB];
static float cbAct2[CB_RECORD_NB];
static float cbAct3[CB_RECORD_NB];
static float cbFol1[CB_RECORD_NB];
static float cbFol2[CB_RECORD_NB];
static float cbFol3[CB_RECORD_NB];
static float cbCur3[CB_RECORD_NB];
static float cbCur2[CB_RECORD_NB];
static float cbCur3[CB_RECORD_NB];
static float cbKam1[CB_RECORD_NB];
static float cbKam2[CB_RECORD_NB];
static float cbKam3[CB_RECORD_NB];
static float cbInteg1[CB_RECORD_NB];
static float cbInteg2[CB_RECORD_NB];
static float cbInteg3[CB_RECORD_NB];
static float cbAzGuide[CB_RECORD_NB];
static float cbElGuide[CB_RECORD_NB];
static float cbZCmd[CB_RECORD_NB];
static float cbAzCmd[CB_RECORD_NB];
static float cbElCmd[CB_RECORD_NB];
static float cbZUnused[CB_RECORD_NB];
static float cbAzTotCmd[CB_RECORD_NB];
static float cbElTotCmd[CB_RECORD_NB];
static float cbZTotCmd[CB_RECORD_NB];
static float cbRate1[CB_RECORD_NB];
static float cbRate2[CB_RECORD_NB];
static float cbRate3[CB_RECORD_NB];
static float cbErr1[CB_RECORD_NB];
static float cbErr2[CB_RECORD_NB];
static float cbErr3[CB_RECORD_NB];
static float cbRf1[CB_RECORD_NB];
static float cbRf2[CB_RECORD_NB];
static float cbRf3[CB_RECORD_NB];
static float cbCor1[CB_RECORD_NB];
static float cbCor2[CB_RECORD_NB];
static float cbCor3[CB_RECORD_NB];
static float cbF1[CB_RECORD_NB];
static float cbF2[CB_RECORD_NB];
static float cbF3[CB_RECORD_NB];

/* Declare externals */

int m2LogActive = FALSE;

/* Define function prototypes */

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * m2LogInit    - initialise m2 data logging function
 * 
 * Purpose:
 * Initialise the data logging functions with the filename specified
 * 
 *
 * Invocation:
 * m2LogInit
 *
 * Parameters in:
 *  none
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
 * 04-Feb-1999: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long m2LogInit (void)
{
    static char filename[MAX_FILENAME_SIZE];

    /* create filename to include timestamp e.g. m2_1999_02_04:20:23:54 */

    int     j, c[7];

    j = timeNowC (TAI, 3, c);

    if(j == 0)
	/* Note: limitation of amsLib: filename can only be 16 char long */
        /*sprintf (filename, "./m2_%d_%2.2d_%2.2d:%2.2d:%2.2d:%2.2d.log", c[0], c[1], c[2], c[3], c[4], c[5]);*/
      sprintf (filename, "./m2%2.2d:%2.2d:%2.2d.log", c[3], c[4], c[5]);
    else
    {
      sprintf (filename, "m2_timeError");
      return (ERROR);
    }

    printf ("m2LogInit - opening log file = %s\n", filename);

    /* initialise elg logging */

    elgOpen (filename);

    return (OK);
}

long m2LogClose (void)
{
    elgClose ();

    return (OK);
}

/* Later: needs header
 *
 * Purpose: Called each time through scsReceive, as is done for m2LoggerTask
 */
long m2CircBufferTask (memMap *ptr)
{
    double timeStamp;

    if (timeNow(&timeStamp) != OK)
    {
	errorLog ("m2CircBufferTask - error reading timeStamp\n", 1, ON);
	return (ERROR);
    }

    cbTick[cbM2Counter] = tickGet();
    cbTime[cbM2Counter] = timeStamp;
    cbXTilt[cbM2Counter] = ptr->page1.xTilt; 
    cbYTilt[cbM2Counter] = ptr->page1.yTilt;
    cbZfocus[cbM2Counter] = ptr->page1.zFocus;
    cbAct1[cbM2Counter] = ptr->page1.actuator1;
    cbAct2[cbM2Counter] = ptr->page1.actuator2;
    cbAct3[cbM2Counter] = ptr->page1.actuator3;
    cbFol1[cbM2Counter] = ptr->m2Eng.follow1;
    cbFol2[cbM2Counter] = ptr->m2Eng.follow2;
    cbFol3[cbM2Counter] = ptr->m2Eng.follow3;
    cbCur3[cbM2Counter] = ptr->m2Eng.current1;
    cbCur2[cbM2Counter] = ptr->m2Eng.current2;
    cbCur3[cbM2Counter] = ptr->m2Eng.current3;
    cbKam1[cbM2Counter] = ptr->m2Eng.kaman1;
    cbKam2[cbM2Counter] = ptr->m2Eng.kaman2;
    cbKam3[cbM2Counter] = ptr->m2Eng.kaman3;
    cbInteg1[cbM2Counter] = ptr->m2Eng.integ1;
    cbInteg2[cbM2Counter] = ptr->m2Eng.integ2;
    cbInteg3[cbM2Counter] = ptr->m2Eng.integ3;
    cbAzGuide[cbM2Counter] = ptr->m2Eng.azguide;
    cbElGuide[cbM2Counter] = ptr->m2Eng.elguide;
    cbZCmd[cbM2Counter] = ptr->m2Eng.zcmd;
    cbAzCmd[cbM2Counter] = ptr->m2Eng.azcmd;
    cbElCmd[cbM2Counter] = ptr->m2Eng.elcmd;
    cbZUnused[cbM2Counter] = ptr->m2Eng.zunused;
    cbAzTotCmd[cbM2Counter] = ptr->m2Eng.aztotcmd;
    cbElTotCmd[cbM2Counter] = ptr->m2Eng.eltotcmd;
    cbZTotCmd[cbM2Counter] = ptr->m2Eng.ztotcmd;
    cbRate1[cbM2Counter] = ptr->m2Eng.azrate;
    cbRate2[cbM2Counter] = ptr->m2Eng.elrate;
    cbRate3[cbM2Counter] = ptr->m2Eng.zrate;
    cbErr1[cbM2Counter] = ptr->m2Eng.azerr;
    cbErr2[cbM2Counter] = ptr->m2Eng.elerr;
    cbErr3[cbM2Counter] = ptr->m2Eng.zerr;
    cbRf1[cbM2Counter] = ptr->m2Eng.azrf;
    cbRf2[cbM2Counter] = ptr->m2Eng.elrf;
    cbRf3[cbM2Counter] = ptr->m2Eng.zrf;
    cbCor1[cbM2Counter] = ptr->m2Eng.azcor;
    cbCor2[cbM2Counter] = ptr->m2Eng.elcor;
    cbCor3[cbM2Counter] = ptr->m2Eng.zcor;
    cbF1[cbM2Counter] = ptr->m2Eng.azf;
    cbF2[cbM2Counter] =	ptr->m2Eng.elf;
    cbF3[cbM2Counter] =	ptr->m2Eng.zf;

    if (++cbM2Counter == CB_RECORD_NB )
    {
        cbM2Counter = 0;
    }
    return (OK);
}


/* Later: needs header
 *
 * Invocation: from VME prompt whenever want to save a snapshot
 * of CB_RECORD_NB datapoints to a file.
 *
 * Purpose: Dumps contents of ring buffers to a file.
 *
 */
int saveM2Cb ()
{
    char fileName[80];
    double fileTime;
    int i;
    FILE *pFile;

    
    if (timeNow(&fileTime) != OK)
	errorLog ("saveM2Cb - error reading timeStamp\n", 1, ON);

    sprintf(fileName, "./m2-chop-guide-%d.log", (int)fileTime);
    pFile = fopen ( fileName, "w" );

    if ( pFile == (FILE *) NULL )
    {
        printf ( "error opening file %s\n", fileName );
        return (-1);
    }

    /* write out from oldest data to end of array */
    for ( i = cbM2Counter ; i < CB_RECORD_NB ; i ++ )
    {
        fprintf ( pFile, "  3 %f %ld %d\n", cbTime[i], cbTick[i], i);
        fprintf ( pFile, "  4 %+4.2f %+4.2f %+4.2f\n", 
            cbXTilt[i], cbYTilt[i], cbZfocus[i]);
        fprintf ( pFile, "  5 %+4.2f %+4.2f %+4.2f\n", 
            cbAct1[i], cbAct2[i], cbAct3[i]);
        fprintf ( pFile, "  6 %+4.2f %+4.2f %+4.2f\n", 
            cbFol1[i], cbFol2[i], cbFol3[i]);
        fprintf ( pFile, "  7 %+4.2f %+4.2f %+4.2f\n", 
            cbCur3[i], cbCur2[i], cbCur3[i]);
        fprintf ( pFile, "  8 %+4.2f %+4.2f %+4.2f\n", 
            cbKam1[i], cbKam2[i], cbKam3[i]);
        fprintf ( pFile, "  9 %+4.2f %+4.2f %+4.2f\n", 
            cbInteg1[i], cbInteg2[i], cbInteg3[i]);
        fprintf ( pFile, " 10 %+4.2f %+4.2f %+4.2f\n", 
            cbAzGuide[i], cbElGuide[i], cbZCmd[i]);
        fprintf ( pFile, " 11 %+4.2f %+4.2f %+4.2f\n", 
            cbAzCmd[i], cbElCmd[i], cbZUnused[i]);
        fprintf ( pFile, " 12 %+4.2f %+4.2f %+4.2f\n", 
            cbAzTotCmd[i], cbElTotCmd[i], cbZTotCmd[i]);
        fprintf ( pFile, " 13 %+4.2f %+4.2f %+4.2f\n", 
            cbRate1[i], cbRate2[i], cbRate3[i]);
        fprintf ( pFile, " 14 %+4.2f %+4.2f %+4.2f\n", 
            cbErr1[i], cbErr2[i], cbErr3[i]);
        fprintf ( pFile, " 15 %+4.2f %+4.2f %+4.2f\n", 
            cbRf1[i], cbRf2[i], cbRf3[i]);
        fprintf ( pFile, " 16 %+4.2f %+4.2f %+4.2f\n", 
            cbCor1[i], cbCor2[i], cbCor3[i]);
        fprintf ( pFile, " 17 %+4.2f %+4.2f %+4.2f\n", 
            cbF1[i], cbF2[i], cbF3[i]);

    }

    /* write from beginning of array to newest data */
    for ( i = 0 ; i < cbM2Counter ; i ++ )
    {
        fprintf ( pFile, "  3 %f %ld %d\n", cbTime[i], cbTick[i], i);
        fprintf ( pFile, "  4 %+4.2f %+4.2f %+4.2f\n", 
            cbXTilt[i], cbYTilt[i], cbZfocus[i]);
        fprintf ( pFile, "  5 %+4.2f %+4.2f %+4.2f\n", 
            cbAct1[i], cbAct2[i], cbAct3[i]);
        fprintf ( pFile, "  6 %+4.2f %+4.2f %+4.2f\n", 
            cbFol1[i], cbFol2[i], cbFol3[i]);
        fprintf ( pFile, "  7 %+4.2f %+4.2f %+4.2f\n", 
            cbCur3[i], cbCur2[i], cbCur3[i]);
        fprintf ( pFile, "  8 %+4.2f %+4.2f %+4.2f\n", 
            cbKam1[i], cbKam2[i], cbKam3[i]);
        fprintf ( pFile, "  9 %+4.2f %+4.2f %+4.2f\n", 
            cbInteg1[i], cbInteg2[i], cbInteg3[i]);
        fprintf ( pFile, " 10 %+4.2f %+4.2f %+4.2f\n", 
            cbAzGuide[i], cbElGuide[i], cbZCmd[i]);
        fprintf ( pFile, " 11 %+4.2f %+4.2f %+4.2f\n", 
            cbAzCmd[i], cbElCmd[i], cbZUnused[i]);
        fprintf ( pFile, " 12 %+4.2f %+4.2f %+4.2f\n", 
            cbAzTotCmd[i], cbElTotCmd[i], cbZTotCmd[i]);
        fprintf ( pFile, " 13 %+4.2f %+4.2f %+4.2f\n", 
            cbRate1[i], cbRate2[i], cbRate3[i]);
        fprintf ( pFile, " 14 %+4.2f %+4.2f %+4.2f\n", 
            cbErr1[i], cbErr2[i], cbErr3[i]);
        fprintf ( pFile, " 15 %+4.2f %+4.2f %+4.2f\n", 
            cbRf1[i], cbRf2[i], cbRf3[i]);
        fprintf ( pFile, " 16 %+4.2f %+4.2f %+4.2f\n", 
            cbCor1[i], cbCor2[i], cbCor3[i]);
        fprintf ( pFile, " 17 %+4.2f %+4.2f %+4.2f\n", 
            cbF1[i], cbF2[i], cbF3[i]);
    }

    fclose (pFile);
    return (0);
}

long m2LoggerTask (memMap *ptr)
{
    double timeStamp;
    static char m2Data[M2_DATA_SIZE];
    static int ticker = 0;

    /* exit if logging not selected */

    if(m2LogActive != TRUE)
      return(OK);

    /* return if not time to log */

    if(ticker++ < logInterval)
    {
    return(OK);
    }
    ticker = 0;

    /* fetch timestamp from bancomm card */

    if (timeNow (&timeStamp) != OK)
    {
      logMsg ("m2loggerTask - error reading timeStamp\n", 0, 0, 0, 0, 0, 0);
    }

    /* fetch parameters from reflective memory, compile into string */


    sprintf (m2Data, "%16.6f", timeStamp);
    elgs (3, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* tip, tilt, focus positions */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->page1.xTilt, ptr->page1.yTilt, ptr->page1.zFocus);

    elgs (4, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* actuator positons */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->page1.actuator1, ptr->page1.actuator2, ptr->page1.actuator3);

    elgs (5, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* follower errors */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.follow1, ptr->m2Eng.follow2, ptr->m2Eng.follow3);

    elgs (6, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* actuator currents */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.current1, ptr->m2Eng.current2, ptr->m2Eng.current3);

    elgs (7, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* kaman sensors */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.kaman1, ptr->m2Eng.kaman2, ptr->m2Eng.kaman3);

    elgs (8, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* integrators */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.integ1, ptr->m2Eng.integ2, ptr->m2Eng.integ3);

    elgs (9, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* xguide, yguide and z(guide+cmd) */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azguide, ptr->m2Eng.elguide, ptr->m2Eng.zcmd);

    elgs (10, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* xcmd, ycmd and zunused */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azcmd, ptr->m2Eng.elcmd, ptr->m2Eng.zunused);

    elgs (11, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* x(guide+cmd), y(guide+cmd), z(guide+cmd) */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.aztotcmd, ptr->m2Eng.eltotcmd, ptr->m2Eng.ztotcmd);

    elgs (12, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* rate (error derivative) */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azrate, ptr->m2Eng.elrate, ptr->m2Eng.zrate);

    elgs (13, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* position error (demand - actual) */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azerr, ptr->m2Eng.elerr, ptr->m2Eng.zerr);

    elgs (14, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* raw force (resulting from PID, before DECS, before notches */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azrf, ptr->m2Eng.elrf, ptr->m2Eng.zrf);

    elgs (15, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* DECS correction */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azcor, ptr->m2Eng.elcor, ptr->m2Eng.zcor);

    elgs (16, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* force (rawforce + DECS + notches */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azf, ptr->m2Eng.elf, ptr->m2Eng.zf);

    elgs (17, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* PG - proportional gain constant */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azp, ptr->m2Eng.elp, ptr->m2Eng.zp);

    elgs (18, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* IG - integrator gain constant */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azi, ptr->m2Eng.eli, ptr->m2Eng.zi);

    elgs (19, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    /* DG - differential gain constant */

    sprintf (m2Data, "%+4.2f %+4.2f %+4.2f",
             ptr->m2Eng.azd, ptr->m2Eng.eld, ptr->m2Eng.zd);

    elgs (20, m2Data);    
    strncpy (m2Data, "blank", M2_DATA_SIZE);

    return(OK);
}

