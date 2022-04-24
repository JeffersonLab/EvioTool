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
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#ifdef sun
#include <thread.h>
#endif
#include "et.h"





int main(int argc,char **argv)
{
    int             i, j, c, i_tmp, status, swappedData, numRead;
    int             startingVal=0, errflg=0, group=1, chunk=1, size=32, verbose=0;
    int             sendBufSize=0, recvBufSize=0, noDelay=0;
    unsigned short  serverPort = ET_SERVER_PORT;
    char            et_name[ET_FILENAME_LENGTH], host[256], mcastAddr[16], interface[16];
    void            *fakeData;

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
    pthread_t       tid;

    /* statistics variables */
    double          eventRate=0.0, avgEventRate=0.0;
    int64_t         eventCount=0, totalEventCount=0;
    
    double          byteRate=0.0, avgByteRate=0.0;
    int64_t         byteCount=0, totalByteCount=0;
    
    int64_t         totalT=0, time, time1, time2;
    

    /* multiple character command-line options */
    static struct option long_options[] = {
        {"host", 1, NULL, 0},
        {"rb",   1, NULL, 1},
        {"sb",   1, NULL, 2},
        {"nd",   0, NULL, 3},
        {0, 0, 0, 0}
    };

    memset(host, 0, 16);
    memset(mcastAddr, 0, 16);
    memset(et_name, 0, ET_FILENAME_LENGTH);

    while ((c = getopt_long_only(argc, argv, "vn:s:p:d:f:c:g:i:", long_options, 0)) != EOF) {
      
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
                if (i_tmp > 0) {
                    size = i_tmp;
                } else {
                    printf("Invalid argument to -s. Must be a positive integer.\n");
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

            case 0:
                if (strlen(optarg) >= 255) {
                    fprintf(stderr, "host name is too long\n");
                    exit(-1);
                }
                strcpy(host, optarg);
                break;

            case 1:
                i_tmp = atoi(optarg);
                if (i_tmp < 1) {
                    printf("Invalid argument to -rb. Recv buffer size must be > 0.\n");
                    exit(-1);
                }
                recvBufSize = i_tmp;
                break;

            case 2:
                i_tmp = atoi(optarg);
                if (i_tmp < 1) {
                    printf("Invalid argument to -rb. Send buffer size must be > 0.\n");
                    exit(-1);
                }
                sendBufSize = i_tmp;
                break;

            case 3:
                noDelay = 1;
                break;

            case 'v':
                verbose = ET_DEBUG_INFO;
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
                "usage: %s  %s\n%s\n%s\n\n",
                argv[0],
                "-f <ET name> -host <ET host> [-h] [-v] [-c <chunk size>] [-s <event size>]",
                "                     [-g <group>] [-p <ET server port>] [-i <interface address>]",
                "                     [-rb <buf size>] [-sb <buf size>] [-nd]");

        fprintf(stderr, "          -host ET system's host\n");
        fprintf(stderr, "          -f ET system's (memory-mapped file) name\n");
        fprintf(stderr, "          -h help\n");
        fprintf(stderr, "          -v verbose output\n");
        fprintf(stderr, "          -c number of events in one get/put array\n");
        fprintf(stderr, "          -s event size in bytes\n");
        fprintf(stderr, "          -g group from which to get new events (1,2,...)\n");
        fprintf(stderr, "          -p ET server port\n");
        fprintf(stderr, "          -i outgoing network interface IP address (dot-decimal)\n");
        fprintf(stderr, "          -rb TCP receive buffer size (bytes)\n");
        fprintf(stderr, "          -sb TCP send    buffer size (bytes)\n");
        fprintf(stderr, "          -nd use TCP_NODELAY option\n\n");
        fprintf(stderr, "          This blaster works by making a direct connection to the\n");
        fprintf(stderr, "          ET system's server port and blasting as much data as possible\n");
        fprintf(stderr, "          over the connecting TCP socket.\n\n");
        exit(2);
    }

    /* allocate some memory */
    pe = (et_event **) calloc(chunk, sizeof(et_event *));
    if (pe == NULL) {
        printf("%s: out of memory\n", argv[0]);
        exit(1);
    }
    
    fakeData = (void *) malloc(size);
    if (fakeData == NULL) {
        printf("%s: out of memory\n", argv[0]);
        exit(1);
    }
        

    /******************/
    /* open ET system */
    /******************/
    et_open_config_init(&openconfig);
    et_open_config_setcast(openconfig, ET_DIRECT);
    et_open_config_sethost(openconfig, host);
    et_open_config_setserverport(openconfig, serverPort);
    et_open_config_setmode(openconfig, ET_HOST_AS_REMOTE);
    /* Defaults are to use operating system default buffer sizes and turn off TCP_NODELAY */
    et_open_config_settcp(openconfig, recvBufSize, sendBufSize, noDelay);
    if (strlen(interface) > 6) {
        et_open_config_setinterface(openconfig, interface);
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

    /* create thread to deal with client */
//    pthread_create(&tid, NULL, newEventsThread, (void *) pinfo);

  
    /* read time for future statistics calculations */
#if defined __APPLE__
    gettimeofday(&t1, NULL);
    time1 = 1000L*t1.tv_sec + t1.tv_usec/1000L; /* milliseconds */
#else
    clock_gettime(CLOCK_REALTIME, &t1);
    time1 = 1000L*t1.tv_sec + t1.tv_nsec/1000000L; /* milliseconds */
#endif


    while (1) {
      
        status = et_events_new_group(id, attach1, pe, ET_SLEEP | ET_NOALLOC,
                                     NULL, size, chunk, group, &numRead);

        if (status != ET_OK) {
            printf("%s: et_events_new_group error, status = %d\n", argv[0], status);
            goto error;
        }

        /* set data to existing buffer (with ET_NOALLOC), and set data length */
        if (1) {
            for (i=0; i < numRead; i++) {
                et_event_setlength(pe[i], size);
                et_event_setdatabuffer(id, pe[i], fakeData);
            }
        }

        /* put events back into the ET system */
        status = et_events_put(id, attach1, pe, numRead);
        if (status != ET_OK) {
            printf("%s: et_events_put error, status = %d\n", argv[0], status);
            goto error;
        }

        eventCount += numRead;
        byteCount  += numRead*(size + 52) + 20; /* 52 is event overhead, 20 is ET call overhead */
       
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
        
            eventRate = 1000.0 * ((double) eventCount) / time;
            byteRate  = 1000.0 * ((double) byteCount)  / time;
            
            totalEventCount += eventCount;
            totalByteCount  += byteCount;
            
            totalT += time;
            
            avgEventRate = 1000.0 * ((double) totalEventCount) / totalT;
            avgByteRate  = 1000.0 * ((double) totalByteCount)  / totalT;
            
            printf("evRate: %3.4g Hz,  %3.4g avg;  byteRate: %3.4g bytes/sec,  %3.4g avg\n",
                   argv[0], eventRate, avgEventRate, byteRate, avgByteRate);
            
            eventCount = 0;
            byteCount  = 0;
            
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

    printf("%s: DONE\n", argv[0]);
    exit(0);
}
