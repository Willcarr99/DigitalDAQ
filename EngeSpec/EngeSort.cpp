#include <iostream>
#include <vector>
#include <random>
#include <chrono>

#include "EngeSort_v1730.h"
#include "TV792Data.hxx"

Messages messages;

std::string EngeSort::sayhello( ) {
  return messages.sayhello("EngeSort v1730");
}
std::string EngeSort::saygoodbye( ) {
  return messages.saygoodbye();
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

// FP Trigger (By channel number -- for Pos1 use iFrontHE or iFrontLE. Similarly for Pos2)
int FPTrigger = iE; // Scintillator (E)
// Note: Si triggers on EITHER SiE or SiDE

// Coincidence Windows for Focal Plane and Si Detector (in ns)
int win_ns = 5000;
int winSi_ns = 5000;

// Automatically close the windows when all appropriate coincidences have been recorded
// Although this reduces the deadtime significantly, false coincidences occur significantly more often
//bool autoClose = false;
//bool autoCloseSi = false;

// Max value of the timetag, at which point it rolls back to 0
// Default timetag for v1730 (EXTRAS disabled) is a 31-bit number. So max is 2^31 - 1 = 2147483648 or 0x7FFFFFFF
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

Histogram *hSiDEvsSiE;

// Gated Spectra
Histogram *hPos1_gDEvPos1_G1;
Histogram *hPos1_gDEvPos1_G2;

Histogram *hE_gE_G1;

// Counters
int totalCounter=0;
int gateCounter=0;

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

  hSiDEvsSiE = new Histogram("Si DE vs Si E", Channels2D, 2);

  //--------------------
  // Gated Histograms
  hPos1_gDEvPos1_G1 = new Histogram("Pos 1; GDEvPos1-G1", Channels1D, 1);
  hPos1_gDEvPos1_G2 = new Histogram("Pos 1; GDEvPos1-G2", Channels1D, 1);

  hE_gE_G1 = new Histogram("E; GE-G1", Channels1D, 1);

  //--------------------
  // Gates
  //g2d_DEvsPos1_1 = new Gate("Gate 1");
  //g2d_DEvsPos1_2 = new Gate("Gate 2");
  //g2d_DEvsPos1_3 = new Gate("Gate 3");

  hE -> addGate("Energy Gate");
  
  hDEvsPos1 -> addGate("Protons");
  hDEvsPos1 -> addGate("Deuterons");
  //hDEvsPos1 -> addGate(g2d_DEvsPos1_2);
  //hDEvsPos1 -> addGate(g2d_DEvsPos1_3);


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

}

