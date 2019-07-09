//
//  Cluster.h
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/27/19.
//  Copyright © 2019 UNH. All rights reserved.
//
// Base class for cluters: EcalGTPCluster and EcalCluster.
//
#ifndef Cluster_h
#define Cluster_h

#include "EvioTool.h"
#include "EcalHit.h"

class Cluster: public TObject {
 
public:
  Ecal_point_t seed_ixy;         // The seed ix,iy point as stored in the hitmap.
  int          seed_idx=-1;      // Which hit at that point.
  vector<Ecal_point_t> hit_ixy;  // The ix,iy in the hitmap for the N hits.
  vector<int> hit_idx;           // Which hit at each of those points.
  
  float      energy=0;
  float      time=0;
  
public:
  Cluster(){};
  Cluster(const EcalHit_t &hit,const int hit_id){
    seed_ixy = hit.ixy;
    seed_idx = hit_id;
    energy = hit.hits[hit_id].energy;
    time   = hit.hits[hit_id].time;
  };
  
  void AddHit(const EcalHit_t &hit,const int hit_i){
    // Add the hit into this cluster.
    hit_ixy.push_back(hit.ixy);
    hit_idx.push_back(hit_i);
    energy += hit.hits[hit_i].energy;
  }
  
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(Cluster,1);
#pragma clang diagnostic pop
};

#endif /* Cluster_h */
