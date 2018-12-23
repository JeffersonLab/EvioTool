//
//  main.cpp
//  EvioTool_Test
//
//  Created by Maurik Holtrop on 5/7/14.
//  Copyright (c) 2014 UNH. All rights reserved.
//
#include <time.h>
#include <sys/time.h>
#include <limits.h>

#include <iostream>
#include <string>
using namespace std;

#include "EvioEvent.h"
#include "EvioTool.h"

struct Arguments_t {
  string filename;
  string et_name;
  string et_host_name;
  int    et_port;
  int    debug;
  bool   use_et;
  bool   et_block;
  
};

void Print_Usage(const char *name);
void Parse_Args(int *argc, const char **argv, Arguments_t *p_arg);

int main(int argc, const char * argv[])
{
  Arguments_t args;
  EVIO_Event_t *evt;

  Parse_Args(&argc,argv,&args);
  
#if defined __APPLE__
  struct timeval  t1, t2;
#else
  struct timespec t1, t2;
#endif
  long time1,time2;
  
  evt = new EVIO_Event_t;
  EvioEventInit(evt);
  
  EvioTool *etool;
  if(args.use_et){
    etool= new EvioTool();
    if(args.et_name.length()) etool->et_file_name=args.et_name;
    if(args.et_port) etool->et_port = args.et_port;
    if(args.et_host_name.length()) etool->et_host_name =args.et_host_name;
    
    if(etool->openEt()){
      cout << "Error opeing the ET system. Exit. \n";
      exit(1);
    }
  }else{
    etool= new EvioTool(args.filename.c_str());
  }
  etool->fDebug = args.debug;

#if defined __APPLE__
  gettimeofday(&t1, NULL);
  time1 = 1000L*t1.tv_sec + t1.tv_usec/1000L; /* milliseconds */
#else
  clock_gettime(CLOCK_REALTIME, &t1);
  time1 = 1000L*t1.tv_sec + t1.tv_nsec/1000000L; /* milliseconds */
#endif
  
  long evt_count=0;
  long totalCount=0;
  long totalT=0;
  
  while(etool->read()){
    if(args.debug) cout<<"EVIO Event " << evt_count << endl;
    etool->parse(evt);
    evt_count++;
    
    if(evt_count%10000 ==0 ){
      /* statistics */
#if defined __APPLE__
      gettimeofday(&t2, NULL);
      time2 = 1000L*t2.tv_sec + t2.tv_usec/1000L; /* milliseconds */
#else
      clock_gettime(CLOCK_REALTIME, &t2);
      time2 = 1000L*t2.tv_sec + t2.tv_nsec/1000000L; /* milliseconds */
#endif
      long time = time2 - time1;
      if (time > 1000) {
        /* reset things if necessary */
        if ( (totalCount >= (LONG_MAX - evt_count)) ||
            (totalT >= (LONG_MAX - time)) )  {
          totalT = totalCount = evt_count = 0;
          time1 = time2;
          continue;
        }
        double rate = 1000.0 * ((double) evt_count) / time;
        totalCount += evt_count;
        totalT += time;
        double avgRate = 1000.0 * ((double) totalCount) / totalT;
        printf("%s: %3.4g kHz,  %3.4g kHz Avg. Event: %6d\n", argv[0], rate/1000., avgRate/1000.,evt->event_number);
        evt_count = 0;
#if defined __APPLE__
        gettimeofday(&t1, NULL);
        time1 = 1000L*t1.tv_sec + t1.tv_usec/1000L;
#else
        clock_gettime(CLOCK_REALTIME, &t1);
        time1 = 1000L*t1.tv_sec + t1.tv_nsec/1000000L;
#endif
      }
    }
  }
  printf("Last event: %6d\n",evt->event_number);
  return 0;
}

void Parse_Args(int *argc,const char **argv, Arguments_t *p_arg){
  // Read and parse all the options, leaving only input files in the
  // argv array.
  // This version, all options flags are globals.
#define REMOVE_ONE {(*argc)--;for(j=i;j<(*argc);j++)argv[j]=argv[j+1];i--;}
#define I_PLUS_PLUS if((i+1)<(*argc)){i++;}else{break;}
  int  i,j;
  
  p_arg->debug=0;
  p_arg->use_et=0;
  p_arg->filename="";
  p_arg->et_name="";
  p_arg->et_host_name="";
  p_arg->et_port=0;
  p_arg->et_block=false;
  
  for(i=1;i<(*argc);i++){
    if(argv[i][0]=='-'){
      if(strcmp(argv[i],"-quiet")==0 || strcmp(argv[i],"-q")==0){
        p_arg->debug=0;
      }else if(strcmp(argv[i],"-debug")==0 || strcmp(argv[i],"-d")==0){
        p_arg->debug++;
      }else if(strcmp(argv[i],"-block")==0 || strcmp(argv[i],"-b")==0){
        p_arg->et_block=true;
      }else if(strcmp(argv[i],"-et")==0 || strcmp(argv[i],"-etring")==0){
        p_arg->use_et=1;
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
  if( (*argc) != 2 && !p_arg->use_et){
    fprintf(stderr,"\nPlease supply one and only one file name\n");
    exit(1);
  }
  if((*argc) >= 2) p_arg->filename = argv[1];
  if(p_arg->debug){
    cout << "Debug set to: " << p_arg->debug << endl;
    if(p_arg->use_et){
      cout << "Opening a channel to the ET system." << endl;
    }else{
      cout << "File to open: " << p_arg->filename << endl;
    }
  }
}

//***********************************************************************************
//  Print Usage
//***********************************************************************************

void Print_Usage(const char *name){
  cout << name << " <options>  EVIO_file " << endl;
  cout << endl << " Options: \n";
  cout << "  -q                 Quiet \n";
  cout << "  -d  -debug         Debug \n";
  cout << "  -et                Use ET ring \n";
  cout << "  -f  -et_name name  Attach ET to process with file <name>\n";
  cout << "  -h  -host    host  Attach ET to host\n";
  cout << "  -p  -et_port port  Attach ET to port \n";
}



