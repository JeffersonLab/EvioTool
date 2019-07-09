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
  raw_data.clear();
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
  raw_data.clear();
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
  int  fadc_crate_number=0;
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
      if( dat == "hps1") fadc_crate_number=1;
      if( dat == "hps2") fadc_crate_number=2;
      if( dat == "end")  fadc_crate_number=0;
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
    
    if(fadc_crate_number){
      // Initialize crates if needed
      // Note: We create a crate[0], which should not exist in any configuration.
      //       Doing so permits us to use crate=1 and crate=2 everywhere, rather than always translating to 0, 1.
      int crnum = fadc_crate_number;
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
          for(auto s: crates[crnum].slots) crates[crnum].slots[s.first].NSB = stoi(dat);   // All slots, so set all slots!
        }else{
          crates[crnum].slots[fadc_current_slot].NSB=stoi(dat);
        }
      }else if(tok == "FADC250_NSA"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.NSA = stoi(dat);
          for(auto s: crates[crnum].slots) crates[crnum].slots[s.first].NSA = stoi(dat);
        }else{
          crates[crnum].slots[fadc_current_slot].NSA=stoi(dat);
        }
      }else if(tok == "FADC250_NPEAK"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.npeak = stoi(dat);
          for(auto s: crates[crnum].slots) crates[crnum].slots[s.first].npeak = stoi(dat);
        }else{
          crates[crnum].slots[fadc_current_slot].npeak=stoi(dat);
        }
      }else if(tok == "FADC250_TET"){
        if(fadc_current_slot == 0){
          fill(crates[crnum].all_slots.threshold.begin(),crates[crnum].all_slots.threshold.end(),stoi(dat));
          for(auto s: crates[crnum].slots) fill(crates[crnum].slots[s.first].threshold.begin(),crates[crnum].slots[s.first].threshold.end(),stoi(dat));
        }else{
          fill(crates[crnum].slots[fadc_current_slot].threshold.begin(),crates[crnum].slots[fadc_current_slot].threshold.end(),stoi(dat));
        }
      }else if(tok == "FADC250_W_OFFSET"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.w_offset = stoi(dat);
          for(auto s: crates[crnum].slots) crates[crnum].slots[s.first].w_offset = stoi(dat);
        }else{
          crates[crnum].slots[fadc_current_slot].w_offset=stoi(dat);
        }
      }else if(tok == "FADC250_W_WIDTH"){
        if(fadc_current_slot == 0){
          crates[crnum].all_slots.window = stoi(dat);
          for(auto s: crates[crnum].slots) crates[crnum].slots[s.first].window = stoi(dat);
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

void TriggerConfig::WriteToFile(string file){
  // Write the content from the trigger config event to a file, similar to the
  // original trigger config file. We just dump the data we read in from the bank back out.
  ofstream fo(file);
  for(auto s: data){
    fo << s << endl;
  }
  fo.close();
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

void TriggerConfig::InitDAQMaps(void){
  // Initialize the RF,ECAL and Hodoscope DAQ Maps.
  //
  // TODO: Get this from the DB.
  //
  // I know, I know, cringe, this is ugly like old bananas.
  //
  // Generate this blob with:
  // mysql -h hpsdb.jlab.org -u hpsuser --password=darkphoton -e "select 'crates[',crate,'].slots[',LPAD(slot,2,' '),'].set_sxy(',LPAD(channel,2,' '),', 2,',LPAD(x,3,' '),',',LPAD(y,3,' '),');' from ecal_channels where collection_id=2017;" hps_conditions  | tr -d '\t' > ecal_daqmap.h
  // Then "pbcopy < ecal_daqmap.h" and paste it here. Delete top line.
  // Could also #include it here.
  //
  // Make sure the crates exist. If not using a file, they may not.
  while(crates.size()<2+1){
    FADC250_crate_t cr;
    cr.crate=crates.size();
    crates.push_back(cr);
  }

  //
  //crates[crate].slots[LPAD(slot,2,' ')].set_sxy(LPAD(channel,2,' '), 2,LPAD(x,3,' '),LPAD(y,3,' '));
  crates[1].slots[20].set_sxy(12, 2,-23,  5);
  crates[1].slots[20].set_sxy( 7, 2,-22,  5);
  crates[1].slots[20].set_sxy( 2, 2,-21,  5);
  crates[1].slots[19].set_sxy(13, 2,-20,  5);
  crates[1].slots[19].set_sxy( 8, 2,-19,  5);
  crates[1].slots[19].set_sxy( 3, 2,-18,  5);
  crates[1].slots[18].set_sxy(14, 2,-17,  5);
  crates[1].slots[18].set_sxy( 9, 2,-16,  5);
  crates[1].slots[18].set_sxy( 4, 2,-15,  5);
  crates[1].slots[17].set_sxy(15, 2,-14,  5);
  crates[1].slots[17].set_sxy(10, 2,-13,  5);
  crates[1].slots[17].set_sxy( 5, 2,-12,  5);
  crates[1].slots[17].set_sxy( 0, 2,-11,  5);
  crates[1].slots[16].set_sxy(11, 2,-10,  5);
  crates[1].slots[16].set_sxy( 7, 2, -9,  5);
  crates[1].slots[16].set_sxy( 3, 2, -8,  5);
  crates[1].slots[15].set_sxy(15, 2, -7,  5);
  crates[1].slots[15].set_sxy(11, 2, -6,  5);
  crates[1].slots[15].set_sxy( 7, 2, -5,  5);
  crates[1].slots[15].set_sxy( 3, 2, -4,  5);
  crates[1].slots[14].set_sxy(15, 2, -3,  5);
  crates[1].slots[14].set_sxy(11, 2, -2,  5);
  crates[1].slots[14].set_sxy( 7, 2, -1,  5);
  crates[1].slots[14].set_sxy( 2, 2,  1,  5);
  crates[1].slots[ 9].set_sxy(13, 2,  2,  5);
  crates[1].slots[ 9].set_sxy( 8, 2,  3,  5);
  crates[1].slots[ 9].set_sxy( 3, 2,  4,  5);
  crates[1].slots[ 8].set_sxy(14, 2,  5,  5);
  crates[1].slots[ 8].set_sxy( 9, 2,  6,  5);
  crates[1].slots[ 8].set_sxy( 4, 2,  7,  5);
  crates[1].slots[ 7].set_sxy(15, 2,  8,  5);
  crates[1].slots[ 7].set_sxy(10, 2,  9,  5);
  crates[1].slots[ 7].set_sxy( 5, 2, 10,  5);
  crates[1].slots[ 7].set_sxy( 0, 2, 11,  5);
  crates[1].slots[ 6].set_sxy(11, 2, 12,  5);
  crates[1].slots[ 6].set_sxy( 6, 2, 13,  5);
  crates[1].slots[ 6].set_sxy( 1, 2, 14,  5);
  crates[1].slots[ 5].set_sxy(12, 2, 15,  5);
  crates[1].slots[ 5].set_sxy( 7, 2, 16,  5);
  crates[1].slots[ 5].set_sxy( 2, 2, 17,  5);
  crates[1].slots[ 4].set_sxy(13, 2, 18,  5);
  crates[1].slots[ 4].set_sxy( 8, 2, 19,  5);
  crates[1].slots[ 4].set_sxy( 3, 2, 20,  5);
  crates[1].slots[ 3].set_sxy(14, 2, 21,  5);
  crates[1].slots[ 3].set_sxy( 9, 2, 22,  5);
  crates[1].slots[ 3].set_sxy( 4, 2, 23,  5);
  crates[1].slots[20].set_sxy(11, 2,-23,  4);
  crates[1].slots[20].set_sxy( 6, 2,-22,  4);
  crates[1].slots[20].set_sxy( 1, 2,-21,  4);
  crates[1].slots[19].set_sxy(12, 2,-20,  4);
  crates[1].slots[19].set_sxy( 7, 2,-19,  4);
  crates[1].slots[19].set_sxy( 2, 2,-18,  4);
  crates[1].slots[18].set_sxy(13, 2,-17,  4);
  crates[1].slots[18].set_sxy( 8, 2,-16,  4);
  crates[1].slots[18].set_sxy( 3, 2,-15,  4);
  crates[1].slots[17].set_sxy(14, 2,-14,  4);
  crates[1].slots[17].set_sxy( 9, 2,-13,  4);
  crates[1].slots[17].set_sxy( 4, 2,-12,  4);
  crates[1].slots[16].set_sxy(15, 2,-11,  4);
  crates[1].slots[16].set_sxy(10, 2,-10,  4);
  crates[1].slots[16].set_sxy( 6, 2, -9,  4);
  crates[1].slots[16].set_sxy( 2, 2, -8,  4);
  crates[1].slots[15].set_sxy(14, 2, -7,  4);
  crates[1].slots[15].set_sxy(10, 2, -6,  4);
  crates[1].slots[15].set_sxy( 6, 2, -5,  4);
  crates[1].slots[15].set_sxy( 2, 2, -4,  4);
  crates[1].slots[14].set_sxy(14, 2, -3,  4);
  crates[1].slots[14].set_sxy(10, 2, -2,  4);
  crates[1].slots[14].set_sxy( 6, 2, -1,  4);
  crates[1].slots[14].set_sxy( 1, 2,  1,  4);
  crates[1].slots[ 9].set_sxy(12, 2,  2,  4);
  crates[1].slots[ 9].set_sxy( 7, 2,  3,  4);
  crates[1].slots[ 9].set_sxy( 2, 2,  4,  4);
  crates[1].slots[ 8].set_sxy(13, 2,  5,  4);
  crates[1].slots[ 8].set_sxy( 8, 2,  6,  4);
  crates[1].slots[ 8].set_sxy( 3, 2,  7,  4);
  crates[1].slots[ 7].set_sxy(14, 2,  8,  4);
  crates[1].slots[ 7].set_sxy( 9, 2,  9,  4);
  crates[1].slots[ 7].set_sxy( 4, 2, 10,  4);
  crates[1].slots[ 6].set_sxy(15, 2, 11,  4);
  crates[1].slots[ 6].set_sxy(10, 2, 12,  4);
  crates[1].slots[ 6].set_sxy( 5, 2, 13,  4);
  crates[1].slots[ 6].set_sxy( 0, 2, 14,  4);
  crates[1].slots[ 5].set_sxy(11, 2, 15,  4);
  crates[1].slots[ 5].set_sxy( 6, 2, 16,  4);
  crates[1].slots[ 5].set_sxy( 1, 2, 17,  4);
  crates[1].slots[ 4].set_sxy(12, 2, 18,  4);
  crates[1].slots[ 4].set_sxy( 7, 2, 19,  4);
  crates[1].slots[ 4].set_sxy( 2, 2, 20,  4);
  crates[1].slots[ 3].set_sxy(13, 2, 21,  4);
  crates[1].slots[ 3].set_sxy( 8, 2, 22,  4);
  crates[1].slots[ 3].set_sxy( 3, 2, 23,  4);
  crates[1].slots[20].set_sxy(10, 2,-23,  3);
  crates[1].slots[20].set_sxy( 5, 2,-22,  3);
  crates[1].slots[20].set_sxy( 0, 2,-21,  3);
  crates[1].slots[19].set_sxy(11, 2,-20,  3);
  crates[1].slots[19].set_sxy( 6, 2,-19,  3);
  crates[1].slots[19].set_sxy( 1, 2,-18,  3);
  crates[1].slots[18].set_sxy(12, 2,-17,  3);
  crates[1].slots[18].set_sxy( 7, 2,-16,  3);
  crates[1].slots[18].set_sxy( 2, 2,-15,  3);
  crates[1].slots[17].set_sxy(13, 2,-14,  3);
  crates[1].slots[17].set_sxy( 8, 2,-13,  3);
  crates[1].slots[17].set_sxy( 3, 2,-12,  3);
  crates[1].slots[16].set_sxy(14, 2,-11,  3);
  crates[1].slots[16].set_sxy( 9, 2,-10,  3);
  crates[1].slots[16].set_sxy( 5, 2, -9,  3);
  crates[1].slots[16].set_sxy( 1, 2, -8,  3);
  crates[1].slots[15].set_sxy(13, 2, -7,  3);
  crates[1].slots[15].set_sxy( 9, 2, -6,  3);
  crates[1].slots[15].set_sxy( 5, 2, -5,  3);
  crates[1].slots[15].set_sxy( 1, 2, -4,  3);
  crates[1].slots[14].set_sxy(13, 2, -3,  3);
  crates[1].slots[14].set_sxy( 9, 2, -2,  3);
  crates[1].slots[14].set_sxy( 5, 2, -1,  3);
  crates[1].slots[14].set_sxy( 0, 2,  1,  3);
  crates[1].slots[ 9].set_sxy(11, 2,  2,  3);
  crates[1].slots[ 9].set_sxy( 6, 2,  3,  3);
  crates[1].slots[ 9].set_sxy( 1, 2,  4,  3);
  crates[1].slots[ 8].set_sxy(12, 2,  5,  3);
  crates[1].slots[ 8].set_sxy( 7, 2,  6,  3);
  crates[1].slots[ 8].set_sxy( 2, 2,  7,  3);
  crates[1].slots[ 7].set_sxy(13, 2,  8,  3);
  crates[1].slots[ 7].set_sxy( 8, 2,  9,  3);
  crates[1].slots[ 7].set_sxy( 3, 2, 10,  3);
  crates[1].slots[ 6].set_sxy(14, 2, 11,  3);
  crates[1].slots[ 6].set_sxy( 9, 2, 12,  3);
  crates[1].slots[ 6].set_sxy( 4, 2, 13,  3);
  crates[1].slots[ 5].set_sxy(15, 2, 14,  3);
  crates[1].slots[ 5].set_sxy(10, 2, 15,  3);
  crates[1].slots[ 5].set_sxy( 5, 2, 16,  3);
  crates[1].slots[ 5].set_sxy( 0, 2, 17,  3);
  crates[1].slots[ 4].set_sxy(11, 2, 18,  3);
  crates[1].slots[ 4].set_sxy( 6, 2, 19,  3);
  crates[1].slots[ 4].set_sxy( 1, 2, 20,  3);
  crates[1].slots[ 3].set_sxy(12, 2, 21,  3);
  crates[1].slots[ 3].set_sxy( 7, 2, 22,  3);
  crates[1].slots[ 3].set_sxy( 2, 2, 23,  3);
  crates[1].slots[20].set_sxy( 9, 2,-23,  2);
  crates[1].slots[20].set_sxy( 4, 2,-22,  2);
  crates[1].slots[19].set_sxy(15, 2,-21,  2);
  crates[1].slots[19].set_sxy(10, 2,-20,  2);
  crates[1].slots[19].set_sxy( 5, 2,-19,  2);
  crates[1].slots[19].set_sxy( 0, 2,-18,  2);
  crates[1].slots[18].set_sxy(11, 2,-17,  2);
  crates[1].slots[18].set_sxy( 6, 2,-16,  2);
  crates[1].slots[18].set_sxy( 1, 2,-15,  2);
  crates[1].slots[17].set_sxy(12, 2,-14,  2);
  crates[1].slots[17].set_sxy( 7, 2,-13,  2);
  crates[1].slots[17].set_sxy( 2, 2,-12,  2);
  crates[1].slots[16].set_sxy(13, 2,-11,  2);
  crates[1].slots[16].set_sxy( 8, 2,-10,  2);
  crates[1].slots[16].set_sxy( 4, 2, -9,  2);
  crates[1].slots[16].set_sxy( 0, 2, -8,  2);
  crates[1].slots[15].set_sxy(12, 2, -7,  2);
  crates[1].slots[15].set_sxy( 8, 2, -6,  2);
  crates[1].slots[15].set_sxy( 4, 2, -5,  2);
  crates[1].slots[15].set_sxy( 0, 2, -4,  2);
  crates[1].slots[14].set_sxy(12, 2, -3,  2);
  crates[1].slots[14].set_sxy( 8, 2, -2,  2);
  crates[1].slots[14].set_sxy( 4, 2, -1,  2);
  crates[1].slots[ 9].set_sxy(15, 2,  1,  2);
  crates[1].slots[ 9].set_sxy(10, 2,  2,  2);
  crates[1].slots[ 9].set_sxy( 5, 2,  3,  2);
  crates[1].slots[ 9].set_sxy( 0, 2,  4,  2);
  crates[1].slots[ 8].set_sxy(11, 2,  5,  2);
  crates[1].slots[ 8].set_sxy( 6, 2,  6,  2);
  crates[1].slots[ 8].set_sxy( 1, 2,  7,  2);
  crates[1].slots[ 7].set_sxy(12, 2,  8,  2);
  crates[1].slots[ 7].set_sxy( 7, 2,  9,  2);
  crates[1].slots[ 7].set_sxy( 1, 2, 10,  2);
  crates[1].slots[ 6].set_sxy(13, 2, 11,  2);
  crates[1].slots[ 6].set_sxy( 8, 2, 12,  2);
  crates[1].slots[ 6].set_sxy( 3, 2, 13,  2);
  crates[1].slots[ 5].set_sxy(14, 2, 14,  2);
  crates[1].slots[ 5].set_sxy( 9, 2, 15,  2);
  crates[1].slots[ 5].set_sxy( 4, 2, 16,  2);
  crates[1].slots[ 4].set_sxy(15, 2, 17,  2);
  crates[1].slots[ 4].set_sxy(10, 2, 18,  2);
  crates[1].slots[ 4].set_sxy( 5, 2, 19,  2);
  crates[1].slots[ 4].set_sxy( 0, 2, 20,  2);
  crates[1].slots[ 3].set_sxy(11, 2, 21,  2);
  crates[1].slots[ 3].set_sxy( 6, 2, 22,  2);
  crates[1].slots[ 3].set_sxy( 1, 2, 23,  2);
  crates[1].slots[20].set_sxy( 8, 2,-23,  1);
  crates[1].slots[20].set_sxy( 3, 2,-22,  1);
  crates[1].slots[19].set_sxy(14, 2,-21,  1);
  crates[1].slots[19].set_sxy( 9, 2,-20,  1);
  crates[1].slots[19].set_sxy( 4, 2,-19,  1);
  crates[1].slots[18].set_sxy(15, 2,-18,  1);
  crates[1].slots[18].set_sxy(10, 2,-17,  1);
  crates[1].slots[18].set_sxy( 5, 2,-16,  1);
  crates[1].slots[18].set_sxy( 0, 2,-15,  1);
  crates[1].slots[17].set_sxy(11, 2,-14,  1);
  crates[1].slots[17].set_sxy( 6, 2,-13,  1);
  crates[1].slots[17].set_sxy( 1, 2,-12,  1);
  crates[1].slots[16].set_sxy(12, 2,-11,  1);
  crates[1].slots[14].set_sxy( 3, 2, -1,  1);
  crates[1].slots[ 9].set_sxy(14, 2,  1,  1);
  crates[1].slots[ 9].set_sxy( 9, 2,  2,  1);
  crates[1].slots[ 9].set_sxy( 4, 2,  3,  1);
  crates[1].slots[ 8].set_sxy(15, 2,  4,  1);
  crates[1].slots[ 8].set_sxy(10, 2,  5,  1);
  crates[1].slots[ 8].set_sxy( 5, 2,  6,  1);
  crates[1].slots[ 8].set_sxy( 0, 2,  7,  1);
  crates[1].slots[ 7].set_sxy(11, 2,  8,  1);
  crates[1].slots[ 7].set_sxy( 6, 2,  9,  1);
  crates[1].slots[ 7].set_sxy( 2, 2, 10,  1);
  crates[1].slots[ 6].set_sxy(12, 2, 11,  1);
  crates[1].slots[ 6].set_sxy( 7, 2, 12,  1);
  crates[1].slots[ 6].set_sxy( 2, 2, 13,  1);
  crates[1].slots[ 5].set_sxy(13, 2, 14,  1);
  crates[1].slots[ 5].set_sxy( 8, 2, 15,  1);
  crates[1].slots[ 5].set_sxy( 3, 2, 16,  1);
  crates[1].slots[ 4].set_sxy(14, 2, 17,  1);
  crates[1].slots[ 4].set_sxy( 9, 2, 18,  1);
  crates[1].slots[ 4].set_sxy( 4, 2, 19,  1);
  crates[1].slots[ 3].set_sxy(15, 2, 20,  1);
  crates[1].slots[ 3].set_sxy(10, 2, 21,  1);
  crates[1].slots[ 3].set_sxy( 5, 2, 22,  1);
  crates[1].slots[ 3].set_sxy( 0, 2, 23,  1);
  crates[2].slots[20].set_sxy( 8, 2,-23, -1);
  crates[2].slots[20].set_sxy( 3, 2,-22, -1);
  crates[2].slots[19].set_sxy(14, 2,-21, -1);
  crates[2].slots[19].set_sxy( 9, 2,-20, -1);
  crates[2].slots[19].set_sxy( 4, 2,-19, -1);
  crates[2].slots[18].set_sxy(15, 2,-18, -1);
  crates[2].slots[18].set_sxy(10, 2,-17, -1);
  crates[2].slots[18].set_sxy( 5, 2,-16, -1);
  crates[2].slots[18].set_sxy( 0, 2,-15, -1);
  crates[2].slots[17].set_sxy(11, 2,-14, -1);
  crates[2].slots[17].set_sxy( 6, 2,-13, -1);
  crates[2].slots[17].set_sxy( 1, 2,-12, -1);
  crates[2].slots[16].set_sxy(12, 2,-11, -1);
  crates[2].slots[14].set_sxy( 3, 2, -1, -1);
  crates[2].slots[ 9].set_sxy(14, 2,  1, -1);
  crates[2].slots[ 9].set_sxy( 9, 2,  2, -1);
  crates[2].slots[ 9].set_sxy( 4, 2,  3, -1);
  crates[2].slots[ 8].set_sxy(15, 2,  4, -1);
  crates[2].slots[ 8].set_sxy(10, 2,  5, -1);
  crates[2].slots[ 8].set_sxy( 5, 2,  6, -1);
  crates[2].slots[ 8].set_sxy( 0, 2,  7, -1);
  crates[2].slots[ 7].set_sxy(11, 2,  8, -1);
  crates[2].slots[ 7].set_sxy( 6, 2,  9, -1);
  crates[2].slots[ 7].set_sxy( 1, 2, 10, -1);
  crates[2].slots[ 6].set_sxy(12, 2, 11, -1);
  crates[2].slots[ 6].set_sxy( 7, 2, 12, -1);
  crates[2].slots[ 6].set_sxy( 2, 2, 13, -1);
  crates[2].slots[ 5].set_sxy(13, 2, 14, -1);
  crates[2].slots[ 5].set_sxy( 8, 2, 15, -1);
  crates[2].slots[ 5].set_sxy( 3, 2, 16, -1);
  crates[2].slots[ 4].set_sxy(14, 2, 17, -1);
  crates[2].slots[ 4].set_sxy( 9, 2, 18, -1);
  crates[2].slots[ 4].set_sxy( 4, 2, 19, -1);
  crates[2].slots[ 3].set_sxy(15, 2, 20, -1);
  crates[2].slots[ 3].set_sxy(10, 2, 21, -1);
  crates[2].slots[ 3].set_sxy( 5, 2, 22, -1);
  crates[2].slots[ 3].set_sxy( 0, 2, 23, -1);
  crates[2].slots[20].set_sxy( 9, 2,-23, -2);
  crates[2].slots[20].set_sxy( 4, 2,-22, -2);
  crates[2].slots[19].set_sxy(15, 2,-21, -2);
  crates[2].slots[19].set_sxy(10, 2,-20, -2);
  crates[2].slots[19].set_sxy( 5, 2,-19, -2);
  crates[2].slots[19].set_sxy( 0, 2,-18, -2);
  crates[2].slots[18].set_sxy(11, 2,-17, -2);
  crates[2].slots[18].set_sxy( 6, 2,-16, -2);
  crates[2].slots[18].set_sxy( 1, 2,-15, -2);
  crates[2].slots[17].set_sxy(12, 2,-14, -2);
  crates[2].slots[17].set_sxy( 7, 2,-13, -2);
  crates[2].slots[17].set_sxy( 2, 2,-12, -2);
  crates[2].slots[16].set_sxy(13, 2,-11, -2);
  crates[2].slots[16].set_sxy( 8, 2,-10, -2);
  crates[2].slots[16].set_sxy( 4, 2, -9, -2);
  crates[2].slots[16].set_sxy( 0, 2, -8, -2);
  crates[2].slots[15].set_sxy(12, 2, -7, -2);
  crates[2].slots[15].set_sxy( 8, 2, -6, -2);
  crates[2].slots[15].set_sxy( 4, 2, -5, -2);
  crates[2].slots[15].set_sxy( 0, 2, -4, -2);
  crates[2].slots[14].set_sxy(12, 2, -3, -2);
  crates[2].slots[14].set_sxy( 8, 2, -2, -2);
  crates[2].slots[14].set_sxy( 4, 2, -1, -2);
  crates[2].slots[ 9].set_sxy(15, 2,  1, -2);
  crates[2].slots[ 9].set_sxy(10, 2,  2, -2);
  crates[2].slots[ 9].set_sxy( 5, 2,  3, -2);
  crates[2].slots[ 9].set_sxy( 0, 2,  4, -2);
  crates[2].slots[ 8].set_sxy(11, 2,  5, -2);
  crates[2].slots[ 8].set_sxy( 6, 2,  6, -2);
  crates[2].slots[ 8].set_sxy( 1, 2,  7, -2);
  crates[2].slots[ 7].set_sxy(12, 2,  8, -2);
  crates[2].slots[ 7].set_sxy( 7, 2,  9, -2);
  crates[2].slots[ 7].set_sxy( 2, 2, 10, -2);
  crates[2].slots[ 6].set_sxy(13, 2, 11, -2);
  crates[2].slots[ 6].set_sxy( 8, 2, 12, -2);
  crates[2].slots[ 6].set_sxy( 3, 2, 13, -2);
  crates[2].slots[ 5].set_sxy(14, 2, 14, -2);
  crates[2].slots[ 5].set_sxy( 9, 2, 15, -2);
  crates[2].slots[ 5].set_sxy( 4, 2, 16, -2);
  crates[2].slots[ 4].set_sxy(15, 2, 17, -2);
  crates[2].slots[ 4].set_sxy(10, 2, 18, -2);
  crates[2].slots[ 4].set_sxy( 5, 2, 19, -2);
  crates[2].slots[ 4].set_sxy( 0, 2, 20, -2);
  crates[2].slots[ 3].set_sxy(11, 2, 21, -2);
  crates[2].slots[ 3].set_sxy( 6, 2, 22, -2);
  crates[2].slots[ 3].set_sxy( 1, 2, 23, -2);
  crates[2].slots[20].set_sxy(10, 2,-23, -3);
  crates[2].slots[20].set_sxy( 5, 2,-22, -3);
  crates[2].slots[20].set_sxy( 0, 2,-21, -3);
  crates[2].slots[19].set_sxy(11, 2,-20, -3);
  crates[2].slots[19].set_sxy( 6, 2,-19, -3);
  crates[2].slots[19].set_sxy( 1, 2,-18, -3);
  crates[2].slots[18].set_sxy(12, 2,-17, -3);
  crates[2].slots[18].set_sxy( 7, 2,-16, -3);
  crates[2].slots[18].set_sxy( 2, 2,-15, -3);
  crates[2].slots[17].set_sxy(13, 2,-14, -3);
  crates[2].slots[17].set_sxy( 8, 2,-13, -3);
  crates[2].slots[17].set_sxy( 3, 2,-12, -3);
  crates[2].slots[16].set_sxy(14, 2,-11, -3);
  crates[2].slots[16].set_sxy( 9, 2,-10, -3);
  crates[2].slots[16].set_sxy( 5, 2, -9, -3);
  crates[2].slots[16].set_sxy( 1, 2, -8, -3);
  crates[2].slots[15].set_sxy(13, 2, -7, -3);
  crates[2].slots[15].set_sxy( 9, 2, -6, -3);
  crates[2].slots[15].set_sxy( 5, 2, -5, -3);
  crates[2].slots[15].set_sxy( 1, 2, -4, -3);
  crates[2].slots[14].set_sxy(13, 2, -3, -3);
  crates[2].slots[14].set_sxy( 9, 2, -2, -3);
  crates[2].slots[14].set_sxy( 5, 2, -1, -3);
  crates[2].slots[14].set_sxy( 0, 2,  1, -3);
  crates[2].slots[ 9].set_sxy(11, 2,  2, -3);
  crates[2].slots[ 9].set_sxy( 6, 2,  3, -3);
  crates[2].slots[ 9].set_sxy( 1, 2,  4, -3);
  crates[2].slots[ 8].set_sxy(12, 2,  5, -3);
  crates[2].slots[ 8].set_sxy( 7, 2,  6, -3);
  crates[2].slots[ 8].set_sxy( 2, 2,  7, -3);
  crates[2].slots[ 7].set_sxy(13, 2,  8, -3);
  crates[2].slots[ 7].set_sxy( 8, 2,  9, -3);
  crates[2].slots[ 7].set_sxy( 3, 2, 10, -3);
  crates[2].slots[ 6].set_sxy(14, 2, 11, -3);
  crates[2].slots[ 6].set_sxy( 9, 2, 12, -3);
  crates[2].slots[ 6].set_sxy( 4, 2, 13, -3);
  crates[2].slots[ 5].set_sxy(15, 2, 14, -3);
  crates[2].slots[ 5].set_sxy(10, 2, 15, -3);
  crates[2].slots[ 5].set_sxy( 5, 2, 16, -3);
  crates[2].slots[ 5].set_sxy( 0, 2, 17, -3);
  crates[2].slots[ 4].set_sxy(11, 2, 18, -3);
  crates[2].slots[ 4].set_sxy( 6, 2, 19, -3);
  crates[2].slots[ 4].set_sxy( 1, 2, 20, -3);
  crates[2].slots[ 3].set_sxy(12, 2, 21, -3);
  crates[2].slots[ 3].set_sxy( 7, 2, 22, -3);
  crates[2].slots[ 3].set_sxy( 2, 2, 23, -3);
  crates[2].slots[20].set_sxy(11, 2,-23, -4);
  crates[2].slots[20].set_sxy( 6, 2,-22, -4);
  crates[2].slots[20].set_sxy( 1, 2,-21, -4);
  crates[2].slots[19].set_sxy(12, 2,-20, -4);
  crates[2].slots[19].set_sxy( 7, 2,-19, -4);
  crates[2].slots[19].set_sxy( 2, 2,-18, -4);
  crates[2].slots[18].set_sxy(13, 2,-17, -4);
  crates[2].slots[18].set_sxy( 8, 2,-16, -4);
  crates[2].slots[18].set_sxy( 3, 2,-15, -4);
  crates[2].slots[17].set_sxy(14, 2,-14, -4);
  crates[2].slots[17].set_sxy( 9, 2,-13, -4);
  crates[2].slots[17].set_sxy( 4, 2,-12, -4);
  crates[2].slots[16].set_sxy(15, 2,-11, -4);
  crates[2].slots[16].set_sxy(10, 2,-10, -4);
  crates[2].slots[16].set_sxy( 6, 2, -9, -4);
  crates[2].slots[16].set_sxy( 2, 2, -8, -4);
  crates[2].slots[15].set_sxy(14, 2, -7, -4);
  crates[2].slots[15].set_sxy(10, 2, -6, -4);
  crates[2].slots[15].set_sxy( 6, 2, -5, -4);
  crates[2].slots[15].set_sxy( 2, 2, -4, -4);
  crates[2].slots[14].set_sxy(14, 2, -3, -4);
  crates[2].slots[14].set_sxy(10, 2, -2, -4);
  crates[2].slots[14].set_sxy( 6, 2, -1, -4);
  crates[2].slots[14].set_sxy( 1, 2,  1, -4);
  crates[2].slots[ 9].set_sxy(12, 2,  2, -4);
  crates[2].slots[ 9].set_sxy( 7, 2,  3, -4);
  crates[2].slots[ 9].set_sxy( 2, 2,  4, -4);
  crates[2].slots[ 8].set_sxy(13, 2,  5, -4);
  crates[2].slots[ 8].set_sxy( 8, 2,  6, -4);
  crates[2].slots[ 8].set_sxy( 3, 2,  7, -4);
  crates[2].slots[ 7].set_sxy(14, 2,  8, -4);
  crates[2].slots[ 7].set_sxy( 9, 2,  9, -4);
  crates[2].slots[ 7].set_sxy( 4, 2, 10, -4);
  crates[2].slots[ 6].set_sxy(15, 2, 11, -4);
  crates[2].slots[ 6].set_sxy(10, 2, 12, -4);
  crates[2].slots[ 6].set_sxy( 5, 2, 13, -4);
  crates[2].slots[ 6].set_sxy( 0, 2, 14, -4);
  crates[2].slots[ 5].set_sxy(11, 2, 15, -4);
  crates[2].slots[ 5].set_sxy( 6, 2, 16, -4);
  crates[2].slots[ 5].set_sxy( 1, 2, 17, -4);
  crates[2].slots[ 4].set_sxy(12, 2, 18, -4);
  crates[2].slots[ 4].set_sxy( 7, 2, 19, -4);
  crates[2].slots[ 4].set_sxy( 2, 2, 20, -4);
  crates[2].slots[ 3].set_sxy(13, 2, 21, -4);
  crates[2].slots[ 3].set_sxy( 8, 2, 22, -4);
  crates[2].slots[ 3].set_sxy( 3, 2, 23, -4);
  crates[2].slots[20].set_sxy(12, 2,-23, -5);
  crates[2].slots[20].set_sxy( 7, 2,-22, -5);
  crates[2].slots[20].set_sxy( 2, 2,-21, -5);
  crates[2].slots[19].set_sxy(13, 2,-20, -5);
  crates[2].slots[19].set_sxy( 8, 2,-19, -5);
  crates[2].slots[19].set_sxy( 3, 2,-18, -5);
  crates[2].slots[18].set_sxy(14, 2,-17, -5);
  crates[2].slots[18].set_sxy( 9, 2,-16, -5);
  crates[2].slots[18].set_sxy( 4, 2,-15, -5);
  crates[2].slots[17].set_sxy(15, 2,-14, -5);
  crates[2].slots[17].set_sxy(10, 2,-13, -5);
  crates[2].slots[17].set_sxy( 5, 2,-12, -5);
  crates[2].slots[17].set_sxy( 0, 2,-11, -5);
  crates[2].slots[16].set_sxy(11, 2,-10, -5);
  crates[2].slots[16].set_sxy( 7, 2, -9, -5);
  crates[2].slots[16].set_sxy( 3, 2, -8, -5);
  crates[2].slots[15].set_sxy(15, 2, -7, -5);
  crates[2].slots[15].set_sxy(11, 2, -6, -5);
  crates[2].slots[15].set_sxy( 7, 2, -5, -5);
  crates[2].slots[15].set_sxy( 3, 2, -4, -5);
  crates[2].slots[14].set_sxy(15, 2, -3, -5);
  crates[2].slots[14].set_sxy(11, 2, -2, -5);
  crates[2].slots[14].set_sxy( 7, 2, -1, -5);
  crates[2].slots[14].set_sxy( 2, 2,  1, -5);
  crates[2].slots[ 9].set_sxy(13, 2,  2, -5);
  crates[2].slots[ 9].set_sxy( 8, 2,  3, -5);
  crates[2].slots[ 9].set_sxy( 3, 2,  4, -5);
  crates[2].slots[ 8].set_sxy(14, 2,  5, -5);
  crates[2].slots[ 8].set_sxy( 9, 2,  6, -5);
  crates[2].slots[ 8].set_sxy( 4, 2,  7, -5);
  crates[2].slots[ 7].set_sxy(15, 2,  8, -5);
  crates[2].slots[ 7].set_sxy(10, 2,  9, -5);
  crates[2].slots[ 7].set_sxy( 5, 2, 10, -5);
  crates[2].slots[ 7].set_sxy( 0, 2, 11, -5);
  crates[2].slots[ 6].set_sxy(11, 2, 12, -5);
  crates[2].slots[ 6].set_sxy( 6, 2, 13, -5);
  crates[2].slots[ 6].set_sxy( 1, 2, 14, -5);
  crates[2].slots[ 5].set_sxy(12, 2, 15, -5);
  crates[2].slots[ 5].set_sxy( 7, 2, 16, -5);
  crates[2].slots[ 5].set_sxy( 2, 2, 17, -5);
  crates[2].slots[ 4].set_sxy(13, 2, 18, -5);
  crates[2].slots[ 4].set_sxy( 8, 2, 19, -5);
  crates[2].slots[ 4].set_sxy( 3, 2, 20, -5);
  crates[2].slots[ 3].set_sxy(14, 2, 21, -5);
  crates[2].slots[ 3].set_sxy( 9, 2, 22, -5);
  crates[2].slots[ 3].set_sxy( 4, 2, 23, -5);

  for(int i=0; i< crates.size(); ++i){
    for(auto s: crates[i].slots ){  // Careful! s is a copy.
      crates[i].slots[s.first].slot = s.first;
    }
  }
}
