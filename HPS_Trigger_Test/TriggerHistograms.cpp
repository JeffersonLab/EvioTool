//
//  TriggerHistograms.cpp
//  HPS_Trigger_test
//
//  Created by Maurik Holtrop on 7/9/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#include "TriggerHistograms.h"

void TriggerHistograms::Fill(void){
  
  event_hist->Fill((double)etool->Head->GetEventNumber());

  unsigned int trigbits            = etool->Trigger->GetTriggerInt();
  unsigned int trigbits_noprescale = etool->Trigger->GetTriggerInt(false);
  for(int i=0;i<32;++i){
    int mask = (1<<i);
    if( trigbits&mask ){
      trig_bits->Fill(i);
    }
    if( trigbits_noprescale&mask ){
      no_pre_trig_bits->Fill(i);
    }
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
    
    if(cluster.InFiducial()){
      fiducial_seeds->Fill(cluster.seed_ixy.first,cluster.seed_ixy.second);
      fiducial_seed_e->Fill(etool->ECAL->hitmap[cluster.seed_ixy].hits[cluster.seed_idx].energy);
      fiducial_cluster_e->Fill(cluster.energy);
    }
  }
  
  if(etool->ECAL->GTPClusters.size() == 1){  // Single cluster!
    EcalGTPCluster &cluster =etool->ECAL->GTPClusters[0];
    single_seeds->Fill(cluster.seed_ixy.first,cluster.seed_ixy.second);
    // find the seed hit.
    auto seed_hit=etool->ECAL->hitmap.find(cluster.seed_ixy);
    if(seed_hit == etool->ECAL->hitmap.end() ) cout << "Problem! Seed hit not in map. \n";
    single_seed_e->Fill(etool->ECAL->hitmap[cluster.seed_ixy].hits[cluster.seed_idx].energy);
    single_cluster_e->Fill(cluster.energy);
    if(cluster.InFiducial()){
      fiducial_seeds2->Fill(cluster.seed_ixy.first,cluster.seed_ixy.second);
      fiducial_seed_e2->Fill(etool->ECAL->hitmap[cluster.seed_ixy].hits[cluster.seed_idx].energy);
      fiducial_cluster_e2->Fill(cluster.energy);
    }

  }
  
}

