/* $Id: control.h,v 1.16 2015/03/17 18:49:37 mrippa Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * control.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for control.c
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
 * 19-OCT-2017: gon conversion to EPICS OSI (mdw)
 *
 */
/* ===================================================================== */
#ifndef _INCLUDED_CONTROL_H
#define _INCLUDED_CONTROL_H

#include "chopControl.h"        /* For BEAMA definition */
#include "elgLib.h"
#include "guide.h"
#include "utilities.h"          /* For MAX_SOURCES */

#define AUTOGUIDE 0
#define PROJECT   1

#define SCS_NODE                0       /* Node 0 is the SCS       */
#define M2_NODE                 1       /* Node 1 is the M2 system */      
#define AGP1_NODE               2       /* Node 2 is the A&G PWFS1 */   
#define AGP2_NODE               3       /* Node 3 is the A&G PWFS2 */   

#define AGOI_NODE               4       /* Node 4 is the A&G OIWFS */   
#define GAOS_NODE               5       /* Node 5 is the ALTAIR    */   

#define SYSTEM_CLOCK_RATE      200      /* Number of ticks per second */ 

#define SEM_TIMEOUT            0.5      /* This had been 100 ticks (@ 200 ticks/sec) */

#define WFS_TIMEOUT             20

#define STATUS_BLOCK_SIZE       23      /* number of longs for checksum 
                                           to consider                  */
#define COMMAND_BLOCK_SIZE      61      /* number of longs for checksum 
                                           to consider                  */

#define MAX_FAULTS              30  /* first 30 faults in diag block */
#define CEM_TIME_SIZE           16  /* String for CEM showing current scs time */

/* Define structure to hold three term controller constants */

typedef struct
{
    double  P;              /* proportional gain term                */
    double  I;              /* integral gain term                    */
    double  D;              /* derivative gain term                  */
    double  windUpLimit;    /* integrator windup limit               */
    double  rateLimit;      /* maximum step change in u per sample   */
    double  oldError;       /* previous error sample                 */
    double  sum;            /* running integration of error signal   */
    double  oldOutput;      /* last output value                     */
    double  oldSum;         /* sum last time through the loop        */
}PID;

typedef struct demands
{
    double  timeSent;
    double  timeApply;
    long    trackId;
    double  xTiltA;
    double  yTiltA;
    double  xTiltB;
    double  yTiltB;
    double  xTiltC;
    double  yTiltC;
    double  zFocus;
    double  xPosition;
    double  yPosition;
    double  xGuide;
    double  yGuide;
}Demands;

/* structure of the M2 to SCS status word defined as bitfield */

typedef union
{
    unsigned all;   /* name to refer to the whole structure */
    char byte[4];   /* individual bytes of whole structure  */

    struct
    {
        unsigned offloaders             : 1;    /* bit 31 */
        unsigned mirrorControl          : 1;
        unsigned mirrorMoving           : 1;
        unsigned mirrorCommanded        : 1;
        unsigned mirrorResponding       : 1;
        unsigned sensorLimit            : 1;
        unsigned actuatorLimit          : 1;
        unsigned thermalLimit           : 1;
        unsigned mirrorDspInt           : 1;
        unsigned vibDspInt              : 1;
        unsigned blank                  : 10;   
        /* bits 12 through 21 currently unused */
        unsigned space                  : 1;
        unsigned decsFrozen             : 1;
        unsigned decsPaused             : 1;
        unsigned decsOn                 : 1;
        unsigned chopOn                 : 1;
        unsigned vibControlOn           : 1;
        unsigned testInProgress         : 1;
        unsigned resetInProgress        : 1;
        unsigned initInProgress         : 1;
        unsigned powerEnabled           : 1;
        unsigned diagnosticsAvailable   : 1;
        unsigned health                 : 1;    /* bit 0 */
    } flags;

} bitFieldM2;

/* structure of diagnostic codes */

typedef struct
{
    short int index;
    char subsystem;
    char code;
}fault;


/* SCS to M2 command block */

