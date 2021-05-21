/*****************************************************************************\
 * File: arcnet.c                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains functions and data specific to the ARCNet driver,    *
 *   used by the LeCroy application device driver for EPICS control at       *
 *   Thomas Jefferson National Accelerator Facility.                         *
 *                                                                           *
 * References:                                                               *
 *   "Technical Manual CC121 ARCNET Interface for VMEbus" Version 2.0        *
 *   COMPCONTROL; http://www.compcontrol.com                                 *
 *   Standard Microsystems Corp. (SMSC); http://www.smsc.com                 *
 *   LeCroy - "1454 / 1458 HV Mainframe User's Guide V3.04"                  *
 *   LeCroy research systems; http://www.lecroy.com                          *
 *   Universal Voltronics; http://www.universalvoltronics.com                *
 *                                                                           *
 * Revision History:                                                         *
 *   02/16/2001 - Initial release.                                           *
 *   05/03/2001 - Corrected unexpected transmit interrupt error for 1-1.     *
 *   06/18/2001 - added conditional compilation for PowerPC, forcing         *
 *                Universe write buffer flush.                               *
\*****************************************************************************/

#include "arcnet.h"
#include "extern.h"
#include "stdio.h"
#include "string.h"
#include "vme.h"
#include "sysLib.h"
#include "vxLib.h"
#include "taskLib.h"
#include "intLib.h"
#include "iv.h"
#include "wdLib.h"
#include "semLib.h"
#include "logLib.h"

/* Allocate data structures for use by functions within this file.
 */
static CC121 f_cc121;                  /* board template; see arcnet.h       */
static SEM_ID f_wakeup;                /* VxWorks synchronize semaphore      */
static SEM_ID f_mutex;                 /* VxWorks mutual exclusion semaphore */
static MSG_Q_ID f_recvQ[256] = {0};    /* ARCNet node <--> Message Q table   */

#ifdef mv2700
static unsigned char f_read;   /* force a write flush with a read cycle */
#endif

/* Prototypes for local functions.
 */
static int MapVME(void);
static int SetInterrupt(void);
static void Handler(int);