//======================================================================
// This is the equivalent to the "sort" function in jam
void EngeSort::sort(uint32_t *dADC, int nADC, uint32_t *dTDC, int nTDC){

  totalCounter++;

  // double ADCsize = sizeof(dADC)/sizeof(dADC[0]);
  // double TDCsize = sizeof(dTDC)/sizeof(dTDC[0]);
  // std::cout << ADCsize << "  " << TDCsize << std::endl;

  int ch, time; // 32-bit signed integer. Timetag is 31 (unsigned) bits of a 32-bit unsigned integer, so int cast is ok. The MSB is always 0.
  int16_t qlong, cDet; // 16-bit signed integers. Qlong is a 16-bit signed integer in a 32-bit unsigned buffer (pg 100-101 of manual). So qlong range is -32768 to 32767. cDet range is -8192 to 8191. Negative cDet values are coverted to 0 by the threshold check.

  // Window start time - gets updated each trigger. The initial value here prevents having to do an extra if statement for the first trigger in the buffer.
  int winStart = -win - 1;
  int winStartSi = -winSi - 1;

  // Data for each event is stored in 2 32-bit memory locations. 
  // Energies (and Ch #) are EVEN nADC counts, timetags are ODD counts (see ReadQLong in v1730DPP.c)
  for(int i = 0; i<nADC; i+=2){

    // Extract energy, channel, and timetag from event
    qlong = (int16_t) (dADC[i] & 0xFFFF); // dADC[i] includes channel # and qlong (energy). qlong max = 65535
    ch = (int) (dADC[i] & 0xFFFF0000) >> 16;
    cDet = (int16_t) std::floor(qlong/4.0); // TODO - This maxes out at Channels1D (8,192).
    time = (int) dADC[i+1];

    // Handle noise for E, DE, SiE, and SiDE (Pos1 and Pos2 handled separately in FP coincidence window below)
    if (cDet < Thresh || cDet > Channels1D - 1){dADC[i]=0;} // TODO - To move 8192 spike to 0, use >=

    //------------------- Si Window -------------------
    // Triggering on either SiE or SiDE
    if (ch == iSiE || ch == iSiDE){
      // Check if Si coincidence window is already open (accounting for possible rollback to 0)
      if ((time > winStartSi && time < winStartSi + winSi) || (winStartSi > timetagReset - winSi && time > winStartSi + winSi - timetagReset)){
        // Check if the signal is from SiE and SiDE was the trigger, ignoring multiple occurrences of SiE
        if (ch == iSiE && !fSiE && fSiDE){
          // Coincidence! Increment histograms
          fSiE = true;
          SiE = cDet;
          hSiE -> inc(SiE);
          int SiEComp = (int) std::floor(SiE/8.0); // TODO - This maxes out at 1,024
          int SiDEComp = (int) std::floor(SiDE/8.0);
          hSiDEvsSiE -> inc(SiEComp,SiDEComp);
        }
        // Check if the signal is from SiDE and SiE was the trigger, ignoring multiple occurrences of SiDE
        else if (ch == iSiDE && !fSiDE && fSiE){
          // Coincidence! Increment histograms
          fSiDE = true;
          SiDE = cDet;
          hSiDE -> inc(SiDE);
          int SiEComp = (int) std::floor(SiE/8.0); // TODO - This maxes out at 1,024
          int SiDEComp = (int) std::floor(SiDE/8.0);
          hSiDEvsSiE -> inc(SiEComp,SiDEComp);
        }
      }
      else{
        // Previous window ended. Now open new window from trigger.
        winStartSi = time;
        if (ch == iSiE){
          SiE = cDet;
          hSiE -> inc(SiE); // TODO - Do we want this to increment even if no coincidence?
          fSiE = true;
          fSiDE = false;
        }
        else if (ch == iSiDE){
          SiDE = cDet;
          hSiDE -> inc(SiDE); // TODO - Do we want this to increment even if no coincidence?
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
        if (FPTrigger == iE){ // E Trigger
          if (ch == iFrontHE && !fFrontHE){ // Pos1 HE signal
            
          }
          else if (ch == iFrontLE && !fFrontLE){ // Pos1 LE signal

          }
          else if (ch == iBackHE && !fBackHE){ // Pos2 HE signal

          }
          else if (ch == iBackLE && !fBackLE){ // Pos2 LE signal

          }
          else if (ch == iDE && !fDE){ // DE signal

          }
        }
        else if (FPTrigger == iFrontHE || FPTrigger == iFrontLE){ // Front Trigger
          if (ch == iE && !fE){ // E signal

          }
          else if ((ch == iFrontHE && !fFrontHE) || (ch == iFrontLE && !fFrontLE)){ // Coincidence: Pos1 HE and LE
            
          }
          else if (ch == iBackHE && !fBackHE){ // Pos2 HE signal
            // Check if Pos2 coincidence
          }
          else if (ch == iBackLE && !fBackLE){ // Pos2 LE signal
            // Check if Pos2 coincidence
          }
          else if (ch == iDE && !fDE){ // DE signal

          }
        }
        else if (FPTrigger == iBackHE || FPTrigger == iBackLE){ // Back Trigger
          if (ch == iE && !fE){ // E signal

          }
          else if (ch == iFrontHE && !fFrontHE){ // Pos1 HE signal
            // Check if Pos1 coincidence
          }
          else if (ch == iFrontLE && !fFrontLE){ // Pos1 LE signal
            // Check if Pos1 coincidence
          }
          else if ((ch == iBackHE && !fBackHE) || (ch == iBackLE && !fBackLE)){ // Coincidence: Pos2 HE and LE

          }
          else if (ch == iDE && !fDE){ // DE signal

          }
        }
        else if (FPTrigger == iDE){ // DE Trigger
          if (ch == iE && !fE){ // E signal

          }
          else if (ch == iFrontHE && !fFrontHE){ // Pos1 HE signal
            
          }
          else if (ch == iFrontLE && !fFrontLE){ // Pos1 LE signal

          }
          else if (ch == iBackHE && !fBackHE){ // Pos2 HE signal

          }
          else if (ch == iBackLE && !fBackLE){ // Pos2 LE signal

          }
        }
      }
      else if (ch == FPTrigger){
        // Previous window ended. Now open new window.
        winStart = time;
        if (FPTrigger == iE){ // E Trigger
          E = cDet;
          hE -> inc(E);
          fE = true;
          fDE = false;
          fFrontHE = false;
          fFrontLE = false;
          fBackHE = false;
          fBackLE = false;
          fTheta = false;

          // E Gate
          Gate &G = hE -> getGate(0);
          if (G.inGate(E)){
            hE_gE_G1 -> inc(E);
          }
        }
        // If the trigger is Pos1, open the window, but we can't increment Pos1 histogram until we get a HE and LE coincidence (dealt with above)
        else if (FPTrigger == iFrontHE || FPTrigger == iFrontLE){ // Pos1 trigger
          if (ch == iFrontHE){
            fFrontHE = true;
            fFrontLE = false;
          }
          else if (ch == iFrontLE){
            fFrontLE = true;
            fFrontHE = false;
          }
          fE = false;
          fDE = false;
          fBackHE = false;
          fBackLE = false;
          fTheta = false;
        }
        // If the trigger is Pos2, open the window, but we can't increment Pos2 histogram until we get a HE and LE coincidence (dealt with above)
        else if (FPTrigger == iBackHE || FPTrigger == iBackLE){ // Pos2 trigger
          if (ch == iBackHE){
            fBackHE = true;
            fBackLE = false;
          }
          else if (ch == iBackLE){
            fBackLE = true;
            fBackHE = false;
          }
          fE = false;
          fDE = false;
          fFrontHE = false;
          fFrontLE = false;
          fTheta = false;
        }
        else if (FPTrigger == iDE){ // DE Trigger
          DE = cDet;
          hDE -> inc(DE);
          fDE = true;
          fE = false;
          fFrontHE = false;
          fFrontLE = false;
          fBackHE = false;
          fBackLE = false;
          fTheta = false;
        }
      }
    }
    else{
      std::cout << "Error: Unknown Channel" << std::endl;
    }
  }
}




/*

    if (winOpen == false){
      if (cDet > 20 && cDet < Channels1D){
        if (ch == 4){ // E
          winOpen = true;
          winStart = (int) timetag;
          E = cDet;
          hE -> inc(E);
          // E Gate
          Gate &G = hE -> getGate(0);
          if (G.inGate(E)){
            hE_gE_G1 -> inc(E);
          }
        }
      }
    }
    else{
      // Check if timetag is outside of scintillator trigger window (taking into account timetag rollback to zero)
      int int_timetag = (int) timetag;
      if ((int_timetag > winStart + win) || (winStart > 4294967296 - win && int_timetag > winStart + win - 4294967296)){
        winOpen = false;
        fDE = false;
        fFrontHE = false;
        fFrontLE = false;
        fBackHE = false;
        fBackLE = false;
        fTheta = false;
      }
      else{
        if (cDet > 20 && cDet < Channels1D){
          if (ch == 5 && fDE == false){ // DE
            fDE = true;
            DE = cDet;
            hDE -> inc(DE);

            // 2D DE vs E
            int scaled_E = (int) std::floor(E/4.0);
            int scaled_DE = (int) std::floor(DE/4.0);
            hDEvsE -> inc(scaled_E, scaled_DE);

            // 2D DE vs Pos1
            if (fFrontHE == true && fFrontLE == true){
              // Cut off the outer-edges of the time range with center at 256. Adjust scaling by timeScale setting
              int scaled_Pos1 = ((Pos1 - (Channels1D / 2)) / timeScale) + (Channels2D / 2);
              hDEvsPos1 -> inc(scaled_Pos1, scaled_DE);

              // DE vs Pos1 Gates
              Gate &G1 = hDEvsPos1 -> getGate(0);
              //G1.Print();
              if(G1.inGate(scaled_Pos1,scaled_DE)){
                gateCounter++;
                hPos1_gDEvPos1_G1 -> inc(Pos1);
              }

              Gate &G2 = hDEvsPos1 -> getGate(1);
              //G2.Print();
              if(G2.inGate(scaled_Pos1,scaled_DE)){
                gateCounter++;
                hPos1_gDEvPos1_G2 -> inc(Pos1);
              }
            }
          }
        }
        if (ch == 0 && fFrontHE == false){
          fFrontHE = true;
          FrontHE = timetag;
          // Coincidence Pos1
          if (fFrontLE == true){
            int int_FrontHE = (int) FrontHE;
            int int_FrontLE = (int) FrontLE; 
            int diff_Pos1 = int_FrontHE - int_FrontLE; // HE - LE is the referrence
            Pos1 = diff_Pos1 + (Channels1D / 2); // offset to center
            hPos1 -> inc(Pos1);

            // 2D E vs Pos1
            int scaled_E = (int) std::floor(E/4.0);
            int scaled_Pos1 = ((Pos1 - (Channels1D / 2)) / timeScale) + (Channels2D / 2);
            hEvsPos1 -> inc(scaled_Pos1, scaled_E);

            // 2D DE vs Pos1
            if (fDE == true){
              int scaled_DE = (int) std::floor(DE/4.0);
              hDEvsPos1 -> inc(scaled_Pos1, scaled_DE);

              // DE vs Pos1 Gates
              Gate &G1 = hDEvsPos1 -> getGate(0);
              //G1.Print();
              if(G1.inGate(scaled_Pos1,scaled_DE)){
                gateCounter++;
                hPos1_gDEvPos1_G1 -> inc(Pos1);
              }

              Gate &G2 = hDEvsPos1 -> getGate(1);
              //G2.Print();
              if(G2.inGate(scaled_Pos1,scaled_DE)){
                gateCounter++;
                hPos1_gDEvPos1_G2 -> inc(Pos1);
              }
            }
          }
        }
        else if (ch == 1 && fFrontLE == false){
          fFrontLE = true;
          FrontLE = timetag;
          // Coincidence Pos1
          if (fFrontHE == true){
            int int_FrontHE = (int) FrontHE;
            int int_FrontLE = (int) FrontLE;
            int diff_Pos1 = int_FrontHE - int_FrontLE; // HE - LE is the referrence
            Pos1 = diff_Pos1 + (Channels1D / 2); // offset to center
            hPos1 -> inc(Pos1);

            // 2D E vs Pos1
            int scaled_E = (int) std::floor(E/4.0);
            int scaled_Pos1 = ((Pos1 - (Channels1D / 2)) / timeScale) + (Channels2D / 2);
            hEvsPos1 -> inc(scaled_Pos1, scaled_E);

            // 2D DE vs Pos1
            if (fDE == true){
              int scaled_DE = (int) std::floor(DE/4.0);
              hDEvsPos1 -> inc(scaled_Pos1, scaled_DE);

              // DE vs Pos1 Gates
              Gate &G1 = hDEvsPos1 -> getGate(0);
              //G1.Print();
              if(G1.inGate(scaled_Pos1,scaled_DE)){
                gateCounter++;
                hPos1_gDEvPos1_G1 -> inc(Pos1);
              }

              Gate &G2 = hDEvsPos1 -> getGate(1);
              //G2.Print();
              if(G2.inGate(scaled_Pos1,scaled_DE)){
                gateCounter++;
                hPos1_gDEvPos1_G2 -> inc(Pos1);
              }
            }
          }
        }
        else if (ch == 2 && fBackHE == false){
          fBackHE = true;
          BackHE = timetag;
          // Coincidence Pos2
          if (fBackLE == true){
            int int_BackHE = (int) BackHE;
            int int_BackLE = (int) BackLE;
            int diff_Pos2 = int_BackHE - int_BackLE; // HE - LE is the referrence
            Pos2 = diff_Pos2 + (Channels1D / 2); // offset to center
            hPos2 -> inc(Pos2);
          }
        }
        else if (ch == 3 && fBackLE == false){
          fBackLE = true;
          BackLE = timetag;
          // Coincidence Pos2
          if (fBackHE == true){
            int int_BackHE = (int) BackHE;
            int int_BackLE = (int) BackLE;
            int diff_Pos2 = int_BackHE - int_BackLE; // HE - LE is the referrence
            Pos2 = diff_Pos2 + (Channels1D / 2); // offset to center
            hPos2 -> inc(Pos2);
          }
        }
        // Theta and Pos2 vs Pos1
        if (fFrontHE == true && fFrontLE == true && fBackHE == true && fBackLE == true && fTheta == false){
          fTheta == true;
          Theta = (int) std::round(10000.0*atan((Pos2 - Pos1)/100.)/3.1415 - 4000.); // TODO Is this still correct?
          Theta = std::max(0,Theta);
          hTheta -> inc(Theta);
          // Theta vs Pos1
          int scaled_Pos1 = ((Pos1 - (Channels1D / 2)) / timeScale) + (Channels2D / 2);
          hThetavsPos1 -> inc(scaled_Pos1, Theta); // Should theta be scaled? It is in EngeSort. But should this be treated like a coin. time or energy or something else?
          // Pos2 vs Pos1
          int scaled_Pos2 = ((Pos2 - (Channels1D / 2)) / timeScale) + (Channels2D / 2);
          hPos2vsPos1 -> inc(scaled_Pos1, scaled_Pos2);
        }
        // Automatically close window if all coincidences have been recorded
        if (fFrontHE == true && fFrontLE == true && fBackHE == true && fBackLE == true && fTheta == true && fDE == true){
          if (autoClose == true){
            winOpen = false;
            fDE = false;
            fFrontHE = false;
            fFrontLE = false;
            fBackHE = false;
            fBackLE = false;
            fTheta = false;
          }
        }
      }
    }
*/
//  }
//}
//======================================================================

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
  while (p2 && ac < kMaxArgs-1)
    {
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
  for(auto h: Histograms)
    {
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
      for (int i = 0; i < h -> getnChannels(); i++)
	{
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
      //if(h -> getNGates() > 0){
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
  printf("Counted %d events\n",fTotalEventCounter);
  std::cout << "number of spectra: " << eA->getSpectrumNames().size() << std::endl;
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

    //    printf("V1730 Bank: Name = %s, Type = %d, Size = %d\n",&bADC->name[0],
    //	   bADC->type,bADC->data_size); 

    uint64_t dat;
    dat = dADC[0] & 0xFFFF;
    //  printf("dADC[0] = 0x%x\n",dat);
    //  printf("dADC[0] = %d\n",dat);

    int singleADCSize = 0;
    int singleTDCSize = 0;
    if(bADC->type == 4)singleADCSize = 2;
    if(bADC->type == 6)singleADCSize = 4;
    
    // Find the size of the data
    int nADC = 0;
    int nTDC = 0;
    if(bADC)nADC=(bADC->data_size - 2)/singleADCSize;
    if(bTDC)nTDC=(bTDC->data_size - 2)/singleTDCSize;

    //std::cout << "nADC = " << nADC << " nTDC = " << nTDC << std::endl;
    
  
    fRunEventCounter++;
    fModule->fTotalEventCounter++;
    fModule->eA->sort(dADC, nADC, dTDC, nTDC);

  } else if(event->event_id == 2){

    std::cout << "This is a scaler event. It should never happen!" << std::endl;

  }
    
  return flow;

}

/* 
   manalyzer run
*/

void MidasAnalyzerRun::BeginRun(TARunInfo* runinfo){
  printf("Begin run %d\n",runinfo->fRunNo);
  time_t run_start_time = 0; // runinfo->fOdb->odbReadUint32("/Runinfo/Start time binary", 0, 0);
  printf("ODB Run start time: %d: %s", (int)run_start_time, ctime(&run_start_time));

  fRunEventCounter = 0;
}
void MidasAnalyzerRun::EndRun(TARunInfo* runinfo){
  printf("End run %d\n",runinfo->fRunNo);
  printf("Counted %d events\n",fRunEventCounter);
  std::cout << "Total counts: " << totalCounter << "   Gated counts: " << gateCounter << std::endl;
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
    .def("ClearData", &EngeSort::ClearData)        // void
    .def("getGateNames", &EngeSort::getGateNames)             // string vector of gate names
    .def("putGate", &EngeSort::putGate)            // void
    .def("data", range(&EngeSort::begin, &EngeSort::end))
    //    .def("Histogram1D", &Histogram1D::Histogram1D)
    ;

}
