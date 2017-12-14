/* ===================================================================== */
/*+
 *
 * FILENAME
 * --------
 * control.c
 *
 * PURPOSE
 * -------
 * Implement the control loops necessary for the blending of guide and
 * TCS demand sources for the tilt system
 *
 * FUNCTION NAME(S)
 * ----------------
 * projectSource   - project demand from apply time to current time using 
 *                   TCS model
 * blendSources    - combine guide sources
 * fireLoops       - handle watchdog timer at each timeout
 * processGuides   - assemble the M2 commands in page0 and raise the interrupt
 * iir_filter      - perform filter operation
 * updateEventPage - updates eventData (formerly a page in RM, now a global
 *                   structure) and global var "currentBeam"
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
 * 22-Oct-1997: Add macro function limit(i,j) to keep i within limits of j
 * 23-Oct-1997: Use  followOn flag instead of FOLLOWING state
 * 24-Oct-1997: modify to use guideOn flag only, remove guideOverride
 * 07-Nov-1997: change wfs arrays to externals
 * 05-Jan-1998: Simplify writeGuide processing
 * 24-Feb-1998: add array index check to setPid
 * 30-Mar-1998: remove writeGuide, move functionality to wfs.c
 * 04-May-1998: Correct pointer reference in eventPageUpdate to scsBase not 
 *              scsPtr
 * 23-Jun-1998: Change calls from setHealth to reportHealth
 *              fix - call to gaos2m2 changed to gyro2m2
 * 29-Jun-1998: Add processing for jogBeam to select beam whilst not yet 
 *              chopping
 * 21-Aug-1998: Increase guideUpdateNow timeout to 2 ticks
 * 19-Feb-1999: Bug fix - only perform PID calculation when there has been a new
 *              guide update.
 * 02-Mar-1999: Previous modification incorrect, copy current guide correction 
 *              to nGuideTcs _after_ the pid algorithm. Remove logMsg and printf
 *              statements left over from testing
 * 15-Apr-1999: Split AG_NODE into AGP1_NODE and AGP2_NODE to accomodate 
 *              separation of WFSs onto two different CPUs
 * 07-May-1999: Changed processGuides for pwfs2 so that guides added when
 *              "interval" changes instead of "time".
 * 07-May-1999: Added RCS id
 * 21-May-1999: Include interp.h instead of defining external prototype for
 *              getInterpolation
 * 31-May-1999: temp printf to see if interval is getting updated
 * 01-Jun-1999: Changed guideOn to type "long" to match scs_st.stpp
 *              Moved updateInterval declaration to guide.c
 *              Changed tabs to blanks
 *              Commented out the printfs in processGuides
 * 07-Dec-1999: Moved iir_filter and frameConvert from utilities.c to here
 *              since only used by control.c
 * 27-Sep-2002: Freeze last guide values when stopping guide loop
 * 19-Oct-2017: Begin conversion to EPIS OSI (mdw)
 *
 */
/* ===================================================================== */
#include <string.h>     /* For strncpy */
#include <math.h>       /* For abs */
#include <stdio.h>      /* for sprintf() */

#include <timeLib.h>    /* For timeNow */
#include <vmi5588.h>    /* For rmIntSend */
#include <drvXy240.h>   /* for xy240_writePortBit() */

#include "utilities.h"  /* For debugLevel, ag2m2 */
#include "archive.h"    /* For refMemFree */
#include "chop.h"       /* For jogBeam, chopIsOn */
#include "control.h"    /* For logThreshold, SYSTEM_CLOCK_RATE */
#include "guide.h"      /* For updateInterval, guideOn, guideOnA, guideOnB,
                           weight */
#include "interlock.h"  /* For lockPosition, scsState */
#include "interp.h"     /* For AX, AY, ..., Z axis identifiers */
#include "eventBus.h"   /* fo XYCARDNUM */

 /* Define limits for incremental steps */
#define TILT_GUIDE_STEP_LIMIT   32.0   /* arcsec  */

 /* This limit is now so big that it really isn't a limit at all.
    It could be removed completely and maybe for the TILT_GUIDE_STEP_LIMIT
    as well. But of course, doing this may introduce new problems - for eg.
    the offload to the mount - and so leave in for now
  */
#define Z_GUIDE_STEP_LIMIT      3000.0   /* microns */

#define DEFAULT_VARIANCE 0.01
#define COVAR_MIN        0.01
#define COVAR_MAX        100

//#define RECEIVE_TIMEOUT         100     /* 1 second * system clock rate */
#define RECEIVE_TIMEOUT         1.0       /* 1 second */

#define MAGIC_NUMBER            1958    /* magic number so sequences don't
                                           all start at zero    */
#define CB_RECORD_NB            2000
#define SCS_DATA_SIZE        81
#define MAX_FILENAME_SIZE       100
#define LOG_INTERVAL            5

int sep = 0;   /* Send Every guide pulse (Default, NO. Send every other pulse.*/ 
static void reportTimes();
struct timespec timeStart, timeEnd;
int mytimeshow = 0;

 typedef struct
{
   double  x;
   double  y;
   double  z;
}converted;

/* In-file globals */
static int grab = 0;
static char errBuff[81];

/* define structure of M2 command names for debug purposes */

static char *m2CmdName[] =
{
   "FAST_ONLY           ",
   "POSITION            ",
   "CHOP_ON             ",
   "CHOP_OFF            ",
   "SYNC_SOURCE_SCS     ",
   "SYNC_SOURCE_M2      ",
   "ACT_PWR_ON          ",
   "ACT_PWR_OFF         ",
   "CMD_INIT            ",
   "CMD_RESET           ",
   "CMD_TEST            ",
   "MSTART              ",
   "MEND                ",
   "VIBSTART             ",
   "VEND                ",
   "MOFFLON             ",
   "MOFFLOFF            ",
   "DECS_ON             ",
   "DECS_OFF            ",
   "DECS_PAUSE          ",
   "DECS_CONTINUE       ",
   "DECS_FREEZE         ",
   "DECS_UNFREEZE       ",
   "DECS_ZERO           ",
   "TILT_SPACE          ",
   "ACTUATOR_SPACE      ",
   "BAFFLE_CHANGE       ",
   "DECS_CHANGE         ",
   "BANDWIDTH_CHANGE    ",
   "TOLERANCE_CHANGE    ",
   "CHOP_CHANGE         ",
   "DIAGNOSTICS_REQUEST ",
   "UPDATE_XY_RANGE     ",
   "UPDATE_XPYP_RANGE   ",
   "VSTARTDRVFOL        ",
   "VDRVFOL             ",
   "VSTOPDRVFOL         ",
   "MSTARTDRVOFL        ",
   "MDRVOFL             ",
   "MSTOPDRVOFL         ",
   "DBDRV               ",
   "CBDRV               ",
   "XYDRV               ",
   "VDECS_ON            ",
   "VDECS_OFF           ",
   "LOGGING_ON          ",
   "LOGGING_OFF         ",      /* 46 */
   "CEM_ON              ",
   "CEM_OFF             ",
   "TTL_SERVO_ON        ",
   "TTL_SERVO_OFF       ",
   "TOGGLE_CEM_POWER    ",
   "OPEN_LOG            ",
   "START_LOG           ",
   "STOP_LOG            ",
   "CLOSE_LOG           ",
   "SEQUENCE1           ",
   "SEQUENCE2           ",
   "SEQUENCE3           ",
   "CMD_XYINIT          ",
   "XY_DEADBAND_CHANGE  ",
   "SCS_TIME_UPDATE     ",
   NULL
};

/* structure to hold volatile status of ref mem links */

static struct
{
   long NS;
   long scsHeartbeat;
   long m2Heartbeat; long testRequest;
}   local =

{
   0,
   MAGIC_NUMBER,               /* initialise the heartbeats at arbitrary
                                * non-zero figure */
   MAGIC_NUMBER + 1,
   0
};


#ifdef MK 
static Phasor phasorX = {
        {{1.0},{0.0}},      /* Snew */
        {{1.0},{0.0}},      /* Sold*/
        0.0,                /* command result*/
        12.3,               /* Frequency of signal*/
        0.1,                /* Amplitude of signal*/
        198.9,              /* Sample Rate*/
        0.0050277,          /* Sample time = 1/Fs    ==> 5ms @ 200Hz  */
        0.0,                /* Theta Initial Valie*/
        {{0.0, 0.0}, {0.0, 0.0}}, /* Rotation Matrix Initial values*/
};

static Phasor phasorY = {
        {{1.0},{0.0}},      /* Snew */
        {{1.0},{0.0}},      /* Sold*/
        0.0,                /* command result*/
        12.5,               /* Frequency of signal*/
        0.1,                /* Amplitude of signal*/
        198.9,              /* Sample Rate*/
        0.0050277,          /* Sample time = 1/Fs    ==> 5ms @ 200Hz  */
        0.0,                /* Theta Initial Valie*/
        {{0.0, 0.0}, {0.0, 0.0}}, /* Rotation Matrix Initial values*/
};



extern double phasorNewAmp;
extern int phasorAmpRequestChanged;
extern double phasorNewFreq;
extern int phasorFreqRequestChanged;
extern int phasorNewSR;
extern int phasorSRRequestChanged;
long phasorXApply = 0;
long phasorYApply = 0;
long xvtkGuideRecycle = 0;
long yvtkGuideRecycle = 0;
#define DEFAULT_TILT_SCALE 3.917

static Vtk vtkX = {
       {{{1.0},{0.0}},  {{1.0},{0.0}}},     /* Oscillator {newval , oldval}*/
       198.9,                               /* Vtk Sample Rate (Hz) matches P2 for now*/ 
       0.0050277,                           /* Vtk Sample period (seconds) matches P2 for now*/ 
       {0.0050, 0.00},                      /* {Gain.phase, Gain.frequency}*/
       {12.0, 0.2, 0.0, 0.0},               /* {Frequency.initialValue, Frequency.tolerance, Frequency.error, Frequency.current} */
       1.0,                                 /* Maxamplitude (arc-seconds in skyangle) Vtk will attempt to correct for. */
       1.557,                               /* Scale */
       -9.7,                                /* Angle of Rotation (degrees)*/
       {{0.0, 0.0}, {0.0, 0.0}},            /* Rotation Matrix Initial values*/
       {{{0.0},{0.0}},  {{0.0},{0.0}}},      /* Local Oscillator {newval , oldval}*/
       {{0.0},{0.0}},                        /* Integral {{Avalue}, {Bvalue}}*/
       0.0,                                  /* phase*/
       0.0,                                  /* phaseOld */
       0.0,                                  /* deltaPhase */
       0.0,                                  /* command */
       0,                                    /* counter */
  
};

static Vtk vtkY = {
       {{{1.0},{0.0}},  {{1.0},{0.0}}},     /* Oscillator {newval , oldval}*/
       198.9,                               /* Vtk Sample Rate (Hz) matches P2 for now*/ 
       0.0050277,                           /* Vtk Sample period (seconds) matches P2 for now*/ 
       {0.0050, 0.00},                       /* {Gain.phase, Gain.frequency}*/
       {12.0, 0.2,0.0,0.0},                 /* {Frequency.initialValue, Frequency.tolerance, Frequency.error, Frequency.current} */
       1.0,                                 /* Maxamplitude (arc-seconds in skyangle) Vtk will attempt to correct for. */
       1.557,                              /* Scale */
       -9.7,                                /* Angle of Rotation (degrees)*/
       {{0.0, 0.0}, {0.0, 0.0}},            /* Rotation Matrix Initial values*/
       {{{0.0},{0.0}},  {{0.0},{0.0}}},      /* Local Oscillator {newval , oldval}*/
       {{0.0},{0.0}},                        /* Integral {{Avalue}, {Bvalue}}*/
       0.0,                                  /* phase*/
       0.0,                                  /* phaseOld */
       0.0,                                  /* deltaPhase */
       0.0,                                  /* command */
       0,                                    /* counter */
};

#endif

int applyGuide = FALSE;
int guideUpdate = FALSE;
static double xNetGuide = 0.0;
static double yNetGuide = 0.0;
static double zNetGuide = 0.0;
static double xNetGuideT = 0.0;
static double yNetGuideT = 0.0;
static double zNetGuideT = 0.0;
static double xNetGuideU = 0.0;
#ifdef MK 
static double xRecycleGuideU = 0.0;
static double yRecycleGuideU = 0.0;
#endif
static double yNetGuideU = 0.0;
static double zNetGuideU = 0.0;
static int positionUpdate = FALSE;


int scstimeUpdate = 0; 
long servoOnStatus;

/* function prototypes */
static int  frameConvert (converted *result, frameChange *f, const double x, 
      const double y, const double z);


#ifdef MK 
void phasorShow(void);
#endif

int flipGuide = 0;
int simLevel = 0;
memMap *scsPtr = NULL;
memMap *scsBase = NULL;
memMap *m2Ptr = NULL;
epicsMutexId m2MemFree = NULL;
epicsEventId slowUpdate = NULL;
epicsMutexId wfsFree[MAX_SOURCES];
epicsEventId diagnosticsAvailable = NULL;
epicsEventId guideUpdateNow = NULL;
epicsEventId scsDataAvailable = NULL;
epicsEventId scsReceiveNow = NULL;
epicsMutexId eventDataSem = NULL;
epicsEventId cemTimerEndSem = NULL;
epicsEventId cemTimerStartSem = NULL;
long interlockFlag = OFF;
statusBlock safeBlock;

