/* $Id: elgLib.c,v 1.1 2002/02/05 13:19:48 gemvx Exp $ */
/* elgLib.c - Edict logging system library       */
/* Original as edloglib by Alan Pickup - ROE - 1997 May 8 */
/*
modification history
--------------------
1997 May  8: Original (dap)
1997 May 16: Renamed elglib (dap)
1997 May 21: Conditional compilation via VxMP_ENABLED (dap)
1997 Aug 28: Renamed from elglib to elgLib (dap)
1997 Aug 28: Added elgi and elgf (dap)
1999 May  7: Added RCS id

*/


/* Hash include (which includes vxworks.h,... etc) */
#include "elgLib.h"

#include <stdio.h>
#include <time.h>
#include <string.h>


#include "tickLib.h"
#include "amsLib.h"

/* Global variables */
int elgInitialised  = 0; /* 1 after initialisation via elgInit */
int elgInitialising = 0; /* 1 during initialisation */
MSG_Q_ID elgQ = NULL;    /* for messages to the elogger task */

/* elg - Enter a message into the log as from module 0*/
STATUS elg
(
char *message     /* Message text (given) */
)
{
   char msg[AMS_PARLEN];

   if (message == NULL)
   {
      printf("Supply log msg> ");
      gets(msg);
   } else
   {
      strcpy(msg,message);
   }

   /* Send message (uses module number of 0) */
   return (elgPut(0,"log",msg));
}
/* elgs - Enter a character string from given module into the log */
STATUS elgs
(
   int module,    /* Module number prefixes message in log (given) */
   char * message /* Message text (given) */
)
{
   /* Write the supplied message to the log as from module */
   return (elgPut(module,"log",message));
}
/* elgi - Enter string and integer value from given module into log */
STATUS elgi
(
   int module,    /* Module number prefixes message in log (given) */
   char * message,/* Message text (given) */
   int ivalue     /* Integer value appended to message (given) */
)
{
   /* Write message and integer ivalue to log as from module */
   char msg[AMS_PARLEN];
   sprintf(msg,"%s %i",message,ivalue);
   return (elgPut(module,"log",msg));
}
/* elgl - Enter string and long value from given module into log */
STATUS elgl
(
   int module,    /* Module number prefixes message in log (given) */
   char * message,/* Message text (given) */
   long lvalue    /* Long value appended to message (given) */
)
{
   /* Write message and long lvalue to log as from module */
   char msg[AMS_PARLEN];
   sprintf(msg,"%s %ld",message,lvalue);
   return (elgPut(module,"log",msg));
}
/* elgf - Enter string and float value from given module into log */
STATUS elgf
(
   int module,    /* Module number prefixes message in log (given) */
   char * message,/* Message text (given) */
   float fvalue   /* Float value appended to message (given) */
)
{
   /* Write message and float fvalue to log as from module */
   char msg[AMS_PARLEN];
   sprintf(msg,"%s %f",message,fvalue);
   return (elgPut(module,"log",msg));
}
/* elgd - Enter string and double value from given module into log */
STATUS elgd
(
   int module,    /* Module number prefixes message in log (given) */
   char * message,/* Message text (given) */
   double dvalue  /* Double value appended to message (given) */
)
{
   /* Write message and double dvalue to log as from module */
   char msg[AMS_PARLEN];
   sprintf(msg,"%s %f",message,dvalue);
   return (elgPut(module,"log",msg));
}
/* elgOpen - Open logging file */
STATUS elgOpen
(
   char * filename   /* Filename to open (given) */
)
{
   /* Open a logging file of the given name */
   return (elgPut(0,"open",filename));
}
/* elgFlush - Flush the current logging file to disk */
STATUS elgFlush()
{
   /* Flush the logging file to disk */
   return (elgPut(0,"flush"," "));
}
/* elgReport - Report status of logging system to console */
STATUS elgReport()
{
   /* Report the status of the logging and logging message queue */
   return (elgPut(0,"report"," "));
}
/* elgClose - Close the current logging file */
STATUS elgClose()
{
   /* Close the current logging file */
   return (elgPut(0,"close"," "));
}
/* elgDone - Close any curreny logging file and delete the logging task */
STATUS elgDone()
{
   /* Close any open logging file and delete the logging task */
   return (elgPut(0,"done"," "));
}
/* elgPut - Write command, module number and mesage to logging queue */
STATUS elgPut
(
   int module,    /* Module number prefixes message in log (given) */ 
   char * command,/* Command to logging task (eg "log") (given) */
   char * message /* Message string (given) */
)
{
   /* Queue the command and message to the logging task as from module */
   long amsStat;
   long messNo;
   char msg[AMS_PARLEN];
   time_t now;

   /* Write a message to the logging queue */

   /* Error if queue not initialised */
   if (elgQ == NULL)
   {
      printf("elgPut> logging queue not initialised\n");
      return ERROR;
   } else
   {
      /* If command is "log", prefix message with module number 
         and timestamp */
      if (!strcmp(command,"log"))
      {
         now = time(NULL);
         sprintf(msg,"%3i ",module);
         strncpy(&msg[5],&asctime(localtime(&now))[11],8);
         strcpy(&msg[13]," ");
         sprintf(msg,"%s %lu  %s",msg,tickGet(),message);

      } else
      {
         strcpy(msg,message);
      }

      /* Place the command and message on the logging queue */
      if (amsSend(elgQ,command,msg,&messNo,&amsStat) != OK)
      {
         printf("elgPut> Failed to log message. Error = %ld\n",
            amsStat);
         printf("elgPut> message was: %s\n",message);
         return ERROR;
      }
   }
   return OK;
}
/* elgInit - Initialise the elg system */
STATUS elgInit (void)
{
   /* Initialise the logging system */

   /* If the elg system is already initialised, do nothing */
   if (elgInitialised == 1)
   {
      printf("elgInit> logging already initialised\n");
      return OK;
   }

   /* Set flag to indicate initialising */
   elgInitialising = 1;

   /* Spawn elogger */
   if (taskSpawn("elogger",ELG_PRIORITY,0,20000,(FUNCPTR)elogger,
      0,0,0,0,0,0,0,0,0,0) == ERROR)
   {
      printf("elgInit> could not spawn elogger\n");
      return ERROR;
   }

   /* Wait for the elg system to exist */
   if (elgConnect() != OK)
   {
      printf("elgInit> Could not connect to elg logging system\n");
      return ERROR;
   } else
   {
      return OK;
   }
}               
#ifdef VxMP_ENABLED
/* elgAttach - Attach to the elg logging system from a secondary processor */
STATUS elgAttach (void)
{
   /* Attach to the elg logging system from a secondary processor */
   MSG_Q_ID path;
   long amsStat;

   /* If the elg system is already initialised, do nothing */
   if (elgInitialised == 1)
   {
      printf("elgAttach> logging already initialised\n");
      return OK;
   }

   if (elgInitialising == 0)
   {
      elgInitialising = 1;
      /* Need to connect to the ams system from this processor */
      if (amsAttach(&amsStat) != OK)
      {
         printf("elgAttach> Failed to attach to ams system\n");
         printf("elgAttach> amsStat = %d\n",amsStat);
         return ERROR;
      }
      /* Get the ID for the logging message queue and copy 
         to elgQ in global memory */
      if (amsPath(ELG_CHANNEL,&path,&amsStat) != OK)
      {
         printf("elgAttach> Failed to get path for logging queue\n");
         printf("elgAttach> amsStat = %d\n",amsStat);
         return ERROR;
      } else
      {

         elgQ = path;
      }

      /* Set elgInitialised to 1 */
      elgInitialised  = 1;
      elgInitialising = 0;

      return OK;
   } else
   {
      /* Another task is handling the initialisation */
      /* Wait for the edlogInitialised flag to be set */
      for (;;)
      {
         if (elgInitialised == 1)
         {
            return OK;
         } else
         {
            taskDelay(10);
         }
      }
   }
}               
#else
STATUS elgAttach (void)
{
   /* Routine just returns error in a non-VxMP environment */
   printf("elgAttach> Illegal call in non-VxMP environment\n");
   return ERROR;
}
#endif
/* elgConnect - Wait until the elg system is initialised on this processor */
STATUS elgConnect (void)
{
   /* Wait until the elg system has been initialised */

   for (;;)
   {
      if (elgInitialised == 1)
      {
         return OK;
      } else
      {
         taskDelay(10);
      }
   }
}
/* elogger - Logging task */
STATUS elogger (void)
{
   /* The function executed by the logging task */
   /* Creates the message queue for logging messages and
      acts on messages it receives */
   MSG_Q_ID repPath;
   long amsStat;
   long messNo;
   long lineNo;
   int numMsgs;
   int maxNumMsgs;
   int replies;
   int flushing;
   char command[AMS_CMDLEN];
   char params[AMS_PARLEN];
   char filename[AMS_CMDLEN];
   FILE *elgFp;

   printf("elogger> Logging task activated\n");

   /* If required, initialise the ams system */
   if (amsInitialised == 0)
   {
      if (amsInit(&amsStat) == ERROR)
      {
         printf("elgInit> Failed to initialise; amsStat= %ld\n",amsStat);
         return ERROR;
      }
   }
   /* Create channel for elg messages */
   if (amsCreate(ELG_CHANNEL,ELG_TYPE,ELG_SIZE,
       &elgQ,&amsStat) == ERROR)
   {
      printf("elgInit> Failed to create channel %ld; amsStat= %ld\n",
         ELG_CHANNEL,amsStat);
      return ERROR;
   } else
   {
      /* Flush the queue of any old messages */
      flushing = 1;
      while (flushing == 1)
      {
         if (amsReceive(elgQ,command,params,&messNo,&replies,
            &repPath,NO_WAIT,&amsStat) != OK)
         {
            flushing = 0;
         }
      }
   }         

   /* Initialise the file pointer to be used */
   elgFp = NULL;

   /* Initialise lineNo and maxNumMsgs */
   lineNo = 0L;
   maxNumMsgs = 0;

   /* Declare the elg system initialised */
   elgInitialised  = 1;
   elgInitialising = 0;

   /* Now wait for messages to arrive */
   for (;;)
   {

      if (amsReceive(elgQ,command,params,&messNo,&replies,
         &repPath,WAIT_FOREVER,&amsStat) != OK)
      {
         printf("elogger> Failure on amsReceive\n");
      } else
      {
         /* Update record of maximum number of messages on queue */
         numMsgs = msgQNumMsgs(elgQ)+1;
         if (numMsgs > maxNumMsgs) maxNumMsgs = numMsgs;
         if (!strcmp(command,"open"))
         {
            if (elgFp != NULL)
            {
               /* Close existing file */
               if (fclose(elgFp) == EOF)
               {
                  printf("elogger> Failed to close file %s\n",filename);
               } else
               {
                  printf("elogger> Logging terminated to file %s\n",
                     filename);
                }
               elgFp = NULL;
             } 
            strcpy(filename,params);
            elgFp = fopen(filename,"w");
            if (elgFp == NULL)
            {
               printf("elogger> Failed to open file %s\n",filename);
            } else
            {
            printf("elogger> Logging enabled to file %s\n",filename);
             }
            lineNo = 0L;
         } else if (!strcmp(command,"log"))
         {
            /* Write message to file */
            if (elgFp != NULL)
            {
               fprintf(elgFp,"%s\n",params);
               lineNo++;
            } else
            {
               printf("elogger> Logging not enabled\n");
               printf("elogger> Message lost: %s\n",params);
            }
         } else if (!strcmp(command,"flush"))
         {
            /* Flush the current file to disk */
            /* Do this by closing the file and reopening for append */
            if (elgFp != NULL)
            {
               if (fclose(elgFp) != OK)
               {
                  printf("elogger> Failed to close file while flushing\n");
                  elgFp = NULL;
               } else
               {
                  /* Reopen file for append */
                  elgFp= fopen(filename,"a");
                  if (elgFp == NULL)
                  {
                     printf("elogger> Failed to reopen file while flushing\n");
                  }
               }
               if (elgFp != NULL)
               {
                  printf("elogger> File %s flushed OK\n",filename);
               } else
               {
                  printf("elogger> Flush failed - no file open\n");
               }
            } else
            {
               printf("elogger> Logging not enabled - cannot flush\n");
            }
         } else if (!strcmp(command,"report"))
         {
            /* Report the state of the logger */
            if (elgFp == NULL)
            {
               printf("elogger report> No file open\n");
            } else
            {
               printf("elogger report> File open = %s\n",filename);
               printf("                %ld lines written\n",lineNo);
            }
            printf("                %i messages currently on logging queue\n",
               numMsgs-1);
            printf("                Maximum of %i messages on logging queue\n",
               maxNumMsgs);
            printf("                Version of ams message system: %s\n",
               AMS_VERSION);
            printf("                Version of elg logging system: %s\n",
               ELG_VERSION);
#ifdef VxMP_ENABLED
            printf("                Built for shared operation under VxMP\n");
#else
            printf("                Built for operation on single processor (non-VxMP)\n");
#endif
         } else if (!strcmp(command,"close"))
         {
            /* Close file */
            if (elgFp != NULL)
            {
               if (fclose(elgFp) != OK)
               {
                  printf("elogger> Failed to close file\n");
               } else
               {
                  printf("elogger> Logging terminated to file %s\n",
                     filename);
               }
               elgFp = NULL;
            }
         } else if (!strcmp(command,"done"))
         {
            /* Exit after closing the file if one is open */
            if (elgFp != NULL)
            {
               if (fclose(elgFp) == EOF)
               {
                  printf("elogger> Failed to close file\n");
               } else
               {
                  printf("elogger> Logging terminated to file %s\n",
                     filename);
               }
               elgFp = NULL;
            }
            elgInitialised = 0;
            printf("elogger> Logging task terminated\n");
            return OK;
         }
      }
   }
}








