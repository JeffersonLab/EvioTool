//
//  Headbank.h
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/18/19.
//  Copyright © 2019 UNH. All rights reserved.
//
// For HPS data the Headbank is a Leaf from the tag=46 bank.
//
// Headbank   tag = 0xe10f (57615)   unsigned int
//  0(31:0)  "version number"
//  1(31:0)  "run number"
//  2(31:0)  "event number"
//  3(31:0)  "event unix time"
//  4(31:0)  "event type"
//  5(31:0)  "roc pattern"
//  6(31:0)  "event classification (17,18,20 etc)"
//  7(31:0)  "trigger bits"
//
//  The above list is according to clonbanks.xml.
//  The actual HPS data varies:
//  For 2015/2016 - Only the first 5 words are written to data.
//  For 2019      - Only the first 6 words are written to data.
//  IF and only if, event_type = 1, the the time is written, otherwise it is 0.
//
#ifndef Headbank_h
#define Headbank_h

#include "EvioTool.h"
#include "Leaf.h"

class Headbank: public Leaf<unsigned int>{
  
public:
  Headbank(){};
  Headbank(Bank *b,unsigned short itag=57615,unsigned short inum=0): Leaf("Headbank",itag,inum,"Headbank data"){
    b->AddThisLeaf(this);
  };
  
  unsigned int GetData(int i){
    if(data.size()>i) return( data[i]);
    else return(0);
  }
  unsigned int GetVersionNumber(){ return(GetData(0));};
  unsigned int GetRunNumber()    { return(GetData(1));};
  unsigned int GetEventNumber()  { return(GetData(2));};
  unsigned int GetTime()         { return(GetData(3));};  // This is 0 in 2019 data.
  unsigned int GetType()         { return(GetData(4));};  // This is 0 in 2019 data.
  unsigned int GetRocPattern()   { return(GetData(5));};  // This is 0 in 2019 data.
  unsigned int GetEvtClass()     { return(GetData(6));};  // This is 0 in 2019 data.
  unsigned int GetTriggerBits()  { return(GetData(7));};  // This is 0 in 2019 data.

  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(Headbank,1);
#pragma clang diagnostic pop
};

#endif /* Headbank_h */
