/*****************************************************************************\
 * File: lecroy.c                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains functions related to the LeCroy 1458 HV mainframe    *
 *   and its associated hardware modules.  The functions read the state of   *
 *   the chassis and issue user requests to achieve desired states.  The     *
 *   lower level I/O functions are found in socket.c.  Note that summary     *
 *   numbers are used to detect when the state of the LeCroy has changed and *
 *   specific values need to be updated (see LeCroy documentation).          *
 *                                                                           *
 * Revision History:                                                         *
 *   12/22/2000 - Initial release                                            *
 *   04/11/2001 - Modified alarm status for release 1-1                      *
\*****************************************************************************/

//#include "vxWorks.h"
#include "time.h"
#include "lecroy.h"
#include "define.h"
#include "math.h"
#include "extern.h"
#include "dbDefs.h"

static int ScanUnit(Chassis *, unsigned, BOOL);
static int CfgCmd(Chassis *);
static BOOL Alarm1461(HV1461 *);
static BOOL Alarm1471(HV1471 *);
static BOOL Alarm1468(HV1468 *);
static BOOL Alarm1469(HV1469 *);

/*                                                                           *\
 | Name: GetConfig                                                           |
 | Parameters: chassis - Pointer to instance of a Lecroy chassis             |
 | Return: 0 OK;  < 0 Error                                                  |
 | Remarks:                                                                  |
 |   This function detects the configuration of the LeCroy 1458 chassis.  It |
 |   does this by issuing 1458 commands and parsing the responses.  Memory   |
 |   is allocated for each hardware module detected.                         |
\*                                                                           */
int GetConfig(Chassis *chassis)
{
  char *ptr;
  int lines;
  unsigned i, index, slots[MAX_UNITS];
  char IDcmd[] = "ID L000";

  /* Issue the Logical unit List command to find the slots containing one
   * or more logical units (modules and submodules).
   */
  lines = ARCsend(chassis->m_sock, "LL", chassis->m_reply, REPLY_SIZE);
  if (lines != 1) return ErrReport("No logical units", R_Fatal);

  /* The number of spaces in the response equals the number of logical units
   * in the chassis (a space delimits each identifier).  Get the slot numbers
   * while counting the number of logical units.  Then allocate an array of
   * "LogUnit" structures for this chassis.
   */
  ptr = chassis->m_reply;
  while ((ptr = strchr(ptr, ' ')) != NULL)
  { if (chassis->m_units >= MAX_UNITS)
      return ErrReport("Too many units", R_Fatal);
    if (*(++ptr) != 'S') return ErrReport("Format Error", R_Fatal);
    slots[chassis->m_units] = atoi(++ptr);
    if (slots[chassis->m_units] >= NUM_SLOTS)
      return ErrReport("Slot # error", R_Fatal);
    chassis->m_units++;
  }
  chassis->m_logUnit = (LogUnit *) calloc(chassis->m_units, sizeof(LogUnit));

  /* Send Global and Logical Unit summary commands to initialize the summary
   * counts to the values stored in the chassis.  Note that the "LS" command
   * response may be more than one line (sting in m_reply).  These lines
   * are simply concatenated by replacing the string terminator with a space.
   */
  if (ARCsend(chassis->m_sock, "GS", chassis->m_reply, REPLY_SIZE) != 1)
    return ErrReport("Global Summary", R_Warn);
  sscanf(&chassis->m_reply[3], "%x", &chassis->m_sumM);
  sscanf(&chassis->m_reply[8], "%x", &chassis->m_sumS);
  sscanf(&chassis->m_reply[13], "%x", &chassis->m_sumC);

  lines = ARCsend(chassis->m_sock, "LS", chassis->m_reply, REPLY_SIZE);
  if (lines != 1) return ErrReport("Logical Summary", R_Warn);
  ptr = chassis->m_reply;
  for (i = 0, index = 3; i < chassis->m_units; i++)
  { sscanf(&ptr[index], "%x", &chassis->m_logUnit[i].sumM);
    sscanf(&ptr[index + 5], "%x", &chassis->m_logUnit[i].sumS);
    index += 10;
  }

  CfgCmd(chassis);                                   /* Get chassis setings */

  /* For each logical unit in the chassis, identify the type of module and
   * allocate appropriate channel data structures.  Note that HVcards with
   * submodules are detected because the submodules share a slot.
   */
  for (i = 0; i < chassis->m_units; i++)
  { LogUnit *uptr = &chassis->m_logUnit[i];
    unsigned slot = slots[i];
    char *p, *type, polarity;

    /* A used slot means that a submodule logical unit is being processed.
     * Otherwise ID the HV hardware module.
     */
    if (chassis->m_slot[slot] == NULL)
    {
      /* Format an ID command and send.  Look at the response to determine
       * board type and polarity (+ or -).
       */
      sprintf(&IDcmd[4], "%3.3u", i);
      lines = ARCsend(chassis->m_sock, IDcmd, chassis->m_reply, REPLY_SIZE);
      if (lines != 1) return ErrReport(chassis->m_reply, R_Fatal);
      p = strchr(chassis->m_reply,' ');
      if(p) p = strchr(p+1,' ');
      if(p==NULL) return ErrReport(chassis->m_reply, R_Fatal);
      type = p+1;
      //      type = chassis->m_reply + strlen(IDcmd) + 1;
      polarity = type[4];

      /* Allocate memory appropriate for the type of board.  Note that
       * the 1469 board has two submodules, so additional initialization
       * is required.
       */
      if (memcmp(type, "1461", 4) == 0)
      { chassis->m_slot[slot] = (HVgeneric *) calloc(1, sizeof(HV1461));
        chassis->m_slot[slot]->id = HV_1461;
      }
      else if (memcmp(type, "1471", 4) == 0)
      { chassis->m_slot[slot] = (HVgeneric *) calloc(1, sizeof(HV1471));
        chassis->m_slot[slot]->id = HV_1471;
      }
      else if (memcmp(type, "1468", 4) == 0)
      { chassis->m_slot[slot] = (HVgeneric *) calloc(1, sizeof(HV1468));
        chassis->m_slot[slot]->id = HV_1468;
      }
      else if (memcmp(type, "1469", 4) == 0)
      { chassis->m_slot[slot] = (HVgeneric *) calloc(1, sizeof(HV1469));
        chassis->m_slot[slot]->id = HV_1469;
        uptr[1].slot = slot;               /* define additional logical unit */
        uptr[1].submodule = 1;
      }
      else return ErrReport(chassis->m_reply, R_Fatal);

      /* Set Chassis and LogUnit variables common to all board types.
       * Then get the initial value for all properties of all channels.
       */
      chassis->m_slot[slot]->polarity = polarity;
      chassis->m_slot[slot]->unit = i;
      uptr->slot = slot;
      uptr->submodule = 0;
    }

    /* Get initial values for all properties on all channels of this unit
     */
    ScanUnit(chassis, i, TRUE);
  }

  return 0;
}


