//
//  Bank.h
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/19/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

// The "Bank" class contains a list of "Leaf" nodes, each of the Leaf nodes contains a vector of data.
//
// Notes:
// We must store a vector or TObjArray of pointers, and not objects, because the objects
// will change size and thus cause trouble with the memory allocation of the vector.
// We may as well derive everything (i.e. the leafs and sub-banks) from TObject, so we can simply
// store pointers to TObject, since the whole point of this class is to be ROOT compliant.
// In that situation, we may as well use TObjArray, which is a reasonable implementation, and get Find and
// other useful functionality. I hesitantly will do this the ROOT way. I have an inherent bias towards STL.


#ifndef __AprimeAna__Bank__
#define __AprimeAna__Bank__

#include <cstdio>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <map>

using namespace std;

#include "TROOT.h"
#include "TObject.h"
#include "TObjArray.h"
#include "TNamed.h"

#include "Leaf.h"

#define MAX_DATA 1000
// TODO:
// - Add a fast lookup for tag,num finding.

class Bank : public TNamed {
  // Mimicks a container bank in EVIO, with extra's.
public:
  vector< unsigned short>tags;  // Tags to select bank with.
  unsigned char  num;           // num to select bank with. If set to 0, ignore.
  
  unsigned short this_tag;      // The actual tag of the parsed bank.
  unsigned char  this_num;      // The actual num of the parsed bank.
  
  map<string,unsigned short> name_index;    // Maps names to index.
  TObjArray  *leafs;             // Stores the leafs.
  TObjArray  *banks;             // Stores other banks. In separate array, since separte content.
  
public:
  Bank(){
    Init();
  };
  virtual ~Bank(){
    delete leafs;
    delete banks;
  }
  Bank(string n,std::initializer_list<unsigned short> tags,unsigned char num,string desc):tags(tags),num(num){
    SetName(n.c_str());
    SetTitle(desc.c_str());
    Init();
  }
  
  Bank(string n,unsigned short tag,unsigned char num,string desc):num(num){
    tags.push_back(tag);
    SetName(n.c_str());
    SetTitle(desc.c_str());
    Init();
  }
  
  Bank(Bank const &cp) {
    tags = cp.tags;
    num = cp.num;
    SetName(cp.GetName());
    SetTitle(cp.GetTitle());
    leafs = (TObjArray *)cp.leafs->Clone();
    banks = (TObjArray *)cp.banks->Clone();
  }
  Bank(Bank *cp){
    tags = cp->tags;
    num  = cp->num;
    SetName(cp->GetName());
    SetTitle(cp->GetTitle());
    leafs = (TObjArray *)cp->leafs->Clone();
    banks = (TObjArray *)cp->banks->Clone();
  }
  
  void Init(void){
    // Initialize the Bank class.
    leafs = new TObjArray();
    leafs->SetOwner(kTRUE);
    banks = new TObjArray();
    banks->SetOwner(kTRUE);
  }
  
  unsigned char GetNum(void){ return(this_num);};
  unsigned short GetTag(void){ return(this_tag);};
  vector<unsigned short> &GetTags(void){return(tags);}
  
  int Add_Leaf(string name,unsigned short itag, unsigned char inum,string desc,int type);
  template<typename T> Leaf<T> * Add_Leaf(Leaf<T> &leaf){
    int location= leafs->GetEntriesFast();
    string name=StoreLocation(leaf.GetName(),location);
    Leaf<T> *new_leaf = new Leaf<T>(leaf); //// COPY the leaf, don't ref to original.
    leafs->Add(new_leaf);
    return(new_leaf);
  }
  
  template<typename T> Leaf<T> *Add_Leaf(string name,unsigned short itag,unsigned char inum, string desc){
    int location= leafs->GetEntriesFast();
    name=StoreLocation(name,location);
    Leaf<T> *new_leaf =new Leaf<T>(name,itag,inum,desc);
    leafs->Add(new_leaf);
    return(new_leaf);
  }
  
