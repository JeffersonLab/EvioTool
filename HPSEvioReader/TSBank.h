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
#include <bitset>

class TSBank: public Leaf<unsigned int>{
  
  
public:
  
  // Note on using a bitwiae structure:
  // You cannot safely cast between the bitwise structure and an unsigned int.
  // So:
  //   TriggerBits_t my_bits;
  //   unsigned int *my_int = (unsigned_int *)my_bits;
  //   my_bits.FEE_Top = true;
  //   cout << my_int << endl;
  // You number that is printed will be DIFFERENT depending on whether the code was optimized or not!!!
  // The reason for this is memory alignmment, I think.
  //
  
  static map<std::string,unsigned char> TriggerNames;
  static map<std::string,unsigned char> TriggerNames2021;
  
  struct TriggerBits_t {  // Trigger structure for the 2019 data set.
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
    bool Pair_3      : 1; // 11    muon
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
  
  typedef union{
    unsigned int intval;
    TriggerBits_t  bits;
  } TriggerBits;
  
public:
  TSBank(){};
  TSBank(Bank *b,unsigned short itag=57610,unsigned short inum=0): Leaf("TSBank",itag,inum,"TSBank data"){
    b->AddThisLeaf(this);
  };
  
  unsigned int GetData(int i){
    if( i < data.size()) return( data[i]);
    else return(0);
  }

  bool IsPulser(void){
    // Return true if the current event is a pulser or FCup event.
    // There are 2 ways that a pulser event can be created: Front Panel and VTP.
    // If Front Panel, then bit-15 is set on data[5].
    // If VTP        , then bit-15 is set on data[4].
    if( data.size() == 5 ) return(data[0] & (1<<29));
    if( data.size() == 7 ) if( data[4] & (1<<15)) return(true);
    return(false);
    }

  bool IsFCup(void){
    // Return true if the current event is a pulser or FCup event.
    // There are 2 ways that a pulser event can be created: Front Panel and VTP.
    // If Front Panel, then bit-15 is set on data[5].
    // If VTP        , then bit-15 is set on data[4].
    if( data.size() == 5 ) return(data[0] & (1<<29));
    if( data.size() == 7 ) if( data[5] & (1<<15)) return(true);
    return(false);
  }

  bool IsRandom(void){
      // Return true if the current event is a pulser or FCup event.
      // There are 2 ways that a pulser event can be created: Front Panel and VTP.
      // If Front Panel, then bit-15 is set on data[5].
      // If VTP        , then bit-15 is set on data[4].
      if( data.size() == 5 ) return(data[0] & (1<<29));
      if( data.size() == 7 ) if( (data[4] & (1<<15)) || (data[5] & (1<<15))) return(true);
      return(false);
  }
  
  bool IsSingle0(void){
    // Return true if it is a Single0-Top or Bottom.
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Single_0_Top || tbits.bits.Single_0_Bot );
  }

