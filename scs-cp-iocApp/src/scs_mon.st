program scs_mon

option -a;  /* synchronous          Default  */
option +c;  /* wait for connections Default  */
option +d;  /* debug messages on    Default  */
option +e;  /* new event flag mode  Default  */
option +l;  /* C compiler messages  Default  */
option -r;  /* not reentrant        Default (Only one instance of this seq program allowed)   */
option +w;  /* SNC warnings         Default */

int32_t monitorProcessState;
assign monitorProcessState to "{T}monitorProcessState.VAL";
monitor monitorProcessState;

int32_t coincidence;
assign coincidence to "{T}coincidence.VAL";
monitor coincidence;

%%#include "control.h"      /* For writeCommand, scsPtr, scsBase, filtered,
%%#include "utilities.h"    /* For UPDATEPARAMETERS*/

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

         epicsMutexLock(refMemFree);
         /* invert sense of in position*/
         coincidence = !scsPtr->page1.inPosition;
         epicsMutexUnlock(refMemFree);

         pvPut(coincidence);

      } state updateParameters
   }

}


