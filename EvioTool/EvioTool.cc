//
//  EvioTool.cpp
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/17/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

// TODO:
// - Add a type for 0x0f - FADC format banks = Mode 7
// - Add types for uint32_t = unsigned int
// - A

#include "EvioTool.h"

//----------------------------------------------------------------------------------
// EvioTool
//----------------------------------------------------------------------------------

EvioTool::EvioTool(): Bank("EvioTool",{},0,"The top node of the EVIO tree"){

}

int EvioTool::Open(const char *filename,const char *dictf){
    // Open an EVIO file for parsing.
    int stat;
    if((stat=evOpen((char *)filename,(char *)"r",&evio_handle))!=S_SUCCESS){
        cerr << "EvioTool::Open -- ERROR -- Could not open file " << filename << endl;
        return(1);
    }
    
    char *d;
    uint32_t len;
    stat=evGetDictionary(evio_handle,&d,&len);
    if((stat==S_SUCCESS)&&(d!=NULL)&&(len>0)){
        cout << "The Evio file has a dictionary, but I don't have a parser yet.\n";
    }
    return(0);
}

void EvioTool::Close(){
  evClose(evio_handle);
}

void EvioTool::parseDictionary(const char *dictf){
    // Open the file pointed to by dictf and parse the XML as a dictionary for the file.
    
    //  ifstream f(dictf);
    //  Dictionary = new evioDictionary(f);
}

int EvioTool::NextNoParse(){
  // Read an event from the EVIO file, but don't parse it.
  
  int stat=evReadNoCopy(evio_handle,&evio_buf,&evio_buflen);
  if(stat==EOF) return(0);
  if(stat!=S_SUCCESS){
    cerr << "EvioTool::Next() -- ERROR -- problem reading file. \n";
    return(-1);
  }
  
  return 1;
}

