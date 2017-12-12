/* ===================================================================== */
/*+
 *
 * FILENAME
 * -------- 
 * eventBus.c 
 * 
 * PURPOSE
 * -------
 * Event Bus related routines.
 * 
 * FUNCTION NAME(S)
 * ----------------
 * 
 * DEPENDENCIES
 * ------------
 *
 * LIMITATIONS
 * -----------
 * DON'T TURN DEBUG LEVEL = DEBUG_RESERVED1 ON AT CHOP FREQUENCIES OF
 * > 1 Hz since the amount printed out during each chop cycle is great
 * enough to impact the processing of interrupts. 
 * 
 * AUTHOR
 * ------
 * Dayle Kotturi and Jim Wright 
 * 
 * HISTORY
 * -------
 * 10-Jun-2000: Original (based on chopControl.c with modifications to
 *              ISR, interrupt handlers and low-level h/w test routines
 * 12-Jun-2000: Add new "xyIntHandler" routine to replace the Xycom240
 *              interrupt handling formerly done by runChop
 * 12-Jun-2000: Add new "eventHandler" routine to run as a new task
 * 06-Dec-2017: Begin EPICS OSI conversion (mdw)
 *              Copied from xycom.c and deleted all xycom driver related code
 * 12-Dec-2017: Major reworking of interaction with xycom card in order to use the
                Common Code Base version of the xycom driver module. (mdw)
 */
/* ===================================================================== */

#include <math.h>               /* For round */
#include <timeLib.h>            /* For timeNow */
#include <drvXy240.h>           /* for xy240 driver stuff */

#include "utilities.h"
#include "chop.h"               /* For getChopDuty, getChopFreq, chopIsPending,
               decsIsPending */
#include "chopControl.h"        /* For chopEventSem */
#include "control.h"            /* For simLevel, updateEventPage, scsPtr, flip */

#define XYCARDNUM 0             /* we're only using xycom240 card number 0 */


/* Interrupt mask values and variables */
#define STROBES_MASK 0x1f    /* 00011111 */
#define M2POWERON   0x1     /* 00000001 */
#define M2POWEROFF  0xfe    /* 11111110 */
#define M2SERVOON   0x2     /* 00000010 */
#define M2SERVOOFF  0xfd    /* 11111101 */
#define INPOSTIMEOUT    3

#define M2POWER_BIT 0
#define M2SERVO_BIT 1
#define M2_BIT      5

#define SET   1
#define RESET 0

#define M2INPOS_STATE_INPUT_MASK   0x08   /* 00001000 */

#define ICS0_INT_MASK               0x01    /* 00000001 (32) */
#define ICS1_INT_MASK               0x02    /* 00000010 (32) */
#define ICS2_INT_MASK               0x04    /* 00000100 (32) */
#define ICS3_INT_MASK               0x08    /* 00001000 (32) */
#define ICS4_INT_MASK               0x10    /* 00010000 (32) */
#define M2SYNCIN_INT_MASK           0x20    /* 00100000 (32) */
#define M2INPOSITION_INT_MASK       0x40    /* 01000000 (32) */
#define FREE_INT_MASK               0x80    /* 10000000 (32) */


/* Note: static globals DO NOT end up in the VxWorks symbol table, so
 * you can't look at them via command line. Taking away "static" keyword
 * puts them in the symbol table */

static unsigned char    ICSmasks[6] = {0, 1, 2, 4, 8, 16};
unsigned char    currentICSMask = 0;
unsigned char    currentMask = 0;

/* Local beam-related variables */

static int     beamSeq[4] = {BEAMA, BEAMB, BEAMA, BEAMC};

int     demandedBeam = BEAMA;
static int     beamIndex = 0;
static int     numBeams = 0;
static int     maxBeam = 0;
static int     beamProfile = 0;

double  timeToWait; 
double intervalIT;    /* to fix bug extra interrupts */

/* Local device variables */

static int     controller_A = 0;
static int     ICScapability = ONESHOT;
static int     firstTransition = FALSE;
int     scsInPos = FALSE;
int     presentBeam;
int     badBeams1;
int     badBeams2;
int     missedInterrupts;
int     interruptCounter = 0;
int    counter = 0;

/* Event system initial configuration params */

