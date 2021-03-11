/*
 *      Original Author: Ben-chin Cha
 *      Date:            3-10-92
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *      This software was produced under  U.S. Government contracts:
 *      (W-7405-ENG-36) at the Los Alamos National Laboratory,
 *      and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *      Initial development by:
 *              The Controls and Automation Group (AT-8)
 *              Ground Test Accelerator
 *              Accelerator Technology Division
 *              Los Alamos National Laboratory
 *
 *      Co-developed with
 *              The Controls and Computing Group
 *              Accelerator Systems Division
 *              Advanced Photon Source
 *              Argonne National Laboratory
 *
 * Modification Log:
 * -----------------
 * .01  mm-dd-yy        iii     Comment
 */


#include "cadef.h"
#include <db_access.h>
#if 0
#include <stdio.h>
#include <string.h>
#endif

/*#define NULL            0*/
#define NAME_LENGTH	31
#define CA_SUCCESS 0
#define CA_FAIL   -1
#define CA_WAIT   -2            /* error = -2 waiting for new value */
                                /* error = -1 not connected */

#define cs_never_conn   0       /* channel never conn*/
#define cs_prev_conn    1       /* channel previously  conn*/
#define cs_conn         2       /* channel conn*/
#define cs_closed       3       /* channel cleared because never conn*/

struct caGlobals {
        int CA_ERR;
        int devprflag;
        float PEND_IO_TIME;
        float PEND_IOLIST_TIME;
        float PEND_EVENT_TIME;
        };

typedef struct chandata{
	struct chandata *next;
/*        char name[NAME_LENGTH]; */
        chid chid;
        chtype type;
        evid evid;
	int state;
	int form;
	int error;  int event;
        int status;
        int severity;
        float value; 
        float uopr;   /* upper_disp_limit */
        float lopr;   /* lower_disp_limit */
        float upper_alarm_limit;
        float upper_warning_limit;
        float lower_warning_limit;
        float lower_alarm_limit;
        float upper_ctrl_limit;
        float lower_ctrl_limit;
        float setpoint;
        float max;
        float min;
	char units[8];
	char len;
	char string[MAX_STRING_SIZE]; 
        } chandata;

