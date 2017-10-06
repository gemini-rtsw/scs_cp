/* $Id: xycom.c,v 1.12 2008/04/16 23:23:48 mrippa Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * xycom.c 
 * 
 * PURPOSE
 * -------
 * Xycom XY240 board related routines.
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
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include <vxWorks.h>		/* General vxworks stuff */
#include <intLib.h>		/* For intConnect */
#include <iv.h>			/* For interrupt vector stuff */
#include <logLib.h>		/* For logMsg */
#include <math.h>               /* For round */
#include <msgQLib.h>		/* For message queues */
#include <semLib.h>		/* For semaphores */
#include <stdio.h>		/* printf and friends */
#include <sysLib.h>		/* For sysXXX */
#include <taskLib.h>		/* Spawning and deleting tasks */
#include <tickLib.h>            /* For tickGet */
#include <timeLib.h>            /* For timeNow */
#include <vxLib.h>		/* For vxMemProbe */

#include "xycom.h"		
#include "chop.h"               /* For getChopDuty, getChopFreq, chopIsPending,
				   decsIsPending */
#include "chopControl.h"        /* For chopEventSem */
#include "control.h"            /* For simLevel, updateEventPage, scsPtr, flip */

/* Interrupt mask values and variables */

#define SET_STROBES 0x1f    /* 00011111 */
#define CLR_STROBES 0xe0    /* 11100000 */
#define CLR_M2_BIT  0xdf    /* 11011111 */
#define SET_M2_BIT  0x20    /* 00100000 */
#define M2POWERON   0x1     /* 00000001 */
#define M2POWEROFF  0xfe    /* 11111110 */
#define M2SERVOON   0x2     /* 00000010 */
#define M2SERVOOFF  0xfd    /* 11111101 */
#define INPOSTIMEOUT    3


#define M2INPOS_STATE_INPUT_MASK	0x08	/* 00001000 */

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

int ticksToWait; 

double intervalIT; /* to fix bug extra interrupts */

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
int 	counter = 0;

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

/* Note: this must be volatile, so that not only the CPU can write
 * into these memory addresses */

/* pointers to all the registers */
static volatile unsigned char *intInReg
             = (unsigned char *)INTERRUPT_INPUTS_REG_ADDR;

static volatile unsigned char *statConReg
             = (unsigned char *)STATUS_CONTROL_REG_ADDR;

static volatile unsigned char *intPendReg
             = (unsigned char *)INTERRUPT_PENDING_REG_ADDR;

static volatile unsigned char *intMaskReg
             = (unsigned char *)INTERRUPT_MASK_REG_ADDR;

static volatile unsigned char *intClrReg
             = (unsigned char *)INTERRUPT_CLEAR_REG_ADDR;

static volatile unsigned char *intVecReg
             = (unsigned char *)INTERRUPT_VECTOR_REG_ADDR;

static volatile unsigned char *flagOutReg
             = (unsigned char *)FLAG_OUTPUTS_REG_ADDR;

static volatile unsigned char *portDirReg
             = (unsigned char *)PORT_DIRECTION_REG_ADDR;

/* pointers to data ports on xycom 240 (initialize array below) */
static volatile unsigned char *ports[8];

SEM_ID xySem;			/* semaphore */
SEM_ID xyInpositionSem;			/* semaphore */
static unsigned char intRead = 0; 	/* what we read during interrupt */
static int xyInited = FALSE;		/* have we initialized the board? */
static int icsTid = ERROR;			/* task id for interrupt helper */
static int inposTid = ERROR;			/* task id for interrupt helper */

/* function prototypes */
static void    updateEventSys (void);
static int     readDemand (void);
int            xyIntON (void);
int            xyIntOFF (void);
int            xySIE (int);

