//
//  TIBank.hpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/18/19.
//  Copyright © 2019 UNH. All rights reserved.
//
//  TI Hadrware Data" tag="0xe10A" (57610) type="uint32">
//
//0(31:27)=0x10+0x00 "BLOCK_HEADER_FIRST_WORD"
//0(26:22)        "SLOT_NUMBER"
//0(21:18)        0
//0(17:08)        "BLOCK_NUMBER"
//0(07:00)        "BLOCK_LEVEL"
//
//0(31:17)=0x7F88 "BLOCK_HEADER_SECOND_WORD"
//0(16:16)        1-have time stamp, 0-no time stamp
//0(15:08)        0x20
//0(07:00)        "BLOCK_LEVEL"
//
//0(31:24)        "EVENT_TYPE"
//0(23:16)        0X0F
//0(15:00)        "EVENT_WORD_COUNT"
//
//0(31:00)        "EVENT_NUMBER_LOW_32_BITS"
//
//0(31:00)        "TIME_STAMP_LOW_32_BITS"
//
//(optional)   0(31:16)        "EVENT_NUMBER_HIGH_16_BITS"
//0(15:00)        "TIME_STAMP_HIGH_16_BITS"
//
//
//0(31:27)=0x10+0x01 "BLOCK_TRAILER"
//0(26:22)        "SLOTID"
//0(21:0)         "BLOCK_WORD_COUNT"
//
//0(31:27)=0x10+0x0E "DNV"
//
//0(31:27)=0x10+0x0F "FILLER"
//0(26:22)        "SLOT_NUMBER"
//0(21:00)        "BLOCK_NUMBER", or '0011110001000100010000' for 128-bit alignment



#ifndef TIBank_h
#define TIBank_h

#include "EvioTool.h"
#include "Leaf.h"

class TIBank: public Leaf<unsigned int>{
  
public:
  TIBank(){};
  TIBank(Bank *b,unsigned short itag=57615,unsigned short inum=0): Leaf("TIBank",itag,inum,"TIBank data"){
    b->AddThisLeaf(this);
  };
  
  unsigned int GetData(int i){
    if(data.size()==8) return( data[i]);
    else return(0);
  }
  unsigned int GetVersionNumber(){ return(GetData(0));};
  unsigned int GetRunNumber()    { return(GetData(1));};
  unsigned int GetEventNumber()  { return(GetData(2));};
//  unsigned int GetTime()         { return(GetData(3));};
//  unsigned int GetType()         { return(GetData(4));};
//  unsigned int GetRocPattern()   { return(GetData(5));};

  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(TIBank,1);
#pragma clang diagnostic pop
};

#endif /* TIBank_h */
