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
#include <cmath>
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

  void v1730DPP_LoadSettings(bool print_settings = true, bool print_default = false, bool print_new = false);
  vector<uint32_t> v1730DPP_str_to_uint32t(string str);
  void v1730DPP_PrintSettings(vector<uint32_t> v, string str, vector <uint32_t> ch, int mode=0, const vector<int> couple_indices = vector<int>());
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
int extras; // 32-bit EXTRAS word recording. Assigned in LoadSettings(). 0=disabled, 1=enabled
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
  char* end;
  while (ss.good()){
    string substr;
    getline(ss, substr, ',');
    //vec.push_back(static_cast<uint32_t>(stoul(substr))); // std::stoul introduced in C++11
    vec.push_back(static_cast<uint32_t>(strtoul(&substr[0], &end, 10))); // std::strtoul in C standard library <stdlib.h>
  }
  return vec;
}

void v1730DPP_PrintSettings(vector<uint32_t> v, string str, vector<uint32_t> ch, int mode, const vector<int> couple_indices)
{
  cout << str << " = ";
  for(uint32_t i=0; i<v.size(); i++){
    cout << v[i];
    if (mode==0 && v.size()>1){
      cout << " (Ch " << ch[i] << ")";
    }
    //else if (mode==0 && v.size()==1){
    //  cout << " (All)";
    //}
    else if (mode==2 && v.size()>1){
      cout << " (Ch " << ch[couple_indices[i]] << ")";
    }
    if (v.size()>1 && i<v.size()-1){
      if (mode==3){
        cout << "\nERROR: Setting must be applied to all channels. Use a single value." << endl;
        return;
      }
      else{
        cout << ", ";
      }
    }
  }
  cout << "" << endl;
}

