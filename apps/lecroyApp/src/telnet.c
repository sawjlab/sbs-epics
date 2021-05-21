/* Use telnet instead of arcnet */
/***************************************************************************** \
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


#include "telnet.h"
#include "extern.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "dbDefs.h"
//#include "vme.h"
//#include "sysLib.h"
//#include "vxLib.h"
//#include "taskLib.h"
//#include "intLib.h"
//#include "iv.h"
//#include "wdLib.h"
//#include "semLib.h"
//#include "logLib.h"
//#include "stdlib.h"
#include "unistd.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#ifdef vxWorks
#include "sockLib.h"
#endif

/* Allocate data structures for use by functions within this file.
 */

#define SERVERPORT 2001
#define BUFLEN 2048

/* Name: ARCsend                                                            *\
 | Parameters: sock - the socket of the destination LeCroy chassis         |
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
int ARCsend(int sock, const char *text, char *reply, unsigned size)
{
  int code;
  BOOL getMore;
  unsigned total = 0, lines = 0;
  unsigned comlength = strlen(text);
  int charsread;
  int charssent;
  char *ptr, *ptrterm;
  char buf[BUFLEN+1];

  if (!sock) return(-1);
  if (comlength > 253) return ErrReport("Message length", R_Warn);

  // Flush data
  //  charsread = read(sock, big, BUFLEN);
  if(reply) reply[0] = '\0';
  strcpy(buf,text);
  buf[comlength] = '\n';
  buf[comlength+1] = '\0';//printf("Sending '%s'\n",buf);
  charssent = send(sock, buf, comlength+1, 0);
  if(charssent < 0) {
    return(-1);
  }
  charsread = recv(sock, buf, BUFLEN, 0);
  if(charsread <= 0) {
    return(-1);
  }
  buf[charsread] = '\0';
  //  printf("Read %s\n",buf);

  ptr = buf;
  do {
    getMore = (ptr[0] == 'C');       /* Multiline reponse ?          */
    code = atoi(&buf[1]);
    ptrterm = strpbrk(ptr,"\r\f\n");
    if(ptrterm == NULL) {
      ptrterm = ptr+strlen(ptr);
    } else {
      *ptrterm = '\0';
    // Find end of line.  Either \r \f \0 or no more chars
    }
    code = atoi(&buf[1]);
    if(reply == NULL ) {
      char *codestring;
      printf("Code = %d\n",code);
      switch (code)
	{ case 1: codestring = "OK"; break;
	case 2: codestring = "VIEW"; break;
	case 3: codestring = "LOCAL"; break;
	case 4: codestring = "PANIC"; break;
	default: codestring = (code < 20) ? "UNDEF" : "ERR";
	}
      printf("Full response %s\n%s\n",codestring,ptr);
      printf("Without response code\n%s\n",&buf[7]);
    } else {
      int length = strlen(ptr);
      if (code !=1 || length < 8) {
	printf("%d %d\n%s\n",code,length,ptr);
	return ErrReport("Bad reply", R_Warn);
      }	  
      ptr += 7;
      length -= 7;
      if (total + length + 1 > size)
	return ErrReport("Buffer Overflow", R_Warn);
      strcat(reply,ptr);
      strcat(reply,"\n");
      total += length+1;
      ptr = ptrterm+1;
      lines++;
      // Find next char that is not null, \n \r or \t.
      // But don't walk past end of buffer
      while(*ptr=='\r' || *ptr=='\n' || *ptr == '\f') ptr++;
      if(ptr >= buf+charsread) getMore = 0;
    }
  } while (getMore);

  return lines;
}
void ARCsendtest() {
  char reply[2001];
  int lines = ARCsend(1,"SYSINFO",reply,2000);
  printf("ARCsend SYSINFO returned %d lines:\n%s\n",lines,reply);
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
  // This will be changed to take a node # and an IP address
  // It will build up a list list of sockets
  
  //  printf("Lecroy NODE %d at %s:%d\n",node,SERVERIP,SERVERPORT);

  return 0;
}

int ARCreconnect(int sock, char *ipaddr)
{
  int newsock;
  if(sock!=-1) {
    close(sock);
  }
  newsock = ARCsetup(ipaddr);
  if(newsock != -1) printf("Reconnected to %s\n",ipaddr);
  return(newsock);
}
/* Name: ARCsetup                                                           *\
 | Return: OK or ERROR                                                      |
 | Remarks:                                                                 |
 |   This function is called to initialize data structures prior to         |
 |   performing network I/O on the ARCNet.  It should only be called once   |
 |   during the lifetime of an application.                                 |
\*                                                                          */
int ARCsetup(char *ipaddr)
{
  // This will connect each socket and flush out any data
  // that comes back
  // For now just do one connection.
  struct sockaddr_in server;
  struct timeval tv;
  int sock;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("connect failed. Error");
    printf("Error opening socket in ARCsetup\n");
    return -1;
  }
  server.sin_addr.s_addr = inet_addr(ipaddr);
  server.sin_family = AF_INET;
  server.sin_port = htons(SERVERPORT);
  
  if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
    perror("Connect failed. Error");
    printf("Connecting to %s:%d failed\n",ipaddr,SERVERPORT);
    return -1;
  }
  tv.tv_sec = 10;
  tv.tv_usec = 0;
  
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,(void *) &tv, sizeof(struct timeval));
  //  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));

  // Send a command to flush any extra stuff
  
  //  ARCsend(sock, "\n", 0, 0);
#if 0
  send(sock, "\n", 1, 0);
  charsread = read(sock, buf, BUFLEN);
  if(charsread>0) printf("Received response from server\n%s\n",buf);
#endif
  return(sock);			/* Return the socket id */
}

/* Name: ARCend                                                             *\
 | Remarks:                                                                 |
 |   This function is called to end an ARCNet I/O session.  It resets the   |
 |   COM90C26 chip, disables interrupts, and frees the allocation of        |
 |   VxWorks data structures.                                               |
\*                                                                          */
void ARCend(void)
{

  /* Should close the telent connections here */

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
#if 0
int keyboard(unsigned ID)
{
  char buffer[128];
  int sock;

  errno = 0;                                 /* Initialize to OK          */
  if (ARCnode(ID) != 0) return -1;
  if ((sock=ARCsetup("129.57.168.23")) != -1) return -1;

  while (TRUE)
  { printf("\nEnter HV command: ");          /* Get the command from user */
    if (gets(buffer) == NULL) break;
    if (buffer[0] == '\0') break;
    ARCsend(sock, buffer, NULL, 0);
  }

  ARCend();
  return 0;
}
#endif

/* Name: ComStats                                                           *\
 | Remarks:                                                                 |
 |   This functions prints CC121 network communications statistics to       |
 |   the console.                                                           |
\*                                                                          */
void ComStats(void)
{
  printf("\nUsing telnet protocol\n");
}
