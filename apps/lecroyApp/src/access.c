/*****************************************************************************\
 * File: access.c                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains functions for interacting with a LeCroy 1458         *
 *   chassis from tasks outside the chassis's maintenance task, such as      *
 *   EPICS device support.  Included are routines to access chassis          *
 *   database  values and to send chassis commands.                          *
 *                                                                           *
 * Revision History:                                                         *
 *   12/22/2000 - Initial release                                            *
 *   04/11/2001 - Modified to include additional alarm logic for release 1-1 *
\*****************************************************************************/

//#include "vxWorks.h"
#include "math.h"
#include "lecroy.h"
#include "extern.h"

int tickMgr(void *param);

#ifdef vxWorks
static void dummything()
{
  /* Makes sure that hvstart.c's symbols appear in munch file */
  tickMgr((void *)0);
}
#endif

/*                                                                           *\
 | Name: Enqueue                                                             |
 | Parameters: chassis - pointer to a LeCroy chassis instance                |
 |             cmd - text string with valid chassis command                  |
 | Return: OK or ERROR                                                       |
 | Remarks:                                                                  |
 |   This function inserts the passed command into a chassis task's message  |
 |   queue (see VxWorks).  The task's watchdog timer is first cancelled,     |
 |   then the insertion is made.  The watchdog timer is resarted with one    |
 |   clock tick remaining to force prompt handling of the command.           |
 |   Subsequent watchdog timers will revert back to the tasks original       |
 |   update rate.                                                            |
\*                                                                           */
static STATUS Enqueue(Chassis *chassis, char *cmd)
{
  STATUS status;

  epicsTimerCancel(chassis->m_timerid);
  status = epicsMessageQueueSendWithTimeout(chassis->m_queue,
					    cmd, strlen(cmd) + 1,
					    0.0);
  epicsTimerStartDelay(chassis->m_timerid, 0.01);
  return status;
}


/*                                                                           *\
 | Name: GetChassis                                                          |
 | Parameters: id - chassis assigned ID number                               |
 | Return: Pointer to located chassis instance (or NULL)                     |
 | Remarks:                                                                  |
 |   This function is passed a LeCroy chassis ID number and uses it to look  |
 |   up a chassis instance pointer in the global chassis table.  It returns  |
 |   a NULL pointer if a chassis with that ID does not exist.                |
\*                                                                           */
static Chassis *GetChassis(unsigned id)
{
  Chassis *chassis;
  unsigned i;

  for (i = 0; i < g_chassisCnt; i++)
    if ((chassis = g_chassis[i])->m_hvID == id) return chassis;

  return NULL;
}


/*                                                                           *\
 | Name: GetSlot                                                             |
 | Parameters: id - chassis assigned ID number                               |
 |             slot - slot number in chassis                                 |
 |             chassis - pointer to a Chassis pointer for return             |
 | Return: Pointer to HV module (or NULL)                                    |
 | Remarks:                                                                  |
 |   This function is passed a LeCroy chassis ID number and slot number and  |
 |   returns a pointer to the corresponding HV nodule.                       |
\*                                                                           */
static HVgeneric *GetSlot(unsigned id, unsigned slot, Chassis **chassis)
{
  /* Get a pointer to the chassis instance associated with the passed ID.
   */
  *chassis = GetChassis(id);
  if (*chassis == NULL) return NULL;

  if (slot >= NUM_SLOTS) return NULL;      /* Valid slot number ?            */
  return (*chassis)->m_slot[slot];         /* Pointer to module in the slot. */
}


/*                                                                           *\
 | Name: HVcmd                                                               |
 | Parameters: id - chassis assigned ID number                               |
 |             cmd - text string with valid chassis command                  |
 | Return: 0 success, -1 failure                                             |
 | Remarks:                                                                  |
 |   This function finds the passed chassis ID and inserts the passed        |
 |   command into the chassis task's message via the Enqueue() function.     |
\*                                                                           */
STATUS HVcmd(unsigned id, char *cmd)
{
  Chassis *chassis = GetChassis(id);
  return (chassis == NULL) ? ERROR : Enqueue(chassis, cmd);
}


