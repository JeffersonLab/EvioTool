//
//  Leaf.cc
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/19/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

#include "Leaf.h"

// Template Specifications.
template<> int Leaf<int>::Get_type(void){ return(Leaf_Int); };
template<> int Leaf<uint32_t>::Get_type(void){ return(Leaf_Uint32); };
template<> int Leaf<float>::Get_type(void){ return(Leaf_Float); };
template<> int Leaf<double>::Get_type(void){ return(Leaf_Double); };
template<> int Leaf<string>::Get_type(void){ return(Leaf_String); };

ClassImp(Leaf_base);
ClassImp(Leaf<int>);
ClassImp(Leaf<uint32_t>);
ClassImp(Leaf<float>);
ClassImp(Leaf<double>);
ClassImp(Leaf<string>);

// This keeps the root interpreter happy when dealing with these structures.




