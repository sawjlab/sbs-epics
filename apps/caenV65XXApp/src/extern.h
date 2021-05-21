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

#include "unistd.h"
#include "stdint.h"

/* entry points for shell execution */
int HVInit(int, void *, void *);

extern unsigned int g_boardCnt;
extern char *g_vme_base;
extern char *g_vme_delta;

extern uint16_t* g_laddr[];

extern uint32_t g_cmd2mem[];

#endif