/*                                                                           * \
 | Name: FlushQ                                                              |
 | Parameters: chassis - Pointer to instance of a Lecroy chassis             |
 | Return: 0 success,  < 0 error                                             |
 | Remarks:                                                                  |
 |   This function removes all pending commands from this chassis' message   |
 |   queue.  Each command is issued to the LeCroy unit.  Note that it will   |
 |   not wait for messages to arrive in an empty queue, but simply returns.  |
 |   Also note that the LeCroy responses are ignored, as long as they are    |
 |   not indicating an error.
\*                                                                           */
int FlushQ(Chassis *chassis)
{
  char command[MSGMAX];

  while (epicsMessageQueueReceiveWithTimeout(chassis->m_queue,
					     command, MSGMAX, 0.0 ) > 0)
    if (ARCsend(chassis->m_sock, command, chassis->m_reply, REPLY_SIZE) < 1)
      return ErrReport("Rejected", R_Warn);

  return 0;
}

/*                                                                           *\
 | Name: CheckStatus                                                         |
 | Parameters: chassis - Pointer to instance of a Lecroy chassis             |
 | Return: 0 success, < 0 error                                              |
 | Remarks:                                                                  |
 |   This function is called to determine is a significant change in the     |
 |   state on the chassis has occured.  It does this by us of the chassis    |
 |   summary numbers (see LeCroy documentation), which are a shortcut to     |
 |   finding what needs to be re-read from the chassis.  It would be a lot   |
 |   of I/O to re-read every property of every channel on all cards each     |
 |   update cycle.  When state changes occur, dig deeper to know exactly     |
 |   what needs to be updated.  The actual property updates are performed    |
 |   by CheckUnit().  Note that some chassis specific information is         |
 |   obtained each cycle.                                                    |
\*                                                                           */
int CheckStatus(Chassis *chassis)
{
  unsigned measured, settable, configure;
  unsigned i, index;
  int lines;

  /* Issue the Global Summary command and decode the first two 16 bit
   * fields.  If they differ from the local database, investigate further
   */
  if (ARCsend(chassis->m_sock, "GS", chassis->m_reply, REPLY_SIZE) != 1) {
    /* Try to reconnect */
    chassis->m_sock = ARCreconnect(chassis->m_sock, chassis->m_ipaddr);
    return ErrReport("Global Summary", R_Warn);
  }
  sscanf(&chassis->m_reply[3], "%x", &measured);
  sscanf(&chassis->m_reply[8], "%x", &settable);
  sscanf(&chassis->m_reply[13], "%x", &configure);

  /* If the chassis configuration has changed, update it.
   */
  if (configure != chassis->m_sumC)
  { chassis->m_sumC = configure;
    CfgCmd(chassis);
  }

  if (measured != chassis->m_sumM || settable != chassis->m_sumS)
  { chassis->m_sumM = measured;                   /* Overwrite old with new */
    chassis->m_sumS = settable;

    /* Issue the Logical Unit Summary command to show which units have
     * channel that have changes.  Note that the "LS" command response may
     * be more than one line (sting in m_reply).  These lines are simply
     * concatenated by replacing the string terminator with a space.  Decode
     * and save the returned summary numbers for late comparison.
     */
    lines = ARCsend(chassis->m_sock, "LS", chassis->m_reply, REPLY_SIZE);
    if (lines != 1) return ErrReport("Logical Summary", R_Warn);

    for (i = 0, index = 3; i < chassis->m_units; i++)
    { LogUnit *uptr = &chassis->m_logUnit[i];
      sscanf(&chassis->m_reply[index], "%x", &uptr->oldM);
      sscanf(&chassis->m_reply[index + 5], "%x", &uptr->oldS);
      index += 10;
    }

    /* Compare the returned summary numbers with the ones stored in the
     * chassis database.  Each difference corresponds to a logical unit
     * that has properties that have changed.
     */
    for (i = 0; i < chassis->m_units; i++)
    { LogUnit *uptr = &chassis->m_logUnit[i];

      if (uptr->sumM != uptr->oldM || uptr->sumS != uptr->oldS)
      { uptr->sumM = uptr->oldM;                 /* Overwrite old with new */
        uptr->sumS = uptr->oldS;
        if (ScanUnit(chassis, i, FALSE) < 0) {
	  printf("Log unit = %d\n",i);
          return ErrReport("Log Unit Error", R_Warn);
	}
      }
    }
  }

  return 0;
}


