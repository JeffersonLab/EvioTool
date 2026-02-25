//
//  Bank.h
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/19/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

// The "Bank" class contains a list of "Leaf" nodes, each of the Leaf nodes contains a vector of data.
//
//
//
//
// Some Implementation Notes:
// We must store a vector or TObjArray of pointers, and not objects, because the objects
// will change size and thus cause trouble with the memory allocation of the vector.
// We may as well derive everything (i.e. the leafs and sub-banks) from TObject, so we can simply
// store pointers to TObject, since the whole point of this class is to be ROOT compliant.
// In that situation, we may as well use TObjArray, which is a reasonable implementation, and get Find and
// other useful functionality. I hesitantly will do this the ROOT way. I have an inherent bias towards STL.
//
// Note on OOP versus Generic Programming (STL)
//
//  There is tension between "Generic Programming" i.e. using STL, and "Object Oriented Programming" (OOP).
//  See an explanation at: https://www.artima.com/cppsource/type_erasure.html
//
//  The templates work just fine, most of the time, but they do not allow you to write a super class which overrides these function.
//  That would be OK, if we used STL everywhere, however the ROOT model is OOP, with TObject as the base class, and we run into problems there.
//  We also run into problems for those situations where we want make a special version of the Bank class that understands the contents of the leafs.
//  In those cases, C++ would not know to call the super class when filling.

#ifndef __Bank__
#define __Bank__

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <algorithm>

using namespace std;

#include "TROOT.h"
#include "TObject.h"
#include "TObjArray.h"
#include "TNamed.h"

#include "Leaf.h"
#include "FADCdata.h"

// TODO:
// - Add a fast lookup for tag,num finding.

class Bank : public TNamed {
  // Mimicks a container bank in EVIO, with extra's.
public:
  // tags and tag_mask:
  // Each bank tag is compared to the tags in the following manner:
  // If there is only one tag_masks, then each bank tag is masked with that tag_mask and then compared to see if
  // the result is in the list of tags.
  // If there are as many tag_masks as tags, then each tag_masks element is paired with a tags element.
  // Each bank tag is masked by the tag_mask element, and then compared to only the collesponding tags element.
  vector< unsigned short>tags;  // Tags to select bank with.
  vector<unsigned short> tag_masks = {0xFFFF};  // This mask is applied to a tag before comparing with the tags list.
  unsigned char  num=0;           // num to select bank with. If set to 0, ignore.
  
  unsigned short this_tag=0;      // The actual tag of the parsed bank.
  unsigned char  this_num=0;      // The actual num of the parsed bank.
  
  map<string,unsigned short> name_index;    // Maps names to index.
  TObjArray  *leafs = nullptr;             // Stores the leafs. Must be able to store heterogeneous objects.
  TObjArray  *banks = nullptr;             // Stores other banks. In separate array, since separte content.
  
public:
  Bank(){
    Bank::Init();
  };
  
  ~Bank() override {
    delete leafs;
    delete banks;
  }

  Bank(const string &n, std::vector<unsigned short> itags, unsigned char inum, const string& desc):
            TNamed(n.c_str(),desc.c_str() ), tags(std::move(itags)),num(inum){
    Bank::Init();
  }
  
  Bank(const string &n,unsigned short tag,unsigned char num,const string &desc):
            TNamed(n.c_str(),desc.c_str() ), num(num){
    tags.push_back(tag);
    Bank::Init();
  }
  
  Bank(Bank const &cp)  : TNamed(cp) {
    tags = cp.tags;
    num = cp.num;
    leafs = (TObjArray *)cp.leafs->Clone();
    banks = (TObjArray *)cp.banks->Clone();
  }

  explicit Bank(Bank *cp): TNamed(*cp){
    tags = cp->tags;
    num  = cp->num;
    leafs = (TObjArray *)cp->leafs->Clone();
    banks = (TObjArray *)cp->banks->Clone();
  }
  
  virtual void Init(){
    // Initialize the Bank class.
    leafs = new TObjArray();
    leafs->SetOwner(kTRUE);
    banks = new TObjArray();
    banks->SetOwner(kTRUE);
  }

