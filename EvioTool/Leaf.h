//
//  Leaf.h
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/19/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

#ifndef __Leaf__
#define __Leaf__

#include <iostream>
#include <vector>
using namespace std;

#include "TROOT.h"
#include "TObject.h"
#include "TNamed.h"

enum LeafDataTypes_t { Leaf_None, Leaf_Bank, Leaf_Int, Leaf_Uint32, Leaf_Float, Leaf_Double, Leaf_String ,Leaf_FADC, Leaf_END};

class Leaf_base: public TNamed {
  //// This is a base class for the templated leaf class. This makes it a lot easier
  //// to use the Leaf class, since you don't always need to know the type of Leaf.
public:
  int tag;
  int num;
  
public:
  Leaf_base(): tag(0),num(0){;};
  Leaf_base(int itag, int inum): tag(itag),num(inum){;};
  virtual void Clear(Option_t *){;};
  virtual size_t Size(void){cerr<<"ERROR-Must have override!\n";return(-99);};  // Pure virtual; Must have an override, but =0 does not work with rootcling.
  virtual void Print(Option_t *op) const {;};
  ClassDef(Leaf_base,1);
};

template <class T> class Leaf : public Leaf_base {
  //// This class stores the actual data of the EVIO "vectors"
public:
  //  string name;
  //  string description;
  
  vector<T> data;
  
public:
  Leaf(){;};
  Leaf(Leaf const&cp):Leaf_base(cp.tag,cp.num), data(cp.data){ SetName(cp.GetName()); SetTitle(cp.GetTitle());};
  Leaf(Leaf *cp): Leaf_base(cp->tag,cp->num),data(cp->data){ SetName(cp->GetName()); SetTitle(cp->GetTitle());};
  Leaf(string n,int ta, int nu,string desc): Leaf_base(ta,nu) {SetName(n.c_str()); SetTitle(desc.c_str());}
  
  virtual void CallBack(void){
    // Could be implemented in a derived class to futher process the data right after the class is filled from an EVIO bank.
  }
  
  virtual void Clear(Option_t *opt=""){
    // Clear out the contents of the leaf, but do not delete the leaf.
    data.clear();
  }
  
  void PushDataVector(vector<T> &vec){
    // Put the data from vector at the end of the leaf.
    data.insert(data.end(),vec.begin(),vec.end());
  }

  void PushDataArray(const T* dat, const int len){
    // Put the data from vector at the end of the leaf.
    data.insert(data.end(),dat,dat+len);
  }

  void SwapDataVector(vector<T> &vec){
    // Put the data from vector at the end of the leaf.
    data.swap(vec);
  }
  
  void PushBack(T dat){
    data.push_back(dat);
  }
  vector<T> GetDataVector(void){
    // Get the data vector. Creates a copy of the data. This is safe.
    return data;
  }
  vector<T> *GetDataVectorPtr(void){
    // Get the pointer to the data vector. This is not safe, but faster.
    // The data pointed to may disappear (i.e. at the next event.)
    return &data;
  }

  T GetData(const int indx){
    // Get the data at indx. Throws exception if out of range.
    return(data.at(indx));
  }

  T At(const int indx){
    // Get the data at indx. Throws exception if out of range.
    return(data.at(indx));
  }
  
  T operator[](const int indx){
    // Get the data at indx. Throws exception if out of range.
    return(data.at(indx));
  }

  size_t Size(void){
    // Get the size in the data vector. (ROOT has capital first idiom)
    return(data.size());
  }
  
  size_t size(void){
    // Get the size in the data vector. (std:: style size)
    return(data.size());
  }
  
  static int Type(void){
    // Return the type stored in this leaf. Static so can be used as
    // Leaf<int>::Type() etc.
    return(Leaf_None);
  };
  
  virtual void Print(Option_t *op) const{
    // Print your self, and num of the contents.
    // To be compatible with TObject::Print(), which is needed for virtual use,
    // the num and level are encoded in the opt= "N10L3" is num=10, level=3
    string s;
    int level;
    int n;
    if(strlen(op)>4){
      sscanf(op,"N%dL%d",&n,&level);
      for(int i=0;i<level;i++) s.append("    ");
      cout << s << ClassName() << ":\t" << GetName() << "\t tag = " << tag << " num = " << num << endl;
      cout << s << "Data: ";
      for(int i=0;i<n && i< (int)data.size();i++){
        cout << " " << data[i];
      }
    }else{
      cout << "  -?- " << ClassName() << ":\t" << GetName() << "\t tag = " << tag << " num = " << num << endl;
    }
  };
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(Leaf,1);
#pragma clang diagnostic pop

};

#endif /* defined(__AprimeAna__Leaf__) */
