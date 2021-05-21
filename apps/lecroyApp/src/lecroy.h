#ifndef LECROY_H
#define LECROY_H

/*****************************************************************************\
 * File: lecroy.h                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This is the main header file for the ethernet device driver software    *
 *   used with the LeCroy 1458 High Voltage (HV) Mainframe.  The data        *
 *   structures, types, and definitions that depict the HV hardware are      *
 *   contained here.  The software was developed to run under the VxWorks    *
 *   real-time embedded operating system for applications at Thomas          *
 *   Jefferson National Accelerator Facility.  Refer to the software         *
 *   document for detailed information.                                      *
 *                                                                           *
 * References:                                                               *
 *   LeCroy Research Systems; "1454/1458 HV Mainframe"; ECO-1003             *
 *   Universal Voltronics (bought out LeCroy HV division)                    *
 *     http://www.voltronics.com/index2.html                                 *
 *   VxWorks Operating System; Wind Rivers Systems Corp.; http://www.wrs.com *
 *   Jefferson Labs; http://www.jlab.org                                     *
 *   LeCroy 1458 High Voltage Mainframe EPICS Control Software Description   *
 *     Christopher Slominski (cjs@jlab.org)                                  *
 *                                                                           *
 * Revision History:                                                         *
 *   12/22/2000 - Initial release                                            *
 *   04/11/2001 - Updated header comment for release 1-1                     *
\*****************************************************************************/

#include <epicsTimer.h>
#include "epicsEvent.h"
#include "epicsMutex.h"
#include "stdlib.h"
#include "string.h"
#include "epicsMessageQueue.h"
#include <epicsThread.h>
#include "typedefs.h"

#define SERVER_PORT 1090               /* LeCroy TCP/IP socket port          */
#define NUM_1461_CHAN 12               /* Channels per 1461 card             */
#define NUM_1471_CHAN 8                /* Channels per 1471 card             */
#define NUM_1468_CHAN 6                /* Channels per 1468 card             */
#define NUM_1469_GEN 3                 /* Block generators per 1469 card     */
#define NUM_1469_CHAN 24               /* Channels per 1469 block generator  */
#define NUM_1461_PROP 11               /* Properties per 1461 channel        */
#define NUM_1471_PROP 15               /* Properties per 1471 channel        */
#define NUM_1468_PROP 11               /* Properties per 1468 channel        */
#define NUM_1469_PROP 13               /* Properties per 1469 generator      */
#define NUM_1469_SUBPROP 6             /* Properties per 1469 channel        */
#define MAX_PROPS NUM_1469_PROP        /* Max properties for any card        */
#define NUM_SLOTS 16                   /* Card slots per HV mainframe        */
#define MAX_UNITS (NUM_SLOTS * 3)      /* Max logical units per chassis      */
#define REPLY_SIZE 512                 /* Response buffer size               */

/* The following enumerations have the types of boards that may be found in a
 * chassis' card slots and the states of the network connection.
 */

typedef enum {HV_1461 = 0, HV_1468, HV_1469, HV_1471} HVtype;

    /*                                                              *\
     | The following Type Definitions are created to describe the   |
     | layout of LeCroy hardware.  Refer to the hardware reference  |
     | manuals for details.                                         |
    \*                                                              */

/* These type definitions describe channels and generators found on the
 * various LeCroy boards.  Note the channel data is stored as a union to
 * allow both indexing and direct reference of properties.  Therefore do
 * not change the property order without a matching change in lecroy.c
 * where properties are stored by index.
 */

typedef union
{ float properties[NUM_1461_PROP];
  struct { float MC, MV, DV, RUP, RDN, TC, CE, ST, MVDZ, MCDZ, HVL; } property;
} HV1461Channel;

// Has other properties.  Use same as 1461 for now
typedef union
{ float properties[NUM_1471_PROP];
  struct { float MC, MV, DV, RUP, RDN, TC, CE, ST, MVDZ, MCDZ, HVL, MCpk, TC_pk, RTE, RLY; } property;
} HV1471Channel;

typedef HV1461Channel HV1468Channel;

