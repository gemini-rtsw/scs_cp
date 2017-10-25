/* ALICE message system library header file            */
/* Original by Alan Pickup - ROE - 1997 March 11       */
/*
modification history
--------------------
1997 Apr  2: Functions return int status and long amsStat (dap)
1997 May 13: Non-VxMP version (dap)
1997 May 21: Use VxMP_ENABLED to indicate whether shared mem version (dap)
1997 May 29: Tidy up for documenting (dap)
2017 Oct 06: Begin conversion to EPICS OSI (mdw)
             Remove all VxMP_ENABLED code (we don't use it anyway)
*/

#ifndef _INCLUDED_AMSLIB_H
#define _INCLUDED_AMSLIB_H


/* EPICS includes */
#include <epicsMutex.h>
#include <epicsThread.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <errlog.h>

#define OK     0
#define ERROR (-1)
#define NO_WAIT 0.0
#define WAIT_FOREVER (-1.0)

/* Define version number of ams */
#define AMS_VERSION "1.00"

/* Define status/errors */
#define AMS__OK          0L     /* Status OK */
#define AMS__ERROR       1L     /* General failure status */
#define AMS__NOTINIT     2L     /* Not initialised (needs prior call of amsInit) */
#define AMS__FULL        3L     /* All (amsMax) channels allocated */
#define AMS__INVCHAN     4L     /* Invalid channel number (must be >0) */
#define AMS__INVTYPE     5L     /* Invalid channel type (must be AMS_CMD or AMS_REP) */
#define AMS__INVQSIZE    6L     /* Invalid queue size (must be >0 messages) */
#define AMS__CHANFIXED   7L     /* Cannot re-type an existing channel */
#define AMS__TIMEOUT     8L     /* Timeout */
#define AMS__NOCHAN      9L     /* Channel not allocated */
#define AMS__DONE       10L     /* Command completed + status OK */
#define AMS__SEMFAIL    11L     /* Failed to take or give semaphore */
#define AMS__MQDELFAIL  12L     /* Failed to delete message queue */
#define AMS__SEMCREFAIL 13L     /* Failed to create semaphore */
#define AMS__NAMADDFAIL 14L     /* Failed to add amsData to shared mem database */
#define AMS__NAMFNDFAIL 15L     /* Failed to find amsData in shared mem database */
#define AMS__MQCREFAIL  16L     /* Failed to create message queue */
#define AMS__MQSENDFAIL 17L     /* Failed to place message on message queue */
#define AMS__INVREPLIES 18L     /* Did not specify one or more replies */
#define AMS__INVREPPATH 19L     /* Reply path invalid */
#define AMS__NOTAVAIL   20L     /* Not supported in this amslib version */

/* Global constants */
#define AMS_YES        0L
#define AMS_NO         1L
#define AMS_CMD        1
#define AMS_REP        2
#define AMS_SMCMD      3
#define AMS_SMREP      4
#define AMS_CMDLEN     16     /* No of bytes in command string */
#define AMS_PARLEN     132    /* No of bytes in params string */
#define AMS_REPLEN     132    /* No of bytes in reply string */
#define AMS_CMDSIZE    AMS_CMDLEN+AMS_PARLEN+12  /* Bytes in aliceCmd struct*/
#define AMS_REPSIZE    AMS_REPLEN+12             /* Bytes in aliceRep struct */
#define AMS_MAXCHAN    10     /* Max no of channels allowed */
#define AMS_NAME    "amsData" /* Name for amsData in shared memory database */

/* Structure definitions */
struct amsCmd
{
   long messNo;       /* assigned by sending task */
   int replies;       /* number of reply messages expected */
   epicsMessageQueueId Q;    /* addr of message queue for receipt/replies (NULL for no receipt) */
   char command[AMS_CMDLEN]; /* name of command */
   char params[AMS_PARLEN];  /* command parameters */
};

struct amsRep
{
   long messNo;       /* identifies alice_cmd being replied to */
   int replyNo;       /* 0 for receipt; otherwise reply number */
   long repStatus;    /* AMS__OK, AMS__DONE or ... */
   char rep[AMS_REPLEN];    /* reply string and/or parameters */
};

typedef struct _amsChan
{
   long channel;             /* Channel number */
   epicsMessageQueueId Q;    /* Message queue ID for channel */
   int type;                 /* Channel type (AMS_CMD or AMS_REP) */
} amsChan;

typedef struct _amsData
{
   epicsMutexId amsSmID;     /* Shared semaphore */
   amsChan amsIndex[AMS_MAXCHAN]; /* Assigned channels */
} AMS_DATA;

/* Global variables */
extern int amsInitialised;   /* 1 after initialisation via amsInit */
//extern long amsShortTime;    /* Short timeout */
//extern AMS_DATA amsData;
//extern epicsMutexId amsID;
//extern int amsObjType;

/* Function prototypes */
/* amsInit - Initialise the ams system */
int amsInit ( long *amsStat);

