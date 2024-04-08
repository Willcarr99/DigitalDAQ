/********************************************************************\

  Name:         fev1730-DPP.cxx
  Created by:   R. Longland / W. Fox

  Contents:     Frontend for VME DAQ using CAEN ADC, and V1730
                (based on the generic frontend by K.Olcanski)
                This version works with the DPP firmware
$Id$
\********************************************************************/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include "midas.h"
#include "mfe.h"
#include "mvmestd.h"

using namespace std;

//#define HAVE_V792
#define HAVE_V1730

#ifdef HAVE_V792
#include "vme/v792.h"
#endif
#ifdef HAVE_V1730
#include "v1730DPPdrv.h"
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
   const char *frontend_name = "fev1730-DPP";
/* The frontend file name, don't change it */
   const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
   BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
   INT display_period = 000;

/* maximum event size produced by this frontend */
   INT max_event_size = 4100000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
   INT max_event_size_frag = 5*1024*1024;

/* buffer size to hold events */
   INT event_buffer_size = 50000000;

  extern INT run_state;
  extern HNDLE hDB;

/*-- Function declarations -----------------------------------------*/
  INT frontend_init();
  INT frontend_exit();
  INT begin_of_run(INT run_number, char *error);
  INT end_of_run(INT run_number, char *error);
  INT pause_run(INT run_number, char *error);
  INT resume_run(INT run_number, char *error);
  INT frontend_loop();

  INT read_event(char* pevent, INT off);

  void AllDataClear();

  void v1730DPP_LoadSettings();
  void v1730DPP_ApplySettings();
  vector<uint32_t> v1730DPP_str_to_uint32t(string str);
  void v1730DPP_PrintSettings(vector<uint32_t> v, string str, vector <uint32_t> ch, int mode=0);
/*-- Bank definitions ----------------------------------------------*/

/*-- Equipment list ------------------------------------------------*/

  EQUIPMENT equipment[] = {

    {"Trigger",               /* equipment name */
     {1, TRIGGER_ALL,         /* event ID, trigger mask */
      "SYSTEM",               /* event buffer */
      EQ_POLLED,              /* equipment type */
      //EQ_PERIODIC,
      1,                      /* event source */
      "MIDAS",                /* format */
      TRUE,                   /* enabled */
      RO_RUNNING,             /* read only when running */
      500,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      0,                      /* don't log history */
      "", "", "", }
     ,
     read_event,      /* readout routine */
     NULL, NULL,
     NULL,       /* bank list */
    }
    ,

    {""}
  };

/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.

  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.
\********************************************************************/

MVME_INTERFACE *gVme = 0;

int gAdcBase   = 0x200000;
int gV1730Base = 0xABFE0000;


// counters
int nEvtAdc = 0;
int nEvtV1730 = 0;

// Globals for the V1730
#ifdef HAVE_V1730
int nchannels;
uint32_t nsamples;
bool useBLT = true;   // Use BLock Transfers?
#endif

int mvme_read16(int addr)
{
  mvme_set_dmode(gVme, MVME_DMODE_D16);
  return mvme_read_value(gVme, addr);
}

int mvme_read32(int addr)
{
  mvme_set_dmode(gVme, MVME_DMODE_D32);
  return mvme_read_value(gVme, addr);
}

void encodeU32(char*pdata,uint32_t value)
{
  pdata[0] = (value&0x000000FF)>>0;
  pdata[1] = (value&0x0000FF00)>>8;
  pdata[2] = (value&0x00FF0000)>>16;
  pdata[3] = (value&0xFF000000)>>24;
}

uint32_t odbReadUint32(const char*name,int index,uint32_t defaultValue = 0)
{
  int status;
  uint32_t value = 0;
  int size = 4;
  HNDLE hkey=0;
  status = db_get_data_index(hDB,hkey,&value,&size,index,TID_DWORD);
  if (status != SUCCESS)
    {
      cm_msg (MERROR,frontend_name,"Cannot read \'%s\'[%d] from odb, status %d",name,index,status);
      return defaultValue;
    }
  return value;
}

int enable_trigger()
{

  AllDataClear();

  // Turn on the test pulse for channel 0
  //v1730DPP_setTestPulse(gVme, gV1730Base, 1, 0);
  //v1730DPP_setTestPulseRate(gVme, gV1730Base, 0, 0); 
  //v1730DPP_waitForReady(gVme, gV1730Base);
  
  //v1730_AcqCtl(gVme,gV1730Base,V1730_RUN_START);
  v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_START);
  return 0;
}

void AllDataClear()
{

  // Clear the ADC
#ifdef HAVE_V792
  v792_DataClear(gVme, gAdcBase);
  v792_EvtCntReset(gVme,gAdcBase);
#endif

  
#ifdef HAVE_V1730
  // Clear the V1730
  v1730DPP_SoftClear(gVme, gV1730Base);
#endif
  
}

int disable_trigger()
{

#ifdef HAVE_V1730

  // Disable acquisition on V1730
  v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_STOP);

