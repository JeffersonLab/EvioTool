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


#include "et.h"

#define NUMEVENTS 2000000

/* prototype */
static void * signal_thread (void *arg);

int main(int argc,char **argv) {  
  int             status, swtch, ntransferred=0;
  pthread_t       tid;
  et_statconfig   sconfig;
  et_openconfig   openconfig;
  et_bridgeconfig bconfig;
  sigset_t        sigblock;
  struct timespec timeout;
  et_att_id       att_from, att_to;
  et_stat_id      stat_from;
  et_sys_id       id_from, id_to;

 
  if (argc != 5) {
    printf("Usage: %s <from etname> <to etname> <station_name> <mode (1,2)>\n", argv[0]);
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

  /* spawn signal handling thread */
  pthread_create(&tid, NULL, signal_thread, (void *)NULL);
  
  /* open 2 ET systems w/ broadcasting & multicasting on default address & port */
  et_open_config_init(&openconfig);
  et_open_config_setcast(openconfig, ET_BROADANDMULTICAST);
  et_open_config_sethost(openconfig, ET_HOST_ANYWHERE);
  et_open_config_addmulticast(openconfig, ET_MULTICAST_ADDR);
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
 
  if (swtch == 2) {
    /* non-blocking station, no filter */
    et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
    et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  }
  else {
    /* blocking station, no filter */
    et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
    et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
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
  status = ET_OK;
  while (status == ET_OK) {
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
