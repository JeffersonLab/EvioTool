//
//  main.cpp
//  EvioTool_Test
//
//  Created by Maurik Holtrop on 12/29/18.
//  Copyright © 2018 UNH. All rights reserved.
//
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
#include "TH1D.h"
#include "TH2F.h"
#include "Arguments.h"

void Print_Usage(const char *name);
void Parse_Args(int *argc, const char **argv, Arguments_t *p_arg);

int main(int argc, const char * argv[])
{
  Arguments_t args;
  args.Parse_Args(&argc,argv);
  
  HPSEvioReader *etool=new HPSEvioReader(args.filenames[0].c_str());
//  etool->SVT->fStoreRaw=true;
  
  if(args.trigger_config_file.size()>2){
    etool->TrigConf->Parse_trigger_file(args.trigger_config_file); // If requested, parse the Trigger config file supplied.
    etool->ECAL->Config();
    cout << "Parsed trigger file: " << args.trigger_config_file << endl;
  }
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

  // Determine which data set this data belongs to.
  // To do so, read events until a normal physics event is encountered and then read the trigger bank (tag=46) which contains a
  // header bank 57615 that contains the run number.
  int run_number;
  {
    bool found = false;
    while( !found &&   etool->Next()== S_SUCCESS){
      if( (etool->this_tag & 128) == 128){
        found = true;
        run_number = etool->TrigHead->GetRunNumber();
        break;
      }
    }
    if( found == false) std::cout << "WARNING -- Not able to find a bank with a runnumber! \n";
    etool->Close();
  }

  if(run_number < 8000){ // This is 2015 or 2016 data.
    etool->SVT->Set2016Data();
  }else{
    etool->SVT->Set2019Data();
  }
  
  
  TFile root_file("EvioTool_out.root","RECREATE");
  TH1D *event_hist = new TH1D("event_hist","Events Histogram",1000,0,100000000);
  int ecal_nx=23;
  int ecal_ny=5;
  
  TH2F *ecal_hits =new TH2F("ecal_hits","Ecal Hits",(ecal_nx+1)*2+1,-ecal_nx-1.5,ecal_nx+2-0.5,(ecal_ny+1)*2+1,-ecal_ny-1.5,ecal_ny+1.5);
  TH1F *ecal_hit_e = new TH1F("ecal_hit_e","Ecal Hits Energy",500,0.,5000.);
  TH1F *ecal_hit_m = new TH1F("ecal_hit_m","Ecal Hits max adc",500,0.,5000.);
  
  TH2F *ecal_seeds =new TH2F("ecal_seeds","Ecal Cluster Seed Hits",(ecal_nx+1)*2+1,-ecal_nx-1.5,ecal_nx+2-0.5,(ecal_ny+1)*2+1,-ecal_ny-1.5,ecal_ny+1.5);
  TH1F *ecal_seed_e = new TH1F("ecal_seed_e","Ecal Seed Energy",500,0.,5000.);
  TH1F *ecal_cluster_e = new TH1F("ecal_cluster_e","Ecal Cluster Energy",500,0.,5000.);
  
  TH1F *svt_eventsize = new TH1F("svt_eventsize","SVT Number of Hits",5000,-0.5,4999.5);
  TH1F *svt_big_event_dist = new TH1F("svt_big_event_dist","Big SVT Event Distribution;evt num",87501,-0.5,350000.5);
  etool->fAutoAdd = args.auto_add;
  
  cout << "Debug set to " << etool->fDebug << " Auto add = " << etool->fAutoAdd << endl;
  
  etool->PrintBank(5);

  long evt_count=0;
  long totalCount=0;
  std::chrono::microseconds totalTime(0);

  auto start = std::chrono::system_clock::now();
  auto time1 = start;
  
  long big_svt_event=0;
  
  for(auto file: args.filenames){
    etool->Open(file.c_str());
    while(etool->Next() == S_SUCCESS){
      if(args.debug) cout<<"EVIO Event " << evt_count << endl;
//      etool->VtpTop->ParseBank();
//      etool->VtpBot->ParseBank();
      evt_count++;
//      cout << "Event:  " << etool->head->GetEventNumber() << "  seq: " << evt_count << endl;
      event_hist->Fill((double)etool->Head->GetEventNumber());
      if(args.print_evt) {
        etool->PrintBank(10);
  //      if(args.show_head) {};
  //      if(args.show_svt)  {};
  //      if(args.show_ecal) {};
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
      
      for(auto ecal_hit: etool->ECAL->hitmap){
        for(int i=0;i<ecal_hit.second.hits.size();++i){
          ecal_hit_e->Fill(ecal_hit.second.hits[i].energy);
          ecal_hit_m->Fill(ecal_hit.second.hits[i].max_adc);
          if(ecal_hit.second.hits[i].energy>100.){
            ecal_hits->Fill(ecal_hit.second.get_ix(),ecal_hit.second.get_iy());
          }
        }
        
      }
      
      for(auto cluster: etool->ECAL->GTPClusters ){
        ecal_seeds->Fill(cluster.seed_ixy.first,cluster.seed_ixy.second);
        // find the seed hit.
        auto seed_hit=etool->ECAL->hitmap.find(cluster.seed_ixy);
        if(seed_hit == etool->ECAL->hitmap.end() ) cout << "Problem! Seed hit not in map. \n";
        ecal_seed_e->Fill(etool->ECAL->hitmap[cluster.seed_ixy].hits[cluster.seed_idx].energy);
        ecal_cluster_e->Fill(cluster.energy);
      }
      
      if(etool->SVT ) svt_eventsize->Fill(etool->SVT->size());
      if(etool->SVT->size()>1500){
        svt_big_event_dist->Fill(etool->Head->GetEventNumber());
        big_svt_event++;
      }
    }
    totalCount += evt_count;
    cout << " ------------- \n";
    cout << "Out of " << totalCount << " events there were " << big_svt_event << " large SVT events\n";
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

