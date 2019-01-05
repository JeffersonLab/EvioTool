//
//  SVTbank.h
//  EvioTool
//
//  Created by Maurik Holtrop on 1/2/19.
//
// The SVTbank is a specialized bank for interpreting the SVT data. The SVT data can be read "raw" as
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
#ifndef __SVTdata__
#define __SVTdata__

#include "TROOT.h"
#include "TObject.h"
#include "EvioTool.h"
#include "Bank.h"

// For an explanation on bit packing with CLANG or G++: http://jkz.wtf/bit-field-packing-in-gcc-and-clang
// Key issue - spanning a 32-bit long data field, the upper bit fields need to have type unsigned int.
// Note that the bools are OK.
// The *memory shape* of this structure or class MUST correspond perfectly to the memory layout of the EVIO data.
// That way we can use the much faster memory overlay rather than data copy.
//

#define MAX_NUM_SVT_FPGA 12
#define MAX_NUM_SVT_SAMPLES 6
#define MAX_SVT_DATA   1024

struct SVT_header_t {
  unsigned char XXX1    :8;   // bit 0-7   UNKNOWN
  unsigned char feb_id  :8;   //  bit  8-15   = FEB ID
  unsigned int  chan    :7;   //   bit  16-22 = Channel ID
  unsigned int  apv     :3;   //   bit  23-25 = APV number
  unsigned int  hyb_id  :3;    //   bit  26-28 = Hybrid ID
  bool          isTail  :1;   //    bit  29  = Is a Tail (seem to never be the case?)
  bool          isHeader:1;  //   bit  30   = Is a Header (these are useless?)
  bool          XXX2    :1;    //    bit  31  =  UNKNOWN
};

struct SVT_chan_t{
  unsigned short samples[MAX_NUM_SVT_SAMPLES];
  SVT_header_t head;
};

class SVTbank: public Bank {

public:
  vector<SVT_chan_t>    svt_data;  // Pointers to decoded data.
  Leaf<unsigned int>    *SVTleaf;         //! Handy pointer to the Leaf with the data -> (Leaf<unsigned int> *)leafs->At(0)
  bool                  fStoreRaw{false}; //! Determines whether the unparsed ints are stored or not.
  
public:
  SVTbank(){
//    SetName("SVTbank");
//    SetTitle("Bank specifically for reading SVT data");
//    tags={51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66};
//    num = 0;
//    Init();
  };

  // Initialize the SVT bank with a pointer to the EvioTool.
  SVTbank(EvioTool *p, string n,std::initializer_list<unsigned short> tags,unsigned char num,string desc): Bank(n,tags,num,desc){
    p->banks->Add(this);
    Init();
  };

  void Init(void) override {
    // Initialize the bank substructure.
    SVTleaf = Add_Leaf<unsigned int>("SVTLeaf",3,0,"SVT unsigned int data");
  }
  
  void Clear(Option_t* = "") override {
    svt_data.clear();
  }
  
  void  Push_data_array(const int idx, const unsigned int *dat,const int len) override{
    // Add the vector to the back of the data of the leaf at index idx
    
    // First and last int in the block are the header and tail.
    // These could be used for verification purposes.
    // int header = (*dat)[0];          // HEADER for block.
    // int tail   = (*dat)[data_end];   // TAIL for the block.
    //
    
    for(unsigned int i=1;i< len-1 ; i+=4){
      SVT_chan_t *cn = (SVT_chan_t *)&dat[i]; // Direct data overlay, so there is no copy here. MUCH faster.
      if( !cn->head.isHeader && !cn->head.isTail){
        svt_data.emplace_back(*cn);               // The push_back copies the data onto SVT_data. This makes a copy.
        
      }
    }
    if( fStoreRaw){
      SVTleaf->Push_data_array(dat,len);         // This will call the Leaf to store the raw data as well.
    }
    // Fill the svt_data vector with the data in the SVTleaf.
  }
  
  size_t size(int type=0) override{
    return(svt_data.size());
  }
  
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(SVTbank,1);
  #pragma clang diagnostic pop
};

#endif /* __SVTbank__ */
