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
#include <sys/time.h>
#include <getopt.h>
#include <limits.h>
#include <time.h>

#include "et.h"

/* prototype */
static void *signal_thread (void *arg);


int main(int argc,char **argv) {

    int             i, j, c, i_tmp, status, numRead, locality;
    int             flowMode=ET_STATION_SERIAL, position=ET_END, pposition=ET_END;
    int             errflg=0, chunk=1, qSize=0, verbose=0, remote=0, blocking=1, dump=0, readData=0;
    int             multicast=0, broadcast=0, broadAndMulticast=0;
    int		        con[ET_STATION_SELECT_INTS];
    int             sendBufSize=0, recvBufSize=0, noDelay=0;
    int             debugLevel = ET_DEBUG_ERROR;
    unsigned short  port=0;
    char            stationName[ET_STATNAME_LENGTH], et_name[ET_FILENAME_LENGTH], host[256], interface[16];
    char            localAddr[16];

    int             mcastAddrCount = 0, mcastAddrMax = 10;
    char            mcastAddr[mcastAddrMax][16];

    pthread_t       tid;
    et_att_id       attach1;
    et_stat_id      my_stat;
    et_sys_id       id;
    et_statconfig   sconfig;
    et_event        **pe;
    et_openconfig   openconfig;
    sigset_t        sigblock;
    struct timespec timeout;
#if defined __APPLE__
    struct timeval  t1, t2;
#else
    struct timespec t1, t2;
#endif

    /* statistics variables */
    double          rate=0.0, avgRate=0.0;
    int64_t         count=0, totalCount=0, totalT=0, time, time1, time2, bytes=0, totalBytes=0;


    /* 4 multiple character command-line options */
    static struct option long_options[] =
            { {"host", 1, NULL, 1},
              {"nb",   0, NULL, 2},
              {"pos",  1, NULL, 3},
              {"ppos", 1, NULL, 4},
              {"rb",   1, NULL, 5},
              {"sb",   1, NULL, 6},
              {"nd",   0, NULL, 7},
              {"dump", 0, NULL, 8},
              {"read", 0, NULL, 9},
              {0,0,0,0}};

    memset(host, 0, 256);
    memset(interface, 0, 16);
    memset(mcastAddr, 0, (size_t) mcastAddrMax*16);
    memset(et_name, 0, ET_FILENAME_LENGTH);
    memset(stationName, 0, ET_STATNAME_LENGTH);

    while ((c = getopt_long_only(argc, argv, "vbmhrn:s:p:f:c:q:a:i:", long_options, 0)) != EOF) {

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
                    port = (unsigned short)i_tmp;
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

                /* case dump */
            case 8:
                dump = 1;
                break;

                /* case read */
            case 9:
                readData = 1;
                break;

            case 'v':
                verbose = 1;
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
                argv[0], "-f <ET name> -s <station name>",
                "                     [-h] [-v] [-nb] [-r] [-m] [-b] [-nd] [-read] [-dump]",
                "                     [-host <ET host>] [-p <ET port>]",
                "                     [-c <chunk size>] [-q <Q size>]",
                "                     [-pos <station pos>] [-ppos <parallel station pos>]",
                "                     [-i <interface address>] [-a <mcast addr>]",
                "                     [-rb <buf size>] [-sb <buf size>]");

        fprintf(stderr, "          -f    ET system's (memory-mapped file) name\n");
        fprintf(stderr, "          -host ET system's host if direct connection (default to local)\n");
        fprintf(stderr, "          -s    create station of this name\n");
        fprintf(stderr, "          -h    help\n\n");

        fprintf(stderr, "          -v    verbose output (also prints data if reading with -read)\n");
        fprintf(stderr, "          -read read data (1 int for each event)\n");
        fprintf(stderr, "          -dump dump events back into ET (go directly to GC) instead of put\n");
        fprintf(stderr, "          -c    number of events in one get/put array\n");
        fprintf(stderr, "          -r    act as remote (TCP) client even if ET system is local\n");
        fprintf(stderr, "          -p    port, TCP if direct, else UDP\n\n");

        fprintf(stderr, "          -nb   make station non-blocking\n");
        fprintf(stderr, "          -q    queue size if creating non-blocking station\n");
        fprintf(stderr, "          -pos  position of station (1,2,...)\n");
        fprintf(stderr, "          -ppos position of within a group of parallel stations (-1=end, -2=head)\n\n");

        fprintf(stderr, "          -i    outgoing network interface address (dot-decimal)\n");
        fprintf(stderr, "          -a    multicast address(es) (dot-decimal), may use multiple times\n");
        fprintf(stderr, "          -m    multicast to find ET (use default address if -a unused)\n");
        fprintf(stderr, "          -b    broadcast to find ET\n\n");

        fprintf(stderr, "          -rb   TCP receive buffer size (bytes)\n");
        fprintf(stderr, "          -sb   TCP send    buffer size (bytes)\n");
        fprintf(stderr, "          -nd   use TCP_NODELAY option\n\n");

        fprintf(stderr, "          This consumer works by making a direct connection to the\n");
        fprintf(stderr, "          ET system's server port and host unless at least one multicast address\n");
        fprintf(stderr, "          is specified with -a, the -m option is used, or the -b option is used\n");
        fprintf(stderr, "          in which case multi/broadcasting used to find the ET system.\n");
        fprintf(stderr, "          If multi/broadcasting fails, look locally to find the ET system.\n");
        fprintf(stderr, "          This program gets all events from the given station and puts them back.\n\n");

        exit(2);
    }


    timeout.tv_sec  = 2;
    timeout.tv_nsec = 0;

    /* allocate some memory */
    pe = (et_event **) calloc((size_t)chunk, sizeof(et_event *));
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

    /* Find out if we have a remote connection to the ET system */
    et_system_getlocality(id, &locality);
    if (locality == ET_REMOTE) {
        printf("ET is remote\n\n");

        et_system_gethost(id, host);
        et_system_getlocaladdress(id, localAddr);
        printf("Connect to ET, from ip = %s to %s\n", localAddr, host);
    }
    else {
        printf("ET is local\n\n");
    }

    /* set level of debug output (everything) */
    et_system_setdebug(id, debugLevel);

    /* define station to create */
    et_station_config_init(&sconfig);
    et_station_config_setflow(sconfig, flowMode);
    if (!blocking) {
        et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
        if (qSize > 0) {
            et_station_config_setcue(sconfig, qSize);
        }
    }

    if ((status = et_station_create_at(id, &my_stat, stationName, sconfig, position, pposition)) != ET_OK) {
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
        //status = et_events_get(id, attach1, pe, ET_TIMED | ET_MODIFY, &timeout, chunk, &numRead);

        /* example of reading array of up to "chunk" events */
        status = et_events_get(id, attach1, pe, ET_SLEEP, NULL, chunk, &numRead);
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
        else {
            printf("%s: get error, status = %d\n", argv[0], status);
            goto error;
        }

        /*******************/
        /* read/print data */
        /*******************/
        if (readData) {
            size_t len;
            int *data, endian, swap;

            for (j = 0; j < numRead; j++) {
                et_event_getdata(pe[j], (void **) &data);
                et_event_getlength(pe[j], &len);
                et_event_getendian(pe[j], &endian);
                et_event_needtoswap(pe[j], &swap);

                bytes += len;
                totalBytes += len;

                if (verbose) {
                    printf("data byte order = %s\n", (endian == ET_ENDIAN_BIG ? "BIG" : "LITTLE"));
                    if (swap) {
                        printf("    data (len = %d) needs swapping, swapped int = %d\n", (int) len, ET_SWAP32(data[0]));
                    }
                    else {
                        printf("    data (len = %d) does NOT need swapping, int = %d\n", (int) len, data[0]);
                    }

                    et_event_getcontrol(pe[j], con);
                    printf("control array = {");
                    for (i = 0; i < ET_STATION_SELECT_INTS; i++) {
                        printf("%d ", con[i]);
                    }
                    printf("}\n");
                }
            }
        }
        else {
            size_t len;
            for (j = 0; j < numRead; j++) {
                et_event_getlength(pe[j], &len);
                /* include ET overhead by adding commented out portions */
                bytes += len /* +52 */;
                totalBytes += len /* +52 */;
            }
            /*
            bytes += 20;
            totalBytes += 20;
             */
        }

        /*******************/
        /* put/dump events */
        /*******************/

        if (!dump) {
            /* putting array of events */
            status = et_events_put(id, attach1, pe, numRead);
        }
        else {
            /* dumping array of events */
            status = et_events_dump(id, attach1, pe, numRead);
        }

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
                bytes = totalBytes = totalT = totalCount = count = 0;
                time1 = time2;
                continue;
            }

            rate = 1000.0 * ((double) count) / time;
            totalCount += count;
            totalT += time;
            avgRate = 1000.0 * ((double) totalCount) / totalT;

            /* Event rates */
            printf("%s Events: %3.4g Hz,  %3.4g Avg.\n", argv[0], rate, avgRate);

            /* Data rates */
            rate    = ((double) bytes) / time;
            avgRate = ((double) totalBytes) / totalT;
            printf("%s Data:   %3.4g kB/s,  %3.4g Avg.\n\n", argv[0], rate, avgRate);

            bytes = count = 0;

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
static void *signal_thread (void *arg) {

    sigset_t        signal_set;
    int             sig_number;

    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGINT);

    /* Wait for Control-C */
    sigwait(&signal_set, &sig_number);

    printf("Got control-C, exiting\n");
    exit(1);
}