#endif

  return 0;
}

vector<uint32_t> v1730DPP_str_to_uint32t(string str)
{
  vector<uint32_t> vec;
  stringstream ss(str);
  while (ss.good()){
    string substr;
    getline(ss, substr, ',');
    vec.push_back(static_cast<uint32_t>(stoul(substr)));
  }
  return vec;
}

void v1730DPP_PrintSettings(vector<uint32_t> v, string str, vector<uint32_t> ch, int mode)
{
  cout << str << " = ";
  for(int i=0; i<v.size(); i++){
    cout << v[i];
    if (mode==0 && v.size()>1){
      cout << " (Ch " << ch[i] << ")";
    }
    //else if (mode==0 && v.size()==1){
    //  cout << " (All)";
    //}
    if (v.size()>1 && i<v.size()-1){
      cout << ", ";
    }
  }
  cout << "" << endl;
}

void v1730DPP_LoadSettings(){
  
  string enableCh_str, tlong_str, tshort_str, toffset_str, trigHoldOff_str, preTrig_str;
  string inputSmoothing_str, meanBaseline_str, negSignals_str, dRange_str, discrimMode_str;
  string trigCountMode_str, trigPileUp_str, oppPol_str, restartBaseline_str, offset_str;
  string cGain_str, cThresh_str, cCFDDelay_str, cCFDFraction_str, fixedBaseline_str, chargeThresh_str;

  vector<uint32_t> enableCh, tlong, tshort, toffset, trigHoldOff, preTrig, inputSmoothing, meanBaseline;
  vector<uint32_t> negSignals, dRange, discrimMode, trigCountMode, trigPileUp, oppPol, restartBaseline;
  vector<uint32_t> offset, cGain, cThresh, cCFDDelay, cCFDFraction, fixedBaseline, chargeThresh;

  ifstream f("settings-DPP.dat");
  if (!f){
    // Print an error and exit
    cerr << "ERROR! settings-DPP.dat could not be opened" << endl;
    exit(1);
  }
  
  printf("Reading settings-DPP.dat...\n\n");

  // Skip header lines
  for(int i=0;i<15;i++){f.ignore(200,'\n');}

  // Assign strings
  f >> enableCh_str;
  f.ignore(200,'\n');
  f >> tlong_str;
  f.ignore(200,'\n');
  f >> tshort_str;
  f.ignore(200,'\n');
  f >> toffset_str;
  f.ignore(200,'\n');
  f >> trigHoldOff_str;
  f.ignore(200,'\n');
  f >> preTrig_str;
  f.ignore(200,'\n');
  f >> cGain_str;
  f.ignore(200,'\n');
  f >> negSignals_str;
  f.ignore(200,'\n');
  f >> offset_str;
  f.ignore(200,'\n');
  f >> cThresh_str;
  f.ignore(200,'\n');
  f >> cCFDDelay_str;
  f.ignore(200,'\n');
  f >> cCFDFraction_str;
  f.ignore(200,'\n');
  
  f.ignore(200,'\n');
  f >> dRange_str;
  f.ignore(200,'\n');
  f >> inputSmoothing_str;
  f.ignore(200,'\n');
  f >> meanBaseline_str;
  f.ignore(200,'\n');
  f >> fixedBaseline_str;
  f.ignore(200,'\n');
  f >> restartBaseline_str;
  f.ignore(200,'\n');
  f >> discrimMode_str;
  f.ignore(200,'\n');
  f >> trigCountMode_str;
  f.ignore(200,'\n');
  f >> trigPileUp_str;
  f.ignore(200,'\n');
  f >> oppPol_str;
  f.ignore(200,'\n');
  f >> chargeThresh_str;

  f.close();

  // Convert strings to 32-bit integers
  enableCh = v1730DPP_str_to_uint32t(enableCh_str);
  tlong = v1730DPP_str_to_uint32t(tlong_str);
  tshort = v1730DPP_str_to_uint32t(tshort_str);
  toffset = v1730DPP_str_to_uint32t(toffset_str);
  trigHoldOff = v1730DPP_str_to_uint32t(trigHoldOff_str);
  preTrig = v1730DPP_str_to_uint32t(preTrig_str);
  cGain = v1730DPP_str_to_uint32t(cGain_str);
  negSignals = v1730DPP_str_to_uint32t(negSignals_str);
  offset = v1730DPP_str_to_uint32t(offset_str);
  cThresh = v1730DPP_str_to_uint32t(cThresh_str);
  cCFDDelay = v1730DPP_str_to_uint32t(cCFDDelay_str);
  cCFDFraction = v1730DPP_str_to_uint32t(cCFDFraction_str);
  dRange = v1730DPP_str_to_uint32t(dRange_str);
  inputSmoothing = v1730DPP_str_to_uint32t(inputSmoothing_str);
  meanBaseline = v1730DPP_str_to_uint32t(meanBaseline_str);
  fixedBaseline = v1730DPP_str_to_uint32t(fixedBaseline_str);
  restartBaseline = v1730DPP_str_to_uint32t(restartBaseline_str);
  discrimMode = v1730DPP_str_to_uint32t(discrimMode_str);
  trigCountMode = v1730DPP_str_to_uint32t(trigCountMode_str);
  trigPileUp = v1730DPP_str_to_uint32t(trigPileUp_str);
  oppPol = v1730DPP_str_to_uint32t(oppPol_str);
  chargeThresh = v1730DPP_str_to_uint32t(chargeThresh_str);

  printf("DPP Settings:\n");
  printf("--------------------------------------------------\n\n");

  // Print settings
  v1730DPP_PrintSettings(enableCh, "Channels Enabled", enableCh, 1);
  v1730DPP_PrintSettings(tlong, "Long gate", enableCh);
  v1730DPP_PrintSettings(tshort, "Short gate", enableCh);
  v1730DPP_PrintSettings(toffset, "Gate offset", enableCh);
  v1730DPP_PrintSettings(trigHoldOff, "Trigger Hold-Off", enableCh);
  v1730DPP_PrintSettings(preTrig, "Pre-Trigger", enableCh);
  v1730DPP_PrintSettings(cGain, "Gain", enableCh);
  v1730DPP_PrintSettings(negSignals, "Negative Signals", enableCh);
  v1730DPP_PrintSettings(offset, "DC Offset", enableCh);
  v1730DPP_PrintSettings(cThresh, "Threshold", enableCh);
  v1730DPP_PrintSettings(cCFDDelay, "CFD Delay", enableCh);
  v1730DPP_PrintSettings(cCFDFraction, "CFD Fraction", enableCh);
  v1730DPP_PrintSettings(dRange, "Dynamic Range", enableCh);
  v1730DPP_PrintSettings(inputSmoothing, "Input Smoothing", enableCh);
  v1730DPP_PrintSettings(meanBaseline, "Mean Baseline Calculation", enableCh);
  v1730DPP_PrintSettings(fixedBaseline, "Fixed Baseline", enableCh);
  v1730DPP_PrintSettings(restartBaseline, "Restart Baseline after Long Gate", enableCh);
  v1730DPP_PrintSettings(discrimMode, "Discrimination Mode", enableCh);
  v1730DPP_PrintSettings(trigCountMode, "Trigger Counting Mode", enableCh);
  v1730DPP_PrintSettings(trigPileUp, "Pile Up Counted as Trigger", enableCh);
  v1730DPP_PrintSettings(oppPol, "Detect Opposite Polarity Signals", enableCh);
  v1730DPP_PrintSettings(chargeThresh, "Charge Zero Suppression Threshold", enableCh);
  printf("\n--------------------------------------------------\n");
}

