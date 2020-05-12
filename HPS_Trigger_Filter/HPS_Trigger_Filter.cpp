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

#include <cstring>
#include <chrono>
#include <bitset>

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

  if(args.debug<=2){
    etool->fDebug = 0b000000;
  }else if(args.debug==3){
    etool->fDebug = 0b000001;
  }else if(args.debug==4){
    etool->fDebug = 0b000011;
  } else if(args.debug == 5){
    etool->fDebug = 0b000111;
  } else{
    etool->fDebug = 0xFF;
  }
  /* open evio output file */
  int status=0;
  int output_handle=0;

  string outfile;
  if(args.output_name.size()>1){
    outfile = args.output_name;
  }else{
    string inf=args.filenames[0];
    outfile =   inf.substr(0,inf.find(".evio"))+"_FEE.evio";
  }
  
#define NumTrigBits 21
  vector<long> Trigger_counts(NumTrigBits,0);   // Vector with NumTrigBits zeros.
  vector<int>  FEE_zone_counts(7,0);
  long fb_pulser=0;
  long no_trig=0;

  if( !args.nooutput){
    if((status=evOpen(const_cast<char *>(outfile.c_str()),const_cast<char *>("w"),&output_handle))!=0) {
      cout << "Unable to open output file " << outfile << " status=" << status << endl;
      exit(EXIT_FAILURE);
    }
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
  }else if( args.trigger_name == "pulser" ){
    trigger_setting.bits.Pulser = true;
  }else{
    try{
      size_t loc=0;
      if( (loc = args.trigger_name.find("x")) != -1 ){   // HEX pattern
        trigger_setting.intval = stoi(args.trigger_name.substr(loc+1),0,16);
      }else if( (loc = args.trigger_name.find("b")) != -1 ){ // Binary pattern
        trigger_setting.intval = stoi(args.trigger_name.substr(loc+1),0,2);
      }else{
        trigger_setting.intval = stoi(args.trigger_name);
      }
    }catch(std::invalid_argument &e){
      cout << "Unknown trigger specifier to -T argument: " << args.trigger_name << "\n";
      cout << "Please specify one of: \n";
      cout << " 'FEE'       - FEE either top or bottom \n";
      cout << " 'FEE_Top'  - FEE top only. \n";
      cout << " 'FEE_Bot'  - FEE bottom only\n";
      cout << " 'muon'    - Pair3 mu+mu- trigger\n";
      cout << " '2gamma'   - Multiplicity-0 or 2 photon trigger. \n";
      cout << " '3gamma'   - Multiplicity-1 or 3 photon trigger. \n";
      cout << " 'pulser'   - Pulser trigger bit. \n";
      cout << " '######'   - Where ##### is an integer value (int, hex, bin) whose bits represent the trigger you want.\n";
      return(1);
    }
  }
    
  unsigned int filt_int = trigger_setting.intval;
  bitset<32> bit_pattern(filt_int);
  if(!args.quiet) printf("Filter integer is: %6d  = 0x%04X = 0b%s\n",filt_int,filt_int,bit_pattern.to_string().c_str());
  
  for(auto file: args.filenames){
    etool->Open(file.c_str());
    while(etool->Next() == S_SUCCESS){
      evt_count++;
      if(!args.quiet && evt_count>=50000 ){
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
      if( (!args.exclusive && etool->Trigger->IsTrigger(trigger_setting)) ||
         ( args.exclusive && etool->Trigger->IsExactTrigger(trigger_setting))
         ){
        if(args.debug >=2){
          bitset<32> trig_pattern(etool->Trigger->GetTriggerInt());
          cout << "Trigger: " << trig_pattern.to_string() << " Filter: " << bit_pattern.to_string() << endl;
        }
        if( !args.nooutput){
          status=evWrite(output_handle, etool->GetBufPtr());
          if(status!=0) {
            cout << "evWrite error output file "<<outfile << " status= " << status;
            exit(EXIT_FAILURE);
          }
          totalOut++;
        }
      }
      if( args.analyze ){ // Analyze the trigger bits.
        unsigned int trigbits = etool->Trigger->GetTriggerInt();
        for(int i=0;i<NumTrigBits;i++){
          if( trigbits & (1<<i) ) Trigger_counts[i]++;
        }
        if( etool->Trigger->GetExtTriggerInt() == 0x8000 ) fb_pulser ++;
        if(trigbits == 0 && etool->Trigger->GetExtTriggerInt() == 0) no_trig++;
        if( etool->Trigger->IsFEE()){
          for( int j=0; j<etool->VtpTop->feetrigger.size(); j++){
            int region_bits = etool->VtpTop->feetrigger[j].region;
            for( int k=0 ; k<7; k++){
              if( region_bits & (1<<k) ) FEE_zone_counts[k]++;
            }
          }
        }
      }
    } // event loop.
    etool->Close();  // Close the INPUT file.
  } // file list loop.
  
  evClose(output_handle); // Close the OUTPUT file.

  auto time2 = std::chrono::system_clock::now();
  std::chrono::microseconds delta_t = std::chrono::duration_cast<std::chrono::microseconds>(time2-time1);
  totalTime += delta_t;
  totalCount += evt_count;
  double avgRate = 1000000.0 * ((double) totalCount) / totalTime.count();
  printf("Last event:           %9d\n",etool->Head->GetEventNumber());
  printf("Total events:         %9ld \n",totalCount);
  printf("Total events written: %9ld \n",totalOut);
  printf("Final: %3.4g kHz \n", avgRate/1000.);
  if(args.analyze){
    cout << "\n";
    
    printf("Bit   Trigger_Name     Number   Fraction\n");
    unsigned long total_trig=0;
    for(int i=0;i<NumTrigBits;i++) total_trig += Trigger_counts[i];
    total_trig += fb_pulser;
    printf("%2d  %14s %10ld  %5.2f%%\n",-1,"FB Pulser",fb_pulser,(100.*(double)fb_pulser/total_trig));
    for(int i=0;i<NumTrigBits;i++){
      if(Trigger_counts[i]>0){
        string name=etool->Trigger->GetTriggerName(i);
        printf("%2d  %14s %10ld  %5.2f%%\n",i,name.c_str(),Trigger_counts[i],(100.*(double)Trigger_counts[i]/total_trig));
      }
    }
    printf("Total Triggers     %10ld\n",total_trig);
    printf("Total no-triggers  %10ld\n",no_trig);

    cout << "\nFEE Zones : \n";
    unsigned long fee_count = Trigger_counts[TSBank::TriggerNames["FEE_Top"]]+Trigger_counts[TSBank::TriggerNames["FEE_Bot"]];
    for(int k=0; k<7; k++){
      printf("region %d   %10d  %5.2f%%\n",k,FEE_zone_counts[k],
             100.*(double)FEE_zone_counts[k]/fee_count);
    }
  }
  return 0;
}
