/* $Id: testFunctions.c,v 1.10 2015/04/30 23:59:36 mrippa Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * testFunctions.c
 * 
 * PURPOSE
 * -------
 * Suite of functions for testing and diagnostics during development of
 * the SCS software
 * 
 * FUNCTION NAME(S)
 * ----------------
 * tiltState    - Display current contents of tilt system ref memory buffer
 * big          - Display sizes of types for this platform
 * showCounts   - Display sequence counts for ref mem (NS and NR)
 * showTime     - Display current time formatted to string
 * rawTime      - Display current time as seconds
 * source1      - Generate dummy guide data
 * testMem      - Display contents of specified ref mem buffer
 * testm22tcs   - Test the conversion routine "m22tcs"
 * testtcs2m2   - Test the conversion routine "tcs2m2"
 * printPage[0-2,7-13] - Display contents of a particular page of RM buffer
 * driveP1      - Simulate P1 guiding
 * driveP2      - Simulate P2 guiding
 * checkSafeBlock
 * checkFiltered
 * fillWfs
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
 * 21-Oct-1997: Modify testMem to use ? operator instead of if .. else
 * 23-Oct-1997: Move tcs test drive functions to file tcs.c
 *      startLoops to control.c
 * 24-Feb-1998: Move functions carInit and carDrive to house.c, percentCalc to chop.c
 * 07-May-1999: Added RCS id
 * 07-Dec-1999: Added testm22tcs, testtcs2m2 routines
 * 15-Dec-1999: Added printPage[0-2,7-13] routines
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include "archive.h"        /* For refMemFree */
#include "control.h"        /* For scsPtr, scsBase, m2Ptr, 
                               diagnosticsAvailable, controller,
                               currentBeam, raw, filtered */
#include "guide.h"          /* For guideMaster */
#include "testFunctions.h"
#include "utilities.h"      /* For tcs2m2, errorLog, etc. */

#include <vmi5588.h> 
#include <timeLib.h>

#include <stdio.h>
#include <string.h>
#include <math.h>           /* for trig functions */
#include <stdlib.h>         /* Used for rand */
#include <tcslib.h>         /* for tcsDcString */



/* define externals */

#ifdef LATER
extern excIntStub;              /* Default handler. */
#endif

/* define globals */
/* The following implement Daytime Pseudo-Guiding*/

extern void rmISR3(int);
float gInterval = 0.0001; /* Increment the interval for simulated P2 guiding*/
double max_z = 1.2;       /* 1.0 + the upper bound of the uniform random variable */
int stopGuideSim = 0;     /* Stop the guide simulation task*/
int freeRunGuideSim = 0;  /* When true, this calls rmISR3(3) to pretend to be P2*/
double xTiltGuideSimScale = 0.0;
double yTiltGuideSimScale = 0.0;

epicsThreadId guideSimTaskId = 0;  /* The taskId of the guideSimulation task */

/* RANDnorm returns a bounded random variable between (-1.0 and 1.0-max_z) */
double RANDnorm () {
   double result;
   result = (max_z * (double) rand()/(double)RAND_MAX - 1.0);
   return result;
}

#ifdef MK
double vibfreq = 12.0;
double vibamp = 1.0;
double vibphase = 0.0;
double guideSrate = 100.0;
//int guideSimDelayTicks = 1; /* Delay (ms) = guideSimDelayTicks / (sysClkRateGet()==usually 200) */
double guideSimDelay = 0.001; /* Delay (ms) */

void zeroMat(double mat[][1]) {
    int r,c;
    /*printf("clearing result matrix\n");*/
    for (r=0; r<2; r++) {
        for(c=0; c<1; c++) {
            mat[r][c] = 0.0;
            /*printf("mat[%d][%d]=%f ", r,c,mat[r][c]);*/
        }
        /*printf("\n");*/
    }
    /*printf("----\n");*/
}

void MatMult( double matA[][2], double matB[][1], double matC[][1]) {
    int i,j,k;
    zeroMat(matC);
    for (i=0;i<2;i++){
        for(j=0;j<1;j++){
            for(k=0;k<2;k++){
                matC[i][j] += matA[i][k] * matB[k][j];
                /*printf("resy1(%d,%d,%d)=%f ==> v1=%f * v2=%f\n",i,j,k,matC[i][j], matA[i][k], matB[k][j]);*/
            }
            /*printf("-----\nres[%d][%d]=%f\n\n",i,j,matC[i][j]);*/
        } 
    }
}

void ScalerMult(double scaler, double matA[][1]) {
    
    int i,j;
    for(i=0; i<2; i++) {
        for(j=0; j<1; j++) {
           matA[i][j] = scaler * matA[i][j]; 
        }
    }

}

void vtkInit(Vtk *vtk) {

    double vtkAngle;

    vtk->dt = 1.0/vtk->Fs;
    vtk->frequency.currentValue = vtk->frequency.initialValue;
    vtkAngle = vtk->angle * PI/180.0;
    
    vtk->Rotator[0][0] = cos(vtkAngle);
    vtk->Rotator[0][1] = -sin(vtkAngle);
    vtk->Rotator[1][0] = sin(vtkAngle);
    vtk->Rotator[1][1] = cos(vtkAngle);

}


void phasorInit (Phasor *p) {

    p->dt = 1.0/p->Fs;

    p->Theta = 2*PI* p->freq * p->dt;
    p->Rotator[0][0] = cos(p->Theta);
    p->Rotator[0][1] = -sin(p->Theta);
    p->Rotator[1][0] = sin(p->Theta);
    p->Rotator[1][1] = cos(p->Theta);

}

void _phasorShow (Phasor *p) {

    printf("phasor snapshot\n----------------\n");
    printf("snew = {%f,%f}\n",p->Snew[0][0], p->Snew[0][1]);
    printf("sold = {%f,%f}\n",p->Sold[0][0], p->Sold[0][1]);
    printf("command = %f\n", p->command);
    printf("frequency = %f\n", p->freq);
    printf("amplitude = %f\n", p->amp);
    printf("sample rate = %f\n", p->Fs);
    printf("sample time = %f\n", p->dt);
    printf("theta = %f\n", p->Theta);
    showPhasorRotation(p);

}

void showPhasorRotation(Phasor *p) {

    int i,j;
    for (i=0; i<2; i++) {
        for (j=0; j<2; j++) {
            printf("rot[%d][%d]=%f\n",i,j,p->Rotator[i][j]);
        }
        printf("\n");
    }
}


void _vtkReset (Vtk *vtk) {

    vtk->integral[0][0] = 0.0;
    vtk->integral[1][0] = 0.0;
    vtk->frequency.currentValue = vtk->frequency.initialValue;
    /*
     * _vtkShow(vtk);
     */
}

void _vtkShow (Vtk *vtk) {

    printf("Vtk snapshot\n----------------\n");
    printf("Oscillator.snew = {{%f},{%f}}\n", vtk->oscillator.Snew[0][0], vtk->oscillator.Snew[1][0]);
    printf("Oscillator.sold = {{%f},{%f}}\n", vtk->oscillator.Sold[0][0], vtk->oscillator.Sold[1][0]);
    printf("Sample rate = %f\n", vtk->Fs);
    printf("Sample time = %f\n", vtk->dt);
    printf("{gain.phase, gain.frequency} = {%f,%f}\n",vtk->gain.phase, vtk->gain.frequency);
    printf("Frequency:{initial, tolerance, error, currentValue} = {%f, %f, %f, %f}\n",
            vtk->frequency.initialValue, vtk->frequency.tolerance, vtk->frequency.error, vtk->frequency.currentValue);
    printf("Max Amplitude = %f\n", vtk->maxAmplitude);
    printf("Scale = %f\n", vtk->scale);
    printf("Angle = %f\n", vtk->angle);
    printf("LocalOscillator.snew = {{%f},{%f}}\n", vtk->localOscillator.Snew[0][0], vtk->localOscillator.Snew[1][0]);
    printf("Integral = {{%f},{%f}}\n", vtk->integral[0][0], vtk->integral[1][0]);
    printf("Phases{new,last} = {%f,%f}\n", vtk->phase, vtk->phaseOld);
    printf("deltaPhase = %f\n", vtk->deltaPhase);
    printf("command = %f\n", vtk->command);
    printf("Counter = %ld\n", vtk->counter);
    showVtkRotation(vtk);

}

