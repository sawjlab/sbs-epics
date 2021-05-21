/*****************************************************************************\
 * File: hvstart.c                                  Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains functions used in starting up, managing, and         *
 *  stopping control sessions with a LeCroy 1458 High Voltage Mainframe.     *
 *  See the header file "lecroy.h" for more information on the LeCroy HV.    *
 *                                                                           *
 * Revision History:                                                         *
 *   12/22/2000 - Initial release.                                           *
 *   04/19/2001 - Added version printout for release 1-1.                    *
 *   06/18/2001 - Added conditional compilation for PowerPC version, to keep *
 *                VME access limited to 16 bits.                             *
\*****************************************************************************/

#include "lecroy.h"
#include "define.h"
#include "extern.h"
#include "errno.h"
#include "dbDefs.h"
//#include "sysLib.h"
#include "signal.h"

static epicsTimerQueueId lecroyTimerQueue;

static int HVtask(Chassis *);
static void RestoreMemory(Chassis *);

/*
 | Name: HVAddChassis
 | Paramters: Chassis#, IPaddress
 | Remarks:
 | Create a new Chassis database entry.
*/
int HVAddCrate(int hvID, char* ipaddr)
{
  Chassis *chassis;

  if (hvID == 0 || hvID > 255)
    { printf("Invalid node ID\n");
      //      return -1;
    }

  if (g_chassisCnt == MAX_CHASSIS)         /* Any more entries available ? */
    { printf("No more allowed\n");
      //      return -1;
    }

  /* Allocate a Chassis data structure and save a pointer to it in the
   * global database array.  Fill in the IP address and HV identifier for
   * use in the spawned task.  All fields are initialized to zero.
   */
  chassis = (Chassis *) calloc(1, sizeof(Chassis));
  chassis->m_hvID = hvID;
  chassis->m_ipaddr = (char *) malloc(sizeof(ipaddr)+1);
  strcpy(chassis->m_ipaddr,ipaddr);
  chassis->m_sock = 0;

  g_chassis[g_chassisCnt++] = chassis;

  printf("Added %s as Lecroy chassis %d\n",chassis->m_ipaddr,chassis->m_hvID);
  return(0);
}

  /*                                                                           * \
 | Name: HVstart                                                             |
 | Parameters: mfList - Space separated list of LeCroy chassis node numbers  |
 | Return: 0 success;  < 0 Failure                                           |
 | Remarks:                                                                  |
 |   This function is the VxWorks console interface for starting a task      |
 |   to manage the HV units that have been defined with HVAddChassis calls.  |
 |   It spawns a HVtask for each chassis.  The chassis entry includes a      |
 |   watchdog timer instance that will repeatedly wake the task via a        |
 |   semaphore.                                                              |
\*                                                                           */
int HVstart()
{
  char taskName[] = "tLecroy00";
  Chassis *chassis;
  unsigned i;

  printf("<< LeCroy Driver Version %s >>\n", VERSION);

  lecroyTimerQueue  =  epicsTimerQueueAllocate(1,epicsThreadPriorityScanHigh);
  //  printf("Timer queue ID = %p\n",lecroyTimerQueue);
  
  signal(SIGPIPE, SIG_IGN);

  for (i = 0; i < g_chassisCnt; i++) {
    chassis = g_chassis[i];
    
    /* Create system objects, watchdog timer, message queue, and semaphores
     * for this instance.
     */

    if ((chassis->m_queue =
	 epicsMessageQueueCreate(QMAX,MSGMAX)) == NULL)
    { printf("Message Q error\n");
      return -1;
    }

    chassis->m_timerid = epicsTimerQueueCreateTimer(lecroyTimerQueue,(void *) tickMgr, (void *) chassis);
    //    printf("Timer ID for crate %d = %pa\n",hvID,chassis->m_timerid);

    if ((chassis->m_wakeup = epicsEventCreate(epicsEventEmpty)) == NULL)
    { printf("Binary Semaphore");
      return -1;
    }
    if ((chassis->m_mutex = epicsMutexCreate()) == NULL)
    { printf("Mutex Semaphore");
      return -1;
    }

    /* Create the connection to the crate and save the socket
     * in the chassis structure
     */
    if((chassis->m_sock = ARCsetup(chassis->m_ipaddr)) == -1) {
      return(-1);
    }
    sprintf(&taskName[7], "%2.2d", chassis->m_hvID);
    chassis->m_tid = epicsThreadCreate(taskName, THREAD_PRIORITY,
				       epicsThreadGetStackSize(epicsThreadStackMedium),
				       (EPICSTHREADFUNC) HVtask,
				       (void *) chassis);
    /* Force a delay to allow the spawned task enough time to get
     * initialized.  This will eliminate the problem of the EPICS device support
     * requesting data before the task has ascertained the configuration of the
     * chassis.
     */
    printf("Starting driver for LeCroy #%u\n", chassis->m_hvID);
    epicsThreadSleep(1.0);
    printf("  Configuration detected for chassis #%u\n\n", chassis->m_hvID);
  }
  return 0;
}