configStructure eventConfig =
{
    0,                 /* set to 1 for change */
    0,                 /* set to 1 if chopping OFF or ON changes */
    OFF,               /* chopping { OFF | ON } */
    TWOPOINT,          /* profile { TWOPOINT | THREEPOINT | TRIANGLE } */
    LMMS,              /* chop source { LMMS | INST } */
    0,                 /* controlling instrument { SCS | .. | ICS4 } */
    STROBE,            /* capability */
    OFF,               /* m2 power */
    OFF,               /* m2 servo */
};

/* static global variables are automatically initialized to zeroes */
static epicsEventId ICS_eventSem;        /* ICS event semaphore */
static epicsEventId inposition_eventSem; /* In Position event semaphore */
static unsigned char intRead;            /* what we read during interrupt */
static int eventBusInited;               /* have we initialized the board? */
static int icsTid;                       /* task id for ics event watcher thread */
static int inposTid;                     /* task id for inPosition watcher thread */
static int icsThreadExit;                /* flag to tell ics thread to exit */
static int inposThreadExit;              /* flag to tell inpos Thread to exit */

/* function prototypes */
static void    updateEventSys (void);
static int     readDemand (void);
int            xyIntON (void);
int            xyIntOFF (void);
int            xySIE (int);


void clearInterruptCounts(void) {
   interruptCounter = 0;
}

void showInterruptCounts(void) {
   epicsPrintf("Total InPosition Interrupts Counted: %d\n", interruptCounter);
}

/***
*
* Function name:
* inPositionHandler
*
* Purpose:
*
* Handle the 'InPosition' TTL signal. This signal is conditioned from its
* origin at the CEM. The raw 'InPosition' signal decides when M2 is 'ON_BEAM'
* and ( logical &) the servo position error is within tolerance. 
*
* This is carried out by servicing interrupts which can signal a transition
* demand from an ICS, an imminent transition of the M2, or by M2 reaching its
* demanded position after a transition.  Information is provided to other
* systems by means of the Gemini Event System and the Gemini Synchro Bus. 
*
*
*
*/
void inPosition_eventHandler(void *p) {

   for(;;) {
      epicsEventWait(inposition_eventSem);

      interruptCounter++;

      /*Check whether we're interrupting on the rising or 
        falling edge of IN_POSITION. State is captured on JK1/32 */

      if ( (xy240_readPortByte(XYCARDNUM, PORT2) >> 4) & M2INPOS_STATE_INPUT_MASK ) { 
         scsInPos = epicsTrue;
      }

      else {
         scsInPos = epicsFalse;
      }

      updateEventPage (scsInPos, presentBeam);

      /* Report status back to ICSs via flag output registers */
      updateEventSys (); 
   }

}


/* ===================================================================== */
/*
 * Function name:
 * ICS_eventHandler
 * 
 * 
 * Purpose:
 * Handle and synchronise the flow of chop transition information
 * and requests between the SCS, the M2 hardware and controlling
 * Instrument Control Systems.
 * This is carried out by servicing interrupts which can signal
 * a transition demand from an ICS, an imminent transition of the
 * M2, or by M2 reaching its demanded position after a transition.
 * Information is provided to other systems by means of the Gemini
 * Event System and the Gemini Synchro Bus.
 *
 * Invocation:
 * spawnTask ... xyIntHandler
 * 
 * Parameters in:
 * 
 * Parameters out:
 * 
 * Return value:
 * 
 * Globals: 
 * 
 * External functions:
 * None
 * 
 * External variables:
 * None
 * 
 * Requirements:
 * 
 * Author:
 * 
 * History:
 * 12-Jun-2000: original
 * 
 */

/* ===================================================================== */

/*
 * This routine is used with the interrupt service routine. It waits on a 
 * semaphore,
 * and prints a message when the semaphone is released.  This function
 * is spawned as a separate task by xySetupInterrupt() so that we
 * still have the command line
 */