/* Name: ARCsend                                                            *\
 | Parameters: ID - the ID number of the destination LeCroy chassis         |
 |             text - The message to send                                   |
 |             reply - The response after the message is sent; If NULL,     |
 |                     just print the reply to the screen.                  |
 |             size - The size of the response buffer                       |
 | Return:                                                                  |
 |   The number of lines in the response (separated by \n) if successful.   |
 |   ErrType if not successful.                                             |
 |                                                                          |
 | Remarks:                                                                 |
 |   This function performs an I/O transaction with a LeCroy chassis via    |
 |   the ARCNet controller.  A transaction consists of sending one message  |
 |   and receiving one or more responses from a chassis.  Multiple replies  |
 |   from a chassis are concatenated, separated by \n, into the response    |
 |   buffer.  Since this routine accesses the COM902C26 network controller  |
 |   for reads and writes, mutual exclusion is employed to allow only one   |
 |   task to have access to the network controller.  Note that this module  |
 |   will force the calling task to sleep while waiting for sends and       |
 |   receives to complete.                                                  |
\*                                                                          */
int ARCsend(unsigned ID, const char *text, char *reply, unsigned size)
{
  int code, Qcnt;
  char buffer[MsgMax];
  BOOL getMore;
  unsigned total = 0, lines = 0;
  unsigned length = strlen(text);
  unsigned count = 256 - length;                  /* output buffer offset */

  if (length > 253) return ErrReport("Message length", R_Warn);

  /* Flush this task's message queue of any unsolicited replies.  This can
   * happen when the sender and receiver are out of sync due to timeout
   * or other failures.
   */
  Qcnt = msgQNumMsgs(f_recvQ[ID]);
  if (Qcnt == ERROR) return ErrReport("Q flush failure", R_Fatal);
  while (Qcnt-- > 0)
  { printf("flushing message for node %u\n", ID);
    msgQReceive(f_recvQ[ID], buffer, 1, NO_WAIT);
  }

  /* Get exclusive access to the COM90C26 so the message can be transmitted
   * and the reply received without interference.
   */
  if (semTake(f_mutex, WAIT_FOREVER) != OK)
    return ErrReport("Semaphore fail", R_Fatal);

  /* Load page #0 of onboard memory with the message header and text.  The
   * text is stored at the end of the 256 byte buffer.
   */
  f_cc121.page0[0] = f_cc121.nodeID;
  f_cc121.page0[1] = (unsigned char) ID;
  f_cc121.page0[2] = count;
  memcpy((void *) &f_cc121.page0[count], text, length);

  /* Issue the send command and arm the completion interrupt in both the
   * shadow register and onboard register.  The shadow is used by the
   * interrupt handler since the intMask is write only.
   */
  *f_cc121.command = cmd_SEND_PAGE | pg_0;
  f_cc121.shadowMask |= b_TA;
  *f_cc121.intMask = f_cc121.shadowMask;

  /* Sleep-wait for the send completion.  This may timeout if nobody
   * is listening.  Interrupts are locked so the TA can't finally show up
   * in the middle of the disable instructions.
   */
  if (semTake(f_wakeup, f_cc121.timeout) == ERROR)
  { int key = intLock();                           /* Lock out interrupts */
    f_cc121.shadowMask &= ~b_TA;
    *f_cc121.intMask = f_cc121.shadowMask;         /* Cancel interrupt    */
    *f_cc121.command = cmd_DISABLE_SEND;           /* Cancel send         */
    intUnlock(key);                                /* Restore interrupts  */
    f_cc121.sendTO++;
    semGive(f_mutex);
    return ErrReport("Write timeout", R_Warn);
  }

  /* A wakeup was issued.  Make sure the destination node issued an "ACK"
   * to verify the transmission was successful.  Then return exclusive
   * access.
   */
  if ((*f_cc121.status & b_TMA) == 0)
  { semGive(f_mutex);
    return ErrReport("No ACK", R_Warn);
  }

  /* Restore access to the transmitter.
   */
  semGive(f_mutex);


  /* Get the response to the just issued command.  A loop is used because
   * a message may illicit multiple replies from the destination chassis.
   * the task will sleep until the interrupt handler has inserted the
   * response into the task's message queue.
   */
  do
  { length = msgQReceive(f_recvQ[ID], buffer, MsgMax, f_cc121.timeout);
    if (length == ERROR)
    { f_cc121.recvTO++;
      return ErrReport("Read Timeout", R_Warn);
    }

    getMore = (buffer[0] == 'C');       /* More replies to come ?          */
    code = atoi(&buffer[1]);            /* Decode the message status field */

    /* If no reply buffer was supplied, simply print the reply, with an
     * interpretation of its status code, to the console.  Otherwise add
     * the message to the reply buffer.
     */
    if (reply == NULL)
    { char *bufPtr;
      switch (code)
      { case 1: bufPtr = "OK"; break;
        case 2: bufPtr = "VIEW"; break;
        case 3: bufPtr = "LOCAL"; break;
        case 4: bufPtr = "PANIC"; break;
        default: bufPtr = (code < 20) ? "UNDEF" : "ERR";
      }
      buffer[length] = '\0';
      printf("LeCroy{%s}> %s\n", bufPtr, &buffer[7]);
    }
    else
    { char *ptr = buffer + 7;
      if (code != 1 || length < 8)
        return ErrReport("Bad reply", R_Warn);
      length -= 7;
 
      /* Append it to the reply buffer.  If more lines are coming, 
       * separate by a \n.
       */
      if (total + length + 1 > size)
        return ErrReport("Buffer Overflow", R_Warn);
      memcpy(&reply[total], ptr, length);
      lines++;                                 /* Number of replies         */
      total += length;                         /* Total size of all replies */
      reply[total++] = getMore ? '\n' : '\0';
    }
  } while (getMore);

  /* return exclusive access to the receiver.
   */
#if DEBUG != 0
  printf("%u> %s\n", ID, reply);
#endif
  return lines;
}


/* Name: ARCnode                                                            *\
 | Parameters: node - ARCNet remote node number                             |
 | Return: ErrType                                                          |
 | Remarks:                                                                 |
 |   This function adds a node to the table of message queues.              |
 |   The table associates remote nodes, that may transmit to the LeCroy     |
 |   unit, with a message queue used to wake the task that is waiting       |
 |   to hear from that node.                                                |
\*                                                                          */
int ARCnode(unsigned char node)
{
  if ((f_recvQ[node] = msgQCreate(Qmax, MsgMax, MSG_Q_FIFO)) == NULL)
    return ErrReport("Q creation", R_Fatal);

  return 0;
}