  Bank *Add_Bank(string name,unsigned short itags,unsigned char inum, string desc){
    // Add a Bank witn name,tag,num,description.
    // Returns a pointer to the new bank.
    // int location = banks->GetEntriesFast();
    // name=StoreLocation(name,location);
    Bank *newbank = new Bank(name,itags,inum,desc);
    banks->Add(newbank);
    return(newbank);
  }

  Bank *Add_Bank(string name,std::initializer_list<unsigned short> itags, unsigned char inum, string desc){
    // Add a Bank witn name,tag,num,description.
    // Returns a pointer to the new bank.
    // int location = banks->GetEntriesFast();
    // name=StoreLocation(name,location);
    Bank *newbank = new Bank(name,itags,inum,desc);
    banks->Add(newbank);
    return(newbank);
  }
  
  void  Clear(Option_t* = "");
  
  string StoreLocation(string name,unsigned short location){
    // Store the location under name, make sure name is unique!
    map<string,unsigned short>::iterator loc=name_index.find(name);
    if(loc == name_index.end()){
      name_index[name]=location;
    }else{
      cerr << "Bank::StoreLocation -- ERROR -- name " << name << " already in bank. \n";
      name+="+";
      name = StoreLocation(name,location);
    }
    return(name);
  }
  
  int Find(string name){
    // Find the leaf item with name, return location.
    // If not found, return -1.
    map<string,unsigned short>::iterator loc=name_index.find(name);
    if(loc != name_index.end()){
      return loc->second;
    }else return( -1);
  }
  
  int Find(unsigned short itag,unsigned char inum){
    // Find the location of the leaf with num, tag.
    // Returns the location, or -1 if not found.
    // If a leaf has num=0, or inum = 0 then inum is ignored.
    //
    for(int i=0;i<leafs->GetEntriesFast();++i){
      if( ((Leaf_base *)leafs->At(i))->tag == itag &&
          ( ((Leaf_base *)leafs->At(i))->num==0 || inum==0 || ((Leaf_base *)leafs->At(i))->num == inum ) ){
        return(i);
      }
    }
    return(-1);
  }

  int Find_bank(string name){
    // Find a bank by name.
    TObject *o = banks->FindObject(name.c_str());
    int idx=banks->IndexOf(o);
    return(idx);
  }
  
  int Find_bank(unsigned short itag,unsigned char inum){
    // Find the location of the leaf or bank with tag, num.
    // Since "num" is not always used to identify a specific bank,
    // bank->num=0 or inum=0 is treated as "don't care".
    // returns -1 if not found.
    // TODO: STL Fast Search
    for(int i=0;i<banks->GetEntriesFast();++i){
      if( std::find( ((Bank *)banks->At(i))->tags.begin(),((Bank *)banks->At(i))->tags.end(),itag) != ((Bank *)banks->At(i))->tags.end() &&
         ( inum==0 || ((Bank *)banks->At(i))->num==0 || ((Bank *)banks->At(i))->num == inum)) {
        return(i);
      }
    }
    return(-1);
  }