   virtual void CallBack() {
      // Could be implemented in a derived class to further process the data right after the sub-bank
      // and leaves is filled from an EVIO bank.
  };

  unsigned char GetNum(){ return(this_num);};
  unsigned short GetTag(){ return(this_tag);};
  vector<unsigned short> &GetTags(){return(tags);}
  
  virtual bool CheckTag(const unsigned short tag){
    // Check if the tag passes the required tag test:
    // if tag_masks.size()==1 then bank tag, masked by tag_masks[0], is looked for in the tags list. If found then OK.
    // if tag_masks.size()==tags.size() then for each item the bank tag is masked by tag_masks[i] and compared to tags[i]. If equal, then OK.
    if(!tags.empty() && tag_masks.size()==1){
      return( !(std::find(tags.begin(),tags.end(),(tag&tag_masks[0])) == tags.end())); // If not == end, then found.
    }else if(tags.size() == tag_masks.size()){
      for(int i=0;i<tags.size();++i){
        if( (tag&tag_masks[i]) == tags[i] ) return(true);
      }
      return(false); // Not found for any i, so false.
    }else if(tags.empty()){
      return(true);
    }else{
      std::cerr << "ERROR - Bank::CheckTag -- inconsistend tags and tag_masks definiion.\n";
      return(true);
    }
  };
  
  // Add a new leaf type to this bank. COPY the leaf into the array.
  virtual int AddLeaf(string name,unsigned short itag, unsigned char inum,string desc,int type);
  template<typename T> Leaf<T> * Add_Leaf(Leaf<T> &leaf){
    int location= leafs->GetEntriesFast();
    string name=StoreLocation(leaf.GetName(),location);
    auto *new_leaf = new Leaf<T>(leaf); //// COPY the leaf, don't ref to original.
    leafs->Add(new_leaf);
    return(new_leaf);
  }

  // Add a new leaf type to this bank. DON'T COPY the leaf into the array.
  template<typename T> void AddThisLeaf(Leaf<T> *new_leaf){
    int location= leafs->GetEntriesFast();
    string name=StoreLocation(new_leaf->GetName(),location);
    leafs->Add(new_leaf);
 }
  
  template<typename T> Leaf<T> *AddLeaf(string name,unsigned short itag,unsigned char inum, string desc){
    // Create a new leaf and store it.
    int location= leafs->GetEntriesFast();
    name=StoreLocation(name,location);
    auto *new_leaf =new Leaf<T>(name,itag,inum,desc);
    leafs->Add(new_leaf);
    return(new_leaf);
  }
  
  template<typename T> void RemoveLeaf(const string &name){
    // Remove the leaf with name from the leaf list.
    //
    int leaf_loc = FindLeaf(name);
    if( leaf_loc<0){
      cout << "Bank::RemoveLead - Trying to remove a leaf I do not own: " << name << endl;
      return;
    }
    Leaf<T> *this_leaf = leafs->At(leaf_loc);
    delete this_leaf;
    leafs->RemoveAt(leaf_loc);      // Remove from the leafs.
    name_index.erase(name);         // Remove from the index
  }
  
  virtual Bank *AddBank(const string &name,unsigned short itag,unsigned char inum, const string &desc,
                        bool override=false){
    // Add a Bank witn name,tag,num,description.
    // Returns a pointer to the new bank.
    // int location = banks->GetEntriesFast();
    // name=StoreLocation(name,location);
    // Modification for 2022: First check if *that* combination of bank with tag and num is added already.
    // If it is, then return the pointer to the bank that was added before *unless* you specify override=true.
    int idx = FindBank(itag, inum);
    if( idx < 0 || override) {
       Bank *newbank = new Bank(name, itag, inum, desc);
       banks->Add(newbank);
       return (newbank);
    }else{
       Bank *foundbank = (Bank *)banks->At(idx);
       return foundbank;
    }
  }