/* Name: ARCsetup                                                           *\
 | Return: OK or ERROR                                                      |
 | Remarks:                                                                 |
 |   This function is called to initialize data structures prior to         |
 |   performing network I/O on the ARCNet.  It should only be called once   |
 |   during the lifetime of an application.                                 |
\*                                                                          */
int ARCsetup(void)
{
  /* Map the VME address of the CC121 ARCNet board, which initializes the
   * f_cc121 register/memory pointers.  Then initialize the non VME based
   * f_cc121 variables.
   */
  if (MapVME() != 0) return ErrReport("VME Mapping failure", R_Fatal);
  f_cc121.intTA = f_cc121.intRECON = f_cc121.intPOR = f_cc121.intRI = 0;
  f_cc121.shadowMask = 0;
  f_cc121.timeout = (sysClkRateGet() * IO_TIMEOUT) / 1000;

  /* Clear status flags and configure the CC121 to use the short message
   * buffer.  Also set up interrupt generation and handling.
   */
  *f_cc121.command = (cmd_CLEAR_FLAGS | CLR_POR | CLR_RECON);
  *f_cc121.command = cmd_CONFIGURE | cfg_256;
  if (SetInterrupt() != 0) return ErrReport("Interrupt setup", R_Fatal);

  /* Create VxWorks data structures for semaphores.  The binary semaphore
   * is for sleep/wake of driver threads, and the mutex semaphore is for 
   * perfoming mutual exclusion of access to the COM90C26 network controller
   * registers.
   */

  if ((f_wakeup = semBCreate(SEM_Q_FIFO, SEM_EMPTY)) == NULL)
    return ErrReport("Binary Semaphore", R_Fatal);

  if ((f_mutex = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE |
                                     SEM_INVERSION_SAFE)) == NULL)
    return ErrReport("Mutex Semaphore", R_Fatal);

  /* Issue the receive command and arm the completion interrupt in both the
   * shadow register and onboard register.  The shadow is used by the
   * interrupt handler since the intMask is write only.  The SID field is
   * cleared in the receive buffer to verify a message was received and
   * not a disable receive command.
   */
  f_cc121.page1[0] = 0;
  *f_cc121.command = cmd_RECV_PAGE | pg_1;
  f_cc121.shadowMask |= b_RI;
  *f_cc121.intMask = f_cc121.shadowMask;

  return 0;
}


/* Name: ARCend                                                             *\
 | Remarks:                                                                 |
 |   This function is called to end an ARCNet I/O session.  It resets the   |
 |   COM90C26 chip, disables interrupts, and frees the allocation of        |
 |   VxWorks data structures.                                               |
\*                                                                          */
void ARCend(void)
{
  int i;

  *f_cc121.BCR = b_RES;
  sysIntDisable(IRQ);
  *f_cc121.BCR = 0;
  semDelete(f_wakeup);
  semDelete(f_mutex);
  for (i = 0; i < 256; i++)
    if (f_recvQ[i] != NULL) msgQDelete(f_recvQ[i]);
}


