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
#include "EcalData.h"
#include "Header.h"
#include "Headbank.h"
#include "SVTbank.h"
#include "TIData.h"
#include "VTPData.h"
#include "TriggerConfig.h"
#include "EcalHit.h"
#include "Cluster.h"
#include "EcalGTPCluster.h"
#include "EcalCluster.h"
#include "EcalData.h"


class HPSEvioReader: public EvioTool{
  
public:
  Header   *head;
  Bank     *trig;
  Headbank *trighead;
  TIData   *tidata;
  Bank     *ECALbank;
  Leaf<FADCdata> *FADC;
  EcalData *ECAL;
  SVTbank  *SVT;
  Bank     *TrigTop;
  Bank     *TrigBot;
  VTPData  *VtpTop;
  VTPData  *VtpBot;
  
  TriggerConfig *TrigConf;
  
public:
  HPSEvioReader(string infile="",string trigfile="");
  virtual ~HPSEvioReader(){};
  
  virtual int Next(void);
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(HPSEvioReader,1);
#pragma clang diagnostic pop

};


#endif /* __HPSEvioReader__ */
