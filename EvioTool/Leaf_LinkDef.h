//
//  Leaf_LinkDef.h
//  EvioTool
//
//  Created by Maurik Holtrop on 12/19/18.
//  Copyright © 2018 UNH. All rights reserved.
//
// NOTE:
// This Leaf_LinkDef needs to run separately from the EvioTool_LinkDef.
// The reason isn't 100% clear to me, but has to do with the subtleties of C++ template class specification.
// ROOT requires specific instantiation of each version of the template class you intent to use. However, because of ordering issues
// in the generation of these templates, you get an error: "Explicit specialization of 'Class' after instantiation" for the specializations in the ROOT dictionary.
// Running Leaf separately through rootcling and compiling separately avoids this issue.
// Please let me know if you know better! Thanks, Maurik.
//
#ifdef __ROOTCLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link off all typedef;

#pragma link C++ class Leaf_base+;
#pragma link C++ class FADCchan+;
#pragma link C++ class FADCchan_raw+;
#pragma link C++ class FADCdata+;

#pragma link C++ enum  LeafDataTypes_t;

#pragma link C++ class Leaf<char>+;
#pragma link C++ class Leaf<unsigned char>+;
#pragma link C++ class Leaf<short>+;
#pragma link C++ class Leaf<unsigned short>+;
#pragma link C++ class Leaf<int>+;
#pragma link C++ class Leaf<unsigned int>+;
#pragma link C++ class Leaf<float>+;
#pragma link C++ class Leaf<double>+;
#pragma link C++ class Leaf<string>+;
#pragma link C++ class Leaf<long long>+;
#pragma link C++ class Leaf<unsigned long long>+;

#pragma link C++ class vector<Leaf<char> >;
#pragma link C++ class vector<Leaf<unsigned char> >;
#pragma link C++ class vector<Leaf<short> >;
#pragma link C++ class vector<Leaf<unsigned short> >;
#pragma link C++ class vector<Leaf<int> >;
#pragma link C++ class vector<Leaf<unsigned int> >;
#pragma link C++ class vector<Leaf<long long> >;
#pragma link C++ class vector<Leaf<unsigned long long> >;
#pragma link C++ class vector<Leaf<float> >;
#pragma link C++ class vector<Leaf<double> >;
#pragma link C++ class vector<Leaf<string> >;
// #pragma link C++ class vector<Leaf<FADCdata> >;
// #pragma link C++ class Leaf<FADCdata>+;

#endif 