/*****************************************************************************/
int xyInit (void)
{
    int status;
    char dummy = '\0';

    if (xyInited)
    {
        printf ("Xycom is already initialised\n");
	return OK;
    }

    /* check that XYCOM board is present */
    if ((status = vxMemProbe ((void *)XYCOM_BASE, READ, sizeof (char), &dummy)) != OK) {
	printf ("xycom card not detected\n");
	return status;
    }

    /* the array of addresses for the 8 ports on the card */
    ports[0] = (unsigned char *)PORT_0_ADDR;
    ports[1] = (unsigned char *)PORT_1_ADDR;
    ports[2] = (unsigned char *)PORT_2_ADDR;
    ports[3] = (unsigned char *)PORT_3_ADDR;
    ports[4] = (unsigned char *)PORT_4_ADDR;
    ports[5] = (unsigned char *)PORT_5_ADDR;
    ports[6] = (unsigned char *)PORT_6_ADDR;
    ports[7] = (unsigned char *)PORT_7_ADDR;

    /* Green LED on, red off, clear all other status/config bits */
    *statConReg = SC_RED_LED | SC_GREEN_LED;

    /* create the semaphore */
    if ((xySem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL) {
	printf ("unable to create xySem sem\n");
	return ERROR;
    }

    /* create the semaphore */
    if (( xyInpositionSem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == NULL) {
	printf ("unable to create xySem sem\n");
	return ERROR;
    }

    /* done */
    xyInited = TRUE;
    return OK;
}

/*****************************************************************************/
int xyPrintID (void)
{
    int i;
    char *ptr;

    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    printf ("Board ID:\n");
    printf ("   ID PROM: ");
    for (i = 0x01; i < 0x0b; i += 2) {
	ptr = (char *)(XYCOM_BASE + i);
	printf ("%c", (char)(*ptr));
    }
    printf ("   Manufacturer: ");
    for (i = 0x0b; i < 0x11; i += 2) {
	ptr = (char *)(XYCOM_BASE + i);
	printf ("%c", (char)(*ptr));
    }
    printf ("   Model: ");
    for (i = 0x11; i < 0x1f; i += 2) {
	ptr = (char *)(XYCOM_BASE + i);
	printf ("%c", (char)(*ptr));
    }
    printf ("\n");

    ptr = (char *)(XYCOM_BASE + 0x1f);
    printf ("Number of 1KByte blocks of I/O space on this board: %c\n", (char)(*ptr));

    printf ("Board revision = ");
    for (i = 0x21; i < 0x25; i += 2) {
	ptr = (char *)(XYCOM_BASE + i);
	printf ("%c", (char)(*ptr));
    }
    printf (".");
    for (i = 0x25; i < 0x29; i += 2) {
	ptr = (char *)(XYCOM_BASE + i);
	printf ("%c", (char)(*ptr));
    }
    printf ("\n");
    return OK;
}

void clearInterruptCounts(void) {
   interruptCounter = 0;
}

void showInterruptCounts(void) {
   printf("Total InPosition Interrupts Counted: %d\n", interruptCounter);
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
void inPositionHandler(void) {

   for(;;) {
      semTake (xyInpositionSem, WAIT_FOREVER);

      interruptCounter++;

      /*Check whether we're interrupting on the rising or 
        falling edge of IN_POSITION. State is captured on JK1/32 */

      if ( (*(ports[2]) >> 4) & M2INPOS_STATE_INPUT_MASK ) { 
         /* printf("Inpos: high\n"); */
         scsInPos = TRUE;
      }

      else {
        /*  printf("Inpos: Low\n"); */
         scsInPos = FALSE;
      }

      updateEventPage (scsInPos, presentBeam);

      /* Report status back to ICSs via flag output registers */
      updateEventSys (); 
   }

}


/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * xyIntHandler
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

/* INDENT ON */
/* ===================================================================== */

/*
 * This routine is used with the interrupt service routine. It waits on a 
 * semaphore,
 * and prints a message when the semaphone is released.  This function
 * is spawned as a separate task by xySetupInterrupt() so that we
 * still have the command line
 */
void xyIntHandler (void) {

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
      logMsg ("watching forever for interrupt...\n", 0, 0, 0, 0, 0, 0);

   for (;;) {
      counter++; 

      /* Wait for a semaphore indicating an interrupt from the
       * Xycom. */

      semTake (xySem, WAIT_FOREVER);


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

         /* Only one more condition to meet before was can chop...
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


         *flagOutReg |= SET_M2_BIT;   /* pulse is high when 5th bit set */

         taskDelay(2); /* Assuming 200 Hz clock:                       */
         /* 1) get to next tick edge - this takes 0-5 ms */
         /* 2) then wait a full tick - this takes 5 ms   */

         *flagOutReg &= CLR_M2_BIT;   /* pulse is low when 5th bit clr */


         if ((semTake (xySem, ticksToWait) == ERROR)) 
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
/* INDENT OFF */
/*
 * Function name:
 * eventHandler
 * 
 * 
 * Purpose:
 * New routine to process changes to chop configuration and other TTL
 * signal changes. Frees up xyIntHandler to just process the Xycom 
 * interrupts. 
 *
 * Invocation:
 * spawnTask ... eventHandler
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
void eventHandler (void) 
{
   double dutyCycle;
   double frequency;

   printf ("eventHandler - watching for eventSem to be given...\n");

   for (;;)
   {
      semTake(eventSem, WAIT_FOREVER);

      /* Wait for a semaphore indicating a semGive by scs_st.stpp 
       * two places: startInit and chopControlCADstart */

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
               logMsg ("eventHandler - 4 - presentBeam out of range\n", 0, 0, 0, 0, 0, 0);

            currentMask = (ICSmasks[controller_A] + M2INPOSITION_INT_MASK + M2SYNCIN_INT_MASK + FREE_INT_MASK);
            currentICSMask = ICSmasks[controller_A];

            /* Based on config data in Instrument data file */

            ICScapability = eventConfig.capability;
            beamProfile = eventConfig.profile;

            if (beamProfile == THREEPOINT)
               numBeams = 4;
            else
               numBeams = 2;

            logMsg ("\t\t- chop controlled by currentICSMask=0x%02x, (%d)\n", 
                  (int) currentICSMask, (int) currentICSMask, 0, 0, 0, 0); 

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
         {
            *(ports[7]) |= M2POWERON; 
         }
         else
         {
            *(ports[7]) &= M2POWEROFF; 
         }

         /* Check whether we're switching M2 servos on|off */
         if (eventConfig.m2Servo == ON)
         {
            *(ports[7]) |= M2SERVOON; 
         }
         else
         {
            *(ports[7]) &= M2SERVOOFF; 
         }
         /* Check whether we're switching chop on or off */
         if ((eventConfig.on == 1) & (eventConfig.onChange == 1))
         {
            /* Clear then enable interrupts */

            if (xyIntON() == ERROR)
            {
               logMsg ("event Handler - error in xyIntON\n", 0, 0, 0, 0, 0, 0);
            }
            *intMaskReg = currentMask; 
            xySIE( INTERRUPT_LEVEL ); 

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
               ticksToWait = 1 + 
                  (int)((((1./frequency)/2. * ((100. - dutyCycle)/100.)/2.)/0.005)); 
               /* (int)round((((1./frequency)/2. * ((100. - dutyCycle)/100.)/2.)/0.005)); */
               /* logMsg ("\t\t- freq = %d; dutyCycle = %d; ticksToWait = %d\n", 
                  (int)frequency, (int)dutyCycle, ticksToWait, 0, 0, 0); */			

               /* Take away 2 ticks (if there are this many since
                * 2 ticks are spent keeping the M2_BIT low 
                * for the M2 to latch the signal (see xyIntHandler).
                */

               if (ticksToWait >=2 ) 
                  ticksToWait -= 2;
               else
                  ticksToWait = 0;

               /* intervalIT = 80 % of interval between two chop 
                * transitions 
                */
               printf("eventHandler : ticksToWait %d \n",ticksToWait);

               intervalIT = ((1./(double)frequency)/2.0) * 0.8 ;
            }
            else
               logMsg ("\t\t- invalid frequency = %d\n", 
                     (int)frequency, 0, 0, 0, 0, 0);

            /* Reset */
            eventConfig.onChange = 0;

         }
         else if ((eventConfig.on == 0) && (eventConfig.onChange == 1))
         {

            logMsg ("event Handler - badBeams (SCS-ICS)=%d (SCS-M2)=%d missed interrupts=%d counter=%d\n",
                  badBeams1, badBeams2, missedInterrupts, counter, 0, 0);

            /* Disable interrupts and clear registers */

            /* logMsg ("event Handler - disabling event interrupts\n", 0, 0, 0, 0, 0, 0); */

            if (xyIntOFF() == ERROR)
            {
               logMsg ("eventHandler - error in xyIntOFF\n", 0, 0, 0, 0, 0, 0);
            }
            *intMaskReg = 0;

            /* I don't think this is necessary here. 
             *flagOutReg = CLR_M2_BIT;
             */

            /* Leave the green LED on after end of chopping */

            *statConReg |= SC_GREEN_LED;	

            /* Reset the beam to A and post to RM so that 
             * guiding can work. Always move to BEAMA */

            /*		if (presentBeam != BEAMA)
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
        demand = (*(ports[0])) & portMask;
        break;

    case ICS1:
        /* set the demand to the LSB of the 4 MSB of the port */
        demand = (*(ports[0]) >> 4) & portMask;
        break;

    case ICS2:
        demand = (*(ports[1])) & portMask;
        break;

    case ICS3:
        demand = (*(ports[1]) >> 4) & portMask;
        break;

    case ICS4:
        demand = (*(ports[2])) & portMask;
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

    *(ports[6]) = presentBeam + posFlag;
    *(ports[5]) = ((*(ports[6])) << 4) + (*(ports[6]));
    *(ports[4]) = *(ports[5]);

    /* Now pulse the FOR lines used to strobe the data into the event
     * system hub */

    *flagOutReg |= SET_STROBES;

    /* Allow time for the electronics to latch the signal, before
     * resetting pulse. Divide by 100000 to make a 10 us delay.  
     * Note: sysClkRateGet() returns ticks per second. */

    taskDelay(sysClkRateGet()/100000);

    /* reset pulse */

    *flagOutReg &= CLR_STROBES;
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

      intRead = *intPendReg;  	 /* store the pend register contents. 
                                  * Use pend instead of in reg, because
                                  * it is after the mask */

      *intClrReg = currentICSMask;  /* clear the bits we just read */
      if (debugLevel == DEBUG_RESERVED1)
         logMsg("Interrupt on currentICSMask: %02x \n\n", currentICSMask, 0, 0, 0, 0, 0);
      semGive (xySem); /* release the semaphore */
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
      semGive (xyInpositionSem);
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


   /* *intClrReg = intRead; */ 	/* clear the bits we just read */
   /* *intClrReg = 0xFF; */ 	/* clear all bits */
   /* *intClrReg = 0x02; */ 	/* clear the bits we just read */
   /* *statConReg ^= SC_RED_LED; */ 	/* toggle the red LED */
   /* logMsg("isr = intRead %d \n", intRead, 0, 0, 0, 0, 0);  */

   /* *intClrReg = *intPendReg;  */	/* clear the bits we just read */
   /* interruptCounter++; */

   /* *(char *) intClr = 0xFF; */ /* clear everything */
   /* *intClrReg = 0xFF; */ /* clear the bits we just read */
}

/*****************************************************************************/
long getSyncMask(void)
{
    return (long)currentICSMask;
}

/*****************************************************************************/
int xyIntON (void)
{
    int status;

    xyInited = FALSE;	
    xyInit();

    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    /* careful not to run it twice
     */
    if (icsTid != ERROR) {
	printf ("It appears the interrupt tasks are already running\n");
	return ERROR;
    }

    /* careful not to run it twice
     */
    if (inposTid != ERROR) {
	printf ("It appears the interrupt tasks are already running\n");
	return ERROR;
    }

    /* spawn the task which will keep an eye on our interrupts
     */
    if ((icsTid = taskSpawn ("tWatchInt", 9, 0, 20000, (FUNCPTR)xyIntHandler, 
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) == ERROR) {
	printf ("Cannot watch for interrupt\n");
	return ERROR;
    }

    /* spawn the task which will keep an eye on our interrupts
     */
    if ((inposTid = taskSpawn ("tWatchInpos", 7, 0, 20000, (FUNCPTR)inPositionHandler, 
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) == ERROR) {
	printf ("Cannot watch for interrupt\n");
	return ERROR;
    }

    /* set up the MC680x0 so it has our routine in its interrupt vector
     * table at position INT_VECTOR
     */
    if ((status = intConnect (INUM_TO_IVEC (INT_VECTOR), (VOIDFUNCPTR) isr, 0))
	    == ERROR) 
    {
	printf ("Can not install interrupt service routine\n");
	return status;
    }
    /* clear the interrupt input register by writing to the
     * interrupt clear register
     */
     *intClrReg = 0xFF;  

    /* write the vector address to the xycom so that the MC680x0 can
     * ask it for this when an interrupt occurs, and thus know what
     * routine ought to be executed
     */
    *intVecReg = INT_VECTOR;

    /* write a mask to the interrupt mask register to allow or disallow
     * specific interrupts; initially we will mask out all interrupts
     */
    *intMaskReg = 0x00; 

    /* enable interrupt capability by writing bit 3 of the status and
     * control register
     */
    *statConReg |= SC_INTERRUPT_ENABLE;

    /*
     * Now we are ready to get interrupts, but two things have to be
     * taken care of:
     * 1) We have to set the interrupt mask register in the xy240 so
     *    that interrupt signals are processed on the xy240.  Use the
     *    xyWriteIMR() routine for this.
     * 2) We have to set up the MC680x0 to recognize interrupts on the
     *    appropriate interrupt level.  The interrupt level on which
     *    the xy240 asserts an interrupt is configured by switch
     *    settings on the card.  The only way to know this is to remove
     *    the card and look.  So, figure out the setting, remember it,
     *    and use it in the xySIE() call.
     */
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

   semDelete(xySem);
   semDelete(xyInpositionSem);

   return OK;
}

/*****************************************************************************/
int xyStatus (void)
{
   int i;

   if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

   printf ("Interrupt Input register   = 0x%02x\n", *intInReg);
   printf ("Interrupt Mask register    = 0x%02x\n", *intMaskReg);
   printf ("Interrupt Pending register = 0x%02x\n", *intPendReg);
   printf ("Interrupt Pending flag     = %d\n", (*statConReg & SC_INTERRUPT_PENDING)?1:0);
   printf ("\n");
   printf ("Status/Control register    = 0x%02x\n", *statConReg);
   printf ("Interrupt Vector register  = 0x%02x\n", *intVecReg);
   printf ("Flag Outputs register      = 0x%02x\n", *flagOutReg);
   printf ("Port Direction register    = 0x%02x\n", *portDirReg);
   printf ("\n");
   printf ("Ports: ");
   for (i=0 ; i<8 ; i++) {
      printf ("0x%02x  ", *(ports[i]));
   }
   printf ("\n");

   return OK;
}

/*****************************************************************************/
/*
 * Print interrupt vector table.  Assumes MC680x0 architecture.
 * excIntStub is the default function installed in the table.
 * See .../arch/mc68k/excMc68kLib.h for exception library defines.
 */

#define INT_INFO_NUM_ROW 64
#define INT_INFO_NUM_COL 256/INT_INFO_NUM_ROW

int showIntInfo (void)
{
   int i, j;
   VOIDFUNCPTR vec;

   printf ("Current interrupt lock level: %d\n", intLockLevelGet ());
   printf ("Interrupt vectors 64-255 are available for user use.\n");
   printf ("Current interrupt vector table:\n");
   for (i = 0; i < INT_INFO_NUM_ROW; i++) {
      for (j = i; j < 256; j += INT_INFO_NUM_ROW) {
         vec = (VOIDFUNCPTR)intVecGet ((FUNCPTR *) INUM_TO_IVEC (j));
         /*	    if (vec != excIntStub) { */
         printf ("%03d(%02x):%07x    ", j, j, (unsigned int)vec);
         /*	    } else { 
                   printf ("%03d(%02x):-------    ", j, j);
                   }*/ 
      }
      printf ("\n");
   }
   return OK;
}

/*****************************************************************************/
int xyReadByte (int port)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( port<0 || port>7 ) {
	printf ("argument out of range: port 0..7\n");
	return ERROR;
    }
    printf ("port %1d contains: 0x%02x (%d)\n", port, *(ports[port]), *(ports[port]));
    return OK;
}

/*****************************************************************************/
int xyReadBit (int port, int bitnum)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( port<0 || port>7 || bitnum<0 || bitnum>7 ) {
	printf ("argument out of range: port 0..7, bitnum 0..7\n");
	return ERROR;
    }
    printf ("port %1d bit %1d value is: %1d\n", port, bitnum, ((*(ports[port]) & (1 << bitnum)) ? 1 : 0));
    return OK;
}

/*****************************************************************************/
int xyWriteByte (int port, unsigned char byte)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( port<0 || port>7 ) {
	printf ("argument out of range: port 0..7, byte 0..255\n");
	return ERROR;
    }
    *(ports[port]) = byte;
    return OK;
}

/*****************************************************************************/
int xyWriteBit (int port, int bitnum, int val)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( port<0 || port>7 || bitnum<0 || bitnum>7 || val<0 || val>1 ) {
	printf ("argument out of range: port 0..7, bitnum 0..7, val 0..1\n");
	return ERROR;
    }
    if (val) {
	*(ports[port]) |=  (1 << bitnum);
    } else {
	*(ports[port]) &= ~(1 << bitnum);
    }
    return OK;
}

/*****************************************************************************/
int xyReadFlagByte (void)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    printf ("Flag Output Register: 0x%02x (%d)\n", *flagOutReg, *flagOutReg);
    return OK;
}

/*****************************************************************************/
int xyReadFlagBit (int bitnum)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 ) {
	printf ("argument out of range: bitnum 0..7\n");
	return ERROR;
    }
    printf ("Flag Output Register bit %1d: %d\n", bitnum, ((*(flagOutReg) & (1 << bitnum)) ? 1 : 0));
    return OK;
}

