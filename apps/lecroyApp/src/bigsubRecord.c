/* recBigsub.c */
/* share/src/rec @(#)recBigsub.c	1.19     6/4/93 */

/* recBigsub.c - Record Support Routines for Subroutine records */
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            01-25-90
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
 * .01  10-10-90	mrk	Made changes for new record support
 * .02  11-11-91        jba     Moved set and reset of alarm stat and sevr to macros
 * .03  01-08-92        jba     Added casts in symFindByName to avoid compile warning messages
 * .04  02-05-92	jba	Changed function arguments from paddr to precord 
 * .05  02-28-92        jba     Changed get_precision,get_graphic_double,get_control_double
 * .06  02-28-92	jba	ANSI C changes
 * .07  04-10-92        jba     pact now used to test for asyn processing, not status
 * .08  06-02-92        jba     changed graphic/control limits for hihi,high,low,lolo
 * .09  07-15-92        jba     changed VALID_ALARM to INVALID alarm
 * .10  07-16-92        jba     added invalid alarm fwd link test and chngd fwd lnk to macro
 * .11  07-21-92        jba     changed alarm limits for non val related fields
 * .12  08-06-92        jba     New algorithm for calculating analog alarms
 * .13  08-06-92        jba     monitor now posts events for changes in a-t
 * .14  10-10-92        jba     replaced code with recGblGetLinkValue call
 * .15  10-18-93        mhb     Built big subroutine record from sub record
 * .16  02-05-95	jt	use update to r3.12
 * .16  11-10-00	csh	monitor now posts events for changes in nla-nlt and 
 *				symFindByName is replaced with symFindByNameEPICS for PPC support
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "dbDefs.h"
#include "epicsPrint.h"
#include "registryFunction.h"
#include "alarm.h"
#include "callback.h"
#include "cantProceed.h"
#include "dbAccess.h"
#include "epicsPrint.h"
#include "dbEvent.h"
#include "dbFldTypes.h"
#include "errMdef.h"
#include "recSup.h"
#include "recGbl.h"
#define GEN_SIZE_OFFSET
#include	<bigsubRecord.h>
#undef GEN_SIZE_OFFSET
#include "epicsExport.h"

#include "a_out.h"

#ifndef OK
#define OK (0)
#endif


/* Create RSET - Record Support Entry Table*/
#define report NULL
#define initialize NULL
static long init_record();
static long process();
#define special NULL
#define get_value NULL
#define cvt_dbaddr NULL
#define get_array_info NULL
#define put_array_info NULL
static long get_units();
static long get_precision();
#define get_enum_str NULL
#define get_enum_strs NULL
#define put_enum_str NULL
static long get_graphic_double();
static long get_control_double();
static long get_alarm_double();

rset bigsubRSET={
	RSETNUMBER,
	report,
	initialize,
	init_record,
	process,
	special,
	get_value,
	cvt_dbaddr,
	get_array_info,
	put_array_info,
	get_units,
	get_precision,
	get_enum_str,
	get_enum_strs,
	put_enum_str,
	get_graphic_double,
	get_control_double,
	get_alarm_double };
epicsExportAddress(rset,bigsubRSET);

static void myalarm();
static long do_sub();
static long fetch_values();
static long push_values();
static void monitor();

#define IN_ARG_MAX 20
#define OUT_ARG_MAX 8
typedef long (*SUBFUNCPTR)();


