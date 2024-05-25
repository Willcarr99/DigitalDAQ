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

int main(){

  // ************************************************************************************************************

  // Testing foward and backward in time coincidence window with EXTRAS word enabled

  // ************************************************************************************************************
  // Sort Routine Settings
  
  // Number of channels in 1D and 2D histograms
  int Channels1D = 8192; // TODO - Test this scale (was 4096)
  int Channels2D = 1024; // TODO - Test this scale (was 512)

  double Compression = 8.0; // Compression for 2D histograms, 

  int Histogram_Threshold = 20;

  // Define the channels (0-15)
  int iFrontHE = 0; // Pos1 (Front) High-Energy
  int iFrontLE = 1; // Pos1 (Front) Low-Energy
  int iBackHE = 2;  // Pos2 (Back) High-Energy
  int iBackLE = 3;  // Pos2 (Back) Low-Energy
  int iE = 4;       // Scintillator (E)
  int iDE = 5;      // Proportionality Counter (Delta E)
  int iSiE = 6;     // Silicon Detector (E)
  int iSiDE = 7;    // Silicon Detector (Delta E)

  int FP_trigger_ch = iE; // or iFrontHE
  int Si_trigger_ch = iSiE; // or iSiDE

  // Coincidence Windows for Focal Plane and Si Detector (in ns, number must be even)
  int window_FP_ns = 1000;
  int window_Si_ns = 1000;

  // Scaling coincidence time (for E vs time histograms) by this factor
  // timeScale * 2 ns per bin [min = 1 (best resolution - 2 ns), max = 16 (32 ns)]
  int timeScale = 1;

  // Slope of Si spectrum
  const double pSiSlope = 0.3; // TODO - Need to read parameters.dat file with pSiSlope value

  // EXTRAS Recording Enabled (True) or Disabled (False)
  bool extras = true;
  // 32-bit word: bits[31:16] = Extended Time Stamp, bits[15:10] = Flags, bits[9:0] = Fine Time Stamp

  // ************************************************************************************************************
  // Global sort routine variables

  // Converting window duration to timetag units (2 ns/unit - v1730)
  uint32_t window_FP = (uint32_t) (window_FP_ns/2.0);
  uint32_t window_Si = (uint32_t) (window_Si_ns/2.0);

  // Half coincidence window width
  uint32_t half_window_FP = (uint32_t) (window_FP/2.0);
  uint32_t half_window_Si = (uint32_t) (window_Si/2.0);

  // Maximum timetag value, at which point timetags reset
  // Default timetag for v1730 (EXTRAS disabled) is a 31-bit number. So max is 2^31 - 1 = 2147483647 or 0x7FFFFFFF or INT_MAX
  // Each timetag unit is 2 ns (v1730), so roll back time is 2 ns/unit * (2^31 - 1) units ~ 4.295 s
  uint32_t timetag_rollback = (uint32_t) INT_MAX;

  // Previous trigger window end timetag (initial value ensures 1st trigger is not ignored)
  uint32_t timetag_window_stop_previous_FP = 0;
  uint32_t timetag_window_stop_previous_Si = 0;

  // Flag indicating previous trigger window extended below 0 or beyond rollback (initial value ensures 1st trigger is not ignored)
  bool is_extended_previous_FP = false;
  bool is_extended_previous_Si = false;

  std::vector<int16_t> energies;
  std::vector<uint32_t> timetags;
  std::vector<int> digitizer_chs;
  std::vector<int> trigger_indices;

  // ************************************************************************************************************

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

    if (digitizer_ch == FP_trigger_ch || digitizer_ch == Si_trigger_ch){
      trigger_indices.push_back((int) i/2.0);
    }
  }

  // Collect coincidence events for each FP or Si trigger
  for (int trigger_index : trigger_indices){
    std::cout << "\nTrigger Index: " << trigger_index << "\n" << std::endl;

    int trigger_ch = digitizer_chs[trigger_index];
    uint32_t half_window;
    // Get window size based on the kind of trigger
    if (trigger_ch == FP_trigger_ch){
      half_window = half_window_FP;
    }
    else if (trigger_ch == Si_trigger_ch){
      half_window = half_window_Si;
    }

    // Timetags of the trigger and its coincidence window start/stop. Bools account for timetag rollback.
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
    if (trigger_ch == FP_trigger_ch){ // FP window
      if (timetag_window_start < timetag_window_stop_previous_FP && !(is_extended && is_extended_previous_FP)){ // At least one trigger does not extend beyond 0 or timetag_rollback
        continue;
      }
      else if (timetag_window_start > timetag_window_stop_previous_FP && is_extended && is_extended_previous_FP){ // Both triggers extend beyond 0 or timetag_rollback
        continue;
      }
    }
    else if (trigger_ch == Si_trigger_ch){ // Si window
      if (timetag_window_start < timetag_window_stop_previous_Si && !(is_extended && is_extended_previous_Si)){ // At least one trigger does not extend beyond 0 or timetag_rollback
        continue;
      }
      else if (timetag_window_start > timetag_window_stop_previous_Si && is_extended && is_extended_previous_Si){ // Both triggers extend beyond 0 or timetag_rollback
        continue;
      }
    }
    // Note: If timetag_window_start == timetag_window_stop_previous and an event happens to occur at that timetag, it is collected by the later event
    // Note: If a trigger is inhibited, the next trigger window is compared with the last uninhibited trigger window
    //       Technically we could dream up a scenario where 2 consecutive (uninhibited) triggers are separated by an entire timetag rollback duration.
    //       This would give a false coincidence, but this duration is ~ 4 seconds, and the windows are < ~5 microseconds, making this unlikely

    // Update trigger info for next trigger to check (must come after overlapping triggers are inhibited)
    if (trigger_ch == FP_trigger_ch){
      timetag_window_stop_previous_FP = timetag_window_stop;
      is_extended_previous_FP = is_extended;
    }
    else if (trigger_ch == Si_trigger_ch){
      timetag_window_stop_previous_Si = timetag_window_stop;
      is_extended_previous_Si = is_extended;
    }

    // ***********************************************************************************************
    // Event loop variables for each coincidence window

    bool in_window = true;
    bool increment_backwards = true;
    int event_increment = 0;

    // Data to save for coincidences. Stays 0 if no corresponding event in window
    uint32_t FrontHE = 0, FrontLE = 0, BackHE = 0, BackLE = 0;
    int E = 0, DE = 0, SiE = 0, SiDE = 0;

    // Flags to prevent more than one signal to be collected from a single component during the coincidence window.
    // True when signal is detected during window.
    bool fFrontHE = false, fFrontLE = false, fBackHE = false, fBackLE = false,
         fE = false, fDE = false, fSiE = false, fSiDE = false;
    
    // ***********************************************************************************************

    // Collect each event in the trigger coincidence window
    while(in_window){

      int event_index;
      uint32_t timetag;
      int event_ch;
      int energy;

      // Look for events before trigger (between trigger and start of window)
      if (increment_backwards){
        event_index = trigger_index - event_increment;
      }
      // Look for events after trigger (between trigger and end of window)
      else{
        event_index = trigger_index + event_increment;
      }
      event_increment++;
      // Ensure we have not indexed below the first event or above the last event in the board aggregate
      if (event_index >= 0 && event_index <= (int)num/2){
        timetag = timetags[event_index];
        event_ch = digitizer_chs[event_index];
        energy = (int) energies[event_index];

        // Skip if Si event in FP window
        if ((event_ch == iSiE || event_ch == iSiDE) && trigger_ch == FP_trigger_ch){
          std::cout << "Si Event in FP window: Ch = " << event_ch << ", Index = " << event_index << ", Timetag = " << timetag << std::endl;
          continue;
        }
        // Skip if FP event in Si window
        else if ((event_ch == iE || event_ch == iDE || event_ch == iFrontHE || event_ch == iFrontLE || 
                  event_ch == iBackHE || event_ch == iBackLE) && trigger_ch == Si_trigger_ch){
          std::cout << "FP Event in Si window: Ch = " << event_ch << ", Index = " << event_index << ", Timetag = " << timetag << std::endl;
          continue;
        }

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

          // Bring energy to 0 if signal is below our Histogram threshold or above Channels1D (Pos1 threshold handled separately below)
          if ((event_ch == iE || event_ch == iDE || event_ch == iSiE || event_ch == iSiDE) && 
              (energy < Histogram_Threshold || energy > Channels1D - 1)){
            energy = 0;
          }
                    
          // ***********************************************************************************************
          // Event is within window. Save each event to its correspnding variable

          std::cout << "Timetag of event #" << event_index << " = " << timetag << std::endl;
          std::cout << "Ch of event #" << event_index << " = " << event_ch << std::endl;
          std::cout << "Energy of event #" << event_index << " = " << energy << std::endl;

          if (event_ch == iE && !fE){fE = true; E = energy;}
          else if (event_ch == iDE && !fDE){fDE = true; DE = energy;}
          else if (event_ch == iSiE && !fSiE){fSiE = true; SiE = energy;}
          else if (event_ch == iSiDE && !fSiDE){fSiDE = true; SiDE = energy;}
          else if (event_ch == iFrontHE && !fFrontHE){fFrontHE = true; FrontHE = timetag;}
          else if (event_ch == iFrontLE && !fFrontLE){fFrontLE = true; FrontLE = timetag;}
          else if (event_ch == iBackHE && !fBackHE){fBackHE = true; BackHE = timetag;}
          else if (event_ch == iBackLE && !fBackLE){fBackLE = true; BackLE = timetag;}          
          // ***********************************************************************************************
        }
        else{
          // Event not within window
          if (increment_backwards){
            // Reset and start searching forwards after trigger
            increment_backwards = false;
            event_increment = 1;
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
          event_increment = 1;
        }
        else{
          // Collected all events in the window
          in_window = false;
        }
      } 
    }
    // ***********************************************************************************************
    // Derive info from saved energy/timetag data during the coincidence window, or 0 if nonexistent

    int Ecomp = (int) std::floor(E/Compression);
    int DEcomp = (int) std::floor(DE/Compression);
    int SiEcomp = (int) std::floor(SiE/Compression);
    int SiDEcomp = (int) std::floor(SiDE/Compression);

    int SiTotalE = 0;

    if (fSiE && fSiDE){
      SiTotalE = (int) std::floor((SiE + pSiSlope*SiDE) / (1.0 + pSiSlope));
    }
    int SiTotalEcomp = (int) std::floor(SiTotalE/Compression);

    int Pos1 = 0;
    int Pos2 = 0;
    int Pos1comp = 0;
    int Pos2comp = 0;
    int Theta = 0;

    if (fFrontHE && fFrontLE){
      // Pos = HE - LE, scaled so Max Ch = 1 us and Min Ch = -1 us, offset to center (4,096)
      Pos1 = (int) std::floor(((Channels1D/1000) * ((int) FrontHE - (int) FrontLE))) + (Channels1D/2);
      if (Pos1 < Histogram_Threshold || Pos1 >= Channels1D){Pos1=0;}
    }
    Pos1comp = (int) std::floor(Pos1/Compression);

    if (fBackHE && fBackLE){
      Pos2 = (int) std::floor(((Channels1D/1000) * ((int) BackHE - (int) BackLE))) + (Channels1D/2);
      if (Pos2 < Histogram_Threshold || Pos2 >= Channels1D){Pos2=0;}
    }
    Pos2comp = (int) std::floor(Pos2/Compression);

    if (fFrontHE && fFrontLE && fBackHE && fBackLE){
      Theta = (int) std::round(10000.0*atan((Pos2-Pos1)/100.)/3.1415 - 4000.);
      Theta = std::max(0,Theta);
    }
    int Thetacomp = (int) std::floor(Theta/Compression);

    std::cout << "\nE: " << E << std::endl;
    std::cout << "DE: " << DE << std::endl;
    std::cout << "Pos1: " << Pos1 << std::endl;
    std::cout << "Pos2: " << Pos2 << std::endl;
    std::cout << "SiE: " << SiE << std::endl;
    std::cout << "SiDE: " << SiDE << std::endl;

    std::cout << "Ecomp: " << Ecomp << std::endl;
    std::cout << "DEcomp: " << DEcomp << std::endl;
    std::cout << "Pos1comp: " << Pos1comp << std::endl;
    std::cout << "Pos2comp: " << Pos2comp << std::endl;
    std::cout << "SiEcomp: " << SiEcomp << std::endl;
    std::cout << "SiDEcomp: " << SiDEcomp << std::endl;
    std::cout << "SiTotalE: " << SiTotalE << std::endl;
    std::cout << "SiTotalEcomp: " << SiTotalEcomp << std::endl;

    std::cout << "Theta: " << Theta << std::endl;
    std::cout << "Thetacomp: " << Thetacomp << "\n" << std::endl;

    // ***********************************************************************************************
    // Increment Histograms for all events in the window

    // 1D Histograms
    /*
    hPos1 -> inc(Pos1);
    hPos2 -> inc(Pos2);
    hE -> inc(E);
    hDE -> inc(DE);
    hSiE -> inc(SiE);
    hSiDE -> inc(SiDE);
    hTheta -> inc(Theta);
    */

    // 2D Histograms
    /*
    hDEvsPos1 -> inc(Pos1comp, DEcomp);
    hEvsPos1 -> inc(Pos1comp, Ecomp);
    hDEvsE -> inc(Ecomp, DEcomp);
    hPos2vsPos1 -> inc(Pos1comp, Pos2comp);
    hThetavsPos1 -> inc(Pos1comp, Thetacomp);
    hSiDEvsSiE -> inc(SiEcomp, SiDEcomp);
    */

    // Gated 1D Histograms
    /*

    // E Gate
    Gate &G = hE -> getGate(0);
    //G.Print();
    if (G.inGate(E)){
      hE_gE_G1 -> inc(E);
    }

    // Pos1 G DE vs Pos1
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

    // Si Gates
    Gate &G3 = hSiDEvsSiE -> getGate(0);
    //G3.Print();
    if (G3.inGate(SiEcomp,SiDEcomp)){
      gateCounter++;
      //hSiE_G_SiDEvsSiE -> inc(SiE);
      hSiTotalE_G_SiDEvsSiE -> inc(SiTotalE);
      hSiDEvsSiTotalE_G_SiDEvsSiE -> inc(SiTotalEcomp,SiDEcomp);
    }

    // Pos1 G DE vs E Gates
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
    */
    // ***********************************************************************************************
  }
}