/*****************************************************************************/
int xyWriteFlagByte (unsigned char byte)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    *flagOutReg = byte;
    return OK;
}

/*****************************************************************************/
int xyWriteFlagBit (int bitnum, int val)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 || val<0 || val>1 ) {
	printf ("argument out of range: bitnum 0..7, val 0..1\n");
	return ERROR;
    }
    if (val) {
	*flagOutReg |=  (1 << bitnum);
    } else {
	*flagOutReg &= ~(1 << bitnum);
    }
    return OK;
}

/*****************************************************************************/
int xyReadIIR (void)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    printf ("Interrupt Input Register: 0x%02x (%d)\n", *intInReg, *intInReg);
    return OK;
}

/* note: IIR is read-only */

/*****************************************************************************/
int xyReadIPR (void)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    printf ("Interrupt Pending Register: 0x%02x (%d)\n", *intPendReg, *intPendReg);
    return OK;
}

/* note: IPR is read-only */

/*****************************************************************************/
int xyReadIVR (void)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    printf ("Interrupt Vector Register: 0x%02x (%d)\n", *intVecReg, *intVecReg);
    return OK;
}

/*****************************************************************************/
int xyWriteIVR (unsigned char byte)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    *intVecReg = byte;
    return OK;
}

/*****************************************************************************/
int xyReadSCR (void)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    printf ("Status/Control Register: 0x%02x (%d)\n", *statConReg, *statConReg);
    return OK;
}

