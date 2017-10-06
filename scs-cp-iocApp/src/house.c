/* $Id: house.c,v 1.1 2002/02/05 13:19:49 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * house.c 
 * 
 * PURPOSE
 * -------
 * Functions supporting CAD records for the TCS/engineering screen interface
 * 
 * FUNCTION NAME(S)
 * ----------------
 * CADtest      - initiate test function
 * CADinit      - initiate initialisation
 * CADinitXY    - initiate XY positioner initialisation
 * CADreboot        - reboot the whole IOC
 * CADdatum     - datum the system, not necessary therefore dummy only
 * CADsimulate      - set the simulation level for the system
 * carInit      - INAM routine for carDrive
 * carDrive1        - OR together all CAR outputs to derive activeC CAR
 * carDrive2        - OR together all CAR outputs to derive activeC CAR
 * carDrive3        - OR together all CAR outputs to derive activeC CAR
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
 * 17-Oct-1997: Original (srp)
 * 24-Feb-1998: Add carInit and carDrive from testFunctions.c
 * 11-Mar-1998: Include checks for CAR_PAUSED in carDrive
 * 24-Jun-1998: Modify carDrive to give preference to BUSY and reflect
 *      indicate status of most recently changed CAR. Maintaining
 *      history requires splitting to one per genSub
 *  7-May-1999: Added RCS id
 *  9-Feb-2001: Added CAD initXY to XY positioner initialization
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include "house.h"
#include "archive.h"        /* For cadDirLog */
#include "control.h"            /* For simLevel, interlockFlag */
#include "utilities.h"      /* For errorLog */

#include <tcslib.h>

#include <cad.h>
#include <car.h>

