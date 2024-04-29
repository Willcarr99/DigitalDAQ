/*********************************************************************

  Name:         v1730.c
  Created by:   Pierre-A. Amaudruz / K.Olchanski / R. Longland

  Contents:     V1730 16 ch. 14bit 500Msps
 
  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "v1730drv.h"
#include "mvmestd.h"


// Buffer organization map for number of samples
uint32_t V1730_NSAMPLES_MODE[10] = { (625<<10), (625<<9), (625<<8), (625<<7), (625<<6), (625<<5)
			       ,(625<<4), (625<<3), (625<<2), (625<<1)};

/*****************************************************************/
/*
Read V1730 register value
*/
static uint32_t regRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write V1730 register value
*/
static void regWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base + offset, value);
}

/*****************************************************************/
uint32_t v1730_RegisterRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  return regRead(mvme, base, offset);
}

/*****************************************************************/
void v1730_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  regWrite(mvme, base, offset, value);
}

/*****************************************************************/
void v1730_Reset(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1730_SW_RESET, 0);
}

/*****************************************************************/
void v1730_TrgCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t reg, uint32_t mask)
{
  regWrite(mvme, base, reg, mask);
}

/*****************************************************************/
void v1730_ChannelCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t reg, uint32_t mask)
{
  regWrite(mvme, base, reg, mask);
}

/*****************************************************************/
void v1730_ChannelSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what, uint32_t that)
{
  uint32_t reg, mask;

  if (what == V1730_CHANNEL_THRESHOLD)   mask = 0x3FFF;
  //  if (what == V1730_CHANNEL_OUTHRESHOLD) mask = 0x0FFF;
  if (what == V1730_CHANNEL_DAC)         mask = 0xFFFF;
  reg = what | (channel << 8);
  printf("base:0x%x reg:0x%x, this:%x\n", base, reg, that);
  regWrite(mvme, base, reg, (that & 0xFFF));
}

/*****************************************************************/
uint32_t v1730_ChannelGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what)
{
  uint32_t reg, mask, ret;

  if (what == V1730_CHANNEL_THRESHOLD)   mask = 0x3FFF;
  //  if (what == V1730_CHANNEL_OUTHRESHOLD) mask = 0x0FFF;
  if (what == V1730_CHANNEL_DAC)         mask = 0xFFFF;
  reg = what | (channel << 8);
  ret = regRead(mvme, base, reg);
  return ret & mask;
}

/*****************************************************************/
void v1730_ChannelThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold)
{
  uint32_t reg;
  reg = V1730_CHANNEL_THRESHOLD | (channel << 8);
  //  printf("base:0x%x reg:0x%x, threshold:%x\n", base, reg, threshold);
  regWrite(mvme, base, reg, (threshold & 0x3FFF));
}

/*****************************************************************/
/*
void v1730_ChannelOUThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold)
{
  uint32_t reg;
  reg = V1730_CHANNEL_OUTHRESHOLD | (channel << 8);
  printf("base:0x%x reg:0x%x, outhreshold:%x\n", base, reg, threshold);
  regWrite(mvme, base, reg, (threshold & 0xFFF));
}
*/
/*****************************************************************/
void v1730_ChannelDACSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t dac)
{
  uint32_t reg;

  if(dac < 0){
    printf("ERROR: DAC value is less than zero. Not setting!\n");
    return;
  }
  if(dac > 1<<14){
    printf("ERROR: DAC value is larger than maximum allowed. Not setting!\n");
    return;
  }

  printf("Setting offset = %d for channel %d\n",dac,channel);
  dac = (uint32_t)(16735-dac)/0.2589;
  printf("Corresponding DAC value is %d\n",dac);

  // check the status register first
  reg = V1730_CHANNEL_STATUS | (channel << 8);
  printf("Waiting for SPI Bus...");
  while(regRead(mvme, base, reg) & 0x4){
    printf(".");
    usleep(10000);
  }
  printf(" Done!\n");

  reg = V1730_CHANNEL_DAC | (channel << 8);
  printf("base:0x%x reg:0x%x, DAC:%x\n", base, reg, dac);
  regWrite(mvme, base, reg, (dac & 0xFFFF));

}

