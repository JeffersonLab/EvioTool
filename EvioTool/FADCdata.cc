//
//  FADCdata.cpp
//  EvioTool
//
//  Created by Maurik Holtrop on 12/28/18.
//  Copyright © 2018 UNH. All rights reserved.
//
#include "FADCdata.h"
#include "EvioTool.h"

//
// Custom streamers so std::cout << MyFADC << std::endl; works.
//
std::ostream &operator<<(std::ostream &os, FADCdata const &m) {
  os << m.Class_Name() << "(" << (int)m.crate << "," << (int)m.slot << "," << (int)m.chan << ") [";
  for(int i=0; i< m.samples.size(); ++i){
    os << (int)m.samples[i];
    if(i<(m.samples.size()-1)) os << ",";
  }
  os << "]";
  return(os);
}

unsigned short FADCdata::GetTime(){
  return(time);
}

unsigned short FADCdata::GetAdc(){
  return(adc);
}


ClassImp(FADCdata);
ClassImp(Leaf<FADCdata>);


//template <> int EvioTool::AddOrFillLeaf<FADCdata>(const unsigned int *buf,int len,unsigned short tag,unsigned char num,Bank *node){
//  // Add or Fill a float leaf in the bank node.
//  // If fAutoAdd is false, find the leaf with tag, num and fill it. If not found do nothing.
//  // If fAutoAdd is true, if not found, a new leaf is added and filled.
//  int loc = node->Find(tag,num);
//  if( loc == -1){
//    if(fAutoAdd){
//      char str[100];
//      sprintf(str,"FADC-%u-%u",tag,num);
//      if(fDebug&Debug_L2) cout << "Adding a new Leaf node to node: " << node->GetNum() << " with name: " << str << endl;
//      node->Add_Leaf<FADCdata>(str,tag,num,"Auto added string leaf");
//      loc= node->leafs->GetEntriesFast()-1;
//    }else{
//      return 0;
//    }
//  }
//
//  if(fDebug&Debug_L2) cout << "Adding data to Leaf at idx = " << loc << " with specified AddOrFillLeaf<FADCdata> \n";
//  //  node->Push_data_array(loc, buf, len);
//
//  const uint32_t *d   = (const uint32_t*)buf;
//  unsigned short formatTag       = (d[0]>>20)&0xfff;
//  unsigned short formatLen       = d[0]&0xffff;
//  string formatString = string((const char *) &(((uint32_t*)buf)[1]));
//  int dataLen         = d[1+formatLen]-1;
//  //  int dataTag         = (d[1+formatLen+1]>>16)&0xffff;
//  //  int dataNum         = d[1+formatLen+1]&0xff;
//
//  unsigned char *cbuf = (unsigned char *) &d[3+formatLen];
//
//#define GET_CHAR(b,i)   b[i]; i+=1;
//#define GET_SHORT(b,i) ((short *)(&b[i]))[0];i+=2;
//#define GET_INT(b,i)  ((int *)(&b[i]))[0];i+=4;
//#define GET_L64(b,i) ((unsigned long long *)(&b[i]))[0];i+=8;
//
//  int buflen = dataLen*4 - 4;
//  int indx=0;
//  vector<FADCdata> fadc_data;
//  fadc_data.reserve(16);
//  FADCdata fadc;
//  fadc.crate      = node->this_tag;
//  while( indx < buflen){
//    if(formatTag == 13){
//      fadc.slot       = cbuf[indx];indx+=1;    // GET_CHAR(buf,indx);
//      fadc.trig       = GET_INT(cbuf,indx);
//      fadc.time  = GET_L64(cbuf,indx);
//      int nchan                = GET_INT(cbuf,indx);
//      fadc.raw_data.reserve(nchan);
//      for(int jj=0; jj<nchan; ++jj){
//        FADCchan_raw ch;
//        ch.chan = GET_CHAR(cbuf,indx);
//        int nsample = GET_INT(cbuf,indx);
//        ch.samples.reserve(nsample);
//        unsigned short *samples=(unsigned short *)&cbuf[indx];
//        ch.samples.assign(samples,samples+nsample);                           // Takes 3.3 micro seconds/event
//        indx+=nsample*2;
//      }
//      fadc_data.push_back(fadc);
//    }
//  }
//
//  return 1;
//};
