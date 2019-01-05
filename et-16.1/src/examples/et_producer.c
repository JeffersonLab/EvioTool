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

#include "et.h"

/* prototype */
static void * signal_thread (void *arg);

int main(int argc,char **argv) {

    int i, j, c, i_tmp, status, junkData, numRead, locality;
    int startingVal=0, errflg=0, group=1, chunk=1, size=32, writeData=0, localEndian=1;
    int delay=0, remote=0, multicast=0, broadcast=0, broadAndMulticast=0;
    int sendBufSize=0, recvBufSize=0, noDelay=0, blast=0, noAllocFlag=0;
    int debugLevel = ET_DEBUG_ERROR;
    unsigned short port=0;
    char et_name[ET_FILENAME_LENGTH], host[256], interface[16], localAddr[16];
    void *fakeData;

    int mcastAddrCount = 0, mcastAddrMax = 10;
    char mcastAddr[mcastAddrMax][16];

    et_att_id attach1;
    et_sys_id id;
    et_openconfig openconfig;
    et_event **pe;
    struct timespec timeout;
#if defined __APPLE__
    struct timeval  t1, t2;
#else
    struct timespec t1, t2;
#endif
    sigset_t sigblock;
    pthread_t tid;

    /* statistics variables */
    double rate = 0.0, avgRate = 0.0;
    int64_t count = 0, totalCount = 0, totalT = 0, time, time1, time2;

    /* control int array for event header if writing junk data */
    int control[] = {17, 8, -1, -1, 0, 0};

    /* 4 multiple character command-line options */
    static struct option long_options[] =
            {{"host",  1, NULL, 1},
             {"rb",    1, NULL, 2},
             {"sb",    1, NULL, 3},
             {"nd",    0, NULL, 4},
             {"blast", 0, NULL, 5},
             {0,       0, 0,    0}
            };

    memset(host, 0, 256);
    memset(interface, 0, 16);
    memset(mcastAddr, 0, mcastAddrMax*16);
    memset(et_name, 0, ET_FILENAME_LENGTH);

    while ((c = getopt_long_only(argc, argv, "vbmhrn:a:s:p:d:f:c:g:i:w:", long_options, 0)) != EOF) {

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

            case 'w':
                writeData = 1;
                i_tmp = atoi(optarg);
                if (i_tmp != 1) {
                    localEndian = 0;
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
                    port = i_tmp;
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

            case 'a':
                if (strlen(optarg) >= 16) {
                    fprintf(stderr, "Multicast address is too long\n");
                    exit(-1);
                }
                if (mcastAddrCount >= mcastAddrMax) break;
                strcpy(mcastAddr[mcastAddrCount++], optarg);
                multicast = 1;
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

                /* case blast */
            case 5:
                blast = 1;
                break;

            case 'v':
                debugLevel = ET_DEBUG_INFO;
                break;

            case 'r':
                remote = 1;
                break;

            case 'm':
                multicast = 1;
                break;

            case 'b':
                broadcast = 1;
                break;

            case ':':
            case 'h':
            case '?':
            default:
                errflg++;
        }
    }

    if (!multicast && !broadcast) {
        /* Default to local host if direct connection */
        if (strlen(host) < 1) {
            strcpy(host, ET_HOST_LOCAL);
        }
    }

    if (optind < argc || errflg || strlen(et_name) < 1) {
        fprintf(stderr,
                "\nusage: %s  %s\n%s\n%s\n%s\n%s\n%s\n%s\n\n",
                argv[0], "-f <ET name>",
                "                     [-h] [-v] [-r] [-m] [-b] [-nd] [-blast]",
                "                     [-host <ET host>] [-w <local endian? 0/1>]",
                "                     [-s <event size>] [-c <chunk size>] [-g <group>]",
                "                     [-d <delay>] [-p <ET port>]",
                "                     [-i <interface address>] [-a <mcast addr>]",
                "                     [-rb <buf size>] [-sb <buf size>]");


        fprintf(stderr, "          -f     ET system's (memory-mapped file) name\n");
        fprintf(stderr, "          -host  ET system's host if direct connection (default to local)\n");
        fprintf(stderr, "          -h     help\n");
        fprintf(stderr, "          -v     verbose output\n\n");

        fprintf(stderr, "          -s     event size in bytes\n");
        fprintf(stderr, "          -c     number of events in one get/put array\n");
        fprintf(stderr, "          -g     group from which to get new events (1,2,...)\n");
        fprintf(stderr, "          -d     delay in millisec between each round of getting and putting events\n\n");

        fprintf(stderr, "          -p     ET port (TCP for direct, UDP for broad/multicast)\n");
        fprintf(stderr, "          -r     act as remote (TCP) client even if ET system is local\n");
        fprintf(stderr, "          -w     write data (1 sequential int per event), 1 local endian, 0 else\n");
        fprintf(stderr, "          -blast if remote, use external data buf (no mem allocation),\n");
        fprintf(stderr, "                 do not write data (overrides -w)\n\n");

        fprintf(stderr, "          -i     outgoing network interface address (dot-decimal)\n");
        fprintf(stderr, "          -a     multicast address(es) (dot-decimal), may use multiple times\n");
        fprintf(stderr, "          -m     multicast to find ET (use default address if -a unused)\n");
        fprintf(stderr, "          -b     broadcast to find ET\n\n");

        fprintf(stderr, "          -rb    TCP receive buffer size (bytes)\n");
        fprintf(stderr, "          -sb    TCP send    buffer size (bytes)\n");
        fprintf(stderr, "          -nd    use TCP_NODELAY option\n\n");

        fprintf(stderr, "          This producer works by making a direct connection to the\n");
        fprintf(stderr, "          ET system's server port and host unless at least one multicast address\n");
        fprintf(stderr, "          is specified with -a, the -m option is used, or the -b option is used\n");
        fprintf(stderr, "          in which case multi/broadcasting used to find the ET system.\n");
        fprintf(stderr, "          If multi/broadcasting fails, look locally to find the ET system.\n");
        fprintf(stderr, "          This program gets new events from the system and puts them back.\n\n");
        exit(2);
    }

    /* fake data for blasting */
    fakeData = (void *) malloc(size);
    if (fakeData == NULL) {
        printf("%s: out of memory\n", argv[0]);
        exit(1);
    }

    /* delay is in milliseconds */
    if (delay > 0) {
        timeout.tv_sec = delay / 1000;
        timeout.tv_nsec = (delay - (delay / 1000) * 1000) * 1000000;
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

    /* spawn signal handling thread */
    pthread_create(&tid, NULL, signal_thread, (void *) NULL);

    /******************/
    /* open ET system */
    /******************/
    et_open_config_init(&openconfig);

    if (broadcast && multicast) {
        broadAndMulticast = 1;
    }

    /* if multicasting to find ET */
    if (multicast) {
        if (mcastAddrCount < 1) {
            /* Use default mcast address if not given on command line */
            status = et_open_config_addmulticast(openconfig, ET_MULTICAST_ADDR);
        }
        else {
            /* add multicast addresses to use  */
            for (j = 0; j < mcastAddrCount; j++) {
                if (strlen(mcastAddr[j]) > 7) {
                    status = et_open_config_addmulticast(openconfig, mcastAddr[j]);
                    if (status != ET_OK) {
                        printf("%s: bad multicast address argument\n", argv[0]);
                        exit(1);
                    }
                    printf("%s: adding multicast address %s\n", argv[0], mcastAddr[j]);
                }
            }
        }
    }

    if (broadAndMulticast) {
        printf("Broad and Multicasting\n");
        if (port == 0) {
            port = ET_UDP_PORT;
        }
        et_open_config_setport(openconfig, port);
        et_open_config_setcast(openconfig, ET_BROADANDMULTICAST);
        et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
    }
    else if (multicast) {
        printf("Multicasting\n");
        if (port == 0) {
            port = ET_UDP_PORT;
        }
        et_open_config_setport(openconfig, port);
        et_open_config_setcast(openconfig, ET_MULTICAST);
        et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
    }
    else if (broadcast) {
        printf("Broadcasting\n");
        if (port == 0) {
            port = ET_UDP_PORT;
        }
        et_open_config_setport(openconfig, port);
        et_open_config_setcast(openconfig, ET_BROADCAST);
        et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
    }
    else {
        if (port == 0) {
            port = ET_SERVER_PORT;
        }
        et_open_config_setserverport(openconfig, port);
        et_open_config_setcast(openconfig, ET_DIRECT);
        if (strlen(host) > 0) {
            et_open_config_sethost(openconfig, host);
        }
        et_open_config_gethost(openconfig, host);
        printf("Direct connection to %s\n", host);
    }
    /*et_open_config_sethost(openconfig, ET_HOST_REMOTE);*/

    /* Defaults are to use operating system default buffer sizes and turn off TCP_NODELAY */
    et_open_config_settcp(openconfig, recvBufSize, sendBufSize, noDelay);
    if (strlen(interface) > 6) {
        et_open_config_setinterface(openconfig, interface);
    }

    if (remote) {
        printf("Set as remote\n");
        et_open_config_setmode(openconfig, ET_HOST_AS_REMOTE);
    }

    /* If responses from different ET systems, return error. */
    et_open_config_setpolicy(openconfig, ET_POLICY_ERROR);

    /* debug level */
    et_open_config_setdebugdefault(openconfig, debugLevel);

    et_open_config_setwait(openconfig, ET_OPEN_WAIT);
    if (et_open(&id, et_name, openconfig) != ET_OK) {
        printf("%s: et_open problems\n", argv[0]);
        exit(1);
    }
    et_open_config_destroy(openconfig);

    /*-------------------------------------------------------*/

    /* Make things self-consistent by not taking time to write data if blasting.
     * Blasting flag takes precedence. */
    if (blast) {
        writeData = 0;
    }

    /* Find out if we have a remote connection to the ET system
     * so we know if we can use external data buffer for events
     * for blasting - which is quite a bit faster. */
    et_system_getlocality(id, &locality);
    if (locality == ET_REMOTE) {
        if (blast) {
            noAllocFlag = ET_NOALLOC;
        }
        printf("ET is remote\n\n");

        et_system_gethost(id, host);
        et_system_getlocaladdress(id, localAddr);
        printf("Connect to ET, from ip = %s to %s\n", localAddr, host);
    }
    else {
        /* local blasting is just the same as local producing */
        blast = 0;
        printf("ET is local\n\n");
    }

    /* set level of debug output (everything) */
    et_system_setdebug(id, debugLevel);

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
      
        status = et_events_new_group(id, attach1, pe, ET_SLEEP | noAllocFlag,
                                     NULL, size, chunk, group, &numRead);

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
        else {
            printf("%s: request error\n", argv[0]);
            goto error;
        }

        /* if blasting data (and remote), don't write anything, just use what's in buffer when allocated */
        if (blast) {
            for (i=0; i < numRead; i++) {
                et_event_setlength(pe[i], size);
                et_event_setdatabuffer(id, pe[i], fakeData);
            }
        }
        /* write data, set control values here */
        else if (writeData) {
            char *pdata;
            for (i=0; i < numRead; i++) {
                junkData = i + startingVal;
                if (!localEndian) {
                    junkData = ET_SWAP32(junkData);
                    et_event_setendian(pe[i], ET_ENDIAN_NOTLOCAL);
                }
                et_event_getdata(pe[i], (void **) &pdata);
                memcpy((void *)pdata, (const void *) &junkData, sizeof(int));

                /* Send all data even though we only wrote one int. */
                et_event_setlength(pe[i], size);
                et_event_setcontrol(pe[i], control, sizeof(control)/sizeof(int));
            }
            startingVal += numRead;
        }
        else {
            for (i = 0; i < numRead; i++) {
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
        else {
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

            /* Event rates */
            printf("%s Events:  %3.4g Hz,    %3.4g Avg.\n", argv[0], rate, avgRate);

            /* Data rates */
            rate    = ((double) count) * size / time;
            avgRate = ((double) totalCount) * size / totalT;
            printf("%s Data:    %3.4g kB/s,  %3.4g Avg.\n\n", argv[0], rate, avgRate);

            /* If including msg overhead in data rates, need to do the following
            avgRate = 1000.0 * (((double) totalCount) * (size + 52) + 20)/ totalT; */

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
static void * signal_thread (void *arg) {

  sigset_t   signal_set;
  int        sig_number;
 
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  
  /* Not necessary to clean up as ET system will do it */
  sigwait(&signal_set, &sig_number);
  printf("Got control-C, exiting\n");
  exit(1);
}