void ICS_eventHandler (void *p) {

   /* Fix pb extra interrupt: we know how much time we should have between
      two chop transitions or two interrupts, then reject all interrupts which
      arrive in between */

   int m2PresentBeam;
   int inSync = 0;
   int chopIsPendingLocal = -1;

   /* default settings for chopping */
   numBeams = 2;
   maxBeam = 1;
   presentBeam = BEAMB; /* we start on A; so the "expected next beam" is B
                           initially */
   beamIndex = 1;
   badBeams1 = 0;    /* the number of times ICS doesn't agree with SCS */
   badBeams2 = 0;    /* the number of times M2 doesn't agree with M2 */
   missedInterrupts = 0;
   counter = 0;
   chopIsPendingLocal = -1;


   if (debugLevel == DEBUG_RESERVED1)
      errlogMessage("watching forever for interrupt...\n");

   for (;;) {
      counter++; 

      /* Wait for a semaphore indicating an interrupt from the
       * Xycom. */

      epicsEventWait(ICS_eventSem);


      /* If all is well, this is the NEXT successive interrupt (and
       * so interruptCounter is 1. Even if this is not the case, but 
       * the number of interrupts arriving is an odd number, proceed
       * as if all is well.
       *
       * Assumption: Two position chopping. This wouldn't work for three.
       */

      /* Else the number of interrupts that have arrived is EVEN
       * and so we risk going out of phase for a two position chop.
       * So throw this interrupt away (i.e. do nothing), clear the
       * counter and wait for the next one.
       */

      /* NOTE: Even with external sync
       * source, the M2 sends an interrupt matching the M2SYNCIN_MASK.
       * In the case that we are chopping with external sync,
       * we don't want to execute the code for the internal sync, 
       * so check currentICSMask value 
       */

      if (intRead & currentICSMask) { 

         /* scsInPos = FALSE; */
         /* The ICS is capable of demanding a particular beam */

         demandedBeam = readDemand();

         /* Error checking on demanded beam */ 
         if (presentBeam != demandedBeam)
         {
            badBeams1++;
            /*   logMsg("count:%d SCS:%d ICS:%d\n", 
                 counter, presentBeam, demandedBeam, 0, 0, 0); */
            if (inSync && chopIsOn) 
            {
               chopIsPendingLocal = -1;
               counter=0;
               badBeams1=0;
               badBeams2=0;
               inSync=0;
               presentBeam = BEAMB; 
               beamIndex = 1;
            }
            else
               continue;
         }
         else
         {
            /* if ( counter < 20 )
               {
               logMsg("* count:%d SCS:%d ICS:%d\n", 
               counter, presentBeam, demandedBeam, 0, 0, 0); 
               } */
         }

         /* To get to here, we have met the condition that the
          * SCS internal beam counter, "presentBeam" agrees with
          * the ICS demanded beam */

         /* Turn chop and decs on in M2 now */

         if (chopIsPending)
         {
            writeCommand(CHOP_ON);
            chopIsOn = 1;
            chopIsPending = 0;
            decsIsPending = 0;
            chopIsPendingLocal = 0;
         }

         /* Only one more condition to meet before we can chop...
          * We need to ensure that the M2 is ready */

         if (!m2ChopResponseOK)
         {
            /*  logMsg("* count:%d !m2ChopResponseOK \n", 
                counter, 0, 0, 0, 0, 0); */
            continue; 
         }

         /* OK; we've established the beam pattern for the
          * event system. Post the OUT OF POSITION and the newly
          * determined beam to global structure so that 
          * procGuides task can read it */

         /* updateEventPage (scsInPos, presentBeam); */

         /* Report status back to ICSs via flag output registers */

        /*  updateEventSys (); */


          /* pulse is high when 5th bit set */
         xy240_writeFlagBit(XYCARDNUM, M2_BIT, epicsTrue);

         epicsThreadSleep(0.01); 
         /* Assuming 200 Hz clock:                       */
         /* 1) get to next tick edge - this takes 0-5 ms */
         /* 2) then wait a full tick - this takes 5 ms   */

         /* pulse is low when 5th bit clr */
         xy240_writeFlagBit(XYCARDNUM,M2_BIT, epicsFalse);


         if ((epicsEventWaitWithTimeout(ICS_eventSem, timeToWait ) == epicsEventTimeout)) 
         {
            /* printf("timeout reached \n"); */
            /* timeout reached, we waited long enough for it
             * to get there, and so we ASSUME it is */
         }

         /* Broadcast to the ICSs again.  */
         /* Note: scsInPos is TRUE after x ticks, no matter what. */

         /* scsInPos = TRUE; */

         /* Finaly, we don't want to do it for focus - 14 dec 2000 */

         if ( guideOn)
         {
            controller[XTILT].sum = 0.0;
            controller[YTILT].sum = 0.0;
            controller[XTILT].oldSum = 0.0;
            controller[YTILT].oldSum = 0.0;
            controller[XTILT].oldError = 0.0;
            controller[YTILT].oldError = 0.0;
         }

         /* Report status back to ICSs via flag output registers */

         /*         updateEventSys ();   */

         /* Common code for all sync sources: */
         /* 1. broadcast current state */

          /* updateEventPage (scsInPos, presentBeam); */

         /* 2. update for next time through, UNLESS we are still waiting
          *    for the ICS and SCS's expected beam to agree. I.e. if they
          *    don't agree, DON'T change "presentBeam". */

         /* Compare with what M2 knows */

         m2PresentBeam = (int)(scsPtr->page1.beamPosition);

         if (m2PresentBeam != presentBeam)
         {
            badBeams2++;    /* keep count */
            /*  logMsg("count:%d SCS:%d M2:%d\n", 
                counter, presentBeam, m2PresentBeam, 0, 0, 0); */
            if (inSync && chopIsOn) 
            {
               chopIsPendingLocal = -1;
               counter=0;
               badBeams1=0;
               badBeams2=0;
               inSync=0;
               presentBeam = BEAMB; 
               beamIndex = 1;
            }
            else
               continue;
         }
         else
         {
            /* if ( counter < 20 )
               logMsg("* count:%d SCS:%d M2:%d\n", 
               counter, presentBeam, m2PresentBeam, 0, 0, 0); */
            inSync = 1;
            chopIsPendingLocal = 0;
         }

         if (!chopIsPendingLocal)
         {
            beamIndex = (++beamIndex) % numBeams;
            presentBeam = beamSeq[beamIndex]; 
            /*  if ( counter < 20 )
                logMsg("* count:%d !chopIsPendingLocal\n", 
                counter, 0, 0, 0, 0, 0);  */
         }
      }

   } /* end forever loop */
} 

