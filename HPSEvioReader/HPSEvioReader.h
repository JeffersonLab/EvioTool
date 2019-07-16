//
//  Header_LinkDef.h
//  EvioTool
//
//  Created by Maurik Holtrop on 1/17/19.
//  Copyright © 2019 UNH. All rights reserved.
//
//  This tool defines a reader for EVIO data for the HPS runs.
//
//  Example Use (root prompt):
//  R__LOAD_LIBRARY(libHPSEvioReader);
//  HPSEvioReader *et = new HPSEvioReader("hpsnosvt_009317.evio.00000");
//  et->Next();   // Reads the trigger config, since the first event in the file is tag=16
//  for(int i=0;i<10;i++) et->NextNoParse();  // Fast skip 10 events.
//  et->Next();   // Read and parse event.
//  et->PrintBank(10);  // Print event contents.
//  ... do stuff ...
//
#ifndef __HPSEvioReader__
#define __HPSEvioReader__

#include "EvioTool.h"
#include "EcalBank.h"
#include "Header.h"
#include "Headbank.h"
#include "SVTBank.h"
#include "TSBank.h"
#include "VTPBank.h"
#include "TriggerConfig.h"
#include "EcalHit.h"
#include "Cluster.h"
#include "EcalGTPCluster.h"
#include "EcalCluster.h"


class HPSEvioReader: public EvioTool{
  
public:
  Header   *Head = nullptr;
  Bank     *TrigCrate = nullptr;
  Headbank *TrigHead  = nullptr;
  TSBank   *Trigger   = nullptr;
  Bank     *ECALCrate = nullptr;
  Leaf<FADCdata> *FADC= nullptr;
  EcalBank *ECAL      = nullptr;
  SVTBank  *SVT       = nullptr;
  Bank     *TrigTop   = nullptr;
  Bank     *TrigBot   = nullptr;
  VTPBank  *VtpTop    = nullptr;
  VTPBank  *VtpBot    = nullptr;
  
  TriggerConfig *TrigConf;
  
public:
  HPSEvioReader(string infile="",string trigfile="",int dataset=2019);
  virtual ~HPSEvioReader(){};
  
  virtual int Next(void);
  
  void Set2019Data();  // Setup for 2019 data.
  void Set2016Data();  // Setup for 2015/16 data.
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(HPSEvioReader,1);
#pragma clang diagnostic pop

};


#endif /* __HPSEvioReader__ */