static long init_record(pbigsub,pass)
    struct bigsubRecord	*pbigsub;
    int pass;
{
    SUBFUNCPTR	psubroutine;
    char	sub_type;  
    char	temp[40];
    long	status;
    /*    STATUS	ret;  */
    struct link *plink;
    int i;
    double *pvalue;

    if (pass==0) return(0);

    plink = &pbigsub->inpa;
    pvalue = &pbigsub->a;
    for(i=0; i<IN_ARG_MAX; i++, plink++, pvalue++) {
        if(plink->type==CONSTANT){
            recGblInitConstantLink(plink, DBF_DOUBLE, pvalue);
        } 
    }

    if (strlen(pbigsub->inam)!=0) {
    /* convert the initialization subroutine name  */
    temp[0] = 0;			/* all global variables start with _ */
    if (pbigsub->inam[0] != '_'){
      /*    strcpy(temp,"_");  */
    }
    strcat(temp,pbigsub->inam);

    pbigsub->sadr = registryFunctionFind( (const char*)temp );

    if ( pbigsub->sadr == 0 ) {
      recGblRecordError(S_db_BadSub,(void *)pbigsub,"recBigsub(init_record)");
      return(S_db_BadSub);
    }

    /* invoke the initialization subroutine */
    psubroutine = (SUBFUNCPTR)(pbigsub->sadr);
    status = psubroutine(pbigsub,process);

    }

    if (strlen(pbigsub->snam)==0) {
	  epicsPrintf("%s snam not specified\n", 
				  pbigsub->name); 
		pbigsub->pact = TRUE;
		return(0);
    }

    /* convert the subroutine name to an address and type */
    /* convert the processing subroutine name  */
    temp[0] = 0;			/* all global variables start with _ */
    if (pbigsub->snam[0] != '_'){
      /*      strcpy(temp,"_");  */
    }
    strcat(temp,pbigsub->snam);

    pbigsub->sadr = registryFunctionFind( (const char*)temp );
    if ( pbigsub->sadr == 0 ) {
	recGblRecordError(S_db_BadSub,(void *)pbigsub,"recBigsub(init_record)");
	return(S_db_BadSub);
    } 
	  
    sub_type = N_TEXT;
    pbigsub->styp = sub_type;
    return(0);
}

static long process(pbigsub)
	struct bigsubRecord *pbigsub;
{
	long		 status=0;
	unsigned char	 pact=pbigsub->pact;

	if(!pbigsub->pact || !pbigsub->sadr){
		pbigsub->pact = TRUE;
		status = fetch_values(pbigsub);
		pbigsub->pact = FALSE;
	}

	if(status==0) status = do_sub(pbigsub);
	if(!pact && pbigsub->pact) return(0);
        pbigsub->pact = TRUE;
	if(status==1) return(0);
	recGblGetTimeStamp(pbigsub);
/* check for alarms */
        myalarm(pbigsub);
        /* check event list */
        monitor(pbigsub);
        /* process the forward scan link record */
        recGblFwdLink(pbigsub);

	/* Push out the output link data values */
	status = push_values(pbigsub);
//        /* check for alarms */
//        myalarm(pbigsub);
        pbigsub->pact = FALSE;
        return(status);
}


static long get_units(paddr,units)
    struct dbAddr *paddr;
    char	  *units;
{
    struct bigsubRecord	*pbigsub=(struct bigsubRecord *)paddr->precord;

    strncpy(units,pbigsub->egu,DB_UNITS_SIZE);
    return(0);
}

static long get_precision(paddr,precision)
    struct dbAddr *paddr;
    long	  *precision;
{
    struct bigsubRecord	*pbigsub=(struct bigsubRecord *)paddr->precord;

    *precision = pbigsub->prec;
    if(paddr->pfield==(void *)&pbigsub->val) return(0);
    recGblGetPrec(paddr,precision);
    return(0);
}


static long get_graphic_double(paddr,pgd)
    struct dbAddr *paddr;
    struct dbr_grDouble	*pgd;
{
    struct bigsubRecord	*pbigsub=(struct bigsubRecord *)paddr->precord;

    if(paddr->pfield==(void *)&pbigsub->val
    || paddr->pfield==(void *)&pbigsub->hihi
    || paddr->pfield==(void *)&pbigsub->high
    || paddr->pfield==(void *)&pbigsub->low
    || paddr->pfield==(void *)&pbigsub->lolo){
        pgd->upper_disp_limit = pbigsub->hopr;
        pgd->lower_disp_limit = pbigsub->lopr;
        return(0);
    }

