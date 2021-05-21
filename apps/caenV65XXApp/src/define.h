#ifndef DEFINE_H
#define DEFINE_H

/*****************************************************************************\a
 * File: define.h                                   Author: Stephen A. Wood  *
 *                                                                           *
 * Overview:                                                                 *
 *   This header file contains symbolic definitions and user data types for  *
 * the CAEN V65XX device driver.                                             *
 *                                                                           *
 * Revision History:                                                         3 *   05/13/2021 - Initial release                                            *
\*****************************************************************************/

#define VERSION "0-9.0"                /* Driver version number               */
#define MAX_SLOTS 21                  /* Maximum slots allowed               */

// Command codes used in INP and OUT fields

#define NCMD 19
#define NSETCMD 13

#define S_CE      0    /* Set enable/disable             */
#define S_DV      1    /* Set demand voltage             */
#define S_RDN     2    /* Set ramp down                  */
#define S_RUP     3    /* Set ramp up                    */
#define S_TC      4    /* Set trip current               */
#define S_MVDZ    5    /* Set measured voltage dead-zone */
#define S_MCDZ    6    /* Set measured current dead-zone */
#define S_HV      7    /* Set HV on/off                  */
//#define S_SOT     8    /* Set samples over threshold     */
#define S_PWDOWN  8    /* Ramp or Kill on power off      */
#define S_PRD     9    /* Set post ramp delay            */
#define S_CHHV    10   /* Set Channel HV on/off          */
#define S_BDHV    11   /* Set board HV on/of             */
#define S_VMAX    12   /* Set Max channel voltage        */
#define G_STAT    13   /* Get Channel Status             */
#define G_VMON    14   /* Get Measured Voltage           */
#define G_IMON    15   /* Get Measured Current           */
#define G_Valid   16   /* Get HV validity                */
#define G_HV      17   /* Get HV on/off                  */
#define G_Alarm   18   /* Get Chassis alarm status       */

// Memory offsets on V65XX card

#define VHV_VMAX 0x0050/2
#define VHV_IMAX 0x0054/2
#define VHV_STATUS 0x0058/2
#define VHV_FWREL 0x005C/2

#define VHV_VSET 0x0080/2
#define VHV_ISET 0x0084/2
#define VHV_VMON 0x0088/2
#define VHV_IMON 0x008C/2
#define VHV_PWUP 0x0090/2
#define VHV_CHSTATUS 0x0094/2
#define VHV_TRIP 0x0098/2
#define VHV_SVMAX 0x009C/2
#define VHV_RDOWN 0x00A0/2
#define VHV_RUP   0x00A4/2
#define VHV_PWDOWN 0x00A8/2
#define VHV_POLARITY 0x00AC/2
#define VHV_TEMP 0x00B0/2

#define VHV_CHSTEP 0x0080/2

#define VHV_CHNUM 0x8100/2
#define VHV_DESCR 0x8102/2
#define VHV_DESCR_LENGTH 10
#define VHV_MODEL 0x8116/2
#define VHV_MODEL_LENGTH 4
#define VHV_SERNUM 0x811E/2
#define VHV_GFWREL 0x8120/2

#define VHV_NCHAN 6

#define OK 0
#define ERROR (-1)

int V65XX_Get(int command, int iboard, int ichan, int *value);
int V65XX_Set(int command, int iboard, int ichan, int value, int *readback);

#endif
