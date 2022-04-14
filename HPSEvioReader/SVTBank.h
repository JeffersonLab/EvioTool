//
//  SVTBank.h
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 1/2/19.
//
// The SVTBank is a specialized bank for interpreting the SVT data. The SVT data can be read "raw" as
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


struct SVT_event_builder_header_t {
  unsigned int event_count      : 28;   //31:00 = Event Counter (starting from 1)
  unsigned int header_mark      : 4;    // = 0x0B (2019)
  unsigned int timestamp_low24  : 24;     //23:00 = Timestamp bits 23:0 //A 48-bit running counter gets reset to 0 at the start of each run
  unsigned int junk1            : 8;                                    //This number will be the same across all DPMs for a given trigger event
  unsigned int timestamp_hi24   : 24;     //55:32 = Timestamp bits 47:24
  unsigned int junk2            : 8;
  unsigned int rce_addr         : 8;      //07:00 = RCE Address
  unsigned int total_evt_size   : 20;     //28:08 = 0x000000 - Event Builder software will overwrite this with the total event size in bytes
  unsigned int header_mark2     :  4;     //31:29 - 0xa (2019)
  unsigned long GetTimeStamp(void){
    unsigned long time = ((unsigned long)timestamp_low24) + (((unsigned long)timestamp_hi24)<<24);
    return(time);
  }
};

struct SVT_event_builder_tail_t{       // First 32 bits of the 128 bit event builder tail. The remaining bits are not used.
  unsigned int  num_multisamples: 12;  //11:00: Number of MultiSamples (M)
  unsigned int  skip_count:       12;  //23:12: SkipCount - Number of MultiSamples that got dumped due to FIFO backup
  unsigned int  unknown1:         2;
  bool          apv_sync_error:   1;   //26: ApvBufferAddressSyncError - APV frames didn't have the same addresses
  bool          fifo_backup_error:1;   //27: FIFO backup error (Should only see skip counts if this is 1)
  unsigned int  unused1:          4;
};


struct SVT_header_t {   // This is the layout of the multisample "header" that is word 3 in multi-samples.
  unsigned char rce_addr    :8;   // bit 0-7  =   RCE Address
  unsigned char feb_id      :8;   //  bit  8-15   = FEB ID
  unsigned int  chan        :7;   //   bit  16-22 = Channel ID
  unsigned int  apv         :3;   //   bit  23-25 = APV number
  unsigned int  hyb_id      :2;    //   bit  26-27 = Hybrid ID
  bool          read_err    :1;    //   bit 28 = Read Error
  bool          isTail      :1;   //    bit  29  = Is a Tail (seem to never be the case?)
  bool          isHeader    :1;  //   bit  30   = Is a Header (these are useless?)
  bool          filter_flag :1;    //    bit  31  =  UNKNOWN
};

struct SVT_chan_t{
  unsigned short samples[MAX_NUM_SVT_SAMPLES];
  SVT_header_t head;
};

class SVTBank: public Bank {

public:
  vector<SVT_chan_t>    svt_data;           // Store of the decoded data. It is copied here.
  vector<SVT_event_builder_header_t> svt_headers;
  vector<SVT_event_builder_tail_t>   svt_tails;
  Leaf<unsigned int>    *SVTleaf;           //! Handy pointer to the Leaf with the data -> (Leaf<unsigned int> *)leafs->At(0)
  bool                  fStoreRaw=false;    //! Determines whether the unparsed ints are stored or not.
  bool                  fSaveHeaders=false; //! Save all the event builder headers and tail, not just those that have sample data.
                                            //  Setting this to true slows things down, so only set to true when debugging SVT raw data.
  
public:
  SVTBank(){};

  // Initialize the SVT bank with a pointer to the EvioTool.
  SVTBank(EvioTool *p, string n,std::initializer_list<unsigned short> tags,unsigned char num,string desc): Bank(n,tags,num,desc){
    p->banks->Add(this);
    Init();
  };

  virtual ~SVTBank(){
    
  };
  
  virtual void Init(void) override {
    // Initialize the bank substructure.
    SVTleaf = AddLeaf<unsigned int>("SVTLeaf",57648,0,"SVT unsigned int data");  // Tag = 57648 for 2019, or tag = 3 for 2015/16.
  }
  
  virtual void Clear(Option_t* = "") override {
    svt_data.clear();
    svt_headers.clear();
    svt_tails.clear();
  }
  
  virtual void  PushDataArray(const int idx, const unsigned int *dat,const int len) override{
    // Add the vector to the back of the data of the leaf at index idx
    
    // First and last int in the block are an extra header and tail.
    // These are added by Sergey, not sure what is in it.
    //
    // These could be used for verification purposes.
    // int header = (*dat)[0];          // HEADER for block.
    // int tail   = (*dat)[data_end];   // TAIL for the block.
    //
    // Parse the data for 2019 data set.
    
    for(unsigned int i=1;i< len-1 ; i+=4){
      SVT_event_builder_header_t *svt_head = (SVT_event_builder_header_t *)&dat[i]; // First one is always a header.
      if(svt_head->header_mark != 0x0B) cout << "ERROR ON SVT HEADER MARK \n";
      if(fSaveHeaders) svt_headers.emplace_back(*svt_head);
      if(svt_head->total_evt_size<8) cout << "Bad SVT Data: Too small a frame. \n";
      int frame_end = i + svt_head->total_evt_size/4 - 4;
      for(i=i+4;i< frame_end; i+=4){
        // unsigned int check = dat[i+3];
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
  
  size_t size() override{
    return(svt_data.size());
  }

  void PrintBank(int print_leaves=0,int depth=10,int level=0) override;

  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(SVTBank,1);
  #pragma clang diagnostic pop
};

#endif /* __SVTBank__ */
