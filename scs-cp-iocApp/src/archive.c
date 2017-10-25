/*+
 * FILENAME
 * -------- 
 * archive.c
 * 
 * PURPOSE
 * -------
 * Set of functions for logging positions and states of the SCS
 * 
 * FUNCTION NAME(S)
 * ----------------
 * writeArchive - create ring buffers and write new sample
 * readArchive  - print sample for a given time to screen
 * CADlog   - read logging parameters from the engineering screens
 * cadDirLog    - log the time of all CAD directives
 * 
 * DEPENDENCIES
 * ------------
 *
 * LIMITATIONS
 * -----------
 * 
 * AUTHOR
 * ------
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * HISTORY
 * -------
 * 17-Oct-1997: Original (srp)
 * 21-Oct-1997: Bug fix. buffPtr declared incorrectly as *buffPtr
 * 04-Dec-1997: Add cadDirLog function
 * 21-Jan-1998: remove functions logit, fileit, runit
 * 01-Feb-1998: cadDirLog - write parameters to screen as well as directives
 * 02-Feb-1998: replace logMsg with call to errorLog
 * 11-Feb-1998: Add more checks on function return values
 * 23-Feb-1998: make cadDirLog more robust with string length checking
 * 07-May-1999: Added RCS id
 * 06-OCT-2017: Started conversion to EPICS OSI (MDW)
 */

/* ===================================================================== */
#include <stdio.h>


#include <string.h>
#include <stdlib.h>     /* For atoi */

#include <cad.h>
#include <car.h>
#include <timeLib.h>
#include <errlog.h>

#include "archive.h"
#include "control.h"    /* For simLevel, scsBase, m2Ptr, m2MemFree */
#include "utilities.h"  /* For debugLevel */
#include "elgLib.h"

#define BUFFERSIZE   400  /* Number of samples of m2 position to log.
                             Assuming 200Hz update gives 1 second of capture */
#define MAX_LOG_FREQ 200
#define MIN_LOG_FREQ   1
#define MAX_LOG_DUR   60  /* Duration of max logging in seconds */
#define MIN_LOG_DUR    1

#define CAD_INPUT_STRING_SIZE 40

enum                      /* index to define desired log elements */
{
        TILTS = 0,
        GUIDES,
        POSITIONS,
        CMDS,
        ARGS
};

/* Define global variables */

static double newestTime, oldestTime = 1.2345;
static m2History *topPtr = NULL, *endPtr = NULL, *inPtr = NULL;
static m2History archive[BUFFERSIZE];
static epicsMutexId archiveFree = NULL;
/* Indicates that logging has been turned on */
static int loggingArmed = OFF;

static int logChoice = TILTS;
static int startLog, endLog;
static int CADlogging = OFF;

/* declare externals */
int loggingNow = OFF;
int logThreshold = 20;
epicsMutexId refMemFree = NULL;
epicsEventId logNow = NULL;
DBADDR logCAddr;

/* define function prototypes */
static long scsLogInit (char *, int);

/* ===================================================================== */
/*
 * Function name:
 * writeArchive - create ring buffers and write new sample
 * 
 * Purpose:
 * The function checks whether the necessary buffers have been
 * created. If not they are put in place. The current time is
 * read and added to the structure of logged data. The newest
 * and oldest times in the archive are then updated.
 * 
 * Invocation:
 * status = writeArchive (double xTilt, double yTilt, double zFocus);
 * 
 * Parameters in:
 *  > xTilt     double  current x tilt position
 *  > yTilt     double  current y tilt position
 *  > zFocus    double  current z focus position
 * 
 * Parameters out:
 *  None
 * 
 * Return value:
 *  status      int OK or ERROR
 * 
 * Globals: 
 *  External functions:
 *  timeNow
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * None
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 30-Jun-1997: Original(srp)
 * 
 */


/* ===================================================================== */


