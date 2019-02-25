//
//  Bank.cc
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/19/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

#include "Bank.h"

ClassImp(Bank);

int  Bank::AddLeaf(string name,unsigned short itag,unsigned char inum,string desc,int type){
  // Add a leaf to the Bank with definitions given in the parameters.
  int location= leafs->GetEntriesFast();
  name=StoreLocation(name,location);
  // Find the correct location in the vectors depending on type,
  // Create a new Leaf of type in correct vector at location.
  switch(type){
    case Leaf_Int:
      leafs->Add(new Leaf<int>(name,itag,inum,desc));
      break;
    case Leaf_Float:
      leafs->Add(new Leaf<float>(name,itag,inum,desc));
      break;
    case Leaf_Double:
      leafs->Add(new Leaf<double>(name,itag,inum,desc));
      break;
    case Leaf_String:
      leafs->Add(new Leaf<string>(name,itag,inum,desc));
      break;
    case Leaf_Bank:
      cerr << "Please add a bank by calling Add_Bank() \n";
      return(-1);
      break;

    default:
      cout << "ERROR -- Adding a leaf with unknown type: " << type << endl;
      return -1;
  }
  return(location);
}

vector<string> Bank::GetNames(){
  // Get all the names stored in the bank for type.
  vector<string> out;
  // map<string,unsigned short>::iterator it;
  for(auto it = name_index.begin(); it != name_index.end(); ++it){
    out.push_back(it->first);
  }
  return( out );
}


int Bank::GetDataInt(string name,int idx){
  return GetData<int>(name,idx);
}
float Bank::GetDataFloat(string name,int idx){
  return GetData<float>(name,idx);
}
double Bank::GetDataDouble(string name,int idx){
  return GetData<double>(name,idx);
}
string Bank::GetDataString(string name,int idx){
  return GetData<string>(name,idx);
}

int Bank::GetDataInt(int ind,int idx){
  return GetData<int>(ind,idx);
}

float Bank::GetDataFloat(int ind,int idx){
  return GetData<float>(ind,idx);
}
double Bank::GetDataDouble(int ind,int idx){
  return GetData<double>(ind,idx);
}
string Bank::GetDataString(int ind,int idx){
  return GetData<string>(ind,idx);
}

void Bank::Clear(Option_t*opt){
  // Clears out all the data vectors to ready for new data.
  // Do NOT clear the bank properties.
  //
  // This would be the normal clear between events.
  // For a complete clear, including the banks, give option="Full" or "F"
  
  if( opt[0]=='F'){
    leafs->Clear();
    banks->Clear();
  }else{
    for(int i=0; i< leafs->GetEntriesFast(); ++i){
      leafs->At(i)->Clear();
    }
    for(int i=0; i< banks->GetEntriesFast(); ++i){
      banks->At(i)->Clear();
    }

  }
}

void Bank::PrintBank(int print_leaves, int depth, int level){
  // Print the contents of this bank to depth. The level parameter keeps track of the depth.
  // If print_leaves = 0, no leaves are printed, else a maximum of print_leaves leave content is printed.
  
  string s;
  for(int i=0;i<level;i++) s.append("    ");
  level++;
  char opts[48];
  sprintf(opts,"N%03dL%03d",print_leaves,level);
  cout << s << "Bank: " << GetName() << "\t tags= [";
  for(unsigned short t: tags){
    cout<<t << ",";
  }
  cout << "] num = " << (int)num << endl;
  if(print_leaves && leafs->GetEntriesFast() ){
    cout << s << "-----------------------------------------------------------------------\n";
    for(int i=0; i< leafs->GetEntriesFast() && i<print_leaves; ++i){
      leafs->At(i)->Print(opts);
    }
    cout << endl;
  }
  if(level<=depth){
    for(int j=0;j< banks->GetEntriesFast(); ++j){
      Bank *b=(Bank *)banks->At(j);
      b->PrintBank(print_leaves,depth,level);
    }

  }
  
}

