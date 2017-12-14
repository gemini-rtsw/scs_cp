/* $Id: guide.c,v 1.23 2015/05/01 00:09:39 mrippa Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * --------
 * guide.c
 *
 * PURPOSE
 * -------
 * Functions accept configuration data regarding the filtering to be applied
 * to the guide sources. Additional functions provided to clear the filters
 * and to examine the coefficient values for test purposes.
 *
 * FUNCTION NAME(S)
 * ----------------
 * CADguideControl      - Read guide on/off from CAD
 * CADguideConfig       - Read guide configuration parameters
 * createFilter         - create filter to demanded specification
 * clearFilters         - reset filter history to zero
 * displayFilter        - show filter coefficients
 * displayCoeffs        - show filter coefficient table
 * lookupConfig         - keep record of previous guide configs to update widgets
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
 * 24-Oct-1997: modify guide control to use local guideRqst and global guideOn flags
 * 24-Feb-1998: function readCoeffs now subsumed by createFilter - remove
 * 03-Mar-1998: load filter files from /data directory rather that /src
 * 22-Jun-1998: Remove calls to setHealth, command completion reported via CARs only
 * 23-Jun-1998: Write to mess field following param conversion failure
 * 02-Jul-1998: Reject paramters with extraneous items e.g. 3.1xx
 * 07-Jul-1998: Correct ommitted 'interlocks active' message for CADguideConfig
 * 04-Dec-1998: Bug fix to calculation of pgsub->valr (dk)
 * 11-Jan-1999: Bug fix-initialise values of freq1, freq2 and samplefreq so
 *              valid values are present even when RAW filtering is selected
 * 19-Feb-1999: zero PID integral accumulators when guiding turned on/off
 *              Modify decimator code to supply TCS with guide values before
 *              application of m2 PID algorithm
 * 02-Mar-1999: Previous mod was incorrect. The TCS needs the output of the PID
 *              algorithm. Also, only the guide values are read by the TCS so there
 *              is no need to filter the other values - this reduces processor load.
 * 02-Mar-1999: Zero net guide values in CadGuideControl
 * 11-May-1999: Added RCS id
 * 01-Jun-1999: Changed guideOn to type "long" to match scs_st.stpp
 *              Reset the updateInterval for pwfs2 to zero each time guide
 *              is restarted.
 *              Added declaration of global updateInterval since it is 
 *              compiled before control.c (where it used to be declared)
 *              Zero the interval for pwfs2 
 *              Changed tabs to blanks
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <tcslib.h>
#include <cad.h>
#include <car.h>

#include "guide.h"
#include "archive.h"        /* For cadDirLog */
#include "control.h"        /* For simLevel, scsBase, m2Ptr, m2MemFree, 
                               interlockFlag, controller, *GuideTcs,
                               currentBeam */
#include "utilities.h"      /* For weight2string, errorLog, debugLevel */
#include "chop.h"           /* For chopIsOn */




#define MAX_FILTER_CHANNELS 15  /* Used by dfilter to set array size */
#define NUM_DECIMATORS  12      /* Number of channels needing decimation 
                                   filters */
#define MAX_HISTORY     11      /* Number of previous samples filters 
                                   need to keep track of     */

/* Filenames of filter coefficients */

#define LOW_COEFFS "./data/low.dat"
#define HIGH_COEFFS "./data/high.dat"
#define PASS_COEFFS "./data/pass.dat"
#define STOP_COEFFS "./data/stop.dat"

#define LOW_WEIGHT_LIMIT   -2
#define HIGH_WEIGHT_LIMIT 100

#define DECIM_CUTOFF        0.05 /* cutoff for decimation filters 
                                    (1.0 = half sample frequency) */

/* Guide source names */

static char *filterName[] =
{
    "OFF     ",
    "RAW     ",
    "LOWPASS ",
    "HIGHPASS",
    "BANDPASS",
    "BANDSTOP",
    NULL
};

/*
char *wfsName[] =
{
    "PWFS1",
    "PWFS2",
    "OIWFS",
    "GAOS ",
    "GYRO ",
#ifndef MK
     "GPI"
#endif
    NULL
};
*/

static MATLAB decFilter[12];

/* define prototypes */
static int clearFilters (int);
int displayFilter (const int source, const int axis);
static int readFilters (MATLAB * filterAddr, int type, 
                        double freq1, double freq2);
static double dfilter(double newSample, int Id);

/* Declare external variables */

anUpdateInterval updateInterval = { 0.0, 0.0, 0.0, 0.0  };

#ifdef MK
GuideInfo guideInfo = { 0.0, 0, {1.557,-9.7, 198.9}, {1.557,-9.7, 198.9}};
#endif

long guideOn;
long guideSimOn;
int guideOnA = OFF;
int guideOnB = OFF;
int guideOnC = OFF;

double weight[MAX_SOURCES][MAX_BEAMS] =
{
    {-2.0, -2.0, -2.0, -2.0, -2.0},
    {-2.0, -2.0, -2.0, -2.0, -2.0},
    {-2.0, -2.0, -2.0, -2.0, -2.0},
    {-2.0, -2.0, -2.0, -2.0, -2.0},
    {-2.0, -2.0, -2.0, -2.0, -2.0}
};
int guideMaster[MAX_SOURCES][MAX_BEAMS] =
{
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}
};

MATLAB filter[MAX_SOURCES][MAX_AXES];

#ifdef MK
HighSpeed *highSpeedData;
#endif


/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADclearGuideFocus
 *
 * Purpose:
 * Clear the focus applied
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADclearGuideFocus(pcad)
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
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 14-May-2001: Original(rro)
 *
 */

/* INDENT ON */
/* ===================================================================== */

long CADclearGuideFocus (struct cadRecord * pcad)
{
     long status = CAD_ACCEPT;

     cadDirLog ("clearGuideFocusLog", pcad->dir, 1, pcad);

     /* Fetch name of cad for messages */
     tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL) ;

     switch (pcad->dir)
     {
     case menuDirectiveSTART:
          printf ("CADclearGuideFocus - menuDirectiveSTART\n");
          /*
           * read current state of SCS to determine if guide start is
           * appropriate
           */

          if (interlockFlag == ON)
          {
               strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
               status = CAD_REJECT;
          }
          else
          {
	       if (guideOn)
	       {
                  printf ("CADclearGuideFocus - guide is ON \n");
               	  strncpy (pcad->mess, "guide active", MAX_STRING_SIZE - 1);
                  status = CAD_REJECT;
               }
	       else
	       {
                  printf ("CADclearGuideFocus - sum %f oldS %f oldE %f \n",
               	  	controller[FOCUS].sum, 
			controller[FOCUS].oldSum,
			controller[FOCUS].oldError);
/*
		  if (controller[FOCUS].sum > 500. )
		  {
			controller[FOCUS].sum -= 500.;
			controller[FOCUS].oldSum -= 500.;
                        printf ("CADclearGuideFocus - sum %f oldS %f \n",
               	  	controller[FOCUS].sum, 
			controller[FOCUS].oldSum);
		  }
		  else	
		  {
               	  controller[FOCUS].sum = 0.0;
               	  controller[FOCUS].oldSum = 0.0;
               	  controller[FOCUS].oldError = 0.0;
		  }  */

	       }

          }
          break;

     case menuDirectiveCLEAR:
          break;

     case menuDirectivePRESET:

          /* status = CAD_REJECT; */
          break;

     case menuDirectiveMARK:
          printf ("CADclearGuideFocus - menuDirectiveMARK\n");
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
/* INDENT OFF */
/*
 * Function name:
 * CADclearTiltGuide
 *
 * Purpose:
 * Clear the tilt applied guide
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADclearTiltGuide(pcad)
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
 *
 * History:
 * 15-May-2002: Original(rro)
 *
 */

/* INDENT ON */
/* ===================================================================== */

long CADclearTiltGuide (struct cadRecord * pcad)
{
     long status = CAD_ACCEPT;

     cadDirLog ("clearTiltGuide", pcad->dir, 1, pcad);

     /* Fetch name of cad for messages */
     tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL) ;

     switch (pcad->dir)
     {
     case menuDirectiveMARK:
          printf ("CADclearTiltGuide - menuDirectiveMARK\n");
          /*
           * read current state of SCS to determine if guide start is
           * appropriate
           */

          if (interlockFlag == ON)
          {
               strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
               status = CAD_REJECT;
          }
          else
          {
	       if (guideOn)
	       	{
          	printf ("CADclearTiltGuide - guide os ON \n");
               	strncpy(pcad->mess,"guide active",MAX_STRING_SIZE - 1);
                status = CAD_REJECT;
          	}
	       else
	       {
               	controller[XTILT].sum = 0.0;
               	controller[XTILT].oldSum = 0.0;
               	controller[XTILT].oldError = 0.0;
               	controller[YTILT].sum = 0.0;
               	controller[YTILT].oldSum = 0.0;
               	controller[YTILT].oldError = 0.0;
	       }

          }
          break;

     case menuDirectiveCLEAR:
          break;

     case menuDirectivePRESET:

/*           status = CAD_REJECT; */
          break;

     case menuDirectiveSTART:
          printf ("CADclearTiltGuide - menuDirectiveSTART\n");
          break;

     case menuDirectiveSTOP:
          break;

     default:
          strncpy(pcad->mess,"inappropriate CAD directive",MAX_STRING_SIZE-1);
          status = CAD_REJECT;
          break;
     }

     return (status);
}