/* Name: MapVME                                                             *\
 | Return: OK or ERROR                                                      |
 | Remarks:                                                                 |
 |   This function is called to map the CC121's VME addess to local         |
 |   address space and to setup the f_cc121 data structure's register       |
 |   and memory pointers.  It resets the board's COM90C26 network           |
 |   controller and does not return until the reset is complete.            |
\*                                                                          */
static int MapVME(void)
{
  char testChar, *base;
  unsigned wait = 0, timeout = sysClkRateGet() * 2;

  /* Map the A24 bus address into local space.
   */ 
  if (sysBusToLocalAdrs(VME_AM_STD_SUP_DATA, (char *) VME_BASE, &base) != OK)
    return ErrReport("Mapping error", R_Fatal);

  /* Probe the address by reading one character.  vxMemPrope traps any
     segmentation fault generated by a bus timeout.
   */
  if (vxMemProbe(base, VX_READ, 1, &testChar) != OK)
    return ErrReport("No hardware response", R_Fatal);

  /* Initialize the global data structure's address pointers for access of
   * onboard registers and memory.
   */
  f_cc121.page0 =  base;
  f_cc121.page1 =  base + 0x200;
  f_cc121.page2 =  base + 0x400;
  f_cc121.page3 =  base + 0x600;
  f_cc121.status = base + 0x801;
  f_cc121.intMask = base + 0x801;
  f_cc121.command = base + 0x803;
  f_cc121.intVec = base + 0xA01;
  f_cc121.BCR = base + 0xC01;

  /* Clear the two memory bytes used to store information on reset completion.
   * Reset the COM90C26 network controller chip and wait for it to write the
   * two bytes, denoting reset is complete.  A timeout is used to keep this
   * from becoming infinite.  The first byte written is 0xD1 and the second
   * is the CC121's node ID from the register at 0xE01.
   */
  *f_cc121.BCR = b_RES;              /* Start reset sequence               */
  f_cc121.page0[0] = 0;              /* Clear reset feedback memory (and   */
  f_cc121.page0[1] = 0;              /*  space reset sequence)             */
  *f_cc121.BCR = 0;                  /* Finish reset sequence              */

  while (f_cc121.page0[0] != 0xD1 || f_cc121.page0[1] != *(base + 0xE01))
  { if (wait++ == timeout) return ErrReport("ARCNet reset error", R_Fatal);
    taskDelay(1);
  }

  /* Save the board's node ID and return
   */
  f_cc121.nodeID = *(base + 0xE01);
  return OK;
}


/* Name: SetInterrupt                                                       *\
 | Return: OK or ERROR                                                      |
 | Remarks:                                                                 |
 |   This function interrupts by establishing a handler for VxWorks and     |
 |   enabling them on the COM90C26 controller.                              |
\*                                                                          */
static int SetInterrupt(void)
{
  /*  Attach 'Handler' to the VxWorks interrupt handler for 'intnum'.
   *  Enable the CPU's reception of VME 'level' interrupts.
   */
  if (intConnect(INUM_TO_IVEC(INT_NUM), Handler, 0) == ERROR)
    return ErrReport("intConnect", R_Fatal);
  if (sysIntEnable(IRQ) == ERROR)
     return ErrReport("IntEnable", R_Fatal);

  /* Enable interrupts on the ARCNet board.
   */
  *f_cc121.intVec = INT_NUM;
  *f_cc121.BCR = b_EBI;

  return OK;
}


/* Name: Handler                                                            *\
 | Remarks:                                                                 |
 |   This is the interrupt handler for the CC121 VME board.  Four types     |
 |   of interrupts may be generated by the CC121's COM90C26 network         |
 |   controller.  The power reset (POR) is non-maskable.  The network       |
 |   reconfiguration (RECON), transmitter available (TA), and receiver      |
 |   inhibited (RI) are all subject to the bits in the Interrupt Mask       |
 |   register.  The function determines which of the maskable interrupts    |
 |   occured by ANDing the COM90C26 status register with the interrupt      |
 |   mask, and checking the three possible types.  Note the interrupt mask  |
 |   register is write only, so a shadow mask register maintained in local  |
 |   processor memory is used for reading.                                  |
\*                                                                          */
static void Handler(int notUsed)
{
  unsigned char status = *f_cc121.status;
  unsigned char ID;

  /* Detect the interrupt type and acknowledge appropriately.  The TA and
   * RI interrupts convey completion of I/O.  Their handling consists of
   * clearing the associated interrupt mask bit (to cancel interrupt),
   *  and wakeing the task that is waiting for I/O completion.
   */
  if ((status & b_POR) != 0)
  { f_cc121.intPOR++;
    *f_cc121.command = (cmd_CLEAR_FLAGS | CLR_POR);
  }
  else
  { unsigned char select = status & f_cc121.shadowMask;

    /* Simply count the reconfigure interrupts.
     */
    if ((select & b_RECON) != 0) 
    { f_cc121.intRECON++;
      *f_cc121.command = (cmd_CLEAR_FLAGS | CLR_RECON);
    }

    /* When a meassge was sent, increment the send counter, disable the
     * send interrupt, and wake up the task waiting for send confirmation.
     */
    else if ((select & b_TA) != 0)
    { f_cc121.intTA++;
      f_cc121.shadowMask &= ~b_TA;
      *f_cc121.intMask = f_cc121.shadowMask;
      semGive(f_wakeup);
    }

    /* A message was received from a remote node.  Always count the interrupt
     * and re-issue the receive command.  Process the message if it has a
     * valid sender.
     */
    else if ((select & b_RI) != 0) 
    { f_cc121.intRI++;

      /* Verify that a message was received and determine if it was from
       * a node that has been enabled in the LeCroy configuration.  If so,
       * insert the message into the appropriate queue.
       */
      if ((ID = f_cc121.page1[0]) != 0)
      { if (f_recvQ[ID] != NULL)
        { unsigned offset = f_cc121.page1[2];
          unsigned length = 256 - offset;
          msgQSend(f_recvQ[ID], (char *) &f_cc121.page1[offset], length,
            NO_WAIT, MSG_PRI_NORMAL);
        }
        else f_cc121.recvNoQ++;
      }
      else f_cc121.recvWho++;

      /* Re-issue the receive command.  The SID field is cleared so a
       * "disable receive" command won't repeat the last reception.
       */
      f_cc121.page1[0] = 0;
      *f_cc121.command = cmd_RECV_PAGE | pg_1;
    }

    /* An unexpected interrupt occured.  Just count it and ignore it.  It
     * could have been a RECON or TA, but not a RI (RIs are always armed).
     */
    else
    { *f_cc121.command = (cmd_CLEAR_FLAGS | CLR_RECON);
      *f_cc121.intMask = f_cc121.shadowMask;
      f_cc121.intHuh++;
    }
  }

#ifdef mv2700
  /* When this application is run on a PowerPC, the Universe chip allows VME
   * bus writes to be queued while the processor proceeds.  When this interrupt
   * handler returns without the interrupt being cleared, problems arise.
   * This read cycle forces the nearby "command" or "intMask" writes to
   * flush out of the Universe chip.
   */
  f_read = *f_cc121.status;
#endif
}


