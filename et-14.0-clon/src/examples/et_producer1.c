/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12H        *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      ET system sample event producer
 *
 *----------------------------------------------------------------------------*/

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

/* prototype */
static void * signal_thread (void *arg);

int main(int argc,char **argv)
{
    int             i, j, c, i_tmp, status, swappedData, numRead;
    int             startingVal=0, errflg=0, group=1, chunk=1, size=32, verbose=0, delay=0, remote=0;
    int             sendBufSize=0, recvBufSize=0, noDelay=0;
    unsigned short  serverPort = ET_SERVER_PORT;
    char            et_name[ET_FILENAME_LENGTH], host[256], mcastAddr[16], interface[16];

    et_att_id	    attach1;
    et_sys_id       id;
    et_openconfig   openconfig;
    et_event        **pe;
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
    int control[] = {17,8,-1,-1,0,0};

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
    memset(et_name, 0, ET_FILENAME_LENGTH);

    while ((c = getopt_long_only(argc, argv, "vrn:s:p:d:f:c:g:i:", long_options, 0)) != EOF) {
      
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
                    size = i_tmp;
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

            case 'f':
                if (strlen(optarg) >= ET_FILENAME_LENGTH) {
                    fprintf(stderr, "ET file name is too long\n");
                    exit(-1);
                }
                strcpy(et_name, optarg);
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
                verbose = ET_DEBUG_INFO;
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
    
    if (optind < argc || errflg || strlen(host) < 1 || strlen(et_name) < 1) {
        fprintf(stderr,
                "usage: %s  %s\n%s\n\n",
                argv[0],
                "-f <ET name> -host <ET host> [-h] [-v] [-r] [-c <chunk size>] [-d <delay>]",
                "                     [-s <event size>] [-g <group>] [-p <ET server port>] [-i <interface address>]",
                "                     [-rb <buf size>] [-sb <buf size>] [-nd]");


        fprintf(stderr, "          -host ET system's host\n");
        fprintf(stderr, "          -f ET system's (memory-mapped file) name\n");
        fprintf(stderr, "          -h help\n");
        fprintf(stderr, "          -v verbose output\n");
        fprintf(stderr, "          -r act as remote (TCP) client even if ET system is local\n");
        fprintf(stderr, "          -c number of events in one get/put array\n");
        fprintf(stderr, "          -d delay in millisec between each round of getting and putting events\n");
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
        timeout.tv_sec  = delay/1000;
        timeout.tv_nsec = (delay - (delay/1000)*1000)*1000000;
    }

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
    if (et_open(&id, et_name, openconfig) != ET_OK) {
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


    while (1) {
      
        status = et_events_new_group(id, attach1, pe, ET_SLEEP, NULL, size, chunk, group, &numRead);
        /*status = et_events_new(id, attach1, pe, ET_SLEEP, NULL, size, chunk, &count);*/

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
        if (1) {
            /*
            char *pdata;
            for (i=0; i < numRead; i++) {
                swappedData = ET_SWAP32(i + startingVal);
                et_event_getdata(pe[i], (void **) &pdata);
                memcpy((void *)pdata, (const void *) &swappedData, sizeof(int));

                et_event_setendian(pe[i], ET_ENDIAN_NOTLOCAL);
                et_event_setlength(pe[i], sizeof(int));
                et_event_setcontrol(pe[i], control, sizeof(control)/sizeof(int));
            }
            startingVal += numRead;
            */
            for (i=0; i < numRead; i++) {
                et_event_setlength(pe[i], size);
            }
        }

        /* put events back into the ET system */
        status = et_events_put(id, attach1, pe, numRead);
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
            printf("%s: %3.4g Hz,  %3.4g Hz Avg.\n", argv[0], rate, avgRate);
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
