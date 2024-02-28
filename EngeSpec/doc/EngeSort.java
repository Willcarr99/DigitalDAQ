package sort;

import jam.data.*;
import jam.sort.*;


public class EngeSort extends AbstractSortRoutine {

    ////////////////////////////////////////////////////////////////////////////////////
    // GENERAL DEFINITIONS
    ////////////////////////////////////////////////////////////////////////////////////

    // Set VME Base Adresses for Modules Read Directly by JAM
    static final int ADC_BASE_First = 0xfaa00000;

    // Global Thresholds To Use For ADC and TDC 
    static final int ADC_THRESHOLDS = 0; //factor of 16

    static final int SPC_CHANNELS = 4095;   // Number of channels in spectrum
    static final int TWO_D_CHANNELS = 512; // Number of Channels per dimension in 2D histograms

    // Amount of bits to shift for compression
    final int TWO_D_FACTOR =
	Math.round((float) (Math.log(SPC_CHANNELS / TWO_D_CHANNELS) / Math.log(2.0)));

    // ID definitions for the signals read directly by ADC and TDC
    int idE;
    int idDE;
    int idPos1;
    int idPos2;

    // Si detectors
    int idSiE;
    int idSiDE;
    
    int idTDC_E;
    int idTDC_DE;
    int idTDC_Pos1;
    int idTDC_Pos2;
    
    // Raw data are assigned to the following after read
    int cE;
    int cDE;
    int cPos1;
    int cPos2;

    int cSiE;
    int cSiDE;

    int cTDC_E;
    int cTDC_DE;
    int cTDC_Pos1;
    int cTDC_Pos2;
    
    // Calculated values
    int cTheta;
    int cTDC_PosSum;      // This is the sum of the position TDCs (which should make a peak)
    int cPos1_recon;      // Reconstructed position 1
    
    // ----------------------------------------------------------------------
    // PARAMETERS
    // ----------------------------------------------------------------------
    double cH;     DataParameter pH;
    double cS;     DataParameter pS;
    double cAlpha; DataParameter pAlpha;
    double cOffset; DataParameter pOffset;

    
    // ---------------------------------------------------------
    // HISTOGRAMS
    // ---------------------------------------------------------

    // Ungated 1D spectra taken directly from ADC's and TDC's
    HistInt1D hE;
    HistInt1D hDE;
    HistInt1D hPos1;
    HistInt1D hPos2;
    HistInt1D hTheta;

    HistInt1D hSiE;
    HistInt1D hSiDE;

    HistInt1D hTDC_E;
    HistInt1D hTDC_DE;
    HistInt1D hTDC_PosSum;
    
    // 2D Spectra
    HistInt2D h2dPos2vsPos1;
    HistInt2D h2dDEvsPos1;
    HistInt2D h2dEvsPos1;
    HistInt2D h2dDEvsE;
    HistInt2D h2dThetavsPos1;

    HistInt2D h2dSiDEvsE;

    // Gated Spectra
    HistInt1D hPos1_G_Pos2vsPos1;
    HistInt1D hPos1_G_DEvsPos1;
    HistInt1D hPos1_G_EvsPos1;
    HistInt1D hPos1_G_DEvsE;
    HistInt1D hPos1_G_Pos2vsPos1_DEvsE;

    HistInt1D hR1_G_Pos2vsPos1_DEvsE;

    HistInt2D h2dPos2vsPos1_G_DEvsE;
    
    // Gated Spectra with TDC requirements
    HistInt1D hPos1_G_TDC_Pos2vsPos1;
    HistInt1D hPos1_G_TDC_DEvsPos1;
    HistInt1D hPos1_G_TDC_EvsPos1;
    HistInt1D hPos1_G_TDC_DEvsE;
    HistInt1D hPos1_G_TDC_Pos2vsPos1_DEvsE;

    
    // ---------------------------------------------------------
    // GATES
    // ---------------------------------------------------------

    // TDC Gates
    Gate gTDC_E;
    Gate gTDC_DE;
    Gate gTDC_PosSum;
    
    // 2D Gate in the Pos2 vs Pos1 Spectrum
    Gate g2dPos2vsPos1;
    // 2D Gate in the DE vs Pos1 Spectrum
    Gate g2dDEvsPos1;
    // 2D Gate in the E vs Pos1 Spectrum
    Gate g2dEvsPos1;
    // 2D Gate in the DE vs E Spectrum
    Gate g2dDEvsE;


