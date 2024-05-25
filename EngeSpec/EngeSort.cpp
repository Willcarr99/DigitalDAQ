#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <climits> // For INT_MAX
#include <cmath>

#include "EngeSort.h"
#include "TV792Data.hxx"

Messages messages;

std::string EngeSort::sayhello( ) {
  return messages.sayhello("EngeSort v1730");
}
std::string EngeSort::saygoodbye( ) {
  return messages.saygoodbye();
}
std::string EngeSort::saysomething(std::string str) {
  
  return messages.saysomething(str);
}

//---------------------------- Settings --------------------------------
//----------------------------------------------------------------------
// Number of channels in 1D and 2D histograms
int Channels1D = 8192; // TODO - Test this scale (was 4096)
int Channels2D = 1024; // TODO - Test this scale (was 512)

// cDet threshold (Channels1D scale, 0-8191)
int Thresh = 20;

// Define the channels (0-15)
int iFrontHE = 0;
int iFrontLE = 1;
int iBackHE = 2;
int iBackLE = 3;
int iE = 4;
int iDE = 5;
int iSiE = 6;
int iSiDE = 7;

// FP Trigger (By channel number -- for Pos1 use iFrontHE or iFrontLE.)
// TODO - Implement the ability to trigger on the back.
int FPTrigger = iE; // Scintillator (E)
// Note: Si triggers on EITHER SiE or SiDE

// Coincidence Windows for Focal Plane and Si Detector (in ns)
int win_ns = 2000;
int winSi_ns = 2000;

// Max value of the timetag, at which point it rolls back to 0
// Default timetag for v1730 (EXTRAS disabled) is a 31-bit number. So max is 2^31 - 1 = 2147483648 or 0x7FFFFFFF
// Each timetag unit is 2 ns (v1730), so roll back time is 2 ns/unit * (2^31 - 1) units ~ 4.295 s
int timetagReset = INT_MAX;

// Scaling coincidence time (for E vs time histograms) by this factor
// timeScale * 2 ns per bin [min = 1 (best resolution - 2 ns), max = 16 (32 ns)]
int timeScale = 1;

// EXTRAS Recording Enabled (True) or Disabled (False)
bool extras = true;
// 32-bit word: bits[31:16] = Extended Time Stamp, bits[15:10] = Flags, bits[9:0] = Fine Time Stamp
//----------------------------------------------------------------------
//----------------------------------------------------------------------

//------------------ Global sort routine variables --------------------
// Data to save for each coincidence window (updated each window)
int FrontHE;
int FrontLE;
int BackHE;
int BackLE;
int E;
int DE;
int SiE;
int SiDE;

int Pos1; // FrontHE - FrontLE (+ offset to center)
int Pos2; // BackHE - BackLE (+ offset to center)
int Theta; // TODO - May need to fix definition of Theta below

int Pos1comp; // compressed for 2D histograms
int Pos2comp;
int Ecomp;
int DEcomp;
int SiEcomp;
int SiDEcomp;

const double pSiSlope = 0.0;
int SiTotalE;
int SiTotalEcomp;

// Converting window duration to timetag units (2 ns/unit - v1730)
int win = (int) (win_ns/2.0);
int winSi = (int) (winSi_ns/2.0);

// Flags to prevent more than one signal to be collected from a single component during the coincidence window.
// True when signal is detected during window.
bool fE = false;
bool fDE = false;
bool fFrontHE = false;
bool fFrontLE = false;
bool fBackHE = false;
bool fBackLE = false;
bool fTheta = false;

bool fSiE = false;
bool fSiDE = false;
//----------------------------------------------------------------------

std::vector<int> vPos1;
std::vector<int> vDE;
std::vector<int> vSiE;
std::vector<int> vSiDE;
std::vector<int> vSiTotalE;

// 1D Spectra
Histogram *hPos1;
Histogram *hPos2;
Histogram *hDE;
Histogram *hE;
Histogram *hTheta;

Histogram *hSiE;
Histogram *hSiDE;

//Histogram *hTDC_E;
//Histogram *hTDC_DE;
//Histogram *hTDC_PosSum;

// 2D Spectra
Histogram *hDEvsPos1;
Histogram *hEvsPos1;
Histogram *hDEvsE;
Histogram *hPos2vsPos1;
Histogram *hThetavsPos1;

Histogram *hPos1vsEvt;

Histogram *hSiDEvsSiE;

// Gated Spectra
Histogram *hPos1_gDEvsPos1_G1;
Histogram *hPos1_gDEvsPos1_G2;
Histogram *hPos1_gDEvsE_G1;
Histogram *hPos1_gDEvsE_G2;

Histogram *hPos1_gPos1vsEvt;
Histogram *hPos1_gDEvPos1_gPos1vsEvt;
Histogram *hDEvsPos1_G_Pos1vsEvt;

Histogram *hE_gE_G1;

//Histogram *hSiE_G_SiDEvsSiE;
Histogram *hSiTotalE_G_SiDEvsSiE;
Histogram *hSiDEvsSiTotalE_G_SiDEvsSiE;

Histogram *hSiTotalE_G_Pos1vsEvt;
Histogram *hSiTotalE_G_SiDEvsSiE_G_Pos1vsEvt;
Histogram *hSiTotalEvsEvt_G_SiDEvsSiE;

// Counters
int totalCounter=0;
int gateCounter=0;
int EventNo=0;

/* // TODO - How do scalers change between v785 and v1730?
// Scalers
Scaler *sGates;
Scaler *sGatesLive;
Scaler *sClock;
Scaler *sClockLive;
Scaler *sFrontLE;
Scaler *sFrontHE;
Scaler *sBackLE;
Scaler *sBackHE;
Scaler *sE;
Scaler *sDE;
Scaler *BCI;
*/