/* get current guide values to send to TCS */
double xGuideTcs = 0.0;
double yGuideTcs = 0.0;
double zGuideTcs = 0.0;
Demands setPoint;
epicsMutexId setPointFree = NULL;
int currentBeam = BEAMA;
int flip2 = 0;
int flip = 0;      /* indicates whether or not beams s/b flipped
                           eg. 1 for sig gen; 0 for OSCIR. set to 1 at
                           crate console */

double sampleData[5][3];
double coeffData[5][3];
int nodeISR2 = 0;
int nodeISR3 = 0;
int guideType = AUTOGUIDE;
epicsMessageQueueId commandQId = NULL;
epicsMessageQueueId receiveQId = NULL;
double tiptiltGuideLimitFactor = 1.0;
double focusGuideLimitFactor = 1.0;

PID controller[MAX_AXES]
= {
   {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
   {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
   {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
};

wfs filtered[MAX_SOURCES]
= {
   {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, "not initialised", 0.0, 0.0, 0.0},
   {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, "not initialised", 0.0, 0.0, 0.0},
   {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, "not initialised", 0.0, 0.0, 0.0},
   {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, "not initialised", 0.0, 0.0, 0.0},
   {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, "not initialised", 0.0, 0.0, 0.0}
};

/* Structure to hold position demands */
Demands tcs = {
   0.0, 0.0, 0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0
};

/* Structure to hold event data */
eventBlock eventData;

/* flag to show following active { ON | OFF} */
long followOn = OFF;

/* flag to show tilt/focus PIDs active { ON | OFF} */
long tiltPidOn = ON;
long focusPidOn = ON;

#ifdef MK
long vibrationXTrackOn = OFF;
long vibrationYTrackOn = OFF;
#endif

/* Circular buffers for FG + chopping debugging purposes */
/* The data type is chosen based on the type of the variable 
 * assigned to it. Typically, the subroutines use doubles for
 * the calculations and the reflective memory uses floats.
 * Note: the debugging is set up to log P2 guides. Changes 
 * would be required to debug P1 or OIWFS, etc. */

static int cbCounter=0;

static float cbXRawGuide[CB_RECORD_NB];
static float cbYRawGuide[CB_RECORD_NB];
static float cbZRawGuide[CB_RECORD_NB];

static double cbXGuideBeforePID[CB_RECORD_NB];
static double cbYGuideBeforePID[CB_RECORD_NB];
static double cbZGuideBeforePID[CB_RECORD_NB];

static double cbXGuideAfterPID[CB_RECORD_NB];
static double cbYGuideAfterPID[CB_RECORD_NB];
static double cbZGuideAfterPID[CB_RECORD_NB];

static double cbXGuideDemand[CB_RECORD_NB];
static double cbYGuideDemand[CB_RECORD_NB];
static double cbZGuideDemand[CB_RECORD_NB];

static int cbCurBeam[CB_RECORD_NB];
static int cbInPos[CB_RECORD_NB];
static int cbApplyGuide[CB_RECORD_NB];
static int cbGuideOnA[CB_RECORD_NB];

static double cbTime[CB_RECORD_NB];
static double cbP2Time[CB_RECORD_NB];
static float cbP2Interval[CB_RECORD_NB];
//static unsigned long cbTick[CB_RECORD_NB];

static float cbAXDemand[CB_RECORD_NB];
static float cbAYDemand[CB_RECORD_NB];
static float cbBXDemand[CB_RECORD_NB];
static float cbBYDemand[CB_RECORD_NB];

#ifdef MK
static double cbVTKXIntegrator0[CB_RECORD_NB];
static double cbVTKXIntegrator1[CB_RECORD_NB];

static double cbVTKYIntegrator0[CB_RECORD_NB];
static double cbVTKYIntegrator1[CB_RECORD_NB];

static double cbVTKXCommand[CB_RECORD_NB];
static double cbVTKYCommand[CB_RECORD_NB];

static double cbVTKXPhaseOld[CB_RECORD_NB];
static double cbVTKXPhaseNew[CB_RECORD_NB];

static double cbVTKYPhaseOld[CB_RECORD_NB];
static double cbVTKYPhaseNew[CB_RECORD_NB];

static double cbVTKXFrequency[CB_RECORD_NB];
static double cbVTKYFrequency[CB_RECORD_NB];

static double     cbVTKXFreqError[CB_RECORD_NB];
static double    cbVTKYFreqError[CB_RECORD_NB];

static double     cbVTKXdeltaPhase[CB_RECORD_NB];
static double     cbVTKYdeltaPhase[CB_RECORD_NB];

static double     cbXGuidePhasor[CB_RECORD_NB];
static double     cbYGuidePhasor[CB_RECORD_NB];
#endif


void setNS(int value)
{
   local.NS = value;
}
/* ===================================================================== */
/*
 * Function name:
 * projectSource
 *
 * Purpose:
 * The function uses the timestamp from the WFS data to find the M2
 * mirror position for that time. These are summed together to get the
 * corrected position at the time of the sample The change in TCS open loop
 * demands for the period between the WFS sample and the control loop processing
 * time are calculated from the interpolation coefficients and used to project
 * the corrected position to the current time
 *
 * Invocation:
 * status = projectSource(source, timeStamp)
 *
 * Parameters in:
 * > source        int     index related to guide source 
 * > timeStamp     double  time to which the demand should be projected
 *
 * Parameters out:
 * None
 *
 * Return value:
 * < status        int     OK or ERROR
 *
 * Globals:
 *    External functions:
 *    readArchive
 *
 *    External variables:
 *    None
 *
 * Requirements:
 * None
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 15-Oct-1997: Original(srp)
 *
 */

/* ===================================================================== */

void projectSource (void)
{
   double deltaX = 0, deltaY = 0, deltaZ = 0.0;
   double xNow, yNow, zNow;
   double timeStamp;
   int source = PWFS1;

   m2History archiveEntry;

   if (timeNow (&timeStamp) != OK)
   {
      errorLog ("projectSource - error reading timeStamp\n", 1, ON);
   }

   /* fetch the current TCS open loop demand value */

   if (readArchive (&archiveEntry, timeStamp) == OK)
   {
      xNow = archiveEntry.setX;
      yNow = archiveEntry.setY;
      zNow = archiveEntry.setZ;
   }
   else
   {
      xNow = 0.0;
      yNow = 0.0;
      zNow = 0.0;
   }

   for (source = PWFS1; source <= GYRO; source++)
   {
      if ((weight[source][currentBeam] >= -1 && currentBeam == BEAMA) ||
            (weight[source][currentBeam] >= -1 && currentBeam == BEAMB) ||
            (weight[source][currentBeam] >= -1 && currentBeam == BEAMC))
      {

         /*
          * find how much the open loop TCS demands have changed since the
          * WFS update
          */

         /*
          * fetch pointer to the mirror position corresponding to the WFS
          * time update
          */

         epicsMutexLock(wfsFree[source]);
         if (readArchive (&archiveEntry, filtered[source].time) == OK)
         {
            deltaX = xNow - archiveEntry.setX;
            deltaY = yNow - archiveEntry.setY;
            deltaZ = zNow - archiveEntry.setZ;

            filtered[source].z1 = 
               archiveEntry.xTilt + filtered[source].z1 + deltaX;
            filtered[source].z2 = 
               archiveEntry.yTilt + filtered[source].z2 + deltaY;
            filtered[source].z3 = 
               archiveEntry.zFocus + filtered[source].z3 + deltaZ;
         }
         else
         {

            /*
             * if position cannot be fetched, use current position
             */

            errorLog ("projectSource - can't retrieve archive position",
                     1, ON);

            filtered[source].z1 = scsPtr->page1.xTilt + filtered[source].z1 + deltaX;
            filtered[source].z2 = scsPtr->page1.yTilt + filtered[source].z2 + deltaY;
            filtered[source].z3 = scsPtr->page1.zFocus + filtered[source].z3 + deltaZ;
         }
         epicsMutexUnlock(wfsFree[source]);
      }
   }
}

/* ===================================================================== */
/*
 * Function name:
 * blendSources
 *
 * Purpose:
 * Combine the multiple sources of guide data according to the weights
 * specified for the current beam
 *
 * Invocation:
 * blendSources()
 *
 * Parameters in:
 * None
 *
 * Parameters out:
 * None
 *
 * Return value:
 * None;
 *
 * Globals:
 *    External functions:
 *    None
 *
 *    External variables:
 *    xNetGuide
 *    yNetGuide
 *    zNetGuide
 *
 * Requirements:
 * None
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 15-Oct-1997: Original(srp)
 * 24-Oct-1997: Name change currentBeam to beam
 * 11-Nov-1997: Rewrite to use global currentBeam and precalculated weights 
 *              weight[][]
 * 06-Jan-1998: Allow selection of AUTOGUIDE to use filtered only or not 
 *              AUTOGUIDE to use projected guide values
 */

/* ===================================================================== */

void blendSources (void)
{
   int source;
   double Kx, Ky, Kz;
   double varianceX = DEFAULT_VARIANCE;
   double varianceY = DEFAULT_VARIANCE;
   double varianceZ = DEFAULT_VARIANCE;
   double netVarX = DEFAULT_VARIANCE;
   double netVarY = DEFAULT_VARIANCE; 
   double netVarZ = DEFAULT_VARIANCE;
   int first = TRUE;

   for (source = PWFS1; source <= GYRO; source++)
   {
      if (weight[source][currentBeam] < -1)
      {
         /* guide source not used */
         continue;
      }
      else if (weight[source][currentBeam] >= 0)
      {

         /* calculate variance from user entry of standard error */

         varianceX = weight[source][currentBeam] * 
            weight[source][currentBeam];
         varianceY = varianceX;
         varianceZ = varianceX;
      }
      else if (weight[source][currentBeam] >= -1)
      {
         /*
          * calculate variance from value supplied by guide source
          */

         if (filtered[source].err1 >= 0)
            varianceX = filtered[source].err1 * filtered[source].err1;
         else
            varianceX = DEFAULT_VARIANCE;

         if (filtered[source].err1 >= 0)
            varianceY = filtered[source].err2 * filtered[source].err2;
         else
            varianceY = DEFAULT_VARIANCE;

         if (filtered[source].err3 >= 0)
            varianceZ = filtered[source].err3 * filtered[source].err3;
         else
            varianceZ = DEFAULT_VARIANCE;
      }
      else
      {
         /* this guide source is not used for this beam */

         continue;
      }

      /* range check the variances */

      varianceX = confine (varianceX, COVAR_MAX, COVAR_MIN);
      varianceY = confine (varianceY, COVAR_MAX, COVAR_MIN);
      varianceZ = confine (varianceZ, COVAR_MAX, COVAR_MIN);

      if (first == TRUE)
      {
         /* the first guide source to report does not need combining */

         netVarX = varianceX;
         netVarY = varianceY;
         netVarZ = varianceZ;

         Kx = 1; Ky = 1; Kz = 1;
         first = FALSE;
      }
      else
      {
         Kx = netVarX / (netVarX + varianceX);
         netVarX = (1 - Kx) * netVarX;

         Ky = netVarY / (netVarY + varianceY);
         netVarY = (1 - Ky) * netVarY;

         Kz = netVarZ / (netVarZ + varianceZ);
         netVarZ = (1 - Kz) * netVarZ;
      }

      xNetGuide = (double) filtered[source].z1;
      yNetGuide = (double) filtered[source].z2;
      zNetGuide = (double) filtered[source].z3;
   }

#if 0
   if (debugLevel == DEBUG_RESERVED2)
   {
      epicsPrintf("blendSources - adjusted net guides are: %f %f %f\n", 
            xNetGuide, yNetGuide, zNetGuide); 
   }
#endif
}

int profileCem = 0;
int time_debug = 0;
/* ===================
 *       
 *
 * cemTimerEnd() 
 */

void cemTimerEnd () {
   for(;;) {
      /* The only place this event is signalled is commented out for some reason.           */
      /* This wait will always time out. Maybe it should be replaced with a taskDelay()    */
      /* or EPICS OSI equivalent [epicsThreadSleep()]  20171030 (mdw)                        */
      if (epicsEventWaitWithTimeout(cemTimerEndSem, SEM_TIMEOUT) == epicsEventWaitOK)  {
         
        clock_gettime( CLOCK_REALTIME, &timeEnd);

        if (profileCem) { 
            profileCem = 0;
            reportTimes();
        }
      }
      else {
         if (time_debug) {
            errlogPrintf("cemTimerEnd timeout\n");
         }
      }
   }
}

/* ===================
 *       
 *
 * cemTimerStart() 
 */

void cemTimerStart () 
{
   for(;;) {
      if (epicsEventWaitWithTimeout(cemTimerStartSem, SEM_TIMEOUT) == epicsEventWaitOK)  {
         clock_gettime( CLOCK_REALTIME, &timeStart);
      } 
      else {
         if (time_debug) {
            errlogMessage("cemTimerStart timeout\n");
         }
      }
   }
}

/* Report on times elapsed to service the CEM Interrupt*/
static void reportTimes() {

   struct timespec result;
   
   if ((timeEnd.tv_nsec - timeStart.tv_nsec) < 0) {
      result.tv_sec = timeEnd.tv_sec - timeStart.tv_sec -1;
      result.tv_nsec = 1E+9 + timeEnd.tv_nsec - timeStart.tv_nsec;
   }
   else {
      result.tv_sec = timeEnd.tv_sec - timeStart.tv_sec;
      result.tv_nsec = timeEnd.tv_nsec - timeStart.tv_nsec;
   }

   errlogPrintf("Elapsed time: %ld:%ld\n",result.tv_sec, result.tv_nsec);
}

/* ===================================================================== */
/*
 * Function name:
 * fireLoops
 *
 * Purpose:
 * Watchdog timer to fire to provide a semaphore at the loop sample rate
 *
 * Invocation:
 * fireLoops
 *
 * Parameters in:
 * None
 *
 * Parameters out:
 * None
 *
 * Return value:
 * < status        int     OK or ERROR
 *
 * Globals:
 *    External functions:
 *    None
 *
 *    External variables:
 *    None
 *
 * Requirements:
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 15-Oct-1997: Original(srp)
 * 06-Dec-2017: Converted to a thread that runs periodically (mdw)
 *
 */

/* ===================================================================== */
void fireLoops (void *param)
{

   while (1){
      /* translation demands update to m2 system at 1Hz */
      /* set flag to indicate position update */
      positionUpdate = TRUE;
      epicsThreadSleep(1.0);
   }
}


/* Later: add header */
void rmISR2 (int node)
{
   nodeISR2 = node;
   epicsEventSignal(scsReceiveNow);

   /* why is this commented out? This is the only place the 
    * event is signalled. 20171030 (mdw) */ 
   /*
   semGive (cemTimerEndSem);
   */
}

/* Later: add header */
void rmISR3 (int node)
{
   nodeISR3 = node;
   epicsEventSignal(guideUpdateNow);
}

/* ===================================================================== */
/*
 * Function name:
 * processGuides
 *
 * Purpose:
 * Fetch new guide data from reflective memory, apply coordinate conversion
 * and filtering as appropriate. Combine the guide signals and call PID
 * controllers. Write the resulting guide corrections to m2 and raise an
 * interrupt to alert the m2 system
 *
 * Invocation:
 * processGuides
 *
 * Parameters in:
 * None
 *
 * Parameters out:
 * None
 *
 * Return value:
 *      None
 *
 * Globals:
 *    External functions:
 *    None
 *
 *    External variables:
 *    None
 *
 * Requirements:
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 15-Oct-1997: Original(srp)
 * 19-Feb-1999: Bug fix - only perform PID calculation when there has been a new
 *              guide update.
 * 02-Mar-1999: Copy current guide correction to nGuideTcs _after_ the pid algorithm
 *
 */

/* ===================================================================== */

#ifdef MK
int rxwaitticks = 0;
int useDynamicVtk =0;
#endif

int    waittime = 0.08;   

void processGuides (void) 
{
   long command = FAST_ONLY;
   location    position;
   converted   result = {0,0,0};
   int indx = 0; 
   //char message[200];
   long lastNS = 0;

#ifdef MK 
   long sensedGuideRate = GUIDE_200_HZ;
#endif

   static struct
   {
      double pwfs1;
      double pwfs2;
      double oiwfs;
      double gaos;
      double gyro;

#ifndef MK
      double gpi;
#endif

#ifdef MK
   } updateTime = { 0.0, 0.0, 0.0, 0.0, 0.0 };  
#else
   } updateTime = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };  
#endif
   
   /* Used to time stamp a set of data written to the ring buffers */
   double cbTimeStamp;

#ifdef MK
   double tsdiff=0.0;
   static double tsold=0.0;

   /* Initialize Vibration Tracking*/
   phasorInit(&phasorX);
   showPhasorRotation(&phasorX);

   phasorInit(&phasorY);
   showPhasorRotation(&phasorY);

   vtkInit(&vtkX);
   showVtkRotation(&vtkX);

   vtkInit(&vtkY);
   showVtkRotation(&vtkY);
#endif

   /* Initialize eventData structure */ 
   eventData.currentBeam = 0;
   eventData.inPosition = 0;

   for (;;)
   {

      /* Wait for either guide update or timeout */

      /* OLD WAY: No status checking. - wait 2 ticks -- if it fails
       *   we still proceed.  semTake (guideUpdateNow, 2);

       * NEW. Try to take the semaphore given by "rmISR3" But don't wait for
       * it. We have things to do.    Idea: If don't take this semaphore, is
       * there any point to check the .time fields for an update? I don't see
       * it, so put the .time checking for all P1, P2, OI, Gyro, Gaos into the
       * clause when sem is successfully taken. 

       * NOTES: if I use NO_WAIT for the timeout, the task hangs the crate :(
       * So, using 1 tick or 0.05 seconds as the timeout. But, since the ISR
       * gives the semaphore, if guide is updated, this code should never need
       * to wait for the timeout. 

       * Change to 10 ticks (08-jun-2000, since see that crate is now at 95%
       * CPU usage. Maybe this is the reason? And really, the semaphore can be
       * as long as the rate the TCS sends at 20 Hz = 20 x per sec = 0.05 s 
       */


      if (epicsEventWaitWithTimeout(guideUpdateNow, waittime) == epicsEventWaitOK) 
         /* then ISR has given sem or it has never been taken */
      {
         epicsThreadSleep(0.001);
         /* Find which sources have been updated since last ISR call 
          * first, check PWFS1 */

         if (debugLevel == DEBUG_RESERVED2)
         {
            errlogPrintf( "***** nodeISR3 = %d intervalas %f > %f \n",
                  nodeISR3, scsBase->pwfs1.interval,  updateInterval.pwfs1); 
         }

         if ( (nodeISR3 == AGP1_NODE) && (weight[PWFS1][currentBeam] > -2) )
         {
            if (scsBase->pwfs1.interval > updateInterval.pwfs1) 
            {
               if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
               {
                  epicsPrintf("processGuides - increasing time detected for PWFS1\n");
               }
               updateInterval.pwfs1 = scsBase->pwfs1.interval;
               updateTime.pwfs1 = scsBase->pwfs1.time;

               if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
               {
                  epicsPrintf("processGuides - read RM data from PWFS1\n");
               }

               /* copy the data from reflective memory page */
               frameConvert(&result, ag2m2[PWFS1], 
                     (double)scsBase->pwfs1.z1, 
                     (double)scsBase->pwfs1.z2, 
                     (double)scsBase->pwfs1.z3);

               filtered[PWFS1].z1 = result.x;
               filtered[PWFS1].z2 = result.y;
               filtered[PWFS1].z3 = result.z;
               filtered[PWFS1].err1 = scsBase->pwfs1.err1;
               filtered[PWFS1].err2 = scsBase->pwfs1.err2;
               filtered[PWFS1].err3 = scsBase->pwfs1.err3;
               filtered[PWFS1].time = scsBase->pwfs1.time;

               /* filter the transformed demands */
               switch (filter[PWFS1][XTILT].type)
               {
                  case RAW:
                  case NOTUSED:
                     break;

                  default:
                     filtered[PWFS1].z1 = iir_filter 
                        ((double) filtered[PWFS1].z1, 
                         &filter[PWFS1][XTILT]);
                     filtered[PWFS1].z2 = iir_filter 
                        ((double) filtered[PWFS1].z2, 
                         &filter[PWFS1][YTILT]);
                     filtered[PWFS1].z3 = iir_filter 
                        ((double) filtered[PWFS1].z3, 
                         &filter[PWFS1][ZFOCUS]);
               }

               /* instead of calling blend sources later on */
               xNetGuide = (double) filtered[PWFS1].z1;
               yNetGuide = (double) filtered[PWFS1].z2;
               zNetGuide = (double) filtered[PWFS1].z3;

               guideUpdate = TRUE;
            }
         }
         else if ( (nodeISR3 == AGP2_NODE) && (weight[PWFS2][currentBeam] > -2) )
         {
            if (scsBase->pwfs2.interval > updateInterval.pwfs2) 
            {
               /* Then check PWFS2 */
               if ((debugLevel > DEBUG_MIN ) & (debugLevel <= DEBUG_MED))
               {
                  epicsPrintf("increasing time detected for PWFS2\n"); 
               }

               updateInterval.pwfs2 = scsBase->pwfs2.interval; 
               updateTime.pwfs2 = scsBase->pwfs2.time;

               if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
               {
                  epicsPrintf("processGuides - read RM data from PWFS2\n");
               }

#ifdef MK
               /* N.B. Use this if you're not guiding with P2 and want to check the
                * functionality of VTK. Here we recycle the previous output
                * xNetGuideU. It is scaled by the P2 Signal Scale factor 
                * (0.1/0.143 = 0.7 pixels/arcsec) as if it was coming out of P2. 
                *
                *
                * This can work when you're using Synthesized Waves (SW) to simulate a vibration Signal.
                *
                * */
               if (  xvtkGuideRecycle ) {

                    scsBase->pwfs2.z1 =  xRecycleGuideU * (-0.7 / DEFAULT_TILT_SCALE) ;
               }

               if (  yvtkGuideRecycle ) {

                    scsBase->pwfs2.z2 = yRecycleGuideU * (-0.7/ DEFAULT_TILT_SCALE) ;
               }
#endif

               /* copy the data from reflective memory page */
               frameConvert(&result, ag2m2[PWFS2], 
                     (double)scsBase->pwfs2.z1, 
                     (double)scsBase->pwfs2.z2, 
                     (double)scsBase->pwfs2.z3);

               filtered[PWFS2].z1 = result.x;
               filtered[PWFS2].z2 = result.y;
               filtered[PWFS2].z3 = result.z;
               filtered[PWFS2].err1 = scsBase->pwfs2.err1;
               filtered[PWFS2].err2 = scsBase->pwfs2.err2;
               filtered[PWFS2].err3 = scsBase->pwfs2.err3;
               filtered[PWFS2].time = scsBase->pwfs2.time;

               /* filter the transformed demands */
               switch (filter[PWFS2][XTILT].type)
               {
                  case RAW:
#ifdef MK
                     if (debugLevel == DEBUG_RESERVED2) epicsPrintf("RAW\n");
#endif
                     break;
                  case NOTUSED:
#ifdef MK
                     if (debugLevel == DEBUG_RESERVED2) epicsPrintf("NOTUSED\n");
#endif
                     break;

                  default:
#ifdef MK
                     if (debugLevel == DEBUG_RESERVED2) epicsPrintf("DEFAULT\n");
#endif
                     filtered[PWFS2].z1 = iir_filter 
                        ((double) filtered[PWFS2].z1, 
                         &filter[PWFS2][XTILT]);
                     filtered[PWFS2].z2 = iir_filter 
                        ((double) filtered[PWFS2].z2, 
                         &filter[PWFS2][YTILT]);
                     filtered[PWFS2].z3 = iir_filter 
                        ((double) filtered[PWFS2].z3, 
                         &filter[PWFS2][ZFOCUS]);
               }

               /* instead of calling blend sources later on */
               xNetGuide = (double) filtered[PWFS2].z1;
               yNetGuide = (double) filtered[PWFS2].z2;
               zNetGuide = (double) filtered[PWFS2].z3;


               guideUpdate = TRUE;
            }
         }
#ifdef MK
         else if ( (nodeISR3 == AGOI_NODE) && (weight[OIWFS][currentBeam] > -2) )
#else
         else if ( (nodeISR3 == AGOI_NODE || nodeISR3 == F2OI_NODE) && (weight[OIWFS][currentBeam] > -2) )
#endif
         {
            if (scsBase->oiwfs.interval > updateInterval.oiwfs) 
            {
               updateTime.oiwfs = scsBase->oiwfs.time;
               updateInterval.oiwfs = scsBase->oiwfs.interval; 

               if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
               {
                  epicsPrintf("processGuides - read RM data from OIWFS\n");
               }

               /* copy the data from reflective memory page */
               frameConvert(&result, ag2m2[OIWFS], 
                     (double)scsBase->oiwfs.z1, 
                     (double)scsBase->oiwfs.z2, 
                     (double)scsBase->oiwfs.z3);

               filtered[OIWFS].z1 = result.x;
               filtered[OIWFS].z2 = result.y;
               filtered[OIWFS].z3 = result.z;
               filtered[OIWFS].err1 = scsBase->oiwfs.err1;
               filtered[OIWFS].err2 = scsBase->oiwfs.err2;
               filtered[OIWFS].err3 = scsBase->oiwfs.err3;
               filtered[OIWFS].time = scsBase->oiwfs.time;

               /* do focus scaling conversion */
               filtered[OIWFS].z3 *= frame.focusScaling;

               /* filter the transformed demands */
               switch (filter[OIWFS][XTILT].type)
               {
                  case RAW:
                  case NOTUSED:
                     break;

                  default:
                     filtered[OIWFS].z1 = iir_filter 
                        ((double) filtered[OIWFS].z1, 
                         &filter[OIWFS][XTILT]);
                     filtered[OIWFS].z2 = iir_filter 
                        ((double) filtered[OIWFS].z2, 
                         &filter[OIWFS][YTILT]);
                     filtered[OIWFS].z3 = iir_filter 
                        ((double) filtered[OIWFS].z3, 
                         &filter[OIWFS][ZFOCUS]);
               }

               /* instead of calling blend sources later on */
               xNetGuide = (double) filtered[OIWFS].z1;
               yNetGuide = (double) filtered[OIWFS].z2;
               zNetGuide = (double) filtered[OIWFS].z3;

               guideUpdate = TRUE;
            }
         }

         else if ( (nodeISR3 == GAOS_NODE) && (weight[GAOS][currentBeam] > -2) )
         {
            if (scsBase->gaos.interval > updateInterval.gaos) 
            { 
               updateTime.gaos = scsBase->gaos.time;
               updateInterval.gaos = scsBase->gaos.interval; 

               if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
               {
                  epicsPrintf("processGuides - read RM data from GAOS\n");
               }

               /* copy the data from reflective memory page */
               frameConvert(&result, ag2m2[GAOS],
                     (double)scsBase->gaos.z1,
                     (double)scsBase->gaos.z2,
                     (double)scsBase->gaos.z3);

               filtered[GAOS].z1 = result.x;
               filtered[GAOS].z2 = result.y;
               filtered[GAOS].z3 = result.z;
               filtered[GAOS].err1 = scsBase->gaos.err1;
               filtered[GAOS].err2 = scsBase->gaos.err2;
               filtered[GAOS].err3 = scsBase->gaos.err3;
               filtered[GAOS].time = scsBase->gaos.time;

               /* filter the transformed demands */

               switch (filter[GAOS][XTILT].type)
               {
                  case RAW:
                  case NOTUSED:
                     break;

                  default:
                     filtered[GAOS].z1 = iir_filter 
                        ((double) filtered[GAOS].z1, 
                         &filter[GAOS][XTILT]);
                     filtered[GAOS].z2 = iir_filter 
                        ((double) filtered[GAOS].z2, 
                         &filter[GAOS][YTILT]);
                     filtered[GAOS].z3 = iir_filter 
                        ((double) filtered[GAOS].z3, 
                         &filter[GAOS][ZFOCUS]);
               }

               /* instead of calling blend sources later on */
               xNetGuide = (double) filtered[GAOS].z1;
               yNetGuide = (double) filtered[GAOS].z2;
               zNetGuide = (double) filtered[GAOS].z3;

               guideUpdate = TRUE;
            } 
         }

#ifndef MK
        else if ( (nodeISR3 == GPI_NODE) && (weight[OIWFS][currentBeam] > -2) )
         {
             if (scsBase->gpi.interval > updateInterval.gpi) 
            { 
               if (debugLevel == DEBUG_RESERVED2) 
                   errlogMessage("processGuides - GPI NODE interrupting me\n"); 
  
               updateTime.gpi = scsBase->gpi.time;
               updateInterval.gpi = scsBase->gpi.interval; 

               if ((debugLevel > DEBUG_MIN) & (debugLevel <= DEBUG_MED))
               {
                  printf("processGuides - read RM data from GPI\n");
               }

               /* copy the data from reflective memory page */
               frameConvert(&result, ag2m2[GPI],
                     (double)scsBase->gpi.z1,
                     (double)scsBase->gpi.z2,
                     (double)scsBase->gpi.z3);

               filtered[GPI].z1 = result.x;
               filtered[GPI].z2 = result.y;
               filtered[GPI].z3 = result.z;
               filtered[GPI].err1 = scsBase->gpi.err1;
               filtered[GPI].err2 = scsBase->gpi.err2;
               filtered[GPI].err3 = scsBase->gpi.err3;
               filtered[GPI].time = scsBase->gpi.time;

               /* filter the transformed demands */

               switch (filter[GPI][XTILT].type)
               {
                  case RAW:
                  case NOTUSED:
                     break;

                  default:
                     filtered[GPI].z1 = iir_filter
                        ((double) filtered[GPI].z1,
                         &filter[GPI][XTILT]);
                     filtered[GPI].z2 = iir_filter
                        ((double) filtered[GPI].z2,
                         &filter[GPI][YTILT]);
                     filtered[GPI].z3 = iir_filter
                        ((double) filtered[GPI].z3,
                         &filter[GPI][ZFOCUS]);
               }

               /* instead of calling blend sources later on */
               xNetGuide = (double) filtered[GPI].z1;
               yNetGuide = (double) filtered[GPI].z2;
               zNetGuide = (double) filtered[GPI].z3;

               guideUpdate = TRUE;
             } 
         }
#endif
         else if (scsBase->gyro.time > updateTime.gyro)
         {
            updateTime.gyro = scsBase->gyro.time;

            if (weight[GYRO][currentBeam] > -2)
            {
               /* copy the data from reflective memory page */
               filtered[GYRO].z1 = scsBase->gyro.z1;
               filtered[GYRO].z2 = scsBase->gyro.z2;
               filtered[GYRO].z3 = scsBase->gyro.z3;
               filtered[GYRO].err1 = scsBase->gyro.err1;
               filtered[GYRO].err2 = scsBase->gyro.err2;
               filtered[GYRO].err3 = scsBase->gyro.err3;
               filtered[GYRO].time = scsBase->gyro.time;

               /* do focus scaling conversion */
               position.xTilt = filtered[GYRO].z1;
               position.yTilt = filtered[GYRO].z2;
               position.zFocus = filtered[GYRO].z3;

               gyro2m2 (&position);

               filtered[GYRO].z1 = position.xTiltNew;
               filtered[GYRO].z2 = position.yTiltNew;
               filtered[GYRO].z3 = position.zFocusNew;

               /* filter the transformed demands */
               switch (filter[GYRO][XTILT].type)
               {
                  case RAW:
                  case NOTUSED:
                     break;

                  default:
                     filtered[GYRO].z1 = iir_filter 
                        ((double) filtered[GYRO].z1, 
                         &filter[GYRO][XTILT]);
                     filtered[GYRO].z2 = iir_filter 
                        ((double) filtered[GYRO].z2, 
                         &filter[GYRO][YTILT]);
                     filtered[GYRO].z3 = iir_filter 
                        ((double) filtered[GYRO].z3, 
                         &filter[GYRO][ZFOCUS]);
               }

               /* instead of calling blend sources later on */
               xNetGuide = (double) filtered[GYRO].z1;
               yNetGuide = (double) filtered[GYRO].z2;
               zNetGuide = (double) filtered[GYRO].z3;

               guideUpdate = TRUE;
            }
         }
         else
         {
            guideUpdate = FALSE;
         }

      } /* END Semtake for guideUpdateNow == OK */

      else
      {
#if 0
         if (debugLevel == DEBUG_RESERVED2) 
            errlogMessage("processGuides - guideUpdateNow timeout\n"); 
#endif

         /* New, assume timeout must mean no guide update has
          * occurred and bypass timestamp checking 
          */
         guideUpdate = FALSE;         
      }

#ifdef MK
      if (debugLevel == DEBUG_RESERVED2)
      {
         errlogPrintf ("currentBeam = %1d, (0=BEAMA,3=A2BRAMP); guideOnA = %1d\n",
               currentBeam, guideOnA); 

         errlogPrintf("guideOn = %1ld; applyGuide = %1d, guideUpdate = %1d\n",
             guideOn, applyGuide, guideUpdate); 
      } 
#endif

      /* Notes about all these conditions...
       *
       *   - "guideOn" refers to request from TCS/SCS CAD
       *     executed to turn on guiding   
       *
       * - "applyGuide" refers to the beam setting being
       *   set up correctly and being ON BEAM (ie. not
       *   transitioning)
       *
       * - "guideUpdate" refers to the guide values in RM
       *   being validated by checking timestamp
       *
       */

      /* Always set applyGuide to TRUE when we're not chopping. If we are
       * chopping we need to assert the Guide Gate is valid by checking
       * inPosition.*/
      if ((!chopIsOn) || (chopIsOn && (eventData.inPosition == TRUE))) {
         applyGuide = TRUE;
      }
      else {
         applyGuide = FALSE;
      }


      if (guideOn == TRUE && applyGuide == TRUE)
      {

         /* Set pin JK2/41 high to show guiding is appplied 
         */
         xy240_writePortBit (XYCARDNUM, PORT7, BIT4, epicsTrue); /* card 0, port 7, bit 4 */

         if (guideUpdate == TRUE) /* a new guide update has arrived */
         {

            if (guideType == AUTOGUIDE)
            {
               /* Not used by M2, so not part of the checksum
                  just used for displaying the raw guide values.
                  Decided to use RM page 0 just to make sure these 2 vals are
                  synched with other guide values */

               scsBase->page0.rawXGuide = (float)xNetGuide; 
               scsBase->page0.rawYGuide = (float)yNetGuide;
               scsBase->page0.rawZGuide = (float)zNetGuide;



               /* Apply the PID loop if PidOn vars are set.
                * Note: default is ON*/
               if (tiltPidOn == ON)
               {
#if 0
                  if (debugLevel == DEBUG_RESERVED2) 
                     epicsPrintf("tiltPidOn is ON\n");
#endif
                  xNetGuideT = control (XTILT, xNetGuide, 0.0);
                  yNetGuideT = control (YTILT, yNetGuide, 0.0);
               }
               else
               {
#if 0
                  if (debugLevel == DEBUG_RESERVED2) 
                     epicsPrintf("tiltPidOn is OFF\n");
#endif
                  xNetGuideT = xNetGuide;
                  yNetGuideT = yNetGuide;
               }

#ifdef MK
               if (vibrationXTrackOn == ON) {

                   /* Calculate new vtk X Command and 
                    * Disable VTK if ERROR is returned. */
                   if (vtkControl(&vtkX, xNetGuide) != OK) {
                       vibrationXTrackOn = OFF;
                   }

                   xNetGuideT += vtkX.command;

               } 
               if (vibrationYTrackOn == ON) 
               {

                   /* Calculate new vtk Y Command and 
                    * Disable VTK if ERROR is returned. */
                   if (vtkControl(&vtkY, yNetGuide) != OK) {
                       vibrationYTrackOn = OFF;
                   }
    
                   /* Add the latest */
                   yNetGuideT += vtkY.command;
               }
#endif

               if (focusPidOn == ON)
               {
                  zNetGuideT = control (FOCUS, zNetGuide, 0.0);
               }
               else
               {
                  zNetGuideT = zNetGuide;

#ifdef MK
               }

               /* Synthesized Signal in X TILT axis*/
               if(phasorXApply) {

                   MatMult(phasorX.Rotator, phasorX.Sold, phasorX.Snew);   /* Advance the phasorX */
                   phasorX.command = phasorX.amp * phasorX.Snew[0][0];      /* New Value = A*cos(wt); */
                   memcpy(phasorX.Sold, phasorX.Snew, 2*1*sizeof(double)); /* Store new Svector Snew into Sold*/
                   xNetGuideT +=  xTiltGuideSimScale * phasorX.command * DEFAULT_TILT_SCALE;
               }

               /* Synthesized Signal in Y TILT axis*/
               if(phasorYApply) {

                   MatMult(phasorY.Rotator, phasorY.Sold, phasorY.Snew);   /* Advance the phasorY */
                   phasorY.command = phasorY.amp * phasorY.Snew[0][0];      /* New Value = A*cos(wt); */
                   memcpy(phasorY.Sold, phasorY.Snew, 2*1*sizeof(double)); /* Store new Svector Snew into Sold*/
                   yNetGuideT +=  yTiltGuideSimScale * phasorY.command * DEFAULT_TILT_SCALE;
#endif
 
               }

               /* Clamp the values of the guide. 
                  Tell the TCS the clamped values too, to keep 
                  it in sync with what the M2 receives. 
                  And, do this after the PID */
               xNetGuideU = confine (xNetGuideT, TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor,
                 -TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor);
          yNetGuideU = confine (yNetGuideT, TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor,
                      -TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor);
          zNetGuideU = confine (zNetGuideT, Z_GUIDE_STEP_LIMIT*focusGuideLimitFactor,
                 -Z_GUIDE_STEP_LIMIT*focusGuideLimitFactor);

               /* write values to TCS variables */
               xGuideTcs = xNetGuideU;
               yGuideTcs = yNetGuideU;
               zGuideTcs = zNetGuideU;


            } /* guideType == AUTOGUIDE */

            else /* guideType == PROJECT.
                    Note: We are NEVER using this. There is NO WAY to
                    set VALK field of the CAD PID from the dm screens.
                    this is truly never needed for the future, it could
                    be removed!!! */
            {
               errlogMessage("guideType is PROJECT\n"); 

               projectSource ();
               blendSources ();

               xNetGuideU = xNetGuide;
               yNetGuideU = yNetGuide;
               zNetGuideU = zNetGuide;

               /* Clamp the values of the guide */

               xNetGuideU = confine (xNetGuideU, TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor, 
                     -TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor);
               yNetGuideU = confine (yNetGuideU, TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor, 
                     -TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor); 
               zNetGuideU = confine (zNetGuideU, Z_GUIDE_STEP_LIMIT*focusGuideLimitFactor, 
                     -Z_GUIDE_STEP_LIMIT*focusGuideLimitFactor); 
            } /* not AUTOGUIDE */
         }
      } /* guideOn == TRUE && applyGuide == TRUE */

      /* Not guiding on this beam or not applying guide so
       * zero corrections.
       */
      else {   

         /* Set pin JK2/41 high to show guiding is *NOT*
          * appplied 
          */
         xy240_writePortBit(XYCARDNUM, PORT7, BIT4, epicsFalse);

         /* If the guide gate has been turned off, zero ALL the corrections*/
         xNetGuideT = 0.0;
         yNetGuideT = 0.0;

         xNetGuideU = confine (xNetGuideT, TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor,
               -TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor);
         yNetGuideU = confine (yNetGuideT, TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor, 
               -TILT_GUIDE_STEP_LIMIT*tiptiltGuideLimitFactor); 
        
      if (focusPidOn == ON)
         {
            zNetGuideT = control (FOCUS, 0.0, 0.0);
         }
         else
         {
            zNetGuideT = 0.0;
         }
         zNetGuideU = confine (zNetGuideT, Z_GUIDE_STEP_LIMIT*focusGuideLimitFactor, 
               -Z_GUIDE_STEP_LIMIT*focusGuideLimitFactor); 
      }

      /* ---------------------------ScsSend---------------------------*
       * Write new guide values et al commands to reflective memory. This is
       * where "processGuides" does more than its name suggests: since it is
       * actually filling in the entire M2 command page of RM, it should be more
       * appropriately be called "scsSend" 
       */

      if (simLevel == 0) /* No simulation, write to real reflective memory */
      {
#ifdef MK
          xRecycleGuideU = xNetGuideU;
          yRecycleGuideU = yNetGuideU;

          if (xvtkGuideRecycle)
            xNetGuideU = 0.0;

          if (yvtkGuideRecycle)
            yNetGuideU = 0.0;
#endif

          scsBase->page0.xTiltGuide = (float) xNetGuideU;
          scsBase->page0.yTiltGuide = (float) yNetGuideU;
          scsBase->page0.zFocusGuide = 
            (float) confine ((setPoint.zFocus + zNetGuideU), 
                  Z_FOCUS_LIMIT, -Z_FOCUS_LIMIT);

         /* Not used by M2, so not part of the checksum just used for displaying
          * the components of the focus. Decided to use RM page 0 just to make sure
          * these 2 vals are synched with zFocusGuide value */

         /* package M2 data for RM */
         scsBase->page0.zFocus = (float)setPoint.zFocus;
         scsBase->page0.zGuide = (float)zNetGuideU;
         scsBase->page0.xGrossTiltDmd = scsBase->page0.AxTilt + (float) xNetGuideU;
         scsBase->page0.yGrossTiltDmd = scsBase->page0.AyTilt + (float) yNetGuideU;

         /* fetch command from message queue */
         if( epicsMessageQueueTryReceive(commandQId, (char *) &command, sizeof (long)) < 0 )
            command = FAST_ONLY;

         if (command == CMD_TEST)
            local.testRequest = 1;

         /* print command to screen for testing */
         if ((command > POSITION) && (debugLevel == DEBUG_MED))
         {
            errlogPrintf ("processGuides - sent command =  %s (%d)", 
                  m2CmdName[command], (int)command);
         }

         scsBase->page0.commandCode = command;
         lastNS = scsBase->page0.NS;
         scsBase->page0.NS = ++local.NS;
         if (fabs(lastNS - scsBase->page0.NS) > 1000)
         {
            epicsPrintf("SCS sending NS = %ld\n", lastNS);
         }
         scsBase->page0.heartbeat = local.scsHeartbeat++;
         scsBase->page0.checksum = 
            checkSum ((void *) &scsBase->page0.NS, COMMAND_BLOCK_SIZE);

         /* flag availability of new data */
         /* The original ideal of sending only everyother pulse has bee removed */

         /* DO NOT Send Every Pulse */
         if (!sep) {
             indx++; 
             if (command > FAST_ONLY || indx > 1 )
             {  
                 rmIntSend (INT2, M2_NODE);
                 indx = 0; 
             }
         }

         /* Send Every Pulse */
         else {

#ifndef MK
            if(command > FAST_ONLY) {
               rmIntSend (INT2, M2_NODE);
            }
#else
           rmIntSend (INT2, M2_NODE);
#endif

         }
         /*Start timer to profile interrupt cycle times between SCS and CEM*/
         /*
         semGive(cemTimerStartSem);
         */
      }
      else /* simulation active, write to m2 buffer */
      {
         epicsMutexLock(m2MemFree);
         m2Ptr->page0.xTiltGuide = (float) xNetGuideU;
         m2Ptr->page0.yTiltGuide = (float) yNetGuideU;
         m2Ptr->page0.zFocusGuide = 
              (float) confine ((setPoint.zFocus + zNetGuideU), 
                     Z_FOCUS_LIMIT, -Z_FOCUS_LIMIT);

         /* package data */

         /* fetch command from message queue */

         if( epicsMessageQueueTryReceive(commandQId, (char *) &command, sizeof (long)) < 0)
            command = FAST_ONLY;

         if (command == CMD_TEST)
            local.testRequest = 1;

         m2Ptr->page0.commandCode = command;
         m2Ptr->page0.NS = ++local.NS;
         m2Ptr->page0.heartbeat = local.scsHeartbeat++;
         m2Ptr->page0.checksum = 
            checkSum ((void *) &m2Ptr->page0.NS, COMMAND_BLOCK_SIZE);

         epicsMutexUnlock(m2MemFree);

         /* print command to screen for testing */

         if ((command > POSITION) && (debugLevel == DEBUG_MIN))
         {
            errlogPrintf("sent command =  %s (%d)\n", m2CmdName[command], (int)command);
         }

         /* flag availability of new data */
         epicsEventSignal(scsDataAvailable);
      }
   

      /* flag that fast transmission is complete and slow updates may occur */

      /* Give the semaphore taken by "slowTransmit" since it is the guide task
       * that does the transmit of the slower items prepared for transmission to the
       * m2 system by "slowTransmit"  */
      epicsEventSignal(slowUpdate);  

      /* Update the ring buffers, all of them, here. Yes, even 
       * the ones that pertain to P2. This is where you would 
       * need to change if want to debug with another guide
       * source. */

      /* go get another timestamp */
      if (timeNow (&cbTimeStamp) != OK)
      {
         errorLog ("processGuides - error reading timeStamp\n", 1, ON);
         /* if there is a problem, don't update the cb */
         continue;
      }
      cbTime[cbCounter] = cbTimeStamp;
      // cbTick[cbCounter] = tickGet();


      /* Here is all the P2 specific stuff */
      cbXRawGuide[cbCounter] = scsBase->pwfs2.z1;
      cbYRawGuide[cbCounter] = scsBase->pwfs2.z2;
      cbZRawGuide[cbCounter] = scsBase->pwfs2.z3;
      cbP2Interval[cbCounter] = scsBase->pwfs2.interval;
      cbP2Time[cbCounter] = scsBase->pwfs2.time;

      cbCurBeam[cbCounter] = currentBeam;
      cbApplyGuide[cbCounter] = applyGuide;
      cbGuideOnA[cbCounter] = guideOnA;
      cbInPos[cbCounter] = eventData.inPosition;

      cbAXDemand[cbCounter] = scsBase->page0.AxTilt;
      cbAYDemand[cbCounter] = scsBase->page0.AyTilt;
      cbBXDemand[cbCounter] = scsBase->page0.BxTilt;
      cbBYDemand[cbCounter] = scsBase->page0.ByTilt;

      cbXGuideBeforePID[cbCounter] = xNetGuide;
      cbYGuideBeforePID[cbCounter] = yNetGuide;
      cbZGuideBeforePID[cbCounter] = zNetGuide;

      cbXGuideAfterPID[cbCounter] = xNetGuideT;
      cbYGuideAfterPID[cbCounter] = yNetGuideT;
      cbZGuideAfterPID[cbCounter] = zNetGuideT;

#ifdef MK
      cbXGuideDemand[cbCounter] = xRecycleGuideU;
      cbYGuideDemand[cbCounter] = yRecycleGuideU;
#else
      cbXGuideDemand[cbCounter] = xNetGuideU;
      cbYGuideDemand[cbCounter] = yNetGuideU;
#endif

      cbZGuideDemand[cbCounter] = zNetGuideU;

#ifdef MK
      cbVTKXCommand[cbCounter] = vtkX.command;
      cbVTKYCommand[cbCounter] = vtkY.command;

      cbVTKXPhaseOld[cbCounter] = vtkX.phaseOld;
      cbVTKYPhaseOld[cbCounter] = vtkY.phaseOld;

      cbVTKXPhaseNew[cbCounter] = vtkX.phase;
      cbVTKYPhaseNew[cbCounter] = vtkY.phase;

     cbVTKXFrequency[cbCounter] = vtkX.frequency.currentValue;
     cbVTKYFrequency[cbCounter] = vtkY.frequency.currentValue;

     cbVTKXFreqError[cbCounter] = vtkX.frequency.error;
     cbVTKYFreqError[cbCounter] = vtkY.frequency.error;

     cbVTKXdeltaPhase[cbCounter] = vtkX.deltaPhase;
     cbVTKYdeltaPhase[cbCounter] = vtkY.deltaPhase;

     cbVTKXIntegrator0[cbCounter] = vtkX.integral[0][0];
     cbVTKXIntegrator1[cbCounter] = vtkX.integral[1][0];

     cbVTKYIntegrator0[cbCounter] = vtkY.integral[0][0];
     cbVTKYIntegrator1[cbCounter] = vtkY.integral[1][0];

     cbXGuidePhasor[cbCounter] = phasorX.command;
     cbYGuidePhasor[cbCounter] = phasorY.command;
#endif

      /* increment the counter and wrap around if necessary,
       * it is a circular buffer after all. */
      if ( ++ cbCounter == CB_RECORD_NB )
      {
         cbCounter = 0;
      }
#ifdef MK
      tsdiff = cbTimeStamp - tsold;
      tsold = cbTimeStamp;
      guideInfo.sensedRate = 1/(double)tsdiff;
      sensedGuideRate = myround_nearest10((long)guideInfo.sensedRate);

      /*Have we changed guide modes?
       *
       * Set VTK system gain and phase accordingly.
       * */
      if (useDynamicVtk && checkGuideModeChange(sensedGuideRate) != ERROR) 
         guideInfo.rate = sensedGuideRate;
#endif 

   } /* end for(;;) FOREVER*/
}

/* ===================================================================== */

void slowTransmit (void)
{
   double timeStamp;
   int c[7];
   char cemtime[CEM_TIME_SIZE];

   /* before 10sep: localCommandBlock was the whole memMap structure - sheesh
      and *localPtr; was a pointer to a memMap */

   commandBlock localCommandBlock;
   commandBlock *localPtr;

   static long dayle = 1964;     /* used to check for corruption */

   localPtr = &localCommandBlock;

   /*
    * task prepares the slower items for transmission to the m2 system ready
    * for the guide task to pick up and transmit
    */

   for (;;)
   {
      if (localPtr == (commandBlock *) NULL)
      {
         errorLog("slowTransmit - localPtr is NULL\n", 1, ON);
         continue;
      }

      if (dayle != 1964)
         errlogPrintf("dayle detected corruption, dayle is %ld\n", dayle);

      epicsEventMustWait(slowUpdate);

      /* prepare the TCS demands, interpolate if following */

      if (scsState == MOVING && followOn == ON)
      {
         if (timeNow (&timeStamp) == OK)
         {
            epicsMutexLock(setPointFree);
            setPoint.xTiltA = getInterpolation (AX, timeStamp);
            setPoint.yTiltA = getInterpolation (AY, timeStamp);
            setPoint.xTiltB = getInterpolation (BX, timeStamp);
            setPoint.yTiltB = getInterpolation (BY, timeStamp);
            setPoint.xTiltC = getInterpolation (CX, timeStamp);
            setPoint.yTiltC = getInterpolation (CY, timeStamp);
            setPoint.zFocus = getInterpolation (Z, timeStamp);
            setPoint.xPosition = tcs.xPosition;
            setPoint.yPosition = tcs.yPosition;
            epicsMutexUnlock(setPointFree);
         }
         else
         {
            errorLog ("slowTransmit - error reading timeStamp", 1, ON);
         }
         if (timeNowC (TAI, 3, c) == 0) {
            sprintf (cemtime, "%d%2.2d%2.2dT%2.2d%2.2d%2.2d", c[0], c[1], c[2], c[3], c[4], c[5]); /* ISO8601 format */

         }

         else
         {
            strncpy (cemtime, "time read err", CEM_TIME_SIZE - 1);
         }

         if (mytimeshow) {

            epicsPrintf("mytime %s\n", cemtime);
         }

      }
      else if (followOn == ON) {
         errlogMessage("Warning: tcs demands ignored in follow mode!");
      }

      /* check for position update */

      if (positionUpdate == TRUE)
      {
         positionUpdate = FALSE;
         writeCommand(POSITION);
      }

      if (scstimeUpdate == 1) {
         writeCommand(SCS_TIME_UPDATE);
         scstimeUpdate = 0;
      }

      /* copy current SCS internal buffer to local buffer */
      epicsMutexLock(refMemFree);
      /* before 10sep: localCommandBlock = *(memMap *)scsPtr; */
      localCommandBlock = *(commandBlock *)&(scsPtr->page0);
      epicsMutexUnlock(refMemFree);

      /* write demands to reflective memory interface */

      if (simLevel == 0)
      {
         if (interlockFlag != ON)
         {

            switch (jogBeam)
            {
               case BEAMB:
                  scsBase->page0.AxTilt = 
                     (float) confine (setPoint.xTiltB, X_TILT_LIMIT, 
                           -X_TILT_LIMIT);
                  scsBase->page0.AyTilt = 
                     (float) confine (setPoint.yTiltB, Y_TILT_LIMIT,
                           -Y_TILT_LIMIT);
                  break;

               case BEAMC:
                  scsBase->page0.AxTilt = 
                     (float) confine (setPoint.xTiltC, X_TILT_LIMIT, 
                           -X_TILT_LIMIT);
                  scsBase->page0.AyTilt = 
                     (float) confine (setPoint.yTiltC, Y_TILT_LIMIT, 
                           -Y_TILT_LIMIT);
                  break;

               default:
                  scsBase->page0.AxTilt = 
                     (float) confine (setPoint.xTiltA, X_TILT_LIMIT, 
                           -X_TILT_LIMIT);
                  scsBase->page0.AyTilt = 
                     (float) confine (setPoint.yTiltA, Y_TILT_LIMIT, 
                           -Y_TILT_LIMIT);
            }
            scsBase->page0.BxTilt = (float) confine (setPoint.xTiltB, X_TILT_LIMIT, -X_TILT_LIMIT);
            scsBase->page0.ByTilt = (float) confine (setPoint.yTiltB, Y_TILT_LIMIT, -Y_TILT_LIMIT);
            scsBase->page0.CxTilt = (float) confine (setPoint.xTiltC, X_TILT_LIMIT, -X_TILT_LIMIT);
            scsBase->page0.CyTilt = (float) confine (setPoint.yTiltC, Y_TILT_LIMIT, -Y_TILT_LIMIT);
            scsBase->page0.xDemand = (float) setPoint.xPosition;
            scsBase->page0.yDemand = (float) setPoint.yPosition;
         }
         else
         {
            /*
             * interlocks set, adjust demands to current position
             */

            scsBase->page0.AxTilt = lockPosition.xTilt;
            scsBase->page0.BxTilt = lockPosition.xTilt;
            scsBase->page0.CxTilt = lockPosition.xTilt;
            scsBase->page0.xTiltGuide = 0.0;

            scsBase->page0.AyTilt = lockPosition.yTilt;
            scsBase->page0.ByTilt = lockPosition.yTilt;
            scsBase->page0.CyTilt = lockPosition.yTilt;
            scsBase->page0.yTiltGuide = 0.0;

            scsBase->page0.zFocusGuide = lockPosition.zFocus;

            scsBase->page0.xDemand = lockPosition.xPos;
            scsBase->page0.yDemand = lockPosition.yPos;
         }

         scsBase->page0.centralBaffle = localPtr->centralBaffle;
         scsBase->page0.deployBaffle = localPtr->deployBaffle;
         scsBase->page0.chopProfile = localPtr->chopProfile;
         scsBase->page0.chopFrequency = localPtr->chopFrequency;
         scsBase->page0.chopDutyCycle = localPtr->chopDutyCycle;
         scsBase->page0.xTiltTolerance = localPtr->xTiltTolerance;
         scsBase->page0.yTiltTolerance = localPtr->yTiltTolerance;
         scsBase->page0.zFocusTolerance = localPtr->zFocusTolerance;
         scsBase->page0.xPositionTolerance = 
            localPtr->xPositionTolerance;
         scsBase->page0.yPositionTolerance = 
            localPtr->yPositionTolerance;
         scsBase->page0.bandwidth = localPtr->bandwidth;
         scsBase->page0.xTiltGain = localPtr->xTiltGain;
         scsBase->page0.yTiltGain = localPtr->yTiltGain;
         scsBase->page0.zFocusGain = localPtr->zFocusGain;
         scsBase->page0.xTiltShift = localPtr->xTiltShift;
         scsBase->page0.yTiltShift = localPtr->yTiltShift;
         scsBase->page0.zFocusShift = localPtr->zFocusShift;
         scsBase->page0.xTiltSmooth = localPtr->xTiltSmooth;
         scsBase->page0.yTiltSmooth = localPtr->yTiltSmooth;
         scsBase->page0.zFocusSmooth = localPtr->zFocusSmooth;
         scsBase->page0.xTcsMinRange = localPtr->xTcsMinRange;
         scsBase->page0.yTcsMinRange = localPtr->yTcsMinRange;
         scsBase->page0.xTcsMaxRange = localPtr->xTcsMaxRange;
         scsBase->page0.yTcsMaxRange = localPtr->yTcsMaxRange;
         scsBase->page0.xPMinRange = localPtr->xPMinRange;
         scsBase->page0.yPMinRange = localPtr->yPMinRange;
         scsBase->page0.xPMaxRange = localPtr->xPMaxRange;
         scsBase->page0.yPMaxRange = localPtr->yPMaxRange;
         scsBase->page0.follower = localPtr->follower;
         scsBase->page0.foldir = localPtr->foldir;
         scsBase->page0.followersteps = localPtr->followersteps;
         scsBase->page0.offloader = localPtr->offloader;
         scsBase->page0.ofldir = localPtr->ofldir;
         scsBase->page0.offloadersteps = localPtr->offloadersteps;
         scsBase->page0.cbafdir = localPtr->cbafdir;
         scsBase->page0.cbsteps = localPtr->cbsteps;
         scsBase->page0.deployable_baffle = localPtr->deployable_baffle;
         scsBase->page0.dbafdir = localPtr->dbafdir;
         scsBase->page0.dbsteps = localPtr->dbsteps;
         scsBase->page0.xy_motor = localPtr->xy_motor;
         scsBase->page0.xydir = localPtr->xydir;
         scsBase->page0.xysteps = localPtr->xysteps;
         scsBase->page0.xyPositionDeadband = localPtr->xyPositionDeadband;
         strncpy (scsBase->page0.scsTime, cemtime, CEM_TIME_SIZE - 1);

      }
      else
      {
         /* simulation active */

         epicsMutexLock(m2MemFree);
         if (interlockFlag != ON)
         {
               switch(jogBeam)
               {
                  case BEAMB:
                     m2Ptr->page0.AxTilt = 
                        (float) confine (setPoint.xTiltB, 
                              X_TILT_LIMIT, -X_TILT_LIMIT);
                     m2Ptr->page0.AyTilt = 
                        (float) confine (setPoint.yTiltB, 
                              Y_TILT_LIMIT, -Y_TILT_LIMIT);
                     break;

                  case BEAMC:
                     m2Ptr->page0.AxTilt = 
                        (float) confine (setPoint.xTiltC, 
                              X_TILT_LIMIT, -X_TILT_LIMIT);
                     m2Ptr->page0.AyTilt = 
                        (float) confine (setPoint.yTiltC, 
                              Y_TILT_LIMIT, -Y_TILT_LIMIT);
                     break;

                  default:
                     m2Ptr->page0.AxTilt = 
                        (float) confine (setPoint.xTiltA, 
                              X_TILT_LIMIT, -X_TILT_LIMIT);
                     m2Ptr->page0.AyTilt = 
                        (float) confine (setPoint.yTiltA, 
                              Y_TILT_LIMIT, -Y_TILT_LIMIT);

            }

            m2Ptr->page0.BxTilt = (float) confine (setPoint.xTiltB, X_TILT_LIMIT, -X_TILT_LIMIT);
            m2Ptr->page0.ByTilt = (float) confine (setPoint.yTiltB, Y_TILT_LIMIT, -Y_TILT_LIMIT);
            m2Ptr->page0.CxTilt = (float) confine (setPoint.xTiltC, X_TILT_LIMIT, -X_TILT_LIMIT);
            m2Ptr->page0.CyTilt = (float) confine (setPoint.yTiltC, Y_TILT_LIMIT, -Y_TILT_LIMIT);
            m2Ptr->page0.xDemand = (float) setPoint.xPosition;
            m2Ptr->page0.yDemand = (float) setPoint.yPosition;
         }
         else
         {
            /*
             * interlocks set, adjust demands to current position
             */

            m2Ptr->page0.AxTilt = lockPosition.xTilt;
            m2Ptr->page0.BxTilt = lockPosition.xTilt;
            m2Ptr->page0.CxTilt = lockPosition.xTilt;
            m2Ptr->page0.xTiltGuide = 0.0;

            m2Ptr->page0.AyTilt = lockPosition.yTilt;
            m2Ptr->page0.ByTilt = lockPosition.yTilt;
            m2Ptr->page0.CyTilt = lockPosition.yTilt;
            m2Ptr->page0.yTiltGuide = 0.0;

            m2Ptr->page0.zFocusGuide = lockPosition.zFocus;

            m2Ptr->page0.xDemand = lockPosition.xPos;
            m2Ptr->page0.yDemand = lockPosition.yPos;
         }

         m2Ptr->page0.centralBaffle = localPtr->centralBaffle;
         m2Ptr->page0.deployBaffle = localPtr->deployBaffle;
         m2Ptr->page0.chopProfile = localPtr->chopProfile;
         m2Ptr->page0.chopFrequency = localPtr->chopFrequency;
         m2Ptr->page0.chopDutyCycle = localPtr->chopDutyCycle;
         m2Ptr->page0.xTiltTolerance = localPtr->xTiltTolerance;
         m2Ptr->page0.yTiltTolerance = localPtr->yTiltTolerance;
         m2Ptr->page0.zFocusTolerance = localPtr->zFocusTolerance;
         m2Ptr->page0.xPositionTolerance = 
            localPtr->xPositionTolerance;
         m2Ptr->page0.yPositionTolerance = 
               localPtr->yPositionTolerance;
         m2Ptr->page0.xyPositionDeadband = 
               localPtr->xyPositionDeadband;
         m2Ptr->page0.bandwidth = localPtr->bandwidth;
         m2Ptr->page0.xTiltGain = localPtr->xTiltGain;
         m2Ptr->page0.yTiltGain = localPtr->yTiltGain;
         m2Ptr->page0.zFocusGain = localPtr->zFocusGain;
         m2Ptr->page0.xTiltShift = localPtr->xTiltShift;
         m2Ptr->page0.yTiltShift = localPtr->yTiltShift;
         m2Ptr->page0.zFocusShift = localPtr->zFocusShift;
         m2Ptr->page0.xTiltSmooth = localPtr->xTiltSmooth;
         m2Ptr->page0.yTiltSmooth = localPtr->yTiltSmooth;
         m2Ptr->page0.zFocusSmooth = localPtr->zFocusSmooth;
         /* some missing here */
         epicsMutexUnlock(m2MemFree);
      }
   } // for(;;)
}

/* ===================================================================== */
/*
 * Function name:
 * tiltReceive
 *
 * Purpose:
 * When simulation mode is active, new data from the scs causes the semaphore
 * scsDataAvailable to be set. The task waits for this semaphore, calculates
 * the checksum of the received data block and examines the sequence numbers.
 * If all is well the data is copied to the simulation buffers else the data
 * is ignored and the sequence number NR is not updated
 *
 * Invocation:
 * taskSpawn ("tiltReceive", 48, VX_FP_TASK, 1000, (FUNCPTR) tiltReceive, 
 *            0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
 *
 * Parameters in:
 * None
 *
 * Parameters out:
 * None
 *
 * Return value:
 * None
 *
 * Globals:
 *      External functions:
 *              checkSum
 *
 *      External variables:
 *              scsDataAvailable
 *              m2MemFree
 *
 * Requirements:
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * Hisory:
 * 15-Oct-1997: Original(srp)
 *
 */

/* ===================================================================== */

void tiltReceive (void) 
{
   static long scsCheck;
   static long oldHeartbeat = 0;
   static long m2Heartbeat = 0;
   static long myTiltRxErrcount = 0;
   long localCommandCode = FAST_ONLY;
   //char message[200];


   for (;;) {
      if (epicsEventWaitWithTimeout(scsDataAvailable, RECEIVE_TIMEOUT) == epicsEventWaitOK) {

         epicsMutexMustLock(m2MemFree);

         scsCheck = checkSum ((void *) &m2Ptr->page0.NS, COMMAND_BLOCK_SIZE);

         if (scsCheck == m2Ptr->page0.checksum) {

            if (oldHeartbeat == m2Ptr->page0.heartbeat) {
               errorLog ("tiltReceive - SCS heartbeat stuck", 1, ON);
               if (myTiltRxErrcount++ < 100) epicsPrintf("tiltReceive - SCS heartbeat stuck\n");
            }

            oldHeartbeat = m2Ptr->page0.heartbeat;

            /* copy command into local buffer */
            localCommandCode = m2Ptr->page0.commandCode;

            /*
             * place command into FIFO buffer for slower reading from
             * state code
             */
             if (localCommandCode != FAST_ONLY) {
                if (epicsMessageQueueSendWithTimeout(
                        receiveQId, (char *) &localCommandCode,
                        sizeof(long), SEM_TIMEOUT) == ERROR)
                {
                   errorLog ("timeout appending command to receiveQId", 1, ON);
                   if (myTiltRxErrcount++ < 100) 
                      epicsPrintf("timeout appending command to receiveQId\n");
                }
             }

             /*
              * block received without error so update sequence
              * counter
              */
              m2Ptr->page1.NR = m2Ptr->page0.NS;
           }
           else {
              errorLog ("tiltReceive - checksums fail", 1, ON);
              if (myTiltRxErrcount++ < 100) 
                 epicsPrintf ("tiltReceive - checksums fail\n");
           }

           /* package status data to return to SCS */
           m2Ptr->page1.heartbeat = m2Heartbeat++;
           m2Ptr->page1.checksum = checkSum ((void *) &m2Ptr->page1.NR, 
                 STATUS_BLOCK_SIZE);

           epicsMutexUnlock(m2MemFree);

           /* flag status data available */
           epicsEventSignal(scsReceiveNow);

            /* print command to screen for testing */
            if ((localCommandCode > POSITION) & 
                  (debugLevel > DEBUG_MIN) &
                  (debugLevel <= DEBUG_MED)) {
               errlogPrintf ("sim receive command =  %s (%d)\n", 
                     m2CmdName[localCommandCode], (int)localCommandCode); 
            }
         }
      else {
         if(simLevel != 0) {
            errorLog ("tiltReceive - scsDataAvailable timeout", 1, ON);
            if (myTiltRxErrcount++ < 100) epicsPrintf("tiltReceive - scsDataAvailable timeout\n");
         }
      }
   }
}

/* ===================================================================== */
/*
 * Function name:
 * scsReceive
 *
 * Purpose: 
 * Receive status block from M2 and decipher contents
 *
 * Invocation:
 * receiveID = taskSpawn ("scsReceive", 48, VX_FP_TASK, 1000, 
 *                 (FUNCPTR) scsReceive, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
 *
 * Parameters in:
 *
 * Parameters out:
 *
 * Return value:
 *
 * Globals:
 *      External functions:
 *      None
 *
 *      External variables:
 *              ! refmemFree            SEM_ID
 *              ! m2MemFree             SEM_ID
 *              > scsReceiveNow         SEM_ID
 *              > currentBeam           int
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

/* ===================================================================== */

void scsReceive (void)
{
   long simCheck = 0xabcd;
   statusBlock localStatusBlock;

   for (;;)
   {
      if (epicsEventWaitWithTimeout(scsReceiveNow, RECEIVE_TIMEOUT) == epicsEventWaitOK)
      {
         if (simLevel == 0)
         {
            /* no simulation active, grab data from reflective memory */

            localStatusBlock = *(statusBlock *) & scsBase->page1;

            /* grab the engineering data for logging */
            /* only does anything if m2LogActive is set (via
             * primitive commands dm screen */

            // m2LoggerTask (scsBase);

            /* put the engineering data in a ring buffer
             * Right now, this is ALWAYS done. Later, could 
             * make it toggle-able via dm screen access and global
             * variable: m2CircBufferActive, for eg. */

#ifndef MK
             
            // m2CircBufferTask (scsBase);
#endif
            
         }
         else
         {
            /* simulation active, get data from m2 buffers */
            epicsMutexLock(m2MemFree);
            localStatusBlock = *(statusBlock *) & m2Ptr->page1;
            epicsMutexUnlock(m2MemFree);

            /* grab the engineering data for logging */
            // m2LoggerTask (m2Ptr);
         }

         /* check the received block */

         simCheck = 
            checkSum ((void *) &localStatusBlock.NR, STATUS_BLOCK_SIZE);

         /* for diagnostics, grab a whole frame with checksum */

         if(grab == 1)
         {
            safeBlock = localStatusBlock;
            grab = 0;
         }

         if (simCheck == localStatusBlock.checksum)
         {
            if (local.m2Heartbeat == localStatusBlock.heartbeat)
            {
               errorLog ("scsReceive - heartbeat stuck", 1, ON);
            }
            else
            {
               /* all is well, copy the checked data to the scs buffer */
               local.m2Heartbeat = localStatusBlock.heartbeat;

               epicsMutexLock(refMemFree);
               *(statusBlock *) & scsPtr->page1 = localStatusBlock;
               epicsMutexUnlock(refMemFree);


               if (local.NS != localStatusBlock.NR)
               {
                  errorLog ("scsReceive - frame unacknowledged", 1, ON);
               }

               if (local.testRequest == 0 && 
                     localStatusBlock.statusWord.flags.diagnosticsAvailable == 1)
               {
                  *(diagBlock *) & scsPtr->testResults = 
                     *(diagBlock *) & m2Ptr->testResults;
                  epicsEventSignal(diagnosticsAvailable);
               }

               local.testRequest = 
                  scsPtr->page1.statusWord.flags.diagnosticsAvailable;
            }
         }
         else
         {
#ifdef MK
            if ((debugLevel > DEBUG_NONE) & (debugLevel <= DEBUG_MED))
#else
            if (debugLevel > DEBUG_RESERVED1)
#endif
            {
               sprintf(errBuff, "checksum calc = %lx, received = %lx\n", 
                     simCheck, localStatusBlock.checksum);
               errlogPrintf("%s", errBuff);
            }
            errorLog ("scsReceive - checksum fail", 1, ON);
         }
      }
      else
      {
         errorLog ("scsReceive - scsReceiveNow timeout", 1, ON);
         errlogMessage("rscsReceive - scsReceiveNow timeout\n");
      }
   }
}

/* ===================================================================== */
/*
 * Function name:
 * checkTiltStatus
 *
 * Purpose:
 * Check for the setting of error bits in the m2 status word returned
 * via reflective memory. If found to be set, report error and change
 * health of the SCS system accordingly. Only report each error once.
 *
 * Invocation:
 * status = checkTiltStatus(void)
 *
 * Parameters in:
 * None
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
 *              > refMemFree    SEM_ID  mutex for access to ref mem
 *              > scsPtr        *memMap pointer to ref mem
 *
 * Requirements:
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 03-Feb-1998: Original(srp)
 * 01-Oct-1998: If no error conditions exist, return health to good
 *
 */

/* ===================================================================== */

int checkTiltStatus (void)
{
   bitFieldM2 tiltStatusWord;

   static struct
   {
      int health;
      int sensorLimit;
      int actuatorLimit;
      int thermalLimit;
      int mirrorDspInt;
      int vibDspInt;
      int recover;
   } errorLatch =
   {
      0,
      0,
      0,
      0,
      0,
      0,
      0
   };

   /* grab copy of m2 status word */

   epicsMutexLock(refMemFree);
   tiltStatusWord = scsPtr->page1.statusWord;
   epicsMutexUnlock(refMemFree);

   /* if a fault has arisen that wasn't present before, set health bad */
   if (tiltStatusWord.flags.health != 0 && errorLatch.health == 0)
   {
      errorLog ("M2 reports bad health", 0, ON);
      reportHealth (BAD, "M2 reports bad health");
      errorLatch.health = 1;
   }
   else if (tiltStatusWord.flags.sensorLimit != 0 &&  
         errorLatch.sensorLimit == 0)
   {
      errorLog ("M2 sensors out of range", 0, ON);
      reportHealth (BAD, "M2 sensors out of range");
      errorLatch.sensorLimit = 1;
   }
   else if (tiltStatusWord.flags.actuatorLimit != 0 && 
         errorLatch.actuatorLimit == 0)
   {
      errorLog ("M2 actuators out of range", 0, ON);
      reportHealth (BAD, "M2 actuators out of range");
      errorLatch.actuatorLimit = 1;
   }
   else if (tiltStatusWord.flags.thermalLimit != 0 && 
         errorLatch.thermalLimit == 0)
   {
      errorLog ("M2 thermal overload", 0, ON);
      reportHealth (BAD, "M2 thermal overload");
      errorLatch.thermalLimit = 1;
   }
   else if (tiltStatusWord.flags.mirrorDspInt != 0 && 
         errorLatch.mirrorDspInt == 0)
   {
      errorLog ("M2 invalid C31 interrupt on DSP board", 0, ON);
      reportHealth (BAD, "M2 invalid C31 interrupt on DSP board");
      errorLatch.mirrorDspInt = 1;
   }
   else if (tiltStatusWord.flags.vibDspInt != 0 && 
         errorLatch.vibDspInt == 0)
   {
      errorLog ("M2 invalid C31 interrupt on vib ctrl board", 0, ON);
      reportHealth (BAD, "M2 invalid C31 interrupt on vib ctrl board");
      errorLatch.vibDspInt = 1;
   }

   /* check whether an existing health condition has recovered */

   if (tiltStatusWord.flags.health == 0 && errorLatch.health == 1)
   {
      errorLatch.health = 0;
      errorLatch.recover = 1;
   }
   else if (tiltStatusWord.flags.sensorLimit == 0 &&  
         errorLatch.sensorLimit == 1)
   {
      errorLatch.sensorLimit = 0;
      errorLatch.recover = 1;
   }
   else if (tiltStatusWord.flags.actuatorLimit == 0 && 
         errorLatch.actuatorLimit == 1)
   {
      errorLatch.actuatorLimit = 0;
      errorLatch.recover = 1;
   }
   else if (tiltStatusWord.flags.thermalLimit == 0 && 
         errorLatch.thermalLimit == 1)
   {
      errorLatch.thermalLimit = 0;
      errorLatch.recover = 1;
   }
   else if (tiltStatusWord.flags.mirrorDspInt == 0 && 
         errorLatch.mirrorDspInt == 1)
   {
      errorLatch.mirrorDspInt = 0;
      errorLatch.recover = 1;
   }
   else if (tiltStatusWord.flags.vibDspInt == 0 && 
         errorLatch.vibDspInt == 1)
   {
      errorLatch.vibDspInt = 0;
      errorLatch.recover = 1;
   }

   /* if at least one error condition has recovered, check whether 
      health is now good */

   if(errorLatch.recover == 1)
   {
      errorLatch.recover = 0;
      if((errorLatch.health + errorLatch.sensorLimit +
               errorLatch.actuatorLimit + errorLatch.thermalLimit +
               errorLatch.mirrorDspInt + errorLatch.vibDspInt) == 0)
      {
         reportHealth(GOOD, "");
      }
   }

   return (OK);
}

/* ===================================================================== */
/*
 * Function name:
 * updateEventPage 
 *
 * Purpose:
 * Set global flags for in position and current beam. 
 * (not a page in RM anymore. Now just a data structure)
 *
 * Invocation:
 * status = updateEventPage(void);
 *
 * Parameters in:
 *              > scsInPosition int     set to 1 if scs is in position
 *              > beamPosition  int     set to current beam position
 *
 * Parameters out:
 * None
 *
 * Return value:
 *              < status        long    OK or ERROR
 * Globals:
 *      External functions:
 *              timeNow
 *
 *      External variables:
 *              None
 *
 * Requirements:
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 05-Dec-1997: Original(srp)
 * 06-Feb-1998: Add parameters for current beam and in position
 *
 */

/* ===================================================================== */
int updateEventPage (int scsInPosition, int scsPresentBeam)
{

   int myScsInPosition = 0;

   /* Protect eventData access to the eventData structure */
   epicsMutexLock(eventDataSem); 

   if (scsPresentBeam == BEAMA) {
      /*Only switch beam logic on high beam */
      currentBeam = BEAMA;

      if (!guideOnA) 
         myScsInPosition = 0;
      else 
         myScsInPosition = scsInPosition; 
   }
   else if (scsPresentBeam == BEAMB) {

      /*Only switch beam logic on high beam */
      currentBeam = BEAMB;

      if (!guideOnB) 
         myScsInPosition = 0;
      else
         myScsInPosition = scsInPosition;
   }
   else {
         myScsInPosition = 0; /* Anything other than Beam A|B is invalid*/
   }

   eventData.inPosition = myScsInPosition;
   eventData.currentBeam = currentBeam;
   epicsMutexUnlock(eventDataSem);


   nodeISR3 = -99;
   //semFlush(guideUpdateNow); /* what to do about this?  Hmmm... (mdw) */
   return (OK);
}

/* ===================================================================== */
/*
 * Function name:
 * writeCommand
 *
 * Purpose:
 * wrap up the writing of command codes to the message buffer in this
 * function together with the error checking to simplify the calls in
 * the individual functions
 *
 * Invocation:
 * status = writeCommand(commandCode);
 *
 * Parameters in:
 * >       long    command code
 *
 * Parameters out:
 * None
 *
 * Return value:
 * < status        long    OK
 * -1 unable to append message to queue
 * -2 unable to append message because interlocks active
 * Globals:
 *      External functions:
 *      None
 *
 *      External variables:
 *      < interlockFlag int     set to ON if interlocks active else OFF
 *
 * Requirements:
 *
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 *
 * History:
 * 05-Dec-1997: Original(srp)
 *
 */
/* ===================================================================== */

long writeCommand (const long command)
{
   long localCommand;

   localCommand = command;

   /* if interlocks are active, do not write message to queue */
   if (interlockFlag != ON)
   {
      // taskDelay(sysClkRateGet()/3);  
                epicsThreadSleep(1.0/3.0);

      if (epicsMessageQueueSendWithTimeout(commandQId, (char *)&localCommand, sizeof (long), SEM_TIMEOUT) == ERROR)
      {
         epicsPrintf ("failed to append command message %s to message queue\n", 
               m2CmdName[command]);
         return (-1);
      }
      return (0);
   }
   else
   {
      return (-2);
   }
}

/* ===================================================================== */
/*
 * Function name:
 * frameConvert
 * 
 * Purpose:
 * Convert demands to new frame of reference
 *
 * Invocation:
 * STATUS = frameConvert(converted *results, frameChange *f, z1, z2)
 *
 * Parameters in:
 * > frameChange *f pointer to structure of skew angles, scales and offsets
 * > converted   *result    pointer to structure to hold results
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
 * 11-Nov-1998: Original(srp)
 * 20-Nov-1998: Modify to include mutex field (srp)
 * 24-Nov-1998: Adapt to SCS usage
 */

/* ===================================================================== */
static int frameConvert (converted *result, 
                frameChange *f, 
                const double x, 
                const double y, 
                const double z)
{
    /* check that frame structure has been initialised */
    if (f == NULL || result == NULL) {
        errlogMessage("frame conversion pointers not initialised\n");
        return(ERROR);
    }

    /* access frame */
    epicsMutexLock(f->access); 
    /* perform the conversion */
    result->x = f->scaleX*(f->cosTheta*x - f->sinTheta*y) + f->offsetX;
    result->y = f->scaleY*(f->sinTheta*x + f->cosTheta*y) + f->offsetY;
    result->z = f->scaleZ * z;
    epicsMutexUnlock(f->access);

   return OK;
}

/* ===================================================================== */
/*
 * Function name:
 * iir_filter
 *
 * Purpose:
 * perform filter operation
 *
 * Invocation:
 * filtered output = iir_filter(input, *iir)
 *
 * Parameters in:
 *              > input double  sample value to be filtered
 *              > iir   *MATLAB pointer to structure of filter parameters
 *
 * Parameters out:
 * None
 *
 * Return value:
 *              < result        double  filtered output sample
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

/* ===================================================================== */

double iir_filter (const double input, MATLAB * iir)
{
  double *inPtr = NULL, *outPtr = NULL, *aPtr = NULL, *bPtr = NULL;

  iir->inHistory[0] = input;
  iir->outHistory[0] = 0;

  inPtr = iir->inHistory + iir->nb - 1;
  bPtr = iir->numerator + iir->nb - 1;
  outPtr = iir->outHistory + iir->na - 1;
  aPtr = iir->denominator + iir->na - 1;

  while (inPtr >= iir->inHistory) {
    *iir->outHistory += (*bPtr--) * (*inPtr--);
  }

  while (outPtr > iir->outHistory) {
    *iir->outHistory -= (*aPtr--) * (*outPtr--);
  }

  /* ripple histories ready for next sample */
  inPtr = iir->inHistory + iir->nb - 1;
  bPtr = iir->numerator + iir->nb - 1;
  outPtr = iir->outHistory + iir->na - 1;
  aPtr = iir->denominator + iir->na - 1;

     while (inPtr > iir->inHistory)
     {
          *inPtr = *(inPtr - 1);
          inPtr--;
     }

     while (outPtr > iir->outHistory)
     {
          *outPtr = *(outPtr - 1);
          outPtr--;
     }

     return (iir->outHistory[0]);
}


/* 16-Aug-2000: changed to display in chronological order 
   16-Aug-2000: added Ax,Ay,Bx,By position demands */
int saveCb ()
{
    char fileName[80];
    double fileTime;
    int i;
    FILE *pFile;

    
    if (timeNow(&fileTime) != OK)
    {
   errorLog ("saveCb - error reading timeStamp\n", 1, ON);
   return (ERROR);
    }

    sprintf(fileName, "./chop-guide-%d.log", (int)fileTime);
    pFile = fopen ( fileName, "w" );

    if ( pFile == (FILE *) NULL )
    {
        epicsPrintf ( "error opening file %s\n", fileName );
        return (ERROR);
    }

    for ( i = cbCounter ; i < CB_RECORD_NB ; i ++ ) 
    {
        //fprintf ( pFile, "  3 %f %f %f %ld %d\n", 
      //cbTime[i], cbP2Time[i], cbP2Interval[i], cbTick[i], i);
        fprintf ( pFile, "  3 %f %f %f %d\n", 
      cbTime[i], cbP2Time[i], cbP2Interval[i], i);
        fprintf ( pFile, "  4 %+4.2f %+4.2f %+4.2f\n", 
      cbXRawGuide[i], cbYRawGuide[i], cbZRawGuide[i]);
        fprintf ( pFile, "  7 %+4.2f %+4.2f %+4.2f\n", 
      cbXGuideBeforePID[i], cbYGuideBeforePID[i], 
                cbZGuideBeforePID[i]);
        fprintf ( pFile, "  8 %+4.2f %+4.2f %+4.2f\n", 
      cbXGuideAfterPID[i], cbYGuideAfterPID[i], cbZGuideAfterPID[i]);
        fprintf ( pFile, "  9 %+4.2f %+4.2f %+4.2f\n", 
      cbXGuideDemand[i], cbYGuideDemand[i], cbZGuideDemand[i]);
   fprintf ( pFile, " 10 %1d %1d %1d %1d\n",
                 cbCurBeam[i], cbApplyGuide[i], cbGuideOnA[i], cbInPos[i] );
        fprintf ( pFile, " 11 %+4.2f %+4.2f %4.2f %4.2f\n", 
      cbAXDemand[i], cbAYDemand[i], cbBXDemand[i], cbBYDemand[i]);

#ifdef MK
        fprintf ( pFile, " 13 %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f \n", 
      cbVTKXCommand[i], cbVTKXPhaseOld[i], cbVTKXPhaseNew[i], cbVTKXFrequency[i], cbVTKXIntegrator0[i], cbVTKXIntegrator1[i], cbXGuideBeforePID[i], cbXGuidePhasor[i], cbXGuideAfterPID[i], cbXGuideDemand[i]);
        fprintf ( pFile, " 15 %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f \n", 
      cbVTKYCommand[i], cbVTKYPhaseOld[i], cbVTKYPhaseNew[i], cbVTKYFrequency[i], cbVTKYIntegrator0[i], cbVTKYIntegrator1[i], cbYGuideBeforePID[i], cbYGuidePhasor[i], cbYGuideAfterPID[i], cbYGuideDemand[i]);
#endif
    }

    /* write from beginning of array to newest data */
    for ( i = 0 ; i < cbCounter ; i ++ )
    {
        //fprintf ( pFile, "  3 %f %f %f %ld %d\n", 
   //   cbTime[i], cbP2Time[i], cbP2Interval[i], cbTick[i], i);
        fprintf ( pFile, "  3 %f %f %f %d\n", 
      cbTime[i], cbP2Time[i], cbP2Interval[i], i);
        fprintf ( pFile, "  4 %+4.2f %+4.2f %+4.2f\n", 
      cbXRawGuide[i], cbYRawGuide[i], cbZRawGuide[i]);
        fprintf ( pFile, "  7 %+4.2f %+4.2f %+4.2f\n", 
      cbXGuideBeforePID[i], cbYGuideBeforePID[i], 
                cbZGuideBeforePID[i]);
        fprintf ( pFile, "  8 %+4.2f %+4.2f %+4.2f\n", 
      cbXGuideAfterPID[i], cbYGuideAfterPID[i], cbZGuideAfterPID[i]);
        fprintf ( pFile, "  9 %+4.2f %+4.2f %+4.2f\n", 
      cbXGuideDemand[i], cbYGuideDemand[i], cbZGuideDemand[i]);
   fprintf ( pFile, " 10 %1d %1d %1d %1d\n",
                 cbCurBeam[i], cbApplyGuide[i], cbGuideOnA[i], cbInPos[i] );
        fprintf ( pFile, " 11 %+4.2f %+4.2f %4.2f %4.2f\n", 
      cbAXDemand[i], cbAYDemand[i], cbBXDemand[i], cbBYDemand[i]);

#ifdef MK
        fprintf ( pFile, " 13 %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f \n", 
      cbVTKXCommand[i], cbVTKXPhaseOld[i], cbVTKXPhaseNew[i], cbVTKXFrequency[i], cbVTKXIntegrator0[i], cbVTKXIntegrator1[i], cbXGuideBeforePID[i], cbXGuidePhasor[i], cbXGuideAfterPID[i], cbXGuideDemand[i]);
        fprintf ( pFile, " 15 %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f %+7.5f \n", 
      cbVTKYCommand[i], cbVTKYPhaseOld[i], cbVTKYPhaseNew[i], cbVTKYFrequency[i], cbVTKYIntegrator0[i], cbVTKYIntegrator1[i], cbYGuideBeforePID[i], cbYGuidePhasor[i], cbYGuideAfterPID[i], cbYGuideDemand[i]);
#endif

    }

    fclose (pFile);
    return (OK);
}


/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * newDfilter
 *
 * INVOCATION:
 * double newSample
 * int Id
 *
 * double   newDfilter(double newSample, int Id)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > double newSample       - latest data sample
 * > int    iD              - identification of zernikes (0 = xtilt, 1 = ytilt,
 *                            2 = focus)
 *
 * FUNCTION VALUE:
 * double     returns current filtered value
 *
 * PURPOSE:
 * Filter the data in accordance with the IIR filter coefficients specified
 *
 * DESCRIPTION:
 * The function performs a low pass butterworth filter on the supplied
 * data. A history array is maintained for each zernikes identified by the
 * index Id.
 * The cutoff frequency is set in detControl.c (detSigInitGain CAD) and
 * coefficients of the filter are computed according the cutoof frequency and
 * exposure time.
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
 * 08-Dec-2000  Coeff are computing in detControl.c and the cutoffFreq set by
 *              the user
 * 28-Oct-1998  Original version - Sean Prior
 *-
 */

double newDfilter
   (
   double newSample,
   int Id
   )
{
   int i = 0;
   double sum = 0;

   /* put new sample into the array */

   sampleData[2][Id] = newSample;

   /* multiply samples by coefficients and accumulate */

   for(i=0; i < 5; i++)
      sum += sampleData[i][Id]*coeffData[i][Id];

   /* ripple samples ready for next call */

   sampleData[4][Id] = sampleData[3][Id];
   sampleData[3][Id] = sampleData[2][Id];
   sampleData[1][Id] = sampleData[0][Id];
   sampleData[0][Id] = sum;

   /*printf ( "sum[%d] = %f\n" , Id, sum );*/
   return(sum);
}


#ifdef MK
/****
 *
 *
 */
void phasorShowX() {

    _phasorShow(&phasorX);
}

void phasorShowY() {

    _phasorShow(&phasorY);
}

void vtkShowX() {
    _vtkShow(&vtkX);
}

void vtkShowY() {
    _vtkShow(&vtkY);
}

void vtkResetX() {
    /**
     * epicsPrintf("VTK Reseting X...\n");
     */
    _vtkReset(&vtkX);
}

void vtkResetY() {
    /*
     * epicsPrintf("VTK Reseting Y...\n");
     */
    _vtkReset(&vtkY);
}

void vtkxon() {

    epicsPrintf("VTK Turning on X...\n");
    vibrationXTrackOn = 1;
    _vtkReset(&vtkX);
}

void vtkxoff() {
    epicsPrintf("VTK Turning off X...\n");
    vibrationXTrackOn = 0;

}

void vtkyon() {

    epicsPrintf("VTK Turning on Y...\n");
    vibrationYTrackOn = 1;
    _vtkReset(&vtkY);
}

void vtkyoff() {
    epicsPrintf("VTK Turning off Y...\n");
    vibrationYTrackOn = 0;
}

void swxon() {
    epicsPrintf("SW Turning on X...\n");
    phasorXApply = 1;
    xTiltGuideSimScale = 1.0;
}

void swxoff() {
    epicsPrintf("SW Turning off X...\n");
    phasorXApply = 0;
    xTiltGuideSimScale = 1.0;
}

void swyon() {
    epicsPrintf("SW Turning on Y...\n");
    phasorYApply = 1;
    yTiltGuideSimScale = 1.0;
}

void swyoff() {
    epicsPrintf("SW Turning off Y...\n");
    phasorYApply = 0;
    yTiltGuideSimScale = 1.0;
}

void setVtkX(Vtk *vtkx) {

    
}

Vtk* getVtkX(void) {
    return &vtkX;
}

void setVtkY(Vtk *vtky) {

}


Vtk* getVtkY(void) {
    return &vtkY;

}

Phasor* getPhasorX(void) {
    return &phasorX;
}

Phasor* getPhasorY(void) {
    return &phasorY;
}

long srmisscount;
int checkGuideModeChange( long mode) {
    
    static int currentmode = GUIDE_200_HZ; /*default mode is 200 Hz*/
    
    /*Only change if new mode is different*/
    if(mode == currentmode) {
        return(ERROR);
    }

    switch (mode) {
        case GUIDE_200_HZ:
            vtkX.Fs = 198.9; 
            vtkX.scale = 1.758;
            vtkX.angle = -15.9;

            vtkY.Fs = 198.9; 
            vtkY.scale = 1.758;
            vtkY.angle = -15.9;

            break;

        case GUIDE_100_HZ:    
            vtkX.Fs = 99.5; 
            vtkX.scale = 1.591;
            vtkX.angle = 15.3;

            vtkY.Fs = 99.5; 
            vtkY.scale = 1.591;
            vtkY.angle = 15.3;

            break;

        case GUIDE_50_HZ:    

            vtkX.Fs = 49.8; 
            vtkX.scale = 1.637;
            vtkX.angle = 47.1;

            vtkY.Fs = 49.8; 
            vtkY.scale = 1.637;
            vtkY.angle = 47.1;
            break;

        case GUIDE_20_HZ:    

            vtkX.Fs = 19.9; 
            vtkX.scale = 1.758;
            vtkX.angle = -15.9;

            vtkY.Fs = 19.9; 
            vtkY.scale = 1.758;
            vtkY.angle = -15.9;

            break;

        default:
            /*
             * 
             * sprintf(errBuff, "checkGuideModeChange: unknown mode: %d, input srate=%f\n", mode, srate);
             *
             * logMsg("%s", (int)errBuff, 0, 0, 0, 0, 0);
             */

            srmisscount++;

            /*Unsuported modes return early and don't change VTK.*/
            return(ERROR);
    }
    
    guideInfo.vtkXdata[0] = vtkX.scale;
    guideInfo.vtkXdata[1] = vtkX.angle;
    guideInfo.vtkXdata[2] = vtkX.Fs;

    guideInfo.vtkYdata[0] = vtkY.scale;
    guideInfo.vtkYdata[1] = vtkY.angle;
    guideInfo.vtkYdata[2] = vtkY.Fs;

    /*Reinitialize VTK with new settings*/
    vtkResetX();
    vtkResetY();
    vtkInit(&vtkX);
    vtkInit(&vtkY);

    currentmode = mode;
    return(OK);
}

#endif