  virtual Bank *AddBank(const string &name,std::vector<unsigned short> itags, unsigned char inum, const string &desc){
    // Add a Bank witn name,tag,num,description.
    // Returns a pointer to the new bank.
    // int location = banks->GetEntriesFast();
    // name=StoreLocation(name,location);
    Bank *newbank = new Bank(name, std::move(itags), inum, desc);
    banks->Add(newbank);
    return(newbank);
  }

  void AddBank(Bank *b){
      // Add the already created bank b to the list of banks.
      if(b) banks->Add(b);
      else{
          cout << "Cowardly not adding a null pointer to the banks list.\n";
      }
  }
  
  void RemoveBank(const string &name){
    // Remove the bank with name and delete.
    TObject *o = banks->FindObject(name.c_str());
    if( o == nullptr){
      cout << "Bank::RemoveBank - Trying to remove a bank I do not own: " << name << endl;
      return;
    }
    RemoveBank(o);
  }
  
  void RemoveBank(TObject *o){
    // Remove object o from banklist and delete.
    TObject *r = banks->Remove(o);
    if( r == nullptr){
      cout << "Bank::RemoveBank - Trying to remove a bank by pointer, but I do not own it. \n";
      return;
    }
    delete r;
    banks->Compress();
  }

  void  Clear(Option_t*) override;
  virtual void  Clear(){this->Clear("");}

  string StoreLocation(string name,unsigned short location){
    // Store the location under name, make sure name is unique!
    auto loc=name_index.find(name);
    if(loc == name_index.end()){
      name_index[name]=location;
    }else{
      cerr << "Bank::StoreLocation -- ERROR -- name " << name << " already in bank. \n";
      name+="+";
      name = StoreLocation(name,location);
    }
    return(name);
  }
  
  virtual int FindLeaf(const string &name){
    // Find the leaf item with name, return location.
    // If not found, return -1.
    auto loc=name_index.find(name);
    if(loc != name_index.end()){
      return loc->second;
    }else return( -1);
  }
  
  virtual int FindLeaf(unsigned short itag,unsigned char inum){
    // Find the location of the leaf with num, tag.
    // Returns the location, or -1 if not found.
    // If a leaf has num=0, then inum is ignored.
    // This is rarely a problem, BUT, rarely there are duplicate leafs distinguished only by the leaf num.
    // We used to test inum == 0, but then there is no longer any way to distinguish these leaves.
    // So now, if you expect multiple leafs with different num, you should book them with 0 last, as a catchall.
    // TODO: Make this an efficient search.
    for(int i=0;i<leafs->GetEntriesFast();++i){
      if( ((Leaf_base *)leafs->At(i))->tag == itag &&
          ( ((Leaf_base *)leafs->At(i))->num == 0 ||
             // inum==0 ||
             ((Leaf_base *)leafs->At(i))->num == inum ) ){
        return(i);
      }
    }
    return(-1);
  }

  virtual int FindBank(const string &name){
    // Find a bank by name.
    TObject *o = banks->FindObject(name.c_str());
    int idx=banks->IndexOf(o);
    return(idx);
  }
  
