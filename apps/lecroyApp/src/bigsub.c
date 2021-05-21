/*****************************************************************************\
 * File: bigsub.c                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains the functions called by EPICS "bigsub" record        *
 *   processing for the LeCroy High Voltage Mainframe database application.  *
 *   Note that the significant part of this procedure is through the         *
 *   external function "GetChannel", which is part of the lecroyDev device   *
 *   support package.                                                        *
 *                                                                           *
 * Revision History:                                                         *
 *   01/10/2001 - Initial release                                            *
\*****************************************************************************/

#include <stdio.h>
#include <registryFunction.h>
#include <dbBase.h>
#include <bigsubRecord.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <recGbl.h>
#include "typedefs.h"

STATUS GetChannel(unsigned, unsigned, unsigned, double *, double *);


/*                                                                           *\
 | Name: InitChannel                                                         |
 | Parameters: psub - pointer to a bigsub record                             |
 | Return: EPICS completion status; 0 = success                              |
 | Remarks:                                                                  |
 |   This function is a place holder and does nothing but return success.    |
\*                                                                           */
long InitChannel(struct bigsubRecord *psub)
{
   return(0);
}


/*                                                                           *\
 | Name: ScanChannel                                                         |
 | Parameters: psub - pointer to bigsub record                               |
 | Return: EPICS completion status; 0 = success                              |
 | Remarks:                                                                  |
 |   This function is called for each scan of a LeCroyApp bigsub record.  It |
 |   calls the device support routine "GetChannel" to fill record slots with |
 |   channel information.  Note that the input fields 'e' through 't' are    |
 |   used for this purpose, giving a maximum of 16 pieces of channel data    |
 |   that may be fetched.                                                    |
\*                                                                           */
long ScanChannel(struct bigsubRecord *psub)
{
  /* The first three input fields of the record identify the specific channel
   * desired by chassis ID, slot number, and onboard channel number.  The
   * fourth field contains non-zero when the desired information is a LeCroy
   * 1469 module's block generator as opposed to a regular channel.  If it is
   * a generator, the channel number's bit 31 is set as a flag.
   */
  unsigned chassis = (unsigned) psub->a;
  unsigned slot = (unsigned) psub->b;
  unsigned channel = (unsigned) psub->c;
  if (psub->d != 0) channel |= 0x80;

  /* Call device support routine to get channel data for this record, filled
   * in starting at field 'e'.  Also pass the 'val' field which will be set to
   * the difference (absolute value) between measured and demand voltage (if
   * applicable).
   */
  if (GetChannel(chassis, slot, channel, &psub->e, &psub->val) == ERROR)
  { char alert[128];
    sprintf(alert, "Bigsub - %s(%d): Chassis=%u Slot=%u Channel=%u",
            __FILE__, __LINE__, chassis, slot, channel);
    recGblRecordError(S_db_badField, (void *) psub, alert);
    return(S_db_badField);
  }

  return(0);
}

epicsRegisterFunction(InitChannel);
epicsRegisterFunction(ScanChannel);