typedef struct
{
    long            checksum;
    long            NS;
    long            commandCode;
    float           xTiltGuide;
    float           yTiltGuide;
    float           zFocusGuide;
    float           AxTilt;
    float           AyTilt;
    float           BxTilt;
    float           ByTilt;
    float           CxTilt;
    float           CyTilt;
    float           actuator1;
    float           actuator2;
    float           actuator3;
    long            heartbeat;
    float           xDemand;
    float           yDemand;
    long            centralBaffle;
    long            deployBaffle;
    long            chopProfile;
    float           chopFrequency;
    float           chopDutyCycle;
    float           xTiltTolerance;
    float           yTiltTolerance;
    float           zFocusTolerance;
    float           xPositionTolerance;
    float           yPositionTolerance;
    float           bandwidth;
    float           xTiltGain;
    float           yTiltGain;
    float           zFocusGain;
    float           xTiltShift;
    float           yTiltShift;
    float           zFocusShift;
    float           xTiltSmooth;
    float           yTiltSmooth;
    float           zFocusSmooth;
    float           xTcsMinRange;
    float           yTcsMinRange;
    float           xTcsMaxRange;
    float           yTcsMaxRange; 
    float           xPMinRange;
    float           yPMinRange;
    float           xPMaxRange;
    float           yPMaxRange; 
    long            follower;
    long            foldir;
    long            followersteps;
    long            offloader;
    long            ofldir;
    long            offloadersteps;
    long            cbafdir;
    long            cbsteps;
    long            deployable_baffle;
    long            dbafdir;
    long            dbsteps;
    long            xy_motor;
    long            xydir;
    long            xysteps;
    float           xyPositionDeadband;
    char            scsTime[16];
    float           zFocus;    /*not used by M2. for dm display purposes only*/
    float           zGuide;    /*not used by M2. for dm display purposes only*/
    float           rawXGuide; /*not used by M2. for dm display purposes only*/
    float           rawYGuide; /*not used by M2. for dm display purposes only*/
    float           rawZGuide; /*not used by M2. for dm display purposes only*/
    float           xGrossTiltDmd; /*not used by M2. for dm display purposes only*/
    float           yGrossTiltDmd; /*not used by M2. for dm display purposes only*/
    float           pad[184]; /* Was 188 before scsTime */
}commandBlock;

/* M2 to SCS status block */

typedef struct
{
    long            checksum;               /*  0 0x00 */
    long            NR;                     /*  1 0x04 */
    float           xTilt;                  /*  2 0x08 */
    float           yTilt;                  /*  3 0x0c */
    float           zFocus;                 /*  4 0x10 */
    float           actuator1;              /*  5 0x14 */
    float           actuator2;              /*  6 0x18 */
    float           actuator3;              /*  7 0x1c */
    long            inPosition;             /*  8 0x20 */
    long            chopTransition;         /*  9 0x24 */
    bitFieldM2      statusWord;             /* 10 0x28 */
    long            heartbeat;              /* 11 0x2c */
    long            beamPosition;           /* 12 0x30 */
    float           xPosition;              /* 13 0x34 */
    float           yPosition;              /* 14 0x38 */
    long            deployBaffle;           /* 15 0x3c */
    long            centralBaffle;          /* 16 0x40 */
    float           baffleEncoderA;         /* 17 0x44 */
    float           baffleEncoderB;         /* 18 0x48 */
    float           baffleEncoderC;         /* 19 0x4c */
    long            topEnd;                 /* 20 0x50 */
    float           enclosureTemp;          /* 21 0x54 */
    float           upperBearingAngle;      /* 22 0x58 */
    float           lowerBearingAngle;      /* 23 0x5c */
    float           pad[232];
}statusBlock;

typedef struct
{
    long    checksum;
    long    number;
    fault   faults[MAX_FAULTS];
    float   pad[224];
}diagBlock;

typedef struct
{
    float   z1;
    float   z2;
    float   z3;
    float   err1;
    float   err2;
    float   err3;
    char    name[16];
    float   interval;
    float   notUsed;        
    double  time;
    float   pad[242];   
}wfsBlock;

typedef struct
{
    float   z1;
    float   z2;
    float   z3;
    float   err1;
    float   err2;
    float   err3;
    char    name[16];
    float   interval;
    float   notUsed;        
    double  time;
}wfs;

typedef struct
{
    float   pad[256];
}unusedBlock;

