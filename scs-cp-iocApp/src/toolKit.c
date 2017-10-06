/* $Id: toolKit.c,v 1.1 2002/02/05 13:19:52 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * toolKit.c
 * 
 * PURPOSE
 * -------
 * Suite of functions for testing and diagnostics during development of
 * the SCS software
 * 
 * FUNCTION NAME(S)
 * ----------------
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
 * 
 * 22-Feb-1998: Original (srp)
 * 02-Jun-1998: Simulate Lockheed test bootup sequence
 * 10-May-1999: Added RCS id
 *
 */
/* INDENT ON */
/* ===================================================================== */
#include "toolKit.h"
#include "xycom.h"          /* For FLAG_OUTPUTS_REG_ADDR define */
#include "control.h"        /* For writeCommand and for iir_filter,
                                   controller */
#include "guide.h"          /* For setPoint, setPointFree, filter */
#include "testFunctions.h"  /* For showTime */

#include <timeLib.h>        /* For timeNow */
#include <stdio.h> 
#include <string.h> 


/* ============================================ */


int searchMem(long *startPtr, long *stopPtr, long pattern)
{
    long    *indexPtr = NULL;

    for(indexPtr = startPtr; indexPtr < stopPtr; indexPtr++)
    {
        if(*indexPtr == pattern)
        {
            /* printf("Address = %x, Content = %x\n", (long)indexPtr, (long)*indexPtr); */
            printf("Address = %x, Content = %x\n", (int)indexPtr,(int)*indexPtr);
        }
    }
    return(OK);
}

int checkController(void)
{
    int i = 0;
    printf("Controller settings for controller address = %x\n", 
        (int)&controller);

    for(i = 0; i < MAX_AXES; i++)
    {
        switch (i)
        {
        case 0:
            printf("\nX TILT\n");
            break;
        case 1:
            printf("\nY TILT\n");
            break;
        case 2:
            printf("\nFOCUS\n");
            break;
        }
 
        printf("P              Addr = %p, Value = %f\n", 
            &controller[i].P, controller[i].P);
        printf("I              Addr = %p, Value = %f\n", 
            &controller[i].I, controller[i].I);
        printf("D              Addr = %p, Value = %f\n", 
            &controller[i].D, controller[i].D);
        printf("windUpLimit    Addr = %p, Value = %f\n", 
            &controller[i].windUpLimit, controller[i].windUpLimit);
        printf("rateLimit      Addr = %p, Value = %f\n", 
            &controller[i].rateLimit, controller[i].rateLimit);
        printf("oldError       Addr = %p, Value = %f\n", 
            &controller[i].oldError, controller[i].oldError);
        printf("sum            Addr = %p, Value = %f\n", 
            &controller[i].sum, controller[i].sum);
        printf("oldOutput      Addr = %p, Value = %f\n", 
            &controller[i].oldOutput, controller[i].oldOutput);
        printf("oldSum         Addr = %p, Value = %f\n", 
            &controller[i].oldSum, controller[i].oldSum);
    }

    return(OK);
}

int checkSetPoint(void)
{

        /* If you will wait forever, how could you ever return !OK ??? */
        if(semTake(setPointFree, WAIT_FOREVER) != OK)
        {
            return(ERROR);
        }
        else
        {
            printf("setpointaddr = %p\n", &setPoint);
            printf("xTiltA = %f\n", setPoint.xTiltA);
            printf("yTiltA = %f\n", setPoint.yTiltA);
            printf("xTiltB = %f\n", setPoint.xTiltB);
            printf("yTiltB = %f\n", setPoint.yTiltB);
            printf("xTiltC = %f\n", setPoint.xTiltC);
            printf("yTiltC = %f\n", setPoint.yTiltC);
            printf("x pos  = %f\n", setPoint.xPosition);
            printf("y pos  = %f\n", setPoint.yPosition);
            printf("zFocus = %f\n", setPoint.zFocus);

            semGive(setPointFree);
        }
        return(OK);
}


int checkFilterConfig(const int source, const int axis)
{
    int count = 0;

    if(source < 0 || source > (MAX_SOURCES - 1))
    {
        printf("source out of range\n");
        return(ERROR);
    }

    if(axis < 0 || axis > (MAX_AXES - 1))
    {
        printf("axis out of range\n");
        return(ERROR);
    }

    printf("filter[%d] axis[%d] at address = %p\n\n", source, axis, &filter[source][axis]); 

    printf("nb              = %d\n",(int)filter[source][axis].nb);
    printf("na              = %d\n",(int)filter[source][axis].na);

    for(count = 0; count < filter[source][axis].nb; count++)
    {
        printf("num[%d]             = %f\n",count, filter[source][axis].numerator[count]);
    }

    for(count = 0; count < filter[source][axis].na; count++)
    {
        printf("den[%d]             = %f\n", count, filter[source][axis].denominator[count]);
    }

    printf("inHistory ptr   = %p\n",filter[source][axis].inHistory);
    printf("outHistory ptr  = %p\n",filter[source][axis].outHistory);
    printf("weight A        = %f\n",filter[source][axis].weightA);
    printf("weight B        = %f\n",filter[source][axis].weightB);
    printf("weight C        = %f\n",filter[source][axis].weightC);
    printf("type            = %d\n",filter[source][axis].type);
    printf("sampleFreq      = %f\n",filter[source][axis].sampleFreq);
    printf("freq1           = %f\n",filter[source][axis].freq1);
    printf("freq2           = %f\n",filter[source][axis].freq2);

    return(OK);
}