/*****************************************************************************/
int xyReadSCRBit (int bitnum)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 ) {
	printf ("argument out of range: bitnum 0..7\n");
	return ERROR;
    }
    printf ("Status/Control Register bit %1d: %d\n", bitnum, ((*(statConReg) & (1 << bitnum)) ? 1 : 0));
    return OK;
}

/*****************************************************************************/
int xyWriteSCR (unsigned char byte)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    *statConReg = byte;
    return OK;
}

/*****************************************************************************/
int xyWriteSCRBit (int bitnum, int val)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 || val<0 || val>1 ) {
	printf ("argument out of range: bitnum 0..7, val 0..1\n");
	return ERROR;
    }
    if (val) {
	*statConReg |=  (1 << bitnum);
    } else {
	*statConReg &= ~(1 << bitnum);
    }
    return OK;
}

/*****************************************************************************/
int xyReadIMR (void)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    printf ("Interrupt Mask Register: 0x%02x (%d)\n", *intMaskReg, *intMaskReg);
    return OK;
}

/*****************************************************************************/
int xyReadIMRBit (int bitnum)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 ) {
	printf ("argument out of range: bitnum 0..7\n");
	return ERROR;
    }
    printf ("Interrupt Mask Register bit %1d: %d\n", bitnum, ((*(intMaskReg) & (1 << bitnum)) ? 1 : 0));
    return OK;
}

