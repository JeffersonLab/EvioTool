//
//  TSBank.hpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/18/19.
//  Copyright © 2019 UNH. All rights reserved.
//
//  TS Hardware Data" tag="0xe10A" (57610) type="uint32">
//  Only the 0xE10A bank associated with bank tag=46 has the full information about the trigger.

//  For the 2019 data, this bank will have 7 words in the tag="46" bank.
//  The other occurances are truncated with only 4 words.
//
//  In the 2015/2016 data, this bank is quite different and has completely different meaning.
//  The 2015/16 data, the size in the tag="46" bank is 5 words, and 4 words in the other banks.
//
//  NOTE: This class uses the size of the bank to determine if it is 2019 or 2015/16 data. That assumes you are
//        only using it from the tag="46" ROCID bank.
//
//  Data meaning for the 2019 RUN.
//  0: (Event word 1 = Header )
//    Bit(31:24): Trigger Type;
//    0: filler events,
//    1-32: GTP Physics trigger;
//    33-64: Front panel physics trigger;
//    250: multi-hit on GTP or Front panel physics trigger 251: Multi-hit on GTP and Front panel physics trigger 253: VME trigger;
//    254: VME random trigger;
//    For TImaster bit(31:26) represents the TS#6-1 inputs;
//    Bit(23:16): 0000,0001, or 0x01;
//    Bit(15:0): Event wordcount; Event header is excluded from the count
//
//   1: (Event word 2) = Trigger number (low 32 bits) = Event number.
//   2: (Event word 3) = Trigger timing 4ns steps (low 32 bits)
//   3: (Event word 4) = bits 31:16 - high 16 bits of trigger number, bits 15:0 - high 16 bits of trigger timing.
//   4: (Event word 5) = VTP Trigger bits AFTER prescaling is applied.
//   5: (Event word 6) = EXT Trigger bits AFTER prescaling is applied.
//   6: (Event word 7) = bits 31:16 EXT, bits 15:0  VTP trigger bits before prescaling is applied.
//
// The trigger bits are defined in the trigger configuration file, VTP_HPS_PRESCALE. This is also where they are set.
// You can read it back from the EVIO file in the TrigConfig.
//
// VTP_HPS_PRESCALE               0        0   # Single 0 Top    ( 150-8191) MeV (-31,31)   Low energy cluster
// VTP_HPS_PRESCALE               1        0   # Single 1 Top    ( 300-3000) MeV (  5,31)   e+
// VTP_HPS_PRESCALE               2        0   # Single 2 Top    ( 300-3000) MeV (  5,31)   e+ : Position dependent energy cut
// VTP_HPS_PRESCALE               3        0   # Single 3 Top    ( 300-3000) MeV (  5,31)   e+ : HODO L1*L2  Match with cluster
// VTP_HPS_PRESCALE               4        0   # Single 0 Bot    ( 150-8191) MeV (-31,31)   Low energy cluster
// VTP_HPS_PRESCALE               5        0   # Single 1 Bot    ( 300-3000) MeV (  5,31)   e+
// VTP_HPS_PRESCALE               6        0   # Single 2 Bot    ( 300-3000) MeV (  5,31)   e+ : Position dependent energy cut
// VTP_HPS_PRESCALE               7        0   # Single 3 Bot    ( 300-3000) MeV (  5,31)   e+ : HODO L1*L2  Match with cluster
// VTP_HPS_PRESCALE               8        0   # Pair 0          A'
// VTP_HPS_PRESCALE               9        0   # Pair 1          Moller
// VTP_HPS_PRESCALE               10       0   # Pair 2          pi0
// VTP_HPS_PRESCALE               11       0   # Pair 3          -
// VTP_HPS_PRESCALE               12       0   # LED
// VTP_HPS_PRESCALE               13       0   # Cosmic
// VTP_HPS_PRESCALE               14       0   # Hodoscope
// VTP_HPS_PRESCALE               15       0   # Pulser
// VTP_HPS_PRESCALE               16       0   # Multiplicity-0 2 Cluster Trigger
// VTP_HPS_PRESCALE               17       0   # Multiplicity-1 3 Cluster trigger
// VTP_HPS_PRESCALE               18       0   # FEE Top       ( 2600-5200)
// VTP_HPS_PRESCALE               19       0   # FEE Bot       ( 2600-5200)
//
#ifndef TSBank_h
#define TSBank_h

#include "EvioTool.h"
#include "Leaf.h"

class TSBank: public Leaf<unsigned int>{
  
  
public:
  struct TriggerBits {
    bool Single_0_Top: 1; //  0   ( 150-8191) MeV (-31,31)   Low energy cluster
    bool Single_1_Top: 1; //  1   ( 300-3000) MeV (  5,31)   e+
    bool Single_2_Top: 1; //  2   ( 300-3000) MeV (  5,31)   e+ : Position dependent energy cut
    bool Single_3_Top: 1; //  3   ( 300-3000) MeV (  5,31)   e+ : HODO L1*L2  Match with cluster
    bool Single_0_Bot: 1; //  4   ( 150-8191) MeV (-31,31)   Low energy cluster
    bool Single_1_Bot: 1; //  5   ( 300-3000) MeV (  5,31)   e+
    bool Single_2_Bot: 1; //  6   ( 300-3000) MeV (  5,31)   e+ : Position dependent energy cut
    bool Single_3_Bot: 1; //  7   ( 300-3000) MeV (  5,31)   e+ : HODO L1*L2  Match with cluster
    bool Pair_0      : 1; //  8    A'
    bool Pair_1      : 1; //  9    Moller
    bool Pair_2      : 1; // 10    pi0
    bool Pair_3      : 1; // 11    -
    bool LED         : 1; // 12    LED
    bool Cosmic      : 1; // 13    Cosmic
    bool Hodoscope   : 1; // 14    Hodoscope
    bool Pulser      : 1; // 15    Pulser
    bool Mult_0      : 1; // 16    Multiplicity-0 2 Cluster Trigger
    bool Mult_1      : 1; // 17    Multiplicity-1 3 Cluster trigger
    bool FEE_Top     : 1; // 18    FEE Top       ( 2600-5200)
    bool FEE_Bot     : 1; // 19    FEE Bot       ( 2600-5200)
    unsigned int NA  :12; // 20-31 Not used
  };
  
public:
  TSBank(){};
  TSBank(Bank *b,unsigned short itag=57610,unsigned short inum=0): Leaf("TSBank",itag,inum,"TSBank data"){
    b->AddThisLeaf(this);
  };
  
  unsigned int GetData(int i){
    if(data.size()==7 && i<7) return( data[i]);
    else return(0);
  }

  TriggerBits GetTriggerBits(){
    if(data.size()==7 ){
      TriggerBits *bits = reinterpret_cast<TriggerBits *>(&data[4]);
      return(*bits);
    }else if(data.size() == 5){
      TriggerBits bits;
      
      return(bits);
    }else{
      TriggerBits tmp;
      return(tmp);
      std::cerr << "GetTriggerBits:: Cannot get you those bits.\n";
      
    }
  }
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(TSBank,1);
#pragma clang diagnostic pop
};

#endif /* TSBank_h */
