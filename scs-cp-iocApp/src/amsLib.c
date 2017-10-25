/* $Id: amsLib.c,v 1.1 2002/02/05 13:19:46 gemvx Exp $ */
/* amsLib.c - ALICE message system library       */
/* Original by Alan Pickup - ROE - 1997 March 11 */
/*
modification history
--------------------
1997 Apr  2: Functions return int status and long amsStat (dap)
1997 May 13: Non-VxMP version (dap)
1997 May 21: Allow conditional compilation depending on VxMP_ENABLED (dap)
1997 May 29: Tidy up for documenting (dap)
1997 Aug 28: Renamed from amslib to amsLib (dap)
1997 Sep 12: Return status from amsGetReply always OK if reply received (dap)
1997 Oct 24: Set waiting=0 if timeout in amsGetReply (dap)
1999 May  7: Added RCS id
2017 Oct 10: Begin conversion to EPICS OSI (mdw)
             Removed code for VxMP_ENABLED (we don't use it)
*/

#include <string.h>
#include "amsLib.h"

/* Global variables */
int amsInitialised = 0;      /* 1 after initialisation via amsInit */
double amsShortTime;           /* Short timeout */
AMS_DATA amsData;
epicsMutexId amsID;                /* To control access to amsData */
epicsMutexId amsMessID;            /* To control access to amsMessCount */
long amsMessCount;           /* Last message number */
int amsObjType;

/* amsInit - Initialise the ams system */
int amsInit
(
   long *amsStat     /* global status (given and returned) */
)
{
   /* Initialise the ams system - non-VxMP version */
   int i;

   /* If ams has been initialised, delete any message queues */
   if (amsInitialised == 1)
   {
      amsInitialised = 0;
      for (i=0;i<AMS_MAXCHAN;i++)
      {
         if (amsData.amsIndex[i].type == 0)
         {
            epicsMessageQueueDestroy(amsData.amsIndex[i].Q);
         }
      }
   } else
   {
      /* Create the semaphore for control of access to amsData */
      amsID = epicsMutexMustCreate();
   }

   /* Initialised the amsData structure */
   for (i=0;i<AMS_MAXCHAN;i++)
   {
      amsData.amsIndex[i].channel = 0L;
      amsData.amsIndex[i].Q       = NULL;
      amsData.amsIndex[i].type    = 0;
   }

   /* Create amsMessID semaphore if it does not exist */
   if (!amsMessID)
      amsMessID = epicsMutexMustCreate();

   /* Initialise amsMessCount */
   amsMessCount = 0;

   /* Give both semaphores */
   epicsMutexUnlock(amsID);
   epicsMutexUnlock(amsMessID);
      
   // amsShortTime = sysClkRateGet()/10;
   amsShortTime = 0.1;
   amsInitialised = 1;
   *amsStat = AMS__OK;
   errlogPrintf("Non-VxMP version of ams initialised\n");
   return OK;          /* Return success */
}


/* amsAttach - Attach to an existing ams system from a slave processor */
/* Not appropriate in non-VxMP system, but here for compatibility */
int amsAttach
(
   long *amsStat     /* global status (given and returned) */
)
{
   /* Routine just returns error in a non-VxMP environment */
   *amsStat = AMS__NOTAVAIL;
   errlogPrintf("amsAttach> Illegal call in non-VxMP environment\n");
   return ERROR;
}

/* amsConnect - Connect to an ams system from the current task */
int amsConnect
(
   long *amsStat     /* global status (given and returned) */
)
{
   /* Just returns OK when the ams system has been initialised */
   for (;;)
   {
      if (amsInitialised == 1)
      {
         *amsStat = AMS__OK;
         return OK;
      } else
      {
         //taskDelay(10);
         epicsThreadSleep(0.01);
      }
   }
}
/* amsTakeSem - Take semaphore controlling access to ams database */
int amsTakeSem
(
   long *amsStat     /* global status (given and returned) */
)
{
   /* Wait for ams semaphore */
   if (amsInitialised != 1)
   {
      errlogPrintf("amsTakeSem> ams not initialised");
      *amsStat = AMS__NOTINIT;
      return ERROR;
   }
   epicsMutexLock(amsID);
   *amsStat = AMS__OK;
   return OK;
}

/* amsGiveSem - Release semaphore controlling access to ams database */
int amsGiveSem
(
   long *amsStat     /* global status (given and returned) */
)
{
   /* Give the ams semaphore */
   if (amsInitialised != 1)
   {
      errlogPrintf("amsGiveSem> ams not initialised");
      *amsStat = AMS__NOTINIT;
      return ERROR;
   }
   epicsMutexUnlock(amsID);
   *amsStat = AMS__OK;
   return OK;
}

