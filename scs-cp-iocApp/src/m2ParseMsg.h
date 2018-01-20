/* $Id: m2ParseMsg.h,v 1.2 2003/01/18 23:36:36 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * m2ParseMsg.h
 * 
 * PURPOSE
 * -------
 * 
 *
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
 * Dayle Kotturi ()
 * 
 * HISTORY
 * -------
 * 
 * 04-Feb-2000: Original (kdk)
 *
 */
/* INDENT ON */
/* ===================================================================== */
                /* System IDs */
#define ERRLOG_MISC     1       /* miscellaneous */
#define ERRLOG_MCD      2       /* mirror control DSP */
#define ERRLOG_VCD      3       /* vibration control DSP */
#define ERRLOG_XY       4       /* XY translation table */
#define ERRLOG_DBAF     5       /* deployable baffle */
#define ERRLOG_CBAF     6       /* central baffle */

               /* Miscellaneous Codes */
#define NO_OPEN_GAIN    1       /* Can't open file for init'l MC gains */
#define NO_READ_GAIN    2       /* Can't read properly from gain file */
#define NO_SCS_HBEAT    3       /* SCS heartbeat hasn't changed for awhile */
#define NO_OPEN_NONLIN  4       /* Can't open file for non-lin compenstation */
#define NO_READ_NONLIN  5       /* trouble reading non-lin compensation */
#define NO_ACT_POWER    6       /* Actuator power refuses to turn on */
#define ADC_HANGUP      7       /* A/D timed out waiting for EOC */
#define NO_OPEN_DPBLIM  8       /* can't open file for dbaf position limits */
#define NO_READ_DPBLIM  9       /* can't read properly from dbaf file */
#define NO_DSP_DATA    10       /* can't get GRON data from DSP    (NGI) */

            /* Mirror Control DSP Codes */
#define MC_NO_OPEN_NOTCH              1 /* Can't open file for notch filter para
ms */
#define MC_NO_READ_NOTCH              2 /* Can't read properly from notch filter
 file */
#define MC_NO_LOAD                    3 /* Can't load DSP program */
#define MC_NO_RUN                     4 /* Can't run DSP program */
#define MC_NO_SUCH_BANDWIDTH          5 /* Bandwidth selection not supported */
#define MC_INIT_OFFLOAD_TIMEOUT       6
#define MC_INIT_SENSOR_SWITCH_TIMEOUT 7

           /* Vibration Control DSP Codes */
#define VC_NO_OPEN_NOTCH       1 /* Can't open file for notch filter params */
#define VC_NO_READ_NOTCH       2 /* Can't read properly from notch filter file */
#define VC_NO_LOAD             3 /* Can't load DSP program */
#define VC_NO_RUN              4 /* Can't run DSP program */
#define VC_INIT_FOLLOW_TIMEOUT 5

           /* XY table Codes */
#define XY_TOO_CLOSE        1
#define XY_TOO_FAR          2
#define XY_CMD_OUT_OF_RANGE 3
#define XY_INIT_TIMEOUT     4
#define XY_NEG_SQ_ROOT      5

           /* Deployable Baffle Codes */
#define DB_BAD_POS_CMD      1
#define DB_UPPER_LIMIT      2
#define DB_LOWER_LIMIT      3

           /* Central Baffle Codes */
#define CB_BAD_POS_CMD      1
#define CB_UPPER_LIMIT      2
#define CB_LOWER_LIMIT      3

#define MAX_SYS_MSGS 11
#define NUM_M2SUBSYS  7

typedef struct {
    char msgBody[80];
} msgContainer;

typedef struct {
    int numerrs;
    msgContainer msg[MAX_SYS_MSGS];
} sysErrContainer;

typedef struct {
    sysErrContainer sys[NUM_M2SUBSYS];
} m2ErrorContainer;



char *parseM2Msg( long errorSystem, long errorCode );

