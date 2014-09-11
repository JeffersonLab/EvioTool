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
myptr[6] = 1; /* Reserved */ \
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

#include <string>
using namespace std;

/* prototype */
static void * signal_thread (void *arg);

int main(int argc,char **argv)
{
  int             c, i_tmp, status, numRead;
  int             errflg=0;
  int             group=1;
  int             chunk=1;
  int             et_event_size_max=2*1024;
  int             verbose=0;
  int             delay=0;
  int             remote=0;
  int             sendBufSize=0, recvBufSize=0, noDelay=0;
  unsigned short  serverPort = ET_SERVER_PORT;
  char            host[256], mcastAddr[16], interface[16];
  int control[] = {0,0,0,0,0,0};   // Control int array for header.
  
  string et_name;
  string evio_file_name;
  
  int evio_handle=0;
  
  et_att_id	    attach1;
  et_sys_id       id;
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
  
  /* control int array for event header */
  //  int control[] = {17,8,-1,-1,0,0};
  
  /* 4 multiple character command-line options */
  static struct option long_options[] =
  { {"host", 1, NULL, 1},
    {"rb",   1, NULL, 2},
    {"sb",   1, NULL, 3},
    {"nd",   0, NULL, 4},
    {0,0,0,0}
  };
  
  memset(host, 0, 16);
  memset(mcastAddr, 0, 16);
  
  while ((c = getopt_long_only(argc, argv, "vrn:s:p:d:f:c:g:i:e:", long_options, 0)) != EOF) {
    
    if (c == -1)
      break;
    
    switch (c) {
      case 'c':
        i_tmp = atoi(optarg);
        if (i_tmp > 0 && i_tmp < 1001) {
          chunk = i_tmp;
          printf("Setting chunk to %d\n", chunk);
        } else {
          printf("Invalid argument to -c. Must < 1001 & > 0.\n");
          exit(-1);
        }
        break;
        
      case 's':
        i_tmp = atoi(optarg);
        if (i_tmp > -1) {
          et_event_size_max = i_tmp;
        } else {
          printf("Invalid argument to -s. Must be a positive integer.\n");
          exit(-1);
        }
        break;
        
      case 'd':
        delay = atoi(optarg);
        if (delay < 0) {
          printf("Delay must be >= 0\n");
          exit(-1);
        }
        break;
        
      case 'p':
        i_tmp = atoi(optarg);
        if (i_tmp > 1023 && i_tmp < 65535) {
          serverPort = i_tmp;
        } else {
          printf("Invalid argument to -p. Must be < 65535 & > 1023.\n");
          exit(-1);
        }
        break;
        
      case 'g':
        i_tmp = atoi(optarg);
        if (i_tmp > 0 && i_tmp < 501) {
          group = i_tmp;
        } else {
          printf("Invalid argument to -g. Must be 501 > g > 0.\n");
          exit(-1);
        }
        break;
        
      case 'e':
        evio_file_name = optarg;
        break;
        
      case 'f':
        et_name = optarg;
        break;
        
      case 'i':
        if (strlen(optarg) > 15 || strlen(optarg) < 7) {
          fprintf(stderr, "interface address is bad\n");
          exit(-1);
        }
        strcpy(interface, optarg);
        break;
        
      case 1:
        if (strlen(optarg) >= 255) {
          fprintf(stderr, "host name is too long\n");
          exit(-1);
        }
        strcpy(host, optarg);
        break;
        
        /* case rb */
      case 2:
        i_tmp = atoi(optarg);
        if (i_tmp < 1) {
          printf("Invalid argument to -rb. Recv buffer size must be > 0.\n");
          exit(-1);
        }
        recvBufSize = i_tmp;
        break;
        
        /* case sb */
      case 3:
        i_tmp = atoi(optarg);
        if (i_tmp < 1) {
          printf("Invalid argument to -sb. Send buffer size must be > 0.\n");
          exit(-1);
        }
        sendBufSize = i_tmp;
        break;
        
        /* case nd */
      case 4:
        noDelay = 1;
        break;
        
      case 'v':
        if(verbose<ET_DEBUG_INFO)
          verbose = ET_DEBUG_INFO;
        else
          verbose++;
        break;
        
      case 'r':
        remote = 1;
        break;
        
      case ':':
      case 'h':
      case '?':
      default:
        errflg++;
    }
  }
  if( strlen(host) < 1 ){
    strcpy(host,"localhost");
  }
  
  if (optind < argc || errflg  || et_name.length() < 1) {
    fprintf(stderr,
            "usage: %s  %s\n%s\n%s\n\n",
            argv[0],
            "-e <EVIO_File> -f <ET name> -host <ET host> [-h] [-v] [-r] [-c <chunk size>] [-d <delay>]",
            "                     [-s <event size>] [-g <group>] [-p <ET server port>] [-i <interface address>]",
            "                     [-rb <buf size>] [-sb <buf size>] [-nd]");
    
    fprintf(stderr, "          -e EVIO file name.\n");
    fprintf(stderr, "          -host ET system's host\n");
    fprintf(stderr, "          -f ET system's (memory-mapped file) name\n");
    fprintf(stderr, "          -h help\n");
    fprintf(stderr, "          -v verbose output\n");
    fprintf(stderr, "          -r act as remote (TCP) client even if ET system is local\n");
    fprintf(stderr, "          -c number of events in one get/put array\n");
    fprintf(stderr, "          -d delay in micro-sec between each round of getting and putting events\n");
    fprintf(stderr, "          -s event size in bytes\n");
    fprintf(stderr, "          -g group from which to get new events (1,2,...)\n");
    fprintf(stderr, "          -p ET server port\n");
    fprintf(stderr, "          -i outgoing network interface IP address (dot-decimal)\n\n");
    fprintf(stderr, "          -rb TCP receive buffer size (bytes)\n");
    fprintf(stderr, "          -sb TCP send    buffer size (bytes)\n");
    fprintf(stderr, "          -nd use TCP_NODELAY option\n\n");
    fprintf(stderr, "          This consumer works by making a direct connection to the\n");
    fprintf(stderr, "          ET system's server port.\n");
    exit(2);
  }
  
  /* delay is in milliseconds */
  if (delay > 0) {
    timeout.tv_sec  = (int)(delay/1000000);
    timeout.tv_nsec = (delay - (int)(delay/1000000)*1000000)*1000;
  }
  
  /* allocate some memory */
  p_et_event = (et_event **) calloc(chunk, sizeof(et_event *));
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
  et_open_config_sethost(openconfig, host);
  et_open_config_setserverport(openconfig, serverPort);
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
  
  if (remote) {
    et_open_config_setmode(openconfig, ET_HOST_AS_REMOTE);
  }
  
  et_open_config_setwait(openconfig, ET_OPEN_WAIT);
  if (et_open(&id, et_name.c_str(), openconfig) != ET_OK) {
    printf("%s: et_open problems\n", argv[0]);
    exit(1);
  }
  et_open_config_destroy(openconfig);
  
  /* set level of debug output (everything) */
  if (verbose) {
    et_system_setdebug(id, ET_DEBUG_INFO);
  }
  
  /* attach to grandcentral station */
  if (et_station_attach(id, ET_GRANDCENTRAL, &attach1) < 0) {
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
  
  if( evOpen((char *)evio_file_name.c_str(),(char *)"r",&evio_handle) !=S_SUCCESS){
    printf("Error opening EVIO file: %s \n",evio_file_name.c_str());
    exit(1);
  }
  
  
  printf("Starting the main event loop\n");
  
  unsigned long evt_count=0;
  bool evio_has_events=true;

  
//  uint32_t *evio_buf=(uint32_t *) malloc(et_event_size_max*sizeof(uint32_t));
  uint32_t *evio_buf=NULL;
  
  while (evio_has_events) {
    
#if defined __APPLE__
    //    et_system_setgroup(id, group); // Setting the group does not work on a Mac.
    status = et_event_new(id, attach1, p_et_event, ET_SLEEP, NULL, et_event_size_max*sizeof(uint32_t));
    numRead=1;
    //    status = et_events_new(id, attach1, p_et_event, ET_SLEEP, NULL, et_event_size_max, chunk, &numRead); // Neither does sending more than one event request.
#else
    status = et_events_new_group(id, attach1, p_et_event, ET_SLEEP, NULL, et_event_size_max*sizeof(uint32_t), chunk, group, &numRead);
#endif
    
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
    uint32_t evio_buflen=2048;
    uint32_t evio_buflen2=0;
    uint32_t *evio_ptr;
 
    for (int i=0; i < numRead; i++) {
      int handle1=1;
      et_event_getdata(p_et_event[i], (void **) &pdata);
      et_event_getlength(p_et_event[i],&et_data_buflen);

      if( (status=evReadAlloc(evio_handle,&evio_ptr,&evio_buflen)) != S_SUCCESS){
        if(status == EOF) printf("EVIO -- End of events reached at event %d\n",pdata[4]);
        else  printf("EVIO -- Other error: %d \n",status);
        evio_has_events=false;
      }
      char wr[3]="w";
//D      printf("READ: evio_buflen = %d -- \n",evio_buflen);
      
      evio_buf = pdata;
      unsigned int stat = evOpenBuffer((char *)evio_buf, evio_buflen+16,wr,&handle1); // Buffer length is in 32-bit words.
      if(stat!=0) printf("evOpenBuffer returns status %x \n",stat);

      stat = evWrite(handle1,(uint32_t *)evio_ptr);
//      if(evio_buflen + 16 > et_event_size_max){
//        printf("Event too large: %d > %d \n",evio_buflen,et_event_size_max);
//        memcpy(pdata,evio_buf,et_event_size_max*sizeof(uint32_t));
//      }else{
//        memcpy(pdata,evio_buf,(evio_buflen+16)*sizeof(uint32_t));
//      }
//      
      if(stat!=0) printf("evWrite returns status %x \n",stat);
      stat=evGetBufferLength(handle1,&evio_buflen2);
      if(stat!=0) printf("evGetBufferLength returns status %x \n",stat);
//      if(verbose == ET_DEBUG_INFO) printf("GetBufferLength = %d  SetLength= %d\n",evio_buflen2,evio_buflen);
      stat = evClose(handle1);

      if(stat!=0) printf("evClose returns status %x \n",stat);
      et_data_buflen = evio_buflen+16;
//      et_data_buflen = pdata[0]+1;

//D      printf("Pdata: %5d %3d %3d %3d %3d 0x%04x 0x%08x \n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[7]);
      //       et_event_setendian(p_et_event[i], ET_ENDIAN_NOTLOCAL);
      et_event_setendian(p_et_event[i], ET_ENDIAN_LOCAL);
      et_event_setlength(p_et_event[i], et_data_buflen*sizeof(uint32_t));
      et_event_setcontrol(p_et_event[i], control, sizeof(control)/sizeof(int));
      
      if(verbose == ET_DEBUG_INFO){
          printf("data (%5d) :",et_data_buflen);
          for(int ii=0; ii<et_data_buflen+8 && ii<12;ii++){
            printf("%12u,",pdata[ii]);
          }
          printf("\n");
      }
    }
    /* put events back into the ET system */
    status = et_events_put(id, attach1, p_et_event, numRead);
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
    
    if (delay > 0) {
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
  
  
error:
  
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