/* Name: keyboard                                                           *\
 | Parameters: Node ID of LeCroy chassis to communicate with                |
 | Return: OK or ERROR                                                      |
 | Remarks:                                                                 |
 |   This is the entry point for interactive communication with a LeCroy    |
 |   mainframe via ARCNet.  It takes keyboard command lines and sends then  |
 |   to the Lecroy unit.  The LeCroy response is printed to the screen.     |
 |   The session is terminated by entering an empty line.                   |
\*                                                                          */
int keyboard(unsigned ID)
{
  char buffer[128];

  errno = 0;                                 /* Initialize to OK          */
  if (ARCnode(ID) != 0) return -1;
  if (ARCsetup() != 0) return -1;

  while (TRUE)
  { printf("\nEnter HV command: ");          /* Get the command from user */
    if (gets(buffer) == NULL) break;
    if (buffer[0] == '\0') break;
    ARCsend(ID, buffer, NULL, 0);
  }

  ARCend();
  return 0;
}


/* Name: ComStats                                                           *\
 | Remarks:                                                                 |
 |   This functions prints CC121 network communications statistics to       |
 |   the console.                                                           |
\*                                                                          */
void ComStats(void)
{
   printf("\nCC121 ARCNet Controller Statistics\n"
     "  Reset Interrupts: %u\n"
     "  Reconfiguration Interrupts: %u\n"
     "  Transmit Available Interrupts: %u\n"
     "  Receiver Inhibit Interrupts: %u\n"
     "  Unexpected Interrupts: %u\n"
     "  Message Received, no destination: %u\n"
     "  Message Received, No messageQ: %u\n"
     "  Transmit Timeouts: %u\n"
     "  Receive Timeouts: %u\n\n", f_cc121.intPOR, f_cc121.intRECON,
     f_cc121.intTA, f_cc121.intRI, f_cc121.intHuh, f_cc121.recvWho,
     f_cc121.recvNoQ, f_cc121.sendTO, f_cc121.recvTO);
}


/* Name: Mark                                                               *\
 | Parameters: byte - code to write to VME memory                           |
 | Remarks:                                                                 |
 |   This functions is used simply for timing tests with the VMETRO VME     |
 |   bus analyzer.  A 8 bit code is written to an unused onboard memory     |
 |   location.  The bus analyzer can trap the event and timing studies      |
 |   can be made.
\*                                                                          */
void Mark(unsigned char byte)
{
  f_cc121.page3[0] = byte;
}

