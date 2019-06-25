//
//  TriggerConfig.cpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/24/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#include "TriggerConfig.h"
#include <sstream>

ClassImp(TriggerConfig);

void TriggerConfig::Parse_trigger_file(string filename){
  // Read the file from filename, presumably an trigger.cnf file (e.g. hps_v4.cnf)
  // Then parse the content of this file, and fill the configuration structures.
  //
  Clear("");
  ifstream infile(filename); // Open file as a stream.
  string line;
  while(std::getline(infile,line)){
    line.erase(line.begin(),std::find_if(line.begin(), line.end(), std::bind1st(std::not_equal_to<char>(), ' '))); // Erase any leading spaces.
    if(line.size()<1 || line[0]=='#') continue;  // Comments are either blank or start with a #
    raw_data.push_back(line);
  }
  Parse_raw_data();
}
void TriggerConfig::Parse_evio_bank(void){
  // Read the data from the appropriate evio leaf bank. (Should be 0xE10E in event with tag=17)
  // Then parse the content of this file, and fill the configuration structures.
  // This method is called automatically by CallBack() when the Leaf is filled from EVIO.
  //
  Clear("");
  for(string line: data){
    line.erase(line.begin(),std::find_if(line.begin(), line.end(), std::bind1st(std::not_equal_to<char>(), ' '))); // Erase any leading spaces.
    if(line.size()<1 || line[0]=='#') continue;  // Comments are either blank or start with a #
    raw_data.push_back(line);
  }
  Parse_raw_data();
}

void TriggerConfig::Parse_raw_data(void){
  // Internally used to parse the data in raw_data;
  //
  // Most information in VTP_HPS and FADC250 are parsed.
  // If needed other banks would need to be added.
  //
  bool vtp_parse_area=false;
  int  fadc_parse_area=0;
  int  fadc_current_slot=0;
  int  config_num=0;
  
  for(string line: raw_data){
    size_t pos = line.find_first_of(" \t");
    string tok=line.substr(0,pos);
    string dat=line.substr(pos+1,line.size());
    dat.erase(dat.begin(),std::find_if(dat.begin(), dat.end(), std::bind1st(std::not_equal_to<char>(), ' ')));
    dat.erase(std::find_if(dat.rbegin(), dat.rend(), std::bind1st(std::not_equal_to<char>(), ' ')).base(), dat.end());

    if( tok == "VTP_CRATE" ){
      if( dat == "all") vtp_parse_area=true;
      if( dat == "end") vtp_parse_area=false;
      continue;
    }
    if( tok == "FADC250_CRATE" ){
      if( dat == "hps1") fadc_parse_area=1;
      if( dat == "hps2") fadc_parse_area=2;
      if( dat == "end")  fadc_parse_area=0;
      continue;
    }

    //
    // Below we parse the VTP_HPS_XXX tokens into the configs map.
    // The configs map is organized as:
    // configs["config name"][config_number]["config item"]= vector<double>
    
    if(vtp_parse_area){
      if(tok.substr(0,8)=="VTP_HPS_"){
        size_t pos_third_underscore = tok.find("_",9);
        string config = tok.substr(8,pos_third_underscore-8);
        string item = tok.substr(pos_third_underscore+1);
        if(pos_third_underscore>255){ // OOPS, some configs don't have a third underscore
          item="";
        }
        if(config == "FEE"){ // The FEE config follows a different patterns. UGH.
          string fee_item = config+"_"+item;
          istringstream iss(dat);
          if(item == "PRESCALE"){ // VTP_HPS_FEE_PRESCALE
            string prescale_num;
            iss >> prescale_num;
            fee_item += "_"+prescale_num;
          }
          vector<double> d_dat(istream_iterator<double>{iss},istream_iterator<double>()); // same as iss >> d_dat[i]
          vtp_other[fee_item] = d_dat;
        }else if(config=="PRESCALE"){ // VTP_HPS_PRESCALE
          istringstream iss(dat);
          string prescale_num;
          iss >> prescale_num;
          double prescale_val;
          iss >> prescale_val;
          vtp_other[config+"_"+prescale_num]={prescale_val};
        }else if( !(std::find(parse_configs.begin(),parse_configs.end(),config)==parse_configs.end()) ){ // We want this config.
          if( vtp_configs.find(config) == vtp_configs.end() ){                                            // It is not yet in the map.
            vector<map<string,vector<double> > > dummy_config;
            vtp_configs[config]=dummy_config;
          }
          // In general there is a list of numbers after the token, so convert the dat string to a vector of double. Use streaming.
          istringstream iss(dat);
          iss >> config_num;
          vector<double> d_dat(istream_iterator<double>{iss},istream_iterator<double>()); // same as iss >> d_dat[i]
          
          //  Check if this config_num was already put on the configs[config]
          while(vtp_configs[config].size()< config_num +1){ // The space was not yet created, so allocate it up to config_num.
            map<string,vector<double> > dummy_config;
            vtp_configs[config].push_back(dummy_config);
          }
          if(config_num < vtp_configs[config].size()) vtp_configs[config][config_num][item]=d_dat;
        }else{ // Other than the standard config.
          istringstream iss(dat);
          vector<double> d_dat(istream_iterator<double>{iss},istream_iterator<double>()); // same as iss >> d_dat[i]
          vtp_other[config+"_"+item]=d_dat;
        }
      }else{
        // cerr << "Extra tokens in VTP_HPS area: " << tok << endl;
      }
    }
    
    if(fadc_parse_area){
      // Initialize crates if needed
      int crnum = fadc_parse_area-1;
      while(crates.size()<crnum+1){
        FADC250_crate_t cr;
        cr.crate=crates.size();
        crates.push_back(cr);
      }
      
      if(tok == "FADC250_SLOT"){
        if(dat == "all" ){
          fadc_current_slot = 0;
        }else{
          fadc_current_slot = stoi(dat);
          if(crates[crnum].slots.find(fadc_current_slot) == crates[crnum].slots.end()){
            // Initialize the slot.
            FADC250_slot_t slot;
            slot.slot = fadc_current_slot;
            slot.NSB = crates[crnum].all_slots.NSB;
            slot.NSA = crates[crnum].all_slots.NSA;
            slot.npeak=crates[crnum].all_slots.npeak;
            slot.w_offset=crates[crnum].all_slots.w_offset;
            slot.window=crates[crnum].all_slots.window;
            slot.pedestal =crates[crnum].all_slots.pedestal;
            slot.gain =crates[crnum].all_slots.gain;
            slot.threshold =crates[crnum].all_slots.threshold;
            crates[crnum].slots[fadc_current_slot] = slot;
          }
        }
      }else if(tok == "FADC250_MODE"){
        crates[crnum].mode = stoi(dat);
      }else if(tok == "FADC250_NSB"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.NSB = stoi(dat);
        }else{
          crates[crnum].slots[fadc_current_slot].NSB=stoi(dat);
        }
      }else if(tok == "FADC250_NSA"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.NSA = stoi(dat);
        }else{
          crates[crnum].slots[fadc_current_slot].NSA=stoi(dat);
        }
      }else if(tok == "FADC250_NPEAK"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.npeak = stoi(dat);
        }else{
          crates[crnum].slots[fadc_current_slot].npeak=stoi(dat);
        }
      }else if(tok == "FADC250_TET"){
        if(fadc_current_slot == 0){
          fill(crates[crnum].all_slots.threshold.begin(),crates[crnum].all_slots.threshold.end(),stoi(dat));
        }else{
          fill(crates[crnum].slots[fadc_current_slot].threshold.begin(),crates[crnum].slots[fadc_current_slot].threshold.end(),stoi(dat));
        }
      }else if(tok == "FADC250_W_OFFSET"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.w_offset = stoi(dat);
        }else{
          crates[crnum].slots[fadc_current_slot].w_offset=stoi(dat);
        }
      }else if(tok == "FADC250_W_WIDTH"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.window = stoi(dat);
        }else{
          crates[crnum].slots[fadc_current_slot].window=stoi(dat);
        }
      }else if(tok == "FADC250_CH_TET"){
        if(fadc_current_slot == 0){
          cout << "TriggerConfig PARSING ERROR: cannot set channel threshold without knowing slot \n";
        }else{
          int chan;
          int thres;
          istringstream iss(dat);
          iss >> chan;
          iss >> thres;
          crates[crnum].slots[fadc_current_slot].threshold[chan]=thres;
        }
      }else if(tok == "FADC250_ALLCH_GAIN"){
        if(fadc_current_slot == 0){
          cout << "TriggerConfig PARSING ERROR: cannot set channel gains without knowing slot \n";
        }else{
          istringstream iss(dat);
          crates[crnum].slots[fadc_current_slot].gain.assign(istream_iterator<float>{iss},istream_iterator<float>());
        }
      }else if(tok == "FADC250_ALLCH_PED"){
        if(fadc_current_slot == 0){
          cout << "TriggerConfig PARSING ERROR: cannot set channel pedestals without knowing slot \n";
        }else{
          istringstream iss(dat);
          crates[crnum].slots[fadc_current_slot].pedestal.assign(istream_iterator<float>{iss},istream_iterator<float>());
        }
      }
    }
  }
  is_initialized=true;  // Set the initialized flag.
}