/*                                                                           *\
 | Name: ChassisAlarm                                                        |
 | Parameters: chassis - Pointer to instance of a Lecroy chassis             |
 | Return: TRUE if the chassis has any alarm set                             |
 | Remarks:                                                                  |
 |   This function is called to determine if an alarm is set for any         |
 |   channels on any HV module in the specified chassis.  An alarm           |
 |   condition exists when any demand voltage other than zero exists         |
 |   with the corresponding measured voltage deviating by 10 or more         |
 |   volts.                                                                  |
\*                                                                           */
BOOL ChassisAlarm(Chassis *chassis)
{
  unsigned slot;
  BOOL alarm = FALSE;

  /* The channel parameters are accessed by multiple tasks, so mutual
   * exclusion needs to occur for reliable data transfer.
   */
  if (epicsMutexLock(chassis->m_mutex) == epicsMutexLockError) return TRUE;
  // or != epicsMutexLockOK

  /* Look at each slot in the chassis to see if a HV module is there.
   * If so, call the corresponding alarm function to determine if any
   * of the channels is deviating.
   */
  for (slot = 0; slot < NUM_SLOTS; slot++)
  { HVgeneric *HV = chassis->m_slot[slot];
    if (HV != NULL)
    { switch (HV->id)
      { case HV_1461:
          if (Alarm1461((HV1461 *) HV)) alarm = TRUE;
          break;
        case HV_1471:
          if (Alarm1471((HV1471 *) HV)) alarm = TRUE;
          break;
        case HV_1468:
          if (Alarm1468((HV1468 *) HV)) alarm = TRUE;
          break;
        case HV_1469:
          if (Alarm1469((HV1469 *) HV)) alarm = TRUE;
          break;
      }
    }
  }

  /* Restore access to other tasks and return the alarm status of the chassis
   */
  epicsMutexUnlock(chassis->m_mutex);
  return alarm;
}


