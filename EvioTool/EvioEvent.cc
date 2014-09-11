//
//  EvioEvent.cpp
//  EvioTool
//
//  Created by Maurik Holtrop on 5/7/14.
//  Copyright (c) 2014 UNH. All rights reserved.
//

#include "EvioEvent.h"

void EvioEventInit(EVIO_Event_t *evt){
  // Clear the data stores of the current event.
  evt->run_number=0;
  evt->start_time=0;
  evt->file_number=0;
  evt->event_number=0;
  evt->event_type=0;
  evt->or_bits=0;
  evt->top_bits=0;
  evt->bottom_bits=0;
  evt->pair_bits=0;
  evt->trig_time=0;
  evt->FADC_13.clear();
  evt->FADC_13.reserve(MAX_NUM_FADC);
  evt->FADC_15.clear();
  evt->FADC_15.reserve(MAX_NUM_FADC);
  for(int i=0;i<MAX_NUM_SVT_FPGA;++i){
    evt->SVT[i].fpga=0;
    evt->SVT[i].trigger=0;
    for(int j=0;j<NUM_FPGA_TEMPS;++j){
      evt->SVT[i].temps[j]=0;
    }
//    evt->SVT[i].data.clear();
//    evt->SVT[i].data.reserve(MAX_SVT_DATA);
  }
//  memset(evt->SVT,0,MAX_NUM_SVT_FPGA*sizeof(SVT_FPGA_t));
  evt->SVT_data.clear();
  evt->SVT_data.reserve(MAX_SVT_DATA);

}

void EvioEventClear(EVIO_Event_t *evt){
// Clear the data stores of the current event.
//  evt->run_number=0;   --- These are not cleared, they are retained since they are only read during special event types.
//  evt->start_time=0;
//  evt->file_number=0;
  evt->event_number=0;
  evt->event_type=0;
  evt->or_bits=0;
  evt->top_bits=0;
  evt->bottom_bits=0;
  evt->pair_bits=0;
  evt->trig_time=0;
  evt->FADC_13.clear();
  evt->FADC_15.clear();
  for(int i=0;i<MAX_NUM_SVT_FPGA;++i){
    evt->SVT[i].fpga=0;
    evt->SVT[i].trigger=0;
    for(int j=0;j<NUM_FPGA_TEMPS;++j){
      evt->SVT[i].temps[j]=0;
    }
//    evt->SVT[i].data.clear();
  }
//  memset(evt->SVT,0,MAX_NUM_SVT_FPGA*sizeof(SVT_FPGA_t));
  evt->SVT_data.clear();
//  evt->SVT.clear();
}

void EvioEventPrint(EVIO_Event_t *evt, int level){
  // Print the contents of the event to the screen for debugging.
  
  printf("Run: %5d, event: %6d,  type: %2d \n",evt->run_number,evt->event_number,evt->event_type);
  printf("FADC hits: %3d \n",(int)(evt->FADC_13.size()>evt->FADC_15.size()?evt->FADC_13.size():evt->FADC_15.size()) );
  
}
