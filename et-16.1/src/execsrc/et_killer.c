/*----------------------------------------------------------------------------*
 *  Copyright (c) 2013        Jefferson Science Associates,                   *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, Suite 3       *
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
#include <getopt.h>

#include "et.h"


int main(int argc,char **argv) {  

    int             c, j, status, i_tmp, remote=0, numEvents, errflg=0, verbose=0;
    int             multicast=0, broadcast=0, broadAndMulticast=0;
    size_t          eventSize;
    unsigned short  port=0;
    char            et_name[ET_FILENAME_LENGTH], host[256];
    et_sys_id       id;
    et_openconfig   openconfig;
    int             mcastAddrCount = 0, mcastAddrMax = 10;
    char            mcastAddr[mcastAddrMax][16];


    /* 4 multiple character command-line options */
    static struct option long_options[] =
            { {"host", 1, NULL, 1},
              {0,0,0,0}};

    memset(host, 0, 16);
    memset(mcastAddr, 0, (size_t) mcastAddrMax*16);
    memset(et_name, 0, ET_FILENAME_LENGTH);

    while ((c = getopt_long_only(argc, argv, "vbmrn:p:f:a:", long_options, 0)) != EOF) {

        if (c == -1)
            break;

        switch (c) {
            case 'p':
                i_tmp = atoi(optarg);
                if (i_tmp > 1023 && i_tmp < 65535) {
                    port = (unsigned short) i_tmp;
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

            case 'a':
                if (strlen(optarg) >= 16) {
                    fprintf(stderr, "Multicast address is too long\n");
                    exit(-1);
                }
                if (mcastAddrCount >= mcastAddrMax) break;
                strcpy(mcastAddr[mcastAddrCount++], optarg);
                multicast = 1;
                break;

            case 'r':
                remote = 1;
                break;

                /* case host: */
            case 1:
                if (strlen(optarg) >= 255) {
                    fprintf(stderr, "host name is too long\n");
                    exit(-1);
                }
                strcpy(host, optarg);
                break;

            case 'v':
                verbose = ET_DEBUG_INFO;
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
        if (strlen(host) < 1) {
            fprintf(stderr, "\nNeed to specify the specific host with -host flag\n\n");
            errflg++;
        }
    }

    if (optind < argc || errflg || strlen(et_name) < 1) {
        fprintf(stderr, "usage: %s  %s\n%s\n%s\n\n", argv[0],
                "-f <ET name>  ",
                "                     [-h] [-v] [-r] [-m] [-b] [-host <ET host>]",
                "                     [-p <ET port>] [-a <mcast addr>]");

        fprintf(stderr, "          -f    ET system's (memory-mapped file) name\n");
        fprintf(stderr, "          -host ET system's host if direct connection (default to local)\n");
        fprintf(stderr, "          -h    help\n");
        fprintf(stderr, "          -v    verbose output\n\n");

        fprintf(stderr, "          -a    multicast address(es) (dot-decimal), may use multiple times\n");
        fprintf(stderr, "          -m    multicast to find ET (use default address if -a unused)\n");
        fprintf(stderr, "          -b    broadcast to find ET\n\n");

        fprintf(stderr, "          -r    act as remote (TCP) client even if ET system is local\n");
        fprintf(stderr, "          -p    port (TCP for direct, UDP for broad/multicast)\n\n");

        fprintf(stderr, "          This program works by making a direct connection to the\n");
        fprintf(stderr, "          ET system's server port and host unless at least one multicast address\n");
        fprintf(stderr, "          is specified with -a, the -m option is used, or the -b option is used\n");
        fprintf(stderr, "          in which case multi/broadcasting used to find the ET system.\n");
        fprintf(stderr, "          If multi/broadcasting fails, look locally to find the ET system.\n");
        fprintf(stderr, "          The system is then killed.\n\n");
        exit(2);
    }

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

    if (remote) {
        printf("Set as remote\n");
        et_open_config_setmode(openconfig, ET_HOST_AS_REMOTE);
    }

    et_open_config_setwait(openconfig, ET_OPEN_WAIT);
    if (et_open(&id, et_name, openconfig) != ET_OK) {
        printf("%s: et_open problems\n", argv[0]);
        exit(1);
    }
    et_open_config_destroy(openconfig);

    /*-------------------------------------------------------*/

    if (verbose) {
        et_system_setdebug(id, ET_DEBUG_INFO);
    }

    et_system_getnumevents(id, &numEvents);
    et_system_geteventsize(id, &eventSize);

    printf("Opened ET system with %d number of events, %d bytes each\n", numEvents, (int)eventSize);
  

    if (et_kill(id) != ET_OK) {
        printf("%s: error in killing ET system\n", argv[0]);
    }

    return 0;
}