/*****************************************************************/
int v1730_ChannelDACGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t *dac)
{
  uint32_t reg;
  int   status;

  reg = V1730_CHANNEL_DAC | (channel << 8);
  *dac = regRead(mvme, base, reg);
  //  printf("base:0x%x reg:0x%x, DAC:%x\n", base, reg, *dac);
  reg = V1730_CHANNEL_STATUS | (channel << 8);
  status = regRead(mvme, base, reg);
  return status;
}

/*****************************************************************/
void v1730_Align64Set(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1730_VME_CONTROL, V1730_ALIGN64);
}

/*****************************************************************/
void v1730_AcqCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation)
{
  uint32_t reg;
  
  reg = regRead(mvme, base, V1730_ACQUISITION_CONTROL);  
  switch (operation) {
  case V1730_RUN_START:
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL, (reg | 0x4));
    break;
  case V1730_RUN_STOP:
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL, (reg & ~(0x4)));
    break;
  case V1730_REGISTER_RUN_MODE:
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL, (reg & ~(0x3)));
    break;
  case V1730_SIN_RUN_MODE:
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL, (reg | 0x01));
    break;
  case V1730_SIN_GATE_RUN_MODE:
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL, (reg | 0x02));
    break;
  case V1730_MULTI_BOARD_SYNC_MODE:
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL, (reg | 0x03));
    break;
  case V1730_COUNT_ACCEPTED_TRIGGER:
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL, (reg | 0x08));
    break;
  case V1730_COUNT_ALL_TRIGGER:
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL, (reg & ~(0x08)));
    break;
  default:
    break;
  }
}

/*****************************************************************/
void v1730_ChannelConfig(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation)
{
  uint32_t reg;
  
  regWrite(mvme, base, V1730_CHANNEL_CONFIG, 0x10);
  reg = regRead(mvme, base, V1730_CHANNEL_CONFIG);  
  printf("Channel_config1: 0x%x\n", regRead(mvme, base, V1730_CHANNEL_CONFIG));  
  switch (operation) {
  case V1730_TRIGGER_UNDERTH:
    regWrite(mvme, base, V1730_CHANNEL_CONFIG, (reg | 0x40));
    break;
  case V1730_TRIGGER_OVERTH:
    regWrite(mvme, base, V1730_CHANNEL_CONFIG, (reg & ~(0x40)));
    break;
  default:
    break;
  }
  printf("Channel_config2: 0x%x\n", regRead(mvme, base, V1730_CHANNEL_CONFIG));  
}

/**********************************************************************/
/* Run calibration on the channel (monitor through the status register) */
void v1730_ChannelCalibrate(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel)
{

  uint32_t reg;

  printf("Calibrating channel %d\n",channel);

  // check the status register first
  reg = V1730_CHANNEL_STATUS | (channel << 8);
  printf("Waiting for SPI Bus");
  while(regRead(mvme, base, reg) & 0x4){
    printf(".");
    usleep(10000);
  }
  printf("\n");

  // Then write to the calibration register
  reg = V1730_CHANNEL_CALIBRATE | (channel << 8);
  regWrite(mvme, base, reg, 0x1);

  // Monitor to see when if finishes
  printf("Calibrating...");
  reg = V1730_CHANNEL_STATUS | (channel << 8);
  while(!regRead(mvme, base, reg) & 0x8){
    printf(".");
    usleep(10000);
  }
  printf(" Done!\n");

}

