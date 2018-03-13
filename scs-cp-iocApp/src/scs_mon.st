program scs_mon

option -a;  /* synchronous          Default  */
option +c;  /* wait for connections Default  */
option +e;  /* new event flag mode  Default  */
option +l;  /* C compiler messages  Default  */
option -r;  /* not reentrant        Default (Only one instance of this seq program allowed)   */
option +w;  /* SNC warnings         Default */

%%#include "control.h"      /* For writeCommand, scsPtr, scsBase, filtered,
%%#include "utilities.h"    /* For UPDATEPARAMETERS*/
%%#include "chop.h"         /* For chopIsOn, jogBeam, instruments */
%%#include "config.h"       /* For depBaffle, cenBaffle */
%%#include <car.h>

int32_t monitorProcessState;
assign monitorProcessState to "{T}monitorProcessState.VAL";
monitor monitorProcessState;

int32_t coincidence;
assign coincidence to "{T}coincidence.VAL";
monitor coincidence;

int32_t monitorSadState;
assign monitorSadState to "{T}monitorSadState.VAL";
monitor monitorSadState;

string inPositionSad;                   /* in position (1=not in position, 0=in position) */
assign inPositionSad to "{D}inPosition.VAL";

string deployableSad;                   /* deployable baffle position (key as for demand) */
assign deployableSad to "{D}DeployBafflePos.VAL";

string centralSad;                      /* central baffle position (key as for demand) */ 
assign centralSad to "{D}CentralBafflePos.VAL";

int32_t moveBaffleCar;
assign moveBaffleCar to "{T}moveBaffleC.IVAL";

double tempSad;
assign tempSad to "{D}temperature.VAL";

string chopping;                        /* currently chopping (yes or no) */
assign chopping to "{D}chopping.VAL";

//string topEndSad;                       /* current top end (F6 or F16) */
//assign topEndSad to "{D}topEnd.VAL";

int32_t followS;
assign followS to "{T}followS.VAL";

double xWindup;
assign xWindup to "{T}xWindup.VAL";
double yWindup;
assign yWindup to "{T}yWindup.VAL";
double zWindup;
assign zWindup to "{T}zWindup.VAL";

int32_t source;                            /* sensor source to which this record applies */
assign source to "{T}guideConfig.VALA";

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
   
   state start {

      option -e;
      entry {
        puts("MonitorProcess:updateParameters start");
      }
      when() {
      } state updateParameters
   }

   state updateParameters {

      option +e;
      entry {
        puts("MonitorProcess:updateParameters start");
      }
      when(delay(0.5) ) {
         monitorProcessState = UPDATEPARAMETERS; 
         pvPut(monitorProcessState);

         if(refMemFree) {
            epicsMutexLock(refMemFree);
            /* invert sense of in position*/
            coincidence = !scsPtr->page1.inPosition;
            epicsMutexUnlock(refMemFree);
         }
         else {
            errorLog ("monitorProcess - refMemFree timeout", 1, ON);
         }

         pvPut(coincidence);

      } state updateParameters
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
   state init {
      when() {
         monitorSadState = MONSADENTRY; 
         pvPut(monitorSadState);

      }   state updateSad
   }

   state updateSad {
      when(delay(1.0)) {
         monitorSadState = UPDATESAD; 
         pvPut(monitorSadState);

         printf("mon:updatesad page1\n");
         %{
            if(refMemFree) {
                 epicsMutexLock(refMemFree);

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
                 epicsMutexUnlock(refMemFree);
             }
             else
             {
                 errorLog("state updateSad - couldn't obtain refMemFree mutex", 1, ON);
             }
         }%

         printf("mon:updatesad page1\n");
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

         printf("mon:updatesad source\n");
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



