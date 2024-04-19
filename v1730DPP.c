/*********************************************************************

  Name:         v1730DPP.c
  Created by:   Pierre-A. Amaudruz / K.Olchanski / R. Longland / W. Fox

  Contents:     V1730 16 ch. 14bit 500Msps
                Using DPP firmware
 
  $Id$
*********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include "v1730DPPdrv.h"
#include "mvmestd.h"

// Buffer organization map for number of samples
uint32_t V1730DPP_NSAMPLES_MODE[10] = { (625<<10), (625<<9), (625<<8), (625<<7), (625<<6), (625<<5)
			       ,(625<<4), (625<<3), (625<<2), (625<<1)};

/*****************************************************************/
/*
Read V1730DPP register value
*/
static uint32_t regRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  return mvme_read_value(mvme, base + offset);
}

/*****************************************************************/
/*
Write V1730DPP register value
*/
static void regWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  mvme_set_am(mvme, MVME_AM_A32);
  mvme_set_dmode(mvme, MVME_DMODE_D32);
  mvme_write_value(mvme, base + offset, value);
}

/*****************************************************************/
uint32_t v1730DPP_RegisterRead(MVME_INTERFACE *mvme, uint32_t base, int offset)
{
  return regRead(mvme, base, offset);
}

/*****************************************************************/
void v1730DPP_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value)
{
  regWrite(mvme, base, offset, value);
}

/*****************************************************************/
void v1730DPP_setAggregateOrg(MVME_INTERFACE *mvme, uint32_t base, int code)
{
  if((code < 0) | (code > 10)){
    printf("ERROR: Aggregate code must be between 0 and 10!\n");
    return;
  }
  uint32_t c = code & 0xF;
  regWrite(mvme, base, V1730DPP_AGGREGATE_ORGANIZATION, c);

}

/*****************************************************************/
void v1730DPP_setAggregateSizeG(MVME_INTERFACE *mvme, uint32_t base, int nevents)
{
  if(nevents > 1023){
    printf("ERROR: nevents must be smaller than 1024!\n");
    return;
  }
      
  int N = nevents & 0x3FF;
  regWrite(mvme, base, V1730DPP_EVENTS_PER_AGGREGATE_G, N);
}

/*****************************************************************/
void v1730DPP_setAggregateSize(MVME_INTERFACE *mvme, uint32_t base, int nevents, int channel)
{
  if(nevents > 1023){
    printf("ERROR: nevents must be smaller than 1024!\n");
    return;
  }
      
  int N = nevents & 0x3FF;
  // Channel mask
  uint32_t reg = V1730DPP_EVENTS_PER_AGGREGATE | (channel << 8);
    
  regWrite(mvme, base, reg, N);
}

/*****************************************************************/
void v1730DPP_setRecordLengthG(MVME_INTERFACE *mvme, uint32_t base, int nsamples)
{
  if(nsamples % 8 != 0){
    printf("ERROR: nsamples must be divisible by 8!\n");
    return;
  }
    
  int N = nsamples/8;
  // Mask it
  N = N & 0x3FFF;
  regWrite(mvme, base, V1730DPP_RECORD_LENGTH_G, N);
}

/*****************************************************************/
void v1730DPP_setRecordLength(MVME_INTERFACE *mvme, uint32_t base, int nsamples, int channel)
{
  if(nsamples % 8 != 0){
    printf("ERROR: nsamples must be divisible by 8!\n");
    return;
  }
    
  int N = nsamples/8;
  // Mask it
  N = N & 0x3FFF;

  // Channel mask
  uint32_t reg = V1730DPP_RECORD_LENGTH | (channel << 8);
  regWrite(mvme, base, reg, N);
}

/*****************************************************************/
void v1730DPP_setDynamicRangeG(MVME_INTERFACE *mvme, uint32_t base, int i)
{
  if(i > 1){
    printf("ERROR: i must be 0 (2 Vpp) or 1 (0.5 Vpp)!\n");
    return;
  }
    
  regWrite(mvme, base, V1730DPP_DYNAMIC_RANGE_G, i);
}
/*****************************************************************/
void v1730DPP_setDynamicRange(MVME_INTERFACE *mvme, uint32_t base, int i, int channel)
{
  if(i > 1){
    printf("ERROR: i must be 0 (2 Vpp) or 1 (0.5 Vpp)!\n");
    return;
  }

  // Channel mask
  uint32_t reg = V1730DPP_DYNAMIC_RANGE | (channel << 8);
    
  regWrite(mvme, base, reg, i);
}
/*****************************************************************/
void v1730DPP_setCFDG(MVME_INTERFACE *mvme, uint32_t base, uint32_t delay, uint32_t percent)
{
  if(delay > 255*2){
    printf("ERROR: delay must be less than 510 ns\n");
    return;
  }
  if(percent > 100){
    printf("ERROR: percent can't be larger than 100, silly!\n");
    return;
  }

  uint32_t d = (delay/2) & 0xFF;
  
  uint32_t p=0;
  if(percent>37)p=1;
  if(percent>62)p=2;
  if(percent>87)p=3;

  // Put it all together (ignore interpolation options for now)
  uint32_t value = d | (p << 8);

  regWrite(mvme, base, V1730DPP_CFD_G, value);
}
/*****************************************************************/
void v1730DPP_setCFD(MVME_INTERFACE *mvme, uint32_t base, uint32_t delay, uint32_t percent, int channel)
{
  if(delay > 255*2){
    printf("ERROR: delay must be less than 510 ns\n");
    return;
  }
  if(percent > 100){
    printf("ERROR: percent can't be larger than 100, silly!\n");
    return;
  }

  uint32_t d = (delay/2) & 0xFF;
  
  uint32_t p=0;
  if(percent>37)p=1;
  if(percent>62)p=2;
  if(percent>87)p=3;

  // Put it all together (ignore interpolation options for now)
  uint32_t value = d | (p << 8);

  // Channel mask
  uint32_t reg = V1730DPP_CFD | (channel << 8);
    
  regWrite(mvme, base, reg, value);
}


/*****************************************************************/
void v1730DPP_setShortGateG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width)
{
  if(width > 8190){
    printf("ERROR: Short gate width must be less than 8190 ns\n");
    return;
  }

  uint32_t w = (width/2) & 0xFFF;

  regWrite(mvme, base, V1730DPP_SHORT_GATE_WIDTH_G, w);
}
/*****************************************************************/
void v1730DPP_setShortGate(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel)
{
  if(width > 8190){
    printf("ERROR: Short gate width must be less than 8190 ns\n");
    return;
  }

  uint32_t w = (width/2) & 0xFFF;

  // Channel mask
  uint32_t reg = V1730DPP_SHORT_GATE_WIDTH | (channel << 8);
    
  regWrite(mvme, base, reg, w);
}

/*****************************************************************/
void v1730DPP_setLongGateG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width)
{
  if(width > 131070){
    printf("ERROR: Long gate width must be less than 131 us\n");
    return;
  }

  uint32_t w = (width/2) & 0xFFFF;

  regWrite(mvme, base, V1730DPP_LONG_GATE_WIDTH_G, w);
}
/*****************************************************************/
void v1730DPP_setLongGate(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel)
{
  if(width > 131070){
    printf("ERROR: Long gate width must be less than 131 us\n");
    return;
  }

  uint32_t w = (width/2) & 0xFFFF;

  // Channel mask
  uint32_t reg = V1730DPP_LONG_GATE_WIDTH | (channel << 8);
    
  regWrite(mvme, base, reg, w);
}

/*****************************************************************/
void v1730DPP_setGateOffsetG(MVME_INTERFACE *mvme, uint32_t base, uint32_t offset)
{
  if(offset > 510000){
    printf("ERROR: Gate offset width must be less than 510 us\n");
    return;
  }

  uint32_t o = (offset/2) & 0xFF;

  regWrite(mvme, base, V1730DPP_GATE_OFFSET_G, o);
}
/*****************************************************************/
void v1730DPP_setGateOffset(MVME_INTERFACE *mvme, uint32_t base, uint32_t offset, int channel)
{
  if(offset > 510000){
    printf("ERROR: Gate offset width must be less than 510 us\n");
    return;
  }

  uint32_t o = (offset/2) & 0xFF;

  // Channel mask
  uint32_t reg = V1730DPP_GATE_OFFSET | (channel << 8);
    
  regWrite(mvme, base, reg, o);
}