/*****************************************************************/
void v1730_info(MVME_INTERFACE *mvme, uint32_t base, int *nchannels, uint32_t *n32word)
{
  int i, chanmask;

  printf("\n");
  printf("================================================\n");
  printf("V1730 Buffer Organisation\n");

  // Evaluate the event size
  // Number of samples per channels
  uint32_t bitblocks = regRead(mvme, base, V1730_BUFFER_ORGANIZATION);
  uint32_t nblocks = 1<<bitblocks;
  printf("Blocks per channel     : %d\n",nblocks);
  
  // Get either the custom samples number or the default for this
  // buffer organisation
  *n32word = regRead(mvme, base, V1730_CUSTOM_SIZE)*10;
  if(*n32word == 0)
    *n32word = V1730_NSAMPLES_MODE[bitblocks];

  printf("Samples per channel    : %d\n",*n32word);

  // times the number of active channels
  chanmask = 0xffff & regRead(mvme, base, V1730_CHANNEL_EN_MASK); 
  *nchannels = 0;
  for (i=0;i<16;i++) {
    if (chanmask & (1<<i))
      *nchannels += 1;
  }

  printf("Channels enabled       : %d\n",*nchannels);

  *n32word *= *nchannels;
  *n32word /= 2;   // 2 samples per 32bit word
  *n32word += 4;   // Headers

  printf("32-bit words per event : %d\n",*n32word);
  printf("kB per event           : %.3f\n",(float)*n32word*32.*0.125/1000);
  printf("================================================\n");

}

/**********************************************************************/
uint32_t v1730_SamplesGet(MVME_INTERFACE *mvme, uint32_t base)
{

  uint32_t bitblocks = regRead(mvme, base, V1730_BUFFER_ORGANIZATION);
  //  printf("Blocks per channel     : %d\n",nblocks);

  // Read the custom samples number
  uint32_t nsamples = regRead(mvme, base, V1730_CUSTOM_SIZE)*10;
  if(nsamples==0)
    nsamples = V1730_NSAMPLES_MODE[bitblocks];
  //  printf("Samples per channel    : %d\n",*n32word);

  return nsamples;
  
}
/**********************************************************************/
void v1730_PostTriggerSet(MVME_INTERFACE *mvme, uint32_t base, int post)
{
  // Page 17 in register documentation, number of samples after
  // trigger is (post trigger value)*2+NpostOffset
  // NOTE: The manual inccorectly states that the multiplicative
  //       factor should be 4.
  int NpostOffset = 144;
  regWrite(mvme, base, V1730_POST_TRIGGER_SETTING, (uint32_t)(post-NpostOffset)/8);

}

/*****************************************************************/
uint32_t v1730_BufferOccupancy(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel)
{
  uint32_t reg;
  reg = V1730_BUFFER_OCCUPANCY | (channel<<8);
  printf("WARNING: v1730_BufferOccupancy has been disabled because it crashes the V1730\n");
  return 0;
  //  return regRead(mvme, base, reg) & 0x7FF;
}


/*****************************************************************/
/*
uint32_t v1730_BufferFree(MVME_INTERFACE *mvme, uint32_t base, int nbuffer)
{
  int mode;

  mode = regRead(mvme, base, V1730_BUFFER_ORGANIZATION);
  if (nbuffer <= (1<< mode) ) {
    regWrite(mvme, base, V1730_BUFFER_FREE, nbuffer);
    return mode;
  } else
    return mode;
}
*/
/*****************************************************************/
/*
uint32_t v1730_BufferFreeRead(MVME_INTERFACE *mvme, uint32_t base)
{
  return regRead(mvme, base, V1730_BUFFER_FREE);
}
*/
/*****************************************************************/
uint32_t v1730_DataRead(MVME_INTERFACE *mvme, uint32_t base, uint32_t *pdata, uint32_t n32w)
{
  uint32_t i;

  for (i=0;i<n32w;i++) {
    *pdata = regRead(mvme, base, V1730_EVENT_READOUT_BUFFER);
    //printf ("pdata[%i]:%x\n", i, *pdata); 
//    if (*pdata != 0xffffffff)
    pdata++;
    //    else
      //      break;
  }
  return i;
}

