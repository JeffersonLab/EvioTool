//
//  EvioTool.cpp
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/17/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

#include "EvioTool.h"

//----------------------------------------------------------------------------------
// EvioTool
//----------------------------------------------------------------------------------

EvioTool::EvioTool(string infile): Bank("EvioTool",{},0,"The top node of the EVIO tree"){
  if( infile.size() > 1){
    Open(infile.c_str());
  }
}

int EvioTool::Open(const char *filename,const char *dictf){
  // Open an EVIO file for parsing.
  int stat;
//  string title="Evio tree for ";
//  title.append(filename);
  if((stat=evOpen((char *)filename,(char *)"r",&fEvioHandle)) != S_SUCCESS){
    cerr << "EvioTool::Open -- ERROR -- Could not open file " << filename << endl;
    fIsOpen = false;
    return(1);
  }
  fIsOpen = true;
//  char *d;
//  uint32_t len;
//  stat=evGetDictionary(fEvioHandle,&d,&len);
//  if((stat==S_SUCCESS)&&(d!=NULL)&&(len>0)){
//    cout << "The Evio file has a dictionary, but I don't have a parser yet.\n";
//  }
  return(0);
}

int EvioTool::OpenEt(const string station_name, const string et_name, const string host,
                     unsigned short port, int pos, int ppos, bool no_block){
   // Open an ET system channel to read the events from.
   // Arguments:
   // station_name  - Arbitrary station name
   // et_name       - File name of the ET buffer, see -f for et_start.
   // host          - host name the ET system is running on.
   // port          - port number the ET system is listening on, default 11111
   // pos           - requested relative position in the ET system ring.
   // ppos          - requested parallel position in the ET system.

   fReadFromEt = true;
   et_openconfig   openconfig;
   et_statconfig   sconfig;
   et_stat_id      my_stat;
   int sendBufSize=0;
   int recvBufSize=0;
   int queue_size = 1;
   int noDelay=0;

   // Open the ET system
   et_open_config_init(&openconfig);
   et_open_config_sethost(openconfig, host.c_str());
   et_open_config_setcast(openconfig, ET_DIRECT);
   et_open_config_setserverport(openconfig, port);
   et_open_config_settcp(openconfig, recvBufSize, sendBufSize, noDelay);
   if (et_open(&fEventId, et_name.c_str(), openconfig) != ET_OK) {
      std::cout << "EvioTool::OpenEt - et_open problems\n";
      et_open_config_destroy(openconfig);
      fReadFromEt = false;
      return(ET_ERROR);
   }
   et_open_config_destroy(openconfig);
   if(fDebug & EvioTool_Debug_Info2) et_system_setdebug(fEventId, ET_DEBUG_INFO);

   // Configure the ET Station.
   et_station_config_init(&sconfig);
   int flowMode=ET_STATION_SERIAL;
   et_station_config_setflow(sconfig, flowMode);
   if (no_block) {
      et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
      if (queue_size > 0) {
         et_station_config_setcue(sconfig, queue_size);
      }
   }

   int status;
   if ((status = et_station_create(fEventId, &my_stat, station_name.c_str(), sconfig)) != ET_OK) {
      if (status == ET_ERROR_EXISTS) {
         /* my_stat contains pointer to existing station */
         std::cout << "EvioTool:OpenET() -- Station '"<< station_name  << " already exists\n";
         fReadFromEt = false;
         return(ET_ERROR);
      }
      else if (status == ET_ERROR_TOOMANY) {
         std::cout << "EvioTool:OpenET() -- Too many stations created.\n";
         fReadFromEt = false;
         return(ET_ERROR);
      }
      else {
         std::cout << "EvioTool:OpenET() -- Error creating station.\n";
         fReadFromEt = false;
         return(ET_ERROR);
      }
   }
   et_station_config_destroy(sconfig);

   if (et_station_attach(fEventId, my_stat, &fEtAttach) != ET_OK) {
      std::cout << "EvioTool:OpenET() -- Error attaching station. "<< station_name <<"\n";
      fReadFromEt = false;
      return(ET_ERROR);
   }
   /* allocate some memory */
   fPEventBuffer = (et_event **) calloc(fEtReadChunkSize, sizeof(et_event *));
   if (fPEventBuffer == NULL) {
      std::cout << "EvioTool:OpenET() -- Unable to allocate event buffer header.\n";
      fReadFromEt = false;
      return(ET_ERROR);
   }

   return(ET_OK);
}

void EvioTool::Close(){
   if (fReadFromEt){
      et_station_detach(fEventId, fEtAttach);
      et_close(fEventId);
      delete fPEventBuffer;
      fReadFromEt = false;
   }
   if(fIsOpen) {
      evClose(fEvioHandle);
      fIsOpen = false;
   }
}

