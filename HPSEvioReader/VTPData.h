//
//  VTPData.hpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/19/19.
//  Copyright © 2019 UNH. All rights reserved.
//
// bit31 = 1 - Data defining word
// bit31 = 0 - Data continuation word.
//
// For bit31 = 1, bit 30-27 contain the data type:
// Data Types:
// 0 Block Header
// 1 Block Trailer
// 2 Event Header
// 3 Trigger Time
// 12 Expansion (Data SubType)
// 12.2 HPS Cluster
// 12.3 HPS Single Trigger
// 12.4 HPS Pair Trigger
// 12.5 HPS Calibration Trigger
// 12.6 HPS Cluster Multiplicity Trigger
// 12.7 HPS FEE Trigger
// 14 Data Not Valid (empty module)
// 15 Filler Word (non-data)
//
//
// 0(31:27)=0x10+0x00 "BLKHDR"
// 0(26:22)        "SLOTID"
// 0(17:08)        "BLOCK_NUMBER"
// 0(07:00)        "BLOCK_LEVEL"
//
// 0(31:27)=0x10+0x01 "BLKTLR"
// 0(26:22)        "SLOTID"
// 0(21:0)         "NWORDS"
//
// 0(31:27)=0x10+0x02 "EVTHDR"
// 0(26:0)         "EVENTNUM"
//
// 0(31:27)=0x10+0x03 "TRGTIME"
// 0(23:0)         "TIMEL"
// 1(23:0)         "TIMEH"
//
// 0(31:27)=0x10+0x04 "EC_PEAK"
// 0(26:26)        "PEAK_INST"
// 0(25:24)        "PEAK_VIEW"
// 0(23:16)        "PEAK_TIME"
// 1(25:16)        "PEAK_COORD"
// 1(15:00)        "PEAK_ENERGY"
//
// 0(31:27)=0x10+0x05 "EC_CLUSTER"
// 0(26:26)        "CLUSTER_INST"
// 0(23:16)        "CLUSTER_TIME"
// 0(15:00)        "CLUSTER_ENERGY"
// 1(29:20)        "CLUSTER_COORDW"
// 1(19:10)        "CLUSTER_COORDV"
// 1(09:00)        "CLUSTER_COORDU"
//
// 0(31:27)=0x10+0x06 "HTCC_CLUSTER"
// 0(26:16)        "CLUSTER_TIME"
// 1(16:00)        "CLUSTER_MASK_HIGH"
// 2(30:00)        "CLUSTER_MASK_LOW"
//
// 0(31:27)=0x10+0x07 "FT CLUSTER"
// 0(15:15)        "H2TAG"
// 0(14:14)        "H1TAG"
// 0(13,10)        "NHITS"
// 0(9,5)          "COORDY"
// 0(4,0)          "COORDX"
// 1(29,16)        "ENERGY"
// 1(10,0)         "TIME"
//
// 0(31:27)=0x10+0x08 "FTOF_CLUSTER"
// 0(26:16)        "CLUSTER_TIME"
// 1(30:00)        "CLUSTER_MASK_HIGH"
// 2(30:00)        "CLUSTER_MASK_LOW"
//
// 0(31:27)=0x10+0x09 "CTOF_CLUSTER"
// 0(26:16)        "CLUSTER_TIME"
// 1(16:00)        "CLUSTER_MASK_HIGH"
// 2(30:00)        "CLUSTER_MASK_LOW"
//
// 0(31:27)=0x10+0x0A "CND_CLUSTER"
// 0(26:16)        "CLUSTER_TIME"
// 0(09:00)        "CLUSTER_MASK_HIGH"
// 1(30:00)        "CLUSTER_MASK_MIDDLE"
// 2(30:00)        "CLUSTER_MASK_LOW"
//
// 0(31:27)=0x10+0x0B "PCU_CLUSTER"
// 0(26:16)        "CLUSTER_TIME"
// 0(05:00)        "CLUSTER_MASK_HIGH"
// 1(30:00)        "CLUSTER_MASK_MIDDLE"
// 2(30:00)        "CLUSTER_MASK_LOW"
//
// 0(31:23)=0x10+0x0C0 "DC_ROAD"
// 0(22:17)        "SEG_SL"
// 0(11:11)        "ROAD_INBEND"
// 0(10:10)        "ROAD_OUTBEND"
// 0(09:09)        "ROAD_VALID"
// 0(08:00)        "TIME"
// 1(30:00)        "ROAD_FTOF(61:31)"
// 2(30:00)        "ROAD_FTOF(30:0)"
//
// 0(31:23)=0x10+0x0C1 "DC_SEGMENT"
// 0(11:09)        "SUPERLAYER_NUMBER"
// 0(08:00)        "TIME"
// 1(18:00)        "SEGMENT_WIRE(111:93)"
// 2(30:00)        "SEGMENT_WIRE(92:62)"
// 3(30:00)        "SEGMENT_WIRE(61:31)"
// 4(30:00)        "SEGMENT_WIRE(30:0)"
//
// O(31:23)=0x10+0x0C2 "HPS_CLUSTER"
// 0(22:10)        "E"
// 0(09:06)        "Y"
// 0(05:00)        "X"
// 1(13:10)        "N"
// 1(09:00)        "T"
//
// O(31:23)=0x10+0x0C3 "HPS_SINGLE_TRIG"
// 0(22:20)        "INSTANCE"
// 0(19:19)        "TOP_NBOT"
// 0(18:18)        "H_L1L2X_GEOM_PASS"
// 0(17:17)        "H_L1L2_GEOM_PASS"
// 0(16:16)        "H_L2_PASS"
// 0(15:15)        "H_L1_PASS"
// 0(14:14)        "PDE_PASS"
// 0(13:13)        "MINX_PASS"
// 0(12:12)        "NMIN_PASS"
// 0(11:11)        "EMAX_PASS"
// 0(10:10)        "EMIN_PASS"
// 0(09:00)        "T"
//
// O(31:23)=0x10+0x0C4 "HPS_PAIR_TRIG"
// 0(22:20)        "INSTANCE"
// 0(19:14)        "reserved"
// 0(13:13)        "COPLANAR_PASS"
// 0(12:12)        "EDSLOPE_PASS"
// 0(11:11)        "DIFF_PASS"
// 0(10:10)        "SUM_PASS"
// 0(09:00)        "T"
//
// 0(31:23)=0x10+0x0C5 "HPS_CALIB_TRIG"
// 0(22:19)        "CALIB_TYPE"
// 0(18:10)        "CALIB_FLAGS"
// 0(09:00)        "T"
//
// 0(31:23)=0x10+0x0C6 "HPS_MULT_TRIG"
// 0(22:22)        "INSTANCE"
// 0(21:18)        "MULT_TOT"
// 0(17:14)        "MULT_BOT"
// 0(13:10)        "MULT_TOP"
// 0(09:00)        "T"
//
// 0(31:23)=0x10+0x0C7 "HPS_FEE_TRIG"
// 0(22:17)        "reserved"
// 0(16:10)        "REGION_MASK"
// 0(09:00)        "T"
//
// 0(31:27)=0x10+0x0D "TRIGGER"
// 0(26:16)        "TRIG_TIME"
// 0(15:0)         "TRIG_PATTERN(15:0)"
// 1(15:0)         "TRIG_PATTERN(31:16)"
//
// 0(31:27)=0x10+0x0E "DNV"
// 0(31:27)=0x10+0x0F "FILLER"

