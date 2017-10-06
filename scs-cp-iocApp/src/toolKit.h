/* $Id: toolKit.h,v 1.1 2002/02/05 13:19:52 gemvx Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * toolKit.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for toolKit.c.
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
#ifndef _INCLUDED_TOOLKIT_H
#define _INCLUDED_TOOLKIT_H

#ifndef _INCLUDED_SEMLIB_H
#define _INCLUDED_SEMLIB_H
#include <semLib.h>
#endif

/* Public functions */

/* Probably used from console */
int searchMem(long *startPtr, long *stopPtr, long pattern);

int checkController(void);

int checkSetPoint(void);

int checkFilterConfig(const int source, const int axis);

double runFilter(const double input);

void showMem(void);

void blocking(SEM_ID semId);

void togglePort(void);

void kickPort(void);

void checkTimes(int reps);

int checkFiles(char* filename);

int timeDate(int reps);

#endif
