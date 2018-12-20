//
//  EvioTool_Linkdef.h
//  EvioTool
//
//  Created by Maurik Holtrop on 4/20/14.
//  Copyright (c) 2014 UNH. All rights reserved.
//
#ifdef __ROOTCLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link off all typedef;


#pragma link C++ class Bank+;
#pragma link C++ class vector<Bank>;
#pragma link C++ function Bank::Add_Leaf<int>;
#pragma link C++ function Bank::Add_Leaf<float>;
#pragma link C++ function Bank::Add_Leaf<double>;
#pragma link C++ function Bank::Add_Leaf<string>;
#pragma link C++ function Bank::Push_data<int>;
#pragma link C++ function Bank::Push_data<float>;
#pragma link C++ function Bank::Push_data<double>;
#pragma link C++ function Bank::Push_data<string>;
#pragma link C++ function Bank::Push_data_vector<int>;
#pragma link C++ function Bank::Push_data_vector<float>;
#pragma link C++ function Bank::Push_data_vector<double>;
#pragma link C++ function Bank::Push_data_vector<string>;
#pragma link C++ function Bank::Get_data<int>(int,int);
#pragma link C++ function Bank::Get_data<float>(int,int);
#pragma link C++ function Bank::Get_data<double>(int,int);
#pragma link C++ function Bank::Get_data<string>(int,int);
#pragma link C++ function Bank::Get_data<int>(string,int);
#pragma link C++ function Bank::Get_data<float>(string,int);
#pragma link C++ function Bank::Get_data<double>(string,int);
#pragma link C++ function Bank::Get_data<string>(string,int);
#pragma link C++ function Bank::Get_data_vector<int>;
#pragma link C++ function Bank::Get_data_vector<float>;
#pragma link C++ function Bank::Get_data_vector<double>;
#pragma link C++ function Bank::Get_data_vector<string>;

#pragma link C++ class EvioTool-;  // Don't try to make a streamer!
#pragma link C++ class EvioParser+;

#pragma link C++ struct EVIO_Event_t;
#pragma link C++ struct FADC_chan_f13_t;
#pragma link C++ struct FADC_chan_f15_t;
#pragma link C++ struct FADC_data_f13_t;
#pragma link C++ struct FADC_data_f15_t;
#pragma link C++ struct SVT_chan_t;
#pragma link C++ struct SVT_FPGA_t;
#pragma link C++ class vector<FADC_chan_f13_t>;
#pragma link C++ class vector<FADC_chan_f15_t>;
#pragma link C++ class vector<FADC_data_f13_t>;
#pragma link C++ class vector<FADC_data_f15_t>;
#pragma link C++ class vector<SVT_chan_t>;
#pragma link C++ class vector<SVT_FPGA_t>;
//
#pragma link C++ function EvioEventClear;
#pragma link C++ function EvioEventInit;
#pragma link C++ function EvioEventPrint;
//
#endif
