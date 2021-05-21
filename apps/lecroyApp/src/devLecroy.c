/*****************************************************************************	\
 * File: devLecroy.c                                Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains the EPICS record handlers for the custom LeCroy HV   *
 *   device support layer.  The functions call driver support modules which  *
 *   are in the file access.c.                                               *
 *                                                                           *
 * Revision History:                                                         *
 *   01/04/2001 - Initial release                                            *
\*****************************************************************************/

//#include <vxWorks.h>
//#include <stdioLib.h>
#include <alarm.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <recSup.h>
#include <devSup.h>
#include <boRecord.h>
#include <biRecord.h>
#include <aoRecord.h>
#include <epicsExport.h>
#include "command.h"
#include "extern.h"

static long write_ao(struct aoRecord *);
static long init_ao(struct aoRecord *);
static long read_bi(struct biRecord *);
static long write_bo(struct boRecord *);
static long init_bo(struct boRecord *);

/* Thses EPICS structures associate the LecroyDev records with the
 * initialization and processing routines defined in this file.  The
 * global structure names are required by epics because of the
 * definitions in the device application's '.dbd' file.
 */

struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN write_bo;
} devBoLecroy = {5, NULL, NULL, init_bo, NULL, write_bo};

struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read_bi;
} devBiLecroy = {5, NULL, NULL, NULL, NULL, read_bi};

struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN write_ao;
  DEVSUPFUN special_linconv;
} devAoLecroy = {6, NULL, NULL, init_ao, NULL, write_ao, NULL};

