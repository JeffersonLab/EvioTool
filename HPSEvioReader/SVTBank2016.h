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
  
  void PushDataArray(const int idx, const unsigned int *dat,const int len) override;
  
  size_t size() override{
    return(svt_data.size());
  }
  
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(SVTBank2016,1);
  #pragma clang diagnostic pop
};

#endif /* __SVTBank__ */