    // ---------------------------------------------------------
    // SCALERS AND MONITORS
    // ---------------------------------------------------------

    // Scalers Read by the SIS UNIT Requires NIM input
    Scaler sMG;
    Scaler sMGLive;
    Scaler sClock;
    Scaler sClockBusy;
    Scaler sFrontHE;
    Scaler sFrontLE;
    Scaler sBackHE;
    Scaler sBackLE;
    Scaler sE;
    Scaler sDE;
    Scaler BCI;

    ///////////////////////////////////////////////////////////////////////
    // INITIALIZATIONS
    ///////////////////////////////////////////////////////////////////////

    public void initialize() throws Exception {
	
	/*
	  vmeMap.setScalerInterval(3);               // Sets up the Scaler
	  vmeMap.setV775Range(TDC_BASE, TIME_RANGE); // Sets the TIME Range of the TDC

	  // ----------------------------------------------------------------------------------
	  // Set up the mapping for each used channel of the ADC and TDC to the appropriate id 
	  // eventParameters, args = (slot, base address, channel, threshold channel)
	  // ----------------------------------------------------------------------------------

	  for (int i = 0; i < 5; i++) {
	  idSciADC[i] = vmeMap.eventParameter(3, ADC_BASE_First, i+0, SciADC_THRESHOLDS);
	  }

	  idGe     = vmeMap.eventParameter(3, ADC_BASE_First, 5, GeADC_THRESHOLDS);
	  idSciTAC = vmeMap.eventParameter(3, ADC_BASE_First, 7, SciTAC_THRESHOLDS);
	  idNaITAC = vmeMap.eventParameter(3, ADC_BASE_First, 8, NaITAC_THRESHOLDS);	

	  for (int i = 0; i < 16; i++) {
	  idNaIADC[i] = vmeMap.eventParameter(3, ADC_BASE_First, i+16, NaIADC_THRESHOLDS);
	  idNaITDC[i] = vmeMap.eventParameter(7, TDC_BASE, i, NaITDC_THRESHOLDS);
	  }
	*/
	vmeMap.setScalerInterval(3);               // Sets up the Scaler
	//vmeMap.setV775Range(TDC_BASE, TIME_RANGE); // Sets the TIME Range of the TDC
	idE = vmeMap.eventParameter(3, ADC_BASE_First, 0, ADC_THRESHOLDS);
	idDE = vmeMap.eventParameter(3, ADC_BASE_First, 1, ADC_THRESHOLDS);
	idPos1 = vmeMap.eventParameter(3, ADC_BASE_First, 2, ADC_THRESHOLDS);
	idPos2 = vmeMap.eventParameter(3, ADC_BASE_First, 3, ADC_THRESHOLDS);

	idSiE = vmeMap.eventParameter(3, ADC_BASE_First, 5, ADC_THRESHOLDS);
	idSiDE = vmeMap.eventParameter(3, ADC_BASE_First, 7, ADC_THRESHOLDS);

	idTDC_E = vmeMap.eventParameter(4, ADC_BASE_First, 0, ADC_THRESHOLDS);
	idTDC_DE = vmeMap.eventParameter(4, ADC_BASE_First, 1, ADC_THRESHOLDS);
	idTDC_Pos1 = vmeMap.eventParameter(4, ADC_BASE_First, 2, ADC_THRESHOLDS);
	idTDC_Pos2 = vmeMap.eventParameter(4, ADC_BASE_First, 3, ADC_THRESHOLDS);

	// ----------------------------------------------------------------------------------
	// Set up histograms 
	// args = (spectrum name, 1d/2d, number of channels, spectra title, x-label, y-label)
	// ----------------------------------------------------------------------------------

	hE = createHist1D(SPC_CHANNELS,"E","Total Energy Scintillator");
	hDE = createHist1D(SPC_CHANNELS,"DE","Delta-E");
	hPos1 = createHist1D(SPC_CHANNELS,"Pos1","Front Position");
	hPos2 = createHist1D(SPC_CHANNELS,"Pos2","Back Position");
	hTheta = createHist1D(SPC_CHANNELS,"Theta","Incident Particle Angle");

	hSiE = createHist1D(SPC_CHANNELS,"SiE","Silicon E");
	hSiDE = createHist1D(SPC_CHANNELS,"SiDE","Silicon DE");

	// The TDC histograms
	hTDC_E = createHist1D(SPC_CHANNELS,"TDC E","TDC - Total Energy Scintillator");
	hTDC_DE = createHist1D(SPC_CHANNELS,"TDC DE","TDC - Delta-E");
	hTDC_PosSum = createHist1D(SPC_CHANNELS,"TDC PosSum","TDC - Sum of Position TDCs");
	
	h2dPos2vsPos1 = createHist2D(TWO_D_CHANNELS, "Pos2 vs Pos1",
				     "Back Position vs Front Position", "Pos1", "Pos2");
	h2dDEvsPos1 = createHist2D(TWO_D_CHANNELS, "DE vs Pos1",
				   "Delta-E vs Front Position", "Pos1", "DE");
	h2dEvsPos1 = createHist2D(TWO_D_CHANNELS, "E vs Pos1",
				  "Total Energy vs Front Position", "Pos1", "E");
	h2dDEvsE = createHist2D(TWO_D_CHANNELS, "DE vs E",
				"Delta-E vs Total Energy", "E", "DE");
	h2dThetavsPos1 = createHist2D(TWO_D_CHANNELS, "Theta vs Pos1",
				      "Incident Angle vs Front Position", "Pos1", "Theta");


	h2dSiDEvsE = createHist2D(TWO_D_CHANNELS, "Si DE vs E",
				  "Silicon DE vs E", "E", "DE");
	
	// Gated spectra
	hPos1_G_Pos2vsPos1 = createHist1D(SPC_CHANNELS,"P1 G-P2vP1","Front Position Gated on Pos2vsPos1");
        hPos1_G_DEvsPos1   = createHist1D(SPC_CHANNELS,"P1 G-DEvP1","Front Position Gated on DEvsPos1");
	hPos1_G_EvsPos1    = createHist1D(SPC_CHANNELS,"P1 G-EvP1","Front Position Gated on EvsPos1");
        hPos1_G_DEvsE      = createHist1D(SPC_CHANNELS,"P1 G-DEvE","Front Position Gated on DEvsE");
        hPos1_G_Pos2vsPos1_DEvsE = createHist1D(SPC_CHANNELS,"P1 G-P2vsP1&DEvE","Front Position Gated on Pos2vsPos1 and DEvsE");

	hR1_G_Pos2vsPos1_DEvsE      = createHist1D(SPC_CHANNELS,"R1 G-DEvE","Reconstructed Front Position Gated on DEvsE");

	hPos1_G_TDC_Pos2vsPos1 = createHist1D(SPC_CHANNELS,"P1 G-TDC&P2vP1","Front Position Gated on TDC & Pos2vsPos1");
        hPos1_G_TDC_DEvsPos1   = createHist1D(SPC_CHANNELS,"P1 G-TDC&DEvP1","Front Position Gated on TDC & DEvsPos1");
	hPos1_G_TDC_EvsPos1    = createHist1D(SPC_CHANNELS,"P1 G-TDC&EvP1","Front Position Gated on TDC & EvsPos1");
        hPos1_G_TDC_DEvsE      = createHist1D(SPC_CHANNELS,"P1 G-TDC&DEvE","Front Position Gated on TDC & DEvsE");
        hPos1_G_TDC_Pos2vsPos1_DEvsE = createHist1D(SPC_CHANNELS,"P1 G-TDC&P2vsP1&DEvE","Front Position Gated on TDC & Pos2vsPos1 and DEvsE");

	h2dPos2vsPos1_G_DEvsE = createHist2D(TWO_D_CHANNELS, "Pos2 vs Pos1 G-DEvsE",
				     "Back Position vs Front Position Gated on DEvsE", "Pos1", "Pos2");

	
	// ---------------------------------------------------------
	// Set up gates
	// ---------------------------------------------------------

	// The TDC gates
	gTDC_E = new Gate("TDC E", hTDC_E);
	gTDC_DE = new Gate("TDC DE", hTDC_DE);
	gTDC_PosSum = new Gate("TDC PosSum", hTDC_PosSum);
	
	g2dPos2vsPos1 = new Gate("Pos2 vs Pos1 gate", h2dPos2vsPos1);
        g2dDEvsPos1 = new Gate("DE vs Pos1 gate", h2dDEvsPos1);
        g2dEvsPos1 = new Gate("E vs Pos1 gate", h2dEvsPos1);
        g2dDEvsE = new Gate("DE vs E gate", h2dDEvsE);


	// ---------------------------------------------------------
	// Set up scaler information ("Input id", "Input Channel")
	// ---------------------------------------------------------
	sMG =        createScaler("Master Gate", 0);
	sMGLive =    createScaler("Master Gate Live", 1);
	sClock =     createScaler("Clock", 2);
	sClockBusy = createScaler("Clock Live", 3);
	sFrontHE =   createScaler("Front HE", 4);
	sFrontLE =   createScaler("Front LE", 5);
	sBackHE =    createScaler("Back HE", 6);
	sBackLE =    createScaler("Back LE", 7);
	sE =         createScaler("E", 8);
	sDE =        createScaler("DE", 9);
	BCI =        createScaler("BCI", 15);
	
	// ---------------------------------------------------------
	// Monitors
	// ---------------------------------------------------------
	//	mBeam = new Monitor("Beam",sBCI);

	// ---------------------------------------------------------
	// Sort Parameters
	// ---------------------------------------------------------		

	pH = createParameter("pH");
	pS = createParameter("pS");
	pAlpha = createParameter("pAlpha");
	pOffset = createParameter("pOffset");
	
	// Enable output error messages to screen
	System.err.println("# Parameters: " + getEventSize());
	System.err.println("SPC channels: " + SPC_CHANNELS);
	System.err.println("2d channels: "+ TWO_D_CHANNELS + ", compression factor: "
			   + TWO_D_FACTOR);
    }

