//
//  TSBank.cpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 6/18/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#include "TSBank.h"

map<std::string,unsigned char> TSBank::TriggerNames ={
   {"Single_0_Top",0},
   {"Single_1_Top",1},
   {"Single_2_Top",2},
   {"Single_3_Top",3},
   {"Single_0_Bot",4},
   {"Single_1_Bot",5},
   {"Single_2_Bot",6},
   {"Single_3_Bot",7},
   {"Pair_0",8},
   {"Pair_1",9},
   {"Pair_2",10},
   {"Pair_3",11},
   {"LED",12},
   {"Cosmic",13},
   {"Hodoscope",14},
   {"Pulser",15},
   {"Mult_0",16},
   {"Mult_1",17},
   {"FEE_Top",18},
   {"FEE_Bot",19}
 };

map<std::string,unsigned char> TSBank::TriggerNames2021 ={
        {"Moller_top",0},
        {"Moller_PDE_top",1},
        {"Positron_top",2},
        {"Positron_hodo_top",3},
        {"Moller_bot",4},
        {"Moller_PDE_bot",5},
        {"Positron_bot",6},
        {"Positron_hodo_bot",7},
        {"Aprime_old",8},
        {"Moller_pair",9},
        {"Pair_2",10},
        {"Muon_pair",11},
        {"LED",12},
        {"Cosmic",13},
        {"Hodoscope",14},
        {"Pulser",15},
        {"Mult_0",16},
        {"Mult_1",17},
        {"FEE_Top",18},
        {"FEE_Bot",19}
};

ClassImp(TSBank);