void EngeSort::Initialize(){

  //--------------------
  // 1D Histograms
  hPos1 = new Histogram("Position 1", Channels1D, 1);
  hPos2 = new Histogram("Position 2", Channels1D, 1);
  hDE = new Histogram("Delta E", Channels1D, 1);
  hE = new Histogram("E", Channels1D, 1);
  hTheta = new Histogram("Theta", Channels1D, 1);

  hSiE = new Histogram("Silicon E", Channels1D, 1);
  hSiDE = new Histogram("Silicon DE", Channels1D, 1);

  //hTDC_E = new Histogram("TDC E", Channels1D, 1);
  //hTDC_DE = new Histogram("TDC DE", Channels1D, 1);
  //hTDC_PosSum = new Histogram("TDC Position Sum", Channels1D, 1);
  
  //--------------------
  // 2D Histograms
  hDEvsPos1 = new Histogram("DE vs Pos1", Channels2D, 2);
  hEvsPos1 = new Histogram("E vs Pos1", Channels2D, 2);
  hDEvsE = new Histogram("DE vs E", Channels2D, 2);
  hPos2vsPos1 = new Histogram("Pos 2 vs Pos 1", Channels2D, 2);
  hThetavsPos1 = new Histogram("Theta vs Pos 1", Channels2D, 2);

  hPos1vsEvt = new Histogram("Pos 1 vs Event", Channels2D, 2);

  hSiDEvsSiE = new Histogram("SiDE vs SiE", Channels2D, 2);

  //--------------------
  // Gated Histograms
  hPos1_gDEvsPos1_G1 = new Histogram("Pos 1; G-DEvsPos1-G1", Channels1D, 1);
  hPos1_gDEvsPos1_G2 = new Histogram("Pos 1; G-DEvsPos1-G2", Channels1D, 1);

  hPos1_gDEvsE_G1 = new Histogram("Pos 1; G-DEvsE-G1", Channels1D, 1);
  hPos1_gDEvsE_G2 = new Histogram("Pos 1; G-DEvsE-G2", Channels1D, 1);

  hPos1_gPos1vsEvt = new Histogram("Pos 1; GPos1vsEvent", Channels1D, 1); // Pos1vsEvent gates incremented in EngeSort::FillEndofRun(), not EngeSort::sort()
  hPos1_gDEvPos1_gPos1vsEvt = new Histogram("Pos 1; GDEvPos1-G2 + GPos1vsEvent", Channels1D, 1);
  hDEvsPos1_G_Pos1vsEvt = new Histogram("DE vs Pos1; GPos1vsEvent", Channels2D, 2);

  hE_gE_G1 = new Histogram("E; GE-G1", Channels1D, 1);

  //hSiE_G_SiDEvsSiE = new Histogram("SiE; G-SiDEvsSiE", Channels1D, 1);
  hSiTotalE_G_SiDEvsSiE = new Histogram("SiTotE; G-SiDEvsSiE", Channels1D, 1);
  hSiDEvsSiTotalE_G_SiDEvsSiE = new Histogram("SiDEvsSiTotE; G-SiDEvsSiE", Channels2D, 2);

  hSiTotalE_G_Pos1vsEvt = new Histogram("SiTotE; GPos1vsEvent", Channels1D, 1);
  hSiTotalE_G_SiDEvsSiE_G_Pos1vsEvt = new Histogram("SiTotE; GSiDEvsSiE + GPos1vsEvent", Channels1D, 1);

  hSiTotalEvsEvt_G_SiDEvsSiE = new Histogram("SiTotE vs Event; GSiDEvsSiE", Channels2D, 2);

  //--------------------
  // Gates
  hE -> addGate("Energy Gate");
  
  hDEvsPos1 -> addGate("DE vs Pos1 Gate 1");
  hDEvsPos1 -> addGate("DE vs Pos1 Gate 2");

  hDEvsE -> addGate("DE vs E Gate 1");
  hDEvsE -> addGate("DE vs E Gate 2");
  
  hPos1vsEvt -> addGate("Event Gate");

  hSiDEvsSiE -> addGate("SiDE vs SiE Gate");

  /*
  // Loop through and print all histograms
  for(auto h: Histograms){
    if(h->getnDims() == 1) {
      std::cout << "Found a 1D histogram!" << std::endl;
      h->Print(0,10);
    } else if(h->getnDims() == 2) {
      std::cout << "Found a 2D histogram!" << std::endl;
      h->Print(0,10,0,10);
    }
    std::cout << std::endl;
  }
  */

  /* // TODO - How do scalers change between v785 and v1730?
  // Build the scalers
  sGates = new Scaler("Total Gates", 0);    // Name, index
  sGatesLive = new Scaler("Total Gates Live", 1);    // Name, index
  
  sClock = new Scaler("Clock",2);
  sClockLive = new Scaler("Clock Live",3);
  sFrontLE = new Scaler("Front HE",4);
  sFrontHE = new Scaler("Front LE",5);
  sBackLE = new Scaler("Back HE",6);
  sBackHE = new Scaler("Back LE",7);
  sE = new Scaler("E",8);
  sDE = new Scaler("DE",9);
  BCI = new Scaler("BCI",15);
  */

}

