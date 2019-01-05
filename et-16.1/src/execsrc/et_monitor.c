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
 *      Monitors an ET system by text output
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#include "et_private.h"
#include "et_data.h"

/* prototypes */
static int  display_remotedata(et_sys_id sys_id, double tperiod, uint64_t *prev_out);
static int  display_localdata(et_sys_id sys_id, double tperiod, uint64_t *prev_out);
static int  test_mutex(pthread_mutex_t *mp);

/******************************************************/

static void usage(char *programName) {
    fprintf(stderr,
            "usage: %s  %s\n%s\n%s\n\n",
            programName,
            "-f <ET name> [-h] [-r] [-m] [-b]",
            "                      [-host <ET host>] [-t <time (sec)>]",
            "                      [-p <ET port>] [-a <mcast addr>]");

    fprintf(stderr, "          -f    ET system's (memory-mapped file) name\n");
    fprintf(stderr, "          -host ET system's host if direct connection (default to local)\n");
    fprintf(stderr, "          -h    help\n");
    fprintf(stderr, "          -t    time period in seconds between updates\n");
    fprintf(stderr, "          -r    act as remote (TCP) client even if ET system is local\n\n");

    fprintf(stderr, "          -p    ET port (TCP for direct, UDP for broad/multicast)\n");
    fprintf(stderr, "          -a    multicast address(es) (dot-decimal), may use multiple times\n");
    fprintf(stderr, "          -m    multicast to find ET (use default address if -a unused)\n");
    fprintf(stderr, "          -b    broadcast to find ET\n\n");

    fprintf(stderr, "          This program displays the current status of an ET system\n\n");

}

