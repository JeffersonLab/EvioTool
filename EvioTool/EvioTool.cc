/*
 *  EvioTool.cp
 *  EvioTool
 *
 *  Created by Maurik Holtrop on 4/19/14.
 *  Copyright (c) 2014 UNH. All rights reserved.
 *
 // Timing on version using vector<> and push_back:
                                              9.6 kHz, or  103 micro seconds/event. (-O3)
 // With all push_back calls commented out:   117 kHz, or  8.5 micro seconds/event
 //
 // Not storing the SVT.push_back(sfpga): 11.4328 kHz, or 87.4678 micro seconds
 // Also Replacing the SVT_chan_t.vector<int> samples  with SVT_chan_t.samples[6]: 57.8463 kHz 17.2872 micro seconds
 //
 // Storing with SVT.push_back(sfpga), but replacing the SVT_chan_t.vector<int> samples  with SVT_chan_t.samples[6]:
 //                                        51.974 kHz, or 19.2404 micro seconds
 // Storing SVT[] using memset to clear and memcpy to copy:  === This is erroneous, does not store the date in correct place.
 //                                        60.0224 kHz, or 16.6604 micro seconds
 //
 // Storing SVT[7].data[i-7].fpga etc, with data a vector that is pre-allocated to MAX_SVT_DATA:
 //         build/Release/EvioTool_Test: 88.73 kHz,  99.74 kHz Avg. Event: 739998
 //
 // Replacing SVT[7].data with SVT_data vector, pre-allocated to MAX_SVT_DATA, using a single push_back and then SVT_data[SVT_data.size()-1].fpga for filling
 //         build/Release/EvioTool_Test: 102.6 kHz,  102.2 kHz Avg. Event: 659998
 //
 // The same, but using a safer "late pushback"
 //         build/Release/EvioTool_Test: 99.55 kHz,  100.5 kHz Avg. Event: 649998
 //
 // This last version seems to be the best choice. The penalty one additional SVT_chan_t object copy onto the vector each push_back seems small enough, and the code is a lot safer.
 */
#include "EvioTool.h"
#include "EvioToolPriv.h"

EvioTool::EvioTool(const char *filename, const char *dictf){
//
  init();
  if(filename){
    open(filename,dictf);
  };

};

EvioTool::~EvioTool(){
  // Destructor and cleanup.
  if(et_evt_ptr) delete et_evt_ptr;
  
}
void EvioTool::init(void){
  // Initialize the class
  MaxBuff = 1000000;
  EvioChan = nullptr;
  Dictionary= nullptr;
  fDebug=0;
  run_number=0;
  start_time=0;
  file_number=0;
  et_que_size=100;
  et_receive_buffer_size=16*1024;
  et_send_buffer_size=16*1024;
  get_events_from_et=false;
  et_events_need_put=false;
  et_num_read=0;
  et_station_name="RootConsumer";
  et_host_name = "localhost";
  et_file_name="/tmp/ETBuffer";
  et_port = 11111;
  et_is_blocking=false;
  et_mode = ET_HOST_AS_REMOTE;
  et_events_remaining=0;
  et_events_chunk=1;
  et_evt_ptr = NULL;
}

int  EvioTool::getBufSize(void){
  if(get_events_from_et){
    return(et_data_len);
  }else{
    return( EvioChan->getBufSize());
  }
}

const unsigned int *EvioTool::getBuffer(void){
  if(get_events_from_et){
    return( et_data );
  }else{
    return( EvioChan->getNoCopyBuffer());
  }
};

int EvioTool::openEv(char *filename){
  int handle;
  string mode="r";
  evOpen( filename,const_cast<char *>(mode.c_str()),&handle);
  return(handle);
}


#ifdef EVIO_V4_0
int EvioTool::ReadNoCopy(int handle,const unsigned int **buffer,int *buflen){
  return(evReadNoCopy(handle,buffer,buflen));
}
#else
int EvioTool::ReadNoCopy(int handle,const unsigned int **buffer,unsigned int *buflen){
  return(evReadNoCopy(handle,buffer,buflen));
}
#endif


