/*
 * Author: Andy Foster
 * Date:   15th November 95
 *
 * Modifications:  
 *	   29apr96,bdg added  CAR states defines
 *         19dec96,bdg removed UNAVAILABLE and UNKNOWN
 *
 * Include file for the Gemini CAR record
 *
*/

/* Allowed States of the CAR */
enum carstate { IDLE, PAUSED, BUSY, ERR};
#define CAR_IDLE	0
#define	CAR_PAUSED	1
#define CAR_BUSY	2
#define CAR_ERROR	3

/* Allowed Simulation Modes for the CAR */
#define SIMM_NONE 0
#define SIMM_VSM  1
#define SIMM_FAST 2
#define SIMM_FULL 3

/* Special Error Code returned by the CAR for invalid input */
/* Users should define further error codes which start at 2 */

#define CAR_INVALID_INPUT  1
