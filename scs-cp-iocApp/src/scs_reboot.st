program scs_reboot

option -a;  /* synchronous          Default  */
option +c;  /* wait for connections Default  */
option +d;  /* debug messages on    Default  */
option +e;  /* new event flag mode  Default  */
option +l;  /* C compiler messages  Default  */
option -r;  /* not reentrant        Default (Only one instance of this seq program allowed)   */
option +w;  /* SNC warnings         Default */
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
        when(servoOnStat == 0) {
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
            %{
                if(epicsMutexLock(setPointFree)) {
                    setPoint.xTiltA = parkPosition.xTiltNew;
                    setPoint.yTiltA = parkPosition.yTiltNew;
                    setPoint.zFocus = parkPosition.zFocusNew;
                    setPoint.xTiltB = parkPosition.xTiltNew;
                    setPoint.yTiltB = parkPosition.yTiltNew;
                    setPoint.xTiltC = parkPosition.xTiltNew;
                    setPoint.yTiltC = parkPosition.yTiltNew;
                    setPoint.xPosition = parkPosition.xPosNew;
                    setPoint.yPosition = parkPosition.yPosNew;

                    epicsMutexUnlock(setPointFree);
                }
                else {
                    errorLog("state proceedWithRebootPark - couldn't obtain setPointFree mutex", 1, ON);
                }
            }%
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
                epicsEventSignal(eventSem); 
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
                epicsEventSignal(eventSem); 
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
                epicsEventSignal(eventSem); 
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

            //if(loggingNow != OFF)
            //   %%scsLogDestruct();

            //%%reboot(BOOT_QUICK_AUTOBOOT);

            /* At this point the system shall reboot so no further code */
            /* execution is permitted. The return state has been set to */
            /* startReboot to keep it tidy and local            */

        }   state startReboot
    }
}