epicsExportAddress(dset, devBoLecroy);
/* Name: init_bo                                                       *\
 | Parameters: pbo - Bo record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function is called at startup of the IOC to perform any      |
 |   initialization required by the record handler.  In this case,     |
 |   the HVcmd records are intialized to the status of the chassis.    |
\*                                                                     */
static long init_bo(struct boRecord  *pbo)
{
  float value;
  STATUS status;
  
  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B is ignored and
     C is the crate, N is the slot, A is the channel and F is the command.
  */
  struct camacio *pcamacio = (struct camacio *) &(pbo->out.value);  

  unsigned chassis = pcamacio->c;
  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  status = OK;
  
  if (command == S_CE) {
    status = GetProperty(chassis, slot, channel, "CE", &value);
    pbo->rval = value;
  } else if (command == S_RTE) {
    status = GetProperty(chassis, slot, channel, "RTE", &value);
    pbo->rval = value;
  } else if (command == S_RLY) {
    status = GetProperty(chassis, slot, channel, "RLY", &value);
    pbo->rval = value;
  } else if (command == S_HV) {
    pbo->rval = GetHV(chassis);
  }
  if (status == ERROR) {
    char alert[128];
    sprintf(alert, "Lecroy init_bo - %s(%d): Chassis=%d, Slot=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, chassis, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pbo, alert);
    return(S_db_badField);
  }
  return 0;
}


/* Name: write_bo                                                      *\
 | Parameters: pbo - Bo record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Bo record requests to the LeCroy HV   |
 |   device support.  The only binary outputs are the commanding of    |
 |   high voltage on/off for a chassis and the enable/disable of a     |
 |   channel.                                                          |
\*                                                                     */
static long write_bo(struct boRecord *pbo)
{
  STATUS status;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B is ignored and
     C is the crate, N is the slot, A is the channel and F is the command.
  */
  struct camacio *pcamacio = (struct camacio *) &(pbo->out.value);  

  unsigned chassis = pcamacio->c;
  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  /* identify the type of Bo and issue a command to the desired chassis.
   */
  if (command == S_CE)
    status = HVload(chassis, slot, channel, "CE", (pbo->rval == 0) ? "0" : "1");
  else if (command == S_RTE)
    status = HVload(chassis, slot, channel, "RTE", (pbo->rval == 0) ? "0" : "1");
  else if (command == S_RLY)
    status = HVload(chassis, slot, channel, "RLY", (pbo->rval == 0) ? "0" : "1");
  else if (command == S_HV)
    status = HVcmd(chassis, (pbo->rval == 0) ? "HVOFF" : "HVON");
  else status = ERROR;

  /* Alert if an error occures processing the request.
   */
  if (status == ERROR)
  { char alert[128];
    sprintf(alert, "Lecroy Bo - %s(%d): Chassis=%d, Slot=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, chassis, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pbo, alert);
    return(S_db_badField);
  }

  return 0;  
}


epicsExportAddress(dset, devBiLecroy);
/* Name: read_bi                                                       *\
 | Parameters: pbi - Bi record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Bi record requests to access the      |
 |   state of a LeCroy HV chassis.  The three types of binary accesses |
 |   are chassis validity, alarm status,  and high voltage on.         |
\*                                                                     */
static long read_bi(struct biRecord *pbi)
{
  int result;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B is ignored and
     C is the crate, N is the slot, A is the channel and F is the command.
  */
  struct camacio *pcamacio = (struct camacio *) &(pbi->inp.value);  

  unsigned chassis = pcamacio->c;
  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  /* Access the requested chassis's database, depending on which of the two
   * bi commands was sent.  Show an error if the request is not recognized.
   */
  if (command == G_Valid) result = GetValidity(chassis);
  else if (command == G_HV) result = GetHV(chassis);
  else if (command == G_Alarm) result = GetAlarm(chassis);
  else
  { char alert[128];
    sprintf(alert, "Lecroy Bi - %s(%d): Chassis=%d, Slot=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, chassis, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pbi, alert);
    return(S_db_badField);
  }

  /* Show error if the requested chassis does not exist.
   */
  if (result == -1)
  { recGblRecordError(S_db_badField, (void *) pbi, "No such chassis");
    return(S_db_badField);
  }
  else pbi->rval = result;

  return 0;  
}


epicsExportAddress(dset, devAoLecroy);
/* Name: init_ao                                                       *\
 | Parameters: pao - Ao record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function is called at startup of the IOC to perform any      |
 |   initialization required by the record handler.  In this case,     |
 |   the numeric control records are intialized to the status of the   |
 |   chassis.                                                          |
\*                                                                     */
static long init_ao(struct aoRecord  *pao)
{
  STATUS status;
  float value;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B is ignored and
     C is the crate, N is the slot, A is the channel and F is the command.
  */
  struct camacio *pcamacio = (struct camacio *) &(pao->out.value);  

  unsigned chassis = pcamacio->c;
  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  /* Initialize the record depending on the passed parameters.
   */
  switch (command)
  { case S_DV:
      status = GetProperty(chassis, slot, channel, "DV", &value); break;
    case S_RDN:
      status = GetProperty(chassis, slot, channel, "RDN", &value); break;
    case S_RUP:
      status = GetProperty(chassis, slot, channel, "RUP", &value); break;
    case S_TC:
      status = GetProperty(chassis, slot, channel, "TC", &value); break;
    case S_MVDZ:
      status = GetProperty(chassis, slot, channel, "MVDZ", &value); break;
    case S_MCDZ:
      status = GetProperty(chassis, slot, channel, "MCDZ", &value); break;
    case S_SOT:
      status = GetProperty(chassis, slot, channel, "SOT", &value); break;
    case S_PRD:
      status = GetProperty(chassis, slot, channel, "PRD", &value); break;
    default: status = ERROR; break;
  }

  /* Report any failure to initialize
   */
  if (status ==OK) pao->rval = value;
  else
  { char alert[128];
    sprintf(alert, "Lecroy init_ao - %s(%d): Chassis=%d, Slot=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, chassis, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pao, alert);
    return(S_db_badField);
  }

  return 0;  
}


/* Name: write_ao                                                      *\
 | Parameters: pao - Ao record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Ao record requests for the LeCroy     |
 |   HV device support.  A command string is generated, depending on   |
 |   the type record received. The HVload function is called to issue  |
 |   commands to a chassis.                                            |
\*                                                                     */
static long write_ao(struct aoRecord *pao)
{
  STATUS status = OK;
  char value[8], *property;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B is ignored and
     C is the crate, N is the slot, A is the channel and F is the command.
  */
  struct camacio *pcamacio = (struct camacio *) &(pao->out.value);  

  unsigned chassis = pcamacio->c;
  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  /* Convert the record's assignment value to ASCII for passing to HVload.
   */
  sprintf(value, "%7.1f", pao->val);

  /* Set the property name for HVload, depending on the passed command code.
   */
  switch (command)
  { case S_DV:   property = "DV"; break;
    case S_RDN:  property = "RDN"; break;
    case S_RUP:  property = "RUP"; break;
    case S_TC:   property = "TC"; break;
    case S_MVDZ: property = "MVDZ"; break;
    case S_MCDZ: property = "MCDZ"; break;
    case S_SOT:  property = "SOT"; break;
    case S_PRD:  property = "PRD"; break;
    default:
      status = ERROR;
      property = "?";
      break;
  }

  /* Command the property of one channel on a HV module in a slot of the
   * chassis to the desired value.
   */
  if (status == OK)
    status = HVload(chassis, slot, channel, property, value);

  if (status == ERROR)
  { char alert[128];
    sprintf(alert, "Lecroy Ao - %s(%d): Chassis=%d, Slot=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, chassis, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pao, alert);
    return(S_db_badField);
  }

  return 0;  
}