/* amsFind - Looks up channel number and returns position in amsIndex database */
int amsFind
(
   long channel       /* Supplied channel number */
)
{
   /* Return index of channel in amsIndex */
   /* Returns -1 if channel not found */
   /* Must already own ams semaphore */
   int i;

   for (i=0;i<AMS_MAXCHAN;i++)
   {
      if (amsData.amsIndex[i].channel == channel)
      {
         return i;
      }
   }
   return -1;
}
/* amsPath - Returns message queue ID for given channel number */
int amsPath
(
   long channel,                /* Channel number (given) */
   epicsMessageQueueId *path,   /* Message queue ID (returned) */
   long *amsStat                /* global status (given and returned) */
)
{
   /* Return message queue ID for channel */
   int i;
   long retval;
   
   /* Take the ams semaphore */
   if (amsTakeSem(amsStat) == ERROR)
   {
      /* Failed to take semaphore - abort */
      errlogPrintf("amsPath> aborting");
      return ERROR;
   }
   
   i = amsFind(channel);

   if (i >= 0)
   {
      *path = amsData.amsIndex[i].Q;
      retval = AMS__OK;
   } else
   {
      *path = NULL;
      retval = AMS__NOCHAN;
   }

   /* Give back the semaphore */
   if (amsGiveSem(amsStat) == ERROR)
   {
      return ERROR;
   } else
   {
      *amsStat = retval;
      if (*amsStat == AMS__OK)
      {
         return OK;
      } else
      {
         return ERROR;
      }
   }
}
/* amsCreate - Creates message queue of given type and size */
int amsCreate
(
   long channel,     /* channel number (given) */
   int type,         /* Channel type (AMS_CMD/REP/SMCMD/SMREP) (given) */
   long size,        /* Size of message queue (given) */
   epicsMessageQueueId *path,   /* Meassage queue ID (returned) */
   long *amsStat     /* global status (given and returned) */
)
{
   /* Create a channel */
   
   long retval;
   int i;
   int allocated; /* 1 when channel allocated, else 0 */
   int ownSem;    /* ownership of ams semaphore (0 = no; 1=yes) */
   int status;
   
   /* Initialise amsStat to AMS__OK */
   *amsStat = AMS__OK;
   ownSem = 0;
   if (amsInitialised != 1)
   {
      /* ams not initialised */
      *amsStat = AMS__NOTINIT;
   } else if (channel <= 0L)
   {
      /* invalid channel number */
      *amsStat = AMS__INVCHAN;
   } else if ((type != AMS_CMD) & (type != AMS_REP))
   {
      /* invalid channel type */
      *amsStat = AMS__INVTYPE;
   } else if (size <= 0L)
   {
      /* invalid queue size */
      *amsStat = AMS__INVQSIZE;
   } else
   {
      /* Get the ams semaphore */
      status = amsTakeSem(amsStat);
   }

   if (*amsStat == AMS__OK)
   {
      ownSem = 1;
      allocated = 0;
      /* Check for dupicate channel number */
      i = amsFind(channel);
      if (i >= 0)
      {
         /* Channel already exists */
         if (amsData.amsIndex[i].type != type)
         {
            /* Can't change type of existing channel */
            *amsStat = AMS__CHANFIXED;
         } else
         {
            *path = amsData.amsIndex[i].Q;
            allocated = 1;
         }
      }
   }
   
   if ((*amsStat == AMS__OK) && (allocated == 0))
   {
      /* Allocate channel number to first free slot
        in ams database */
      for (i=0;i<AMS_MAXCHAN;i++)
      {
         if ((allocated == 0) & (amsData.amsIndex[i].channel == 0L))
         {
            amsData.amsIndex[i].channel = channel;
            /* Create message queue of required type */
            if (type == AMS_CMD)
            {
               amsData.amsIndex[i].Q = 
                        epicsMessageQueueCreate(size, AMS_CMDSIZE);
            } else  /* type is AMS_REP */
            {
               amsData.amsIndex[i].Q = 
                        epicsMessageQueueCreate(size, AMS_REPSIZE);
            }
            if (amsData.amsIndex[i].Q == NULL)
            {
               /* Message queue creation failed */
               errlogPrintf("amsCreate> Failed to create message queue\n");
               *amsStat = AMS__MQCREFAIL;
               /* Deallocate the channel */
               amsData.amsIndex[i].channel = 0L;
            } else
            {
               allocated = 1;
               amsData.amsIndex[i].type = type;
               *path = amsData.amsIndex[i].Q;
             }
          }
      }
      if ((*amsStat == AMS__OK) & (allocated == 0))
      {
         /* Out of channels */
         *amsStat = AMS__FULL;
      }
   }
   /* If the semaphore was taken, give it back */
   if (ownSem == 1)
   {
      retval = *amsStat;
      if (amsGiveSem(amsStat) == OK)
      {
         *amsStat = retval;
      }
   }
   if (*amsStat == AMS__OK)
   {
      return OK;
   } else
   {
      return ERROR;
   }
}
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
)
{

   /* Send command */

   struct amsCmd outgoing;

   *amsStat = AMS__OK;

   /* Compose message */
   /* Increment amsMessCount which is protected by amsMessID semaphore */
   epicsMutexLock(amsMessID);

   if (amsMessCount == 1000000L)
   {
      amsMessCount = 1;
   } else
   {
      amsMessCount++;
   }
   *messNo = amsMessCount;

   epicsMutexUnlock(amsMessID);

   outgoing.messNo = *messNo;
   outgoing.replies = replies;
   outgoing.Q = repPath;
   strcpy(outgoing.command,command);
   strcpy(outgoing.params,params);

   /* Send message */
   if (epicsMessageQueueSendWithTimeout(path,(char *) &outgoing,AMS_CMDSIZE,
     amsShortTime) == ERROR)
     {
        *amsStat = AMS__TIMEOUT;
     }
      
   if (*amsStat == AMS__OK)
   {
      return OK;
   } else
   {
      return ERROR;
   }
}