  bool IsSingle1(void){
    // Return true if it is a Single1-Top or Bottom.
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Single_1_Top || tbits.bits.Single_1_Bot );
  }

  bool IsSingle2(void){
    // Return true if it is a Single2-Top or Bottom.
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Single_2_Top || tbits.bits.Single_2_Bot );
  }

  bool IsSingle3(void){
    // Return true if it is a Single3-Top or Bottom.
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Single_3_Top || tbits.bits.Single_3_Bot );
  }

  bool IsPair0(void){
    // Return true if it is a Pair0
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Pair_0 );
  }

  bool IsPair1(void){
    // Return true if it is a Pair1
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Pair_1 );
  }

  bool IsPair2(void){
    // Return true if it is a Pair2
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Pair_2 );
  }
  
  bool IsPair3(void){
    // Return true if it is a Pair3
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Pair_3 );
  }

  bool IsLED(void){
    // Return true if it is a LED trigger
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.LED );
  }

  bool IsCosmic(void){
    // Return true if it is a LED trigger
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Cosmic );
  }

  
  bool IsHodoscope(void){
    // Return true if it is a Hodoscope trigger
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Hodoscope );
  }

  bool IsMult0(void){
    // Return true if it is a Multi 0 trigger
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.Mult_0 );
  }

 bool IsMult1(void){
   // Return true if it is a Multi 1 trigger
   TriggerBits tbits = GetTriggerBits();
   return( tbits.bits.Mult_1);
 }

  bool IsFEE(void){
    // Return true if it is an FEE Top or Bottom trigger
    TriggerBits tbits = GetTriggerBits();
    return( tbits.bits.FEE_Bot || tbits.bits.FEE_Top );
  }

  bool IsExactTrigger(TriggerBits test){
    // Return true is the trigger bit pattern is exactly the same as in test.
    unsigned int trig = GetTriggerInt();
    //    std::cout << "trig: " << trig << " test: " << test.intval << " istrue: " << (trig & test.intval )  <<  endl;
    return( trig == test.intval);
  }

  bool IsTrigger(TriggerBits test){
    // Return true is the trigger bits in test are set.
    unsigned int trig = GetTriggerInt();
//    std::cout << "trig: " << trig << " test: " << test.intval << " istrue: " << (trig & test.intval )  <<  endl;
    return( trig & test.intval);
  }
  
  bool IsTrigger(std::string name){
    // Return true if the named bit in name is set.
    unsigned int trig = GetTriggerInt();
    unsigned char bits = TriggerNames[name];
    return( trig & (1<<bits));
  }
  
  unsigned int GetTriggerInt(bool prescaled=true){
    // Return the integer that contains the standard trigger bits.
    // For 2019 data, that is data[4] ("word 5") which has the post-prescale trigger bits.
    // If prescaled==false then data[6] is returned, which are the pre-prescale trigger bits.
    // This is useful for the pulser trigger.
    // The high 16 bits (bits 31:16 ) contain the front board trigger. 0x8000 is the pulser.
    // The low  16 bits (bits 15:0  ) contain the first 16 trigger bits, not prescaled.
    //
    // For 2015/16 data, that is data[0], which contains the trigger bits in the bits 29:24.
    
    if(data.size()==7 ){
      if(prescaled) return(data[4]);
      else          return(data[6]);
    }else if(data.size() == 5){
      TriggerBits trbits = GetTriggerBits();
      return(trbits.intval);
    }
    return(0);
  }
  
  unsigned int GetExtTriggerInt(){
    if(data.size()==7) return( data[5] );
    return(0);
  }
  
  TriggerBits GetTriggerBits(bool prescaled=true){
    // This fills the TriggerBits structure with the bits from data[4] (prescaled) or data[6] (not prescaled)
    // Note that this will only give you the VTP internal trigger status, not the front panel triggers.
    //
    if(data.size()==7 ){
      TriggerBits *bits;
      if(prescaled) bits = reinterpret_cast<TriggerBits *>(&data[4]);
      else          bits = reinterpret_cast<TriggerBits *>(&data[6]);
      return(*bits);
    }else if(data.size() == 5){
      TriggerBits trigb;
      trigb.bits.Single_0_Bot = (data[0] & (1<<24));
      trigb.bits.Single_0_Top = trigb.bits.Single_0_Bot;
      trigb.bits.Single_1_Bot = (data[0] & (1<<25));
      trigb.bits.Single_1_Top = trigb.bits.Single_1_Bot;
      trigb.bits.Pair_0       = (data[0] & (1<<26));
      trigb.bits.Pair_1       = (data[0] & (1<<27));
      trigb.bits.Cosmic       = (data[0] & (1<<28));
      trigb.bits.Pulser       = (data[0] & (1<<29));
      return(trigb);
    }else{
      TriggerBits tmp;
      tmp.intval = 0;
      return(tmp);
    }
  }
  
  std::string GetTriggerName(unsigned char num){
    // Return the name of a trigger given the bit number.
    // I.e. do a reverse lookup on the TriggerNames map.
    auto it=std::find_if(TriggerNames.begin(),TriggerNames.end(),[&num]( const std::pair<std::string,int> &p ){return(p.second==num);});
    if(it != TriggerNames.end()){
      return(it->first);
    }
    return("NA");
  }
  
  unsigned long GetTime(){
    // Return the Trigger time in units of ns as an integer.
    if(data.size()>4){
      unsigned long time = static_cast<unsigned long>(data[2]) + ( (static_cast<unsigned long>(data[3]&0xFFFF)<<32));
      return(time*4);
    } else{
      return(0);
    }
  }
  
  unsigned long GetTriggerNumber(){
    // Return the trigger number ~= event number
    if(data.size()>4){
      unsigned long trignum = static_cast<unsigned long>(data[1]) + ( (static_cast<unsigned long>(data[3]&0xFFFF0000)<<16));
      return(trignum);
    }
    return(0);
  }
  
  void Print(Option_t *opt){
    // Print some information about the the trigger.
    int trig=GetTriggerInt();
    std::bitset<24> trbits = trig;
    printf("Trigger:  int = %7d  hex=%04x  bits: ",trig,trig);
    std::cout << trbits << std::endl;
    for(int i=0;i<24;i++){
      if( trig & (1<<i) ){
        cout << GetTriggerName(i) << ", ";
      }
    }
    std::cout << std::endl;
  }
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(TSBank,1);
#pragma clang diagnostic pop
};

#endif /* TSBank_h */
