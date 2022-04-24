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
 *      Sample event transfer between 2 ET system
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#ifdef sun
#include <thread.h>
#endif
#include "et.h"

#define NUMEVENTS 2000000
#define CHUNK 100

/* prototype */
static void * signal_thread (void *arg);

int main(int argc,char **argv) {  
  int             status, swtch, loops=0;
  int		  ntransferred=0;
  pthread_t       tid;
  et_statconfig   sconfig;
  et_openconfig   openconfig;
  et_bridgeconfig bconfig;
  sigset_t        sigblock;
  struct timespec timeout;
  et_att_id       att_from, att_to;
  et_stat_id      stat_from;
  et_sys_id       id_from, id_to;
  int             selections[] = {17,15,-1,-1}; /* 17,15 are arbitrary */

 
  if (argc != 5) {
    printf("Usage: %s <from etname> <to etname> <station_name> <mode>\n", argv[0]);
    exit(1);
  }
  
  timeout.tv_sec  = 0;
  timeout.tv_nsec = 1;
  
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
  
  /* open 2 ET systems */
  et_open_config_init(&openconfig);
  et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
  if (et_open(&id_from, argv[1], openconfig) != ET_OK) {
    printf("%s: et_open problems\n", argv[0]);
    exit(1);
  }
  if (et_open(&id_to, argv[2], openconfig) != ET_OK) {
    printf("%s: et_open problems\n", argv[0]);
    exit(1);
  }
  et_open_config_destroy(openconfig);
  
  swtch = atoi(argv[4]);
    
  et_station_config_init(&sconfig);
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig, 1);
  et_station_config_setcue(sconfig, 150);
 
  if (swtch==1) {
      /* DD system "all" mode */
      et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
      et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
  }
  else if (swtch==2) {
      /* DD system "on req" mode */
      et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
      et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  }
  else if (swtch==3) {
      /* DD system "condition" mode */
      et_station_config_setselect(sconfig, ET_STATION_SELECT_MATCH);
      et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
      et_station_config_setselectwords(sconfig, selections);
  }
  else if (swtch==4) {
      /* new non-blocking "condition" mode */
      et_station_config_setselect(sconfig, ET_STATION_SELECT_MATCH);
      et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
      et_station_config_setselectwords(sconfig, selections);
  }
  else if (swtch==5) {
      /* user's condition, blocking  mode */
      et_station_config_setselect(sconfig, ET_STATION_SELECT_USER);
      et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
      et_station_config_setselectwords(sconfig, selections);
      if (et_station_config_setfunction(sconfig, "et_carls_function") == ET_ERROR) {
	printf("%s: cannot set function\n", argv[0]);
	exit(1);
      }
      if (et_station_config_setlib(sconfig, "/home/timmer/cvs/coda/source/et/src/libet_user.so") == ET_ERROR) {
        printf("%s: cannot set library\n", argv[0]);
	exit(1);
      }
  }
  else if (swtch==6) {
      /* user's condition, nonblocking mode */
      et_station_config_setselect(sconfig, ET_STATION_SELECT_USER);
      et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
      et_station_config_setselectwords(sconfig, selections);
      et_station_config_setfunction(sconfig, "et_carls_function");
      et_station_config_setlib(sconfig, "/home/timmer/cvs/coda/source/et/src/libet_user.so");
  }
  
  /* set debug level */
  et_system_setdebug(id_from, ET_DEBUG_INFO);
  et_system_setdebug(id_to,   ET_DEBUG_INFO);

  if ((status = et_station_create(id_from, &stat_from, argv[3], sconfig)) < ET_OK) {
    if (status == ET_ERROR_EXISTS) {
      /* my_stat contains pointer to existing station */;
      printf("%s: station already exists\n", argv[0]);
    }
    else {
      printf("%s: error in station creation\n", argv[0]);
      goto error;
    }
  }
  et_station_config_destroy(sconfig);

  if (et_station_attach(id_from, stat_from, &att_from) < 0) {
    printf("%s: error in station attach\n", argv[0]);
    goto error;
  }

  if (et_station_attach(id_to, ET_GRANDCENTRAL, &att_to) < 0) {
    printf("%s: error in station attach\n", argv[0]);
    goto error;
  }

  et_bridge_config_init(&bconfig);
  /* automatically swap data with provided function */
  /* et_bridge_config_setfunc(bconfig, et_bridge_CODAswap); */
  
  while ((loops++ < 1) && (status == ET_OK)) {
    status = et_events_bridge(id_from, id_to, att_from, att_to,
			      bconfig, NUMEVENTS, &ntransferred);
    printf("status from et_bridge = %d, ntransferred = %d\n", status, ntransferred);
  }
  et_bridge_config_destroy(bconfig);

  et_forcedclose(id_from);
  et_forcedclose(id_to);
  return 0;
error:
  return -1;
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
  printf("et_2_et: got a control-C, exiting\n");
  exit(1);
}
