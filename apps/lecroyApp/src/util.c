/*****************************************************************************\
 * File: util.c                                     Author: Chris Slominski  *
 *                                                                           *
 * Overview:                                                                 *
 *   This file contains commonly used functions for the LeCroy 1458 driver   *
 *                                                                           *
 * Revision History:                                                         *
 *   12/22/2000 - Initial release                                            *
 *   04/19/2001 - Added time stamp to ErrReport for (v1-1).                  *
\*****************************************************************************/

#include "define.h"
#include "string.h"
#include "time.h"
#include "errno.h"
#include "typedefs.h"

/*                                                                           *\
 | Name: ErrReport                                                           |
 | Parameters: msg - message string to show                                  |
 |             type - severity level to return                               |
 | Return: passed severity level                                             |
 | Remarks:                                                                  |
 |   This function is called from functions that have detected an anomoly    |
 |   as follows "return ErrReport("message", R_...);".  The message they     |
 |   pass is printed and their severity code is returned.  If the task's     |
 |   error number has a valid code, it is also appended to the message.      |
\*                                                                           */
int ErrReport(char *msg, ErrType type)
{
  char *ptr;
  time_t ltime;

  /* Used passed severity code to select corresponding severity text.
   * Then print severity along with the a timestamp.
   */
  switch (type)
  { case R_Info: ptr = "Information"; break;
    case R_Warn: ptr = "Warning"; break;
    case R_Fatal: ptr = "Fatal Error"; break;
    default: ptr = "<unknown>";
  }

  time(&ltime);
  printf("!!! %s issued on %s", ptr, ctime(&ltime));

  if (errno == 0)
    printf("  Diagnostic -> %s\n", msg);
  else
  { printf("  Diagnostic -> %s (%s)\n", msg, strerror(errno));
    errno = 0;
  }
  return type;
}