/*                                                                           *\
 | Name: HVload                                                              |
 | Parameters: id - chassis assigned ID number                               |
 |             slot - slot number within chassis                             |
 |             channel - channel number on LeCroy module                     |
 |             property - LeCroy module property name                        |
 |             value - value to assign to the property                       |
 | Return: OK or ERROR                                                       |
 | Remarks:                                                                  |
 |   This function issues a LeCroy load (LD) command to set a property of    |
 |   a channel to a desired value.  A text command is formed depending on    |
 |   the passed parameters, which is sent to the Enqueue() function to       |
 |   give to the selected chassis.                                           |
\*                                                                           */
STATUS HVload(unsigned id, unsigned slot, unsigned channel,
              char *property, char *value)
{
  unsigned chCount, unit;
  char cmd[128];
  Chassis *chassis;
  HVgeneric *hv = GetSlot(id, slot, &chassis);
  if (hv == NULL) return ERROR;            /* Is there a module there ?      */

  /* The command that needs to be formed depends on the type of HV module in
   * the designated slot.  The information needed, besides what was passed
   * to this function, is the logical unit number.  This is stored in the
   * module's data structure, but may need to be incremented if the module
   * is a 1469.  When it is a 1469, bit 7 of the channel number is a flag
   * that indicates if the target is a block generator (on) or channel (off).
   */
  unit = hv->unit;
  switch (hv->id)
  { case HV_1461:
      chCount = NUM_1461_CHAN;
      break;
    case HV_1468:
      chCount = NUM_1468_CHAN;
      break;
    case HV_1469:
      if ((channel & 0x80) != 0)       /* Generator ?                        */
      { channel &= 0x7F;               /* Clear flag from channel number.    */
        chCount = NUM_1469_GEN;
      }
      else                             /* Its a channel, increment the unit  */
      { unit++;                        /* number to refer to submodule one.  */
        chCount = NUM_1469_CHAN;
      }
      break;
    default:
      return ERROR;
  }

  /* Make sure the specified channel is appropriate for the type of module
   * (and subodule).  Form the command and give it to the chassis.
   */
  if (channel >= chCount) return ERROR;
  sprintf(cmd, "LD L%3.3u.%2.2u %s %s", unit, channel, property, value);
  return Enqueue(chassis, cmd);
}


/*                                                                           *\
 | Name: GetState                                                         |
 | Parameters: chassis - pointer to a chassis instance                       |
 |             prp - pointer to a boolean property of the instance           |
 | Return: 0 property off, 1 property on, -1 failure                         |
 | Remarks:                                                                  |
 |   This function returns the state of one logical property of a channel.   |
\*                                                                           */
static int GetState(Chassis *chassis, BOOL *prp)
{
  int result = -1;

  if (epicsMutexLock(chassis->m_mutex) != epicsMutexLockError)
  // or == epicsMutexLockOK
  { result = *prp ? 1 : 0;
    epicsMutexUnlock(chassis->m_mutex);
  }

  return result;
}


/*                                                                           *\
 | Name: GetValidity                                                         |
 | Parameters: id - chassis assigned ID number                               |
 | Return: 1 chassis OK  0 chassis not OK, -1 no such chassis or error       |                                             |
 | Remarks:                                                                  |
 |   This function is passed a LeCroy chassis ID and finds it in the global  |
 |   chassis database.  If found, it returns the validity status, which is   |
 |   true if communications are operational, by calling GetProperty.         |
\*                                                                           */
int GetValidity(unsigned id)
{
  Chassis *chassis = GetChassis(id);
  return (chassis == NULL) ? -1: GetState(chassis, &chassis->m_valid);
}