/*                                                                           *\
 | Name: HVtask                                                              |
 | Parameters: chassis - Pointer to instance of a Lecroy chassis             |
 | Return: Task completion status                                            |
 | Remarks:                                                                  |
 |   This function is the entry point to all tasks that will manage Lecroy   |
 |   chassis.  The task will connect across the ethernet and get information |
 |   about the boards in the chassis.  It then sets the watchdog timer and   |
 |   performs a repeat cycle of network communication with the HV unit.      |
\*                                                                           */
static int HVtask(Chassis *chassis)
{
  /* Initialize the task error to "OK", detect the hardware configuration,
   * and create data structures to reflect the layout.  Flag the learning
   * process as complete.
   */
  errno = 0;
  if (GetConfig(chassis) != 0) StopAll(chassis);
  chassis->m_learned = TRUE;

  /* Get the clock rate, ticks per second, from the system and use the
   * defined cycle time, milliseconds, to compute the number of clock
   * ticks per cycle to use for the watchdog timer.  Start the timer.
   */
  epicsTimerStartDelay(chassis->m_timerid, CYCLE_TIME/1000.0);

  /* This is the task "infinite" loop, which is performed each time the
   * task's watchdog timer wakes it via semaphore.  In each loop the state
   * of the HV chassis is checked to update local state variables and any
   * queued commands to the chassis are issued.
   */
  while (TRUE)
  { chassis->m_busy = FALSE;
    epicsEventMustWait(chassis->m_wakeup);
    chassis->m_busy = TRUE;

    /* Flush the command queue and get any changes to the chassis state.
     */
    if (FlushQ(chassis) < 0) chassis->m_valid = FALSE;
    else chassis->m_valid = (CheckStatus(chassis) == 0);

    /* Fetch voltage alarm status and increment frame counter.  Alarm status
     * will default to "Alarming" if the chassis status is bad.
     */
    chassis->m_alarm = (chassis->m_valid ? ChassisAlarm(chassis) : TRUE);
    chassis->m_frameCnt++;
  }

  StopAll(chassis);                     /* Shutdown if it exits frame loop. */
  return 0;
}


