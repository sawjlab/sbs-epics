/*****************************************************************************	\
 * File: devV65XX.c                                Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains the EPICS record handlers for the custom V65XX HV   *
 *   device support layer.  The functions call driver support modules which  *
 *   are in the file access.c.                                               *
 *                                                                           *
 * Revision History:                                                         *
 *   01/04/2001 - Initial release                                            *
\*****************************************************************************/

#include "stdio.h"
#include <alarm.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <recSup.h>
#include <devSup.h>
#include <boRecord.h>
#include <biRecord.h>
#include <aoRecord.h>
#include <aiRecord.h>
#include <epicsExport.h>
//#include "command.h"
#include "define.h"
#include "extern.h"

static long init_bo(struct boRecord *);
static long write_bo(struct boRecord *);
static long read_bi(struct biRecord *);
static long init_ao(struct aoRecord *);
static long write_ao(struct aoRecord *);
static long read_ai(struct aiRecord *);

/* Thses EPICS structures associate the V65XXDev records with the
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
} devBoV65XX = {5, NULL, NULL, init_bo, NULL, write_bo};

struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read_bi;
} devBiV65XX = {5, NULL, NULL, NULL, NULL, read_bi};

struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN write_ao;
  DEVSUPFUN special_linconv;
} devAoV65XX = {6, NULL, NULL, init_ao, NULL, write_ao, NULL};

struct
{ long number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read_ai;
  DEVSUPFUN special_linconv;
} devAiV65XX = {6, NULL, NULL, NULL, NULL, read_ai, NULL};

epicsExportAddress(dset, devBoV65XX);
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
  //  float value;
  int value;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B and C are ignored
     and N is the slot, A is the channel and F is the command.
   */
  struct camacio *pcamacio = (struct camacio *) &(pbo->out.value);  

  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;
  
  if (V65XX_Get(command, slot, channel, &value) == ERROR) {
    char alert[128];
    sprintf(alert, "V65XX init_bo - %s(%d): Card=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pbo, alert);
    return(S_db_badField);
  }

  pbo->rval = value;

  return 0;
}


/* Name: write_bo                                                      * \
 | Parameters: pbo - Bo record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Bo record requests to the V65XX HV   |
 |   device support.  The only binary outputs are the commanding of    |
 |   high voltage on/off for a chassis and the enable/disable of a     |
 |   channel.                                                          |
\*                                                                     */
static long write_bo(struct boRecord *pbo)
{
  int value;
  int readback;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B and C are ignored
     and N is the slot, A is the channel and F is the command.
   */
  struct camacio *pcamacio = (struct camacio *) &(pbo->out.value);  

  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  value = (pbo->rval==0)?0:1;
  if (V65XX_Set(command, slot, channel, value, &readback) == ERROR) {
    char alert[128];
    sprintf(alert, "V65XX Bo - %s(%d): Card=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pbo, alert);
    return(S_db_badField);
  }
  pbo->rval = (readback==0)?0:1;
  
  return 0;  
}

epicsExportAddress(dset, devBiV65XX);
/* Name: read_bi                                                       *\
 | Parameters: pbi - Bi record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Bi record requests to access the      |
 |   state of a V65XX HV chassis.  The three types of binary accesses |
 |   are chassis validity, alarm status,  and high voltage on.         |
\*                                                                     */
static long read_bi(struct biRecord *pbi)
{
  int value;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B and C are ignored
     and N is the slot, A is the channel and F is the command.
   */
  struct camacio *pcamacio = (struct camacio *) &(pbi->inp.value);  

  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  if (V65XX_Get(command, slot, channel, &value) == ERROR) {
    char alert[128];
    sprintf(alert, "V65XX read_bi - %s(%d): Card=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pbi, alert);
    return(S_db_badField);
  }

  pbi->rval = value;

  return 0;  
}


epicsExportAddress(dset, devAoV65XX);
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
  int value;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B and C are ignored
     and N is the slot, A is the channel and F is the command.
   */
  struct camacio *pcamacio = (struct camacio *) &(pao->out.value);  

  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  if (V65XX_Get(command, slot, channel, &value) == ERROR) {
    char alert[128];
    sprintf(alert, "V65XX init_ao - %s(%d): Card=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pao, alert);
    return(S_db_badField);
  }
  pao->rval = value;

  return 0;  
}


/* Name: write_ao                                                      *\
 | Parameters: pao - Ao record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Ao record requests for the V65XX      |
 |   HV device support.  A command string is generated, depending on   |
 |   the type record received. The HVload function is called to issue  |
 |   commands to a chassis.                                            |
\*                                                                     */
static long write_ao(struct aoRecord *pao)
{
  int value;
  int readback;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B and C are ignored
     and N is the slot, A is the channel and F is the command.
   */
  struct camacio *pcamacio = (struct camacio *) &(pao->out.value);  

  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  value = pao->rval;
  if (V65XX_Set(command, slot, channel, value, &readback) == ERROR) {
    char alert[128];
    sprintf(alert, "V65XX Ao - %s(%d): Card=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pao, alert);
    return(S_db_badField);
  }
  pao->rval = readback;

  return 0;  
}

epicsExportAddress(dset, devAiV65XX);
/* Name: read_ai                                                      *\
 | Parameters: pai - Ai record pointer                                 |
 | Return: Error status, zero is normal return                         |
 | Remarks:                                                            |
 |   This function handles EPICS Ao record requests for the V65XX      |
 |   HV device support.
\*                                                                     */
static long read_ai(struct aiRecord *pai)
{
  int value;

  /* Get the card & signal numbers from the record, pointed to by pbo.
     Use the CAMAC structure of B, C, N, A, F where B and C are ignored
     and N is the slot, A is the channel and F is the command.
   */
  struct camacio *pcamacio = (struct camacio *) &(pai->inp.value);  

  unsigned slot = pcamacio->n;
  unsigned channel = pcamacio->a;
  unsigned command = pcamacio->f;

  if (V65XX_Get(command, slot, channel, &value) == ERROR) {
    char alert[128];
    sprintf(alert, "V65XX Ao - %s(%d): Card=%d, Chan=%d, Command=%d",
	    __FILE__, __LINE__, slot, channel, command);
    recGblRecordError(S_db_badField, (void *) pai, alert);
    return(S_db_badField);
  }
  pai->rval = value;

  return 0;  
}
