//
//  TriggerHistograms.hpp
//  HPS_Trigger_test
//
//  Created by Maurik Holtrop on 7/9/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#ifndef TriggerHistograms_h
#define TriggerHistograms_h

#include <stdio.h>
#include "TH1D.h"
#include "TH2F.h"
#include "HPSEvioReader.h"

class TriggerHistograms {
public:
  TH1D *event_hist = new TH1D("event_hist","Events Histogram",1000,0,100000000);
  int ecal_nx=23;
  int ecal_ny=5;
  
  TH2F *ecal_hits =new TH2F("ecal_hits","Ecal Hits",(ecal_nx+1)*2+1,-ecal_nx-1.5,ecal_nx+2-0.5,(ecal_ny+1)*2+1,-ecal_ny-1.5,ecal_ny+1.5);
  TH1F *ecal_hit_e = new TH1F("ecal_hit_e","Ecal Hits Energy",500,0.,5000.);
  TH1F *ecal_hit_m = new TH1F("ecal_hit_m","Ecal Hits max adc",500,0.,5000.);
  
  TH2F *ecal_seeds =new TH2F("ecal_seeds","Ecal Cluster Seed Hits",(ecal_nx+1)*2+1,-ecal_nx-1.5,ecal_nx+2-0.5,(ecal_ny+1)*2+1,-ecal_ny-1.5,ecal_ny+1.5);
  TH1F *ecal_seed_e = new TH1F("ecal_seed_e","Ecal Seed Energy",500,0.,5000.);
  TH1F *ecal_cluster_e = new TH1F("ecal_cluster_e","Ecal Cluster Energy",500,0.,5000.);
  
  TH2F *single_seeds =new TH2F("single_seeds","Single Ecal Cluster Seed Hits",(ecal_nx+1)*2+1,-ecal_nx-1.5,ecal_nx+2-0.5,(ecal_ny+1)*2+1,-ecal_ny-1.5,ecal_ny+1.5);
  TH1F *single_seed_e = new TH1F("single_seed_e","Single Ecal Seed Energy",500,0.,5000.);
  TH1F *single_cluster_e = new TH1F("single_cluster_e","Single Ecal Cluster Energy",500,0.,5000.);

  HPSEvioReader *etool;
  
public:
  TriggerHistograms(){};
  TriggerHistograms(HPSEvioReader *et): etool(et){};
  void Fill(void);
};

#endif /* TriggerHistograms_hpp */