    if(paddr->pfield>=(void *)&pbigsub->a && paddr->pfield<=(void *)&pbigsub->t){
        pgd->upper_disp_limit = pbigsub->hopr;
        pgd->lower_disp_limit = pbigsub->lopr;
        return(0);
    }
    if(paddr->pfield>=(void *)&pbigsub->la && paddr->pfield<=(void *)&pbigsub->lt){
        pgd->upper_disp_limit = pbigsub->hopr;
        pgd->lower_disp_limit = pbigsub->lopr;
        return(0);
    }
    return(0);
}

static long get_control_double(paddr,pcd)
    struct dbAddr *paddr;
    struct dbr_ctrlDouble *pcd;
{
    struct bigsubRecord	*pbigsub=(struct bigsubRecord *)paddr->precord;

    if(paddr->pfield==(void *)&pbigsub->val
    || paddr->pfield==(void *)&pbigsub->hihi
    || paddr->pfield==(void *)&pbigsub->high
    || paddr->pfield==(void *)&pbigsub->low
    || paddr->pfield==(void *)&pbigsub->lolo){
        pcd->upper_ctrl_limit = pbigsub->hopr;
        pcd->lower_ctrl_limit = pbigsub->lopr;
       return(0);
    } 

    if(paddr->pfield>=(void *)&pbigsub->a && paddr->pfield<=(void *)&pbigsub->t){
        pcd->upper_ctrl_limit = pbigsub->hopr;
        pcd->lower_ctrl_limit = pbigsub->lopr;
        return(0);
    }
    if(paddr->pfield>=(void *)&pbigsub->la && paddr->pfield<=(void *)&pbigsub->lt){
        pcd->upper_ctrl_limit = pbigsub->hopr;
        pcd->lower_ctrl_limit = pbigsub->lopr;
        return(0);
    }
    return(0);
}

static long get_alarm_double(paddr,pad)
    struct dbAddr *paddr;
    struct dbr_alDouble	*pad;
{
    struct bigsubRecord	*pbigsub=(struct bigsubRecord *)paddr->precord;

    if(paddr->pfield==(void *)&pbigsub->val){
         pad->upper_alarm_limit = pbigsub->hihi;
         pad->upper_warning_limit = pbigsub->high;
         pad->lower_warning_limit = pbigsub->low;
         pad->lower_alarm_limit = pbigsub->lolo;
    } else recGblGetAlarmDouble(paddr,pad);
    return(0);
}

static void myalarm(pbigsub)
    struct bigsubRecord	*pbigsub;
{
	double		val;
	float		hyst, lalm, hihi, high, low, lolo;
	unsigned short	hhsv, llsv, hsv, lsv;

	if(pbigsub->udf == TRUE ){
 		recGblSetSevr(pbigsub,UDF_ALARM,INVALID_ALARM);
		return;
	}
	hihi = pbigsub->hihi; lolo = pbigsub->lolo; high = pbigsub->high; low = pbigsub->low;
	hhsv = pbigsub->hhsv; llsv = pbigsub->llsv; hsv = pbigsub->hsv; lsv = pbigsub->lsv;
	val = pbigsub->val; hyst = pbigsub->hyst; lalm = pbigsub->lalm;

	/* alarm condition hihi */
	if (hhsv && (val >= hihi || ((lalm==hihi) && (val >= hihi-hyst)))){
	        if (recGblSetSevr(pbigsub,HIHI_ALARM,pbigsub->hhsv)) pbigsub->lalm = hihi;
		return;
	}

	/* alarm condition lolo */
	if (llsv && (val <= lolo || ((lalm==lolo) && (val <= lolo+hyst)))){
	        if (recGblSetSevr(pbigsub,LOLO_ALARM,pbigsub->llsv)) pbigsub->lalm = lolo;
		return;
	}

	/* alarm condition high */
	if (hsv && (val >= high || ((lalm==high) && (val >= high-hyst)))){
	        if (recGblSetSevr(pbigsub,HIGH_ALARM,pbigsub->hsv)) pbigsub->lalm = high;
		return;
	}

	/* alarm condition low */
	if (lsv && (val <= low || ((lalm==low) && (val <= low+hyst)))){
	        if (recGblSetSevr(pbigsub,LOW_ALARM,pbigsub->lsv)) pbigsub->lalm = low;
		return;
	}

