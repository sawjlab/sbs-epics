#include "jvme.h"

#include "define.h"
#include "extern.h"

void V65XX_Init(unsigned int nboards, uint32_t base, uint32_t delta)
{
  unsigned int i;
  int res;
  uintptr_t laddr;
  
  if(vmeOpenDefaultWindows()!=OK) {
    printf("ERROR opening default VME windows\n");
    vmeCloseDefaultWindows();
    return;
  }

  g_boardCnt = nboards;

  if(base) {
    g_vme_base = base;
  }
  if(delta) {
    g_vme_delta = delta;
  }

  for(i=0;i<nboards;i++) {
    res = vmeBusToLocalAdrs(0x39, (char *) (g_vme_base+i*(uint32_t)g_vme_delta), (char **)&laddr);
    
    /* Need error checking here */
    g_laddr[i] = laddr;
  }
  // Populate the table that maps commands to memory locations

  g_cmd2mem[S_CE] = 0;
  g_cmd2mem[S_DV] = VHV_VSET;
  g_cmd2mem[S_RDN] = VHV_RDOWN;
  g_cmd2mem[S_RUP] = VHV_RUP;
  g_cmd2mem[S_TC] = VHV_ISET;
  g_cmd2mem[S_MVDZ] = 0;
  g_cmd2mem[S_MCDZ] = 0;
  g_cmd2mem[S_HV] = 0;
  //  g_cmd2mem[S_SOT] = 0;
  g_cmd2mem[S_PWDOWN] = VHV_PWDOWN;
  g_cmd2mem[S_PRD] = 0;
  g_cmd2mem[S_CHHV] = VHV_PWUP;
  g_cmd2mem[S_BDHV] = 0;
  g_cmd2mem[S_VMAX] = VHV_SVMAX;
  g_cmd2mem[G_STAT] = VHV_CHSTATUS;
  g_cmd2mem[G_VMON] = VHV_VMON;
  g_cmd2mem[G_IMON] = VHV_IMON;
  g_cmd2mem[G_Valid] = 0;
  g_cmd2mem[G_Alarm] = 0;

  printf("\n");
  for(i=0;i<g_boardCnt;i++) {
    V65XX_PrintBoardInfo(i);
  }
  printf("\n");

}  
    
void V65XX_PrintBoardInfo(unsigned int iboard) {
  char str[2*VHV_DESCR_LENGTH+10];
  int i;
  uint16_t *address;
  uint16_t data;

  vmeBusLock();

  address = g_laddr[iboard]+VHV_DESCR;

  for(i=0;i<VHV_DESCR_LENGTH;i++) {
    data = vmeRead16(address+i);
    str[2*i] = data & 0xff;
    str[2*i+1] = (data>>8)&0xff;
  }
  str[2*VHV_DESCR_LENGTH] = '\0';

  printf("VX65XX Board %d: %s  ",iboard,str);

  address = g_laddr[iboard]+VHV_MODEL;
  for(i=0;i<VHV_MODEL_LENGTH;i++) {
    data = vmeRead16(address+i);
    str[2*i] = data & 0xff;
    str[2*i+1] = (data>>8)&0xff;
  }
  str[2*VHV_MODEL_LENGTH] = '\0';

  printf("Model: %s  ",str);

  address = g_laddr[iboard]+VHV_SERNUM;
  data = vmeRead16(address);
  printf("SN: %d  ",data);

  address = g_laddr[iboard]+VHV_GFWREL;
  data = vmeRead16(address);
  printf("Firmware: %d.%d  ",(data>>8)&0xff,data&0xff);

  address = g_laddr[iboard]+VHV_CHNUM;
  data = vmeRead16(address);
  printf("Channels: %d\n",data);

  vmeBusUnlock();
  
}
