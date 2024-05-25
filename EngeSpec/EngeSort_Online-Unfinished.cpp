#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>

#include "EngeSort.h"
#include "TV792Data.hxx"

Messages messages;

std::string EngeSort::sayhello( ) {
  return messages.sayhello("EngeSort v7130");
}
std::string EngeSort::saygoodbye( ) {
  return messages.saygoodbye();
}
std::string EngeSort::saysomething(std::string str) {
  
  return messages.saysomething(str);
}

//int Channels1D = 4096;
//int Channels2D = 512;
int Channels1D = 8192; // TODO - Test this scale (was 4096)
int Channels2D = 1024; // TODO - Test this scale (was 512)

int Threshold = 20;

// EXTRAS Recording Enabled (True) or Disabled (False)
bool extras = true;
// 32-bit word: bits[31:16] = Extended Time Stamp, bits[15:10] = Flags, bits[9:0] = Fine Time Stamp

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

Histogram *hTDC_E;
Histogram *hTDC_DE;
Histogram *hTDC_PosSum;

// 2D Spectra
Histogram *hDEvsPos1;
Histogram *hEvsPos1;
Histogram *hDEvsE;
Histogram *hPos2vsPos1;
Histogram *hThetavsPos1;

Histogram *hPos1vsEvt;

Histogram *hSiDEvsSiE;

// Gated Spectra
Histogram *hPos1_gDEvPos1_G1;
Histogram *hPos1_gDEvPos1_G2;
Histogram *hPos1_gDEvsE_G1;
Histogram *hPos1_gDEvsE_G2;

Histogram *hPos1_gPos1vsEvt;
Histogram *hPos1_gDEvPos1_gPos1vsEvt; // *************** NEW 4/17/23 - Will ***************

Histogram *hDEvsPos1_G_Pos1vsEvt;

Histogram *hE_gE_G1;

Histogram *hSiDEvsSiTotalE_G_SiDEvsSiE;  // Silicon DE vs total E
Histogram *hSiTotalE_G_SiDEvsSiE;          // Total silicon energy

Histogram *hSiTotalE_G_Pos1vsEvt; // *************** NEW 4/17/23 - Will ***************
Histogram *hSiTotalE_G_SiDEvsSiE_G_Pos1vsEvt; // *************** NEW 4/17/23 - Will ***************

Histogram *hSiTotalEvsEvt_G_SiDEvsSiE; // *************** NEW 5/4/23 - Will ***************

// 2D Gates on the DE vs Pos1 spectrum
//Gate *g2d_DEvsPos1_1;
//Gate *g2d_DEvsPos1_2;
//Gate *g2d_DEvsPos1_3;

// Counters
int totalCounter=0;
int gateCounter=0;
int EventNo=0;

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

double pSiSlope = 0.0;