typedef union
{ float properties[NUM_1469_SUBPROP];
  struct { float MC, MV, TC, CE, ST, MCDZ; } property;
} HV1469Channel;

typedef union
{ float properties[NUM_1469_PROP];
  struct
  { float MC, MV, DV, RUP, RDN, TC, CE, ST, PRD, SOT, MVDZ, MCDZ, HVL;
  } property;
} HV1469Generator;

/* These type definitions describe LeCroy boards.  Note there is a generic
 * template used as a placeholder when needed.
 */

typedef struct
{ HVtype id;                                /* Board type (1461 ...)        */
  char polarity;                            /* Positive or negative version */
  unsigned unit;                            /* Logical unit within chassis  */
} HVgeneric;

typedef struct
{ HVtype id;
  char polarity;
  unsigned unit;
  unsigned m_sum[NUM_1461_PROP];
  HV1461Channel m_chan[NUM_1461_CHAN];
} HV1461;

typedef struct
{ HVtype id;
  char polarity;
  unsigned unit;
  unsigned m_sum[NUM_1471_PROP];
  HV1471Channel m_chan[NUM_1471_CHAN];
} HV1471;

typedef struct
{ HVtype id;
  char polarity;
  unsigned unit;
  unsigned m_sum[NUM_1468_PROP];
  HV1468Channel m_chan[NUM_1468_CHAN];
} HV1468;

typedef struct
{ HVtype id;
  char polarity;
  unsigned unit;
  unsigned m_sumGen[NUM_1469_PROP];
  unsigned m_sumChan[NUM_1469_SUBPROP];
  HV1469Generator m_gen[NUM_1469_GEN];
  HV1469Channel m_chan[NUM_1469_CHAN];
} HV1469;

/* The Chassis data structure has both hardware layout elements and
 * software task parameters.  An array of pointers to the cards in
 * each slot is maintained.  Software variables related to a single
 * chassis are encapsulated here.  The Chassis also maintains an array
 * of logical unit information (see LeCroy manual for relation between
 * slots and logical units).  The "LogUnit" structure has data
 * pertinent to the  logical unit organization of the Chassis.
 */

typedef struct
{ unsigned sumM, sumS, oldM, oldS;  /* Logical unit's summary numbers        */
  unsigned slot, submodule;         /* Logical unit's slot and submodule     */
} LogUnit;

typedef struct
{ HVgeneric *m_slot[NUM_SLOTS];     /* LeCroy card slots                     */
  LogUnit *m_logUnit;               /* LeCroy logical units                  */
  unsigned m_hvID,                  /* User assigned chassis ID              */
           m_bufSize,               /* Allocation size of m_response         */
           m_overflow,              /* Frame overflow counter                */
           m_units,                 /* Number of logical units in m_logUnit  */
           m_config0,               /* LeCroy configuration word #0          */
           m_frameCnt,              /* Frame counter                         */
           m_lastCnt,               /* Snapshot of m_updCnt from watchdog    */
           m_sameCnt,               /* Count frames of no response           */
           m_sumM, m_sumS, m_sumC;  /* Chassis summary numbers               */
  epicsThreadId m_tid;     /* Task ID                               */
  //    int  m_ticks;                      /* System clock ticks per frame          */
  char m_reply[REPLY_SIZE];         /* Command response from LeCroy unit     */
  BOOL m_busy,                      /* Task cycle active                     */
       m_valid,                     /* Chassis data valid flag               */
       m_alarm,                     /* Chassis alarm status                  */
       m_learned,                   /* Flag chassis's config is learned      */
       m_HVstatus;                  /* High Voltage on/off                   */
  fd_set m_fdset;                   /* File descriptor set for "select"      */
  epicsTimerId m_timerid;           /* Epics watchdog timer                  */
  epicsEventId m_wakeup;            /* Synchronize semaphore                 */
  epicsMutexId m_mutex;             /* Mutual exclusion semaphore    */
  epicsMessageQueueId m_queue;      /* Message queue for commands to LeCroy  */
  char *m_ipaddr;		    /* IP address of crate */
  int m_sock;			    /* Socket */
} Chassis;

#endif
