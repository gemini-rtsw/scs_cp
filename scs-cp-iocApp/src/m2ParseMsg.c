/* $Id: m2ParseMsg.c,v 1.4 2005/07/12 20:03:56 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * m2ParseMsg.c
 * 
 * PURPOSE
 * -------
 * For a given system and error code, return the string which describes
 * this error
 * 
 * FUNCTION NAME(S)
 * ----------------
 * m2ParseMsg
 * 
 * DEPENDENCIES
 * ------------
 *
 * LIMITATIONS
 * -----------
 * 
 * AUTHOR
 * ------
 * Dayle Kotturi 
 * 
 * HISTORY
 * -------
 * 
 * NOTES
 * -----
 * It would be better to so the filling in of the structure of error msgs
 * only once, in an init routine. Right now, the struct is filled each time.
 *
 * 04-Feb-2000: Original (kdk)
 *
 */
/* INDENT ON */
/* ===================================================================== */


#include <stdio.h>
#include <string.h>

#include "m2ParseMsg.h"

char *parseM2Msg( long errorSystem, long errorCode )
{

#ifdef DEBUG
    char msg[80];
    int i, j;
#endif

    m2ErrorContainer m2Errors;
  
    /* Do range checking on errorSystem and errorCode */
    if (((errorSystem < 1) || (errorCode <1)) ||
	(errorSystem > NUM_M2SUBSYS ))
	return (char *)NULL;

    /* It is still possible that errorCode number is invalid
     * for a given errorSystem. Do this test after setup.
     */

    /* This setup could be moved to an init routine called only once */
    m2Errors.sys[ERRLOG_MISC].numerrs = 10;
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_OPEN_GAIN].msgBody, 
           "Can't open file for init'l MC gains");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_READ_GAIN].msgBody, 
           "Can't read properly from gain file");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_SCS_HBEAT].msgBody,
           "SCS heartbeat hasn't changed for awhile");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_OPEN_NONLIN].msgBody, 
           "Can't open file for non-lin compenstation");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_READ_NONLIN].msgBody, 
           "trouble reading non-lin compensation");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_ACT_POWER].msgBody, 
           "Actuator power refuses to turn on");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[ADC_HANGUP].msgBody, 
           "A/D timed out waiting for EOC");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_OPEN_DPBLIM].msgBody, 
           "Can't open file for dbaf position limits");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_READ_DPBLIM].msgBody, 
           "Can't read properly from dbaf file");
    strcpy(m2Errors.sys[ERRLOG_MISC].msg[NO_DSP_DATA].msgBody, 
           "Can't get GRON data from DSP    (NGI)");

    m2Errors.sys[ERRLOG_MCD].numerrs = 7;
    strcpy(m2Errors.sys[ERRLOG_MCD].msg[MC_NO_OPEN_NOTCH].msgBody, 
           "Can't open file for notch filter params");
    strcpy(m2Errors.sys[ERRLOG_MCD].msg[MC_NO_READ_NOTCH].msgBody, 
           "Can't read properly from notch filter file");
    strcpy(m2Errors.sys[ERRLOG_MCD].msg[MC_NO_LOAD].msgBody, 
           "Can't load DSP program");
    strcpy(m2Errors.sys[ERRLOG_MCD].msg[MC_NO_RUN].msgBody, 
           "Can't run DSP program");
    strcpy(m2Errors.sys[ERRLOG_MCD].msg[MC_NO_SUCH_BANDWIDTH].msgBody, 
           "Bandwidth selection not supported");
    strcpy(m2Errors.sys[ERRLOG_MCD].msg[MC_INIT_OFFLOAD_TIMEOUT].msgBody, 
           "Offloader init timeout");
    strcpy(m2Errors.sys[ERRLOG_MCD].msg[MC_INIT_SENSOR_SWITCH_TIMEOUT].msgBody, 
           "Switch to microEs timeout");

    m2Errors.sys[ERRLOG_VCD].numerrs = 5;
    strcpy(m2Errors.sys[ERRLOG_VCD].msg[VC_NO_OPEN_NOTCH].msgBody, 
           "Can't open file for notch filter params");
    strcpy(m2Errors.sys[ERRLOG_VCD].msg[VC_NO_READ_NOTCH].msgBody, 
           "Can't read properly from notch filter file");
    strcpy(m2Errors.sys[ERRLOG_VCD].msg[VC_NO_LOAD].msgBody, 
           "Can't load DSP program");
    strcpy(m2Errors.sys[ERRLOG_VCD].msg[VC_NO_RUN].msgBody, 
           "Can't run DSP program");
    strcpy(m2Errors.sys[ERRLOG_VCD].msg[VC_INIT_FOLLOW_TIMEOUT].msgBody, 
           "Follower init timeout");

    m2Errors.sys[ERRLOG_XY].numerrs = 5;
    strcpy(m2Errors.sys[ERRLOG_XY].msg[XY_TOO_CLOSE].msgBody,
           "XY positioner too close to rotational axis of upper bearing");
    strcpy(m2Errors.sys[ERRLOG_XY].msg[XY_TOO_FAR].msgBody,
           "XY positioner too far out (fully extended 'arms'");
    strcpy(m2Errors.sys[ERRLOG_XY].msg[XY_INIT_TIMEOUT].msgBody,
           "XY positioner init timeout");
    strcpy(m2Errors.sys[ERRLOG_XY].msg[XY_NEG_SQ_ROOT].msgBody,
           "XY positioner algorithm has negative number in square root sign");

    m2Errors.sys[ERRLOG_DBAF].numerrs = 3;
    strcpy(m2Errors.sys[ERRLOG_DBAF].msg[DB_BAD_POS_CMD].msgBody,
           "Deployable baffle command is out of range"); 
    strcpy(m2Errors.sys[ERRLOG_DBAF].msg[DB_UPPER_LIMIT].msgBody,
           "Deployable baffle hit upper limit");
    strcpy(m2Errors.sys[ERRLOG_DBAF].msg[DB_LOWER_LIMIT].msgBody,
           "Deployable baffle hit lower limit");

    m2Errors.sys[ERRLOG_CBAF].numerrs = 3;
    strcpy(m2Errors.sys[ERRLOG_CBAF].msg[CB_BAD_POS_CMD].msgBody,
           "Periscope command is out of range"); 
    strcpy(m2Errors.sys[ERRLOG_CBAF].msg[CB_UPPER_LIMIT].msgBody,
           "Periscope hit upper limit");
    strcpy(m2Errors.sys[ERRLOG_CBAF].msg[CB_LOWER_LIMIT].msgBody,
           "Periscope hit lower limit");

/*
    for (i=1; i<NUM_M2SUBSYS; i++)
      {
      printf("number of errors for system[%1d] is %d\n", i, m2Errors.sys[i].numerrs);
      for (j=1; j<=m2Errors.sys[i].numerrs; j++)
        printf("msgs are %s\n", m2Errors.sys[i].msg[j].msgBody);
      printf("\n"); 
      }
    sprintf(msg, "%d %d\n", errorSystem, errorCode);
    return msg;
*/

    /* Do more range checking on errorCode */
    if (errorCode > m2Errors.sys[errorSystem].numerrs)
	return (char *)NULL;

    return m2Errors.sys[errorSystem].msg[errorCode].msgBody;
}

