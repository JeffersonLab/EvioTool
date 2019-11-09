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
  
  //HPSEvioReader *etool=new HPSEvioReader(args.filenames[0].c_str());
  HPSEvioReader *etool=new HPSEvioReader();
  etool->Open(args.filenames[0].c_str());
//  etool->SVT->fStoreRaw=true;
  etool->SVT->fSaveHeaders = true;  // Slows things down, but read the SVT headers into storage as well.
  
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
  unsigned long trigtime_start=0;
  {
    bool found = false;
    while( !found &&   etool->Next()== S_SUCCESS){
      if( (etool->this_tag & 128) == 128){
        found = true;
        run_number = etool->GetRunNumber();
        trigtime_start = etool->GetTrigTime();
        break;
      }
    }
    if( found == false) std::cout << "WARNING -- Not able to find a bank with a runnumber! \n";
    etool->Close();
  }

  if(etool->SVT){
    if(run_number < 8000){ // This is 2015 or 2016 data.
      etool->Set2016Data();
    }else{
      etool->Set2019Data();
    }
  }else{
    cout << "NO SVT initialized \n";
  }
  
  TFile root_file(args.output_name.c_str(),"RECREATE");
  TH1I *event_hist = new TH1I("event_hist","Events Histogram",1000,0,100000000);
  TH1D *trigtime_hist = new TH1D("trigtime_hist","Trigger time relative to event 1",10000,0,3.E12); // 3*10^12 ns = 3000 s = 50min.
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
  TH1F *svt_big_since_dist0 = new TH1F("svt_big_since_dist0","Big SVT Event Distribution since Single_2_Top;evt num",1001,-0.5,1000.5);
  TH1F *svt_big_since_dist1 = new TH1F("svt_big_since_dist1","Big SVT Event Distribution since Single_2_Bot;evt num",1001,-0.5,1000.5);
  TH1F *svt_big_since_dist2 = new TH1F("svt_big_since_dist2","Big SVT Event Distribution since Pulser;evt num",1001,-0.5,1000.5);
  TH1F *evt_since_last_pulser = new TH1F("evt_since_last_pulser","Num events since last pulser;evt num",1001,-0.5,1000.5);
  TH1F *evt_since_last_monster = new TH1F("evt_since_last_monster","Num events since last monster;evt num",100,0,10000-100);
  TH1D *monster_time = new TH1D("monster_time","Monster trig time relative to event 1;time (ns)",10000,0,3.E12); // 3*10^12 ns = 3000 s = 50min.
  TH1D *monster_dt   = new TH1D("monster_dt","Time since last monster;time (ns)",1000,0,1.E9);
  TH1D *monster_dt2   = new TH1D("monster_dt2","Time since last event for monsters;time (ns)",1000,0,1.E5);
  TH1D *event_dt   = new TH1D("event_dt","Time since last event;time (ns)",1000,0,1.E5);
  TH1D *monster_dt3   = new TH1D("monster_dt3","Time to next event for monsters;time (ns)",1000,0,1.E5);
  TH1I *last_event_type = new TH1I("last_event_type","Last event type before monster",33,-0.5,32.5);
  
  etool->fAutoAdd = args.auto_add;
  
  cout << "Debug set to " << etool->fDebug << " Auto add = " << etool->fAutoAdd << endl;
  
  etool->PrintBank(5);

  unsigned long evt_count=0;
  unsigned long totalCount=0;
  
  std::chrono::microseconds totalTime(0);

  auto start = std::chrono::system_clock::now();
  auto time1 = start;
  
  long big_svt_event=0;
  int  big_svt_since[3]={0,0,0};
  int n_events_since_last_pulser=0;
  int n_events_since_last_monster=0;
  long time_of_last_monster = etool->GetTrigTime();
  long time_of_last_event=0;
  bool last_event_was_monster=false;
  unsigned int last_event_trigger_bits;
  
  for(auto file: args.filenames){
    etool->Open(file.c_str());
    while(etool->Next() == S_SUCCESS){
      if( (etool->this_tag & 128) != 128) continue;
      if(args.debug) cout<<"EVIO Event " << evt_count << endl;
//      etool->VtpTop->ParseBank();
//      etool->VtpBot->ParseBank();
      evt_count++;
//      cout << "Event:  " << etool->head->GetEventNumber() << "  seq: " << evt_count << endl;
      event_hist->Fill((double)etool->Head->GetEventNumber());
      trigtime_hist->Fill((double)(etool->GetTrigTime()-trigtime_start));
      
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
      
      TSBank::TriggerBits tstrig = etool->Trigger->GetTriggerBits();
      if(etool->SVT ){
        svt_eventsize->Fill(etool->SVT->size());

        if(last_event_was_monster){
          monster_dt3->Fill((double)(etool->GetTrigTime()-time_of_last_monster));
          for(int i=0;i<32;++i){
            if(last_event_trigger_bits & (1<<i)) last_event_type->Fill(i);
          }
          last_event_was_monster=false;
        }

        if(etool->SVT->size()>1500){             // Monster SVT Events.
          svt_big_event_dist->Fill(etool->Head->GetEventNumber());
          evt_since_last_monster->Fill(n_events_since_last_monster);
          n_events_since_last_monster=0;
          big_svt_event++;
          svt_big_since_dist0->Fill(big_svt_since[0]);
          svt_big_since_dist1->Fill(big_svt_since[1]);
          svt_big_since_dist2->Fill(big_svt_since[2]);
          for(int i=0;i<3;++i)  big_svt_since[i]=0;
          
          monster_time->Fill((double)(etool->GetTrigTime()-trigtime_start));
          monster_dt->Fill((double)(etool->GetTrigTime()-time_of_last_monster));
          monster_dt2->Fill((double)(etool->GetTrigTime()-time_of_last_event));
          time_of_last_monster = etool->GetTrigTime();
          last_event_was_monster=true;
        }
        
        if(tstrig.bits.Single_2_Top) big_svt_since[0] = 0;
        if(tstrig.bits.Single_2_Bot) big_svt_since[1] = 0;
        if(tstrig.bits.Pulser){
          evt_since_last_pulser->Fill(n_events_since_last_pulser);
          big_svt_since[2] = 0;
          n_events_since_last_pulser = 0;
        }
        for(int i=0;i<3;++i)  big_svt_since[i]++;
        n_events_since_last_pulser++;
        n_events_since_last_monster++;
      }
      
 //     printf("Event : %9d  Time: %16ld ----------------------------------- \n",etool->GetEventNumber(),etool->GetTrigTime()-trigtime_start);
      if(etool->SVT->fSaveHeaders){
        for(int i=0;i<etool->SVT->svt_tails.size(); ++i){
          if(etool->SVT->svt_tails[i].apv_sync_error ) cout << "APV Sync ERROR \n";
          if(etool->SVT->svt_tails[i].fifo_backup_error ) cout << "FIFO Backup ERROR \n";
          if(etool->SVT->svt_tails[i].skip_count >0 ) cout << "Skip count is set. \n";
          //          printf("Tail: %2d - Multi: %6d  Skip: %3d \n",i,etool->SVT->svt_tails[i].num_multisamples,etool->SVT->svt_tails[i].skip_count);
        }
        for(int i=0;i<etool->SVT->svt_headers.size();++i){
          cout << "Header " << i << " time: " << etool->SVT->svt_headers[i].GetTimeStamp() << " trig: "<< etool->GetTrigTime()<< "  DT: " << ((long)etool->GetTrigTime() - (long)etool->SVT->svt_headers[i].GetTimeStamp() - (long)trigtime_start) << endl;
        }
      }
      event_dt->Fill((double)(etool->GetTrigTime()-time_of_last_event));
      time_of_last_event= etool->GetTrigTime();
      last_event_trigger_bits = etool->Trigger->GetTriggerInt();
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

