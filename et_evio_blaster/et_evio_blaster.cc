//
//  main.cpp
//  et_evio_blaster
//
//  Created by Maurik Holtrop on 5/5/14.
//
// Based on the example code: et_producer1
//
//
// Description:
// Read data from an EVIO file and put it onto the ET ring.
//
// Issues:
//
// The format of EVIO data on the ET ring changed to accomodate the JAVA
// based ET clients, which need the EVIO header information. The appropriate call
// for this should be evOpenBuffer and evWrite, however this slows things down a lot.
//
//
// Email from Carl Timmer:
//Yes, if you use evOpenBuffer() and evWrite(), all necessary headers will be written.
//Sorry, I didn't make that clear.
//Carl
//
//On 6/10/2014 9:59 AM, Serguei Boiarinov wrote:
//Carl,
//is that 8-word header suppose to be created by evOpenBuffer ?
//
//----- Original Message -----
//From: Carl Timmer <timmer@jlab.org>
//To: Maurik Holtrop <maurik@physics.unh.edu>, Serguei Boiarinov <boiarino@jlab.org>
//Sent: Tue, 10 Jun 2014 09:37:13 -0400 (EDT)
//Subject: Re: Evio on the ET ring?
//
//Maurik,
//
//In evio 4, the format we use to send evio data over the network is the
//same as used to write an evio file.
//That means one cannot simply get only the bytes for a single event, send
//that, and expect the java
//reader to understand it. It must be sent by code which places the event
//in the proper, evio-file
//format.
//
//In Java this is handled transparently, but C is more primitive. You can
//make the bytes of a single event
//into file format by the addition (at the beginning) of an 8 word, block
//header. The 32 bit ints you need
//to place before the event are:
//
//1)  length in words of everything including full block header.
//This will = 8 + length of event in words
//
//2) block number (1 since it's the first block)
//
//                 3) block header length (always 8)
//
//4) event count (1 in your case)
//
//5) reserved (0 is fine)
//
//6) bit info + version. For evio version 4 and specifying that this is
//the last block header, use
//0x204
//
//7) reserved (0 is fine)
//
//8) magic number used to handle endian issues (use 0xc0da0100)
//
//Place these in the buffer before your event and things should work.
//
//Carl
//

//-------------------------
// From Serguey, header bank macro:

#define EVIO_RECORD_HEADER(myptr, mylen) \
myptr[0] = mylen + 8; /* Block Size including 8 words header + mylen event size  */ \
myptr[1] = 1; /* block number */ \
myptr[2] = 8; /* Header Length = 8 (EV_HDSIZ) */ \
myptr[3] = 1; /* event count (1 in our case) */ \
myptr[4] = 0; /* Reserved */ \
myptr[5] = 0x204; /*evio version (10th bit indicates last block ?)*/ \
myptr[6] = 0; /* Reserved */ \
myptr[7] = 0xc0da0100; /* EV_MAGIC */

#define EVIO_RECORD_FOOTER(myptr, mylen) \
myptr[0] = mylen + 8; /* Block Size including 8 words header + mylen event size  */ \
myptr[1] = 2; /* block number */ \
myptr[2] = 8; /* Header Length = 8 (EV_HDSIZ) */ \
myptr[3] = 0; /* event count (1 in our case) */ \
myptr[4] = 0; /* Reserved */ \
myptr[5] = 0x204; /*evio version (10th bit indicates last block ?)*/ \
myptr[6] = 0; /* Reserved */ \
myptr[7] = 0xc0da0100; /* EV_MAGIC */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#ifdef sun
#include <thread.h>
#endif
#include "et.h"

#include "evio.h"

#include <iostream>
#include <string>
#include <vector>
using namespace std;


struct Arguments_t {
  vector<string> filenames;
  string et_name;
  string et_host_name;
  int    et_port;
  int    debug;
  bool   et_block;
  int    chunk;
  int    delay;
  int    et_event_size_max;
  bool   remote;
};

/* prototype */
static void * signal_thread (void *arg);

