/********************************************************************\

  Name:         fev1730.cxx
  Created by:   R. Longland

  Contents:     Frontend for VME DAQ using CAEN ADC, and V1730
                (based on the generic frontend by K.Olcanski)
$Id$
\********************************************************************/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <assert.h>
#include "midas.h"
#include "mfe.h"
#include "mvmestd.h"

using namespace std;

//#define HAVE_V792
#define HAVE_V1730

#ifdef HAVE_V792
#include "vme/v792.h"
#endif
#ifdef HAVE_V1730
#include "v1730drv.h"
#endif

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
   const char *frontend_name = "fev1730";
/* The frontend file name, don't change it */
   const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
   BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
   INT display_period = 000;

/* maximum event size produced by this frontend */
   INT max_event_size = 4100000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
   INT max_event_size_frag = 5*1024*1024;

/* buffer size to hold events */
   INT event_buffer_size = 50000000;

  extern INT run_state;
  extern HNDLE hDB;

/*-- Function declarations -----------------------------------------*/
  INT frontend_init();
  INT frontend_exit();
  INT begin_of_run(INT run_number, char *error);
  INT end_of_run(INT run_number, char *error);
  INT pause_run(INT run_number, char *error);
  INT resume_run(INT run_number, char *error);
  INT frontend_loop();

  INT read_event(char *pevent, INT off);

  void AllDataClear();
/*-- Bank definitions ----------------------------------------------*/

/*-- Equipment list ------------------------------------------------*/

  EQUIPMENT equipment[] = {

    {"Trigger",               /* equipment name */
     {1, TRIGGER_ALL,         /* event ID, trigger mask */
      "SYSTEM",               /* event buffer */
      EQ_POLLED,              /* equipment type */
      //EQ_PERIODIC,
      1,                      /* event source */
      "MIDAS",                /* format */
      TRUE,                   /* enabled */
      RO_RUNNING,             /* read only when running */
      500,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      0,                      /* don't log history */
      "", "", "",}
     ,
     read_event,      /* readout routine */
     NULL, NULL,
     NULL,       /* bank list */
    }
    ,

    {""}
  };

/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.

  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.
\********************************************************************/

MVME_INTERFACE *gVme = 0;

int gAdcBase   = 0x200000;
int gV1730Base = 0xABFE0000;


// counters
int nEvtAdc = 0;
int nEvtV1730 = 0;

// Globals for the V1730
#ifdef HAVE_V1730
int nchannels;
uint32_t nsamples;
bool useBLT = true;   // Use BLock Transfers?
#endif

int mvme_read16(int addr)
{
  mvme_set_dmode(gVme, MVME_DMODE_D16);
  return mvme_read_value(gVme, addr);
}

int mvme_read32(int addr)
{
  mvme_set_dmode(gVme, MVME_DMODE_D32);
  return mvme_read_value(gVme, addr);
}

void encodeU32(char*pdata,uint32_t value)
{
  pdata[0] = (value&0x000000FF)>>0;
  pdata[1] = (value&0x0000FF00)>>8;
  pdata[2] = (value&0x00FF0000)>>16;
  pdata[3] = (value&0xFF000000)>>24;
}

uint32_t odbReadUint32(const char*name,int index,uint32_t defaultValue = 0)
{
  int status;
  uint32_t value = 0;
  int size = 4;
  HNDLE hkey=0;
  status = db_get_data_index(hDB,hkey,&value,&size,index,TID_DWORD);
  if (status != SUCCESS)
    {
      cm_msg (MERROR,frontend_name,"Cannot read \'%s\'[%d] from odb, status %d",name,index,status);
      return defaultValue;
    }
  return value;
}

int enable_trigger()
{

  AllDataClear();
  v1730_AcqCtl(gVme,gV1730Base,V1730_RUN_START);
  return 0;
}

void AllDataClear(){

  // Clear the ADC
#ifdef HAVE_V792
  v792_DataClear(gVme, gAdcBase);
  v792_EvtCntReset(gVme,gAdcBase);
#endif
  
#ifdef HAVE_V1730
  // Enable the running on the V1730
  v1730_RegisterWrite(gVme, gV1730Base,V1730_SW_CLEAR,1);
#endif

}

int disable_trigger()
{

#ifdef HAVE_V1730

  // Disable acquisition on V1730
  v1730_AcqCtl(gVme,gV1730Base,V1730_RUN_STOP);

#endif

  return 0;
}

