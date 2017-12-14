/* $Id: guide.h,v 1.8 2015/05/01 00:09:50 mrippa Exp $ */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * guide.h
 * 
 * PURPOSE
 * -------
 * Header file defines the public interface for guide.c
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
 * 17-Nov-1999: Created new header files.
 * 16-Dec-1999: Added global variables
 *
 */
/* INDENT ON */
/* ===================================================================== */
#ifndef _INCLUDED_GUIDE_H
#define _INCLUDED_GUIDE_H

#ifndef _INCLUDED_CADRECORD_H
#define _INCLUDED_CADRECORD_H
#include <cadRecord.h>
#endif

#ifndef _INCLUDED_GENSUBRECORD_H
#define _INCLUDED_GENSUBRECORD_H
#include <genSubRecord.h>
#endif

#include "utilities.h"

#define XTILT   0       /* axis identifiers */
#define YTILT   1
#define ZFOCUS  2       /* focus and zfocus are identical and interchangeable */
#define FOCUS   2
#define XPOSITION 3
#define YPOSITION 4

/* Symbolic constants for array sizing */

#define MAX_AXES        3       /* Numnber of axes - xtilt, ytilt, focus */
#define MAX_BEAMS       5       /* Current beam A, B, C, A2B, B2A */

/* Define instrument indexes on reflective memory */

enum
{
  PWFS1 = 0,
  PWFS2,
  OIWFS,
  GAOS,

#ifndef MK
  GPI,
#endif

  GYRO
};

#ifdef MK
enum
{
    GUIDE_200_HZ = 200,
    GUIDE_100_HZ = 100,
    GUIDE_50_HZ = 50,
    GUIDE_25_HZ = 25,
    GUIDE_20_HZ = 20
};
#endif


/* Processing available to guide sources */

enum
{
        NOTUSED = 0,
        RAW,
        LOWPASS,
        HIGHPASS,
        BANDPASS,
        BANDSTOP
};

/* Define structures to hold filter configurations */   

typedef struct
{
        double  wn1;
        double  wn2;
        int     nb;
        int     na;
        double  Bcoeffs[11];
        double  Acoeffs[11];
} FILTER;

typedef struct
{
        long    nb;             /* number of numerator coefficients     */
        long    na;             /* number of denominator coefficients   */
        double  numerator[11];  /* B coefficients (b0, b1, b(nb-1)      */
        double  denominator[11];/* A coefficients (1, a1, a2, a(na-1)   */
        double  inHistory[11];  /* input history                        */
        double  outHistory[11]; /* ouput history                        */
        double  weightA;
        double  weightB;
        double  weightC;
        int     type;           /* type of processing for this source   */
        double  sampleFreq;     /* sample frequency                     */
        double  freq1;          /* low cutoff                           */
        double  freq2;          /* high cutoff                          */
} MATLAB;

/* Get the updateInterval for pwfs2 */
typedef struct  
{
   float pwfs2;
   float pwfs1;
   float oiwfs;
   float gaos;
#ifndef MK
   float gpi;
#endif
} anUpdateInterval;

#ifdef MK
typedef struct
{
    double sensedRate;
    long rate;
    double vtkXdata[3]; /*vtk gain, scale and actual sampleRate (198.9Hz, 99.5Hz or 49.5Hz)*/
    double vtkYdata[3]; /*vtk gain, scale and actual sampleRate (198.9Hz, 99.5Hz or 49.5Hz)*/
} GuideInfo;
#endif

/* Public functions */

long CADguideControl(struct cadRecord* pcad);

long CADguideConfig(struct cadRecord* pcad);

long guideConfig(struct genSubRecord* pgsub);

long initDecimate(struct genSubRecord* pgsub);

long decimate(struct genSubRecord* pgsub);

long lookupGuide(struct genSubRecord* pgsub);

int createFilter(int source, int filterType, 
                 double sampleRate, double freq1, double freq2, 
                 double weightA, double weightB, double weightC);

/* Global variables */

extern anUpdateInterval updateInterval;
#ifdef MK
extern GuideInfo guideInfo;
#endif
extern long guideOn;
extern long guideSimOn;
extern int guideOnA;
extern int guideOnB;
extern int guideOnC;

extern double weight[MAX_SOURCES][MAX_BEAMS];
extern int guideMaster[MAX_SOURCES][MAX_BEAMS];
extern MATLAB filter[MAX_SOURCES][MAX_AXES];

#endif

