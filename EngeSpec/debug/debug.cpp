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
#include <vector>
#include <algorithm>

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
int window_ns = 1000;
int windowSi_ns = 1000;

// Max value of the timetag, at which point it rolls back to 0
// Default timetag for v1730 (EXTRAS disabled) is a 31-bit number. So max is 2^31 - 1 = 2147483647 or 0x7FFFFFFF or INT_MAX
// Each timetag unit is 2 ns (v1730), so roll back time is 2 ns/unit * (2^31 - 1) units ~ 4.295 s
int t_reset = INT_MAX;

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
int window = (int) (window_ns/2.0);
int windowSi = (int) (windowSi_ns/2.0);

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

  std::cout << "Window: " << window << std::endl;
  std::cout << "Window Si: " << windowSi << std::endl;

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
  int winStart = -window - 1;
  int winStartSi = -windowSi - 1;

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
    if (cDet < Thresh || cDet > Channels1D - 1){cDet=0;} // TODO - To move 8192 spike to 0, use >=

    //------------------- Si Window -------------------
    // Triggering on either SiE or SiDE
    if (ch == iSiE || ch == iSiDE){ // Si signal above threshold and below Ch max
      // Check if Si coincidence window is still open (accounting for potential rollback to 0)
      if ((time > winStartSi && time < winStartSi + windowSi) || (winStartSi > t_reset - windowSi && time > winStartSi + windowSi - t_reset)){
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

  // ************************************************************************************************************

  // Testing foward and backward coincidence window

  std::vector<int16_t> energies;
  std::vector<uint32_t> timetags;
  std::vector<int> digitizer_chs;
  std::vector<int> trigger_indices;
  int trigger_ch = iE;

  // Give example qlong and timetag data for debugging purposes
  const int num = 18; // Number of buffers for each readout (each board aggregate), max = 2 [memory locations/event] * 1023 [events/ch agg] * 8 [ch aggs/board agg] = 16368.
  // First event:  qlong = 5000 (cDet = 1250), ch = iFLE (1), timetag = 30 (60 ns)
  // Second event: qlong = 5000 (cDet = 1250), ch = iFHE (0), timetag = 40 (80 ns)
  // Third event:  qlong = 5000 (cDet = 1250), ch = iE (4),   timetag = 50 (100 ns)
  // Fourth event: qlong = 5000 (cDet = 1250), ch = iDE (5),  timetag = 60 (120 ns)
  // Fifth event:  qlong = 5000 (cDet = 1250), ch = iBHE (2), timetag = 70 (140 ns)
  // Sixth event:  qlong = 5000 (cDet = 1250), ch = iBLE (3), timetag = 80 (160 ns)
  // Seventh event:qlong = 5000 (cDet = 1250), ch = iE (4),   timetag = 90 (180 ns)
  // Eighth event: qlong = 5000 (cDet = 1250), ch = iFHE (0), timetag = 100 (200 ns)
  // Nineth event: qlong = 5000 (cDet = 1250), ch = iE (4),   timetag = 600 (1200 ns)
  // uint32_t dADC[nADC] = {0x1388, 0x1E, 0x1388, 0x28, 0x1388, 0x32, 0x1388, 0x3C, 0x1388, 0x46, 0x1388, 0x50, 0x1388, 0x5A, 0x1388, 0x64, 0x1388, 0x258}; // qlong without ch
  uint32_t data[num] = {0x11388, 0x1E, 0x1388, 0x28, 0x41388, 0x32, 0x51388, 0x3C, 0x21388, 0x46, 0x31388, 0x50, 0x41388, 0x5A, 0x1388, 0x64, 0x41388, 0x258}; // qlong | ch included

  // Extract energy, channel, and timetag from each event, and extract index if ch is a trigger
  for(int i=0; i<num; i+=2){
    int16_t qlong = (int16_t) (data[i] & 0xFFFF);
    energies.push_back((int16_t) std::floor(qlong/4.0)); // Maxes out at Channels1D (8,192).
    int digitizer_ch = (int) (data[i] & 0xFFFF0000) >> 16;
    digitizer_chs.push_back(digitizer_ch);
    uint32_t timetag = data[i+1];
    timetags.push_back(timetag);

    if (digitizer_ch == trigger_ch){
      trigger_indices.push_back((int) i/2.0);
    }
  }

  // Half coincidence window width
  uint32_t half_window = (uint32_t) (window/2.0);

  // Maximum timetag value, at which point timetags reset
  uint32_t timetag_rollback = (uint32_t) INT_MAX;

  // Previous trigger window end timetag (initial value ensures 1st trigger is not ignored)
  uint32_t timetag_window_stop_previous = 0;

  // Flag indicating previous trigger window extended below 0 or beyond rollback (initial value ensures 1st trigger is not ignored)
  bool is_extended_previous = false;

  // Collect coincidence events for each trigger
  for (int trigger_index : trigger_indices){
    std::cout << "Trigger index: " << trigger_index << std::endl;
    uint32_t timetag_trigger = timetags[trigger_index];
    uint32_t timetag_window_start;
    uint32_t timetag_window_stop;
    bool is_extended = false;
    bool is_below_zero = false;
    bool is_above_rollback = false;
    
    // Find window start and stop timetags ... if they don't extend below 0 or beyond rollback
    if (timetag_trigger >= half_window && timetag_trigger <= timetag_rollback - half_window){
      timetag_window_start = timetag_trigger - half_window;
      timetag_window_stop = timetag_trigger + half_window;
    }
    // ... if window extends below 0
    else if (timetag_trigger < half_window){
      timetag_window_start = timetag_rollback - (half_window - timetag_trigger);
      timetag_window_stop = timetag_trigger + half_window;
      is_extended = true;
      is_below_zero = true;
    }
    // ... if window extends beyond rollback timetag
    else if (timetag_trigger > timetag_rollback - half_window){
      timetag_window_start = timetag_trigger - half_window;
      timetag_window_stop = timetag_trigger + half_window - timetag_rollback;
      is_extended = true;
      is_above_rollback = true;
    }

    // Inhibit trigger if current window overlaps with the previous window. See Offline_Coincidences.pdf for explanation
    if (timetag_window_start < timetag_window_stop_previous && !(is_extended && is_extended_previous)){ // At least one trigger does not extend beyond 0 or timetag_rollback
      continue;
    }
    else if (timetag_window_start > timetag_window_stop_previous && is_extended && is_extended_previous){ // Both triggers extend beyond 0 or timetag_rollback
      continue;
    }
    // Note: If timetag_window_start == timetag_window_stop_previous and an event happens to occur at that timetag, it is collected by the previous event (see lines 313 vs 336)
    // Note: If a trigger is inhibited, the next trigger window is compared with the last uninhibited trigger window

    // Update trigger info for next trigger to check
    timetag_window_stop_previous = timetag_window_stop;
    is_extended_previous = is_extended;

    bool in_window = true;
    bool increment_backwards = true;
    int event_increment = 0;
    int event_index;
    uint32_t timetag;

    while(in_window){
      event_increment++;
      // Look for events before trigger (between trigger and start of window)
      if (increment_backwards){
        event_index = trigger_index - event_increment;
      }
      // Look for events after trigger (between trigger and end of window)
      else{
        event_index = trigger_index + event_increment;
      }
      // Ensure we have not indexed below the first event or above the last event in the board aggregate
      if (event_index >= 0 && event_index <= (int)num/2){
        timetag = timetags[event_index];
        
        // Check if the next event is in the window, taking into account rollback extension and whether we are incrementing forward or backward
            // ...window does not extend below 0 or above rollback timetag...
        if ((increment_backwards && !is_extended && (timetag >= timetag_window_start && timetag <= timetag_trigger)) ||
            (!increment_backwards && !is_extended && (timetag < timetag_window_stop && timetag > timetag_trigger)) ||
            // ... window extends below 0
            (increment_backwards && is_below_zero && (timetag >= timetag_window_start || timetag <= timetag_trigger )) ||
            (!increment_backwards && is_below_zero && (timetag < timetag_window_stop && timetag > timetag_trigger)) ||
            // ... window extends above rollback timetag
            (increment_backwards && is_above_rollback && (timetag >= timetag_window_start && timetag <= timetag_trigger)) ||
            (!increment_backwards && is_above_rollback && (timetag < timetag_window_stop || timetag > timetag_trigger))){

          // Inhibit additional trigger(s) within coincidence window
          if (digitizer_chs[event_index] == trigger_ch){
            std:: cout << "Inhibited Trigger: Ch = " << digitizer_chs[event_index] << ", Index = " << event_index << ", Timetag = " << timetag << std::endl;
            continue;
          }
          // ***********************************************************************************************
          // Event is within window. Increment histogram(s) here (or save event to vector for later)
          
          // ***********************************************************************************************
          std::cout << "Timetag of event #" << event_index << " = " << timetag << std::endl;
        }
        else{
          // Event not within window
          if (increment_backwards){
            // Reset and start searching forwards
            increment_backwards = false;
            event_increment = 0;
          }
          else{
            // Collected all events in the window
            in_window = false;
          }
        }
      }
      else{
        // Reached either the first or last event in the board aggregate
        if (increment_backwards){
          // Reset and start searching forwards
          increment_backwards = false;
          event_increment = 0;
        }
        else{
          // Collected all events in the window
          in_window = false;
        }
      } 
    }
  }
}