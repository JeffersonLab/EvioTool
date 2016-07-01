//
//  Leaf_Linkdef.h
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/19/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

#ifndef AprimeAna_Leaf_Linkdef_h
#define AprimeAna_Leaf_Linkdef_h

#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ enum  LeafDataTypes_t+;
#pragma link C++ class Leaf<int>+;
#pragma link C++ class Leaf<uint32_t>+;
#pragma link C++ class Leaf<float>+;
#pragma link C++ class Leaf<double>+;
#pragma link C++ class Leaf<string>+;
#pragma link C++ class vector<Leaf<int> >;
#pragma link C++ class vector<Leaf<uint32_t> >;
#pragma link C++ class vector<Leaf<float> >;
#pragma link C++ class vector<Leaf<double> >;
#pragma link C++ class vector<Leaf<string> >;
#endif



#endif