/******************************************************/
int main(int argc,char **argv) {

 	/* booleans */
    int             remote=0, multicast=0, broadcast=0, broadAndMulticast=0;
    int             c, j, status, counter, etdead, errflg=0, locality, tmparg;
    unsigned int    newheartbt, oldheartbt=0;
    unsigned short  port=0;
    extern char     *optarg;
    extern int      optind, opterr, optopt;
    uint64_t        prev_out;
    struct timespec period;
    double	  tperiod, hbperiod;
    et_sys_id       sys_id;
    et_id 	  *id;
    et_openconfig   openconfig;
    char            host[ET_MAXHOSTNAMELEN];
    char            et_name[ET_FILENAME_LENGTH];
    char            *tmp_etname=NULL;

    int             mcastAddrCount = 0, mcastAddrMax = 10;
    char            mcastAddr[mcastAddrMax][16];

    /* defaults */
    period.tv_sec = 5;
    period.tv_nsec = 0;
    tperiod  = period.tv_sec + (1.e-9)*period.tv_nsec;
    hbperiod = ET_IS_ALIVE_SEC + (1.e-9)*ET_IS_ALIVE_NSEC;

    strcpy(host, "localhost");
  
    /* 1 multiple character command-line option (host) */
    static struct option long_options[] =
            {{"host", 1, NULL, 0},
             {0, 0, 0, 0}
            };

    /* decode command line options */
    while ((c = getopt_long_only(argc, argv, "rhbmf:t:p:a:", long_options, 0)) != EOF) {
        switch (c) {
            case 'f':
                if (strlen(optarg) >= ET_FILENAME_LENGTH) {
                    fprintf(stderr, "%s: ET file name is too long\n", argv[0]);
                    exit(-1);
                }
                strcpy(et_name, optarg);
                tmp_etname = et_name;
                break;

            case 't':
                tmparg = atoi(optarg);
                if (tmparg <= 0) {
                    fprintf(stderr, "%s: argument for -t <time period (sec)> must be integer >0\n\n", argv[0]);
                    errflg++;
                    break;
                }
                period.tv_sec = tmparg;
                tperiod = period.tv_sec + (1.e-9)*period.tv_nsec;
                break;

            case 'p':
                tmparg = atoi(optarg);
                if ((tmparg <= 1024) || (tmparg > 65535)) {
                    fprintf(stderr, "%s: argument for -p <port #> must be integer > 1024 and < 65536\n\n", argv[0]);
                    errflg++;
                    break;
                }
                port = (unsigned short) tmparg;
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

            case 0:
                if (strlen(optarg) >= ET_MAXHOSTNAMELEN) {
                    fprintf(stderr, "host name is too long\n");
                    exit(-1);
                }
                strcpy(host, optarg);
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

            case 'h':
            case 'H':
                errflg++;
                break;

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
        usage(argv[0]);
        exit(2);
    }

    /* Check the ET system name */
    if (tmp_etname == NULL) {
        fprintf(stderr, "\nET file name required\n\n");
        usage(argv[0]);
        exit(-1);
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

    /* before we open things, find out if we're local or not */
    locality = et_findlocality(et_name, openconfig);
/*printf("LOCALITY = %d\n", locality);*/
    /* if we're local, do an et_look not an et_open */
    if (locality != ET_REMOTE) {
        if (et_look(&sys_id, et_name) != ET_OK) {
            printf("%s: et_attach problems\n", argv[0]);
            exit(1);
        }
    }
    else {
        if (et_open(&sys_id, et_name, openconfig) != ET_OK) {
            printf("%s: et_attach problems\n", argv[0]);
            exit(1);
        }
    }

    et_open_config_destroy(openconfig);

    /*-------------------------------------------------------*/

    id = (et_id *) sys_id;
    
    /* initializations */
    if (locality != ET_REMOTE) {
        oldheartbt = id->sys->heartbeat;
    }
    prev_out = 0ULL;
    counter  = 0;
    etdead   = 0;
  
    while (1) {
        if (locality == ET_REMOTE) {
            if (display_remotedata(sys_id, tperiod, &prev_out) != ET_OK) {
                break;
            }
        }
        else {
            /* see if ET system is alive or not */
            if ((counter*tperiod) > hbperiod) {
                newheartbt = id->sys->heartbeat;
                if (oldheartbt == newheartbt) {
                    etdead = 1;
                }
                else {
                    etdead = 0;
                }
                oldheartbt = newheartbt;
                counter = 0;
            }
            counter++;
            if (display_localdata(sys_id, tperiod, &prev_out) != ET_OK) {
                break;
            }
            if (etdead) {
                printf("ET SYSTEM is DEAD!\n");
                printf("*****************************************\n\n");
            }
        }
        /*  wait for "period" before looking at another round of data */
        nanosleep(&period, NULL);
    }
  
    if (locality == ET_REMOTE) {
        et_close(sys_id);
    }
    else {
        et_unlook(sys_id);
    }
  
    return 0;
}


/******************************************************/
static int display_remotedata(et_sys_id sys_id, double tperiod,
                              uint64_t *prev_out)
{
  int             i, j, blocking=0;
  double	  rate = 0.0;
  et_alldata      data;
  et_id 	  *id = (et_id *) sys_id;

  if (et_data_get(sys_id, &data) != ET_OK) {
    printf("display_remotedata: error getting data\n");
    return ET_ERROR;
  }

  printf("  ET SYSTEM - (%s) (host %s) ", data.sysdata->filename, id->ethost);
  if (id->bit64) { printf("(bits 64)\n"); }
  else { printf("(bits 32)\n"); }
  
  printf("              (tcp port %d) (udp port %d)\n",
	  data.sysdata->tcp_port,
	  data.sysdata->udp_port);
  printf("              (pid %d)", data.sysdata->mainpid);
	      
  if (id->lang == ET_LANG_JAVA) {
    printf(" (lang Java)");
  }
  else if (id->lang == ET_LANG_C) {
    printf(" (lang C)");
  }
  else if (id->lang == ET_LANG_CPP) {
    printf(" (lang C++)");
  }
  else {
    printf(" (lang unknown)");
  }
  
  if (id->locality == ET_LOCAL) {
    printf(" (%s) (period = %.0f sec)\n\n", "local", tperiod);
  }
  else if (id->locality == ET_REMOTE) {
    printf(" (%s) (period = %.0f sec)\n\n", "remote", tperiod);
  }
  else {
    printf(" (%s) (period = %.0f sec)\n\n", "local-noshare", tperiod);
  }

  printf("  STATIC INFO - maximum of:\n");
  printf("    events(%d), event size(%llu), temps(%d)\n    stations(%d), attaches(%d), procs(%d)",
	      data.sysdata->nevents,
	      data.sysdata->event_size,
	      data.sysdata->ntemps_max,
	      data.sysdata->nstations_max,
	      data.sysdata->nattachments_max,
	      data.sysdata->nprocesses_max);
  
  if (data.sysdata->ifaddrs.count > 0) {
    printf("\n    network interfaces(%d):", data.sysdata->ifaddrs.count);
    for (i=0; i < data.sysdata->ifaddrs.count; i++) {
      printf(" %s,", data.sysdata->ifaddrs.addr[i]);
    }     
    printf("\n");
  }
  else {
    printf("\n    network interfaces(0): none\n");
  }
  
  if (data.sysdata->mcastaddrs.count > 0) {
    printf("    multicast addresses(%d):", data.sysdata->mcastaddrs.count);
    for (i=0; i < data.sysdata->mcastaddrs.count; i++) {
      printf(" %s,", data.sysdata->mcastaddrs.addr[i]);
    }     
    printf("\n");
  }
  
  printf("\n");

  printf("  DYNAMIC INFO - currently there are:\n");
  printf("    processes(%d), attachments(%d), temps(%d)\n    stations(%d), hearbeat(%d)\n\n",
              data.sysdata->nprocesses,
	      data.sysdata->nattachments,
              data.sysdata->ntemps,
              data.sysdata->nstations,
              data.sysdata->heartbeat);

  printf("  STATIONS:\n");
  for (i=0; i < data.nstations; i++) {      
    printf("    \"%s\" (id = %d)\n      static info\n", data.statdata[i].name, data.statdata[i].num);

    if (data.statdata[i].status == ET_STATION_IDLE) {
      printf("        status(IDLE), ");
    }
    else {
      printf("        status(ACTIVE), ");
    }

    if (data.statdata[i].flow_mode == ET_STATION_SERIAL) {
      printf("flow(SERIAL), ");
    }
    else {
      printf("flow(PARALLEL), ");
    }

    if (data.statdata[i].block_mode == ET_STATION_BLOCKING) {
      printf("blocking(YES), ");
      blocking = 1;
    }
    else {
      printf("blocking(NO), ");
      blocking = 0;
    }

    if (data.statdata[i].user_mode == ET_STATION_USER_MULTI)
      printf("user(MULTI), ");
    else
      printf("user(%d), ", data.statdata[i].user_mode);

    if (data.statdata[i].select_mode == ET_STATION_SELECT_ALL)
      printf("select(ALL)\n");
    else if (data.statdata[i].select_mode == ET_STATION_SELECT_MATCH)
      printf("select(MATCH)\n");
    else if (data.statdata[i].select_mode == ET_STATION_SELECT_USER)
      printf("select(USER)\n");
    else if (data.statdata[i].select_mode == ET_STATION_SELECT_RROBIN)
      printf("select(RROBIN)\n");
    else
      printf("select(EQUALCUE)\n");

    if (data.statdata[i].restore_mode == ET_STATION_RESTORE_OUT)
      printf("        restore(OUT), ");
    else if (data.statdata[i].restore_mode == ET_STATION_RESTORE_IN)
      printf("        restore(IN), ");
    else if (data.statdata[i].restore_mode == ET_STATION_RESTORE_GC)
      printf("        restore(GC), ");
    else
      printf("        restore(REDIST), ");

    printf("prescale(%d), cue(%d), ", data.statdata[i].prescale, data.statdata[i].cue);

    printf("select words(");
    for (j=0; j < ET_STATION_SELECT_INTS; j++) {
        printf("%d,", data.statdata[i].select[j]);
    }
    printf(")\n");

    if (data.statdata[i].select_mode == ET_STATION_SELECT_USER) {
      printf("        lib = %s,  function = %s, class = %s\n",
               data.statdata[i].lib,
	       data.statdata[i].fname,
	       data.statdata[i].classs);
    }
/*
    if (data.statdata[i].status != ET_STATION_ACTIVE) {
      printf("\n");
      continue;
    }
*/
    printf("      dynamic info\n");
    printf("        attachments: total#(%d),  ids(", data.statdata[i].nattachments);

    for (j=0; j < ET_ATTACHMENTS_MAX; j++) {
      if (data.statdata[i].att[j] > -1) {
        printf("%d,", j);
      }
    }
    printf(")\n");

    printf("        input  list: cnt = %5d, events in  = %llu", data.statdata[i].inlist_cnt, 
    data.statdata[i].inlist_in);

    /* if blocking station and not grandcentral ... */
    if (blocking && (data.statdata[i].num != 0)) {
      printf(", events try = %llu\n", data.statdata[i].inlist_try);
    }
    else {
      printf("\n");
    }

    printf("        output list: cnt = %5d, events out = %llu\n\n", data.statdata[i].outlist_cnt,
    data.statdata[i].outlist_out);

    /* keep track of grandcentral data rate */
    if (i==0) {
      rate = (double)(data.statdata[i].outlist_out - *prev_out)/tperiod;
      *prev_out = data.statdata[i].outlist_out;
    } 
  } /* for (i=0; i < data.nstations; i++) */

  /* user processes */
  printf("  LOCAL USERS:\n");
  for (i=0; i < data.nprocs; i++) {      
    if (data.procdata[i].nattachments < 1) {
      printf("    process #%d, # attachments(0), ", data.procdata[i].num);
    }
    else {
      printf("    process #%d, # attachments(%d), attach ids(",
	      data.procdata[i].num, data.procdata[i].nattachments);
      for (j=0; j < data.procdata[i].nattachments; j++) {
	printf("%d,", data.procdata[i].att[j]);
      }
      printf("), ");
    }

    printf("pid(%d), hbeat(%d)\n", data.procdata[i].pid, data.procdata[i].heartbeat);
  }
  printf("\n");

  /* user attachments */
  printf("  ATTACHMENTS:\n");
  for (i=0; i < data.natts; i++) {
      printf("    att #%d, is at station(%s) on host(%s)\n",
             data.attdata[i].num,
             data.attdata[i].station,
             data.attdata[i].host);
      printf("               at pid(%d) from address(%s)\n",
	      data.attdata[i].pid,
          data.attdata[i].interface);
      printf("             proc(%d), ", data.attdata[i].proc);
    if (data.attdata[i].blocked == 1) {
      printf("blocked(YES)");
    }
    else {
      printf("blocked(NO)");
    }
    if (data.attdata[i].quit == 1) {
      printf(", (told to quit)");
    }
    printf("\n             ");
    printf("events:  make(%llu), get(%llu), put(%llu), dump(%llu)\n",
              data.attdata[i].events_make,
              data.attdata[i].events_get,
              data.attdata[i].events_put,
              data.attdata[i].events_dump);
  }
  printf("\n");

  printf("  EVENTS OWNED BY:\n");
  printf("    system (%d),", data.sysdata->events_owned);
  for (i=0; i < data.natts; i++) {
    printf("  att%d (%d),", data.attdata[i].num, data.attdata[i].owned);
  }
  printf("\n\n");

  /* Event rate */
  printf("  EVENT RATE of GC = %.0f events/sec\n\n", rate);

  /* idle stations */
  printf("  IDLE STATIONS:      ");
  for (i=0; i < data.nstations; i++) {
    if (data.statdata[i].status != ET_STATION_IDLE) {
      continue;
    }
    printf("%s, ", data.statdata[i].name);
  }
  printf("\n");

 /* stations linked list */
  printf("  STATION CHAIN:      ");
  for (i=0; i < data.nstations; i++) {
    printf("%s, ", data.statdata[i].name);
  }
  printf("\n");

  /* mutexes */
  if (id->lang != ET_LANG_JAVA) {
    printf("  LOCKED MUTEXES:     ");
    if (data.sysdata->mutex == ET_MUTEX_LOCKED)         printf("system, ");
    if (data.sysdata->stat_mutex == ET_MUTEX_LOCKED)    printf("station, ");
    if (data.sysdata->statadd_mutex == ET_MUTEX_LOCKED) printf("add_station, ");

    for (i=0; i < data.nstations; i++) {
      if (data.statdata[i].mutex == ET_MUTEX_LOCKED)         printf("%s, ",  data.statdata[i].name);
      if (data.statdata[i].inlist_mutex == ET_MUTEX_LOCKED)  printf("%s-in, ",  data.statdata[i].name);
      if (data.statdata[i].outlist_mutex == ET_MUTEX_LOCKED) printf("%s-out, ", data.statdata[i].name);
    }
  }
  printf("\n\n*****************************************\n\n");

  /* free all allocated memory */
  et_data_free(&data);
  return ET_OK;
}


/******************************************************/
static int test_mutex(pthread_mutex_t *mp)
{
  int status;
  
  status = pthread_mutex_trylock(mp);
  if (status != EBUSY) {
    if (status != 0) {
      return ET_ERROR;
    }
    status = pthread_mutex_unlock(mp);
    if (status != 0) {
      return ET_ERROR;
    }
    return ET_MUTEX_UNLOCKED;
  }
  
  return ET_MUTEX_LOCKED;
}


/******************************************************/
static int display_localdata(et_sys_id sys_id, double tperiod, uint64_t *prev_out)
{
  int             i, j, blocking=0;
  int		  *owner, system;
  double	  rate = 0.0;
  et_system       *sys;
  et_id 	  *id;
  et_station      *ps;
  et_event	  *pe;

  id = (et_id *) sys_id;
  sys = id->sys;
  
  if ((owner = (int *) calloc(sys->config.nattachments, sizeof(int))) == NULL) {
      return ET_ERROR;
  }
  
  printf("  ET SYSTEM - (%s) (host %s) ", sys->config.filename, id->ethost);
  if (id->bit64) { printf("(bits 64)\n"); }
  else { printf("(bits 32)\n"); }
  
  printf("              (tcp port %d) (udp port %d)\n",
	  sys->config.serverport, sys->config.port);
  printf("              (pid %d)", (int) sys->mainpid);


  if (id->lang == ET_LANG_JAVA) {
    printf(" (lang Java)");
  }
  else if (id->lang == ET_LANG_C) {
    printf(" (lang C)");
  }
  else if (id->lang == ET_LANG_CPP) {
    printf(" (lang C++)");
  }
  else {
    printf(" (lang unknown)");
  }
  
  if (id->locality == ET_LOCAL) {
    printf(" (%s) (period = %.0f sec)\n\n", "local", tperiod);
  }
  else if (id->locality == ET_REMOTE) {
    printf(" (%s) (period = %.0f sec)\n\n", "remote", tperiod);
  }
  else {
    printf(" (%s) (period = %.0f sec)\n\n", "local-noshare", tperiod);
  }

  printf("  STATIC INFO - maximum of:\n");
  printf("    events(%d), event size(%llu), temps(%d)\n    stations(%d), attaches(%d), procs(%d)",
	      sys->config.nevents,
	      sys->config.event_size,
	      sys->config.ntemps,
	      sys->config.nstations,
	      sys->config.nattachments,
	      sys->config.nprocesses);

  if (sys->config.netinfo.count > 0) {
    printf("\n    network interfaces(%d):", sys->config.netinfo.count);
    for (i=0; i < sys->config.netinfo.count; i++) {
      printf(" %s,", sys->config.netinfo.ipinfo[i].addr);
    }     
    printf("\n");
  }
  else {
    printf("\n    network interfaces(0): none\n");
  }
  
  if (sys->config.mcastaddrs.count > 0) {
    printf("    multicast addresses(%d):", sys->config.mcastaddrs.count);
    for (i=0; i < sys->config.mcastaddrs.count; i++) {
      printf(" %s,", sys->config.mcastaddrs.addr[i]);
    }     
    printf("\n");
  }
  
  printf("\n");
  printf("  DYNAMIC INFO - currently there are:\n");
  printf("    processes(%d), attachments(%d), temps(%d)\n    stations(%d), hearbeat(%d)\n\n",
              sys->nprocesses,
	      sys->nattachments,
              sys->ntemps,
              sys->nstations,
              sys->heartbeat);

  printf("  STATIONS:\n");
  ps = id->stats;
  for (i=0; i < id->sys->config.nstations; i++) {
    if ((ps->data.status != ET_STATION_ACTIVE) &&
        (ps->data.status != ET_STATION_IDLE))    {
      ps++;
      continue;
    }

    printf("    \"%s\" (id = %d)\n      static info\n", ps->name, ps->num);

    if (ps->data.status == ET_STATION_IDLE) {
      printf("        status(IDLE), ");
    }
    else {
      printf("        status(ACTIVE), ");
    }
    
    if (ps->config.flow_mode == ET_STATION_PARALLEL) {
      printf("flow(PARALLEL), ");
    }
    else {
      printf("flow(SERIAL), ");
    }

    if (ps->config.block_mode == ET_STATION_BLOCKING) {
      printf("blocking(YES), ");
      blocking = 1;
    }
    else {
      printf("blocking(NO), ");
      blocking = 0;
    }

    if (ps->config.user_mode == ET_STATION_USER_MULTI)
      printf("user(MULTI), ");
    else
      printf("user(%d), ", ps->config.user_mode);

    if (ps->config.select_mode == ET_STATION_SELECT_ALL)
      printf("select(ALL)\n");
    else if (ps->config.select_mode == ET_STATION_SELECT_MATCH)
      printf("select(MATCH)\n");
    else if (ps->config.select_mode == ET_STATION_SELECT_USER)
      printf("select(USER)\n");
    else if (ps->config.select_mode == ET_STATION_SELECT_RROBIN)
      printf("select(RROBIN)\n");
    else
      printf("select(EQUALCUE)\n");

    if (ps->config.restore_mode == ET_STATION_RESTORE_OUT)
      printf("        restore(OUT), ");
    else if (ps->config.restore_mode == ET_STATION_RESTORE_IN)
      printf("        restore(IN), ");
    else if (ps->config.restore_mode == ET_STATION_RESTORE_GC)
      printf("        restore(GC), ");
    else
      printf("        restore(REDIST), ");

    printf("prescale(%d), cue(%d), ", ps->config.prescale, ps->config.cue);

    printf("select words(");
    for (j=0; j < ET_STATION_SELECT_INTS; j++) {
        printf("%d,", ps->config.select[j]);
    }
    printf(")\n");

    if (ps->config.select_mode == ET_STATION_SELECT_USER) {
      printf("        lib = %s,  function = %s, class = %s\n",
              ps->config.lib,
	      ps->config.fname,
	      ps->config.classs);
    }
/*
    if (ps->data.status != ET_STATION_ACTIVE) {
      printf("\n");
      ps++;
      continue;
    }
*/
    printf("      dynamic info\n");
    printf("        attachments: total#(%d),  ids(", ps->data.nattachments);

    for (j=0; j < ET_ATTACHMENTS_MAX; j++) {
      if (ps->data.att[j] > -1) {
        printf("%d,", j);
      }
    }
    printf(")\n");

    printf("        input  list: cnt = %5d, events in  = %llu", ps->list_in.cnt,
           ps->list_in.events_in);

    /* if blocking station and not grandcentral ... */
    if (blocking && (ps->num != 0)) {
      printf(", events try = %llu\n", ps->list_in.events_try);
    }
    else {
      printf("\n");
    }

    printf("        output list: cnt = %5d, events out = %llu\n\n", ps->list_out.cnt,
           ps->list_out.events_out);

    /* keep track of grandcentral data rate */
    if (i==0) {
      rate = (double) (ps->list_out.events_out - *prev_out)/tperiod;
      *prev_out = ps->list_out.events_out;
    } 
    ps++;
  }  /* for (i=0; i < id->sys->config.nstations; i++) */

  /* user processes */
  printf("  LOCAL USERS:\n");
  for (i=0; i < sys->config.nprocesses; i++) {
    if (sys->proc[i].status != ET_PROC_OPEN) {
      continue;
    }

    if (sys->proc[i].nattachments < 1) {
      printf("    process #%d, # attachments(0), ", sys->proc[i].num);
    }
    else {
      printf("    process #%d, # attachments(%d), attach ids(",
	      sys->proc[i].num, sys->proc[i].nattachments);
      for (j=0; j < sys->config.nattachments; j++) {
        if (sys->proc[i].att[j] != -1) {
	  printf("%d,", sys->proc[i].att[j]);
	}
      }
      printf("), ");
    }

    printf("pid(%d), hbeat(%d)\n", (int) sys->proc[i].pid, sys->proc[i].heartbeat);
  }
  printf("\n");
  
  /* user attachments */
  printf("  ATTACHMENTS:\n");
  for (i=0; i < sys->config.nattachments; i++) {
    if (sys->attach[i].status == ET_ATT_UNUSED) {
      continue;
    }
    ps = id->stats + sys->attach[i].stat;
    printf("    att #%d, is at station(%s) on host(%s)\n",
           sys->attach[i].num,
           ps->name,
           sys->attach[i].host);
    printf("               at pid(%d) from address(%s)\n",
           sys->attach[i].pid,
           sys->attach[i].interface);

    printf("             proc(%d), ", sys->attach[i].proc);
    if (sys->attach[i].blocked == 1) {
      printf("blocked(YES)");
    }
    else {
      printf("blocked(NO)");
    }
    if (sys->attach[i].quit == 1) {
      printf(", (told to quit)");
    }
    printf("\n             ");

    printf("events:  make(%llu), get(%llu), put(%llu), dump(%llu)\n",
              sys->attach[i].events_make,
              sys->attach[i].events_get,
              sys->attach[i].events_put,
              sys->attach[i].events_dump);
  }
  printf("\n");

  /* find events' owners (client attachments or system) */
  for (i=0; i < sys->config.nattachments; i++) {
    owner[i] = 0;
  }
  system = 0;

  pe = id->events;
  for (i=0; i < sys->config.nevents; i++) {
    if (pe->owner == ET_SYS) {
      system++;
    }
    else {
      owner[pe->owner]++;
    }
    pe++;
  }

  printf("  EVENTS OWNED BY:\n");
  printf("    system (%d),", system);
  for (i=0; i < sys->config.nattachments; i++) {
    if (sys->attach[i].status != ET_ATT_ACTIVE) {
      continue;
    }
    printf("  att%d (%d),", i, owner[i]);
  }
  free(owner);
  printf("\n\n");

  /* Event rate */
  printf("  EVENT RATE of GC = %.0f events/sec\n\n", rate);

  /* creating stations */
  printf("  CREATING STATIONS:  ");
  ps = id->stats;
  for (i=0; i < id->sys->config.nstations; i++) {
    if (ps->data.status != ET_STATION_CREATING) {
      ps++;
      continue;
    }
    printf("%s, ", ps->name);
    ps++;
  }
  printf("\n");

  /* idle stations */
  printf("  IDLE STATIONS:      ");
  ps = id->stats;
  for (i=0; i < id->sys->config.nstations; i++) {
    if (ps->data.status != ET_STATION_IDLE) {
      ps++;
      continue;
    }
    printf("%s, ", ps->name);
    ps++;
  }
  printf("\n");

  /* stations linked list */
  printf("  STATION CHAIN:      ");
  for (ps = id->stats + sys->stat_head;; ps = id->stats + ps->next) {
    printf("%s, ", ps->name);
    if ((ps->num == sys->stat_tail) || (ps->next < 0)) {
      break;
    }
  }
  printf("\n");

  if (id->share == ET_MUTEX_SHARE) {
    if (id->lang != ET_LANG_JAVA) {
      /* test mutexes */
      printf("  LOCKED MUTEXES:     ");
      if (test_mutex(&sys->mutex) == ET_MUTEX_LOCKED)         printf("system, ");
      if (test_mutex(&sys->stat_mutex) == ET_MUTEX_LOCKED)    printf("station, ");
      if (test_mutex(&sys->statadd_mutex) == ET_MUTEX_LOCKED) printf("add_station, ");

      ps = id->stats;
      for (i=0; i < sys->config.nstations; i++) {
        if (test_mutex(&ps->mutex) == ET_MUTEX_LOCKED)          printf("%s, ", ps->name);
        if (test_mutex(&ps->list_in.mutex) == ET_MUTEX_LOCKED)  printf("%s-in, ", ps->name);
        if (test_mutex(&ps->list_out.mutex) == ET_MUTEX_LOCKED) printf("%s-out, ", ps->name);
        ps++;
      }
    }
    printf("\n");
  }

  printf("\n*****************************************\n\n");
  return ET_OK;
}
