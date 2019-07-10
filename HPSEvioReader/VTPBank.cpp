//
//  VTPBank.cpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/19/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#include "VTPBank.h"

ClassImp(VTPBank);

void VTPBank::ParseBank(void){
  // First Clear out all the old data.
  Clear();
  for(int i=0; i<data.size(); ++i){
    if(data[i] & 1<<31) { // Type data set.
      int type = (data[i]>>27)&0x0F;
      int subtype;
      switch (type){
        case 0:  // Block Header
          ((unsigned int *)&BlockHeader)[0]=data[i];  // Copy the data onto the structure, old style.
          break;
        case 1: //  Block Tail
          ((unsigned int *)&BlockTail)[0]=data[i];
          break;
        case 2:  // Event Header
          ((unsigned int *)&EventHeader)[0]=data[i];
          break;
        case 3:  // Trigger time
          TrigTime = (data[i] & 0x00FFFFFF) + ((data[i+1]& 0x00FFFFFF )<<24);
          i++;
          break;
        case 12:  // Expansion type
          subtype = (data[i]>>23)&0x0F;
          switch(subtype){
            case 2: // HPS Cluster
              HPSCluster_t  clus;
              ((unsigned long *)&clus)[0]=((long)data[i] + (((long)data[i+1])<<32) );  // lotsa brackets!
              clusters.push_back(clus);
              break;
            case 3: // HPS Single Cluster
              HPSSingleTrig_t strig;
              ((unsigned long *)&strig)[0]=data[i];
              singletrigs.push_back(strig);
              break;
            case 4: // HPS Pair Trigger
              HPSPairTrig_t ptrig;
              ((unsigned long *)&ptrig)[0]=data[i];
              pairtrigs.push_back(ptrig);
              break;
            case 5: // HPS Calibration Trigger
              HPSCalibTrig_t ctrig;
              ((unsigned long *)&ctrig)[0]=data[i];
              calibtrigs.push_back(ctrig);
              break;
            case 6: // HPS Cluster Multiplicity Trigger
              HPSClusterMult_t clmul;
              ((unsigned long *)&clmul)[0]=data[i];
              clustermult.push_back(clmul);
              break;
            case 7: // HPS FEE Trigger
              HPSFEETrig_t fee;
              ((unsigned long *)&fee)[0]=data[i];
              feetrigger.push_back(fee);
              break;
            default:
              cout << "At " << i << " invalid HPS type: " << type << " subtype: " << subtype << endl;
          }

          break;
        case 14:
          cout << i << "VTP data type not valid: " << type << endl;
          break;
        default:
          cout << i << "I was not expecting a VTP data type of " << type << endl;
      }
    }
  }
}