    ///////////////////////////////////////////////////////////////////////
    // DATA SORT [loop for each event to be processed]
    ///////////////////////////////////////////////////////////////////////

    // This Section Handles the Data that is Read in
    public void sort(int[] data) throws Exception {
     
	// -----------------------------------------------------
	// Read data and give appropriate assignment 
	// -----------------------------------------------------

	int cE = data[idE];
	int cDE = data[idDE];
	int cPos1 = data[idPos1];
	int cPos2 = data[idPos2];

	int cSiE = data[idSiE];
	int cSiDE = data[idSiDE];

	double dPos1 = cPos1+Math.random()-0.5;
	double dPos2 = cPos2+Math.random()-0.5;

	int cTDC_E = data[idTDC_E];
	int cTDC_DE = data[idTDC_DE];
	int cTDC_Pos1 = data[idTDC_Pos1];
	int cTDC_Pos2 = data[idTDC_Pos2];
	
	cTheta = (int) Math.round(10000.0*Math.atan((cPos2-cPos1)/100.)/3.1415 - 4000.);
	cTDC_PosSum = (int) Math.round((cTDC_Pos1 + cTDC_Pos2)/2.);

	// Ray tracing
	cH = pH.getValue();
	cS = pS.getValue();
	cAlpha = pAlpha.getValue();
	cOffset = pOffset.getValue();

	double numerator = cH + cS*Math.cos(cAlpha)*(dPos2)/(dPos1-dPos2);
	double denominator = cS*Math.cos(cAlpha)/(dPos1-dPos2) + Math.sin(cAlpha);
	double recon = numerator/denominator;

	cPos1_recon = (int) Math.floor(recon+cOffset);
	
	// ------------------------------------------------------------
	// Compressed versions for 2D spectra
	// ------------------------------------------------------------
	int cEComp = (int) Math.round(cE * 0.125);
	int cDEComp = (int) Math.round(cDE * 0.125);
	int cPos1Comp = (int) Math.round(cPos1 * 0.125);
	int cPos2Comp = (int) Math.round(cPos2 * 0.125);
	int cThetaComp = (int) Math.round(cTheta * 0.125);

	int cSiEComp = (int) Math.round(cSiE * 0.125);
	int cSiDEComp = (int) Math.round(cSiDE * 0.125);
	
	// --------------------------------------------------------
	// INCREMENT HISTOGRAMS
	// --------------------------------------------------------
        
	// Increment appropriate 1D singles spectra with raw data
	hE.inc(cE);
	hDE.inc(cDE);
	hPos1.inc(cPos1);
	hPos2.inc(cPos2);

	hSiE.inc(cSiE);
	hSiDE.inc(cSiDE);

	// TDC Spectra
	hTDC_E.inc(cTDC_E);
	hTDC_DE.inc(cTDC_DE);
	hTDC_PosSum.inc(cTDC_PosSum);
	
	if(cPos1>200 & cPos1<4096 & cPos2>200 & cPos2<4096)
	    hTheta.inc(cTheta);
                 
	// Increment 2D spectra
	h2dPos2vsPos1.inc(cPos1Comp, cPos2Comp);
	h2dDEvsPos1.inc(cPos1Comp, cDEComp);
	h2dEvsPos1.inc(cPos1Comp, cEComp);
	h2dDEvsE.inc(cEComp, cDEComp);

	h2dThetavsPos1.inc(cPos1Comp, cThetaComp);

	h2dSiDEvsE.inc(cSiEComp,cSiDEComp);
	
	// ----------------------------------------------------------------------------
	// Increment gated spectra:
	// specify explicitly all conditions, otherwise confusion may arise; do not
	// assume any implicit conditions [see message from Dale Visser, Feb. 18, 2013]
	// ----------------------------------------------------------------------------
	// Increment gated Ge spectra (in channels) for logical AND condition: if
	// (NaI fired) AND (event in gate x of 2D energy vs. energy spectrum);

	//if (naifire && (g2d_1.inGate(calibGe2, calibNaIsum2d))) {
	//    hGeNaITE2d_1.inc(cGe);
	//}

	boolean goodtime = gTDC_E.inGate(cTDC_E) & gTDC_DE.inGate(cTDC_DE);
 	
	if(g2dPos2vsPos1.inGate(cPos1Comp,cPos2Comp)){
	    hPos1_G_Pos2vsPos1.inc(cPos1);
	}
	if(g2dDEvsPos1.inGate(cPos1Comp,cDEComp)){
            hPos1_G_DEvsPos1.inc(cPos1);
        }
	if(g2dEvsPos1.inGate(cPos1Comp,cEComp)){
            hPos1_G_EvsPos1.inc(cPos1);
        }
	if(g2dDEvsE.inGate(cEComp,cDEComp)){
            hPos1_G_DEvsE.inc(cPos1);
	    h2dPos2vsPos1_G_DEvsE.inc(cPos1Comp, cPos2Comp);
        }
	if(g2dPos2vsPos1.inGate(cPos1Comp,cPos2Comp) && g2dDEvsE.inGate(cEComp,cDEComp)){
            hPos1_G_Pos2vsPos1_DEvsE.inc(cPos1);
	    hR1_G_Pos2vsPos1_DEvsE.inc(cPos1_recon);
        }

	if(goodtime){
	    if(g2dPos2vsPos1.inGate(cPos1Comp,cPos2Comp)){
		hPos1_G_TDC_Pos2vsPos1.inc(cPos1);
	    }
	    if(g2dDEvsPos1.inGate(cPos1Comp,cDEComp)){
		hPos1_G_TDC_DEvsPos1.inc(cPos1);
	    }
	    if(g2dEvsPos1.inGate(cPos1Comp,cEComp)){
		hPos1_G_TDC_EvsPos1.inc(cPos1);
	    }
	    if(g2dDEvsE.inGate(cEComp,cDEComp)){
		hPos1_G_TDC_DEvsE.inc(cPos1);
	    }
	    if(g2dPos2vsPos1.inGate(cPos1Comp,cPos2Comp) && g2dDEvsPos1.inGate(cPos1Comp,cDEComp)){
		hPos1_G_TDC_Pos2vsPos1_DEvsE.inc(cPos1);
	    }
	}
	///////////////////////////////////////////////////////////////////////
	// FINISH
	///////////////////////////////////////////////////////////////////////
    }