static int writeArchive (double xTilt, double yTilt, double zFocus, double setX, double setY, double setZ)
{
    double timeStamp;
    m2History *lookAheadPtr = NULL;

    /*
     * if not already present, create mutex semaphore to protect access to
     * the archive
     */

    if (archiveFree == NULL)
    {
       archiveFree = epicsMutexMustCreate();
    }

    epicsMutexMustLock(archiveFree);

    /* capture current timestamp */
    if (timeNow (&timeStamp) != OK)
    {
       errorLog ("writeArchive - error reading timeStamp", 1, ON);
       epicsMutexUnlock(archiveFree);
       return (ERROR);
    }

    /* if first invocation, set pointers to start and end of buffer area */

    if (topPtr == NULL)
    {
    topPtr = (m2History *) archive;
    endPtr = (m2History *) (archive + BUFFERSIZE - 1);
    inPtr = topPtr;

    if (timeNow (&oldestTime) != OK)
    {
        errorLog ("writeArchive - error reading oldestTime", 1, ON);
        epicsMutexUnlock(archiveFree);
        return (ERROR);
    }

    printf ("archive buffer, topPtr = %lx, endPtr = %lx, time = %f\n", (long) topPtr, (long) endPtr, oldestTime);
    }

    /*
     * if the next entry will overwrite the oldest entry, update oldestTime.
     * Beware testing these floats for equality
     */

    if (inPtr->time == oldestTime)
    {
    lookAheadPtr = inPtr + 1;   /* the oldest time will be the next
                     * entry */

    if (lookAheadPtr >= endPtr) /* check for wrapped condition */
        lookAheadPtr = topPtr;

    oldestTime = lookAheadPtr->time;    /* update oldest time */
    }

    /* copy new sample into the archive */

    inPtr->time = timeStamp;
    inPtr->xTilt = xTilt;
    inPtr->yTilt = yTilt;
    inPtr->zFocus = zFocus;
    inPtr->setX = setX;
    inPtr->setY = setY;
    inPtr->setZ = setZ;

    /* point to next free location */

    inPtr++;

    /* update top and bottom limits */

    newestTime = timeStamp;

    if (timeStamp < oldestTime)
    oldestTime = timeStamp;

    /*
     * increment pointer to next free location, wrap back to start if full
     */

    if (inPtr > endPtr)
    inPtr = topPtr;

    epicsMutexUnlock(archiveFree);

    /* add scope for testing */

    if (inPtr < topPtr || inPtr > endPtr)
    {
       errlogPrintf ("writeArchive(): inPtr = %p", inPtr);
    }
    return (OK);
}

/* ===================================================================== */
/*
 * Function name:
 * readArchive  - print sample for a given time to screen 
 * 
 * Purpose:
 * For test purposes this function writes a sample from the log file to
 * screen.
 *
 * Invocation:
 * status  = readArchive (m2History *archiveEntry, double targetTime)
 *
 * Parameters in:
 *  > archiveEntry  *m2History  pointer to structure to hold result
 *  > targetTime    double      time for which sample required
 * 
 * Parameters out:
 *  None
 *
 * Return value:
 *  < status    int OK or ERROR
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 30-Jun-1997: Original(srp)
 * 
 */

/* ===================================================================== */

int readArchive (m2History * archivePtr, double targetTime)
{
    m2History *searchPtr = NULL;

    /* check that an archive exists, if not return NULL pointer */

    if (topPtr == NULL)
    {
    errorLog ("readArchive - archive not present", 1, ON);
    return (ERROR);
    }

    /* check that requested time is within archive range */

    epicsMutexMustLock(archiveFree);
    
    if (targetTime < oldestTime)
    {
        epicsMutexUnlock(archiveFree);
        return (ERROR);
    }

    if (targetTime > newestTime)
        targetTime = newestTime;


    /* search archive for time matching the search time */

    /* start search pointer at last entry, allow for wrapping */

    searchPtr = inPtr - 1;

    if (searchPtr < topPtr)
        searchPtr = endPtr;

    while (searchPtr->time > targetTime)
    {
        searchPtr--;

        if (searchPtr < topPtr)
            searchPtr = endPtr;

	/* as a caution, ensure that this could never be an endless loop */
	if (searchPtr == (inPtr - 1))
	    break;
    }

    epicsMutexUnlock(archiveFree);

    if (searchPtr != NULL)
    {
        *(m2History *) archivePtr = *(m2History *) searchPtr;
        return (OK);
    }
    else
    {
        return (ERROR);
    }
}

/* ===================================================================== */
/*
 * Function name:
 * CADlog   - read logging parameters from the engineering screens
 * 
 * Purpose:
 * Read logging parameters from the engineering screens
 *
 * Invocation:
 * struct cadRecord *pcad
 * status = CADlog(pcad)
 *
 * Parameters in:
 *      > pcad->dir *string CAD directive
 *      > pcad->a   *string logging frequency (1 - 200 Hz)
 *      > pcad->b   *string start time
 *      > pcad->c   *string duration
 *      > pcad->d   *string filename
 *      > pcad->e   *string logging index (0 = tilts, 1 = guides, 2 = positions)
 * 
 * Parameters out:
 *      < pcd->mess string  status message
 *      < pcad->vala    long    23
 *      < pcad->valb    long    frequency
 *      < pcad->valc    long    start time
 *      < pcad->vald    long    duration
 *      < pcad->vale    long    logChoice
 *      < pcad->valf    string  filename
 *
 *
 * Return value:
 *      < status    long
 * 
 * Globals: 
 *  External functions:
 *  None
 * 
 *  External variables:
 *  None
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 30-Jun-1997: Original(srp)
 * 28-Nov-1997: Modify parameters to eliminate start/stop, use DIR instead
 */

