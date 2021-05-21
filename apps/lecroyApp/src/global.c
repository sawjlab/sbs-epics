/*****************************************************************************\
 * File: global.c                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains the global variables for the LeCroy 1458 device      *
 *   driver software.                                                        *
 *                                                                           *
 * Revision History:                                                         *
 *   12/22/2000 - Initial release                                            *
\*****************************************************************************/

#include "lecroy.h"
#include "define.h"

    /* This is the LeCroy Chassis database shared by all tasks. */

unsigned g_chassisCnt = 0;
Chassis *g_chassis[MAX_CHASSIS];

      /* These are the LeCroy property name tables defined for each *\
       | HV module type.  The order of names must match the channel |
      \* type definitions in lecroy.h                               */

const char *prp1461[NUM_1461_PROP] = {"MC", "MV", "DV", "RUP", "RDN",
  "TC", "CE", "ST", "MVDZ", "MCDZ", "HVL"};
const char *prp1471[NUM_1471_PROP] = {"MC", "MV", "DV", "RUP", "RDN",
  "TC", "CE", "ST", "MVDZ", "MCDZ", "HVL","MCpk","TC_pk","RTE","RLY"};
const char *prp1468[NUM_1468_PROP] = {"MC", "MV", "DV", "RUP", "RDN",
  "TC", "CE", "ST", "MVDZ", "MCDZ", "HVL"};
const char *prp1469[NUM_1469_PROP] = {"MC", "MV", "DV", "RUP", "RDN",
  "TC", "CE", "ST", "PRD", "SOT", "MVDZ", "MCDZ", "HVL"};
const char *prp1469s[NUM_1469_SUBPROP] = {"MC", "MV", "TC", "CE", "ST",
  "MCDZ"};
