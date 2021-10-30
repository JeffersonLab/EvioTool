//
//  EcalHit.hpp
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 7/9/19.
//  Copyright © 2019 UNH. All rights reserved.
//

#ifndef EcalHit_h
#define EcalHit_h

#include "EvioTool.h"

using  Ecal_point_t=std::pair<int,int>;

class block_iterator{
    // This iterator will traverse the ECAL crystals, skipping x=0 or y=0, and the ECal hole
public:
  using self_type = block_iterator;
  using iterator_category = std::input_iterator_tag;
  using value_type = Ecal_point_t;
  using reference = value_type const&;
  using pointer = value_type const*;
  using difference_type = ptrdiff_t;
  
  block_iterator(const self_type &init): ixy(init.ixy),ixy0(init.ixy0){};
  block_iterator(const value_type start): ixy(start),ixy0(start){};
  block_iterator(const value_type start,value_type center): ixy(start),ixy0(center){};
  block_iterator(const pointer seed): ixy(*seed),ixy0(*seed){};
  
  self_type operator++() {  // This will be called for a ++itt
    ixy.first++;
    if(ixy.first==0) ixy.first =1;       // Dealing with the stupid ecal numbering scheme!!!
    if( (ixy.first > ( (ixy0.first< -span || ixy0.first>0 )?ixy0.first+span:ixy0.first+span+1)) || ixy.first > ecal_max_x){
      ixy.first = ixy0.first-span;
      if(ixy.first == 0) ixy.first = -1;
      if(ixy.first < ecal_min_x) ixy.first = ecal_min_x;
      ixy.second++;
    }
    if(ixy==ixy0) this->operator++();
    if(abs(ixy.second)==1 && ixy.first> -11 && ixy.first< -1){  // Ecal Hole
      this->operator++();
    }
    return(*this);
  }
  self_type operator++(int){ // This will be called for the itt++ -- extra cost of one copy.
    self_type save = *this;
    ++(*this);
    return(save);
  }
  bool operator==(self_type rhs) const {return( ixy == rhs.ixy);}
  bool operator!=(self_type rhs) const {return( ixy != rhs.ixy);}
  
  reference operator*() const {return( ixy);}
  pointer   operator->(){return(&ixy);}
  self_type begin(int dist=1){
    span=dist;
    int ix = ixy.first-span;
    if(ix<ecal_min_x) ix=ecal_min_x;
    if(ix == 0) ix = -1;
    int iy = ixy.second-span;
    if(ixy.second>0 && iy<1) iy=1;
    if(iy < ecal_min_y){
      iy = ecal_min_y;
      if( ix == ecal_min_x ) ix = ecal_min_x+1;
    }
    if( abs(iy) == 1 && ix> -11 && ix< -1){   // If we start in the ECAL hole, then step out.
      self_type begin_test({ix,iy},ixy0);
      ++begin_test;
      return(begin_test);
    }
    return(self_type({ix,iy},ixy0));
  };
  self_type end(int dist=1)  {
    span=dist;
    int ix=ixy.first+span;
    if(ix == 0) ix = 1;
    if(ix>ecal_max_x) ix = ecal_max_x;
    int iy = ixy.second+span;
    if(ixy.second<0 && iy> -1) iy=-1;
    if(iy> ecal_max_y) iy=ecal_max_y;
    self_type end_test({ix,iy},ixy0);         // Step to +1, because you are not supposed to reach the end!
    ++end_test;
    return(end_test);
  };
  
  // private:
public:
  static constexpr int ecal_min_x=-23;
  static constexpr int ecal_min_y=-5;
  static constexpr int ecal_max_x=+23;
  static constexpr int ecal_max_y=+5;
  value_type ixy;
  value_type ixy0;
  int span=1;
  
};

class EcalHit_t {
public:
  int fadc_index;            // Index of the RAW FADC data (Leaf<FADCdata> *FADC_leaf) of the FADC hits.
  Ecal_point_t ixy;          // This stores the ix,iy point of the crystal in the ECAL.
  vector<FADC_Hit_t> hits;   // Each crystal can have more than one hit in the event. This stores the FADC hits for this crystal for this event.
public:
  EcalHit_t(Ecal_point_t xy):fadc_index(-1),ixy(xy){};
  EcalHit_t(int fidx=-1,int x=0,int y=0):fadc_index(fidx),ixy(x,y){};
  EcalHit_t(FADC_Hit_t fadch,int fidx=-1,int x=0, int y=0):fadc_index(fidx),ixy(x,y){ hits.push_back(fadch);};
  EcalHit_t(vector<FADC_Hit_t> adc_hits,int fidx=-1,int x=0, int y=0):fadc_index(fidx),ixy(x,y),hits(adc_hits){};
  int get_ix(){return(ixy.first);}
  int get_iy(){return(ixy.second);}
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(EcalHit_t,1);
#pragma clang diagnostic pop
};


#endif /* EcalHit_hpp */
