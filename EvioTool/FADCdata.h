//
//  FADCdata.h
//  EvioTool
//
//  Created by Maurik Holtrop on 12/28/18.
//  Copyright © 2018 UNH. All rights reserved.
//
//
//  These are specialized data types to store FADC data.
//
#ifndef __FADCdata__
#define __FADCdata__

#include "TROOT.h"
#include "TObject.h"
#include "Leaf.h"

#define GET_CHAR(b,i)   b[i]; i+=1;
#define GET_SHORT(b,i) ((short *)(&b[i]))[0];i+=2;
#define GET_INT(b,i)  ((int *)(&b[i]))[0];i+=4;
#define GET_L64(b,i) ((unsigned long long *)(&b[i]))[0];i+=8;

#define N_PEDESTAL 10  // Number of initial values to use for the pedestal

class FADCdata{
  
public:
  unsigned char crate=0;
  unsigned char  slot=0;
  unsigned char  chan=0;
  unsigned int  trig=0;
  unsigned long long reftime=0;       /* 64 bit unsinged int */
  unsigned short time=0;
  unsigned short adc=0;
  std::vector<unsigned short> samples;

public:
  FADCdata(){};
  FADCdata(unsigned char cr,unsigned char sl,unsigned int tr,unsigned long long refti, unsigned short ch, unsigned short ti, unsigned short adc_val):
  crate(cr),slot(sl),trig(tr),reftime(refti),chan(ch),time(ti),adc(adc_val){
  };
  FADCdata(unsigned char cr,unsigned char sl,unsigned int tr,unsigned long long ti, int &indx, unsigned char *cbuf):
  crate(cr),slot(sl),trig(tr),reftime(ti){
    chan = GET_CHAR(cbuf,indx);
    int nsamples = GET_INT(cbuf,indx);
    samples.reserve(nsamples);
    unsigned short *sdata = (unsigned short *)&cbuf[indx];
    samples.assign(sdata,sdata+nsamples);  // Fast(ish) copy of sample data. ~ 11ns for 50 samples, ~14ns for 100 samples.
    indx+=nsamples*2;
  };
  
public:
  size_t         GetSampleSize(){return samples.size();};
  unsigned short GetSample(int i){return samples[i];};
  unsigned char  GetCrate(){return( crate);};
  unsigned char  GetSlot(){return(slot);};
  unsigned char  GetChan(){return(chan);};
  unsigned int   GetTrig(){return(trig);};
  unsigned long long GetRefTime(){return(reftime);};
  unsigned short GetTime();
  unsigned short GetAdc();
  std::tuple<float,float,float,int,int> ComputeMode3(float thres=0); // return integral and time and pedestal as Mode3 computes it.
  std::tuple<float,float,float,int,int> ComputeMode7(float thres=0, int NSB=5,int NSA=25); // return integral and time and pedestal as Mode7 computes it.
  std::tuple<float,float,float> PulseFit(float width=0.);  // return integral and time and pedestal using a 3-pole fit.
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(FADCdata,1);
#pragma clang diagnostic pop

};

std::ostream &operator<<(std::ostream &os, FADCdata const &m);


#endif /* FADCdata_h */