/*****************************************************************************/
int xyWriteIMR (unsigned char byte)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    *intMaskReg = byte;
    return OK;
}

/*****************************************************************************/
int xyWriteIMRBit (int bitnum, int val)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 || val<0 || val>1 ) {
	printf ("argument out of range: bitnum 0..7, val 0..1\n");
	return ERROR;
    }
    if (val) {
	*intMaskReg |=  (1 << bitnum);
    } else {
	*intMaskReg &= ~(1 << bitnum);
    }
    return OK;
}

/*****************************************************************************/
int xyReadPDR (void)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    printf ("Port Direction Register: 0x%02x (%d)\n", *portDirReg, *portDirReg);
    return OK;
}

/*****************************************************************************/
int xyReadPDRBit (int bitnum)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 ) {
	printf ("argument out of range: bitnum 0..7\n");
	return ERROR;
    }
    printf ("Port Direction Register bit %1d: %d\n", bitnum, ((*(portDirReg) & (1 << bitnum)) ? 1 : 0));
    return OK;
}

/*****************************************************************************/
int xyWritePDR (unsigned char byte)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    *portDirReg = byte;
    return OK;
}

/*****************************************************************************/
int xyWritePDRBit (int bitnum, int val)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 || val<0 || val>1 ) {
	printf ("argument out of range: bitnum 0..7, val 0..1\n");
	return ERROR;
    }
    if (val) {
	*portDirReg |=  (1 << bitnum);
    } else {
	*portDirReg &= ~(1 << bitnum);
    }
    return OK;
}

