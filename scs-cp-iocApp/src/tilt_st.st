/* $Id: tilt_st.st,v 1.1 2002/02/05 13:19:51 gemvx Exp $ */
/* =========================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * tilt_st.st
 * 
 * PURPOSE
 * -------
 * State notation code for control of the M2 system simulation.
 * 
 * STATESET NAME(S)
 * ----------------
 * tiltControl  -
 * chopConfig   -
 * chopTimer    -
 * readRefMem   -
 * updateScreen -
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
 * 03-Jul-1996: Original (srp)
 * 11-Jul-1996: move definitions to file tilt.h (srp)
 * 12-Jul_1996: Change to emacs editing, various layout shifts (srp)
 * 15-Jul-1996: Variables renamed for consistency (srp)
 * 17-Jul-1996: tilt structure removed, features incorporated in the systemDefinition structure (srp)
 * 19-Jul-1996: addition of in position checks and indicators (srp)
 * 23-Jul-1996: add structure to define position demands for initialisation parking (srp)
 * 24-Jul-1996: remove in position checks (now in capfast diagram). Renaming of variables in accordance
 *      with hierarchical new rearrangement of the capfast designs (srp)
 * 25-Jul-1996: Initialise to a set position for verification purposes (srp)
 * 30-Jul-1996: Modify to prevent automatically moving from boot to initialise to resetting. This
 *      shall now be only in response to the SCS commanding such transitions. (srp)
 * 20-Aug-1996: Modify parking state transitions and parked coordinates
 * 21-Aug-1996: Tidy up, change macro name from subsystem to S
 * 03-Sep-1996: Bug fix. Incorrect change to state idle1 when DUMMY command received corrected to
 *      ignore and wait for next SCS command. Also modify escaping of DEBUG statements
 * 28-May-1997: Move chop timer simulation into SNL from capfast ss chopTimer, tickerfreq now at 20Hz
 * 27-Aug-1997: Corrections to bit settings for power enable/offloaders/vibration control (sense inverted)
 * 21-Oct-1997: Tidy up layout
 * 28-Oct-1997: Modify so demands are provided directly to mechanism gensubs. Hene processing of neutral
 *      demands is not necessary in SNL - strip out references.
 * 02-Feb-1998: Add record processing to set error bits for test purposes
 * 10-May-1999: Added RCS id
 *
 */
/* INDENT ON */
/* =========================================================== */

program tilt_st

%%#include "utilities.h"    /* For checksum */
%%#include "control.h"      /* For M2 commands, m2Ptr, m2MemFree,
                                   diagnosticsAvailable, receiveQId */
%%#include "tiltSim.h"      /* For INTERNAL/EXTERNAL */

%%#include <string.h>

%%#define STATE_TIMEOUT         200     /* longer timeout for low priority 
                                           state code taskes    */

%{

/* define globals */

static  bitFieldM2  statusWord;
static  long        receivedCommand;
static  long        testNow, resetNow, initNow, chopNow;
static  int     count;
static  int     topEnd = 0;
static  float       temperature = 23.4;


}%

/* =========================================================== */
/* The following groups of paramaters access the channels to the Capfast diagrams */

long vibControl;
assign vibControl to "{S}vibControl.VAL";

/* chop configuration variables */

long chopProfile;
assign chopProfile to "{S}mechanism.C";

double chopFrequency;
assign chopFrequency to "{S}mechanism.D";

long chopOn;                        /* chop on/off demand */
assign chopOn to "{S}mechanism.B";

long syncSource;
assign syncSource to "{S}mechanism.E";

/* command and status data to and from the SCS */

evflag scsCommand;                  /* new command received */

long tiltEnable;                    /* enable signal for all m2 mechanisms */
assign tiltEnable to "{S}enable.VAL";

long offLoaders;
assign offLoaders to "{S}offLoaders.VAL";

/* strings to display system status to the engineering screens */

string tiltState;
assign tiltState to "{S}tiltstate.VAL";

/* =========================================================== */

long m2Central;                         /* central baffle position */
assign m2Central to "{S}central.VAL";

long m2Deployable;                      /* deployable baffle position */
assign m2Deployable to "{S}deployable.VAL";

long m2Heartbeat;                       /* 200Hz incrementing hearbeat counter */
assign m2Heartbeat to "{S}heartbeat.VAL";
monitor m2Heartbeat;