  virtual int FindBank(unsigned short itag,unsigned char inum){
    // Find the location of the bank with tag, num.
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

  
  virtual void  PushDataArray(const int idx, const unsigned long long *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<unsigned long long> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }

  virtual void  PushDataArray(const int idx, const long long *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<long long> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }

  virtual void  PushDataArray(const int idx, const unsigned int *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<unsigned int> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }
  
  virtual void  PushDataArray(const int idx, const int *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<int> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }

  virtual void  PushDataArray(const int idx, const unsigned short *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<unsigned short> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }
  
  virtual void  PushDataArray(const int idx, const short *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<short> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }

  
  virtual void  PushDataArray(const int idx, const unsigned char *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<unsigned char> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }

  virtual void PushDataArray(const int idx, const char *dat,const int len){
    // Add the vector to the leaf at index.
    // Put a buffer of char into the string if Leaftype is string
    auto *ll=(Leaf<char> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }
  
  virtual void  PushDataArray(const int idx, const double *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<double> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }
  
  virtual void  PushDataArray(const int idx, const float *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<float> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }
  
  virtual void  PushDataArray(const int idx, const FADCdata *dat,const int len){
    //    // Add the vector to the back of the data of the leaf at index idx
    auto *ll=(Leaf<FADCdata> *)leafs->At(idx);
    ll->PushDataArray(dat,len);
  }

  // Specialized version for an array of character strings.
 virtual void PushDataArray(int index, const string *dat, int len){
    // Add the vector to the leaf at index.
    // Put a buffer of char into the string if Leaftype is string
    const char *start =(const char *) dat;
    char *c = (char *)start;
    string s;
    while((c[0]!=0x4)&&((c-start)< len)) {
      char *n=c;
      while( std::isprint(*n++)){;};
      if( (n-c-1)>0){
        s.assign(c,n-c-1); // This chomps off the non-print, usually \n.
        ((Leaf<string> *)leafs->At(index))->PushBack(s);
      }
      c=n;
    }
  }
  
  vector<string>  GetNames();

  int GetNumLeaves(){
    return( leafs->GetEntries());
  }

  int GetNumBanks(){
    return( banks->GetEntries());
  }

  
  size_t GetLeafSize(int loc){
    // Return the size of the leaf at index loc
    return ((Leaf_base *)leafs->At(loc))->Size();
  }
  
  size_t GetLeafSize(const string &leaf_name){
    // Return the size of the leaf with name leaf_name
    int loc=GetIndexFromName(leaf_name);
    if(loc<0){
      return 0;
    }
    return GetLeafSize(loc);
  }
  
  template<typename T> T GetData(int location, int ind){
    // Return the data for the leaf at location and index ind.
    auto *li= (Leaf<T> *)leafs->At(location);
    return li->GetData(ind);
  }

  template<typename T> T GetData(const string &leaf_name, int ind){
    // Return the data for the leaf with leaf_name at index ind.
    int loc=GetIndexFromName(leaf_name);
    if(loc<0){
      return 0;
    }
    return GetData<T>(loc,ind);
  }

  template<typename T> vector<T> &GetDataVector(int loc){
    // Return a reference to the vector of the data for leaf at loc
    // Warning: The reference may go out of scope when the Bank goes out of scope!
//    if( leafs->At(loc)->IsA() != Leaf<T>::Class() ){
//      cerr << "Banks::Get_data_vector -- ERROR incorrect vector type for leaf. \n";
//      return 0; // return an empty vector.
//    }
    return( (static_cast<Leaf<T> *>(leafs->At(loc))->data) );
  }

  template<typename T> vector<T> &GetDataVector(const string &leaf_name){
  // Return a reference to the data vector for the leaf with leaf_name.
    int loc=GetIndexFromName(leaf_name);
    return GetDataVector<T>(loc);
  }

  template<typename T> vector<T> *GetDataVectorPtr(int loc){
    // Return a pointer to the vector of the data for leaf at loc
    if( leafs->At(loc)->IsA() != Leaf<T>::Class() ){
      cerr << "Banks::Get_data_vector -- ERROR incorrect vector type for leaf. \n";
      return nullptr; // return an empty vector.
    }
    return( &(static_cast<Leaf<T> *>(leafs->At(loc))->data) );
  }
  
  template<typename T> vector<T> *GetDataVectorPtr(const string &leaf_name){
    // Return a reference to the data vector for the leaf with leaf_name.
    int loc=GetIndexFromName(leaf_name);
    return GetDataVector<T>(loc);
  }

  
  virtual Bank  *GetBankPtr(const string &name){
    // Return pointer to bank with name.
    return ((Bank *)banks->FindObject(name.c_str()));
  }

  virtual Bank  *GetBankPtr(int idx){
    // Return pointer to bank at idx.
    return (Bank *)banks->At(idx);
  }

  // Full acknowledgement that these are redundant with GetData template version!
  virtual int    GetDataInt(string name,int idx);
  virtual float  GetDataFloat(string name,int idx);
  virtual double GetDataDouble(string name,int idx);
  virtual string GetDataString(string name,int idx);
  virtual int    GetDataInt(int ind,int idx);
  virtual float  GetDataFloat(int ind,int idx);
  virtual double GetDataDouble(int ind,int idx);
  virtual string GetDataString(int ind,int idx);

  virtual inline int GetIndexFromName(const string &leaf_name){
    // Gets the *location*, i.e type*MAX_DATA + index
    // map<string,unsigned short>::iterator found;
    auto found = name_index.find(leaf_name);
    if( found == name_index.end()){
      cerr << "Bank:: Get_location_from_name -- ERROR - Leaf with name " << leaf_name << " not found. \n";
      return(-1);
    }
    return(found->second);
  }
  
  virtual size_t size(){
    return( leafs->GetEntriesFast());
  }

  virtual void PrintBank(int print_leaves,int depth,int level);
  virtual void PrintBank(){this->PrintBank(10,10,0);}
  virtual void PrintBank(int print_leaves){this->PrintBank(print_leaves,10,0);}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  ClassDef(Bank,1);
#pragma clang diagnostic pop
};



#endif /* defined(__Bank__) */

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           SCRAP HEAP OF DISCARDED IDEAS
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//  template<typename T> void  Push_data(string leaf_name, T dat){
//    // Push an individual data element to the end of the leaf with leaf_name
//    int index = Get_index_from_name(leaf_name);
//    if(index<0){
//      cout << "Bank::Push_data - ERROR - We do not know about the Leaf called " << leaf_name << endl;
//      return;
//    }
//    Push_data(index,dat);
//    return;
//  }
//
//  template<typename T> void Push_data(int leaf_index,T dat){
//    // Add data to the end of leaf at leaf_index.
//    if( leafs->At(leaf_index)->IsA() == Leaf<T>::Class() ){
//      Leaf<T> *l=(Leaf<T> *)leafs->At(leaf_index);
//      l->Push_back(dat);
//    }else{   /// This is mis-use of the method. Carry on best as we can.
//      cerr << "Leaf::Push_data("<<leaf_index<<","<< dat << ") -- Trying to add incorrect data type to leaf\n";
//    }
//  }
//
//
//  Here there is tension between "Generic Programming" i.e. using STL, and "Object Oriented Programming" (OOP).
//  See an explanation at: https://www.artima.com/cppsource/type_erasure.html
//
//  The templates work just fine, most of the time, but they do not allow you to write a super class which overrides these function.
//  That would be OK, if we used STL everywhere, however the ROOT model is OOP, with TObject as the base class, and we run into problems there.
//  We also run into problems for those situations where we want make a special version of the Bank class that understands the contents of the leafs.
//  In those cases, C++ would not know to call the super class when filling.
//
//  template<typename T> void  Push_data_vector(string leaf_name, vector<T> &dat){
//    // Add the vector to the back of the data of leaf with name leaf_name
//    int index = Get_index_from_name(leaf_name);
//    if(index<0){
//      cout << "Bank::Push_data_vector - ERROR - We do not know about the Leaf called " << leaf_name << endl;
//      return;
//    }
//
//    Push_data_vector(index,dat);
//    return;
//  }
//
//  template<typename T> void  Push_data_vector(const int idx, vector<T> &dat){
//    // Add the vector to the back of the data of the leaf at index idx
//    Leaf<T> *ll=(Leaf<T> *)leafs->At(idx);
//    ll->Push_data_vector(dat);
//  }
//
//  template<typename T> void  Push_data_array(const int idx, const T *dat,const int len){
//    // Add the vector to the back of the data of the leaf at index idx
//    Leaf<T> *ll=(Leaf<T> *)leafs->At(idx);
//    ll->Push_data_array(dat,len);
//  }
//

//
// Unfortunately, OOP design requires one Push_data_array for every possible type that we support.
//











// The commented out template method above works, except when you try to put 123 in a Leaf<double>
// or other constructs that you would expect to work with automatic casting.
// For the purpose of usability (and ROOT macro sloppiness) we specialize the methods:
//void  Push_data(int leaf_index,int dat){
//  // Put an int on the leaf at leaf_index. The leaf can be an int, float, double or string.
//  if( leafs->At(leaf_index)->IsA() == Leaf<int>::Class() ){
//    Leaf<int> *l=(Leaf<int> *)leafs->At(leaf_index);
//    l->Push_back(dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
//    Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
//    l->Push_back(dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<float>::Class() ){
//    Leaf<float> *l=(Leaf<float> *)leafs->At(leaf_index);
//    l->Push_back((float)dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<double>::Class() ){
//    Leaf<double> *l=(Leaf<double> *)leafs->At(leaf_index);
//    l->Push_back((double)dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<string>::Class() ){
//    Leaf<string> *l=(Leaf<string> *)leafs->At(leaf_index);
//    char buf[30];
//    sprintf(buf,"%d",dat);
//    l->Push_back(buf);
//  }
//}
//
//void Push_data(int leaf_index,float dat){
//  // Put a float on the leaf at leaf_index.
//  if( leafs->At(leaf_index)->IsA() == Leaf<float>::Class() ){
//    Leaf<float> *l=(Leaf<float> *)leafs->At(leaf_index);
//    l->Push_back(dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<int>::Class() ){
//    Leaf<int> *l=(Leaf<int> *)leafs->At(leaf_index);
//    l->Push_back((int)dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
//    Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
//    l->Push_back(dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
//    Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
//    l->Push_back(dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<double>::Class() ){
//    Leaf<double> *l=(Leaf<double> *)leafs->At(leaf_index);
//    l->Push_back((double)dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<string>::Class() ){
//    Leaf<string> *l=(Leaf<string> *)leafs->At(leaf_index);
//    char buf[30];
//    sprintf(buf,"%f",dat);
//    l->Push_back(buf);
//  }
//}
//
//void      Push_data(int leaf_index,double dat){
//  // Put a float on the leaf at leaf_index.
//  if( leafs->At(leaf_index)->IsA() == Leaf<double>::Class() ){
//    Leaf<double> *l=(Leaf<double> *)leafs->At(leaf_index);
//    l->Push_back(dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<int>::Class() ){
//    Leaf<int> *l=(Leaf<int> *)leafs->At(leaf_index);
//    l->Push_back((int)dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
//    Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
//    l->Push_back(dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<float>::Class() ){
//    Leaf<float> *l=(Leaf<float> *)leafs->At(leaf_index);
//    l->Push_back((float)dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<string>::Class() ){
//    Leaf<string> *l=(Leaf<string> *)leafs->At(leaf_index);
//    char buf[30];
//    sprintf(buf,"%f",dat);
//    l->Push_back(buf);
//  }
//}
//
//
//void      Push_data(int leaf_index,string dat){
//  // Put a float on the leaf at leaf_index.
//  if( leafs->At(leaf_index)->IsA() == Leaf<string>::Class() ){
//    Leaf<string> *l=(Leaf<string> *)leafs->At(leaf_index);
//    l->Push_back(dat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<int>::Class() ){
//    Leaf<int> *l=(Leaf<int> *)leafs->At(leaf_index);
//    int idat;
//    sscanf(dat.c_str(),"%d",&idat);
//    l->Push_back(idat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<uint32_t>::Class() ){
//    Leaf<uint32_t> *l=(Leaf<uint32_t> *)leafs->At(leaf_index);
//    uint32_t idat;
//    sscanf(dat.c_str(),"%ud",&idat);
//    l->Push_back(idat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<float>::Class() ){
//    Leaf<float> *l=(Leaf<float> *)leafs->At(leaf_index);
//    float fdat;
//    sscanf(dat.c_str(),"%f",&fdat);
//    l->Push_back(fdat);
//  }else if( leafs->At(leaf_index)->IsA() == Leaf<double>::Class() ){
//    Leaf<double> *l=(Leaf<double> *)leafs->At(leaf_index);
//    double fdat;
//    sscanf(dat.c_str(),"%lf",&fdat);
//    l->Push_back(fdat);
//  }
//}
