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

#pragma link C++ class SVTBank+;
#pragma link C++ class SVTBank2016+;
#pragma link C++ struct SVT_chan_t;
#pragma link C++ struct SVT_header_t;

#pragma link C++ class Header+;
#pragma link C++ class Headbank+;
#pragma link C++ class TSBank+;
#pragma link C++ class VTPBank+;
#pragma link C++ class TriggerConfig+;

#pragma link C++ struct FADC250_slot_t+;
#pragma link C++ struct FADC250_crate_t+;

#pragma link C++ class EcalBank+;
#pragma link C++ class Cluster+;
#pragma link C++ class EcalGTPCluster+;
#pragma link C++ class EcalCluster+;
#pragma link C++ class EcalHit_t+;


#endif
