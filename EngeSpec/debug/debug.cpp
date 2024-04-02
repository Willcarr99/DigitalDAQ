#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <cmath>
#include <climits> // For INT_MAX

//---------------------------- Settings --------------------------------
//----------------------------------------------------------------------
// Number of channels in 1D and 2D histograms
int Channels1D = 8192; // TODO - Test this scale (was 4096)
int Channels2D = 1024; // TODO - Test this scale (was 512)

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

// FP Trigger (By channel number -- for Pos1 use iFrontHE or iFrontLE. Similarly for Pos2)
int FPTrigger = iE; // Scintillator (E)
// Note: Si triggers on EITHER SiE or SiDE

// Coincidence Windows for Focal Plane and Si Detector (in ns, number must be even)
int win_ns = 1000;
int winSi_ns = 1000;

// Max value of the timetag, at which point it rolls back to 0
// Default timetag for v1730 (EXTRAS disabled) is a 31-bit number. So max is 2^31 - 1 = 2147483647 or 0x7FFFFFFF or INT_MAX
// Each timetag unit is 2 ns (v1730), so roll back time is 2 ns/unit * (2^31 - 1) units ~ 4.295 s
int timetagReset = INT_MAX;

// Scaling coincidence time (for E vs time histograms) by this factor
// timeScale * 2 ns per bin [min = 1 (best resolution - 2 ns), max = 16 (32 ns)]
int timeScale = 1;
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

const double pSiSlope = 0.3; // TODO - Need to read parameters.dat file with pSiSlope value
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