void EvioTool::open(const char *filename,const char *dictf){
  // Open an EVIO file. If dictf is given, open the XML dictionary file for the dictionary.
  // If dictf is not spedified (or is nullptr), then an attempt is made to get the dictionary from the EVIO file.

  if(EvioChan) delete EvioChan;
  if(dictf){ // User supplied a dictionary.
    parseDictionary(dictf);
  }
  try {
    EvioChan = new evioFileChannel(filename,Dictionary, "r", MaxBuff);
    EvioChan->open();
    get_events_from_et=false;
  } catch (evioException e) {
    cerr << "Error opening file: " <<  filename << endl;
    cerr << e.toString() << endl;
    cerr << "Abort.\n";
    return;
  }

};

int EvioTool::openEt(const char *name,int port,int prescale){
  // Open a station to the ET system with the standard station name "RootConsumer".
  // Standard setting is to open the ET file "/tmp/ETBuffer" and port "11111" on localhost,
  // and have no pre-scaling.
  if(strlen(name)>0)et_file_name=name;
  if(port) et_port=port;
  if(prescale) et_pre_scale = prescale;
  
  et_openconfig   openconfig;
  et_open_config_init(&openconfig);
  et_open_config_sethost(openconfig, et_host_name.c_str());
  et_open_config_setserverport(openconfig, et_port);
  et_open_config_setmode(openconfig,et_mode);
  /* Defaults are to use operating system default buffer sizes and turn off TCP_NODELAY */
  et_open_config_settcp(openconfig, et_receive_buffer_size, et_send_buffer_size, et_nodelay);
  if (et_open(&et_id, et_file_name.c_str(), openconfig) != ET_OK) {
    printf("EvioTool:: openEt had problems with et_open\n");
    return(1);
  }
  et_open_config_destroy(openconfig);
  get_events_from_et = true;
  
  if(fDebug>1){
    et_system_setdebug(et_id, ET_DEBUG_INFO);
  }

  /* define station to create */
  et_statconfig   sconfig;
  int             flowMode=ET_STATION_SERIAL;
  
  et_station_config_init(&sconfig);
  et_station_config_setflow(sconfig, flowMode);
  
  if (!et_is_blocking) {
    et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
    et_station_config_setcue(sconfig, et_que_size);
  }
  
  int status=0;
  if ((status = et_station_create(et_id, &et_stat, et_station_name.c_str(), sconfig)) != ET_OK) {
    et_station_config_destroy(sconfig);
    if (status == ET_ERROR_EXISTS) {
      /* my_stat contains pointer to existing station */
      printf("EvioTool::openEt() et_station_create - station %s already exists\n", et_station_name.c_str());
    }
    else if (status == ET_ERROR_TOOMANY) {
      printf("EvioTool::openEt() -- too many stations created\n");
      get_events_from_et=false;
      return(1);
    }
    else {
      printf("EvioTool::openEt() error in station creation\n");
      get_events_from_et=false;
      return(1);
    }
  }
  et_station_config_destroy(sconfig);
  
  if (et_station_attach(et_id, et_stat, &et_attach) != ET_OK) {
    printf("EvioTool::openEt() error in station attach\n");
    return(1);
  }
  
  // Allocate memory for the event buffer.
  
  et_evt_ptr = (et_event **) calloc(et_events_chunk, sizeof(et_event *));
  if(et_evt_ptr == NULL){
    cout << "EvioTool::openEt() -- error allocating memory.\n";
    return(1);
  }
  
  return(0);
};

void EvioTool::parseDictionary(const char *dictf){
  // Open the file pointed to by dictf and parse the XML as a dictionary for the file.
  
  ifstream f(dictf);
  Dictionary = new evioDictionary(f);
}

void EvioTool::printDictionary(){
#ifdef EVIO_V4_0
#else
  if( Dictionary ){
    cout << Dictionary->toString() << endl;
  }else{
    cout << "No dictionary present.\n";
  }
#endif
}

