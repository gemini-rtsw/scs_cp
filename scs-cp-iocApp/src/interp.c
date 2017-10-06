/* $Id: interp.c,v 1.2 2002/05/08 20:20:29 gemvx Exp $ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * interp.c
 * 
 * PURPOSE
 * -------
 * Functions to curve fit a quadratic to the 20Hz TCS follow demands
 * 
 * FUNCTION NAME(S)
 * ----------------
 * testInterpolator - Provide demand values to exercise the interpolator
 * getInterpolation - perform quadratic calculation to get estimate
 * tcsInterpolate   - curve fit quadratic to TCS demand data
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
 * 24-Oct-1997: Expand number of axes to 7 to include tilts for each beam
 * 07-May-1999: Added RCS id
 *
 */
/* INDENT ON */
/* ===================================================================== */

#include "interp.h"
#include "control.h"      /* For definition of Demands */
#include "utilities.h"    /* For debugLevel */

#include <logLib.h>
#include <stdio.h>

#define NUM_INTERP_AXES    7    /* number of channels that need interpolating */
#define NUM_INTERP_SAMPLES 3    /* number of time samples needed for 
                                   interpolation      */
#define NUM_INTERP_COEFFS  3    /* number of time samples needed for 
                                   interpolation      */

/* Define function prototypes */

int errorLog(char *errorString, int debugLevel, int fileLog);

/* Define globals */

double  t[NUM_INTERP_SAMPLES];          /* t0, t1, t2   zero is oldest */

/* sample values for each axis x0, x1, x2 zero is oldest */
double  sample[NUM_INTERP_AXES][NUM_INTERP_SAMPLES] =  
{
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};

double  interpCoeff[NUM_INTERP_AXES][NUM_INTERP_COEFFS] =   /* a, b, c coefficients for each axis */
{
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};

double  denominator;
double  timeDatum;

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * getInterpolation
 * 
 * Purpose:
 * current value of variable is estimated for the supplied time using
 * the quadratic coefficients
 * 
 * Invocation:
 * value = getInterpolation(axis, targetTime);
 * 
 * Parameters in:
 *      > selectAxis    int current axis
 *      > targetTime    double  time for which estimate required
 * 
 * Parameters out:
 * None
 * 
 * Return value:
 *      < value     double  estimated value
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
 * 15-Oct-1997: Original(srp)
 * 
 */

/* INDENT ON */
/* ===================================================================== */

double  getInterpolation (int selectAxis, double targetTime)
{
    if (debugLevel == DEBUG_RESERVED2)
    {
        printf
        ("getInterpolation with axis=%1d, targettime=%f, coeffs=%f %f %f \n",
        selectAxis, targetTime, interpCoeff[selectAxis][0], 
        interpCoeff[selectAxis][1], interpCoeff[selectAxis][2]);
    }

    if(selectAxis < 0 || selectAxis > (NUM_INTERP_AXES - 1))
    {
        logMsg("getInterpolation - axis out of range\n", 0, 0, 0, 0, 0, 0);
        return(ERROR);
    }

    targetTime -= timeDatum;
    return ((interpCoeff[selectAxis][0] * targetTime
         + interpCoeff[selectAxis][1]) * targetTime
        + interpCoeff[selectAxis][2]);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * tcsInterpolate
 * 
 * Purpose:
 * Curve fit a quadratic to the received 20Hz TCS follow demands
 * 
 * Invocation:
 * tcsInterpolate(newTcsDemands)
 * 
 * Parameters in:
 *      > newTcsDemands Demands structure of timestamped demands
 * 
 * Parameters out:
 *      < coefficients a, b and c for the quadratic
 * 
 * Return value:
 * None
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
 * 15-Oct-1997: Original(srp)
 * 24-Oct-1997: Expand to allow for 7 axes for all beam positions
 * 
 */

/* INDENT ON */
/* ===================================================================== */

void    tcsInterpolate (Demands newTcsDemand)
{
    /* this routine called each time the TCS demand updates (20Hz)       */
    /* calculates the interpolation coefficients for a quadratic fit     */

    char    message[81];
    int axis;

    /* first remove an offset from the time value in case of numerical   */
    /* problems dealing with large numbers. The offfset is taken as the      */
    /* oldest of the three times                         */

    /* the time values are common to all axes */

    timeDatum = t[0];
    t[1] = t[1] - timeDatum;
    t[2] = newTcsDemand.timeApply - timeDatum;

    denominator = t[1] * t[2] * (t[1] - t[2]);

/* As a test put the new data into all three samples. This will effectively
*  bypass the interpolation.
*/
    sample[AX][2] = newTcsDemand.xTiltA;
    sample[AY][2] = newTcsDemand.yTiltA;
    sample[BX][2] = newTcsDemand.xTiltB;
    sample[BY][2] = newTcsDemand.yTiltB;
    sample[CX][2] = newTcsDemand.xTiltC;
    sample[CY][2] = newTcsDemand.yTiltC;
    sample[Z][2] = newTcsDemand.zFocus;

    if (denominator != 0)
    {
        for (axis = 0; axis < NUM_INTERP_AXES; axis++)
        {
            /* coefficient a */
            interpCoeff[axis][0] = (sample[axis][0] * (t[1] - t[2])
               + sample[axis][1] * t[2] - sample[axis][2] * t[1]) / denominator;

            /* coefficient b */
            interpCoeff[axis][1] = (sample[axis][0] * (t[2] * t[2] - t[1] * t[1])
               - sample[axis][1] * t[2] * t[2] + sample[axis][2] * t[1] * t[1])
               / denominator;

            /* coefficient c */
            interpCoeff[axis][2] = sample[axis][0];

            /* this debug level above that which can be set on the eng screens, set from console if required */

            interpCoeff[axis][0] = 0.0;
            interpCoeff[axis][1] = 0.0;
            if ((debugLevel > DEBUG_MED) & (debugLevel <= DEBUG_MED))
            {
                sprintf(message, "axis = %d\n", axis);
                logMsg("%s", (int)message, 0, 0, 0, 0, 0);

                sprintf(message, "coefficients = %4.2f, b= %4.2f, c=%4.2f\n",
                    interpCoeff[axis][0], interpCoeff[axis][1], interpCoeff[axis][2]);
                logMsg("%s", (int)message, 0, 0, 0, 0, 0);

                sprintf(message, "position     = %4.2f %4.2f %4.2f\n",
                    sample[axis][0], sample[axis][1], sample[axis][2]); 
                logMsg("%s", (int)message, 0, 0, 0, 0, 0);

                sprintf(message, "times        = %4.2f %4.2f %4.2f\n", 
                   t[0], t[1], t[2]);
                logMsg("%s", (int)message, 0, 0, 0, 0, 0);

            }

            /* ripple down old values */

            sample[axis][0] = sample[axis][1];
            sample[axis][1] = sample[axis][2];
        }
    }
    else
    {
        sprintf(message, "interp denom zero t[0] = %4.2f, t[1] = %4.2f, t[2] = %4.2f\n", t[1], t[1], t[2]);
        errorLog("tcsInterpolate - denominator zero", 2, ON);
    }

    /* ripple down old time but restore to real time rather than offset
     * time */

    t[0] = t[1] + timeDatum;
    t[1] = t[2] + timeDatum;
}


