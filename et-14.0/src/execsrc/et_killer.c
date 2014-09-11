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
#include <sys/types.h>
#include <getopt.h>
#include <unistd.h>
#include "et.h"


int main(int argc,char **argv) {  
    int             c, i_tmp, status, remote, numEvents, eventSize, errflg=0, verbose=0;
    unsigned short  serverPort = ET_SERVER_PORT, udpPort = ET_MULTICAST_PORT;
    char            et_name[ET_FILENAME_LENGTH], host[256], mcastAddr[16];
    et_sys_id       id;
    et_openconfig   openconfig;


    /* 4 multiple character command-line options */
    static struct option long_options[] =
    { {"host", 1, NULL, 1},
      {0,0,0,0}};
    
      memset(host, 0, 16);
      memset(mcastAddr, 0, 16);
      memset(et_name, 0, ET_FILENAME_LENGTH);

      while ((c = getopt_long_only(argc, argv, "vrn:p:u:f:", long_options, 0)) != EOF) {
      
          if (c == -1)
              break;

          switch (c) {
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

              case 'f':
                  if (strlen(optarg) >= ET_FILENAME_LENGTH) {
                      fprintf(stderr, "ET file name is too long\n");
                      exit(-1);
                  }
                  strcpy(et_name, optarg);
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

              case ':':
              case 'h':
              case '?':
              default:
                  errflg++;
          }
      }
    
      if (optind < argc || errflg || strlen(host) < 1 || strlen(et_name) < 1) {
          fprintf(stderr, "usage: %s  %s\n\n", argv[0],
                  "-f <ET name> -host <ET host> [-h] [-v] [-r] [-p <server port>] [-u mcast port>");

          fprintf(stderr, "          -host ET system's host\n");
          fprintf(stderr, "          -f ET system's (memory-mapped file) name\n");
          fprintf(stderr, "          -h help\n");
          fprintf(stderr, "          -v verbose output\n");
          fprintf(stderr, "          -r as remote(network) client\n");
          fprintf(stderr, "          -p TCP server port\n");
          fprintf(stderr, "          -u UDP multicast port\n");
          fprintf(stderr, "          This consumer works by making a direct connection\n");
          fprintf(stderr, "          to the ET system's server port.\n\n");
          exit(2);
      }

      /* open ET system */
      et_open_config_init(&openconfig);

      /* EXAMPLE: multicasting to find ET */
    
      et_open_config_setcast(openconfig, ET_MULTICAST);
      et_open_config_addmulticast(openconfig, ET_MULTICAST_ADDR);
      et_open_config_setmultiport(openconfig, udpPort);
      et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
      et_open_config_setTTL(openconfig, 32);

      /* EXAMPLE: direct connection to ET */
      /*
      et_open_config_setcast(openconfig, ET_DIRECT);
      et_open_config_sethost(openconfig, host);
      et_open_config_setserverport(openconfig, serverPort);
      */
      
    if (remote) {
        et_open_config_setmode(openconfig, ET_HOST_AS_REMOTE);
    }
    if (et_open(&id, et_name, openconfig) != ET_OK) {
        printf("%s: et_open problems\n", argv[0]);
        exit(1);
    }
    et_open_config_destroy(openconfig);

    if (verbose) {
        et_system_setdebug(id, ET_DEBUG_INFO);
    }

    et_system_getnumevents(id, &numEvents);
    et_system_geteventsize(id, &eventSize);

    printf("Opened ET system with %d number of events, %d bytes each\n", numEvents, eventSize);
  

    if ((status = et_kill(id)) != ET_OK) {
        printf("%s: error in killing ET system\n", argv[0]);
    }

    return 0;
}