/*                                                                           *\
 | Name: HVstop                                                              |
 | Return: Task exit status                                                  |
 | Remarks:                                                                  |
 |   This task is spawned, from console command or program error detection,  |
 |   to close down all LeCroy HV tasks.  It performs an orderly shutdown     |
 |   and cleans up task resources.  Note that it synchronizes to a task's    |
 |   regular update completion so it does not interrupt any ethernet I/O.    |
 |   It will wait 1/3 longer (1000 / 750) than the task normal update rate   |
 |   before timing out.
\*                                                                           */
int HVstop(void)
{
  unsigned i, sample, ticks;
#ifdef vxWorks
  unsigned tickWaitLimit = (sysClkRateGet() * CYCLE_TIME) / 750;
#else
  unsigned tickWaitLimit = (50 * CYCLE_TIME) / 750;
#endif
  BOOL normal;

  for (i = 0; i < g_chassisCnt; i++)
  {
    /* Perfrom shutdown for each chassis defined in the global list.  Sample
     * the update counter and wait until you see it change to synchronize
     * with the task's frame loop.
     */
    Chassis *chassis = g_chassis[i];
    printf("Terminating LeCroy #%d ... ", chassis->m_hvID);
    sample = chassis->m_frameCnt;
    normal = TRUE;
    ticks = 0;

    while (sample == chassis->m_frameCnt)    /* Task completed its update?   */
    {
      epicsThreadSleep(0.02);

      if (++ticks > tickWaitLimit)           /* Waited plenty long enough?   */
      { printf("TIMEOUT\n");
        normal = FALSE;
        break;
      }
    }

    /* If properly synchronized, bring it all down and clean up.
     */
    if (normal)
    {
      epicsTimerCancel(chassis->m_timerid);
      epicsTimerQueueDestroyTimer(lecroyTimerQueue, chassis->m_timerid);
      epicsMessageQueueDestroy(chassis->m_queue);
// Need a way to tell thread to stop
      epicsEventDestroy(chassis->m_wakeup);
      epicsMutexDestroy(chassis->m_mutex);
      RestoreMemory(chassis);
      printf("OK\n");
    }
  }
  epicsTimerQueueRelease(lecroyTimerQueue);

  ARCend();                                      /* Cleanup ARCNet interace */
  g_chassisCnt = 0;                              /* No chassis tasks exist  */
  return 0;
}


/*                                                                           *\
 | Name: RestoreMemory                                                       |
 | Parameters: Chassis instance pointer                                      |
 | Remarks:                                                                  |
 |   This task restores dynamically allocated memory for a Chassis task.     |
 |   Included are the main chassis database and individual HV cards.         |
\*                                                                           */
static void RestoreMemory(Chassis *chassis)
{
  unsigned i;

  for (i = 0; i < NUM_SLOTS; i++)
  { if (chassis->m_slot[i] != NULL) free(chassis->m_slot[i]);
  }

  free(chassis->m_logUnit);
  free(chassis);
}


/*                                                                           *\
 | Name: tickMgr                                                             |
 | Parameters: Address of Chassis instance                                   |
 | Return: OK for WxWorks                                                    |
 | Remarks:                                                                  |
 |   This is the Chassis task watchdog timer service routine.  It is used    |
 |   by all Chassis tasks.  The individual Chassis instance is passed in.    |
 |   Its purpose is to wake the task in a repeated cycle.                    |
\*                                                                           */
int tickMgr(void * param)
{
  Chassis *chassis = (Chassis *) param;

  /* A chassis's data is invalidated if its frame counter has not incremented
   * in WD_TIMEOUT cycles.
   */
  if (chassis->m_valid)
  { if (chassis->m_lastCnt == chassis->m_frameCnt)
    { if (++chassis->m_sameCnt == WD_TIMEOUT) chassis->m_valid = FALSE; }
    else chassis->m_sameCnt = 0;
  }
  else chassis->m_sameCnt = 0;

  chassis->m_lastCnt = chassis->m_frameCnt;

  /* Wake the task if the last frame completed, otherwise note an overflow
   * of the frame time.
   */
  if (chassis->m_busy) chassis->m_overflow++;
  else epicsEventSignal(chassis->m_wakeup);

  /* Restart the timer for another cycle.
   */
  epicsTimerStartDelay(chassis->m_timerid, CYCLE_TIME/1000.0);
  return OK;
}


/*                                                                           *\
 | Name: StopAll                                                             |
 | Parameters: Address of Chassis instance                                   |
 | Return: Should never return.                                              |
 | Remarks:                                                                  |
 |   This function is called to spawn a task which will shutdown the HV      |
 |   tasks.  It is called by HV tasks, so it simply waits for itself to      |
 |   be deleted.                                                             |
\*                                                                           */
void StopAll(Chassis *chassis)
{
  /* The loop above is exited only under erroneous circumstances.  Run the
   * termination task to bring down all LeCroy management tasks (including
   * self).
   */
  epicsThreadCreate("terminator", THREAD_PRIORITY+1,
		    epicsThreadGetStackSize(epicsThreadStackMedium),
		    (EPICSTHREADFUNC) HVstop, 0);
  while (TRUE) { epicsThreadSleep(0.01); chassis->m_frameCnt++; }
}