  template<typename T> void  Push_data(string leaf_name, T dat){
    // Push an individual data element to the end of the leaf with leaf_name
    int index = Get_index_from_name(leaf_name);
    if(index<0){
      cout << "Bank::Push_data - ERROR - We do not know about the Leaf called " << leaf_name << endl;
      return;
    }
    Push_data(index,dat);
    return;
  }
  
//  template<typename T> void Push_data(int leaf_index,T dat){
//    // Add data to the end of leaf at leaf_index.
//    if( leafs->At(leaf_index)->IsA() == Leaf<T>::Class() ){
//      Leaf<T> *l=(Leaf<T> *)leafs->At(leaf_index);
//      l->Push_back(dat);
//    }else{   /// This is mis-use of the method. Carry on best as we can.
//      cerr << "Leaf::Push_data("<<leaf_index<<","<< dat << ") -- Trying to add incorrect data type to leaf\n";
//    }
//  }

// The commented out template method above works, except when you try to put 123 in a Leaf<double>
// or other constructs that you would expect to work with automatic casting.
// For the purpose of usability (and ROOT macro sloppiness) we specialize the methods:
  void      Push_data(int leaf_index,int dat){
// Put an int on the leaf at leaf_index. The leaf can be an int, float, double or string.
    if( leafs->At(leaf_index)->IsA() == Leaf<int>::Class() ){
      Leaf<int> *l=(Leaf<int> *)leafs->At(leaf_index);
      l->Push_back(dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
      Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
      l->Push_back(dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<float>::Class() ){
      Leaf<float> *l=(Leaf<float> *)leafs->At(leaf_index);
      l->Push_back((float)dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<double>::Class() ){
      Leaf<double> *l=(Leaf<double> *)leafs->At(leaf_index);
      l->Push_back((double)dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<string>::Class() ){
      Leaf<string> *l=(Leaf<string> *)leafs->At(leaf_index);
      char buf[30];
      sprintf(buf,"%d",dat);
      l->Push_back(buf);
    }
  }

  void      Push_data(int leaf_index,float dat){
// Put a float on the leaf at leaf_index.
    if( leafs->At(leaf_index)->IsA() == Leaf<float>::Class() ){
      Leaf<float> *l=(Leaf<float> *)leafs->At(leaf_index);
      l->Push_back(dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<int>::Class() ){
      Leaf<int> *l=(Leaf<int> *)leafs->At(leaf_index);
      l->Push_back((int)dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
      Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
      l->Push_back(dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
      Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
      l->Push_back(dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<double>::Class() ){
      Leaf<double> *l=(Leaf<double> *)leafs->At(leaf_index);
      l->Push_back((double)dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<string>::Class() ){
      Leaf<string> *l=(Leaf<string> *)leafs->At(leaf_index);
      char buf[30];
      sprintf(buf,"%f",dat);
      l->Push_back(buf);
    }
  }

  void      Push_data(int leaf_index,double dat){
      // Put a float on the leaf at leaf_index.
      if( leafs->At(leaf_index)->IsA() == Leaf<double>::Class() ){
        Leaf<double> *l=(Leaf<double> *)leafs->At(leaf_index);
        l->Push_back(dat);
      }else if( leafs->At(leaf_index)->IsA() == Leaf<int>::Class() ){
        Leaf<int> *l=(Leaf<int> *)leafs->At(leaf_index);
        l->Push_back((int)dat);
      }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
        Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
        l->Push_back(dat);
      }else if( leafs->At(leaf_index)->IsA() == Leaf<float>::Class() ){
        Leaf<float> *l=(Leaf<float> *)leafs->At(leaf_index);
        l->Push_back((float)dat);
      }else if( leafs->At(leaf_index)->IsA() == Leaf<string>::Class() ){
        Leaf<string> *l=(Leaf<string> *)leafs->At(leaf_index);
        char buf[30];
        sprintf(buf,"%f",dat);
        l->Push_back(buf);
      }
    }

  
  void      Push_data(int leaf_index,string dat){
    // Put a float on the leaf at leaf_index.
    if( leafs->At(leaf_index)->IsA() == Leaf<string>::Class() ){
      Leaf<string> *l=(Leaf<string> *)leafs->At(leaf_index);
      l->Push_back(dat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<int>::Class() ){
      Leaf<int> *l=(Leaf<int> *)leafs->At(leaf_index);
      int idat;
      sscanf(dat.c_str(),"%d",&idat);
      l->Push_back(idat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
      Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
      uint32_t idat;
      sscanf(dat.c_str(),"%ud",&idat);
      l->Push_back(idat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<float>::Class() ){
      Leaf<float> *l=(Leaf<float> *)leafs->At(leaf_index);
      float fdat;
      sscanf(dat.c_str(),"%f",&fdat);
      l->Push_back(fdat);
    }else if( leafs->At(leaf_index)->IsA() == Leaf<double>::Class() ){
      Leaf<double> *l=(Leaf<double> *)leafs->At(leaf_index);
      double fdat;
      sscanf(dat.c_str(),"%lf",&fdat);
      l->Push_back(fdat);
    }
  }
  
  
  template<typename T> void  Push_data_vector(string leaf_name, vector<T> &dat){
    // Add the vector to the back of the data of leaf with name leaf_name
    int index = Get_index_from_name(leaf_name);
    if(index<0){
      cout << "Bank::Push_data_vector - ERROR - We do not know about the Leaf called " << leaf_name << endl;
      return;
    }
    
    Push_data_vector(index,dat);
    return;
  }

  template<typename T> void  Push_data_vector(const int idx, vector<T> &dat){
    // Add the vector to the back of the data of the leaf at index idx
    Leaf<T> *ll=(Leaf<T> *)leafs->At(idx);
    ll->Push_data_vector(dat);
  }

  template<typename T> void  Push_data_array(const int idx, const T *dat,const int len){
    // Add the vector to the back of the data of the leaf at index idx
    Leaf<T> *ll=(Leaf<T> *)leafs->At(idx);
    ll->Push_data_array(dat,len);
  }
   
  vector<string>  Get_names();

  size_t Get_Leaf_size(int loc){
    // Return the size of the leaf at index loc
    return ((Leaf_base *)leafs->At(loc))->Get_size();
  }

  
  size_t Get_Leaf_size(string leaf_name){
    // Return the size of the leaf with name leaf_name
    int loc=Get_index_from_name(leaf_name);
    if(loc<0){
      return 0;
    }
    return Get_Leaf_size(loc);
  }
  
  template<typename T> T Get_data(int location, int ind){
    // Return the data for the leaf at location and index ind.
    Leaf<T> *li= (Leaf<T> *)leafs->At(location);
    return li->Get_data(ind);
  }

  template<typename T> T Get_data(string leaf_name, int ind){
    // Return the data for the leaf with leaf_name at index ind.
    int loc=Get_index_from_name(leaf_name);
    if(loc<0){
      return 0;
    }
    return Get_data<T>(loc,ind);
  }

  template<typename T> vector<T> Get_data_vector(int loc){
// Return a vector of the data for leaf at loc
    if( leafs->At(loc)->IsA() != Leaf<T>::Class() ){
      cerr << "Banks::Get_data_vector -- ERROR incorrect vector type for leaf. \n";
      return vector<T>(); // return an empty vector.
    }
    return ((Leaf<T> *)leafs->At(loc))->data;
  }

  template<typename T> vector<T> Get_data_vector(string leaf_name){
// Return the data vector for the leaf with leaf_name.
    int loc=Get_index_from_name(leaf_name);
    return Get_data_vector<T>(loc);
  }
  
  
  Bank  *Get_bank_ptr(string name){
    // Return pointer to bank with name.
    return ((Bank *)banks->FindObject(name.c_str()));
  }

  Bank  *Get_bank_ptr(int idx){
    // Return pointer to bank at idx.
    return (Bank *)banks->At(idx);
  }

  
  int    Get_data_int(string name,int idx);
  float  Get_data_float(string name,int idx);
  double Get_data_double(string name,int idx);
  string Get_data_string(string name,int idx);
  int    Get_data_int(int ind,int idx);
  float  Get_data_float(int ind,int idx);
  double Get_data_double(int ind,int idx);
  string Get_data_string(int ind,int idx);

  inline int Get_index_from_name(string leaf_name){
    // Gets the *location*, i.e type*MAX_DATA + index
    // map<string,unsigned short>::iterator found;
    auto found = name_index.find(leaf_name);
    if( found == name_index.end()){
      cerr << "Bank:: Get_location_from_name -- ERROR - Leaf with name " << leaf_name << " not found. \n";
      return(-1);
    }
    return(found->second);
  }
  
  int Get_size(int type=0){
    return( leafs->GetEntriesFast());
  }
  
  void PrintBank(int print_leaves=0,int depth=10,int level=0);
  
  ClassDef(Bank,1);
};



#endif /* defined(__AprimeAna__Bank__) */
