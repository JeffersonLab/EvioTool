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
  os << m.Class_Name() << "(" << (int)m.crate << "," << (int)m.slot << "," << (int)m.chan << ") [";
  for(int i=0; i< m.samples.size(); ++i){
    os << (int)m.samples[i];
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

std::tuple<float,float,float,int,int> FADCdata::ComputeMode3(float thres){
  // return integral and time and pedestal as Mode3 computes it.
  // The first 10 samples determine the pedestal.
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
  return( std::make_tuple(integral,time,pedestal,max,max_loc));
};

std::tuple<float,float,float,int,int> FADCdata::ComputeMode7(float thres, int NSB,int NSA){
  // Return integral,time and pedestal as Mode7 computes it.
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
  float pedestal   = ((float)pedest)/N_PEDESTAL;
  
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
  
  if(thres_loc<0) return( std::make_tuple(0.,0.,0.,0,0));
  
  float time = 0;
  if(samples[half_max_loc_p1] > samples[half_max_loc_p1-1]){
    float mid_line = samples[half_max_loc_p1] - samples[half_max_loc_p1-1];
    time = (((max+pedestal)/2-samples[half_max_loc_p1-1])/mid_line) + half_max_loc_p1-1;
  }

  float integral = sum - pedestal*(stop - start);
  return( std::make_tuple(integral,time,pedestal,max,max_loc));
}

std::tuple<float,float,float> FADCdata::PulseFit(float width){
// return integral and time and pedestal using a 3-pole fit.
  return( std::make_tuple(0.,0.,0.));
}


ClassImp(FADCdata);
ClassImp(Leaf<FADCdata>);