void v1730DPP_ApplySettings(){
  
  // General setup
  v1730DPP_RegisterWrite(gVme, gV1730Base, 0x8000, 0x800C0110); // 0x800C0110 - Board Config
  //v1730DPP_RegisterWrite(gVme, gV1730Base, 0x8084, 0x); // DPP Algorithm Control
  v1730DPP_setAggregateOrg(gVme, gV1730Base, 3);
  v1730DPP_setAggregateSizeG(gVme, gV1730Base, 1023);   // 1023 events per aggregate
  v1730DPP_setRecordLengthG(gVme, gV1730Base, 64);    // Don't collect waveforms
  v1730DPP_setDynamicRangeG(gVme, gV1730Base, dRange);    //(0 = 2 Vpp)  (1 = 0.5 Vpp)
  
  // Disable all channels
  for(int i=0; i<15; i++){
    v1730DPP_DisableChannel(gVme, gV1730Base, i);
  }

  // Enable channels specified in settings
  for(int i=0; i<enableCh.size(); i++){
    v1730DPP_EnableChannel(gVme, gV1730Base, enableCh[i]);
  }

  // *****************************************************
  // Apply settings
  // *****************************************************

  // Gain
  if (cGain.size()==1){
    v1730DPP_setGainG(gVme, gV1730Base, cGain[0]);
  }
  else {
    for(int i=0; i<cGain.size(); i++){
      v1730DPP_setGain(gVme, gV1730Base, cGain[i], ch[i]);
    }
  }

  // Input Smoothing
  if (inputSmoothing.size()==1){
    v1730DPP_setInputSmoothingG(gVme, gV1730Base, inputSmoothing[0]);
  }
  else {
    for(int i=0; i<inputSmoothing.size(); i++){
      v1730DPP_setInputSmoothing(gVme, gV1730Base, inputSmoothing[i], ch[i]);
    }
  }

  // Polarity
  if (negSignals.size()==1){
    v1730DPP_setPolarityG(gVme, gV1730Base, negSignals[0]);
  }
  else {
    for(int i=0; i<negSignals.size(); i++){
      v1730DPP_setPolarity(gVme, gV1730Base, negSignals[i], ch[i]);
    }
  }

  // Mean Baseline Calculation
  if (meanBaseline.size()==1){
    v1730DPP_setMeanBaselineCalcG(gVme, gV1730Base, meanBaseline[0]);
  }
  else {
    for(int i=0; i<meanBaseline.size(); i++){
      v1730DPP_setMeanBaselineCalc(gVme, gV1730Base, meanBaseline[i], ch[i]);
    }
  }

  // Fixed Baseline
  if (fixedBaseline.size()==1){
    V1730DPP_setFixedBaselineG(gVme, gV1730Base, fixedBaseline[0]);
  }
  else {
    for(int i=0; i<fixedBaseline.size(); i++){
      V1730DPP_setFixedBaseline(gVme, gV1730Base, fixedBaseline[i], ch[i]);
    }
  }

  // Baseline Calculation Restart
  if (restartBaseline.size()==1){
    v1730DPP_setBaselineCalcRestartG(gVme, gV1730Base, restartBaseline[0]);
  }
  else {
    for(int i=0; i<restartBaseline.size(); i++){
      v1730DPP_setBaselineCalcRestart(gVme, gV1730Base, restartBaseline[i], ch[i]);
    }
  }

  // Opposite Polarity Detection
  if (oppPol.size()==1){
    v1730DPP_setOppositePolarityDetectionG(gVme, gV1730Base, oppPol[0]);
  }
  else {
    for(int i=0; i<oppPol.size(); i++){
      v1730DPP_setOppositePolarityDetection(gVme, gV1730Base, oppPol[i], ch[i]);
    }
  }

  // Charge Zero Suppression Threshold
  if (chargeThresh.size()==1){
    v1730DPP_setChargeZeroSuppressionThresholdG(gVme, gV1730Base, chargeThresh[0]);
  }
  else {
    for(int i=0; i<chargeThresh.size(); i++){
      v1730DPP_setChargeZeroSuppressionThreshold(gVme, gV1730Base, chargeThresh[i], ch[i]);
    }
  }

  // CFD Delay and Fraction
  if (cCFDDelay.size()==1 && cCFDFraction.size()==1){
    v1730DPP_setCFDG(gVme, gV1730Base, cCFDDelay[0], cCFDFraction[0]);
  }
  else if (cCFDDelay.size()==1 && cCFDFraction.size()>1){
    for(int i=0; i<cCFDFraction.size(); i++){
      v1730DPP_setCFD(gVme, gV1730Base, cCFDDelay[0], cCFDFraction[i], ch[i]);
    }
  }
  else if (cCFDDelay.size()>1 && cCFDFraction.size()==1){
    for(int i=0; i<cCFDDelay.size(); i++){
      v1730DPP_setCFD(gVme, gV1730Base, cCFDDelay[i], cCFDFraction[0], ch[i]);
    }
  }
  else if (cCFDDelay.size() == cCFDFraction.size()){
    for(int i=0; i<cCFDDelay.size(); i++){
      v1730DPP_setCFD(gVme, gV1730Base, cCFDDelay[i], cCFDFraction[i], ch[i]);
    }
  }
  else{
    cout << "Error: CFD Delay and Fraction settings cannot be applied" << endl;
  }

  // Long Gate
  if (tlong.size()==1){
    v1730DPP_setLongGateG(gVme, gV1730Base, tlong[0]);
  }
  else {
    for(int i=0; i<tlong.size(); i++){
      v1730DPP_setLongGate(gVme, gV1730Base, tlong[i], ch[i]);
    }
  }

  // Short Gate
  if (tshort.size()==1){
    v1730DPP_setShortGateG(gVme, gV1730Base, tshort[0]);
  }
  else {
    for(int i=0; i<tshort.size(); i++){
      v1730DPP_setShortGate(gVme, gV1730Base, tshort[i], ch[i]);
    }
  }

  // Gate Offset
  if (toffset.size()==1){
    v1730DPP_setGateOffsetG(gVme, gV1730Base, toffset[0]);
  }
  else {
    for(int i=0; i<toffset.size(); i++){
      v1730DPP_setGateOffset(gVme, gV1730Base, toffset[i], ch[i]);
    }
  }

  // Trigger Holdoff Width
  if (trigHoldOff.size()==1){
    v1730DPP_setTriggerHoldoffG(gVme, gV1730Base, trigHoldOff[0]);
  }
  else {
    for(int i=0; i<trigHoldOff.size(); i++){
      v1730DPP_setTriggerHoldoff(gVme, gV1730Base, trigHoldOff[i], ch[i]);
    }
  }

  // Pre Trigger Width
  if (preTrig.size()==1){
    v1730DPP_setPreTriggerG(gVme, gV1730Base, preTrig[0]);
  }
  else {
    for(int i=0; i<preTrig.size(); i++){
      v1730DPP_setPreTrigger(gVme, gV1730Base, preTrig[i], ch[i]);
    }
  }

  // Threshold
  if (cThresh.size()==1){
    v1730DPP_setThresholdG(gVme, gV1730Base, cThresh[0]);
  }
  else {
    for(int i=0; i<cThresh.size(); i++){
      v1730DPP_setThreshold(gVme, gV1730Base, cThresh[i], ch[i]);
    }
  }

  // DC Offset
  if (offset.size()==1){
    v1730DPP_setDCOffsetG(gVme, gV1730Base, offset[0]);
  }
  else {
    for(int i=0; i<offset.size(); i++){
      v1730DPP_setDCOffset(gVme, gV1730Base, offset[i], ch[i]);
    }
  }

  // Discrimination Mode (LED or CFD)
  if (discrimMode.size()==1){
    v1730DPP_setDiscriminationModeG(gVme, gV1730Base, discrimMode[0]);
  }
  else {
    for(int i=0; i<discrimMode.size(); i++){
      v1730DPP_setDiscriminationMode(gVme, gV1730Base, discrimMode[i], ch[i]);
    }
  }

  // Trigger Counting Mode (Accepted Self-Triggers Only or All Self-Triggers)
  if (trigCountMode.size()==1){
    v1730DPP_setTriggerCountingModeG(gVme, gV1730Base, trigCountMode[0]);
  }
  else {
    for(int i=0; i<trigCountMode.size(); i++){
      v1730DPP_setTriggerCountingMode(gVme, gV1730Base, trigCountMode[i], ch[i]);
    }
  }

  // Trigger Pile-Up
  if (trigPileUp.size()==1){
    v1730DPP_setTriggerPileupG(gVme, gV1730Base, trigPileUp[0]);
  }
  else {
    for(int i=0; i<trigPileUp.size(); i++){
      v1730DPP_setTriggerPileup(gVme, gV1730Base, trigPileUp[i], ch[i]);
    }
  }
}

