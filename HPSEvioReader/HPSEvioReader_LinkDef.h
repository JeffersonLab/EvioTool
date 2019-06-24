//
//  SVTdata_LinkDef.h
//  EvioTool
//
//  Created by Maurik Holtrop on 1/2/19.
//
#ifdef __ROOTCLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link off all typedef;

#pragma link C++ class HPSEvioReader;

#pragma link C++ class SVTbank+;
#pragma link C++ struct SVT_chan_t;
#pragma link C++ struct SVT_header_t;

#pragma link C++ class Header+;
#pragma link C++ class Headbank+;
#pragma link C++ class TIData+;
#pragma link C++ class VTPData+;

#endif
