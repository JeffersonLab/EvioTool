//
//  HPSEvioReader.cpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/19/19.
//  Copyright © 2019 UNH. All rights reserved.
//

//
// HPS run 2019 data structure.
//
// The event is "etool" = top level bank.
// --> head = Leaf
// --> Trig = container with tag 46
//     | -> Headbank  - header info
//     | -> TIData    - Trigger info
//
// --> ECAL = container with tags 37 or 39
//     | -> Headbank  - header info
//     | -> FADC      - FADC data
//
// --> SVT  = container with tags 51 - 65
//     | -> Headbank(?)
//     | -> SVT Data   - Special blob of ints.
//
#include "HPSEvioReader.h"

HPSEvioReader::HPSEvioReader(string infile){
  fAutoAdd = false;
  fChop_level=1;
  tags={128};
  tag_mask = 128;
  
  
  head = new Header(this);
  trig = AddBank("Trig",46,0,"Trigger bank");
  trighead = new Headbank(trig);
  tidata   = new TIData(trig);
  ECAL     = AddBank("ECAL",{37,39},0,"Ecal banks");
  FADC     = ECAL->AddLeaf<FADCdata>("FADC",57601,0,"FADC mode 1 data");
  SVT      = new SVTbank(this,"SVT",{51,52,53,54,55,56,57,58,59,60,61,62,63,64,65},0,"SVT banks");
  TrigTop  = AddBank("TrigTop",11,0,"Trigger Bank top");
  VtpTop   = new VTPData(TrigTop);
  TrigBot  = AddBank("TrigTop",12,0,"Trigger Bank bottom");
  VtpBot   = new VTPData(TrigBot);
};

ClassImp(HPSEvioReader);
