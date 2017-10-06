/* $Id: testFunctions.h,v 1.3 2008/09/24 01:15:49 mrippa Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * testFunctions.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for testFunctions.c
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

#ifndef _INCLUDED_TESTFUNCTIONS_H
#define _INCLUDED_TESTFUNCTIONS_H

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif
#ifndef _INCLUDE_CADRECORD_H
#define _INCLUDE_CADRECORD_H
#include <cad.h>
#endif


#ifndef _INCLUDE_CADRECORD_H
#define _INCLUDE_CADRECORD_H
#include <cad.h>
#endif


/* Public functions */

void checkSafeBlock(int count);

void tiltState(const memMap* buffPtr);

void big(void);

void showCounts(void);

void showTime(void);

void rawTime(void);

void testMem(const memMap* buffPtr);
void printPage0(const memMap* buffPtr);
void printPage1(const memMap* buffPtr);
void printPage2(const memMap* buffPtr);
void printPage7(const memMap* buffPtr);
void printPage8(const memMap* buffPtr);
void printPage9(const memMap* buffPtr);
void printPage10(const memMap* buffPtr);
void printPage11(const memMap* buffPtr);
void printPage12(const memMap* buffPtr);
void printPage13a(const memMap* buffPtr);
void printPage13b(const memMap* buffPtr);

long initSelector(struct genSubRecord* pgsub);

void testFrame(double xTilt, double yTilt, double zFocus, 
               double xPos, double yPos);

long initSelector(struct genSubRecord* pgsub);

long selector(struct genSubRecord* pgsub);

void checkFiltered(int index);

void showMaster(void);

void driveP1(void);

void driveP2(void);

void fillWfs(double value);

void startGuideSim();

void endGuideSim();

#endif