double runFilter(const double input)
{
    return(iir_filter(input, &filter[0][0]));
}

void showMem(void)
{
    showTime();
    memShow(0);
}

void blocking(SEM_ID semId)
{
    int i;
    int idList[10];

    if(semInfo(semId, idList, 10) == OK)
    {
        for(i = 0; i < 10; i++)
        {
            printf("task %d > %x\n", i, idList[i]);
        }
    }
    else
    {
        printf("error on semInfo\n");
    }
}

void togglePort(void)
{
    unsigned char 	*flagOutReg;

    flagOutReg = (unsigned char *)FLAG_OUTPUTS_REG_ADDR;

    if(*flagOutReg > 0)
        *flagOutReg = 0x0;
    else
        *flagOutReg = 0x7f;

    printf("contents flag out reg addr %p = %x\n", (char *)flagOutReg, *flagOutReg);
}

void kickPort(void)
{
    unsigned char 	*flagOutReg;

    flagOutReg = (unsigned char *)FLAG_OUTPUTS_REG_ADDR;

    /* set and clear port to establish baseline for timing */
    for(;;)
    {
        *flagOutReg = 0x7f;
        *flagOutReg = 0x0;
    }
}

void checkTimes(int reps)
{
    int i;
    double  t1, t2;

    typedef struct
    {
        double  a;
        double  b;
        double  c;
        double  d;
    } myStruct;

    myStruct    src, dest;
    myStruct    *srcPtr, *destPtr;

    timeNow(&t1);
    for(i = 0; i < reps; i++)
    {
        *(myStruct *)&dest = *(myStruct *)&src;
    }
    timeNow(&t2);

    printf("slick copy time = %f\n", t2 - t1);

    timeNow(&t1);
    for(i = 0; i < reps; i++)
    {
        dest.a = src.a;
        dest.b = src.b;
        dest.c = src.c;
        dest.d = src.d;

    }
    timeNow(&t2);

    printf("basic copy time = %f\n", t2 - t1);

    srcPtr = &src;
    destPtr = &dest;

    timeNow(&t1);
    for(i = 0; i < reps; i++)
    {
        destPtr->a = srcPtr->a;
        destPtr->b = srcPtr->b;
        destPtr->c = srcPtr->c;
        destPtr->d = srcPtr->d;
    }
    timeNow(&t2);

    printf("pointer copy time = %f\n", t2 - t1);

    srcPtr = &src;
    destPtr = &dest;

    timeNow(&t1);
    for(i = 0; i < reps; i++)
    {
        memcpy(destPtr, srcPtr, sizeof(myStruct));
    }
    timeNow(&t2);

    printf("memcpy copy time = %f\n", t2 - t1);

}


int checkFiles(char* filename)
{
    FILE    *fptr;
    FILTER  myFilter;
    int i;

    if ((fptr = fopen (filename, "rb")) == NULL)
    {
        printf("checkFiles - Unable to open %s\n", filename);
        return(ERROR);
    }
    else
    {
        /* read coefficients from file */

        while(!feof(fptr))
        {
            if (fread ((FILTER *)&myFilter, sizeof (FILTER), 1, fptr) != 1)
                {
                printf("checkFiles - file coefficients read error\n");
            }
            else
            {
                printf("wn1 = %f\n", myFilter.wn1);
                printf("wn1 = %f\n", myFilter.wn2);
                printf("nb  = %d\n", myFilter.nb);
                printf("na  = %d\n", myFilter.na);

                for(i = 0; i < myFilter.nb; i++)
                {
                    printf("Bcoeffs[%d] = %f\n", i, myFilter.Bcoeffs[i]);
                }
                printf("\n");

                for(i = 0; i < myFilter.na; i++)
                {
                    printf("Acoeffs[%d] = %f\n", i, myFilter.Acoeffs[i]);
                }
                printf("\n\n");
            }
        }
        fclose(fptr);
    }
    return(OK);
}

int timeDate(int reps)
{
    double start, finish;
    int i;

    timeNow(&start);

    for(i = 0; i < reps; i++)
    {
    timeNow(&finish);
    }

    printf("start = %f, finish = %f, reps = %d\n", start, finish, reps);
    printf("per rep = %f\n", (finish - start)/reps);

    return(OK);
}