void EngeSort::Initialize(){
  
  //--------------------
  // 1D Histograms
  hPos1 = new Histogram("Position 1", Channels1D, 1);
  //  hPos1 -> Print(0, 10);
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
  
  hSiDEvsSiE = new Histogram("Si DE vs Si E", Channels2D, 2);
  
  //--------------------
  // Gated Histograms
  hPos1_gDEvPos1_G1 = new Histogram("Pos 1; GDEvPos1-G1", Channels1D, 1);
  hPos1_gDEvPos1_G2 = new Histogram("Pos 1; GDEvPos1-G2", Channels1D, 1);

  hPos1_gDEvsE_G1 = new Histogram("Pos 1; G-DEvsE-G1", Channels1D, 1);
  hPos1_gDEvsE_G2 = new Histogram("Pos 1; G-DEvsE-G2", Channels1D, 1);

  hPos1_gPos1vsEvt = new Histogram("Pos 1; GPos1vsEvent", Channels1D, 1); // Pos1vsEvent gates incremented in EngeSort::FillEndofRun(), not EngeSort::sort()
  hPos1_gDEvPos1_gPos1vsEvt = new Histogram("Pos 1; GDEvPos1-G2 + GPos1vsEvent", Channels1D, 1); // NEW 4/17/23 - Will
  hDEvsPos1_G_Pos1vsEvt = new Histogram("DE vs Pos1; GPos1vsEvent", Channels2D, 2);

  hE_gE_G1 = new Histogram("E; GE-G1", Channels1D, 1);

  hSiTotalE_G_SiDEvsSiE = new Histogram("SiTotE; GSiDEvsSiE", Channels1D, 1);
  hSiDEvsSiTotalE_G_SiDEvsSiE = new Histogram("SiDE vs SiTotE; GSiDEvsSiE",
						Channels2D, 2);

  hSiTotalE_G_Pos1vsEvt = new Histogram("SiTotE; GPos1vsEvent", Channels1D, 1); // NEW 4/17/23 - Will
  hSiTotalE_G_SiDEvsSiE_G_Pos1vsEvt = new Histogram("SiTotE; GSiDEvsSiE + GPos1vsEvent", Channels1D, 1); // NEW 4/17/23 - Will

  hSiTotalEvsEvt_G_SiDEvsSiE = new Histogram("SiTotE vs Event; GSiDEvsSiE", Channels2D, 2); // NEW 5/4/23 - Will
  
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

  hDEvsE -> addGate("DE vs E Gate 1");
  hDEvsE -> addGate("DE vs E Gate 2");

  hPos1vsEvt -> addGate("Event Gate");

  hSiDEvsSiE -> addGate("Silicon Gate");
  
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
  EventNo++;

  double ADCsize = nADC; //sizeof(&dADC)/sizeof(&dADC[0]);
  double TDCsize = nTDC; //sizeof(dTDC)/sizeof(dTDC[0]);

  //std::cout << ADCsize << "  " << TDCsize << std::endl;
  //std::cout << dADC << "  " << dTDC << std::endl;
  
  // Thresholds
  for(int i=0; i<ADCsize; i++)
    if(dADC[i] < Threshold || dADC[i] > Channels1D)dADC[i]=0;
  for(int i=0; i<TDCsize; i++)
    if(dTDC[i] < Threshold || dTDC[i] > Channels1D)dTDC[i]=0;

  //  std::cout << "Done thresholding" << std::endl;
  int cE=0, cDE=0, cPos1=0, cPos2=0, cSiE=0, cSiDE=0;
  int cTDC_E=0, cTDC_DE=0, cTDC_Pos1=0, cTDC_Pos2=0;
  // Define the channels
  if(ADCsize > 6){
    cE = dADC[0];
    cDE = dADC[1];
    cPos1 = dADC[2];
    cPos2 = dADC[3];
    cSiE = dADC[5];
    cSiDE = dADC[7];
  }
  if(TDCsize > 3){
    cTDC_E = dTDC[0];
    cTDC_DE = dTDC[1];
    cTDC_Pos1 = dTDC[2];
    cTDC_Pos2 = dTDC[3];
  } 
  //  std::cout << cPos1 << std::endl;

  vPos1.push_back(cPos1);
  vDE.push_back(cDE);
  vSiE.push_back(cSiE);
  vSiDE.push_back(cSiDE);
  
  // Calculate some things
  int cTheta = (int)std::round(10000.0*atan((cPos2-cPos1)/100.)/3.1415 - 4000.);
  cTheta = std::max(0,cTheta);
  int cTDC_PosSum = (int)std::round((cTDC_Pos1 + cTDC_Pos2)/2.);

  // ------------------------------------------------------------
  // Compressed versions for 2D spectra
  // ------------------------------------------------------------
  double compression = (double)Channels1D/ (double)Channels2D;
  int cEcomp = (int) std::floor(cE / compression);
  int cDEcomp = (int) std::floor(cDE / compression);
  int cPos1comp = (int) std::floor(cPos1 / compression);
  int cPos2comp = (int) std::floor(cPos2 / compression);
  int cThetacomp = (int) std::floor(cTheta / compression);
  
  int cSiEcomp = (int) std::floor(cSiE / compression);
  int cSiDEcomp = (int) std::floor(cSiDE / compression);
	
  // std::cout << "Compressed data" << std::endl;
  // Calculate the total silicon energy
  int cSiTotalE = (int) std::floor((cSiE + pSiSlope*cSiDE) / (1.0+pSiSlope));
  int cSiTotalEcomp = (int) std::floor(cSiTotalE / compression);
  
  vSiTotalE.push_back(cSiTotalE);

  // Increment 1D histograms
  hPos1 -> inc(cPos1);
  hPos2 -> inc(cPos2);
  hDE -> inc(cDE);
  hE -> inc(cE);
  hTheta -> inc(cTheta);

  hSiE -> inc(cSiE);
  hSiDE -> inc(cSiDE);

  //std::cout << "Incremented 1d spec" << std::endl;
  
  hTDC_E -> inc(cTDC_E);
  hTDC_DE -> inc(cTDC_DE);
  hTDC_PosSum -> inc(cTDC_PosSum);

  //std::cout << "Incremented TDC" << std::endl;
  
  // Increment 2D histograms
  //std::cout << cPos1comp << " " << cDEcomp <<
  //  " " << cEcomp << " " << cPos2comp << " " << cThetacomp << std::endl;
  hDEvsPos1 -> inc(cPos1comp, cDEcomp);
  hEvsPos1 -> inc(cPos1comp, cEcomp);
  hDEvsE -> inc(cEcomp, cDEcomp);
  hPos2vsPos1 -> inc(cPos1comp, cPos2comp);
  hThetavsPos1 -> inc(cPos1comp, cThetacomp);

  hSiDEvsSiE -> inc(cSiEcomp, cSiDEcomp);

  //std::cout << "2D spect" << std::endl;
  
  // The gated spectrum
  //std::cout << "gated spec" << std::endl; 

  Gate &G = hE->getGate(0);
  if(G.inGate(cE)){
    hE_gE_G1->inc(cE);
  }
  
  Gate &G1 = hDEvsPos1->getGate(0);
  //G1.Print();
  if(G1.inGate(cPos1comp,cDEcomp)){
    gateCounter++;;
    hPos1_gDEvPos1_G1->inc(cPos1);
  }

  Gate &G2 = hDEvsPos1->getGate(1);
  //G2.Print();
  if(G2.inGate(cPos1comp,cDEcomp)){
    gateCounter++;;
    hPos1_gDEvPos1_G2->inc(cPos1);
  }

  // The silicon gates
  Gate &G3 = hSiDEvsSiE->getGate(0);
  if(G3.inGate(cSiEcomp, cSiDEcomp)){
    gateCounter++;;
    hSiTotalE_G_SiDEvsSiE->inc(cSiTotalE);
    hSiDEvsSiTotalE_G_SiDEvsSiE->inc(cSiTotalEcomp,cSiDEcomp);
  }
}

