/* ===================================================================== */
/*+
 *
 * FILENAME
 * -------- 
 * config.c
 * 
 * PURPOSE
 * -------
 * Collection of CAD related routines to accept configuration data.
 * I don't think CADmovebaffle belongs here - not config stuff. Someday
 * move it to scs.c
 * 
 * FUNCTION NAME(S)
 * ----------------
 * CADmovebaffle    - accept demand positions for baffles
 * CADservoBandwidth- accept bandwidth request for tilt system
 * CADcontroller    - read PID settings from engineering screens
 * CADdecsAdjust    - read DECS configuration parameters from engineering screens
 * CADtransforms    - read coordinate transform data from engineering screens
 * CADtolerance     - accept 'in position' window size
 * CADdebug         - read desired debug level
 * CADdriveFollower - drive an M2 follower a given no. steps in a given dir 
 * CADdriveOffloader- drive an M2 offloader a given no. steps in a given dir
 * CADdriveDB       - drive an M2 dep baffle a given no. steps in a given dir
 * CADdriveCB       - drive M2 central baffle a given no. steps in a given dir
 * CADdriveXY       - drive M2 XY positioner stepper motor a given no. steps 
 *                    in a given dir
 * CADtiltPidControl-
 * CADfocusPidControl-
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
 * 04-Dec-1997: Remove function ccmove - see moveCurve inscs.c
 * 02-Feb-1998: convert logMsg to errorLog for error reporting
 * 23-Feb-1998: decsAdjust should call cadDirLog with 9 pars, not 12
 * 23-Jun-1998: Put lower limit on tolerance windows
 * 07-May-1999: Added RCS id
 * 18-Jan-2000: Added CADdriveFollower 
 * 01-Feb-2000: Added CADdrive* routines for offloaders, cbaf, dbafs and xy
 * 06-Mar-2000: Added CAD*PidControl to allow toggling of PID control
 * 26-Oct-2017: Begin conversion to EPICS OSI (mdw)
 *
 */
/* ===================================================================== */

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <cad.h>
#include <tcslib.h>

#include "config.h"
#include "utilities.h"      /* For setPid, errorLog, debugLevel */
#include "archive.h"        /* For cadDirLog, refMemFree */
#include "control.h"        /* For writeCommand, scsPtr, interlockFlag, 
                                       guideType */

/* Define limits for PID parameters */

#define LOW_P              0
#define HIGH_P           100
#define LOW_I              0
#define HIGH_I         10000
#define LOW_D              0
#define HIGH_D         10000
#define LOW_ACCEL          0
#define HIGH_ACCEL      1000
#define LOW_WINDUP         0
#define HIGH_WINDUP    10000

/* Define decs adjustment limits */

#define DECS_MAX_GAIN      1e8
#define DECS_MAX_SHIFT   100
#define DECS_MAX_SMOOTH    1

/* Define limits of tolerance parameters */

#define TILT_TOL_MAX     100.0
#define TILT_TOL_MIN       0.1
#define FOCUS_TOL_MAX    100.0
#define FOCUS_TOL_MIN      0.1
#define POSITION_TOL_MAX 100.0
#define POSITION_TOL_MIN   0.1
#define XYPOS_DEADBAND_MAX   1000.0
#define XYPOS_DEADBAND_MIN   0.0

#define FIRST_FOLLOWER     1    /* for range checking the requested fol/ofl */
#define LAST_FOLLOWER      3
#define FIRST_OFFLOADER    1
#define LAST_OFFLOADER     3
#define FIRST_DBAF         0
#define LAST_DBAF          2
#define UPPER_MOTOR        0
#define LOWER_MOTOR        1


#define OUT                0     /* defining follower and offloader direction */
#define IN                 1 

#define UP                 1
#define DOWN               0

#define CW                 1
#define CCW                0

#define OPEN               1
#define CLOSE              0

#define DSP_STEPS_DEFAULT  300   /* for driving followers and offloaders */
#define STEPS_DEFAULT      600   /* for driving dbaf, cbaf and xy */

/* Define globals */
static double blend = 0.0;

#ifdef OLDWAY
char *cenBaffle[] = 
{
    "POSITION 1",          /* CLOSED */
    "POSITION 2",          /* OPEN   */
     NULL
}; 
#endif

char *periscopeOption[] = 
{
    "CLOSED",
    "OPEN",
     NULL
};


char *cenBaffle[] = 
{
    "CLOSED",
    "OPEN",
     NULL
};

#ifdef OLDWAY
char *depBaffle[] = 
{
    "RETRACTED",           /* FAR IR  */ 
    "POSITION 1",          /* NEAR IR */
    "POSITION 2",          /* VISIBLE */
    "EXTENDED",            /* MAINTENANCE */
     NULL
};
#endif

char *depBaffle[] = 
{
    "RETRACTED",           /* FAR IR  */ 
    "NEAR IR",  
    "VISIBLE", 
    "EXTENDED",            /* MAINTENANCE */
     NULL
};

/* ===================================================================== */
/*
 * Function name:
 * CADmovebaffle
 *
 * Purpose:
 * The demanded positions for deployable and central baffles are read
 * from the CAD inputs and converted to long format. If the current
 * state of the scs is moving or chopping then the forward start link
 * is triggered otherwise the command is rejected
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADmovebaffle(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string demanded deployable baffle position
 *  > pcad->b   *string demanded central baffle position
 *
 *  < pcad->mess    *string status message
 *  < pcad->vala    long demanded deployable baffle position
 *  < pcad->valb    long demanded central baffle position
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 *  > scsState  current state of the SCS
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Sean Prior (srp@roe.ac.uk)
 *
 * History:
 * 24-Jul-1996: Original(srp)
 * 25-Dec-1998: Changed strings in cenBaffle[] and depBaffle[]
 * 08-Feb-2000: Move cenBaffle and depBaffle to be globals (so they can 
 *              be used by scs_st.stpp
 * 08-Feb-2000: No more SNL to move baffles. Command M2 from the CAD.
 *
 *
 */
/* ===================================================================== */