void EvioTool::parseDictionary(const char *dictf){
  // Open the file pointed to by dictf and parse the XML as a dictionary for the file.
  
  //  ifstream f(dictf);
  //  Dictionary = new evioDictionary(f);
}

int EvioTool::NextNoParse(){
  // Read an event from the EVIO file, but don't parse it.
   int stat = ET_OK;
   if(fReadFromEt){
      if(fEventValid) EndEvent();   // User should have called this, but didn't, so put the last event back on ET.
      // This is a *blocking* read. If no events are available, this will hang.

#ifdef __APPLE__
// For now we only do single event reads. If it is needed, it is possible to do multi event reads
// on Linux. Multi event reads still need to be tested and likely have some added infrastructure.
      stat = et_event_get(fEventId, fEtAttach, fPEventBuffer, ET_SLEEP, NULL);
      fNumRead=1;
      fCurrentChunk = 0;
#else
      if(fCurrentChunk <= 0){   // Need to read more events.
         stat = et_events_get(fEventId, fEtAttach, fPEventBuffer, ET_SLEEP, NULL, fEtReadChunkSize, &fNumRead);
         fCurrentChunk = fNumRead -1;
      }else{
         fCurrentChunk--;
      }
#endif
      switch(stat){
         case ET_OK:
            break;
         case ET_ERROR_DEAD:
            cout << "EvioTool:Next() - ET system is dead.\n";
            return(stat);
         case ET_ERROR_TIMEOUT:
            cout << "EvioTool:Next() - ET system timeout.\n";
            return(stat);
         case ET_ERROR_EMPTY:
            cout << "EvioTool:Next() - ET system got no events.\n";
            return(stat);
         case ET_ERROR_BUSY:
            cout << "EvioTool:Next() - ET system station is busy\n";
            return(stat);
         case ET_ERROR_READ:
         case ET_ERROR_WRITE:
            cout << "EvioTool:Next() - ET system socket communication error.\n";
            return(stat);
         default:
            cout << "EvioTool:Next() - ERROR number:" << stat << " Lookup in ET system manual. \n";
            return(stat);
      }

      size_t len4;
      et_event_getdata(fPEventBuffer[fCurrentChunk], (void **) &fEvioBuf);
      et_event_getlength(fPEventBuffer[fCurrentChunk], &len4);
      size_t len = len4/4;
      if( len > 10) {
         if (fEvioBuf[7] != 0xc0da0100) {
            cout << "EvioTool::Next() - Warning - EVIO ET Buffer has wrong magic number.\n";
            return (ET_ERROR_READ);
         }
         fEvioBuf += 8;  // The data starts after the 8 word header.
      }else{
         return (EvioTool_Status_ERROR);
      }
   }else {
      stat = evReadNoCopy(fEvioHandle, &fEvioBuf, &fEvioBufLen);
      if (stat == EOF) return (EvioTool_Status_EOF);
      if (stat != S_SUCCESS) {
         cerr << "EvioTool::Next() -- ERROR -- problem reading file. -- evio status = " << stat << "\n";
         return (EvioTool_Status_ERROR);
      }
   }
  fEventValid = true;
  return stat;
}

int EvioTool::Next(){
  // Read an event from the EVIO file and parse it.
  // Returns 0 on success, or an error code if not.

  int stat = NextNoParse();
  if(stat != S_SUCCESS) return stat;

  if (fFullErase) Clear("Full"); // Clear out old data.
  else Clear();

  stat = ParseEvioBuff(fEvioBuf);  // Recursively parse the banks in the buffer.
  // We can call EndEvent here, because the ParseEvioBuff will *copy* the data (just once) into the
  // allocated structure of the EvioTool. So in EndEvent, we no longer rely on the fEvioBuf to contain the data.
  EndEvent();  // We are done with the event, so it can be put back on ET.
  return stat;

}

int EvioTool::EndEvent() {
   // End of the Event. This is not important for reading from a file, but is very important for reading from ET.
   // At the end of the event, it needs to be put back on the ET ring for the next consumer. If not, the ET ring will
   // block.
   // For safety, when reading from ET, the EndEvent() will be called at the top of NextNoParse() if there is an event active.
   if(fEventValid && fReadFromEt){
#ifdef __APPLE__
      int status = et_event_put(fEventId, fEtAttach, fPEventBuffer[0]);
#else
      int status = et_events_put(fEventId, fEtAttach, fPEventBuffer, fNumRead);
#endif
      if (status == ET_ERROR_DEAD) {
         cout << "EvioTool::EndEvent() -- ERROR -- ET system seems dead.\n";
         return status;
      }
      else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
         cout << "EvioTool::EndEvent() -- ERROR -- ET system socket error. \n";
         return status;
      }
      else if (status != ET_OK) {
         cout << "EvioTool::EndEvent() -- ERROR -- ET system error: " << status << ". \n";
         return status;
      }
   }
   fEventValid = false;
   fCurrentChunk--;   // Done with this event.
   return ET_OK;
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
      if(fDebug & EvioTool_Debug_L2) cout << "Adding a new Leaf node to node: " << node->GetNum() << " with name: " << str << endl;
      node->AddLeaf<FADCdata>(str,tag,num,"Auto added string leaf");
      loc= node->leafs->GetEntriesFast()-1;
    }else{
      return 0;
    }
  }
  
  if(fDebug & EvioTool_Debug_L2) cout << "Adding data to Leaf at idx = " << loc << " with specified AddOrFillLeaf<FADCdata> \n";

