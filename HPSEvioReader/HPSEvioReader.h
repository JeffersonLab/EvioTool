//
//  Header_LinkDef.h
//  EvioTool
//
//  Created by Maurik Holtrop on 1/17/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#ifndef __HPSEvioReader__
#define __HPSEvioReader__

#include "EvioTool.h"
#include "SVTbank.h"
#include "Header.h"
#include "Headbank.h"
#include "TIData.h"
#include "VTPData.h"

class HPSEvioReader: public EvioTool{
  
public:
  Header   *head;
  Bank     *trig;
  Headbank *trighead;
  TIData   *tidata;
  Bank     *ECAL;
  Leaf<FADCdata> *FADC;
  SVTbank  *SVT;
  Bank     *TrigTop;
  Bank     *TrigBot;
  VTPData  *VtpTop;
  VTPData  *VtpBot;
  
public:
  HPSEvioReader(string infile="");
  virtual ~HPSEvioReader(){};
  
  
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(HPSEvioReader,1);
#pragma clang diagnostic pop

};


#endif /* __HPSEvioReader__ */
