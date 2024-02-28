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
int Channels1D = 8192;
int Channels2D = 512;

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
int win_ns = 5000;
int winSi_ns = 5000;

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
uint32_t FrontHE;
uint32_t FrontLE;
uint32_t BackHE;
uint32_t BackLE;
int E;
int DE;
int SiE;
int SiDE;

int Pos1; // FrontHE - FrontLE (+ offset to center)
int Pos2; // BackHE - BackLE (+ offset to center)
int Theta; // TODO - May need to fix definition of Theta below

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
  // Second event: qlong = 8448 (cDet = 2112), ch = iSiE (6),  timetag = 1000 (2000 ns)  True coincidence.
  // Third event:  qlong = 6536 (cDet = 1634), ch = iSiDE (7), timetag = 2000 (4000 ns)  Within coincidence window, but ignore.
  // Fourth event: qlong = 4352 (cDet = 1088), ch = iSiE (6),  timetag = 2600 (5200 ns)  Outside window. Start new window.
  // uint32_t dADC[nADC] = {0x2000, 0x8, 0x2100, 0x3E8, 0x1988, 0x7D0, 0x1100, 0xA28}; // qlong without ch
  uint32_t dADC[nADC] = {0x72000, 0x8, 0x62100, 0x3E8, 0x71988, 0x7D0, 0x61100, 0xA28}; // qlong | ch included

  int qlong, ch, time, cDet;
  // Window start time - gets updated each trigger. The initial value here prevents having to do an extra if statement for the first trigger in the buffer.
  int winStart = -win - 1;
  int winStartSi = -winSi - 1;

  for (int i = 0; i<nADC; i++){
    std::cout << dADC[i] << std::endl;
  }

  // Energies (and Ch #) are EVEN nADC counts, timetags are ODD counts (see ReadQLong in v1730DPP.c)
  for(int i = 0; i<nADC; i+=2){

    // Extract energy, channel, and timetag from event
    qlong = (int) dADC[i] & 0xFFFF; // dADC[i] includes channel # and qlong (energy)
    std::cout << "qlong: " << qlong << std::endl;
    ch = (int) (dADC[i] & 0xFFFF0000) >> 16;
    std::cout << "ch: " << ch << std::endl;
    cDet = (int) std::floor(qlong/4.0); // TODO - This maxes out at 16,383, 2x Channels1D (8,192). Make sure this is okay.
    std::cout << "cDet: " << cDet << std::endl;
    time = (int) dADC[i+1];
    std::cout << "time: " << time << std::endl;

    // Handle noise for E, DE, SiE, and SiDE (Pos1 and Pos2 handled separately below)
    if (cDet < Thresh || cDet > Channels1D){dADC[i]=0;}

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
          SiE = cDet;
          //hSiE -> inc(SiE);
          int SiEComp = (int) std::floor(SiE/4.0);
          int SiDEComp = (int) std::floor(SiDE/4.0);
          //hSiDEvsSiE -> inc(SiEComp,SiDEComp);
          std::cout << "Increment SiE and SiDEvsSiE" << std::endl;
        }
        // Check if the signal is from SiDE and SiE initiated the window
        else if (ch == iSiDE && !fSiDE && fSiE){
          // Coincidence! Increment histograms
          fSiDE = true;
          SiDE = cDet;
          //hSiDE -> inc(SiDE);
          int SiEComp = (int) std::floor(SiE/4.0);
          int SiDEComp = (int) std::floor(SiDE/4.0);
          //hSiDEvsSiE -> inc(SiEComp,SiDEComp);
          std::cout << "Increment SiDE and SiDEvsSiE" << std::endl;
        }
      }
      else {
        // Previous window ended. Now open new window.
        std::cout << "Event #" << (i+2)/2 << " opened new window" << std::endl;
        winStartSi = time;
        if (ch == iSiE){
          SiE = cDet;
          //hSiE -> inc(SiE);
          std::cout << "Increment SiE" << std::endl;
          fSiE = true;
          fSiDE = false;
        }
        else if (ch == iSiDE){
          SiDE = cDet;
          //hSiDE -> inc(SiDE);
          std::cout << "Increment SiDE" << std::endl;
          fSiDE = true;
          fSiE = false;
        }
      }
    }
  }
}
