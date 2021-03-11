/* EPICS channel access calls for tcl */
/* Johannes van Zeijts, March 94, updated 28 August 1994 */
/* Fixed a bug that only became apparant on SUN, (saw) July 7, 2000 */
/*   had chandataPtr->state = ca_field_type(...) instead of =ca_state() */

#include "tcl.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#undef INLINE
#include "db_access.h"
#include "cadef.h"
#include "chandata.h"

struct caGlobals CA = {
  0,				/* CA_ERR */
  0,				/* devprflag */
  0.35,				/* PEND_IO_TIME */
  3.0,				/* PEND_IOLIST_TIME */
  0.001};			/* PEND_EVENT_TIME */

static Tcl_Interp *pinterp;
static Tcl_HashTable chandataTable;

/* this code is not used anymore , use C code below instead */
int EPICSINITED = 0;
char *epicsupdatecmd = "global Control; set Control(Epicsupdate) 200; proc update.epics { } {\n global Control\n epics update\n after $Control(Epicsupdate) update.epics\n}\n update.epics\n";


int ca_find_dev(char *name, chandata **chandataHandle);
int ca_pvlist_search(int noName, const char **pvName, chandata **list);
int ca_monitor_add_event_array(int noName, const char **pvName);
void ca_check_command_error(int i);
void ca_monitor_add_event(chandata *chandataPtr);
void ca_monitor_clear_event(chandata *chandataPtr);
int ca_monitor_clear_event_array(int noName, const char **pvName);
int ca_check_return_code(chandata *chandataPtr, int status);
void ca_execerror(const char *s, const char *t);

void EpicsUpdateProc(dummy)
     ClientData dummy;
{
  ca_pend_event(0.0001);
  Tcl_CreateTimerHandler(200, EpicsUpdateProc, dummy);
}