#ifndef VTPData_h
#define VTPData_h

#include "EvioTool.h"
#include "Leaf.h"

class VTPData: public Leaf<unsigned int>{
  
public:
  struct {
    unsigned int blocklevel:  8; // bit 0-7
    unsigned int  blocknum:  10; // bit 8-17
    unsigned int  nothing:    4; // bit 18-21
    unsigned int  slotid:     5; // bit 22-26
    unsigned int  type:       4; // bit 27-30
    bool          istype:     1; // bit31
  } BlockHeader;
  
  struct {
    unsigned int  nwords:    22; // bit 0-21
    unsigned int  slotid:     5; // bit 22-26
    unsigned int  type:       4; // bit 27-30
    bool          istype:     1; // bit 31
  } BlockTail;
  
  struct {
    unsigned int eventnum:  27;  // bit  0-26
    unsigned int type:       4;  // bit 27-30
    bool         istype:     1;  // bit 31
  } EventHeader;
  
  unsigned long TrigTime;
  
  struct HPSCluster_t {
    int X:    6;        // bit 0-5
    int Y:    4;        // bit 6-9
    unsigned int E: 13; // bit 10-22
    unsigned int subtype:    4;  // bit 23-26
    unsigned int type:       4;  // bit 27-30
    bool         istype:     1;  // bit 31
    unsigned int T:         10;  // bit 0-9
    unsigned int N:          4;  // bit 10-13   Second word
    unsigned int nothing:   18;  // bit 14-31   Second word not used.
  };
  
