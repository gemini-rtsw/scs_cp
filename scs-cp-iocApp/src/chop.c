/*+
 *
 * FILENAME
 * -------- 
 * chop.c
 * 
 * PURPOSE
 * -------
 * Set of functions for the configuration and control of chopping
 * 
 * FUNCTION NAME(S)
 * ----------------
 * CADchopConfig    - accept chop configuration parameters
 * CADchopControl   - accept chop control parameters
 * CADbeamJog       - force M2 to beam position A, B or C
 * percentCalc      - Calculate percentage samples in position while chopping
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
 * 24-Jul-1997: original (srp)
 * 23-Oct-1997: modify chopcontrol so chop on may only be started from state MOVING  
 * 23-Feb-1998: change strcpy to strncpy for robustness
 * 29-Jun-1998: Add beamJog funtion
 * 01-Jul-1998: Modify to use tcs input parameter checking functions
 * 07-May-1999: Added RCS id
 * 19-May-1999: Replaced hard-coded values in calcEnvelope with macros 
 *      where possible
 * 12-Jun-2000: Move chopIsOn here from chopControl.c
 * 24-Oct-2017: Begin conversion to EPICS OSI (mdw)
 *
 */
/* ===================================================================== */
#include <stdio.h>
#include <string.h>

#include <cad.h>
#include <tcslib.h>

#include "utilities.h"      /* for all the EPICS base includes, plus other stuff */
#include "chop.h"               
#include "archive.h"        /* for cadDirLog, refMemFree */
#include "control.h"        /* For scsPtr, interlockFlag */




/* Define frequency limits for the various chop profiles (Hertz)  */

#define TWO_POINT_MIN           0.12
#define TWO_POINT_MAX           10
#define THREE_POINT_MIN         0.1
#define THREE_POINT_MAX         5
#define TRIANGLE_MIN            0.01
#define TRIANGLE_MAX            0.1
#define DUTY_LOW                60.0
#define DUTY_HIGH               95.0

/* Define limits of operating envelope */

typedef struct
{
   double breakChopFreq;           /* Hertz        */
   double breakChopFreqThrow;      /* arcsecs      */
   double maxChopFreq;             /* arcsecs      */
   double maxChopFreqThrow;        /* arcsecs      */
   double maxTilt;                 /* arcsecs      */
   double maxTiltZ;                /* microns      */
   double minChopFreq;             /* arcsecs      */
   double minChopFreqThrow;        /* arcsecs      */
   double minTilt;                 /* arcsecs      */
   double minTiltZ;                /* microns      */
} envelope;

/* Declare external variables */
int chopIsOn = 0;
int m2ChopResponseOK = 0;
int chopIsPending = 0;
int decsIsPending = 0;
int jogBeam = BEAMA;

instStructure instruments[MAX_CHOP_CONTROLLERS] = {
   {"scs        ", 0, "no", 0, "STROBE"},
   {"instrument1", 0, "no", 0, "STROBE"},
   {"instrument2", 0, "no", 0, "STROBE"},
   {"instrument3", 0, "no", 0, "STROBE"},
   {"instrument4", 0, "no", 0, "STROBE"},
   {"instrument5", 0, "no", 0, "STROBE"}
};

/* Local globals */
static double frequency = 0.0;
static double dutyCycle = 0.0;

/* New: Accessor function */
double getChopDuty(void)
{
   return dutyCycle;
}

/* New: Accessor function */
double getChopFreq(void)
{
   return frequency;
}

/* ===================================================================== */
/*
 * Function name:
 * CADchopConfig
 * 
 * Purpose:
 * Set up the desired chop configuration for the tilt system. Limit check
 * values and accept or reject accordingly
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADchopconfig(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 *      > pcad->a   *string chop profile
 *      > pcad->b   *string synch source
 *      > pcad->c   *string frequency
 *      > pcad->d   *string duty cycle

 * 
 * Parameters out:
 *      < pcd->mess *string status message
 *      < pcad->vala    long    demanded chop profile
 *      < pcad->valb    long    demanded synchronisation source
 *      < pcad->valc    double  demanded chop frequency
 *      < pcad->vald    double  demanded duty cycle
 *      < pcad->vale    double  controlling instrument port
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
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 24-Jul-1996: Original(srp)
 * 16-May-1997: add chop duty cycle, change chopconfig to chopConfig
 *      also ensure inputs interpreted as strings
 * 15-Jun-1997: look up instrument details for event system interface.
 * 24-Feb-1998: defer looking up port from syncSource until menuDirectiveSTART
 * 
 */

/* ===================================================================== */