/* ===================================================================== */

long CADlog (struct cadRecord * pcad)
{
    long status = CAD_ACCEPT;
    long conversionFlag;
    char buffer[MAX_STRING_SIZE];
    double timeStamp;

    static struct
    {
    int frequency;
    int start;
    int duration;
    int end;
    char name[MAX_STRING_SIZE];
    }   logParams;

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
    break;

    case menuDirectiveCLEAR:
    break;

    case menuDirectivePRESET:

    conversionFlag = sscanf (pcad->a, "%d", &logParams.frequency);

    if (conversionFlag != 1)
    {
        errorLog ("failed logging frequency conversion", 2, ON);
        strncpy (pcad->mess, "freq conversion fail", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }
    else if (logParams.frequency > MAX_LOG_FREQ || logParams.frequency < MIN_LOG_FREQ)
    {
        errorLog ("logging frequency out of range", 2, ON);
        strncpy (pcad->mess, "frequency out of range", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    if (atoi (pcad->b) < 0)
    {
        /* -ve start time means start now */

        if (timeNow (&timeStamp) != OK)
        {
           errlogMessage("CADlog - error reading timeStamp\n");
           return (ERROR);
        }
        logParams.start = (int) timeStamp;
    }
    else
    {
        strncpy (buffer, pcad->b, MAX_STRING_SIZE - 1);

        if ((logParams.start = date2secs (buffer)) < 0)
        {
        errorLog ("CADlog date format not recognised", 2, ON);
        strncpy (pcad->mess, "date format incorrect", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
        }
    }

    if (sscanf (pcad->c, "%d", &logParams.duration) != 1)
    {
        errorLog ("failed logging duration conversion", 2, ON);
        strncpy (pcad->mess, "duration conversion fail", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }
    else if (logParams.duration < 0)
    {
        /* -ve selection means log until stopped, set to max duration */

        logParams.duration = MAX_LOG_DUR;
    }
    else if (logParams.duration > MAX_LOG_DUR || logParams.duration < MIN_LOG_DUR)
    {
        errorLog ("CADlog duration out of range", 2, ON);
        strncpy (pcad->mess, "duration out of range", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
    }

    /* check that choice of parameters to log is within range */

    if (sscanf (pcad->e, "%d", &logChoice) != 1)
    {
        errorLog ("CADlog failed conversion of log choice", 2, ON);
        strncpy (pcad->mess, "logChoice conversion fail", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }
    else if (logChoice < TILTS || logChoice > POSITIONS)
    {
        errorLog ("CADlog - log choice not recognised", 2, ON);
        strncpy (pcad->mess, "logChoice not valid", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    /* calculate logging times */

    startLog = logParams.start;
    logParams.end = logParams.start + logParams.duration;
    endLog = logParams.end;

    strncpy (logParams.name, pcad->d, MAX_STRING_SIZE - 1);
    /*strncpy (logfile, pcad->d, MAX_STRING_SIZE - 1);*/

    printf ("Logging freq = %d, start = %d, duration = %d, end = %d, filename = %s\n",
        logParams.frequency, logParams.start, logParams.duration, logParams.end, logParams.name);

    break;

    case menuDirectiveSTART:

    scsLogInit (logParams.name, logParams.frequency);

    /* write parameters out for checking purposes */

    *(long *) pcad->vala = 23;
    *(long *) pcad->valb = logParams.frequency;
    *(long *) pcad->valc = logParams.start;
    *(long *) pcad->vald = logParams.duration;
    *(long *) pcad->vale = logChoice;
    strncpy (pcad->valf, logParams.name, MAX_STRING_SIZE - 1);

    break;

    case menuDirectiveSTOP:

    /* halt logging */

    scsLogDestruct ();
    break;

    default:
    strncpy (pcad->mess, "log - inappropriate cad directive", MAX_STRING_SIZE - 1);
    status = CAD_REJECT;
    }

    return (status);
}

void showArchive (void)
{
    /* test file to write to and read from the archive */

    /* pointer to structure of positions retrieved */

    m2History archiveEntry;
    double seekTime;

    /* fetch current time to act as timestamp */

    if (timeNow (&seekTime) != OK)
    {
    errlogMessage("showArchive - error reading seekTime\n");
    }

    printf ("topPtr = %lx, endPtr = %lx, oldestTime = %f, newestTime = %f\n", (long) topPtr, (long) endPtr, oldestTime, newestTime);

    if (readArchive (&archiveEntry, seekTime) == OK)
    {
    printf ("x = %f, y = %f, z = %f\n", archiveEntry.xTilt, archiveEntry.yTilt, archiveEntry.zFocus);
    }
    else
    {
    printf ("unable to retrieve positions for time specified, use current position\n");
    }
}

/* ===================================================================== */
/*
 * Function name:
 * scsLogInit   - initialise data logging function
 * 
 * Purpose:
 * Initialise the data logging functions with the filename specified
 * 
 *
 * Invocation:
 * scsLogInit(filename)
 *
 * Parameters in:
 *  string  filename
 * 
 * Parameters out:
 * None
 *
 * Return value:
 *      < status    int OK or ERROR
 * 
 * Globals: 
 *  External functions:
 *  None
 *
 *  External variables:
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 01-Dec-1997: Original(srp)
 * 
 */

/* ===================================================================== */

static long scsLogInit (char *name, int logFreq)
{
    static char filename[MAX_STRING_SIZE];
    long carValue = CAR_BUSY;

    /* initialise elg logging */

    strncpy (filename, name, MAX_STRING_SIZE - 1);
    printf ("scsLogInit: log file = %s\n", filename);

    /* access to the target stream protected by semaphore */

    elgOpen (filename);

    /* write BUSY to logC CAR */

    if (dbPutField (&logCAddr, DBR_LONG, &carValue, 1) != 0)
    {
    errorLog ("scsLogInit - unable to write to logC CAR", 1, ON);
    }

    loggingArmed = ON;

    /* set logging frequency on the auxiliary clock and enable */

    logThreshold = (int) (200.0 / logFreq);

    epicsEventSignal(logNow);

    return (OK);
}

long scsLogDestruct (void)
{
    long carValue = CAR_IDLE;

    /* indicate that logging is over */

    loggingNow = OFF;
    loggingArmed = OFF;

    /* write IDLE to the logC CAR */

    if (dbPutField (&logCAddr, DBR_LONG, &carValue, 1) != 0)
    {
    errorLog ("scsLogDestruct - unable to write to logC CAR", 1, ON);
    }

    /* close logging files */

    elgClose ();

    return (OK);
}


void loggerTask (void)
{
   double timeStamp;
   static char message[81];

   for (;;)
   {
      /* wait for semaphore tick to write log */
      epicsEventMustWait(logNow);
      if (timeNow (&timeStamp) != OK)
      {
         errlogMessage("loggerTask - error reading timeStamp\n");
      }

      if (loggingNow != ON && timeStamp >= startLog && timeStamp <= endLog)
      {
         /* start logging */
         loggingNow = ON;
         errlogPrintf ("logging status = %d\n", loggingNow);
      }

      if (timeStamp > endLog)
      {
         /* halt logging */
         loggingNow = OFF;
         errlogPrintf ("logging status = %d\n", loggingNow);
         scsLogDestruct ();
      } 

      if (loggingNow == ON)
      {
         /* log the message */
         if(simLevel == 0)
         {
            switch (logChoice)
            {
               case TILTS:
                /* fetch parameters from reflective memory */
                 sprintf (message, "%16.6f %+4.2f %+4.2f %+4.2f %+4.2f %+4.2f %+4.2f", timeStamp,
                    scsBase->page0.AxTilt, scsBase->page0.AyTilt, scsBase->page0.zFocusGuide,
                    scsBase->page1.xTilt, scsBase->page1.yTilt, scsBase->page1.zFocus);
                 elgs (TILTS, message);
                 strncpy (message, "blank", 80);
                 break;

               case GUIDES:
                  sprintf (message, "%16.6f %+4.2f %+4.2f %+4.2f", timeStamp,
                       scsBase->page0.xTiltGuide, scsBase->page0.yTiltGuide, scsBase->page0.zFocusGuide);
                  elgs (GUIDES, message);
                  break;

               case POSITIONS:
                  sprintf (message, "%16.6f %+4.2f %+4.2f %+4.2f %+4.2f", timeStamp,
                     scsBase->page0.xDemand, scsBase->page0.yDemand,
                     scsBase->page1.xPosition, scsBase->page1.yPosition);
                  elgs (POSITIONS, message);
                  break;
            
               default:
                  sprintf (message, "%16.6f no items selected", timeStamp);
                  elgs (0, message);
            } /* switch(logChoice) */
         }
         else   /* if (simLevel == 0) */
         {
            /* simulation active */
            switch (logChoice)
            {
               case TILTS:
                  /* fetch parameters from reflective memory */
                  epicsMutexMustLock(m2MemFree);
                  sprintf (message, "%16.6f %+4.2f %+4.2f %+4.2f %+4.2f %+4.2f %+4.2f", timeStamp,
                     m2Ptr->page0.AxTilt, m2Ptr->page0.AyTilt, m2Ptr->page0.zFocusGuide,
                     m2Ptr->page1.xTilt, m2Ptr->page1.yTilt, m2Ptr->page1.zFocus);
                  epicsMutexUnlock(m2MemFree);
                  elgs (TILTS, message);
                  strncpy (message, "blank", 80);
                  break;

               case GUIDES:
                  epicsMutexMustLock(m2MemFree);
                  sprintf (message, "%16.6f %+4.2f %+4.2f %+4.2f", timeStamp,
                     m2Ptr->page0.xTiltGuide, m2Ptr->page0.yTiltGuide, m2Ptr->page0.zFocusGuide);
                  epicsMutexUnlock(m2MemFree);
                  elgs (GUIDES, message);
                  break;

               case POSITIONS:
                  epicsMutexMustLock(m2MemFree);
                  sprintf (message, "%16.6f %+4.2f %+4.2f %+4.2f %+4.2f", timeStamp,
                     m2Ptr->page0.xDemand, m2Ptr->page0.yDemand,
                     m2Ptr->page1.xPosition, m2Ptr->page1.yPosition);
                  epicsMutexUnlock(m2MemFree);
                  elgs (POSITIONS, message);
                  break;

               default:
                  sprintf (message, "%16.6f no items selected", timeStamp);
            } /* switch (logChoice) */
         } /* else if(simLevel == 0) */
      } /* if (loggingNow == ON) */
   } /* for(;;) */
}

/* ===================================================================== */
/*
 * Function name:
 * cadDirLog
 * 
 * Purpose:
 * Log the time and type of each CAD directive
 *
 * Invocation:
 * cadDirLog(cadName, directive)
 *
 * Parameters in:
 *      > cadName   string  name of cad record
 *      > directive string  directive
 * 
 * Parameters out:
 * None
 *
 * Return value:
 *      < status    int OK or ERROR
 * 
 * Globals: 
 *  External functions:
 *  None
 *
 *  External variables:
 *      > debugLevel    long
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 04-Dec-1997: Original(srp)
 * 01-Feb-1998: Write parameters to screen as well as directives
 */

/* ===================================================================== */
int cadDirLog (char *cadName, int directive, int argc, struct cadRecord * pcad)
{
    double timeStamp;
    char message1[50], message2[500], message3[100];
    int i;

    /* define structure of CAD directive names */

    char *dirNames[] =
    {
    "MARK  ", "CLEAR ", "PRESET", "START ", "STOP  ", NULL
    };

    /* define structure of CAD port names */

    char *portNames[] =
    {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", NULL
    };

    if (CADlogging != ON)
    return (OK);

    if (timeNow (&timeStamp) != OK)
    {
       errlogMessage("loggerTask - error reading timeStamp\n");
    return (ERROR);
    }

    /* create message strings of CAD directive and current time */

    /* message 1 length fixed at 44 + terminator */

    sprintf (message1, "%16.6f-%-.20s-%-.6s%c", timeStamp, cadName, dirNames[directive], '\0');

    /* initial message 2 length 17 + terminator */

    sprintf (message2, "%16.6f-%c", timeStamp, '\0');

    /* read input port arguments from pcad structure */
    /* the cad input string size is currently 40 chars */

    i = 0;

    while (i < argc)
    {

    /*
     * read next port and compile message, each port takes up to 43 +
     * forced terminator
     */

    sprintf (message3, "%.1s = %.*s, %c", portNames[i], (MAX_STRING_SIZE - 1), (pcad->a + CAD_INPUT_STRING_SIZE * i), '\0');

    /*
     * check for available space in message2 then concatenate to message
     * 2
     */

    if ((499 - strlen (message2)) > strlen (message3))
    {
        strcat (message2, message3);
    }
    else
    {
        printf ("message 2 full, cannot append next port\n");
        printf ("message 3 > %s\n", message3);
    }

    i++;
    }

    if ((debugLevel > DEBUG_NONE) & (debugLevel <= DEBUG_MED ))
    {
    /* write to screen */

    printf ("%s\n", message1);
    printf ("%s\n", message2);
    }
    return (OK);
}

/*
 * Returns true if logging is ARMED on.
 * Allows replacement of the global loggingArmed. 
 */
int isLoggingArmed(void)
{
  return loggingArmed == ON;
}
