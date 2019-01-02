//
//  FADCdata.cpp
//  EvioTool
//
//  Created by Maurik Holtrop on 12/28/18.
//  Copyright © 2018 UNH. All rights reserved.
//
#include "FADCdata.h"
#include "EvioParser.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
// Template Specification on how to fill leafs of specific not simple types.
//
// These are specification for: template<typename T> int AddOrFillLeaf(const unsigned int *buf,int len,int tag,int num,Bank *node)
//
//////////////////////////////////////////////////////////////////////////////////////////

template <> int EvioParser::AddOrFillLeaf<FADCdata>(const unsigned int *buf,int len,unsigned short tag,unsigned char num,Bank *node){
  // Add or Fill a float leaf in the bank node.
  // If fAutoAdd is false, find the leaf with tag, num and fill it. If not found do nothing.
  // If fAutoAdd is true, if not found, a new leaf is added and filled.
  int loc = node->Find(tag,num);
  if( loc == -1){
    if(fAutoAdd){
      char str[100];
      sprintf(str,"FADC-%u-%u",tag,num);
      if(fDebug&Debug_L2) cout << "Adding a new Leaf node to node: " << node->GetNum() << " with name: " << str << endl;
      node->Add_Leaf<FADCdata>(str,tag,num,"Auto added string leaf");
      loc= node->leafs->GetEntriesFast()-1;
    }else{
      return 0;
    }
  }
  
  if(fDebug&Debug_L2) cout << "Adding data to Leaf at idx = " << loc << " with specified AddOrFillLeaf<FADCdata> \n";
 
  unsigned short formatTag  = (buf[0]>>20)&0xfff;
  unsigned short formatLen  = buf[0]&0xffff;
#ifdef DEBUG
 string formatString       = string((const char *) &(((uint32_t*)buf)[1]));
#endif
  int dataLen               = buf[1+formatLen]-1;
#ifdef DEBUG
  int dataTag         = (buf[1+formatLen+1]>>16)&0xffff;
  int dataNum         = buf[1+formatLen+1]&0xff;
#endif

  unsigned char *cbuf = (unsigned char *) &buf[3+formatLen];
  
  int buflen = dataLen*4 - 4;
  int indx=0;
  
// We use emplace_back instead of creating the class and then push_back. This should save at least one copying step of the data
// thus speeding up the code. Turns out that this code is 4 to 5x faster than the commented code at end of file.
// The constructor for FADCdata is created to do the work of re-interpreting the buffer.
// See: Effective Modern C++, item 42.

  Leaf<FADCdata> *ll =(Leaf<FADCdata> *)node->leafs->At(loc);
  ll->data.reserve(16);
  unsigned char crate = node->this_tag;
  while( indx < buflen){
    unsigned char slot       = GET_CHAR(cbuf,indx);
    unsigned int  trig       = GET_INT(cbuf,indx);
    unsigned long long time  = GET_L64(cbuf,indx);
    int nchan  = GET_INT(cbuf,indx);
    for(int jj=0; jj<nchan; ++jj){
        ll->data.emplace_back(crate,slot,trig,time, indx, cbuf);
//      ll->data.emplace_back(indx, cbuf);
    }
}
  
  return 1;
};

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


ClassImp(FADCdata);
ClassImp(Leaf<FADCdata>);


//template <> int EvioParser::AddOrFillLeaf<FADCdata>(const unsigned int *buf,int len,unsigned short tag,unsigned char num,Bank *node){
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