void Print_Usage(const char *name);
void Parse_Args(int *argc, char **argv, Arguments_t *p_arg);



int main(int argc,char **argv)
{
  int             status, numRead;
  int             sendBufSize=0, recvBufSize=0, noDelay=0;
  char            interface[16];
  int control[] = {0,0,0,0,0,0};   // Control int array for header.
  
  Arguments_t args;
  int evio_handle=0;
  
  et_att_id	    attach1;
  et_sys_id       et_system_id;
  et_openconfig   openconfig;
  et_event        **p_et_event;
  struct timespec timeout;
#if defined __APPLE__
  struct timeval  t1, t2;
#else
  struct timespec t1, t2;
#endif
  sigset_t        sigblock;
  pthread_t       tid;
  
  /* statistics variables */
  double          rate=0.0, avgRate=0.0;
  int64_t         count=0, totalCount=0, totalT=0, time, time1, time2;
  
  Parse_Args(&argc, argv, &args);
  
  /* delay is in milliseconds */
  if (args.delay > 0) {
    timeout.tv_sec  = (int)(args.delay/1000000);
    timeout.tv_nsec = (args.delay - (int)(args.delay/1000000)*1000000)*1000;
  }
  
  /* allocate some memory */
  p_et_event = (et_event **) calloc(args.chunk, sizeof(et_event *));
  if (p_et_event == NULL) {
    printf("%s: out of memory\n", argv[0]);
    exit(1);
  }
  
  /*************************/
  /* setup signal handling */
  /*************************/
  /* block all signals */
  sigfillset(&sigblock);
  status = pthread_sigmask(SIG_BLOCK, &sigblock, NULL);
  if (status != 0) {
    printf("%s: pthread_sigmask failure\n", argv[0]);
    exit(1);
  }
  
#ifdef sun
  /* prepare to run signal handling thread concurrently */
  thr_setconcurrency(thr_getconcurrency() + 1);
#endif
  
  /* spawn signal handling thread */
  pthread_create(&tid, NULL, signal_thread, (void *)NULL);
  
  /******************/
  /* open ET system */
  /******************/
  et_open_config_init(&openconfig);
  
  /* EXAMPLE: direct connection to ET */
  et_open_config_setcast(openconfig, ET_DIRECT);
  et_open_config_sethost(openconfig, args.et_host_name.c_str() );
  et_open_config_setserverport(openconfig, args.et_port);
  /* Defaults are to use operating system default buffer sizes and turn off TCP_NODELAY */
  et_open_config_settcp(openconfig, recvBufSize, sendBufSize, noDelay);
  if (strlen(interface) > 6) {
    et_open_config_setinterface(openconfig, interface);
  }
  
  /* EXAMPLE: multicasting to find ET */
  /*
   et_open_config_setcast(openconfig, ET_MULTICAST);
   et_open_config_addmulticast(openconfig, ET_MULTICAST_ADDR);
   et_open_config_setmultiport(openconfig, 11112);
   et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
   */
  
  /* EXAMPLE: broadcasting to find ET */
  /*et_open_config_setcast(openconfig, ET_BROADCAST);*/
  /*et_open_config_addbroadcast(openconfig, "129.57.29.255");*/
  /*et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);*/
  
  if (args.remote) {
    et_open_config_setmode(openconfig, ET_HOST_AS_REMOTE);
  }
  
  et_open_config_setwait(openconfig, ET_OPEN_WAIT);
  if (et_open(&et_system_id, args.et_name.c_str(), openconfig) != ET_OK) {
    printf("%s: et_open problems\n", argv[0]);
    exit(1);
  }
  et_open_config_destroy(openconfig);
  
  /* set level of debug output (everything) */
  if (args.debug > 1) {
    et_system_setdebug(et_system_id, ET_DEBUG_INFO);
  }
  
  /* attach to grandcentral station */
  if (et_station_attach(et_system_id, ET_GRANDCENTRAL, &attach1) < 0) {
    printf("%s: error in et_station_attach\n", argv[0]);
    exit(1);
  }
  
  /* read time for future statistics calculations */
#if defined __APPLE__
  gettimeofday(&t1, NULL);
  time1 = 1000L*t1.tv_sec + t1.tv_usec/1000L; /* milliseconds */
#else
  clock_gettime(CLOCK_REALTIME, &t1);
  time1 = 1000L*t1.tv_sec + t1.tv_nsec/1000000L; /* milliseconds */
#endif
  
  for(int i_file=0;i_file < args.filenames.size(); i_file++){
    
    if( evOpen((char *)args.filenames[i_file].c_str(),(char *)"r",&evio_handle) !=S_SUCCESS){
      printf("Error opening EVIO file: %s \n",args.filenames[0].c_str());
      exit(1);
    }else{
      if(args.debug) cout << "Opened file: " << args.filenames[i_file] << endl;
    }
    
    
    printf("Starting the main event loop\n");
    
    bool evio_has_events=true;
    
    while (evio_has_events) {
      
//#if defined __APPLE__
//      //    et_system_setgroup(id, group); // Setting the group does not work on a Mac.
//      status = et_event_new(id, attach1, p_et_event, ET_SLEEP, NULL, args.et_event_size_max*sizeof(uint32_t));
//      numRead=1;
//      //    status = et_events_new(id, attach1, p_et_event, ET_SLEEP, NULL, args.et_event_size_max, chunk, &numRead); // Neither does sending more than one event request.
//#else
      if(args.chunk>1){
        status = et_events_new(et_system_id, attach1, p_et_event, ET_SLEEP, NULL, args.et_event_size_max*sizeof(uint32_t), args.chunk, &numRead);
      }else{
        status = et_event_new(et_system_id, attach1, p_et_event, ET_SLEEP, NULL, args.et_event_size_max*sizeof(uint32_t));
        numRead=1;
      }
//#endif
      
      if (status == ET_OK) {
        ;
      }
      else if (status == ET_ERROR_DEAD) {
        printf("%s: ET system is dead\n", argv[0]);
        break;
      }
      else if (status == ET_ERROR_TIMEOUT) {
        printf("%s: got timeout\n", argv[0]);
        break;
      }
      else if (status == ET_ERROR_EMPTY) {
        printf("%s: no events\n", argv[0]);
        break;
      }
      else if (status == ET_ERROR_BUSY) {
        printf("%s: grandcentral is busy\n", argv[0]);
        break;
      }
      else if (status == ET_ERROR_WAKEUP) {
        printf("%s: someone told me to wake up\n", argv[0]);
        break;
      }
      else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
        printf("%s: socket communication error\n", argv[0]);
        goto error;
      }
      else if (status != ET_OK) {
        printf("%s: request error\n", argv[0]);
        goto error;
      }
      
      /* write data, set priority, set control values here */
      size_t et_data_buflen;
      unsigned int *pdata;
      uint32_t evio_buflen=0;
      const uint32_t *evio_ptr;
      
      for (int i=0; i < numRead; i++) {
        et_event_getdata(p_et_event[i], (void **) &pdata);
        et_event_getlength(p_et_event[i],&et_data_buflen);
        
        //      if( (status=evReadAlloc(evio_handle,&evio_ptr,&evio_buflen)) != S_SUCCESS){
        if( (status=evReadNoCopy(evio_handle,&evio_ptr,&evio_buflen)) != S_SUCCESS){
          if(status == EOF) printf("EVIO -- End of events reached at event %d\n",pdata[4]);
          else  printf("EVIO -- Other error: %d \n",status);
          evio_has_events=false;
          break;
        }
        
        if(evio_buflen + 16 > args.et_event_size_max){
          printf("Event too large: %d > %d \n",evio_buflen,args.et_event_size_max);
          break;
        }else{
          EVIO_RECORD_HEADER(pdata,(evio_buflen));
          memcpy(pdata+8,evio_ptr,evio_buflen*sizeof(uint32_t));
          //        unsigned int *tmp_ptr = (pdata + 8 + evio_buflen);
          //        EVIO_RECORD_FOOTER( tmp_ptr,0);
        }
        
        et_data_buflen = evio_buflen+8;
        
        et_event_setendian(p_et_event[i], ET_ENDIAN_LOCAL);
        et_event_setlength(p_et_event[i], et_data_buflen*sizeof(uint32_t));
        et_event_setcontrol(p_et_event[i], control, sizeof(control)/sizeof(int));
        
        if(args.debug > 1){
          printf("data (%5zul) :",et_data_buflen);
          for(int ii=0; ii<et_data_buflen+8 && ii<12;ii++){
            printf("%12u,",pdata[ii]);
          }
          printf("\n");
        }
      }
      /* put events back into the ET system */
      
      status = et_events_put(et_system_id, attach1, p_et_event, numRead);
      if (status == ET_OK) {
        ;
      }
      else if (status == ET_ERROR_DEAD) {
        printf("%s: ET is dead\n", argv[0]);
        break;
      }
      else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
        printf("%s: socket communication error\n", argv[0]);
        goto error;
      }
      else if (status != ET_OK) {
        printf("%s: put error, status = %d\n", argv[0], status);
        goto error;
      }
      
      count += numRead;
      
      if (args.delay > 0) {
        nanosleep(&timeout, NULL);
      }
      
      /* statistics */
#if defined __APPLE__
      gettimeofday(&t2, NULL);
      time2 = 1000L*t2.tv_sec + t2.tv_usec/1000L; /* milliseconds */
#else
      clock_gettime(CLOCK_REALTIME, &t2);
      time2 = 1000L*t2.tv_sec + t2.tv_nsec/1000000L; /* milliseconds */
#endif
      time = time2 - time1;
      if (time > 500) {
        /* reset things if necessary */
        if ( (totalCount >= (LONG_MAX - count)) ||
            (totalT >= (LONG_MAX - time)) )  {
          totalT = totalCount = count = 0;
          time1 = time2;
          continue;
        }
        rate = 1000.0 * ((double) count) / time;
        totalCount += count;
        totalT += time;
        avgRate = 1000.0 * ((double) totalCount) / totalT;
        printf("%s: %3.4g kHz,  %3.4g kHz Avg.\n", argv[0], rate/1000., avgRate/1000.);
        count = 0;
#if defined __APPLE__
        gettimeofday(&t1, NULL);
        time1 = 1000L*t1.tv_sec + t1.tv_usec/1000L;
#else
        clock_gettime(CLOCK_REALTIME, &t1);
        time1 = 1000L*t1.tv_sec + t1.tv_nsec/1000000L;
#endif
      }
      
    } /* while(1) */
    
    evClose(evio_handle);
    
  }
  