/*                                                                           *\
 | Name: Alarm1461                                                           |
 | Parameters: HV - pointer to a HV1461 module's data structure              |
 | Return: TRUE if a any channel has an alarm set                            |
 | Remarks:                                                                  |
 |   This function is called to determine if an alarm is set for any         |
 |   channels on the HV module.  An alarm condition exists when any          |
 |   demand voltage other than zero exists with the corresponding measured   |
 |   voltage deviating by 10 or more volts.  Note that no alarm exists       |
 |   when the channel is manually disabled (not by a trip).                  |
\*                                                                           */
static BOOL Alarm1461(HV1461 *hv)
{
  unsigned i;

  /* look at each channel on the HV module.
   */
  for (i = 0; i < NUM_1461_CHAN; i++)
  { HV1461Channel *ch = &hv->m_chan[i];
    if (ch->property.DV != 0. &&
       (ch->property.CE != 0. || ch->property.ST >= 16) &&
       (fabs(ch->property.DV - ch->property.MV) >= 10.)) return TRUE;
  }

  return FALSE;
}

/*                                                                           *\
 | Name: Alarm1471                                                           |
 | Parameters: HV - pointer to a HV1471 module's data structure              |
 | Return: TRUE if a any channel has an alarm set                            |
 | Remarks:                                                                  |
 |   This function is called to determine if an alarm is set for any         |
 |   channels on the HV module.  An alarm condition exists when any          |
 |   demand voltage other than zero exists with the corresponding measured   |
 |   voltage deviating by 10 or more volts.  Note that no alarm exists       |
 |   when the channel is manually disabled (not by a trip).                  |
\*                                                                           */
static BOOL Alarm1471(HV1471 *hv)
{
  unsigned i;

  /* look at each channel on the HV module.
   */
  for (i = 0; i < NUM_1471_CHAN; i++)
  { HV1471Channel *ch = &hv->m_chan[i];
    if (ch->property.DV != 0. &&
       (ch->property.CE != 0. || ch->property.ST >= 16) &&
       (fabs(ch->property.DV - ch->property.MV) >= 10.)) return TRUE;
  }

  return FALSE;
}


/*                                                                           *\
 | Name: Alarm1468                                                           |
 | Parameters: HV - pointer to a HV1468 module's data structure              |
 | Return: TRUE if a any channel has an alarm set                            |
 | Remarks:                                                                  |
 |   This function is called to determine if an alarm is set for any         |
 |   channels on the HV module.  An alarm condition exists when any          |
 |   demand voltage other than zero exists with the corresponding measured   |
 |   voltage deviating by 10 or more volts.  Note that no alarm exists       |
 |   when the channel is manually disabled (not by a trip).                  |
\*                                                                           */
static BOOL Alarm1468(HV1468 *hv)
{
  unsigned i;

  /* look at each channel on the HV module.
   */
  for (i = 0; i < NUM_1468_CHAN; i++)
  { HV1468Channel *ch = &hv->m_chan[i];
    if (ch->property.DV != 0. &&
       (ch->property.CE != 0. || ch->property.ST >= 16) &&
       (fabs(ch->property.DV - ch->property.MV) >= 10.)) return TRUE;
  }

  return FALSE;
}


