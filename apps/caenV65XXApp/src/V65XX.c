// Routines for reading and writing to V65XX modules

#include "define.h"
#include "extern.h"
#include "jvme.h"

int V65XX_Set(int command, int iboard, int ichan, int value, int *readback)
{
  // Check that iboard and ichan are in range
  // Eventually print error
  uint16_t data;
  uint16_t* address;
  uint16_t read;
  uint32_t offset;

  if(command < 0 || command >=NSETCMD) {
    return ERROR;
  }
  offset = g_cmd2mem[command];
  if(offset == 0) {
    return ERROR;
  }
  if(iboard >=0 && iboard < g_boardCnt &&
     ichan >=0 && ichan < VHV_NCHAN) {
    data = value;
    address = (unsigned short int *)g_laddr[iboard] + offset + ichan*VHV_CHSTEP;
    vmeBusLock();
    vmeWrite16(address,data);
    read = vmeRead16(address);
    vmeBusUnlock();
    *readback = read;
  } else {
    return ERROR;
  }
  return OK;
}
int V65XX_Get(int command, int iboard, int ichan, int *value)
{
  uint16_t read;
  uint16_t* address;
  uint32_t offset;
  
  if(command < 0 || command >= NCMD) {
    return ERROR;
  }
  offset = g_cmd2mem[command];
  if(offset == 0) {
    return ERROR;
  }

  if(iboard >=0 && iboard < g_boardCnt &&
     ichan >=0 && ichan < VHV_NCHAN) {
    address = g_laddr[iboard] + offset + ichan*VHV_CHSTEP;    
    vmeBusLock();
    read = vmeRead16(address);
    vmeBusUnlock();
    *value = read;
  } else {
    return ERROR;
  }
  return OK;
}