/*****************************************************************/
void v1730DPP_setDCOffsetG(MVME_INTERFACE *mvme, uint32_t base, uint32_t offset)
{
  // Check that bit[2] of 0x1088 (Channen n Status) is 0 first.
  // Write DC Offset value to be half of DAQ LSB unit max (0xFFFF/2 = 0x8000 = 32768).

  while(((regRead(mvme, base, V1730DPP_CHANNELN_STATUS) & 0x4)>>2) == 1){
    printf(".");
    sleep(1);
  };
  printf("\nReady to set DC offset!\n");

  uint32_t value = regRead(mvme, base, V1730DPP_DC_OFFSET);
  //printf("DC Offset Buffer: %u\n", value);

  value = (value & 0xFFFF0000); // bits 16-31 unchanged. bits 15-0 set to 0 temporarily.
  value = (value | offset);
  //printf("DC Offset Buffer: %u\n", value);

  regWrite(mvme, base, V1730DPP_DC_OFFSET_G, value);

  printf("Waiting a few seconds for DC offset value to stablize...\n");
  for (int i=0; i<4; i++){
    sleep(1);
    //printf("...\n");
  }
}
/*****************************************************************/
void v1730DPP_setDCOffset(MVME_INTERFACE *mvme, uint32_t base, uint32_t offset, int channel)
{
  // Check that bit[2] of 0x1088 (Channen n Status) is 0 first.
  // Write DC Offset value to be half of DAQ LSB unit max (0xFFFF/2 = 0x8000 = 32768).

  while(((regRead(mvme, base, V1730DPP_CHANNELN_STATUS) & 0x4)>>2) == 1){
    printf(".");
    sleep(1);
  };
  printf("\nDone!\n");

  uint32_t reg = V1730DPP_DC_OFFSET | (channel << 8);
  
  uint32_t value = regRead(mvme, base, reg);

  value = (value & 0xFFFF0000); // bits 16-31 unchanged. bits 15-0 set to 0 temporarily.
  value = (value | offset);

  regWrite(mvme, base, reg, value);

  printf("Waiting a few seconds for DC offset to stablize...\n");
  for (int i=0; i<4; i++){
    sleep(1);
    //printf("...\n");
  }
}
/*****************************************************************/
void v1730DPP_setMeanBaselineCalcG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 4) | (mode < 0)){
    printf("ERROR: Baseline calculation settings have 5 options, 0-4.\n");
    return;
  }

  uint32_t bin;
  switch(mode){
  case 0:
    bin = 0x0; break; // Fixed baseline written to register 0x1064
  case 1:
    bin = 0x1; break; //nsamples = 16
  case 2:
    bin = 0x2; break; //nsamples = 64
  case 3:
    bin = 0x3; break; //nsamples = 256
  case 4:
    bin = 0x4; break; //nsamples = 1024
  }

  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  //printf("DPP1 Before Baseline: %u\n", value);
  value = value | (bin << 20);
  //printf("DPP1 After Baseline: %u\n", value);
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setMeanBaselineCalc(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 4) | (mode < 0)){
    printf("ERROR: Baseline calculation settings have 5 options, 0-4.\n");
    return;
  }

  uint32_t bin;
  switch(mode){
  case 0:
    bin = 0x0; break; // Fixed baseline written to register 0x1064
  case 1:
    bin = 0x1; break; //nsamples = 16
  case 2:
    bin = 0x2; break; //nsamples = 64
  case 3:
    bin = 0x3; break; //nsamples = 256
  case 4:
    bin = 0x4; break; //nsamples = 1024
  }

  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);
  
  uint32_t value = regRead(mvme, base, reg);
  value = value | (bin << 20);
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setFixedBaselineG(MVME_INTERFACE *mvme, uint32_t base, uint32_t baseline)
{
  uint32_t fb = 1000*baseline/120;
  uint32_t vRange = regRead(mvme, base, V1730DPP_DYNAMIC_RANGE);
  if (vRange == 1)fb = 1000*baseline/30;
  
  if (fb > 16382){
    printf("ERROR: Fixed Baseline is too high!\n");
    return;
  }
    
  switch(baseline){
  case 0:
    printf("Fixed baseline disabled for all channels\n"); break;
  default:
    printf("Fixed baseline enabled for all channels\n");
  }
  
  fb = fb & 0x3FFF;
  
  regWrite(mvme, base, V1730DPP_FIXED_BASELINE_G, fb);
}
/*****************************************************************/
void v1730DPP_setFixedBaseline(MVME_INTERFACE *mvme, uint32_t base, uint32_t baseline, int channel)
{    
  uint32_t fb = 1000*baseline/120;
  uint32_t vRange = regRead(mvme, base, (V1730DPP_DYNAMIC_RANGE | channel << 8));
  if (vRange == 1)fb = 1000*baseline/30;
  
  if (fb > 16382){
    printf("ERROR: Fixed Baseline is too high!\n");
    return;
  }
    
  switch(baseline){
  case 0:
    printf("Fixed baseline disabled for channel %d\n", channel); break;
  default:
    printf("Fixed baseline enabled for channel %d\n", channel);
  }
  
  fb = fb & 0x3FFF;
  
  // Channel mask
  uint32_t reg = V1730DPP_FIXED_BASELINE | (channel << 8);
  
  regWrite(mvme, base, reg, fb);
}
/*****************************************************************/
void v1730DPP_setThresholdG(MVME_INTERFACE *mvme, uint32_t base, uint32_t threshold)
{
  // Convert the threshold, in mV, into an LSB value depending on the
  // dynamic range of the channel
  uint32_t t = 1000*threshold/120;
  uint32_t vRange = regRead(mvme, base, V1730DPP_DYNAMIC_RANGE);
  if(vRange == 1)t=1000*threshold/30;
  
  if(t > 16382){
    printf("ERROR: Threshold is too high!\n");
    return;
  }

  t = t & 0x3FFF;

  regWrite(mvme, base, V1730DPP_TRIGGER_THRESHOLD_G, t);
}
/*****************************************************************/
void v1730DPP_setChargePedestalG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
      case 0:
          printf("Charge pedestal disabled for all channels\n"); break;
      case 1:
          printf("Charge pedestal enabled for all channels: a fixed value of 1024 is added to the charge\n"); break;
  }
    
  // Read the current algorithm control register, then add this new charge pedestal setting
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 4;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setChargePedestal(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
    return;
  }
  switch(mode){
    case 0:
      printf("Charge pedestal for channel %d disabled\n",channel); break;
    case 1:
      printf("Charge pedestal for channel %d enabled: a fixed value of 1024 is added to the charge\n",channel); break;
  }

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new charge pedestal setting
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 4;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setThreshold(MVME_INTERFACE *mvme, uint32_t base, uint32_t threshold, int channel)
{
  // Convert the threshold, in mV, into an LSB value depending on the
  // dynamic range of the channel
  uint32_t t = 1000*threshold/120;
  uint32_t vRange = regRead(mvme, base, (V1730DPP_DYNAMIC_RANGE | channel << 8));
  if(vRange == 1)t=1000*threshold/30;
  
  if(t > 16382){
    printf("ERROR: Threshold is too high!\n");
    return;
  }

  t = t & 0x3FFF;

  // Channel mask
  uint32_t reg = V1730DPP_TRIGGER_THRESHOLD | (channel << 8);

  regWrite(mvme, base, reg, t);
}
/*****************************************************************/
void v1730DPP_setChargeZeroSuppressionThresholdG(MVME_INTERFACE *mvme, uint32_t base, uint32_t qthresh)
{
  if(qthresh > 32675){
    printf("ERROR: Charge Threshold is too high!\n");
    return;
  }
  
  switch(qthresh){
    case 0:
        printf("Disabled charge zero suppression threshold for all channels\n"); break;
    default:
        printf("Enabled charge zero suppression threshold for all channels\n");
        uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
        value = value | 1 << 25;
        regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
  }
  
  qthresh = qthresh & 0xFFFF;
  
  regWrite(mvme, base, V1730DPP_CHARGE_THRESHOLD_G, qthresh);
}
/*****************************************************************/
void v1730DPP_setChargeZeroSuppressionThreshold(MVME_INTERFACE *mvme, uint32_t base, uint32_t qthresh, int channel)
{
  if(qthresh > 32675){
    printf("ERROR: Charge Threshold is too high!\n");
    return;
  }
  
  switch(qthresh){
    case 0:
        printf("Disabled charge zero suppression threshold for channel %d\n", channel); break;
    default:
        printf("Enabled charge zero suppression threshold for channel %d\n", channel);
        // Channel mask
        uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);
        // Read the current algorithm control register, then enable charge zero suppresion threshold
        uint32_t value = regRead(mvme, base, reg);
        value = value | 1 << 25;
        regWrite(mvme, base, reg, value);
  }
  
  qthresh = qthresh & 0xFFFF;
  
  // Channel mask
  uint32_t reg = V1730DPP_CHARGE_THRESHOLD | (channel << 8);
  
  regWrite(mvme, base, reg, qthresh);
}
/*****************************************************************/
void v1730DPP_setTriggerHoldoffG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width)
{
  if(width > 32767){
    printf("ERROR: Trigger Holdoff must be less than 32767 ns\n");
    return;
  }
  if(width % 8 != 0){
    printf("ERROR: Trigger holdoff must be a multiple of 8 ns\n");
    return;
  }

  uint32_t w = (width/8) & 0x7FFF;
    
  regWrite(mvme, base, V1730DPP_TRIGGER_HOLDOFF_G, w);
}
/*****************************************************************/
void v1730DPP_setTriggerHoldoff(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel)
{
  if(width > 32767){
    printf("ERROR: Trigger Holdoff must be less than 32767 ns\n");
    return;
  }
  if(width % 8 != 0){
    printf("ERROR: Trigger holdoff must be a multiple of 8 ns\n");
    return;
  }

  uint32_t w = (width/8) & 0x7FFF;

  // Channel mask
  uint32_t reg = V1730DPP_TRIGGER_HOLDOFF | (channel << 8);
    
  regWrite(mvme, base, reg, w);
}
/*****************************************************************/
void v1730DPP_setPreTriggerG(MVME_INTERFACE *mvme, uint32_t base, uint32_t preTrigger)
{
    uint32_t gate_offset = regRead(mvme, base, V1730DPP_GATE_OFFSET);
    gate_offset = gate_offset & 0xFF;
    
    if (preTrigger < gate_offset + 32){
      printf("Pre-Trigger automatically adjusted to minimum possible width\n");
      return;
    }
    
    if(preTrigger % 8 != 0){
      printf("ERROR: Pre-Trigger must be a multiple of 8 ns\n");
      return;
    }
    
    uint32_t w = (preTrigger/8) & 0x1FF;
    
    regWrite(mvme, base, V1730DPP_PRE_TRIGGER_WIDTH_G, w);
}
/*****************************************************************/
void v1730DPP_setPreTrigger(MVME_INTERFACE *mvme, uint32_t base, uint32_t preTrigger, int channel)
{
    uint32_t gate_offset = regRead(mvme, base, (V1730DPP_GATE_OFFSET | channel << 8 ));
    gate_offset = gate_offset & 0xFF;
    
    if (preTrigger < gate_offset + 32){
      printf("Pre-Trigger automatically adjusted to minimum possible width\n");
      return;
    }
    
    if(preTrigger % 8 != 0){
      printf("ERROR: Pre-Trigger must be a multiple of 8 ns\n");
      return;
    }
    
    uint32_t w = (preTrigger/8) & 0x1FF;
    
    uint32_t reg = V1730DPP_PRE_TRIGGER_WIDTH | (channel << 8 );
    
    regWrite(mvme, base, reg, w);
}
/*****************************************************************/
void v1730DPP_setGainG(MVME_INTERFACE *mvme, uint32_t base, uint32_t gain)
{
  if((gain > 5) | (gain < 0)){
    printf("ERROR: Gain setting must be between 0 and 5\n");
    return;
  }
  uint32_t vRange = regRead(mvme, base, V1730DPP_DYNAMIC_RANGE) & 0x0;
  if(vRange == 0){
    printf("Channel %d using Range = 2 Vpp\n",0);
    switch(gain){
    case 0:
      printf("Gain is 5 fC/channel\n"); break;
    case 1:
      printf("Gain is 20 fC/channel\n"); break;
    case 2:
      printf("Gain is 80 fC/channel\n"); break;
    case 3:
      printf("Gain is 320 fC/channel\n"); break;
    case 4:
      printf("Gain is 1.28 pC/channel\n"); break;
    case 5:
      printf("Gain is 5.12 pC/channel\n"); break;      
    }
  } else {
    printf("Channel %d using Range = 0.5 Vpp\n",0);
    switch(gain){
    case 0:
      printf("Gain is 1.25 fC/channel\n"); break;
    case 1:
      printf("Gain is 5 fC/channel\n"); break;
    case 2:
      printf("Gain is 20 fC/channel\n"); break;
    case 3:
      printf("Gain is 80 fC/channel\n"); break;
    case 4:
      printf("Gain is 320 fC/channel\n"); break;
    case 5:
      printf("Gain is 1.28 pC/channel\n"); break;      
    }
  }
  uint32_t g = (gain & 0x7);

  // Read the current algorithm control register, then add this new gain
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | g;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setGain(MVME_INTERFACE *mvme, uint32_t base, uint32_t gain, int channel)
{
  if((gain > 5) | (gain < 0)){
    printf("ERROR: Gain setting must be between 0 and 5\n");
    return;
  }
  uint32_t vRange = regRead(mvme, base, (V1730DPP_DYNAMIC_RANGE | channel << 8));
  if(vRange == 0){
    printf("Channel %d using Range = 2 Vpp\n",channel);
    switch(gain){
    case 0:
      printf("Gain is 5 fC/channel\n"); break;
    case 1:
      printf("Gain is 20 fC/channel\n"); break;
    case 2:
      printf("Gain is 80 fC/channel\n"); break;
    case 3:
      printf("Gain is 320 fC/channel\n"); break;
    case 4:
      printf("Gain is 1.28 pC/channel\n"); break;
    case 5:
      printf("Gain is 5.12 pC/channel\n"); break;      
    }
  } else {
    printf("Channel %d using Range = 0.5 Vpp\n",channel);
    switch(gain){
    case 0:
      printf("Gain is 1.25 fC/channel\n"); break;
    case 1:
      printf("Gain is 5 fC/channel\n"); break;
    case 2:
      printf("Gain is 20 fC/channel\n"); break;
    case 3:
      printf("Gain is 80 fC/channel\n"); break;
    case 4:
      printf("Gain is 320 fC/channel\n"); break;
    case 5:
      printf("Gain is 1.28 pC/channel\n"); break;      
    }
  }
  uint32_t g = gain & 0x7;

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new gain
  uint32_t value = regRead(mvme, base, reg);
  value = value | g;
  
  regWrite(mvme, base, reg, value);
}

/*****************************************************************/
void v1730DPP_setDiscriminationModeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (LED) or 1 (CFD)\n");
    return;
  }
  switch(mode){
    case 0:
      printf("Discrimination mode for all channels set to LED\n"); break;
    case 1:
      printf("Discrimination mode for all channels set to CFD\n"); break;
  }

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 6;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setDiscriminationMode(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (LED) or 1 (CFD)\n");
    return;
  }
  switch(mode){
    case 0:
      printf("Discrimination mode for channel %d set to LED\n",channel); break;
    case 1:
      printf("Discrimination mode for channel %d set to CFD\n",channel); break;
  }

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 6;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setTriggerPileupG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
    case 0:
      printf("Pile-up within the gate is not counted as a trigger for all channels\n"); break;
    case 1:
      printf("Pile-up within the gate is counted as a trigger for all channels\n"); break;
  }
    
  // Read the current algorithm control register, then add this new trigger pile-up setting
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 7;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setTriggerPileup(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
    case 0:
      printf("Pile-up within the gate is not counted as a trigger for channel %d\n", channel); break;
    case 1:
      printf("Pile-up within the gate is counted as a trigger for channel %d\n", channel); break;
  }
    
  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);
    
  // Read the current algorithm control register, then add this new trigger pile-up setting
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 7;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setPileUpRejectionG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
    case 0:
      printf("Pile-up within the gate is not rejected for all channels\n"); break;
    case 1:
      printf("Pile-up within the gate is rejected for all channels\n"); break;
  }
    
  // Read the current algorithm control register, then add this new pile-up rejection setting
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 26;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);  
}
/*****************************************************************/
void v1730DPP_setPileUpRejection(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
    case 0:
      printf("Pile-up within the gate is not rejected for channel %d\n", channel); break;
    case 1:
      printf("Pile-up within the gate is rejected for channel %d\n", channel); break;
  }
    
  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);
    
  // Read the current algorithm control register, then add this new pile-up rejection setting
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 26;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setSelfTriggerAcquisitionG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
    case 0:
      printf("Self-trigger is used to acquire events and is propagated to the motherboard for all channels\n"); break;
    case 1:
      printf("Self-trigger is not used to acquire events but is propagated to the motherboard for all channels\n"); break;
  }
    
  // Read the current algorithm control register, then add this new self-trigger acquisition setting
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 24;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);    
}
/*****************************************************************/
void v1730DPP_setSelfTriggerAcquisition(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
    case 0:
      printf("Self-trigger is used the acquire events and is propagated to the motherboard for channel %d\n", channel); break;
    case 1:
      printf("Self-trigger is not used to acquire events but is propagated to the motherboard for channel %d\n", channel); break;
  }
    
  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);
    
  // Read the current algorithm control register, then add this new self-trigger acquisition setting
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 24;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setOverRangeRejectionG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
    case 0:
      printf("Disabled event rejection when a sample is over/under dynamic range during long gate integration for all channels\n"); break;
    case 1:
      printf("Enabled event rejection when sample is over/under dynamic range during long gate integration for all channels\n"); break;
  }
    
  // Read the current algorithm control register, then add this new over-range rejection setting
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 29;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);    
}
/*****************************************************************/
void v1730DPP_setOverRangeRejection(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
    case 0:
      printf("Disabled event rejection when a sample is over/under dynamic range during long gate integration for channel %d\n", channel); break;
    case 1:
      printf("Enabled event rejection when a sample is over/under dynamic range during long gate integration for channel %d\n", channel); break;
  }
    
  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);
    
  // Read the current algorithm control register, then add this new over-range rejection setting
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 29;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setBaselineCalcRestartG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
      case 0:
          printf("Baseline calculation does not restart at the end of the long gate\n"); break;
      case 1:
          printf("Baseline calculation restarts at the end of the long gate\n"); break;
  }
    
  // Read the current algorithm control register, then add this new baseline calc setting
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 15;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setBaselineCalcRestart(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (disabled) or 1 (enabled)\n");
  }
  switch(mode){
      case 0:
          printf("Baseline calculation does not restart at the end of the long gate for channel %d\n", channel); break;
      case 1:
          printf("Baseline calculation restarts at the end of the long gate for channel %d\n", channel); break;
  }
    
  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);
    
  // Read the current algorithm control register, then add this new baseline calc setting
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 15;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setInputSmoothingG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 4) | (mode < 0)){
    printf("ERROR: input smoothing factor setting must be 0 (disabled), 1 (2 samples), 2 (4 samples), 3 (8 samples), or 4 (16 samples)\n");
    return;
  }

  switch(mode){
    case 0:
        printf("Disabled smoothed signal for charge integration for all channels\n"); break;
    default:
        printf("Enabled smoothed signal for charge integration for all channels\n");
        uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL2);
        value = value | 1 << 11;
        regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL2_G, value);
  }
  
  uint32_t bin;
  switch(mode){
    case 0:
      bin = 0x0; break; // Disabled
    case 1:
      bin = 0x1; break; // 2 samples
    case 2:
      bin = 0x2; break; // 4 samples
    case 3:
      bin = 0x3; break; // 8 samples
    case 4:
      bin = 0x4; break; // 16 samples
  }

  // Read the current algorithm control register, then add this new input smoothing factor mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL2);
  value = value | (bin << 12);
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL2_G, value);
}
/*****************************************************************/
void v1730DPP_setInputSmoothing(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 4) | (mode < 0)){
    printf("ERROR: input smoothing factor setting must be 0 (disabled), 1 (2 samples), 2 (4 samples), 3 (8 samples), or 4 (16 samples)\n");
    return;
  }

  switch(mode){
    case 0:
        printf("Disabled smoothed signal for charge integration for channel %d\n", channel); break;
    default:
        printf("Enabled smoothed signal for charge integration for channel %d\n", channel);
        // Channel mask
        uint32_t reg = V1730DPP_ALGORITHM_CONTROL2 | (channel << 8);
        // Read the current algorithm control register, then enable input smoothing setting
        uint32_t value = regRead(mvme, base, reg);
        value = value | 1 << 11;
        regWrite(mvme, base, reg, value);
  }
  
  uint32_t bin;
  switch(mode){
    case 0:
      bin = 0x0; break; // Disabled
    case 1:
      bin = 0x1; break; // 2 samples
    case 2:
      bin = 0x2; break; // 4 samples
    case 3:
      bin = 0x3; break; // 8 samples
    case 4:
      bin = 0x4; break; // 16 samples
  }

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL2 | (channel << 8);

  // Read the current algorithm control register, then add this new input smoothing factor mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | (bin << 12);
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setTestPulseG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: test pulse setting must be 0 (off) or 1 (on)\n");
    return;
  }
  switch(mode){
    case 0:
      printf("Test pulse for all channels is off\n"); break;
    case 1:
      printf("Test pulse for all channels is on\n"); break;
  }

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 8;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setTestPulse(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: test pulse setting must be 0 (off) or 1 (on)\n");
    return;
  }
  switch(mode){
    case 0:
      printf("Test pulse for channel %d is off\n",channel); break;
    case 1:
      printf("Test pulse for channel %d is on\n",channel); break;
  }

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 8;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setTestPulseRateG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 3) | (mode < 0)){
    printf("ERROR: test pulse rate setting must be 0 (1 kHz), 1 (10 kHz), 2 (100 kHz), or 3 (1 MHz)\n");
    return;
  }

  uint32_t bin;
  switch(mode){
    case 0:
      bin = 0x0; break; // 1 kHz
    case 1:
      bin = 0x1; break; // 10 kHz
    case 2:
      bin = 0x2; break; // 100 kHz
    case 3:
      bin = 0x3; break; // 1 MHz
  }

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | (bin << 9);
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setTestPulseRate(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 3) | (mode < 0)){
    printf("ERROR: test pulse rate setting must be 0 (1 kHz), 1 (10 kHz), 2 (100 kHz), or 3 (1 MHz)\n");
    return;
  }

  uint32_t bin;
  switch(mode){
    case 0:
      bin = 0x0; break; // 1 kHz
    case 1:
      bin = 0x1; break; // 10 kHz
    case 2:
      bin = 0x2; break; // 100 kHz
    case 3:
      bin = 0x3; break; // 1 MHz
  }
  
  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | (bin << 9);
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setPolarityG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Polarity setting must be 0 (positive) or 1 (negative)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Polarity for all channels is positive\n"); break;
    case 1:
      printf("Polarity for all channels is negative\n"); break;
  }

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 16;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setPolarity(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Polarity setting must be 0 (positive) or 1 (negative)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Polarity for channel %d is positive\n",channel); break;
    case 1:
      printf("Polarity for channel %d is negative\n",channel); break;
  }

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 16;
  
  regWrite(mvme, base, reg, value);
}
/*****************************************************************/
void v1730DPP_setTriggerHysteresisG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Trigger Hysteresis setting must be 0 (enabled) or 1 (disabled)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Trigger hysteresis for all channels is on\n"); break;
    case 1:
      printf("Trigger hysteresis for all channels is off\n"); break;
  }

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 30;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}