/*                                                                           *\
 | Name: Alarm1469                                                           |
 | Parameters: HV - pointer to a HV1469 module's data structure              |
 | Return: TRUE if a any generator has an alarm set                          |
 | Remarks:                                                                  |
 |   This function is called to determine if an alarm is set for any         |
 |   generators on the HV module.  An alarm condition exists when any        |
 |   demand voltage other than zero exists with the corresponding measured   |
 |   voltage deviating by 10 or more volts.  Note that no alarm exists       |
 |   when the channel is manually disabled (not by a trip).                  |
\*                                                                           */
static BOOL Alarm1469(HV1469 *hv)
{
  unsigned i;

  /* look at each channel on the HV module.
   */
  for (i = 0; i < NUM_1469_GEN; i++)
  { HV1469Generator *gn = &hv->m_gen[i];
    if (gn->property.DV != 0. &&
       (gn->property.CE != 0. || gn->property.ST >= 16) &&
       (fabs(gn->property.DV - gn->property.MV) >= 10.)) return TRUE;
  }

  return FALSE;
}


/*                                                                           *\
 | Name: ScanUnit                                                            |
 | Parameters: chassis - Pointer to instance of a Lecroy chassis             |
 |             unit - logical unit number of module                          |
 |             ScanAll - ignore summaries and and scan all properties        |
 | Return: 0 success, < 0 error                                              |
 | Remarks:                                                                  |
 |   Look at a hardware module (1461 ...) to read and save proprty values    |
 |   as needed.  If scanAll is false, only the properties indicated by       |
 |   summary number changes will be obtained from the chassis for this       |
 |   logical unit.  Summary numbers do not indicate a channel that           |
 |   has changed, but a property on a logical unit.  Therefore the Recall    |
 |   command is issued to get property values for all channels on a unit.    |
 |   Note that the properties listed in the "const char *property" list      |
 |   must match the definition in the chassis database (lecroy.h).           |
\*                                                                           */
static int ScanUnit(Chassis *chassis, unsigned unit, BOOL scanAll)
{
  unsigned i, j, sum, propCnt, *sumList, chanCnt, submodule = 0, slot;
  BOOL doProp[MAX_PROPS];
  char RCcmd[] = "RC L000 CCCC", PScmd[] = "PS L000";
  char *ptr;
  const char **prpList;
  float *fptr;
  HVgeneric *HV;

  /* Format and issue the property summary command for this logical unit.
   */
  sprintf(&PScmd[4], "%3.3u", unit);
  if (ARCsend(chassis->m_sock, PScmd, chassis->m_reply, REPLY_SIZE) != 1)
    return ErrReport(chassis->m_reply, R_Warn);

  /* Determine the type of HV unit and set local variables accordingly.
   */
  slot = chassis->m_logUnit[unit].slot;
  HV = chassis->m_slot[slot];
  switch (HV->id)
  { case HV_1461:
      propCnt = NUM_1461_PROP;
      sumList = ((HV1461 *) HV)->m_sum;
      prpList = prp1461;
      chanCnt = NUM_1461_CHAN;
      break;
    case HV_1471:
      propCnt = NUM_1471_PROP;
      sumList = ((HV1471 *) HV)->m_sum;
      prpList = prp1471;
      chanCnt = NUM_1471_CHAN;
      break;
    case HV_1468: 
      propCnt = NUM_1468_PROP;
      sumList = ((HV1468 *) HV)->m_sum;
      prpList = prp1468;
      chanCnt = NUM_1468_CHAN;
      break;
    case HV_1469:
      if ((submodule = chassis->m_logUnit[unit].submodule) == 0)
      { propCnt = NUM_1469_PROP;
        sumList = ((HV1469 *) HV)->m_sumGen;  
        prpList = prp1469;
        chanCnt = NUM_1469_GEN;
      }
      else
      { propCnt = NUM_1469_SUBPROP;
        sumList = ((HV1469 *) HV)->m_sumChan;  
        prpList = prp1469s;
        chanCnt = NUM_1469_CHAN;
      }
      break;
    default: return ErrReport("Unknown board type", R_Warn);
  }

  /* Use summary numbers, or scanALL to determine which properties of
   * this unit will be obtained.  Also update to the new summary values.
   */
  /* The LNNN in the response can be variable width.  Advance ptr to
     the first space after the L field. */
  ptr = chassis->m_reply+3;
  if(*ptr != 'L') return ErrReport(chassis->m_reply, R_Warn);
  ptr = strchr(ptr,' ');
  if(!ptr) return ErrReport(chassis->m_reply, R_Warn);
  //  for (i = 0, ptr = chassis->m_reply + 8; i < propCnt; i++)
  for (i = 0; i < propCnt; i++)
  { sscanf(ptr, "%x", &sum);
    ptr += 5;

    if (scanAll) doProp[i] = TRUE;
    else doProp[i] = (sum == sumList[i]) ? FALSE : TRUE;

    sumList[i] = sum;
  }

  /* For all properties that are to be updated, issue a Recall (RC) command.
   * The response will have that property value for each channel on this
   * logical unit.  Grab a mutual exclusive lock, since other tasks may be
   * accessing property values, and update the chassis database.
   */
  for (i = 0; i < propCnt; i++)
  { if (doProp[i])
    { sprintf(RCcmd, "RC L%3.3u %s", unit, prpList[i]);
      if (ARCsend(chassis->m_sock, RCcmd, chassis->m_reply, REPLY_SIZE) != 1)
        return ErrReport(chassis->m_reply, R_Warn);
      //      printf("Got reply %s\n",chassis->m_reply);
      /* Point to the first returned data value in the response.
       */
      //      ptr = chassis->m_reply + 9 + strlen(prpList[i]);
      ptr = strstr(chassis->m_reply,prpList[i]);
      if(!ptr) {
	return ErrReport(chassis->m_reply, R_Warn);
      }
      ptr += strlen(prpList[i]);
      if(strlen(ptr) < 3) {
	// string not long enough to have information
	if(strcmp(prpList[i],"RLY")==0) {
	  // If relays are not configured that's OK
	  continue;
	}
	return ErrReport(chassis->m_reply, R_Warn);
      }
      ptr += 1;

      /* A property update is perfromed for each channel "j".  Where to
       * store the decoded value depends on the kind of module, property,
       * and channel.  Note that mutual exclusion is performed so values
       * can be stored in a database shared with EPICS device access
       * routines (see access.c).
       */
      if (epicsMutexLock(chassis->m_mutex) == epicsMutexLockError) return TRUE;
      // or != epicsMutexLockOK
      { for (j = 0; j < chanCnt; j++)
        { switch(HV->id)
          { case HV_1461:
              fptr =  &((HV1461 *) HV)->m_chan[j].properties[i];
              break;
	    case HV_1471:
              fptr =  &((HV1471 *) HV)->m_chan[j].properties[i];
              break;
            case HV_1468:
              fptr =  &((HV1468 *) HV)->m_chan[j].properties[i];
              break;
            case HV_1469:
              fptr = (submodule == 0) ?
                &((HV1469 *) HV)->m_gen[j].properties[i] :
                &((HV1469 *) HV)->m_chan[j].properties[i];
              break;
            default: return R_Warn;
          }

          /* All of the properties values, excpert for ST, may be read as
           * floating point data.  The ST is a hexadecimal integer.
           */
          if (strcmp(prpList[i], "ST") != 0) *fptr = atof(ptr);
          else *fptr = (float) strtoul(ptr, NULL, 16);

          /* advance along the response to the next channels's propery value.
           */
          ptr = strchr(ptr, ' ') + 1;
        }
	epicsMutexUnlock(chassis->m_mutex);
      }
    }
  }

  return 0;
}