/********************************************************************/
/** v1730_DataBlockRead
Read N entries (32bit) 
@param mvme vme structure
@param base  base address
@param pdest Destination pointer
@return nentry
*/
uint32_t v1730_DataBlockRead(MVME_INTERFACE *mvme, uint32_t base, uint32_t *pbuf32,
			     uint32_t nwords32)
{

  int to_read32, status;
  uint32_t w;
  //  printf("--------------------- DataBlockRead() nwords32:%d\n", nwords32);
  if (nwords32 < 100) { // PIO
    for (uint32_t i=0; i<nwords32; i++) {
      w = regRead(mvme, base, 0);
      //printf("word %d: 0x%08x\n", i, w);                                                    
      *pbuf32++ = w;
    }
  } else {
    mvme_set_dmode(mvme, MVME_DMODE_D64);
    //mvme_set_blt(mvme, MVME_BLT_BLT32);                                                     
    //mvme_set_blt(mvme, MVME_BLT_MBLT64);                                                    
    //mvme_set_blt(mvme, MVME_BLT_2EVME);                                                     
    mvme_set_blt(mvme, MVME_BLT_2ESST);

    // Read just a few more bytes from the V1730 - why? I don't know! But it works!!!
    //nwords32+=8;

    while (nwords32>0) {			
      // Get the number of words left to read
      to_read32 = nwords32;
      //      printf("1 to read = %d\n",to_read32);
      // round this number to the nearest 4 bytes
      to_read32 &= ~0x3;
      //      printf("2 to read = %d\n",to_read32);

      // Now calculate the number of bytes to read (either 4080 or less)
      if (to_read32*4 >= 0xFF0)
        to_read32 = 0xFF0/4;
      //    if (to_read32 <= 0)
      //      break;
      //      printf("3 to read = %d\n",to_read32);
      
      //      printf("going to read: read %d, total %d\n", to_read32*4, nwords32*4);                
      status=mvme_read(mvme, pbuf32, base, to_read32*4);
      //      printf("block read %d, status %d, total %d\n", to_read32*4, status, nwords32);
      nwords32 -= to_read32;
      pbuf32 += to_read32;
    }

    //    printf("reading remaining %d words\n",nwords32);
    while (nwords32) {
      *pbuf32 = regRead(mvme, base, 0);
      pbuf32++;
      nwords32--;
    }
  }
  return nwords32;
}


/*****************************************************************/
void  v1730_Status(MVME_INTERFACE *mvme, uint32_t base)
{
  printf("================================================\n");
  printf("V1730 at A32 0x%x\n", (int)base);
  printf("Board ID             : 0x%x\n", regRead(mvme, base, V1730_BOARD_ID));
  printf("Board Info           : 0x%x\n", regRead(mvme, base, V1730_BOARD_INFO));
  printf("Acquisition status   : 0x%8.8x\n", regRead(mvme, base, V1730_ACQUISITION_STATUS));
  printf("================================================\n");
}

/*****************************************************************/
/**
Sets all the necessary paramters for a given configuration.
The configuration is provided by the mode argument.
Add your own configuration in the case statement. Let me know
your setting if you want to include it in the distribution.
- <b>Mode 1</b> : 

@param *mvme VME structure
@param  base Module base address
@param mode  Configuration mode number
@return 0: OK. -1: Bad
*/
int  v1730_Setup(MVME_INTERFACE *mvme, uint32_t base, int mode)
{
  switch (mode) {
  case 0x0:
    printf("--------------------------------------------\n");
    printf("Setup Skip\n");
    printf("--------------------------------------------\n");
  case 0x1:
    printf("--------------------------------------------\n");
    printf("Trigger from LEMO, 4ch, 500Ms, 32 blocks    \n");
    printf("--------------------------------------------\n");
    regWrite(mvme, base, V1730_BUFFER_ORGANIZATION,  0x05);    // 32 20K buffers
    regWrite(mvme, base, V1730_TRIG_SRCE_EN_MASK,    0x40000000);  // External Trigger
    regWrite(mvme, base, V1730_CHANNEL_EN_MASK,      0xF);     // Enable 1st 4 channels
    //    regWrite(mvme, base, V1730_POST_TRIGGER_SETTING, 800);     // PreTrigger (1K-800)
    regWrite(mvme, base, V1730_ACQUISITION_CONTROL,   0x00);   // Reset Acq Control
    printf("\n");
    break;
  case 0x2:
    printf("--------------------------------------------\n");
    printf("Trigger from LEMO\n");
    printf("--------------------------------------------\n");
    regWrite(mvme, base, V1730_BUFFER_ORGANIZATION, 1);
    printf("\n");
    break;
  default:
    printf("Unknown setup mode\n");
    return -1;
  }
  v1730_Status(mvme, base);
  return 0;
}