/* amsAttach - Attach to an existing ams system from a slave processor */
int amsAttach ( long *amsStat );

/* amsConnect - Connect to an ams system from the current task */
int amsConnect ( long *amsStat );

/* amsTakeSem - Take semaphore controlling access to ams database */
int amsTakeSem ( long *amsStat );

/* amsGiveSem - Release semaphore controlling access to ams database */
int amsGiveSem ( long *amsStat );

/* amsFind - Looks up channel number and returns position in amsIndex database */
int amsFind ( long channel );

/* amsPath - Returns message queue ID for given channel number */
int amsPath
(
   long channel,     /* Channel number (given) */
   epicsMessageQueueId *path,   /* Message queue ID (returned) */
   long *amsStat     /* global status (given and returned) */
);

/* amsCreate - Creates message queue of given type and size */
int amsCreate
(
   long channel,     /* channel number (given) */
   int type,         /* Channel type (AMS_CMD/REP/SMCMD/SMREP) (given) */
   long size,        /* Size of message queue (given) */
   epicsMessageQueueId *path,   /* Meassage queue ID (returned) */
   long *amsStat     /* global status (given and returned) */
);

/* amsSend0 - Send a message */
int amsSend0
(
   epicsMessageQueueId path,    /* Message queue ID of queue to send on (given) */
   char * command,   /* Command string (given) */
   char * params,    /* Parameters string (given) */
   int replies,      /* Number of replies expected (given) */
   long *messNo,     /* Number of generated message (returned) */
   epicsMessageQueueId repPath, /* Message queue ID of queue for reply (given) */
   long *amsStat     /* global status (given and returned) */
);

/* amsSend - Send a message; don't await receipt or reply */
int amsSend
(
   epicsMessageQueueId path,    /* Message queue ID of queue to send on (given) */
   char * command,   /* Command string (given) */
   char * params,    /* Parameters string (given) */
   long *messNo,     /* Number of generated message (returned) */
   long *amsStat     /* global status (given and returned) */
);

/* amsCommand - send a message requiring replies, but don's await receipt */
int amsCommand
(
   epicsMessageQueueId path,    /* Message queue ID of queue to send on (given) */
   char * command,   /* Command string (given) */
   char * params,    /* Parameters string (given) */
   int replies,      /* Number of replies expected (given) */
   epicsMessageQueueId repPath, /* Message queue ID of queue for reply (given) */
   long *messNo,     /* Number of generated message (returned) */
   long *amsStat     /* global status (given and returned) */
);

/* amsCommandR - Send message and await first reply (= receipt) */
int amsCommandR
(
   epicsMessageQueueId path,    /* Message queue ID of queue to send on (given) */
   char * command,   /* Command string (given) */
   char * params,    /* Parameters string (given) */
   int replies,      /* Number of replies expected (given) */
   epicsMessageQueueId repPath, /* Message queue ID of queue for reply (given) */
   long *messNo,     /* Number of generated message (returned) */
//   long timeout,     /* Timeout to allow while awaiting reply (given) */
   double  timeout,     /* Timeout (in seconds) to allow while awaiting reply (given) */
   long *amsStat     /* global status (given and returned) */
);

/* amsGetReply - Await and return reply replyNo to message messNo */
int amsGetReply
(
   epicsMessageQueueId repPath, /* Message queue ID of queue for reply (given) */
   long messNo,      /* Message number for which reply required (given) */
   int replyNo,      /* Reply number (given) */
   char * reply,     /* Reply string (returned) */
//   long timeout,     /* Timeout to allow while awaiting reply (given) */
   double timeout,     /* Timeout (in seconds) to allow while awaiting reply (given) */
   long *amsStat     /* global status (given and returned) */
);

/* amsReceive - Await command/parameter string message */
int amsReceive
(
   epicsMessageQueueId cmdPath, /* Message queue ID of queue to listen on (given) */
   char * command,   /* Command string (returned) */
   char * params,    /* Parameters string (returned) */
   long *messNo,     /* Number of received message (returned) */
   int *replies,     /* Number of replies expected (returned) */
   epicsMessageQueueId *repPath,/* Message queue ID of queue for reply (returned) */
//   long timeout,     /* Timeout to allow while awaiting message (given) */
   double timeout,     /* Timeout (in seconds) to allow while awaiting message (given) */
   long *amsStat     /* global status (given and returned) */
);

/* amsReply - Send reply replyNo to message MessNo */
extern int amsReply
(
   epicsMessageQueueId repPath, /* Message queue ID of queue for reply (given) */
   char * reply,     /* Reply string (given) */
   long messNo,      /* Number of message being replied to (given) */
   int replyNo,      /* Number of this reply (given) */
   long repStatus,   /* Supplied reply status (given) */
   long *amsStat     /* global status (given and returned) */
);

#endif    // #ifndef _INCLUDED_AMSLIB_H