INT init_vme_modules()
{

#ifdef HAVE_V792

  /*
  printf("---------------------------\n");
  printf("------ CAEN V785 ADC ------\n\n");
  v792_DataClear(gVme, gAdcBase);
  */

  // reset the module, event counter, and data
  v792_SoftReset(gVme, gAdcBase);
  v792_EvtCntReset(gVme,gAdcBase);
  v792_DataClear(gVme, gAdcBase);

  // --- Custom setup --- 
  // Allow writing the over- and under-threshold events (so we get a
  // number even if it's a bad event). This is important for event
  // matching.
  v792_Write16(gVme, gAdcBase, V792_BIT_SET2_RW, 0x8);
  //  v792_EmptyEnable(gVme, gAdcBase);

  // Setup the thresholds
  printf("Setting thresholds\n");
  WORD threshold[32];
  for (int i=0;i<32;i++) {
    threshold[i] = 0x0000;
  }
  v792_ThresholdWrite(gVme, gAdcBase, threshold);

  // printf("Getting V785 Status\n");
  // v792_Status(gVme,gAdcBase);
  // printf("--------------------------\n");

#endif

#ifdef HAVE_V1730
  printf("--------------------------------------------------\n");
  printf("-------------- CAEN V1730 Digitizer --------------\n");

  uint32_t status;
  int tpre, tlength;
  bool extTrigger, negSignals;
  uint32_t offset, cThresh;

  ifstream f("settings.dat");
  if (!f){
    // Print an error and exit
    cerr << "ERROR! settings.dat could not be opened" << endl;
    exit(1);
  }
  /*
  int tpre = 100;          // Pre-trigger time (ns)
  int tlength = 1600;       // Length of signals (ns)
  bool extTrigger = TRUE;  // Do we want to use an external trigger?
  bool negSignals = FALSE;  // Are the signals negative?
  uint32_t offset = 100;    // Signal offset (~ 0-16535) - Use large
			    // numbers for negative signals
  uint32_t cThresh = 200;   // Trigger threshold (in ADC channels, 0-16535)
  */
  f >> tpre;
  f.ignore(100,'\n');
  f >> tlength;
  f.ignore(100,'\n');
  f >> extTrigger;
  f.ignore(100,'\n');
  f >> negSignals;
  f.ignore(100,'\n');
  f >> offset;
  f.ignore(100,'\n');
  f >> cThresh;

  f.close();

  cout << "tpre = " << tpre << endl;
  cout << "tlength = " << tlength << endl;
  cout << "extTrigger = " << extTrigger << endl;
  cout << "negSignals = " << negSignals << endl;
  cout << "offset = " << offset << endl;
  cout << "cThresh = " << cThresh << endl;

  // reset amd setup the digitizer
  v1730_AcqCtl(gVme,gV1730Base,V1730_RUN_STOP);
  v1730_Reset(gVme,gV1730Base);
  v1730_Setup(gVme,gV1730Base,1);  // standard settings
  if(useBLT)
    v1730_SetupBLT(gVme,gV1730Base);

  // Now that the buffers are setup, print a summary
  //  int nchannels;
  uint32_t n32word;
  nsamples = v1730_SamplesGet(gVme, gV1730Base);

  // We need to reduce the number of samples to prevent buffer
  // overwrites (custom event size)
//  if(useBLT){
//    v1730_RegisterWrite(gVme,gV1730Base,0x8020,0);
//  } else {
  v1730_RegisterWrite(gVme,gV1730Base,0x8020,(tlength/2)/10);
  nsamples = v1730_SamplesGet(gVme, gV1730Base);
    //  }
  v1730_info(gVme, gV1730Base, &nchannels, &n32word); 

  // Post trigger
  // We know how much time we want to record prior to the trigger: tpre
  int Npost = nsamples - tpre/2;
  printf("\n --- Setting up post-trigger time\n");
  v1730_PostTriggerSet(gVme, gV1730Base,Npost);
  Npost = 8*v1730_RegisterRead(gVme,gV1730Base,V1730_POST_TRIGGER_SETTING)+144;
  printf("Post trigger samples = %d\n",Npost);
  printf("Post trigger time = %d ns\n",Npost*2);
  printf("Pre trigger time = %d ns\n",(nsamples-Npost)*2);

  // Set up acquisition modes
  // Run control by register (SW control)
  v1730_AcqCtl(gVme, gV1730Base, V1730_REGISTER_RUN_MODE);

  // Software or External trigger (for now)
  if(extTrigger){
    v1730_TrgCtl(gVme, gV1730Base, V1730_TRIG_SRCE_EN_MASK,
		 V1730_SOFT_TRIGGER|V1730_EXTERNAL_TRIGGER);
  }else{
    v1730_TrgCtl(gVme, gV1730Base, V1730_TRIG_SRCE_EN_MASK,
		 V1730_SOFT_TRIGGER|0xF); // for first 4 channels
  }
  // Software or External trigger output
  v1730_TrgCtl(gVme, gV1730Base, V1730_FP_TRIGGER_OUT_EN_MASK,
	       V1730_SOFT_TRIGGER|V1730_EXTERNAL_TRIGGER);

  
  // Calibrate the channels
  printf("\n --- Calibrating channels\n");
  int chanmask = 0xFFFF & v1730_RegisterRead(gVme, gV1730Base, V1730_CHANNEL_EN_MASK);
  for(int i=0; i<16; i++)
    if(chanmask & (1<<i))
      v1730_ChannelCalibrate(gVme, gV1730Base, i);

  // set the offsets and thresholds
  printf("\n --- Setting DC offsets\n");
  for(int i=0; i<4; i++){
    v1730_ChannelDACSet(gVme,gV1730Base,i,offset);
  }
  
  sleep(2);   // wait a few seconds for things to stablise

  
  printf("\n --- Setting the thresholds\n");
  for(int i=0; i<4; i++){
    v1730_ChannelThresholdSet(gVme,gV1730Base,i,cThresh);
    cThresh = v1730_ChannelGet(gVme,gV1730Base,i,V1730_CHANNEL_THRESHOLD);
    printf("Channel %d  : Threshold = %d\n",i,cThresh);
  }
  
  if(negSignals){
    // Set trigger to be underthreshold
    printf("\n --- Triggering on negative signals\n");
    v1730_ChannelConfig(gVme, gV1730Base, V1730_TRIGGER_UNDERTH);
  }

  // TODO - Read the temperatures
  

  // test by turning on the module for a second
  printf("\n --- Acquiring a second of data\n");
  v1730_AcqCtl(gVme,gV1730Base,V1730_RUN_START);
  // write a software trigger
  sleep(1);
  v1730_RegisterWrite(gVme, gV1730Base, V1730_SW_TRIGGER, 1);
  usleep(100);

  // Read stuff
  uint32_t   data[100000];
  INT  nentry, nsize, npt;
  npt = nsamples;

  uint32_t n32read;

  printf("\n\n");
  printf("--- Reading V1730! ---\n");
  
  nentry = v1730_RegisterRead(gVme, gV1730Base, V1730_EVENT_STORED);
  printf("There are %d stored events",nentry);
  nsize = v1730_RegisterRead(gVme, gV1730Base, V1730_EVENT_SIZE);
  printf(" with size = %d\n",nsize);
  
  n32word = 4 + npt*nchannels/2;
  printf("We want to read %d samples in %d channels so n32read = %d\n",npt,nchannels,n32word);

  //  uint32_t channel=0; // read channel 0
  
  do {
    status = v1730_RegisterRead(gVme, gV1730Base, V1730_ACQUISITION_STATUS) & 0xFFFF;
    printf("Acq Status1:0x%x\n", status);
  } while ((status & 0x8)==0);
  
  if(useBLT){
    n32read = v1730_DataBlockRead(gVme, gV1730Base, &(data[0]), n32word);
    n32read = n32word - n32read;
    printf("Finished block reading with n32read = %d\n",n32read);
  } else {
    n32read = v1730_DataRead(gVme, gV1730Base, &(data[0]), n32word);
    printf("Finished reading with n32read = %d\n",n32read);
  }

  // Look at first 20 samples
  printf("First 20 samples are:\n");
  int s=0;
  uint32_t val;
  for(int i=4; i<14; i++){
    val=data[i]>>16 & 0x3FFF;
    printf("s[%d]: %x: %d    ",s,val,val);
    s++;
    val=data[i] & 0x3FFF;
    printf("s[%d]: %x: %d\n",s,val,val);
    s++;
  }

  // do {
  //   status = v1730_RegisterRead(gVme, gV1730Base, V1730_ACQUISITION_STATUS);
  //   printf("Acq Status2:0x%x\n", status);
  // } while ((status & 0x8)==0);


  
  // for (int i=0; i<50;i+=4) {
  //   printf("[%d]:0x%x - [%d]:0x%x - [%d]:0x%x - [%d]:0x%x\n"
  // 	   , i, data[i], i+1, data[i+1], i+2, data[i+2], i+3, data[i+3]);
  // }

  //v1730_AcqCtl(gVme,gV1730Base,V1730_RUN_STOP);

#endif

  return SUCCESS;
}

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
  int status;


  std::cout << "Initialising frontend" << std::endl;

  setbuf(stdout,NULL);
  setbuf(stderr,NULL);

  status = mvme_open(&gVme,0);
  status = mvme_set_am(gVme, MVME_AM_A24_ND);
  status = mvme_set_dmode(gVme, MVME_DMODE_D32);

  init_vme_modules();

  // Disable triggering until the beginning of a run
  // (this doesn't do anything at the moment, but everything is
  // cleared at the beginning of the run anyway)
  // disable_trigger();

  return status;
}

