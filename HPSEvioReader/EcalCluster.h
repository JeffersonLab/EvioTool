//
//  EcalCluster.h
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/27/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#ifndef EcalCluster_h
#define EcalCluster_h

#include "EvioTool.h"
#include "Cluster.h"

class EcalCluster: public Cluster {

public:
  EcalCluster(){};
  
  
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(EcalCluster,1);
#pragma clang diagnostic pop
};

#endif /* EcalGTPCluster_h */