INT init_vme_modules()
{

#ifdef HAVE_V792

  /*
  printf("---------------------------\n");
  printf("------ CAEN V785 ADC ------\n\n");
  v792_DataClear(gVme, gAdcBase);
  */

  // reset the module, event counter, and data
  v792_SoftReset(gVme, gAdcBase);
  v792_EvtCntReset(gVme,gAdcBase);
  v792_DataClear(gVme, gAdcBase);

  // --- Custom setup --- 
  // Allow writing the over- and under-threshold events (so we get a
  // number even if it's a bad event). This is important for event
  // matching.
  v792_Write16(gVme, gAdcBase, V792_BIT_SET2_RW, 0x8);
  //  v792_EmptyEnable(gVme, gAdcBase);

  // Setup the thresholds
  printf("Setting thresholds\n");
  WORD threshold[32];
  for (int i=0;i<32;i++) {
    threshold[i] = 0x0000;
  }
  v792_ThresholdWrite(gVme, gAdcBase, threshold);

  // printf("Getting V785 Status\n");
  // v792_Status(gVme,gAdcBase);
  // printf("--------------------------\n");

#endif

#ifdef HAVE_V1730
  printf("--------------------------------------------------\n");
  printf("--------- CAEN V1730 Digitizer DPP mode ----------\n");

  uint32_t status;

  v1730DPP_LoadSettings();

  // Is the digitizer working?
  v1730DPP_getFirmwareRev(gVme, gV1730Base);

  // Reset the module
  printf("Reset module\n");
  v1730DPP_SoftReset(gVme, gV1730Base);
  
  sleep(1);

  v1730DPP_ApplySettings();
  
  // Make sure the acquisition is stopped
  v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_STOP);

  // Clear the data
  v1730DPP_SoftClear(gVme, gV1730Base);

  // Get the board and algorithm configs
  printf("Board config: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x8000));
  printf("DPP Algorithm 1 config: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x1080));
  printf("DPP Algorithm 2 config: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x1084));
  
  // Diagnostics with register values
  //printf("Shaped Trigger Width: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x8070));
  //printf("Trigger Hold-Off Width: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x8074));
  
  // Check a single channel
  v1730DPP_getChannelStatus(gVme, gV1730Base, 1);

  // Calibrate the ADC
  v1730DPP_CalibrateADC(gVme, gV1730Base);

  // Check a single channel
  v1730DPP_getChannelStatus(gVme, gV1730Base, 1);

  // Enable some channels (Moved to v1730DPP.c - ReadAndApplySettings())
  // ---------------------------------------------
  //for(int i=0; i<8; i++){
  //  v1730DPP_DisableChannel(gVme, gV1730Base, i);
  //}
  // ---------------------------------------------
  
  //for(int i=0; i<1; i++){
  //  v1730DPP_EnableChannel(gVme, gV1730Base, i);
  //}
  
  // ---------------------------------------------
  //v1730DPP_EnableChannel(gVme, gV1730Base, 0); // First NaI Detector - Anode signal - Neg Polarity (Eu-152 source) connected to ch 0
  //v1730DPP_EnableChannel(gVme, gV1730Base, 1); // Second NaI Detector - Anode signal - Neg Polarity (Eu-152 source) connected to ch 1
  // ---------------------------------------------
  
  //v1730DPP_EnableChannel(gVme, gV1730Base, 0); // Emulator (Fixed energy) connected to ch 0 - Neg Polarity
  //v1730DPP_EnableChannel(gVme, gV1730Base, 1); // NaI Detector - Dynode signal - Pos Polarity (Eu-152 source)  connected to ch 1
  //v1730DPP_EnableChannel(gVme, gV1730Base, 1); // Emulator (Cobalt spectrum) connected to ch 1
  //v1730DPP_EnableChannel(gVme, gV1730Base, 1); // Emulator (Fixed energy) connected to ch 1 - Neg Polarity
  
  // Turn on the test pulse for channel 0
  // Make sure a detector is not connected to the channel
  // May need to comment out ForceDataFlush (and buffer) in DataPrint_Updated (and ReadQLong?)
  //v1730DPP_setTestPulse(gVme, gV1730Base, 1, 0);
  //v1730DPP_setTestPulseRate(gVme, gV1730Base, 0, 0); 
  //v1730DPP_waitForReady(gVme, gV1730Base);
  
  // Run for a few seconds
  cout << "--------------------------------------------------" << endl;
  cout << "Events ready: " << v1730DPP_EventsReady(gVme, gV1730Base) << endl;
  cout << "Waiting a few seconds to finish initialization..." << endl;
  // Need to sleep for a few seconds before ACQ Control Starts! (Fixed artificial events issue)
  sleep(10);
  cout << "Running for a few seconds!" << endl;
  v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_START);
  
  for(int i=0; i<12; i++){ //12
    sleep(1);
    cout << "Is running: " << v1730DPP_isRunning(gVme, gV1730Base) << endl;
  }

  v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_STOP);
  
  cout << "Done!" << endl;
  cout << "Is running: " << v1730DPP_isRunning(gVme, gV1730Base) << endl;

  cout << "Events ready: " << v1730DPP_EventsReady(gVme, gV1730Base) << endl;
  
  v1730DPP_ForceDataFlush(gVme, gV1730Base);
  
  cout << "Readout ready: " << v1730DPP_ReadoutReady(gVme, gV1730Base) << endl;

  cout << "\n";
  sleep(3); //3

  //while(v1730DPP_EventsReady(gVme, gV1730Base)){
  v1730DPP_DataPrint_Updated(gVme, gV1730Base, 8); // Will's updated version
  // }
  
  //while(v1730DPP_EventsReady(gVme, gV1730Base)){
  //for(int i=0; i<10; i++){
  //v1730DPP_DataPrint(gVme, gV1730Base);
  //}
  //}
  
  //v1730DPP_ForceDataFlush(gVme, gV1730Base);
  //sleep(1);
  //cout << "Readout ready: " << v1730DPP_ReadoutReady(gVme, gV1730Base) << endl;

  /*
    cout << "Trying to trigger a few times" << endl;
  v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_START);
  v1730DPP_SoftwareTrigger(gVme, gV1730Base);
  v1730DPP_SoftwareTrigger(gVme, gV1730Base);
  v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_STOP);
  v1730DPP_ForceDataFlush(gVme, gV1730Base);
  sleep(1);
  cout << "Events ready: " << v1730DPP_EventsReady(gVme, gV1730Base) << endl;
  cout << "Readout ready: " << v1730DPP_ReadoutReady(gVme, gV1730Base) << endl;
  */
  
  // reset amd setup the digitizer
  //  v1730_AcqCtl(gVme,gV1730Base,V1730_RUN_STOP);
  //  v1730_Reset(gVme,gV1730Base);
  //  v1730_Setup(gVme,gV1730Base,1);  // standard settings
  //  if(useBLT)
  //    v1730_SetupBLT(gVme,gV1730Base);

  // Now that the buffers are setup, print a summary
  //  int nchannels;
  uint32_t n32word;
  //  nsamples = v1730_SamplesGet(gVme, gV1730Base);

  // We need to reduce the number of samples to prevent buffer
  // overwrites (custom event size)
  //  v1730_info(gVme, gV1730Base, &nchannels, &n32word); 

  // Set up acquisition modes
  // Run control by register (SW control)
  //  v1730_AcqCtl(gVme, gV1730Base, V1730_REGISTER_RUN_MODE);

  
  // Calibrate the channels
  /*
  printf("\n --- Calibrating channels\n");
  int chanmask = 0xFFFF & v1730_RegisterRead(gVme, gV1730Base, V1730_CHANNEL_EN_MASK);
  for(int i=0; i<16; i++)
    if(chanmask & (1<<i))
      v1730_ChannelCalibrate(gVme, gV1730Base, i);
  */
  // set the offsets and thresholds
  /*
  printf("\n --- Setting DC offsets\n");
  for(int i=0; i<4; i++){
    v1730_ChannelDACSet(gVme,gV1730Base,i,offset);
  }
  */
  //sleep(2);   // wait a few seconds for things to stablise

  /*  
  printf("\n --- Setting the thresholds\n");
  for(int i=0; i<4; i++){
    v1730_ChannelThresholdSet(gVme,gV1730Base,i,cThresh);
    cThresh = v1730_ChannelGet(gVme,gV1730Base,i,V1730_CHANNEL_THRESHOLD);
    printf("Channel %d  : Threshold = %d\n",i,cThresh);
  }
  
  if(negSignals){
    // Set trigger to be underthreshold
    printf("\n --- Triggering on negative signals\n");
    v1730_ChannelConfig(gVme, gV1730Base, V1730_TRIGGER_UNDERTH);
  }
  */
  // TODO - Read the temperatures
  

  /*
  // test by turning on the module for a second
  printf("\n --- Acquiring a second of data\n");
  v1730_AcqCtl(gVme,gV1730Base,V1730_RUN_START);
  // write a software trigger
  sleep(1);
  v1730_RegisterWrite(gVme, gV1730Base, V1730_SW_TRIGGER, 1);
  usleep(100);

  // Read stuff
  uint32_t   data[100000];
  INT  nentry, nsize, npt;
  npt = nsamples;

  uint32_t n32read;

  printf("\n\n");
  printf("--- Reading V1730! ---\n");
  
  nentry = v1730_RegisterRead(gVme, gV1730Base, V1730_EVENT_STORED);
  printf("There are %d stored events",nentry);
  nsize = v1730_RegisterRead(gVme, gV1730Base, V1730_EVENT_SIZE);
  printf(" with size = %d\n",nsize);
  
  n32word = 4 + npt*nchannels/2;
  printf("We want to read %d samples in %d channels so n32read = %d\n",npt,nchannels,n32word);

  //  uint32_t channel=0; // read channel 0
  
  do {
    status = v1730_RegisterRead(gVme, gV1730Base, V1730_ACQUISITION_STATUS) & 0xFFFF;
    printf("Acq Status1:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  if(useBLT){
    n32read = v1730_DataBlockRead(gVme, gV1730Base, &(data[0]), n32word);
    n32read = n32word - n32read;
    printf("Finished block reading with n32read = %d\n",n32read);
  } else {
    n32read = v1730_DataRead(gVme, gV1730Base, &(data[0]), n32word);
    printf("Finished reading with n32read = %d\n",n32read);
  }
  
  // Look at first 20 samples
  printf("First 20 samples are:\n");
  int s=0;
  uint32_t val;
  for(int i=4; i<14; i++){
    val=data[i]>>16 & 0x3FFF;
    printf("s[%d]: %x: %d    ",s,val,val);
    s++;
    val=data[i] & 0x3FFF;
    printf("s[%d]: %x: %d\n",s,val,val);
    s++;
  }
  */

#endif

  return SUCCESS;
}

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
  int status;


  cout << "Initialising frontend" << endl;

  setbuf(stdout,NULL);
  setbuf(stderr,NULL);

  status = mvme_open(&gVme,0);
  status = mvme_set_am(gVme, MVME_AM_A24_ND);
  status = mvme_set_dmode(gVme, MVME_DMODE_D32);

  init_vme_modules();

  // Disable triggering until the beginning of a run
  // (this doesn't do anything at the moment, but everything is
  // cleared at the beginning of the run anyway)
  // disable_trigger();

  return status;
}

