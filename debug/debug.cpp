/********************************************************************\

  Name:         debug.cpp
  Created by:   W. Fox

  Contents:     Frontend for VME DAQ using CAEN ADC, and V1730
                (based on the generic frontend by K.Olcanski)
                This version works with the DPP firmware
$Id$
\********************************************************************/

//#include <stdio.h>
//#include <errno.h>
//#include <unistd.h>
#include <string.h>
#include <stdlib.h>
//#include <stdint.h>
#include <iostream>
#include <fstream>
//#include <sys/time.h>
//#include <assert.h>

#include <vector>
#include <sstream>

using namespace std;

void v1730DPP_LoadSettings();
vector<uint32_t> v1730DPP_str_to_uint32t(string str);
void v1730DPP_PrintSettings(vector<uint32_t> v, string str, vector <uint32_t> ch, int mode=0);

vector<uint32_t> v1730DPP_str_to_uint32t(string str){
  vector<uint32_t> vec;
  stringstream ss(str);
  char* end = nullptr;
  while (ss.good()){
    string substr;
    getline(ss, substr, ',');
    //vec.push_back(static_cast<uint32_t>(stoul(substr))); // std::stoul introduced in C++11
    vec.push_back(static_cast<uint32_t>(strtoul(&substr[0], &end, 10))); // std::strtoul in C standard library <stdlib.h>
  }
  return vec;
}

void v1730DPP_PrintSettings(vector<uint32_t> v, string str, vector<uint32_t> ch, int mode){
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
  
  printf("Reading settings-DPP.dat...\n");
  printf("\n");

  for(int i=0;i<15;i++){f.ignore(200,'\n');}

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
  std::cout << "--------------------------------------------------" << "\n" << std::endl;
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
  std::cout << std::endl << "--------------------------------------------------" << std::endl;

  if (enableCh.size()==1){std::cout << enableCh[0] << std::endl;}
  else {for(int i=0; i<enableCh.size(); i++){std::cout << enableCh[i] << std::endl;}}
}

int main(){

  printf("--------------------------------------------------\n");
  printf("--------- CAEN V1730 Digitizer DPP mode ----------\n");
  printf("\n");

  v1730DPP_LoadSettings();

  // Get the board config
  // printf("Board config: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x8000));
  // printf("DPP Algorithm 1 config: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x1080));
  // printf("DPP Algorithm 2 config: 0x%x\n", v1730DPP_RegisterRead(gVme, gV1730Base, 0x1084));

}
