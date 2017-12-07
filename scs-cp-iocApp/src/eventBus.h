/* $Id: xycom.h,v 1.4 2008/04/16 23:23:49 mrippa Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * xycom.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for eventBus.c
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
 *
 * 
 * HISTORY
 * -------
 * 10-Jun-2000: Original
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_EVENTBUS_H
#define _INCLUDED_EVENTBUS_H

int xyInit (void);
void eventHandler (void);
int xyStatus (void);
long getSyncMask(void);
void showInterruptCounts(void);
void clearInterruptCounts(void);
int xyWriteBit (int port, int bitnum, int val);


#endif