bool EvioTool::read(){
// Read the next event from the file or ET system
  
  if(get_events_from_et){
    if(et_events_remaining ==0){
      if(et_num_read){
        // We need to put or dump the events we got before.
        // If we get here then we dump.
        int status = et_events_dump(et_id,et_attach, et_evt_ptr, et_num_read);
        if (status == ET_ERROR_DEAD) {
          printf("EvioTool::read() -- at dump, ET is dead\n");
          return false;
        }
        else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
          printf("EvioTool::read() -- at dump, socket communication error\n");
          return false;
        }
        else if (status != ET_OK) {
          printf("EvioTool::ead() - at dump, put error\n");
          return false;
        }

      }

#ifdef __APPLE__
      int status = et_event_get(et_id, et_attach, et_evt_ptr, ET_SLEEP, NULL);
      et_num_read=1;
#else
      int status = et_events_get(et_id, et_attach, et_evt_ptr, ET_SLEEP, NULL, et_events_chunk, &et_num_read);
#endif
      if (status == ET_OK) {
        ;
      }
      else if (status == ET_ERROR_DEAD) {
        printf("EvioTool::read() --  ET system is dead\n");
        return false;
      }
      else if (status == ET_ERROR_TIMEOUT) {
        printf("EvioTool::read() -- got timeout\n");
        return false;
      }
      else if (status == ET_ERROR_EMPTY) {
        printf("EvioTool:read() --  no events\n");
        return false;
      }
      else if (status == ET_ERROR_BUSY) {
        printf("EvioTool::read() -- station is busy\n");
        return false;
      }
      else if (status == ET_ERROR_WAKEUP) {
        printf("EvioTool::read() -- someone told me to wake up??\n");
        return false;
      }
      else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
        printf("EvioTool::read() -- socket communication error\n");
        return false;
      }
      else if (status != ET_OK) {
        printf("EvioTool::read() -- Undetermined error %d\n",status);
        return false;
      }
      et_events_remaining = et_num_read;
    }
    
    et_event_getdata(et_evt_ptr[et_events_remaining -1], (void **) &et_data);
    et_event_getlength(et_evt_ptr[et_events_remaining -1], &et_data_len);
    int *tmp_pdata=(int *)et_data;
    printf("pdata (%5zu) :",et_data_len);
    for(int ii=0;ii<5;ii++){
      printf("%12u,",tmp_pdata[ii]);
    }
    printf("\n");

//    et_event_getendian(et_evt_ptr[et_events_remaining -1], &endian);
    int swap;
    et_event_needtoswap(et_evt_ptr[et_events_remaining -1], &swap);
    if(swap){
      cout << "EvioTool::read() -- It appears as if this data needs swapping.\n";
    }
    if(fDebug>1){
      printf("ET data: pointer %10lx  length= %3zu \n",(unsigned long)et_data,et_data_len);
    }
    et_events_remaining--;
    
    return true;
  }else{
    bool success;
    try {
      success = EvioChan->readNoCopy();
    } catch (evioException e) {
      cerr << "Read Error \n";
      cerr << e.toString() << endl;
      cerr << "Abort.\n";
      return false;
    }
    return success;
  }
};

