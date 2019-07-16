//
//  EcalGTPCluster.h
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/27/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#ifndef EcalGTPCluster_h
#define EcalGTPCluster_h

#include "EvioTool.h"
#include "Cluster.h"

class EcalGTPCluster: public Cluster {

public:
  EcalGTPCluster(){};
  EcalGTPCluster(const EcalHit_t &hit,const int hit_id): Cluster(hit,hit_id){};
  
  bool InFiducial(void){
    // Tests if this cluster's seed is in the simple fiducial region, defined by 1 crystal from the edge.
    if( abs(seed_ixy.second) <= 1 ) return(false);
    if( abs(seed_ixy.second) >= 5 ) return(false);
    if( abs(seed_ixy.first) >= 23 ) return(false);
    if( seed_ixy.first>= -11 && seed_ixy.first <= -1 && abs(seed_ixy.second) == 2) return(false);
    return(true);
  }
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(EcalGTPCluster,1);
#pragma clang diagnostic pop
};

#endif /* EcalGTPCluster_h */