/*****************************************************************************/
int xyWriteICR (unsigned char byte)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    *intClrReg = byte;
    return OK;
}

/* note: ICR is write-only */

/*****************************************************************************/
int xyWriteICRBit (int bitnum, int val)
{
    if (!xyInited) { printf ("run xyInit first\n"); return ERROR; }

    if ( bitnum<0 || bitnum>7 || val<0 || val>1 ) {
	printf ("argument out of range: bitnum 0..7, val 0..1\n");
	return ERROR;
    }
    if (val) {
	*intClrReg |=  (1 << bitnum);
    } else {
	*intClrReg &= ~(1 << bitnum);
    }
    return OK;
}

/*****************************************************************************/
int xySIE (int level)
{
   int status;

   if ( level<1 || level>7 ) {
       printf ("argument out of range: level 1..7\n");
       return ERROR;
   }
   if ((status = sysIntEnable(level)) == ERROR) {
       printf ("error attempting to enable bus interrupt level %d\n", level);
       return ERROR;
   }
   return OK;
}

/*****************************************************************************/
int xySID (int level)
{
    int status;

    if ( level<1 || level>7 ) {
        printf ("argument out of range: level 1..7\n");
        return ERROR;
    }
    if ((status = sysIntDisable(level)) == ERROR) {
        printf ("error attempting to disable bus interrupt level %d\n", level);
        return ERROR;
    }
    return OK;
}

