/* $Id: utilities.h,v 1.7 2015/04/30 23:57:24 mrippa Exp $ */
/*+
 *
 * FILENAME
 * -------- 
 * utilities.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for utilities.c
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
 * 17-Nov-1999: Created new header files.
 * 16-Dec-1999: Added global variables
 * 04-Jan-2000: Moved scsReady to setup.h since only used by setup.c
 *              Moved flagOutReg to chopControl.h since not part of 
 *              utilities.c
 * 19-Oct-2017: Begin conversion to EPICS OSI (mdw)
 *
 */
/* ===================================================================== */

#ifndef _INCLUDED_UTILITIES_H
#define _INCLUDED_UTILITIES_H

#ifndef _INCLUDED_SUBRECORD_H
#define _INCLUDED_SUBRECORD_H
#include <subRecord.h>
#endif

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif

#include <epicsMutex.h>
#include <epicsEvent.h>
#include <errlog.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>


#if defined (__rtems__)
#include <os/RTEMS/devIocStatsOSD.h>   /* for reboot() */
#include <bsp/bootcard.h>              /* for bsp_reset() */
#endif

/* Define true/false, on/off etc */

#define OK              0
#define ERROR         (-1)

#define ENABLED         0
#define DISABLED        1

#define VALID           0
#define INVALID         1

#define TRANS           2
#define ON              1
#define OFF             0

#define PI              ((double)3.14159265)

#define MSG_Q_EMPTY   (-1)


/* Define conversion factors */

#define RADS2DEGS       ((double)360.0/((double)(2.0*PI))) /* radians to degrees */
#define DEGS2RADS       ((double)(2.0*PI)/(double)360.0)   /* degrees to radians */
#define DEGS2ASECS      ((double)3600.0)                  /* degrees to arcseconds */
#define METRES2MICRONS  1e6

#define ACTUATOR_RADIUS 0.318  /* radius of circle of actuators (metres) */

/* Define constants for interface with the TCS - ref ICD7b */

#define BOOTING         0
#define INIT            1   /* not 'initialising' as this would conflict with
                               SCS internal state name */
#define RUNNING         2

#ifdef MK
#define MAX_SOURCES     5       /* number of guide sources - 
                   pwfs1, pwfs2, oiwfs, gaos, gyro    */
#else
#define MAX_SOURCES     6       /* number of guide sources - 
                   pwfs1, pwfs2, oiwfs, gaos, gpi, gyro    */
#endif

/* health related items */

typedef struct
{
  int severity;
  char    message[MAX_STRING_SIZE];
} healthReport;

#define GOOD            0               /* scs health settings */
#define WARNING         1
#define BAD             2

#define NORMAL          0               /* request change in health status
                                           of scs       */
#define OVERRIDE        1               /* force change in health status of
                                           scs */

/* Define sanity limits for absolute demand values */

#define X_TILT_LIMIT            3600.0
#define Y_TILT_LIMIT            3600.0
#define Z_FOCUS_LIMIT           10000.0
#define X_POSITION_LIMIT        2500.0
#define Y_POSITION_LIMIT        6000.0
#define Y_POSITION_LIMIT_MAX    6000.0
#define Y_POSITION_LIMIT_MIN    -5000.0

/* Define state indicators for SCS system */

enum
{
        SYSTEMIDLE = 0,
        INITIALISING,
        REBOOTING,
        TESTING,
        MOVING,
        CHOPPING,   /*<--- not used*/
        PARKED,
        FOLLOWING,      /*<--- not used*/
        INTERLOCKED  
};

/* for cadProcessorState */
enum
{
        CADPROCENTRY = 0,
        CADPROCIDLE,
        STARTTESTHANDSHAKE,
        READAGAIN,
        STARTINIT,
        LOADFILES,
        WAITINITRESPONSE,
        WAITINITCOMPLETE,
        INITCOMPLETETIMEOUT,
        STARTMOVE,
        PROCEEDWITHMOVE,
        SERVOOFFTIMEOUT,
        MOVEWAITFORCOINCIDENCE,
        MOVETIMEOUTFORCOINCIDENCE,
        STARTACTUATOR,
        PROCEEDWITHACTMOVE,
        ACTUATORSERVOOFFTIMEOUT,
        ACTUATORWAITFORCOINCIDENCE,
        ACTUATORTIMEOUTFORCOINCIDENCE,
        STARTFOLLOW,
        PROCEEDWITHFOLLOW,
        FOLLOWSERVOTIMEOUT,
        FOLLOWWAITFORCOINCIDENCE,
        FOLLOWTIMEOUT,
        STARTSTOP, 
        STARTPARK,
        WAITFORPARK, 
        TIMEOUTWAITFORPARK, 
        STARTCHOPCONFIG, 
        STARTCHOPCONTROL, 
        WAITFORCHOPRESPONSE, 
        TIMEOUTWAITFORCHOPRESPONSE, 
        STARTBANDWITH, 
        STARTTOLERANCE, 
        STARTDRIVEFOLLOWER,
	PARKWAITFORCOINCIDENCE
};

