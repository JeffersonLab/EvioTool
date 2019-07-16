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

// The 2015/2016 SVT Data structure:
// --> SVT  = container with tags 51 - 65
//     | -> Headbank   - tag 57610
//     | -> SVT Data   - tag = 3     Special blob of ints.
//
// The 2019 SVT Data structure:
// --> SVT  = container with tags 2-3
//     | -> Headbank   - tag = 57610
//     | -> SVT Data   - tag = 57648
//
#include "HPSEvioReader.h"

HPSEvioReader::HPSEvioReader(string infile,string trigfile,int dataset): EvioTool(infile){
 
  if(trigfile.size()>1){
    TrigConf = new TriggerConfig(trigfile); // Just load from the file, do not also look for the bank in the data.
  }else{
    TrigConf = new TriggerConfig(this);     // Add the Trigger Config to the banks to look for.
  }
  fAutoAdd = false;
  fChop_level=1;
  tags={128,17};          // Only check for physics banks, i.e. bit7 = 1, and also 17 = Trigger config.
  tag_masks = {128,17};
  
  // We setup the data structure the way is it layed out in the EVIO file.
  // Differences between 2019 and 2015/16 data require some choices.
  //
  Head = new Header(this);
  TrigCrate = AddBank("Trig",46,0,"Trigger bank");
  TrigHead = new Headbank(TrigCrate);
  Trigger   = new TSBank(TrigCrate);
  ECALCrate = AddBank("ECAL",{37,39},0,"Ecal banks");
  FADC     = ECALCrate->AddLeaf<FADCdata>("FADC",57601,0,"FADC mode 1 data");
  ECAL     = new EcalBank(FADC,TrigConf);
  if(dataset == 2019 || dataset ==2){
    Set2019Data();
  }else if(dataset == 2015 | dataset == 2016 | dataset == 1){
    Set2016Data();
  }
};

void HPSEvioReader::Set2019Data(){
// Setup for the 2019 data set.

// First clean up.
  if(SVT)     delete SVT;
  if(TrigTop) delete TrigTop;
  if(VtpTop)  delete VtpTop;
  if(TrigBot) delete TrigBot;
  if(VtpBot)  delete VtpBot;
  
  SVT      = new SVTBank(this,"SVT",{2,3},0,"SVT banks");                                           //  2019 data.
  SVT->Set2019Data();
  TrigTop  = AddBank("TrigTop",11,0,"Trigger Bank top");
  VtpTop   = new VTPBank(TrigTop);
  TrigBot  = AddBank("TrigTop",12,0,"Trigger Bank bottom");
  VtpBot   = new VTPBank(TrigBot);
}

void HPSEvioReader::Set2016Data(){
  // Setup for the 2015 or 2016 data sets.
  if(SVT) delete SVT;
  SVT      = new SVTBank(this,"SVT",{51,52,53,54,55,56,57,58,59,60,61,62,63,64,65},0,"SVT banks");  // 2015 or 2016 data set.
  SVT->Set2016Data();
}

int HPSEvioReader::Next(void){
  // Override of EvioTool::Next() to call local parsers.
  
  int stat=EvioTool::Next();
  if(this_tag == 17 && ECAL){
    ECAL->Config();
    cout << "ECAL Configured.\n";
  }
  if(ECAL && ECAL->is_configured){
    ECAL->Parse();
    ECAL->FindGTPClusters();
  }else{
    cout << "ECAL not yet configured. Provide a trigger file! \n";
  }
  return(stat);
}
ClassImp(HPSEvioReader);
