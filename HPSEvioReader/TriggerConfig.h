//
//  TriggerConfig.hpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/24/19.
//  Copyright © 2019 UNH. All rights reserved.
//
//  This class will parse the DAQ Trigger config file for HPS runs with a VTP, the 2019 run.
//  It can be extended to also include 2015, 2016 data if needed.
//
//  USE:
//  TriggerConfig *TC = new TriggerConfig(); //
//  TC->Parse_trigger_file("hps_v4.cnf");    // Read data from a file.
//  //------------ OR read from EVIO -----------------
//  auto et = new HPSEvioReader();
//  et->Open("hpsnosvt_009317.evio.00000");
//  auto TrigBank=et->AddLeaf<string>("TrigConfig",57614,0,"TriggerConfig");
//  et->tags.push_back(17);                  // Setup to parse events with tag = 17
//  et->tag_masks.push_back(17);
//  et->Next();                              // Read an event.
//  TC->Parse_evio_bank(TrigBank);           // Parse the data, assuming the event was type 17.
//  //------------- Access the information -------------------------
//  TrigBank->vtp_configs["SINGLE"][1]["EMIN"][0]  // Get the EMIN value for single trigger 1: VTP_HPS_SINGLE_EMIN 1 200  1
//
#ifndef TriggerConfig_h
#define TriggerConfig_h

#include "EvioTool.h"

struct FADC250_slot_t{
  int    slot=0;
  int    NSB=0;
  int    NSA=0;
  int    npeak=0;
  int    w_offset=0;
  int    window=0;
  vector<float> pedestal={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  vector<float> gain={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  vector<float> threshold={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
};

struct FADC250_crate_t{
  int    crate=0;
  int    mode=0;
  FADC250_slot_t all_slots;
  map<int,FADC250_slot_t> slots;
};

class TriggerConfig: public TObject {

public:
  bool               is_initialized;
  vector<string> parse_configs = {"SINGLE","PAIR","MULT"};

  vector<string>     raw_data;
  map<string,vector<map<string,vector<double> > > > vtp_configs;   // vtp trigger configurations.
  map<string,vector<double> >                       vtp_other;     // vtp other configurations.
  
  vector<FADC250_crate_t>  crates;

public:
  TriggerConfig(string trigfile=""){
    if(trigfile.size()>1) Parse_trigger_file(trigfile);
  };
  void Parse_trigger_file(string filename);      // Put data from hps_vx.cnf file into raw_data and parse.
  void Parse_evio_bank(Leaf<string> *TrigBank);  // Put data from TrigBank into raw_data and parse.
  void Parse_raw_data(void);                     // Internally used to parse data in raw_data;
  void Print(Option_t *option="");               // Printout the content of the TriggerConfig.

  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
ClassDef(TriggerConfig,1);
#pragma clang diagnostic pop
};

#endif /* TriggerConfig_h */