int main(){

  std::cout << "Window: " << win << std::endl;
  std::cout << "Window Si: " << winSi << std::endl;

  // Give example qlong and timetag data for debugging purposes
  const int nADC = 8; // Number of memory locations in single aggregate - 2 per event (max 1023 * 2 = 2046). 4 events --> 8
  // First event:  qlong = 8192 (cDet = 2048), ch = iSiDE (7), timetag = 8 (16 ns)      Starts window
  // Second event: qlong = 8448 (cDet = 2112), ch = iSiE (6),  timetag = 100 (200 ns)  True coincidence.
  // Third event:  qlong = 6536 (cDet = 1634), ch = iSiDE (7), timetag = 200 (400 ns)  Within coincidence window, but ignore.
  // Fourth event: qlong = 37120 (cDet = 9280), ch = iSiE (6),  timetag = 800 (1600 ns)  Outside window. Start new window.
  // uint32_t dADC[nADC] = {0x2000, 0x8, 0x2100, 0x64, 0x1988, 0xC8, 0x9100, 0x320}; // qlong without ch
  uint32_t dADC[nADC] = {0x72000, 0x8, 0x62100, 0x64, 0x71988, 0xC8, 0x69100, 0x320}; // qlong | ch included

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
    //std::cout << "Event #" << (i+2)/2 << " qlong_ch dADC = " << dADC[i] << std::endl;
    //std::cout << "Event #" << (i+2)/2 << " timetag dADC = " << dADC[i+1] << std::endl;
    // Extract energy, channel, and timetag from event
    qlong = (int16_t) (dADC[i] & 0xFFFF); // dADC[i] includes channel # and qlong (energy)
    std::cout << "qlong: " << qlong << std::endl;
    ch = (int) (dADC[i] & 0xFFFF0000) >> 16;
    std::cout << "ch: " << ch << std::endl;
    cDet = (int16_t) std::floor(qlong/4.0); // TODO - This maxes out at Channels1D (8,192).
    std::cout << "cDet: " << cDet << std::endl;
    time = (int) dADC[i+1];
    std::cout << "timetag: " << time << std::endl;

    // Handle noise for E, DE, SiE, and SiDE (Pos1 and Pos2 handled separately below)
    if (cDet < Thresh || cDet > Channels1D - 1){dADC[i]=0;} // TODO - To move 8192 spike to 0, use >=

    //------------------- Si Window -------------------
    // Triggering on either SiE or SiDE
    if (ch == iSiE || ch == iSiDE){ // Si signal above threshold and below Ch max
      // Check if Si coincidence window is still open (accounting for potential rollback to 0)
      if ((time > winStartSi && time < winStartSi + winSi) || (winStartSi > timetagReset - winSi && time > winStartSi + winSi - timetagReset)){
        std::cout << "Event #" << (i+2)/2 << " collected within window" << std::endl;
        // Check if the signal is from SiE and SiDE initiated the window
        if (ch == iSiE && !fSiE && fSiDE){
          // Coincidence! Increment histograms
          fSiE = true;
          SiE = (int) cDet;
          //hSiE -> inc(SiE);
          SiEcomp = (int) std::floor(SiE/8.0); // TODO - This maxes out at 1,024
          std::cout << "SiEcomp: " << SiEcomp << std::endl;
          std::cout << "SiDEcomp: " << SiDEcomp << std::endl;
          //hSiDEvsSiE -> inc(SiEComp,SiDEComp);
          std::cout << "Increment SiE and SiDEvsSiE" << std::endl;
          SiTotalE = (int) std::floor((SiE + pSiSlope*SiDE) / (1.0 + pSiSlope));
          SiTotalEcomp = (int) std::floor(SiTotalE/8.0);
          std::cout << "SiTotalE: " << SiTotalE << std::endl;
          std::cout << "SiTotalEcomp: " << SiTotalEcomp << std::endl;
          //Gate &G3 = hSiDEvsSiE -> getGate(0);
          ////G3.Print();
          //if (G3.inGate(SiEcomp,SiDEcomp)){
          //  gateCounter++;
          //  hSiE_G_SiDEvsSiE -> inc(SiE);
          //  hSiTotalE_G_SiDEvsSiE -> inc(SiTotalE);
          //  hSiDEvsSiTotalE_G_SiDEvsSiE -> inc(SiTotalEcomp,SiDEcomp);
          //}
        }
        // Check if the signal is from SiDE and SiE initiated the window
        else if (ch == iSiDE && !fSiDE && fSiE){
          // Coincidence! Increment histograms
          fSiDE = true;
          SiDE = (int) cDet;
          //hSiDE -> inc(SiDE);
          SiDEcomp = (int) std::floor(SiDE/8.0); // TODO - This maxes out at 1,024
          std::cout << "SiEcomp: " << SiEcomp << std::endl;
          std::cout << "SiDEcomp: " << SiDEcomp << std::endl;
          //hSiDEvsSiE -> inc(SiEComp,SiDEComp);
          std::cout << "Increment SiDE and SiDEvsSiE" << std::endl;
          SiTotalE = (int) std::floor((SiE + pSiSlope*SiDE) / (1.0 + pSiSlope));
          SiTotalEcomp = (int) std::floor(SiTotalE/8.0);
          std::cout << "SiTotalE: " << SiTotalE << std::endl;
          std::cout << "SiTotalEcomp: " << SiTotalEcomp << std::endl;
          //Gate &G3 = hSiDEvsSiE -> getGate(0);
          ////G3.Print();
          //if (G3.inGate(SiEcomp,SiDEcomp)){
          //  gateCounter++;
          //  hSiE_G_SiDEvsSiE -> inc(SiE);
          //  hSiTotalE_G_SiDEvsSiE -> inc(SiTotalE);
          //  hSiDEvsSiTotalE_G_SiDEvsSiE -> inc(SiTotalEcomp,SiDEcomp);
          //}
        }
      }
      else {
        // Previous window ended. Now open new window.
        std::cout << "Event #" << (i+2)/2 << " opened new window" << std::endl;
        winStartSi = time;
        if (ch == iSiE){
          SiE = (int) cDet;
          //hSiE -> inc(SiE); // TODO - Do we want to increment histograms even if no coincidence occurred?
          std::cout << "Increment SiE" << std::endl;
          SiEcomp = (int) std::floor(SiE/8.0); // TODO - This maxes out at 1,024
          fSiE = true;
          fSiDE = false;
        }
        else if (ch == iSiDE){
          SiDE = (int) cDet;
          //hSiDE -> inc(SiDE); // TODO - Do we want to increment histograms even if no coincidence occurred?
          std::cout << "Increment SiDE" << std::endl;
          SiDEcomp = (int) std::floor(SiDE/8.0); // TODO - This maxes out at 1,024
          fSiDE = true;
          fSiE = false;
        }
      }
    }
  }
}
