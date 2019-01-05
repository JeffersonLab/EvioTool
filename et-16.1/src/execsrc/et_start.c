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
 *      An example program to show how to start up an Event Transfer system.
 *
 *----------------------------------------------------------------------------*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#include "et_private.h"


void printHelp(char *program) {
    fprintf(stderr,
            "\nusage: %s  %s\n%s\n%s\n%s",
            program,
            "[-h] [-v] [-d] [-f <file>] [-n <events>] [-s <eventSize>]",
            "                 [-g <groups>] [-stats <max # of stations>]",
            "                 [-p <TCP server port>] [-u <UDP port>] [-a <multicast address>]",
            "                 [-rb <buf size>] [-sb <buf size>] [-nd]\n\n");

    fprintf(stderr, "          -h     help\n");
    fprintf(stderr, "          -v     verbose output\n");
    fprintf(stderr, "          -d     deletes any existing file first\n");
    fprintf(stderr, "          -f     memory-mapped file name\n\n");

    fprintf(stderr, "          -n     number of events\n");
    fprintf(stderr, "          -s     event size in bytes\n");
    fprintf(stderr, "          -g     number of groups to divide events into\n");
    fprintf(stderr, "          -stats max # of stations (default 200)\n\n");

    fprintf(stderr, "          -p     TCP server port #\n");
    fprintf(stderr, "          -u     UDP (broadcast &/or multicast) port #\n");
    fprintf(stderr, "          -a     multicast address\n\n");

    fprintf(stderr, "          -rb    TCP receive buffer size (bytes)\n");
    fprintf(stderr, "          -sb    TCP send    buffer size (bytes)\n");
    fprintf(stderr, "          -nd    use TCP_NODELAY option\n\n");

    fprintf(stderr, "          This program starts up an ET system.\n");
    fprintf(stderr, "          Listens on 239.200.0.0 by default.\n\n");
}


