//
//  main.cpp
//  test1
//
//  Created by Maurik Holtrop on 11/9/19.
//  Copyright © 2019 UNH. All rights reserved.
//
#include "HPSEvioReader.h"
#include <iostream>

int main(int argc, const char * argv[]) {
  // insert code here...
  std::cout << "Running Test1\n";
  const char * file="/data/HPS/data/physrun2019/evio/hps_010051.evio.00000";
  HPSEvioReader *p = new HPSEvioReader(file);
  p->tags = {1<<7};
  p->tag_masks = {1<<7};
  p->Next();
  p->Trigger->Print("");
  return 0;
}
