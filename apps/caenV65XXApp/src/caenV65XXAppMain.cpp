/* caenVX65XXMain.cpp */
/* Author:  Marty Kraimer Date:    17MAR2000 */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <epicsStdioRedirect.h>

#include "epicsVersion.h"
#if (EPICS_VERSION>=3) && (EPICS_REVISION>=14) && (EPICS_MODIFICATION>=7)
#include "epicsExit.h"
#else
static void epicsExit(int code)
{
  /* dummy function, do nothing */
}
#endif

#include "epicsThread.h"
#include "iocsh.h"

/*
  In this file we define the IOC commands
  for initializing, starting and stopping 
  the sub processes that communicates with the CAEN V64XX HV boards.
*/

extern "C" 
{
#include <pthread.h>
  void V65XX_Init(int, uint32_t, uint32_t);
};
#include <iostream> 
using namespace std;

/* Define Arguments descriptors for "Start and Atop Commands */
static const iocshArg InitArgs[3] = { {"Number of VME boards", iocshArgInt }, 
				      {"Base Address", iocshArgInt },
				      {"Address Delta", iocshArgInt }};

/* Define the array of the pointers to the Arguments descriptors for
   Init command */
static const iocshArg* const  InitArgs_ptr[3] =
  { &InitArgs[0], &InitArgs[1], &InitArgs[2] };

/* Define arguments structures for Init, Start and Stop Commans */
static const iocshFuncDef Init_commDef  = {"V65XXInit",  3, InitArgs_ptr };

/* Initializing function */
static void  runInit_v65xx( const iocshArgBuf* args ) 
{
  cout << "Initializing v65xx Application " << endl;
  printf("Nboards = %d, %x, %x\n",args[0].ival,args[1].ival,args[2].ival);
  V65XX_Init((int)args[0].ival, (uint32_t)args[1].ival, (uint32_t)args[2].ival);
}


/* Define a class whos constructor registers Init, Start and Stop commands */
class IocShellComReg { 
 public:
  IocShellComReg( ) { 
    iocshRegister( &Init_commDef , runInit_v65xx ); 
  }
};

/*
  Define a dummy global variable to register Init, Start and Stop Commands  
  before the execution starts 
*/
static IocShellComReg dummyObj;


int main(int argc,char *argv[])
{
    if(argc>=2) {
        iocsh(argv[1]);
        epicsThreadSleep(.2);
    }
    iocsh(NULL);
    epicsExit(0);
    return(0);
}
