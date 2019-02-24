//
//  Leaf.cc
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/19/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

#include "Leaf.h"
#include "EvioTool.h"

// Template Specifications.
template<> int Leaf<int>::Type(void){ return(Leaf_Int); };
template<> int Leaf<uint32_t>::Type(void){ return(Leaf_Uint32); };
template<> int Leaf<float>::Type(void){ return(Leaf_Float); };
template<> int Leaf<double>::Type(void){ return(Leaf_Double); };
template<> int Leaf<string>::Type(void){ return(Leaf_String); };
template<> int Leaf<FADCdata>::Type(void){ return(Leaf_FADC); };

// Push the array if T is "string".
// This needs specification, because strings are not a simple type.
//template<> void Leaf<string>::Push_data_array(const string* dat, const int len){
  // Put the data from vector at the end of the leaf.
//  const char *ch = (const char *)dat;
//  data.insert(data.end(),ch,ch+len);
//}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Template Specification on how to fill leafs of specific not simple types.
//
// These are specification for: template<typename T> int AddOrFillLeaf(const unsigned int *buf,int len,int tag,int num,Bank *node)
//
//////////////////////////////////////////////////////////////////////////////////////////

template <> int EvioTool::AddOrFillLeaf<string>(const unsigned int *buf,int len,unsigned short tag,unsigned char num,Bank *node){
  // Add or Fill a float leaf in the bank node.
  // If fAutoAdd is false, find the leaf with tag, num and fill it. If not found do nothing.
  // If fAutoAdd is true, if not found, a new leaf is added and filled.
  int loc = node->FindLeaf(tag,num);
  if( loc == -1){
    if(fAutoAdd){
      char str[100];
      sprintf(str,"String-%u-%u",tag,num);
      if(fDebug&Debug_L2) cout << "Adding a new Leaf node to node: " << node->GetNum() << " with name: " << str << endl;
      node->AddLeaf<string>(str,tag,num,"Auto added string leaf");
      loc= node->leafs->GetEntriesFast()-1;
    }else{
      return 0;
    }
  }
  
  if(fDebug&Debug_L2) cout << "Adding data to Leaf at idx = " << loc << " template specification version for <string> " << endl;

  // Specialized version for an array of character strings.
  // Add the vector to the leaf at index.
  // Put a buffer of char into the string if Leaftype is string
    const char *start =(const char *) buf;
    char *c = (char *)start;
    string s;
    while((c[0]!=0x4)&&((c-start)< len)) {
      char *n=c;
      while( std::isprint(*n++)){};
      if( (n-c-1)>0){
        s.assign(c,n-c-1); // This chomps off the non-print, usually \n.
        ((Leaf<string> *)node->leafs->At(loc))->PushBack(s);
      }
      c=n;
    }
  return 1;
};


ClassImp(Leaf_base);
ClassImp(Leaf<int>);
ClassImp(Leaf<uint32_t>);
ClassImp(Leaf<float>);
ClassImp(Leaf<double>);
ClassImp(Leaf<string>);

// This keeps the root interpreter happy when dealing with these structures.




