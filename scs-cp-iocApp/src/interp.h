/* $Id: interp.h,v 1.1 2002/02/05 13:19:49 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * interp.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for interp.c. 
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
 * 17-Nov-1999: Created new header files. KG
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_INTERP_H
#define _INCLUDED_INTERP_H

#include "control.h"

enum                            /* axis identifiers for each beam */
{
        AX = 0,
        AY,
        BX,
        BY,
        CX,
        CY,
        Z
};

double getInterpolation (int, double);

void tcsInterpolate (Demands);

#endif
