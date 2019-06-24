//
//  Header.hpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 1/17/19.
//  Copyright © 2019 UNH. All rights reserved.
//
//  Default header bank is 49152 = 0xc000
#ifndef Header_hpp
#define Header_hpp

#include "EvioTool.h"
#include "Leaf.h"

class Header: public Leaf<unsigned int>{
  
public:
  Header(){};
  Header(Bank *b,unsigned short itag=49152,unsigned short inum=0): Leaf("Header",itag,inum,"Event Header data"){
    b->AddThisLeaf(this);
  };
  unsigned int GetEventNumber(){
    if(data.size()>2){
      return( data[0]);
    }else{
      return(0);
    }
  }
  
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(Header,1);
#pragma clang diagnostic pop
};

#endif /* Header_hpp */
