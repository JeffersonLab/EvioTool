//
//  EvioEvent_Linkdef.h
//  EvioTool
//
//  Created by Maurik Holtrop on 5/7/14.
//  Copyright (c) 2014 UNH. All rights reserved.
//
#ifdef __ROOTCLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link off all typedef;

#pragma link C++ class  HPSEvioReader-;  // Don't try to make a streamer!
#pragma link C++ struct EVIO_Event_t;
#pragma link C++ struct FADC_chan_f13_t;
#pragma link C++ struct FADC_chan_f15_t;
#pragma link C++ struct FADC_data_f13_t;
#pragma link C++ struct FADC_data_f15_t;
#pragma link C++ struct SVT_chan_t;
#pragma link C++ struct SVT_header_t;
#pragma link C++ class vector<FADC_chan_f13_t>;
#pragma link C++ class vector<FADC_chan_f15_t>;
#pragma link C++ class vector<FADC_data_f13_t>;
#pragma link C++ class vector<FADC_data_f15_t>;
#pragma link C++ class vector<SVT_chan_t>;
//
#pragma link C++ function EvioEventClear;
#pragma link C++ function EvioEventInit;
#pragma link C++ function EvioEventPrint;

//#pragma link C++ defined_in "EvioEvent.h";

#endif
