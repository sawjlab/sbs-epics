#ifndef ARCNET_H
#define ARCNET_H

/*****************************************************************************\
 * File: arcnet.h                                   Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This header file contains definitions specific to the ARCNet driver,    *
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
 *   02/09/2001 - Initial release                                            *
\*****************************************************************************/

/* Define VME parameters for the COMPCONTROL CC121 ARCNet bnoard.
 */
#define IRQ 1                                     /* Interrupt level        */
#define INT_NUM 0x7F                              /* Interupt Vector        */
#define VME_BASE 0xDF0000                         /* Base (A24) VME address */

/* Define VxWorks message queue constants
 */
#define Qmax 4
#define MsgMax 256

/* Define bit masks for the SMSC COM90C26 status and interrupt mask
 * registers.
 */
#define b_TA    0x01
#define b_TMA   0x02
#define b_RECON 0x04
#define b_POR   0x10
#define b_RI    0x80

/* Define bit masks for the SMSC COM90C26 board control register.
 */
#define b_RES 0x01
#define b_EBI 0x02

/* Define bit masks for the SMSC COM90C26 command register.
 */
#define cmd_DISABLE_SEND 0x01
#define cmd_DISABLE_RECV 0x02
#define cmd_SEND_PAGE    0x03
#define cmd_RECV_PAGE    0x04
#define pg_0               0x00
#define pg_1               0x08
#define pg_2               0x10
#define pg_3               0x18
#define rcv_BROADCAST      0x80
#define cmd_CONFIGURE    0x05
#define cfg_256            0x00
#define cfg_512            0x08
#define cmd_CLEAR_FLAGS  0x06
#define CLR_POR            0x08
#define CLR_RECON          0x10

/* Define a data structure to be used by the driver software for accessing
 * the CC121 and its COM90C26 network controller chip.
 */
typedef struct
{
  /* Pointers to 512 byte memory pages
   */
  volatile unsigned char *page0, *page1, *page2, *page3;

  /* Pointers to onboard registers
   */
  volatile unsigned char *status, *intMask, *command, *intVec, *BCR;

  unsigned char
    nodeID,              /* ARCNet node number                    */
    shadowMask;          /* Copy of intMask (write only) register */

  int timeout;           /* Clock tick count of I/O timeout       */

  unsigned
    intTA,               /* TA interrupt counter                  */
    intRECON,            /* RECON interrupt counter               */
    intPOR,              /* POR interrupt counter                 */
    intRI,               /* RI interrupt counter                  */
    intHuh,              /* Unexpected interrupt counter          */
    recvWho,             /* Received message with no destination  */
    recvNoQ,             /* No MsgQ for received message          */
    sendTO,              /* Send timeout counter                  */
    recvTO;              /* Receive timeout counter               */
} CC121;

#endif

