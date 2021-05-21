#ifndef DEFINE_H
#define DEFINE_H

/*****************************************************************************\
 * File: define.h                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This header file contains symbolic definitions and user data types for  *
 * the LeCroy High Voltage device driver.                                    *
 *                                                                           *
 * Revision History:                                                         *
 *   12/22/2000 - Initial release                                            *
 *   04/11/2001 - Set task priority to 99, added version number and changed  *
 *               IO_TIMEOUT from 3 to 5 seconds (release 1-1)                *
\*****************************************************************************/

#include "stdio.h"
//#include "ioLib.h"

#define VERSION "2-0.0"               /* Driver version number               */
#define MAX_CHASSIS 100               /* Maximum chassis allowed             */
#define CYCLE_TIME  2000              /* Chassis access time (msec)          */
#define IO_TIMEOUT 5000               /* ARCNet I/O timeout (msec)           */
#define WD_TIMEOUT 5                  /* Watchdog inactive frames to invalid */
#define THREAD_PRIORITY 80            /* Thread priority number              */

#define TASK_STACK 5000               /* Task stack size (bytes)             */
#define QMAX 8                        /* Size of VxWorks message queues      */
#define MSGMAX 64                     /* Size of a queued message            */
#define DEBUG 0                       /* Select debug information            */

/* These symbols are used for return codes from ErrReport(). */
typedef enum {R_Info = 0, R_Warn = -1, R_Fatal = -2} ErrType;

#endif
