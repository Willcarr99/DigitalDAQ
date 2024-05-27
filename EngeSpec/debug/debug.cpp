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
#include <iomanip> // For std::setprecision

int main(){

  // ************************************************************************************************************

  // Testing foward and backward in time coincidence window with EXTRAS word enabled

  // ************************************************************************************************************
  // Sort Routine Settings
  
  // Number of channels in 1D and 2D histograms
  int Channels1D = 8192; // TODO - Test this scale (was 4096)
  int Channels2D = 1024; // TODO - Test this scale (was 512)

  // Threshold for EngeSpec Histograms
  int Histogram_Threshold = 10;

  // Define the channels (0-15)
  int iFrontHE = 0; // Pos1 (Front) High-Energy
  int iFrontLE = 1; // Pos1 (Front) Low-Energy
  int iBackHE = 2;  // Pos2 (Back) High-Energy
  int iBackLE = 3;  // Pos2 (Back) Low-Energy
  int iE = 4;       // Scintillator (E)
  int iDE = 5;      // Proportionality Counter (Delta E)
  int iSiE = 6;     // Silicon Detector (E)
  int iSiDE = 7;    // Silicon Detector (Delta E)

  // Channels to trigger on for the Focal-Plane detector (FP) and Silicon detectors (Si)
  int FP_trigger_ch = iE; // iE, iFrontHE, or iFrontLE
  int Si_trigger_ch = iSiE; // iSiE or iSiDE

  // Coincidence Windows for FP and Si detectors (in ns, number must be even)
  int window_FP_ns = 2000;
  int window_Si_ns = 2000;

  // Slope of Si spectrum
  const double pSiSlope = 0.3; // TODO - Need to read parameters.dat file with pSiSlope value

  // EXTRAS Recording Enabled (True) or Disabled (False)
  // bool extras = true;
  // 32-bit word: bits[31:16] = Extended Time Stamp, bits[15:10] = Flags, bits[9:0] = Fine Time Stamp

  // ************************************************************************************************************
  // Global sort routine variables

  // Converting window duration to timetag units (2 ns/unit - v1730)
  uint64_t window_FP = (uint64_t) std::floor(window_FP_ns/2.0);
  uint64_t window_Si = (uint64_t) std::floor(window_Si_ns/2.0);

  // Half coincidence window width
  uint64_t half_window_FP = (uint64_t) std::floor(window_FP/2.0);
  uint64_t half_window_Si = (uint64_t) std::floor(window_Si/2.0);

  // Maximum timetag value, at which point timetags reset - EXTRAS word extends rollover time
  // - Default timetag for v1730 (EXTRAS disabled) is a 31-bit number. So max is 2^31 - 1 = 2147483647 or 0x7FFFFFFF or INT_MAX
  //   Each timetag unit is 2 ns (v1730), so rollover time is 2 ns/unit * (2^31 - 1) units ~ 4.295 s
  // - Timetag for v1730 with EXTRAS enabled and 0x010 (2) written to bits[10:8] of DPP Algorithm Control 2 is a 31+16=47 bit number
  //   with an additional 10 bits reserved for the fine time stamp. So rollover is (2^47 - 1) units ~ 1.407x10^14, so 64-bit int needed
  //   but realistically rollover is not necessary to consider. Time step is 2 ns / 1024 = 0.001953125 ns with fine time stamp.
  // uint32_t timetag_rollover = (uint32_t) INT_MAX;
  // uint64_t timetag_rollover = 2^47 - 1;

  // Compression for 2D histograms
  double Compression = (double)Channels2D / (double)Channels1D; //8.0

  // Previous trigger window end timetag (initial value ensures 1st trigger is not ignored)
  uint64_t timetag_window_stop_previous_FP = 0;
  uint64_t timetag_window_stop_previous_Si = 0;

  std::vector<int16_t> energies;
  std::vector<uint64_t> course_timestamps; // With EXTRAS enabled and 0x010 (2) format, 47-bit max | T = EXTRAS[31:16] + 31-bit Trigger Timetag | Timetag units
  std::vector<double> fine_timestamps; // With EXTRAS enabled and 0x010 (2) format, 10-bit max floating point number | T = EXTRAS[9:0] / 1024 | Fraction of 1 timetag unit
  std::vector<int> digitizer_chs;
  std::vector<int> trigger_indices;

  // Set number of siginficant digits for cout to print to terminal
  std::cout << std::setprecision(15);
  // ************************************************************************************************************

  // Give example qlong and timetag data for debugging purposes
  // trigger_timetag - TTT - 31-bit number in the TTT+CH buffer
  // extended_timetag - 16-bit number in the EXTRAS buffer: EXTRAS[31:16]
  // fine_timetag - 10-bit number in the EXTRAS buffer: EXTRAS[9:0]
  // course_timestamp = (extended_timetag << 31) + trigger_timetag (extended_timetag is read as the most significant bits)
  // fine_timestamp = fine_timetag / 1024 (fraction less than 1, since fine_timetag max is 1023)
  const int num = 36; // Number of buffers for each readout (each board aggregate), max = 3 [memory locations/event] * 1023 [events/ch agg] * 8 [ch aggs/board agg] = 24552.
  // event 0:  qlong = 5000 (cDet = 1250), ch = iFLE (1), trigger_timetag = 3000, extended_timetag = 15, fine_timetag = 165  | course_timestamp = 32212257720 (0x780000BB8), fine_timestamp = 0.1611328125
  // event 1:  qlong = 5000 (cDet = 1250), ch = iFHE (0), trigger_timetag = 3100, extended_timetag = 15, fine_timetag = 348  | course_timestamp = 32212257820 (0x780000C1C), fine_timestamp = 0.33984375
  // event 2:  qlong = 5000 (cDet = 1250), ch = iE (4),   trigger_timetag = 3200, extended_timetag = 15, fine_timetag = 872  | course_timestamp = 32212257920 (0x780000C80), fine_timestamp = 0.8515625
  // event 3:  qlong = 5000 (cDet = 1250), ch = iDE (5),  trigger_timetag = 3300, extended_timetag = 15, fine_timetag = 91   | course_timestamp = 32212258020 (0x780000CE4), fine_timestamp = 0.0888671875
  // event 4:  qlong = 5000 (cDet = 1250), ch = iBHE (2), trigger_timetag = 3400, extended_timetag = 15, fine_timetag = 1014 | course_timestamp = 32212258120 (0x780000D48), fine_timestamp = 0.990234375
  // event 5:  qlong = 5000 (cDet = 1250), ch = iBLE (3), trigger_timetag = 3500, extended_timetag = 15, fine_timetag = 649  | course_timestamp = 32212258220 (0x780000DAC), fine_timestamp = 0.6337890625
  // event 6:  qlong = 5000 (cDet = 1250), ch = iE (4),   trigger_timetag = 3600, extended_timetag = 15, fine_timetag = 491  | course_timestamp = 32212258320 (0x780000E10), fine_timestamp = 0.4794921875
  // event 7:  qlong = 5000 (cDet = 1250), ch = iFHE (0), trigger_timetag = 3650, extended_timetag = 15, fine_timetag = 368  | course_timestamp = 32212258370 (0x780000E42), fine_timestamp = 0.359375
  // event 8:  qlong = 5000 (cDet = 1250), ch = iE (4),   trigger_timetag = 4000, extended_timetag = 15, fine_timetag = 5    | course_timestamp = 32212258720 (0x780000FA0), fine_timestamp = 0.0048828125
  // event 9:  qlong = 5000 (cDet = 1250), ch = iE (4),   trigger_timetag = 4600, extended_timetag = 15, fine_timetag = 1020 | course_timestamp = 32212259320 (0x7800011F8), fine_timestamp = 0.99609375
  // event 10: qlong = 5000 (cDet = 1250), ch = iFHE (0), trigger_timetag = 4601, extended_timetag = 15, fine_timetag = 657  | course_timestamp = 32212259321 (0x7800011F9), fine_timestamp = 0.6416015625
  // event 11: qlong = 5000 (cDet = 1250), ch = iFLE (1), trigger_timetag = 5099, extended_timetag = 15, fine_timetag = 75   | course_timestamp = 32212259819 (0x7800013EB), fine_timestamp = 0.0732421875
  uint32_t data[num] = {0x11388, 0xBB8, 0xF00A5, // 1 = qlong | ch, where ch is bits[19:16] and qlong is bits[15:0]
                        0x1388, 0xC1C, 0xF015C,  // 2 = trigger_timetag, 31-bit trigger time tag (TTT) 
                        0x41388, 0xC80, 0xF0368, // 3 = EXTRAS, where EXTRAS[31:16] = extended_timetag, EXTRAS[9:0] = fine timetag, EXTRAS[10:15] = flags (all 0 in this example)
                        0x51388, 0xCE4, 0xF005B,
                        0x21388, 0xD48, 0xF03F6,
                        0x31388, 0xDAC, 0xF0289,
                        0x41388, 0xE10, 0xF01EB,
                        0x1388, 0xE42, 0xF0170,
                        0x41388, 0xFA0, 0xF0005,
                        0x41388, 0x11F8, 0xF03FC,
                        0x1388, 0x11F9, 0xF0291,
                        0x11388, 0x13EB, 0xF004B}; // 0x11388, 0x13EB, 0xF004B

  // Extract energy, channel, and timetag from each event, and extract index if ch is a trigger
  for(int i=0; i<num; i+=3){
    int16_t qlong = (int16_t) (data[i] & 0xFFFF);
    std::cout << "\nIndex " << (int) i/3.0 << std::endl;
    std::cout << "qlong = " << qlong << std::endl;
    energies.push_back((int16_t) std::floor(qlong/4.0)); // Maxes out at Channels1D (8,192).
    int digitizer_ch = (int) (data[i] & 0xFFFF0000) >> 16;
    std::cout << "Ch = " << digitizer_ch << std::endl;
    digitizer_chs.push_back(digitizer_ch);
    uint32_t timetag = data[i+1]; // 31-bit trigger time tag (TTT)
    std::cout << "Trigger Time Tag (TTT) = " << timetag << std::endl;
    uint64_t extended_timetag = (uint64_t) ((data[i+2] & 0xFFFF0000) >> 16); // 16-bit timetag rollover extension
    std::cout << "Extended Time Stamp = " << extended_timetag << std::endl;
    uint64_t course_timestamp = (uint64_t) ((extended_timetag & 0xFFFF) << 31) + (uint64_t) timetag;
    std::cout << "Course Time Stamp = " << course_timestamp << std::endl;
    course_timestamps.push_back(course_timestamp);
    uint32_t fine_timetag = (uint32_t) (data[i+2] & 0x3FF); // 10-bit fine time tag (from CFD ZC interpolation)
    std::cout << "Fine Timetag (10-bit) = " << fine_timetag << std::endl;
    double fine_timestamp = (double) fine_timetag / 1024.0; // Fraction with 2^10 = 1024 steps
    std::cout << "Fine Time Stamp (10-bit / 1024) = " << fine_timestamp << std::endl;
    fine_timestamps.push_back(fine_timestamp);

    if (digitizer_ch == FP_trigger_ch || digitizer_ch == Si_trigger_ch){
      trigger_indices.push_back((int) i/3.0);
      std::cout << "Index is trigger" << std::endl;
    }
  }

  // Collect coincidence events for each FP or Si trigger
  for (int trigger_index : trigger_indices){
    std::cout << "\nTrigger Index: " << trigger_index << "\n" << std::endl;

    int trigger_ch = digitizer_chs[trigger_index];
    uint64_t half_window;
    // Get window size based on the kind of trigger
    if (trigger_ch == FP_trigger_ch){
      half_window = half_window_FP;
    }
    else if (trigger_ch == Si_trigger_ch){
      half_window = half_window_Si;
    }

    // Trigger timestamp and its coincidence window start/stop timestamps.
    uint64_t course_timestamp_trigger = course_timestamps[trigger_index];
    // double fine_timestamp_trigger = fine_timestamps[trigger_index]; // Windows not accounting for fine timestamp
    uint64_t timetag_window_start;
    uint64_t timetag_window_stop;
    
    // Find window start and stop timetags. Not considering fine timestamp here.
    if (course_timestamp_trigger >= half_window){
      timetag_window_start = course_timestamp_trigger - half_window;
    }
    // ... if window extends below 0
    else if (course_timestamp_trigger < half_window){
      timetag_window_start = 0;
    }

    timetag_window_stop = course_timestamp_trigger + half_window;

    std::cout << "Trigger Window Start = " << timetag_window_start << std::endl;
    std::cout << "Trigger Window Stop = " << timetag_window_stop << std::endl;

    // Inhibit trigger if current window overlaps with the previous window.
    if (trigger_ch == FP_trigger_ch){ // FP window
      if (timetag_window_start < timetag_window_stop_previous_FP){
        std::cout << "FP Trigger Inhibited, Index = " << trigger_index << std::endl;
        continue;
      }
    }
    else if (trigger_ch == Si_trigger_ch){ // Si window
      if (timetag_window_start < timetag_window_stop_previous_Si){
        std::cout << "Si Trigger Inhibited, Index = " << trigger_index << std::endl;
        continue;
      }
    }
    // Note: If timetag_window_start == timetag_window_stop_previous and an event happens to occur at that timetag, it is collected by the later event
    // Note: If a trigger is inhibited, the next trigger window is compared with the last uninhibited trigger window

    // Update trigger info for next trigger to check (must come after overlapping triggers are inhibited)
    if (trigger_ch == FP_trigger_ch){
      timetag_window_stop_previous_FP = timetag_window_stop;
    }
    else if (trigger_ch == Si_trigger_ch){
      timetag_window_stop_previous_Si = timetag_window_stop;
    }

    // ***********************************************************************************************
    // Event loop variables for each coincidence window

    bool in_window = true;
    bool increment_backwards = true;
    int event_increment = 0;

    // Data to save for coincidences. Stays 0 if no corresponding event in window
    uint64_t FrontHE_course = 0, FrontLE_course = 0, BackHE_course = 0, BackLE_course = 0;
    double FrontHE_fine = 0, FrontLE_fine = 0, BackHE_fine = 0, BackLE_fine = 0;
    int E = 0, DE = 0, SiE = 0, SiDE = 0;

    // Flags to prevent more than one signal to be collected from a single component during the coincidence window.
    // True when signal is detected during window.
    bool fFrontHE = false, fFrontLE = false, fBackHE = false, fBackLE = false,
         fE = false, fDE = false, fSiE = false, fSiDE = false;
    
    // ***********************************************************************************************

    // Collect each event in the trigger coincidence window
    while(in_window){

      int event_index;
      uint64_t course_timestamp;
      double fine_timestamp;
      int event_ch;
      int energy;

      // Look for events before trigger first (between trigger and start of window)
      if (increment_backwards){
        event_index = trigger_index - event_increment;
      }
      // Then look for events after trigger (between trigger and end of window)
      else{
        event_index = trigger_index + event_increment;
      }
      event_increment++;

      // Ensure we have not indexed below the first event or above the last event in the board aggregate
      if (event_index >= 0 && event_index <= ((int)(num/3.0) - 1)){
        course_timestamp = course_timestamps[event_index];
        fine_timestamp = fine_timestamps[event_index];
        event_ch = digitizer_chs[event_index];
        energy = (int) energies[event_index];

        // Skip if Si event in FP window
        if ((event_ch == iSiE || event_ch == iSiDE) && trigger_ch == FP_trigger_ch){
          std::cout << "Si Event in FP window: Ch = " << event_ch << ", Index = " << event_index << ", Course Timestamp = " << course_timestamp << std::endl;
          continue;
        }
        // Skip if FP event in Si window
        else if ((event_ch == iE || event_ch == iDE || event_ch == iFrontHE || event_ch == iFrontLE || 
                  event_ch == iBackHE || event_ch == iBackLE) && trigger_ch == Si_trigger_ch){
          std::cout << "FP Event in Si window: Ch = " << event_ch << ", Index = " << event_index << ", Course Timestamp = " << course_timestamp << std::endl;
          continue;
        }

        // Check if the next event is in the window
        if (course_timestamp >= timetag_window_start && course_timestamp < timetag_window_stop){

          // Bring energy to 0 if signal is below our Histogram threshold or above Channels1D (Pos1 threshold handled separately below)
          if ((event_ch == iE || event_ch == iDE || event_ch == iSiE || event_ch == iSiDE) && 
              (energy < Histogram_Threshold || energy >= Channels1D)){
            energy = 0;
          }
                    
          // ***********************************************************************************************
          // Event is within window. Save each unique event to its correspnding variable(s)

          std::cout << "\nCourse Timestamp of event #" << event_index << " = " << course_timestamp << std::endl;
          std::cout << "Fine Timestamp of event #" << event_index << " = " << fine_timestamp << std::endl;
          std::cout << "Ch of event #" << event_index << " = " << event_ch << std::endl;
          std::cout << "Energy of event #" << event_index << " = " << energy << std::endl;

          if (event_ch == iE && !fE){fE = true; E = energy;}
          else if (event_ch == iDE && !fDE){fDE = true; DE = energy;}
          else if (event_ch == iSiE && !fSiE){fSiE = true; SiE = energy;}
          else if (event_ch == iSiDE && !fSiDE){fSiDE = true; SiDE = energy;}
          else if (event_ch == iFrontHE && !fFrontHE){fFrontHE = true; FrontHE_course = course_timestamp; FrontHE_fine = fine_timestamp;}
          else if (event_ch == iFrontLE && !fFrontLE){fFrontLE = true; FrontLE_course = course_timestamp; FrontLE_fine = fine_timestamp;}
          else if (event_ch == iBackHE && !fBackHE){fBackHE = true; BackHE_course = course_timestamp; BackHE_fine = fine_timestamp;}
          else if (event_ch == iBackLE && !fBackLE){fBackLE = true; BackLE_course = course_timestamp; BackLE_fine = fine_timestamp;}          
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

    int Ecomp = (int) std::floor(E * Compression);
    int DEcomp = (int) std::floor(DE * Compression);
    int SiEcomp = (int) std::floor(SiE * Compression);
    int SiDEcomp = (int) std::floor(SiDE * Compression);

    int SiTotalE = 0;

    if (fSiE && fSiDE){
      SiTotalE = (int) std::floor((SiE + pSiSlope*SiDE) / (1.0 + pSiSlope));
    }
    int SiTotalEcomp = (int) std::floor(SiTotalE * Compression);

    int Pos1 = 0;
    int Pos2 = 0;
    int Pos1comp = 0;
    int Pos2comp = 0;
    int Theta = 0;

    if (fFrontHE && fFrontLE){
      // Pos = HE - LE, scaled so Max Ch = 1 us and Min Ch = -1 us, offset to center (4,096)
      int Pos1_course = (int) (FrontHE_course - FrontLE_course); // from -1 us to +1 us or -500 to +500 timetag units
      double Pos1_fine = FrontHE_fine - FrontLE_fine; // from -1023/1024 to +1023/1024 timetag units
      double Pos1_interpolated = ((double) Pos1_course) + Pos1_fine;
      Pos1 = (int) std::floor((Channels1D / 1000.0) * Pos1_interpolated) + (Channels1D/2); // scaled to [0, Channels1D] with each bin unique
      if (Pos1 < Histogram_Threshold || Pos1 >= Channels1D){Pos1=0;}
    }
    Pos1comp = (int) std::floor(Pos1 * Compression);

    if (fBackHE && fBackLE){
      int Pos2_course = (int) (BackHE_course - BackLE_course);
      double Pos2_fine = BackHE_fine - BackLE_fine;
      double Pos2_interpolated = ((double) Pos2_course) + Pos2_fine;
      Pos2 = (int) std::floor((Channels1D / 1000.0) * Pos2_interpolated) + (Channels1D/2);
      if (Pos2 < Histogram_Threshold || Pos2 >= Channels1D){Pos2=0;}
    }
    Pos2comp = (int) std::floor(Pos2 * Compression);

    if (fFrontHE && fFrontLE && fBackHE && fBackLE){
      Theta = (int) std::round(10000.0*atan((Pos2-Pos1)/100.)/3.1415 - 4000.);
      Theta = std::max(0,Theta);
    }
    int Thetacomp = (int) std::floor(Theta * Compression);

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