long CADchopConfig (struct cadRecord * pcad)
{
   long    status = CAD_ACCEPT;
   static int profile, syncSource;
   static char *profileOpts[] = {"2POSN","3POSN","TRIANGULAR", NULL};
   static char *ssOpts[] = {"SCS", "ICS0", "ICS1", "ICS2", "ICS3",
                                 "ICS4", NULL} ; 

   cadDirLog ("chopConfig", pcad->dir, 4, pcad);

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

       if (tcsDcString (profileOpts, "profile - ", pcad->a, &profile, pcad))
       {
           errorLog ("CADchopConfig - failed profile conversion", 2, ON);
           break ;
       }

       if (tcsDcString (ssOpts, "sync. source - ", pcad->b, &syncSource, pcad))
       {
           errorLog ("CADchopConfig - failed syncsource conversion", 2, ON);
           break ;
       }

       if (tcsDcDouble ("frequency - ", pcad->c, &frequency, pcad))
       {
           errorLog ("CADchopConfig - failed frequency conversion", 2, ON);
           break ;
       }
   printf("in CADchopConfig preset, frequency is %f\n", frequency);

       if (tcsDcDouble ("duty cycle - ", pcad->d, &dutyCycle, pcad))
       {
           errorLog ("CADchopConfig - failed duty cycle conversion", 2, ON);
           break ;
       }
       else
       {
           if( dutyCycle < DUTY_LOW || dutyCycle > DUTY_HIGH)
           {
               tcsCsAppendMessage (pcad, "duty cycle out of range") ;
               break;
           } 
       }

       /* all conversions complete, check values are in range */

       if (profile == TWOPOINT)
       {
           if ((frequency > TWO_POINT_MAX) || (frequency < TWO_POINT_MIN))
           {
               tcsCsAppendMessage (pcad, "frequency out of range") ;
               break;
           }
       }
       else if (profile == THREEPOINT)
       {
           if ((frequency > THREE_POINT_MAX) || (frequency < THREE_POINT_MIN))
           {
               tcsCsAppendMessage (pcad, "frequency out of range") ;
               break;
           }
       }
       else if (profile == TRIANGLE)
       {
           if ((frequency > TRIANGLE_MAX) || (frequency < TRIANGLE_MIN))
           {
               tcsCsAppendMessage (pcad, "frequency out of range") ;
               break;
           }
       }
       else
       {
           tcsCsAppendMessage (pcad, "profile not recognised");
           break;
       }

       status = CAD_ACCEPT ;
       break;

   case menuDirectiveSTART:

       /* check the current state of the SCS */

       if (chopIsOn == 0)
       {
          if (interlockFlag != 1)
          {
           *(long *) pcad->vala = (long)profile;
           *(long *) pcad->valb = (long)syncSource;
           *(double *) pcad->valc = frequency;
           *(double *) pcad->vald = dutyCycle;
           *(long *) pcad->vale = instruments[syncSource].port;

      if (syncSource == 0)
       {
           writeCommand(SYNC_SOURCE_M2);
      errlogMessage("CADchopConfig - telling M2 sync source is M2\n");
       }
       else
       {
           writeCommand(SYNC_SOURCE_SCS);
           errlogMessage("CADchopConfig - telling M2 sync source is external to M2\n");
       }
       /* write chop parameters to reflective memory */

       if(semTake(refMemFree, SEM_TIMEOUT) == OK)
       {
           scsPtr->page0.chopFrequency = (float)frequency;
           scsPtr->page0.chopProfile   = (long)profile;
           scsPtr->page0.chopDutyCycle = (float)dutyCycle;
           semGive(refMemFree);
       }
       else
       {
           errorLog("CADchopConfig - timeout on refMemFree", 1, ON);
       }

       /* flag that chop configuration has been changed */

       writeCommand(CHOP_CHANGE);
       printf
      ("CADchopConfig - telling M2 chop config has changed to freq=%f, dutyCycle=%f\n", 
       frequency, dutyCycle);
/*       taskDelay(5);
       writeCommand(CHOP_CHANGE);
       printf
      ("CADchopConfig - telling M2 chop config has changed to freq=%f, dutyCycle=%f\n", 
       frequency, dutyCycle); */
            }
           else
            {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            }
        }
        else
        {
            strncpy (pcad->mess, "Cannot change while chop in progress", MAX_STRING_SIZE - 1);
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
 * CADchopControl
 * 
 * Purpose:
 * Turn the chopping off and on. Provide control of some of the
 * tilt system's adaptive chopping using the DECS switches
 * The DECS command switches are copied directly to the outputs.
 * The chop on/off request is examined with the current state of
 * the scs.  If appropriate the chop on/off request is copied to
 * the output.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADchopControl(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 *      > pcad->a   *string chop on
 *      > pcad->b   *string decs on
 *      > pcad->c   *string decs pause
 *      > pcad->d   *string decs freeze
 *      > pcad->e   *string decs reset
 * 
 * Parameters out:
 *      < pcd->mess *string status message
 *      < pcad->vala    long chop on
 *      < pcad->valb    long decs on
 *      < pcad->valc    long decs pause
 *      < pcad->vald    long decs freeze
 *      < pcad->vale    long decs reset
 * 
 * Return value:
 *      < status    long
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *      > scsState  current state of the scs
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 24-Jul-1997: Original(srp)
 * 23-Oct-1997: reject command if chop on request is made when scs not MOVING
 * 19-Jun-1998: Use input strings rather than numbers (cjm)
 * 
 */

/* ===================================================================== */

long    CADchopControl (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    static int chopon, decson, decspause, decsfreeze,
            decsreset;
        static char *chopOpts[] = {"OFF", "ON", NULL} ;

    cadDirLog ("chopControl", pcad->dir, 5, pcad);

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

                if (tcsDcString (chopOpts, "chopon - ", pcad->a, &chopon,
                    pcad)) break ;
                if (tcsDcString (chopOpts, "decson - ", pcad->b, &decson,
                    pcad)) break ;
                if (tcsDcString (chopOpts, "decspause - ", pcad->c, &decspause,
                    pcad)) break ;
                if (tcsDcString (chopOpts, "decsfreeze - ", pcad->d, &decsfreeze,
                    pcad)) break ;
                if (tcsDcString (chopOpts, "decsreset - ", pcad->e, &decsreset,
                    pcad)) break ;
 
                status = CAD_ACCEPT ;
        break;

    case menuDirectiveSTART:

        if(interlockFlag == 1)
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        else
        {
            /* trigger the start link only */

            *(long *) pcad->vala = (long)chopon;
            *(long *) pcad->valb = (long)decson;
            *(long *) pcad->valc = (long)decspause;
            *(long *) pcad->vald = (long)decsfreeze;
            *(long *) pcad->vale = (long)decsreset;
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
 * CADbeamJog
 * 
 * Purpose:
 * Allow the user to force the M2 position to beam A, B or C in order to
 * set up for a chopping run.
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADbeamJog(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 *      > pcad->a   *string desired beam {BEAMA | BEAMB | BEAMC}
 * 
 * Parameters out:
 *      < pcd->mess *string status message
 *      < pcad->vala    long    desired beam {BEAMA | BEAMB | BEAMC}
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
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 29-Jun-1998: Original(srp)
 * 
 */

/* ===================================================================== */

long    CADbeamJog (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    static  int requestedBeam = BEAMA;

    cadDirLog ("beamJog", pcad->dir, 1, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

        if(!strcmp(pcad->a, "A"))
        {
            requestedBeam = BEAMA;
        }
        else if(!strcmp(pcad->a, "B"))
        {
            requestedBeam = BEAMB;
        }
        else if(!strcmp(pcad->a, "C"))
        {
            requestedBeam = BEAMC;
        }
        else
        {
            strncpy (pcad->mess, "beam not recognised", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        if(status == CAD_ACCEPT)
        {
            *(long *)pcad->vala = requestedBeam;
        }

        break;

    case menuDirectiveSTART:

        /* check the current state of the SCS */

        if (interlockFlag == 1)
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        if (chopIsOn == 1)
        {
            strncpy (pcad->mess, "chop in progress", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        else
        {
            /* copy value to global */

            jogBeam = requestedBeam;
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
 * initEnvelope
 * calcEnvelope
 * 
 * Purpose:
 * Function takes the current values of chop profile, frequency and
 * mirror position to calculate demand limits
 * 
 * Invocation:
 * struct genSubRecord *pgsub
 * status = calcEnvelope(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 *      <   pgsub->a    long    profile
 *      <   pgsub->b    double  frequency
 *      <   pgsub->c    double  xTilt
 *      <   pgsub->d    double  yTilt
 *      <   pgsub->e    double  zFocus
 * 
 * Parameters out:
 *      >   pgsub->vala double  breakChopFreq
 *      >   pgsub->valb double  breakChopFreqThrow
 *      >   pgsub->valc double  maxChopFreq
 *      >   pgsub->vald double  maxChopFreqThrow
 *      >   pgsub->vale double  maxTilt
 *      >   pgsub->valf double  maxTiltZ
 *      >   pgsub->valg double  minChopFreq
 *      >   pgsub->valh double  minChopFreqThrow
 *      >   pgsub->vali double  minTilt
 *      >   pgsub->valj double  minTiltZ
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
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 14-Aug--1997: Original(srp)
 * 
 */

/* ===================================================================== */

/* static long    initEnvelope (struct genSubRecord * pgsub)
{
    return (OK);
}
*/

long    calcEnvelope (struct genSubRecord * pgsub)
{
    int     profile;
    long    status = OK;

    /* create structure to hold chop envelope */

    envelope chopLimit;

    /* fetch current chop parameters from inputs */

    profile     = *(int *) pgsub->a;

    /* calculate limits of envelope */

    switch (profile)
    {
    case TWOPOINT:
        chopLimit.breakChopFreq = 5;
        chopLimit.breakChopFreqThrow = 132;
        chopLimit.maxChopFreq = TWO_POINT_MAX;
        chopLimit.maxChopFreqThrow = 60;
        chopLimit.maxTilt = X_TILT_LIMIT;
        chopLimit.maxTiltZ = 5;
        chopLimit.minChopFreq = TWO_POINT_MIN;
        chopLimit.minChopFreqThrow = 132;
        chopLimit.minTilt = 0;
        chopLimit.minTiltZ = 10;

        break;

    case THREEPOINT:
        chopLimit.breakChopFreq = 2.5;
        chopLimit.breakChopFreqThrow = 132;
        chopLimit.maxChopFreq = THREE_POINT_MAX;
        chopLimit.maxChopFreqThrow = 60;
        chopLimit.maxTilt = X_TILT_LIMIT;
        chopLimit.maxTiltZ = 5;
        chopLimit.minChopFreq = THREE_POINT_MIN;
        chopLimit.minChopFreqThrow = 132;
        chopLimit.minTilt = 0;
        chopLimit.minTiltZ = 10;

        break;

    case TRIANGLE:
        chopLimit.breakChopFreq = 0.1;
        chopLimit.breakChopFreqThrow = 40;
        chopLimit.maxChopFreq = TRIANGLE_MAX;
        chopLimit.maxChopFreqThrow = 40;
        chopLimit.maxTilt = X_TILT_LIMIT;
        chopLimit.maxTiltZ = 5;
        chopLimit.minChopFreq = TRIANGLE_MIN; 
        chopLimit.minChopFreqThrow = 40;
        chopLimit.minTilt = 0;
        chopLimit.minTiltZ = 10;

        break;

    default:

        errorLog ("Chop Envelope limits - unrecognised chop profile", 2, ON);

        /* Although profile unrecognised, use two point set up for
         * initialisation */

        chopLimit.breakChopFreq = 5;
        chopLimit.breakChopFreqThrow = 132;
        chopLimit.maxChopFreq = TWO_POINT_MAX;
        chopLimit.maxChopFreqThrow = 60;
        chopLimit.maxTilt = X_TILT_LIMIT;
        chopLimit.maxTiltZ = 5;
        chopLimit.minChopFreq = TWO_POINT_MIN;
        chopLimit.minChopFreqThrow = 132;
        chopLimit.minTilt = 0;
        chopLimit.minTiltZ = 10;

        status = ERROR;

    }

    /* write envelope limits to output ports */

    if (status == OK)
    {
        *(double *) pgsub->vala = chopLimit.breakChopFreq;
        *(double *) pgsub->valb = chopLimit.breakChopFreqThrow;
        *(double *) pgsub->valc = chopLimit.maxChopFreq;
        *(double *) pgsub->vald = chopLimit.maxChopFreqThrow;
        *(double *) pgsub->vale = chopLimit.maxTilt;
        *(double *) pgsub->valf = chopLimit.maxTiltZ;
        *(double *) pgsub->valg = chopLimit.minChopFreq;
        *(double *) pgsub->valh = chopLimit.minChopFreqThrow;
        *(double *) pgsub->vali = chopLimit.minTilt;
        *(double *) pgsub->valj = chopLimit.minTiltZ;
    }

    return (OK);
}


double  percentCalc (struct subRecord * psub)
{
    static  int oldBeam = 0;
    static  int sampleCount = 0;
    static  int coincidence = 0;
    static  double percentage = 0;
    int inPos, beamNow;

    /* if not chopping, set to 100% and exit */

    if(chopIsOn != 1)
    {
        psub->val = 100.0;
        return(OK);
    }   

    if(semTake(refMemFree, SEM_TIMEOUT) == OK)
    {
        /* grab in position and beam position */

        beamNow = scsPtr->page1.beamPosition;
        /*NEW (srp) invert sense of inPosition */
        /*inPos = scsPtr->page1.inPosition;*/
        inPos = !scsPtr->page1.inPosition;

        semGive(refMemFree);

        /* check if beam has chopped */

        if (beamNow != oldBeam)
        {
            /* calculate statistics for the last cycle */
            /* guard for divide by zero error */

            if(sampleCount > 0)
                percentage = 100 * ((double) coincidence / (double) sampleCount);

            oldBeam = beamNow;

            coincidence = 0;
            sampleCount = 0;

            psub->val = percentage;
        }

        /* if no transition just increment counts */

        sampleCount++;
    
        if (inPos == 0)
            coincidence++;

        return(OK);
    }
    else
    {
        logMsg("percent in position - semtimeout\n", 0, 0, 0, 0, 0, 0);
        return(ERROR);
    }
}