static int gHaveRun = 0;

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  gHaveRun = 0;
  disable_trigger();

  mvme_close(gVme);

  return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  gHaveRun = 1;
  printf("begin run %d\n",run_number);

  //init_vme_modules();

  enable_trigger();

  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  static bool gInsideEndRun = false;

  if (gInsideEndRun)
    {
      printf("breaking recursive end_of_run()\n");
      return SUCCESS;
    }

  gInsideEndRun = true;

  gHaveRun = 0;
  printf("end run %d\n",run_number);
  disable_trigger();

  gInsideEndRun = false;

  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  gHaveRun = 0;
  disable_trigger();
  return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  gHaveRun = 1;
  enable_trigger();
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop()
{
  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
  return SUCCESS;
}

/********************************************************************\

  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/
int poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{

  // Look for an event in the ADC before going any further
  for (int i=0 ; i<count ; i++){
    //    int lam = v792_DataReady(gVme,gAdcBase);
    int nV1730 = v1730_RegisterRead(gVme, gV1730Base, V1730_EVENT_STORED);
    int nV785 = 1;
#ifdef HAVE_V792
    nV785 = v792_DataReady(gVme,gAdcBase);
#endif
    if (nV1730 && nV785)
      if (!test)
        return TRUE;
  }
  return FALSE;

}

/*-- Interrupt configuration ---------------------------------------*/
int interrupt_configure(INT cmd, INT source, PTYPE adr)
{
   switch (cmd) {
   case CMD_INTERRUPT_ENABLE:
     break;
   case CMD_INTERRUPT_DISABLE:
     break;
   case CMD_INTERRUPT_ATTACH:
     break;
   case CMD_INTERRUPT_DETACH:
     break;
   }
   return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

#ifdef HAVE_V792
int read_v792(int base,const char*bname,char*pevent,int nchan)
{
  int wcount = 0, eventcount=0, readcount=0;
  DWORD data[100];
  WORD *pdata;
  DWORD counter = 0;

  /* Event counter */
  v792_EvtCntRead(gVme, base, &counter);

  //  printf("Reading MEB from V785\n");
  //printf("V785 Event counter = %d\n",counter);

  while(v792_DataReady(gVme,base)){

    //printf("v785 entry %d\n",eventcount);
    eventcount++;
    nEvtAdc++;
    
    /* create ADCS bank */
    bk_create(pevent, bname, TID_WORD, (void **)&pdata);
    
    /* Read Single Event */
    wcount=0;
    v792_EventRead(gVme, base, data, &wcount);


    // Figure out what channels are firing and arrange the data accordingly
    for(int i=0; i<nchan; i++)
      pdata[i]=0;
    //printf("wcount = %d\n",wcount);
    for (int i=0; i<wcount; i++)
      {
	uint32_t w = data[i];
	if (((w>>24)&0x7) != 0) continue;
	int chan = (w>>16)&0x1F;
	int val  = (w&0x1FFF);
	//	printf("i: %d  chn: %d  val: %d\n",i,chan,val);
	pdata[chan] = val;
      }

    pdata += nchan;
    readcount += wcount;
    
    // Close the bank
    bk_close(pevent, pdata);
    
  } // end of while(v792_DataReady())

  //  printf("Read %d events\n",eventcount);

  return eventcount;
}
#endif

#ifdef HAVE_V1730
int read_v1730(int base,const char* bname, char* pevent, int eventcount)
{
  uint32_t   data[100000];
  INT  npt, nentry, nsize;
  uint32_t status, n32word, n32read, totaln32read=0;
  WORD* pdata;


  //  v1730_info(gVme, gV1730Base, &nchannels, &n32word); 
  //uint32_t nsamples = v1730_SamplesGet(gVme, gV1730Base);
  npt=nsamples;
  
  //  printf("--- Reading V1730! ---\n");
  nentry = v1730_RegisterRead(gVme, gV1730Base, V1730_EVENT_STORED);
#ifdef HAVE_V792
  nentry = min(nentry,eventcount); // only read as many events as there were in the ADC
#endif

  //printf("V1730 event counter = %d\n",nentry);
  nsize = v1730_RegisterRead(gVme, gV1730Base, V1730_EVENT_SIZE);
  //  printf(" with size = %d\n",nsize);

  n32word = 4 + npt*nchannels/2;
  //  printf("We only want to read %d samples so n32read = %d\n",npt,n32word);

  // Make sure there's something to read
  status = v1730_RegisterRead(gVme, gV1730Base, V1730_ACQUISITION_STATUS) & 0xFFFF;
  //printf("Acq Status1:0x%x\n", status);
  if((status & 0x8) == 0){
    printf("No events to read!\n");
    // make an empty bank
    bk_create(pevent, bname, TID_DWORD, (void **)&pdata);  
    bk_close(pevent,pdata);
    return 0;
  }

  // Loop through all of the stored events
  for(int entry=0; entry<nentry; entry++){
    nEvtV1730++;
    //printf("v1730 entry %d\n",entry);
    //    if(nentry > 1)printf("Entry %d\n",entry+1);
    // read the data 
    if(useBLT){
      n32read = v1730_DataBlockRead(gVme, gV1730Base, &(data[0]), n32word);
      n32read = n32word - n32read;
      //      printf("Finished block reading with n32read = %d\n",n32read);
    } else {
      n32read = v1730_DataRead(gVme, gV1730Base, &(data[0]), n32word);
      //      printf("Finished data reading with n32read = %d\n",n32read);
    }

    // Put the data into the pevent array
    // make the bank
    bk_create(pevent, bname, TID_WORD, (void **)&pdata);  
    // ignore the header
    int s=0;
    uint32_t val;
    for(int i=4; i<(int)n32read; i++){
      val = data[i] & 0x3FFF;
      //    printf("s[%d]: 0x%x: %d    ",s,val,val);
      pdata[s++]=val;
      val = data[i]>>16 & 0x3FFF;
      //    printf("s[%d]: 0x%x: %d\n",s,val,val);
      pdata[s++]=val;
    }
    pdata += s;
    // close the bank
    //  cout << "closing the bank" << endl;
    bk_close(pevent,pdata);
  
    totaln32read += n32read;

  }
  // send a trigger
  //  v1730_RegisterWrite(gVme, gV1730Base, V1730_SW_TRIGGER, 1);
  return totaln32read;
}
#endif

INT read_event(char *pevent, INT off)
{

  int dsize,eventcount;
  //printf("read event!\n");
  BOOL have_data = FALSE;

  /* init bank structure */
  bk_init32(pevent);

#ifdef HAVE_V792
  dsize = read_v792(gAdcBase,"ADC1",pevent,32);
  eventcount = dsize;
  if(dsize > 0)have_data = TRUE;
#endif
#ifdef HAVE_V1730
  dsize = read_v1730(gV1730Base,"1730",pevent,eventcount);
  if(dsize > 0)have_data = TRUE;
#endif

  //printf("%d\t%d\n",nEvtAdc, nEvtV1730);

  // See if the event counters agree
#ifdef HAVE_V792
  if(nEvtAdc != nEvtV1730){
    cm_msg(MERROR, frontend_name, 
	   "VME trigger problem: ADC Events: %d   V1730 Events: %d", nEvtAdc, nEvtV1730);

    // Clear ADC and Digitizer
    //disable_trigger();
    AllDataClear();
    //enable_trigger();
    have_data = FALSE;
    //   nEvtAdc = min(nEvtAdc,nEvtV1730);
    nEvtV1730 = nEvtAdc;
    
  } 
#endif

  INT retval = have_data ? bk_size (pevent) : 0;

  return retval;
}

