/* $id: scs_st.st,v 1.14 2006/02/15 12:36:04 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
   *
   * FILENAME
   * -------- 
   * scs_st.stpp 
   * 
   * PURPOSE
   * -------
   * State notation code for control of the SCS state machine
   *
   * 
   * STATESET NAME(S)
   * ----------------
   * CADprocessor         - Examine and act of commands received from CAD records
   * followDemandProcessing   - Update and apply beam positions whilst in moving state
   * monitorSad           - Updates the SAD database at a refresh rate of 1Hz
   * monitorProcess       - Collate parameters and refresh the SAD database
   * 
   * rebootScs            - Waits for and executes the reboot command
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
   * 27-Jul-1996: Original (srp)
   * 30-Jul-1996: Change M2 command and response to separate handler
   * 31-Jul-1996: Boot routine enhanced with checks for response timeout
   *      and lockup within routines
   * 07-Aug-1996: Addition of move and stop handlers
   * 11-Aug-1996: Addition of chop configuration and control handler
   * 14-Aug-1996: Addition of monitor process
   * 21-Aug-1996: Remove redundant states and test comments
   * 23-Aug-1996: Modify stop command handling such that the guide corrections
   *      are zeroed
   * 05-Nov-1996: Client ID functions stripped out
   * 06-Nov-1996: Change channel names to reflect non hierarchical approach
   *            e.g. top:test replaces top:testblock:test
   * 25-Nov-1996: Modify follow and chop to update demands on newBeamA only
   * 26-Nov-1996: Add code to handle the follow command
   * 08-Dec-1996: Remove individual records for receiving positions demands
   * 10-Dec-1996: Modify move/follow handling.  Remove chopdemandprocessing
   *      (now covered by move/follow).
   * 08-Aug-1997: Add reboot call from startReboot
   * 14-Aug-1997: Add pvload of default data from startInit
   * 29-Aug-1997: Add moveCurve handling, adapted from actuator handling
   * 24-Sep-1997: Modify order of commands in startInit
   * 21-Oct-1997: Tidy up layout
   * 23-Oct-1997: Remove redundant code from startReboot, nothing will execute
   *      after the reboot command is issued
   *      FOLLOWING and CHOPPING should not be a separate states, change
   *      to use the followS and chopIsOn flags respectively when the state
   *      is MOVING 
   * 24-Oct-1997: Remove references to guideOverride and guidestate, use guideOn instead
   * 10-Feb-1998: Reset guide config widget
   * 11-Feb-1998: Remove debug/datum/guidecontrol processing, now on capfast diagram
   * 25-Feb-1998: remove all references to moveCurve
   * 23-Jun-1998: Do not set health bad for command completion problems.
   * 24-Jun-1998: Modify CAR BUSY setting for moveBaffle
   * 29-Jun-1998: Ensure jogBeam is reset to BEAMA before chopping starts
   * 03-Jul-1998: Remove BUST CAR setting from chop functions, now done by CAD
   * 19-Aug-1998: State set 'monitorProcess' found to be largely redundant as all
   *              but one of the signals is brought out to epics by dmDrive
   * 15-Dec-1998: Changed strings for centralNames[] and deployNames[]
   * 15-Dec-1998: Increased timeout for init to 1080.0 (about 6 min)
   * 28-Jan-1999: Changed sequence of commands
   * 09-Feb-1999: Reduced timeout for init to 540.0 (about 3 min)
   * 16-Feb-1999: Automatic enabling of mirror control
   * 17-Feb-1999: Added automatic enabling of mirror control to actuatorWaitFor-
   *      Coincidence and to waitForCoincidence (in follow)
   * 07-May-1999: Added RCS id
   * 03-Nov-1999: Added new state sets: moveBaffle and rebootScs
   *      to separate out these two commands so that they will not
   *      be missed (if the SNL is in busy in another state).
   * 18-Nov-1999: Fix to states: proceedWithMove, proceedWithActMove and
   *      proceedWithFollow to handle the case when the SERVOs fail
   *      to come on. Timeout of SERVO_OFF_TIMEOUT added. Code now
   *      only waits this long, then leaves the state.
   * 28-Jun-2000: Remove startChopConfig and associated PVs. Do steps in
   *      CADchopConfig START case, instead
   * 06-Jul-2000: Remove state set for baffles. Do steps in CADmoveBaffle
   *      instead.
   *
   */
   /* INDENT ON */
   /* ===================================================================== */

   program scs_st

   option -a;  /* synchronous      */
   option +c;  /* wait for connections */
   option +d;  /* debug messages on    */
   option +l;  /* C compiler messages  */
   option -r;  /* not reentrant    */
   option +w;  /* SNC warnings     */
   option +e;  /* new event flag mode  */

   %%#include "archive.h"      /* For scsLogDestruct, loggingNow, refMemFree */
   %%#include "chop.h"         /* For chopIsOn, jogBeam, instruments */
   %%#include "chopControl.h"  /* For eventSem */
   %%#include "config.h"       /* For depBaffle, cenBaffle */
   %%#include "control.h"      /* For writeCommand, scsPtr, scsBase, filtered,
                                  followOn, controller */
   %%#include "guide.h"        /* For guideOn, setPoint, setPointFree */
   %%#include "interp.h"       /* For AX, AY, ..., Z axis identifiers */
   %%#include "scs.h"          /* For resetFirstFollowDemand */
   %%#include "utilities.h"    /* For checksum, tcs2m2, doPvLoad, pvLoadComplete */
   %%#include "xycom.h"        /* For eventConfig */
   %%#include "testFunctions.h"        /* For startGuideSim, endGuideSim */
   %%#include "interlock.h"        /* For startGuideSim, endGuideSim */


   %%#include <stdlib.h>
   %%#include <string.h>
   %%#include <math.h>

   %%#include <car.h>

   %%#include <logLib.h>
   %%#include <rebootLib.h>
   %%#include <sysLib.h>

   %%#define CMD_RESPONSE_TIMEOUT    40.0 /* command acknowledgement timeout in 
                                             units equal to ~0.33 seconds */
   %%#define INIT_COMPLETE_TIMEOUT 1200.0 /* init execution timeout in units
                                             equal to ~0.33 seconds */
   %%#define TIMEOUT                200.0 /* normal command execution timeout in units equal to ~0.33 seconds */
   %%#define CHOP_TIMEOUT                100.0 /* chop timeout */
   %%#define MOVE_TIMEOUT           800.0 /* move command execution timeout in units equal to ~0.33 seconds */
   %%#define SERVO_OFF_TIMEOUT       10.0 /* Time for sevos to switched off */

   %%#define COUNTER_LIMIT           1    /* Times to count */

   /* Define general status indicators */

   %%#define STATUS_OK       0
   %%#define STATUS_WARNING  1
   %%#define STATUS_ERROR    2

   %{

      /* declare globals */

      /*
         char    *topEndNames[] =
         {
         "F6",
         "F16",
         NULL
         };
       */

      static struct
      {
         long    oldNR;
         long    newNR;
         long    oldHeartbeat;
         long    newHeartbeat;
      }handshake;

      /* define park position for the m2 mechanism */
      /* Goofy that these are here. They should be moved to SCSInit.dat */

      static double   xTiltPark   =   0.0;
      static double   yTiltPark   =   0.0;
      static double   zFocusPark  =   0.0;
      static double   xPosPark    =   2125.0;
      static double   yPosPark    =   1500.0;

      static location parkPosition;
      static location xyInitPosition;

      static double   xTiltAdef;
      static double   yTiltAdef;
      static double   xTiltBdef;
      static double   yTiltBdef;
      static double   xTiltCdef;
      static double   yTiltCdef;
      static double   zFocusDef;
      static double   xPosDef;
      static double   yPosDef;
      static double   xPosTcsDef;
      static double   yPosTcsDef;
      static double   xyDeadband;

      static long     myOldGuideSimState = 0;

   }%


/* ===================================================================== */
/* connecting to records holding the instrument configuration */

string  scs0name;
assign  scs0name to "{I}scs0.ZRST";

string  scs0port;
assign  scs0port to "{I}scs0.ONST";

string  scs0capability;
assign  scs0capability  to "{I}scs0.TWST";

string  ics0name;
assign  ics0name to "{I}ics0.ZRST";

string  ics0port;
assign  ics0port to "{I}ics0.ONST";

string  ics0capability;
assign  ics0capability  to "{I}ics0.TWST";

string  ics1name;
assign  ics1name to "{I}ics1.ZRST";

string  ics1port;
assign  ics1port to "{I}ics1.ONST";

string  ics1capability;
assign  ics1capability  to "{I}ics1.TWST";

string  ics2name;
assign  ics2name to "{I}ics2.ZRST";

string  ics2port;
assign  ics2port to "{I}ics2.ONST";

string  ics2capability;
assign  ics2capability  to "{I}ics2.TWST";

string  ics3name;
assign  ics3name to "{I}ics3.ZRST";

string  ics3port;
assign  ics3port to "{I}ics3.ONST";

string  ics3capability;
assign  ics3capability  to "{I}ics3.TWST";

string  ics4name;
assign  ics4name to "{I}ics4.ZRST";

string  ics4port;
assign  ics4port to "{I}ics4.ONST";

string  ics4capability;
assign  ics4capability  to "{I}ics4.TWST";

/* ===================================================================== */

/* records holding frame of reference correction factors are held in */
/* a separate database {F}. */

double tiltSkew;
assign tiltSkew to "{F}tiltSkew.VAL";

double tiltOffsetX;
assign tiltOffsetX to "{F}tiltOffsetX.VAL";

double tiltOffsetY;
assign tiltOffsetY to "{F}tiltOffsetY.VAL";

double posSkew;
assign posSkew to "{F}posSkew.VAL";

double posOffsetX;
assign posOffsetX to "{F}posOffsetX.VAL";

double posOffsetY;
assign posOffsetY to "{F}posOffsetY.VAL";

double gaosSkew;
assign gaosSkew to "{F}gaosSkew.VAL";

double gaosOffsetX;
assign gaosOffsetX to "{F}gaosOffsetX.VAL";

double gaosOffsetY;
assign gaosOffsetY to "{F}gaosOffsetY.VAL";

double gyroSkew;
assign gyroSkew to "{F}gyroSkew.VAL";

double gyroOffsetX;
assign gyroOffsetX to "{F}gyroOffsetX.VAL";

double gyroOffsetY;
assign gyroOffsetY to "{F}gyroOffsetY.VAL";

double focusScaling;
assign focusScaling to "{F}focusScaling.VAL";

double currFocus;
/* ===================================================================== */

/* monitor command progress from m2 */

long testResponse;
assign testResponse to "{T}byte0:b5.VAL";
monitor testResponse;

long rebootResponse;
assign rebootResponse to "{T}byte0:b4.VAL";
monitor rebootResponse;

long initResponse;
assign initResponse to "{T}byte0:b3.VAL";
monitor initResponse;

long chopResponse;
assign chopResponse to "{T}byte0:b7.VAL";
monitor chopResponse;

long servoOnStatus;
assign servoOnStatus to "{T}byte3:b6.VAL";
monitor servoOnStatus;

long XYmovingStatus;
assign XYmovingStatus to "{T}byte1:b4.VAL";
monitor XYmovingStatus;

/* ==================================================================== */

/* The following groups of parameters access the channels for each of   */
/* the CAD records                          */

/* Test record connections
 * The test record has only the standard template connections with 
 * no other parameters to be considered
 */

long testCadStart;
assign testCadStart to "{T}testP:start.VAL";
monitor testCadStart;

evflag testCadChange;           /* flag to detect start or stop */
sync testCadStart to testCadChange; /* forward link activation  */

long testCarIn;             /* input to testC CAR       */
assign testCarIn to "{T}testC.IVAL";

long testCarErrIn;          /* error input to testC CAR     */
assign testCarErrIn to "{T}testC.IERR";

string testCarMessIn;           /* message input to testC CAR   */
assign testCarMessIn to "{T}testC.IMSS";

string testResultString;        /* field to report test results */
assign testResultString to "{T}fault0.VAL";
/* ==================================================================== */

/* Init record connections
 * The Init record has only the template set of CAD connections with 
 * no additional arguments to consider
 */

long initCadStart;
assign initCadStart to "{T}initP:start.VAL";
monitor initCadStart;

evflag initCadChange;
sync initCadStart to initCadChange;

long initCarIn;                 /* input to initC CAR */
assign initCarIn to "{T}initC.IVAL";

long initCarErrIn;              /* error input to initC CAR */
assign initCarErrIn to "{T}initC.IERR";

string initCarMessIn;               /* message input to initC CAR */
assign initCarMessIn to "{T}initC.IMSS";
/* ==================================================================== */

/* InitXY record connections
 * The Init record has only the template set of CAD connections with 
 * no additional arguments to consider
 */

long initXYCadStart;
assign initXYCadStart to "{T}initPXY:start.VAL";
monitor initXYCadStart;

evflag initXYCadChange;
sync initXYCadStart to initXYCadChange;

long initXYCarIn;                 /* input to initC CAR */
assign initXYCarIn to "{T}initCXY.IVAL";

long initXYCarErrIn;              /* error input to initC CAR */
assign initXYCarErrIn to "{T}initCXY.IERR";

string initXYCarMessIn;               /* message input to initC CAR */
assign initXYCarMessIn to "{T}initCXY.IMSS";


/* ===================================================================== */

/* When triggered these channels initialise the relevant records on their
 * respective capfast schematics. There should be a better way of doing this
 * but I have had no success setting default values in the record properties
 */

string source0;
assign source0 to "{T}source.ZRST";

string source1;
assign source1 to "{T}source.ONST";

string source2;
assign source2 to "{T}source.TWST";

string source3;
assign source3 to "{T}source.THST";

string source4;
assign source4 to "{T}source.FRST";

string source5;
assign source5 to "{T}source.FVST";

string source6;
assign source6 to "{T}source.SXST";

string inst0;
assign inst0 to "{T}syncsource.ZRST";

string inst1;
assign inst1 to "{T}syncsource.ONST";

string inst2;
assign inst2 to "{T}syncsource.TWST";

string inst3;
assign inst3 to "{T}syncsource.THST";

string inst4;
assign inst4 to "{T}syncsource.FRST";

string inst5;
assign inst5 to "{T}syncsource.FVST";

string inst6;
assign inst6 to "{T}syncsource.SXST";

/* ===================================================================== */

/* Reboot record connections
 * Reboot requires only the template set of connections with no additional
 * parameters to consider
 */

long rebootCadStart;
assign rebootCadStart to "{T}rebootP:start.VAL";
monitor rebootCadStart;

evflag rebootCadChange;
sync rebootCadStart to rebootCadChange;

long rebootCarIn;               /* input to rebootC CAR */
assign rebootCarIn to "{T}rebootC.IVAL";

long rebootCarErrIn;                /* error input to rebootC CAR */
assign rebootCarErrIn to "{T}rebootC.IERR";

string rebootCarMessIn;             /* message input to     */
assign rebootCarMessIn to "{T}rebootC.IMSS";    /* rebootC CAR      */


/* ===================================================================== */

long clearGuideFocusCadStart;
assign clearGuideFocusCadStart to "{T}clearGuideFocusP:start.VAL";
monitor clearGuideFocusCadStart;

evflag clearGuideFocusCadChange;
sync clearGuideFocusCadStart to clearGuideFocusCadChange;

long clearGuideFocusCarIn;               /* input to clearGuideFocusC CAR */
assign clearGuideFocusCarIn to "{T}clearGuideFocusC.IVAL";