long    CADmoveBaffle (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    static  int deployable, central;

    cadDirLog ("moveBaffle", pcad->dir, 2, pcad);

    /* Set message prefix */
    tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char *) NULL);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        status = CAD_REJECT ;

        if (tcsDcString (depBaffle, "deployable - ", pcad->a,
            &deployable, pcad))
            break;

        if (deployable < 0 || deployable > 3)
        {
            tcsCsAppendMessage (pcad, "deploy out of range") ;
            break;
        }

        if (tcsDcString (cenBaffle, "central - ", pcad->b,
            &central, pcad))
            break;

        if (central < 0 || central > 1)
        {
            tcsCsAppendMessage (pcad, "central out of range") ;
            break;
        }

        status = CAD_ACCEPT ;

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* DOES TCS NEED? The SCS doesn't.
               These values are only set so they can be
               monitored and accessed in the SNL
            *(long *) pcad->vala = (long)deployable;
            *(long *) pcad->valb = (long)central; */

            /* brought from SNL 08-FEB-2000 */
            epicsMutexLock(refMemFree);
            scsPtr->page0.deployBaffle  = (long)deployable;
            scsPtr->page0.centralBaffle = (long)central;
            epicsMutexUnlock(refMemFree);

            writeCommand(BAFFLE_CHANGE);

            errlogPrintf( "sent command %2d to drive deployable to pos: %1d and periscope to pos: %1d\n",
                               BAFFLE_CHANGE, deployable, central);

            /* TO DO: use a wait record to time up to TIMEOUT if not
               done via delay in seq record

               Condition for OK completion:
               (deployableDemand == scsPtr->page1.deployBaffle) &&
               (centralDemand == scsPtr->page1.centralBaffle) AND
               time < TIMEOUT

               Condition for ERROR completion:
               (deployableDemand != scsPtr->page1.deployBaffle) &&
               (centralDemand != scsPtr->page1.centralBaffle) AND
               time >= TIMEOUT
            */

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
 * CADservoBandwidth
 *
 * Purpose:
 * The requested bandwidth is checked to verify that it is one of
 * the available options.  If so the value is copied to the output
 * else the command is rejected
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADservoBandwidth(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string tilt system bandwidth
 *
 *  < pcd->mess *string status message
 *  < pcad->vala    double tilt system bandwidth
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Sean Prior (srp@roe.ac.uk)
 *
 * History:
 * 24-Jul-1996: Original(srp)
 *
 */
/* ===================================================================== */

long    CADservoBandwidth (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    static  double bandwidth;
    char    dumpString[MAX_STRING_SIZE];

    cadDirLog ("servoBandwidth", pcad->dir, 1, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* convert the input string to a number for checking */

        if(sscanf (pcad->a, "%lf%s", &bandwidth, dumpString) != 1)
        {
            strncpy (pcad->mess, "Failed BW conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(bandwidth != 10 && bandwidth != 15 && bandwidth != 20 &&bandwidth != 25)
        {
            strncpy (pcad->mess, "Must be 10, 15, 20, 25 Hz", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            *(double *) pcad->vala = bandwidth;
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
 * CADcontroller
 *
 * Purpose:
 * Read the controller setting values from the inputs. Perform sanity checks
 * and write to the control loop data structures
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADcontroller(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string P term tilt
 *  > pcad->b   *string I term tilt
 *  > pcad->c   *string D term tilt
 *  > pcad->d   *string windup limit tilt
 *  > pcad->e   *string accel limit tilt
 *  > pcad->f   *string P term focus
 *  > pcad->g   *string I term focus
 *  > pcad->h   *string D term focus
 *  > pcad->i   *string windup limit focus
 *  > pcad->j   *string accel limit focus
 *  > pcad->k   *string guideType value 0=autoguide, !0 = projected
 *  > pcad->l   *string bleed value for high pass filtered guide signals
 *  < pcad->mess    *string error message
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Sean Prior (srp@roe.ac.uk)
 *
 * History:
 * 11-Jul-1996: Original(srp)
 * 15-Jul-1998: Bug fix, tilt windup limit incorrect
 *
 */
/* ===================================================================== */

long    dummyInit (struct cadRecord * pcad)
{
        return (OK);
}

long    CADcontroller (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static double pid[17];
    int     tmp;

    cadDirLog ("controller", pcad->dir, 17, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* convert the input strings to numbers for checking */

        if ((sscanf (pcad->a, "%lf%s", &pid[0], dumpString)) != 1)
        {
            strncpy(pcad->mess, "P X tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->b, "%lf%s", &pid[1], dumpString)) != 1)
        {
            strncpy(pcad->mess, "I X tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->c, "%lf%s", &pid[2], dumpString)) != 1)
        {
            strncpy(pcad->mess, "D X tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->d, "%lf%s", &pid[3], dumpString)) != 1)
        {
            strncpy(pcad->mess, "windup X tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->e, "%lf%s", &pid[4], dumpString)) != 1)
        {
            strncpy(pcad->mess, "rate X tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->f, "%lf%s", &pid[5], dumpString)) != 1)
        {
            strncpy(pcad->mess, "P focus failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->g, "%lf%s", &pid[6], dumpString)) != 1)
        {
            strncpy(pcad->mess, "I focus failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->h, "%lf%s", &pid[7], dumpString)) != 1)
        {
            strncpy (pcad->mess, "D focus failed conversion", 
                     MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->i, "%lf%s", &pid[8], dumpString)) != 1)
        {
            strncpy(pcad->mess, "windup focus failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->j, "%lf%s", &pid[9], dumpString)) != 1)
        {
            strncpy(pcad->mess, "rate focus failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->k, "%lf%s", &pid[10], dumpString)) != 1)
        {
            strncpy(pcad->mess, "guide type failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->l, "%lf%s", &pid[11], dumpString)) != 1)
        {
            strncpy(pcad->mess, "bleed  failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->m, "%lf%s", &pid[12], dumpString)) != 1)
        {
            strncpy(pcad->mess, "P Y tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->n, "%lf%s", &pid[13], dumpString)) != 1)
        {
            strncpy(pcad->mess, "I Y tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->o, "%lf%s", &pid[14], dumpString)) != 1)
        {
            strncpy(pcad->mess, "D Y tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->p, "%lf%s", &pid[15], dumpString)) != 1)
        {
            strncpy(pcad->mess, "windup Y tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->q, "%lf%s", &pid[16], dumpString)) != 1)
        {
            strncpy(pcad->mess, "rate Y tilt failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        /* check parameters lie within limits */

        if(pid[0] < LOW_P || pid[0] > HIGH_P)
        {
            strncpy(pcad->mess, "P X tilt gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[1] < LOW_I || pid[1] > HIGH_I)
        {
            strncpy(pcad->mess, "I X tilt gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[2] < LOW_D || pid[2] > HIGH_D)
        {
            strncpy(pcad->mess, "D X tilt gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[3] < LOW_WINDUP || pid[3] > HIGH_WINDUP)
        {
            strncpy(pcad->mess, "X tilt windup out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[4] < LOW_ACCEL || pid[4] > HIGH_ACCEL)
        {
            strncpy(pcad->mess, "X tilt accel limit out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[5] < LOW_P || pid[5] > HIGH_P)
        {
            strncpy(pcad->mess, "P focus gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[6] < LOW_I || pid[6] > HIGH_I)
        {
            strncpy(pcad->mess, "I focus gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[7] < LOW_D || pid[7] > HIGH_D)
        {
            strncpy(pcad->mess, "D focus gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[8] < LOW_WINDUP || pid[8] > HIGH_WINDUP)
        {
            strncpy(pcad->mess, "focus windup out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[9] < LOW_ACCEL || pid[9] > HIGH_ACCEL)
        {
            strncpy(pcad->mess, "focus accel limit out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[10] < 0 || pid[10] > 1)
        {
            strncpy(pcad->mess, "guide choice out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[11] < 0 || pid[11] > 1)
        {
            strncpy(pcad->mess, "bleed out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[12] < LOW_P || pid[12] > HIGH_P)
        {
            strncpy(pcad->mess, "P Y tilt gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[13] < LOW_I || pid[13] > HIGH_I)
        {
            strncpy(pcad->mess, "I Y tilt gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[14] < LOW_D || pid[14] > HIGH_D)
        {
            strncpy(pcad->mess, "D Y tilt gain out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[15] < LOW_WINDUP || pid[15] > HIGH_WINDUP)
        {
            strncpy(pcad->mess, "Y tilt windup out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(pid[16] < LOW_ACCEL || pid[16] > HIGH_ACCEL)
        {
            strncpy(pcad->mess, "Y tilt accel limit out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* write the coefficients to the controllers */

            if (setPid (XTILT, pid[0], pid[1], pid[2], pid[3], pid[4])!=OK)
	    {
                strncpy (pcad->mess, "error setting Pid for X", 
                         MAX_STRING_SIZE - 1);
		status = CAD_REJECT;
            }
            if (setPid (YTILT, pid[12], pid[13], pid[14], pid[15], pid[16])!=OK)
	    {
                strncpy (pcad->mess, "error setting Pid for Y", 
                         MAX_STRING_SIZE - 1);
		status = CAD_REJECT;
            }

            if (setPid (ZFOCUS, pid[5], pid[6], pid[7], pid[8], pid[9])!=OK)
	    {
                strncpy (pcad->mess, "error setting Pid for Z", 
                         MAX_STRING_SIZE - 1);
		status = CAD_REJECT;
            }

            blend = pid[10];
	    tmp = (int) blend;

                        /* blend is a double -- so test with tmp */
            if(tmp == 0)
                guideType = AUTOGUIDE;
            else
                guideType = PROJECT;

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
        strncpy (pcad->mess, "inappropriate CAD directive", 
                 MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

#ifdef MK
long    CADsetPhasorControl (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static double phasorInput[17];
    Phasor *phasor;
    int axis;

    cadDirLog ("Phasorcontroller", pcad->dir, 17, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /*
         *  A  phasor->freq;     Frequency            phasorInput[0]
         *  B  phasor->amp;      Amplitude            phasorInput[1] 
         *  C  phasor->Fs;       SampleRate           phasorInput[2] 
         *
         *  O  AXIS  (X=0 or Y=1)                     phasorInput[14]
         * */
        
        /* convert the input strings to numbers for checking */
        if ((sscanf (pcad->a, "%lf%s", &phasorInput[0], dumpString)) != 1)
        {
            strncpy(pcad->mess, "Phasor input frequency failed conversion", 
                    MAX_STRING_SIZE - 1);

            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->b, "%lf%s", &phasorInput[1], dumpString)) != 1)
        {
            strncpy(pcad->mess, "Phasor input amplitude failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if ((sscanf (pcad->c, "%lf%s", &phasorInput[2], dumpString)) != 1)
        {
            strncpy(pcad->mess, "Phasor input sampleRate failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        /* ----AXIS---is on INPO phasorInput[14]*/
        else if ((sscanf (pcad->o, "%lf%s", &phasorInput[14], dumpString)) != 1)
        {
            strncpy(pcad->mess, "Phasor AXIS failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }


        /* check parameters lie within limits */

        if(phasorInput[0] < PHASOR_FREQUENCY_LOWLIMIT || phasorInput[0] > PHASOR_FREQUENCY_HIGHLIMIT)
        {
            strncpy(pcad->mess, "Phasor frequency out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(phasorInput[1] < PHASOR_AMPLITUDE_LOWLIMIT || phasorInput[1] > PHASOR_AMPLITUDE_HIGHLIMIT)
        {
            strncpy(pcad->mess, "Phasor amplitude out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if(phasorInput[2] < PHASOR_SR_LOWLIMIT || phasorInput[2] > PHASOR_SR_HIGHLIMIT)
        {
            strncpy(pcad->mess, "Phasor sample rate out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        printf("CADsetPhasorControl:PRESETEND \n");
        break;

    case menuDirectiveSTART:

        printf("CADsetPhasorControl:STARTStart \n");
        if (interlockFlag != 1)
        {

            axis = (int) phasorInput[14];
            printf("CADsetPhasorControl: axis = %d\n", axis);

            if (axis == XTILT) {
                phasor = getPhasorX();
            }
            else if (axis == YTILT) {
                phasor = getPhasorY(); 
            }
            else {
                strncpy(pcad->mess, "Phasor AXIS INVALID", 
                        MAX_STRING_SIZE - 1);
                status = CAD_REJECT;
                break;
            }

            if (phasor == NULL) {

                strncpy(pcad->mess, "Phasor  structure is NULL", 
                        MAX_STRING_SIZE - 1);
                printf("CADsetPhasorControl: Phasor Structure is NULL\n");
                status = CAD_REJECT;
                break;
            }

            phasor->freq = phasorInput[0];
            phasor->amp = phasorInput[1];
            phasor->Fs = phasorInput[2];

            /*Reinitialize Phasor*/
            phasorInit(phasor);

        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        printf("CADsetPhasorControl:STARTend \n");

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", 
                 MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    printf("CADsetPhasorControl:Endstatus=%ld \n", status);
    return(status);

} 

#endif

#ifdef MK
long    CADsetVTKControl (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static double vtkInput[17];
    Vtk *vtk;
    int axis;

    cadDirLog ("VTKcontroller", pcad->dir, 17, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /*
         *  A  vtk->Fs; Sample Rate of in coming WFS    vtkInput[0]
         *  B  vtk->gain.phase;                         vtkInput[1] 
         *  C  vtk->gain.frequency;                     vtkInput[2] 
         *  D  vtk->frequency.initialValue;             vtkInput[3]
         *  E  vtk->frequency.tolerance;                vtkInput[4]
         *  F  vtk->scale;                              vtkInput[5]
         *  G  vtk->angle;                              vtkInput[6]
         *
         *
         *  O  AXIS  (X=0 or Y=1)                       vtkInput[14]
         * */

        
        /* convert the input strings to numbers for checking */
        if ((sscanf (pcad->a, "%lf%s", &vtkInput[0], dumpString)) != 1)
        {
            strncpy(pcad->mess, "VTK tilt SampleRate failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->b, "%lf%s", &vtkInput[1], dumpString)) != 1)
        {
            strncpy(pcad->mess, "VTK tilt Gain.phase failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if ((sscanf (pcad->c, "%lf%s", &vtkInput[2], dumpString)) != 1)
        {
            strncpy(pcad->mess, "VTK tilt Gain.frequency failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if ((sscanf (pcad->d, "%lf%s", &vtkInput[3], dumpString)) != 1)
        {
            strncpy(pcad->mess, "VTK tilt Frequency Initial value failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->e, "%lf%s", &vtkInput[4], dumpString)) != 1)
        {
            strncpy(pcad->mess, "VTK tilt Frequency tolerance failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->f, "%lf%s", &vtkInput[5], dumpString)) != 1)
        {
            strncpy(pcad->mess, "VTK tilt scale failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if ((sscanf (pcad->g, "%lf%s", &vtkInput[6], dumpString)) != 1)
        {
            strncpy(pcad->mess, "VTK tilt angle failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        /* ----AXIS---is on INPO vtkInput[14]*/
        else if ((sscanf (pcad->o, "%lf%s", &vtkInput[14], dumpString)) != 1)
        {
            strncpy(pcad->mess, "VTK AXIS failed conversion", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }


        /* check parameters lie within limits */

        if(vtkInput[0] < VTK_SR_LOW || vtkInput[0] > VTK_SR_HIGH)
        {
            strncpy(pcad->mess, "VTK tilt sample rate out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(vtkInput[1] < VTK_GAINPHASE_LOW || vtkInput[1] > VTK_GAINPHASE_HIGH)
        {
            strncpy(pcad->mess, "VTK tilt gain phase out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if(vtkInput[2] < VTK_GAINFREQ_LOW || vtkInput[2] > VTK_GAINFREQ_HIGH)
        {
            strncpy(pcad->mess, "VTK tilt gain frequency out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if(vtkInput[3] < VTK_FREQUENCY_LOW || vtkInput[3] > VTK_FREQUENCY_HIGH)
        {
            strncpy(pcad->mess, "VTK tilt Initial Frequency out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if(vtkInput[4] < VTK_FREQ_TOLERANCE_LOW || vtkInput[4] > VTK_FREQ_TOLERANCE_HIGH)
        {
            strncpy(pcad->mess, "VTK tilt Frequency Tolerance  out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if(vtkInput[5] < VTK_SCALE_LOW || vtkInput[5] > VTK_SCALE_HIGH)
        {
            strncpy(pcad->mess, "VTK tilt Scale out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if(vtkInput[6] < VTK_ANGLE_LOW || vtkInput[6] > VTK_ANGLE_HIGH)
        {
            strncpy(pcad->mess, "VTK tilt Angle out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        else if(vtkInput[14] < XTILT || vtkInput[14] > YTILT)
        {
            strncpy(pcad->mess, "VTK AXIS out of range", 
                    MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        printf("CADsetVTKControl:PRESETEND \n");
        break;

    case menuDirectiveSTART:

        printf("CADsetVTKControl:STARTStart \n");
        if (interlockFlag != 1)
        {

            axis = (int) vtkInput[14];
            printf("CADsetVTKControl: axis = %d\n", axis);

            if (axis == XTILT) {
                vtk = getVtkX();
            }
            else if (axis == YTILT) {
                vtk = getVtkY(); 
            }
            else {
                strncpy(pcad->mess, "VTK AXIS INVALID", 
                        MAX_STRING_SIZE - 1);
                status = CAD_REJECT;
                break;
            }

            if (vtk == NULL) {

                strncpy(pcad->mess, "VTK  structure is NULL", 
                        MAX_STRING_SIZE - 1);
                printf("CADsetVTKControl: VTK Structure is NULL\n");
                status = CAD_REJECT;
                break;
            }

            vtk->Fs = vtkInput[0];
            vtk->gain.phase = vtkInput[1];
            vtk->gain.frequency = vtkInput[2];
            vtk->frequency.initialValue = vtkInput[3];
            vtk->frequency.tolerance = vtkInput[4];
            vtk->scale = vtkInput[5];
            vtk->angle = vtkInput[6];

            /*Reinitialize VTK*/
            vtkInit(vtk);

            /*Copy valid data to confirmed Outputs*/
            *(double *) pcad->vala = vtk->Fs;
            *(double *) pcad->valb = vtk->gain.phase;
            *(double *) pcad->valc = vtk->gain.frequency;
            *(double *) pcad->vald = vtk->frequency.initialValue;
            *(double *) pcad->vale = vtk->frequency.tolerance;
            *(double *) pcad->valf = vtk->scale;
            *(double *) pcad->valg = vtk->angle;

        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        printf("CADsetVTKControl:STARTend \n");

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", 
                 MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    printf("CADsetVTKControl:Endstatus=%ld \n", status);
    return(status);

} 
#endif

/* ===================================================================== */
/*
 * Function name:
 * CADdecsAdjust
 *
 * Purpose:
 * Read the adjustment parameters from the inputs. Perform sanity checks
 * and make available at the output with type conversion
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADdecsAdjust(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string xTiltGain
 *  > pcad->b   *string yTiltGain
 *  > pcad->c   *string zGain
 *  > pcad->d   *string xTiltBinShift
 *  > pcad->e   *string yTiltBinShift
 *  > pcad->f   *string zBinShift
 *  > pcad->g   *string xTiltSmoothingGain
 *  > pcad->h   *string yTiltSmmothingGain
 *  > pcad->i   *string zSmootingGain
 *  < pcad->mess    *string status message
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Sean Prior (srp@roe.ac.uk)
 *
 * History:
 * 06-Dec-1996: Original(srp)
 *
 */
/* ===================================================================== */

long    CADdecsAdjust (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];

    static  struct
        {
            double  xGain;
            double  yGain;
            double  zGain;
            double  xShift;
            double  yShift;
            double  zShift;
            double  xSmooth;
            double  ySmooth;
            double  zSmooth;
        } decsValue;

    cadDirLog ("decsConfig", pcad->dir, 9, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* read input ports */

        if (sscanf (pcad->a, "%lf%s", &decsValue.xGain, dumpString) != 1)
        {
            strncpy (pcad->mess, "x gain failed conversion", MAX_STRING_SIZE - 1);          
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.xGain) > DECS_MAX_GAIN)
        {
            strncpy (pcad->mess, "x gain out of range", MAX_STRING_SIZE - 1);           
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->b, "%lf%s", &decsValue.yGain, dumpString) != 1)
        {
            strncpy (pcad->mess, "y gain failed conversion", MAX_STRING_SIZE - 1);          
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.yGain) > DECS_MAX_GAIN)
        {
            strncpy (pcad->mess, "y gain out of range", MAX_STRING_SIZE - 1);           
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->c, "%lf%s", &decsValue.zGain, dumpString) != 1)
        {
            strncpy (pcad->mess, "z gain failed conversion", MAX_STRING_SIZE - 1);          
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.zGain) > DECS_MAX_GAIN)
        {
            strncpy (pcad->mess, "z gain out of range", MAX_STRING_SIZE - 1);           
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->d, "%lf%s", &decsValue.xShift, dumpString) != 1)
        {
            strncpy (pcad->mess, "x shift failed conversion", MAX_STRING_SIZE - 1);         
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.xShift) > DECS_MAX_SHIFT)
        {
            strncpy (pcad->mess, "x shift out of range", MAX_STRING_SIZE - 1);          
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->e, "%lf%s", &decsValue.yShift, dumpString) != 1)
        {
            strncpy (pcad->mess, "y shift failed conversion", MAX_STRING_SIZE - 1);         
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.yShift) > DECS_MAX_SHIFT)
        {
            strncpy (pcad->mess, "y shift out of range", MAX_STRING_SIZE - 1);          
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->f, "%lf%s", &decsValue.zShift, dumpString) != 1)
        {
            strncpy (pcad->mess, "z shift failed conversion", MAX_STRING_SIZE - 1);         
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.zShift) > DECS_MAX_SHIFT)
        {
            strncpy (pcad->mess, "z shift out of range", MAX_STRING_SIZE - 1);          
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->g, "%lf%s", &decsValue.xSmooth, dumpString) != 1)
        {
            strncpy (pcad->mess, "x smooth failed conversion", MAX_STRING_SIZE - 1);            
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.xSmooth) > DECS_MAX_SMOOTH)
        {
            strncpy (pcad->mess, "x smooth out of range", MAX_STRING_SIZE - 1);         
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->h, "%lf%s", &decsValue.ySmooth, dumpString) != 1)
        {
            strncpy (pcad->mess, "y smooth failed conversion", MAX_STRING_SIZE - 1);            
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.ySmooth) > DECS_MAX_SMOOTH)
        {
            strncpy (pcad->mess, "y smooth out of range", MAX_STRING_SIZE - 1);         
            status = CAD_REJECT;
            break;
        }

        if (sscanf (pcad->i, "%lf%s", &decsValue.zSmooth, dumpString) != 1)
        {
            strncpy (pcad->mess, "z smooth failed conversion", MAX_STRING_SIZE - 1);            
            status = CAD_REJECT;
            break;
        }
        else if (fabs(decsValue.zSmooth) > DECS_MAX_SMOOTH)
        {
            strncpy (pcad->mess, "z smooth out of range", MAX_STRING_SIZE - 1);         
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag == ON)
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        /* write values to reflective memory */

        epicsMutexLock(refMemFree);
        scsPtr->page0.xTiltGain = (float)decsValue.xGain;
        scsPtr->page0.yTiltGain = (float)decsValue.yGain;
        scsPtr->page0.zFocusGain = (float)decsValue.zGain;
        scsPtr->page0.xTiltShift = (float)decsValue.xShift;
        scsPtr->page0.yTiltShift = (float)decsValue.yShift;
        scsPtr->page0.zFocusShift = (float)decsValue.zShift;
        scsPtr->page0.xTiltSmooth = (float)decsValue.xSmooth;
        scsPtr->page0.yTiltSmooth = (float)decsValue.ySmooth;
        scsPtr->page0.zFocusSmooth = (float)decsValue.zSmooth;
        epicsMutexUnlock(refMemFree);

        /* indicate to M2 that a change has occurred */

        writeCommand(DECS_CHANGE);

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
 * CADtolerance
 *
 * Purpose:
 * Receive and sanity check the in position tolerance values for each
 * of the five degrees of freedom . The tolerance values are received
 * and subjected to limit checks. If successful they are written to the 
 * outputs.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADtolerance(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string x tilt tolerance
 *  > pcad->b   *string y tilt tolerance
 *  > pcad->c   *string z focus tolerance
 *  > pcad->d   *string x position tolerance
 *  > pcad->e   *string y position tolerance
 *
 *  < pcd->mess *string status message
 *  < pcad->vala    double x tilt tolerance
 *  < pcad->valb    double y tilt tolerance
 *  < pcad->valc    double z focus tolerance
 *  < pcad->vald    double x position tolerance
 *  < pcad->vale    double y position tolerance
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Sean Prior (srp@roe.ac.uk)
 *
 * History:
 * 24-Jul-1996: Original(srp)
 * 18-May-1997: Write values to output on start rather than preset
 * 23-Jun-1998: Put lower limit on tolerance windows
 *
 */
/* ===================================================================== */

long    CADtolerance (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static  double xPosTol, yPosTol, xTiltTol, yTiltTol, zFocusTol, xyPosDeadband;

    cadDirLog ("tolerance", pcad->dir, 6, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* convert the input strings to numbers for checking */

        if (sscanf (pcad->a, "%lf%s", &xTiltTol, dumpString) != 1)
        {
            strncpy(pcad->mess, "x tilt fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(sscanf (pcad->b, "%lf%s", &yTiltTol, dumpString) != 1)
        {
            strncpy(pcad->mess, "y tilt fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(sscanf (pcad->c, "%lf%s", &zFocusTol, dumpString) != 1)
        {
            strncpy(pcad->mess, "focus fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(sscanf (pcad->d, "%lf%s", &xPosTol, dumpString) != 1)
        {
            strncpy(pcad->mess, "x pos fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(sscanf (pcad->e, "%lf%s", &yPosTol, dumpString) != 1)
        {
            strncpy(pcad->mess, "y pos fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if(sscanf (pcad->f, "%lf%s", &xyPosDeadband, dumpString) != 1)
        {
            strncpy(pcad->mess, "xy ps Deadband fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if (xTiltTol < TILT_TOL_MIN || (xTiltTol > TILT_TOL_MAX))
        {
            strncpy (pcad->mess, "x tilt tolerance out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if (yTiltTol < TILT_TOL_MIN || (yTiltTol > TILT_TOL_MAX))
        {
            strncpy (pcad->mess, "y tilt tolerance out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if (zFocusTol < FOCUS_TOL_MIN || (zFocusTol > FOCUS_TOL_MAX))
        {
            strncpy (pcad->mess, "z focus tolerance out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if (xPosTol < POSITION_TOL_MIN || (xPosTol > POSITION_TOL_MAX))
        {
            strncpy (pcad->mess, "x pos tolerance out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if (yPosTol < POSITION_TOL_MIN || (yPosTol > POSITION_TOL_MAX))
        {
            strncpy (pcad->mess, "y pos tolerance out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        else if (xyPosDeadband < XYPOS_DEADBAND_MIN || (xyPosDeadband > XYPOS_DEADBAND_MAX))
        {
            strncpy (pcad->mess, "xy pos deadband out of range", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            *(double *) pcad->vala = xTiltTol;
            *(double *) pcad->valb = yTiltTol;
            *(double *) pcad->valc = zFocusTol;
            *(double *) pcad->vald = xPosTol;
            *(double *) pcad->vale = yPosTol;
            *(double *) pcad->valf = xyPosDeadband;
            printf("CADtolerance: xyposDeadband = %f\n", xyPosDeadband);
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
 * CADdebug
 *
 * Purpose:
 * Allow setting of the debug level
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADdebug(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string debug level
 *  < pcad->mess    *string status message
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Sean Prior (srp@roe.ac.uk)
 *
 * History:
 * 24-Jun-1997: Original(srp)
 * 19-Jun-1998: Accept "None", "Min" or "Full" (cjm)
 * 02-Feb-2000: Add "Med", "Max", "Reserved 1" and "Reserved 2". Drop "Full".
 *
 */
/* ===================================================================== */

long    CADdebug (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    static int debugBuffer;
        static char *debugOpts[]= 
               {"NONE", "MIN", "MED", "MAX", "RESERVED1", "RESERVED2", NULL} ;

    cadDirLog ("debug", pcad->dir, 1, pcad);

    /* Fetch name of cad for messages */
    tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL) ;

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

                status = CAD_REJECT ;

                if (pcad->a[0])
        {
                  if (tcsDcString (debugOpts, "level - ", pcad->a, &debugBuffer, pcad) )
          {
              errorLog ("CADdebug - unrecognised debug selection", 2, ON);
              break ;
                  }
                }
        else
        {
            tcsCsAppendMessage(pcad, "no level specified");
            break ;
                }

                status = CAD_ACCEPT ;
        break;

    case menuDirectiveSTART:

        debugLevel = debugBuffer;
                strcpy(pcad->vala, debugOpts[debugBuffer]);
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
 * CADdriveFollower
 *
 * Purpose:
 * Receive and sanity check the follower,foldir and folsteps values.
 * If successful they are written to the outputs.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADdriveFollower(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string follower
 *  > pcad->b   *string foldir
 *  > pcad->c   *string followersteps
 *
 *  < pcd->mess     *string status message
 *  < pcad->vala    long follower
 *  < pcad->valb    long foldir
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Dayle Kotturi (dayle@gemini.edu)
 *
 * History:
 * 18-Jan-2000: Original(kdk)
 *
 */
/* ===================================================================== */

long    CADdriveFollower (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static  long follower, foldir, followersteps;

    cadDirLog ("driveFollower", pcad->dir, 5, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* convert the input strings to numbers for checking */

        if (sscanf (pcad->a, "%ld%s", &follower, dumpString) != 1)
        {
            strncpy(pcad->mess, "follower fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->b, "%ld%s", &foldir, dumpString) != 1)
        {
            strncpy(pcad->mess, "direction fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->c, "%ld%s", &followersteps, dumpString) != 1)
        {
            strncpy(pcad->mess, "using default #steps", MAX_STRING_SIZE - 1);
            followersteps = DSP_STEPS_DEFAULT;
        }
        if ((foldir != OUT) && (foldir != IN))
        {
            strncpy (pcad->mess, "bad direction", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if ((follower < FIRST_FOLLOWER) || (follower > LAST_FOLLOWER ))
        {
            strncpy (pcad->mess, "bad follower number", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* I don't think there is any reason to set vala and valb
            *(long *) pcad->vala = follower--;                Need to move 
                                 this down by one for M2 naming convention */
            *(long *) pcad->valb = foldir;

            epicsMutexLock(refMemFree);
                /* Note: should be long already from database */
            scsPtr->page0.follower      = (long) follower--;
            scsPtr->page0.foldir        = (long) foldir;
            scsPtr->page0.followersteps = (long) followersteps;
            epicsMutexUnlock(refMemFree);

            writeCommand(VDRVFOL);

            if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
                printf(
                "sent command %2d to drive follower %1d %d steps in direction %1d\n", 
                VDRVFOL, (int)follower, (int)followersteps, (int)foldir);
       
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
 * CADdriveOffloader
 *
 * Purpose:
 * Receive and sanity check the offloader, ofldir and offloadersteps values.
 * If successful they are written to the outputs.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADdriveOffloader(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string offloader
 *  > pcad->b   *string ofldir
 *  > pcad->c   *string offloadersteps
 *
 *  < pcd->mess *string status message
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Dayle Kotturi (dayle@gemini.edu)
 *
 * History:
 * 01-Feb-2000: Original(kdk)
 *
 */
/* ===================================================================== */

long    CADdriveOffloader (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static  long offloader, ofldir, offloadersteps;

    cadDirLog ("driveOffloader", pcad->dir, 5, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* convert the input strings to numbers for checking */

        if (sscanf (pcad->a, "%ld%s", &offloader, dumpString) != 1)
        {
            strncpy(pcad->mess, "offloader fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->b, "%ld%s", &ofldir, dumpString) != 1)
        {
            strncpy(pcad->mess, "direction fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->c, "%ld%s", &offloadersteps, dumpString) != 1)
        {
            strncpy(pcad->mess, "using default #steps", MAX_STRING_SIZE - 1);
            offloadersteps = DSP_STEPS_DEFAULT;
        }
        if ((ofldir != OUT) && (ofldir != IN))
        {
            strncpy (pcad->mess, "bad direction", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if ((offloader < FIRST_OFFLOADER) || (offloader > LAST_OFFLOADER))
        {
            strncpy (pcad->mess, "bad offloader number", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            epicsMutexLock(refMemFree);
            /* Note: should be long already from database */
            scsPtr->page0.offloader      = (long) offloader--;
            scsPtr->page0.ofldir        = (long) ofldir;
            scsPtr->page0.offloadersteps = (long) offloadersteps;
            epicsMutexUnlock(refMemFree);

            writeCommand(MDRVOFL);

            if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
                printf(
                "sent command %2d to drive offloader %1d %d steps in direction %1d\n", 
                MDRVOFL, (int)offloader, (int)offloadersteps, (int)ofldir);
       
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
 * CADdriveDB
 *
 * Purpose:
 * Receive and sanity check the deployable_baffle, dbsteps and dbafdir values.
 * If successful they are written to the outputs.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADdriveDB(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string deployable_baffle
 *  > pcad->b   *string dbafdir
 *  > pcad->c   *string dbsteps
 *
 *  < pcd->mess *string status message
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Dayle Kotturi (dayle@gemini.edu)
 *
 * History:
 * 01-Feb-2000: Original(kdk)
 *
 */
/* ===================================================================== */

long    CADdriveDB (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static  long deployable_baffle, dbafdir, dbsteps;

    cadDirLog ("driveDB", pcad->dir, 5, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* convert the input strings to numbers for checking */

        if (sscanf (pcad->a, "%ld%s", &deployable_baffle, dumpString) != 1)
        {
            strncpy(pcad->mess, "deployable_baffle fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->b, "%ld%s", &dbafdir, dumpString) != 1)
        {
            strncpy(pcad->mess, "direction fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->c, "%ld%s", &dbsteps, dumpString) != 1)
        {
            strncpy(pcad->mess, "using default #steps", MAX_STRING_SIZE - 1);
            dbsteps = STEPS_DEFAULT;
        }
        if ((dbafdir != UP) && (dbafdir != DOWN))
        {
            strncpy (pcad->mess, "bad direction", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if ((deployable_baffle < FIRST_DBAF) || (deployable_baffle > LAST_DBAF))
        {
            strncpy (pcad->mess, "bad deployable baffle number", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            epicsMutexLock(refMemFree);
            /* Note: should be long already from database */
            scsPtr->page0.deployable_baffle      = (long) deployable_baffle;
            scsPtr->page0.dbafdir = (long) dbafdir;
            scsPtr->page0.dbsteps = (long) dbsteps;
            epicsMutexUnlock(refMemFree);

            writeCommand(DBDRV);

            if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
                printf(
                "sent command %2d to drive deployable_baffle %1d %d steps in direction %1d\n", 
                DBDRV, (int)deployable_baffle, (int)dbsteps, (int)dbafdir);
       
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
 * CADdriveCB
 *
 * Purpose:
 * Receive and sanity check the cbafdir and cbsteps values.
 * If successful they are written to the outputs.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADdriveCB(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string cbafdir
 *  > pcad->b   *string cbsteps
 *
 *  < pcd->mess *string status message
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Dayle Kotturi (dayle@gemini.edu)
 *
 * History:
 * 01-Feb-2000: Original(kdk)
 *
 */
/* ===================================================================== */

long    CADdriveCB(struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static  long cbafdir, cbsteps;

    cadDirLog ("driveCB", pcad->dir, 5, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* convert the input strings to numbers for checking */

        if(sscanf (pcad->a, "%ld%s", &cbafdir, dumpString) != 1)
        {
            strncpy(pcad->mess, "direction fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->b, "%ld%s", &cbsteps, dumpString) != 1)
        {
            strncpy(pcad->mess, "using default #steps", MAX_STRING_SIZE - 1);
            cbsteps = STEPS_DEFAULT;
        }
        if ((cbafdir != OPEN) && (cbafdir != CLOSE))
        {
            strncpy (pcad->mess, "bad direction", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            epicsMutexLock(refMemFree);
            /* Note: should be long already from database */
            scsPtr->page0.cbafdir = (long) cbafdir;
            scsPtr->page0.cbsteps = (long) cbsteps;
            epicsMutexUnlock(refMemFree);

            writeCommand(CBDRV);

            if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
                printf(
                "sent command %2d to drive central baffle %d steps in direction %1d\n", 
                CBDRV, (int)cbsteps, (int)cbafdir);
       
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
 * CADdriveXY
 *
 * Purpose:
 * Receive and sanity check the xy_motor, xysteps and xydir values.
 * If successful they are written to the outputs.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADdriveXY(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string xy_motor
 *  > pcad->b   *string ofldir
 *  > pcad->c   *string xysteps
 *
 *  < pcd->mess *string status message
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 * None
 *
 * Prior requirements:
 * None
 *
 * Author:
 * Dayle Kotturi (dayle@gemini.edu)
 *
 * History:
 * 01-Feb-2000: Original(kdk)
 *
 */
/* ===================================================================== */

long    CADdriveXY (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    char    dumpString[MAX_STRING_SIZE];
    static  long xy_motor, xydir, xysteps;

    cadDirLog ("driveXY", pcad->dir, 5, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        /* convert the input strings to numbers for checking */

        if (sscanf (pcad->a, "%ld%s", &xy_motor, dumpString) != 1)
        {
            strncpy(pcad->mess, "xy motor fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->b, "%ld%s", &xydir, dumpString) != 1)
        {
            strncpy(pcad->mess, "direction fail conversion", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if(sscanf (pcad->c, "%ld%s", &xysteps, dumpString) != 1)
        {
            strncpy(pcad->mess, "using default #steps", MAX_STRING_SIZE - 1);
            xysteps = STEPS_DEFAULT;
        }
        if ((xydir != CW) && (xydir != CCW))
        {
            strncpy (pcad->mess, "bad direction", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }
        if ((xy_motor < UPPER_MOTOR) || (xy_motor > LOWER_MOTOR))
        {
            strncpy (pcad->mess, "bad xy motor number", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
        }

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            epicsMutexLock(refMemFree);
            /* Note: should be long already from database */
            scsPtr->page0.xy_motor      = (long) xy_motor;
            scsPtr->page0.xydir         = (long) xydir;
            scsPtr->page0.xysteps = (long) xysteps;
            epicsMutexUnlock(refMemFree);

            writeCommand(XYDRV);

            if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
                printf(
                "sent command %2d to drive xy motor[%1d] %d steps in direction %1d\n", 
                XYDRV, (int)xy_motor, (int)xysteps, (int)xydir);
       
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
 * CADtiltPidControl
 *
 * Purpose:
 * Accept a tilt PID on/off command from the engineering screens or TCS
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADtiltPidcontrol(pcad)
 *
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 *              > pcad->a       *string tilt PID on/off request
 *
 * Parameters out:
 *              < pcd->mess     *string status message
 *              < pcad->vala    long    tilt PID on/off request
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
 *
 * Author:
 * Dayle Kotturi  (dayle@gemini.edu)
 *
 * History:
 * 06-Mar-2000: Original(kdk)
 *
 */

/* ===================================================================== */

long CADtiltPidControl (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;
    static int tiltPidRqst;
    static char *tiltPidOpts[]= {"OFF", "ON", NULL};

    cadDirLog ("tiltPidControl", pcad->dir, 1, pcad);

    /* Fetch name of cad for messages */
    tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        status = CAD_REJECT;
        if (pcad->a[0])
        {
             if (tcsDcString (tiltPidOpts, " ", pcad->a, &tiltPidRqst, pcad)) break;
        }
        else
        {
            tcsCsAppendMessage (pcad, "no parameter given");
            break;
        }

        status = CAD_ACCEPT;
        break;

    case menuDirectiveSTART:

        /*
         * read current state of SCS to determine if tiltPid start is
         * appropriate
         */

        if (interlockFlag == ON)
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        else
        {
            tiltPidOn = tiltPidRqst;

#if 0
            if (debugLevel == DEBUG_RESERVED2)
            {
                printf("Setting tiltPidOn to %d\n", (int)tiltPidOn);
            }
#endif
            *(long *)pcad->vala = (long)tiltPidRqst;
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
 * CADfocusPidControl
 *
 * Purpose:
 * Accept a focus PID on/off command from the engineering screens or TCS
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADfocusPidcontrol(pcad)
 *
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 *              > pcad->a       *string focus PID on/off request
 *
 * Parameters out:
 *              < pcd->mess     *string status message
 *              < pcad->vala    long    focus PID on/off request
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
 *
 * Author:
 * Dayle Kotturi  (dayle@gemini.edu)
 *
 * History:
 * 06-Mar-2000: Original(kdk)
 *
 */

/* ===================================================================== */

long CADfocusPidControl (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;
    static int focusPidRqst;
    static char *focusPidOpts[]= {"OFF", "ON", NULL};

    cadDirLog ("focusPidControl", pcad->dir, 1, pcad);

    /* Fetch name of cad for messages */
    tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        status = CAD_REJECT;
        if (pcad->a[0])
        {
            if (tcsDcString (focusPidOpts, " ", pcad->a, &focusPidRqst, pcad)) break;
        }
        else
        {
            tcsCsAppendMessage (pcad, "no parameter given");
            break;
        }

        status = CAD_ACCEPT;
        break;

    case menuDirectiveSTART:

        /*
         * read current state of SCS to determine if focusPid start is
         * appropriate
         */

        if (interlockFlag == ON)
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        else
        {
            focusPidOn = focusPidRqst;
#if 0
            if (debugLevel == DEBUG_RESERVED2)
            {
                printf("Setting focusPidOn to %d\n", (int)focusPidOn);
            }
#endif

            *(long *)pcad->vala = (long)focusPidRqst;
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



#ifdef MK
/*
 *
 *
 *
 *
 */
long CADphasorControl (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;
    static long resetx, resety = 0;
    static int phasorRqst = -1;
    static int phasorAxisRqst = -1;
    static char *phasorOpts[]= {"OFF", "ON", NULL};
    static char *phasorAxisOpts[]= {"XTILT", "YTILT", NULL};

    cadDirLog ("phasorControl", pcad->dir, 1, pcad);

    /* Fetch name of cad for messages */
    tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        status = CAD_REJECT;
        if (pcad->a[0])
        {
             if (tcsDcString (phasorOpts, " phasorOffOn: ", pcad->a, &phasorRqst, pcad)) break;
        }
        else
        {
            tcsCsAppendMessage (pcad, "no parameter given");
            break;
        }

        printf("PHASORControl: Set phasorRqst=%d\n", phasorRqst);
        if (pcad->b[0])
        {
             if (tcsDcString (phasorAxisOpts, " phasorAxis: ", pcad->b, &phasorAxisRqst, pcad)) break;
        }
        else
        {
            tcsCsAppendMessage (pcad, "no parameter given");
            break;
        }

        /* Test for transition to ON and set reset(x|y) accordingly.*/
        if (phasorAxisRqst == XTILT && phasorRqst == ON) {
            resetx = 1;
        }

        else if (phasorAxisRqst == YTILT && phasorRqst == ON) {
            resety = 1;
        }

        printf("PHASORControl: Set phasorAxisRqst=%d\n", phasorAxisRqst);

        status = CAD_ACCEPT;
        break;

    case menuDirectiveSTART:

        /*
         * read current state of SCS to determine if tiltPid start is
         * appropriate
         */

        if (interlockFlag == ON)
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        else
        {


            if (phasorAxisRqst == XTILT) {
                phasorXApply = phasorRqst;
                xTiltGuideSimScale = 1.0;
            }
            else if (phasorAxisRqst == YTILT) {
                phasorYApply = phasorRqst;
                yTiltGuideSimScale = 1.0;
            }

            if (debugLevel == DEBUG_NONE)
            {
                printf("Set phasorXApply to %d\n", (int)phasorXApply);
            }
            if (debugLevel == DEBUG_NONE)
            {
                printf("Set phasorYApply to %d\n", (int)phasorYApply);
            }

            *(long *)pcad->vala = (long)phasorRqst;
            *(long *)pcad->valb = (long)phasorAxisRqst;

        }
        

        resetx = resety = 0;
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





/* 
 *
 *
 *
 *
 *
 *
 *
 * */
long CADvtkControl (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;
    static long resetx, resety = 0;
    static int vibTrackRqst = -1;
    static int vtkAxisRqst = -1;
    static char *vibTrackOpts[]= {"OFF", "ON", NULL};
    static char *vtkAxisOpts[]= {"XTILT", "YTILT", NULL};

    cadDirLog ("vibTrackControl", pcad->dir, 1, pcad);

    /* Fetch name of cad for messages */
    tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        status = CAD_REJECT;
        if (pcad->a[0])
        {
             if (tcsDcString (vibTrackOpts, " vtkOffOn: ", pcad->a, &vibTrackRqst, pcad)) break;
        }
        else
        {
            tcsCsAppendMessage (pcad, "no parameter given");
            break;
        }

        printf("VTKControl: Set vibTrackRqst=%d\n", vibTrackRqst);
        if (pcad->b[0])
        {
             if (tcsDcString (vtkAxisOpts, " vtkAxis: ", pcad->b, &vtkAxisRqst, pcad)) break;
        }
        else
        {
            tcsCsAppendMessage (pcad, "no parameter given");
            break;
        }

        /* Test for transition to ON and set reset(x|y) accordingly.*/
        if (vtkAxisRqst == XTILT && vibTrackRqst == ON) {
            resetx = 1;
        }

        else if (vtkAxisRqst == YTILT && vibTrackRqst == ON) {
            resety = 1;
        }

        printf("VTKControl: Set vtkAxisRqst=%d\n", vtkAxisRqst);

        status = CAD_ACCEPT;
        break;

    case menuDirectiveSTART:

        /*
         * read current state of SCS to determine if tiltPid start is
         * appropriate
         */

        if (interlockFlag == ON)
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        else
        {

            if (vtkAxisRqst == XTILT) {
                vibrationXTrackOn = vibTrackRqst;
            }

            else if (vtkAxisRqst == YTILT) {
                vibrationYTrackOn = vibTrackRqst;
            }        

            if (debugLevel == DEBUG_NONE)
            {
                printf("Set vibrationXTrackOn to %d\n", (int)vibrationXTrackOn);
            }
            if (debugLevel == DEBUG_NONE)
            {
                printf("Set vibrationYTrackOn to %d\n", (int)vibrationYTrackOn);
            }

            *(long *)pcad->vala = (long)vibTrackRqst;
            *(long *)pcad->valb = (long)vtkAxisRqst;

            /*Reset VTK_X if transitioning to ON*/
            if(resetx) vtkResetX();

            /*Reset VTK_Y if transitioning to ON*/
            if(resety) vtkResetY();

        }
        

        resetx = resety = 0;
        printf("VTKControl: Set vtkAxisRqst=%d\n", vtkAxisRqst);
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

#endif



/* ===================================================================== */
/*
 * Function name:
 * CADmovepersicope
 *
 * Purpose:
 * The demanded positions for periscope are read
 * from the CAD inputs and converted to long format. If the current
 * state of the scs is moving or chopping then the forward start link
 * is triggered otherwise the command is rejected
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADmoveperiscope(pcad)
 *
 * Parameters: ( ">" input, "!" modified, "<" output)
 *  > pcad->dir *string CAD directive
 *  > pcad->a   *string demanded periscope position
 *
 *  < pcad->mess    *string status message
 *  < pcad->vala    long demanded periscope position
 *
 * Function value:
 *  < status    long
 *
 * External functions:
 * None
 *
 * External variables:
 *  > scsState  current state of the SCS
 *
 * Prior requirements:
 * None
 *
 * Author:
 *
 * History:
 * 28-feb-2005: creation
 *
 *
 */
/* ===================================================================== */

long    CADmovePeriscope (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    static  int periscope;

    cadDirLog ("movePeriscope", pcad->dir, 1, pcad);

    /* Set message prefix */
    tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char *) NULL);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
 
        status = CAD_REJECT ;
	printf(" CADmovePeriscope : %s\n",pcad->a);
        if (tcsDcString (periscopeOption, "central - ", pcad->a, 
            &periscope, pcad)) 
            break;

        if (periscope < 0 || periscope > 1) 
        {
            tcsCsAppendMessage (pcad, "periscope out of range") ;
            break;
        }

        status = CAD_ACCEPT ;

        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* brought from SNL 08-FEB-2000 */
            epicsMutexLock(refMemFree);
            scsPtr->page0.centralBaffle = (long)periscope;
            epicsMutexUnlock(refMemFree);

            writeCommand(BAFFLE_CHANGE);
    
            errlogPrintf( "sent command %2d to move periscope to pos: %1d \n", 
                BAFFLE_CHANGE, periscope); 
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
