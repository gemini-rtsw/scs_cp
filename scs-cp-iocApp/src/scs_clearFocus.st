program scs_clearFocus

option -a;  /* synchronous          Default  */
option +c;  /* wait for connections Default  */
option +d;  /* debug messages on    Default  */
option +e;  /* new event flag mode  Default  */
option +l;  /* C compiler messages  Default  */
option -r;  /* not reentrant        Default (Only one instance of this seq program allowed)   */
option +w;  /* SNC warnings         Default */

/* ===================================================================== */

int32_t clearGuideFocusCadStart;
assign clearGuideFocusCadStart to "{T}clearGuideFocusP:start.VAL";
monitor clearGuideFocusCadStart;

evflag clearGuideFocusCadChange;
sync clearGuideFocusCadStart to clearGuideFocusCadChange;

int32_t clearGuideFocusCarIn;               /* input to clearGuideFocusC CAR */
assign clearGuideFocusCarIn to "{T}clearGuideFocusC.IVAL";

int32_t clearGuideFocusCarErrIn;          /* error input to clearGuideFocusC CAR */
assign clearGuideFocusCarErrIn to "{T}clearGuideFocusC.IERR";

string clearGuideFocusCarMessIn;      /* mess. input to clearGuideFocusC CAR */
assign clearGuideFocusCarMessIn to "{T}clearGuideFocusC.IMSS";
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

