
/* sy1527epics1.h */

#define NUM_CAEN_PROP 11

#define PROP_MC   0    /* measured current */
#define PROP_MV   1    /* measured voltage */
#define PROP_DV   2    /* demanded voltage */
#define PROP_RUP  3    /*  */
#define PROP_RDN  4    /*  */
#define PROP_TC   5    /*  */
#define PROP_CE   6    /*  */
#define PROP_ST   7    /*  */
//#define PROP_MVDZ 8    /*  */
//#define PROP_MCDZ 9    /*  */
#define PROP_HVL  10   /*  */
#define PROP_TT   11    /* trip time */

// Low Voltage Modules:
#define PROP_RUPT   3 // ramp up time (reusing RUP)
#define PROP_RDNT   4 // ramp down time (reusing RDN)
#define PROP_UNVT   8 // under voltage threshold (reusing MVDZ)
#define PROP_OVVT   9 // over voltage threshold (reusing MCDZ)
#define PROP_TEMP  11 // temperature
#define PROP_INTLK 12 // interlock
#define PROP_VCON  13 // connector voltage

// Dual-range modules:
#define PROP_RANGE  14  // current range

#define PROP_HBEAT  15   /*  */  /// my_n:

#define STATUS int

/* function prototypes */
int CAEN_HVinit();
int CAEN_HVstart(unsigned id, char *ip_address);
int CAEN_HVstop(unsigned id);
int CAEN_GetHv(unsigned id, int *onoff); ///my:
int CAEN_GetAlarm(unsigned id);
int CAEN_GetValidity(unsigned id);
int CAEN_SetHV(unsigned id, unsigned char on_off);

STATUS CAEN_HVload(unsigned id, unsigned slot, unsigned channel,
                   char *property, float value);
STATUS CAEN_GetProperty(unsigned id, unsigned slot, unsigned channel,
                        char *property, float *value);
STATUS CAEN_GetChannel(unsigned id, unsigned slot, unsigned channel,
                       double *property, double *delta);
