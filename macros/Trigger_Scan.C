//
//  Trigger_Scan.C
//  EvioTool
//
//  Created by Maurik Holtrop on 11/8/19.
//  Copyright © 2019 UNH. All rights reserved.
//
R__LOAD_LIBRARY(libHPSEvioReader); // ROOT6 way to load a library.

void Trigger_Scan(const char *file){
  HPSEvioReader *p = new HPSEvioReader(file);
  p->Next();  // This Next() is important for any 000000 files, where the first event is an Trigge/ECal config bank.
  p->tags = {1<<7};
  p->tag_masks = {1<<7};
  
  int singles[4]={0,0,0,0};
  int pairs[3]={0,0,0};
  int fee=0;
  int pulser=0;
  int multi[2]={0,0};
  
  int event=0;
  cout << "Start \n";
  while(p->Next()==0){
    event++;
    // if(event%100000==0) printf("%10d\n",event);
    if( !(p->this_tag & 1<<7)) continue;
    if( p->Trigger->IsSingle0() ) singles[0]++;
    if( p->Trigger->IsSingle1() ) singles[1]++;
    if( p->Trigger->IsSingle2() ) singles[2]++;
    if( p->Trigger->IsSingle3() ) singles[3]++;
    if( p->Trigger->IsPair0() )   pairs[0]++;
    if( p->Trigger->IsPair1() )   pairs[1]++;
    if( p->Trigger->IsPair2() )   pairs[2]++;
    if( p->Trigger->IsFEE()   )   fee++;
    if( p->Trigger->IsPulser())   pulser++;
    if( p->Trigger->IsMult0())   multi[0]++;
    if( p->Trigger->IsMult1())   multi[1]++;
    
  }

  printf("Read: %10d events \n",event);
  printf("Single Triggers:  %7d  %7d  %7d  %7d \n",singles[0],singles[1],singles[2],singles[3]);
  printf("Pair   Triggers:  %7d  %7d  %7d \n",pairs[0],pairs[1],pairs[2]);
  printf("FEE    Triggers:  %7d  \n",fee);
  printf("Pulser Triggers:  %7d  \n",pulser);
  printf("Multi  Triggers:  %7d  %7d \n",multi[0],multi[1]);
  
  gSystem->Exit(0);
  return;
}
