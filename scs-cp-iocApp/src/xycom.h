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
 * Header file defines the public interface for xycom.c
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
#ifndef _INCLUDED_XYCOM_H
#define _INCLUDED_XYCOM_H

/*
 * Assume XYCOM base address is set to ffffd000H.
 * Assume only one XYCOM board in system.
 *
 * Module identification is at odd numbered bytes in the range from 00h
 * to 3fh.  40h to 7fh is undefined.  Registers are at 80h to 8fh.  90h
 * to 3ffh is undefined.
 */

#define XYCOM_BASE                  (0xFBFFD000) 
#define INTERRUPT_INPUTS_REG_ADDR   (XYCOM_BASE+0x0080)
#define STATUS_CONTROL_REG_ADDR     (XYCOM_BASE+0x0081)
#define INTERRUPT_PENDING_REG_ADDR  (XYCOM_BASE+0x0082)
#define INTERRUPT_MASK_REG_ADDR     (XYCOM_BASE+0x0083)
#define INTERRUPT_CLEAR_REG_ADDR    (XYCOM_BASE+0x0084)
#define INTERRUPT_VECTOR_REG_ADDR   (XYCOM_BASE+0x0085)
#define FLAG_OUTPUTS_REG_ADDR       (XYCOM_BASE+0x0086)
#define PORT_DIRECTION_REG_ADDR     (XYCOM_BASE+0x0087)

#define PORT_0_ADDR                 (XYCOM_BASE+0x0088)
#define PORT_1_ADDR                 (XYCOM_BASE+0x0089)
#define PORT_2_ADDR                 (XYCOM_BASE+0x008a)
#define PORT_3_ADDR                 (XYCOM_BASE+0x008b)
#define PORT_4_ADDR                 (XYCOM_BASE+0x008c)
#define PORT_5_ADDR                 (XYCOM_BASE+0x008d)
#define PORT_6_ADDR                 (XYCOM_BASE+0x008e)
#define PORT_7_ADDR                 (XYCOM_BASE+0x008f)

#define PORT_0_INT		    (INTERRUPT_INPUTS_REG_ADDR & 0x00)
#define PORT_1_INT		    (INTERRUPT_INPUTS_REG_ADDR & 0x01)
#define PORT_2_INT		    (INTERRUPT_INPUTS_REG_ADDR & 0x02)
#define PORT_3_INT		    (INTERRUPT_INPUTS_REG_ADDR & 0x03)
#define PORT_4_INT		    (INTERRUPT_INPUTS_REG_ADDR & 0x04)
#define PORT_5_INT		    (INTERRUPT_INPUTS_REG_ADDR & 0x05)
#define PORT_6_INT		    (INTERRUPT_INPUTS_REG_ADDR & 0x06)
#define PORT_7_INT		    (INTERRUPT_INPUTS_REG_ADDR & 0x07)


/*
 * Definitions of the bits in the status/control register.
 */
#define SC_RED_LED                  0x01    /* 1=off 0=on */
#define SC_GREEN_LED                0x02    /* 1=on 0=off */
#define SC_INTERRUPT_PENDING        0x04    /* read-only bit */
#define SC_INTERRUPT_ENABLE         0x08
#define SC_SOFTWARE_RESET           0x10    /* 0,1,0 causes board reset */
#define SC_USER_BIT_0               0x20
#define SC_USER_BIT_1               0x40    /* three bits for user */
#define SC_USER_BIT_2               0x80

#define INT_VECTOR 80		/* interrupt vector for xycom240 */
#define INTERRUPT_LEVEL 5       /* corresponds to switch setting in SW1/1-3 =
			           OPEN-OPEN-CLOSED on Xycom240 */


/* Public functions */

int xyInit (void);
void eventHandler (void);
int xyStatus (void);
long getSyncMask(void);
void showInterruptCounts(void);
void clearInterruptCounts(void);
int xyWriteBit (int port, int bitnum, int val);


#endif
