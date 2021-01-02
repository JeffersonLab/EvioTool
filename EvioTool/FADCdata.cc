//
//  FADCdata.cpp
//  EvioTool
//
//  Created by Maurik Holtrop on 12/28/18.
//  Copyright © 2018 UNH. All rights reserved.
//
// The FADCdata must be filled by a special method in EvioTool.cc to make sure the data is parsed
// properly. This is an exception to the most other data types.
//
#include "FADCdata.h"
#include "EvioTool.h"

//
// Custom streamers so std::cout << MyFADC << std::endl; works.
//
std::ostream &operator<<(std::ostream &os, FADCdata const &m) {
  os << "\n            " << m.Class_Name() << "(" << (int)m.crate << "," << std::setw(2) << (int)m.slot <<
  "," << std::setw(2) << (int)m.chan << ") [";
  for(int i=0; i< m.samples.size(); ++i){
    os << std::setw(3) << (int)m.samples[i];
    if(i<(m.samples.size()-1)) os << ",";
  }
  os << "]";
  return(os);
}

unsigned short FADCdata::GetTime(){
  return(time);
}

unsigned short FADCdata::GetAdc(){
  return(adc);
}

FADC_Hit_t FADCdata::ComputeMode3(float thres){
  // return integral and time and pedestal as Mode3 computes it.
  // The first N_PEDESTAL (4) samples determine the pedestal.
  // This routine then simply scans the data for the first sample above threshold+pedestal
  // For time, it returns the 4 * the max sample index.
  // For integral it returns the sum of all samples - pedestal*nsamples.
  // If no threshold crossing is found, return zero.
  //
  int max=0;
  int max_loc = -1;
  int thres_loc = -1;
  unsigned int pedest=0;
  unsigned int sum=0;
  for(int i=0;i< samples.size();++i){
    if(i<N_PEDESTAL) pedest+=samples[i];
    if(i>=N_PEDESTAL && samples[i]>thres+pedest/N_PEDESTAL && thres_loc == -1) thres_loc = i;
    sum +=samples[i];
    if(samples[i]>max){
      max = samples[i];
      max_loc = i;
    }
  }
  
  float pedestal   = ((float)pedest)/N_PEDESTAL;
  float integral = sum - samples.size()*pedestal;
  float time;
  if(thres_loc<0) time = 0;
  else time = thres_loc;
  return( FADC_Hit_t(integral,time,pedestal,max,max_loc));
};

FADC_Hit_t FADCdata::ComputeMode7Single(float thres, float pedestal, int NSB,int NSA){
  // Return integral,time and pedestal as Mode7 computes it.
  // Input:
  // thres     - Threshold: minimum value above pedestal
  // pedestal  - Pedestal:  Use this as pedestal. If <=0 then compute pedestal from first N_PEDESTAL (4) samples.
  int max=0;
  int max_loc = -1;
  int thres_loc = -1;
  unsigned int pedest=0;
  unsigned int sum=0;
  for(int i=0;i< samples.size();++i){
    if(i<N_PEDESTAL) pedest+=samples[i];
    if(i>=N_PEDESTAL && samples[i]>thres+pedest/N_PEDESTAL && thres_loc == -1) thres_loc = i;
    if(samples[i]>max){
      max = samples[i];
      max_loc = i;
    }
  }
  if(pedestal<=0){
    pedestal   = ((float)pedest)/N_PEDESTAL;
  }
  int half_max_loc_p1=-1;
  int start=thres_loc-NSB;
  int stop=thres_loc+NSA+1;
  if(thres_loc<=NSB){
    start=0;
  }
  if(thres_loc+NSA+1 >= samples.size()){
    stop = samples.size()-1;
  }

  for(int i=start;i<stop; ++i){
    sum += samples[i];
    if(half_max_loc_p1 == -1 && samples[i] > (max+pedestal)/2){
      half_max_loc_p1 = i;
    }
  }
  
  if(thres_loc<0) return( FADC_Hit_t(0.,0.,0.,0,0));
  
  float time = 0;
  if(samples[half_max_loc_p1] > samples[half_max_loc_p1-1]){
    float mid_line = samples[half_max_loc_p1] - samples[half_max_loc_p1-1];
    time = (((max+pedestal)/2-samples[half_max_loc_p1-1])/mid_line) + half_max_loc_p1-1;
  }

  float integral = sum - pedestal*(stop - start);
  return( FADC_Hit_t(integral,time,pedestal,max,max_loc));
}