/*****************************************************************************/
int xyHelp (void)
{

   printf("\
         showIntInfo (void)\n\
         xyInit (void)\n\
         xyPrintID (void)\n\
         xyStatus (void)\n\
         xyIntON (void)                     xyIntOFF (void)\n\
         xySIE (int level)                  xySID (int level)\n\
         xyReadByte (int port)              xyWriteByte (int port, unsigned char byte)\n\
         xyReadBit (int port, int bitnum)   xyWriteBit (int port, int bitnum, int val)\n\
         xyReadFlagByte (void)              xyWriteFlagByte (unsigned char byte)\n\
         xyReadFlagBit (int bitnum)         xyWriteFlagBit (int bitnum, int val)\n\
         xyReadIIR (void)\n\
         xyReadIPR (void)\n\
         xyReadIVR (void)                   xyWriteIVR (unsigned char byte)\n\
         xyReadSCR (void)                   xyWriteSCR (unsigned char byte)\n\
         xyReadSCRBit (int bitnum)          xyWriteSCRBit (int bitnum, int val)\n\
         xyReadIMR (void)                   xyWriteIMR (unsigned char byte)\n\
         xyReadIMRBit (int bitnum)          xyWriteIMRBit (int bitnum, int val)\n\
         xyReadPDR (void)                   xyWritePDR (unsigned char byte)\n\
         xyReadPDRBit (int bitnum)          xyWritePDRBit (int bitnum, int val)\n\
         xyWriteICR (unsigned char byte)\n\
         xyWriteICRBit (int bitnum, int val)\n\
         ");

   return OK;
}

/*****************************************************************************/