/*                                                                           *\
 | Name: GetAlarm                                                            |
 | Parameters: id - chassis assigned ID number                               |
 | Return: 1 chassis OK  0 chassis not OK, -1 no such chassis or error       |                                             |
 | Remarks:                                                                  |
 |   This function is passed a LeCroy chassis ID and finds it in the global  |
 |   chassis database.  If found, it returns the alarm status, which is      |
 |   true if any channels on any HV module report an alarm.  Channel alarms  |
 |   occur when demand and measured voltage differ significantly.            |
\*                                                                           */
int GetAlarm(unsigned id)
{
  Chassis *chassis = GetChassis(id);
  return (chassis == NULL) ? -1: GetState(chassis, &chassis->m_alarm);
}


/*                                                                           *\
 | Name: GetHV                                                               |
 | Parameters: id - chassis assigned ID number                               |
 | Return: 1 chassis OK  0 chassis not OK, -1 no such chassis or error       |                                             |
 | Remarks:                                                                  |
 |   This function is passed a LeCroy chassis ID and finds it in the global  |
 |   chassis database.  If found, it returns the high voltage status, which  |
 |   is true if High Voltage is active on the chassis, by calling            |
 |   GetProperty.                                                            |
\*                                                                           */
int GetHV(unsigned id)
{
  Chassis *chassis = GetChassis(id);
  return (chassis == NULL) ? -1: GetState(chassis, &chassis->m_HVstatus);
}


/*                                                                           *\
 | Name: GetChannel                                                          |
 | Parameters: id - chassis assigned ID number                               |
 |             slot - slot number within chassis                             |
 |             channel - channel number on LeCroy module                     |
 |             prp - array of properties to fill for this channel            |
 | Return: OK or ERROR                                                       |
 | Remarks:                                                                  |
 |   This function is called to get the state of a channel's properties.     |
 |   the values are returned via the array 'prp'.  The number of elements    |
 |   filled depends on the type of module (and submodule).                   |
\*                                                                           */
STATUS GetChannel(unsigned id, unsigned slot, unsigned channel,
                  double *prp, double *delta)
{
  unsigned i, prpCnt;
  float *chData;
  Chassis *chassis;
  double *MV = prp + 1, *DV = prp + 2, *CE = prp + 6, *ST = prp + 7;
  HVgeneric *hv = GetSlot(id, slot, &chassis);
  if (hv == NULL) {
    printf("Slot %d not found\n",slot);
    return ERROR;            /* Is there a module there ?      */
  }

  /* The properties are stored as an array, so a pointer to the start of the
   * property values and the number to copy must be set, which is dependent
   * on the type of module.  If the module is a 1469, then the request may
   * be for a block generator or channel.  A block generator request is
   * designated by bit 7 of the channel number being set.
   */
  switch (hv->id)
  { case HV_1461:
      if (channel >= NUM_1461_CHAN) return ERROR;
      prpCnt = NUM_1461_PROP;
      chData = ((HV1461 *) hv)->m_chan[channel].properties;
      break;

    case HV_1471:
      if (channel >= NUM_1471_CHAN) return ERROR;
      prpCnt = NUM_1471_PROP;
      chData = ((HV1471 *) hv)->m_chan[channel].properties;
      break;

    case HV_1468:
      if (channel >= NUM_1468_CHAN) return ERROR;
      prpCnt = NUM_1468_PROP;
      chData = ((HV1468 *) hv)->m_chan[channel].properties;
      break;

    case HV_1469:
      if ((channel & 0x80) != 0)
      { channel &= 0x7F;
        if (channel >= NUM_1469_GEN) return ERROR;
        prpCnt = NUM_1469_PROP;
        chData = ((HV1469 *) hv)->m_gen[channel].properties;
      }
      else
      { if (channel >= NUM_1469_CHAN) return ERROR;
        prpCnt = NUM_1469_SUBPROP;
        chData = ((HV1469 *) hv)->m_chan[channel].properties;
      }
      break;

    default:
      return ERROR;
  }

  /* Since the property database is assigned and read by different tasks,
   * mutual exclusion must be performed for the access.
   */
  if (epicsMutexLock(chassis->m_mutex) != epicsMutexLockError)
  // or == epicsMutexLockOK
  { 
    for (i = 0; i < prpCnt; i++) *(prp++) = *(chData++);
    epicsMutexUnlock(chassis->m_mutex);
  }
  else return ERROR;

  /* Compute the difference in measured and demand voltage (absolute value)
   * for record alarm purposes.  Always set to zero if no voltage was demanded
   * (=0), or the channel is disabled manually (not because a trip).  The
   * field is left undefined for LeCroy 1469 sub-modules, which
   * do not maintain a demand voltage, since they are driven by their block
   * generator.  Note that the DV, MV, CE, and ST pointers are set to the same
   * offsets into the "prp" array for all HV module types.  This holds true
   * for the current set of 1461, 1468, and 1469, but is not guarenteed to
   * always be true.
   */
  if (prpCnt != NUM_1469_SUBPROP)
  { if (*DV == 0. || (*CE == 0. && *ST < 16)) *delta = 0.;
    else *delta = fabs(*DV - *MV);
  }
  return OK;
}


