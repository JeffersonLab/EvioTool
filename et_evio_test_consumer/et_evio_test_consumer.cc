//
//  main.cpp
//  et_evio_test_consumer
//
//  Created by Maurik Holtrop on 5/9/14.
//  Copyright (c) 2014 UNH. All rights reserved.
//
// Based on the ET example code, et_consumer.c
//
/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      ET system sample event consumer
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

#include "et.h"

/* prototype */
static void *signal_thread (void *arg);


int main(int argc,char **argv) {
  int             i, j, c, i_tmp, status, numRead;
  int             flowMode=ET_STATION_SERIAL, position=1, pposition=1;
  int             errflg=0, chunk=1, qSize=0, verbose=0, blocking=1;
  int		        con[ET_STATION_SELECT_INTS];
  int             sendBufSize=0, recvBufSize=0, noDelay=0;
  unsigned short  serverPort = ET_SERVER_PORT;
  char            stationName[ET_STATNAME_LENGTH], et_name[ET_FILENAME_LENGTH];
  char            host[256], mcastAddr[16];
  pthread_t       tid;
  et_att_id       attach1;
  et_stat_id      my_stat;
  et_sys_id       id;
  et_statconfig   sconfig;
  et_event        **pe;
  et_openconfig   openconfig;
  sigset_t        sigblock;
  struct timespec timeout;
  struct timespec sleeptime;
  
#if defined __APPLE__
  struct timeval  t1, t2;
#else
  struct timespec t1, t2;
#endif
  
  /* statistics variables */
  double          rate=0.0, avgRate=0.0;
  int64_t         count=0, totalCount=0, totalT=0, time, time1, time2;
  
  
  /* 4 multiple character command-line options */
  static struct option long_options[] =
  { {"host", 1, NULL, 1},
    {"nb",   0, NULL, 2},
    {"pos",  1, NULL, 3},
    {"ppos", 1, NULL, 4},
    {"rb",   1, NULL, 5},
    {"sb",   1, NULL, 6},
    {"nd",   0, NULL, 7},
    {0,0,0,0}};
  
  memset(host, 0, 16);
  memset(mcastAddr, 0, 16);
  memset(et_name, 0, ET_FILENAME_LENGTH);
  memset(stationName, 0, ET_STATNAME_LENGTH);
  
  while ((c = getopt_long_only(argc, argv, "vn:s:p:f:c:q:", long_options, 0)) != EOF) {
    
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
        
      case 'q':
        i_tmp = atoi(optarg);
        if (i_tmp > 0) {
          qSize = i_tmp;
          printf("Setting queue size to %d\n", qSize);
        } else {
          printf("Invalid argument to -q. Must > 0.\n");
          exit(-1);
        }
        break;
        
      case 's':
        if (strlen(optarg) >= ET_STATNAME_LENGTH) {
          fprintf(stderr, "Station name is too long\n");
          exit(-1);
        }
        strcpy(stationName, optarg);
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
        
      case 'f':
        if (strlen(optarg) >= ET_FILENAME_LENGTH) {
          fprintf(stderr, "ET file name is too long\n");
          exit(-1);
        }
        strcpy(et_name, optarg);
        break;
        
        /* case host: */
      case 1:
        if (strlen(optarg) >= 255) {
          fprintf(stderr, "host name is too long\n");
          exit(-1);
        }
        strcpy(host, optarg);
        break;
        
        /* case nb: */
      case 2:
        blocking = 0;
        break;
        
        /* case pos: */
      case 3:
        i_tmp = atoi(optarg);
        if (i_tmp > 0) {
          position = i_tmp;
        } else {
          printf("Invalid argument to -pos. Must be > 0.\n");
          exit(-1);
        }
        break;
        
        /* case ppos: */
      case 4:
        i_tmp = atoi(optarg);
        if (i_tmp > -3 && i_tmp != 0) {
          pposition = i_tmp;
          flowMode=ET_STATION_PARALLEL;
        } else {
          printf("Invalid argument to -ppos. Must be > -3 and != 0.\n");
          exit(-1);
        }
        break;
        
        /* case rb */
      case 5:
        i_tmp = atoi(optarg);
        if (i_tmp < 1) {
          printf("Invalid argument to -rb. Recv buffer size must be > 0.\n");
          exit(-1);
        }
        recvBufSize = i_tmp;
        break;
        
        /* case sb */
      case 6:
        i_tmp = atoi(optarg);
        if (i_tmp < 1) {
          printf("Invalid argument to -sb. Send buffer size must be > 0.\n");
          exit(-1);
        }
        sendBufSize = i_tmp;
        break;
        
        /* case nd */
      case 7:
        noDelay = 1;
        break;
        
      case 'v':
        verbose++;
        break;
        
      case ':':
      case 'h':
      case '?':
      default:
        errflg++;
    }
  }
  
  if (optind < argc || errflg || strlen(host) < 1 || strlen(et_name) < 1) {
    fprintf(stderr,
            "usage: %s  %s\n%s\n%s\n%s\n",
            argv[0],
            "-f <ET name> -host <ET host> -s <station name> [-h] [-v] [-nb]",
            "                    [-p <ET server port>] [-c <chunk size>] [-q <queue size>]",
            "                    [-pos <station position>] [-ppos <parallel station position>]",
            "                    [-rb <buf size>] [-sb <buf size>] [-nd]");
    
    fprintf(stderr, "          -host ET system's host\n");
    fprintf(stderr, "          -f ET system's (memory-mapped file) name\n");
    fprintf(stderr, "          -s create station of this name\n");
    fprintf(stderr, "          -h help\n");
    fprintf(stderr, "          -v verbose output\n");
    fprintf(stderr, "          -nb make station non-blocking\n");
    fprintf(stderr, "          -p ET server port\n");
    fprintf(stderr, "          -c number of events in one get/put array\n");
    fprintf(stderr, "          -q  queue size if creating nonblocking station\n");
    fprintf(stderr, "          -pos position of created station in station list (1,2,...)\n");
    fprintf(stderr, "          -ppos position of created station within a group of parallel stations (-1=end, -2=head)\n\n");
    fprintf(stderr, "          -rb TCP receive buffer size (bytes)\n");
    fprintf(stderr, "          -sb TCP send    buffer size (bytes)\n");
    fprintf(stderr, "          -nd use TCP_NODELAY option\n\n");
    fprintf(stderr, "          This consumer works by making a direct connection\n");
    fprintf(stderr, "          to the ET system's server port.\n\n");
    exit(2);
  }
  
  
  timeout.tv_sec  = 2;
  timeout.tv_nsec = 0;
  
  sleeptime.tv_sec=0;
  sleeptime.tv_nsec=100;
  
  /* allocate some memory */
  pe = (et_event **) calloc(chunk, sizeof(et_event *));
  if (pe == NULL) {
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

  /* spawn signal handling thread */
  pthread_create(&tid, NULL, signal_thread, (void *)NULL);
  
  /* open ET system */
  et_open_config_init(&openconfig);
  et_open_config_sethost(openconfig, host);
  et_open_config_setcast(openconfig, ET_DIRECT);
  et_open_config_setserverport(openconfig, serverPort);
  /* Defaults are to use operating system default buffer sizes and turn off TCP_NODELAY */
  et_open_config_settcp(openconfig, recvBufSize, sendBufSize, noDelay);
  if (et_open(&id, et_name, openconfig) != ET_OK) {
    printf("%s: et_open problems\n", argv[0]);
    exit(1);
  }
  et_open_config_destroy(openconfig);
  
  if (verbose) {
    et_system_setdebug(id, ET_DEBUG_INFO);
  }
  
  /* define station to create */
  et_station_config_init(&sconfig);
  et_station_config_setflow(sconfig, flowMode);
  if (!blocking) {
    et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
    if (qSize > 0) {
      et_station_config_setcue(sconfig, qSize);
    }
  }
  
  if ((status = et_station_create(id, &my_stat, stationName, sconfig)) != ET_OK) {
    if (status == ET_ERROR_EXISTS) {
      /* my_stat contains pointer to existing station */
      printf("%s: station already exists\n", argv[0]);
    }
    else if (status == ET_ERROR_TOOMANY) {
      printf("%s: too many stations created\n", argv[0]);
      goto error;
    }
    else {
      printf("%s: error in station creation\n", argv[0]);
      goto error;
    }
  }
  et_station_config_destroy(sconfig);
  
  if (et_station_attach(id, my_stat, &attach1) != ET_OK) {
    printf("%s: error in station attach\n", argv[0]);
    goto error;
  }
  
  
  /* read time for future statistics calculations */
#if defined __APPLE__
  gettimeofday(&t1, NULL);
  time1 = 1000L*t1.tv_sec + t1.tv_usec/1000L; /* milliseconds */
#else
  clock_gettime(CLOCK_REALTIME, &t1);
  time1 = 1000L*t1.tv_sec + t1.tv_nsec/1000000L; /* milliseconds */
#endif
  
  
  while (1) {
    
    /**************/
    /* get events */
    /**************/
    
    /* example of single, timeout read */
    /*status = et_events_get(id, attach1, fPEventBuffer, ET_TIMED, &timeout, chunk, &numRead);*/
    
    /* example of reading array of up to "chunk" events -- This DOES NOT WORK on MACs */
#ifdef __APPLE__X
    status = et_event_get(id, attach1, pe, ET_SLEEP, NULL);
    numRead=1;
#else
    status = et_events_get(id, attach1, pe, ET_SLEEP, NULL, chunk, &numRead);
#endif
    if (status == ET_OK) {
      ;
    }
    else if (status == ET_ERROR_DEAD) {
      printf("%s: ET system is dead\n", argv[0]);
      goto error;
    }
    else if (status == ET_ERROR_TIMEOUT) {
      printf("%s: got timeout\n", argv[0]);
      goto end;
    }
    else if (status == ET_ERROR_EMPTY) {
      printf("%s: no events\n", argv[0]);
      goto end;
    }
    else if (status == ET_ERROR_BUSY) {
      printf("%s: station is busy\n", argv[0]);
      goto end;
    }
    else if (status == ET_ERROR_WAKEUP) {
      printf("%s: someone told me to wake up\n", argv[0]);
      goto error;
    }
    else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
      printf("%s: socket communication error\n", argv[0]);
      goto error;
    }
    else if (status != ET_OK) {
      printf("%s: get error\n", argv[0]);
      goto error;
    }
    
    /**************/
    /* print data */
    /**************/
    size_t len,len4;
    unsigned int *data;
    int endian, swap;
    if (verbose>1) printf("Grabbed %2d events from ET\n",numRead);
    for (j=0; j< numRead; j++) {
      et_event_getdata(pe[j], (void **) &data);
      et_event_getlength(pe[j], &len4);
      len = len4/4;
      et_event_getendian(pe[j], &endian);
      et_event_needtoswap(pe[j], &swap);
      
      if (verbose) {
        if(verbose>1)printf("data byte order = %s\n", (endian == ET_ENDIAN_BIG ? "BIG" : "LITTLE"));
        if (swap) {
          printf("data (len = %zu) needs swapping, swapped int = %10d, unswapped = %10d\n", len, ET_SWAP32(data[0]),data[0]);
        }
        else {
          // Print the required 8 word header.
          printf("data (%5zu) : %5d, %2d, %2d, %2d, %2d, 0x%08x %2d, 0x%08x",len,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
          for(int ii=8;ii< (int)len-8 && ii < 16 ;ii++){
            printf("%12u,",data[ii]);
          }
          // Print part or all of the body of the event.
          if(len>16){
            if( verbose > 1){   // Multiple -v arguments.
              for(int ii=16;ii< (int)len-8 ;ii++){
                printf("%12u,",data[ii]);
              }
            }else
              printf("  ... ");
//            for(int ii=(int)len-9;ii< len ;ii++){
//              printf("%12u,",data[ii]);
//            }
            printf("%5d, %2d, %2d, %2d, %2d, 0x%08x, %2d, 0x%08x",data[len-8],data[len-7],data[len-6],data[len-5],data[len-4],data[len-3],data[len-2],data[len-1]);
          }
          printf("\n");
        }
      }
      
      et_event_getcontrol(pe[j], con);
      if (verbose>=2) {
        printf("control array = {");
        for (i=0; i < ET_STATION_SELECT_INTS; i++) {
          printf("%d ", con[i]);
        }
        printf("}\n");
      }
    }
    
    /**************/
    /* put events */
    /**************/
    
    /* example of putting single event */
    /* status = et_event_put(id, attach1, fPEventBuffer[0]);*/
    
    /* example of putting array of events */
    //status = et_events_dump(id, attach1, pe, numRead);
     status = et_events_put(id, attach1, pe, numRead);
    if (status == ET_ERROR_DEAD) {
      printf("%s: ET is dead\n", argv[0]);
      goto error;
    }
    else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
      printf("%s: socket communication error\n", argv[0]);
      goto error;
    }
    else if (status != ET_OK) {
      printf("%s: put error\n", argv[0]);
      goto error;
    }
    
    count += numRead;
//    nanosleep(&sleeptime,NULL);
  
  end:
    
    /* statistics */
#if defined __APPLE__
    gettimeofday(&t2, NULL);
    time2 = 1000L*t2.tv_sec + t2.tv_usec/1000L; /* milliseconds */
#else
    clock_gettime(CLOCK_REALTIME, &t2);
    time2 = 1000L*t2.tv_sec + t2.tv_nsec/1000000L; /* milliseconds */
#endif
    time = time2 - time1;
    if (time > 5000) {
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
      if(verbose==0)printf("%s: %3.4g kHz,  %3.4g kHz Avg.\n", argv[0], rate/1000., avgRate/1000.);
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
  printf("%s: ERROR\n", argv[0]);
  return 0;
}



/************************************************************/
/*              separate thread to handle signals           */
static void *signal_thread (void *arg)
{
  sigset_t        signal_set;
  int             sig_number;
  
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  
  /* Wait for Control-C */
  sigwait(&signal_set, &sig_number);
  
  printf("et_consumer: got a control-C\n");
  exit(1);
}
