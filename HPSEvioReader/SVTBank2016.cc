//
//  SVTbank2016.cc
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 1/2/19.
//

#include "SVTBank2016.h"

void  SVTBank2016::PushDataArray(const int idx, const unsigned int *dat,const int len){
  // Add the vector to the back of the data of the leaf at index idx
  
  // First and last int in the block are an extra header and tail.
  // These are added by Sergey, not sure what is in it.
  //
  // These could be used for verification purposes.
  // int header = (*dat)[0];          // HEADER for block.
  // int tail   = (*dat)[data_end];   // TAIL for the block.
  //
  
  for(unsigned int i=1;i< len-1 ; i+=4){
    SVT_chan_t *cn = (SVT_chan_t *)&dat[i]; // Direct data overlay, so there is no copy here. MUCH faster.
    if( !cn->head.isHeader && !cn->head.isTail){
      svt_data.emplace_back(*cn);               // The push_back copies the data onto SVT_data. This makes a copy.
    }
    //else if(cn->head.isHeader){}              // No useful information in the header??
  }
  if(fSaveHeaders){  // Store the tail, which is always at [len-1]
    SVT_event_builder_tail_t *svt_tail = (SVT_event_builder_tail_t *)&dat[len-1];
    svt_tails.emplace_back(*svt_tail);
  }
  
  if( fStoreRaw){
    SVTleaf->PushDataArray(dat,len);         // This will call the Leaf to store the raw data as well.
  }
  // Fill the svt_data vector with the data in the SVTleaf.
}


ClassImp(SVTBank2016);