/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADguideControl
 *
 * Purpose:
 * Accept a guide on/off command from the engineering screens or TCS
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADguidecontrol(pcad)
 *
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 *              > pcad->a       *string guide on/off request
 *
 * Parameters out:
 *              < pcd->mess     *string status message
 *              < pcad->vala    long    guide on/off request
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
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 24-Jul-1996: Original(srp)
 * 31-May-1997: Name capitalisation change to CADguideControl from CADguidecontrol
 * 24-Oct-1997: Tidy up multiple guideOn, guideOverride, guideState - use only guideOn
 *              as the global flag and guideRqst for local
 * 19-Feb-1999: zero PID integral accumulators when guiding turned on/off
 * 01-Mar-1999: Also zero net guide values
 *
 */

/* INDENT ON */
/* ===================================================================== */

long CADguideControl (struct cadRecord * pcad)
{
     long status = CAD_ACCEPT;
     static int guideRqst ;
     static char *guideOpts[]= {"OFF", "ON", NULL} ;
     int i;

     cadDirLog ("guideControl", pcad->dir, 1, pcad);

     /* Fetch name of cad for messages */
     tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL) ;

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
               if (tcsDcString (guideOpts, " ", pcad->a, &guideRqst, pcad)) break ;
          }
          else
          {
               tcsCsAppendMessage (pcad, "no parameter given") ;
               break ;
          }

          status = CAD_ACCEPT ;
          break;

     case menuDirectiveSTART:

          /*
           * read current state of SCS to determine if guide start is
           * appropriate
           */

          if (interlockFlag == ON)
          {
               strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
               status = CAD_REJECT;
          }
          else
          {
               guideOn = guideRqst;

               /* zero PID integral values */

               controller[XTILT].sum = 0.0;
               controller[YTILT].sum = 0.0;
               controller[XTILT].oldSum = 0.0;
               controller[YTILT].oldSum = 0.0;
               controller[XTILT].oldError = 0.0;
               controller[YTILT].oldError = 0.0; 

               /*controller[FOCUS].sum = 0.0;
               controller[FOCUS].oldSum = 0.0;
               controller[FOCUS].oldError = 0.0;*/

               /* zero guide outputs to TCS */

               xGuideTcs = 0.0;
               yGuideTcs = 0.0;
               zGuideTcs = 0.0;

               /* zero the interval for all wfs */

               updateInterval.pwfs2 = 0.0;
               updateInterval.pwfs1 = 0.0;
               updateInterval.oiwfs = 0.0;
               updateInterval.gaos = 0.0;
#ifndef MK
               updateInterval.gpi = 0.0;
#endif

               /* zero the filtered error values for all sources */

/*               if (debugLevel == DEBUG_RESERVED2)
                   printf ("CADguideControl - zeroing filtered terms\n"); */

               for (i=0; i<MAX_SOURCES; i++)
               {
                   filtered[i].z1 = 0.0;
                   filtered[i].z2 = 0.0;
                   filtered[i].z3 = 0.0;
               }

	       /* fix up currentBeam if not chopping */
	       if (!chopIsOn)
	       {
/*		   if (debugLevel == DEBUG_RESERVED2)
		       printf("CADguideControl - setting currentBeam to A\n"); */
		   currentBeam = 0;
	       }

               *(long *)pcad->vala = (long)guideRqst ;
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
/* INDENT OFF */
/*
 * Function name:
 * CADguideConfig
 *
 * Purpose:
 * Limit check the guide configuration parameters on PRESET. If acceptable
 * copy to the output ports.
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADguideConfig(pcad)
 *
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 *              > pcad->a       *string sensor source
 *              > pcad->b       *string sample frequency
 *              > pcad->c       *string filter type
 *              > pcad->d       *string edge frequency 1
 *              > pcad->e       *string edge frequency 2
 *              > pcad->f       *string filter type FIR or IIR
 *              > pcad->g       *string combination weight beam A
 *              > pcad->h       *string combination weight beam B
 *              > pcad->i       *string combination weight beam C
 *              > pcad->j       *string reset all to none
 *
 * Parameters out:
 *              < pcd->mess     *string status message
 *              < pcad->vala    long    sensor source
 *              < pcad->valb    double  sample frequency
 *              < pcad->valc    long    filter type
 *              < pcad->vald    double  freq1
 *              < pcad->vale    double  freq2
 *              < pcad->valf    double  weightA
 *              < pcad->valg    double  weightB
 *              < pcad->valh    double  weightC
 *              < pcad->vali    long    reset
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
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 24-Jul-1996: Original(srp)
 * 28-May-1997: design filter internally, provide string of filter status to ouputs
 * 08-Jun-1997: Increase number of sources to six, change init string from "NOT DEFINED" to blank
 * 02-Oct-1997: Number of sources now fixed at 5. Clarify difference between no filtering and not
 *              using that source at all.
 * 06-Jan-1998: increase length of filterString from 50 to 81 characters
 * 23-Jun-1998: Write to mess field following param conversion failure
 * 19-Oct-1998: No need to check filter cutoffs for NOTUSED and RAW selections
 *
 */

/* INDENT ON */
/* ===================================================================== */

long CADguideConfig (struct cadRecord * pcad)
{
     long status = CAD_REJECT;
     static int source;
     static int filterType, reset;

     /* Lovely: hard-coded numbers! */
     static double sampleFreq = 200.0, freq1 = 20.0, freq2 = 25.0 ;

     static double weightA, weightB, weightC;
#ifdef MK
     static char *sourceOpts[] = {"PWFS1", "PWFS2", "OIWFS", "GAOS", "GYRO",
#else
     static char *sourceOpts[] = {"PWFS1", "PWFS2", "OIWFS", "GAOS", "GYRO", "GPI",
#endif
                                  NULL } ;
     static char *filterOpts[] = {"OFF", "RAW", "LOWPASS", "HIGHPASS",
                                  "BANDPASS", "BANDSTOP", NULL} ;

     cadDirLog ("guideConfig", pcad->dir, 10, pcad);

     /* Set message prefix */
     tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char *) NULL);

     switch (pcad->dir)
     {
     case menuDirectiveMARK:
          status = CAD_ACCEPT ;
          break;

     case menuDirectiveCLEAR:
          status = CAD_ACCEPT ;
          break;

     case menuDirectivePRESET:

          /* convert the input strings to numbers for checking */

          if (tcsDcString (sourceOpts, "source - ", pcad->a, &source, pcad))
          {
               errorLog ("CADguideConfig - source out of range", 2, ON);
               break ;
          }
          else if (source < PWFS1 || source > GYRO)
          {
               errorLog ("CADguideConfig - source out of range", 2, ON);
               strncpy (pcad->mess, "source out of range", MAX_STRING_SIZE - 1);
               break;
          }

          if (tcsDcDouble ("sample freq: ", pcad->b, &sampleFreq, pcad))
          {
               errorLog ("CADguideConfig - failed sampleFreq conversion", 2, ON);
               break ;
          }
          else if (sampleFreq < 0.1 || sampleFreq > 200.0)
          {
               errorLog ("CADguideConfig - sample freq out of range", 2, ON);
               tcsCsAppendMessage (pcad, "sample freq. out of range") ;
               break;
          }

          if (tcsDcString (filterOpts, "filter type - ", pcad->c, &filterType, pcad)) {
               errorLog ("CADguideConfig - failed filterType conversion", 2, ON);
               break ;
          }

          if (filterType < NOTUSED || filterType > BANDSTOP)
          {
               errorLog ("CADguideConfig - filter type out of range", 2, ON);
               tcsCsAppendMessage (pcad, "filter type out of range") ;
               break;
          }

          /* Only check the filter edge parameters if the filter type is appropriate */

          if (filterType > RAW) {

               if (tcsDcDouble ("filter edge 1: ", pcad->d, &freq1, pcad)) {
                    errorLog ("CADguideConfig - failed freq1 conversion", 2, ON);
                    break ;
               }
               if (freq1 < 0.1 || freq1 > sampleFreq/2.0) {
                    errorLog ("CADguideConfig - freq1 out of range", 2, ON);
                    tcsCsAppendMessage (pcad, "lower edge freq. out of range") ;
                    break ;
               }

               if (tcsDcDouble ("filter edge 2: ", pcad->e, &freq2, pcad)) {
                    errorLog ("CADguideConfig - failed freq2 conversion", 2, ON);
                    break ;
               }

               if (freq2 < freq1 || freq2 > sampleFreq/2.0) {
                    tcsCsAppendMessage (pcad, "upper edge freq. out of range") ;
                    break ;
               }

          }

          if (tcsDcDouble ("Beam A weight: ", pcad->g, &weightA, pcad)) {
               errorLog ("CADguideConfig - failed weight A conversion", 2, ON);
               break ;
          }

          if (weightA < LOW_WEIGHT_LIMIT || weightA > HIGH_WEIGHT_LIMIT)
          {
               errorLog ("CADguideConfig - weightA out of range", 2, ON);
               tcsCsAppendMessage (pcad, "weight A out of range") ;
               break;
          }

          if (tcsDcDouble ("Beam B weight: ", pcad->h, &weightB, pcad)) {
               errorLog ("CADguideConfig - failed weight B conversion", 2, ON);
               break ;
          }

          if (weightB < LOW_WEIGHT_LIMIT || weightB > HIGH_WEIGHT_LIMIT)
          {
               errorLog ("CADguideConfig - weightB out of range", 2, ON);
               tcsCsAppendMessage (pcad, "weight B out of range") ;
               break;
          }

          if (tcsDcDouble ("Beam C weight: ", pcad->i, &weightC, pcad)) {
               errorLog ("CADguideConfig - failed weight C conversion", 2, ON);
               break ;
          }

          if (weightC < LOW_WEIGHT_LIMIT || weightC > HIGH_WEIGHT_LIMIT)
          {
               errorLog ("CADguideConfig - weightC out of range", 2, ON);
               tcsCsAppendMessage (pcad, "weight C out of range") ;
               break;
          }

          status = CAD_ACCEPT ;

          /* if all parameters are within limits, copy to outputs */

          *(long *)pcad->vala     = source;
          *(double *)pcad->valb   = sampleFreq;
          *(long *)pcad->valc     = filterType;
          *(double *)pcad->vald   = freq1;
          *(double *)pcad->vale   = freq2;
          *(double *)pcad->valf   = weightA;
          *(double *)pcad->valg   = weightB;
          *(double *)pcad->valh   = weightC;
          *(long *)pcad->vali     = reset;

          break;

     case menuDirectiveSTART:

          if (interlockFlag == ON)
          {
               strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
               break;
          }
          status = CAD_ACCEPT ;
          break;

     case menuDirectiveSTOP:
          status = CAD_ACCEPT ;
          break;

     default:
          strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
          status = CAD_REJECT;
          break;
     }

     return (status);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * guideConfig
 *
 * Purpose:
 * Configure the filtering and weighting to be applied to the
 * specified guide sensor source. Source and filter specification
 * parameters are received from the CAD record outputs.
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = guideConfig(pgsub)
 *
 * Parameters in:
 *              > pcad->a       long    sensor source
 *              > pcad->b       double  sample frequency
 *              > pcad->c       long    filter type
 *              > pcad->d       double  edge frequency 1
 *              > pcad->e       double  edge frequency 2
 *              > pcad->f       double  combination weight beam A
 *              > pcad->g       double  combination weight beam B
 *              > pcad->h       double  combination weight beam C
 *              > pcad->i       long    reset all to none
 *
 * Parameters out:
 *              < pcad->vala    *string pwfs1 configuration
 *              < pcad->valb    *string pwfs2 configuration
 *              < pcad->valc    *string oiwfs configuration
 *              < pcad->vald    *string gaos configuration
 *              < pcad->vale    *string gyro configuration
 *              < pcad->valf    *string gpi configuration (GS only)
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
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 24-Jul-1996: Original(srp)
 *
 */

/* INDENT ON */
/* ===================================================================== */

long guideConfig (struct genSubRecord *pgsub)
{
     int i, j;
     double maxFreq[MAX_BEAMS];
     static int source;
     int source2;
     static int filterType, reset;
     static double sampleFreq, freq1, freq2, weightA, weightB, weightC;
     char filterString[MAX_STRING_SIZE];

     /* copy inputs to local variables */

     source = *(long *)pgsub->a;
     sampleFreq = *(double *)pgsub->b;
     filterType = *(long *)pgsub->c;
     freq1 = *(double *)pgsub->d;
     freq2 = *(double *)pgsub->e;
     weightA = *(double *)pgsub->f;
     weightB = *(double *)pgsub->g;
     weightC = *(double *)pgsub->h;
     reset = *(long *)pgsub->i;

     /* create the specified filter */

     clearFilters (source);  /* remove history of any previous configuration */

     createFilter (source, filterType, sampleFreq, freq1, freq2, weightA, weightB, weightC);

     /* write the filter details to the engineering screen */

     switch (filterType)
     {
     case NOTUSED:
          sprintf (filterString, "%.8s", filterName[filterType]);
          break;

     case RAW:
          sprintf (filterString, "%.8s %s", filterName[filterType], weight2string (weightA, weightB, weightC));
          break;
     default:
          sprintf (filterString, "%.8s %3.0f %2.0f %2.0f %s", filterName[filterType], sampleFreq, freq1, freq2, weight2string
                   (weightA, weightB, weightC));

     }

     switch (source)
     {
     case PWFS1:
          strncpy (pgsub->vala, filterString, MAX_STRING_SIZE - 1);
          break;

     case PWFS2:
          strncpy (pgsub->valb, filterString, MAX_STRING_SIZE - 1);
          break;

     case OIWFS:
          strncpy (pgsub->valc, filterString, MAX_STRING_SIZE - 1);
          break;

     case GAOS:
          strncpy (pgsub->vald, filterString, MAX_STRING_SIZE - 1);
          break;

     case GYRO:
          strncpy (pgsub->vale, filterString, MAX_STRING_SIZE - 1);
          break;

#ifndef MK
     case GPI:
          strncpy (pgsub->valf, filterString, MAX_STRING_SIZE - 1);
          break;
#endif


     default:
          break;
     }

     /* if overall reset has been chosen, reset all guide filtering */

     if (reset != 0)
     {
          for (i = PWFS1; i <= GYRO; i++)
          {
               clearFilters (i);
               createFilter (i, OFF, 200.0, 20.0, 20.0, -1.0, -2.0, -2.0);
          }

          strncpy (filterString, "OFF", MAX_STRING_SIZE - 1);

          strncpy (pgsub->vala, filterString, MAX_STRING_SIZE - 1);
          strncpy (pgsub->valb, filterString, MAX_STRING_SIZE - 1);
          strncpy (pgsub->valc, filterString, MAX_STRING_SIZE - 1);
          strncpy (pgsub->vald, filterString, MAX_STRING_SIZE - 1);
          strncpy (pgsub->vale, filterString, MAX_STRING_SIZE - 1);
#ifndef MK
          strncpy (pgsub->valf, filterString, MAX_STRING_SIZE - 1);
#endif

     }

     /* check through resulting configuration to verify which beams guiding applies to */

     /* for each beam find also the fastest updating guide source to use as the guide tick source */

     guideOnA = OFF;
     guideOnB = OFF;
     guideOnC = OFF;

     maxFreq[BEAMA] = 0;
     maxFreq[BEAMB] = 0;
     maxFreq[BEAMC] = 0;

     for (j = PWFS1; j <= GYRO; j++)
     {
          /* check index in range */

          if (j < 0 || j > (MAX_SOURCES - 1))
          {
               errlogMessage("guideMaster index out of range\n");
          }
          else
          {
               guideMaster[j][BEAMA] = OFF;
               guideMaster[j][BEAMB] = OFF;
               guideMaster[j][BEAMC] = OFF;
               guideMaster[j][A2BRAMP] = OFF;
               guideMaster[j][B2ARAMP] = OFF;
          }
     }

     for (source = PWFS1; source <= GYRO; source++)
     {
          /* check index in range */

          if (source < 0 || source > (MAX_SOURCES - 1))
          {
               errlogMessage("guideMaster index out of range\n");
          }
          else
          {
               if (weight[source][BEAMA] >= -1)
               {
                    guideOnA = ON;

                    if (filter[source][XTILT].sampleFreq > maxFreq[BEAMA])
                    {
                         /* this source is the fastest so far */

                         maxFreq[BEAMA] = filter[source][XTILT].sampleFreq;

                         for (source2 = PWFS1; source2 <= GYRO; source2++)
                         {
                              /* check index in range */

                              if (source2 < 0 || source2 > (MAX_SOURCES - 1))
                              {
                                   errlogMessage("guideMaster index out of range\n");
                              }
                              else
                              {
                                   /* set all to OFF */

                                   guideMaster[source2][BEAMA] = OFF;
                                   guideMaster[source2][A2BRAMP] = OFF;
                              }
                         }

                         /* set this source to ON */

                         guideMaster[source][BEAMA] = ON;
                         guideMaster[source][A2BRAMP] = ON;
                    }
               }

               if (weight[source][BEAMB] >= -1)
               {
                    guideOnB = ON;

                    if (filter[source][XTILT].sampleFreq > maxFreq[BEAMB])
                    {
                         maxFreq[BEAMB] = filter[source][XTILT].sampleFreq;

                         for (source2 = PWFS1; source2 <= GYRO; source2++)
                         {
                              /* check index in range */

                              if (source2 < 0 || source2 > (MAX_SOURCES - 1))
                              {
                                   errlogMessage("guideMaster index out of range\n");
                              }
                              else
                              {
                                   /* set all to OFF */

                                   guideMaster[source2][BEAMB] = OFF;
                                   guideMaster[source2][B2ARAMP] = OFF;
                              }
                         }

                         guideMaster[source][BEAMB] = ON;
                         guideMaster[source][B2ARAMP] = ON;
                    }
               }

               if (weight[source][BEAMC] >= -1)
               {
                    guideOnC = ON;

                    if (filter[source][XTILT].sampleFreq > maxFreq[BEAMC])
                    {
                         maxFreq[BEAMC] = filter[source][XTILT].sampleFreq;

                         for (source2 = PWFS1; source2 <= GYRO; source2++)
                         {
                              /* check index in range */

                              if (source2 < 0 || source2 > (MAX_SOURCES - 1))
                              {
                                   errlogMessage("guideMaster index out of range\n");
                              }
                              else
                              {
                                   /* set all to OFF */

                                   guideMaster[source2][BEAMC] = OFF;
                              }
                         }

                         guideMaster[source][BEAMC] = ON;
                    }
               }
          }
     }

     /* set reset widget back to NULL */

     *(long *)pgsub->valj = 0;

     /* write CAR_IDLE to port J to mark completion */

     *(long *)pgsub->valj = CAR_IDLE;

     return(OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADguideReset
 *
 * Purpose:
 * Limit check the guide configuration parameters on PRESET. If acceptable
 * copy to the output ports.
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADguideReset(pcad)
 *
 * Parameters in:
 *              > pcad->dir     *string CAD directive
 *              > pcad->a       *string sensor source
 *              > pcad->b       *string reset all to none
 *
 * Parameters out:
 *              < pcd->mess     *string status message
 *              < pcad->vala    long    sensor source
 *              < pcad->valb    long    reset
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
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 24-Jul-1996: Original(srp)
 *
 */

/* INDENT ON */
/* ===================================================================== */

long CADguideReset (struct cadRecord * pcad)
{
     long status = CAD_REJECT;
     static int reset;

#ifdef MK
     // static double weightA, weightB, weightC;
#endif

    static char *resetOpts[] = {"OFF", "ON", NULL} ;


     printf("CADguideReset: BEGIN \n");

     cadDirLog ("guideConfigReset", pcad->dir, 1, pcad);

     /* Set message prefix */
     tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char *) NULL);

     printf("CADguideReset: pcad->dir %d  \n", pcad->dir);

     switch (pcad->dir)
     {
     case menuDirectiveMARK:
	  printf("CADguideReset: MARK \n");
          status = CAD_ACCEPT ;
          break;

     case menuDirectiveCLEAR:
	  printf("CADguideReset: CLEAR \n");
          status = CAD_ACCEPT ;
          break;

     case menuDirectivePRESET:
	  printf("CADguideReset: PRESET reset %d \n",reset);


          if (tcsDcString (resetOpts, "reset -  ", pcad->a, &reset, pcad)){
               errorLog ("CADguideConfig - failed reset conversion", 2, ON);
               break ;
          }

	  printf("CADguideReset: PRESET reset %d \n",reset);

          status = CAD_ACCEPT ;

          *(long *)pcad->vala     = reset;

          break;

     case menuDirectiveSTART:
	  printf("CADguideReset: START \n");

          if (interlockFlag == ON)
          {
               strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
               break;
          }
	  printf("CADguideReset: menuDirectiveSTART: %d \n",menuDirectiveSTART);
          status = CAD_ACCEPT ;
          break;

     case menuDirectiveSTOP:
	  printf("CADguideReset: STOP \n");
          status = CAD_ACCEPT ;
          break;

     default:
	  printf("CADguideReset: default \n");
          strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
          status = CAD_REJECT;
          break;
     }

     return (status);
}


/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * resetGuideConfig
 *
 * Purpose:
 * Configure the filtering and weighting to be applied to the
 * specified guide sensor source. Source and filter specification
 * parameters are received from the CAD record outputs.
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = resetGuideConfig(pgsub)
 *
 * Parameters in:
 *              > pcad->a       long    reset all to none
 *
 * Parameters out:
 *              < pcad->vala    *string pwfs1 configuration
 *              < pcad->valb    *string pwfs2 configuration
 *              < pcad->valc    *string oiwfs configuration
 *              < pcad->vald    *string gaos configuration
 *              < pcad->vale    *string gyro configuration
 *              < pcad->valf    *string gpi configuration (GS only)
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
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 24-Jul-1996: Original(srp)
 *
 */

/* INDENT ON */
/* ===================================================================== */

long resetGuideConfig (struct genSubRecord *pgsub)
{
     int i;
     char filterString[MAX_STRING_SIZE];



          for (i = PWFS1; i <= GYRO; i++)
          {
               clearFilters (i);
               createFilter (i, OFF, 200.0, 20.0, 20.0, -1.0, -2.0, -2.0);
          }

          strncpy (filterString, "OFF", MAX_STRING_SIZE - 1);

          strncpy (pgsub->vala, filterString, MAX_STRING_SIZE - 1);
          strncpy (pgsub->valb, filterString, MAX_STRING_SIZE - 1);
          strncpy (pgsub->valc, filterString, MAX_STRING_SIZE - 1);
          strncpy (pgsub->vald, filterString, MAX_STRING_SIZE - 1);
          strncpy (pgsub->vale, filterString, MAX_STRING_SIZE - 1);
#ifndef MK
          strncpy (pgsub->valf, filterString, MAX_STRING_SIZE - 1);
#endif


     /* set reset widget back to NULL */

     *(long *)pgsub->valj = 0;

     /* write CAR_IDLE to port J to mark completion */

     *(long *)pgsub->valj = CAR_IDLE;

     return(OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * createFilter
 *
 * Purpose:
 * Function uses the desired crossover and filter type to look up the
 * required coefficients for the specified filter.
 *
 * Invocation:
 * status = createFilter(source, filterType, sampleRate, threeDb, weightA, weightB, weightC)
 *
 * Parameters in:
 *              > source        int     guide source for this filter
 *              > filterType    int     one of OFF or RAW or LOWPASS or HIGHPASS or BANDPASS or BANDSTOP
 *              > sampleRate    double  guide source update rate (Hz)
 *              > freq1         double  low cutoff frequency (Hz)
 *              > freq2         double  high cutoff frequency (Hz)
 *              > weightA       double  weighting for beam A in the range 0 to 1
 *              > weightB       double  weighting for beam B in the range 0 to 1
 *              > weightC       double  weighting for beam C in the range 0 to 1
 *
 * Parameters out:
 *
 * Return value:
 *              < status        int     OK or ERROR
 *
 * Globals:
 *      External functions:
 *      None
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
 * 15-Oct-1997: Original(srp)
 * 09-Jan-1998: Add option to set source OFF
 *
 */

/* INDENT ON */
/* ===================================================================== */

int createFilter (int source, int filterType, double sampleRate, double freq1, double freq2, double weightA, double weightB, double
weightC)
{
     double crossover1, crossover2;
     int axis = XTILT;
     MATLAB tempFilter =
     {
          0,
          0,
          {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
          {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
          {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
          {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
          -2.0,
          -2.0,
          -2.0,
          NOTUSED,
          200.0,
          20.0,
          25.0
     };

     if (source < PWFS1 || source > GYRO)
     {
          printf ("createFilter - source out of range\n");
          return (ERROR);
     }

     if (axis < 0 || axis > (MAX_AXES - 1))
     {
          printf ("createFilter - axis out of range\n");
          return (ERROR);
     }

     switch (filterType)
     {
     case NOTUSED:

          *(MATLAB *) & filter[source][0] = *(MATLAB *) & tempFilter;
          *(MATLAB *) & filter[source][1] = *(MATLAB *) & tempFilter;
          *(MATLAB *) & filter[source][2] = *(MATLAB *) & tempFilter;

          weight[source][BEAMA] = -2;
          weight[source][BEAMB] = -2;
          weight[source][BEAMC] = -2;
          weight[source][A2BRAMP] = -2;
          weight[source][B2ARAMP] = -2;

          break;

     case RAW:
     case LOWPASS:
     case HIGHPASS:
     case BANDPASS:
     case BANDSTOP:

          /* calculate normalised crossover frequency */

          crossover1 = 2 * freq1 / sampleRate;
          crossover2 = 2 * freq2 / sampleRate;

          /* select filter nearest to the crossover just calculated */
          /* use a dummy structure in case the configuration is not available */

          if ((readFilters (&tempFilter, filterType, crossover1, crossover2)) != OK)
          {
               errorLog ("createFilter - unable to retrieve specified filter", 1, ON);
               return (ERROR);
          }
          else
          {
               /* copy the filter coeffs read from file to the structure */

               *(MATLAB *) & filter[source][0] = *(MATLAB *) & tempFilter;

               /* fill in the rest of the structure from the calling parameters */

               filter[source][axis].weightA = weightA;
               filter[source][axis].weightB = weightB;
               filter[source][axis].weightC = weightC;
               filter[source][axis].type = filterType;
               filter[source][axis].sampleFreq = sampleRate;
               filter[source][axis].freq1 = freq1;
               filter[source][axis].freq2 = freq2;

               /* now copy the whole structure to the other two axes */

               *(MATLAB *) & filter[source][1] = *(MATLAB *) & filter[source][0];
               *(MATLAB *) & filter[source][2] = *(MATLAB *) & filter[source][0];

               /* also adjust the blending weights */

               weight[source][BEAMA] = weightA;
               weight[source][BEAMB] = weightB;
               weight[source][BEAMC] = weightC;
               weight[source][A2BRAMP] = weightA;
               weight[source][B2ARAMP] = weightB;
          }

          break;

     default:
          errorLog ("createFilter - requested filter type not recognised", 2, ON);
          break;
     }

     return (OK);
}


/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * clearFilters
 *
 * Purpose:
 * Function goes through the history arrays of a specified filter and
 * sets them to zero effectively initialising the filter
 *
 * Invocation:
 * status = clearFilters(source)
 *
 * Parameters in:
 *              > source        int     clear filter for this guide source
 *
 * Parameters out:
 * None
 *
 * Return value:
 *              < status        int     OK or ERROR
 *
 * Globals:
 *      External functions:
 *      None
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
 * 15-Oct-1997: Original(srp)
 *
 */

/* INDENT ON */
/* ===================================================================== */

static int clearFilters (int source)
{
     int axis, i;

     for (axis = 0; axis < MAX_AXES; axis++)
     {
          for (i = 0; i < MAX_HISTORY; i++)
          {
               filter[source][axis].inHistory[i] = 0;
               filter[source][axis].outHistory[i] = 0;

          }
     }
     return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * displayFilter
 *
 * Purpose:
 * Display selected filter coefficients for the specified guide source
 *
 * Invocation:
 * displayFilter(source, axis)
 *
 * Parameters in:
 *              > source        int     guide source index
 *              > axis          int     axis one of XTILT or YTILT or ZFOCUS
 *
 * Parameters out:
 * None
 *
 * Return value:
 * None
 *
 * Globals:
 *      External functions:
 *      None
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
 * 15-Oct-1997: Original(srp)
 *
 */

/* INDENT ON */
/* ===================================================================== */

int displayFilter (const int source, const int axis)
{
     int index;

     if (source < 0 || source > (MAX_SOURCES - 1))
     {
          printf ("valid source range 0 to %d\n", (MAX_SOURCES - 1));
          return (ERROR);
     }

     if (axis < 0 || axis > (MAX_AXES - 1))
     {
          printf ("valid axis range 0 to %d\n", (MAX_AXES - 1));
          return (ERROR);
     }


     /* display filter structure locations */

     printf ("display filter source = %d\n", source);
     printf ("nb at             %p\n", &filter[source][axis].nb);
     printf ("na at             %p\n", &filter[source][axis].na);
     printf ("num at            %p\n", &filter[source][axis].numerator[0]);
     printf ("den at            %p\n", &filter[source][axis].denominator[0]);
     printf ("inHistory ptr at  %p\n", &filter[source][axis].inHistory[0]);
     printf ("outHistory ptr at %p\n", &filter[source][axis].outHistory[0]);

     printf (" axis = %d na = %d nb = %d\n", axis, (int)filter[source][axis].na, (int)filter[source][axis].nb);

     /* display numerator coefficients */
     for (index = 0; index < filter[source][axis].nb; index++)
          printf ("b%d = %11.8f ", index, filter[source][axis].numerator[index]);
     printf ("\n");

     /* display denominator coefficients */
     for (index = 0; index < filter[source][axis].na; index++)
          printf ("a%d = %11.8f ", index, filter[source][axis].denominator[index]);
     printf ("\n");

     /* display in history */
     for (index = 0; index < MAX_HISTORY; index++)
     {
          printf ("in%d = %8.4f ", index, filter[source][axis].inHistory[index]);
     }
     printf ("\n");

     /* display out history */
     for (index = 0; index < MAX_HISTORY; index++)
     {
          printf ("out%d = %8.4f ", index, filter[source][axis].outHistory[index]);
     }
     printf ("\n");

     /* display blend weights */
     printf ("filter weight  A = %f B = %f C = %f\n", filter[source][axis].weightA, filter[source][axis].weightB, filter[source][axis
          ].weightC);
     printf ("combination weights A = %f B = %f C = %f\n", weight[source][BEAMA], weight[source][BEAMB], weight[source][BEAMC]);
     printf ("filter type is %s\n", filterName[filter[source][axis].type]);

     return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * readFilters
 *
 * Purpose:
 * Read the predefined filter coefficient values from file
 *
 * Invocation:
 * status = readFilters(filterAddr, type, freq1, freq2)
 *
 * Parameters in:
 *              > filterAddr    *FILTER         address of filter structure
 *              > type          int             filter type
 *              > freq1         double          low crossover frequency (Hz)
 *              > freq2         double          high crossover frequency (Hz)
 * None
 *
 * Parameters out:
 * None
 *
 * Return value:
 *              < status        int             OK or ERROR
 *
 * Globals:
 *      External functions:
 *      None
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
 * 10-Dec-1997: Original(srp)
 * 03-Mar-1998: Load filter files from /data directory rather than /src
 * 04-Mar-1998; pass address of filter structure rather than pointer with malloc
 *
 */

/* INDENT ON */
/* ===================================================================== */

static int readFilters (MATLAB * testFilter, int type, double freq1, double freq2)
{
     FILE *fptr = NULL;
     char filename[MAX_STRING_SIZE] = "";
     int found1 = FALSE;
     int i, sum1, lim1, blockSize;

     FILTER dummyFilter =
     {
          0.0,
          0.0,
          0,
          0,
          {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
          {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
     };

     /* read coefficients from file */

     switch (type)
     {
          /* open the file corresponding to the filter type        */
          /* for the RAW (no filtering) option use the lowpass     */
          /* higher levels will handle the option                  */

	  /* This looks bad - why should RAW be in here - the whole
	   * point of RAW is to not use any filters.    21nov2000  */

     case RAW:
     case LOWPASS:
          strncpy (filename, LOW_COEFFS, MAX_STRING_SIZE - 1);
          if (freq1 < 0.01 || freq1 > 0.99)
          {
               errorLog ("readFilters - freq out of range", 1, ON);
               return (ERROR);
          }
          freq2 = freq1;
          break;

     case HIGHPASS:
          strncpy (filename, HIGH_COEFFS, MAX_STRING_SIZE - 1);
          if (freq1 < 0.01 || freq1 > 0.99)
          {
               errorLog ("readFilters - freq out of range", 1, ON);
               return (ERROR);
          }
          freq2 = freq1;
          break;

     case BANDPASS:
          strncpy (filename, PASS_COEFFS, MAX_STRING_SIZE - 1);
          if (freq1 < 0.01 || freq1 > 0.8)
          {
               errorLog ("readFilters - freq out of range", 1, ON);
               return (ERROR);
          }
          break;

     case BANDSTOP:
          strncpy (filename, STOP_COEFFS, MAX_STRING_SIZE - 1);
          if (freq1 < 0.01 || freq1 > 0.8)
          {
               errorLog ("readFilters - freq out of range", 1, ON);
               return (ERROR);
          }
          break;

     default:
          errorLog ("readFilters - filter type not recognised", 2, ON);
          return (ERROR);
     }

     if ((fptr = fopen (filename, "rb")) == NULL)
     {
          errorLog ("readFilters - Unable to open filter coefficients file", 1, ON);
     }
     else
     {
          lim1 = (int) (freq1 * 100 - 1);

          if (type < BANDPASS)
          {
               sum1 = lim1;
          }
          else
          {
               /* find file entry corresponding to this limit */

               blockSize = 97;
               sum1 = 0;
               i = 0;

               while (i < lim1)
               {
                    sum1 += blockSize--;
                    i++;
               }
               sum1 += (int) ((freq2 - freq1) * 100 - 1);
          }

          /* read coefficients from file */

          if (fseek (fptr, sum1 * sizeof (FILTER), SEEK_SET) != 1)
          {
               if (fread ((FILTER *) & dummyFilter, sizeof (FILTER), 1, fptr) != 1)
               {
                    errorLog ("readFilters - file coefficients read error", 1, ON);
               }
               else
               {
                    found1 = TRUE;
               }
          }
          else
          {
               printf ("fseek operation failed\n");
          }

          fclose (fptr);
     }

     if (found1 == TRUE)
     {
          /* copy the filter coefficients into the passed filter structure */

          testFilter->na = dummyFilter.na;
          testFilter->nb = dummyFilter.nb;

          if (dummyFilter.na > MAX_HISTORY || dummyFilter.na < 1)
          {
               printf ("retrieved filter denominator has %d elements\n", dummyFilter.na);
               return (ERROR);
          }
          else if (dummyFilter.nb > MAX_HISTORY || dummyFilter.nb < 1)
          {
               printf ("retrieved filter numerator has %d elements\n", dummyFilter.na);
               return (ERROR);
          }

          for (i = 0; i < dummyFilter.nb; i++)
          {
               testFilter->numerator[i] = dummyFilter.Bcoeffs[i];
          }

          for (i = 0; i < dummyFilter.na; i++)
          {
               testFilter->denominator[i] = dummyFilter.Acoeffs[i];
          }

          return (OK);
     }
     else
     {
          printf ("readFilters - unable to retrieve filter\n");
          return (ERROR);
     }
}

#ifdef MK
void printHS() {

    int i;

    printf("xTiltPosHS:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->xTiltPosHS[i]);
    printf("\n");
return;

    printf("yTiltPosHS:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->yTiltPosHS[i]);
    printf("\n");

    printf("zPosHS:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->zPosHS[i]);
    printf("\n");

    printf("xTiltNetGuideHS:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->xTiltNetGuideHS[i]);
    printf("\n");

    printf("yTiltNetGuideHS:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->yTiltNetGuideHS[i]);
    printf("\n");

    printf("zNetGuideHS:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->zNetGuideHS[i]);
    printf("\n");

    printf("vtkXCommand:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->vtkXCommand[i]);
    printf("\n");

    printf("vtkXFrequency:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->vtkXFrequency[i]);
    printf("\n");

    printf("vtkXPhase:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->vtkXPhase[i]);
    printf("\n");


    printf("vtkYCommand:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->vtkYCommand[i]);
    printf("\n");

    printf("vtkYFrequency:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->vtkYFrequency[i]);
    printf("\n");

    printf("vtkYPhase:");
    for (i=0; i<200; i++)
        printf("[%d]=%f ",i, highSpeedData->vtkYPhase[i]);
    printf("\n");


}

/*
 *
 *
 *
 */
long initHighSpeed (struct genSubRecord *pgsub) {

   printf("initHighSpeed\n");
   /*HighSpeed Initialize*/
   if ((highSpeedData = (HighSpeed *) calloc (0, sizeof (*highSpeedData) ) ) == NULL)    
   {
       printf ("calloc fail on creation of highSpeedData buffer\n");
       return (ERROR);
   }

   printf("HighSpeed calloc set to %p\n",highSpeedData);

   printf("initHighSpeed good\n");
  return(OK);
}


long hsSamples = HS_RECORD_LENGTH/100;
/***
 *
 *
 */
long highSpeed (struct genSubRecord *pgsub) {

    static long count = 0;
    static long subcount = HS_RECORD_LENGTH-1;
    Vtk *vtkx, *vtky;

    static float xTiltPosHS[HS_RECORD_LENGTH];
    static float yTiltPosHS[HS_RECORD_LENGTH]; 
    static float zPosHS[HS_RECORD_LENGTH];

    static float xTiltNetGuideHS[HS_RECORD_LENGTH];
    static float yTiltNetGuideHS[HS_RECORD_LENGTH];
    static float zNetGuideHS[HS_RECORD_LENGTH];

    static float vtkXCommand[HS_RECORD_LENGTH];
    static float vtkXFrequency[HS_RECORD_LENGTH];
    static float vtkXPhase[HS_RECORD_LENGTH];  /*New vtkX phase*/

    static float vtkYCommand[HS_RECORD_LENGTH];
    static float vtkYFrequency[HS_RECORD_LENGTH];
    static float vtkYPhase[HS_RECORD_LENGTH]; /*New vtkY phase*/

    static float xRawGuideHS[HS_RECORD_LENGTH];
    static float yRawGuideHS[HS_RECORD_LENGTH];
    static float zRawGuideHS[HS_RECORD_LENGTH]; /*Raw Guides*/

    /* Set some flag for bad status and leave.*/
    /*
     *
    *(long *) pgsub->valt = 0;
    if (highSpeedData == NULL) {
        *(long *) pgsub->valt = 1;
       
        return(ERROR);
    }
    */
    
    /*Output Number of Samples to VALU*/
    *(long *) pgsub->valu = hsSamples;

    vtkx = getVtkX();
    vtky = getVtkY();

    xTiltPosHS[count]        = (float) scsBase->page1.xTilt;
    yTiltPosHS[count]        = (float) scsBase->page1.yTilt; 
    zPosHS[count]            = (float) scsBase->page1.zFocus;

    /*Guide values after PID, SW, and VTK*/
    xTiltNetGuideHS[count]   = (float) xGuideTcs; 
    yTiltNetGuideHS[count]   = (float) yGuideTcs;
    zNetGuideHS[count]       = (float) zGuideTcs;

    vtkXCommand[count]       = (float) vtkx->command;
    vtkXFrequency[count]     = (float) vtkx->frequency.currentValue;
    vtkXPhase[count]         = (float) vtkx->phase;  /*New vtkX phase*/

    vtkYCommand[count]       = (float) vtky->command;
    vtkYFrequency[count]     = (float) vtky->frequency.currentValue;
    vtkYPhase[count]         = (float) vtky->phase; /*New vtkY phase*/

    /*Guide values BEFORE any of the PID, SW, or VTK */
    xRawGuideHS[count]       = (float) scsBase->page0.rawXGuide; 
    yRawGuideHS[count]       = (float) scsBase->page0.rawYGuide; 
    zRawGuideHS[count]       = (float) scsBase->page0.rawZGuide; 

     /*Test HighSpeed Waveform*/
     if (subcount == 0 ) {
        int numOldSamples = HS_RECORD_LENGTH - (count+1);
        int numNewSamples = HS_RECORD_LENGTH - numOldSamples;

        /*Input pointers */
        float *xTiltin = xTiltPosHS;
        float *yTiltin = yTiltPosHS;
        float *zPosin = zPosHS;
        float *xTiltNetGuidein = xTiltNetGuideHS;
        float *yTiltNetGuidein = yTiltNetGuideHS;
        float *zNetGuidein = zNetGuideHS;
        float *vtkXCommandin = vtkXCommand;
        float *vtkXFrequencyin = vtkXFrequency;
        float *vtkXPhasein = vtkXPhase;
        float *vtkYCommandin = vtkYCommand;
        float *vtkYFrequencyin = vtkYFrequency;
        float *vtkYPhasein = vtkYPhase;
        float *xRawGuidein = xRawGuideHS;
        float *yRawGuidein = yRawGuideHS;
        float *zRawGuidein = zRawGuideHS;

        /*Output pointers */
        float *xTiltPosOut         = (float *)pgsub->vala;
        float *yTiltPosOut         = (float *)pgsub->valb;
        float *zPosOut             = (float *)pgsub->valc;
        float *xTiltNetGuideOut    = (float *)pgsub->vald;
        float *yTiltNetGuideOut    = (float *)pgsub->vale;
        float *zNetGuideOut        = (float *)pgsub->valf;
        float *vtkXCommandOut      = (float *)pgsub->valg;
        float *vtkXFrequencyOut    = (float *)pgsub->valh;
        float *vtkXPhaseOut        = (float *)pgsub->vali;
        float *vtkYCommandOut      = (float *)pgsub->valj;
        float *vtkYFrequencyOut    = (float *)pgsub->valk;
        float *vtkYPhaseOut        = (float *)pgsub->vall;
        float *xRawGuideOut        = (float *)pgsub->valm;
        float *yRawGuideOut        = (float *)pgsub->valn;
        float *zRawGuideOut        = (float *)pgsub->valo;

        memcpy (xTiltPosOut, (float *)(xTiltin+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(xTiltPosOut+numOldSamples), (float *)(xTiltin),  numNewSamples*sizeof (float));

        memcpy (yTiltPosOut, (float *)(yTiltin+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(yTiltPosOut+numOldSamples), (float *)yTiltin,  numNewSamples*sizeof (float));

        memcpy (zPosOut, (float *)(zPosin+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(zPosOut+numOldSamples), (float *)zPosin,  numNewSamples*sizeof (float));

        memcpy (xTiltNetGuideOut, (float *)(xTiltNetGuidein+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(xTiltNetGuideOut+numOldSamples), (float *)xTiltNetGuidein,  numNewSamples*sizeof (float));

        memcpy (yTiltNetGuideOut, (float *)(yTiltNetGuidein+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(yTiltNetGuideOut+numOldSamples), (float *)yTiltNetGuidein,  numNewSamples*sizeof (float));

        memcpy (zNetGuideOut, (float *)(zNetGuidein+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(zNetGuideOut+numOldSamples), (float *)zNetGuidein,  numNewSamples*sizeof (float));

        memcpy (vtkXCommandOut, (float *)(vtkXCommandin+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(vtkXCommandOut+numOldSamples), (float *)vtkXCommandin,  numNewSamples*sizeof (float));

        memcpy (vtkXFrequencyOut, (float *)(vtkXFrequencyin+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(vtkXFrequencyOut+numOldSamples), (float *)vtkXFrequencyin,  numNewSamples*sizeof (float));

        memcpy (vtkXPhaseOut, (float *)(vtkXPhasein+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(vtkXPhaseOut+numOldSamples), (float *)vtkXPhasein,  numNewSamples*sizeof (float));

        memcpy (vtkYCommandOut, (float *)(vtkYCommandin+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(vtkYCommandOut+numOldSamples), (float *)vtkYCommandin,  numNewSamples*sizeof (float));

        memcpy (vtkYFrequencyOut, (float *)(vtkYFrequencyin+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(vtkYFrequencyOut+numOldSamples), (float *)vtkYFrequencyin,  numNewSamples*sizeof (float));

        memcpy (vtkYPhaseOut, (float *)(vtkYPhasein+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(vtkYPhaseOut+numOldSamples), (float *)vtkYPhasein,  numNewSamples*sizeof (float));

        memcpy (xRawGuideOut, (float *)(xRawGuidein+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(xRawGuideOut+numOldSamples), (float *)xRawGuidein,  numNewSamples*sizeof (float));

        memcpy (yRawGuideOut, (float *)(yRawGuidein+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(yRawGuideOut+numOldSamples), (float *)yRawGuidein,  numNewSamples*sizeof (float));

        memcpy (zRawGuideOut, (float *)(zRawGuidein+count+1),  numOldSamples*sizeof (float));
        memcpy ((float *)(zRawGuideOut+numOldSamples), (float *)zRawGuidein,  numNewSamples*sizeof (float));

        /*
         *
         * memcpy ((double *)pgsub->vall, vtkYPhase,  numsamples*sizeof (double));
         *
         */
        subcount = hsSamples-1;

     } else {
        
         subcount--;
     }
     if (count == HS_RECORD_LENGTH-1)
         count=0;
     else
         count++;

     return (OK);
}
#endif

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * decimate
 *
 * Purpose:
 * Filter each of the input channels and filter to allow decimation from
 * 200Hz to 20Hz for the TCS to read.
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = decimate(struct genSubRecord *pgsub)
 *
 * Parameters in:
 *              None
 *
 * Parameters out:
 *              < pgsub->vala   double  x tilt position
 *              < pgsub->valb   double  y tilt position
 *              < pgsub->valc   double  z focus position
 *              < pgsub->vald   double  filtered x tilt guide
 *              < pgsub->vale   double  filtered y tilt guide
 *              < pgsub->valf   double  filtered z focus guide
 *              < pgsub->valg   double  x net tilt demand
 *              < pgsub->valh   double  y net tilt demand
 *              < pgsub->vali   double  z focus net demand
 *              < pgsub->valj   double  x tilt error
 *              < pgsub->valk   double  y tilt error
 *              < pgsub->vall   double  z focus error
 *              < pgsub->valm   double  x position demand
 *              < pgsub->valn   double  y position demand
 *              < pgsub->valo   double  x position
 *              < pgsub->valp   double  y position
 *              < pgsub->valq   double  x position error
 *              < pgsub->valr   double  y position error
 *
 * Return value:
 *              < status        long    OK or ERROR
 *
 * Globals:
 *      External functions:
 *      dfilter
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
 * 18-Dec-1997: Original(srp)
 * 19-Feb-1999: Modify decimator code to supply TCS with guide values before
 *              application of m2 PID algorithm
 * 02-Mar-1999: Previous mod was incorrect. The TCS needs the output of the PID
 *              algorithm. Also, only the guide values are read by the TCS so there
 *              is no need to filter the other values - this reduces processor load.
 * 09-Dec-1999: This code uses a different structure than "location" to 
 *              define the positions. This it too bad because it means 
 *              that the utility functions tcs2m2 and m22tcs can't be used
 *              directly. Poor coding. Leave for now, but would be better
 *              to make "location" struct be the one and only struct and
 *              then always used the utility conversion routines.
 *
 *              Multiply xDmd and xPos by -1. in the conversions, as was
 *              done in tcs2m2 and m22tcs functions.
 */

/* INDENT ON */
/* ===================================================================== */

long initDecimate (struct genSubRecord * pgsub)
{
     MATLAB tempFilter =
     {
          0,
          0,
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          0,
          0,
          0,
          200.0,
          20.0,
          20.0
     };

     int index;

     /* look up filter coefficients from file */

     if ((readFilters (&tempFilter, LOWPASS, DECIM_CUTOFF, DECIM_CUTOFF)) != OK)
     {
          errorLog ("initDecimate - unable to retrieve decimation filter", 1, ON);
          return (ERROR);
     }
     else
     {
          for (index = 0; index < NUM_DECIMATORS; index++)
          {
               *(MATLAB *) & decFilter[index] = *(MATLAB *) & tempFilter;
          }
     }
     return (OK);
}


long decimate (struct genSubRecord * pgsub)
{
     struct
     {
          double xTiltPos;
          double yTiltPos;
          double zPos;
          double xNetTiltDmd;
          double yNetTiltDmd;
          double zNetDmd;
          double xTiltGuide;
          double yTiltGuide;
          double zGuide;
          double xTiltErr;
          double yTiltErr;
          double zErr;
          double xDmd;
          double yDmd;
          double xPos;
          double yPos;

     }   tcsData, position1;
     double xp;
     double yp;

#ifdef MK
     double *vtkxdata = (double *) pgsub->valt;
     double *vtkydata = (double *) pgsub->valu;
#endif

     if(simLevel != 0)
     {
          /* simulation active */

          epicsMutexLock(m2MemFree);

          tcsData.xTiltPos = (double) m2Ptr->page1.xTilt;
          tcsData.yTiltPos = (double) m2Ptr->page1.yTilt;
          tcsData.zPos = (double) m2Ptr->page1.zFocus;

          tcsData.xTiltGuide = (double)xGuideTcs;
          tcsData.yTiltGuide = (double)yGuideTcs;
          tcsData.zGuide = (double)zGuideTcs;

          tcsData.xDmd = (double) m2Ptr->page0.xDemand;
          tcsData.yDmd = (double) m2Ptr->page0.yDemand;

          tcsData.xPos = (double) m2Ptr->page1.xPosition;
          tcsData.yPos = (double) m2Ptr->page1.yPosition;

          epicsMutexUnlock(m2MemFree);
     }
     else
     {
          /* no simulation */

          /* grab data from reflective memory */
          /* WHY NOT scsPtr? */
          tcsData.xTiltPos = (double) scsBase->page1.xTilt;
          tcsData.yTiltPos = (double) scsBase->page1.yTilt;
          tcsData.zPos = (double) scsBase->page1.zFocus;

          tcsData.xTiltGuide = (double)xGuideTcs;
          tcsData.yTiltGuide = (double)yGuideTcs;
          tcsData.zGuide = (double)zGuideTcs;

          tcsData.xDmd = (double) scsBase->page0.xDemand;
          tcsData.yDmd = (double) scsBase->page0.yDemand;

          tcsData.xPos = (double) scsBase->page1.xPosition;
          tcsData.yPos = (double) scsBase->page1.yPosition;

     }

     epicsMutexLock(setPointFree);

     switch (currentBeam)
     {
         case BEAMB:
         case B2ARAMP:
              tcsData.xNetTiltDmd = (double) setPoint.xTiltB;
              tcsData.yNetTiltDmd = (double) setPoint.yTiltB;
              break;

         case BEAMC:
              tcsData.xNetTiltDmd = (double) setPoint.xTiltC;
              tcsData.yNetTiltDmd = (double) setPoint.yTiltC;
              break;

         default:
              tcsData.xNetTiltDmd = (double) setPoint.xTiltA;
              tcsData.yNetTiltDmd = (double) setPoint.yTiltA;
     }

     tcsData.zNetDmd = (double) setPoint.zFocus;

     epicsMutexUnlock(setPointFree);


     /* convert current position readings from m2 to tcs frame of reference */

     position1.xTiltPos = frame.tiltCosTheta * 
         (tcsData.xTiltPos - frame.tiltOffsetX) +
         frame.tiltSinTheta * (tcsData.yTiltPos - frame.tiltOffsetY);
     position1.yTiltPos = -frame.tiltSinTheta * 
         (tcsData.xTiltPos - frame.tiltOffsetX) +
         frame.tiltCosTheta * (tcsData.yTiltPos - frame.tiltOffsetY);
     position1.zPos = tcsData.zPos;

     /* write to output ports */

     *(double *) pgsub->vala = position1.xTiltPos;
     *(double *) pgsub->valb = position1.yTiltPos;
     *(double *) pgsub->valc = position1.zPos;

     /* convert guide corrections from m2 to tcs frame of reference */

     position1.xTiltGuide = frame.tiltCosTheta * 
         (tcsData.xTiltGuide - frame.tiltOffsetX) +
         frame.tiltSinTheta * (tcsData.yTiltGuide - frame.tiltOffsetY);
     position1.yTiltGuide = -frame.tiltSinTheta * 
         (tcsData.xTiltGuide - frame.tiltOffsetX) +
         frame.tiltCosTheta * (tcsData.yTiltGuide - frame.tiltOffsetY);
     position1.zGuide = tcsData.zGuide;

     /* filter guide values and write to output ports */

     *(double *) pgsub->vald = dfilter(position1.xTiltGuide, 0);
     *(double *) pgsub->vale = dfilter(position1.yTiltGuide, 1);
     *(double *) pgsub->valf = dfilter(position1.zGuide, 2);

     /* convert current demands from m2 to tcs frame of reference */

     position1.xNetTiltDmd = frame.tiltCosTheta * 
         (tcsData.xNetTiltDmd - frame.tiltOffsetX) +
         frame.tiltSinTheta * (tcsData.yNetTiltDmd - frame.tiltOffsetY);
     position1.yNetTiltDmd = -frame.tiltSinTheta * 
         (tcsData.xNetTiltDmd - frame.tiltOffsetX) +
         frame.tiltCosTheta * (tcsData.yNetTiltDmd - frame.tiltOffsetY);
     position1.zNetDmd = tcsData.zNetDmd;

     /* write to output ports */

     *(double *) pgsub->valg = position1.xNetTiltDmd;
     *(double *) pgsub->valh = position1.yNetTiltDmd;
     *(double *) pgsub->vali = position1.zNetDmd;

     /* calculate position error from filtered data items */

     tcsData.xTiltErr = position1.xNetTiltDmd + 
         position1.xTiltGuide - position1.xTiltPos;
     tcsData.yTiltErr = position1.yNetTiltDmd + 
         position1.yTiltGuide - position1.yTiltPos;
     if (guideOn)
        tcsData.zErr = position1.zNetDmd + position1.zGuide - position1.zPos; 
     else
        tcsData.zErr = position1.zNetDmd - position1.zPos; 
#if 0
     if (debugLevel == DEBUG_RESERVED2)
        printf("zErr: %f scs.zNetDmd %f zPos: %f \n", 	
	   	tcsData.zErr, position1.zNetDmd, position1.zPos);
#endif

     *(double *) pgsub->valj = tcsData.xTiltErr;
     *(double *) pgsub->valk = tcsData.yTiltErr;
     *(double *) pgsub->vall = tcsData.zErr;
    
     /*
      * convert translation mechanism demands and positions to tcs coordinate
      * system
      */

     position1.xDmd = -(frame.posCosTheta * 
         (tcsData.xDmd - frame.posOffsetX) + 
     frame.posSinTheta * (tcsData.yDmd - frame.posOffsetY));
     position1.yDmd = -frame.posSinTheta * 
         (tcsData.xDmd - frame.posOffsetX) + 
     frame.posCosTheta * (tcsData.yDmd - frame.posOffsetY);

     position1.xPos = -(frame.posCosTheta * 
         (tcsData.xPos - frame.posOffsetX) + 
     frame.posSinTheta * (tcsData.yPos - frame.posOffsetY));
     position1.yPos = -frame.posSinTheta * 
         (tcsData.xPos - frame.posOffsetX) + 
     frame.posCosTheta * (tcsData.yPos - frame.posOffsetY);

     *(double *) pgsub->valm = position1.xDmd;
     *(double *) pgsub->valn = position1.yDmd;
     *(double *) pgsub->valo = position1.xPos;
     *(double *) pgsub->valp = position1.yPos;

/*      *(double *) pgsub->valq = position1.xDmd - position1.xPos;
     *(double *) pgsub->valr = position1.yDmd - position1.yPos; */

#if 0
     if (debugLevel == DEBUG_RESERVED2)
        printf("xdem %f ydem %f xpos %f ypos %f \n",scsBase->page0.xDemand, 
	scsBase->page1.xPosition, scsBase->page0.yDemand, 
						scsBase->page1.yPosition);
#endif

     xp = scsBase->page0.xDemand - scsBase->page1.xPosition;
     yp = scsBase->page0.yDemand - scsBase->page1.yPosition;

     *(double *) pgsub->valq = xp; 
     *(double *) pgsub->valr = yp;

#ifdef MK
     *(long *)   pgsub->vals = guideInfo.rate; /*Guide frequency 200Hz, 100Hz or 50Hz*/

     memcpy (vtkxdata, guideInfo.vtkXdata, 3*sizeof (double));
     memcpy (vtkydata, guideInfo.vtkYdata, 3*sizeof (double));
#endif

     return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * lookupGuide
 * lookupGuideInit
 *
 * Purpose:
 * Store the current guide selections for each guide source to allow the
 * widgets to be updated
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = lookupGuide(struct genSubRecord *pgsub)
 *
 * Parameters in:
 *              > pgsub->a      long    current source selected
 *
 * Parameters out:
 *              < pgsub->vala   double  sample frequency
 *              < pgsub->valb   double  break frequency 1
 *              < pgsub->valc   double  break frequency 2
 *              < pgsub->vald   double  filter type
 *              < pgsub->vale   double  std err 1
 *              < pgsub->valf   double  std err 2
 *              < pgsub->valg   double  std err 3
 *
 * Return value:
 *              < status        long    OK or ERROR
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
 * 22-Jan_1998: Original(srp)
 */

/* INDENT ON */
/* ===================================================================== */

/*
long lookupGuideInit (struct genSubRecord * pgsub)
{
    return (OK);
}
*/

long lookupGuide (struct genSubRecord * pgsub)
{
     long source;

     /* read current source from port A */

     source = *(long *) pgsub->a;

     if (source < PWFS1 || source > GYRO)
     {
          errorLog ("lookupGuide - source out of range", 2, ON);
          return (ERROR);
     }
     else
     {
          sprintf ((char *) pgsub->vala, "%6.2f", filter[source][XTILT].sampleFreq);
          sprintf ((char *) pgsub->valb, "%6.2f", filter[source][XTILT].freq1);
          sprintf ((char *) pgsub->valc, "%6.2f", filter[source][XTILT].freq2);
          sprintf ((char *) pgsub->vald, "%d", filter[source][XTILT].type);
          sprintf ((char *) pgsub->vale, "%6.2f", filter[source][XTILT].weightA);
          sprintf ((char *) pgsub->valf, "%6.2f", filter[source][XTILT].weightB);
          sprintf ((char *) pgsub->valg, "%6.2f", filter[source][XTILT].weightC);
     }

     return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * dfilter
 *
 * INVOCATION:
 * double newSample
 * int Id
 *
 * double       dfilter(double newSample, int Id)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > double newSample       - latest data sample
 * > int    iD              - identification of item to filter to keep
 *                            sample histories separate. Initially only use
 *                            to filter the xTilt, yTilt and zGuide signals
 *                            in the decimate function. Id taken from the scs_const.h
 *                            file XTILT = 0, YTILT = 1, FOCUS = 2
 *
 * FUNCTION VALUE:
 * double     returns current filtered value or 0.0 if error encountered
 *
 * PURPOSE:
 * Filter the data in accordance with the IIR filter coefficients specified
 *
 * DESCRIPTION:
 * The function performs a two pole low pass butterworth filter on the supplied
 * data. A history array is maintained for each bank identified by the index Id.
 * The cutoff frequency is approximately 0.05*Fsample hence for a 200Hz update
 * the spectral content above 10Hz is substantially attenuated suitable for
 * sub-sampling at 20Hz.
 *
 * EXTERNAL VARIABLES:
 *
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 *
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version          Sean Prior
 * 22-Jan-1999  Imported from Gemini A&G, add array index check and return 0.0 for error (srp)
 *-
 */

static double dfilter(double newSample, int Id)
{
     int i = 0;
     double sum = 0.;
     static double sample[6][MAX_FILTER_CHANNELS];

     static double coeffs[] = {
          1.0,
          1.77863177782458,
          -0.80080264666571,
          0.00554271721028,
          0.01108543442056,
          0.00554271721028
     };

     /* check filter Id in range */

     if(Id < 0 || Id > (MAX_FILTER_CHANNELS - 1))
     {
          errlogPrintf("dfilter - item Id [%d] out of range\n", (int)Id);
          return(0.0);
     }

     /* put new sample into the array */

     sample[3][Id] = newSample;

     /* multiply samples by coefficients and accumulate */

     for(i=1; i < 6; i++)
          sum += sample[i][Id]*coeffs[i];

     /* ripple samples ready for next call */

     sample[5][Id] = sample[4][Id];
     sample[4][Id] = sample[3][Id];
     sample[2][Id] = sample[1][Id];
     sample[1][Id] = sum;

     return(sum);
}

