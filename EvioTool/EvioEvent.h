//
//  EvioEvent.h
//  EvioTool
//
//  Created by Maurik Holtrop on 5/7/14.
//  Copyright (c) 2014 UNH. All rights reserved.
//
// The main "event" is set of nested structures that have a either static arrays or vectors in them.
// This design is on purpose, it allows for very fast data access and can be streamed or memory shared fairly easily.
// With this design the decoder can run at over 100kHz on an i7 laptop.
// Using classes, or nested classes, quicky creates headaches like vtables that should not be shared or streamed.
//
// This file and the associated cc file contain some tools for Evio_Event_t.
// The EvioEvent class is a helper class to provided various useful services when dealing with the Evio_Event_t structure
//
#ifndef __EvioTool__EvioEvent__
#define __EvioTool__EvioEvent__

#include <stdio.h>
#include <iostream>
#include <vector>
using namespace std;

#define MAX_NUM_FADC   25        // Depending on the implementation detail, this may be a "reserve" and not a "max".
#define MAX_NUM_SVT_FPGA 7
#define MAX_NUM_SVT_SAMPLES 6
#define MAX_SVT_DATA 1024
#define NUM_FPGA_TEMPS 7

struct FADC_chan_f13_t{
  int chan;
  vector<unsigned short> samples;
};

struct FADC_chan_f15_t{
  int chan;
  vector<short> time;
  vector<int> adc;
};

struct FADC_data_f13_t {
  int  crate;
  int  slot;
  int  trig;
  int  time;
  vector<FADC_chan_f13_t> data;
};

struct FADC_data_f15_t {
  int  crate;
  int  slot;
  int  trig;
  int  time;
  vector<FADC_chan_f15_t> data;
};

struct SVT_chan_t{
  int fpga;
  int chan;
  int apv;
  int hybrid;
  int samples[MAX_NUM_SVT_SAMPLES];
  //  vector<int> samples;
};

struct SVT_FPGA_t{
  int fpga;
  int trigger;
  //  vector<int> temps;
  unsigned int temps[NUM_FPGA_TEMPS];
  //  vector<SVT_chan_t> data;
  //  SVT_chan_t data[MAX_SVT_DATA];
};

struct EVIO_Event_t{
  // The data from the EVIO file header is stored here:
  unsigned int run_number;
  unsigned int start_time;
  unsigned int file_number;
  
  // The data from the EVIO EVENT header is stored here:
  unsigned int topnode_tag;
  unsigned int event_number;
  unsigned int event_type;
  
  // Trigger Bank information
  
  unsigned int or_bits;
  unsigned int top_bits;
  unsigned int bottom_bits;
  unsigned int pair_bits;
  unsigned int trig_time;
  
  // FADC data encountered:
  
  vector<FADC_data_f13_t> FADC_13;  // Mode 13 - Nsamples.
  vector<FADC_data_f15_t> FADC_15;  // Mode 15 - Integrated.
  SVT_FPGA_t SVT[MAX_NUM_SVT_FPGA];           // SVT Crate data
  vector<SVT_chan_t> SVT_data;

};

// This keeps the root interpreter happy when dealing with these structures.

#ifndef __CINT__
template class std::vector<FADC_chan_f13_t>;
template class std::vector<FADC_chan_f15_t>;
template class std::vector<FADC_data_f13_t>;
template class std::vector<FADC_data_f15_t>;
template class std::vector<SVT_chan_t>;
template class std::vector<SVT_FPGA_t>;
#endif

void EvioEventClear(EVIO_Event_t *evt);
void EvioEventInit(EVIO_Event_t *evt);
void EvioEventPrint(EVIO_Event_t *evt, int level=0);


#endif /* defined(__EvioTool__EvioEvent__) */