error:

  et_close(et_system_id);
  printf("Exiting\n");
  exit(0);
}

/************************************************************/
/*              separate thread to handle signals           */
static void * signal_thread (void *arg)
{
  sigset_t   signal_set;
  int        sig_number;
  
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  
  /* Not necessary to clean up as ET system will do it */
  sigwait(&signal_set, &sig_number);
  printf("Got a control-C, exiting\n");
  exit(1);
}



void Parse_Args(int *argc,char **argv, Arguments_t *p_arg){
  // Read and parse all the options, leaving only input files in the
  // argv array.
  // This version, all options flags are globals.
#define REMOVE_ONE {(*argc)--;for(j=i;j<(*argc);j++)argv[j]=argv[j+1];i--;}
#define I_PLUS_PLUS if((i+1)<(*argc)){i++;}else{break;}
  int  i,j;
  
  p_arg->debug=1;
  p_arg->et_name="/tmp/ETBuffer";
  p_arg->et_host_name="localhost";
  p_arg->et_port=ET_SERVER_PORT;
  p_arg->et_block=false;
  p_arg->chunk = 1;
  p_arg->delay = 0;
  p_arg->et_event_size_max=2048;
  p_arg->remote = false;
  
  
  for(i=1;i<(*argc);i++){
    if(argv[i][0]=='-'){
      if(strcmp(argv[i],"-quiet")==0 || strcmp(argv[i],"-q")==0){
        p_arg->debug=0;
      }else if(strcmp(argv[i],"-debug")==0 || strcmp(argv[i],"-v")==0){
        p_arg->debug++;
      }else if(strcmp(argv[i],"-block")==0 || strcmp(argv[i],"-b")==0){
        p_arg->et_block=true;
      }else if(strcmp(argv[i],"-n")==0 || strcmp(argv[i],"-numevt")==0){
        I_PLUS_PLUS;
        long int ii;
        sscanf(argv[i],"%ld",&ii);
        //        G_N_Events = ii;
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-f")==0 || strcmp(argv[i],"-et_name")==0){
        I_PLUS_PLUS;
        p_arg->et_name=argv[i];
        //        G_N_Events = ii;
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-H")==0 || strcmp(argv[i],"-host")==0){
        I_PLUS_PLUS;
        p_arg->et_host_name =argv[i];
        //        G_N_Events = ii;
        REMOVE_ONE;
        
      }else if(strcmp(argv[i],"-p")==0 || strcmp(argv[i],"-et_port")==0){
        I_PLUS_PLUS;
        sscanf(argv[i],"%d",&p_arg->et_port);
        //        G_N_Events = ii;
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-c")==0 || strcmp(argv[i],"-chunk")==0){
        I_PLUS_PLUS;
        sscanf(argv[i],"%d",&p_arg->chunk);
        if(p_arg->chunk <1 || p_arg->chunk > 1000){
          cout << "ERROR - chunk size must be between 1 and 1000 \n";
          p_arg->chunk=1;
        }
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-d")==0 || strcmp(argv[i],"-delay")==0){
        I_PLUS_PLUS;
        sscanf(argv[i],"%d",&p_arg->delay);
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-s")==0 || strcmp(argv[i],"-size")==0){
        I_PLUS_PLUS;
        sscanf(argv[i],"%d",&p_arg->et_event_size_max);
        REMOVE_ONE;
      }else if(strcmp(argv[i],"-r")==0||strcmp(argv[i],"-remote")==0)
      {
        p_arg->remote = true;
        
      }else if(strcmp(argv[i],"-help")==0||strcmp(argv[i],"-h")==0)
      {
        Print_Usage(argv[0]);
        exit(1);
      }
      else
      {
        fprintf(stderr,"\nI did not understand the option : %s\n",argv[i]);
        Print_Usage(argv[0]);
        exit(1);
      }
      /* KILL the option from list */
      REMOVE_ONE;
    }
  }
  if( (*argc) <= 1){
    fprintf(stderr,"\nPlease supply at least one evio file name\n");
    exit(1);
  }
  if((*argc) >= 2){
    for(int i=1;i< (*argc); i++){
      p_arg->filenames.push_back( argv[i]);
    }
  }
  if(p_arg->debug){
    cout << "Debug set to: " << p_arg->debug << endl;
    cout << "Files to open: ";
    for(int i=0;i< p_arg->filenames.size();i++){
      cout << " " << p_arg->filenames.at(i);
    }
    cout << endl;
  }
}

//***********************************************************************************
//  Print Usage
//***********************************************************************************

void Print_Usage(const char *name){
  cout << name << " <options>  EVIO_file(s) " << endl;
  cout << endl << " Options: \n";
  cout << "  -q                 Quiet \n";
  cout << "  -v  -debug         Verbose/Debug, can use multiple times. \n";
  cout << "  -f  -et_name name  Attach ET to process with file <name>\n";
  cout << "  -h  -host    host  Attach ET to host\n";
  cout << "  -p  -et_port port  Attach ET to port \n";
  cout << "  -c  -cunk chunk    Get chunk events in one go. [1]\n";
  cout << "  -s  -size evtsize  Set max event size to evtsize in words. [2048]\n";
  cout << "  -r  -remote        Force a remote (network) connection \n";
  
}
