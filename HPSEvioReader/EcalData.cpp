//
//  EcalBank.cpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/25/19.
//

#include "EcalData.h"

void EcalData::Parse(void){
  // Parse the FADC data that belongs to the ECAL.
  // Fit the RAW mode1 data for each channel of the ECAL, and sort the resulting hits into a hitmap, which stores
  // the hits by (x,y) pair.
  
  hitmap.clear();
  vector<FADC_Hit_t> hits;
  hits.reserve(2);
  for(int fadc_idx=0;fadc_idx < FADC_leaf->data.size();++fadc_idx){
    FADCdata &fadc =FADC_leaf->data[fadc_idx];                                 // REFERENCE, not a copy! for convenience
    FADC250_slot_t *slot = DAQconfig->GetSlot(fadc.GetCrate(),fadc.GetSlot());
    if(slot == nullptr || slot->subsystem[fadc.GetChan()] !=2 ) continue;       // Only use slots,chan for ECAL
    int ix =slot->ix[fadc.GetChan()];
    int iy =slot->iy[fadc.GetChan()];
    fadc.ComputeMode7fast(hits,slot->threshold[fadc.GetChan()],slot->pedestal[fadc.GetChan()],slot->NSB,slot->NSA,slot->npeak,slot->gain[fadc.GetChan()]);
    EcalHit_t ecal_hit(hits,fadc_idx,ix,iy);
    //hitmap.at(ix,iy)=ecal_hit;
    hitmap[{ix,iy}]=ecal_hit;
  }
}

void EcalData::FindGTPClusters(void){
  // Scan the hits in the hitmap for GTPClusters, 3x3 clusters with a max hit in the center.
  //
  
  // For each hit in the hitmap, check if:
  //    1:  The energy of the hit is above the seed threshold.
  //    2:  None of the surrounding hits in a 3x3 square that are inside the ECAL_CLUSTER_HIT_DT time window has larger energy.
  // If both conditions are satisfied, store the cluster.
  
  // Simplest algorithm has cost NxN hits. Could probably be improved to N log(N) ??
  // For instance, sort the hits by energy.
  
  GTPClusters.clear();  // Clear out the previous clusters.
  
  for(auto seed: hitmap){
    //    cout << "Checking hit ("<<ix<<","<<iy<<")\n";
    for(int seed_i = 0;seed_i< seed.second.hits.size(); ++seed_i){
      float energy = seed.second.hits[seed_i].energy;
      if(energy < cluster_seed_thresh) continue;
      bool cluster_seed_ok = true;
      EcalGTPCluster this_cluster(seed.second,0);   // Maybe we have a cluster.
      auto begin = block_iterator(seed.first).begin();
      auto end   = block_iterator(seed.first).end();
      for(auto test_point =begin; test_point != end && cluster_seed_ok; ++test_point ){ // Check the 8 points around this seed.
        auto testh = hitmap.find(*test_point);           // Check if there is a hit there.
        if( testh != hitmap.end()){
          for(int hit_i=0;hit_i<testh->second.hits.size(); ++hit_i){
            if( testh->second.hits[hit_i].energy > seed.second.hits[seed_i].energy ){
              cluster_seed_ok = false;
              break;  // Wasn't the seed, another hit has more energy. Break the hit loop.
            }else if( abs(testh->second.hits[hit_i].time - seed.second.hits[seed_i].time) > cluster_hit_dt ){
              break; // The seed may be OK, but this hit does not belong.
            }
            this_cluster.AddHit(testh->second,hit_i);
          }
        }
      } // We tried all test_point hits in the 9x9 square. If we get here, it is a cluster.
      GTPClusters.push_back(this_cluster);
    }
  }
}

ClassImp(EcalData);