#ifdef DEBUG
  unsigned short formatTag  = (buf[0]>>20)&0xfff;
#endif
  unsigned short formatLen  = buf[0]&0xffff;

#ifdef DEBUG
  string formatString       = string((const char *) &(((uint32_t*)buf)[1]));
  if(fDebug&EvioTool_Debug_L2) cout << "FADC formatTag " << formatTag << " len: " << formatLen << " String: " << formatString << "\n";
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
    unsigned char slot       = GET_CHAR(cbuf,indx);  // "c"
    unsigned int  trig       = GET_INT(cbuf,indx);   // "i"
    unsigned long long time  = GET_L64(cbuf,indx);   // "l"
    // The following is a cheat. For CLAS12 data, FADCs have either formatTag = 13, 8 or 12.
    // Much faster to check an integer than to compare format strings and parse the stuff!!!
    // TODO: This can be made more robust by additional sanity checks.
    //
    if(tag == 57601) { // formatTag = 13, with format string (c,i,l,N(c,Ns)) or (c,i,l,n(s,mc))
        int nchan = GET_INT(cbuf, indx);
        for (int jj = 0; jj < nchan; ++jj) {
            ll->data.emplace_back(crate, slot, trig, time, indx, cbuf);
            //      ll->data.emplace_back(indx, cbuf);
        }
    }else if(tag == 57650){  // formatTag = 8,  (for bank tag 57650), with format string (c,i,l,Ni)
        int nvals = GET_INT(cbuf, indx);
        for (int jj=0; jj< nvals; ++jj){
            unsigned int value = GET_INT(cbuf, indx);
            ll->data.emplace_back(crate, slot, trig, time, value);
        }
    }else if(tag == 57622){ // formatTag = 12, (for bank tag 57622), with format string (c,i,l,N(c,s))
        int nchan = GET_INT(cbuf, indx);
        for (int jj = 0; jj < nchan; ++jj) {
            unsigned char chan = GET_CHAR(cbuf, indx);
            unsigned short val = GET_SHORT(cbuf, indx);
            ll->data.emplace_back(crate, slot, trig, time, chan, val);
            //      ll->data.emplace_back(indx, cbuf);
        }
    }else{
        if(fDebug & EvioTool_Debug_L1) std::cout << "Not processing tag = " << tag << std::endl;
    }
  }
  
  ll->CallBack();
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
  this_tag    = (buf[1]>>16);
  this_num    = buf[1]&0xff;
  
  unsigned short contentType = (buf[1]>>8)&0x3f;
  
  if( fChop_level>0){ // This means the EvioTool is the top level. Check if OK.
    if(contentType != 0x10 && contentType != 0x0e){ // So the events better be of Container type and not Leaf type.
      printf("ERROR Parsing EVIO file. Top level is not 0x10 or 0x0e container type but: 0x%02x \n",contentType);
      printf("Aborting event \n");
      return(-3);
    }
    // Check if the event has the desired tag number. Skip if not.
    if(!CheckTag(this_tag)){ // Event tag not found in tags list, so skip it.
      if(fDebug & EvioTool_Debug_Info2) cout << "Event of tag = " << this_tag << " skipped \n";
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
  
  if((fDebug & EvioTool_Debug_Info2) && contentType < 0x10){
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
      
      if(fDebug & EvioTool_Debug_Info2){
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
      if(fDebug & EvioTool_Debug_Info2){
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
    if(fDebug & EvioTool_Debug_L2) cout << "EvioTool::ContainNodeHandler -- pruning the tree depth=" << depth << endl;
    node->this_tag = tag; // TODO: VERIFY that this is correct and needed here.
    node->this_num = num;
    return node;
  }
  
  int loc = node->FindBank(tag,num);
  if( loc == -1){
    if(fAutoAdd){
      char str[100];
      sprintf(str,"Bank-%d-%d",tag,num);
      if(fDebug & EvioTool_Debug_L2) cout << "Adding a new Bank to " << node->GetName() << " with name: " << str << endl;
      loc=node->banks->GetEntriesFast();
      node->AddBank(str,tag,num,"Auto added Bank");
    }else{
      if(fDebug & EvioTool_Debug_L2) cout << "Not adding a new bank for tag= " << tag << " num= " << num << endl;
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