void showVtkRotation(Vtk *vtk) {

    int i,j;
    for (i=0; i<2; i++) {
        for (j=0; j<2; j++) {
            printf("rot[%d][%d]=%f\n",i,j,vtk->Rotator[i][j]);
        }
        printf("\n");
    }
}


double phasorNewAmp = 0.0;
int phasorAmpRequestChanged = 0;

void phasorSetNewAmp(double amp) {
    phasorNewAmp = amp;
    phasorAmpRequestChanged = 1;
}

double phasorNewFreq = 0.0;
int phasorFreqRequestChanged = 0;

void phasorSetNewFreq(double freq) {
    phasorNewFreq = freq;
    phasorFreqRequestChanged = 1;
}

double phasorNewSR = 200;           /* <--Default SR */
int phasorSRRequestChanged = 0;   /**/

void phasorSetNewSampleRate(double samplerate) {
    phasorNewSR = samplerate;
    phasorSRRequestChanged = 1;
}

/*
 * phasorSetSampleRate()
 * in1 -> Phasor pointer;
 * in2 -> SampleRate in Hz.
 *
 * */
int phasorSetSampleRate(Phasor *p, double newSampleRate) {

    if ( (newSampleRate < PHASOR_SR_LOWLIMIT) ||
            (newSampleRate > PHASOR_SR_HIGHLIMIT) )

    {
        printf("Vibration Phasor SampleRate out of range!\n");
        return -1;
    }
    else {
        p->Fs = newSampleRate; 
        printf("Phasor SampleRate changed to %f\n", p->Fs);
    }
    
    /* Reinitialize the Rotator matrix with the new frequency information.*/
    phasorInit(p);
    phasorSRRequestChanged = 0;
    return 0;
}

int phasorSetFrequency(Phasor *p, double newFrequency) {

    if ( (newFrequency < PHASOR_FREQUENCY_LOWLIMIT) ||
            (newFrequency > PHASOR_FREQUENCY_HIGHLIMIT) )

    {
        printf("Vibration Phasor frequency out of range!\n");
        return -1;
    }
    else {
        p->freq = newFrequency; 
        printf("Phasor Frequency changed to %f\n", p->freq);
    }
    
    /* Reinitialize the Rotator matrix with the new frequency information.*/
    phasorInit(p);
    phasorFreqRequestChanged = 0;
    return 0;
}

int phasorSetAmplitude(Phasor *p, double newAmplitude) {

    if ( (newAmplitude < PHASOR_AMPLITUDE_LOWLIMIT) ||
            (newAmplitude > PHASOR_AMPLITUDE_HIGHLIMIT) )

    {
        printf("Vibration Phasor amplitude out of range!\n");
        return -1;
    }
    else {
        p->amp = newAmplitude; 
        printf("Phasor Amplitude changed to %f\n", p->amp);
    }
    phasorAmpRequestChanged = 0;
    return 0;
}

/*This uses a table and would be subject to phase 'jumping' when the 
 * table repeats.
 * */
double sinus(int n) {
    double t = (double)n/guideSrate; 
    return vibamp * cos(2*PI * vibfreq * t  ) ;
}
#endif


/*bitFieldM2 statusWord;*/
/*
static double   probe1X = 10.1;
static double   probe1Y = 10.2;
static double   probe1Z = 10.3;

static double   probe2X = 20.1;
static double   probe2Y = 20.2;
static double   probe2Z = 20.3;
*/

/* function prototypes */


/* ===================================================================== */
#ifdef LATER
void vectorShow (void)
{
    int ix;
    int vecBase = (int)intVecBaseGet ();
    VOIDFUNCPTR vec;

    for (ix = (vecBase + 64); ix < (vecBase + 256); ix++) {
	vec = (VOIDFUNCPTR)intVecGet ((FUNCPTR *) INUM_TO_IVEC (ix));
        if (vec != excIntStub) {
            printf ("User defined interrupt vector %d has been used.\n", ix);
	}
    }
}
#endif

/* ===================================================================== */
void checkSafeBlock(int count)
{
  long result = 0;

  printf("calculate checksum for safe block\n");
  result = checkSum((void *)&safeBlock.NR, count);
  printf(" for %d longs checksum = %ld = %lx\n", count, result, result);
}