static int
EpicsCmd(dummy, interp, argc, argv)
     ClientData dummy;			/* Not used. */
     Tcl_Interp *interp;		/* Current interpreter. */
     int argc;				/* Number of arguments. */
     char **argv;			/* Argument strings. */
{
  double value;
  int listArgc; const char **listArgv;
  chid chan; int status;
  static char ret_string[MAX_STRING_SIZE];
  int code;
  chandata *chandataPtr;

  if (argc < 2) {
    interp->result = "wrong # args";
    return TCL_ERROR;
  }

  pinterp = interp;

  if (strcmp(argv[1], "update") == 0) {
    if (argc > 2) {
      interp->result = "wrong # args";
      return TCL_ERROR;
    }
    status = ca_pend_event(0.0001);
  }
  else if (strcmp(argv[1], "get") == 0) {
    if (argc > 3) {
      interp->result = "wrong # args";
      return TCL_ERROR;
    }

    if (ca_find_dev(argv[2],&chandataPtr) != TCL_OK) {
      interp->result = "ca_find_dev failed in get";
      printf("ca_find_dev failed in get %s\n",argv[2]);
      return TCL_ERROR;
    }

    chan = chandataPtr->chid;

    if (ca_field_type(chan) == TYPENOTCONN) {
      Tcl_AppendResult(interp, "Signal ", argv[2]," not connected",(char *) NULL);
      return TCL_ERROR;
    } else {
      status = ca_get(DBR_STRING,chan,ret_string);
      if (ca_pend_io(3.0) == ECA_TIMEOUT) {
	Tcl_AppendResult(interp, "Signal ", argv[2]," Get timed out",(char *) NULL);
	return TCL_ERROR;
      } else {
	interp->result = ret_string;
	return TCL_OK;
      }
    }
  }
  else if (strcmp(argv[1], "put") == 0) {
    if (argc > 4) {
      interp->result = "wrong # args";
      return TCL_ERROR;
    }
    if (Tcl_GetDouble(interp,argv[3],&value) != TCL_OK) {
      Tcl_AppendResult(interp, "Expected floating point number got:", argv[3],(char *) NULL);
      return TCL_ERROR;
    }

    if (ca_find_dev(argv[2],&chandataPtr) != TCL_OK) {
	interp->result = "ca_find_dev failed in put";
      return TCL_ERROR;
    }

    chan = chandataPtr->chid;
    if (ca_field_type(chan) == TYPENOTCONN) {
      Tcl_AppendResult(interp, "Signal ", argv[2]," not connected",(char *) NULL);
      return TCL_ERROR;
    } else {
      if (ca_field_type(chan) == DBR_ENUM) {
	sprintf(ret_string, "%d", (int) (value+0.5) );
      } else {
	sprintf(ret_string,"%f",  value);}
      if (ca_field_type(chan) == DBR_STRING) {
	status = ca_put(DBR_STRING,chan,ret_string);
      }
      else {
	status = ca_put(DBR_DOUBLE,chan,&value);
      }
      if (status != ECA_NORMAL) {
	strcpy(interp->result,ca_message(status));
	return TCL_ERROR;}
      status = ca_flush_io();
      if (status != ECA_NORMAL) {
	strcpy(interp->result,ca_message(status));
	return TCL_ERROR;}
    }
  }
  else if (strcmp(argv[1], "init") == 0) {
    if (argc > 3) {
      interp->result = "wrong # args";
      return TCL_ERROR;
    }
    return TCL_OK;
  }
  else if (strcmp(argv[1], "addlist") == 0) {
    if (argc != 3) {
      interp->result = "wrong # args";
      return TCL_ERROR;
    }
    if (!EPICSINITED) {
      if (ca_task_initialize() == ECA_ALLOCMEM) {
	Tcl_AppendResult(interp, "Unable to initialize Channel Access", (char *) NULL);
	return TCL_ERROR;
      }     
      EPICSINITED = 1;
      EpicsUpdateProc(dummy);
    }
    if (Tcl_SplitList(interp,argv[2],&listArgc,&listArgv) != TCL_OK) return TCL_ERROR;   
    code = ca_monitor_add_event_array(listArgc,listArgv); 
    Tcl_Free((char *) listArgv);
    if(code!=TCL_OK) interp->result = "ca_monitor_add_event_array failed";
    return code;
  }
  else if (strcmp(argv[1], "clearlist") == 0) {
    if (argc != 3) {
      interp->result = "wrong # args";
      return TCL_ERROR;
    }
    if (!EPICSINITED) {
      if (ca_task_initialize() == ECA_ALLOCMEM) {
	Tcl_AppendResult(interp, "Unable to initialize Channel Access", (char *) NULL);
	return TCL_ERROR;
      }     
      EPICSINITED = 1;
    }
    if (Tcl_SplitList(interp,argv[2],&listArgc,&listArgv) != TCL_OK) return TCL_ERROR;   
    code = ca_monitor_clear_event_array(listArgc,listArgv); 
    Tcl_Free((char *) listArgv);
    if(code!=TCL_OK) interp->result = "ca_monitor_clear_event_array failed";
    return code;
  } else if (strcmp(argv[1], "exit") == 0) {
    status = ca_task_exit();
    SEVCHK(status,"tclTaskInit: ca_task_initialize failed!");
    if (status != ECA_NORMAL) {
      interp->result = "not ECA_NORMAL";
      return TCL_ERROR;
    }
  }
  else if (strcmp(argv[1], "hashstats") == 0) {
    if (argc != 2) {
      interp->result = "wrong # args";
      return TCL_ERROR;
    }
    interp->result = Tcl_HashStats(&chandataTable);
    interp->freeProc = (Tcl_FreeProc *) free;
    return TCL_OK;
  }
  else  {
    Tcl_AppendResult(interp, "bad argument", argv[2], "should be get or set", (char *) NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

EXTERN int
Epics_Init(interp)
     Tcl_Interp *interp;		/* Interpreter to initialize. */
{
  /* 17-MAR-94  -Johannes add EPICS access */
  Tcl_InitStubs(interp, "8.5", 0);
  Tcl_CreateCommand(interp, "epics", EpicsCmd, (ClientData *) NULL, (Tcl_CmdDeleteProc *)NULL);
  /* 17-JUL-94  -Johannes add Tcl hash table */
  Tcl_PkgProvide(interp, "epics", "2.0");
  Tcl_InitHashTable(&chandataTable,TCL_STRING_KEYS);
  return TCL_OK;
}

/******************************************************
  add a monitor list
  ******************************************************/
int ca_monitor_add_event_array(noName,pvName)
     int noName;
     const char **pvName;
{
  int command_error=0;
  chandata *list,*snode,*chandataPtr;
  
  command_error = ca_pvlist_search(noName,pvName,&list);
  snode = list;
  while (snode)  {
    chandataPtr = snode;
    chandataPtr->type = ca_field_type(chandataPtr->chid);
    if (chandataPtr->type == TYPENOTCONN) {
      int i = 0;
      fprintf(stderr,"%-30s  ***  device not found\n",pvName[i]);
      command_error = CA_FAIL;
    }
    else  if (chandataPtr->evid == NULL) {
      ca_monitor_add_event(chandataPtr); 
    }
    snode = snode->next;
  }
  ca_check_command_error(command_error);
  if (command_error == CA_FAIL) { return TCL_ERROR; } else {return TCL_OK;}
}

/******************************************************
  clear a monitor list
  ******************************************************/
int ca_monitor_clear_event_array(noName,pvName)
     int noName;
     const char **pvName;
{
  int i,command_error=0;
  chandata *chandataPtr,*ca_get_hash_table();
  
  for (i=0;i<noName;i++) {
    chandataPtr = (chandata *) ca_get_hash_table(pvName[i]);
    if (chandataPtr == NULL) {
      command_error = CA_FAIL;
    } else {
      ca_monitor_clear_event(chandataPtr); 
    }
  }
  ca_flush_io();
  if (command_error == CA_FAIL) { return TCL_ERROR; } else {return TCL_OK;}
}

void ca_get_dbr_sts_string_callback(args)
     struct event_handler_args args;
{
  chandata *chandataPtr;
  char *result;
  char *stemp;
  chandataPtr = (chandata *)args.usr;
  result = ((struct dbr_sts_string *)args.dbr)->value;
  stemp = (char *) malloc(strlen(ca_name(chandataPtr->chid))+1);
  strcpy(stemp,ca_name(chandataPtr->chid));
  Tcl_SetVar2(pinterp,"Control",stemp,result,TCL_GLOBAL_ONLY);
  /*  printf("Control(%s)=%s\n",stemp,result);*/

  free(stemp);
}

/****************************************************
 *  change connection event callback 
 ****************************************************/
void ca_connect_change_event(args)
     struct connection_handler_args args;
{
  chandata *chandataPtr;
/* args.usr */
  chandataPtr = (chandata *) ca_puser(args.chid);

  if (chandataPtr->type != TYPENOTCONN) {
    fprintf(stderr,"****WARNING****\n");
    fprintf(stderr,"****Reconnection happend on %s ****\n",
	    ca_name(chandataPtr->chid));
    fprintf(stderr,"****You have to wait for CA  re-connection.\n\n");
  }
}

void ca_connect_handler(args)
     struct connection_handler_args args;
{
  chandata *chandataPtr;
  chandataPtr = (chandata *) ca_puser(args.chid);
}

/****************************************************
 *  add connection event for a channel
 ****************************************************/
void ca_connect_add_event(chandataPtr)
     chandata *chandataPtr;
{
  void ca_connect_change_event();
  /*  ca_puser(chandataPtr->chid) = chandataPtr;*/
  ca_change_connection_event(chandataPtr->chid,ca_connect_change_event);
}

/******************************************************
  add a monitor channel 
  ******************************************************/
void ca_monitor_add_event(chandataPtr)
     chandata *chandataPtr;
{
  int status;
  void ca_get_dbr_sts_string_callback();
  status = ca_add_event(DBR_STS_STRING,
			chandataPtr->chid,
			ca_get_dbr_sts_string_callback,
			chandataPtr,
			&(chandataPtr->evid));

  ca_check_return_code(chandataPtr,status);
  ca_connect_add_event(chandataPtr);
}

/******************************************************
  clear  a monitor channel 
  ******************************************************/
void ca_monitor_clear_event(chandataPtr)
     chandata *chandataPtr;
{
  int status;
  if (chandataPtr->evid) {
    status = ca_clear_event(chandataPtr->evid);

    ca_check_return_code(chandataPtr,status);
    chandataPtr->evid = NULL;
  }
}

void ca_execerrorp(s,t)
     char *s, *t;
{
  Tcl_AppendResult(pinterp, s, t, (char *) NULL);
}


/**************************************************
 * find the channel name from IOC 
 **************************************************/
int ca_find_dev(name, chandataHandle)
     char *name;
     chandata **chandataHandle;
{
  int status;
  chandata *chandataPtr,*ca_check_hash_table();

  /* populate hash table here return the address */
  chandataPtr = (chandata *)ca_check_hash_table(name);
  status = ca_pend_io(CA.PEND_IO_TIME);

  if (ca_check_return_code(chandataPtr,status) != TCL_OK) {
    return TCL_ERROR;
  }
  
  chandataPtr->type = ca_field_type(chandataPtr->chid);

  if (chandataPtr->state != cs_closed)
    chandataPtr->state = ca_state(chandataPtr->chid);
  else {
    CA.CA_ERR = CA_FAIL;
    *chandataHandle = chandataPtr;
    return TCL_ERROR;
  }

  if (chandataPtr->type == TYPENOTCONN) {
    ca_execerror(ca_name(chandataPtr->chid),"--- Invalid device name");
    if (chandataPtr->state == cs_never_conn) 
      status = ca_clear_channel(chandataPtr->chid);
    if (status != ECA_NORMAL)   {
      ca_check_return_code(chandataPtr,status);
    }
    chandataPtr->state = ca_state(chandataPtr->chid);
    /* What was the next line trying to accomplish? */
    /*    ca_field_type(chandataPtr->chid) = TYPENOTCONN;*/
    *chandataHandle = chandataPtr;

    if(chandataPtr->type > 0 && chandataPtr->type != 3) {
      chandataPtr->form = 0;	/* not enum type */ 
    }
    return(TCL_ERROR);
  }

  if(chandataPtr->type > 0 && chandataPtr->type != 3) {
    chandataPtr->form = 0;	/* not enum type */ 
  }

  *chandataHandle = chandataPtr;
  return(TCL_OK);
}

/**************************************************
 * check error code return by CA for a single device
 **************************************************/
int ca_check_return_code(chandataPtr,status)
     chandata *chandataPtr;
     int status;
{
  if (status == ECA_NORMAL) {
    CA.CA_ERR = CA_SUCCESS;
    chandataPtr->error = CA_SUCCESS;
    return TCL_OK;
  }
  else { 
    CA.CA_ERR = CA_FAIL;
    chandataPtr->error = -2;
  }
  if (status == ECA_TIMEOUT)
    ca_execerror("The operation timed out: ",ca_name(chandataPtr->chid));
  if (status == ECA_GETFAIL)
    ca_execerror("A local database get failed: ",ca_name(chandataPtr->chid));
  if (status == ECA_BADCHID)
    ca_execerror("Unconnected or corrupted chid: ",ca_name(chandataPtr->chid));
  if (status == ECA_BADCOUNT)
    ca_execerror("More than native count requested: ",ca_name(chandataPtr->chid));
  if (status == ECA_BADTYPE)
    ca_execerror("Unknown GET_TYPE: ",ca_name(chandataPtr->chid));
  if (status == ECA_STRTOBIG)
    ca_execerror("Unusually large string supplied: ",ca_name(chandataPtr->chid));
  if (status == ECA_ALLOCMEM)
    ca_execerror("Unable to allocate memory: ",ca_name(chandataPtr->chid));

  return TCL_ERROR;
}

/****************************************************
  check for command return error code 
  *****************************************************/
void ca_check_command_error(i) 
     int i;
{
  if (i != 0) 
    CA.CA_ERR = CA_FAIL;
  else CA.CA_ERR = CA_SUCCESS;

}

/**************************************************
 *   print error message
 *************************************************/
void ca_execerror(s,t)
     const char *s, *t;
{
  fprintf(stderr,"%s %s\n ",s,t);
}

/**************************************************
 * check a list of array error code return by CA
 **************************************************/
void ca_check_array_return_code(status)
     int status;
{
  if (status == ECA_NORMAL) {
    CA.CA_ERR = CA_SUCCESS;
    return;
  }
  else CA.CA_ERR = CA_FAIL;
  if (status == ECA_TIMEOUT) {
#if 0
    ca_execerror("The operation timed out: "," on list of devices.");
#else
    fprintf(stderr,"The operation time out on something\n");
#endif
  }
} 

/**************************************************
 *  casearch for a device array
 *  time out return CA_FAIL else return 0
 *************************************************/
int ca_pvlist_search(noName,pvName,list)
     int noName;
     const char **pvName;
     chandata **list;
{
  int i,status,command_error=0;
  chandata *pnow,*phead,*chandataPtr,*ca_check_hash_table();
  
  phead = (chandata *)list;
  pnow = phead;
  for (i=0;i<noName;i++) {
    chandataPtr = (chandata *)ca_check_hash_table(pvName[i]);
    pnow->next = chandataPtr;
    chandataPtr->next = NULL;
    pnow = chandataPtr;
  }
  status = ca_pend_io(CA.PEND_IOLIST_TIME);
  if (status != ECA_NORMAL) ca_check_array_return_code(status);
  
  pnow = phead->next;
  i=0;
  while (pnow)  {
    chandataPtr = pnow;
    if (chandataPtr->error == CA_WAIT) status = ECA_TIMEOUT;
    chandataPtr->type = ca_field_type(chandataPtr->chid);
    if (chandataPtr->state != cs_closed)
      chandataPtr->state = ca_state(chandataPtr->chid);
    
    if (chandataPtr->type == TYPENOTCONN) {
      fprintf(stderr,"%-30s  ***  device not found\n",pvName[i]);
      chandataPtr->error = CA_WAIT;
      command_error = CA_FAIL;
      
      if (chandataPtr->state == cs_never_conn) {
	if (status != ECA_NORMAL)   {
	  ca_check_return_code(chandataPtr,status);
	}
	ca_clear_channel(chandataPtr->chid);
	chandataPtr->state = ca_state(chandataPtr->chid);
      }
    }
    pnow = pnow->next; i++;
  }
  
  ca_check_command_error(command_error);
  if (status == ECA_NORMAL && command_error == CA_SUCCESS) return(CA_SUCCESS);
  else return(CA_FAIL);
}

/******************************************
 * allocate chandata 
 ******************************************/
static chandata *AllocChandata()
{
  chandata *cdata;
  cdata = (chandata *) calloc(1,sizeof(chandata));
  if (cdata == NULL) {
    fprintf(stderr,"AllocChandata failed on chandataPtr\n");
    exit(-2);
  }
  cdata->type = TYPENOTCONN;
  return (cdata);
}

chandata *ca_check_hash_table(name)
     char *name;
{
  void ca_connect_handler();
  int new,status;
  Tcl_HashEntry *entryPtr;
  chandata *chandataPtr;
  entryPtr = Tcl_CreateHashEntry(&chandataTable,name,&new);
  if (new) {
    chandataPtr = (chandata *) AllocChandata(); 
    Tcl_SetHashValue(entryPtr,chandataPtr);
    status = ca_search(name,&(chandataPtr->chid));

    /*
       status = ca_build_and_connect(name,TYPENOTCONN,0,&(chandataPtr->chid),0,ca_connect_handler,0);
     */

    if (status != ECA_NORMAL) {
      ca_check_return_code(chandataPtr,status);
    }
  }
    
  return (chandata *) Tcl_GetHashValue(entryPtr);
}

chandata *ca_get_hash_table(name)
     char *name;
{
  Tcl_HashEntry *entryPtr;
  entryPtr = Tcl_FindHashEntry(&chandataTable,name);
  if (entryPtr == NULL) {
    Tcl_AppendResult(pinterp, "no signal ",name," in event list",(char *) NULL); 
    return NULL;
  }
  return (chandata *) Tcl_GetHashValue(entryPtr);
}