int main(int argc, char **argv) {

    int c, i_tmp, errflg = 0;
    extern char *optarg;
    extern int optind;

    int mcastAddrCount = 0, mcastAddrMax = 10;
    char mcastAddr[mcastAddrMax][ET_IPADDRSTRLEN];
    int j, status, sig_num, serverPort = 0, udpPort = 0;
    int et_verbose = ET_DEBUG_NONE, deleteFile = 0;
    int sendBufSize = 0, recvBufSize = 0, noDelay = 0;
    int maxNumStations = 0;
    sigset_t sigblockset, sigwaitset;
    et_sysconfig config;
    et_sys_id id;

    /************************************/
    /* default configuration parameters */
    /************************************/
    int nGroups = 1;
    int nevents = 2000;               /* total number of events */
    int event_size = 3000;            /* size of event in bytes */
    char *et_filename = NULL;
    char et_name[ET_FILENAME_LENGTH];

    /* multiple character command-line options */
    static struct option long_options[] = {
            {"rb",    1, NULL, 1},
            {"sb",    1, NULL, 2},
            {"nd",    0, NULL, 3},
            {"stats", 1, NULL, 4},
            {0,       0, 0,    0}
    };

    /* Use default multicast address */
    memset(mcastAddr, 0, ET_IPADDRSTRLEN);

    while ((c = getopt_long_only(argc, argv, "vhdn:s:p:u:m:a:f:g:", long_options, 0)) != EOF) {

        if (c == -1)
            break;

        switch (c) {
            case 'n':
                i_tmp = atoi(optarg);
                if (i_tmp > 0) {
                    nevents = i_tmp;
                } else {
                    printf("Invalid argument to -n. Must be a positive integer.\n");
                    exit(-1);
                }
                break;

            case 's':
                i_tmp = atoi(optarg);
                if (i_tmp > 0) {
                    event_size = i_tmp;
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

            case 'u':
                i_tmp = atoi(optarg);
                if (i_tmp > 1023 && i_tmp < 65535) {
                    udpPort = i_tmp;
                } else {
                    printf("Invalid argument to -u. Must be < 65535 & > 1023.\n");
                    exit(-1);
                }
                break;

            case 'a':
                if (strlen(optarg) >= 16) {
                    fprintf(stderr, "Multicast address is too long\n");
                    exit(-1);
                }
                if (mcastAddrCount >= mcastAddrMax) break;
                strcpy(mcastAddr[mcastAddrCount++], optarg);
                break;

            case 'g':
                i_tmp = atoi(optarg);
                if (i_tmp > 0 && i_tmp < 501) {
                    nGroups = i_tmp;
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
                et_filename = et_name;
                break;

                /* Remove an existing memory-mapped file first, then recreate it. */
            case 'd':
                deleteFile = 1;
                break;

            case 'v':
                et_verbose = ET_DEBUG_INFO;
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

            case 4:
                i_tmp = atoi(optarg);
                if (i_tmp < 2) {
                    printf("Invalid argument to -stats. Must allow at least 2 stations.\n");
                    exit(-1);
                }
                maxNumStations = i_tmp;
                break;

            case 'h':
                printHelp(argv[0]);
                exit(1);

            case ':':
            case '?':
            default:
                errflg++;
        }
    }

    /* Listen to default multicast address if nothing else */
    if (mcastAddrCount < 1) {
        strcpy(mcastAddr[0], ET_MULTICAST_ADDR);
        mcastAddrCount++;
    }

    /* Error of some kind */
    if (optind < argc || errflg) {
        printHelp(argv[0]);
        exit(2);
    }

    /* Check et_filename */
    if (et_filename == NULL) {
        /* see if env variable SESSION is defined */
        if ((et_filename = getenv("SESSION")) == NULL) {
            fprintf(stderr, "No ET file name given and SESSION env variable not defined\n");
            exit(-1);
        }
        /* check length of name */
        if ((strlen(et_filename) + 12) >= ET_FILENAME_LENGTH) {
            fprintf(stderr, "ET file name is too long\n");
            exit(-1);
        }
        sprintf(et_name, "%s%s", "/tmp/et_sys_", et_filename);
    }

    for (; optind < argc; optind++) {
        printf("%s\n", argv[optind]);
    }

    if (et_verbose) {
        printf("et_start: ET version %d; %d - %d byte-sized events\n", ET_VERSION, nevents, event_size);
    }

    if (deleteFile) {
        remove(et_name);
    }

    /********************************/
    /* set configuration parameters */
    /********************************/

    if (et_system_config_init(&config) == ET_ERROR) {
        printf("et_start: no more memory\n");
        exit(1);
    }

    /* divide events into equal groups and any leftovers into another group */
    if (nGroups > 1) {
        int i, addgroup = 0, *groups, totalNE = 0;

        int n = nevents / nGroups;
        int r = nevents % nGroups;
        if (r > 0) {
            addgroup = 1;
        }

        groups = calloc(nGroups + addgroup, sizeof(int));
        if (groups == NULL) {
            printf("et_start: out of memory\n");
            exit(1);
        }

        for (i = 0; i < nGroups; i++) {
            groups[i] = n;
            totalNE += n;
        }

        if (addgroup) {
            groups[nGroups] = r;
            totalNE += r;
        }
        printf("Have %d total events divided\n", totalNE);
        et_system_config_setgroups(config, groups, nGroups + addgroup);

        free(groups);
    }

    /* total number of events */
    et_system_config_setevents(config, nevents);

    /* size of event in bytes */
    et_system_config_setsize(config, event_size);

    /* max # of stations */
    if (maxNumStations > 1) {
        et_system_config_setstations(config, maxNumStations);
    }

    /* limit on # of stations = 20 */
    /* hard limit on # of processes = 110 */
    /* hard limit on # of attachments = 110 */
    /* max # of temporary (specially allocated mem) events = 300 */

    /* set TCP server port */
    if (serverPort > 0) et_system_config_setserverport(config, serverPort);

    /* set UDP (broadcast/multicast) port */
    if (udpPort > 0) et_system_config_setport(config, udpPort);

    /* set server's TCP parameters */
    et_system_config_settcp(config, recvBufSize, sendBufSize, noDelay);

    /* add multicast address to listen to  */
    for (j = 0; j < mcastAddrCount; j++) {
        if (strlen(mcastAddr[j]) > 7) {
            status = et_system_config_addmulticast(config, mcastAddr[j]);
            if (status != ET_OK) {
                printf("et_start: bad multicast address argument\n");
                exit(1);
            }
            printf("et_start: adding multicast address %s\n",mcastAddr[j]);
        }
    }

    /* Make sure filename is null-terminated string */
    if (et_system_config_setfile(config, et_name) == ET_ERROR) {
        printf("et_start: bad filename argument\n");
        exit(1);
    }

    /*************************/
    /* setup signal handling */
    /*************************/
    sigfillset(&sigblockset);
    status = pthread_sigmask(SIG_BLOCK, &sigblockset, NULL);
    if (status != 0) {
        printf("et_start: pthread_sigmask failure\n");
        exit(1);
    }
    sigemptyset(&sigwaitset);
    /* Java uses SIGTERM to end processes it spawns.
     * So if this program is run by a Java JVM, the
     * following line allows the JVM to kill it. */
    sigaddset(&sigwaitset, SIGTERM);
    sigaddset(&sigwaitset, SIGINT);

    /*************************/
    /*    start ET system    */
    /*************************/
    if (et_verbose) {
        printf("et_start: starting ET system %s\n", et_name);
    }
    if (et_system_start(&id, config) != ET_OK) {
        printf("et_start: error in starting ET system\n");
        exit(1);
    }

    et_system_setdebug(id, et_verbose);

    /* turn this thread into a signal handler */
    sigwait(&sigwaitset, &sig_num);

    printf("Interrupted by CONTROL-C or SIGTERM\n");
    printf("ET is exiting\n");
    et_system_close(id);

    exit(0);
}