/*                                                                           *\
 | Name: CfgCmd                                                              |
 | Parameters: chassis - Pointer to instance of a Lecroy chassis             |
 | Remarks:                                                                  |
 |   Issue the CONFIG command and save the first 16 bit field of the         |
 |   response.  Certain bits of the field are also tested to determine       |
 |   the ON/OFF status of the chassis.                                       |
\*                                                                           */
static int CfgCmd(Chassis *chassis)
{
  if (ARCsend(chassis->m_sock, "CONFIG", chassis->m_reply, REPLY_SIZE) != 1)
    return ErrReport("CONFIG", R_Warn);
  sscanf(&chassis->m_reply[7], "%x", &chassis->m_config0);
  chassis->m_HVstatus = ((chassis->m_config0 & 0x6000) == 0x2000);
  return 0;
}


/*                                                                           *\
 | Name: ShowCard                                                            |
 | Parameters:  HVg - pointer to a HV module                                 |
 | Remarks:                                                                  |
 |   This function works in conjunction with the "hinv" function to print    |
 |   configuration summary information to the console.  The type of module   |
 |   is identified and pertinent values printed for each channel.            |
\*                                                                           */
static void ShowCard(HVgeneric *HVg)
{
  int i, chanCnt;

  switch (HVg->id)
  { case HV_1461:
    case HV_1468:
      { HV1461 *HV = (HV1461 *) HVg;
        chanCnt = (HVg->id == HV_1461) ? NUM_1461_CHAN : NUM_1468_CHAN;
        for (i = 0 ; i < chanCnt; i++)
          printf("%2.2X", (int) HV->m_chan[i].property.ST);
        printf("}\n");
      }
      break;
    case HV_1471:
      { HV1471 *HV = (HV1471 *) HVg;
        for (i = 0 ; i < NUM_1471_CHAN; i++)
          printf("%2.2X", (int) HV->m_chan[i].property.ST);
        printf("}\n");
      }
      break;
    case HV_1469:
      { HV1469 *HV = (HV1469 *) HVg;
        for (i = 0 ; i < NUM_1469_CHAN; i++)
          printf("%2.2X", (int) HV->m_chan[i].property.ST);
        printf("}\n");
      }
      break;
    default:  break;
  }
}


