/*****************************************************************************\
 * File: global.c                                   Author: Stephen A. Wood  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains the global variables for the CAEN V65XX device       *
 *   driver software.                                                        *
 *                                                                           *
 * Revision History:                        t                                 *
 *   05/13/2021 - Initial release                                            *
\*****************************************************************************/

#include "define.h"
#include "extern.h"

    /* This is the LeCroy Chassis database shared by all tasks. */

unsigned int g_boardCnt = 1;
char *g_vme_base = (char *) 0xA00000;
char *g_vme_delta = (char *) 0x100000;

uint16_t* g_laddr[MAX_SLOTS];
uint32_t g_cmd2mem[NCMD];