/*****************************************************************/
void v1730DPP_setTriggerHysteresis(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Trigger Hysteresis setting must be 0 (enabled) or 1 (disabled)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Trigger hysteresis for channel %d is on\n",channel); break;
    case 1:
      printf("Trigger hysteresis for channel %d is off\n",channel); break;
  }

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 30;
  
  regWrite(mvme, base, reg, value);
}

/**********************************************************************/
void v1730DPP_setOppositePolarityDetectionG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Opposite Polarity Detection setting must be 0 (enabled) or 1 (disabled)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Enabled detection of opposite polarity signals for all channels\n"); break;
    case 1:
      printf("Disabled detection of opposite polarity signals for all channels\n"); break;
  }

  // Read the current algorithm control register, then add this new detection mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 31;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/**********************************************************************/
void v1730DPP_setOppositePolarityDetection(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Opposite Polarity Detection setting must be 0 (enabled) or 1 (disabled)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Enabled detection of opposite polarity signals for channel %d\n", channel); break;
    case 1:
      printf("Disabled detection of opposite polarity signals for channel %d\n", channel); break;
  }

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new detection mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 31;
  
  regWrite(mvme, base, reg, value);
}
/**********************************************************************/
void v1730DPP_setTriggerModeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Trigger mode must be 0 (Normal) or 1 (Coincidence)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Normal Mode enabled: Each channel self-triggers independently\n"); break;
    case 1:
      printf("Coincidence Mode enabled: Each channel saves the event only when it occurs inside the shaped trigger width\n"); break;
  }

  uint32_t bin;
  switch(mode){
    case 0:
      bin = 0x0; break; // Normal
    case 1:
      bin = 0x1; break; // Coincidence
    //case 2:
      //bin = 0x3; break; // Anticoincidence, 0x2 is reserved
  }

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | (bin << 18);
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/**********************************************************************/
void v1730DPP_setTriggerMode(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Trigger mode must be 0 (Normal) or 1 (Coincidence)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Normal Mode enabled for channel %d: Channel self-triggers independently\n", channel); break;
    case 1:
      printf("Coincidence Mode enabled for channel %d: Event saved only when it occurs inside the shaped trigger width\n", channel); break;
  }

  uint32_t bin;
  switch(mode){
    case 0:
      bin = 0x0; break; // Normal
    case 1:
      bin = 0x1; break; // Coincidence
    //case 2:
      //bin = 0x3; break; // Anticoincidence, 0x2 is reserved
  }

  // Channel Mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);

  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | (bin << 18);
  
  regWrite(mvme, base, reg, value);
}
/**********************************************************************/
void v1730DPP_setTriggerPropagation(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 1) | (mode < 0)){
    printf("ERROR: Trigger propagation must be 0 (disabled) or 1 (enabled)\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Trigger propagation from motherboard disabled for all channels\n"); break;
    case 1:
      printf("Trigger propagation from motherboard enabled for all channels\n"); break;
  }

  // Read the current board config register, then add this new trigger propagation mode
  uint32_t value = regRead(mvme, base, V1730DPP_BOARD_CONFIG);
  value = value | mode << 2;
  
  regWrite(mvme, base, V1730DPP_BOARD_CONFIG, value);
}
/*****************************************************************/
void v1730DPP_setTriggerCountingModeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (trigger from only accepted events) or 1 (all events)\n");
  }

  switch(mode){
    case 0:
      printf("Trigger counting mode for all channels set to trigger from only accepted events\n"); break;
    case 1:
      printf("Trigger counting mode for all channels set to trigger from all (even rejected) events\n"); break;
  }
    
  // Read the current algorithm control register, then add this new trigger counting mode
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL);
  value = value | mode << 5;
  
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL_G, value);
}
/*****************************************************************/
void v1730DPP_setTriggerCountingMode(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if ((mode > 1) | (mode < 0)){
    printf("ERROR: mode setting must be 0 (trigger from only accepted events) or 1 (all events)\n");
  }
  switch(mode){
    case 0:
      printf("Trigger counting mode for channel %d set to trigger from only accepted events\n", channel); break;
    case 1:
      printf("Trigger counting mode for channel %d set to trigger from all (even rejected) events\n", channel); break;
  }
    
  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (channel << 8);
    
  // Read the current algorithm control register, then add this new trigger counting mode
  uint32_t value = regRead(mvme, base, reg);
  value = value | mode << 5;
  
  regWrite(mvme, base, reg, value);
}
/**********************************************************************/
void v1730DPP_setShapedTriggerG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width)
{
  if(width > 8176){
    printf("ERROR: Shaped trigger width must be less than 8176 ns\n");
    return;
  }

  uint32_t w = (width/8) & 0x3FF;

  regWrite(mvme, base, V1730DPP_SHAPED_TRIGGER_WIDTH_G, w);
}
/**********************************************************************/
void v1730DPP_setShapedTrigger(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel)
{
  if(width > 8176){
    printf("ERROR: Shaped trigger width must be less than 8176 ns\n");
    return;
  }

  uint32_t w = (width/8) & 0x3FF;

  // Channel mask
  uint32_t reg = V1730DPP_SHAPED_TRIGGER_WIDTH | (channel << 8);

  regWrite(mvme, base, reg, w);
}
/**********************************************************************/
void v1730DPP_setLatencyTimeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width)
{
  if(width > 2040){
    printf("ERROR: Latency time must be less than 2040 ns\n");
    return;
  }

  if (width > 0){
    uint32_t w = (width/8) & 0xFF;

    regWrite(mvme, base, V1730DPP_LATENCY_TIME_G, w);
  }
}
/**********************************************************************/
void v1730DPP_setLatencyTime(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel)
{
  if(width > 2040){
    printf("ERROR: Latency time must be less than 2040 ns\n");
    return;
  }

  if (width > 0){
    uint32_t w = (width/8) & 0xFF;

    // Channel mask
    uint32_t reg = V1730DPP_LATENCY_TIME | (channel << 8);

    regWrite(mvme, base, reg, w);
  }
}
/**********************************************************************/
void v1730DPP_setLocalShapedTriggerModeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  // NOTE: Bits[2:0] set to 1 by default, so they must be cleared before setting
  //       new values. We need a different procedure than:
  //         value = value | (mode << bin)
  //       To first clear bits, use:
  //         value = value & ~ (mask << bin),
  //       where bin is the lowest bit to be cleared, e.g. 1111 0000 --> bin = 4
  //       and mask is the value when the relevant bits are all 1 and are shifted right all the way
  //       e.g. need to clear bits[6:4], then mask = 0000 0111

  if((mode > 4) | (mode < 0)){
    printf("ERROR: Mode must be 0 (disabled), 1 (AND), 2 (Even Only), 3 (Odd Only), or 4 (OR)\n");
    return;
  }

  // Read the current algorithm control register
  uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL2);

  // Clear bits[2:0], since they are 1 by default
  uint32_t mask = 0x7; // 0111
  uint32_t bin = 0;
  value = value & ~(mask << bin); // xxxx ... x000
  regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL2_G, value);

  switch(mode){
    case 0:
      printf("Disabled local shaped trigger for all channels\n"); break;
    default:
      printf("Enabled local shaped trigger for all channels\n");
      // Read the current algorithm control register, then enable local shaped trigger setting
      //uint32_t value = regRead(mvme, base, reg);
      value = value | 1 << 2;
      regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL2_G, value);
  }
  
  if (mode > 0){
    uint32_t bin;
    switch(mode){
      case 1:
        bin = 0x0; break; // AND
      case 2:
        bin = 0x1; break; // Even Only
      case 3:
        bin = 0x2; break; // Odd Only
      case 4:
        bin = 0x3; break; // OR
    }

    // Read the current algorithm control register, then add this new local shaped trigger mode
    uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL2);
    value = value | bin;
    
    regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL2_G, value);
  }
}
/**********************************************************************/
void v1730DPP_setLocalShapedTriggerMode(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  // NOTE: Bits[2:0] set to 1 by default, so they must be cleared before setting
  //       new values. We need a different procedure than:
  //         value = value | (mode << bin)
  //       To first clear bits, use:
  //         value = value & ~ (mask << bin),
  //       where bin is the lowest bit to be cleared, e.g. 1111 0000 --> bin = 4
  //       and mask is the value when the relevant bits are all 1 and are shifted right all the way
  //       e.g. need to clear bits[6:4], then mask = 0000 0111

  if((mode > 4) | (mode < 0)){
    printf("ERROR: Mode must be 0 (disabled), 1 (AND), 2 (Even Only), 3 (Odd Only), or 4 (OR)\n");
    return;
  }

  // Channel mask
  uint32_t reg = V1730DPP_ALGORITHM_CONTROL2 | (channel << 8);

  // Read the current algorithm control register
  uint32_t value = regRead(mvme, base, reg);

  // Clear bits[2:0], since they are 1 by default
  uint32_t mask = 0x7; // 0111
  uint32_t bin = 0;
  value = value & ~(mask << bin); // xxxx ... x000
  regWrite(mvme, base, reg, value);

  switch(mode){
    case 0:
      printf("Disabled local shaped trigger for channel %d (and its coupled channel if enabled)\n", channel); break;
    default:
      printf("Enabled local shaped trigger for channel %d (and its coupled channel if enabled)\n", channel);
      // Read the current algorithm control register, then enable local shaped trigger setting
      //uint32_t value = regRead(mvme, base, reg);
      value = value | 1 << 2;
      regWrite(mvme, base, reg, value);
  }
  
  if (mode > 0){
    uint32_t bin;
    switch(mode){
      case 1:
        bin = 0x0; break; // AND
      case 2:
        bin = 0x1; break; // Even Only
      case 3:
        bin = 0x2; break; // Odd Only
      case 4:
        bin = 0x3; break; // OR
    }

    // Read the current algorithm control register, then add this new local shaped trigger mode
    uint32_t value = regRead(mvme, base, reg);
    value = value | bin;
    
    regWrite(mvme, base, reg, value);
  }
}
/**********************************************************************/
void v1730DPP_setLocalTriggerValidationModeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 4) | (mode < 0)){
    printf("ERROR: Mode must be 0-4. 0 - disabled, 1 - use additional local trigger validation options,\n"); 
    printf("       2 - val0 = val1 = signal from motherboard mask, 3 - AND (val0 = val1 = trg0 AND trg1),\n");
    printf("       4 - OR (val0 = val1 = trg0 OR trg1). Option 4 must only be used in normal mode.\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Disabled local trigger validation for all channels\n"); break;
    default:
      printf("Enabled local trigger validation for all channels\n");
      // Read the current algorithm control register, then enable local trigger validation setting
      uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL2);
      value = value | 1 << 6;
      regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL2_G, value);
  }

  if (mode > 1){
    uint32_t bin;
    switch(mode){
      case 2:
        bin = 0x1; break; // val0 = val1 = signal from motherboard mask
      case 3:
        bin = 0x2; break; // AND (val0 = val1 = trg0 AND trg1)
      case 4:
        bin = 0x3; break; // OR (val0 = val1 = trg0 OR trg1)
    }

    // Read the current algorithm control register, then add this new local trigger validation mode
    uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL2);
    value = value | (bin << 4);
    
    regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL2_G, value);
  }
}
/**********************************************************************/
void v1730DPP_setLocalTriggerValidationMode(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 4) | (mode < 0)){
    printf("ERROR: Mode must be 0-4. 0 - disabled, 1 - use additional local trigger validation options,\n"); 
    printf("       2 - val0 = val1 = signal from motherboard mask, 3 - AND (val0 = val1 = trg0 AND trg1),\n");
    printf("       4 - OR (val0 = val1 = trg0 OR trg1). Option 4 must only be used in normal mode.\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Disabled local trigger validation for channel %d (and its coupled channel if enabled)\n", channel); break;
    default:
      printf("Enabled local trigger validation for channel %d (and its coupled channel if enabled)\n", channel);
      // Channel mask
      uint32_t reg = V1730DPP_ALGORITHM_CONTROL2 | (channel << 8);
      // Read the current algorithm control register
      uint32_t value = regRead(mvme, base, reg);
      value = value | 1 << 6;
      regWrite(mvme, base, reg, value);
  }

  if (mode > 1){
    uint32_t bin;
    switch(mode){
      case 2:
        bin = 0x1; break; // val0 = val1 = signal from motherboard mask
      case 3:
        bin = 0x2; break; // AND (val0 = val1 = trg0 AND trg1)
      case 4:
        bin = 0x3; break; // OR (val0 = val1 = trg0 OR trg1)
    }

    // Channel mask
    uint32_t reg = V1730DPP_ALGORITHM_CONTROL2 | (channel << 8);

    // Read the current algorithm control register, then add this new local trigger validation mode
    uint32_t value = regRead(mvme, base, reg);
    value = value | (bin << 4);
    
    regWrite(mvme, base, reg, value);
  }
}
/**********************************************************************/
void v1730DPP_setAdditionalLocalTriggerValidationModeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode)
{
  if((mode > 2) | (mode < 0)){
    printf("ERROR: Mode must be 0-2. 0 - disabled / options from local trigger validation, 1 - validation from\n");
    printf("       paired channel AND motherboard, 2 - validation from paired channel OR motherboard.\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Disabled additional local trigger validation for all channels\n"); break;
    default:
      printf("Enabled additional local trigger validation for all channels\n");
  }

  if (mode > 0){
    uint32_t bin;
    switch(mode){
      case 1:
        bin = 0x1; break; // validation from paired channel AND motherboard
      case 2:
        bin = 0x2; break; // validation from paired channel OR motherboard
      //case 3:
      //  bin = 0x3; break; // reserved
    }

    // Read the current algorithm control register, then add this new additional local trigger validation mode
    uint32_t value = regRead(mvme, base, V1730DPP_ALGORITHM_CONTROL2);
    value = value | (bin << 25);
    
    regWrite(mvme, base, V1730DPP_ALGORITHM_CONTROL2_G, value);
  }
}
/**********************************************************************/
void v1730DPP_setAdditionalLocalTriggerValidationMode(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel)
{
  if((mode > 2) | (mode < 0)){
    printf("ERROR: Mode must be 0-2. 0 - disabled / options from local trigger validation, 1 - validation from\n");
    printf("       paired channel AND motherboard, 2 - validation from paired channel OR motherboard.\n");
    return;
  }

  switch(mode){
    case 0:
      printf("Disabled additional local trigger validation for channel %d\n", channel); break;
    default:
      printf("Enabled additional local trigger validation for channel %d\n", channel);
  }

  if (mode > 0){
    uint32_t bin;
    switch(mode){
      case 1:
        bin = 0x1; break; // validation from paired channel AND motherboard
      case 2:
        bin = 0x2; break; // validation from paired channel OR motherboard
      //case 3:
      //  bin = 0x3; break; // reserved
    }

    // Channel mask
    uint32_t reg = V1730DPP_ALGORITHM_CONTROL2 | (channel << 8);

    // Read the current algorithm control register, then add this new additional local trigger validation mode
    uint32_t value = regRead(mvme, base, reg);
    value = value | (bin << 25);
    
    regWrite(mvme, base, reg, value);
  }
}
/**********************************************************************/
void v1730DPP_setTriggerValidationMask(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, uint32_t couples, int channel)
{
  if((mode > 2) | (mode < 0)){
    printf("ERROR: Mode must be 0-2. 0 = disabled, 1 = OR, 2 = AND.\n");
    return;
  }

  // Couple mask
  int couple = (int) std::floor(channel/2);
  uint32_t reg = V1730DPP_TRIGGER_VALIDATION_MASK + (4 * couple);

  switch(mode){
    case 0:
      printf("Disabled trigger validation mask for couple %d\n", couple); break;
    default:
      printf("Enabled trigger validation mask for couple %d\n", couple);
  }

  if (mode > 0){
    uint32_t bin;
    switch(mode){
      case 1:
        bin = 0x0; break; // OR
      case 2:
        bin = 0x1; break; // AND
      //case 3:
      //  bin = 0x2; break; // Majority
      //case 4:
      //  bin = 0x3; break; // reserved
    }

    // TODO - Verify that Trigger Validation Mask can be read at 0x8180 level address and not 0x1180
    // TODO - Verify that the bits are 0 by default at register 0x8180.; If not, need to clear them
    // Read the current register, then add this new trigger validation mask
    uint32_t value = regRead(mvme, base, reg);
    value = value | (bin << 8);
    value = value | (couples & 0xFF);
    regWrite(mvme, base, reg, value);
  }
}
/**********************************************************************/
void v1730DPP_CalibrateADC(MVME_INTERFACE *mvme, uint32_t base)
{
  while(((regRead(mvme, base, V1730DPP_CHANNELN_STATUS) & 0x4)>>2) == 1){
    printf(".");
    sleep(1);
  };
  printf("Calibrating ADC...\n");
  regWrite(mvme, base, V1730DPP_CALIBRATE_ADC, 1);
  while(((regRead(mvme, base, V1730DPP_CHANNELN_STATUS) & 0x4)>>2) == 1){
    printf(".");
    sleep(1);
  };
  printf("\nDone!\n");  
}
/**********************************************************************/
void v1730DPP_waitForReady(MVME_INTERFACE *mvme, uint32_t base)
{
  while(((regRead(mvme, base, V1730DPP_CHANNELN_STATUS) & 0x4)>>2) == 1){
    printf(".");
    sleep(1);
  };
  printf("\nDone!\n");  
}
/**********************************************************************/
void v1730DPP_EnableChannel(MVME_INTERFACE *mvme, uint32_t base, int channel)
{
  printf("Enabling channel %d\n",channel);
  
  uint32_t value = regRead(mvme, base, V1730DPP_CHANNEL_ENABLE_MASK);
  value |= 1 << channel;

  regWrite(mvme, base, V1730DPP_CHANNEL_ENABLE_MASK, value);
}
/**********************************************************************/
void v1730DPP_DisableChannel(MVME_INTERFACE *mvme, uint32_t base, int channel)
{
  printf("Disabling channel %d\n",channel);
  
  uint32_t value = regRead(mvme, base, V1730DPP_CHANNEL_ENABLE_MASK);
  value &= ~(1 << channel);

  regWrite(mvme, base, V1730DPP_CHANNEL_ENABLE_MASK, value);
}
/**********************************************************************/
int v1730DPP_isRunning(MVME_INTERFACE *mvme, uint32_t base)
{
  uint32_t value = regRead(mvme, base, V1730DPP_ACQUISITION_STATUS);

  uint32_t status = (value & 0x4) >> 2;

  return status;
}
/**********************************************************************/
int v1730DPP_EventsReady(MVME_INTERFACE *mvme, uint32_t base)
{
  uint32_t value = regRead(mvme, base, V1730DPP_ACQUISITION_STATUS);

  uint32_t status = (value & 0x8) >> 3;

  return status;
}
/**********************************************************************/
int v1730DPP_EventsFull(MVME_INTERFACE *mvme, uint32_t base)
{
  uint32_t value = regRead(mvme, base, V1730DPP_ACQUISITION_STATUS);

  uint32_t status = (value & 0x10) >> 4;

  return status;
}
/**********************************************************************/
int v1730DPP_EventSize(MVME_INTERFACE *mvme, uint32_t base)
{
  return regRead(mvme, base, V1730DPP_EVENT_SIZE);
}
/**********************************************************************/
int v1730DPP_ReadoutReady(MVME_INTERFACE *mvme, uint32_t base)
{     
  uint32_t value = regRead(mvme, base, V1730DPP_READOUT_STATUS);

  uint32_t status = (value & 0x1);

  return status;
}
/**********************************************************************/
void v1730DPP_AcqCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t command)
{
  // Read the current algorithm control register, then add this new trigger mode
  uint32_t value = regRead(mvme, base, V1730DPP_ACQUISITION_CONTROL);
  
  if(command == 1)
    value |= (1 << 2);
  else
    value &= ~(1 << 2);
  
  regWrite(mvme, base, V1730DPP_ACQUISITION_CONTROL, value);
}
/**********************************************************************/
void v1730DPP_getChannelStatus(MVME_INTERFACE *mvme, uint32_t base, int channel)
{
  printf("---------CHANNEL %d----------------------------\n",channel);    
  uint32_t reg = regRead(mvme, base, V1730DPP_CHANNEL_STATUS | (channel << 8));  
  printf("Channel status %d: 0x%x\n", channel, reg );  

  printf("Running: %d\n", reg & 0x1);
  printf("Busy: %d\n", (reg & 0x2) >> 1);
  printf("Over threshhold: %d\n", (reg & 0x10) >> 4);
  printf("Ram Full: %d\n", (reg & 0x80) >> 7);

  printf("\n");
  reg = regRead(mvme, base, V1730DPP_CHANNELN_STATUS | (channel << 8));
  printf("Channel n status %d: 0x%x\n", channel, reg );  

  printf("SPI busy: %d\n", (reg & 0x4) >> 2);
  printf("ADC Calibration: %d\n", (reg & 0x8) >> 3);
  printf("Over Temperature shutdown: %d\n", (reg & 0x100) >> 8);
  printf("------------------------------------------------\n");
}
/**********************************************************************/
void v1730DPP_getFirmwareRev(MVME_INTERFACE *mvme, uint32_t base)
{
  uint32_t reg = regRead(mvme, base, V1730DPP_FIRMWARE_REV);

  printf("Register: 0x%x\n", reg);
  int number = reg & 0xFF;
  int code = (reg & 0xFF00) >> 8;
  int dayup = (reg & 0xF0000) >> 16;
  int daylow = (reg & 0xF00000) >> 20;
  int month = (reg & 0xF000000) >> 24;
  int year = (reg & 0xF0000000) >> 28;
  
  printf("Firmware code and rev are: %d.%d\n",code,number);
  printf("Build date: %d-%d-%d%d\n",2016+year,month,daylow,dayup);
}
/**********************************************************************/
void v1730DPP_SoftwareTrigger(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1730DPP_SOFTWARE_TRIGGER, 1);
}
/**********************************************************************/
void v1730DPP_ForceDataFlush(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1730DPP_FORCE_DATA_FLUSH, 1);
}
/**********************************************************************/
void v1730DPP_SoftReset(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1730DPP_SOFTWARE_RESET, 1);
}
/**********************************************************************/
void v1730DPP_SoftClear(MVME_INTERFACE *mvme, uint32_t base)
{
  regWrite(mvme, base, V1730DPP_SOFTWARE_CLEAR, 1);
}
/**********************************************************************/
//uint32_t v1730DPP_DataRead(MVME_INTERFACE *mvme, uint32_t base, uint32_t *pdata, uint32_t n32w)
//{
//
//}
/**********************************************************************/
void v1730DPP_DataPrint(MVME_INTERFACE *mvme, uint32_t base)
{
  // Changed %d's to %u's, since the integers are declared as unsigned. - Will
  uint32_t buf;
  buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
  printf("Board aggregate size = %u\n", (buf & 0xFFFFFFF));
  buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
  printf("Format: 0x%x\n", buf);
  uint32_t chnmask = buf & 0xFF;
  buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
  printf("Board aggregate counter = %u\n", (buf & 0x7FFFFF));
  buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
  printf("Board aggregate time tag = %u\n\n", buf);
  for(int i=0; i<8; i++){ // Iterating over each dual channel aggregate (8 groups of 2)
    if(((chnmask >> i) & 0x1) == 1){ // e.g. If channel 0 is only one with data, skip to next board aggregate
      printf("Channel %u has data! Whoop!\n",i); // This should read "Channel 0 and/or 1 have data!" etc.
      buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
      uint32_t aggsize = buf & 0x3FFFFF;
      printf("Channel aggregate size = %u\n", aggsize);
      int nevents = (aggsize-2)/2; // was (aggsize-4)/2, but since extras disabled, only 2 lines to ignore
      buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
      printf("Format: 0x%x\n",buf);
      for(int j=0; j<nevents; j++){ // Iterating over each event
        buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
        int c = (buf & 0x80000000) >> 31;
        printf("Channel check: %u\n", c);
        printf("Time tag = %u\n", (buf & 0x7FFFFFFF));
        buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
        // printf("Extras: 0x%x\n",buf);
        // Note: Extras is not enabled, so it is skipped when RegisterRead is called here. - Will
        //buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
        printf("Qshort = %u\n", (buf & 0x7FFF));
        printf("Qlong = %u\n", ((buf & 0xFFFF0000)>>16));
      }
    }
  }
}
/**********************************************************************/
void v1730DPP_DataPrint_Updated(MVME_INTERFACE *mvme, uint32_t base, int N)
{
  // This reads qlong for each event and writes it to a file qlong_data.txt
  // N is number of board aggregates: N = 2**(N_b), where N_b is set in setAggregateOrg (3 here, so N=8)
  // AcqCtl Start and Stop sandwiched between this causes the memory location bits to be all 1's.
  // Try using ReadoutReady and EventsReady after each board aggregate.

  uint32_t buffer, qlong, qshort, timetag, chnmask, aggsize, board_aggsize;
  int16_t qlong_signed, qshort_signed;
  int nevents, c, event_count;
  int good_event_count = 0;
  int bad_event_count = 0, bad_event_count_qlong = 0;
  int stop_run = 0;
  
  for(int i=0; i<N; i++){ // Each board aggregate

    buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
    board_aggsize = buffer & 0xFFFFFFF;

    // Memory bits all becoming 1 means there is no more data to be read in the board agg.
    if ((board_aggsize > 2052) | (board_aggsize == 1)){
      printf("--------------------------------------------------\n");
      //printf("Data has finished reading.\n");
      printf("Ready to start\n");
      printf("--------------------------------------------------\n");
      printf("\n");
      stop_run = 1;
      break;
    }
    
    //printf("--------------------------------------------------\n");
    //printf("--------------------------------------------------\n");
    //printf("Board Aggregate %u\n",i);
    //printf("--------------------------------------------------\n");
    //printf("--------------------------------------------------\n");
    //printf("\n");
    //printf("Board aggregate size = %u\n", board_aggsize);
    buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
    //printf("Board Format: 0x%x\n", buffer);
    chnmask = buffer & 0xFF;
    buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
    //printf("Board aggregate counter = %u\n", (buffer & 0x7FFFFF));
    buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
    //printf("Board aggregate time tag = %u\n\n", buffer);
    //printf("\n");
    
    for(int j=0; j<8; j++){ // Each dual channel aggregate (8 groups of 2)
      if(((chnmask >> j) & 0x1) == 1){ // e.g. If channel 0 is only one with data, skip to next board aggregate

        //printf("----------------------------------------\n");
        //printf("Channels %u and/or %u have data! Whoop!\n",2*j,(2*j)+1);
        //printf("----------------------------------------\n");
        //printf("\n");
        buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
        aggsize = buffer & 0x3FFFFF;
        //printf("Channel aggregate size = %u\n", aggsize);
        nevents = (aggsize-2)/2;
        buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
        //printf("Channel Format: 0x%x\n",buffer);
        //printf("\n");

        event_count = 0;
        for(int k=0; k<nevents; k++){ // Iterating over each event

            buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
            c = (buffer & 0x80000000) >> 31;
            //printf("Event %u\n",event_count);
            //printf("Board Agg: %u\n", i);
            //printf("Channel check: %u\n", c);
            timetag = buffer & 0x7FFFFFFF;
            //printf("Time tag = %u\n", timetag);
            buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
            // printf("Extras: 0x%x\n",buf);
            // Note: Extras is not enabled, so it is skipped when RegisterRead is called here. - Will
            //buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
            qlong = ((buffer & 0xFFFF0000)>>16);
            qshort = (buffer & 0x7FFF);
            
            qlong_signed = qlong & 0xFFFF;
            qshort_signed = qshort & 0xFFFF;
            //printf("Qshort = %u, Qshort_s = %i\n",qshort,qshort_signed);
            //printf("Qlong = %u, Qlong_s = %i\n",qlong,qlong_signed);
            
            //printf("Qlong = %u, Ch. %u\n",qlong,(2*j) + c);
            //printf("\n");
            
            event_count++;
            if((qlong != 65535) & (qshort != 32767)){
                good_event_count++;
            }
            else{
                bad_event_count++;
            }
            if (qlong == 65535){
              bad_event_count_qlong++;
            }
            if(bad_event_count > 5000){
                stop_run = 1;
                printf("\n");
                printf("Error: Memory location bit failure or too many events \n");
                printf("       with higher energy than the maximum cutoff (change the gain/threshold). \n\n");
                break;
            }
        }
        // Flush data after every channel agg. This is done because short runs were causing
        // not all the board aggs to get filled. Whichever board agg was the last one, would
        // have all its memory locations bits be 1, and the last events in that agg were not
        // counted, but now they are. ForceDataFlush has no effect if the board agg is full.
        v1730DPP_ForceDataFlush(mvme, base);
      }
      if(stop_run == 1){
        break;
      }
    }
    if(stop_run == 1){
      break;
    }
  }
  
  //v1730DPP_ForceDataFlush(mvme, base);
  //printf("Total events: %d\n", event_count);
  //printf("Good events: %d\n", good_event_count);
  //printf("Events with max qlong and qshort: %d\n", bad_event_count);
  //printf("Events with only max qlong: %d\n\n", bad_event_count_qlong);
}
/**********************************************************************/
void v1730DPP_ReadQLong(MVME_INTERFACE *mvme, uint32_t base, DWORD *pdest, uint32_t *nentry)
{
  // This reads qlong for each event 
  uint32_t buffer, qlong, qshort, timetag, chnmask, aggsize, board_aggsize, c, ch, qlong_ch;
  int16_t qlong_signed, qshort_signed;
  int nevents;
  int stop_run = 0;

  uint32_t event_count = 0;
  
  //----------------- Temporary ------------------
  //FILE *f_qlong = fopen("qlong_data.txt", "w");
  //FILE *f_qshort = fopen("qshort_data.txt", "w");
  //FILE *f_timetag = fopen("timetag_data.txt", "w");
  //if ((f_qlong == NULL) | (f_timetag == NULL)){
  //  printf("Error opening .txt files!\n");
  //  return;
  //}
  //----------------------------------------------
  
  buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);

  //------------  Will's addition ------------
  board_aggsize = buffer & 0xFFFFFFF;
  // Memory bits all becoming 1 means there is no more data to be read in the current board agg.
  if ((board_aggsize > 2052) | (board_aggsize == 1)){
    printf("--------------------------------------------------\n");
    printf("Error: All bits are 1 in board aggregate header line(s).\n");
    printf("--------------------------------------------------\n");
    printf("\n");
    stop_run = 1;
  }
  //------------------------------------------
  
  //printf("Board aggregate size = %u\n", board_aggsize);
  buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
  //printf("Board Format: 0x%x\n", buffer);
  chnmask = buffer & 0xFF;
  buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
  //printf("Board aggregate counter = %u\n", (buffer & 0x7FFFFF));
  buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
  //printf("Board aggregate time tag = %u\n\n", buffer);
  //printf("\n");
   
  for(int j=0; j<8; j++){ // Each dual channel aggregate (8 groups of 2)
    if(((chnmask >> j) & 0x1) == 1){ // e.g. If channel 0 is only one with data, skip to next board aggregate

      // ------------ Will's addition ------------
      if (stop_run == 1){
        break;
      }
      // -----------------------------------------
      
      //printf("----------------------------------------\n");
      //printf("Channels %u and/or %u have data! Whoop!\n",2*j,(2*j)+1);
      //printf("----------------------------------------\n");
      //printf("\n");
      buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
      aggsize = buffer & 0x3FFFFF;
      //printf("Channel aggregate size = %u\n", aggsize);
      nevents = (aggsize-2)/2;
      buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
      //printf("Channel Format: 0x%x\n",buffer);
      //printf("\n");

      for(int k=0; k<nevents; k++){ // Iterating over each event

        buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
        c = (buffer & 0x80000000) >> 31;
        ch = ((2*j) + c) << 16;
        //printf("Event %u\n",event_count);
        //printf("Board Agg: %u\n", i);
        //printf("Channel check: %u\n", c);
        timetag = buffer & 0x7FFFFFFF;
        //printf("Time tag = %u\n", timetag);
        buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
        // printf("Extras: 0x%x\n",buf);
        // Note: Extras is not enabled, so it is skipped when RegisterRead is called here. - Will
        //buf = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);
        qlong = ((buffer & 0xFFFF0000)>>16);
        qlong_ch = qlong | ch;
        qshort = (buffer & 0x7FFF);
        
        qlong_signed = qlong & 0xFFFF;
        qshort_signed = qshort & 0xFFFF;
        //printf("Qshort = %u, Qshort_s = %i\n",qshort,qshort_signed);
        //printf("Qlong = %u, Qlong_s = %i\n",qlong,qlong_signed);
    
        //----------------- Temporary ------------------
        //fprintf(f_qlong, "%u\n", qlong);   // write qlong/qshort/timetag to .txt files
        //fprintf(f_short, "%u\n", qshort); 
        //fprintf(f_timetag, "%u\n", timetag);
        //----------------------------------------------

        //printf("Evt. %d Qshort = %u\n",k,qshort);
        //printf("Evt. %d Qlong = %u, Ch. %u\n",k,qlong,(2*j) + c);
        //printf("Evt. %d Qlong_ch = %u\n",k,qlong_ch);
        
        pdest[event_count++] = qlong_ch;
        pdest[event_count++] = timetag;

      }
      
      //v1730DPP_ForceDataFlush(mvme, base);
      //sleep(1);
    }
  }

  *nentry = event_count;
  
  //----------------- Temporary ------------------
  //fclose(f_qlong);
  //fclose(f_qshort);
  //fclose(f_timetag);
  //----------------------------------------------
  
}
/**********************************************************************/
void v1730DPP_PrintMemoryLocations(MVME_INTERFACE *mvme, uint32_t base)
{
  printf("--------------------------------------------------\n");
  
  for(int i=1; i<8000; i++){ // 2063
    // Do I need to reset the aggregate after the first 2062 lines? No events read after this.
    // Also, why does the first board aggregate only go through 2 events and the second one
    // goes through the correct number, 1023? (Possibly because it attempts to read something
    // from Channel 1, but nothing is connected to it, so it immediately resets knowing that now
    // Channel 0 is the only one with events.)
    
    uint32_t buffer;
    //uint32_t ql;
    
    buffer = v1730DPP_RegisterRead(mvme, base, V1730DPP_EVENT_READOUT_BUFFER);

    printf("Memory Location %u\n",i);
    printf("Decimal: %u\n", buffer);
    // Converting decimal to binary
    printf("Binary: ");
    for (int j=31; j>=0; j--){ // i=31 since buffer is 32 bits
      int k = buffer >> j;
      if (k & 1)
        printf("1");
      else
        printf("0");
    }
    printf("\n\n");
    // qs = (buffer & 0x7FFF);
    // ql = (buffer & 0xFFFF0000)>>16;
    // printf("qlong = %u\n", ql);
    if (i == 10000){ 
      v1730DPP_ForceDataFlush(mvme, base);
    }
  }
  //v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_STOP);
}

/* emacs
 * Local Variables:
 * mode:C
 * mode:font-lock
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

//end