    /**
     * monitor method
     * calculate the live time
     */

    public double monitor(String name) {

	/*
	  if (name.equals("Dead Time")){
	  // For Dead time with busy veto (not working really)
	  double rateDead=(double)(sBusyVeto.getValue()-(lastDead))/
	  (double)(sClock.getValue() - (lastClock));
	  lastDead=sBusyVeto.getValue();
	  lastClock=sClock.getValue();
	  if (((double)(mEvntRaw.getValue()))>0.0){
	  return 100.0*(1.0 - (double)(rateDead)/(double)(mEvntRaw.getValue()));
	  //return rateDead;
	  } else {
	  return 0.0;
	  }
	  } else if (name.equals("Pulser Dead Time")){
	  // Dead time = 1-(pulser gate)/(pulser scaler)
	  double ratePulserPeak= 100.0*(1.0-(double)(gPulser.getArea())/
	  (double)(sPulser.getValue()));
	  return ratePulserPeak;
	  } else if (name.equals("Average Spectrum Rate")){
	  // Get the number of counts in the spectrum, and divide by time
	  double rateSpectrum = (double)(gSpec.getArea())/
	  (double)(sClock.getValue());
	  return rateSpectrum;
	  } else {
	  return 50.0;
	  }
	*/
	return 0;
    }

}