string m2Health;                        /* current m2 health indication (good, warning, bad) */
assign m2Health to "{S}tilthealth.VAL";
monitor m2Health;

/* error status bits */

long sensorLimit;
assign sensorLimit to "{S}sensorLimit.VAL";

long actuatorLimit;
assign actuatorLimit to "{S}actuatorLimit.VAL";

long thermalLimit;
assign thermalLimit to "{S}thermalLimit.VAL";

long mirrorDsp;
assign mirrorDsp to "{S}mirrorDsp.VAL";

long vibrationDsp;
assign vibrationDsp to "{S}vibrationDsp.VAL";

/* =========================================================== */
/* 
 *  Function Name:
 *  State set "tiltControl"
 *
 *  Purpose:
 *  Provide state machine control of the m2 system simulation
 *
 *  Description:
 *
 *  Invocation:
 *  Automatic after loading
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
 *  delay       delay for the specified time period
 *
 *  External variables:
 *  see parameters
 *
 *  Prior requirements:
 *  see file header for tilt_st.st
 *
 *  Deficiencies:
 *  see file header for tilt_st.st
 */
/* =========================================================== */   

ss      tiltControl
{
    state   initialise
    {
        when ()
        {
            /* set initialisation bit */

            initNow = OFF;
            statusWord.flags.initInProgress = ON;

            /* write system definition */

                strncpy(tiltState, "INITIALISE", MAX_STRING_SIZE - 1);
            tiltEnable  = DISABLED;
            offLoaders  = DISABLED;
            chopOn      = OFF;

            /* write state variables */

            pvPut (tiltState);
            pvPut (tiltEnable);
            pvPut (offLoaders);
            pvPut (chopOn);

            strncpy (m2Health, "GOOD", MAX_STRING_SIZE - 1);
            pvPut (m2Health);

            /* clear event flags */

            efClear (scsCommand);

        } state initialise1
    }

    /* =================================================================== */

    state   initialise1
    {
        /* wait for a while so the state message is visible */

        when (delay (5.0))
        {
            /* reset initialisation bit */

            statusWord.flags.initInProgress = OFF;

        } state reset
    }

    /* =================================================================== */

    state   reset
    {

        /* The reset state writes the current default values to all
         * channels */

        when ()
        {
            /* set reset bit */

            resetNow = OFF;
            statusWord.flags.resetInProgress = ON;

            /* Write system definition */

                strncpy(tiltState, "RESET", MAX_STRING_SIZE - 1);
            tiltEnable  = DISABLED;
            offLoaders  = DISABLED;
            chopOn      = OFF;

            pvPut (tiltState);
            pvPut (tiltEnable);
            pvPut (offLoaders);
            pvPut (chopOn);

        } state reset1
    }

    /* =================================================================== */

    state   reset1
    {
        /* wait for a while so the state message is visible */

        when (delay (5.0))
        {
            /* reset reset bit */

            statusWord.flags.resetInProgress = OFF;

        } state moving
    }

    /* =================================================================== */

    state   moving
    {
        /* Set up the m2 system for the moving state */

        when ()
        {
            /* Write display strings */

                strncpy(tiltState, "MOVING", MAX_STRING_SIZE - 1);
            tiltEnable  = ENABLED;
            offLoaders  = ENABLED;
            chopOn      = OFF;

            pvPut (tiltState);
            pvPut (tiltEnable);
            pvPut (offLoaders);
            pvPut (chopOn);

        } state movingProcessScsCommand
    }

    /* =================================================================== */

    state   movingProcessScsCommand
    {
        /* examine the recieved command and vector to the appropriate
         * handling routine */

        when (efTest (scsCommand) && initNow == ON)
        {
            efClear (scsCommand);
        } state initialise

        when (efTest (scsCommand) && resetNow == ON)
        {
            efClear (scsCommand);
        } state reset

        when (efTest (scsCommand) && testNow == ON)
        {
            efClear (scsCommand);
        } state test

        when (efTest (scsCommand) && chopNow == ON)
        {
            efClear (scsCommand);
        } state chopping
    }

    /* =================================================================== */

    state   chopping
    {
        /* Initialise the chopping state for the selected parameters */

        when ()
        {
            /* set chopping in progress indicator */

            statusWord.flags.chopOn = 1;

            /* Write display strings */

                strncpy(tiltState, "CHOPPING", MAX_STRING_SIZE - 1);
            tiltEnable  = ENABLED;
            offLoaders  = ENABLED;
            chopOn      = ON;

            pvPut (tiltState);
            pvPut (tiltEnable);
            pvPut (offLoaders);
            pvPut (chopOn);

        } state chopping1
    }

    /* =================================================================== */

    state   chopping1
    {
        when (efTest (scsCommand) && initNow == ON)
        {
            efClear (scsCommand);

            statusWord.flags.chopOn = OFF;

        } state initialise


        when (efTest (scsCommand) && resetNow == ON)
        {
            efClear (scsCommand);

            statusWord.flags.chopOn = OFF;

        } state reset

        when (efTest (scsCommand) && testNow == ON)
        {
            efClear (scsCommand);

            statusWord.flags.chopOn = OFF;

        } state test

        when (efTest (scsCommand) && chopNow == OFF)
        {
            efClear (scsCommand);

            statusWord.flags.chopOn = OFF;

        } state moving
    }

    /* =================================================================== */

    state   test
    {
        when ()
        {
            /* clear calling flag and set in progress indicator */

            testNow = OFF;
            statusWord.flags.testInProgress = ON;

                strncpy(tiltState, "TESTING", MAX_STRING_SIZE - 1);
            tiltEnable  = ENABLED;
            offLoaders  = ENABLED;
            chopOn      = OFF;

            pvPut (tiltState);
            pvPut (tiltEnable);
            pvPut (offLoaders);
            pvPut (chopOn);

        } state test1
    }

    state   test1
    {
        /* delay to simulate testing execution */

        when (delay (5.0))
        {

            statusWord.flags.testInProgress = OFF;

        } state moving
    }
}