//epicsRegisterFunction(hinv);
/*                                                                           * \
 | Name: hinv (hardware inventory)                                           |
 | Return: 0 success                                                         |
 | Remarks:                                                                  |
 |   This function is called to printf LeCroy hardware module information,   |
 |   for each chassis, to the console.                                       |
\*                                                                           */
int hinv(void)
{
  unsigned i, slot;
  char *type[] = {"1461", "1468", "1469", "1471"}, *ptr;
  time_t ltime;

  time(&ltime);
  printf("\nHardware Inventory for LeCroy Units - %s", ctime(&ltime));

  /* for each chassis, print chassis data, then look at each slot to print
   * any module specific information.
   */
  for (i = 0; i < g_chassisCnt; i++)
  { Chassis *chassis = g_chassis[i];

    if (!chassis->m_valid) ptr = "INVALID";
    else ptr = (chassis->m_HVstatus) ? "ON" : "OFF"; 
    printf("\nChassis #%u (%s): FR=%u OV=%u CFG=%4.4X  IP address=%s\n",
	   chassis->m_hvID, ptr, chassis->m_frameCnt,
	   chassis->m_overflow, chassis->m_config0,
	   chassis->m_ipaddr);

    for (slot = 0; slot < NUM_SLOTS; slot++)
    { HVgeneric *HV = chassis->m_slot[slot];
      if (HV != NULL)
      { printf("  Slot %2.2d: %s%c {", slot, type[HV->id], HV->polarity);
        ShowCard(HV);
      }
    }
  }

  /* Show the network communications statistics for all chassis's
   */
  ComStats();

  return 0;    
}