/* for followDemandState */
enum
{
        FOLDEMANDENTRY = 0,
        WAITFORDEMAND,
        WAITFORNEXTCOINCIDENCE,
        TIMEOUTWAITFORNEXTCOINCIDENCE
};

/* for monitorSadState */
enum
{
        MONSADENTRY = 0, 
        UPDATESAD = 1
};

/* for monitorProcessState */
enum
{
        UPDATEPARAMETERS = 0
};

/* for rebootScsState */
enum
{
        INITREBOOT = 0,
        WAITFORREBOOTCMD,
        STARTREBOOT,
        WAITFORPARKREBOOT,
        TIMEOUTWAITFORPARKREBOOT,
        REBOOTNOW  
};

/* for moveBaffleState */
enum
{
        INITMOVEBAFFLE = 0,
        WAITFORMOVEBAFFLECMD,
        STARTMOVEBAFFLE,
        WAITFORMOVEBAFFLE,
        TIMEOUTWAITFORMOVEBAFFLE
};

/* Define debug levels */

enum
{
        DEBUG_NONE = 0,
        DEBUG_MIN,
        DEBUG_MED,
        DEBUG_MAX,
        DEBUG_RESERVED1,
        DEBUG_RESERVED2
};

/* Structure to hold the position of the secondary mirror       */
/* in all of the required coordinate systems                    */

typedef struct
{
        double  xTilt;     /* tilt representation          */
        double  yTilt;
        double  zFocus;
        double  actuator1; /* actuator representation      */
        double  actuator2;
        double  actuator3;
        double  xPos;      /* translation position         */
        double  yPos;
        double  theta;     /* moveCurve representation     */
        double  phi;
        double  Xc;
        double  Yc;
        double  Zc;
        double  xTiltNew;  /* position represented in new frame of reference */
        double  yTiltNew;
        double  zFocusNew;
        double  xPosNew;
        double  yPosNew;

} location;

/* Structure of values defining coordinate conversions between tcs and m2 */
/* also gaos/m2 and gyro/m2 */

typedef struct
{
        double  tiltCosTheta;
        double  tiltSinTheta;
        double  tiltOffsetX;
        double  tiltOffsetY;

        double  posCosTheta;
        double  posSinTheta;
        double  posOffsetX;
        double  posOffsetY;

        double  gaosCosTheta;
        double  gaosSinTheta;
        double  gaosOffsetX;
        double  gaosOffsetY;

        double  gyroCosTheta;
        double  gyroSinTheta;
        double  gyroOffsetX;
        double  gyroOffsetY;

        double  focusScaling;

} frameOfReference;

typedef struct
{
        double  theta;
        double  sinTheta;
        double  cosTheta;
        double  offsetX;
        double  offsetY;
        double  scaleX;
        double  scaleY;
        double  scaleZ;
        // SEM_ID  access;
        epicsMutexId  access;

}frameChange;

/* Public functions */

int act2tilt(location *position);

int tilt2act(location *position);

double control (int axis, double input, double systemOutput);

int tcs2m2(location *position);

int m22tcs(location *position);

int gaos2m2(location *position);

int gyro2m2(location *position);

long checkSum(void *ptr, int numLongs);

int date2secs(char * dateString);  /* used only in archive.c */

char* weight2string(double weightA, double weightB, double weightC);

int errorLog (char *errorString, int debugLevelRqst, int fileLog);

int reportHealth(int severity, char *message);

long readHealthInit(struct genSubRecord *pgsub);

long readHealth(struct genSubRecord *pgsub);

int loadInitFiles(void *);

double confine(double value, double upper, double lower);

int setPid(int axis, double P, double I, double D, 
           double windUpLimit, double rateLimit);