	/* we get here only if val is out of alarm by at least hyst */
	pbigsub->lalm = val;
	return;
}

static void monitor(pbigsub)
    struct bigsubRecord	*pbigsub;
{
	unsigned short	monitor_mask;
	double		delta;
	double           *pnew;
	double           *pprev;
	int             i;

//printf("my: bigsubrecord monitor \n");

        monitor_mask = recGblResetAlarms(pbigsub);
        monitor_mask |= (DBE_LOG|DBE_VALUE);
        if(monitor_mask){
//printf("my: bigsubrecord monitor 0\n");
           db_post_events(pbigsub,&pbigsub->val,monitor_mask);
        }
        /* check for value change */
        delta = pbigsub->mlst - pbigsub->val;
        if(delta<0.0) delta = -delta;
        if (delta > pbigsub->mdel) {
                /* post events for value change */
                monitor_mask |= DBE_VALUE;
                /* update last value monitored */
                pbigsub->mlst = pbigsub->val;
        }
        /* check for archive change */
        delta = pbigsub->alst - pbigsub->val;
        if(delta<0.0) delta = -delta;
        if (delta > pbigsub->adel) {
                /* post events on value field for archive change */
                monitor_mask |= DBE_LOG;
                /* update last archive value monitored */
                pbigsub->alst = pbigsub->val;
        }
        /* send out monitors connected to the value field */
        if (monitor_mask){
//printf("my: bigsubrecord monitor 1\n");
                db_post_events(pbigsub,&pbigsub->val,monitor_mask);
        }
	/* check all input fields for changes*/
	for(i=0, pnew=&pbigsub->a, pprev=&pbigsub->la; i<IN_ARG_MAX; i++, pnew++, pprev++) {
		if(*pnew != *pprev) {
//printf("my: bigsubrecord monitor 2\n");
			db_post_events(pbigsub,pnew,monitor_mask|DBE_VALUE);
			*pprev = *pnew;
		}
	}
	/* check all non-link nla-nlt fields for changes */
	for(i=0, pnew=&pbigsub->nla, pprev=&pbigsub->lnla; i<IN_ARG_MAX; i++, pnew++, pprev++) {
		if(*pnew != *pprev) {
//printf("my: bigsubrecord monitor 3 \n");
			db_post_events(pbigsub,pnew,monitor_mask|DBE_VALUE);
			*pprev = *pnew;
		}
	}
        return;
}

static long fetch_values(pbigsub)
struct bigsubRecord *pbigsub;
{
        struct link     *plink; /* structure of the link field  */
        double           *pvalue;
        int             i;
	long		status;

        for(i=0, plink=&pbigsub->inpa, pvalue=&pbigsub->a; i<IN_ARG_MAX; i++, plink++, pvalue++) {
		status=dbGetLink(plink,DBR_DOUBLE,pvalue,0,0);
		if (!RTN_SUCCESS(status)) return(-1);
        }
        return(0);
}

static long push_values(pbigsub)
struct bigsubRecord *pbigsub;
{
        struct link     *plink; /* structure of the link field  */
        double           *pvalue;
        int             i;
	long		status;

        for(i=0, plink=&pbigsub->outa, pvalue=&pbigsub->oa; i<OUT_ARG_MAX; i++, plink++, pvalue++) {
		status=dbPutLink(plink,DBR_DOUBLE,pvalue,0);
		if (!RTN_SUCCESS(status)) return(-1);
        }
        return(0);
}

static long do_sub(pbigsub)
struct bigsubRecord *pbigsub;  /* pointer to subroutine record  */
{
	long	status;
	SUBFUNCPTR	psubroutine;


	/* call the subroutine */
	psubroutine = (SUBFUNCPTR)(pbigsub->sadr);
	if(psubroutine==NULL) {
               	recGblSetSevr(pbigsub,BAD_SUB_ALARM,INVALID_ALARM);
		return(0);
	}
	status = psubroutine(pbigsub);
	if(status < 0){
               	recGblSetSevr(pbigsub,SOFT_ALARM,pbigsub->brsv);
	} else pbigsub->udf = FALSE;
	return(status);
}