void TriggerConfig::Print(Option_t *option){
  // Print the content of the trigger configuration.
  cout << "------ Trigger Configuration ------\n";
  for(auto const &config: vtp_configs){
    for(int config_num=0; config_num< config.second.size();++config_num){
      cout << "----" << config.first << "[" << config_num << "] ----\n";
      auto config_items = config.second[config_num];
      for(auto const &items: config_items){
        cout << "     " << items.first << "  ";
        for(double vals:items.second) cout << vals << " ";
        cout << endl;
      }
    }
    cout << endl;
  }
  cout << "------ Other VTP Data ------\n";
  for(auto const &other: vtp_other){
    cout<< "     " << other.first << "  ";
    for(double vals:other.second) cout << vals << " ";
    cout << endl;
  }
  
  cout << "\n------ FADC250  Data ------\n";
  for(int cr=0;cr<crates.size();++cr){
    cout << "CRATE: " << cr << endl;
    for(auto &sl: crates[cr].slots){
      cout << "     SLOT: " << sl.second.slot << endl;
      cout << "         NSB: " << sl.second.NSB << "  NSA: " << sl.second.NSA << "  npeak: " << sl.second.npeak << "  w_offset: " << sl.second.w_offset << " window: " << sl.second.window <<endl;
      cout <<   "         Peds:";
      for(int ch=0; ch<16; ++ch) printf(" %7.3f",sl.second.pedestal[ch]);
      cout << "\n         Gain:";
      for(int ch=0; ch<16; ++ch) printf(" %7.3f",sl.second.gain[ch]);
      cout << "\n         Thre:";
      for(int ch=0; ch<16; ++ch) printf(" %7.3f",sl.second.threshold[ch]);
      cout << "\n";
    }
    cout << "--------------------\n";
  }
}