#include <string.h>
#include <stdio.h>

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADtest
 * 
 * Purpose:
 * The test command instructs the SCS to begin a test of itself and its
 * subsystems. Only the mark, preset  and start directives are appropriate
 * for the test CAD record all other directives are rejected.
 * There are no arguments to check and the command may be invoked from any
 * state.
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADtest(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 * 
 * Parameters out:
 *      < pcd->mess *string status message
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
 * 24-Jul-1996: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    CADtest (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;

    cadDirLog ("test", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* trigger forward link */
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }
        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "Inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADinit
 * 
 * Purpose:
 * The initialisation command causes the SCS to load any configuration
 * data and set itself to the state at boot up. When invoked this
 * routine provides a trigger for SNL code
 * Only mark and start directives are appropriate for the init CAD record
 * all other directives are rejected. There are no arguments to check
 * 
 * Invocation:
 * struct cadRecord *pcad
 *status = CADinit(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 * 
 * Parameters out:
 *      < pcd->mess *string status message
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
 * 24-Jul-1996: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    CADinit (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;

    cadDirLog ("init", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* trigger forward link */
            puts ("init CAD start");
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "init -Inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADinitXY
 * 
 * Purpose:
 * The initialisation command causes the SCS to load any configuration
 * data and set itself to the state at boot up. When invoked this
 * routine provides a trigger for SNL code
 * Only mark and start directives are appropriate for the init CAD record
 * all other directives are rejected. There are no arguments to check
 * 
 * Invocation:
 * struct cadRecord *pcad
 *status = CADinitXY(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 * 
 * Parameters out:
 *      < pcd->mess *string status message
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
 * 24-Jul-1996: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    CADinitXY (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;

    cadDirLog ("init", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* trigger forward link */
            puts ("initXY CAD start");
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "initXY -Inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADreboot
 * 
 * Purpose:
 * The execution of the reboot command causes a vxWorks reboot command
 * to be invoked
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADreboot(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 * 
 * Parameters out:
 *      < pcd->mess *string status message
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
 * 24-Jul-1996: Original(srp)
 * 21-May-1997: Rename function and all references to 'reboot'
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    CADreboot (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;

    cadDirLog ("reboot", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* trigger forward link */
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADdatum
 * 
 * Purpose:
 * The datum command causes the scs and its subsystems to datum. This
 * function is included for compliance with ICD 7b although in practice
 * the SCS does not need to datum before operation.
 * Only mark and start directives are appropriate for the datum CAD record
 * all other directives are rejected. There are no arguments to check
 * 
 * Invocation:
 *  struct cadRecord *pcad
 *  status = CADdatum(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 * 
 * Parameters out:
 *      < pcd->mess *string status message
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
 * 21-May-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    CADdatum (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;

    cadDirLog ("datum", pcad->dir, 0, pcad);

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            /* trigger forward link */
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * CADsimulate
 * 
 * Purpose:
 * This command allows the setting of the SCS simulation to one of
 * NONE, VSM, FAST or FULL. In practise only none and full are implemented
 * 
 * Invocation:
 * struct cadRecord *pcad
 * status = CADsimulate(pcad)
 * 
 * Parameters in:
 *      > pcad->dir *string CAD directive
 *      > pcad->a   *string simulation level
 * 
 * Parameters out:
 *      < pcd->mess *string status message
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
 * 1-Feb-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

long    CADsimulate (struct cadRecord * pcad)
{
    long    status = CAD_ACCEPT;
    static int simBuffer;
        static char *simOpts[]= {"NONE", "VSM", "FAST", "FULL", NULL} ;

    cadDirLog ("simulate", pcad->dir, 1, pcad);

    /* Fetch name of cad for messages */
    tcsCsSetMessageN (pcad, tcsCsCadName(pcad), ": ", (char*)NULL) ;

    switch (pcad->dir)
    {
    case menuDirectiveMARK:
        break;

    case menuDirectiveCLEAR:
        break;

    case menuDirectivePRESET:

                status = CAD_REJECT ;

                if (pcad->a[0])
        {
                  if (tcsDcString (simOpts, "level - ", pcad->a, &simBuffer, pcad) )
          {
              errorLog ("CADsimulate - unrecognised sim selection", 2, ON);
              break ;
                  }
                }
        else
        {
            tcsCsAppendMessage(pcad, "no level specified");
            break ;
                }

                status = CAD_ACCEPT ;
        break;

    case menuDirectiveSTART:

        if (interlockFlag != 1)
        {
            simLevel = simBuffer;
            strcpy(pcad->vala, simOpts[simBuffer]);
        }
        else
        {
            strncpy (pcad->mess, "interlocks active", MAX_STRING_SIZE - 1);
            status = CAD_REJECT;
        }

        break;

    case menuDirectiveSTOP:
        break;

    default:
        strncpy (pcad->mess, "inappropriate CAD directive", MAX_STRING_SIZE - 1);
        status = CAD_REJECT;
        break;
    }

    return (status);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * carInit
 * carDrive1, carDrive2, carDrive3
 * 
 * Purpose:
 * Or together the individual CAR outputs to drive the activeC CAR
 * 
 * Invocation:
 * 
 * Parameters in:
 *      > pgsub->dir    *string CAD directive
 *      > pgsub->a  long    CAR value
 *      > pgsub->b  long    CAR value
 *      > pgsub->c  long    CAR value
 *      > pgsub->d  long    CAR value
 *      > pgsub->e  long    CAR value
 *      > pgsub->f  long    CAR value
 *      > pgsub->g  long    CAR value
 *      > pgsub->h  long    CAR value
 *      > pgsub->i  long    CAR value
 *      > pgsub->j  long    CAR value
 * 
 * Parameters out:
 *      < pgsub->vala   long    CAR value summation
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
 * 01-Feb-1997: Original(srp)
 * 24-Feb-1998: Move to file house.c from testFunctions.c
 * 24-Jun-1998: Modify carDrive to give preference to BUSY and reflect
 *      indicate status of most recently changed CAR. Maintaining
 *      history requires splitting to one per genSub
 * 
 */

/* INDENT ON */
/* ===================================================================== */

/*
long    carInit (struct genSubRecord * pgsub)
{
    return (OK);
}
*/

long    carDrive1 (struct genSubRecord * pgsub)
{
    static struct
    {
        long    a;
        long    b;
        long    c;
        long    d;
        long    e;
        long    f;
        long    g;
        long    h;
        long    i;
        long    j;
    } oldCar =
    {
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE
    };

    long    output = CAR_IDLE;

    /* check for any CAR_BUSY, if found set output BUSY */

    if (*(long *) pgsub->a == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->b == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->c == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->d == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->e == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->f == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->g == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->h == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->i == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->j == CAR_BUSY)
        output = CAR_BUSY;

    /* if none are BUSY, check for changes and set output to match */

    if(output != CAR_BUSY)
    {
        if (*(long *) pgsub->a != oldCar.a)
        {
            output = *(long *) pgsub->a;
        }
        else if (*(long *) pgsub->b != oldCar.b)
        {
            output = *(long *) pgsub->b;
        }
        else if (*(long *) pgsub->c != oldCar.c)
        {
            output = *(long *) pgsub->c;
        }
        else if (*(long *) pgsub->d != oldCar.d)
        {
            output = *(long *) pgsub->d;
        }
        else if (*(long *) pgsub->e != oldCar.e)
        {
            output = *(long *) pgsub->e;
        }
        else if (*(long *) pgsub->f != oldCar.f)
        {
            output = *(long *) pgsub->f;
        }
        else if (*(long *) pgsub->g != oldCar.g)
        {
            output = *(long *) pgsub->g;
        }
        else if (*(long *) pgsub->h != oldCar.h)
        {
            output = *(long *) pgsub->h;
        }
        else if (*(long *) pgsub->i != oldCar.i)
        {
            output = *(long *) pgsub->i;
        }
        else if (*(long *) pgsub->j != oldCar.j)
        {
            output = *(long *) pgsub->j;
        }
    }   

    /* update history */

    oldCar.a = *(long *)pgsub->a;
    oldCar.b = *(long *)pgsub->b;
    oldCar.c = *(long *)pgsub->c;
    oldCar.d = *(long *)pgsub->d;
    oldCar.e = *(long *)pgsub->e;
    oldCar.f = *(long *)pgsub->f;
    oldCar.g = *(long *)pgsub->g;
    oldCar.h = *(long *)pgsub->h;
    oldCar.i = *(long *)pgsub->i;
    oldCar.j = *(long *)pgsub->j;


    /* write net result to output port */

    *(long *) pgsub->vala = output;

    return (OK);
}

long    carDrive2 (struct genSubRecord * pgsub)
{
    static struct
    {
        long    a;
        long    b;
        long    c;
        long    d;
        long    e;
        long    f;
        long    g;
        long    h;
        long    i;
        long    j;
    } oldCar =
    {
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE
    };

    long    output = CAR_IDLE;

    /* check for any CAR_BUSY, if found set output BUSY */

    if (*(long *) pgsub->a == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->b == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->c == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->d == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->e == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->f == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->g == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->h == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->i == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->j == CAR_BUSY)
        output = CAR_BUSY;

    /* if none are BUSY, check for changes and set output to match */

    if(output != CAR_BUSY)
    {
        if (*(long *) pgsub->a != oldCar.a)
        {
            output = *(long *) pgsub->a;
        }
        else if (*(long *) pgsub->b != oldCar.b)
        {
            output = *(long *) pgsub->b;
        }
        else if (*(long *) pgsub->c != oldCar.c)
        {
            output = *(long *) pgsub->c;
        }
        else if (*(long *) pgsub->d != oldCar.d)
        {
            output = *(long *) pgsub->d;
        }
        else if (*(long *) pgsub->e != oldCar.e)
        {
            output = *(long *) pgsub->e;
        }
        else if (*(long *) pgsub->f != oldCar.f)
        {
            output = *(long *) pgsub->f;
        }
        else if (*(long *) pgsub->g != oldCar.g)
        {
            output = *(long *) pgsub->g;
        }
        else if (*(long *) pgsub->h != oldCar.h)
        {
            output = *(long *) pgsub->h;
        }
        else if (*(long *) pgsub->i != oldCar.i)
        {
            output = *(long *) pgsub->i;
        }
        else if (*(long *) pgsub->j != oldCar.j)
        {
            output = *(long *) pgsub->j;
        }
    }   

    /* update history */

    oldCar.a = *(long *)pgsub->a;
    oldCar.b = *(long *)pgsub->b;
    oldCar.c = *(long *)pgsub->c;
    oldCar.d = *(long *)pgsub->d;
    oldCar.e = *(long *)pgsub->e;
    oldCar.f = *(long *)pgsub->f;
    oldCar.g = *(long *)pgsub->g;
    oldCar.h = *(long *)pgsub->h;
    oldCar.i = *(long *)pgsub->i;
    oldCar.j = *(long *)pgsub->j;


    /* write net result to output port */

    *(long *) pgsub->vala = output;

    return (OK);
}

long    carDrive3 (struct genSubRecord * pgsub)
{
    static struct
    {
        long    a;
        long    b;
        long    c;
        long    d;
        long    e;
        long    f;
        long    g;
        long    h;
        long    i;
        long    j;
    } oldCar =
    {
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE,
        CAR_IDLE
    };

    long    output = CAR_IDLE;

    /* check for any CAR_BUSY, if found set output BUSY */

    if (*(long *) pgsub->a == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->b == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->c == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->d == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->e == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->f == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->g == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->h == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->i == CAR_BUSY)
        output = CAR_BUSY;
    else if (*(long *) pgsub->j == CAR_BUSY)
        output = CAR_BUSY;

    /* if none are BUSY, check for changes and set output to match */

    if(output != CAR_BUSY)
    {
        if (*(long *) pgsub->a != oldCar.a)
        {
            output = *(long *) pgsub->a;
        }
        else if (*(long *) pgsub->b != oldCar.b)
        {
            output = *(long *) pgsub->b;
        }
        else if (*(long *) pgsub->c != oldCar.c)
        {
            output = *(long *) pgsub->c;
        }
        else if (*(long *) pgsub->d != oldCar.d)
        {
            output = *(long *) pgsub->d;
        }
        else if (*(long *) pgsub->e != oldCar.e)
        {
            output = *(long *) pgsub->e;
        }
        else if (*(long *) pgsub->f != oldCar.f)
        {
            output = *(long *) pgsub->f;
        }
        else if (*(long *) pgsub->g != oldCar.g)
        {
            output = *(long *) pgsub->g;
        }
        else if (*(long *) pgsub->h != oldCar.h)
        {
            output = *(long *) pgsub->h;
        }
        else if (*(long *) pgsub->i != oldCar.i)
        {
            output = *(long *) pgsub->i;
        }
        else if (*(long *) pgsub->j != oldCar.j)
        {
            output = *(long *) pgsub->j;
        }
    }   

    /* update history */

    oldCar.a = *(long *)pgsub->a;
    oldCar.b = *(long *)pgsub->b;
    oldCar.c = *(long *)pgsub->c;
    oldCar.d = *(long *)pgsub->d;
    oldCar.e = *(long *)pgsub->e;
    oldCar.f = *(long *)pgsub->f;
    oldCar.g = *(long *)pgsub->g;
    oldCar.h = *(long *)pgsub->h;
    oldCar.i = *(long *)pgsub->i;
    oldCar.j = *(long *)pgsub->j;


    /* write net result to output port */

    *(long *) pgsub->vala = output;

    return (OK);
}


