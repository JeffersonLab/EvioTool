//
//  HPS_Trigger_Test.cpp
//  HPS_Trigger_test
//
//  Created by Maurik Holtrop on 7/9/19.
//  Copyright © 2019 UNH. All rights reserved.
//
#include <ratio>
#include <chrono>

#include <limits.h>

#include <iostream>
#include <string>
using namespace std;

#include "HPSEvioReader.h"
#include "SVTBank.h"
#include "Header.h"
#include "Headbank.h"

#include "TFile.h"
#include "TriggerHistograms.h"
#include "Arguments.h"

int main(int argc, const char * argv[])
{
  Arguments_t args;
  args.Parse_Args(&argc,argv);
  
  HPSEvioReader *etool=new HPSEvioReader();

  if(args.trigger_config_file.size()>2) etool->TrigConf->Parse_trigger_file(args.trigger_config_file); // If requested, parse the Trigger config file supplied.
  
  if(args.use_et){
    cout << "Error ET system not yet implemented. Exit. \n";
    exit(1);
  }
  
  if(args.debug==0){
    etool->fDebug = 0b000000;
  }else if(args.debug==1){
    etool->fDebug = 0b000001;
  }else if(args.debug==2){
    etool->fDebug = 0b000011;
  } else if(args.debug == 3){
    etool->fDebug = 0b000111;
  } else{
    etool->fDebug = 0xFF;
  }
  
  TFile root_file(args.output_name.c_str(),"RECREATE");
  
  TriggerHistograms hists(etool);
  
  std::cout << "Debug set to " << etool->fDebug << std::endl;
  
  long evt_count=0;
  long totalCount=0;
  std::chrono::microseconds totalTime(0);
  
  auto start = std::chrono::system_clock::now();
  auto time1 = start;
  
  for(auto file: args.filenames){
    etool->Open(file.c_str());
    while(etool->Next() == S_SUCCESS){
      if(args.debug) cout<<"EVIO Event " << evt_count << endl;
      evt_count++;
      if(args.print_evt) {
        etool->PrintBank(10);
       }
      if(!args.quiet && evt_count%50000 ==0 ){
        //      /* statistics */
        auto time2 = std::chrono::system_clock::now();
        std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
        totalTime += delta_t;
        double rate = 1000000.0 * ((double) evt_count) / delta_t.count();
        totalCount += evt_count;
        double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
        printf("%s: %6.1f kHz,  %6.1f kHz Avg. Event: %9d\n", argv[0], rate/1000., avgRate/1000.,etool->Head->GetEventNumber());
        evt_count = 0;
        time1 = std::chrono::system_clock::now();
      }
      
      hists.Fill(); // Fill the main histograms.

    }
    cout << " ------------- \n";
    etool->Close();
  }
  auto time2 = std::chrono::system_clock::now();
  std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
  totalTime += delta_t;
  totalCount += evt_count;
  double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
  printf("Last event: %6d\n",etool->Head->GetEventNumber());
  printf("Total events: %6ld \n",totalCount);
  printf("Final: %3.4g kHz \n", avgRate/1000.);
  root_file.Write();
  root_file.Close();
  return 0;
}