  vector<HPSCluster_t> clusters;
  
  struct HPSSingleTrig_t {
    unsigned int T:  10;  // bit 0-9 Trigger time in 4ns units referenced from the beginning of the readout window.
    struct {
      bool emin:   1;
      bool emax:   1;
      bool nmin:   1;
      bool xmin:   1;
      bool pose:   1; // position dependent energy thresh.
      bool hodo1c: 1; // hodoscope layer 1 coincidence.
      bool hodo2c: 1; // hodoscope layer 2 coincidence.
      bool hodogeo:1; // hodoscope layer 1 geometry matched to layer 2 geometry
      bool hodoecal:1;// hodoscope layer 1 and 2 geometry mached to ecal cluster x.
    } Pass;           // 9 bits: bit 10-18
    bool topnbot: 1;  //         bit 19
    unsigned int inst:3; // bit 20-22 = single cluster trigger bit instance.
    unsigned int subtype:    4;  // bit 23-26
    unsigned int type:       4;  // bit 27-30
    bool         istype:     1;  // bit 31
  };
  
  vector<HPSSingleTrig_t> singletrigs;
  
  struct HPSPairTrig_t {
    unsigned int T:  10;  // bit 0-9  Trigger time in 4ns units referenced from the beginning of the readout window.
    struct {
      bool clusesum:   1;
      bool clusedif:   1;
      bool eslope:     1;
      bool coplane:   1;
      unsigned int dummy: 5;
    } Pass;           // 9 bits: bit 10-18
    bool topnbot: 1;  //         bit 19 - dummy!
    unsigned int inst:3; // bit 20-22 = single cluster trigger bit instance.
    unsigned int subtype:    4;  // bit 23-26
    unsigned int type:       4;  // bit 27-30
    bool         istype:     1;  // bit 31
  };
  
  vector<HPSPairTrig_t> pairtrigs;
  
  struct HPSCalibTrig_t {
    unsigned int T:  10;  // bit 0-9  Trigger time in 4ns units referenced from the beginning of the readout window.
    unsigned int reserved: 9;
    struct {
      bool cosmic:    1;
      bool led:       1;
      bool hodoscope: 1;
      bool puler:     1;
    } trigtype;
    unsigned int subtype:    4;  // bit 23-26
    unsigned int type:       4;  // bit 27-30
    bool         istype:     1;  // bit 31
  };
  
  vector<HPSCalibTrig_t> calibtrigs;

  struct HPSClusterMult_t {
    unsigned int T:  10;  // bit 0-9  Trigger time in 4ns units referenced from the beginning of the readout window.
    unsigned int multtop:   4;
    unsigned int multbot:   4;
    unsigned int multtot:   4;
    bool         bitinst:   1;
    unsigned int subtype:    4;  // bit 23-26
    unsigned int type:       4;  // bit 27-30
    bool         istype:     1;  // bit 31
  };
  
  vector<HPSClusterMult_t> clustermult;  // Cluster multiplicity.

  struct HPSFEETrig_t {
    unsigned int T:  10;  // bit 0-9  Trigger time in 4ns units referenced from the beginning of the readout window.
    unsigned int region:   7;
    unsigned int reserved:   6;
    unsigned int subtype:    4;  // bit 23-26
    unsigned int type:       4;  // bit 27-30
    bool         istype:     1;  // bit 31
  };
  
  vector<HPSFEETrig_t> feetrigger;  // Cluster multiplicity.

  
public:
  VTPData(){};
  VTPData(Bank *b,unsigned short itag=0xe122,unsigned short inum=0): Leaf("VTPData",itag,inum,"VTPData data"){
    b->AddThisLeaf(this);
  };
  
  unsigned int GetData(int i){
    if(data.size()>i) return( data[i]);
    else return(0);
  }
 
  void Clear(){
    clusters.clear();
    singletrigs.clear();
    pairtrigs.clear();
    calibtrigs.clear();
    clustermult.clear();
    feetrigger.clear();
  };
  void ParseBank();
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(VTPData,1);
#pragma clang diagnostic pop
};


#endif /* VTPData_h */
