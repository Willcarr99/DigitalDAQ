/*********************************************************************

  Name:         v1730DPPdrv.h
  Created by:   K.Olchanski / R. Longland / W. Fox

  Contents:     v1730 8-channel 250 MHz 12-bit ADC
                using DPP firmware

  $Id$
                
*********************************************************************/
#ifndef  V1730DPPDRV_INCLUDE_H
#define  V1730DPPDRV_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "v1730DPP.h"
  
  uint32_t v1730DPP_RegisterRead(MVME_INTERFACE *mvme, uint32_t base, int offset);
  void v1730DPP_RegisterWrite(MVME_INTERFACE *mvme, uint32_t base, int offset, uint32_t value);
  //uint32_t v1730DPP_BufferFreeRead(MVME_INTERFACE *mvme, uint32_t a32base);
  //uint32_t v1730DPP_BufferOccupancy(MVME_INTERFACE *mvme, uint32_t a32base, uint32_t channel);
  //uint32_t v1730DPP_BufferFree(MVME_INTERFACE *mvme, uint32_t base, int nbuffer);

  void v1730DPP_setAggregateOrg(MVME_INTERFACE *mvme, uint32_t base, int code);
  void v1730DPP_setAggregateSizeG(MVME_INTERFACE *mvme, uint32_t base, int nevents);
  void v1730DPP_setAggregateSize(MVME_INTERFACE *mvme, uint32_t base, int nevents, int channel);
  void v1730DPP_setRecordLengthG(MVME_INTERFACE *mvme, uint32_t base, int nsamples);
  void v1730DPP_setRecordLength(MVME_INTERFACE *mvme, uint32_t base, int nsamples, int channel);
  void v1730DPP_setDynamicRangeG(MVME_INTERFACE *mvme, uint32_t base, int i);
  void v1730DPP_setDynamicRange(MVME_INTERFACE *mvme, uint32_t base, int i, int channel);

  void v1730DPP_setCFDG(MVME_INTERFACE *mvme, uint32_t base, uint32_t delay, uint32_t percent);
  void v1730DPP_setCFD(MVME_INTERFACE *mvme, uint32_t base, uint32_t delay, uint32_t percent, int channel);
  void v1730DPP_setShortGateG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width);
  void v1730DPP_setShortGate(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel);
  void v1730DPP_setLongGateG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width);
  void v1730DPP_setLongGate(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel);
  void v1730DPP_setGateOffsetG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width);
  void v1730DPP_setGateOffset(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel);
  void v1730DPP_setTriggerHoldoffG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width);
  void v1730DPP_setTriggerHoldoff(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel);
  void v1730DPP_setPreTriggerG(MVME_INTERFACE *mvme, uint32_t base, uint32_t preTrigger);
  void v1730DPP_setPreTrigger(MVME_INTERFACE *mvme, uint32_t base, uint32_t preTrigger, int channel);
  void v1730DPP_setDCOffsetG(MVME_INTERFACE *mvme, uint32_t base, uint32_t offset);
  void v1730DPP_setDCOffset(MVME_INTERFACE *mvme, uint32_t base, uint32_t offset, int channel);
  void v1730DPP_setMeanBaselineCalcG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setMeanBaselineCalc(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void V1730DPP_setFixedBaselineG(MVME_INTERFACE *mvme, uint32_t base, uint32_t baseline);
  void V1730DPP_setFixedBaseline(MVME_INTERFACE *mvme, uint32_t base, uint32_t baseline, int channel);
  void v1730DPP_setThresholdG(MVME_INTERFACE *mvme, uint32_t base, uint32_t threshold);
  void v1730DPP_setThreshold(MVME_INTERFACE *mvme, uint32_t base, uint32_t threshold, int channel);
  void v1730DPP_setChargeZeroSuppressionThresholdG(MVME_INTERFACE *mvme, uint32_t base, uint32_t qthresh);
  void v1730DPP_setChargeZeroSuppressionThreshold(MVME_INTERFACE *mvme, uint32_t base, uint32_t qthresh, int channel);
  void v1730DPP_setTriggerHoldoffG(MVME_INTERFACE *mvme, uint32_t base, uint32_t width);
  void v1730DPP_setTriggerHoldoff(MVME_INTERFACE *mvme, uint32_t base, uint32_t width, int channel);

  void v1730DPP_setGainG(MVME_INTERFACE *mvme, uint32_t base, uint32_t gain);
  void v1730DPP_setGain(MVME_INTERFACE *mvme, uint32_t base, uint32_t gain, int channel);
  void v1730DPP_setDiscriminationModeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setDiscriminationMode(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setTriggerCountingModeG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setTriggerCountingMode(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setTriggerPileupG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setTriggerPileup(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setBaselineCalcRestartG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setBaselineCalcRestart(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setInputSmoothingG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setInputSmoothing(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setTestPulseG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setTestPulse(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setTestPulseRateG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setTestPulseRate(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setPolarityG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setPolarity(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setTriggerHysteresisG(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode);
  void v1730DPP_setTriggerHysteresis(MVME_INTERFACE *mvme, uint32_t base, uint32_t mode, int channel);
  void v1730DPP_setOppositePolarityDetectionG(MVME_INTERFACE *mvme, int32_t base, uint32_t mode);
  void v1730DPP_setOppositePolarityDetection(MVME_INTERFACE *mvme, int32_t base, uint32_t mode, int channel);

  void v1730DPP_CalibrateADC(MVME_INTERFACE *mvme, uint32_t base);
  void v1730DPP_waitForReady(MVME_INTERFACE *mvme, uint32_t base);
  void v1730DPP_EnableChannel(MVME_INTERFACE *mvme, uint32_t base, int channel);
  void v1730DPP_DisableChannel(MVME_INTERFACE *mvme, uint32_t base, int channel);

  void v1730DPP_AcqCtl(MVME_INTERFACE *mvme, uint32_t base, uint32_t command);
  int  v1730DPP_isRunning(MVME_INTERFACE *mvme, uint32_t base);
  int  v1730DPP_EventsReady(MVME_INTERFACE *mvme, uint32_t base);
  int  v1730DPP_EventsFull(MVME_INTERFACE *mvme, uint32_t base);
  int  v1730DPP_EventSize(MVME_INTERFACE *mvme, uint32_t base);
  int  v1730DPP_ReadoutReady(MVME_INTERFACE *mvme, uint32_t base);
  
  void v1730DPP_getChannelStatus(MVME_INTERFACE *mvme, uint32_t base, int channel);
  void v1730DPP_getFirmwareRev(MVME_INTERFACE *mvme, uint32_t base);

  void v1730DPP_SoftwareTrigger(MVME_INTERFACE *mvme, uint32_t base);
  void v1730DPP_ForceDataFlush(MVME_INTERFACE *mvme, uint32_t base);
  void v1730DPP_SoftReset(MVME_INTERFACE *mvme, uint32_t base);
  void v1730DPP_SoftClear(MVME_INTERFACE *mvme, uint32_t base);

  void v1730DPP_DataPrint(MVME_INTERFACE *mvme, uint32_t base);
  void v1730DPP_DataPrint_Updated(MVME_INTERFACE *mvme, uint32_t base, int N);
  void v1730DPP_ReadQLong(MVME_INTERFACE *mvme, uint32_t base, DWORD *pdest, uint32_t *nentry);
  void v1730DPP_PrintMemoryLocations(MVME_INTERFACE *mvme, uint32_t base);

#ifdef __cplusplus
}
#endif

#endif // v1730DPPDRV_INCLUDE_H
