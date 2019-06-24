//
//  TIData.hpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/18/19.
//  Copyright © 2019 UNH. All rights reserved.
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
  unsigned int GetTime()         { return(GetData(3));};
  unsigned int GetType()         { return(GetData(4));};
  unsigned int GetRocPattern()   { return(GetData(5));};
  unsigned int GetEvtClass()     { return(GetData(6));};
  unsigned int GetTriggerBits()  { return(GetData(7));};

  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(Headbank,1);
#pragma clang diagnostic pop
};

#endif /* Headbank_h */
