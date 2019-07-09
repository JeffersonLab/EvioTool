//
//  EcalBank.hpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/25/19.
//
// This class will take the FADC250 data and interpret the channels for the ECAL.
// Since the FADC250 data contains more than just the ECAL information, we do NOT derive from Leaf<FADCdata>, but
// instead take and strore a pointer to a Leaf<FADCdata>. This means that the Leaf<FADCdata> needs to be instantiated elsewhere.
//
// Example:
// EvioTool *et= new EvioTool(infile);
// auto ECALbank = et->AddBank("ECAL",{37,39},0,"Ecal banks");
// auto FADC     = ECALbank->AddLeaf<FADCdata>("FADC",57601,0,"FADC mode 1 data");
// auto ECAL  = new EcalData(FADC);
//
#ifndef EcalData_h
#define EcalData_h

#include "EvioTool.h"
#include "FADCdata.h"
#include "TriggerConfig.h"
#include "Leaf.h"
#include "EcalGTPCluster.h"
#include "EcalCluster.h"
#include <unordered_map>


using EcalHitMap_t=std::map<Ecal_point_t, EcalHit_t >;

class EcalData: public TObject {
  
public:
  Leaf<FADCdata> *FADC_leaf;
  TriggerConfig  *DAQconfig;    // Pointer to the configuration setup. Gets DAQMAP.
  
  bool mode3_amplitude=false;
  bool mode7_amplitude=true;
  bool three_pole_fit =false;
  
  EcalHitMap_t hitmap;
  
  vector<EcalGTPCluster>  GTPClusters;
  vector<EcalCluster>     Clusters;
  
  bool is_configured = false;
  int cluster_seed_thresh=100;
  int cluster_hit_dt = 16;
  
public:
  EcalData(){};
  EcalData(Leaf<FADCdata> *FADC,TriggerConfig *conf):FADC_leaf(FADC),DAQconfig(conf){}; //Cannot call Config at creation, because TriggerConfig is not yet read.
  
  void Config(void){
    cluster_seed_thresh=(int)DAQconfig->vtp_other["ECAL_CLUSTER_SEED_THR"][0];
    cluster_hit_dt=(int)DAQconfig->vtp_other["ECAL_CLUSTER_HIT_DT"][0];
    is_configured = true;
  }
  void Parse(void);
  void FindGTPClusters(void);
  
  unsigned int ToDaqId(int crate,int slot,int channel){
    // Encode the (crate,slot,channel) to a DAQ id.
    // Crate: 37 -> 1, 39->2; slot is [0,24], channel is [0,15].
    if(crate == 37) crate =1;
    if(crate == 39) crate =2;
    return( ((crate&0x0F)<<16) + ((slot&0xFF)<<8) + (channel&0x0F));
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(EcalData,1);
#pragma clang diagnostic pop
};

#endif /* EcalBank_h */
