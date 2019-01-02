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
#ifndef FADCdata_h
#define FADCdata_h

#include "TROOT.h"
#include "TObject.h"
#include "Leaf.h"

#define GET_CHAR(b,i)   b[i]; i+=1;
#define GET_SHORT(b,i) ((short *)(&b[i]))[0];i+=2;
#define GET_INT(b,i)  ((int *)(&b[i]))[0];i+=4;
#define GET_L64(b,i) ((unsigned long long *)(&b[i]))[0];i+=8;

class FADCchan_raw{
public:
  FADCchan_raw(){};
  FADCchan_raw(int &indx,unsigned char *cbuf){
    chan = GET_CHAR(cbuf,indx);
    int        nsamples = GET_INT(cbuf,indx);
    samples.reserve(nsamples);
    unsigned short *sdata = (unsigned short *)&cbuf[indx];
    samples.assign(sdata,sdata+nsamples);  // Fast(ish) copy of sample data. ~ 11ns for 50 samples, ~14ns for 100 samples.
    indx+=nsamples*2;
  };
  FADCchan_raw(unsigned char schan,unsigned short *sdata,int nsamples): chan(schan){
    samples.reserve(nsamples);
    samples.assign(sdata,sdata+nsamples);  // Fast(ish) copy of sample data. ~ 11ns for 50 samples, ~14ns for 100 samples.
  };
  unsigned char chan;
  std::vector<unsigned short> samples;
  ClassDef(FADCchan_raw,1);
};

class FADCchan{
public:
  unsigned char chan;
  std::vector<unsigned short> time;
  std::vector<unsigned short> adc;
  ClassDef(FADCchan,1);
};

class FADCdata{
public:
  FADCdata(){};
  FADCdata(unsigned char cr,unsigned short formatTag, int &indx, unsigned char *cbuf){
    crate =cr;
    if(formatTag == 13){
      slot       = GET_CHAR(cbuf,indx);
      trig       = GET_INT(cbuf,indx);
      time       = GET_L64(cbuf,indx);
      int nchan  = GET_INT(cbuf,indx);
      raw_data.reserve(nchan);
      for(int jj=0; jj<nchan; ++jj){
        raw_data.emplace_back(indx,cbuf);
      }
    }
  };
public:
  unsigned char crate;
  unsigned char  slot;
  unsigned int  trig;
  unsigned long long time;       /* 64 bit unsinged int */
  std::vector<FADCchan> data;
  std::vector<FADCchan_raw> raw_data;
  ClassDef(FADCdata,1);
};

std::ostream &operator<<(std::ostream &os, FADCdata const &m);
#endif /* FADCdata_h */
