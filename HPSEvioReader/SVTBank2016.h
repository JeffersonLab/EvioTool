//
//  SVTBank2016.h
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 1/2/19.
//
// The SVTBank is a specialized bank for interpreting the SVT data. This version if for
// the engineering data sets of 2015 and 2016. The SVT data can be read "raw" as
// unsigned int. This class helps to automatically translate that content to meaningful SVT hits.
//
// The bank structure for the SVT is:
// Bank with tag = 51 through 66
//    Leaf<unsigned int> 57610  -- standard bank header.
//    Leaf<unsigned int> 57607  -- For tag 58 only. Presumably these are Hybrid temperatures?
//    Leaf<unsigned int>     3  -- Encoded SVT data.
//
// The type 3 leaf can be decoded very fast by overlaying the correct data structure, which is what we do here.
// We then only store those SVT hits that actually have data in them. The rest are headers.
//
// When adding this bank there is no need to add a Leaf into this bank.
//
// The structure of the N*4 word SVT raw data is documented on Confluence at:
// SVT Bank structure information for 2015/2016 data: https://confluence.slac.stanford.edu/display/hpsg/DPM+Event+Builder+Output
// SVT Bank structure information for 2019 data: https://confluence.slac.stanford.edu/display/hpsg/2019+DPM+Event+Output
//
// Unfortunately, as of 7/16/2019, that documentation is bunk. It needs to be updated.
//
// For 2019 data, using the hps-java parser from Omar in Phys2019SvtEvioReader.java:
// This parses the data from the end backwards. It identifies a "Tail" by

#ifndef __SVTdata2016__
#define __SVTdata2016__

#include "TROOT.h"
#include "TObject.h"
#include "EvioTool.h"
#include "SVTBank.h"


class SVTBank2016: public SVTBank {

public:
 
public:
  SVTBank2016(){};

  // Initialize the SVT bank with a pointer to the EvioTool.
  SVTBank2016(EvioTool *p, string n,std::initializer_list<unsigned short> tags,unsigned char num,string desc): SVTBank(p,n,tags,num,desc){
    SVTleaf->tag = 3;
  } // All the work is done in SVTBank
  
  void  PushDataArray(const int idx, const unsigned int *dat,const int len) override{
    // Add the vector to the back of the data of the leaf at index idx
    
    // First and last int in the block are an extra header and tail.
    // These are added by Sergey, not sure what is in it.
    //
    // These could be used for verification purposes.
    // int header = (*dat)[0];          // HEADER for block.
    // int tail   = (*dat)[data_end];   // TAIL for the block.
    //
    
    for(unsigned int i=1;i< len-1 ; i+=4){
      SVT_event_builder_header_t *svt_head = (SVT_event_builder_header_t *)&dat[i]; // First one is always a header.
      if(svt_head->header_mark != 0x0B) cout << "ERROR ON SVT HEADER MARK \n";
      if(fSaveHeaders) svt_headers.emplace_back(*svt_head);
      if(svt_head->total_evt_size<8) cout << "Bad SVT Data: Too small a frame. \n";
      int frame_end = i + svt_head->total_evt_size/4 - 4;
      for(i=i+4;i< frame_end; i+=4){
        unsigned int check = dat[i+3];
        SVT_chan_t *cn = (SVT_chan_t *)&dat[i]; // Direct data overlay, so there is no copy here. MUCH faster.
        if(!cn->head.isTail & !cn->head.isHeader){
          svt_data.emplace_back(*cn);               // The push_back copies the data onto SVT_data. This makes a copy.
        }else{
          if(cn->head.isTail) cout << "WE HAVE AN SVT PARSING ERROR - TAIL where expecting head. \n";
        }
      }
      {
        if( (dat[i+3]&0xFF000000) != 0xa8000000 ) cout << "SVT - we have a funny tail! \n";
        if( (dat[i]&0x00FFFFFF) != (svt_head->total_evt_size/16 - 2) | dat[i+1] !=0 | dat[i+2] !=0 ) cout << "Extra data in tail: " << (dat[i]&0x00FFFFFF) << "," << dat[i+1] << "," << dat[i+2] << endl;
      }
      if( fSaveHeaders){  // Frame Tail
        SVT_event_builder_tail_t *svt_tail = (SVT_event_builder_tail_t *)&dat[i];
        svt_tails.emplace_back(*svt_tail);
      }
      if( fStoreRaw){
        SVTleaf->PushDataArray(dat,len);         // This will call the Leaf to store the raw data as well.
      }// Fill the svt_data vector with the data in the SVTleaf.
    }
  }
  
  size_t size(int type=0) override{
    return(svt_data.size());
  }
  
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(SVTBank2016,1);
  #pragma clang diagnostic pop
};

#endif /* __SVTBank__ */
