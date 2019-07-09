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
  
  
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(EcalGTPCluster,1);
#pragma clang diagnostic pop
};

#endif /* EcalGTPCluster_h */