static int gHaveRun = 0;

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  gHaveRun = 0;
  disable_trigger();

  mvme_close(gVme);

  return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  gHaveRun = 1;
  printf("begin run %d\n",run_number);

  //init_vme_modules();

  enable_trigger();

  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  static bool gInsideEndRun = false;

  if (gInsideEndRun)
    {
      printf("breaking recursive end_of_run()\n");
      return SUCCESS;
    }

  gInsideEndRun = true;

  gHaveRun = 0;
  printf("end run %d\n",run_number);
  v1730DPP_ForceDataFlush(gVme, gV1730Base);
  disable_trigger();

  gInsideEndRun = false;

  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  gHaveRun = 0;
  disable_trigger();
  return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  gHaveRun = 1;
  enable_trigger();
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop()
{
  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
  return SUCCESS;
}

/********************************************************************\

  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/
int poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{

  // Look for an event in the ADC before going any further
  for (int i=0 ; i<count ; i++){
    //    int lam = v792_DataReady(gVme,gAdcBase);
    //    int nV1730 = v1730_RegisterRead(gVme, gV1730Base, V1730_EVENT_STORED);
    int nV1730 = 1;
    int nV785 = 1;
#ifdef HAVE_V1730
    //nV1730 = v1730DPP_EventsFull(gVme, gV1730Base);
    //nV1730 = v1730DPP_ReadoutReady(gVme, gV1730Base);
    nV1730 = v1730DPP_EventsReady(gVme, gV1730Base);
#endif
#ifdef HAVE_V792
    nV785 = v792_DataReady(gVme,gAdcBase);
#endif
    if (nV1730 && nV785){
      if (!test){
        //event_full = TRUE;
        return TRUE;
      }
    }
  }
  //event_full = FALSE;
  return FALSE;

}

/*-- Interrupt configuration ---------------------------------------*/
int interrupt_configure(INT cmd, INT source, PTYPE adr)
{
   switch (cmd) {
   case CMD_INTERRUPT_ENABLE:
     break;
   case CMD_INTERRUPT_DISABLE:
     break;
   case CMD_INTERRUPT_ATTACH:
     break;
   case CMD_INTERRUPT_DETACH:
     break;
   }
   return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

#ifdef HAVE_V792
int read_v792(int base, const char* bname, char* pevent, int nchan)
{
  int wcount = 0, eventcount=0, readcount=0;
  DWORD data[100];
  WORD *pdata;
  DWORD counter = 0;

  /* Event counter */
  v792_EvtCntRead(gVme, base, &counter);

  //  printf("Reading MEB from V785\n");
  //printf("V785 Event counter = %d\n",counter);

  while(v792_DataReady(gVme,base)){

    //printf("v785 entry %d\n",eventcount);
    eventcount++;
    nEvtAdc++;
    
    /* create ADCS bank */
    bk_create(pevent, bname, TID_WORD, (void **)&pdata);
    
    /* Read Single Event */
    wcount=0;
    v792_EventRead(gVme, base, data, &wcount);


    // Figure out what channels are firing and arrange the data accordingly
    for(int i=0; i<nchan; i++)
      pdata[i]=0;
    //printf("wcount = %d\n",wcount);
    for (int i=0; i<wcount; i++)
      {
	uint32_t w = data[i];
	if (((w>>24)&0x7) != 0) continue;
	int chan = (w>>16)&0x1F;
	int val  = (w&0x1FFF);
	//	printf("i: %d  chn: %d  val: %d\n",i,chan,val);
	pdata[chan] = val;
      }

    pdata += nchan;
    readcount += wcount;
    
    // Close the bank
    bk_close(pevent, pdata);
    
  } // end of while(v792_DataReady())

  //  printf("Read %d events\n",eventcount);

  return eventcount;
}
#endif

#ifdef HAVE_V1730
uint32_t read_v1730(int base, const char* bname, char* pevent, int eventcount)
{

  uint32_t wcount = 0, totaln32read = 0;
  int nEvtV1730 = 0; //totaln32read=0; //, eventcount=0, readcount=0;
  DWORD data[20000]; //20000
  DWORD *pdata;
  DWORD counter = 0;

  //while(v1730DPP_EventsFull(gVme,gV1730Base)){
  while(v1730DPP_EventsReady(gVme,gV1730Base)){
    eventcount++;
    
    /* create Digitizer bank */
    bk_create(pevent, bname, TID_DWORD, (void **)&pdata);

    v1730DPP_ReadQLong(gVme, gV1730Base, data, &wcount);

    // Loop through all of the stored events
    for(uint32_t entry=0; entry<wcount; entry++){
      nEvtV1730++;

      // Put the data into the pevent array. Now pdata contains qlong and ch number
      pdata[entry] = data[entry];
      //printf("v1730 entry %u: %u\n",entry,data[entry]);
    }
    pdata += wcount;
    // close the bank
    //  cout << "closing the bank" << endl;
    bk_close(pevent,pdata);

    totaln32read += wcount;
    
  }
  
  return totaln32read;
}
#endif

INT read_event(char* pevent, INT off)
{

  uint32_t dsize, eventcount = 0;
  //printf("read event!\n");
  BOOL have_data = FALSE;

  /* init bank structure */
  bk_init32(pevent);

#ifdef HAVE_V792
  dsize = read_v792(gAdcBase,"ADC1",pevent,32);
  eventcount = dsize;
  if(dsize > 0)have_data = TRUE;
#endif
#ifdef HAVE_V1730
  dsize = read_v1730(gV1730Base,"V730",pevent,eventcount);
  if(dsize > 0)have_data = TRUE;
#endif

  //printf("%d\t%d\n",nEvtAdc, nEvtV1730);

  // See if the event counters agree
#ifdef HAVE_V792
  if(nEvtAdc != nEvtV1730){
    cm_msg(MERROR, frontend_name, 
	   "VME trigger problem: ADC Events: %d   V1730 Events: %d", nEvtAdc, nEvtV1730);

    // Clear ADC and Digitizer
    //disable_trigger();
    AllDataClear();
    //enable_trigger();
    have_data = FALSE;
    //   nEvtAdc = min(nEvtAdc,nEvtV1730);
    nEvtV1730 = nEvtAdc;
    
  } 
#endif

  INT retval = have_data ? bk_size(pevent) : 0;
  //printf("retval = %u \n", retval);
  //printf("dsize = %u \n", dsize);
  return retval;
}

