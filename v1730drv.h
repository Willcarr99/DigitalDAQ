/*********************************************************************

  Name:         v1730drv.h
  Created by:   K.Olchanski

  Contents:     v1730 8-channel 250 MHz 12-bit ADC

  $Id$
                
*********************************************************************/
#ifndef  V1730DRV_INCLUDE_H
#define  V1730DRV_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "v1730.h"

uint32_t v1730_RegisterRead(MVME_INTERFACE *mvme, uint32_t a32base, int offset);
//uint32_t v1730_BufferFreeRead(MVME_INTERFACE *mvme, uint32_t a32base);
uint32_t v1730_BufferOccupancy(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t channel);
//uint32_t v1730_BufferFree(MVME_INTERFACE *mvme, uint32_t base, int nbuffer);

void     v1730_AcqCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t operation);
void     v1730_ChannelCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t reg, uint32_t mask);
void     v1730_TrgCtl(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t reg, uint32_t mask);

void     v1730_RegisterWrite(MVME_INTERFACE *mvme, uint32_t a32base, int offset, uint32_t value);
void     v1730_Reset(MVME_INTERFACE *mvme, uint32_t a32base);

void     v1730_Status(MVME_INTERFACE *mvme, uint32_t a32base);
int      v1730_Setup(MVME_INTERFACE *mvme, uint32_t a32base, int mode);
void     v1730_SetupBLT(MVME_INTERFACE *mvme, uint32_t a32base);
void     v1730_info(MVME_INTERFACE *mvme, uint32_t a32base, int *nch, uint32_t *n32w);
uint32_t v1730_SamplesGet(MVME_INTERFACE *mvme, uint32_t a32base);
void     v1730_PostTriggerSet(MVME_INTERFACE *mvme, uint32_t a32base, int post);
uint32_t v1730_DataRead(MVME_INTERFACE *mvme,uint32_t a32base, uint32_t *pdata, uint32_t n32w);
uint32_t v1730_DataBlockRead(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t *pdest, 
			     uint32_t nentry);
void     v1730_ChannelThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold);
//void     v1730_ChannelOUThresholdSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t threshold);
void     v1730_ChannelDACSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t dac);
int      v1730_ChannelDACGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t *dac);
void     v1730_ChannelSet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what, uint32_t that);
uint32_t v1730_ChannelGet(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel, uint32_t what);
void     v1730_ChannelConfig(MVME_INTERFACE *mvme, uint32_t base, uint32_t operation);
void     v1730_ChannelCalibrate(MVME_INTERFACE *mvme, uint32_t base, uint32_t channel);
void     v1730_Align64Set(MVME_INTERFACE *mvme, uint32_t base);


#ifdef __cplusplus
}
#endif

#endif // v1730DRV_INCLUDE_H
