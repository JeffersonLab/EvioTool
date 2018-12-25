//
//  EvioEvent.cpp
//  EvioTool
//
//  Created by Maurik Holtrop on 5/7/14.
//  Copyright (c) 2014 UNH. All rights reserved.
//

#include "HPSEvent.h"

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
  evt->SVT_data.clear();
}

void EvioEventPrint(EVIO_Event_t *evt, int level){
  // Print the contents of the event to the screen for debugging.
  
  printf("Run: %5d, event: %6d, type: %4d, ",evt->run_number,evt->event_number,evt->event_type);
  printf("FADC_hits: %3d, ",(int)(evt->FADC_13.size()>evt->FADC_15.size()?evt->FADC_13.size():evt->FADC_15.size()) );
  printf("SVT_hits: %3d\n",(int)evt->SVT_data.size());
  if(level>0){
    EvioEventPrintSVT(evt,level-1);
    EvioEventPrintECAL(evt,level-1);
  }
}

void EvioEventPrintSVT(EVIO_Event_t *evt, int level){
    // Print the contents of the event to the screen for debugging.

  if(level&0x01){
    printf("##### Event %5d,%6d \n",evt->run_number,evt->event_number);
  }
  
  for(auto svt: evt->SVT_data){
      if(level&0x02){
        cout << " feb_id: " << svt.head.feb_id;
        cout << " hyb_id: " << svt.head.hyb_id;
        cout << " apv: " << svt.head.apv;
        cout << " chan: "<< svt.head.chan;
        cout << "[";
        for(int i=0;i<MAX_NUM_SVT_SAMPLES;++i) cout << svt.samples[i] << ",";
        cout << "]";
        }
      else{
        printf("[%2d,%2d,%2d,%4d,",svt.head.feb_id,svt.head.hyb_id,svt.head.apv,svt.head.chan);
        for(int i=0;i<MAX_NUM_SVT_SAMPLES;++i){
          printf("%5d",svt.samples[i]);
          if(i==(MAX_NUM_SVT_SAMPLES-1))printf("]\n");
          else                         printf(",");
      }
    }
  }
}

void EvioEventPrintECAL(EVIO_Event_t *evt, int level){
  // Print the contents of the event to the screen for debugging.
  
  if(level&0x01){
    printf("##### Event: %5d,%6d \n",evt->run_number,evt->event_number);
  }
  
  for(auto ecal: evt->FADC_13){
    for(int i=0;i<ecal.data.size();++i){
      printf("[%2d,%2d,%14llu,",ecal.crate,ecal.slot,ecal.time);
      printf("%3d,",ecal.data[i].chan);
      for(int k=0;k<  ecal.data[i].samples.size();++k){
        printf("%4d",ecal.data[i].samples[k]);
        if(k==( ecal.data[i].samples.size()-1))printf("]\n");
        else                         printf(",");
      }
    }
  }
}