void    tiltState (const memMap *buffPtr)
{
    printf ("__________________________________________________________\n");
    printf ("Item\t\tStatus\n");
    printf ("__________________________________________________________\n");

    /* options are arranged as: argument ? if not zero: if zero */

    printf ("offloaders             %s\n", buffPtr->page1.statusWord.flags.offloaders ? "ON" : "OFF");
    printf ("mirror control         %s\n", buffPtr->page1.statusWord.flags.mirrorControl ? "ON" : "OFF");
    printf ("mirror moving          %s\n", buffPtr->page1.statusWord.flags.mirrorMoving ? "MOVING" : "STATIC");
    printf ("mirror commanded       %s\n", buffPtr->page1.statusWord.flags.mirrorCommanded ? "COMMANDED" : "FREE");
    printf ("mirror responding      %s\n", buffPtr->page1.statusWord.flags.mirrorResponding ? "RESPONDING" : "NOT RESPONDING");
    printf ("sensor limit           %s\n", buffPtr->page1.statusWord.flags.sensorLimit ? "OUT OF LIMITS" : "OK");
    printf ("actuator limit         %s\n", buffPtr->page1.statusWord.flags.actuatorLimit ? "OUT OF LIMITS" : "OK");
    printf ("thermal limit          %s\n", buffPtr->page1.statusWord.flags.thermalLimit ? "OUT OF LIMITS" : "OK");
    printf ("invalid dsp interrupt  %s\n", buffPtr->page1.statusWord.flags.mirrorDspInt ? "INVALID" : "OK");
    printf ("invalid vib interrupt  %s\n", buffPtr->page1.statusWord.flags.vibDspInt ? "INVALID" : "OK");
    printf ("demand space           %s\n", buffPtr->page1.statusWord.flags.space ? "ACTUATOR" : "TILT");
    printf ("decs frozen            %s\n", buffPtr->page1.statusWord.flags.decsFrozen ? "FROZEN" : "NOT FROZEN");
    printf ("decs paused            %s\n", buffPtr->page1.statusWord.flags.decsPaused ? "PAUSED" : "NOT PAUSED");
    printf ("decs on                %s\n", buffPtr->page1.statusWord.flags.decsOn ? "ON" : "OFF");
    printf ("chop on                %s\n", buffPtr->page1.statusWord.flags.chopOn ? "ON" : "OFF");
    printf ("vibration control      %s\n", buffPtr->page1.statusWord.flags.vibControlOn ? "ON" : "OFF");
    printf ("test  in progress      %s\n", buffPtr->page1.statusWord.flags.testInProgress ? "YES" : "NO");
    printf ("init  in progress      %s\n", buffPtr->page1.statusWord.flags.initInProgress ? "YES" : "NO");
    printf ("reset in progress      %s\n", buffPtr->page1.statusWord.flags.resetInProgress ? "YES" : "NO");
    printf ("power enabled          %s\n", buffPtr->page1.statusWord.flags.powerEnabled ? "ENABLED" : "DISABLED");
    printf ("health                 %s\n", buffPtr->page1.statusWord.flags.health ? "BAD" : "GOOD");
    printf ("diagnostics available  %s\n", buffPtr->page1.statusWord.flags.diagnosticsAvailable ? "READY" : "NONE");

    printf ("\nPage 1 - M2 to SCS responses\n");

    printf ("checksum   Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.checksum, buffPtr->page1.checksum);
    printf ("NR     Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.NR, buffPtr->page1.NR);
    printf ("xtilt      Addr = %lx, Value = %f\n", (long) &buffPtr->page1.xTilt, buffPtr->page1.xTilt);
    printf ("ytilt      Addr = %lx, Value = %f\n", (long) &buffPtr->page1.yTilt, buffPtr->page1.yTilt);
    printf ("zfocus     Addr = %lx, Value = %f\n", (long) &buffPtr->page1.zFocus, buffPtr->page1.zFocus);
    printf ("actuator1  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.actuator1, buffPtr->page1.actuator1);
    printf ("actuator2  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.actuator2, buffPtr->page1.actuator2);
    printf ("actuator3  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.actuator3, buffPtr->page1.actuator3);
    printf ("inPosition Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.inPosition, buffPtr->page1.inPosition);
    printf ("chopTrans  Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.chopTransition, buffPtr->page1.chopTransition);
    printf ("statusword Addr = %lx, Value = %x\n", (long) &buffPtr->page1.statusWord.all, buffPtr->page1.statusWord.all);
    printf ("heartbeat  Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.heartbeat, buffPtr->page1.heartbeat);
    printf ("beamPosition   Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.beamPosition, buffPtr->page1.beamPosition);
    printf ("xposition  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.xPosition, buffPtr->page1.xPosition);
    printf ("yposition  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.yPosition, buffPtr->page1.yPosition);
    printf ("deployable Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.deployBaffle, buffPtr->page1.deployBaffle);
    printf ("central    Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.centralBaffle, buffPtr->page1.centralBaffle);
    printf ("encoderA   Addr = %lx, Value = %f\n", (long) &buffPtr->page1.baffleEncoderA, buffPtr->page1.baffleEncoderA);
    printf ("encoderB   Addr = %lx, Value = %f\n", (long) &buffPtr->page1.baffleEncoderB, buffPtr->page1.baffleEncoderB);
    printf ("encoderC   Addr = %lx, Value = %f\n", (long) &buffPtr->page1.baffleEncoderC, buffPtr->page1.baffleEncoderC);
    printf ("topEnd     Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.topEnd, buffPtr->page1.topEnd);
    printf ("temperature    Addr = %lx, Value = %f\n", (long) &buffPtr->page1.enclosureTemp, buffPtr->page1.enclosureTemp);


}

/* ===================================================================== */

void    big (void)
{
    printf ("sizes of types on this machine (bytes)\n\n");
    printf ("char          %d\n", sizeof (char));
    printf ("int           %d\n", sizeof (int));
    printf ("long          %d\n", sizeof (long));
    printf ("float         %d\n", sizeof (float));
    printf ("double        %d\n", sizeof (double));
    printf ("unsigned int  %d\n", sizeof (unsigned int));
    printf ("short int     %d\n", sizeof (short int));

    printf ("m2 status word at %lx\n", (long) &scsPtr->page1.statusWord.all);
}

/* ===================================================================== */

void    showCounts (void)
{
    printf ("BASE NS = %ld, NR = %ld\n", scsBase->page0.NS, scsBase->page1.NR);
    printf ("SCS NS = %ld, NR = %ld\n", scsPtr->page0.NS, scsPtr->page1.NR);
    printf ("M2  NS = %ld, NR = %ld\n", m2Ptr->page0.NS, m2Ptr->page1.NR);
}

/* ===================================================================== */

void    showTime (void)
{
    int     j, c[7];

    j = timeNowC (TAI, 3, c);
    printf ("status = %d time > %d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d (TAI)\n", j, c[0], c[1], c[2], c[3], c[4], c[5], c[6]);
}

/* ===================================================================== */

void    rawTime (void)
{
    int     j;
    double  rawt = 99;

    j = timeNow (&rawt);

    printf ("status = %d rawtime > %f\n", j, rawt);
}

/* ===================================================================== */

void    testMem (const memMap * buffPtr)
{
    /* printout the memory locations of the specified buffer area */

    printPage0 (buffPtr);
    printPage1 (buffPtr);
    printPage2 (buffPtr);
    printPage7 (buffPtr);
    printPage8 (buffPtr);
    printPage9 (buffPtr);
    printPage10 (buffPtr);
    printPage11 (buffPtr);
    printPage12 (buffPtr);
    printPage13a (buffPtr);
    printPage13b (buffPtr);

#ifndef MK
    printPage15  (buffPtr);
#endif

}

/*i ===================================================================== */

void    printPage0 (const memMap * buffPtr)
{
    printf ("\nPage 0 - SCS to M2 commands\n");

    printf ("checksum       Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.checksum, buffPtr->page0.checksum);
    printf ("NS             Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.NS, buffPtr->page0.NS);
    printf ("command        Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.commandCode, buffPtr->page0.commandCode);
    printf ("xtiltguide     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xTiltGuide, buffPtr->page0.xTiltGuide);
    printf ("ytiltguide     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yTiltGuide, buffPtr->page0.yTiltGuide);
    printf ("zfocusguide    Addr = %lx, Value = %f\n", (long) &buffPtr->page0.zFocusGuide, buffPtr->page0.zFocusGuide);
    printf ("axtilt         Addr = %lx, Value = %f\n", (long) &buffPtr->page0.AxTilt, buffPtr->page0.AxTilt);
    printf ("aytilt         Addr = %lx, Value = %f\n", (long) &buffPtr->page0.AyTilt, buffPtr->page0.AyTilt);
    printf ("bxtilt         Addr = %lx, Value = %f\n", (long) &buffPtr->page0.BxTilt, buffPtr->page0.BxTilt);
    printf ("bytilt         Addr = %lx, Value = %f\n", (long) &buffPtr->page0.ByTilt, buffPtr->page0.ByTilt);
    printf ("cxtilt         Addr = %lx, Value = %f\n", (long) &buffPtr->page0.CxTilt, buffPtr->page0.CxTilt);
    printf ("cytilt         Addr = %lx, Value = %f\n", (long) &buffPtr->page0.CyTilt, buffPtr->page0.CyTilt);
    printf ("actuator1      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.actuator1, buffPtr->page0.actuator1);
    printf ("actuator2      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.actuator2, buffPtr->page0.actuator2);
    printf ("actuator3      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.actuator3, buffPtr->page0.actuator3);
    printf ("heartbeat      Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.heartbeat, buffPtr->page0.heartbeat);
    printf ("xDemand        Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xDemand, buffPtr->page0.xDemand);
    printf ("yDemand        Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yDemand, buffPtr->page0.yDemand);
    printf ("central        Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.centralBaffle, buffPtr->page0.centralBaffle);
    printf ("deployable     Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.deployBaffle, buffPtr->page0.deployBaffle);
    printf ("profile        Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.chopProfile, buffPtr->page0.chopProfile);
    printf ("frequency      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.chopFrequency, buffPtr->page0.chopFrequency);
    printf ("dutycycle      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.chopDutyCycle, buffPtr->page0.chopDutyCycle);
    printf ("xtilttol       Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xTiltTolerance, buffPtr->page0.xTiltTolerance);
    printf ("ytilttol       Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yTiltTolerance, buffPtr->page0.yTiltTolerance);
    printf ("zfocustol      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.zFocusTolerance, buffPtr->page0.zFocusTolerance);
    printf ("xpostol        Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xPositionTolerance, buffPtr->page0.xPositionTolerance);
    printf ("ypostol        Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yPositionTolerance, buffPtr->page0.yPositionTolerance);
    printf ("bandwidth      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.bandwidth, buffPtr->page0.bandwidth);
    printf ("xtiltgain      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xTiltGain, buffPtr->page0.xTiltGain);
    printf ("ytiltgain      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yTiltGain, buffPtr->page0.yTiltGain);
    printf ("zfocusgain     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.zFocusGain, buffPtr->page0.zFocusGain);
    printf ("xtiltshift     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xTiltShift, buffPtr->page0.xTiltShift);
    printf ("ytiltshift     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yTiltShift, buffPtr->page0.yTiltShift);
    printf ("zfocusshift    Addr = %lx, Value = %f\n", (long) &buffPtr->page0.zFocusShift, buffPtr->page0.zFocusShift);
    printf ("xtiltsmooth    Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xTiltSmooth, buffPtr->page0.xTiltSmooth);
    printf ("ytiltsmooth    Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yTiltSmooth, buffPtr->page0.yTiltSmooth);
    printf ("zfocussmooth   Addr = %lx, Value = %f\n", (long) &buffPtr->page0.zFocusSmooth, buffPtr->page0.zFocusSmooth);
    printf ("xtcsminrange   Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xTcsMinRange, buffPtr->page0.xTcsMinRange);
    printf ("ytcsminrange   Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yTcsMinRange, buffPtr->page0.yTcsMinRange);
    printf ("xtcsmaxrange   Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xTcsMaxRange, buffPtr->page0.xTcsMaxRange);
    printf ("ytcsmaxrange   Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yTcsMaxRange, buffPtr->page0.yTcsMaxRange);
    printf ("xpminrange     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xPMinRange, buffPtr->page0.xPMinRange);
    printf ("ypminrange     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yPMinRange, buffPtr->page0.yPMinRange);
    printf ("xpmaxrange     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xPMaxRange, buffPtr->page0.xPMaxRange);
    printf ("ypmaxrange     Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yPMaxRange, buffPtr->page0.yPMaxRange);
    printf ("follower       Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.follower, buffPtr->page0.follower);
    printf ("foldir         Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.foldir, buffPtr->page0.foldir);
    printf ("followersteps  Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.followersteps, buffPtr->page0.followersteps);
    printf ("offloader      Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.offloader, buffPtr->page0.offloader);
    printf ("ofldir         Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.ofldir, buffPtr->page0.ofldir);
    printf ("offloadersteps Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.offloadersteps, buffPtr->page0.offloadersteps);
    printf ("cbafdir        Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.cbafdir, buffPtr->page0.cbafdir);
    printf ("cbsteps        Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.cbsteps, buffPtr->page0.cbsteps);
    printf ("deployable_baffle      Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.deployable_baffle, buffPtr->page0.deployable_baffle);
    printf ("dbafdir        Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.dbafdir, buffPtr->page0.dbafdir);
    printf ("dbsteps        Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.dbsteps, buffPtr->page0.dbsteps);
    printf ("xy_motor       Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.xy_motor, buffPtr->page0.xy_motor);
    printf ("xydir          Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.xydir, buffPtr->page0.xydir);
    printf ("xysteps        Addr = %lx, Value = %ld\n", (long) &buffPtr->page0.xysteps, buffPtr->page0.xysteps);
    printf ("zfocus         Addr = %lx, Value = %f\n", (long) &buffPtr->page0.zFocus, buffPtr->page0.zFocus);
    printf ("zguide         Addr = %lx, Value = %f\n", (long) &buffPtr->page0.zGuide, buffPtr->page0.zGuide);
    printf ("rawxguide      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.rawXGuide, buffPtr->page0.rawXGuide);
    printf ("rawyguide      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.rawYGuide, buffPtr->page0.rawYGuide);
    printf ("rawzguide      Addr = %lx, Value = %f\n", (long) &buffPtr->page0.rawZGuide, buffPtr->page0.rawZGuide);
    printf ("xGrossTiltDmd  Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xGrossTiltDmd, buffPtr->page0.xGrossTiltDmd);
    printf ("yGrossTiltDmd  Addr = %lx, Value = %f\n", (long) &buffPtr->page0.yGrossTiltDmd, buffPtr->page0.yGrossTiltDmd);
    printf ("xyPositionDeadband Addr = %lx, Value = %f\n", (long) &buffPtr->page0.xyPositionDeadband, buffPtr->page0.xyPositionDeadband);

}

/* ===================================================================== */

void    printPage1 (const memMap * buffPtr)
{
    printf ("\nPage 1 - M2 to SCS responses\n");

    printf ("checksum   Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.checksum, buffPtr->page1.checksum);
    printf ("NR     Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.NR, buffPtr->page1.NR);
    printf ("xtilt      Addr = %lx, Value = %f\n", (long) &buffPtr->page1.xTilt, buffPtr->page1.xTilt);
    printf ("ytilt      Addr = %lx, Value = %f\n", (long) &buffPtr->page1.yTilt, buffPtr->page1.yTilt);
    printf ("zfocus     Addr = %lx, Value = %f\n", (long) &buffPtr->page1.zFocus, buffPtr->page1.zFocus);
    printf ("actuator1  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.actuator1, buffPtr->page1.actuator1);
    printf ("actuator2  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.actuator2, buffPtr->page1.actuator2);
    printf ("actuator3  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.actuator3, buffPtr->page1.actuator3);
    printf ("inPosition Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.inPosition, buffPtr->page1.inPosition);
    printf ("chopTrans  Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.chopTransition, buffPtr->page1.chopTransition);
    printf ("statusword Addr = %lx, Value = %x\n", (long) &buffPtr->page1.statusWord.all, buffPtr->page1.statusWord.all);
    printf ("heartbeat  Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.heartbeat, buffPtr->page1.heartbeat);
    printf ("beamPosition   Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.beamPosition, buffPtr->page1.beamPosition);
    printf ("xposition  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.xPosition, buffPtr->page1.xPosition);
    printf ("yposition  Addr = %lx, Value = %f\n", (long) &buffPtr->page1.yPosition, buffPtr->page1.yPosition);
    printf ("deployable Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.deployBaffle, buffPtr->page1.deployBaffle);
    printf ("central    Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.centralBaffle, buffPtr->page1.centralBaffle);
    printf ("encoderA   Addr = %lx, Value = %f\n", (long) &buffPtr->page1.baffleEncoderA, buffPtr->page1.baffleEncoderA);
    printf ("encoderB   Addr = %lx, Value = %f\n", (long) &buffPtr->page1.baffleEncoderB, buffPtr->page1.baffleEncoderB);
    printf ("encoderC   Addr = %lx, Value = %f\n", (long) &buffPtr->page1.baffleEncoderC, buffPtr->page1.baffleEncoderC);
    printf ("topEnd     Addr = %lx, Value = %ld\n", (long) &buffPtr->page1.topEnd, buffPtr->page1.topEnd);
    printf ("temperature    Addr = %lx, Value = %f\n", (long) &buffPtr->page1.enclosureTemp, buffPtr->page1.enclosureTemp);
    printf ("upper angle    Addr = %lx, Value = %f\n", (long) &buffPtr->page1.upperBearingAngle, buffPtr->page1.upperBearingAngle);
    printf ("lower angle    Addr = %lx, Value = %f\n", (long) &buffPtr->page1.lowerBearingAngle, buffPtr->page1.lowerBearingAngle);
}

/* ===================================================================== */

void    printPage2 (const memMap * buffPtr)
{
    int i;
    fault *pfaults;

    printf ("\nPage 2 - M2 Diagnostics Data\n");
    printf ("checksum       Addr = %lx, Value = %ld\n", (long) &buffPtr->testResults.checksum, buffPtr->testResults.checksum);
    printf ("number         Addr = %lx, Value = %ld\n", (long) &buffPtr->testResults.number, buffPtr->testResults.number);

    for (i=0; i<MAX_FAULTS; i++)   /* get ride of magic number 30 */
    {
        pfaults  = (fault *)&(buffPtr->testResults.faults[i]); 
        printf ("fault[%2d]      Addr = %p, Index = %u, Subsys = %u, Code = %u\n", 
                i, pfaults, (unsigned)pfaults->index, 
                (unsigned)pfaults->subsystem, (unsigned)pfaults->code);
    } 
}

/* ===================================================================== */

void    printPage7 (const memMap * buffPtr)
{
    printf ("\nPage 7 - Event System Data\n");
    printf ("currentBeam    Addr = %lx, Value = %d\n", (long) &eventData.currentBeam, eventData.currentBeam);
    printf ("inPosition Addr = %lx, Value = %d\n", (long) &eventData.inPosition, eventData.inPosition);
    /*
    printf ("xTilt      Addr = %lx, Value = %f\n", (long) &eventData.xTilt, eventData.xTilt);
    printf ("yTilt      Addr = %lx, Value = %f\n", (long) &eventData.yTilt, eventData.yTilt);
    printf ("zFocus     Addr = %lx, Value = %f\n", (long) &eventData.zFocus, eventData.zFocus);
    printf ("xPosition  Addr = %lx, Value = %f\n", (long) &eventData.xPosition, eventData.xPosition);
    printf ("yPosition  Addr = %lx, Value = %f\n", (long) &eventData.yPosition, eventData.yPosition);
    printf ("time       Addr = %lx, Value = %f\n", (long) &eventData.time, eventData.time);
    */
}

/* ===================================================================== */

void    printPage8 (const memMap * buffPtr)
{
    printf ("\nPage 8  - pwfs1 data\n");
    printf ("pwfs1      Addr = %lx,           \n", (long) &buffPtr->pwfs1.z1);

    printf ("z1     Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs1.z1, buffPtr->pwfs1.z1);
    printf ("z2     Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs1.z2, buffPtr->pwfs1.z2);
    printf ("z3     Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs1.z3, buffPtr->pwfs1.z3);
    printf ("err1       Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs1.err1, buffPtr->pwfs1.err1);
    printf ("err2       Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs1.err2, buffPtr->pwfs1.err2);
    printf ("err3       Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs1.err3, buffPtr->pwfs1.err3);
    printf ("name       Addr = %lx, Value = %s\n", (long) buffPtr->pwfs1.name, buffPtr->pwfs1.name);
    printf ("interval   Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs1.interval, buffPtr->pwfs1.interval);
    printf ("time       Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs1.time, buffPtr->pwfs1.time);
}

/* ===================================================================== */

void    printPage9 (const memMap * buffPtr)
{
    printf ("\nPage 9  - pwfs2 data\n");
    printf ("pwfs2      Addr = %lx,           \n", (long) &buffPtr->pwfs2.z1);
    printf ("z1     Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs2.z1, buffPtr->pwfs2.z1);
    printf ("z2     Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs2.z2, buffPtr->pwfs2.z2);
    printf ("z3     Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs2.z3, buffPtr->pwfs2.z3);
    printf ("err1       Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs2.err1, buffPtr->pwfs2.err1);
    printf ("err2       Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs2.err2, buffPtr->pwfs2.err2);
    printf ("err3       Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs2.err3, buffPtr->pwfs2.err3);
    printf ("name       Addr = %lx, Value = %s\n", (long) buffPtr->pwfs2.name, buffPtr->pwfs2.name);
    printf ("interval   Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs2.interval, buffPtr->pwfs2.interval);
    printf ("time       Addr = %lx, Value = %f\n", (long) &buffPtr->pwfs2.time, buffPtr->pwfs2.time);
}

/* ===================================================================== */

void    printPage10 (const memMap * buffPtr)
{
    printf ("\nPage 10 - oiwfs data\n");
    printf ("oiwfs      Addr = %lx,           \n", (long) &buffPtr->oiwfs.z1);
    printf ("z1     Addr = %lx, Value = %f\n", (long) &buffPtr->oiwfs.z1, buffPtr->oiwfs.z1);
    printf ("z2     Addr = %lx, Value = %f\n", (long) &buffPtr->oiwfs.z2, buffPtr->oiwfs.z2);
    printf ("z3     Addr = %lx, Value = %f\n", (long) &buffPtr->oiwfs.z3, buffPtr->oiwfs.z3);
    printf ("err1       Addr = %lx, Value = %f\n", (long) &buffPtr->oiwfs.err1, buffPtr->oiwfs.err1);
    printf ("err2       Addr = %lx, Value = %f\n", (long) &buffPtr->oiwfs.err2, buffPtr->oiwfs.err2);
    printf ("err3       Addr = %lx, Value = %f\n", (long) &buffPtr->oiwfs.err3, buffPtr->oiwfs.err3);
    printf ("name       Addr = %lx, Value = %s\n", (long) buffPtr->oiwfs.name, buffPtr->oiwfs.name);
    printf ("interval   Addr = %lx, Value = %f\n", (long) &buffPtr->oiwfs.interval, buffPtr->oiwfs.interval);
    printf ("time       Addr = %lx, Value = %f\n", (long) &buffPtr->oiwfs.time, buffPtr->oiwfs.time);
}

/* ===================================================================== */

void    printPage11 (const memMap * buffPtr)
{
    printf ("\nPage 11 - gaos data\n");
    printf ("gaos       Addr = %lx,           \n", (long) &buffPtr->gaos.z1);
    printf ("z1     Addr = %lx, Value = %f\n", (long) &buffPtr->gaos.z1, buffPtr->gaos.z1);
    printf ("z2     Addr = %lx, Value = %f\n", (long) &buffPtr->gaos.z2, buffPtr->gaos.z2);
    printf ("z3     Addr = %lx, Value = %f\n", (long) &buffPtr->gaos.z3, buffPtr->gaos.z3);
    printf ("err1       Addr = %lx, Value = %f\n", (long) &buffPtr->gaos.err1, buffPtr->gaos.err1);
    printf ("err2       Addr = %lx, Value = %f\n", (long) &buffPtr->gaos.err2, buffPtr->gaos.err2);
    printf ("err3       Addr = %lx, Value = %f\n", (long) &buffPtr->gaos.err3, buffPtr->gaos.err3);
    printf ("name       Addr = %lx, Value = %s\n", (long) buffPtr->gaos.name, buffPtr->gaos.name);
    printf ("interval   Addr = %lx, Value = %f\n", (long) &buffPtr->gaos.interval, buffPtr->gaos.interval);
    printf ("time       Addr = %lx, Value = %f\n", (long) &buffPtr->gaos.time, buffPtr->gaos.time);
}

/* ===================================================================== */

void    printPage12 (const memMap * buffPtr)
{
    printf ("\nPage 12 - gyro data\n");
    printf ("gyro       Addr = %lx,           \n", (long) &buffPtr->gyro.z1);
    printf ("z1     Addr = %lx, Value = %f\n", (long) &buffPtr->gyro.z1, buffPtr->gyro.z1);
    printf ("z2     Addr = %lx, Value = %f\n", (long) &buffPtr->gyro.z2, buffPtr->gyro.z2);
    printf ("z3     Addr = %lx, Value = %f\n", (long) &buffPtr->gyro.z3, buffPtr->gyro.z3);
    printf ("err1       Addr = %lx, Value = %f\n", (long) &buffPtr->gyro.err1, buffPtr->gyro.err1);
    printf ("err2       Addr = %lx, Value = %f\n", (long) &buffPtr->gyro.err2, buffPtr->gyro.err2);
    printf ("err3       Addr = %lx, Value = %f\n", (long) &buffPtr->gyro.err3, buffPtr->gyro.err3);
    printf ("name       Addr = %lx, Value = %s\n", (long) buffPtr->gyro.name, buffPtr->gyro.name);
    printf ("interval   Addr = %lx, Value = %f\n", (long) &buffPtr->gyro.interval, buffPtr->gyro.interval);
    printf ("time       Addr = %lx, Value = %f\n", (long) &buffPtr->gyro.time, buffPtr->gyro.time);
}

/* ===================================================================== */

void    printPage13a (const memMap * buffPtr)
{
    printf ("\nPage 13a - M2 Engineering Data\n");

    printf ("follow1    Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.follow1, buffPtr->m2Eng.follow1);
    printf ("follow2    Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.follow2, buffPtr->m2Eng.follow2);
    printf ("follow3    Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.follow3, buffPtr->m2Eng.follow3);
    printf ("current1   Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.current1, buffPtr->m2Eng.current1);
    printf ("current2   Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.current2, buffPtr->m2Eng.current2);
    printf ("current3       Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.current3, buffPtr->m2Eng.current3);
    printf ("kaman1         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.kaman1, buffPtr->m2Eng.kaman1);
    printf ("kaman2         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.kaman2, buffPtr->m2Eng.kaman2);
    printf ("kaman3         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.kaman3, buffPtr->m2Eng.kaman3);
    printf ("integ1         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.integ1, buffPtr->m2Eng.integ1);
    printf ("integ2         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.integ2, buffPtr->m2Eng.integ2);
    printf ("integ3         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.integ3, buffPtr->m2Eng.integ3);

    printf ("rawXTilt       Addr = %lx, Value = %lu\n", (long) &buffPtr->m2Eng.rawXTilt, buffPtr->m2Eng.rawXTilt);
    printf ("rawYTilt       Addr = %lx, Value = %lu\n", (long) &buffPtr->m2Eng.rawYTilt, buffPtr->m2Eng.rawYTilt);
    printf ("rawZFocus      Addr = %lx, Value = %lu\n", (long) &buffPtr->m2Eng.rawZFocus, buffPtr->m2Eng.rawZFocus);

    printf ("TMS2realXTilt  Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.TMS2realXTilt, buffPtr->m2Eng.TMS2realXTilt);
    printf ("TMS2realYTilt  Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.TMS2realYTilt, buffPtr->m2Eng.TMS2realYTilt);
    printf ("TMS2realZFocus Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.TMS2realZFocus, buffPtr->m2Eng.TMS2realZFocus);

    printf ("xTilt          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.xTilt, buffPtr->m2Eng.xTilt);
    printf ("yTilt          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.yTilt, buffPtr->m2Eng.yTilt);
    printf ("zFocus         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zFocus, buffPtr->m2Eng.zFocus);

    printf ("rad2arcsec     Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.rad2arcsec, buffPtr->m2Eng.rad2arcsec);
    printf ("mm2um          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.mm2um, buffPtr->m2Eng.mm2um);

    printf ("#frames        Addr = %lx, Value = %ld\n", (long) &buffPtr->m2Eng.NR, buffPtr->m2Eng.NR);
    printf ("initState      Addr = %lx, Value = %ld\n", (long) &buffPtr->m2Eng.initState, buffPtr->m2Eng.initState);
    printf ("errorSystem    Addr = %lx, Value = %lu\n", (long) &buffPtr->m2Eng.errorSystem, buffPtr->m2Eng.errorSystem);
    printf ("errorCode      Addr = %lx, Value = %lu\n", (long) &buffPtr->m2Eng.errorCode, buffPtr->m2Eng.errorCode);

}
   
/* ===================================================================== */

void    printPage13b (const memMap * buffPtr)
{
    printf ("\nPage 13b - M2 Engineering Data\n");

    printf ("azguide        Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azguide, buffPtr->m2Eng.azguide);
    printf ("elguide        Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elguide, buffPtr->m2Eng.elguide);
    printf ("zcmd           Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zcmd, buffPtr->m2Eng.zcmd);

    printf ("azcmd          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azcmd, buffPtr->m2Eng.azcmd);
    printf ("elcmd          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elcmd, buffPtr->m2Eng.elcmd);
    printf ("zunused        Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zunused, buffPtr->m2Eng.zunused);

    printf ("aztotcmd       Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.aztotcmd, buffPtr->m2Eng.aztotcmd);
    printf ("eltotcmd       Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.eltotcmd, buffPtr->m2Eng.eltotcmd);
    printf ("ztotcmd        Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.ztotcmd, buffPtr->m2Eng.ztotcmd);

    printf ("azrate         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azrate, buffPtr->m2Eng.azrate);
    printf ("elrate         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elrate, buffPtr->m2Eng.elrate);
    printf ("zrate          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zrate, buffPtr->m2Eng.zrate);

    printf ("azerr          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azerr, buffPtr->m2Eng.azerr);
    printf ("elerr          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elerr, buffPtr->m2Eng.elerr);
    printf ("zerr           Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zerr, buffPtr->m2Eng.zerr);

    printf ("azrf           Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azrf, buffPtr->m2Eng.azrf);
    printf ("elrf           Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elrf, buffPtr->m2Eng.elrf);
    printf ("zrf            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zrf, buffPtr->m2Eng.zrf);

    printf ("azcor          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azcor, buffPtr->m2Eng.azcor);
    printf ("elcor          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elcor, buffPtr->m2Eng.elcor);
    printf ("zcor           Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zcor, buffPtr->m2Eng.zcor);

    printf ("azf            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azf, buffPtr->m2Eng.azf);
    printf ("elf            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elf, buffPtr->m2Eng.elf);
    printf ("zf             Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zf, buffPtr->m2Eng.zf);

    printf ("azp            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azp, buffPtr->m2Eng.azp);
    printf ("elp            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elp, buffPtr->m2Eng.elp);
    printf ("zp             Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zp, buffPtr->m2Eng.zp);

    printf ("azi            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azi, buffPtr->m2Eng.azi);
    printf ("eli            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.eli, buffPtr->m2Eng.eli);
    printf ("zi             Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zi, buffPtr->m2Eng.zi);

    printf ("azd            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azd, buffPtr->m2Eng.azd);
    printf ("eld            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.eld, buffPtr->m2Eng.eld);
    printf ("zd             Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zd, buffPtr->m2Eng.zd);

    printf ("g              Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.g, buffPtr->m2Eng.g);

    printf ("azcg           Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azcg, buffPtr->m2Eng.azcg);
    printf ("elcg           Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elcg, buffPtr->m2Eng.elcg);
    printf ("zcg            Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zcg, buffPtr->m2Eng.zcg);

    printf ("azsmg0         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azsmg0, buffPtr->m2Eng.azsmg0);
    printf ("elsmg0         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elsmg0, buffPtr->m2Eng.elsmg0);
    printf ("zsmg0          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zsmg0, buffPtr->m2Eng.zsmg0);

    printf ("azsmg1         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azsmg1, buffPtr->m2Eng.azsmg1);
    printf ("elsmg1         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elsmg1, buffPtr->m2Eng.elsmg1);
    printf ("zsmg1          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zsmg1, buffPtr->m2Eng.zsmg1);

    printf ("azsmg2         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.azsmg2, buffPtr->m2Eng.azsmg2);
    printf ("elsmg2         Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.elsmg2, buffPtr->m2Eng.elsmg2);
    printf ("zsmg2          Addr = %lx, Value = %f\n", (long) &buffPtr->m2Eng.zsmg2, buffPtr->m2Eng.zsmg2);

}


#ifndef MK
/* ===================================================================== */

void    printPage15 (const memMap * buffPtr)
{
    printf ("\nPage 15 - GPI oiwfs data\n");
    printf ("oiwfs      Addr = %lx,           \n", (long) &buffPtr->gpi.z1);
    printf ("z1     Addr = %lx, Value = %f\n", (long) &buffPtr->gpi.z1, buffPtr->gpi.z1);
    printf ("z2     Addr = %lx, Value = %f\n", (long) &buffPtr->gpi.z2, buffPtr->gpi.z2);
    printf ("z3     Addr = %lx, Value = %f\n", (long) &buffPtr->gpi.z3, buffPtr->gpi.z3);
    printf ("err1       Addr = %lx, Value = %f\n", (long) &buffPtr->gpi.err1, buffPtr->gpi.err1);
    printf ("err2       Addr = %lx, Value = %f\n", (long) &buffPtr->gpi.err2, buffPtr->gpi.err2);
    printf ("err3       Addr = %lx, Value = %f\n", (long) &buffPtr->gpi.err3, buffPtr->gpi.err3);
    printf ("name       Addr = %lx, Value = %s\n", (long) buffPtr->gpi.name, buffPtr->gpi.name);
    printf ("interval   Addr = %lx, Value = %f\n", (long) &buffPtr->gpi.interval, buffPtr->gpi.interval);
    printf ("time       Addr = %lx, Value = %f\n", (long) &buffPtr->gpi.time, buffPtr->gpi.time);
}
#endif

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * selector
 * 
 * Purpose:
 * Use selection index on port A to set corresponding port A through H to
 * one else zero. Primarily intended to set color rules on dm screens which
 * operate largely on a '1' or 'not 1' basis.
 *
 * Invocation:
 * struct genSubRecord *pgsub
 * status = selector(struct genSubRecord *pgsub)
 * 
 * Parameters in:
 *      > pgsub->a  long    selection index in range (0 .. 7)
 * 
 * Parameters out:
 *      < pgsub->vala   long    bit 0
 *      < pgsub->valb   long    bit 1
 *      < pgsub->valc   long    bit 2
 *      < pgsub->vald   long    bit 3
 *      < pgsub->vale   long    bit 4
 *      < pgsub->valf   long    bit 5
 *      < pgsub->valg   long    bit 6
 *      < pgsub->valh   long    bit 7
 *
 * Return value:
 *      < status    long
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
 * 31-Oct-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */
long    initSelector (struct genSubRecord * pgsub)
{
    return (OK);
}

long    selector (struct genSubRecord * pgsub)
{
    int choice, outputs[] = {0, 0, 0, 0, 0, 0, 0, 0};

    choice = (int) *(long *)pgsub->a;

    if(choice > 7 || choice < 0)
        return(ERROR);

    outputs[choice] = 1;

    *(long *)pgsub->vala = outputs[0];  
    *(long *)pgsub->valb = outputs[1];  
    *(long *)pgsub->valc = outputs[2];  
    *(long *)pgsub->vald = outputs[3];  
    *(long *)pgsub->vale = outputs[4];  
    *(long *)pgsub->valf = outputs[5];  
    *(long *)pgsub->valg = outputs[6];  
    *(long *)pgsub->valh = outputs[7];  

    return(OK);
}



/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * checkFrames
 * 
 * Purpose:
 * Test frame of reference conversions
 * 
 * Invocation:
 * status = checkFrames(xTilt, yTilt, zFocus, xPos, yPos);
 *
 * Parameters in:
 *      > xTilt     double  x tilt tcs demand
 *      > yTilt     double  y tilt tcs demand
 *      > zFocus    double  z focus tcs demand
 *      > xTilt     double  x pos tcs demand
 *      > xTilt     double  x pos tcs demand
 * None
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      > status    int OK or ERROR
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
 * 07-Nov-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

void testFrame(double xTilt, double yTilt, double zFocus, double xPos, double yPos)
{
    static location position1, position2;

    position1.xTilt = xTilt;
    position1.yTilt = yTilt;
    position1.zFocus = zFocus;
    position1.xPos = xPos;
    position1.yPos = yPos;

    tcs2m2(&position1);

    printf("Parameter           tcs              m2\n");
    printf("---------------------------------------\n");
    printf("xTilt               %2.4f            %2.4f\n", xTilt, position1.xTiltNew);
    printf("yTilt               %2.4f            %2.4f\n", yTilt, position1.yTiltNew);
    printf("zFocus              %2.4f            %2.4f\n", zFocus, position1.zFocusNew);
    printf("xPos                %2.4f            %2.4f\n", xPos, position1.xPosNew);
    printf("yPos                %2.4f            %2.4f\n", yPos, position1.yPosNew);

    position2.xTilt = position1.xTiltNew;
    position2.yTilt = position1.yTiltNew;
    position2.zFocus = position1.zFocusNew;
    position2.xPos = position1.xPosNew;
    position2.yPos = position1.yPosNew;

    m22tcs(&position2);

    printf("Parameter           tcs              m2\n");
    printf("---------------------------------------\n");
    printf("xTilt               %2.4f            %2.4f\n", position1.xTiltNew, position2.xTilt);
    printf("yTilt               %2.4f            %2.4f\n", position1.yTiltNew, position2.yTilt);
    printf("zFocus              %2.4f            %2.4f\n", position1.zFocusNew, position2.zFocus);
    printf("xPos                %2.4f            %2.4f\n", position1.xPosNew, position2.xPos);
    printf("yPos                %2.4f            %2.4f\n", position1.yPosNew, position2.yPos);

} 


void checkFiltered(int index)
{
    printf("filtered[%d]                                  \n", index);
    printf("---------------------------------------\n");
    printf("z1                = %2.4f\n", filtered[index].z1);
    printf("z2                = %2.4f\n", filtered[index].z2);
    printf("z3                = %2.4f\n", filtered[index].z3);
    printf("err1              = %2.4f\n", filtered[index].err1);
    printf("err2              = %2.4f\n", filtered[index].err2);
    printf("err3              = %2.4f\n", filtered[index].err3);
    printf("name              = %s\n", filtered[index].name);
    printf("interval          = %f\n", filtered[index].interval);
    printf("notUsed          = %f\n", filtered[index].notUsed);
    printf("time              = %f\n", filtered[index].time);

}


void showMaster(void)
{
    int source;

    printf("SOURCE     A       B      C      A2B      B2A\n");
    printf("---------------------------------------------\n");

    for(source = PWFS1; source <= GYRO; source++)
    {
        printf("   %d       %d       %d      %d       %d        %d\n",
                source, guideMaster[source][0], guideMaster[source][1], guideMaster[source][2],
                        guideMaster[source][3],guideMaster[source][4]);
    }
}



int printval = 0;

void fillWfs(void *p)
{
   double value = 0.0; 

#ifndef MK
   double smallRand;
#endif

   for (;;) {

#ifdef MK
/*
      scsBase->pwfs2.z1 = xTiltGuideSimScale * phasor.command;
      scsBase->pwfs2.z2 = yTiltGuideSimScale * phasor.command;
      scsBase->pwfs2.z3 = 0.0;
      scsBase->pwfs2.err1 = phasor.command * 0.2;
      scsBase->pwfs2.err2 = phasor.command * 0.2;
      scsBase->pwfs2.err3 = phasor.command * 0.2;
*/
      scsBase->pwfs2.z3 = 0.0;
#else
      smallRand = RANDnorm();

      scsBase->pwfs2.z1 = xTiltGuideSimScale * (smallRand + 0.1);
      scsBase->pwfs2.z2 = yTiltGuideSimScale * (smallRand + 0.1);
      scsBase->pwfs2.z3 = 0.0;
      scsBase->pwfs2.err1 = smallRand * 0.2;
      scsBase->pwfs2.err2 = smallRand * 0.2;
      scsBase->pwfs2.err3 = smallRand * 0.2;
      scsBase->pwfs2.time = value++;
      scsBase->pwfs2.interval += (float)(gInterval);
#endif

      scsBase->pwfs2.time = value++;
      scsBase->pwfs2.interval += (float)(gInterval);

      if (stopGuideSim)
         break;

      if (freeRunGuideSim)
         rmISR3(3);
         
#ifdef MK
      /* guideSimDelayTicks could be set to match the intended 100Hz 
       * sample frequency seen by M2TS (CEM). This normally depends
       * on the incomming WFS interrupt rate, then cut in half
       * since every other sample is discarded.
       *
       * Example:
       *    sysClkRateGet = 200; ==> 5ms per tick
       *    guideSimDelayTicks = 1; ==> 5ms delay
       *    guideSimDelayTicks = 2; ==> 10ms delay
       * */
      epicsThreadSleep(guideSimDelay);
#else
      epicsThreadSleep(0.001);  /* wait only one clock tick */
#endif
   }

   /* thread is deletped when it terminates */
}

void startGuideSim(void) {

   stopGuideSim = 0;
   guideSimOn = 1;

   guideSimTaskId = epicsThreadCreate("testWfs", epicsThreadPriorityHigh, 
                           epicsThreadGetStackSize(epicsThreadStackBig),
                           (EPICSTHREADFUNC)fillWfs, (void *)NULL);
   
   if (guideSimTaskId == NULL)
   {
      errlogMessage("Unable to spawn guideSimTask task. guideSimOn set to 0.\n");
      guideSimOn = 0;
   }
   else {
      printf("Guide simulation activated.\n");
   }
}

void endGuideSim() {
   stopGuideSim = 1;
   guideSimOn = 0;
   
   printf("Guide simulation DE-activated.\n");
}

void startfreeRun() {
   
   freeRunGuideSim=1;
}

void endfreeRun() {
   
   freeRunGuideSim=0;
}

long pulseSteerCAD(struct cadRecord *pcad) {
   long status = CAD_REJECT;

   switch(pcad->dir) {

      case menuDirectiveCLEAR:
         printf("PULSE ISR3: CLEAR \n");
         strncpy (pcad->mess, "", MAX_STRING_SIZE - 1);
         status = CAD_ACCEPT ;
         break;

      case menuDirectiveMARK:
         printf("PULSE ISR3: MARK \n");
         strncpy (pcad->mess, "", MAX_STRING_SIZE - 1);
         status = CAD_ACCEPT ;
         break;

      case menuDirectivePRESET:

         if (!guideSimOn) {
            printf("pulseISR3: Reject if guide sim OFF. \n");
            strncpy (pcad->mess, "pulseISR3: Reject if guide sim OFF.", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
         }

         if (freeRunGuideSim) {
            printf("pulseISR3: Reject pulsing if freeRun is active. \n");
            strncpy (pcad->mess, "pulseISR3: Reject pulse if freeRun", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
         }

         status = CAD_ACCEPT;
         break;

      case menuDirectiveSTART:
            printf("pulsing rmISR3\n");
            rmISR3(3);
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

long CADfreeRun(struct cadRecord *pcad) {
   long status = CAD_REJECT;
   static int myFreeRun;
   static char *freeRunOpts[] = {"OFF", "ON", NULL};

   switch(pcad->dir) {

      case menuDirectiveCLEAR:
         printf("CADguideFreeRun: CLEAR \n");
         strncpy (pcad->mess, "", MAX_STRING_SIZE - 1);
         status = CAD_ACCEPT ;
         break;

      case menuDirectiveMARK:
         printf("CADguideFreeRun: MARK \n");
         strncpy (pcad->mess, "", MAX_STRING_SIZE - 1);
         status = CAD_ACCEPT ;
         break;

      case menuDirectivePRESET:

         if (!guideSimOn) {
            printf("guideSimFreeRun: Reject if guide sim OFF. \n");
            strncpy (pcad->mess, "Reject if sim OFF.", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
            break;
         }

         printf("CADguideFreeRun: PRESET freeRunGuideSim %d \n", myFreeRun);

         if (tcsDcString (freeRunOpts, "guideFreeRun:  ", pcad->a, &myFreeRun, pcad)){
            errorLog ("CADguideFreeRun error validating input", 2, ON);

            status = CAD_REJECT;
            break ;
         }
         printf("CADguideFreeRun: PRESET freeRunGuideSim %d \n", myFreeRun);

         status = CAD_ACCEPT;
         break;

      case menuDirectiveSTART:
         if (myFreeRun) {
            printf("starting free run.\n");
            startfreeRun();
         }
         else {
            printf("ending free run.\n");
            endfreeRun();
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


/*****************************************
 * Name: guideSimControl

 * Description: This controls a highlevel interface to simulate guiding.
 *
 *
 */
long guideSimProc (struct genSubRecord * pgsub) {

   static int myguideSim;
   static long mytest;
   static double myXscale, myYscale;

   myguideSim = *(long *) pgsub->a;
   mytest = *(long *) pgsub->a;

   printf("Turn guide sim %d\n", myguideSim);
   printf("Turn guide sim test %ld\n", mytest);

   if (myguideSim) {  /* Request to turn guide sim on*/

      myXscale = *(double *) pgsub->b;
      myYscale = *(double *) pgsub->c;

      if (myXscale < 0.0 || myXscale > 1.0) { 
        printf("guide sim X scale limit\n");
        myXscale = 0.0;
      }
      if (myYscale < 0.0 || myYscale > 1.0) { 
        printf("guide sim Y scale limit\n");
        myYscale = 0.0;
      }
      xTiltGuideSimScale = myXscale;
      yTiltGuideSimScale = myYscale;

      guideOn = ON;
      guideOnA = ON;
      guideOnB = OFF;
      guideOnC = OFF;

   }
   else {  /* Request to turn guide sim off*/

      xTiltGuideSimScale = 0.0;
      yTiltGuideSimScale = 0.0;

      guideOn = OFF;
      guideOnA = OFF;
      guideOnB = OFF;
      guideOnC = OFF;

   }

   /* Writing to vala causes the startGuideSimChange state
    * machine to engauge, so do it last.*/
   *(long *)pgsub->vala = myguideSim;

   return(OK); 
}

long CADguideSimCont (struct cadRecord * pcad)
{
    long status = CAD_REJECT;
    static int guideSim;
    static char *guideSimOpts[] = {"OFF", "ON", NULL};

    cadDirLog ("guideSim", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
	printf("CADguideSim: MARK \n");
        strncpy (pcad->mess, "", MAX_STRING_SIZE - 1);
        status = CAD_ACCEPT ;
        break;

    case menuDirectiveCLEAR:
	printf("CADguideSim: CLEAR \n");
        strncpy (pcad->mess, "", MAX_STRING_SIZE - 1);
        status = CAD_ACCEPT ;
        break;

    case menuDirectivePRESET:

        printf("CADguideSimulate: PRESET guideSimulate %d \n", guideSim);

        if (tcsDcString (guideSimOpts, "guideSimulate:  ", pcad->a, &guideSim, pcad)){
           errorLog ("CADguideSimulate - failed guide simulate conversion", 2, ON);
           break ;
        }

        printf("CADguideSimulate: PRESET guideSimulate %d \n",guideSim);

        /* If the request turns off guide simulation, then accept it, 
         * otherwise provide additional checks.
         */
        if (guideSim == 0) {
           status = CAD_ACCEPT;
        }

        else {

           if (guideOn ) {

              printf("guideSimControl: ERROR. Found global variable guideOn. Rejecting\n");
              strncpy (pcad->mess, "guide is already active", MAX_STRING_SIZE - 1);
              status = CAD_REJECT ;
              break;
           }

           /*if (chopOn ) {
             printf("guideSimControl: ERROR. Found chopping ON. Rejecting... \n");
             status = CAD_REJECT ;

             }*/

           if ( guideOnA && guideOnB ) {
              printf("guideSimControl: ERROR. Found guide ON. Rejecting... \n");
              strncpy (pcad->mess, "Cannot guide on A and B", MAX_STRING_SIZE - 1);
              status = CAD_REJECT;
              break;
           }

           status = CAD_ACCEPT;
        }

        *(long *)pcad->vala = guideSim;
        break;

    case menuDirectiveSTART:
        if (interlockFlag == ON)
        {
           strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
           break;
        }
        status = CAD_ACCEPT ;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}
void driveP1(void)
{
    double timeStamp;
    double x = 0.;    /* x tilt guide correction in arcsec */
    double y = 0.;
    double z = 0.;

    for (;;)
    {
        timeNow(&timeStamp);

        scsBase->pwfs1.z1 = x;
        scsBase->pwfs1.z2 = y;
        scsBase->pwfs1.z3 = z;
        scsBase->pwfs1.err1 = 0.0;
        scsBase->pwfs1.err2 = 0.0;
        scsBase->pwfs1.err3 = 0.0;
        /*scsBase->pwfs1.time = timeStamp;*/
        scsBase->pwfs1.interval = timeStamp;

        x += 0.0001;
        y += 0.0001;
        z += 0.0001;

	/*delay 2 seconds */
	epicsThreadSleep(2.0);
    }
}


void driveP2(void)
{
    double timeStamp;
    double x = 0.;    /* x tilt guide correction in arcsec */
    double y = 0.;
    double z = 0.;

    for (;;)
    {
        timeNow(&timeStamp);

        scsBase->pwfs2.z1 = x;
        scsBase->pwfs2.z2 = y;
        scsBase->pwfs2.z3 = z;
        scsBase->pwfs2.err1 = 0.0;
        scsBase->pwfs2.err2 = 0.0;
        scsBase->pwfs2.err3 = 0.0;
        scsBase->pwfs2.time = timeStamp;

        x += 0.0001;
        y += 0.0001;
        z += 0.0001;

	/*delay 2 seconds */
	epicsThreadSleep(2.0);
    }
}


void testm22tcs(double xp, double yp)
{
    location m2Position;

    m2Position.xTilt = 0.;
    m2Position.yTilt = 0.;
    m2Position.zFocus = 0.;
    m2Position.xPos = xp;
    m2Position.yPos = yp;

    printf("XY position is: %f um XP, %f um YP\n", 
            m2Position.xPos, m2Position.yPos); 
    m22tcs(&m2Position);
    printf("\tconverted to: %f um TCS, %f um TCS\n", 
           m2Position.xPosNew, m2Position.yPosNew); 
}

void testtcs2m2(double xp, double yp)
{
    location m2Position;

    m2Position.xTilt = 0.;
    m2Position.yTilt = 0.;
    m2Position.zFocus = 0.;
    m2Position.xPos = xp;
    m2Position.yPos = yp;

    printf("XY position is: %f um TCS, %f um TCS\n", 
           m2Position.xPos, m2Position.yPos); 
    tcs2m2(&m2Position);
    printf("\tconverted to: %f um YP, %f um YP\n", 
           m2Position.xPosNew, m2Position.yPosNew); 
}