const vector<FADC_Hit_t> FADCdata::ComputeMode7(float thres, float pedestal, int NSB,int NSA, int npeak,float gain){
  // Return integral,time and pedestal as Mode7 computes it.
  // NOTE: Calling the ComputeMode7 this way involves a *copy* of the returned vector<> at minor speed cost.
  // Input:
  // thres     - Threshold: minimum value above pedestal
  // pedestal  - Pedestal:  Use this as pedestal. If <=0 then compute pedestal from first N_PEDESTAL (4) samples.
  // NSB       - Number of samples before.
  // NSA       - Number of samples after.
  // npeak     - Max number of peaks to find. Set to 0 for all peaks.
  // gain      - Energy = integral*gain. So this can be stored in the hits.

  vector<FADC_Hit_t> out;
  ComputeMode7fast(out,thres,pedestal,NSB,NSA,npeak,gain);
  return(out);
}

const vector<FADC_Hit_t> &FADCdata::ComputeMode7fast(vector<FADC_Hit_t> &in, float thres, float pedestal, int NSB,int NSA, int npeak,float gain){
  // Return integral,time and pedestal as Mode7 computes it.
  // Input:
  // in        - vector<FADC_Hit_t> object to be filled. (Faster so that no copy is required.)
  // thres     - Threshold: minimum value above pedestal
  // pedestal  - Pedestal:  Use this as pedestal. If <=0 then compute pedestal from first N_PEDESTAL (4) samples.
  // NSB       - Number of samples before.
  // NSA       - Number of samples after.
  // npeak     - Max number of peaks to find. Set to 0 for all peaks.
  // gain      - Energy = integral*gain. So this can be stored in the hits.
  int min=0;
  vector<int> threshold_crossings;
  in.clear();
  if(NSA<=0){
    cout << "ERROR - cannot integrate with NSA=0 ";
    return(in);
  }
  
  if(pedestal<=0){   // No pedestal provided, so compute one from the first 4 samples.
    unsigned int pedest=0;
    for(int i=0;i< N_PEDESTAL;++i){
      pedest+=samples[i];
    }
    min = pedest;
    pedestal   = ((float)pedest)/N_PEDESTAL;
  }
  
  // Search for all theshold crossings.
  if(samples[0]>thres+pedestal){ // Special case, first sample is above threshold.
    threshold_crossings.push_back(0);
  }
  
  for(int i=1;i<samples.size();++i){
    if(samples[i]>thres+pedestal && samples[i-1]<thres+pedestal) { // we just crossed the threshold at i.
      threshold_crossings.push_back(i);
      i += NSA/4-1; // Wind forward to the end of the integration window.
      if(npeak>0 && threshold_crossings.size()>=npeak) break;  // No need to find more than asked for.
    }
  }
  
  for(int cross: threshold_crossings){
    int max=0;
    int max_loc = -1;
    int adc=0;
    int start = ((cross-NSB/4)>=0?(cross-NSB/4):0);
    int stop  = ((cross+NSA/4)<samples.size()?(cross+NSA/4):samples.size());
    
    for(int i=start;i<stop;++i){ // From NSB to NSA around crossing, but guard the edges.
      adc += samples[i];
      if(max<samples[i]){
        max=samples[i];
        max_loc=i;
      }
    }
    float time_thr = cross*4.;
    float time_fine=time_thr;
//    float pedsum2 =(stop-start)*pedestal;
    float integral = adc - (stop-start)*pedestal;
    float half_max = (max + pedestal)/2.;
    for(int i=start;i<stop;++i){
      if(samples[i+1]>half_max && samples[i]<half_max){
        time_fine = (i + (half_max - samples[i])/(samples[i+1]-samples[i]) )*4.;
        break;
      }
    }
//    printf("FADC: inegral: %8.3f %8.2f =%8d -%8.2f gain=%8.3f max=%d time: %8.2f %8.2f\n",integral*gain,integral,adc,pedsum2,gain,max,time_thr,time_fine);
    in.emplace_back(integral,time_fine,time_thr,pedestal,max,max_loc,integral*gain);
  }
  
  return(in);
}

FADC_Hit_t FADCdata::PulseFit(float width){
// return integral and time and pedestal using a 3-pole fit.
  return( FADC_Hit_t(0.,0.,0.,0,0));
}

ClassImp(FADC_Hit_t);
ClassImp(FADCdata);
ClassImp(Leaf<FADCdata>);