/* =========================================================== */

ss readRefMem
{
    /* at the specified period the variables are read from reflective memory */
    /* into their state variables. These are then written out to the system */

    state waitUpdate
    {
        when(delay(0.2))
        {
            epicsMutexLock(m2MemFree);
                /* grab relevant data from the command buffer */

                m2Deployable    = m2Ptr->page0.deployBaffle;
                m2Central   = m2Ptr->page0.centralBaffle;
                chopProfile = m2Ptr->page0.chopProfile;
                chopFrequency   = m2Ptr->page0.chopFrequency;

                epicsMutexUnlock(m2MemFree);

                /* read command from message queue, returns ERROR if no message available   */
                /* in which case assume that it's only a fast guide update          */

                {
                if(epicsMessageQueueTryReceive(receiveQId,  &receivedCommand, sizeof (long)) == ERROR)
                    receivedCommand = FAST_ONLY;
                }

                if(receivedCommand == FAST_ONLY)
                {
                    statusWord.flags.diagnosticsAvailable = ON;
                }
                else if(receivedCommand == CHOP_ON)
                {
                    chopNow = ON;
                    statusWord.flags.chopOn = ON;
                }
                else if(receivedCommand == CHOP_OFF)
                {
                    chopNow = OFF;
                    statusWord.flags.chopOn = OFF;
                }
                else if(receivedCommand == SYNC_SOURCE_SCS)
                {
                    syncSource = EXTERNAL;
                    pvPut(syncSource);
                }
                else if(receivedCommand == SYNC_SOURCE_M2)
                {
                    syncSource = INTERNAL;
                    pvPut(syncSource);
                }
                else if(receivedCommand == ACT_PWR_ON)
                {
                    statusWord.flags.powerEnabled = ON;
                }
                else if(receivedCommand == ACT_PWR_OFF)
                {
                    statusWord.flags.powerEnabled = OFF;
                }
                else if(receivedCommand == CMD_INIT)
                {
                    initNow = ON;
                }
                else if(receivedCommand == CMD_RESET)
                {
                    resetNow = ON;
                }
                else if(receivedCommand == CMD_TEST)
                {
                    testNow = ON;
                }
                else if(receivedCommand == MSTART)
                {
                    tiltEnable = ENABLED;
                    statusWord.flags.mirrorControl      = ON;
                    statusWord.flags.mirrorMoving       = 1;
                    statusWord.flags.mirrorCommanded    = 1;
                    statusWord.flags.mirrorResponding   = 1;
                }
                else if(receivedCommand == MEND)
                {
                    tiltEnable = DISABLED;
                    statusWord.flags.mirrorControl      = OFF;
                    statusWord.flags.mirrorMoving       = 0;
                    statusWord.flags.mirrorCommanded    = 0;
                    statusWord.flags.mirrorResponding   = 0;
                }
                else if(receivedCommand == VIBSTART)
                {
                    vibControl = ENABLED;
                    statusWord.flags.vibControlOn = ON;
                }
                else if(receivedCommand == VEND)
                {
                    vibControl = DISABLED;
                    statusWord.flags.vibControlOn = OFF;
                }
                else if(receivedCommand == MOFFLON)
                {
                    offLoaders = ENABLED;
                    statusWord.flags.offloaders = ON;
                }
                else if(receivedCommand == MOFFLOFF)
                {
                    offLoaders = DISABLED;
                    statusWord.flags.offloaders = OFF;
                }
                else if(receivedCommand == DECS_ON)
                {
                    statusWord.flags.decsOn = ON;
                }
                else if(receivedCommand == DECS_OFF)
                {
                    statusWord.flags.decsOn = OFF;
                }
                else if(receivedCommand == DECS_PAUSE)
                {
                    statusWord.flags.decsPaused = ON;
                }
                else if(receivedCommand == DECS_CONTINUE)
                {
                    statusWord.flags.decsPaused = OFF;
                }
                else if(receivedCommand == DECS_FREEZE)
                {
                    statusWord.flags.decsFrozen = ON;
                }
                else if(receivedCommand == DECS_UNFREEZE)
                {
                    statusWord.flags.decsFrozen = OFF;
                }
                else if(receivedCommand == TILT_SPACE)
                {
                    statusWord.flags.space = 0;
                }
                else if(receivedCommand == ACTUATOR_SPACE)
                {
                    statusWord.flags.space = 1;
                }
                else if(receivedCommand == CHOP_CHANGE)
                {
                    pvPut(chopProfile);
                    pvPut(chopFrequency);
                }
                else if(receivedCommand == DIAGNOSTICS_REQUEST)
                {
                    statusWord.flags.diagnosticsAvailable = OFF;

                    for(count = 0; count < 5; count++)
                    {
                        m2Ptr->testResults.faults[count].index      = count;
                        m2Ptr->testResults.faults[count].subsystem  = count;
                        m2Ptr->testResults.faults[count].code       = count;
                        }
                    m2Ptr->testResults.number = 4;
                }
            efSet(scsCommand);

            /* read and set error status bits */

            pvGet(sensorLimit);
            pvGet(actuatorLimit);
            pvGet(thermalLimit);
            pvGet(mirrorDsp);
            pvGet(vibrationDsp);            

            if(sensorLimit > 0)
                statusWord.flags.sensorLimit    = ON;
            else
                statusWord.flags.sensorLimit    = OFF;

            if(actuatorLimit > 0)
                statusWord.flags.actuatorLimit  = ON;
            else
                statusWord.flags.actuatorLimit  = OFF;

            if(thermalLimit > 0)
                statusWord.flags.thermalLimit   = ON;
            else
                statusWord.flags.thermalLimit   = OFF;

            if(mirrorDsp > 0)
                statusWord.flags.mirrorDspInt   = ON;
            else
                statusWord.flags.mirrorDspInt   = OFF;

            if(vibrationDsp > 0)
                statusWord.flags.vibDspInt  = ON;
            else
                statusWord.flags.vibDspInt  = OFF;

            if((sensorLimit + actuatorLimit + thermalLimit + mirrorDsp + vibrationDsp) > 0)
                statusWord.flags.health = 1;
            else
                statusWord.flags.health = 0;

            /* write current status back to buffer */

            epicsMutexLock(m2MemFree);
                m2Ptr->page1.statusWord.all = statusWord.all;
                m2Ptr->page1.heartbeat      = m2Heartbeat;
                m2Ptr->page1.deployBaffle   = m2Deployable;
                m2Ptr->page1.centralBaffle  = m2Central;
                m2Ptr->page1.enclosureTemp  = temperature;  
                m2Ptr->page1.topEnd     = topEnd;
            epicsMutexUnlock(m2MemFree);

        }   state waitUpdate
    }
}


























 
 


















