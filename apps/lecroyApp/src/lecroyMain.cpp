/* ioclecroyMain.cpp */
/* Author:  Marty Kraimer Date:    17MAR2000 */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <epicsStdioRedirect.h>

/* NSCL - Feb 2009 - J.Priller
   R3.14.6 doesn't define epicsExit(), include dummy func if detected */
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
  for initializzing, starting and stopping
  Lecroy HV chasses. 
*/

extern "C" 
{
#include <pthread.h>
  int HVAddCrate(int, char *);
  int HVstart();
  int hinv();
};
#include <iostream> 
using namespace std;

/* Define Arguments descriptors for "Start and Atop Commands */
static const iocshArg AddCrateArgs[2] = { {"Chassis Number", iocshArgInt }, 
				       {"Chassis IP    ", iocshArgString } };

/* Define the array of the pointers to the Arguments descriptors for
   AddCrate command */
static const iocshArg* const  AddCrateArgs_ptr[2] = { &AddCrateArgs[0], &AddCrateArgs[1] };

/* Define arguments structures for Init, Start and Stop Commans */
static const iocshFuncDef Init_commDef  = {"HVstart",  0, 0 };
static const iocshFuncDef Info_commDef  = {"HVinfo",  0, 0 };
static const iocshFuncDef AddCrate_commDef = {"HVAddCrate", 2, AddCrateArgs_ptr };

/* Initializing function */
static void  runInit_lecroy( const iocshArgBuf* args ) 
{
  cout << "Initializing lecroy Application " << endl;
  HVstart(); 
}

/* Inventory  function */
static void  runInfo_lecroy( const iocshArgBuf* args ) 
{
  hinv(); 
}


/* Start function */
static void  runAddCrate_lecroy( const iocshArgBuf* args ) 
{
  cout << "Configuring lecroy Application for chassis " << args[0].ival << 
    " at " << args[1].sval << endl;
  HVAddCrate((int)args[0].ival, args[1].sval ) ;
}

/* Define a class whos constructor registers Init, Start and Stop commands */
class IocShellComReg { 
 public:
  IocShellComReg( ) { 
    iocshRegister( &Init_commDef , runInit_lecroy ); 
    iocshRegister( &Info_commDef , runInfo_lecroy ); 
    iocshRegister( &AddCrate_commDef, runAddCrate_lecroy ); 
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
