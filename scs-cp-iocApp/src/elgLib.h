/* $Id: elgLib.h,v 1.1 2002/02/05 13:19:48 gemvx Exp $ */
/* Edict logging system library header file                       */
/* Original as edloglib.h by Alan Pickup - ROE - 1997 May 8       */
/*
modification history
--------------------
1997 May  8: Original (dap)
1997 May 16: Renamed elglib.h (dap)
1997 May 21: Conditional compilation via VxMP_ENABLED (dap)
1997 Aug 28: Renamed from elglib to elgLib (dap)
1997 Aug 28: Added elgi and elgf (dap)
2017 Oct 11: Begin conversion to EPICS OSI (mdw)
             Removed VxMP_ENABLED code (we don't use it) (mdw)
*/

#ifndef _INCLUDED_ELGLIB_H
#define _INCLUDED_ELGLIB_H

#include "amsLib.h"

/* Define elg version number */
#define ELG_VERSION "1.00"

/* Global constants */
#define ELG_PRIORITY    150L   /* Priority for elogger task */
#define ELG_SIZE         50L   /* Size of the logging message queue */
#define ELG_CHANNEL     200L   /* ams channel number for messages */
#define ELG_TYPE     AMS_CMD   /* local queue */

/* Global variables */
extern int elgInitialised;  /* 1 after initialisation via elgInit */
extern int elgInitialising; /* 1 during initialising */
//extern MSG_Q_ID elgQ;       /* for messages to the logger */
extern epicsMessageQueueId elgQ;       /* for messages to the logger */

/* elg - Enter a message into the log as from module 0*/
extern int elg
(
char *message     /* Message text (given) */
);
/* elgs - Enter a character string from given module into the log */
extern int elgs
(
   int module,    /* Module number prefixes message in log (given) */
   char * message /* Message text (given) */
);
/* elgi - Enter string and integer value from given module into log */
extern int elgi
(
   int module,    /* Module number prefixes message in log (given) */
   char * message,/* Message text (given) */
   int ivalue     /* Integer value appended to message (given) */
);
/* elgl - Enter string and long value from given module into log */
extern int elgl
(
   int module,    /* Module number prefixes message in log (given) */
   char * message,/* Message text (given) */
   long lvalue    /* Long value appended to message (given) */
);
/* elgf - Enter string and float value from given module into log */
extern int elgf
(
   int module,    /* Module number prefixes message in log (given) */
   char * message,/* Message text (given) */
   float fvalue   /* Float value appended to message (given) */
);
/* elgd - Enter string and double value from given module into log */
extern int elgd
(
   int module,    /* Module number prefixes message in log (given) */
   char * message,/* Message text (given) */
   double dvalue  /* Double value appended to message (given) */
);
/* elgOpen - Open logging file */
extern int elgOpen
(
   char * filename   /* Filename to open (given) */
);
/* elgFlush - Flush the current logging file to disk */
extern int elgFlush();
/* elgReport - Report status of logging system to console */
extern int elgReport();
/* elgClose - Close the current logging file */
extern int elgClose();
/* elgDone - Close any curreny logging file and delete the logging task */
extern int elgDone();
/* elgPut - Write command, module number and mesage to logging queue */
int elgPut
(
   int module,    /* Module number prefixes message in log (given) */ 
   char * command,/* Command to logging task (eg "log") (given) */
   char * message /* Message string (given) */
);
/* elgInit - Initialise the elg system */
extern int elgInit(void);
/* elgAttach - Attach to the elg logging system from a secondary processor */
extern int elgAttach(void);
/* elgConnect - Wait until the elg system is initialised on this processor */
extern int elgConnect(void);
/* elogger - Logging task */
//extern int elogger(void);
EPICSTHREADFUNC elogger(void *);

#endif