long clearGuideFocusCarErrIn;          /* error input to clearGuideFocusC CAR */
assign clearGuideFocusCarErrIn to "{T}clearGuideFocusC.IERR";

string clearGuideFocusCarMessIn;      /* mess. input to clearGuideFocusC CAR */
assign clearGuideFocusCarMessIn to "{T}clearGuideFocusC.IMSS";

/* ===================================================================== */

/* Move record connections
 * The move record has the template set of connections to the CAD record
 * together with a position specification block of parameters which although
 * being supplied as continuous data irrespective of the CAD state are
 * considered as part of the move data set
 */

long moveCadStart;
assign moveCadStart to "{T}moveP:start.VAL";
monitor moveCadStart;

evflag moveCadChange;
sync moveCadStart to moveCadChange;

long moveCarIn;                 /* input to moveC CAR */
assign moveCarIn to "{T}moveC.IVAL";

long moveCarErrIn;              /* error input to moveC CAR */
assign moveCarErrIn to "{T}moveC.IERR";

string moveCarMessIn;               /* message input to moveC CAR */
assign moveCarMessIn to "{T}moveC.IMSS";

double moveAXTilt;
assign moveAXTilt to "{T}move.VALA";

double moveAYTilt;
assign moveAYTilt to "{T}move.VALB";

double moveBXTilt;
assign moveBXTilt to "{T}move.VALC";

double moveBYTilt;
assign moveBYTilt to "{T}move.VALD";

double moveCXTilt;
assign moveCXTilt to "{T}move.VALE";

double moveCYTilt;
assign moveCYTilt to "{T}move.VALF";

double moveZFocus;
assign moveZFocus to "{T}move.VALG";

double moveXPosition;
assign moveXPosition to "{T}move.VALH";

double moveYPosition;
assign moveYPosition to "{T}move.VALI";

long moveDir;
assign moveDir to "{T}move.DIR";

long moveBaffleDir;
assign moveBaffleDir to "{T}moveBaffle.DIR";

long chopConfigDir;
assign chopConfigDir to "{T}chopConfig.DIR";


double moveXInputPos;
assign moveXInputPos to "{T}move.G";

double moveYInputPos;
assign moveYInputPos to "{T}move.H";
/* ===================================================================== */

/* PID controller record connections */

long pidDir;
assign pidDir to "{T}setController.DIR";

/* VTK control record connections*/

long vtkXCtrl;
assign vtkXCtrl to "{T}X:vtkOn.VAL";
long vtkXCtrlDir;
assign vtkXCtrlDir to "{T}X:vtkControl.DIR";

long vtkYCtrl;
assign vtkYCtrl to "{T}Y:vtkOn.VAL";
long vtkYCtrlDir;
assign vtkYCtrlDir to "{T}Y:vtkControl.DIR";

long vtkXDir;
assign vtkXDir to "{T}X:setVTKControl.DIR";

long vtkYDir;
assign vtkYDir to "{T}Y:setVTKControl.DIR";

double vtkXSampleRate;
assign vtkXSampleRate to "{T}X:setVTKControl.VALA";
double vtkXScale;
assign vtkXScale to "{T}X:setVTKControl.VALF";
double vtkXAngle;
assign vtkXAngle to "{T}X:setVTKControl.VALG";

double vtkYSampleRate;
assign vtkYSampleRate to "{T}Y:setVTKControl.VALA";
double vtkYScale;
assign vtkYScale to "{T}Y:setVTKControl.VALF";
double vtkYAngle;
assign vtkYAngle to "{T}Y:setVTKControl.VALG";

long measuredSampleRate;
assign measuredSampleRate to "{T}decimator.VALS";
monitor measuredSampleRate; 

evflag vtkChange;
sync measuredSampleRate to vtkChange;

/* Synthetic wave, phasor, control record connections*/

long phasorXDir;
assign phasorXDir to "{T}X:setPhasorControl.DIR";

long phasorYDir;
assign phasorYDir to "{T}Y:setPhasorControl.DIR";

/* ===================================================================== */

/* Actuator record connections */

long actuatorCadStart;
assign actuatorCadStart to "{T}moveActuatorP:start.VAL";
monitor actuatorCadStart;

evflag actuatorCadChange;
sync actuatorCadStart to actuatorCadChange;

long actuatorCarIn;             /* input to actuatorC CAR */
assign actuatorCarIn to "{T}moveActuatorC.IVAL";

long actuatorCarErrIn;                  /* error input to */
assign actuatorCarErrIn to "{T}moveActuatorC.IERR"; /* actuatorC CAR  */

string actuatorCarMessIn;               /* message input to */
assign actuatorCarMessIn to "{T}moveActuatorC.IMSS";    /* actuatorC CAR    */

double actXTilt;
assign actXTilt to "{T}moveActuator.VALD";

double actYTilt;
assign actYTilt to "{T}moveActuator.VALE";

double actZFocus;
assign actZFocus to "{T}moveActuator.VALF";

/* ===================================================================== */

/* Move demand parameters defining beam A. This is the default beam when not
 * chopping.  The application times atappl1 and atappl2 must be identical as
 * a check that the record has not been updated whilst being read.  The writing of
 * the atsent record is monitored to and triggers reading of the data set
 */

double atsent;              /* time of sending data set */
assign atsent to "{T}followA.VALA";
monitor atsent;

evflag newBeamA;            /* atsent monitored to trigger updates */
sync atsent to newBeamA;

/* ===================================================================== */

/* Follow record connections
 * The follow record has the template set of connections to the CAD record
 * but uses the move records beam position records
 */

long followCadStart;
assign followCadStart to "{T}followP:start.VAL";
monitor followCadStart;

evflag followCadChange;
sync followCadStart to followCadChange;

long followCarIn;               /* input to followC CAR */
assign followCarIn to "{T}followC.IVAL";

long followCarErrIn;                /* error input to followC CAR */
assign followCarErrIn to "{T}followC.IERR";

string followCarMessIn;             /* message input to followC CAR */
assign followCarMessIn to "{T}followC.IMSS";

long arrayS;                    /* sanity check results */
assign arrayS to "{T}followA.VALB";     /* of follow demands    */

long activeC;
assign activeC to "{T}activeC.IVAL";

long currentId;

long followS;
assign followS to "{T}followS.VAL";

/* ===================================================================== */

/* Stop record connections
 * The stop CAD uses only the template channels with no additional
 * parameters
 */

long stopCadStart;
assign stopCadStart to "{T}stopP:start.VAL";
monitor stopCadStart;

evflag stopCadChange;
sync stopCadStart to stopCadChange;

long stopCarIn;                 /* input to stopC CAR */
assign stopCarIn to "{T}stopC.IVAL";

long stopCarErrIn;              /* error input to stopC CAR */
assign stopCarErrIn to "{T}stopC.IERR";

string stopCarMessIn;               /* message input to stopC CAR */
assign stopCarMessIn to "{T}stopC.IMSS";

/* ===================================================================== */

/* Park record connections
 * The park CAD uses only the template connections
 */

long parkCadStart;
assign parkCadStart to "{T}parkP:start.VAL";
monitor parkCadStart;

evflag parkCadChange;
sync parkCadStart to parkCadChange;

long parkCarIn;                 /* input to parkC CAR */
assign parkCarIn to "{T}parkC.IVAL";

long parkCarErrIn;              /* error input to parkC CAR */
assign parkCarErrIn to "{T}parkC.IERR";

string parkCarMessIn;               /* message input to parkC CAR */
assign parkCarMessIn to "{T}parkC.IMSS";

/* ===================================================================== */

/* We need a mechanism to turn on/off guide simulation
 *
 */

long guideSimStart;
assign guideSimStart to "{T}guideSimPROC.VALA";
monitor guideSimStart;

evflag guideSimChange;
sync guideSimStart to guideSimChange;

long tiltPidOn;
assign tiltPidOn to "{T}tiltpidon.VAL";
monitor tiltPidOn;

long tiltpidDir;
assign tiltpidDir to "{T}tiltpidControl.DIR";

long focusPidOn;
assign focusPidOn to "{T}focuspidon.VAL";
monitor focusPidOn;

long focuspidDir;
assign focuspidDir to "{T}focuspidControl.DIR";

/* ===================================================================== */

/* Chop control record connections
 * Parameters associated with the chop control are chop on/off and some
 * of the DECS control switches
 */

long chopControlCadStart;
assign chopControlCadStart to "{T}chopControlP:start.VAL";
monitor chopControlCadStart;

evflag chopControlCadChange;
sync chopControlCadStart to chopControlCadChange;

long chopControlCarIn;       /* input to chopControlC CAR */
assign chopControlCarIn to "{T}chopControlC.IVAL";

long chopControlCarErrIn;    /* error input to chopControlC CAR */
assign chopControlCarErrIn to "{T}chopControlC.IERR";

string chopControlCarMessIn; /* message input to chopControlC CAR */
assign chopControlCarMessIn to "{T}chopControlC.IMSS";

long chopOn;                 /* request to start/stop chopping { OFF | ON } */
assign chopOn to "{T}chopControl.VALA";

long chopSwitch;             /* screen widget for chop on/off */
assign chopSwitch to "{T}chopon.VAL";

long decsOn;                 /* request to start/stop DECS (0=off, 1=on) */
assign decsOn to "{T}chopControl.VALB";

long decsPause;              /* request to pause DECS (0=off, 1=on) */
assign decsPause to "{T}chopControl.VALC";

long decsFreeze;             /* request to freeze DECS (0=off, 1=on) */
assign decsFreeze to "{T}chopControl.VALD";

long decsReset;              /* request to reset DECS (0=off, 1=on) */
assign decsReset to "{T}chopControl.VALE";

long profile;                /* demanded chop profile (0=2point, 1=3point, 2=triangle) */
assign profile to "{T}chopConfig.VALA";

long syncSource;             /* chop synchronisation source (0=SCS else instrument ID number */
assign syncSource to "{T}chopConfig.VALB";

double dutyCycle;
assign dutyCycle to "{T}chopConfig.VALD";

/* ===================================================================== */

long guideSwitch;                       /* screen widget for guide on/off */
assign guideSwitch to "{T}guideOn.VAL";

long guideWidget;
assign guideWidget to "{T}guideReset.VAL";          /* guide null widget on guide config screen */
/* ===================================================================== */

/* Tolerance record connections
 * Standard CAD template with five parameters specifying the tolerance
 * window size within which the M2 mechanism is considered in position
 * The window is applied to the magnitude of the position error
 */

long toleranceCadStart;
assign toleranceCadStart to "{T}toleranceP:start.VAL";
monitor toleranceCadStart;

evflag toleranceCadChange;
sync toleranceCadStart to toleranceCadChange;

long toleranceCarIn;                        /* input to toleranceC CAR */
assign toleranceCarIn to "{T}toleranceC.IVAL";

long toleranceCarErrIn;                     /* error input to toleranceC CAR */
assign toleranceCarErrIn to "{T}toleranceC.IERR";

string toleranceCarMessIn;                  /* message input to toleranceC CAR */
assign toleranceCarMessIn to "{T}toleranceC.IMSS";

double xTiltTol;
assign xTiltTol to "{T}tolerance.VALA";

double yTiltTol;
assign yTiltTol to "{T}tolerance.VALB";

double zFocusTol;
assign zFocusTol to "{T}tolerance.VALC";

double xPosTol;
assign xPosTol to "{T}tolerance.VALD";

double yPosTol;
assign yPosTol to "{T}tolerance.VALE";

double xyPosDeadband;
assign xyPosDeadband to "{T}tolerance.VALF";

double tolDir;
assign tolDir to "{T}tolerance.DIR";


/* ===================================================================== */

/* Bandwidth record connections
 * The bandwidth CAD processes and passes on a single value specifying
 * the demanded value of the tilt system bandwidth. This is checked by the
 * CAD processing to be one of 10, 15, 20 or 25 Hertz
 *
 * The prototype does not act on the bandwidth at present so the value
 * is not retrieved from the CAD VALA field
 */

long bandwidthCadStart;
assign bandwidthCadStart to "{T}servoP:start.VAL";
monitor bandwidthCadStart;

evflag bandwidthCadChange;
sync bandwidthCadStart to bandwidthCadChange;

double bandwidth;
assign bandwidth to "{T}servoBandwidth.VALA";

long bandwidthCarIn;                        /* input to servobwC CAR */
assign bandwidthCarIn to "{T}servoBandwidthC.IVAL";

long bandwidthCarErrIn;                     /* error input to servobwC CAR */
assign bandwidthCarErrIn to "{T}servoBandwidthC.IERR";

string bandwidthCarMessIn;                  /* message input to servobwC CAR */
assign bandwidthCarMessIn to "{T}servoBandwidthC.IMSS";

/* ===================================================================== */

/* Guide Configuration record connections
 * The guide configuration CAD processes the filtering and combination
 * recipe for the specified sensor source.  Ultimately the CAD processing
 * shall perform the filter calculation or lookup required so the outputs
 * will be in the form of a pointer to an array of coefficients. It is likely
 * that future versions will not be handled in SNL code but by mosub records.
 * these connection are included to allow the interface to be demonstrated
 */

long source;                            /* sensor source to which this record applies */
assign source to "{T}guideConfig.VALA";

double sampleFrequency;                     /* update frequency of source (Hertz) */
assign sampleFrequency to "{T}guideConfig.VALB";

long  filterType;                       /* type of filter to be applied */
assign filterType to "{T}guideConfig.VALC";

double frequency1;                      /* filter edge frequency */
assign frequency1 to "{T}guideConfig.VALD";

double frequency2;                      /* filter edge frequency */
assign frequency2 to "{T}guideConfig.VALE";

double xweight;                         /* weighting of sensor data for combination in beam A */        
assign xweight to "{T}guideConfig.VALF";

/* ===================================================================== */

/* SAD database connections
 * The following parameters provide connection to the SAD database for
 * in-file globally available data items
 */

long scsStateSad;                       /* current SCS running state (booting, init, running) */
assign scsStateSad to "{D}state.VAL";   /* not to be confused with scsState */

string topEndSad;                       /* current top end (F6 or F16) */
assign topEndSad to "{D}topEnd.VAL";

string chopping;                        /* currently chopping (yes or no) */
assign chopping to "{D}chopping.VAL";

string inPositionSad;                   /* in position (1=not in position, 0=in position) */
assign inPositionSad to "{D}inPosition.VAL";

string deployableSad;                   /* deployable baffle position (key as for demand) */
assign deployableSad to "{D}DeployBafflePos.VAL";

string centralSad;                      /* central baffle position (key as for demand) */ 
assign centralSad to "{D}CentralBafflePos.VAL";

long moveBaffleCar;
assign moveBaffleCar to "{T}moveBaffleC.IVAL";

double tempSad;
assign tempSad to "{D}temperature.VAL";

