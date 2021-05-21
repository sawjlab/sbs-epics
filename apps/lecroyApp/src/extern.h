#ifndef EXTERN_H
#define EXTERN_H

/*****************************************************************************\
 * File: extern.h                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This header file contains references to the global items in the LeCroy  *
 *   High Voltage device driver.  Included are global function prototypes    *
 *   and global variables.                                                   *
 *                                                                           *
 * Revision History:                                                         *
 *   12/22/2000 - Initial release                                            *
\*****************************************************************************/

//#include "vxWorks.h"
#include "lecroy.h"
#include "define.h"

/* entry points for shell execution */
int HVAddCrate(int, char *);
int HVstart();
int HVstop(void);
int hinv(void);
int keyboard(unsigned);

/* ARCNet interface */
int ARCsetup(char *ipaddr);
int ARCsend(int, const char *, char *, unsigned);
void ARCend(void);
void ComStats(void);
void Mark(unsigned char);
int ARCnode(unsigned char);

int GetConfig(Chassis *);
int CheckStatus(Chassis *);
BOOL ChassisAlarm(Chassis *);
int FlushQ(Chassis *);
void StopAll(Chassis *);
int tickMgr(void *);
int ErrReport(char *, ErrType);

/* Access from device layer of EPICS */
STATUS HVcmd(unsigned, char *);
STATUS HVload(unsigned, unsigned, unsigned, char *, char *);
int GetValidity(unsigned);
int GetHV(unsigned);
int GetAlarm(unsigned);
STATUS GetProperty(unsigned, unsigned, unsigned, char *, float *);
STATUS GetChannel(unsigned, unsigned, unsigned, double *, double *);

/* See global.c */
extern int g_chassisCnt;
extern Chassis *g_chassis[];
extern const char *prp1461[NUM_1461_PROP];
extern const char *prp1471[NUM_1471_PROP];
extern const char *prp1468[NUM_1468_PROP];
extern const char *prp1469[NUM_1469_PROP];
extern const char *prp1469s[NUM_1469_SUBPROP];

#endif