int EvioTool::Next(){
    // Read an event from the EVIO file and parse it.
    // Returns 0 on success, or an error code if not.
    
    int stat=evReadNoCopy(evio_handle,&evio_buf,&evio_buflen);
    if(stat==EOF) return(-1);
    if(stat!=S_SUCCESS){
        cerr << "EvioTool::Next() -- ERROR -- problem reading file. \n";
        return(-2);
    }
    if(fFullErase) Clear("Full"); // Clear out old data.
    else Clear();
    
    stat=ParseEvioBuff(evio_buf);  // Recursively parse the banks in the buffer.
    
    return stat;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Template Specification on how to fill leafs of specific not simple types.
//
// These are specification for: template<typename T> int AddOrFillLeaf(const unsigned int *buf,int len,int tag,int num,Bank *node)
//
//////////////////////////////////////////////////////////////////////////////////////////

int EvioTool::AddOrFillLeaf_FADCdata(const unsigned int *buf,int len,unsigned short tag,unsigned char num,Bank *node){
  // Add or Fill a float leaf in the bank node.
  // If fAutoAdd is false, find the leaf with tag, num and fill it. If not found do nothing.
  // If fAutoAdd is true, if not found, a new leaf is added and filled.
  int loc = node->FindLeaf(tag,num);
  if( loc == -1){
    if(fAutoAdd){
      char str[100];
      sprintf(str,"FADC-%u-%u",tag,num);
      if(fDebug&Debug_L2) cout << "Adding a new Leaf node to node: " << node->GetNum() << " with name: " << str << endl;
      node->AddLeaf<FADCdata>(str,tag,num,"Auto added string leaf");
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


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Main Parsing Code
//
// This code is derived from the evioUtil class written by Eliott Wolin.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

int EvioTool::ParseEvioBuff(const unsigned int *buf){
  
// Assumes data type 0x10 for the top EVIO bank.
  this_tag    = buf[1]>>16;
  this_num    = buf[1]&0xff;

  unsigned short contentType = (buf[1]>>8)&0x3f;
  
  if( fChop_level>0){ // This means the EvioTool is the top level. Check if OK.
    if(contentType != 0x10 && contentType != 0x0e){ // So the events better be of Container type and not Leaf type.
      printf("ERROR Parsing EVIO file. Top level is not 0x10 or 0x0e container type but: 0x%02x \n",contentType);
      printf("Aborting event \n");
      return(-3);
    }
    // Check if the event has the desired tag number. Skip if not.
    if(tags.size() && std::find(tags.begin(),tags.end(),this_tag) == tags.end()){ // Event tag not found in tags list, so skip it.
      if(fDebug&Debug_Info2) cout << "Event of tag = " << this_tag << " skipped \n";
      return(S_SUCCESS);
    }
    
//    int length      = buf[0]+1;
//    int padding     = (buf[1]>>14)&0x3;
//    int dataOffset  = 2;
  }

  ParseBank(buf, 0x10,0,this);
  
  return(S_SUCCESS);
}

int EvioTool::ParseBank(const unsigned int *buf, int bankType, int depth, Bank *node){
    
    int length,dataOffset,p,bankLen;
    int contentType;
    uint16_t tag;
    uint8_t num;
    const uint32_t *data;
    uint32_t mask;
    int padding;
    
    /* get type-dependent info */
    switch(bankType) {
            
        case 0xe:
        case 0x10:
            length  	= buf[0]+1;
            tag     	= buf[1]>>16;
            contentType	= (buf[1]>>8)&0x3f;
            num     	= buf[1]&0xff;
            padding     = (buf[1]>>14)&0x3;
            dataOffset  = 2;
            break;
            
        case 0xd:
        case 0x20:
            length  	= (buf[0]&0xffff)+1;
            tag     	= buf[0]>>24;
            contentType = (buf[0]>>16)&0x3f;
            num     	= 0;
            padding     = (buf[0]>>22)&0x3;
            dataOffset  = 1;
            break;
            
        case 0xc:
        case 0x40:
            length  	= (buf[0]&0xffff)+1;
            tag     	= buf[0]>>20;
            contentType	= (buf[0]>>16)&0xf;
            num     	= 0;
            padding     = 0;
            dataOffset  = 1;
            break;
            
        default:
            cerr << "EvioTool::ParseBank -- ERROR -- illegal bank type: " << bankType << endl;
            return(-4);
    }
    
    
    //
    // Look at the content of the bank.
    // If it is of leaf-type, then fill the appropiate leaf and add it to the
    // current Bank.
    // If it is a container type, then recursively call this routine building
    // more containers.
    //
    
    int stat=S_SUCCESS;
    int i;
    Bank *new_node;
  
    buf += dataOffset;
    int len = length-dataOffset;
  
    if( (fDebug & Debug_Info2) && contentType < 0x10){
      for(i=0;i<depth;i++) cout << "    ";
      cout<< "L["<<depth<<"] parent= "<<node->GetName() <<" type = "<< contentType << " tag= " << tag << " num= " << (int)num<< endl;
    }

    switch (contentType) {
      case 0x0:   // uint32_t
      case 0x1:   // uint32_t
        stat = AddOrFillLeaf<unsigned int>(buf,len,tag, num, node);
        break;
      case 0x2:   // float
        stat = AddOrFillLeaf<float>(buf, len, tag, num, node);
        break;
      case 0x3:    // char's
        stat = AddOrFillLeaf<string>(buf,len*4-padding,tag, num, node);
        //        stat = AddOrFillLeaf_string((const char *)buf,len*4-padding,tag, num, node);
        break;
        
      case 0x8: // double
        stat = AddOrFillLeaf<double>(buf, len*2-padding/2, tag, num, node);
        break;
        
      case 0xb:   // int32_t = int
        stat = AddOrFillLeaf<int>(buf, len, tag, num, node);
        break;
      case 0xf:   // FADC compound type
        stat = AddOrFillLeaf_FADCdata(buf, len, tag, num, node);
        break;
        // --------------------- one-byte types
      case 0x4:   // int16_t = short
        stat = AddOrFillLeaf<short>(buf,len,tag, num, node);
        break;
      case 0x5:   // uint16_t = unsigned short
        stat = AddOrFillLeaf<unsigned short>(buf,len,tag, num, node);
        break;
      case 0x6:  // int8_t = char
        stat = AddOrFillLeaf<char>(buf,len,tag, num, node);
        break;
      case 0x7:  // uint8_t = unsigned char
        stat = AddOrFillLeaf<unsigned char>(buf,len,tag, num, node);
        break;
      case 0x9: // int64_t
        stat = AddOrFillLeaf<long long>(buf,len,tag, num, node);
        break;
      case 0xa: // uint64_t
        stat = AddOrFillLeaf<unsigned long long>(buf,len,tag, num, node);
        break;

        
            //newLeaf=handler.leafNodeHandler(length,bankType,contentType,tag,num,depth,
            //                              &buf[0],(length-dataOffset)/2,(int64_t*)(&buf[dataOffset]),userArg);
//            stat = LeafNodeHandler(&buf[dataOffset],(length-dataOffset),padding,contentType,tag,(int)num,node);
//
//            break;
        
        case 0xc:
        case 0xd:
        case 0xe:
        case 0x10:
        case 0x20:
        case 0x40:
            // container types
            //newUserArg=handler.containerNodeHandler(length,bankType,contentType,tag,num,depth,&buf[0],length-dataOffset,&buf[dataOffset],userArg);
            
  //          new_node = ContainerNodeHandler(&buf[dataOffset],(length-dataOffset),padding,contentType,tag,(int)num,node,depth);
            new_node = ContainerNodeHandler(buf,len,padding,contentType,tag,(int)num,node,depth);
            // parse contained banks
            p       = 0;
            bankLen = len;
            data    = buf;
            mask    = ((contentType==0xe)||(contentType==0x10))?0xffffffff:0xffff;
            
            if(fDebug & Debug_Info2){
                for(i=0;i<depth;i++) cout << "    ";
                if(new_node)  cout<< "C["<<depth<<"] parent= "<<node->GetName() << " node= " << new_node->GetName() << "  tag= " << tag << " num= " << (int)num<<  endl;
                else          cout<< "C["<<depth<<"] parent= "<<node->GetName() << " node=  skipped   tag= " << tag << " num= " << (int)num<<  endl;
            }
            depth++;
            while(p<bankLen) {
                if(new_node) ParseBank(&data[p],contentType,depth,new_node);  // Recurse to read one level deeper.
                p+=(data[p]&mask)+1;
            }
            depth--;
            if(fDebug & Debug_Info2){
                for(i=0;i<depth;i++) cout << "    ";
                cout<< "C["<<depth<<"] parent= "<<node->GetName() <<  endl;
            }
            
            break;
            
        default:
            cerr << "EvioTool::ParseBank -- ERROR -- illegal bank contents. \n";
            return(-5);
    }
    return(stat);
};

Bank *EvioTool::ContainerNodeHandler(const unsigned int *buf, int len, int padding, int contentType, unsigned short tag,unsigned char num, Bank *node,int depth){
  
  if(depth<fChop_level || depth > fMax_level){  // We are pruning the tree.
    if(fDebug & Debug_L2) cout << "EvioTool::ContainNodeHandler -- pruning the tree depth=" << depth << endl;
      node->this_tag = tag; // TODO: VERIFY that this is correct and needed here.
      node->this_num = num;
    return node;
  }
  
  int loc = node->FindBank(tag,num);
  if( loc == -1){
    if(fAutoAdd){
      char str[100];
      sprintf(str,"Bank-%d-%d",tag,num);
      if(fDebug&Debug_L2) cout << "Adding a new Bank to " << node->GetName() << " with name: " << str << endl;
      loc=node->banks->GetEntriesFast();
      node->AddBank(str,tag,num,"Auto added Bank");
    }else{
      if(fDebug&Debug_L2) cout << "Not adding a new bank for tag= " << tag<< " num= " << num << endl;
      return NULL;
    }
  }
  Bank *new_node = (Bank *)node->banks->At(loc);
  new_node->this_tag=tag;
  new_node->this_num=num;
  return(new_node);
};



void EvioTool::Test(void){
    Bank *test_bank = AddBank("test_bank",10,10,"A test bank");
    test_bank->AddLeaf<int>("First",1,1,"The first leaf");
    test_bank->AddLeaf("Second",1,2,"The second leaf",Leaf_Int);
    Leaf<int> new_leaf("New",1,3,"A new leaf");
    Add_Leaf(new_leaf);
    
    AddLeaf<double>("D1",1,4,"Second leaf Double");
    AddLeaf<string>("S1",1,5,"Third leaf, string");
    
    vector<int> test_int;
    string name="First";
    for(int i=10;i<13;i++) test_int.push_back(i);
//    test_bank->Push_data_vector<int>(name,test_int);
//    //
//    for(int i=25;i<30;i++) test_bank->Push_data<int>("First",i );
//    double f;
//    for(int i=25;i<30;i++) {
//        f=(double)i/7.;
//        Push_data("D1", f);
//    }
//    Push_data<double>("D1", 1.23456678);
//    
//    Push_data("D1",1.23456009);
//    Push_data("D1",123);
//    Push_data("New",1234);
//    Push_data("New",(int)123.4);
//    unsigned int ui=12345;
//    Push_data<int>("New",ui);
//    short si=321;
//    Push_data<int>("New",si);
//    
//    Push_data("S1","This is OK");
//    string text2="It does work, indeed";
//    Push_data("S1",text2);
//    PrintBank(20,10,0);
}

ClassImp(EvioTool);