/* NOT USED ANYWHERE IN THE SNL 
   double xTiltPosSad;                     /* x tilt position 
   assign xTiltPosSad to "{D}xTiltPos.VAL";

   double yTiltPosSad;                     /* y tilt position 
   assign yTiltPosSad to "{D}yTiltPos.VAL";

   double zFocusPosSad;                    /* z focus position 
   assign zFocusPosSad to "{D}zPos.VAL";

   double xTiltDemandSad;                  /* demanded x tilt position 
   assign xTiltDemandSad to "{D}xNetTiltDmd.VAL";

   double yTiltDemandSad;                  /* demanded y tilt position 
   assign yTiltDemandSad to "{D}yNetTiltDmd.VAL";

   double zFocusDemandSad;                 /* demanded z focus position 
   assign zFocusDemandSad to "{D}zNetDmd.VAL";

   double xTiltErrorSad;                   /* x tilt position error 
   assign xTiltErrorSad to "{D}xTiltErr.VAL";

   double yTiltErrorSad;                   /* y tilt position error 
   assign yTiltErrorSad to "{D}yTiltErr.VAL";

   double zFocusErrorSad;                  /* z focus position error 
   assign zFocusErrorSad to "{D}zErr.VAL";

   double xPositionSad;                    /* x position 
   assign xPositionSad to "{D}xPos.VAL";

   double xDemandSad;                      /* demanded x position 
   assign xDemandSad to "{D}xPosDmd.VAL";

   double xErrorSad;                       /* x position error 
   assign xErrorSad to "{D}xPosErr.VAL";

   double yPositionSad;                    /* y position 
   assign yPositionSad to "{D}yPos.VAL";

   double yDemandSad;                      /* demanded y position 
   assign yDemandSad to "{D}yPosDmd.VAL";

   double yErrorSad;                       /* y position error 
   assign yErrorSad to "{D}yPosErr.VAL";

   double xTiltGuideSad;                   /* net x tilt correction 
   assign xTiltGuideSad to "{D}xTiltGuide.VAL";

   double yTiltGuideSad;                   /* net y tilt correction 
   assign yTiltGuideSad to "{D}yTiltGuide.VAL";

   double zFocusGuideSad;                  /* net z focus correction 
   assign zFocusGuideSad to "{D}zGuide.VAL";

   string scsHealth;                       /* current health of the SCS 
   assign scsHealth to "{D}health.VAL";
 */
/* ===================================================================== */

/* channel addresses to display guide sources */

double x[5];
assign x[0] to "{T}xtilt0.VAL";
assign x[1] to "{T}xtilt1.VAL";
assign x[2] to "{T}xtilt2.VAL";
assign x[3] to "{T}xtilt3.VAL";
assign x[4] to "{T}xtilt4.VAL";

double y[5];
assign y[0] to "{T}ytilt0.VAL";
assign y[1] to "{T}ytilt1.VAL";
assign y[2] to "{T}ytilt2.VAL";
assign y[3] to "{T}ytilt3.VAL";
assign y[4] to "{T}ytilt4.VAL";

double z[5];
assign z[0] to "{T}focus0.VAL";
assign z[1] to "{T}focus1.VAL";
assign z[2] to "{T}focus2.VAL";
assign z[3] to "{T}focus3.VAL";
assign z[4] to "{T}focus4.VAL";

double xerr[5];
assign xerr[0] to "{T}xerr0.VAL";
assign xerr[1] to "{T}xerr1.VAL";
assign xerr[2] to "{T}xerr2.VAL";
assign xerr[3] to "{T}xerr3.VAL";
assign xerr[4] to "{T}xerr4.VAL";

double yerr[5];
assign yerr[0] to "{T}yerr0.VAL";
assign yerr[1] to "{T}yerr1.VAL";
assign yerr[2] to "{T}yerr2.VAL";
assign yerr[3] to "{T}yerr3.VAL";
assign yerr[4] to "{T}yerr4.VAL";

double zerr[5];
assign zerr[0] to "{T}zerr0.VAL";
assign zerr[1] to "{T}zerr1.VAL";
assign zerr[2] to "{T}zerr2.VAL";
assign zerr[3] to "{T}zerr3.VAL";
assign zerr[4] to "{T}zerr4.VAL";

double xRateLimit;
assign xRateLimit to "{T}xRateLimit.VAL";
double yRateLimit;
assign yRateLimit to "{T}yRateLimit.VAL";
double zRateLimit;
assign zRateLimit to "{T}zRateLimit.VAL";

double xWindup;
assign xWindup to "{T}xWindup.VAL";
double yWindup;
assign yWindup to "{T}yWindup.VAL";
double zWindup;
assign zWindup to "{T}zWindup.VAL";

/* ===================================================================== */

/* The following items are made available as state indicators */

long myscsState;
/* current state of the SCS  use definition in interlock.h */

assign myscsState to "{T}scsState.VAL";
monitor myscsState;

long cadProcessorState;
assign cadProcessorState to "{T}cadProcessorState.VAL";
monitor cadProcessorState;

long followDemandState;
assign  followDemandState to "{T}followDemandState.VAL";
monitor followDemandState;

long monitorSadState;
assign monitorSadState to "{T}monitorSadState.VAL";
monitor monitorSadState;

long monitorProcessState;
assign monitorProcessState to "{T}monitorProcessState.VAL";
monitor monitorProcessState;

long rebootScsState;
assign rebootScsState to "{T}rebootScsState.VAL";
monitor rebootScsState;

/* ===================================================================== */

/* Commands and data from the M2 system to the SCS */

long coincidence;
assign coincidence to "{T}coincidence.VAL";
monitor coincidence;

evflag inPositionChange;
sync coincidence to inPositionChange;

/* servoInPosition flag for enabling servo on */

long servoInPosition;
assign servoInPosition to "{T}readRmDiags.VALE";
monitor servoInPosition;

/* ===================================================================== */
/* INDENT OFF */
/* 
 *  Function Name:
 *  State set "CADprocessor"
 *
 *  Purpose:
 *  Manage boot procedure and implement state machine of the SCS
 *
 *  Description:
 *  This state set controls the booting and initialisation behaviour
 *  of the SCS prototype the waits for triggers from individual
 *  CAD records. Each command is handled by a separate processing
 *  block, returning to the waitForCommand state on completion.
 *
 *  Invocation:
 *  Automatic after loading
 *
 *  Parameters: (">" input, "!" modified, "<" output)
 *  !   myscsState
 *  
 *  Function value:
 *  n/a
 *
 *  External functions:
 *  The following SNL functions have been used
 *
 *  pvPut       write a value to a database channel
 *  pvGet       read a value from a database channel
 *  efClear     clear an event flag
 *  efTest      check an event flag
 *  delay       delay for the specified time period
 *
 *  External variables:
 *  see parameters
 *
 *  Prior requirements:
 *  see file header for scs_st.st
 *
 *  Deficiencies:
 *  see file header for scs_st.st
 */
/* INDENT ON */
/* ===================================================================== */

