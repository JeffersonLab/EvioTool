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

class FADCdata{
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
  unsigned char crate;
  unsigned char  slot;
  unsigned char  chan;
  unsigned int  trig;
  unsigned long long reftime;       /* 64 bit unsinged int */
  unsigned short time;
  unsigned short adc;
  std::vector<unsigned short> samples;
  ClassDef(FADCdata,1);
};

std::ostream &operator<<(std::ostream &os, FADCdata const &m);


#endif /* FADCdata_h */