int EvioTool::parse(EVIO_Event_t *evt, const unsigned int *buff){
// If buff=Null, parse the contends of the internal evio buffer (from "read()")
// If buff is a buffer, parse that buffer.
  
  try{
    if( buff == nullptr){
      if(get_events_from_et){
        int *tmp_pdata=(int *)et_data;
        printf("pdata (%5zu) :",et_data_len);
        for(int ii=0;ii<5;ii++){
          printf("%12u,",tmp_pdata[ii]);
        }
        printf("\n");

        ETree = new evioDOMTree(et_data);
      }else{
        ETree = new evioDOMTree(EvioChan);
      }
    }else{
      ETree = new evioDOMTree(buff);
    }
  } catch (evioException e) {
    cerr << "Parse Error \n";
    cerr << e.toString() << endl;
    cerr << "Abort.\n";
    return 0;
  }

  EvioEventClear(evt);  // Clear all the information from any previous events.
  
  evioDOMNodeP topnode = ETree->root;
  evioDOMNodeListP c = topnode->getChildren();
  evioDOMNodeList::iterator iter;

  evt->topnode_tag = topnode->tag;
  
  if(topnode->isLeaf()){
    if(topnode->tag == EVIO_PRESTART){         ////////////////////  Prestart Event. ////////////////
      vector<unsigned int> *cc = topnode->getVector<unsigned int>();
      evt->start_time = start_time = (*cc)[0];
      evt->run_number = run_number  = (*cc)[1];
      evt->file_number= file_number= (*cc)[2];
    }else if(topnode->tag == EVIO_GO){ /////////////////////   Go Event ////////////////////
//      vector<unsigned int> *cc = topnode->getVector<unsigned int>();
    }
  }
  
  if(topnode->isContainer()){   ///////////////////////// Data Events ////////////////////
    for(iter=c->begin(); iter!=c->end(); iter++) {
      
      if((*iter)->tag == EVIO_EVENT_HEADER){  ////////////////// Event Header ////////////////////
        vector<unsigned int> *cc = (*iter)->getVector<unsigned int>();
        if(fDebug>1) cout << "Event " << (*cc)[0] << endl;
        if(fDebug>3){
          cout << "      size=" << cc->size() << "[";
          for(unsigned int i=0;i<cc->size();i++){
            cout << (int) (*cc)[i];
            if(i< cc->size()-1) cout << ",";
          }
          cout << "]\n";
        }
        evt->event_number = (*cc)[0];
        evt->event_type   = (*cc)[1];
        evt->file_number  = (*cc)[2];
      }

    // --------- DECODING HELPERS ----------------
    // These help walk through a complicated EVIO buffer, for which the EVIO decoding is not setup.
    
#define GET_CHAR(b,i) b[i]; i+=1;
#define GET_SHORT(b,i) ((short *)(&b[i]))[0];i+=2;
#define GET_INT(b,i)  ((int *)(&b[i]))[0];i+=4;
#define GET_L64(b,i) ((unsigned long long *)(&b[i]))[0];i+=8;

      else if((*iter)->tag == EVIO_ECAL_FADC_CRATE_1 || (*iter)->tag == EVIO_ECAL_FADC_CRATE_2){
        if((*iter)->isContainer()) {
          const evioDOMContainerNode *container = static_cast<const evioDOMContainerNode*>(*iter);
          evioDOMNodeList::const_iterator leaf;
          
          for(leaf=container->childList.begin(); leaf!=container->childList.end(); leaf++)
          {
            if(fDebug>4)cout << "     Crate:" <<(*iter)->tag <<" tag=" << (*leaf)->tag << "  num=" << (int)(*leaf)->num << "  type="
              << (*leaf)->getContentType() << "  size: " << (*leaf)->getSize()
              << endl;
            
            if((*leaf)->tag == 57603 || (*leaf)->tag == 57601){  // Normal Integral mode (57603) or RAW (57601) mode ECAL data
              evioCompositeDOMLeafNode *ecal=(evioCompositeDOMLeafNode *)(*leaf);
              
              unsigned int buflen = ecal->data.size()*4 - 4;
              unsigned char *buf=(unsigned char *) ecal->data.data();
              unsigned int indx= 0; // sizeof(ECAL_Header);
              
              while(indx < buflen)
              {
                // Format = c,i,l,N(c,N(s,i))   slot,trig,time,N *(chan, N*( samples))

                char slot = GET_CHAR(buf,indx);
                unsigned int  trig = GET_INT(buf,indx);
                unsigned long long time = GET_L64(buf,indx);
                int nchan = GET_INT(buf,indx);
              
                if(ecal->formatTag == 13){
                  
                  FADC_data_f13_t e13_data;
                  e13_data.crate = (*iter)->tag;
                  e13_data.slot  = slot;
                  e13_data.trig  = trig;
                  e13_data.time  = time;
                  for(int jj=0; jj<nchan; jj++){
                    FADC_chan_f13_t ch;
                    ch.chan = GET_CHAR(buf,indx);
                    int nsample = GET_INT(buf,indx);
                  
                    for(int kk=0; kk<nsample; kk++){
                      short sample = GET_SHORT(buf,indx);
                      ch.samples.push_back(sample);
                    }
                     e13_data.data.push_back(ch);
                  }
                  evt->FADC_13.push_back(e13_data);
                  
                }else if(ecal->formatTag == 17){
                  
                  FADC_data_f15_t e15_data;
                  e15_data.crate = (*iter)->tag;
                  e15_data.slot  = slot;
                  e15_data.trig  = trig;
                  e15_data.time  = time;
                  for(int jj=0; jj<nchan; jj++){
                    FADC_chan_f15_t ch;
                    ch.chan = GET_CHAR(buf,indx);
                    
                    int nsample = GET_INT(buf,indx);
                    
                    for(int kk=0; kk<nsample; kk++){
                      short pulse_time = GET_SHORT(buf,indx);
                      pulse_time = pulse_time>>6;               // Shift 6 bits, or divide by 64.
                      ch.time.push_back(pulse_time);
                      int pulse_integral = GET_INT(buf,indx);
                      ch.adc.push_back(pulse_integral);
                    }
                    e15_data.data.push_back(ch);
                  }
                  
                  evt->FADC_15.push_back(e15_data);
                  
                  }
                }
            }
            else if((*leaf)->tag == 57606)
            {
              // Trigger information bank.
              
              vector<unsigned int> *trig_data = (*leaf)->getVector<unsigned int>();
              if(fDebug>2){
                cout << "Trig data len =" << (*trig_data).size() << "  [";
                for(unsigned int i=0;i<(*trig_data).size();i++){
                  printf("0x%04x,",(*trig_data)[i]);
                }
                cout << "]\n";
              }
//              int trig_event_number = (*trig_data)[0] & 0x0ffffff;
//              int trig_unk1 = ((*trig_data)[0]>> 24) & 0x0ffffff;
//              int trig_unk2 = (*trig_data)[1];
//              int trig_unk3 = (*trig_data)[2];
              evt->or_bits = (*trig_data)[3];
              evt->top_bits = (*trig_data)[4];
              evt->bottom_bits = (*trig_data)[5];
              evt->pair_bits = (*trig_data)[6];
              evt->trig_time      = (*trig_data)[7];
              
            }
            else{
               cout << "Unknown ECAL bank with tag = " << (*leaf)->tag << endl;
            }
          }
          
        }
        else{
          cout << "\n\n Expected a container for tag==1,2 ECAL, but got leaf.\n\n";
        }
        
      }
      else if((*iter)->tag == 3)
      {                             /////////////////////  SVT Crate 3  ///////////////////////////

        int n_sfpga=0;
        
        if((*iter)->isContainer()){
          const evioDOMContainerNode *container = static_cast<const evioDOMContainerNode*>(*iter);
          evioDOMNodeList::const_iterator s;
          for(s=container->childList.begin(); s!=container->childList.end(); s++){
            if(fDebug>4)cout << "     SVT: tag=" << (*s)->tag << "  num=" << (int)(*s)->num << "  type="
                              << (*s)->getContentType() << "  size: " << (*s)->getSize()
                              << endl;
            
            n_sfpga = (int)(*s)->num;  // FPGA number.
            if(n_sfpga > MAX_NUM_SVT_FPGA ){
              cout << " ERROR -- FPGA number " << n_sfpga << " is too large for event: " << evt->event_number << endl;
              delete ETree;
              return(0);
            }

            vector<unsigned int> *svt_data = (*s)->getVector<unsigned int>();
            if(fDebug>4)cout << "SVT["<< n_sfpga <<"] data len =" << (*svt_data).size()<< endl;
            
            /////////////// Store Temperatures /////////////////////
            //SVT_FPGA_t sfpga;
            evt->SVT[n_sfpga].fpga = (*s)->tag;
            //sfpga.fpga = (*s)->tag;
            
            //sfpga.trigger = (*svt_data)[0];
            evt->SVT[n_sfpga].trigger = (*svt_data)[0];
            for(int i=1;i<NUM_FPGA_TEMPS;i++){
              //int temp=(*svt_data)[i];
              //sfpga.temps.push_back(temp);
              evt->SVT[n_sfpga].temps[i] =(*svt_data)[i];
            }
            
            /////////////// Data ///////////////////////////////////
#define Vector_push_late
            
#ifdef Vector_push_late
            SVT_chan_t cn;
            evt->SVT_data.reserve(MAX_SVT_DATA);

#else
//            evt->SVT[n_sfpga].data.reserve(MAX_SVT_DATA);
            evt->SVT_data.reserve(MAX_SVT_DATA);
            SVT_chan_t cn;
#endif
            unsigned int data_end;
            if( (*svt_data).size()-1 > MAX_SVT_DATA +7 ){
              data_end = MAX_SVT_DATA+7-5;
              cout << "WARNING: Truncating the SVT Data on event :" << evt->event_number << " Size: :" << (*svt_data).size() -1 << endl;
            }else{
              data_end = (*svt_data).size()-1;
            }
            
            if(fDebug>4) cout <<"SVT["<< n_sfpga <<"] data_end = " << data_end << endl;
            for(unsigned int i=7;i< data_end ;){
              
              unsigned int decode = (*svt_data)[i++];                                     // Word 1
              if( (*s)->tag != (decode&0xffff) ){
                cout << "SVT Data decoding error ! i="<<i<<" (*s)->tag = "<< (*s)->tag << " decode fpga=" << (decode&0xffff) << " \n";
              }
#ifdef Vector_push_late
              cn.fpga = n_sfpga;
              cn.chan = (decode>>16)&0x7f;
              cn.apv  = (decode>>24)&0x07;
              cn.hybrid=(decode>>28)&0x03;

#else
//              evt->SVT[n_sfpga].data.push_back(cn);
//              evt->SVT[n_sfpga].data[i-7].chan = (decode>>16)&0x7f;
//              evt->SVT[n_sfpga].data[i-7].apv  = (decode>>24)&0x07;
//              evt->SVT[n_sfpga].data[i-7].hybrid=(decode>>28)&0x03;
              evt->SVT_data.push_back(cn);
              evt->SVT_data[evt->SVT_data.size()-1].fpga = n_sfpga;
              evt->SVT_data[evt->SVT_data.size()-1].chan =(decode>>16)&0x7f;
              evt->SVT_data[evt->SVT_data.size()-1].apv  =(decode>>24)&0x07;
              evt->SVT_data[evt->SVT_data.size()-1].hybrid=(decode>>28)&0x03;
#endif
              int sample=0;
              decode = (*svt_data)[i++];                                                  // Word 2
              sample = decode&0x3fff;
#ifdef Vector_push_late
              cn.samples[0] = sample;
#else
//              evt->SVT[n_sfpga].data[i-7].samples[0] = sample;
              evt->SVT_data[evt->SVT_data.size()-1].samples[0] = sample;
#endif
              sample = (decode>>16)&0x3fff;
#ifdef Vector_push_late
              cn.samples[1] = sample;
#else
//              evt->SVT[n_sfpga].data[i-7].samples[1] = sample;
              evt->SVT_data[evt->SVT_data.size()-1].samples[1] = sample;
#endif
              
              decode = (*svt_data)[i++];                                                  // Word 3
              sample = decode&0x3fff;
#ifdef Vector_push_late
              cn.samples[2] = sample;
#else
//              evt->SVT[n_sfpga].data[i-7].samples[2] = sample;
              evt->SVT_data[evt->SVT_data.size()-1].samples[2] = sample;
#endif
              sample = (decode>>16)&0x3fff;
#ifdef Vector_push_late
              cn.samples[3] = sample;
#else
//              evt->SVT[n_sfpga].data[i-7].samples[3] = sample;
              evt->SVT_data[evt->SVT_data.size()-1].samples[3] = sample;
#endif

              decode = (*svt_data)[i++];                                                  // Word 4
              sample = decode&0x3fff;
#ifdef Vector_push_late
              cn.samples[4] = sample;
#else
//              evt->SVT[n_sfpga].data[i-7].samples[4] = sample;
              evt->SVT_data[evt->SVT_data.size()-1].samples[4] = sample;
#endif
              sample = (decode>>16)&0x3fff;
#ifdef Vector_push_late
              cn.samples[5] = sample;
#else
//              evt->SVT[n_sfpga].data[i-7].samples[5] = sample;
              evt->SVT_data[evt->SVT_data.size()-1].samples[5] = sample;
#endif

#ifdef Vector_push_late
//              evt->SVT[n_sfpga].data.push_back(cn);
              evt->SVT_data.push_back(cn);
#endif

            }
            
//            evt->SVT[n_sfpga++]=sfpga;
// Better not to copy at ALL!!!
// memcpy does not work! We don't know proper size of sfpga!!!
//            memcpy((void *)&evt->SVT[n_sfpga++],(void *)&sfpga,sizeof(sfpga));

            if(fDebug && ((*svt_data)[(*svt_data).size()-1])!= 0){
              printf(" Event= %7d  ==== Data Decoding error on SVT. Last word %3d !=0 \n",evt->event_number,(*svt_data)[(*svt_data).size()-1]);
            }            
          }
        }
      }
      else{
        cout << "\n\n Unexpected primary container (crate): " << (*iter)->tag;
      }
    }
  }
  delete ETree;
  return 1;
}