/*                                                                           *\
 | Name: GetProperty                                                         |
 | Parameters: id - chassis assigned ID number                               |
 |             slot - slot number within chassis                             |
 |             channel - channel number on LeCroy module                     |
 |             property - Character string name of property to fetch         |
 |             value - returned value of desired property                    |
 | Return: OK or ERROR                                                       |
 | Remarks:                                                                  |
 |   This function is called to get the state of a property for a channel    |
 |   or generator.  The path to the property value depends on the passed     |
 |   parameters and the type of HV module.  This function is called by       |
 |   device support "init" routine, which want to initialize output records  |
 |   to the state of a LeCroy module.                                        |
\*                                                                           */
STATUS GetProperty(unsigned id, unsigned slot, unsigned channel,
                   char *property, float *value)
{
  unsigned number, i;
  float *properties;
  const char **names;
  Chassis *chassis;
  HVgeneric *hv = GetSlot(id, slot, &chassis);
  if (hv == NULL) return ERROR;            /* Is there a module there ?      */

  /* Now that a module has been found, use the type of module to assign
   * a pointer to the list of its property values.  One of these values
   * is the desired return number, but the list of property names for a
   * module type must be searched for a match to the passed name.  If the
   * module is a 1469, then the request may be for a block generator or
   * channel.  A block generator request is designated by bit 7 of the
   * channel number being set.
   */
  switch (hv->id)
  { case HV_1461:
      if (channel >= NUM_1461_CHAN) return ERROR;
      properties = ((HV1461 *) hv)->m_chan[channel].properties;
      names = prp1461;
      number = NUM_1461_PROP;
      break;

    case HV_1471:
      if (channel >= NUM_1471_CHAN) return ERROR;
      properties = ((HV1471 *) hv)->m_chan[channel].properties;
      names = prp1471;
      number = NUM_1471_PROP;
      break;

    case HV_1468:
      if (channel >= NUM_1468_CHAN) return ERROR;
      properties = ((HV1468 *) hv)->m_chan[channel].properties;
      names = prp1468;
      number = NUM_1468_PROP;
      break;

    case HV_1469:
      if ((channel & 0x80) != 0)
      { channel &= 0x7F;
        if (channel >= NUM_1469_GEN) return ERROR;
        properties = ((HV1469 *) hv)->m_gen[channel].properties;
        names = prp1469;
        number = NUM_1469_PROP;
      }
      else
      { if (channel >= NUM_1469_CHAN) return ERROR;
        properties = ((HV1469 *) hv)->m_chan[channel].properties;
        names = prp1469s;
        number = NUM_1469_SUBPROP;
      }
      break;

    default:
      return ERROR;
  }

  /* Use the arrays of property names and values to identify which property
   * is desired and what its current value is.  Initialize the record if found.
   */
  for (i = 0; i < number; i++)
    if (strcmp(property, names[i]) == 0)
    {
      /* Since the property database is assigned and read by different tasks,
       * mutual exclusion must be performed for the access.
       */
      if (epicsMutexLock(chassis->m_mutex) != epicsMutexLockError)
	// or == epicsMutexLockOK
      { *value = properties[i];
	epicsMutexUnlock(chassis->m_mutex);
        return OK;
      }
      else return ERROR;
    }

  return ERROR;
}