typedef struct
{
    int    currentBeam;
    int    inPosition;     
    /*float   xTilt;
    float   yTilt;
    float   zFocus;
    float   xPosition;
    float   yPosition;
    float   notUsed;        
    double  time;*/
}eventBlock;

typedef struct
{
    float           notUsed;
    float           follow1;
    float           follow2;
    float           follow3;
    float           current1;
    float           current2;
    float           current3;
    float           kaman1;
    float           kaman2;
    float           kaman3;
    float           integ1;
    float           integ2;
    float           integ3;
    long            rawXTilt;
    long            rawYTilt;
    long            rawZFocus;
    float           TMS2realXTilt;
    float           TMS2realYTilt;
    float           TMS2realZFocus;
    float           rad2arcsec;
    float           mm2um;
    float           xTilt; 
    float           yTilt;
    float           zFocus; 
    long            NR;
    long            initState;
    long            errorSystem;
    long            errorCode;
    float           azguide;
    float           elguide;
    float           zcmd;
    float           azcmd;
    float           elcmd;
    float           zunused;
    float           aztotcmd;
    float           eltotcmd;
    float           ztotcmd;
    float           azrate;
    float           elrate;
    float           zrate;
    float           azerr;
    float           elerr;
    float           zerr;
    float           azrf;
    float           elrf;
    float           zrf;
    float           azcor;
    float           elcor;
    float           zcor;
    float           azf;
    float           elf;
    float           zf;
    float           azp;
    float           elp;
    float           zp;
    float           azi;
    float           eli;
    float           zi;
    float           azd;
    float           eld;
    float           zd;
    float           g;      /* Not yet available. Here to end of page */
    float           azcg;
    float           elcg;
    float           zcg;
    float           azsmg0;
    float           elsmg0;
    float           zsmg0;
    float           azsmg1;
    float           elsmg1;
    float           zsmg1;
    float           azsmg2;
    float           elsmg2;
    float           zsmg2;
    float           pad[182]; 
}m2EngData;

typedef struct
{
    commandBlock    page0;
    statusBlock     page1;
    diagBlock       testResults;    /* page 2 */
    m2EngData       m2Eng;      /* page13a,b */
    unusedBlock     page4;
    unusedBlock     page5;
    unusedBlock     page6;
    unusedBlock     page7;      /* page 7 - no longer used */
    wfsBlock        pwfs1;      /* page 8 */
    wfsBlock        pwfs2;      /* page 9 */
    wfsBlock        oiwfs;      /* page10 */
    wfsBlock        gaos;       /* page11 */
    wfsBlock        gyro;       /* page12 */
    wfsBlock        altair;       /* page13 */
}memMap;

typedef struct                  /* data from m2 to log */
{
    double  time;
    double  xTilt;
    double  yTilt;
    double  zFocus;
    double  setX;
    double  setY;
    double  setZ;
} m2History;

#define HS_RECORD_LENGTH 4000

