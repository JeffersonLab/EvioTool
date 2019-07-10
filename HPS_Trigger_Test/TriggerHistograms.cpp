//
//  TriggerHistograms.cpp
//  HPS_Trigger_test
//
//  Created by Maurik Holtrop on 7/9/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#include "TriggerHistograms.h"

void TriggerHistograms::Fill(void){
  
  event_hist->Fill((double)etool->head->GetEventNumber());

  
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
  
  if(etool->ECAL->GTPClusters.size() == 1){  // Single cluster!
    EcalGTPCluster &cluster =etool->ECAL->GTPClusters[0];
    single_seeds->Fill(cluster.seed_ixy.first,cluster.seed_ixy.second);
    // find the seed hit.
    auto seed_hit=etool->ECAL->hitmap.find(cluster.seed_ixy);
    if(seed_hit == etool->ECAL->hitmap.end() ) cout << "Problem! Seed hit not in map. \n";
    single_seed_e->Fill(etool->ECAL->hitmap[cluster.seed_ixy].hits[cluster.seed_idx].energy);
    single_cluster_e->Fill(cluster.energy);
  }
  
}