void v1730DPP_LoadSettings(bool print_settings, bool print_default, bool print_new){

  string enableCh_str, tlong_str, tshort_str, toffset_str, preTrig_str, trigHoldOff_str;
  string inputSmoothing_str, meanBaseline_str, negSignals_str, dRange_str, discrimMode_str;
  string trigPileUp_str, oppPol_str, restartBaseline_str, offset_str, cGain_str, cThresh_str;
  string cCFDDelay_str, cCFDFraction_str, fixedBaseline_str, chargeThresh_str, testPulse_str;
  string trigHyst_str, chargePed_str, pileupRej_str, overRangeRej_str, selfTrigAcq_str;
  string trigMode_str, enableTrigProp_str, trigCountMode_str, shapedTrig_str, latTime_str, localTrigMode_str;
  string localTrigValMode_str, addLocalTrigValMode_str, trigValMask1_str, trigValMask2_str, extrasRec_str, extrasMode_str;

  vector<uint32_t> enableCh, tlong, tshort, toffset, preTrig, trigHoldOff, inputSmoothing, meanBaseline;
  vector<uint32_t> negSignals, dRange, discrimMode, trigPileUp, oppPol, restartBaseline, offset;
  vector<uint32_t> cGain, cThresh, cCFDDelay, cCFDFraction, fixedBaseline, chargeThresh, testPulse;
  vector<uint32_t> pileupRej, overRangeRej, selfTrigAcq, chargePed, trigHyst;
  vector<uint32_t> trigMode, enableTrigProp, trigCountMode, shapedTrig, latTime, localTrigMode;
  vector<uint32_t> localTrigValMode, addLocalTrigValMode, trigValMask1, trigValMask2, extrasRec, extrasMode;

  std::vector<int> couples;
  std::vector<int> couple_indices;

  // *****************************************************
  // Read settings
  // *****************************************************

  ifstream f("settings-DPP.dat");
  if (!f){
    // Print an error and exit
    cerr << "ERROR! settings-DPP.dat could not be opened" << endl;
    exit(1);
  }

  printf("\nReading settings-DPP.dat...\n\n");

  // Skip header lines
  for(int i=0;i<20;i++){f.ignore(200,'\n');}

  // Assign strings
  f >> enableCh_str;
  f.ignore(200,'\n');
  f >> tlong_str;
  f.ignore(200,'\n');
  f >> tshort_str;
  f.ignore(200,'\n');
  f >> toffset_str;
  f.ignore(200,'\n');
  f >> preTrig_str;
  f.ignore(200,'\n');
  f >> trigHoldOff_str;
  f.ignore(200,'\n');
  f >> cGain_str;
  f.ignore(200,'\n');
  f >> dRange_str;
  f.ignore(200,'\n');
  f >> negSignals_str;
  f.ignore(200,'\n');
  f >> offset_str;
  f.ignore(200,'\n');
  f >> cThresh_str;
  f.ignore(200,'\n');
  f >> discrimMode_str;
  f.ignore(200,'\n');
  f >> cCFDDelay_str;
  f.ignore(200,'\n');
  f >> cCFDFraction_str;
  f.ignore(200,'\n');
  f >> extrasRec_str;
  f.ignore(200,'\n');
  f >> extrasMode_str;
  f.ignore(200,'\n');

  f.ignore(200,'\n');
  f >> inputSmoothing_str;
  f.ignore(200,'\n');
  f >> meanBaseline_str;
  f.ignore(200,'\n');
  f >> fixedBaseline_str;
  f.ignore(200,'\n');
  f >> restartBaseline_str;
  f.ignore(200,'\n');
  f >> trigPileUp_str;
  f.ignore(200,'\n');
  f >> pileupRej_str;
  f.ignore(200,'\n');
  f >> overRangeRej_str;
  f.ignore(200,'\n');
  f >> selfTrigAcq_str;
  f.ignore(200,'\n');
  f >> oppPol_str;
  f.ignore(200,'\n');
  f >> chargeThresh_str;
  f.ignore(200,'\n');
  f >> chargePed_str;
  f.ignore(200,'\n');
  f >> trigHyst_str;
  f.ignore(200,'\n');
  f >> testPulse_str;
  f.ignore(200,'\n');

  f.ignore(200,'\n');
  f >> trigMode_str;
  f.ignore(200,'\n');
  f >> enableTrigProp_str;
  f.ignore(200,'\n');
  f >> trigCountMode_str;
  f.ignore(200,'\n');
  f >> shapedTrig_str;
  f.ignore(200, '\n');
  f >> latTime_str;
  f.ignore(200, '\n');
  f >> localTrigMode_str;
  f.ignore(200, '\n');
  f >> localTrigValMode_str;
  f.ignore(200, '\n');
  f.ignore(200, '\n');
  f.ignore(200, '\n');
  f >> addLocalTrigValMode_str;
  f.ignore(200, '\n');
  f.ignore(200, '\n');
  f.ignore(200, '\n');
  f >> trigValMask1_str;
  f.ignore(200, '\n');
  f.ignore(200, '\n');
  f.ignore(200, '\n');
  f >> trigValMask2_str;

  f.close();

  // Convert strings to 32-bit integers
  enableCh = v1730DPP_str_to_uint32t(enableCh_str);
  tlong = v1730DPP_str_to_uint32t(tlong_str);
  tshort = v1730DPP_str_to_uint32t(tshort_str);
  toffset = v1730DPP_str_to_uint32t(toffset_str);
  preTrig = v1730DPP_str_to_uint32t(preTrig_str);
  trigHoldOff = v1730DPP_str_to_uint32t(trigHoldOff_str);
  cGain = v1730DPP_str_to_uint32t(cGain_str);
  negSignals = v1730DPP_str_to_uint32t(negSignals_str);
  offset = v1730DPP_str_to_uint32t(offset_str);
  cThresh = v1730DPP_str_to_uint32t(cThresh_str);
  discrimMode = v1730DPP_str_to_uint32t(discrimMode_str);
  cCFDDelay = v1730DPP_str_to_uint32t(cCFDDelay_str);
  cCFDFraction = v1730DPP_str_to_uint32t(cCFDFraction_str);
  extrasRec = v1730DPP_str_to_uint32t(extrasRec_str);
  extrasMode = v1730DPP_str_to_uint32t(extrasMode_str);
  dRange = v1730DPP_str_to_uint32t(dRange_str);
  inputSmoothing = v1730DPP_str_to_uint32t(inputSmoothing_str);
  meanBaseline = v1730DPP_str_to_uint32t(meanBaseline_str);
  fixedBaseline = v1730DPP_str_to_uint32t(fixedBaseline_str);
  restartBaseline = v1730DPP_str_to_uint32t(restartBaseline_str);
  trigPileUp = v1730DPP_str_to_uint32t(trigPileUp_str);
  pileupRej = v1730DPP_str_to_uint32t(pileupRej_str);
  overRangeRej = v1730DPP_str_to_uint32t(overRangeRej_str);
  selfTrigAcq = v1730DPP_str_to_uint32t(selfTrigAcq_str);
  oppPol = v1730DPP_str_to_uint32t(oppPol_str);
  chargeThresh = v1730DPP_str_to_uint32t(chargeThresh_str);
  chargePed = v1730DPP_str_to_uint32t(chargePed_str);
  trigHyst = v1730DPP_str_to_uint32t(trigHyst_str);
  testPulse = v1730DPP_str_to_uint32t(testPulse_str);

  trigMode = v1730DPP_str_to_uint32t(trigMode_str);
  enableTrigProp = v1730DPP_str_to_uint32t(enableTrigProp_str);
  trigCountMode = v1730DPP_str_to_uint32t(trigCountMode_str);
  shapedTrig = v1730DPP_str_to_uint32t(shapedTrig_str);
  latTime = v1730DPP_str_to_uint32t(latTime_str);
  localTrigMode = v1730DPP_str_to_uint32t(localTrigMode_str);
  localTrigValMode = v1730DPP_str_to_uint32t(localTrigValMode_str);
  addLocalTrigValMode = v1730DPP_str_to_uint32t(addLocalTrigValMode_str);
  trigValMask1 = v1730DPP_str_to_uint32t(trigValMask1_str);
  trigValMask2 = v1730DPP_str_to_uint32t(trigValMask2_str);

  // Get the couple for each ch and the indices of 1st occurrence in each couple
  // Important for registers where applying settings to one ch in a couple applies to both
  for(uint32_t i=0; i<enableCh.size(); i++){
    int couple = (int) std::floor(enableCh[i]/2);
    couples.push_back(couple);
    if (i==0){couple_indices.push_back(i);}
    else if (couple != couples[i-1]){couple_indices.push_back(i);}
  }

  // *****************************************************
  // Print settings
  // *****************************************************

  if (print_settings){
    printf("DPP Settings:\n");
    printf("--------------------------------------------------\n");
    v1730DPP_PrintSettings(enableCh, "Channels Enabled", enableCh, 1);
    v1730DPP_PrintSettings(tlong, "Long gate", enableCh);
    v1730DPP_PrintSettings(tshort, "Short gate", enableCh);
    v1730DPP_PrintSettings(toffset, "Gate offset", enableCh);
    v1730DPP_PrintSettings(preTrig, "Pre-Trigger", enableCh);
    v1730DPP_PrintSettings(trigHoldOff, "Trigger Hold-Off", enableCh);
    v1730DPP_PrintSettings(cGain, "Gain", enableCh);
    v1730DPP_PrintSettings(dRange, "Dynamic Range", enableCh);
    v1730DPP_PrintSettings(negSignals, "Negative Signals", enableCh);
    v1730DPP_PrintSettings(offset, "DC Offset", enableCh);
    v1730DPP_PrintSettings(cThresh, "Threshold", enableCh);
    v1730DPP_PrintSettings(discrimMode, "Discrimination Mode", enableCh);
    v1730DPP_PrintSettings(cCFDDelay, "CFD Delay", enableCh);
    v1730DPP_PrintSettings(cCFDFraction, "CFD Fraction", enableCh);
    v1730DPP_PrintSettings(extrasRec, "EXTRAS Recording", enableCh);
    v1730DPP_PrintSettings(extrasMode, "EXTRAS Format", enableCh);

    v1730DPP_PrintSettings(inputSmoothing, "Input Smoothing", enableCh);
    v1730DPP_PrintSettings(meanBaseline, "Mean Baseline Calculation", enableCh);
    v1730DPP_PrintSettings(fixedBaseline, "Fixed Baseline", enableCh);
    v1730DPP_PrintSettings(restartBaseline, "Restart Baseline after Long Gate", enableCh);
    v1730DPP_PrintSettings(trigPileUp, "Pile Up Counted as Trigger", enableCh);
    v1730DPP_PrintSettings(pileupRej, "Pile Up Rejection", enableCh);
    v1730DPP_PrintSettings(overRangeRej, "Over-Range Rejction", enableCh);
    v1730DPP_PrintSettings(selfTrigAcq, "Self Trigger Event Acquisition", enableCh);
    v1730DPP_PrintSettings(oppPol, "Detect Opposite Polarity Signals", enableCh);
    v1730DPP_PrintSettings(chargeThresh, "Charge Zero Suppression Threshold", enableCh);
    v1730DPP_PrintSettings(chargePed, "Charge Pedestal", enableCh);
    v1730DPP_PrintSettings(trigHyst, "Trigger Hysteresis",enableCh);
    v1730DPP_PrintSettings(testPulse, "Test Pulse", enableCh);

    v1730DPP_PrintSettings(trigMode, "Trigger Mode (0=Normal, 1=Coincidence)", enableCh);
    v1730DPP_PrintSettings(enableTrigProp, "Enable Trigger Propagation (For Coincidences)", enableCh, 3);
    v1730DPP_PrintSettings(trigCountMode, "Trigger Counting Mode", enableCh);
    v1730DPP_PrintSettings(shapedTrig, "Shaped Trigger (Coincidence) Width", enableCh);
    v1730DPP_PrintSettings(latTime, "Latency Time", enableCh);
    v1730DPP_PrintSettings(localTrigMode, "Local Shaped Trigger Mode", enableCh, 2, couple_indices);
    v1730DPP_PrintSettings(localTrigValMode, "Local Trigger Validation Mode", enableCh, 2, couple_indices);
    v1730DPP_PrintSettings(addLocalTrigValMode, "Additional Local Trigger Validation Options", enableCh);
    v1730DPP_PrintSettings(trigValMask1, "Trigger Validation Mask 1 (Operation)", enableCh, 2, couple_indices);
    v1730DPP_PrintSettings(trigValMask2, "Trigger Validation Mask 2 (Couples)", enableCh, 2, couple_indices);
    printf("--------------------------------------------------\n\n");
  }

  // *****************************************************
  // General setup
  // *****************************************************

  // Is the digitizer working?
  v1730DPP_getFirmwareRev(gVme, gV1730Base);

  // Reset the module to default register values
  printf("\nReset module...\n\n");
  v1730DPP_SoftReset(gVme, gV1730Base);

  // Print Default Register Values (after software reset)
  if(print_default){
    printf("\nDefault Register Values:\n");
    printf("--------------------------------------------------\n");
    printf("Board Configuration (Reg 0x%x): 0x%x\n", V1730DPP_BOARD_CONFIG, v1730DPP_RegisterRead(gVme, gV1730Base, V1730DPP_BOARD_CONFIG));
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (enableCh[i] << 8);
      printf("DPP Algorithm Control 1 (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_ALGORITHM_CONTROL2 | (enableCh[i] << 8);
      printf("DPP Algorithm Control 2 (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_RECORD_LENGTH | (enableCh[i] << 8);
      printf("Record Length (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_LONG_GATE_WIDTH | (enableCh[i] << 8);
      printf("Long Gate (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_SHORT_GATE_WIDTH | (enableCh[i] << 8);
      printf("Short Gate (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_GATE_OFFSET | (enableCh[i] << 8);
      printf("Gate Offset (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_PRE_TRIGGER_WIDTH | (enableCh[i] << 8);
      printf("Pre-Trigger (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_TRIGGER_HOLDOFF | (enableCh[i] << 8);
      printf("Trigger Hold-Off (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_DC_OFFSET | (enableCh[i] << 8);
      printf("DC Offset (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_TRIGGER_THRESHOLD | (enableCh[i] << 8);
      printf("Trigger Threshold (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_CFD | (enableCh[i] << 8);
      printf("CFD Settings (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_DYNAMIC_RANGE | (enableCh[i] << 8);
      printf("Dynamic Range (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_FIXED_BASELINE | (enableCh[i] << 8);
      printf("Fixed Baseline (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_CHARGE_THRESHOLD | (enableCh[i] << 8);
      printf("Charge Zero Suppression Threshold (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_SHAPED_TRIGGER_WIDTH | (enableCh[i] << 8);
      printf("Shaped Trigger (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_LATENCY_TIME | (enableCh[i] << 8);
      printf("Latency Time (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      int couple = (int) std::floor(enableCh[i]/2);
      uint32_t reg = V1730DPP_TRIGGER_VALIDATION_MASK + (4 * couple);
      printf("Trigger Validation Mask (Couple %d, Reg 0x%x): 0x%x\n", couple, reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    printf("--------------------------------------------------\n\n");
  }

  sleep(1);

  printf("Settings Log:\n");
  printf("--------------------------------------------------\n");


  v1730DPP_RegisterWrite(gVme, gV1730Base, V1730DPP_BOARD_CONFIG, 0x800C0110); // 0x800C0110
  //v1730DPP_RegisterWrite(gVme, gV1730Base, 0x8084, 0x); // DPP Algorithm Control 2
  v1730DPP_setAggregateOrg(gVme, gV1730Base, 3);
  v1730DPP_setAggregateSizeG(gVme, gV1730Base, 1023);   // 1023 events per aggregate
  v1730DPP_setRecordLengthG(gVme, gV1730Base, 64);    // Don't collect waveforms
  v1730DPP_setDynamicRangeG(gVme, gV1730Base, dRange[0]);    //(0 = 2 Vpp)  (1 = 0.5 Vpp)

  // Disable all channels
  for(int i=0; i<16; i++){
    v1730DPP_DisableChannel(gVme, gV1730Base, i);
  }
  printf("\n");

  // Enable channels specified in settings
  for(uint32_t i=0; i<enableCh.size(); i++){
    v1730DPP_EnableChannel(gVme, gV1730Base, enableCh[i]);
  }
  printf("\n");

  // *****************************************************
  // Apply settings
  // *****************************************************

  // Ensure at least for DPP Algorithm Control 1 and 2, apply broadcast mode settings first
  // Otherwise, individual settings can be overwritten

  // *** Broadcast mode (All Channels) ***

  // Gain
  if(cGain.size()==1){v1730DPP_setGainG(gVme, gV1730Base, cGain[0]);}
  // Input Smoothing
  if(inputSmoothing.size()==1){v1730DPP_setInputSmoothingG(gVme, gV1730Base, inputSmoothing[0]);}
  // Polarity
  if(negSignals.size()==1){v1730DPP_setPolarityG(gVme, gV1730Base, negSignals[0]);}
  // Mean Baseline Calculation
  if(meanBaseline.size()==1){v1730DPP_setMeanBaselineCalcG(gVme, gV1730Base, meanBaseline[0]);}
  // Fixed Baseline
  if(fixedBaseline.size()==1){v1730DPP_setFixedBaselineG(gVme, gV1730Base, fixedBaseline[0]);}
  // Baseline Calculation Restart
  if(restartBaseline.size()==1){v1730DPP_setBaselineCalcRestartG(gVme, gV1730Base, restartBaseline[0]);}
  // Opposite Polarity Detection
  if(oppPol.size()==1){v1730DPP_setOppositePolarityDetectionG(gVme, gV1730Base, oppPol[0]);}
  // Charge Zero Suppression Threshold
  if(chargeThresh.size()==1){v1730DPP_setChargeZeroSuppressionThresholdG(gVme, gV1730Base, chargeThresh[0]);}
  // Charge Pedestal
  if(chargePed.size()==1){v1730DPP_setChargePedestalG(gVme, gV1730Base, chargePed[0]);}
  // Trigger Hysteresis
  if(trigHyst.size()==1){v1730DPP_setTriggerHysteresisG(gVme, gV1730Base, trigHyst[0]);}
  // Test Pulse
  if(testPulse.size()==1){
    if (testPulse[0]>0){
      v1730DPP_setTestPulseG(gVme, gV1730Base, 1);
      v1730DPP_setTestPulseRateG(gVme, gV1730Base, testPulse[0]-1);
    }
  }
  // Discrimination Mode (LED or CFD)
  if(discrimMode.size()==1){v1730DPP_setDiscriminationModeG(gVme, gV1730Base, discrimMode[0]);}
  // EXTRAS Recording
  if(extrasRec.size()==1){
    v1730DPP_setExtrasRecording(gVme, gV1730Base, extrasRec[0]);
    // EXTRAS recording flag
    if ((extrasRec[0] == 0) | (extrasRec[0] == 1)){
      extras = extrasRec[0];
    }
    else{printf("Error: 32-bit EXTRAS word recording must be 0 or 1\n");}
  }
  else{cout << "Error: EXTRAS recording must be applied in broadcast mode (all channels)" << endl;}
  // EXTRAS Format (ignore if EXTRAS Recording is disabled)
  if(extrasMode.size()==1){
    if (extrasRec[0]==1){
      v1730DPP_setExtrasFormatG(gVme, gV1730Base, extrasMode[0]);
    }
  }
  else{cout << "Error: EXTRAS Format must be applied in broadcast mode (all channels)" << endl;}
  // Long Gate
  if(tlong.size()==1){v1730DPP_setLongGateG(gVme, gV1730Base, tlong[0]);}
  // Short Gate
  if(tshort.size()==1){v1730DPP_setShortGateG(gVme, gV1730Base, tshort[0]);}
  // Gate Offset
  if(toffset.size()==1){v1730DPP_setGateOffsetG(gVme, gV1730Base, toffset[0]);}
  // Pre Trigger Width
  if(preTrig.size()==1){v1730DPP_setPreTriggerG(gVme, gV1730Base, preTrig[0]);}
  // Trigger Holdoff Width
  if(trigHoldOff.size()==1){v1730DPP_setTriggerHoldoffG(gVme, gV1730Base, trigHoldOff[0]);}
  // Threshold
  if(cThresh.size()==1){v1730DPP_setThresholdG(gVme, gV1730Base, cThresh[0]);}
  // DC Offset
  if(offset.size()==1){v1730DPP_setDCOffsetG(gVme, gV1730Base, offset[0]);}
  // Trigger Pile-Up
  if(trigPileUp.size()==1){v1730DPP_setTriggerPileupG(gVme, gV1730Base, trigPileUp[0]);}
  // Pile-Up Rejection
  if(pileupRej.size()==1){v1730DPP_setPileUpRejectionG(gVme, gV1730Base, pileupRej[0]);}
  // Over-Range Rejection
  if(overRangeRej.size()==1){v1730DPP_setOverRangeRejectionG(gVme, gV1730Base, overRangeRej[0]);}
  // Self Trigger Event Acquisition
  if(selfTrigAcq.size()==1){v1730DPP_setSelfTriggerAcquisitionG(gVme, gV1730Base, selfTrigAcq[0]);}

  // Trigger Mode (Normal or Coincidence)
  if(trigMode.size()==1){v1730DPP_setTriggerModeG(gVme, gV1730Base, trigMode[0]);}
  // Enable Trigger Propagation from Motherboard to Mezzanine (all channels)
  if(enableTrigProp.size()==1){v1730DPP_setTriggerPropagation(gVme, gV1730Base, enableTrigProp[0]);}
  // Trigger Counting Mode
  if(trigCountMode.size()==1){v1730DPP_setTriggerCountingModeG(gVme, gV1730Base, trigCountMode[0]);}
  // Shaped Trigger Width (Coincidence Window Width)
  if(shapedTrig.size()==1){v1730DPP_setShapedTriggerG(gVme, gV1730Base, shapedTrig[0]);}
  // Latency Time
  if(latTime.size()==1){v1730DPP_setLatencyTimeG(gVme, gV1730Base, latTime[0]);}
  // Local Shaped Trigger Mode | Using couple_indices, applies to 1st ch of each couple
  if(localTrigMode.size()==1){v1730DPP_setLocalShapedTriggerModeG(gVme, gV1730Base, localTrigMode[0]);}
  // Local Trigger Validation Mode | Using couple_indices, applies to 1st ch of each couple
  if(localTrigValMode.size()==1){v1730DPP_setLocalTriggerValidationModeG(gVme, gV1730Base, localTrigValMode[0]);}
  // Additional Local Trigger Validation Options
  if(addLocalTrigValMode.size()==1){v1730DPP_setAdditionalLocalTriggerValidationModeG(gVme, gV1730Base, addLocalTrigValMode[0]);}

  // *** Individual mode (Each Channel) ***

  if(cGain.size()>1){for(uint32_t i=0; i<cGain.size(); i++){v1730DPP_setGain(gVme, gV1730Base, cGain[i], enableCh[i]);}}
  if(inputSmoothing.size()>1){for(uint32_t i=0; i<inputSmoothing.size(); i++){v1730DPP_setInputSmoothing(gVme, gV1730Base, inputSmoothing[i], enableCh[i]);}}
  if(negSignals.size()>1){for(uint32_t i=0; i<negSignals.size(); i++){v1730DPP_setPolarity(gVme, gV1730Base, negSignals[i], enableCh[i]);}}
  if(meanBaseline.size()>1){for(uint32_t i=0; i<meanBaseline.size(); i++){v1730DPP_setMeanBaselineCalc(gVme, gV1730Base, meanBaseline[i], enableCh[i]);}}
  if(fixedBaseline.size()>1){for(uint32_t i=0; i<fixedBaseline.size(); i++){v1730DPP_setFixedBaseline(gVme, gV1730Base, fixedBaseline[i], enableCh[i]);}}
  if(restartBaseline.size()>1){for(uint32_t i=0; i<restartBaseline.size(); i++){v1730DPP_setBaselineCalcRestart(gVme, gV1730Base, restartBaseline[i], enableCh[i]);}}
  if(oppPol.size()>1){for(uint32_t i=0; i<oppPol.size(); i++){v1730DPP_setOppositePolarityDetection(gVme, gV1730Base, oppPol[i], enableCh[i]);}}
  if(chargeThresh.size()>1){for(uint32_t i=0; i<chargeThresh.size(); i++){v1730DPP_setChargeZeroSuppressionThreshold(gVme, gV1730Base, chargeThresh[i], enableCh[i]);}}
  if(chargePed.size()>1){for(uint32_t i=0; i<chargePed.size(); i++){v1730DPP_setChargePedestal(gVme, gV1730Base, chargePed[i], enableCh[i]);}}
  if(trigHyst.size()>1){for(uint32_t i=0; i<trigHyst.size(); i++){v1730DPP_setTriggerHysteresis(gVme, gV1730Base, trigHyst[i], enableCh[i]);}}
  if(testPulse.size()>1){
    for(uint32_t i=0; i<testPulse.size(); i++){
      if (testPulse[i]>0){
        v1730DPP_setTestPulse(gVme, gV1730Base, 1, enableCh[i]);
        v1730DPP_setTestPulseRate(gVme, gV1730Base, testPulse[i]-1, enableCh[i]);
      }
    }
  }
  if(discrimMode.size()>1){for(uint32_t i=0; i<discrimMode.size(); i++){v1730DPP_setDiscriminationMode(gVme, gV1730Base, discrimMode[i], enableCh[i]);}}
  if(tlong.size()>1){for(uint32_t i=0; i<tlong.size(); i++){v1730DPP_setLongGate(gVme, gV1730Base, tlong[i], enableCh[i]);}}
  if(tshort.size()>1){for(uint32_t i=0; i<tshort.size(); i++){v1730DPP_setShortGate(gVme, gV1730Base, tshort[i], enableCh[i]);}}
  if(toffset.size()>1){for(uint32_t i=0; i<toffset.size(); i++){v1730DPP_setGateOffset(gVme, gV1730Base, toffset[i], enableCh[i]);}}
  if(preTrig.size()>1){for(uint32_t i=0; i<preTrig.size(); i++){v1730DPP_setPreTrigger(gVme, gV1730Base, preTrig[i], enableCh[i]);}}
  if(trigHoldOff.size()>1){for(uint32_t i=0; i<trigHoldOff.size(); i++){v1730DPP_setTriggerHoldoff(gVme, gV1730Base, trigHoldOff[i], enableCh[i]);}}
  if(cThresh.size()>1){for(uint32_t i=0; i<cThresh.size(); i++){v1730DPP_setThreshold(gVme, gV1730Base, cThresh[i], enableCh[i]);}}
  if(offset.size()>1){for(uint32_t i=0; i<offset.size(); i++){v1730DPP_setDCOffset(gVme, gV1730Base, offset[i], enableCh[i]);}}
  if(trigPileUp.size()>1){for(uint32_t i=0; i<trigPileUp.size(); i++){v1730DPP_setTriggerPileup(gVme, gV1730Base, trigPileUp[i], enableCh[i]);}}
  if(pileupRej.size()>1){for(uint32_t i=0; i<pileupRej.size(); i++){v1730DPP_setPileUpRejection(gVme, gV1730Base, pileupRej[i], enableCh[i]);}}
  if(overRangeRej.size()>1){for(uint32_t i=0; i<overRangeRej.size(); i++){v1730DPP_setOverRangeRejection(gVme, gV1730Base, overRangeRej[i], enableCh[i]);}} 
  if(selfTrigAcq.size()>1){for(uint32_t i=0; i<selfTrigAcq.size(); i++){v1730DPP_setSelfTriggerAcquisition(gVme, gV1730Base, selfTrigAcq[i], enableCh[i]);}}

  if(trigMode.size()>1){for(uint32_t i=0; i<trigMode.size(); i++){v1730DPP_setTriggerMode(gVme, gV1730Base, trigMode[i], enableCh[i]);}} 
  if(trigCountMode.size()>1){for(uint32_t i=0; i<trigCountMode.size(); i++){v1730DPP_setTriggerCountingMode(gVme, gV1730Base, trigCountMode[i], enableCh[i]);}}
  if(shapedTrig.size()>1){for(uint32_t i=0; i<shapedTrig.size(); i++){v1730DPP_setShapedTrigger(gVme, gV1730Base, shapedTrig[i], enableCh[i]);}}
  if(latTime.size()>1){for(uint32_t i=0; i<latTime.size(); i++){v1730DPP_setLatencyTime(gVme, gV1730Base, latTime[i], enableCh[i]);}}
  if(localTrigMode.size()>1){for(uint32_t i=0; i<localTrigMode.size(); i++){v1730DPP_setLocalShapedTriggerMode(gVme, gV1730Base, localTrigMode[i], enableCh[couple_indices[i]]);}}
  if(localTrigValMode.size()>1){for(uint32_t i=0; i<localTrigValMode.size(); i++){v1730DPP_setLocalTriggerValidationMode(gVme, gV1730Base, localTrigValMode[i], enableCh[couple_indices[i]]);}}
  if(addLocalTrigValMode.size()>1){for(uint32_t i=0; i<addLocalTrigValMode.size(); i++){v1730DPP_setAdditionalLocalTriggerValidationMode(gVme, gV1730Base, addLocalTrigValMode[i], enableCh[i]);}}

  // *** Miscellaneous (Mixture of Modes) ***

  // Trigger Validation Mask (Operation and Couples)
  if(trigValMask1.size()==1 && trigValMask2.size()==1){
    for(uint32_t i=0; i<couple_indices.size(); i++){
      v1730DPP_setTriggerValidationMask(gVme, gV1730Base, trigValMask1[0], trigValMask2[0], enableCh[couple_indices[i]]);
    }
  }
  else if(trigValMask1.size()==1 && trigValMask2.size()>1){
    for(uint32_t i=0; i<trigValMask2.size(); i++){
      v1730DPP_setTriggerValidationMask(gVme, gV1730Base, trigValMask1[0], trigValMask2[i], enableCh[couple_indices[i]]);
    }
  }
  else if(trigValMask1.size()>1 && trigValMask2.size()==1){
    for(uint32_t i=0; i<trigValMask1.size(); i++){
      v1730DPP_setTriggerValidationMask(gVme, gV1730Base, trigValMask1[i], trigValMask2[0], enableCh[couple_indices[i]]);
    }
  }
  else if(trigValMask1.size() == trigValMask2.size()){
    for(uint32_t i=0; i<trigValMask1.size(); i++){
      v1730DPP_setTriggerValidationMask(gVme, gV1730Base, trigValMask1[i], trigValMask2[i], enableCh[couple_indices[i]]);
    }
  }
  else{cout << "Error: Trigger Validation Mask settings cannot be applied" << endl;}

  // CFD Delay and Fraction
  if(cCFDDelay.size()==1 && cCFDFraction.size()==1){
    v1730DPP_setCFDG(gVme, gV1730Base, cCFDDelay[0], cCFDFraction[0]);
  }
  else if (cCFDDelay.size()==1 && cCFDFraction.size()>1){
    for(uint32_t i=0; i<cCFDFraction.size(); i++){
      v1730DPP_setCFD(gVme, gV1730Base, cCFDDelay[0], cCFDFraction[i], enableCh[i]);
    }
  }
  else if (cCFDDelay.size()>1 && cCFDFraction.size()==1){
    for(uint32_t i=0; i<cCFDDelay.size(); i++){
      v1730DPP_setCFD(gVme, gV1730Base, cCFDDelay[i], cCFDFraction[0], enableCh[i]);
    }
  }
  else if (cCFDDelay.size() == cCFDFraction.size()){
    for(uint32_t i=0; i<cCFDDelay.size(); i++){
      v1730DPP_setCFD(gVme, gV1730Base, cCFDDelay[i], cCFDFraction[i], enableCh[i]);
    }
  }
  else{cout << "Error: CFD Delay and Fraction settings cannot be applied" << endl;}

  // *****************************************************
  // Print New Register Values
  // *****************************************************

  if(print_new){
    printf("\n\nNew Register Values:\n");
    printf("--------------------------------------------------\n");
    printf("Board Configuration (Reg 0x%x): 0x%x\n", V1730DPP_BOARD_CONFIG, v1730DPP_RegisterRead(gVme, gV1730Base, V1730DPP_BOARD_CONFIG));
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_ALGORITHM_CONTROL | (enableCh[i] << 8);
      printf("DPP Algorithm Control 1 (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_ALGORITHM_CONTROL2 | (enableCh[i] << 8);
      printf("DPP Algorithm Control 2 (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_RECORD_LENGTH | (enableCh[i] << 8);
      printf("Record Length (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_LONG_GATE_WIDTH | (enableCh[i] << 8);
      printf("Long Gate (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_SHORT_GATE_WIDTH | (enableCh[i] << 8);
      printf("Short Gate (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_GATE_OFFSET | (enableCh[i] << 8);
      printf("Gate Offset (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_PRE_TRIGGER_WIDTH | (enableCh[i] << 8);
      printf("Pre-Trigger (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_TRIGGER_HOLDOFF | (enableCh[i] << 8);
      printf("Trigger Hold-Off (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_DC_OFFSET | (enableCh[i] << 8);
      printf("DC Offset (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_TRIGGER_THRESHOLD | (enableCh[i] << 8);
      printf("Trigger Threshold (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_CFD | (enableCh[i] << 8);
      printf("CFD Settings (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_DYNAMIC_RANGE | (enableCh[i] << 8);
      printf("Dynamic Range (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_FIXED_BASELINE | (enableCh[i] << 8);
      printf("Fixed Baseline (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_CHARGE_THRESHOLD | (enableCh[i] << 8);
      printf("Charge Zero Suppression Threshold (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_SHAPED_TRIGGER_WIDTH | (enableCh[i] << 8);
      printf("Shaped Trigger (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      uint32_t reg = V1730DPP_LATENCY_TIME | (enableCh[i] << 8);
      printf("Latency Time (Ch %d, Reg 0x%x): 0x%x\n", enableCh[i], reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    for(uint32_t i=0; i<enableCh.size(); i++){
      int couple = (int) std::floor(enableCh[i]/2);
      uint32_t reg = V1730DPP_TRIGGER_VALIDATION_MASK + (4 * couple);
      printf("Trigger Validation Mask (Couple %d, Reg 0x%x): 0x%x\n", couple, reg, v1730DPP_RegisterRead(gVme, gV1730Base, reg));
    }
    printf("--------------------------------------------------\n\n");
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

  v1730DPP_LoadSettings(true, true, true);

  // Make sure the acquisition is stopped
  v1730DPP_AcqCtl(gVme, gV1730Base, V1730DPP_ACQ_STOP);

  // Clear the data
  v1730DPP_SoftClear(gVme, gV1730Base);

  // Get the board and algorithm configs
  //printf("\nBoard config: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, V1730DPP_BOARD_CONFIG));
  //printf("DPP Algorithm 1 config (ch 0): 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x1080));
  //printf("DPP Algorithm 2 config (ch 0): 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x1084));

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
  v1730DPP_DataPrint_Updated(gVme, gV1730Base, 8, extras);
  // }

  // Clears events from memory
  cout << "Software Clear (clearing data from this test run)" << endl;
  v1730DPP_SoftClear(gVme, gV1730Base);
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
  DWORD data[24572]; // 24572 is the # of buffers in a full board agg with EXTRAS included
  DWORD *pdata;
  DWORD counter = 0;

  // When any channel aggregate becomes full (1023 events)
  while(v1730DPP_EventsFull(gVme, gV1730Base)){
  //while(v1730DPP_EventsReady(gVme, gV1730Base)){
    eventcount++;

    // Flush the data for ch aggs that are not full  
    //v1730DPP_ForceDataFlush(gVme, gV1730Base);

    /* create Digitizer bank */
    bk_create(pevent, bname, TID_DWORD, (void **)&pdata);

    v1730DPP_ReadQLong(gVme, gV1730Base, data, &wcount, extras);

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