typedef struct {
    double xTiltPosHS[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/
    double yTiltPosHS[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/
    double zPosHS[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/

    double xTiltNetGuideHS[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/
    double yTiltNetGuideHS[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/
    double zNetGuideHS[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/

    double vtkXCommand[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/
    double vtkXFrequency[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/
    double vtkXPhase[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/

    double vtkYCommand[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/
    double vtkYFrequency[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/
    double vtkYPhase[HS_RECORD_LENGTH]; /*Set FTV<output> to numsamples*/

} HighSpeed;

enum
{
    INT1 = 1,
    INT2,
    INT3
};

void fireLoops(int param);

void processGuides(void);

void slowTransmit(void);

void tiltReceive(void);

void scsReceive(void);

int checkTiltStatus(void);

void cemTimerStart();
void cemTimerEnd();

/* Write event system data to synchro bus. */
int updateEventPage(int scsInPosition, int scsPresentBeam);

long writeCommand (const long command);

double iir_filter(const double input, MATLAB* iir);

/* SCS to M2 command codes */
enum
{
    FAST_ONLY = 0,       /*  0 */
    POSITION,            /*  1 */ 
    CHOP_ON,             /*  2 */
    CHOP_OFF,            /*  3 */
    SYNC_SOURCE_SCS,     /*  4 */
    SYNC_SOURCE_M2,      /*  5 */
    ACT_PWR_ON,          /*  6 */
    ACT_PWR_OFF,         /*  7 */
    CMD_INIT,            /*  8 */  
    CMD_RESET,           /*  9 */
    CMD_TEST,            /* 10 */
    MSTART,              /* 11 */
    MEND,                /* 12 */
    VSTART,              /* 13 */
    VEND,                /* 14 */
    MOFFLON,             /* 15 */
    MOFFLOFF,            /* 16 */
    DECS_ON,             /* 17 */
    DECS_OFF,            /* 18 */
    DECS_PAUSE,          /* 19 */
    DECS_CONTINUE,       /* 20 */
    DECS_FREEZE,         /* 21 */
    DECS_UNFREEZE,       /* 22 */
    DECS_ZERO,           /* 23 */
    TILT_SPACE,          /* 24 */
    ACTUATOR_SPACE,      /* 25 */
    BAFFLE_CHANGE,       /* 26 */
    DECS_CHANGE,         /* 27 */
    BANDWIDTH_CHANGE,    /* 28 */
    TOLERANCE_CHANGE,    /* 29 */
    CHOP_CHANGE,         /* 30 */
    DIAGNOSTICS_REQUEST, /* 31 */
    UPDATE_XY_RANGE,     /* 32 */
    UPDATE_XPYP_RANGE,   /* 33 */
    VSTARTDRVFOL,        /* 34 */ 
    VDRVFOL,             /* 35 */
    VSTOPDRVFOL,         /* 36 */
    MSTARTDRVOFL,        /* 37 */
    MDRVOFL,             /* 38 */
    MSTOPDRVOFL,         /* 39 */
    DBDRV,               /* 40 */
    CBDRV,               /* 41 */
    XYDRV,               /* 42 */
    VDECS_ON,            /* 43 */
    VDECS_OFF,           /* 44 */
    LOGGING_ON,          /* 45 */
    LOGGING_OFF,         /* 46 */
    /* 47-58 reserved for xycom commands (defined in showEngineering.c */
    CMD_XYINIT=59,        /* 59 */
    XY_DEADBAND_CHANGE=60, /* 60 */
    SCS_TIME_UPDATE=61          /* 61 */
};

/* Global variables*/

extern int simLevel;
extern memMap *scsPtr;
extern memMap *scsBase;
extern memMap *m2Ptr;

extern epicsMutexId m2MemFree;
extern epicsMutexId wfsFree[MAX_SOURCES];
extern epicsMutexId eventDataSem;
extern epicsMutexId setPointFree;

extern epicsEventId xySem;
extern epicsEventId slowUpdate;
extern epicsEventId diagnosticsAvailable; 
extern epicsEventId guideUpdateNow;
extern epicsEventId scsDataAvailable;
extern epicsEventId scsReceiveNow;

extern epicsMessageQueueId commandQId;
extern epicsMessageQueueId receiveQId;

extern long interlockFlag;
extern statusBlock safeBlock;
/* get current guide values to send to TCS */
extern double xGuideTcs;
extern double yGuideTcs;
extern double zGuideTcs;
extern Demands setPoint;
extern int currentBeam;
extern int flip;
extern int guideType;
extern PID controller[MAX_AXES];
extern HighSpeed *highSpeedData;
/* not used extern wfs raw[MAX_SOURCES];*/
extern wfs filtered[MAX_SOURCES];
/* Structure to hold position demands */
extern Demands tcs;

/* Structure to hold event data */
extern eventBlock eventData;

/* flag to show following active { ON | OFF } */
extern long followOn;

/* flag to show tilt, focus PIDs active { ON | OFF } */
extern long tiltPidOn;
extern long focusPidOn;
extern long vibrationXTrackOn;
extern long vibrationYTrackOn;
extern long phasorXApply;
extern double xTiltGuideSimScale;
extern double yTiltGuideSimScale;
extern long phasorYApply;

void vtkResetX(void);
void vtkResetY(void);
void phasorResetX(void);
void phasorResetY(void);
void swxon(void);
void swyon(void);
void swxoff(void);
void swyoff(void);

extern long servoInPosition;
extern long servoOnStatus;

extern double SampleData[5][3];
extern double coeffData[5][3];
extern double tiptiltGuideLimitFactor;
extern double focusGuideLimitFactor;

#endif 