/* amsSend - Send a message; don't await receipt or reply */
int amsSend
(
   epicsMessageQueueId path,    /* Message queue ID of queue to send on (given) */
   char * command,   /* Command string (given) */
   char * params,    /* Parameters string (given) */
   long *messNo,     /* Number of generated message (returned) */
   long *amsStat     /* global status (given and returned) */
)
{
   /* Send a message - don't request receipt or replies */

   return (amsSend0(path,command,params,0,messNo,NULL,amsStat));
}

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
)
{
   /* Send a message requiring a receipt/reply, but don't wait for receipt */
   int retval;

   if (replies < 1)
   {
      /* Invalid use - replies must be 1 or more */
      *amsStat = AMS__INVREPLIES;
      retval = ERROR;
   } else if (repPath == NULL)
   {
      /* Invalid reply path */
      *amsStat = AMS__INVREPPATH;
      retval = ERROR;
   } else
   {
      /* Send command */
      retval = amsSend0(path,command,params,replies,messNo,repPath,amsStat);
   }

   return retval;
}
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
)
{
   /* Send a message and wait for first reply = receipt */
   int retval;
   /*struct amsRep incoming;*/
   char reply[AMS_REPLEN];

   if (amsCommand(path,command,params,replies,repPath,messNo,
     amsStat) == ERROR)
   {
      retval = ERROR;
   } else
   {
      /* Wait for receipt (replyNo = 1) */
      retval = amsGetReply(repPath,*messNo,1,reply,timeout,amsStat);
   }

   return retval;
}
/* amsGetReply - Await and return reply replyNo to message messNo */
int amsGetReply
(
   epicsMessageQueueId repPath, /* Message queue ID of queue for reply (given) */
   long messNo,      /* Message number for which reply required (given) */
   int replyNo,      /* Reply number (given) */
   char * reply,     /* Reply string (returned) */
//   long timeout,     /* Timeout to allow while awaiting reply (given) */
   double timeout,     /* Timeout (in seconds)to allow while awaiting reply (given) */
   long *amsStat     /* global status (given and returned) */
)
{
   /* Wait for reply */
   int waiting;
   //ULONG tickStart;
   //ULONG tickNow;
   //ULONG tickWait;
   epicsTimeStamp timeStart;
   epicsTimeStamp timeNow;
   epicsTimeStamp timeWait;

   int istat;
   struct amsRep incoming;

   *amsStat = AMS__OK;
   istat = OK;
   waiting = 1;

   epicsTimeGetCurrent(&timeStart);
   timeWait = timeStart;

   // tickStart = tickGet();
   // tickWait = tickStart;

   while (waiting == 1)
   {
//      if (msgQReceive(repPath,(char *) &incoming,AMS_REPSIZE,timeout) == ERROR)
      if (epicsMessageQueueReceiveWithTimeout(repPath,&incoming,AMS_REPSIZE,timeout) == ERROR)
      {
         *amsStat = AMS__TIMEOUT;
         istat = ERROR;
         waiting = 0;
      } else
      {
         /* If this is not the expected reply, push it back on the message queue
            and keep waiting */
         if ((messNo != incoming.messNo) || (replyNo != incoming.replyNo))
         {
            /* Not the expected reply so we need to push it back on the queue.
               However, every two ticks, delay for one tick to prevent
               this task locking up the processor. */

//            tickNow = tickGet();
//            if ((tickNow-tickWait) > 2)
//            {
//               taskDelay(1);
//               tickNow = tickGet();
//            }
            /* the following assumes that a "tick" is 1/80 of a second */
            epicsTimeGetCurrent(&timeNow);
            if( epicsTimeDiffInSeconds(&timeNow, &timeWait) > 0.025) {
               epicsThreadSleep(0.0125);
               epicsTimeGetCurrent(&timeNow);
            }

            /* Push the message back on the queue */
//            if (msgQSend(repPath,(char *) &incoming,AMS_REPSIZE,timeout, MSG_PRI_NORMAL) == ERROR)
            if (epicsMessageQueueSendWithTimeout(repPath,&incoming,AMS_REPSIZE,timeout) == ERROR)
            {
               *amsStat = AMS__MQSENDFAIL;
               istat = ERROR;
               waiting = 0;
            } else
            {
               /* Have we waited too long? */
               //if ((timeout != WAIT_FOREVER) && ((tickNow-tickStart)>timeout))
               if ((timeout != WAIT_FOREVER) && (epicsTimeDiffInSeconds(&timeNow, &timeStart)>timeout))
               {
                  *amsStat = AMS__TIMEOUT;
                  istat = ERROR;
                  waiting = 0;
               }
            }
         } else
         {
            *amsStat = incoming.repStatus;
            if (replyNo != 1)
            {
               /* Copy and return the reply string */
               /* Reply 1 = receipt is empty so don't copy */
               strcpy(reply,incoming.rep);
            }
            waiting = 0;
         }
      }
   }
   return istat;
}
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
   double  timeout,     /* Timeout (in seconds)to allow while awaiting message (given) */
   long *amsStat     /* global status (given and returned) */
)
{
   /* Wait for command and parameter strings on cmdPath */
   int retval;
   int istat;
   struct amsCmd incoming;

   /* Await message */
//   istat = msgQReceive(cmdPath,(char *) &incoming,AMS_CMDSIZE,timeout);
   istat = epicsMessageQueueReceiveWithTimeout(cmdPath,&incoming,AMS_CMDSIZE,timeout);
   if (istat != ERROR)
   {

      *amsStat = AMS__OK;
      *messNo  = incoming.messNo;
      *replies = incoming.replies;
      *repPath = incoming.Q;
      strcpy(command,incoming.command);
      strcpy(params,incoming.params);
      /* Return receipt if required */
      if (incoming.Q != NULL)
      {
         /* Need to acknowledge receipt using replyNo of 1 and blank reply string*/
         retval = amsReply(incoming.Q," ",incoming.messNo,
            1,AMS__OK,amsStat);
      } 
   } else
         *amsStat = AMS__TIMEOUT;
// 
//   {
//      if (errno == S_objLib_OBJ_TIMEOUT)
//      {
//         *amsStat = AMS__TIMEOUT;
//       } else
//      {
//         *amsStat = AMS__ERROR;
//       }
//   }

   if (*amsStat == AMS__OK)
   {
      return OK;
   } else
   {
      return ERROR;
   }
}
/* amsReply - Send reply replyNo to message MessNo */
int amsReply
(
   epicsMessageQueueId repPath, /* Message queue ID of queue for reply (given) */
   char * reply,     /* Reply string (given) */
   long messNo,      /* Number of message being replied to (given) */
   int replyNo,      /* Number of this reply (given) */
   long repStatus,   /* Supplied reply status (given) */
   long *amsStat     /* global status (given and returned) */
)
{
   struct amsRep rep;

   /* Compose reply structure */
   rep.messNo = messNo;
   rep.replyNo = replyNo;
   rep.repStatus = repStatus;
   strcpy(rep.rep,reply);
   /* Send reply */
//   if (msgQSend(repPath,(char *) &rep,AMS_REPSIZE, amsShortTime,MSG_PRI_NORMAL) == ERROR)
   if (epicsMessageQueueSendWithTimeout(repPath,&rep,AMS_REPSIZE, amsShortTime) == ERROR)
   {
      *amsStat = AMS__MQSENDFAIL;
      return ERROR;
   } else
   {
      *amsStat = AMS__OK;
      return OK;
   }
}