int  modifyFrame
        (
        frameChange *f,
        const double theta,
        const double scaleX,
        const double scaleY,
        const double scaleZ,
        const double offsetX,
        const double offsetY
        );

long stateInit (struct subRecord * psub);

long scsStateStringInit (struct genSubRecord * pgsub);

long driveEvent (struct subRecord * psub);

/* Global variables */

extern int debugLevel;
extern long inPosition;
extern frameChange *ag2m2[MAX_SOURCES];

/* not used anywhere. 20171019 MDW */
//extern SEM_ID compileStatus;
//extern SEM_ID statusCompiled;


extern epicsEventId doPvLoad;
extern epicsEventId pvLoadComplete;

extern frameOfReference frame;

#ifdef MK
typedef struct {
    double Snew[2][1];
    double Sold[2][1];
    double command;
    double freq;
    double amp;
    double Fs;
    double dt;
    double Theta;
    double Rotator[2][2];

} Phasor;

typedef struct {
    double Snew[2][1];
    double Sold[2][1];
}VtkStateVector;

typedef struct {
    double phase;
    double frequency;

} VtkGain;

typedef struct {
    double initialValue;
    double tolerance;
    double error;
    double currentValue;

} VtkFrequency;

typedef struct {

    VtkStateVector oscillator;
    double Fs;
    double dt;
    VtkGain gain;
    VtkFrequency frequency;
    double maxAmplitude;  /*Max amplitude Vtk attempts to correct for */ 
    double scale;
    double angle;
    double Rotator[2][2]; /* Initialize  Rotation Matrix to avoid extra computation */
    VtkStateVector localOscillator;
    double integral[2][1];
    double phase;
    double phaseOld;
    double deltaPhase;
    double command;
    long counter;
} Vtk;


void zeroMat(double mat[][1]);
void MatMult( double matA[][2], double matB[][1], double matC[][1]);
void ScalerMult(double scaler, double matA[][1]);

void phasorInit (Phasor *p);
void vtkInit (Vtk *vtk);
void showPhasorRotation(Phasor *p);
void showVtkRotation(Vtk *vtk);
int vtkControl(Vtk *vtk, double guideError);
int phasorSetFrequency(Phasor *p, double newFrequency);
int phasorSetAmplitude(Phasor *p, double newAmplitude);
int phasorSetSampleRate(Phasor *p, double newSampleRate);
void _phasorShow (Phasor *p);

void _vtkShow(Vtk *vtk);
void phasorShowX();
void phasorShowY();
void vtkShowX();
void vtkShowY();

void vtkxon();
void vtkxoff();
void vtkyon();
void vtkyoff();


void swxon();
void swxoff(); 
void swyon();
void swyoff(); 

void _vtkReset(Vtk *vtk);
void vtkResetX();
void vtkResetY();

Vtk* getVtkX(void);
double myround( double x, int precision);
double myround1( double x);
int myround_nearest10(int n); 

void setVtkX(Vtk *vtkx);
void setVtkY(Vtk *vtky);
Vtk* getVtkY(void);

Phasor* getPhasorX(void);
Phasor* getPhasorY(void);
void setPhasorX(Phasor *phasorx);
void setPhasorY(Phasor *phasory);
int checkGuideModeChange( long mode);

#define VTK_SR_LOW 25.0
#define VTK_SR_HIGH 200.0
#define VTK_GAINPHASE_LOW 0.00
#define VTK_GAINPHASE_HIGH 0.0060
#define VTK_GAINFREQ_LOW 0.00
#define VTK_GAINFREQ_HIGH 0.0025
#define VTK_FREQUENCY_LOW 0.1
#define VTK_FREQUENCY_HIGH 50.0
#define VTK_FREQ_TOLERANCE_LOW 0.1
#define VTK_FREQ_TOLERANCE_HIGH 0.4
#define VTK_SCALE_LOW 1.0000
#define VTK_SCALE_HIGH 1000.0000
#define VTK_ANGLE_LOW -90.0
#define VTK_ANGLE_HIGH 270.0

#define PHASOR_FREQUENCY_LOWLIMIT 0.0
#define PHASOR_FREQUENCY_HIGHLIMIT 20.0
#define PHASOR_AMPLITUDE_LOWLIMIT 0.0
#define PHASOR_AMPLITUDE_HIGHLIMIT 1.0
#define PHASOR_SR_LOWLIMIT 25.0
#define PHASOR_SR_HIGHLIMIT 200.0
#endif


#endif