/**********************************************************************/
/* Setup the BLT transfers */
void v1730_SetupBLT(MVME_INTERFACE *mvme, uint32_t base){

  int numBLT=1;  // number of events to transfer with each BLT

  printf("Setting block transfer events to %d\n",numBLT);
  regWrite(mvme, base, V1730_BLT_EVENT_NB, numBLT);
  //  v1730_Align64Set(mvme, base);

  return;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {

  uint32_t V1730_BASE  = 0x32100000;

  MVME_INTERFACE *myvme;

  uint32_t data[100000], n32word, n32read;
  int status, channel, i, j, nchannels=0, chanmask;

  if (argc>1) {
    sscanf(argv[1],"%x", &V1730_BASE);
  }

  // Test under vmic
  status = mvme_open(&myvme, 0);

  v1730_Setup(myvme, V1730_BASE, 1);
  // Run control by register
  v1730_AcqCtl(myvme, V1730_BASE, V1730_REGISTER_RUN_MODE);
  // Soft or External trigger
  v1730_TrgCtl(myvme, V1730_BASE, V1730_TRIG_SRCE_EN_MASK     , V1730_SOFT_TRIGGER|V1730_EXTERNAL_TRIGGER);
  // Soft and External trigger output
  v1730_TrgCtl(myvme, V1730_BASE, V1730_FP_TRIGGER_OUT_EN_MASK, V1730_SOFT_TRIGGER|V1730_EXTERNAL_TRIGGER);
  // Enabled channels
  v1730_ChannelCtl(myvme, V1730_BASE, V1730_CHANNEL_EN_MASK, 0x3);
  //  sleep(1);

  channel = 0;
  // Start run then wait for trigger
  v1730_AcqCtl(myvme, V1730_BASE, V1730_RUN_START);
  sleep(1);

  //  regWrite(myvme, V1730_BASE, V1730_SW_TRIGGER, 1);

  // Evaluate the event size
  // Number of samples per channels
  n32word  =  1<<regRead(myvme, V1730_BASE, V1730_BUFFER_ORGANIZATION);
  // times the number of active channels
  chanmask = 0xff & regRead(myvme, V1730_BASE, V1730_CHANNEL_EN_MASK); 
  for (i=0;i<8;i++) 
    if (chanmask & (1<<i))
      nchannels++;
  printf("chanmask : %x , nchannels:  %d\n", chanmask, nchannels);
  n32word *= nchannels;
  n32word /= 2;   // 2 samples per 32bit word
  n32word += 4;   // Headers
  printf("n32word:%d\n", n32word);

  printf("Occupancy for channel %d = %d\n", channel, v1730_BufferOccupancy(myvme, V1730_BASE, channel)); 

  do {
    status = regRead(myvme, V1730_BASE, V1730_ACQUISITION_STATUS);
    printf("Acq Status1:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  n32read = v1730_DataRead(myvme, V1730_BASE, &(data[0]), n32word);
  printf("n32read:%d\n", n32read);
  
  for (i=0; i<n32read;i+=4) {
    printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  
  do {
    status = regRead(myvme, V1730_BASE, V1730_ACQUISITION_STATUS);
    printf("Acq Status2:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  n32read = v1730_DataRead(myvme, V1730_BASE, &(data[0]), n32word);
  printf("n32read:%d\n", n32read);
  
  for (i=0; i<n32read;i+=4) {
    printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  }
  
  
  //  v1730_AcqCtl(myvme, V1730_BASE, RUN_STOP);
  
  regRead(myvme, V1730_BASE, V1730_ACQUISITION_CONTROL);
  status = mvme_close(myvme);

  return 1;

}
#endif

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

//end