ss CADprocessor {
   state entry {
      when() {
         /* set current SCS state to IDLE */

         myscsState = SYSTEMIDLE;
         scsState = myscsState;
         pvPut(myscsState);

         cadProcessorState = CADPROCENTRY;
         pvPut(cadProcessorState);

         /* set health to good until proved otherwise */

         reportHealth(GOOD, "");

         /* write current state to SAD */

         scsStateSad = BOOTING;
         pvPut(scsStateSad);

      }   state clearFlags
   }

   /* ===================================================================== */

   state clearFlags {
      when() {
         /* clear all event flags */

         efClear(testCadChange);
         efClear(initCadChange);
         efClear(initXYCadChange);
         efClear(moveCadChange);
         efClear(stopCadChange);
         efClear(parkCadChange);
         efClear(guideSimChange);
         efClear(chopControlCadChange);
         efClear(newBeamA);
         efClear(bandwidthCadChange);
         efClear(toleranceCadChange);
         efClear(inPositionChange);
         efClear(followCadChange);
         efClear(actuatorCadChange);
         efClear(vtkChange);

         puts("Cleared flags, starting INIT in 10.0 secs...");

      }   state startInit
   }

   /* ===================================================================== */
   state idle {
      when() {
         cadProcessorState = CADPROCIDLE;
         pvPut(cadProcessorState);

         /* write current SCS state to the SAD */
         scsStateSad = RUNNING;
         pvPut(scsStateSad);

      }   state waitForCommand
   }

   /* ===================================================================== */

   state waitForCommand {
      /* when the appropriate event flag is set, clear it and vector to
       * the corresponding handling routine
       */

      when(efTest(testCadChange)) 
      {
         /* clear event flag */
         efClear(testCadChange);

      }   state startTestHandshake

      when(efTest(initCadChange))
      {
         /* clear event flag */
         efClear(initCadChange);

      }   state startInit

      when(efTest(initXYCadChange))
      {
         /* clear event flag */
         efClear(initXYCadChange);

      }   state startInitXY

      when(efTest(moveCadChange))
      {
         /* clear event flag */
         efClear(moveCadChange);

      }   state startMove

      when(efTest(actuatorCadChange))
      {
         /* clear event flag */
         efClear(actuatorCadChange);

      }   state startActuator

      when(efTest(followCadChange))
      {
         /* clear event flag */

         efClear(followCadChange);

      }   state startFollow

      when(efTest(stopCadChange))
      {
         /* clear event flag */

         efClear(stopCadChange);

      }   state startStop

      when(efTest(chopControlCadChange))
      {
         /* clear event flag */

         efClear(chopControlCadChange);

      }   state startChopControl

      when(efTest(parkCadChange))
      {
         /* clear event flag */

         efClear(parkCadChange);

      }   state startPark

      when(efTest(bandwidthCadChange))
      {
         /* clear event flag */

         efClear(bandwidthCadChange);

      }   state startBandwidth

      when(efTest(toleranceCadChange))
      {
         /* clear event flag */

         efClear(toleranceCadChange);

      }   state startTolerance

      when(efTest(guideSimChange))
      {
         efClear(guideSimChange);
      } state startGuideSimChange

      when(efTest(vtkChange))
      {
         /* clear event flag */
         efClear(vtkChange);

      }   state startVtkChange

   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startTestHandshake
    *  readAgain
    *
    *  Purpose:
    *  Perform simple checks on system to ensure M2 is responding
    *
    *  Description:
    *  The test CAR status is set BUSY and the current values of M2 heartbeat
    *  and last acknowledged frame are recorded. A delay of one second is taken
    *  and these values are sampled again to ensure that they are incrementing
    *  Failure to change results in a health error with messages written to both
    *  the test and health message records
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startTestHandshake
   {
      when()
      {
         cadProcessorState = STARTTESTHANDSHAKE;
         pvPut(cadProcessorState);

         /* read frame acknowledgement and hearbeat from M2 */

         handshake.oldNR = scsPtr->page1.NR;
         handshake.oldHeartbeat = scsPtr->page1.heartbeat;

      }   state readAgain
   }

   state readAgain
   {
      when(delay(1.0))
      {
         cadProcessorState = READAGAIN;
         pvPut(cadProcessorState);

         handshake.newNR = scsPtr->page1.NR;
         handshake.newHeartbeat = scsPtr->page1.heartbeat;

         /* check for increments */

         if(handshake.newNR == handshake.oldNR)
         {
            strncpy(testResultString, "M2 not acknowledging command frames", MAX_STRING_SIZE - 1);
            reportHealth(BAD, "M2 not acknowledging command frames");
         }
         else if(handshake.newHeartbeat == handshake.oldHeartbeat)
         {
            strncpy(testResultString, "M2 heartbeat stuck", MAX_STRING_SIZE - 1);
            reportHealth(BAD, "M2 heartbeat stuck");
         }
         else
         {
            strncpy(testResultString, " ", MAX_STRING_SIZE - 1);
         }

         /* write results string to results field */

         pvPut(testResultString);

         /* send idle indication to the testC record */

         testCarIn = CAR_IDLE;
         pvPut(testCarIn);

      }   state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startInit
    *  waitForInitStart
    *  waitForInitCompletion
    *
    *  Purpose:
    *  Issue the initialise command to the m2 system monitoring for start and 
    *  completion with timeout protection.
    *
    *  Description:
    *  The init command is issued to the m2 system whose state is then monitored
    *  to verify a response. If successful the code waits for completion before 
    *  setting the CAR status back to idle. If either the command is ignored or
    *  testing does not complete in time, error routines are invoked setting CAR
    *  status to ERR with error messages
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  !   myscsState
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startInit
   {
      /* The following delay is in units of 0.33 seconds.*/
      when(delay(30.0))
      {
         pvGet(initResponse);
         printf("initResponse is %ld\n", initResponse);

         errorLog("state startInit...", 1, ON);
         myscsState = INITIALISING;
         scsState = myscsState;
         pvPut(myscsState);

         cadProcessorState = STARTINIT;
         pvPut(cadProcessorState);

         scsStateSad = INIT;
         pvPut(scsStateSad);

         activeC = CAR_BUSY;
         pvPut(activeC);

         /* turn off chop */
         printf("startInit - turn off chop...");

         writeCommand(CHOP_OFF);

         chopSwitch = 0;
         pvPut(chopSwitch);

         chopIsOn = 0;
         printf("chopSwitch and chopIsOn are now 0\n");

         /* turn off following */
         printf("startInit - turn off following...\n");

         followOn = OFF;

         /* turn off guiding */
         printf("startInit - turn off guiding...\n");

         guideOn = OFF;

         guideSwitch = 0;
         pvPut(guideSwitch);

         /* REMOVED: 22-June-2000 
            turn on tilt system by writing to the event system 

            if(eventConfig.change == 0)
            {
            if previous changes have been acknowledged, 
            write new data, flag change and give sem 

            printf("startInit - turn on tilt system...\n");
            printf("startInit - setting up a new eventConfig\n");
            eventConfig.change  = 1;
            eventConfig.m2Power = ON;
            eventConfig.m2Servo = ON;
            printf("startInit - giving eventSem semaphore\n");
            semGive(eventSem); 
            }
            else
            {
            errorLog("startInit - command not acknowledged", 1, ON);
            printf("startInit - command not acknowledged\n");
            }
          */

         /* REPLACED WITH: Just set these, so that power is stays ON.
          * Note that it is set to on as part of the reboot sequence.
          * So there is no need to set change to 1 or to give sem to 
          * call the hndlr routine */

         eventConfig.m2Power = ON;
         eventConfig.m2Servo = ON;


         /* give semaphore to allow loading of initialisation files */

         semGive(doPvLoad);

         /* wait on semaphore showing load completed */

         %{
            if(semTake(pvLoadComplete, WAIT_FOREVER) != OK)
            {
               errorLog("timeout waiting for pvLoad completion", 1, ON);
               reportHealth(WARNING, "Timeout loading filter coefficients");
            }
         }%

         puts("Files now loaded, wait 10.0 seconds...");
      } state loadFiles
   }

   state loadFiles
   {
      when(delay(30.0))
      {
         cadProcessorState = LOADFILES;
         pvPut(cadProcessorState);

         pvGet(initResponse);
         printf("state loadFiles ...initResponse is %ld\n", initResponse);

         /* read instrument configuration details and write to local store*/

         pvGet(scs0name);
         strncpy(instruments[0].name, scs0name, MAX_STRING_SIZE - 1);

         pvGet(scs0port);
         instruments[0].port = atoi(scs0port);

         pvGet(scs0capability);
         strncpy(instruments[0].capability, scs0capability, MAX_STRING_SIZE - 1);

         pvGet(ics0name);
         strncpy(instruments[1].name, ics0name, MAX_STRING_SIZE - 1);

         pvGet(ics0port);
         instruments[1].port = atoi(ics0port);

         pvGet(ics0capability);
         strncpy(instruments[1].capability, ics0capability, MAX_STRING_SIZE - 1);
         pvGet(ics1name);
         strncpy(instruments[2].name, ics1name, MAX_STRING_SIZE - 1);

         pvGet(ics1port);
         instruments[2].port = atoi(ics1port);

         pvGet(ics1capability);
         strncpy(instruments[2].capability, ics1capability, MAX_STRING_SIZE - 1);
         pvGet(ics2name);
         strncpy(instruments[3].name, ics2name, MAX_STRING_SIZE - 1);

         pvGet(ics2port);
         instruments[3].port = atoi(ics2port);

         pvGet(ics2capability);
         strncpy(instruments[3].capability, ics2capability, MAX_STRING_SIZE - 1);

         pvGet(ics3name);
         strncpy(instruments[4].name, ics3name, MAX_STRING_SIZE - 1);

         pvGet(ics3port);
         instruments[4].port = atoi(ics3port);

         pvGet(ics3capability);
         strncpy(instruments[4].capability, ics3capability, MAX_STRING_SIZE - 1);

         pvGet(ics4name);
         strncpy(instruments[5].name, ics4name, MAX_STRING_SIZE - 1);

         pvGet(ics4port);
         instruments[5].port = atoi(ics4port);

         pvGet(ics4capability);
         strncpy(instruments[5].capability, ics4capability, MAX_STRING_SIZE - 1);

         printf("instrument data loaded\n");

         strncpy(inst0, instruments[0].name, MAX_STRING_SIZE - 1);
         pvPut(inst0);

         strncpy(inst1, instruments[1].name, MAX_STRING_SIZE - 1);
         pvPut(inst1);

         strncpy(inst2, instruments[2].name, MAX_STRING_SIZE - 1);
         pvPut(inst2);

         strncpy(inst3, instruments[3].name, MAX_STRING_SIZE - 1);
         pvPut(inst3);

         strncpy(inst4, instruments[4].name, MAX_STRING_SIZE - 1);
         pvPut(inst4);

         strncpy(inst5, instruments[5].name, MAX_STRING_SIZE - 1);
         pvPut(inst5);

         /* read frame of reference conversion data */

         pvGet(tiltSkew);
         pvGet(tiltOffsetX);
         pvGet(tiltOffsetY);

         pvGet(posSkew);
         pvGet(posOffsetX);
         pvGet(posOffsetY);

         pvGet(gaosSkew);
         pvGet(gaosOffsetX);
         pvGet(gaosOffsetY);

         pvGet(gyroSkew);
         pvGet(gyroOffsetX);
         pvGet(gyroOffsetY);

         pvGet(focusScaling);

         %{
            frame.tiltCosTheta = cos(tiltSkew * DEGS2RADS);
            frame.tiltSinTheta = sin(tiltSkew * DEGS2RADS);
            frame.tiltOffsetX = tiltOffsetX;
            frame.tiltOffsetY = tiltOffsetY;

            frame.posCosTheta = cos(posSkew * DEGS2RADS);
            frame.posSinTheta = sin(posSkew * DEGS2RADS);;
            frame.posOffsetX = posOffsetX;
            frame.posOffsetY = posOffsetY;

            frame.gaosCosTheta = cos(gaosSkew * DEGS2RADS);
            frame.gaosSinTheta = sin(gaosSkew * DEGS2RADS);;
            frame.gaosOffsetX = gaosOffsetX;
            frame.gaosOffsetY = gaosOffsetY;

            frame.gyroCosTheta = cos(gyroSkew * DEGS2RADS);
            frame.gyroSinTheta = sin(gyroSkew * DEGS2RADS);;
            frame.gyroOffsetX = gyroOffsetX;
            frame.gyroOffsetY = gyroOffsetY;

            frame.focusScaling = focusScaling;
         }%

         writeCommand(ACT_PWR_ON);
         /*            writeCommand(MEND);
                       writeCommand(VEND); */
         writeCommand(MOFFLOFF);
         writeCommand(CMD_INIT);

         /* clear out reflective mem */

	     pvGet(moveXPosition);
	 	 pvGet(moveYPosition);

         if (semTake (setPointFree, SEM_TIMEOUT) == OK)
         {
            logMsg(" moveXposition set to %f \n",moveXPosition, 0, 0, 0, 0, 0);
            logMsg(" moveYPosition set to %f \n",moveYPosition, 0, 0, 0, 0, 0);
            setPoint.xTiltA =  0.0;
            setPoint.yTiltA =  0.0;
            setPoint.xTiltB =  0.0;
            setPoint.yTiltB =  0.0;
            setPoint.xTiltC =  0.0;
            setPoint.yTiltC =  0.0;
            setPoint.zFocus =  0.0;
            setPoint.xPosition =  moveXPosition;
            setPoint.yPosition =  moveYPosition;
            semGive(setPointFree);
         }

         pvGet(moveXInputPos);
         pvGet(moveYInputPos);
         pvGet(xyPosDeadband);

         if (semTake (refMemFree, SEM_TIMEOUT) == OK)
         {
            logMsg(" moveXInputPos to RM %f \n",moveXInputPos, 0, 0, 0, 0, 0);
            logMsg(" moveYInputPos to RM %f \n",moveYInputPos, 0, 0, 0, 0, 0);
            logMsg(" xyPosDeadband to RM %f \n",xyPosDeadband, 0, 0, 0, 0, 0);
            scsBase->page0.AxTilt = 0.0;
            scsBase->page0.BxTilt = 0.0;
            scsBase->page0.CxTilt = 0.0;
            scsBase->page0.AyTilt = 0.0;
            scsBase->page0.ByTilt = 0.0;
            scsBase->page0.CyTilt = 0.0;
            scsBase->page0.xTiltGuide = 0.0;
            scsBase->page0.yTiltGuide = 0.0;
            scsBase->page0.zFocusGuide = 0.0; 
            scsBase->page0.xDemand = moveXPosition;
            scsBase->page0.yDemand = moveYPosition;
            scsBase->page0.centralBaffle = 0; 
            scsBase->page0.deployBaffle = 0;
            scsBase->page0.chopProfile = 0;
            scsBase->page0.chopFrequency = 0.0;
            scsBase->page0.chopDutyCycle = 0.0;
            scsBase->page0.xTiltTolerance = 0.0;
            scsBase->page0.yTiltTolerance = 0.0;
            scsBase->page0.zFocusTolerance = 0.0;
            scsBase->page0.xPositionTolerance = 0.0;
            scsBase->page0.yPositionTolerance = 0.0;
            scsBase->page0.bandwidth = 0.0;
            scsBase->page0.xTiltGain =0.0;
            scsBase->page0.yTiltGain = 0.0;
            scsBase->page0.zFocusGain = 0.0;
            scsBase->page0.xTiltShift = 0.0;
            scsBase->page0.yTiltShift = 0.0;
            scsBase->page0.zFocusShift = 0.0;
            scsBase->page0.xTiltSmooth = 0.0;
            scsBase->page0.yTiltSmooth = 0.0;
            scsBase->page0.zFocusSmooth = 0.0;
            scsBase->page0.xTcsMinRange = 0.0;
            scsBase->page0.yTcsMinRange = 0.0;
            scsBase->page0.xTcsMaxRange = 0.0;
            scsBase->page0.yTcsMaxRange = 0.0; 
            scsBase->page0.xPMinRange = 0.0;
            scsBase->page0.yPMinRange = 0.0;
            scsBase->page0.xPMaxRange = 0.0;
            scsBase->page0.yPMaxRange = 0.0; 
            scsBase->page0.follower = 0;
            scsBase->page0.foldir = 0;
            scsBase->page0.followersteps = 0;
            scsBase->page0.offloader = 0;
            scsBase->page0.ofldir = 0;
            scsBase->page0.offloadersteps = 0;
            scsBase->page0.cbafdir = 0;
            scsBase->page0.cbsteps = 0;
            scsBase->page0.deployable_baffle = 0;
            scsBase->page0.dbafdir = 0;
            scsBase->page0.dbsteps = 0;
            scsBase->page0.xy_motor = 0;
            scsBase->page0.xydir = 0;
            scsBase->page0.xysteps = 0;
            scsBase->page0.zFocus = 0.0; 
            scsBase->page0.zGuide = 0.0; 
            scsBase->page0.rawXGuide = 0.0;
            scsBase->page0.rawYGuide = 0.0;
            scsBase->page0.rawZGuide = 0.0;
            scsBase->page0.xGrossTiltDmd = 0.0;
            scsBase->page0.yGrossTiltDmd = 0.0;
            scsBase->page0.xyPositionDeadband = xyPosDeadband;

            semGive (refMemFree);
         }
         else
         {
            errorLog ("startInit - refMemFree timeout", 1, ON);
         } 

         controller[FOCUS].sum = 0.0;
         controller[FOCUS].oldSum = 0.0;
         controller[FOCUS].oldError = 0.0;
         controller[XTILT].sum = 0.0;
         controller[XTILT].oldSum = 0.0;
         controller[XTILT].oldError = 0.0;
         controller[YTILT].sum = 0.0;
         controller[YTILT].oldSum = 0.0;
         controller[YTILT].oldError = 0.0;

         /* set health to good until proved otherwise */
         reportHealth(GOOD, "");
         puts("init - wait for init response: then wait 10 seconds...");

         /* clear message field */
         strncpy(initCarMessIn, "M2 is initialising...", MAX_STRING_SIZE - 1);
         pvPut(initCarMessIn);

         pvGet(initResponse);

      }   state waitForInitStart
   }

   state waitForInitStart
   {
      /* verify that the M2 system is initialising */

      when( (initResponse == 1) && delay(30.0))
      {
         cadProcessorState = WAITINITRESPONSE;
         pvPut(cadProcessorState);

         puts("init start detected. wait 10.0 seconds"); 

         /* clear message field */

         strncpy(initCarMessIn, "M2 is initialising...", MAX_STRING_SIZE - 1);
         pvPut(initCarMessIn);

         pvGet(initResponse);
      }   state waitForInitCompletion

      /* timeout if no response */

      when(delay(CMD_RESPONSE_TIMEOUT))
      {
         initCarErrIn = STATUS_ERROR;
         pvPut(initCarErrIn);

         puts("init - no response");
         strncpy(initCarMessIn, "M2 no response to init", MAX_STRING_SIZE - 1);
         pvPut(initCarMessIn);

         initCarIn = CAR_ERROR;
         pvPut(initCarIn);

      }   state idle
   }

   state waitForInitCompletion
   {
      when((initResponse == 0) && delay(30.0 ))
      {
         pvGet(initResponse);
         printf("Init Complete ...initResponse is %ld\n", initResponse);

         cadProcessorState = WAITINITCOMPLETE;
         pvPut(cadProcessorState);

         /* send idle indication to the initC record */
         initCarIn = CAR_IDLE;
         pvPut(initCarIn);

         /* clear message field */
         strncpy(initCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(initCarMessIn);
         puts("BYPASS REDUNDANT STARTUP COMMANDS..."); 
         /*
            writeCommand(ACT_PWR_ON);
            writeCommand(MOFFLON);
            writeCommand(VSTART);
            writeCommand(MSTART);
         */

         myscsState = SYSTEMIDLE;
         scsState = myscsState;
         pvPut(myscsState);

         puts("init push tolerance data and wait 6.0 seconds...");
         /* Submit tolerance data to CEM prior to base move*/
         tolDir = 0;
         pvPut(tolDir);

         tolDir = 2;
         pvPut(tolDir);

         tolDir = 3;
         pvPut(tolDir);

      } state initCompleteStartMove

      when(delay(INIT_COMPLETE_TIMEOUT)) 
      {
         cadProcessorState = INITCOMPLETETIMEOUT;
         pvPut(cadProcessorState);

         initCarErrIn = STATUS_ERROR;
         pvPut(initCarErrIn);

         strncpy(initCarMessIn, "M2 did not finish init within 6 minutes", 
               MAX_STRING_SIZE - 1);
         pvPut(initCarMessIn);

         initCarIn = CAR_ERROR;
         pvPut(initCarIn);

         writeCommand(ACT_PWR_ON);
         writeCommand(MOFFLON);
         writeCommand(VSTART);
         writeCommand(MSTART);

         myscsState = SYSTEMIDLE;
         scsState = myscsState;
         pvPut(myscsState);

         /* Note: do this even when timeout has occurred. Sometimes
          * the INIT takes more than 6 minutes and the reason is 
          * that the XY positioner has taken the "long way round"
          * including going off into non-linear space for a while.
          * In this case, you still want to fire off the initial
          * move and set up the pid */

         /* send move command to move to initial position as defined
            in pvloaded data file (SCSinit.dat)  */
         puts(".DICTO");

         moveDir = 2;
         pvPut(moveDir);
         moveDir = 3;
         pvPut(moveDir); 
      }   state idle

   }

   state initCompleteStartMove
   {
      when (delay(18.0))
      {

         puts("...Finished INIT, moving to base");

         /* send move command to move to initial position as defined
            in pvloaded data file (SCSinit.dat)  */
         moveDir = 1;
         pvPut(moveDir);

         moveDir = 0;
         pvPut(moveDir);

         moveDir = 2;
         pvPut(moveDir);

         moveDir = 3;
         pvPut(moveDir); 

         /* fetch demand values from move CAD */
         pvGet(moveAXTilt);
         pvGet(moveAYTilt);
         pvGet(moveBXTilt);
         pvGet(moveBYTilt);
         pvGet(moveCXTilt);
         pvGet(moveCYTilt);
         pvGet(moveZFocus);
         pvGet(moveXPosition);
         pvGet(moveYPosition);
         pvGet(moveXInputPos);
         pvGet(moveYInputPos);
         pvGet(xyPosDeadband);

         /* copy values to default structure */

         xTiltAdef = moveAXTilt;
         yTiltAdef = moveAYTilt;
         xTiltBdef = moveBXTilt;
         yTiltBdef = moveBYTilt;
         xTiltCdef = moveCXTilt;
         yTiltCdef = moveCYTilt;
         zFocusDef = moveZFocus;
         xPosDef = moveXPosition;
         yPosDef = moveYPosition;
         xPosTcsDef = moveXInputPos;
         yPosTcsDef = moveYInputPos;
         xyDeadband = xyPosDeadband;
         printf(" xTiltAdef %f \n",xTiltAdef);
         printf(" yTiltAdef %f \n",yTiltAdef);
         printf(" xPosDef %f \n",xPosDef);
         printf(" yPosDef %f \n",yPosDef);
         printf(" xPosTcsDef %f \n",xPosTcsDef);
         printf(" yPosTcsDef %f \n",yPosTcsDef);
         printf(" xyDeadband %f \n",xyDeadband);

         /* send move bafflecommand to move to initial position as defined
            in pvloaded data file (SCSinit.dat)  */
         /* 
            moveBaffleDir = 2;
            pvPut(moveBaffleDir);
            moveBaffleDir = 3;
            pvPut(moveBaffleDir); */

         /* send controller command to set up pid values */

         pidDir = 2;
         pvPut(pidDir);
         pidDir = 3;
         pvPut(pidDir);

        
        /* send VTK command to setup VTK values*/
        vtkXDir = 2;
        pvPut(vtkXDir);
        vtkXDir = 3;
        pvPut(vtkXDir);

        vtkYDir = 2;
        pvPut(vtkYDir);
        vtkYDir = 3;
        pvPut(vtkYDir);


        /* send synthetic wave (phasor) values */
        phasorXDir = 2;
        pvPut(phasorXDir);
        phasorXDir = 3;
        pvPut(phasorXDir);


        phasorYDir = 2;
        pvPut(phasorYDir);
        phasorYDir = 3;
        pvPut(phasorYDir);

      }   state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startInitXY
    *  waitForInitStartXY
    *  waitForInitCompletionXY
    *
    *  Purpose:
    *  Issue the initialise command to the m2 system monitoring for start and 
    *  completion with timeout protection.
    *
    *  Description:
    *  The init command is issued to the m2 system whose state is then monitored
    *  to verify a response. If successful the code waits for completion before 
    *  setting the CAR status back to idle. If either the command is ignored or
    *  testing does not complete in time, error routines are invoked setting CAR
    *  status to ERR with error messages
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  !   myscsState
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startInitXY
   {
      when()
      {
         errorLog("state startInitXY...", 1, ON);
         myscsState = INITIALISING;
         scsState = myscsState;
         pvPut(myscsState);

         cadProcessorState = STARTINIT;
         pvPut(cadProcessorState);

         scsStateSad = INIT;
         pvPut(scsStateSad);

         /* turn off chop */
         writeCommand(CHOP_OFF);

         chopSwitch = 0;
         pvPut(chopSwitch);

         chopIsOn = 0;

         /* turn off following */

         followOn = OFF;

         /* turn off guiding */

         guideOn = OFF;

         guideSwitch = 0;
         pvPut(guideSwitch);


         /* turn on actuator power */
         writeCommand(ACT_PWR_ON);

      } state loadFilesXY
   }

   state loadFilesXY
   {
      when()
      {
         cadProcessorState = LOADFILES;
         pvPut(cadProcessorState);


         printf("sending command : CMD_XYINIT \n");
         writeCommand(CMD_XYINIT);

         /* clear out reflective mem */

         if (semTake (setPointFree, SEM_TIMEOUT) == OK)
         {
            setPoint.xPosition =  0.0;
            setPoint.yPosition =  0.0;
            semGive(setPointFree);
         }

         if (semTake (refMemFree, SEM_TIMEOUT) == OK)
         {
            scsBase->page0.xDemand = moveXPosition;
            scsBase->page0.yDemand = moveYPosition;
            scsBase->page0.xPositionTolerance = 0.0;
            scsBase->page0.yPositionTolerance = 0.0;
            scsBase->page0.xTcsMinRange = 0.0;
            scsBase->page0.yTcsMinRange = 0.0;
            scsBase->page0.xTcsMaxRange = 0.0;
            scsBase->page0.yTcsMaxRange = 0.0; 
            scsBase->page0.xPMinRange = 0.0;
            scsBase->page0.yPMinRange = 0.0;
            scsBase->page0.xPMaxRange = 0.0;
            scsBase->page0.yPMaxRange = 0.0; 
            scsBase->page0.xy_motor = 0;
            scsBase->page0.xydir = 0;
            scsBase->page0.xysteps = 0;
            scsBase->page0.xyPositionDeadband = xyPosDeadband;

            semGive (refMemFree);
         }
         else
         {
            errorLog ("startInitXY - refMemFree timeout", 1, ON);
         } 

         /* set health to good until proved otherwise */

         reportHealth(GOOD, "");


         /* clear message field */

         strncpy(initXYCarMessIn, "M2 XY Positioner is initialising...", MAX_STRING_SIZE - 1)
            ;
         pvPut(initXYCarMessIn);


      }   state waitForInitStartXY
   }

   state waitForInitStartXY
   {
      /* verify that the M2 system is initialising */

      when(initResponse == 1)
      {
         cadProcessorState = WAITINITRESPONSE;
         pvPut(cadProcessorState);

         puts("init XY start detected"); 

         /* clear message field */

         strncpy(initXYCarMessIn, "M2 XY Positioner is initialising...", MAX_STRING_SIZE - 1);
         pvPut(initXYCarMessIn);

      }   state waitForInitCompletionXY

      /* timeout if no response */

      when(delay(CMD_RESPONSE_TIMEOUT))
      {
         initXYCarErrIn = STATUS_ERROR;
         pvPut(initXYCarErrIn);

         puts("init - no response");
         strncpy(initXYCarMessIn, "M2 no response to init XY Positioner", MAX_STRING_SIZE - 1);
         pvPut(initXYCarMessIn);

         initXYCarIn = CAR_ERROR;
         pvPut(initXYCarIn);

      }   state idle
   }

   state waitForInitCompletionXY
   {
      when(initResponse == 0)
      {
         cadProcessorState = WAITINITCOMPLETE;
         pvPut(cadProcessorState);

         puts("init completion detected");

         /* send idle indication to the initC record */

         initXYCarIn = CAR_IDLE;
         pvPut(initXYCarIn);

         /* clear message field */

         strncpy(initXYCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(initXYCarMessIn);

         myscsState = SYSTEMIDLE;
         scsState = myscsState;
         pvPut(myscsState);


         printf(" xPosTcsDef %f \n",xPosTcsDef);
         printf(" yPosTcsDef %f \n",yPosTcsDef);

         moveXInputPos = xPosTcsDef;
         moveYInputPos = yPosTcsDef;
         pvPut(moveXInputPos);
         pvPut(moveYInputPos);

         moveDir = 2;
         pvPut(moveDir);
         moveDir = 3;
         pvPut(moveDir);

      }   state idle

      when(delay(INIT_COMPLETE_TIMEOUT)) 
      {
         cadProcessorState = INITCOMPLETETIMEOUT;
         pvPut(cadProcessorState);

         initXYCarErrIn = STATUS_ERROR;
         pvPut(initXYCarErrIn);

         strncpy(initXYCarMessIn, "M2 XY Positioner did not finish init within 6 minutes", MAX_STRING_SIZE - 1);
         pvPut(initXYCarMessIn);

         initXYCarIn = CAR_ERROR;
         pvPut(initXYCarIn);

         myscsState = SYSTEMIDLE;
         scsState = myscsState;
         pvPut(myscsState);

         /* Note: do this even when timeout has occurred. Sometimes
          * the INIT takes more than 6 minutes and the reason is 
          * that the XY positioner has taken the "long way round"
          * including going off into non-linear space for a while. */

      }   state idle
   }



   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startMove
    *  waitForMoveStart
    *  waitForCoincidence
    *
    *  Purpose:
    *  Set m2 to the moving state then monitor for achieving the in position
    *  indication.
    *
    *  Description:
    *  The move command is issued to the m2 system whose state is then monitored
    *  to verify a response. If successful the 'in position' indication from the
    *  m2 system is monitored.  When in position is first achieved the move command is
    *  considered completed and the CAR state set back to idle.
    *  If in position is not achieved in time, error routines are invoked
    *  setting CAR status to ERR with error messages
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  !   myscsState
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startMove
   {
      when()
      {
         cadProcessorState = STARTMOVE;
         pvPut(cadProcessorState);

         writeCommand(ACT_PWR_ON);
         writeCommand(MEND);
         writeCommand(VEND); 
         writeCommand(MOFFLON);


      } state proceedWithMove
   }

   state proceedWithMove
   {
      when(servoOnStatus == 0) 
      {
         printf("mirror control disabled - proceed with move\n");

         myscsState = MOVING;
         scsState = myscsState;
         pvPut(myscsState);

         cadProcessorState = PROCEEDWITHMOVE;
         pvPut(cadProcessorState);

         followOn = OFF;

         /* fetch demand values from move CAD */

         pvGet(moveAXTilt);
         pvGet(moveAYTilt);
         pvGet(moveBXTilt);
         pvGet(moveBYTilt);
         pvGet(moveCXTilt);
         pvGet(moveCYTilt);
         pvGet(moveZFocus);
         pvGet(moveXPosition);
         pvGet(moveYPosition);


         currFocus = scsPtr->page1.zFocus;
         printf ("proceedWithMove - currFocus %f moveZFocus %f\n",currFocus,moveZFocus);
         if( (abs(currFocus-moveZFocus)) > 1200 )
         {
            errorLog ("receiveTcsDemand - Z Demand Out of Limit", 1, ON);
         }

         /* copy values to demands structure for access by SNL */

         if(semTake(setPointFree, SEM_TIMEOUT) == OK)
         {
            setPoint.xTiltA = moveAXTilt;
            setPoint.yTiltA = moveAYTilt;
            setPoint.xTiltB = moveBXTilt;
            setPoint.yTiltB = moveBYTilt;
            setPoint.xTiltC = moveCXTilt;
            setPoint.yTiltC = moveCYTilt;
            setPoint.zFocus = moveZFocus;
            setPoint.xPosition = moveXPosition;
            setPoint.yPosition = moveYPosition;

            semGive(setPointFree);
         }
         else
         {
            logMsg("startMove - setPointFree timeout\n", 
                  0, 0, 0, 0, 0, 0);
         }
         servoInPosition = 0; 

      }   state moveWaitForCoincidence

      when(delay(SERVO_OFF_TIMEOUT))
         /* SCS has not detected that the mirror control have been turned off */
         /* so aborts the move                       */
      {
         cadProcessorState = SERVOOFFTIMEOUT;
         pvPut(cadProcessorState);

         printf("proceedWithMove - Servo off timeout n");
         errorLog
            ("proceedWithMove - M2 system not switching servos off",
             1, ON);

         moveCarErrIn = STATUS_ERROR;
         pvPut(moveCarErrIn);

         strncpy(moveCarMessIn, 
               "M2 not switching mirror control off", MAX_STRING_SIZE - 1);
         pvPut(moveCarMessIn);

         moveCarIn = CAR_ERROR;
         pvPut(moveCarIn);

      }   state idle
   }

   state moveWaitForCoincidence
   {
      /* the move CAD record shall be considered done when the 
       * mechanism is first in position
       */

      when(delay(30.0) && (servoInPosition == 1) && ( XYmovingStatus == 0 ))
      {
         cadProcessorState = MOVEWAITFORCOINCIDENCE;
         pvPut(cadProcessorState);

         printf("in close range - turning on mirror control \n");

         writeCommand(VSTART);
         writeCommand(MSTART); 

         /* send idle indication to the moveC record */

         moveCarIn = CAR_IDLE;
         pvPut(moveCarIn);

         /* clear message field */

         strncpy(moveCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(moveCarMessIn);

      }   state idle

      when(delay(MOVE_TIMEOUT))
      {
         cadProcessorState = MOVETIMEOUTFORCOINCIDENCE;
         pvPut(cadProcessorState);

         printf("startMove - timeout on in position\n");
         errorLog("startMove - M2 system not achieving coincidence in time", 1, ON);

         moveCarErrIn = STATUS_ERROR;
         pvPut(moveCarErrIn);

         strncpy(moveCarMessIn, "M2 failed to achieve coincidence", MAX_STRING_SIZE - 1);
         pvPut(moveCarMessIn);

         moveCarIn = CAR_ERROR;
         pvPut(moveCarIn);

      }   state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startActuator
    *  waitForActuatorStart
    *  waitForCoincidence
    *
    *  Purpose:
    *  Set m2 to the moving state and apply the tilt demands then monitor for
    *  achieving the in  position indication.
    *
    *  Description:
    *  The 'in position' indication from the m2 system is monitored.  When in
    *  position is first achieved the actuator command is considered completed
    *  and the CAR state set back to idle. If either the command is ignored or
    *  in position is not achieved in time, error routines are invoked setting
    *  CAR status to ERR with error messages
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  !   myscsState
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startActuator
   {
      when()
      {
         cadProcessorState = STARTACTUATOR;
         pvPut(cadProcessorState);

         writeCommand(ACT_PWR_ON);
         writeCommand(MEND);
         writeCommand(VEND);
         writeCommand(MOFFLON);


      } state proceedWithActMove
   }

   state proceedWithActMove
   {
      when(servoOnStatus == 0)
      {
         printf("mirror control disabled - proceed with move\n");

         myscsState = MOVING;
         scsState = myscsState;
         pvPut(myscsState);

         cadProcessorState = PROCEEDWITHACTMOVE;
         pvPut(cadProcessorState);

         followOn = OFF;

         pvGet(actXTilt);
         pvGet(actYTilt);
         pvGet(actZFocus);

         if(semTake(setPointFree, SEM_TIMEOUT) == OK)
         {
            setPoint.xTiltA = actXTilt;
            setPoint.yTiltA = actYTilt;
            setPoint.zFocus = actZFocus;

            semGive(setPointFree);
         }
         else
         {
            logMsg("startActuator - setPointFree timeout\n", 0, 0, 0, 0, 0, 0);
         }

      }   state actuatorWaitForCoincidence

      when(delay(SERVO_OFF_TIMEOUT))
         /* SCS has not detected that the mirror control have been turned off */
         /* so aborts the action                     */
      {

         cadProcessorState = ACTUATORSERVOOFFTIMEOUT;
         pvPut(cadProcessorState);

         printf("proceedWithActMove - Servo off timeout n");
         errorLog
            ("proceedWithActMove - M2 system not switching sevos off",
             1, ON);

         actuatorCarErrIn = STATUS_ERROR;
         pvPut(actuatorCarErrIn);

         strncpy(actuatorCarMessIn, 
               "M2 not switching mirror control off", MAX_STRING_SIZE - 1);
         pvPut(actuatorCarMessIn);

         actuatorCarIn = CAR_ERROR;
         pvPut(actuatorCarIn);

      }   state idle
   }

   state actuatorWaitForCoincidence
   {
      /* the actuator CAD record shall be considered done when the mechanism is first
       * in position
       */

      when(delay(10.0) && (servoInPosition == 1))
      {  
         cadProcessorState = ACTUATORWAITFORCOINCIDENCE;
         pvPut(cadProcessorState);

         printf("in close range - turning on mirror control\n");

         writeCommand(VSTART);
         writeCommand(MSTART);

         /* send idle indication to the moveC record */

         actuatorCarIn = CAR_IDLE;
         pvPut(actuatorCarIn);

         /* clear message field */

         strncpy(actuatorCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(actuatorCarMessIn);

      }   state idle

      when(delay(TIMEOUT))
      {
         cadProcessorState = ACTUATORTIMEOUTFORCOINCIDENCE;
         pvPut(cadProcessorState);

         actuatorCarErrIn = STATUS_ERROR;
         pvPut(actuatorCarErrIn);

         strncpy(actuatorCarMessIn, "M2 failed to achieve coincidence", MAX_STRING_SIZE - 1);
         pvPut(actuatorCarMessIn);

         actuatorCarIn = CAR_ERROR;
         pvPut(actuatorCarIn);

      }   state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startFollow
    *  waitForFollowStart
    *  waitForCoincidence
    *
    *  Purpose:
    *  Set m2 to the following state then monitor for achieving the in  position
    *  indication.
    *
    *  Description:
    *  The follow command is issued to the m2 system whose state is then monitored to 
    *  verify a response. If successful the 'in position' indication from the m2 
    *  system is monitored.  When in position is first achieved the follow command is
    *  considered completed and the CAR state set back to idle.
    *  If either the command is ignored or in position is not achieved in
    *  time, error routines are invoked setting CAR status to ERR with error
    *  messages
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  !   myscsState
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startFollow
   {
      when()
      {
         cadProcessorState = STARTFOLLOW;
         pvPut(cadProcessorState);

         writeCommand(ACT_PWR_ON);
         writeCommand(MEND);
         writeCommand(VEND);
         writeCommand(MOFFLON);

         printf("wait for mirror control to disable\n");

      } state proceedWithFollow
   }

   /* Note 10jul00: may not need these states anymore - there is
      no point to turn servos off at start of follow */

   state proceedWithFollow
   {
      when(servoOnStatus == 0)
      {
         printf("mirror control disabled - proceed with move\n");

         myscsState = MOVING;
         scsState = myscsState;
         pvPut(myscsState);

         cadProcessorState = PROCEEDWITHFOLLOW;
         pvPut(cadProcessorState);

         followOn = ON;    

         /* broadcast the fact that follow has now been turned on to
            receiveTcsDemand */
         resetFirstFollowDemand();  

         servoInPosition = 0;

      }   state waitForCoincidence

      when(delay(SERVO_OFF_TIMEOUT))
         /* SCS has not detected that the mirror control has been turned off */
         /* so aborts the action                     */
      {
         cadProcessorState = FOLLOWSERVOTIMEOUT;
         pvPut(cadProcessorState);

         printf("proceedWithFollow - Servo off timeout n");
         errorLog( "proceedWithFollow - M2 system not switching sevos off",
               1, ON);

         followCarErrIn = STATUS_ERROR;
         pvPut(followCarErrIn);

         strncpy(followCarMessIn, 
               "M2 not switching mirror control off", MAX_STRING_SIZE - 1);
         pvPut(followCarMessIn);

         followCarIn = CAR_ERROR;
         pvPut(followCarIn);

      }   state idle
   }

   state waitForCoincidence
   {
      /* the follow CAD record shall be considered done when the mechanism is first
       * in position although demand positions will still be passed on.
       */

      /* Note: 10jul00: there is no point to delay. The tcsUpdate demands
         are being rate limited, so that demand is very close to
         actual position. This means that "servoInPosition" is true
         right away, even if the tcsUpdate demand as it arrives from
         the TCS differs greatly. Here, we must let the size of the
         allowed step be what protects the servo control. 
         when(delay(30.0) && (servoInPosition == 1) && ( XYmovingStatus == 0 ))
       */

      when(delay(30.0) && (servoInPosition == 1))
      {
         cadProcessorState = FOLLOWWAITFORCOINCIDENCE;
         pvPut(cadProcessorState);

         printf("in close range - turning on mirror control\n");

         writeCommand(VSTART);
         writeCommand(MSTART);

         /* send idle indication to the followC record */

         followCarIn = CAR_IDLE;
         pvPut(followCarIn);

         /* clear message field */

         strncpy(followCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(followCarMessIn);

         followOn = ON;   

         /* broadcast the fact that follow has now been turned on to
            receiveTcsDemand */
         /* resetFirstFollowDemand();  */


      }   state idle

      when(delay(TIMEOUT)) /* Is not an error, XY positioner so slow */
      {
         cadProcessorState = FOLLOWWAITFORCOINCIDENCE;
         pvPut(cadProcessorState);

         printf("in close range - turning on mirror control\n");

         writeCommand(VSTART);
         writeCommand(MSTART);

         /* send idle indication to the followC record */

         followCarIn = CAR_IDLE;
         pvPut(followCarIn);

         /* clear message field */

         strncpy(followCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(followCarMessIn);

         followOn = ON;   


         /*  cadProcessorState = FOLLOWTIMEOUT;
             pvPut(cadProcessorState);

             errorLog("startFollow - timeout on in position", 1, ON);
             followCarErrIn = STATUS_ERROR;
             pvPut(followCarErrIn);

             strncpy(followCarMessIn, "M2 failed to achieve coincidence", 
             MAX_STRING_SIZE - 1);
             pvPut(followCarMessIn);

             followCarIn = CAR_ERROR;
             pvPut(followCarIn); */

      }   state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startStop
    *  waitForStopStart
    *
    *  Purpose:
    *  Issue the stop command to the m2 system monitoring for response and 
    *  completion with timeout protection. The STOP command ends the following
    *  process preventing m2 slaving to further changes in position demand
    *
    *  Description:
    *  The stop command is issued to the m2 system whose state is then monitored
    *  to verify a response. When the m2 system indicates it is in the idle state
    *  the CAR status is set back to IDLE. If the command is ignored the CAR status
    *  is set to ERR
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  !   myscsState
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startStop
   {
      when()
      {
         myscsState = MOVING; 
         scsState = myscsState;
         pvPut(myscsState);

         cadProcessorState = STARTSTOP; 
         pvPut(cadProcessorState);

         /* turn off chop */

         writeCommand(CHOP_OFF);
         /* writeCommand(VDECS_OFF); */
         chopIsOn = 0;

         /* turn off following */

         followOn = OFF;

         /* turn off guiding */

         guideOn = OFF;

         strncpy(stopCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(stopCarMessIn);

         /* send idle indication to the stopC record */

         stopCarIn = CAR_IDLE;
         /*pvPut(stopCarIn);*/ /* <-- why is this commented out?? */

         /* clear message field */

         strncpy(stopCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(stopCarMessIn);

      }   state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startPark
    *  waitForParkStart
    *
    *  Purpose:
    *  Issue the park command to the m2 system monitoring for response and 
    *  completion with timeout protection.
    *
    *  Description:
    *  The park command is issued to the m2 system whose state is then monitored
    *  to verify a response. When the m2 system indicates it is in the parked
    *  state the CAR status is set back to IDLE. If the command is ignored the
    *  CAR status is set to ERR.
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  !   myscsState
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startPark
   {
      when()
      {
         cadProcessorState = STARTPARK;
         pvPut(cadProcessorState);

         /* write busy to park CAR */
         parkCarIn = CAR_BUSY;
         pvPut(parkCarIn);

         /* turn off chop */

         writeCommand(DECS_OFF);
         /* writeCommand(VDECS_OFF); */
         writeCommand(CHOP_OFF);

         chopIsOn = 0;

         /* turn off guiding */

         guideOn = OFF;

         /* turn off following */

         followOn = OFF;

         writeCommand(ACT_PWR_ON);
         writeCommand(MEND);
         writeCommand(VEND);
         writeCommand(MOFFLON);

        /* Turn VTK OFF*/
        vtkXCtrl = 0;
        vtkYCtrl = 0;
        pvPut(vtkXCtrl);
        pvPut(vtkYCtrl);

        vtkXCtrlDir = 2;
        pvPut(vtkXCtrlDir);
        vtkXCtrlDir = 3;
        pvPut(vtkXCtrlDir);

        vtkYCtrlDir = 2;
        pvPut(vtkYCtrlDir);
        vtkYCtrlDir = 3;
        pvPut(vtkYCtrlDir);


      } state proceedWithPark
   }

   state proceedWithPark
   {
      when(servoOnStatus == 0)
      {
         printf("mirror control disable - proceed with move\n");

         myscsState = PARKED;
         scsState = myscsState;
         pvPut(myscsState);

         /* write park demand conditions to the control loops */

         parkPosition.xTilt  = xTiltAdef;
         parkPosition.yTilt  = yTiltAdef;
         parkPosition.zFocus = zFocusDef;
         parkPosition.xPos   = xPosDef;
         parkPosition.yPos   = yPosDef;

         /*             tcs2m2(&parkPosition); */

         printf(" xTiltA %f \n",xTiltAdef);
         printf(" yTiltA %f \n",yTiltAdef);
         printf(" zfocus %f \n",zFocusDef);
         printf(" xPos %f \n",xPosDef);
         printf(" yPos %f \n",yPosDef);


         if(semTake(setPointFree, SEM_TIMEOUT) == OK)
         {
            setPoint.xTiltA = parkPosition.xTilt;
            setPoint.yTiltA = parkPosition.yTilt;
            setPoint.zFocus = parkPosition.zFocus;
            setPoint.xTiltB = 0.0;
            setPoint.yTiltB = 0.0;
            setPoint.xTiltC = 0.0;
            setPoint.yTiltC = 0.0;
            setPoint.xPosition = parkPosition.xPos;
            setPoint.yPosition = parkPosition.yPos;

            semGive(setPointFree);
         }
         else
         {
            logMsg("proceedWithPark - setPointFree timeout\n", 0, 0, 0, 0, 0, 0);
         }

      }   state parkWaitForCoincidence

      when(delay(SERVO_OFF_TIMEOUT))
         /* SCS has not detected that the mirror control have been turned off */
         /* so aborts the move                       */
      {
         cadProcessorState = SERVOOFFTIMEOUT;
         pvPut(cadProcessorState);

         printf("proceedWithPark - Servo off timeout n");
         errorLog
            ("proceedWithPark - M2 system not switching sevos off",
             1, ON);

         parkCarErrIn = STATUS_ERROR;
         pvPut(parkCarErrIn);
         strncpy(parkCarMessIn,
               "M2 not switching mirror control off", MAX_STRING_SIZE - 1);
         pvPut(parkCarMessIn);

      } state idle
   }

   state parkWaitForCoincidence
   {
      /* wait for system to achieve the desired position */

      when(delay(30.0) && (servoInPosition == 1) && ( XYmovingStatus == 0 ))
      {
         cadProcessorState = PARKWAITFORCOINCIDENCE; 
         pvPut(cadProcessorState);

         writeCommand(VEND);
         writeCommand(MEND);

         /* send idle indication to the parkC record */

         parkCarIn = CAR_IDLE;
         pvPut(parkCarIn);
         strncpy(parkCarMessIn, "", MAX_STRING_SIZE - 1);

      }   state idle


      when(delay(TIMEOUT))
      {
         cadProcessorState = TIMEOUTWAITFORPARK; 
         pvPut(cadProcessorState);

         parkCarErrIn = STATUS_ERROR;
         pvPut(parkCarErrIn);

         strncpy(parkCarMessIn, "No response to park command", MAX_STRING_SIZE - 1);
         pvPut(parkCarMessIn);

         parkCarIn = CAR_ERROR;
         pvPut(parkCarIn);

      }   state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startChopControl
    *  waitForChopResponse
    *
    *  Purpose:
    *  write the relevant chop control parameters to the m2 system
    *
    *  Description:
    *  The CAD parameters are written to the m2 system. Once the command 
    *  for chop on or off is issued the SCS waits for m2 to begin acting
    *  on the command before updating its own state.  Once the reponse has
    *  been received the CAR state is set back to IDLE else ERR
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  !   myscsState
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startChopControl
   {
      /* the SCS merely passes on the chop control variables to the M2 system */

      when()
      {
         cadProcessorState = STARTCHOPCONTROL; 
         pvPut(cadProcessorState);

         /* fetch the values from the chopControl CAD record and copy to M2 */

         pvGet(syncSource);
         pvGet(profile);

         pvGet(dutyCycle);
         pvGet(chopOn);
         pvGet(decsOn);
         pvGet(decsPause);
         pvGet(decsFreeze);
         pvGet(decsReset);
         if(chopIsOn)
         {
            if(chopOn == 1)
            {
               strncpy(chopControlCarMessIn, "Already Chopping",MAX_STRING_SIZE - 1);
               pvPut(chopControlCarMessIn);
            }
            else
            {
               writeCommand(CHOP_OFF); 
               chopIsOn = 0;
               if (eventConfig.change == 0)
               {
                  eventConfig.change  = 1;
                  if (eventConfig.on != chopOn)
                  {
                     eventConfig.onChange = 1;
                  }

                  eventConfig.on      = chopOn;
                  eventConfig.profile = profile; 

               }
               else
               {
                  errorLog("startChopControl - chop mode change not acknowledged by event system", 1, ON);
               }
               writeCommand(CHOP_OFF); 
               semGive(eventSem); 
            }
            return;
         }
         else
         { 

            /* write the configuration out to the M2 system */

            /* order IS important 
               turn DECS off before CHOP off and
               turn CHOP on before DECS on */

            if(decsOn == 0)
            {
               /* writeCommand(DECS_OFF);
                  printf("startChopControl - sending first command to M2 to turn decs off\n");  */
            }

            if(chopOn == 1)
            {
               /* ensure that jogBeam is reset */

               jogBeam = BEAMA;

               /*
                  writeCommand(CHOP_ON);
                  printf("startChopControl - sending command to M2 to turn chop on\n");
                  writeCommand(VDECS_ON);
                  chopIsOn = 1;
                */

               chopIsPending = 1; /* use this when defining PENDING in xycom.c */
               decsIsPending = 1;
            }
            else
            {
               /* a precaution in case of CHOP off without DECS off */
               /* writeCommand(DECS_OFF);    
                  printf("startChopControl - sending second command to M2 to turn decs off\n"); */

               /* writeCommand(VDECS_OFF); */

               /* IMPORTANT: Ensure that chop stops in beam A... 
                  Do this in the eventHandler routine */
               writeCommand(CHOP_OFF); 

               /*printf("startChopControl - sending command to M2 to turn chop off\n");*/
               chopIsOn = 0;
            }

            if (decsOn == 1)
            {
               if (!chopIsPending)
               {
                  /*  printf("startChopControl - if (!chopIsPending)\n");
                      writeCommand(DECS_ON); */
               }
               else
               {
                  /*  printf("startChopControl - set decsIsPending = 1\n"); */
                  decsIsPending = 1;
               }
            }

            if(decsPause == 1)
            {
               writeCommand(DECS_PAUSE);
               /*printf("startChopControl - sending command to M2 to pause decs\n");*/
            }
            else
            {
               writeCommand(DECS_CONTINUE);
               /*printf("startChopControl - sending command to M2 to continue decs\n");*/
            }

            if(decsFreeze == 1)
            {
               writeCommand(DECS_FREEZE);
               /*printf("startChopControl - sending command to M2 to freeze decs\n");*/
            }
            else
            {
               writeCommand(DECS_UNFREEZE);
               /*printf("startChopControl - sending command to M2 to unfreeze decs\n");*/
            }

            if(decsReset == 1)
            {
               writeCommand(DECS_ZERO);
               /*printf("startChopControl - sending command to M2 to zero decs\n");*/
            }


            /* write updated chop config information to event bus handler */

            /* update event system configuration structure */


            if (eventConfig.change == 0)
            {
               /* Set the flag to say that at least one of the terms
                  in the eventConfig structure has changed */
               eventConfig.change  = 1;

               /* Figure out if there has been a change to eventConfig.on 
                  I only want to make it execute the code in the 
                  eventHandler routine iff the value in eventConfig.on 
                  has changed. I.e., if we are just turning decs off and
                  chopping has been on and is staying on, then do not
                  process the code that gets run when chopping is being
                  turned on */

               /* 	logMsg("eventConfig.on is %d and chopOn is %d\n",
                        eventConfig.on, eventConfig.onChange, 0, 0, 0, 0); */
               if (eventConfig.on != chopOn)
               {
                  eventConfig.onChange = 1;
               }

               eventConfig.on      = chopOn;
               eventConfig.profile = profile; 

               if (syncSource == 0)
               {
                  eventConfig.drive = LMMS;
                  /*printf("chop is being driven by LMMS\n");*/
               }
               else
               {
                  eventConfig.drive = INST;
                  /*printf("chop is being driven by an ICS\n");*/
               }

               eventConfig.source  = instruments[syncSource].port;

               /*printf("startChopControl - source =%d capability = '%s'\n", 
                 syncSource, instruments[syncSource].capability);*/

               if(!strcmp(instruments[syncSource].capability, "ONELINE"))
                  eventConfig.capability  = ONELINE;
               else if(!strcmp(instruments[syncSource].capability, "TWOLINE"))
                  eventConfig.capability  = TWOLINE;
               else if(!strcmp(instruments[syncSource].capability, "ONESHOT"))
                  eventConfig.capability  = ONESHOT;
               else
                  eventConfig.capability  = STROBE;

               /*printf("startChopControl -  eventConfig.capability is %d\n",eventConfig.capability);*/
            }
            else
            {
               errorLog("startChopControl - chop mode change not acknowledged by event system", 1, ON);
            }
            /* This clause does not seem to be necessary - assignments
             * done above.
             */
            if(chopOn == 1)
            {
               chopIsOn = ON;
            }
            else
            {
               chopIsOn = OFF;
            }

            printf("startChopControl - giving eventSem semaphore\n");
            semGive(eventSem); 
         }   

      }   state waitForChopResponse   
   }

   state waitForChopResponse
   {
      /* if SCS moving, wait for m2 to start chopping then update SCS state */

      when(chopResponse == 1 && chopOn == 1)  /* CHOPPING IS ON IN M2 and SCS */
      {
         m2ChopResponseOK = 1;
         /* printf("startChopControl - got m2ChopResponse\n"); */

         cadProcessorState = WAITFORCHOPRESPONSE;/*better = GOTCHOPRESPONSE*/  
         pvPut(cadProcessorState);

         /* send idle indication to the chopControlC record */

         chopControlCarIn = CAR_IDLE;
         pvPut(chopControlCarIn);

         strncpy(chopControlCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(chopControlCarMessIn);

      }   state idle

      when(chopResponse == 0 && chopOn == 0)  /* CHOPPING IS OFF IN BOTH M2 AND SCS */
      {
         m2ChopResponseOK = 0;

         myscsState = MOVING;  /* required state for chopping to be on */ 
         scsState = myscsState;
         pvPut(myscsState);

         cadProcessorState = WAITFORCHOPRESPONSE;/*better = GOTCHOPRESPONSE*/
         pvPut(cadProcessorState);

         /* send idle indication to the chopControlC record */

         chopControlCarIn = CAR_IDLE;
         pvPut(chopControlCarIn);

         strncpy(chopControlCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(chopControlCarMessIn);

         chopConfigDir = 3;
         pvPut(chopConfigDir);

      }   state idle

      when(delay(CHOP_TIMEOUT)) 
      {
         /* Do not update m2ChopResponseOK */

         cadProcessorState = TIMEOUTWAITFORCHOPRESPONSE; 
         pvPut(cadProcessorState);

         chopControlCarErrIn = STATUS_ERROR;
         pvPut(chopControlCarErrIn);

         strncpy(chopControlCarMessIn, "M2 no response to chop on/off command", MAX_STRING_SIZE - 1);
         pvPut(chopControlCarMessIn);

         chopControlCarIn = CAR_ERROR;
         pvPut(chopControlCarIn);

      }   state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startBandwidth
    *
    *  Purpose:
    *  write bandwidth data to the m2 system.
    *
    *  Description:
    *  The requested bandwidth is read from the CAD record and written
    *  to the reflective memory page. A change bandwidth command is then
    *  sent to the M2 system
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  None
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startBandwidth
   {
      when()
      {
         cadProcessorState = STARTBANDWITH; 
         pvPut(cadProcessorState);

         pvGet(bandwidth);

         %{
            if(semTake(refMemFree, SEM_TIMEOUT) ==  OK)
            {
               scsPtr->page0.bandwidth = bandwidth;
               semGive(refMemFree);
            }
            else
            {
               errorLog("startBandwidth - refMemFree timeout", 1, ON);
            }
         }%

         writeCommand(BANDWIDTH_CHANGE);

         strncpy(bandwidthCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(bandwidthCarMessIn);

      }   state idle
   }

   /* ==================================================================== */
   /* INDENT OFF */
   /*
    * State Group Names:
    */
   state startVtkChange
   {
        when()
        {

            vtkXScale = guideInfo.vtkXdata[0];
            vtkXAngle = guideInfo.vtkXdata[1];
            vtkXSampleRate = guideInfo.vtkXdata[2];
            pvPut(vtkXScale);
            pvPut(vtkXAngle);
            pvPut(vtkXSampleRate);

            vtkYScale = guideInfo.vtkYdata[0];
            vtkYAngle = guideInfo.vtkYdata[1];
            vtkYSampleRate = guideInfo.vtkYdata[2];
            pvPut(vtkYScale);
            pvPut(vtkYAngle);
            pvPut(vtkYSampleRate);

        } state idle
   }

   /* ==================================================================== */
   /* INDENT OFF */
   /*
    * State Group Names:
    * guideSimChange
    *
    * Purpose:
    * Start the guide simulator running if were turing it on. Otherwise lets stop it.
    */

   state startGuideSimChange {
      when() {

         if (guideSimStart == myOldGuideSimState){
            puts("guideSim no state change");
         }

         else if(guideSimStart == 1) {
            myOldGuideSimState = 1;

            tiltPidOn = 0;
            pvPut(tiltPidOn);
        
            tiltpidDir = 2;
            pvPut(tiltpidDir);

            tiltpidDir = 3;
            pvPut(tiltpidDir);

            focusPidOn = 0;
            pvPut(focusPidOn);
            
            focuspidDir = 2;
            pvPut(focuspidDir);

            focuspidDir = 3;
            pvPut(focuspidDir);

            startGuideSim();
         }

         else
         {
            myOldGuideSimState = 0;

            tiltPidOn = 1;
            pvPut(tiltPidOn);
        
            tiltpidDir = 2;
            pvPut(tiltpidDir);

            tiltpidDir = 3;
            pvPut(tiltpidDir);

            focusPidOn = 1;
            pvPut(focusPidOn);
            
            focuspidDir = 2;
            pvPut(focuspidDir);

            focuspidDir = 3;
            pvPut(focuspidDir);

            endGuideSim();
         }

      } state idle
   }

   /* ===================================================================== */
   /* INDENT OFF */
   /* 
    *  State Group Names:
    *  startTolerance
    *
    *  Purpose:
    *  write in position tolerance data to the m2 system.
    *
    *  Description:
    *  The tolerance data is read from the CAD record. This data is then 
    *  copied to the m2 system and the the CAR record is set back to IDLE
    *  
    *  Invocation:
    *  Called from state waitForCommand
    *
    *  Parameters: (">" input, "!" modified, "<" output)
    *  None
    *
    */
   /* INDENT ON */
   /* ===================================================================== */

   state startTolerance {
      when() {
         cadProcessorState = STARTTOLERANCE; 
         pvPut(cadProcessorState);

         /* fetch the values from the tolerance CAD record and
          * copy to M2
          */

         pvGet(xTiltTol);
         pvGet(yTiltTol);
         pvGet(zFocusTol);
         pvGet(xPosTol);
         pvGet(yPosTol);
         pvGet(xyPosDeadband);

         %{
            if(semTake(refMemFree, SEM_TIMEOUT) == OK)
            {
               scsPtr->page0.xTiltTolerance        = (float) xTiltTol;
               scsPtr->page0.yTiltTolerance        = (float) yTiltTol;
               scsPtr->page0.zFocusTolerance       = (float) zFocusTol;
               scsPtr->page0.xPositionTolerance    = (float) xPosTol;
               scsPtr->page0.yPositionTolerance    = (float) yPosTol;          
               scsPtr->page0.xyPositionDeadband    = (float) xyPosDeadband;          
               semGive(refMemFree);
            }
            else
            {
               errorLog("startTolerance - timeout on refMemFree", 1, ON);
            }
         }%

         writeCommand(TOLERANCE_CHANGE);
         writeCommand(XY_DEADBAND_CHANGE);

         strncpy(toleranceCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(toleranceCarMessIn);

         printf("Registered default tolerances with CEM:\n");
         printf("  xTiltTolerance:   %f\n", xTiltTol);
         printf("  yTiltTolerance:   %f\n", yTiltTol);
         printf("  zFocusTolerance:  %f\n", zFocusTol);
         printf("  xPosTolerance:    %f\n", xPosTol);
         printf("  yPosTolerance:    %f\n", yPosTol);
         printf("  xyDeadbandTol:    %f\n", xyPosDeadband);

      }   state idle
   }
}

/* ===================================================================== */
/* INDENT OFF */
/* 
 *  Function Name:
 *  State set "followDemandProcessing"
 *
 *  Purpose:
 *  Sanity check position demands received whilst in the following state
 *  and pass on to the m2 system.
 *
 *  Description:
 *  When the scs is in the moving state and a new position demand set arrives
 *  as indicated by the event flag linked to the time sent data item, the
 *  demanded positions are read from their records. The following checks
 *  are applied. If the time sent is outside a given window of the current time
 *  or the demand values are outside their absolute limits the data set is
 *  considered invalid and ignored. If all is well the data is passed on
 *  to the m2 system.
 *
 *  Invocation:
 *  Activated by the follow flag being set and the arrival of a
 *  position demand data set
 *
 *  Parameters: (">" input, "!" modified, "<" output)
 *  None
 *  
 *  Function value:
 *  n/a
 *
 *  External functions:
 *  The following SNL functions have been used
 *
 *  pvPut       write a value to a database channel
 *  pvGet       read a value from a database channel
 *  efClear     clear an event flag
 *  efTest      check an event flag
 *
 *  Deficiencies:
 *  The state does not perform interpolation of the demanded values
 */
/* INDENT ON */
/* ===================================================================== */

ss followDemandProcessing {
   state entry {
      when() {
         followDemandState = FOLDEMANDENTRY; 
         pvPut(followDemandState);

      }   state waitForDemand
   }

   state waitForDemand {
      when(efTest(newBeamA) && (followOn == ON)) {
         followDemandState = WAITFORDEMAND; 
         pvPut(followDemandState);

         efClear(newBeamA);

         /* new track to follow ? */
         if(tcs.trackId != currentId && followOn) {
            currentId = tcs.trackId;
            activeC = CAR_BUSY;
            pvPut(activeC); 

         }

      }   state waitForNextCoincidence
   }

   state waitForNextCoincidence {
      when(delay(0.5) && coincidence == ENABLED) {
         followDemandState = WAITFORNEXTCOINCIDENCE; 
         pvPut(followDemandState);

         activeC = CAR_IDLE;
         pvPut(activeC);

      }   state waitForDemand

      when(delay(TIMEOUT)) {
         followDemandState = TIMEOUTWAITFORNEXTCOINCIDENCE; 
         pvPut(followDemandState);

         /* activeC = CAR_ERROR;
            pvPut(activeC); */

      }   state waitForDemand
   }
}

/* ===================================================================== */
/* INDENT OFF */
/* 
 *  Function Name:
 *  State set "monitorSad"
 *
 *  Purpose:
 *  Refresh the SAD database
 *
 *  Description:
 *  The monitor process updates the SAD database at a refresh rate of 1Hz
 *
 *  Invocation:
 *  1.0 second timer elapse
 *
 *  Parameters: (">" input, "!" modified, "<" output)
 *  None
 *  
 *  Function value:
 *  n/a
 *
 *  External functions:
 *  The following SNL functions have been used
 *
 *  pvPut       write a value to a database channel
 *  pvGet       read a value from a database channel
 *
 *  Deficiencies:
 *
 */
/* INDENT ON */
/* ===================================================================== */

ss monitorSad {
   state entry {
      when() {
         monitorSadState = MONSADENTRY; 
         pvPut(monitorSadState);

      }   state updateSad
   }

   state updateSad {
      when(delay(1.0)) {
         monitorSadState = UPDATESAD; 
         pvPut(monitorSadState);

         if(semTake(refMemFree, SEM_TIMEOUT) == OK) {

            if(scsPtr->page1.inPosition != 0 || followOn || guideOn)
               strncpy(inPositionSad, "TRUE", MAX_STRING_SIZE - 1);
            else
               strncpy(inPositionSad, "FALSE", MAX_STRING_SIZE - 1);


            /*strncpy(topEndSad, topEndNames[scsPtr->page1.topEnd], 
              MAX_STRING_SIZE - 1);*/

            /* Write position of deployable baffle to SAD, 0, 1, 2 and 3 
               are valid positions. Any other is treated as moving */

            if ((scsPtr->page1.deployBaffle == 0) | 
                  (scsPtr->page1.deployBaffle == 1) | 
                  (scsPtr->page1.deployBaffle == 2) | 
                  (scsPtr->page1.deployBaffle == 3)) {
               strncpy(deployableSad, 
                     depBaffle[scsPtr->page1.deployBaffle], 
                     MAX_STRING_SIZE - 1);

               moveBaffleCar = CAR_IDLE;
            }
            else {
               strncpy(deployableSad, "MOVING", MAX_STRING_SIZE - 1);
               moveBaffleCar = CAR_BUSY;
            }

            /* Write position of central baffle to SAD, 0 and 1 
               are valid positions. Any other is treated as moving */

            if ((scsPtr->page1.centralBaffle == 0) | 
                  (scsPtr->page1.centralBaffle == 1)) {
               strncpy(centralSad, 
                     cenBaffle[scsPtr->page1.centralBaffle], 
                     MAX_STRING_SIZE - 1);

               moveBaffleCar = CAR_IDLE;
            }

            else {
               strncpy(centralSad, "MOVING", MAX_STRING_SIZE - 1);
               moveBaffleCar = CAR_BUSY;
            }

            tempSad = scsPtr->page1.enclosureTemp;
            semGive(refMemFree);
         }
         else
         {
            logMsg("updateSad - refMemFree timeout\n", 0, 0, 0, 0, 0, 0);
         }

         pvPut(inPositionSad);
         /*pvPut(topEndSad);*/
         pvPut(deployableSad);
         pvPut(centralSad);
         pvPut(moveBaffleCar);
         pvPut(tempSad);

         /* show chopping state */

         if(chopIsOn == ON)
            strncpy(chopping, "YES", MAX_STRING_SIZE - 1);
         else
            strncpy(chopping, "NO", MAX_STRING_SIZE - 1);

         pvPut(chopping);

         /* update follow S indicator */

         followS = followOn;
         pvPut(followS);

         /* check m2 error status bits */

         %%checkTiltStatus();

         /* update guider screens */

         x[0] = filtered[PWFS1].z1;
         y[0] = filtered[PWFS1].z2;
         z[0] = filtered[PWFS1].z3;
         xerr[0] = filtered[PWFS1].err1;
         yerr[0] = filtered[PWFS1].err2;
         zerr[0] = filtered[PWFS1].err3;

         x[1] = filtered[PWFS2].z1;
         y[1] = filtered[PWFS2].z2;
         z[1] = filtered[PWFS2].z3;
         xerr[1] = filtered[PWFS2].err1;
         yerr[1] = filtered[PWFS2].err2;
         zerr[1] = filtered[PWFS2].err3;

         x[2] = filtered[OIWFS].z1;
         y[2] = filtered[OIWFS].z2;
         z[2] = filtered[OIWFS].z3;
         xerr[2] = filtered[OIWFS].err1;
         yerr[2] = filtered[OIWFS].err2;
         zerr[2] = filtered[OIWFS].err3;

         x[3] = filtered[GAOS].z1;
         y[3] = filtered[GAOS].z2;
         z[3] = filtered[GAOS].z3;
         xerr[3] = filtered[GAOS].err1;
         yerr[3] = filtered[GAOS].err2;
         zerr[3] = filtered[GAOS].err3;

         x[4] = filtered[GYRO].z1;
         y[4] = filtered[GYRO].z2;
         z[4] = filtered[GYRO].z3;
         xerr[4] = filtered[GYRO].err1;
         yerr[4] = filtered[GYRO].err2;
         zerr[4] = filtered[GYRO].err3;

         xWindup = controller[XTILT].sum;
         yWindup = controller[YTILT].sum;
         zWindup = controller[FOCUS].sum;

         /* Question for someday... this never seems to get very high.
            ie, instead of being the "sum" it looks more like the 
            instantaneous guide */
         pvPut(xWindup);
         pvPut(yWindup);
         pvPut(zWindup);

         for(source = 0; source < 5; source++) {
            pvPut(x[source]);
            pvPut(y[source]);
            pvPut(z[source]);
            pvPut(xerr[source]);
            pvPut(yerr[source]);
            pvPut(zerr[source]);
         }   

      }   state updateSad
   }
}

/* ===================================================================== */
/* INDENT OFF */
/* 
 *  Function Name:
 *  State set "monitorProcess"
 *
 *  Purpose:
 *  Refresh the public data
 *
 *  Description:
 *  Data which must be made available outside the SCS package at a rate faster
 *  than the SAD updates
 *
 *  Invocation:
 *  0.1 second timer elapse
 *
 *  Parameters: (">" input, "!" modified, "<" output)
 *  None
 *  
 *  Function value:
 *  n/a
 *
 *  External functions:
 *  The following SNL functions have been used
 *
 *  pvPut       write a value to a database channel
 *  pvGet       read a value from a database channel
 *
 *  Deficiencies:
 *
 */
/* INDENT ON */
/* ===================================================================== */

ss monitorProcess {
   state updateParameters {
      when(delay(0.2)) {
         monitorProcessState = UPDATEPARAMETERS; 
         pvPut(monitorProcessState);

         if(semTake(refMemFree, SEM_TIMEOUT) == OK) {
            /* invert sense of in position*/
            coincidence = !scsPtr->page1.inPosition;
            semGive(refMemFree);
         }
         else {
            logMsg("updateParameters - refMemFree timeout\n", 0, 0, 0, 0, 0, 0);
         }

         pvPut(coincidence);

      } state updateParameters
   }
}


/* ===================================================================== */
/* INDENT OFF */
/* 
 *  Function Name:
 *  State set "rebootScs"
 *
 *  State Group Names:
 *  initReboot
 *  waitForRebootCmd
 *  startReboot
 *  waitForParkReboot
 *  rebootNow
 *
 *  Purpose:
 *  To execute the reboot command
 *
 *  Description:
 *  Waits for the reboot command and executes it
 *  The reboot CAR status is set BUSY, a reset command is issued to the
 *  tilt system and the vxWorks reboot command executed
 *
 *  Invocation:
 *  Runs continuously
 *
 *  Parameters: (">" input, "!" modified, "<" output)
 *  None
 *  
 *  Function value:
 *  n/a
 *
 *  External functions:
 *  The following SNL functions have been used
 *
 *  pvPut       write a value to a database channel
 *  pvGet       read a value from a database channel
 *
 *  Deficiencies:
 *
 *  Author:     J.F.Maclean
 *
 */
/* INDENT ON */
/* ===================================================================== */

ss rebootScs {
   state initReboot {
      when() {
         rebootScsState = INITREBOOT;
         pvPut(rebootScsState);

         efClear(rebootCadChange);
      } state waitForRebootCmd
   }

   state waitForRebootCmd {
      when(efTest(rebootCadChange)) {
         rebootScsState = WAITFORREBOOTCMD;
         pvPut(rebootScsState);

         /* clear event flag */
         efClear(rebootCadChange);
      } state startReboot
   }

   state startReboot {
      /* first park the system to ensure safe shutdown */

      when() {
         rebootScsState = STARTREBOOT;
         pvPut(rebootScsState);

         /* write busy to reboot CAR */
         rebootCarIn = CAR_BUSY;
         pvPut(rebootCarIn);

         /* write busy to park CAR */
         parkCarIn = CAR_BUSY;
         pvPut(parkCarIn);

         /* turn off chop */

         writeCommand(DECS_OFF);
         /* writeCommand(VDECS_OFF); */
         writeCommand(CHOP_OFF);

         chopIsOn = 0;

         /* turn off guiding */

         guideOn = OFF;

         /* turn off following */

         followOn = OFF;

         /* Doing this without turning off servos, causes actators
            to fail. 

            Would be better to use the real PARK SNL to avoid
            redundancy here  (11jul00).
          */

         /* NEW 16-Aug-2000 */
         writeCommand(ACT_PWR_ON);
         writeCommand(MEND);
         writeCommand(VEND);
         writeCommand(MOFFLON);

         printf("startReboot - wait for mirror control to disable\n");
      } state proceedWithRebootPark
   }

   state proceedWithRebootPark {
      when(servoOnStatus == 0) {
         printf("mirror control disable - proceed with move\n");

         myscsState = PARKED;
         scsState = myscsState;
         pvPut(myscsState);

         /* write park demand conditions to the control loops */

         parkPosition.xTilt  = xTiltAdef;
         parkPosition.yTilt  = yTiltAdef;
         parkPosition.zFocus = zFocusDef;
         parkPosition.xPos   = xPosTcsDef;
         parkPosition.yPos   = yPosTcsDef;

         tcs2m2(&parkPosition);

         if(semTake(setPointFree, SEM_TIMEOUT) == OK) {
            setPoint.xTiltA = parkPosition.xTiltNew;
            setPoint.yTiltA = parkPosition.yTiltNew;
            setPoint.zFocus = parkPosition.zFocusNew;
            setPoint.xTiltB = parkPosition.xTiltNew;
            setPoint.yTiltB = parkPosition.yTiltNew;
            setPoint.xTiltC = parkPosition.xTiltNew;
            setPoint.yTiltC = parkPosition.yTiltNew;
            setPoint.xPosition = parkPosition.xPosNew;
            setPoint.yPosition = parkPosition.yPosNew;

            semGive(setPointFree);
         }
         else {
            logMsg("proceedWithRebootPark - setPointFree timeout\n", 0, 0, 0, 0, 0, 0);
         }

      } state rebootParkWaitForCoincidence


      /* SCS has not detected that the mirror control have been turned off so
       * aborts the move 
       */
      when(delay(SERVO_OFF_TIMEOUT))       {
         cadProcessorState = SERVOOFFTIMEOUT;
         pvPut(cadProcessorState);

         printf("proceedWithRebootPark - Servo off timeout n");
         errorLog
            ("proceedWithRebootPark - M2 system not switching sevos off",
             1, ON);

         parkCarErrIn = STATUS_ERROR; /* parkCar or rebootCar ?? */
         pvPut(parkCarErrIn);
         strncpy(parkCarMessIn,
               "M2 not switching mirror control off", MAX_STRING_SIZE - 1);
         pvPut(parkCarMessIn);

      } state rebootParkWaitForCoincidence 

      /* bug fix 14nov2000 - this was 'idle' and we do not want this. If the
       * servos will not turn off, keep going to the next state. Later: figure
       * out why servos do not turn off sometimes. */
   }

   state rebootParkWaitForCoincidence {
      /* wait for system to achieve the desired position */

      when(delay(10.0) && (servoInPosition == 1)) {
         rebootScsState = WAITFORPARKREBOOT;
         pvPut(rebootScsState);

         /* in close range - no need to restart mirror control */

         /* shut down the unused functions */

         writeCommand(VEND);
         writeCommand(MEND);
         writeCommand(MOFFLOFF);
         writeCommand(ACT_PWR_OFF);

         /* NEW for 22-jun-2000: */
         /* Toggle tilt system by writing to the event system */

         if (eventConfig.change == 0) {
            /* if previous changes have been acknowledged, 
               write new data, flag change and give sem  */

            printf("powerCycleCEM - Shutting off CEM power\n");
            eventConfig.change  = 1;
            eventConfig.on = 0;    /* make sure you do not send an ON */
            eventConfig.m2Servo = OFF;
            eventConfig.m2Power = OFF;
            semGive(eventSem); 
         }
         else {
            errorLog("waitForParkReboot - command not acknowledged", 1, ON);
            printf("waitForParkReboot - command not acknowledged\n");
         }
      }   state powerCycleCEM

      when(delay(TIMEOUT)) {
         rebootScsState = TIMEOUTWAITFORPARKREBOOT;
         pvPut(rebootScsState);

         parkCarErrIn = STATUS_ERROR;
         pvPut(parkCarErrIn);

         strncpy(parkCarMessIn, "No response to park command", MAX_STRING_SIZE - 1);
         pvPut(parkCarMessIn);

         parkCarIn = CAR_ERROR;
         pvPut(parkCarIn);

         /* shut down the unused functions */

         writeCommand(VEND);
         writeCommand(MEND);
         writeCommand(MOFFLOFF);
         writeCommand(ACT_PWR_OFF);

         /* NEW for 22-jun-2000: */
         /* Toggle tilt system by writing to the event system */

         printf("state waitForParkReboot\n");
         if (eventConfig.change == 0) {
            /* if previous changes have been acknowledged, 
               write new data, flag change and give sem  */

            printf("waitForParkReboot - Shutting off CEM power\n");
            eventConfig.change  = 1;
            eventConfig.on = 0;    /* make sure you do not send an ON */
            eventConfig.m2Servo = OFF;
            eventConfig.m2Power = OFF;
            semGive(eventSem); 
         }
         else {
            errorLog("waitForParkReboot - command not acknowledged", 1, ON);
            printf("waitForParkReboot - command not acknowledged\n");
         }
      } state powerCycleCEM
   }

   state powerCycleCEM {
      when(delay(1.0)) {
         printf("state powerCycleCEM\n");
         if (eventConfig.change == 0) {
            /* if previous changes have been acknowledged, 
               write new data, flag change and give sem  */

            printf("waitForParkReboot - Turning CEM power back on\n");
            eventConfig.change  = 1;
            eventConfig.on = 0;    /* make sure you do not send an ON */
            eventConfig.m2Servo = ON;
            eventConfig.m2Power = ON;
            semGive(eventSem); 
         }
         else {
            errorLog("powerCycleCEM - command not acknowledged", 1, ON);
            printf("waitForParkReboot - command not acknowledged\n");
         }

         /* send idle indication to the parkC record */

         parkCarIn = CAR_IDLE;
         pvPut(parkCarIn);

         strncpy(parkCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(parkCarMessIn);

      }   state rebootNow
   }


   state rebootNow {
      when(delay (5.0)) {
         rebootScsState = REBOOTNOW;
         pvPut(rebootScsState);

         myscsState = REBOOTING;
         scsState = myscsState;
         pvPut(myscsState);

         scsStateSad = BOOTING;
         pvPut(scsStateSad);

         /* write busy to reboot CAR */

         rebootCarIn = CAR_BUSY;
         pvPut(rebootCarIn);

         /* if logging file is open, close it */

         if(loggingNow != OFF)
            %%scsLogDestruct();

         %%reboot(BOOT_QUICK_AUTOBOOT);

         /* At this point the system shall reboot so no further code */
         /* execution is permitted. The return state has been set to */
         /* startReboot to keep it tidy and local            */

      }   state startReboot
   }
}


/* ===================================================================== */
/* INDENT OFF */
/* 
 *  Function Name:
 *  State set "clearGuideFocus"
 *
 *  State Group Names:
 *  initClearGuideFocus
 *  waitForClearGuideFocusCmd
 *  clearGuideFocus
 *  waitForClearGuideFocus
 *
 *  Purpose:
 *  To execute the clear guide focus command
 *
 *  Description:
 *  Waits for the clear guide focus command and executes it
 *  The Clear Guide Focus CAR status is set BUSY, a reset command is issued to
 *  the system 
 *
 *  Invocation:
 *  Runs continuously
 *
 *  Parameters: (">" input, "!" modified, "<" output)
 *  None
 *  
 *  Function value:
 *  n/a
 *
 *  External functions:
 *  The following SNL functions have been used
 *
 *  pvPut       write a value to a database channel
 *  pvGet       read a value from a database channel
 *
 *  Deficiencies:
 *
 *  Author:     J.F.Maclean
 *
 */
/* INDENT ON */
/* ===================================================================== */

ss clearguidefocusScs
{
   state initClearGuideFocus
   {
      when()
      {
         efClear(clearGuideFocusCadChange);

      } state waitForClearGuideFocusCmd
   }

   state waitForClearGuideFocusCmd
   {
      when(efTest(clearGuideFocusCadChange))
      {

         /* clear event flag */
         efClear(clearGuideFocusCadChange);
      } state startClearGuideFocus
   }

   state startClearGuideFocus
   {

      when()
      {
         clearGuideFocusCarIn = CAR_BUSY;
         pvPut(clearGuideFocusCarIn);

         printf("startClearGuideFocus - wait for clear guide focus value\n");
      } state proceedWithClearGuideFocus
   }

   state proceedWithClearGuideFocus
   {
      when(controller[FOCUS].sum >= 500 )
      {
         controller[FOCUS].sum -= 500.;
         controller[FOCUS].oldSum -= 500.;

         printf ("proceedWith + sum %f oldS %f \n",
               controller[FOCUS].sum, controller[FOCUS].oldSum);

      }   state clearGuideFocusWaitForCoincidence

      when( controller[FOCUS].sum <= -500 )
      {
         controller[FOCUS].sum += 500.;
         controller[FOCUS].oldSum += 500.;

         printf ("proceedWith - sum %f oldS %f \n",
               controller[FOCUS].sum, controller[FOCUS].oldSum);

      }   state clearGuideFocusWaitForCoincidence


      when(controller[FOCUS].sum >= -500 && controller[FOCUS].sum <= 500 )
      {
         printf ("proceedWith +- SUm %f oldS %f \n",
               controller[FOCUS].sum, controller[FOCUS].oldSum);
      }   state clearGuideFocusWaitForCoincidence

      when(delay(TIMEOUT))
      {

         clearGuideFocusCarErrIn = STATUS_ERROR;
         pvPut(clearGuideFocusCarErrIn);

         strncpy(clearGuideFocusCarMessIn, "proceedWith:No response to clear guide focus command", MAX_STRING_SIZE - 1);
         pvPut(clearGuideFocusCarMessIn);

         clearGuideFocusCarIn = CAR_ERROR;
         pvPut(clearGuideFocusCarIn);

      } state clearGuideFocusWaitForCoincidence

   }

   state clearGuideFocusWaitForCoincidence
   {

      when(controller[FOCUS].sum >= -500 && controller[FOCUS].sum <= 500 )
      {
         controller[FOCUS].sum = 0.0;
         controller[FOCUS].oldSum = 0.0;
         controller[FOCUS].oldError = 0.0;

         printf ("clearGuideFocusWaitForCoincidence - sum %f oldS %f \n",
               controller[FOCUS].sum, controller[FOCUS].oldSum);

         clearGuideFocusCarIn = CAR_IDLE;
         pvPut(clearGuideFocusCarIn);

         strncpy(clearGuideFocusCarMessIn, "", MAX_STRING_SIZE - 1);
         pvPut(clearGuideFocusCarMessIn);

      }  state waitForClearGuideFocusCmd

      when( delay(3.0) && controller[FOCUS].sum >= 500 )
      {
         printf ("WaitForCoincidence + Sum %f oldS %f \n",
               controller[FOCUS].sum, controller[FOCUS].oldSum);

      }   state proceedWithClearGuideFocus

      when( delay(3.0) && controller[FOCUS].sum <= -500 )
      {
         printf ("WaitForCoincidence - Sum %f oldS %f \n",
               controller[FOCUS].sum, controller[FOCUS].oldSum);

      }   state proceedWithClearGuideFocus

      when(delay(TIMEOUT))
      {

         clearGuideFocusCarErrIn = STATUS_ERROR;
         pvPut(clearGuideFocusCarErrIn);

         strncpy(clearGuideFocusCarMessIn, "WaitForCoincidence:No response to clear guide focus command", MAX_STRING_SIZE - 1);
         pvPut(clearGuideFocusCarMessIn);

         clearGuideFocusCarIn = CAR_ERROR;
         pvPut(clearGuideFocusCarIn);

      }  state waitForClearGuideFocusCmd
   }

}