void EngeSort::FillEndOfRun(int nEvents){

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

  //int numSiEandPos1Coin = 0;
  //int numSiDEandPos1Coin = 0;
  //int numSiEandSiDEandPos1Coin = 0;
  //int numSiEandSiDECoin = 0;

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
      gateCounter++;;
      hPos1_gPos1vsEvt->inc(vPos1[i]); // cPos1
      hDEvsPos1_G_Pos1vsEvt->inc(p,de); // cDEcomp vs cPos1comp
      nEventsInGate_P1++;
      bPos1vsEventGate = true;
    }

    // Cutting the same events from the SiTotalE spectrum as are cut from the Pos1 spectrum, so the P1 peak / Si peak ratio is consistent.
    // Trick: Using Pos1vsEvt gate, but checking if (e,sitotale) is in the gate. The y-axis value doesn't matter since the gate spans the entire y-axis.
    // An alternative solution would be to make a separate histogram of SiTotalE vs Event and gate on this too, 
    // but this would require the Pos1vsEvent and SiTotalEvsEvent histograms to have identical gates to get the identical events.
    if(G4.inGate(e, sitotale)){
      hSiTotalE_G_Pos1vsEvt->inc(vSiTotalE[i]); // cSiTotalE
      nEventsInGate_Si++;
    }

    Gate &G5 = hDEvsPos1->getGate(1); // Same as G2, which is the default DEvsPos1 for some reason
    if(G4.inGate(e, p) && G5.inGate(p, de)){ // Double gate - DEvsPos1 and Pos1vsEvent
      hPos1_gDEvPos1_gPos1vsEvt->inc(vPos1[i]); // cPos1
    }

    Gate &G6 = hSiDEvsSiE->getGate(0);
    // Double Gate - SiDEvsSiE and Pos1vsEvent (same trick as above to get Si events in the same gate as Pos1 vs Event)
    if(G4.inGate(e, sitotale) && G6.inGate(sie, side)){
      hSiTotalE_G_SiDEvsSiE_G_Pos1vsEvt->inc(vSiTotalE[i]); // cSiTotalE
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


// Connect the analyzer to midas
int EngeSort::connectMidasAnalyzer(){

  TARegister tar(&mAMod);

  mAMod.ConnectEngeAnalyzer(this);
  return 0;
}

// Run the midas analyzer
int EngeSort::runMidasAnalyzer(boost::python::list file_list){
  std::cout << "runMidasAnalyzer " << len(file_list) << std::endl;
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

  for(auto Sclr: Scalers){
    Sclr -> Clear();
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
  printf("Finish!\n");
  eA->FillEndOfRun(fTotalEventCounter);
  //  printf("Counted %d events\n",fTotalEventCounter);
  //  std::cout << "number of spectra: " << eA->getSpectrumNames().size() << std::endl;
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
  //  time_t run_start_time = runinfo->fOdb->odbReadUint32("/Runinfo/Start time binary", 0, 0);
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
    .def("getNGates", &EngeSort::getNGates)            // int vector
    .def("getSpectrumNames", &EngeSort::getSpectrumNames) // string vector
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
