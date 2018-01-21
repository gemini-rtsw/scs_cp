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
 * 07-Dec-2017: Copied from xycom.h and deleted xycom-240 driver related stuff (mdw)
 *
 */
/* ===================================================================== */
#ifndef _INCLUDED_EVENTBUS_H
#define _INCLUDED_EVENTBUS_H

#include <drvXy240.h>           /* for xy240 driver stuff */
#define PORT0  0
#define PORT1  1
#define PORT2  2
#define PORT3  3
#define PORT4  4
#define PORT5  5
#define PORT6  6
#define PORT7  7

#define BIT0   0
#define BIT1   1
#define BIT2   2
#define BIT3   3
#define BIT4   4
#define BIT5   5
#define BIT6   6
#define BIT7   7

#define XYCARDNUM 0

int eventBusInit(void);
void eventHandler (void *);
long getSyncMask(void);
void showInterruptCounts(void);
void clearInterruptCounts(void);


#endif
