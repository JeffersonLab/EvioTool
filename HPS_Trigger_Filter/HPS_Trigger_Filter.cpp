//
//  HPS_FEE_Filter.cpp
//
//  Created by Maurik Holtrop on 8/1/19.
//  Copyright © 2019 UNH. All rights reserved.
//
#include <iostream>

#include "Arguments.h"
#include "HPSEvioReader.h"
#include "TSBank.h"
#include "evio.h"

int main(int argc, const char * argv[]) {

  Arguments_t args;
  args.Parse_Args(&argc,argv);
  
  HPSEvioReader *etool=new HPSEvioReader();
  
  // To speed up processing, we unlink the SVT and ECAL decoders, since we don't need them here.
  etool->RemoveBank(etool->SVT);
  etool->ECALCrate->RemoveBank(etool->ECAL);
  etool->RemoveBank(etool->ECALCrate);
  delete etool->ECAL;
  etool->ECAL=nullptr;
  
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
  /* open evio output file */
  int status;
  int output_handle;
  string outfile;
  if(args.output_name.size()>1){
    outfile = args.output_name;
  }else{
    string inf=args.filenames[0];
    outfile =   inf.substr(0,inf.find(".evio"))+"_FEE.evio";
  }
    
  if((status=evOpen(const_cast<char *>(outfile.c_str()),const_cast<char *>("w"),&output_handle))!=0) {
    cout << "Unable to open output file " << outfile << " status=" << status << endl;
    exit(EXIT_FAILURE);
  }

  long evt_count=0;
  long totalCount=0;
  long totalOut=0;
  std::chrono::microseconds totalTime(0);
  
  auto start = std::chrono::system_clock::now();
  auto time1 = start;
  
  TSBank::TriggerBits trigger_setting;
  if( args.trigger_name == "FEE"){
    trigger_setting.bits.FEE_Bot=true;
    trigger_setting.bits.FEE_Top=true;
  }else if( args.trigger_name == "FEE_Top"){
    trigger_setting.bits.FEE_Top=true;
  }else if( args.trigger_name == "FEE_Bot"){
    trigger_setting.bits.FEE_Bot=true;
  }else if( args.trigger_name == "muon"){
    trigger_setting.bits.Pair_3=true;
  }else if(args.trigger_name == "2gamma" || args.trigger_name == "Mult-0" || args.trigger_name == "Multiplicity-0" ){
    trigger_setting.bits.Mult_0 = true;
  }else if(args.trigger_name == "3gamma" || args.trigger_name == "Mult-1" || args.trigger_name == "Multiplicity-1" ){
    trigger_setting.bits.Mult_1 = true;
  }else{
    try{
      trigger_setting.intval = stoi(args.trigger_name);
    }catch(std::invalid_argument &e){
      cout << "Unknown trigger specifier to -T argument. Please specify one of: \n";
      cout << " 'FEE'       - FEE either top or bottom \n";
      cout << " 'FEE_Top'  - FEE top only. \n";
      cout << " 'FEE_Bot'  - FEE bottom only\n";
      cout << "  'muon'    - Pair3 mu+mu- trigger\n";
      cout << " '2gamma'   - Multiplicity-0 or 2 photon trigger. \n";
      cout << " '3gamma'   - Multiplicity-1 or 3 photon trigger. \n";
      cout << " '######'   - Where ##### is an integer value whole bits represent to trigger you want.\n";
      return(1);
    }
  }
    
    unsigned int filt_int = trigger_setting.intval;
  
  cout << "Filter integer is: " << filt_int << endl;
  
  for(auto file: args.filenames){
    etool->Open(file.c_str());
    while(etool->Next() == S_SUCCESS){
      evt_count++;
      if(!args.quiet && evt_count%50000 ==0 ){
        //      /* statistics */
        auto time2 = std::chrono::system_clock::now();
        std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
        totalTime += delta_t;
        double rate = 1000000.0 * ((double) evt_count) / delta_t.count();
        totalCount += evt_count;
        double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
        printf("%s: %6.1f kHz,  %6.1f kHz Avg. Event: %9d  Out: %9ld\n", argv[0], rate/1000., avgRate/1000.,etool->Head->GetEventNumber(),totalOut);
        evt_count = 0;
        time1 = std::chrono::system_clock::now();
      }
      if( etool->Trigger->IsTrigger(trigger_setting) ){
        if(args.debug >2){
          unsigned int *filt_int = (unsigned int *)(&trigger_setting);
          cout << "Trigger " << etool->Trigger->GetTriggerInt() << " filter " << *filt_int << endl;
        }
        status=evWrite(output_handle, etool->GetBufPtr());
        if(status!=0) {
          cout << "evWrite error output file "<<outfile << " status= " << status;
          exit(EXIT_FAILURE);
        }
        totalOut++;
      }
    }
    etool->Close();
  }
  auto time2 = std::chrono::system_clock::now();
  std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
  totalTime += delta_t;
  totalCount += evt_count;
  double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
  printf("Last event:           %9d\n",etool->Head->GetEventNumber());
  printf("Total events:         %9ld \n",totalCount);
  printf("Total events written: %9ld \n",totalOut);
  printf("Final: %3.4g kHz \n", avgRate/1000.);

  return 0;
}