//======================================================================
// This is the equivalent to the "sort" function in jam
void EngeSort::sort(uint32_t *dADC, int nADC, uint32_t *dTDC, int nTDC){

  totalCounter++;
  EventNo++;

  // 32-bit signed integers. Timetag is 31 (unsigned) bits of a 32-bit unsigned integer buffer, so int cast is ok. The MSB is always 0.
  int ch, time;

  // 16-bit signed integers. Qlong is a 16-bit signed integer in a 32-bit unsigned buffer (pg 100-101 of manual).
  // So qlong range is -32768 to 32767. cDet range is -8192 to 8191. Negative cDet values are coverted to 0 by the threshold check.
  int16_t qlong, cDet;

  // Window start time - gets updated each trigger. The initial value here prevents having to do an extra if statement for the first trigger in the buffer.
  int winStart = -win - 1;
  int winStartSi = -winSi - 1;

  // Data for each event is stored in 2 32-bit unsigned integer memory locations. 
  // Energies, i.e. qlong, (and Ch #) are EVEN dADC counts, timetags are ODD counts (see ReadQLong in v1730DPP.c)
  for(int i = 0; i<nADC; i+=2){
    // Extract energy, channel, and timetag from event
    qlong = (int16_t) (dADC[i] & 0xFFFF);
    ch = (int) (dADC[i] & 0xFFFF0000) >> 16;
    cDet = (int16_t) std::floor(qlong/4.0); // Maxes out at Channels1D (8,192).
    time = (int) dADC[i+1];

    // Handle noise for E, DE, SiE, and SiDE (Pos1 and Pos2 handled separately in FP coincidence window below)
    if (cDet < Thresh || cDet > Channels1D - 1){cDet=0;} // To move 8192 spike to 0, use >=

    //------------------- Si Window -------------------
    // Triggering on either SiE or SiDE
    if (ch == iSiE || ch == iSiDE){
      // Check if Si coincidence window is already open (accounting for possible rollback to 0)
      if ((time > winStartSi && time < winStartSi + winSi) || (winStartSi > timetagReset - winSi && time > winStartSi + winSi - timetagReset)){
        // Check if the signal is from SiE and SiDE was the trigger (ignoring multiple occurrences of SiE)
        if (ch == iSiE && !fSiE && fSiDE){
          fSiE = true;
          SiE = (int) cDet;
          hSiE -> inc(SiE);
          vSiE.push_back(SiE);
          vPos1.push_back(0);
          vDE.push_back(0);
          vSiDE.push_back(0);
          SiEcomp = (int) std::floor(SiE/8.0); // This maxes out at 1,024
          hSiDEvsSiE -> inc(SiEcomp,SiDEcomp);
          SiTotalE = (int) std::floor((SiE + pSiSlope*SiDE) / (1.0 + pSiSlope));
          SiTotalEcomp = (int) std::floor(SiTotalE/8.0);
          vSiTotalE.push_back(SiTotalE);

          Gate &G3 = hSiDEvsSiE -> getGate(0);
          //G3.Print();
          if (G3.inGate(SiEcomp,SiDEcomp)){
            gateCounter++;
            //hSiE_G_SiDEvsSiE -> inc(SiE);
            hSiTotalE_G_SiDEvsSiE -> inc(SiTotalE);
            hSiDEvsSiTotalE_G_SiDEvsSiE -> inc(SiTotalEcomp,SiDEcomp);
          }
        }
        // Check if the signal is from SiDE and SiE was the trigger (ignoring multiple occurrences of SiDE)
        else if (ch == iSiDE && !fSiDE && fSiE){
          fSiDE = true;
          SiDE = (int) cDet;
          hSiDE -> inc(SiDE);
          vSiDE.push_back(SiDE);
          vPos1.push_back(0);
          vDE.push_back(0);
          vSiE.push_back(0);
          SiDEcomp = (int) std::floor(SiDE/8.0); // This maxes out at 1,024
          hSiDEvsSiE -> inc(SiEcomp,SiDEcomp);
          SiTotalE = (int) std::floor((SiE + pSiSlope*SiDE) / (1.0 + pSiSlope));
          SiTotalEcomp = (int) std::floor(SiTotalE/8.0);
          vSiTotalE.push_back(SiTotalE);

          Gate &G3 = hSiDEvsSiE -> getGate(0);
          //G3.Print();
          if (G3.inGate(SiEcomp,SiDEcomp)){
            gateCounter++;
            //hSiE_G_SiDEvsSiE -> inc(SiE);
            hSiTotalE_G_SiDEvsSiE -> inc(SiTotalE);
            hSiDEvsSiTotalE_G_SiDEvsSiE -> inc(SiTotalEcomp,SiDEcomp);
          }
        }
      }

      // Previous window ended. Now open new window from trigger.
      else{
        winStartSi = time;
        if (ch == iSiE){
          SiE = (int) cDet;
          hSiE -> inc(SiE); // TODO - Do we want this to increment even if no coincidence?
          vSiE.push_back(SiE);
          vPos1.push_back(0);
          vDE.push_back(0);
          vSiDE.push_back(0);
          vSiTotalE.push_back(0);
          SiEcomp = (int) std::floor(SiE/8.0);
          fSiE = true;
          fSiDE = false;
        }
        else if (ch == iSiDE){
          SiDE = (int) cDet;
          hSiDE -> inc(SiDE); // TODO - Do we want this to increment even if no coincidence?
          vSiDE.push_back(SiDE);
          vPos1.push_back(0);
          vDE.push_back(0);
          vSiE.push_back(0);
          vSiTotalE.push_back(0);
          SiDEcomp = (int) std::floor(SiDE/8.0);
          fSiDE = true;
          fSiE = false;
        }
      }
    }
    //----------------------------------------------------------
    //------------------- Focal Plane Window -------------------
    // Triggering on FPTrigger specified by user
    else if (ch == iFrontHE || ch == iFrontLE || ch == iBackHE || ch == iBackLE || ch == iE || ch == iDE){
      // Check if FP coincidence window is still open (accounting for possible timetag rollback to 0)
      if ((time > winStart && time < winStart + win) || (winStart > timetagReset - win && time > winStart + win - timetagReset)){
        if (FPTrigger == iE){ // E trigger
          if (ch == iFrontHE && !fFrontHE){ // Pos1 HE signal
            fFrontHE = true;
            FrontHE = time;
            if (fFrontLE){ // Pos1 HE and LE coincidence
              Pos1 = (FrontHE - FrontLE) + (Channels1D/2.0); // HE - LE, offset to center (4,096).
              if(Pos1 < Thresh || Pos1 > Channels1D - 1){Pos1=0;}
              hPos1 -> inc(Pos1);
              vPos1.push_back(Pos1);
              vDE.push_back(0);
              vSiE.push_back(0);
              vSiDE.push_back(0);
              vSiTotalE.push_back(0);
              // Cut off the outer-edges of the time range with center at 512. Adjust scaling by timeScale setting
              // TODO - Should I just compress this similar to other 2D histograms?
              Pos1comp = ((Pos1 - (Channels1D/2.0)) / timeScale) + (Channels2D/2.0);
              //Pos1comp = ((FrontHE - FrontLE) / timeScale) + (Channels2D/2.0); // Needs to be derived from Pos1
              
              // E vs Pos1
              hEvsPos1 -> inc(Pos1comp,Ecomp);

              if (fDE){
                // DE vs Pos1
                hDEvsPos1 -> inc(Pos1comp,DEcomp);

                // Pos1 - DE vs Pos1 Gates
                Gate &G1 = hDEvsPos1 -> getGate(0);
                //G1.Print();
                if (G1.inGate(Pos1comp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsPos1_G1 -> inc(Pos1);
                }
                Gate &G2 = hDEvsPos1 -> getGate(1);
                //G2.Print();
                if (G2.inGate(Pos1comp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsPos1_G2 -> inc(Pos1);
                }

                // Pos1 - DE vs E Gates
                Gate &G4 = hDEvsE -> getGate(0);
                //G4.Print();
                if (G4.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsE_G1 -> inc(Pos1);
                }
                Gate &G5 = hDEvsE -> getGate(1);
                //G5.Print();
                if (G5.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_DEvsE_G2 -> inc(Pos1);
                }
              }

              // Pos2 vs Pos1
              if (fBackHE && fBackLE){
                hPos2vsPos1 -> inc(Pos1comp,Pos2comp);
                // TODO - Add Pos2vsPos1 Gates
              }
            }
          }
          else if (ch == iFrontLE && !fFrontLE){ // Pos1 LE signal
            fFrontLE = true;
            FrontLE = time;
            if (fFrontHE){ // Pos1 HE and LE coincidence
              Pos1 = (FrontHE - FrontLE) + (Channels1D/2.0); // HE - LE, offset to center (4,096)
              if(Pos1 < Thresh || Pos1 > Channels1D - 1){Pos1=0;}
              hPos1 -> inc(Pos1);
              vPos1.push_back(Pos1);
              vDE.push_back(0);
              vSiE.push_back(0);
              vSiDE.push_back(0);
              vSiTotalE.push_back(0);
              // Cut off the outer-edges of the time range with center at 512. Adjust scaling by timeScale setting
              // TODO - Should I just compress this similar to other 2D histograms?
              Pos1comp = ((Pos1 - (Channels1D/2.0)) / timeScale) + (Channels2D/2.0);
              //Pos1comp = ((FrontHE - FrontLE) / timeScale) + (Channels2D/2.0); // Needs to be derived from Pos1

              // E vs Pos1
              hEvsPos1 -> inc(Pos1comp,Ecomp);

              // DE vs Pos1
              if (fDE){
                hDEvsPos1 -> inc(Pos1comp,DEcomp);

                // P1 - DE vs Pos1 Gates
                Gate &G1 = hDEvsPos1 -> getGate(0);
                //G1.Print();
                if (G1.inGate(Pos1comp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsPos1_G1 -> inc(Pos1);
                }
                Gate &G2 = hDEvsPos1 -> getGate(1);
                //G2.Print();
                if (G2.inGate(Pos1comp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsPos1_G2 -> inc(Pos1);
                }

                // Pos1 - DE vs E Gates
                Gate &G4 = hDEvsE -> getGate(0);
                //G4.Print();
                if (G4.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsE_G1 -> inc(Pos1);
                }
                Gate &G5 = hDEvsE -> getGate(1);
                //G5.Print();
                if (G5.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_DEvsE_G2 -> inc(Pos1);
                }
              }

              // Pos2 vs Pos1
              if (fBackHE && fBackLE){
                hPos2vsPos1 -> inc(Pos1comp,Pos2comp);
                // TODO - Add Pos2vsPos1 Gates
              }
            }
          }
          else if (ch == iBackHE && !fBackHE){ // Pos2 HE signal
            fBackHE = true;
            BackHE = time;
            if (fBackLE){
              Pos2 = (BackHE - BackLE) + (Channels1D/2.0); // HE - LE, offset to center (4,096)
              if(Pos2 < Thresh || Pos2 > Channels1D - 1){Pos2=0;}
              hPos2 -> inc(Pos2);
              vPos1.push_back(0);
              vDE.push_back(0);
              vSiE.push_back(0);
              vSiDE.push_back(0);
              vSiTotalE.push_back(0);
              // Cut off the outer-edges of the time range with center at 512. Adjust scaling by timeScale setting
              // TODO - Should I just compress this similar to other 2D histograms?
              Pos2comp = ((Pos2 - (Channels1D/2.0)) / timeScale) + (Channels2D/2.0);
              //Pos2comp = ((BackHE - BackLE) / timeScale) + (Channels2D/2.0); // Needs to be derived from Pos2

              // Pos2 vs Pos1
              if (fFrontHE && fFrontLE){
                hPos2vsPos1 -> inc(Pos1comp,Pos2comp);
                // TODO - Add Pos2vsPos1 Gates
              }
            }
          }
          else if (ch == iBackLE && !fBackLE){ // Pos2 LE signal
            fBackLE = true;
            BackLE = time;
            if (fBackHE){
              Pos2 = (BackHE - BackLE) + (Channels1D/2.0); // HE - LE, offset to center (4,096)
              if(Pos2 < Thresh || Pos2 > Channels1D - 1){Pos2=0;}
              hPos2 -> inc(Pos2);
              vPos1.push_back(0);
              vDE.push_back(0);
              vSiE.push_back(0);
              vSiDE.push_back(0);
              vSiTotalE.push_back(0);
              // Cut off the outer-edges of the time range with center at 512. Adjust scaling by timeScale setting
              // TODO - Should I just compress this similar to other 2D histograms?
              Pos2comp = ((Pos2 - (Channels1D/2.0)) / timeScale) + (Channels2D/2.0);
              //Pos2comp = ((BackHE - BackLE) / timeScale) + (Channels2D/2.0); // Needs to be derived from Pos2

              // Pos2 vs Pos1
              if (fFrontHE && fFrontLE){
                hPos2vsPos1 -> inc(Pos1comp,Pos2comp);
                // TODO - Add Pos2vsPos1 Gates
              }
            }
          }
          else if (ch == iDE && !fDE){ // DE signal
            fDE = true;
            DE = (int) cDet;
            hDE -> inc(DE);
            vDE.push_back(DE);
            vPos1.push_back(0);
            vSiE.push_back(0);
            vSiDE.push_back(0);
            vSiTotalE.push_back(0);
            DEcomp = (int) std::floor(DE/8.0);

            // DE vs E
            hDEvsE -> inc(Ecomp,DEcomp);
            
            // DE vs Pos1
            if (fFrontHE && fFrontLE){
              hDEvsPos1 -> inc(Pos1comp,DEcomp);

              // Pos1 - DE vs Pos1 Gates
              Gate &G1 = hDEvsPos1 -> getGate(0);
              //G1.Print();
              if (G1.inGate(Pos1comp,DEcomp)){
                gateCounter++;
                hPos1_gDEvsPos1_G1 -> inc(Pos1);
              }
              Gate &G2 = hDEvsPos1 -> getGate(1);
              //G2.Print();
              if (G2.inGate(Pos1comp,DEcomp)){
                gateCounter++;
                hPos1_gDEvsPos1_G2 -> inc(Pos1);
              }

              // Pos 1 - DE vs E Gates
              Gate &G4 = hDEvsE -> getGate(0);
              //G4.Print();
              if (G4.inGate(Ecomp,DEcomp)){
                gateCounter++;
                hPos1_gDEvsE_G1 -> inc(Pos1);
              }
              Gate &G5 = hDEvsE -> getGate(1);
              //G5.Print();
              if (G5.inGate(Ecomp,DEcomp)){
                gateCounter++;
                hPos1_gDEvsE_G2 -> inc(Pos1);
              }
            }
          }
        }
        else if (FPTrigger == iFrontHE || FPTrigger == iFrontLE){ // Front Trigger
          if (ch == iE && !fE){ // E signal
            fE = true;
            E = (int) cDet;
            hE -> inc(E);
            vPos1.push_back(0);
            vDE.push_back(0);
            vSiE.push_back(0);
            vSiDE.push_back(0);
            vSiTotalE.push_back(0);
            Ecomp = (int) std::floor(E/8.0);

            // E Gate
            Gate &G = hE -> getGate(0);
            //G.Print();
            if (G.inGate(E)){
              hE_gE_G1 -> inc(E);
            }

            // E vs Pos1
            if (fFrontHE && fFrontLE){
              hEvsPos1 -> inc(Pos1comp,Ecomp);
            }

            if (fDE){
              // DE vs E
              hDEvsE -> inc(Ecomp,DEcomp);

              // Pos1 - DE vs E Gates
              if (fFrontHE && fFrontLE){
                Gate &G4 = hDEvsE -> getGate(0);
                //G4.Print();
                if (G4.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsE_G1 -> inc(Pos1);
                }
                Gate &G5 = hDEvsE -> getGate(1);
                //G5.Print();
                if (G5.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsE_G2 -> inc(Pos1);
                }
              }
            }
          }
          else if ((ch == iFrontHE && !fFrontHE) || (ch == iFrontLE && !fFrontLE)){ // Pos1 HE or LE signal
            if (ch == iFrontHE){
              fFrontHE = true;
              FrontHE = time;
            }
            else if (ch == iFrontLE){
              fFrontLE = true;
              FrontLE = time;
            }
            Pos1 = (FrontHE - FrontLE) + (Channels1D/2.0); // HE - LE, offset to center (4,096)
            if(Pos2 < Thresh || Pos2 > Channels1D - 1){Pos2=0;}
            hPos1 -> inc(Pos1);
            vPos1.push_back(Pos1);
            vDE.push_back(0);
            vSiE.push_back(0);
            vSiDE.push_back(0);
            vSiTotalE.push_back(0);
            // Cut off the outer-edges of the time range with center at 512. Adjust scaling by timeScale setting
            // TODO - Should I just compress this similar to other 2D histograms?
            Pos1comp = ((Pos1 - (Channels1D/2.0)) / timeScale) + (Channels2D/2.0);
            //Pos1comp = ((FrontHE - FrontLE) / timeScale) + (Channels2D/2.0); // Needs to be derived from Pos1

            // E vs Pos1
            if (fE){
              hEvsPos1 -> inc(Pos1comp,Ecomp);
            }

            // DE vs Pos1
            if (fDE){
              hDEvsPos1 -> inc(Pos1comp,DEcomp);

              // Pos1 - DE vs Pos1 Gates
              Gate &G1 = hDEvsPos1 -> getGate(0);
              //G1.Print();
              if (G1.inGate(Pos1comp,DEcomp)){
                gateCounter++;
                hPos1_gDEvsPos1_G1 -> inc(Pos1);
              }
              Gate &G2 = hDEvsPos1 -> getGate(1);
              //G2.Print();
              if (G2.inGate(Pos1comp,DEcomp)){
                gateCounter++;
                hPos1_gDEvsPos1_G2 -> inc(Pos1);
              }

              if (fE){
                // Pos1 - DE vs E Gates
                Gate &G4 = hDEvsE -> getGate(0);
                //G4.Print();
                if (G4.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsE_G1 -> inc(Pos1);
                }
                Gate &G5 = hDEvsE -> getGate(1);
                //G5.Print();
                if (G5.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_DEvsE_G2 -> inc(Pos1);
                }
              }
            }

            // Pos2 vs Pos1
            if (fBackHE && fBackLE){
              hPos2vsPos1 -> inc(Pos1comp,Pos2comp);
              // TODO - Add Pos2vsPos1 Gates
            }
          }
          else if (ch == iBackHE && !fBackHE){ // Pos2 HE signal
            fBackHE = true;
            BackHE = time;
            if (fBackLE){
              Pos2 = (BackHE - BackLE) + (Channels1D/2.0); // HE - LE, offset to center (4,096)
              if(Pos2 < Thresh || Pos2 > Channels1D/2.0){Pos2=0;}
              hPos2 -> inc(Pos2);
              vPos1.push_back(0);
              vDE.push_back(0);
              vSiE.push_back(0);
              vSiDE.push_back(0);
              vSiTotalE.push_back(0);
              // Cut off the outer-edges of the time range with center at 512. Adjust scaling by timeScale setting
              // TODO - Should I just compress this similar to other 2D histograms?
              Pos2comp = ((Pos2 - (Channels1D/2.0)) / timeScale) + (Channels2D/2.0);
              //Pos2comp = ((BackHE - BackLE) / timeScale) + (Channels2D/2.0); // Needs to be derived from Pos2

              // Pos2 vs Pos1
              if (fFrontHE && fFrontLE){
                hPos2vsPos1 -> inc(Pos1comp,Pos2comp);
                // TODO - Add Pos2vsPos1 Gates
              }
            }
          }
          else if (ch == iBackLE && !fBackLE){ // Pos2 LE signal
            fBackLE = true;
            BackLE = time;
            if (fBackHE){
              Pos2 = (BackHE - BackLE) + (Channels1D/2.0); // HE - LE, offset to center (4,096)
              if(Pos2 < Thresh || Pos2 > Channels1D/2.0){Pos2=0;}
              hPos2 -> inc(Pos2);
              vPos1.push_back(0);
              vDE.push_back(0);
              vSiE.push_back(0);
              vSiDE.push_back(0);
              vSiTotalE.push_back(0);
              // Cut off the outer-edges of the time range with center at 512. Adjust scaling by timeScale setting
              // TODO - Should I just compress this similar to other 2D histograms?
              Pos2comp = ((Pos2 - (Channels1D/2.0)) / timeScale) + (Channels2D/2.0);
              //Pos2comp = ((BackHE - BackLE) / timeScale) + (Channels2D/2.0); // Needs to be derived from Pos2

              // Pos2 vs Pos1
              if (fFrontHE && fFrontLE){
                hPos2vsPos1 -> inc(Pos1comp,Pos2comp);
                // TODO - Add Pos2vsPos1 Gates
              }
            }
          }
          else if (ch == iDE && !fDE){ // DE signal
            fDE = true;
            DE = (int) cDet;
            hDE -> inc(DE);
            vDE.push_back(DE);
            vPos1.push_back(0);
            vSiE.push_back(0);
            vSiDE.push_back(0);
            vSiTotalE.push_back(0);
            DEcomp = (int) std::floor(DE/8.0);

            if (fFrontHE && fFrontLE){
              // DE vs Pos1
              hDEvsPos1 -> inc(Pos1comp,DEcomp);

              // Pos1 - DE vs Pos1 Gates
              Gate &G1 = hDEvsPos1 -> getGate(0);
              //G1.Print();
              if (G1.inGate(Pos1comp,DEcomp)){
                gateCounter++;
                hPos1_gDEvsPos1_G1 -> inc(Pos1);
              }
              Gate &G2 = hDEvsPos1 -> getGate(1);
              //G2.Print();
              if (G2.inGate(Pos1comp,DEcomp)){
                gateCounter++;
                hPos1_gDEvsPos1_G2 -> inc(Pos1);
              }
            }

            if (fE){
              // DE vs E
              hDEvsE -> inc(Ecomp,DEcomp);
              
              if (fFrontHE && fFrontLE){
                // Pos1 - DE vs E Gates
                Gate &G4 = hDEvsE -> getGate(0);
                //G4.Print();
                if (G4.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsE_G1 -> inc(Pos1);
                }
                Gate &G5 = hDEvsE -> getGate(1);
                //G5.Print();
                if (G5.inGate(Ecomp,DEcomp)){
                  gateCounter++;
                  hPos1_gDEvsE_G2 -> inc(Pos1);
                }        
              }
            }
          }
        }
        /*
        // TODO - We'll probably never need to trigger on the back. Skip this for now.
        else if (FPTrigger == iBackHE || FPTrigger == iBackLE){ // Back Trigger
          if (ch == iE && !fE){ // E signal

          }
          else if (ch == iFrontHE && !fFrontHE){ // Pos1 HE signal

          }
          else if (ch == iFrontLE && !fFrontLE){ // Pos1 LE signal

          }
          else if ((ch == iBackHE && !fBackHE) || (ch == iBackLE && !fBackLE)){ // Pos2 HE or LE signal

          }
          else if (ch == iDE && !fDE){ // DE signal

          }
        }
        */
      }
      // Previous window ended. Now open new window.
      else if (ch == FPTrigger){
        winStart = time;
        if (FPTrigger == iE){ // E Trigger
          E = (int) cDet;
          hE -> inc(E);
          vPos1.push_back(0);
          vDE.push_back(0);
          vSiE.push_back(0);
          vSiDE.push_back(0);
          vSiTotalE.push_back(0);
          Ecomp = (int) std::floor(E/8.0);
          fE = true;
          fDE = false;
          fFrontHE = false;
          fFrontLE = false;
          fBackHE = false;
          fBackLE = false;
          fTheta = false;

          // E Gate
          Gate &G = hE -> getGate(0);
          //G.Print();
          if (G.inGate(E)){
            hE_gE_G1 -> inc(E);
          }
        }
        else if (FPTrigger == iFrontHE || FPTrigger == iFrontLE){ // Pos1 trigger
          if (ch == iFrontHE){
            fFrontHE = true;
            fFrontLE = false;
            FrontHE = time;
          }
          else if (ch == iFrontLE){
            fFrontLE = true;
            fFrontHE = false;
            FrontLE = time;
          }
          fE = false;
          fDE = false;
          fBackHE = false;
          fBackLE = false;
          fTheta = false;
        }
        /*
        // TODO - We'll probably never need to trigger on the back. Skip this for now.
        else if (FPTrigger == iBackHE || FPTrigger == iBackLE){ // Pos2 trigger
          if (ch == iBackHE){
            fBackHE = true;
            fBackLE = false;
            BackHE = time;
          }
          else if (ch == iBackLE){
            fBackLE = true;
            fBackHE = false;
            BackLE = time;
          }
          fE = false;
          fDE = false;
          fFrontHE = false;
          fFrontLE = false;
          fTheta = false;
        }
        */
      }
    }
    else{
      std::cout << "Error: Unknown Channel" << std::endl;
    }
  }
}
//======================================================================

void EngeSort::FillEndOfRun(int nEvents){

  // TODO - Verify nEvents is correct. Should be nADC/2?
  //        But the v785 version of fTotalEventCounter increments every sort routine call

  int nEventsInGate_P1 = 0;
  int nEventsInGate_Si = 0;
  int nEvents_P1 = 0;
  int nEvents_Si = 0;
  double fracEventsInGate_P1 = 0;
  double fracEventsInGate_Si = 0;
  double fracBCI = 0;
  bool bPos1vsEventGate = false;

  double EvtCompression = (double)Channels2D/(double)nEvents;
  double Compression = (double)Channels2D/(double)Channels1D;

  std::cout << "To convert x to event in Pos 1 vs Event spectrum: Event # = x * " <<
    1/EvtCompression << std::endl;

  for(int i=0; i<nEvents; i++){
    int e = (int) std::floor(EvtCompression * i);
    int p = (int) std::floor(Compression * vPos1[i]);
    int de = (int) std::floor(Compression * vDE[i]);
    int sie = (int) std::floor(Compression * vSiE[i]);
    int side = (int) std::floor(Compression * vSiDE[i]);
    int sitotale = (int) std::floor(Compression * vSiTotalE[i]);

    hPos1vsEvt -> inc(e, p);

    Gate &G4 = hPos1vsEvt->getGate(0);
    if(G4.inGate(e, p)){
      gateCounter++;
      hPos1_gPos1vsEvt->inc(vPos1[i]); // Pos1
      hDEvsPos1_G_Pos1vsEvt->inc(p,de); // DEcomp vs Pos1comp
      nEventsInGate_P1++;
      bPos1vsEventGate = true;
    }

    // Cutting the same events from the SiTotalE spectrum as are cut from the Pos1 spectrum, so the P1 peak / Si peak ratio is consistent.
    // Trick: Using Pos1vsEvt gate, but checking if (e,sitotale) is in the gate. The y-axis value doesn't matter since the gate spans the entire y-axis.
    // An alternative solution would be to make a separate histogram of SiTotalE vs Event and gate on this too, 
    // but this would require the Pos1vsEvent and SiTotalEvsEvent histograms to have identical gates to get the identical events.
    if(G4.inGate(e, sitotale)){
      hSiTotalE_G_Pos1vsEvt->inc(vSiTotalE[i]); // SiTotalE
      nEventsInGate_Si++;
    }

    Gate &G5 = hDEvsPos1->getGate(1); // Same as G2, which is the default DEvsPos1 for some reason
    if(G4.inGate(e, p) && G5.inGate(p, de)){ // Double gate - DEvsPos1 and Pos1vsEvent
      hPos1_gDEvPos1_gPos1vsEvt->inc(vPos1[i]); // Pos1
    }

    Gate &G6 = hSiDEvsSiE->getGate(0);
    // Double Gate - SiDEvsSiE and Pos1vsEvent (same trick as above to get Si events in the same gate as Pos1 vs Event)
    if(G4.inGate(e, sitotale) && G6.inGate(sie, side)){
      hSiTotalE_G_SiDEvsSiE_G_Pos1vsEvt->inc(vSiTotalE[i]); // SiTotalE
    }

    if(G6.inGate(sie,side)){
      hSiTotalEvsEvt_G_SiDEvsSiE->inc(e, sitotale);
    }

    if(p>0)
      nEvents_P1++;
    if(sitotale>0)
      nEvents_Si++;

  }
  // Clear vectors
  vPos1.clear();
  vDE.clear();
  vSiE.clear();
  vSiDE.clear();
  vSiTotalE.clear();

  Gate &G4 = hPos1vsEvt->getGate(0);
  if(bPos1vsEventGate){
    // Be careful not to interpret the min/max event gate channels as the total rectangular gate. 
    // It's possible (and often necessary) to create two rectangles with a single gate, to cut out more than one region.
    // This is why getting the fraction of events in the gate is a better strategy to calculate the new BCI, clock, and clocklive,
    // but the Pos1 and SiTotalE fractions are not identical (they are close). Take the average of these fractions as the new BCI, clock, and clocklive fraction to use.
    std::cout << "Pos1vsEvent Gate Min/Max Event Chs: " << std::round(G4.getMinx()) << " to " << std::round(G4.getMaxx()) << std::endl;
    fracEventsInGate_P1 = (double) nEventsInGate_P1 / (double) nEvents_P1;
    fracEventsInGate_Si = (double) nEventsInGate_Si / (double) nEvents_Si;
    std::cout << "Number of events in Pos1vsEvent Gate: " << nEventsInGate_P1 << std::endl;
    std::cout << "Number of events in SiTotalEvsEvent Gate: " << nEventsInGate_Si << std::endl;
    std::cout << "Total Number of Pos1 Events: " << nEvents_P1 << std::endl;
    std::cout << "Total Number of SiTotalE Events: " << nEvents_Si << std::endl;
    std::cout << "Fraction of Events in Pos1vsEvent Gate: " << fracEventsInGate_P1 << std::endl;
    std::cout << "Fraction of Events in SiTotalEvsEvent Gate: " << fracEventsInGate_Si << std::endl;
    fracBCI = (fracEventsInGate_P1 + fracEventsInGate_Si) / 2;
    std::cout << "New BCI, Clock, and Clocklive fraction = " << fracBCI << std::endl;
  }
}

/* // TODO - How do scalers change between v785 and v1730?
// Increment the scalers
// TODO: make this automatic. If the scaler is
// defined we should assume that the user wants to increment it
void EngeSort::incScalers(uint32_t *dSCAL){

  //std::cout << "Incrementing scalers" << std::endl;
  
  sGates -> inc(dSCAL);
  sGatesLive -> inc(dSCAL);
  sClock-> inc(dSCAL);
  sClockLive -> inc(dSCAL);
  sFrontLE -> inc(dSCAL);
  sFrontHE -> inc(dSCAL);
  sBackLE -> inc(dSCAL);
  sBackHE -> inc(dSCAL);
  sE -> inc(dSCAL);
  sDE -> inc(dSCAL);
  BCI -> inc(dSCAL);
}
*/

// Connect the analyzer to midas
int EngeSort::connectMidasAnalyzer(){

  TARegister tar(&mAMod);

  mAMod.ConnectEngeAnalyzer(this);
  return 0;
}

// Run the midas analyzer
int EngeSort::runMidasAnalyzer(boost::python::list file_list){
  std::cout << "runMidasAnalyzer " << len(file_list) << std::endl;;
  // We need to send a dummy argument to manalyzer, which gets ignored
  std::string filename = "dummy ";
  for(int i=0; i<len(file_list); i++){
    std::string file = boost::python::extract<std::string>(file_list[i]);
    std::cout << " " << file;
    filename += file + " ";
  }
  std::cout << std::endl;
  std::cout << filename << std::endl;
  
  enum { kMaxArgs = 64 };
  int ac=0;
  char *av[kMaxArgs];

  char *dup = strdup(filename.c_str());
  char *p2 = strtok(dup, " ");

  while (p2 && ac < kMaxArgs-1){
    av[ac++] = p2;
    p2=strtok(0, " ");
  }
  av[ac]=0;
  
  Py_BEGIN_ALLOW_THREADS
    manalyzer_main(ac,av);
    //    manalyzer_main(0,0);
  Py_END_ALLOW_THREADS
    
  return 0;
}

// Return a vector of spectrum names
StringVector EngeSort::getSpectrumNames(){

  StringVector s;
  for(auto h: Histograms){
    s.push_back(h -> getName());
  }

  return s;
}

// Return a vector of spectrum names
StringVector EngeSort::getScalerNames(){

  StringVector s;
  for(auto Sclr: Scalers){
    s.push_back(Sclr -> getName());
  }

  return s;
}

// Return a bool vector of whether the spectra are 2D
BoolVector EngeSort::getis2Ds(){

  BoolVector is2d;
  for(auto h: Histograms){
    bool b = (h -> getnDims() == 2) ? true : false;
    is2D.push_back(b);
  }

  return is2D;
}
// Return a bool vector of whether the spectra have gates
IntVector EngeSort::getNGates(){

  IntVector ngates;
  for(auto h: Histograms){
    ngates.push_back(h -> getNGates());
  }

  return ngates;
}

// Return a vector of scalers
IntVector EngeSort::getScalers(){

  IntVector sclr;
  for(auto sc: Scalers){
    sclr.push_back(sc -> getValue());
  }

  return sclr;
}

np::ndarray EngeSort::getData(){

  // Create the matrix to return to python
  u_int n_rows = nHist1D;//Histograms.size();
  u_int n_cols = Channels1D; //Histograms[0].getnChannels();
  p::tuple shape = p::make_tuple(n_rows, n_cols);
  p::tuple stride = p::make_tuple(sizeof(int));
  np::dtype dtype = np::dtype::get_builtin<int>();
  p::object own;
  np::ndarray converted = np::zeros(shape, dtype);
  
  
  // Loop through all histograms and pack the 1D histograms into a numpy matrix
  int i=0;
  for(auto h: Histograms){
    if(h -> getnDims()==1){
	    //h.Print(1000,2000);
	    shape = p::make_tuple(h -> getnChannels());
	    converted[i] = np::from_data(h -> getData1D().data(), dtype, shape, stride, own);
	    i++;
    }
  }

  return converted;
}

np::ndarray EngeSort::getData2D(){

  // Create the 3D matrix to return to python
  u_int n_t = nHist2D; //DataMatrix2D.size();
  u_int n_rows = Channels2D; //DataMatrix2D[0].size();
  u_int n_cols = Channels2D; //DataMatrix2D[0][0].size();

  //  std::cout << n_t << " " << n_rows << " " << n_cols << std::endl;

  p::tuple shape = p::make_tuple(n_t, n_rows, n_cols);
  p::tuple stride = p::make_tuple(sizeof(int));
  np::dtype dtype = np::dtype::get_builtin<int>();
  p::object own;
  np::ndarray converted = np::zeros(shape, dtype);

  //  for(u_int t = 0; t<n_t; t++){
  int t=0;
  for(auto h: Histograms){
    if(h -> getnDims()==2){
      for (int i = 0; i < h -> getnChannels(); i++){
	      shape = p::make_tuple(h -> getnChannels());
	      converted[t][i] = np::from_data(h -> getData2D()[i].data(), dtype, shape, stride, own);
	    }
      t++;
    }
  }
  return converted;
}

StringVector EngeSort::getGateNames(std::string hname){

  StringVector gname;
  
  // First find the spectrum that corresponds to the hname
  for(auto h:Histograms){
    if(h -> getName() == hname){
      //std::cout << "Hist: " << hname << " has " << h -> getNGates() << " gates" << std::endl;
      // Make sure this histogram has gates defined
      for(int i=0; i< (h -> getNGates()); i++){
	      Gate G1 = h->getGate(i);
	      gname.push_back(G1.getName());
      }
    }
  }

  return gname;
}

void EngeSort::putGate(std::string name, std::string gname, p::list x, p::list y){

  //std::cout << "Putting gate: " << gname << " into histogram " << name << std::endl;
  
  // First find the spectrum that corresponds to the name
  for(auto h:Histograms){
    if(h -> getName() == name){
      //std::cout << "Found the histogram! With name: " << h->getName() << std::endl;

      // Make sure this histogram has gates defined
      //if(h -> getNGates() > 0){}
      //std::cout << "Yes, this histogram has gates!" << std::endl;
      for(int ig = 0; ig < (h -> getNGates()); ig++){
	      Gate &G1 = h->getGate(ig);
	      if(G1.getName() == gname){

	        G1.Clear();
	        //G1.Print();

	        p::ssize_t len = p::len(x);
	        // Make a vector for the gate
	        for(int i=0; i<len; i++){
	          std::vector<double> tmp;
	          tmp.push_back(p::extract<double>(x[i]));
	          tmp.push_back(p::extract<double>(y[i]));
	          G1.addVertex(tmp);
	        }
	        //G1.Print();
	      }
      }
    }
  }
}

void EngeSort::ClearData(){

  for(auto h:Histograms){
    h->Clear();
  }

 /*
  for(auto Sclr: Scalers){
    Sclr -> Clear();
  }
 */
  
  totalCounter=0;
  gateCounter=0;
  
}

//----------------------------------------------------------------------

/* 
   manalyzer module
*/
void MidasAnalyzerModule::Init(const std::vector<std::string> &args){

  printf("Initializing Midas Analyzer Module\n");
  printf("Arguments:\n");
  for (unsigned i=0; i<args.size(); i++)
    printf("arg[%d]: [%s]\n", i, args[i].c_str());

  fTotalEventCounter = 0;
}
void MidasAnalyzerModule::Finish(){
  printf("Finish!");
  eA->FillEndOfRun(fTotalEventCounter);
  //printf("Counted %d events\n",fTotalEventCounter);
  //std::cout << "number of spectra: " << eA->getSpectrumNames().size() << std::endl;
  eA->setIsRunning(false);
}
TARunObject* MidasAnalyzerModule::NewRunObject(TARunInfo* runinfo){
  printf("NewRunObject, run %d, file %s\n",runinfo->fRunNo, runinfo->fFileName.c_str());
  eA->setIsRunning(true);
  return new MidasAnalyzerRun(runinfo, this);
}
TAFlowEvent* MidasAnalyzerRun::Analyze(TARunInfo* runinfo, TMEvent* event,
				    TAFlags* flags, TAFlowEvent* flow){

  if(event->event_id == 1){

    event->FindAllBanks();
    //std::cout << event->BankListToString() << std::endl;

    // Get the ADC Bank
    TMBank* bADC = event->FindBank("V730");
    uint32_t* dADC = (uint32_t*)event->GetBankData(bADC);
    TMBank* bTDC = event->FindBank("TDC1");
    uint32_t* dTDC = (uint32_t*)event->GetBankData(bTDC);

    // printf("V1730 Bank: Name = %s, Type = %d, Size = %d\n",&bADC->name[0],
    // bADC->type,bADC->data_size); 

    // uint64_t dat;
    // dat = dADC[0] & 0xFFFF;
    // printf("dADC[0] = 0x%x\n",dat);
    // printf("dADC[0] = %d\n",dat);

    int singleADCSize = 0;
    int singleTDCSize = 0;
    if(bADC->type == 4)singleADCSize = 2;
    if(bADC->type == 6)singleADCSize = 4;
    
    // Find the size of the data
    int nADC = 0;
    int nTDC = 0;
    //if(bADC)nADC=(bADC->data_size - 2)/singleADCSize; // TODO - Why data_size - 2 bytes (16 bits)? Ch and Board Agg headers are already removed
    //if(bTDC)nTDC=(bTDC->data_size - 2)/singleTDCSize;
    if(bADC)nADC=(bADC->data_size)/singleADCSize;
    if(bTDC)nTDC=(bTDC->data_size)/singleTDCSize;

    //std::cout << "nADC = " << nADC << " nTDC = " << nTDC << std::endl;
  
    fRunEventCounter++;
    //fModule->fTotalEventCounter++;

    if (extras){
      fModule->fTotalEventCounter += (int) nADC/3; // TODO - 3 memory locations (Timetag, qlong, and EXTRAS) per event?
    }
    else{
      fModule->fTotalEventCounter += (int) nADC/2; // TODO - 2 memory locations (Timetag and qlong) per event?
    }

    fModule->eA->sort(dADC, nADC, dTDC, nTDC);

  } else if(event->event_id == 2){

    // TODO - How do scalers change between v785 and v1730?
    std::cout << "This is a scaler event. It should never happen!" << std::endl;

    /*
    // Get the Scaler Bank
    TMBank* bSCAL = event->FindBank("SCLR");
    uint32_t *dSCAL = (uint32_t*)event->GetBankData(bSCAL);

    fModule->eA->incScalers(dSCAL);
    */
  }
    
  return flow;

}

/* 
   manalyzer run
*/

void MidasAnalyzerRun::BeginRun(TARunInfo* runinfo){
  printf("Begin run %d\n",runinfo->fRunNo);
  //time_t run_start_time = 0; // runinfo->fOdb->odbReadUint32("/Runinfo/Start time binary", 0, 0);
  //printf("ODB Run start time: %d: %s", (int)run_start_time, ctime(&run_start_time));

  auto start = std::chrono::system_clock::now();
  std::time_t start_time = std::chrono::system_clock::to_time_t(start);

  std::cout << "Start time = " << std::ctime(&start_time) << "\n";
  //  time_t run_start_time = run_start_time_binary;
  //  printf("ODB Run start time: %s\n",  run_start_time);

  // Read the parameters
  std::ifstream infile;
  infile.open("Parameters.dat",std::ifstream::in);
  
  infile >> pSiSlope;
  infile.close();

  printf("Silicon slope = %f\n",pSiSlope);

  fRunEventCounter = 0;
}

void MidasAnalyzerRun::EndRun(TARunInfo* runinfo){
  printf("End run %d\n",runinfo->fRunNo);
  printf("Counted %d events\n",fRunEventCounter);
  //std::cout << "Total counts: " << totalCounter << "   Gated counts: " << gateCounter << std::endl;

  //  time_t run_stop_time = runinfo->fOdb->odbReadUint32("/Runinfo/Stop time binary", 0, 0);
  //printf("ODB Run stop time: %d: %s", (int)run_stop_time, ctime(&run_stop_time));

  auto stop = std::chrono::system_clock::now();
  std::time_t stop_time = std::chrono::system_clock::to_time_t(stop);
  std::cout << "Stop time = " << std::ctime(&stop_time) << "\n";

  printf("BCI was %d\n",fModule->eA->getScalers()[10]);
}

BOOST_PYTHON_MODULE(EngeSort)
{
  using namespace boost::python;
  // Initialize numpy
  Py_Initialize();
  boost::python::numpy::initialize();
  //    def( "MakeData", MakeData );

  class_<vec>("double_vec")
    .def(vector_indexing_suite<vec>());
  class_<mat>("double_mat")
    .def(vector_indexing_suite<mat>());
  class_<mat2d>("double_mat2d")
    .def(vector_indexing_suite<mat2d>());
  class_<StringVector>("StringVector")
    .def(vector_indexing_suite<StringVector>());
  class_<BoolVector>("BoolVector")
    .def(vector_indexing_suite<BoolVector>());
  class_<IntVector>("IntVector")
    .def(vector_indexing_suite<IntVector>());
    
  class_<EngeSort>("EngeSort")
    .def("sayhello", &EngeSort::sayhello)          // string
    .def("saygoodbye", &EngeSort::saygoodbye)          // string
    .def("saysomething", &EngeSort::saysomething)      // string
    .def("Initialize", &EngeSort::Initialize)          // void
    .def("connectMidasAnalyzer", &EngeSort::connectMidasAnalyzer) // int
    .def("runMidasAnalyzer", &EngeSort::runMidasAnalyzer) // int
    .def("getData", &EngeSort::getData)                // 1D histograms
    .def("getData2D", &EngeSort::getData2D)            // 2D histograms
    .def("getis2Ds", &EngeSort::getis2Ds)                // bool vector
    .def("getNGates", &EngeSort::getNGates)          // bool vector
    .def("getSpectrumNames", &EngeSort::getSpectrumNames)
    .def("getIsRunning", &EngeSort::getIsRunning)        // bool value
    .def("getScalerNames", &EngeSort::getScalerNames)     // string vector
    .def("getScalers", &EngeSort::getScalers)             // IntVector of scaler values
    .def("getGateNames", &EngeSort::getGateNames)             // string vector of gate names
    .def("ClearData", &EngeSort::ClearData)        // void
    .def("putGate", &EngeSort::putGate)            // void
    .def("data", range(&EngeSort::begin, &EngeSort::end))
    //    .def("Histogram1D", &Histogram1D::Histogram1D)
    ;

}