/* ===================================================================== */
/*
 * Function name:
 * eventHandler
 * 
 * 
 * Purpose:
 * Routine to process changes to chop configuration and other TTL
 * signal changes.
 *
 * Invocation:
 * epicsThreadCreate ... eventHandler
 * 
 * Parameters in:
 * 
 * Parameters out:
 * 
 * Return value:
 * 
 * Globals: 
 * 
 * External functions:
 * None
 * 
 * External variables:
 * None
 * 
 * Requirements:
 * 
 * Author:
 * 
 * History:
 * 12-Jun-2000: original
 * 
 */

/* INDENT ON */
/* ===================================================================== */
void eventHandler (void *p) 
{
   double dutyCycle;
   double frequency;

   epicsPrintf ("eventHandler():  watching for eventSem to be signalled...\n");

   for (;;)
   {

      /* Wait for a semaphore indicating an epicsEventSignal()  by scs_st.stpp 
       * two places: startInit and chopControlCADstart */
      epicsEventWait(eventSem);

      if (eventConfig.change == 1)
      {
         /* First of all, check what's controlling chop */
         /* logMsg("eventHandler - detects the following:\n", 0, 0, 0, 0, 0, 0); */

         if (eventConfig.drive == LMMS)
         {
            currentICSMask = M2SYNCIN_INT_MASK;
         }
         else
         {
            /* ICS in charge.  Which one, and what capability? */

            controller_A = eventConfig.source;
            if (controller_A < 0 || controller_A > 5)
               errlogMessage("eventHandler - 4 - presentBeam out of range\n");

            currentMask = (ICSmasks[controller_A] + M2INPOSITION_INT_MASK + M2SYNCIN_INT_MASK + FREE_INT_MASK);
            currentICSMask = ICSmasks[controller_A];

            /* Based on config data in Instrument data file */

            ICScapability = eventConfig.capability;
            beamProfile = eventConfig.profile;

            if (beamProfile == THREEPOINT)
               numBeams = 4;
            else
               numBeams = 2;

            errlogPrintf("\t\t- chop controlled by currentICSMask=0x%02x, (%d)\n", 
                  (int) currentICSMask, (int) currentICSMask); 

            /* If the ICS wants to start chopping, but
             * not to demand each transition, then it's a
             * ONESHOT capability. The firstTransition
             * flag is used to switch interrupt masks
             * after the first transition.  */

            if (ICScapability == ONESHOT)
               firstTransition = TRUE;
            else
               firstTransition = FALSE;
         }

         /* Check whether we're switching M2 power on|off */
         if (eventConfig.m2Power == ON)
         {  /* M2 Power on */
            xy240_writePortBit(XYCARDNUM, PORT7, M2POWER_BIT, SET);
         }
         else
         {  /* M2 Power off */
            xy240_writePortBit(XYCARDNUM, PORT7, M2POWER_BIT, RESET);
         }

         /* Check whether we're switching M2 servos on|off */
         if (eventConfig.m2Servo == ON)
         {  /* M2 Servos on */
            xy240_writePortBit(XYCARDNUM, PORT7, M2SERVO_BIT, SET);
         }
         else
         {  /* M2 Servos off */
            xy240_writePortBit(XYCARDNUM, PORT7, M2SERVO_BIT, RESET);
         }
         /* Check whether we're switching chop on or off */
         if ((eventConfig.on == 1) & (eventConfig.onChange == 1))
         {
            /* Clear then enable interrupts */

            if (xyIntON == ERROR)
            {
               errlogMessage("event Handler - error in xyIntON\n");
            }
            xy240_writeIMR(XYCARDNUM, currentMask);

            /* Hmmm - this is a move command - do we want to do this
             * here - to me, this has the potential to get the chop
             * one beam out of phase... why do we issue this here? 
             * Or maybe it is unnecessary? given that presentBeam is
             * set in xyIntHandler - i.e. maybe a chop starts with 
             * two demands to go to A
             *
             *flagOutReg = SET_M2_BIT;
             */

            /* Calculate time to wait for IN POSITION bit as a function
             * of dutyCycle and frequency */

            dutyCycle = getChopDuty();
            frequency = getChopFreq();
            if (frequency > 0.05) /* don't divide by close to zero */
            {
               /* Assuming a 200 Hz clock...
                * Take 1/2 of the period (1/f) since the frequency
                * is for an entire A - B cycle. Then take half the
                * duty cycle because it also applies to an entire
                * cycle. I.e, the dutyCycle is the percent of the
                * time that the beam is in position, in either BEAM
                * A or B. Divide by 0.005 to convert to ticks for
                * a 200 Hz clock.
                */
               timeToWait = 0.005 + ((1.0/frequency)/2.0 * ((100.0 - dutyCycle)/100.0)/2.0); 

               /* Take away 2 ticks (if there are this many since
                * 2 ticks are spent keeping the M2_BIT low 
                * for the M2 to latch the signal (see xyIntHandler).
                */

               if (timeToWait >= 0.01 ) 
                  timeToWait -= 0.01;
               else
                  timeToWait = 0.0;

               /* intervalIT = 80 % of interval between two chop 
                * transitions 
                */
               errlogPrintf("eventHandler(): timeToWait %d \n",timeToWait);

               intervalIT = ((1.0/frequency)/2.0) * 0.8 ;
            }
            else
               errlogPrintf("\t\t- invalid frequency = %f\n", frequency);

            /* Reset */
            eventConfig.onChange = 0;

         }
         else if ((eventConfig.on == 0) && (eventConfig.onChange == 1))
         {

            errlogPrintf("event Handler - badBeams (SCS-ICS)=%d (SCS-M2)=%d missed interrupts=%d counter=%d\n",
                  badBeams1, badBeams2, missedInterrupts, counter);

            /* Disable interrupts and clear registers */

            if (xyIntOFF == ERROR)
            {
               errlogMessage("eventHandler(): error in xyIntOFF\n");
            }
            xy240_writeIMR(XYCARDNUM, 0);

            /* I don't think this is necessary here. 
             *flagOutReg = CLR_M2_BIT;
             */

            /* Leave the green LED on after end of chopping */

            xy240_ledCtl(XYCARDNUM, XY240_GREEN_LED, SET);   

            /* Reset the beam to A and post to RM so that 
             * guiding can work. Always move to BEAMA */

            /*      if (presentBeam != BEAMA)
                    { */
            printf("eventHandler - setting presentBeam to BEAMA\n");
            presentBeam = BEAMA;
            /* } */


            /* 
            updateEventPage (scsInPos, BEAMB); 
            */

            /* 
               if (xyIntON() == ERROR)
               {
               logMsg ("event Handler - error in xyIntON\n", 0, 0, 0, 0, 0, 0);
               } 
               taskDelay(10);
               if (xyIntOFF() == ERROR)
               {
               logMsg ("eventHandler - error in xyIntOFF\n", 0, 0, 0, 0, 0, 0);
               } */

            /* Reset */
            eventConfig.onChange = 0;
         }

         eventConfig.change = 0;
      }
      else
      {
         /* Just a wake-up. Acknowledge */
         printf ("eventHandler - no change to eventConfig\n");
      }

   }
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * readDemand
 * 
 * Purpose:
 * On receipt of a valid interrupt from an instrument control system,
 * this function is invoked to read the beam demanded by the ICS.
 *
 * Note: testing 16-oct-2000, demonstrated that when mask = 0x0f is
 * used, top 3 SB are always HIGH. So demandedBeam has values of 14 and 15.
 * Changing logic to just look at LSB (i.e. mask = 0x01) is enough to
 * make a 2 position chop work, but would break things for a 3 position
 * chop and would ignore values if the spare bits were ever used. It
 * seems that what is showing up has been inverted. PKP confirms the
 * inversion is NOT happening via 74HCT365 chip (as anticipated), so will
 * need to trace signal to find where inversion occurs. In the meantime,
 * invert the contents and use all 4 bits.
 *
 * 20-oct2000 - CJC confirms the top 3SB are being dragged high by the
 * event node (not measured, just seen in diags) if no signal is attached.
 * So go back to the idea of just looking at the LSB and ignore other 3.
 * 
 * Invocation:
 * 
 * Parameters in:
 * 
 * Parameters out:
 * 
 * Return value:
 * 
 * Globals: 
 *
 * External functions:
 * None
 * 
 * External variables:
 * None
 * 
 * Requirements:
 * 
 * Author:
 * Magnus Paterson    (mjp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(mjp)
 * 13-Jun-2000: Change logic to do bit shift instead of divide by 16
 * 
 */

/* INDENT ON */
/* ===================================================================== */
static int     readDemand (void)
{
    /* Reads the demanded beam position from the port connected to the
     * current control ICS. The routine selects the demand from either
     * the most significant or least significant 4 bits of the
     * appropriate port and uses a bitmask to guard against unwanted 
     * bits creeping in.
     * 
     * If the routine is called with an invalid controller, then the bit
     * pattern is set to an invalid beam, and a logMsg error is recorded.
     * This should never be reached because of checks at higher levels of
     * the software. */

    int     demand = 0;      /* returned value */

    /* NOTE: this is incorrect if no signals on demand 1, spare 0 and spare 1 
     * as they are dragged high!!! 
    unsigned char    portMask = 0x0f;   bitmask */

    /* VALID only for 2 position chop, because it ignores demand 1 */
    unsigned char    portMask = 0x01;    /* bitmask */

    switch (controller_A)
    {

    case ICS0:
        /* set the demand to the LSB of the 4 LSB of the port */
        demand = xy240_readPortByte(XYCARDNUM, PORT0) & portMask;
        break;

    case ICS1:
        /* set the demand to the LSB of the 4 MSB of the port */
        demand = (xy240_readPortByte(XYCARDNUM, PORT0)>>4) & portMask;
        break;

    case ICS2:
        demand = xy240_readPortByte(XYCARDNUM, PORT1) & portMask;
        break;

    case ICS3:
        demand = (xy240_readPortByte(XYCARDNUM, PORT1)>>4) & portMask;
        break;

    case ICS4:
        demand = xy240_readPortByte(XYCARDNUM, PORT2) & portMask;
        break;

    default:
        demand = 3;
        errorLog ("readDemand - Attempt to read from invalid port", 1, ON);
    }

    /* Temporary - this is necessary when ICS is the sig gen, but we don't
     * want this for OSCIR. For now, if you want to change it, set flip
     * to 1 on the crate console                              21nov2000 */

    if (demand == 0) 
       demand = 1;
    else 
       if (demand == 1) 
          demand = 0;

    return demand;

}
/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * updateEventSys
 * 
 * Purpose:
 * Send beam and M2 position status information to all event system
 * nodes.
 * 
 * Invocation:
 * 
 * Parameters in:
 * 
 * Parameters out:
 * 
 * Return value:
 * 
 * Globals: 
 *
 * External functions:
 * None
 * 
 * External variables:
 * None
 * 
 * Requirements:
 * 
 * Author:
 * Magnus Paterson    (mjp@roe.ac.uk)
 * 
 * History:
 * 15-Oct-1997: Original(mjp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

static void    updateEventSys (void)
{
    int     posFlag;
    
    int port5, port6;

    /* Each output port in the xycom module has to talk to two channels
     * in the event system. Data to each channel are identical, and
     * occupy the upper and lower 4 bits of each port.  The two LSBs in
     * each nibble represent the current beam, and the next bit is the in
     * position flag.  
     * Port 6 only has to handle the lower 4 bits, so we deal with that
     * first, and copy the pattern as required. */

    if (scsInPos == TRUE)
        posFlag = 4;
    else
        posFlag = 0;
    
    port6 = presentBeam + posFlag;
    port5 = (port6 << 4) + port6;
    xy240_writePortByte(XYCARDNUM, PORT6, port6);
    xy240_writePortByte(XYCARDNUM, PORT5, port5);
    xy240_writePortByte(XYCARDNUM, PORT4, port5);

    /* Now pulse the FOR lines used to strobe the data into the event
     * system hub */

    xy240_setResetFlagBits(XYCARDNUM, STROBES_MASK, SET);

    /* Allow time for the electronics to latch the signal, before
     * resetting pulse.  10 us delay.  
     * Note:  At a clock rate of 200 ticks per second, 
              we don't have anywhere near this kind of resolution */
    epicsThreadSleep(0.00001); /* will result in one clock tick delay */

    /* reset pulse */

    xy240_setResetFlagBits(XYCARDNUM, STROBES_MASK, RESET);
}


/*****************************************************************************/
static void isr (int val)
{
   /* volatile  char *intClr = (char *)INTERRUPT_CLEAR_REG_ADDR; */

   /* intRead = *intPendReg; */

   /*  logMsg ("IIR: = 0x%02x\n", *intInReg, 0, 0, 0, 0, 0);  
       logMsg ("IMR: = 0x%02x (%d)\n", *intMaskReg, *intMaskReg, 0, 0, 0, 0);
       logMsg ("SCR: = 0x%02x (%d)\n", *statConReg, *statConReg, 0, 0, 0, 0);
    */

   if ( *intPendReg & currentICSMask)  {

      intRead = *intPendReg;      /* store the pend register contents. 
                                  * Use pend instead of in reg, because
                                  * it is after the mask */

      *intClrReg = currentICSMask;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg("Interrupt on currentICSMask: %02x \n\n", currentICSMask, 0, 0, 0, 0, 0);
      semGive (ICS_eventSem); /* release the semaphore */
   }

   else if ( *intPendReg & ICS0_INT_MASK)  {

      *intClrReg = ICS0_INT_MASK;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg("Interrupt on ICS0_INT_MASK: %02x \n\n", ICS0_INT_MASK, 0, 0, 0, 0, 0);
   }

   else if ( *intPendReg & ICS1_INT_MASK)  {

      *intClrReg = ICS1_INT_MASK;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg("Interrupt on ICS1_INT_MASK: %02x \n\n", ICS1_INT_MASK, 0, 0, 0, 0, 0);
   }

   else if ( *intPendReg & ICS2_INT_MASK)  {

      *intClrReg = ICS2_INT_MASK;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg("Interrupt on ICS2_INT_MASK: %02x \n\n", ICS2_INT_MASK, 0, 0, 0, 0, 0);
   }

   else if ( *intPendReg & ICS3_INT_MASK)  {

      *intClrReg = ICS3_INT_MASK;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg("Interrupt on ICS3_INT_MASK: %02x \n\n", ICS3_INT_MASK, 0, 0, 0, 0, 0);
   }

   else if ( *intPendReg & ICS4_INT_MASK)  {

      *intClrReg = ICS4_INT_MASK;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg("Interrupt on ICS4_INT_MASK: %02x \n\n", ICS4_INT_MASK, 0, 0, 0, 0, 0);
   }

   if ( *intPendReg & M2INPOSITION_INT_MASK) {

      *intClrReg = M2INPOSITION_INT_MASK;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg ("Inposition interrupted\n\n", 0, 0, 0, 0, 0, 0);
      semGive (inposition_eventSem);
   }

   if ( *intPendReg & M2SYNCIN_INT_MASK) {

      *intClrReg = M2SYNCIN_INT_MASK;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg ("M2SyncIn interrupted with no action!\n\n", 0, 0, 0, 0, 0, 0);
   }

   if ( *intPendReg & FREE_INT_MASK) {

      *intClrReg = FREE_INT_MASK;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg ("FreeInterrupt interrupted with no action!\n\n", 0, 0, 0, 0, 0, 0);
   }


   /* *intClrReg = intRead; */    /* clear the bits we just read */
   /* *intClrReg = 0xFF; */    /* clear all bits */
   /* *intClrReg = 0x02; */    /* clear the bits we just read */
   /* *statConReg ^= SC_RED_LED; */    /* toggle the red LED */
   /* logMsg("isr = intRead %d \n", intRead, 0, 0, 0, 0, 0);  */

   /* *intClrReg = *intPendReg;  */   /* clear the bits we just read */
   /* interruptCounter++; */

   /* *(char *) intClr = 0xFF; */ /* clear everything */
   /* *intClrReg = 0xFF; */ /* clear the bits we just read */
}

/*****************************************************************************/
long getSyncMask(void)
{
    return (long)currentICSMask;
}

/* This is our routine to handle xycom240 interrup requests */
static void eventBusIntHandler(int dummy)
{
   int ipr=xy240_readIPR(XYCARDNUM);

   if(ipr & currentICSMask){
      intRead = ipr;
      epicsEventSignal(ICS_eventSem);
   }

   if(ipr & M2INPOSITION_INT_MASK)
      epicsEventSignal(inposition_eventSem);
}

/*****************************************************************************/
/* turn on handling of events detected by the xycom 240 card */
int xyEventHandlingON(void)
{
    int status;

    xyInited = FALSE;   
    xyInit();

    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    /* careful not to run it twice */
    if (icsTid) {
       errlogPrintf("xyEventHandlingON(): ics thread %p is  already running\n" icsTid);
       return ERROR;
    }

    /* careful not to run it twice */
    if (inposTid) {
       errlogPrintf("xyEventHandlingON(): inpos thread %p is  already running\n" inposTid);
       return ERROR;
    }

    /* spawn the task which will keep an eye on our ICS events */
    icsThreadExit = 0;
    if(!(icsTid=epicsThreadCreate("icsEventWatcher", epicsThreadPriorityHigh,
                epicsThreadGetStackSize(epicsThreadStackBig), 
                (EPICSTHREADFUNC)ICS_eventHandler, (void *)NULL))) {
       errlogMessage("xyEventHandlingON(): Cannot create ICS event watcher thread\n");
       return ERROR;
    }

    /* spawn the task which will keep an eye on our inPositon  events */
    inposThreadExit = 0;
    if(!(icsTid=epicsThreadCreate("inposEventWatcher", epicsThreadPriorityHigh,
                epicsThreadGetStackSize(epicsThreadStackBig), 
                (EPICSTHREADFUNC)inPosition_eventHandler, (void *)NULL))) {
       errlogMessage("xyEventHandlingON(): Cannot create inPosition event watcher thread\n");
       return ERROR;
    }

    if(status=xy240_intConnect(XYCARDNUM, XY240_ANY_IRQ, eventBusIntHandler)) {
       errlogMessage("xyEventHandlingON(): Can not install event bus handler interrupt service routine\n");
       return status;
    }

    return OK;
}



/*****************************************************************************/
int xyIntOFF (void)
{
   int status;

   if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

   /* check if something is going
    */
   if (icsTid == ERROR) {
      printf ("It appears the interrupt tasks are not running\n");
      return ERROR;
   }

   /* check if something is going
    */
   if (inposTid == ERROR) {
      printf ("It appears the interrupt tasks are not running\n");
      return ERROR;
   }

   /* turn off interrupt capability on the xycom
    */
   *statConReg &= ~SC_INTERRUPT_ENABLE; 

   /* turn off the red led
    */
   *statConReg |= SC_RED_LED; 

   /* remove the interrupt watcher task
    */
   if ((status = taskDelete (icsTid)) != OK) {
      printf ("Can not delete interrupt watcher task\n");
      return status;
   }

   /* remove the interrupt watcher task
    */
   if ((status = taskDelete (inposTid)) != OK) {
      printf ("Can not delete interrupt watcher task\n");
      return status;
   }

   /* set to special value so we know there is no helper task running
    */
   inposTid = ERROR;
   icsTid = ERROR;

   semDelete(ICS_eventSem);
   semDelete(inposition_eventSem);

   return OK;
}

/*****************************************************************************/
int eventBusInit(void)
{
    int status;
    char dummy = '\0';

    if (eventBusInited)
    {
        errlogMessage("eventBusInit(): Event Bus is already initialized\n");
        return OK;
    }

    /* check that XYCOM board is present */
    if(status = xy240_status(XYCARDNUM)) {
       errlogPrintf("eventBusInit(): xy240 card %d not installed or not initialized\n",
                      XYCARDNUM);
       return status;
    }


    /* create the event semaphores */
    ICS_eventSem = epicsEventMustCreate(epicsEventEmpty);
    inposition_eventSem = epicsEventMustCreate(epicsEventEmpty);

    /* done */
    eventBusInited = TRUE;
    return OK;
}

