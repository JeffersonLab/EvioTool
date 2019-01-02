//
//  FADCdata_LinkDef.h
//  EvioTool
//
//  Created by Maurik Holtrop on 12/30/18.
//
#ifdef __ROOTCLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link off all typedef;

#pragma link C++ class FADCchan+;
#pragma link C++ class FADCchan_raw+;
#pragma link C++ class FADCdata+;
#pragma link C++ class Leaf<FADCdata>+;
#pragma link C++ class vector<Leaf<FADCdata> >;

#endif
