/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 * TJNAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606 *
 *      heyes@cebaf.gov   Tel: (757) 269-7030    Fax: (757) 269-5800          *
 *----------------------------------------------------------------------------*
 * Description:
 *      ET system sample event client
 *
 * Author:
 *	Carl Timmer
 *	TJNAF Data Acquisition Group
 *
 * Revision History:
 *
 *----------------------------------------------------------------------------*/

/*

clon10:

et_start -f /tmp/et_sys_sertest -n 500 -s 100000
fpack2et /tmp/et_sys_sertest
et_2_et clon10:/tmp/et_sys_sertest clon00:/tmp/et_sys_sertest ET2ET

clon00:

et_start -f /tmp/et_sys_sertest -n 500 -s 100000
et2bos_test /tmp/et_sys_sertest TEST

*/

/*

ET system has following definitions

ET_ENDIAN_BIG     0 - sparc
ET_ENDIAN_LITTLE  1 - intel

ET_ENDIAN_SWITCH   4 - will force ET system to switch endian

  use functions

et_event_setendian()
et_event_getendian()

  to set or get endian.

Inside ET system has another parameter:

pe->byteorder - can be 0x04030201 or 0x01020304

 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#ifdef sun
#include <thread.h>
#endif
#include "et.h"


/*sergey:temp, to print endian*/
#include "et_private.h"

/*sergey: temporary*/
#include "./bosRecordSwap.c"

int
et_bridge_BOS(et_event *src_ev, et_event *dest_ev, int bytes, int same_endian)
{
  int   nlongs;
  nlongs = bytes/sizeof(int);

  /* swap if necessary */
  BOSrecordSwap((int *)src_ev->pdata, (int *)dest_ev->pdata);

  return(ET_OK);
}

/* signal handler prototype */
static void *signal_thread (void *arg);

#ifdef CODASSIPC
#include "./codassipc.c"
char  project[1000];
char  session[1000];
char  uniquename[1000];


/*mutex-protected event counter*/
static int nevents;
pthread_mutex_t mutex_nevents;
#define SSTIMEINTERVAL 10.0 /* # seconds to sleep between smartsocket transactions */

/* ssipc thread */
static int force_exit;
static void *
ssipc_thread (void *arg)
{
  SSTHREAD *ssarg, ssargin;
  ssarg = (SSTHREAD *)arg;
  while(1)
  {
    pthread_mutex_lock(&mutex_nevents);
    ssarg->rate_rec = ssarg->rate_proc = ((float)nevents)/SSTIMEINTERVAL;
    nevents = 0;
    /*printf("2: rec = %f %f\n",ssarg->rate_rec,ssarg->rate_proc);*/
    /*printf("2: nev = %d %d\n\n",ssarg->nev_rec,ssarg->nev_proc);*/
    pthread_mutex_unlock(&mutex_nevents);

    ssipc_put_args(ssarg);
    ipc_check(SSTIMEINTERVAL);
    ssipc_get_args(&ssargin);
    if(force_exit)
    {
      printf("ssipc_thread: force_exit\n");
      break;
    }
  }
  force_exit = 0;
  return(0);
}

#endif


#define NUMEVENTS 400
#define CHUNK 100
#define STRLEN 256


int
main(int argc, char **argv)
{
#ifdef CODASSIPC
  SSTHREAD ssarg;
  pthread_t       sid;
#endif
  sigset_t        sigblock;
  pthread_t       tid;

  int             i, j, status, numread, chunk, totalread=0, loops=0;
  int		      con[ET_STATION_SELECT_INTS], ntransferred=0;
  et_statconfig   sconfig;
  et_openconfig   openconfig;
  et_bridgeconfig bconfig;
  struct timespec timeout;
  et_att_id       att_from, att_to;
  et_stat_id      stat_from, stat_to;
  et_sys_id       id_from, id_to;
  int             selections[] = {17,15,-1,-1}; /* 17,15 are arbitrary */

  char from_node[STRLEN], from_et[STRLEN], to_node[STRLEN], to_et[STRLEN];
  char from_station[STRLEN];
  char option[STRLEN];
  char from_node_ssipc[STRLEN];
  char ch[STRLEN], *cc;

  if((argc != 4) && (argc != 5))
  {
    printf("Usage: %s <from_node:from_et> <to_node:to_et> <from_station> [<option>]\n",
            argv[0]);
    exit(1);
  }


  /*******************/
  /* parse arguments */
  /*******************/

  strcpy(ch,argv[1]);
  cc = strchr(ch,':'); /* get pointer to the location of ':' */
  if(cc==NULL)
  {
    printf("wrong arguments in 'from' - exit\n");
    exit(0);
  }
  cc[0] = '\0'; /* replace ':' by the end of string */
  strcpy(from_node,ch);
  strcpy(from_et,(cc+1));
  printf("from_node >%s<, from_et >%s<\n",from_node,from_et);

  strcpy(ch,argv[2]);
  cc = strchr(ch,':'); /* get pointer to the location of ':' */
  if(cc==NULL)
  {
    printf("wrong arguments in 'to' - exit\n");
    exit(0);
  }
  cc[0] = '\0'; /* replace ':' by the end of string */
  strcpy(to_node,ch);
  strcpy(to_et,(cc+1));
  printf("to_node >%s<, to_et >%s<\n",to_node,to_et);

  strcpy(from_station,argv[3]);
  printf("from_station >%s<\n",from_station);

  if(argc == 5)
  {
    strcpy(option,argv[4]);
    printf("option >%s<\n",option);
  }

  if(strlen(from_node)==0)
  {
    printf("wrong 'from_node' - exit\n");
    exit(0);
  }
  if(strlen(from_et)==0)
  {
    printf("wrong 'from_et' - exit\n");
    exit(0);
  }
  if(strlen(to_node)==0)
  {
    printf("wrong 'to_node' - exit\n");
    exit(0);
  }
  if(strlen(to_et)==0)
  {
    printf("wrong 'to_et' - exit\n");
    exit(0);
  }
  if(strlen(from_station)==0)
  {
    printf("wrong 'from_station' - exit\n");
    exit(0);
  }

  timeout.tv_sec  = 0;
  timeout.tv_nsec = 1;


#ifdef CODASSIPC
  /* always use 'clasprod' for project (what about test setut etc ???????) */
  strcpy(project,"clasprod");

  {
    /* extract 'session' from the tail of 'to_et' */
    strcpy(ch,to_et);
    cc = strrchr(ch,'_'); /* get pointer to the location of last '_' */
    if(cc==NULL)
    {
      printf("must be at least one '_' in 'to_et' - exit\n");
      exit(0);
    }
    strcpy(session,(cc+1));
  }
#endif
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



  /* init ET configuration will be used for both ET systems */
  et_open_config_init(&openconfig);


  /*****************/
  /* open local ET */
  /*****************/

  /* clon10: 129.57.167.14 */
  /* clon10-daq1: 129.57.68.21 */
  /* clon10-daq2: 129.57.68.22 */
  /*et_open_config_sethost(openconfig, ET_HOST_LOCAL);*/
  /*et_open_config_sethost(openconfig, "clon10-daq1");*/
  et_open_config_sethost(openconfig, from_node);
  et_open_config_gethost(openconfig, ch);
  printf("local host >%s<\n",ch);
  /*et_open_config_setport(openconfig, 12345);*/



  et_open_config_addbroadcast(openconfig,"129.57.167.255");
  et_open_config_addbroadcast(openconfig,"129.57.176.255");


  /*sergey: increase tcp buffersize 
  {
	int rBufSize;
    int sBufSize;
    int noDelay;

	et_open_config_gettcp(openconfig, &rBufSize, &sBufSize, &noDelay);
    printf("default rBufSize=%d, sBufSize=%d, noDelay=%d\n", rBufSize, sBufSize, noDelay);

	rBufSize = 65000;
    sBufSize = 65000;
    noDelay = 0;
	et_open_config_settcp(openconfig, rBufSize, sBufSize, noDelay);

	et_open_config_gettcp(openconfig, &rBufSize, &sBufSize, &noDelay);
    printf("et2et: set rBufSize=%d, sBufSize=%d, noDelay=%d\n", rBufSize, sBufSize, noDelay);
  }
*/







  if(et_open(&id_from, from_et, openconfig) != ET_OK)
  {
    printf("%s: et_open 'from' problems\n",argv[0]);
    exit(1);
  }


  /******************/
  /* open remote ET */
  /******************/

  /* clon00: 129.57.167.5 */
  /* clon00-daq1: 129.57.68.1 */
  /*et_open_config_sethost(openconfig, "clon00-daq1");*/
  /*et_open_config_sethost(openconfig, "clon00-daq1");*/
  /*!!!!!!!!!!!!!!!! ET_HOST_ANYWHERE works, to_node does not !!!!!!!!*/
  et_open_config_sethost(openconfig, to_node/*ET_HOST_ANYWHERE*/);
	/*et_open_config_sethost(openconfig, "129.57.167.4");*/
  et_open_config_gethost(openconfig, ch);
  printf("remote host >%s<\n",ch);

  if(argc == 5 && !strcmp(option,"direct"))
  {
    printf("Use 'direct' connection: you can connect to any subnet,\n");
    printf("but only one ET system allowed on remote machine\n");
    et_open_config_setcast(openconfig, ET_DIRECT);
  }



  et_open_config_addbroadcast(openconfig,"129.57.167.255");
  et_open_config_addbroadcast(openconfig,"129.57.176.255");


  /*sergey: increase tcp buffersize 
  {
	int rBufSize;
    int sBufSize;
    int noDelay;

	et_open_config_gettcp(openconfig, &rBufSize, &sBufSize, &noDelay);
    printf("default rBufSize=%d, sBufSize=%d, noDelay=%d\n", rBufSize, sBufSize, noDelay);

	rBufSize = 65000;
    sBufSize = 65000;
    noDelay = 0;
	et_open_config_settcp(openconfig, rBufSize, sBufSize, noDelay);

	et_open_config_gettcp(openconfig, &rBufSize, &sBufSize, &noDelay);
    printf("et2et: set rBufSize=%d, sBufSize=%d, noDelay=%d\n", rBufSize, sBufSize, noDelay);
  }
*/



  {
    et_open_config *config = (et_open_config *) openconfig;
    printf("befor et_open: rBufSize=%d, sBufSize=%d, noDelay=%d\n",
      config->tcpSendBufSize,config->tcpRecvBufSize,config->tcpNoDelay);
  }


  if(et_open(&id_to, to_et, openconfig) != ET_OK)
  {
    printf("%s: et_open 'to' problems\n",argv[0]);
    exit(1);
  }
{
  et_id *idto = (et_id *) id_to;
  printf("11111: idto->endian=0x%08x idto->systemendian=0x%08x\n",idto->endian,idto->systemendian);
}

printf("11\n");fflush(stdout);
  /* destroy configuration */
  et_open_config_destroy(openconfig);

  /* init station configuration */
  et_station_config_init(&sconfig);
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig, 1);
  et_station_config_setcue(sconfig, 150);
 
printf("12\n");fflush(stdout);

  /* ET system "all" mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
*/

  /* ET system "on req" mode */
  et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);


  /* ET system "condition" mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_MATCH);
  et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
  et_station_config_setselectwords(sconfig, selections);
  */
  /* new non-blocking "condition" mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_MATCH);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  et_station_config_setselectwords(sconfig, selections);
  */
  /* user's condition, blocking  mode 
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
  */
  /* user's condition, nonblocking mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_USER);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  et_station_config_setselectwords(sconfig, selections);
  et_station_config_setfunction(sconfig, "et_carls_function");
  et_station_config_setlib(sconfig, "/home/timmer/cvs/coda/source/et/src/libet_user.so");
  */
  
  /* set debug level */
  et_system_setdebug(id_from, ET_DEBUG_INFO);
  et_system_setdebug(id_to,   ET_DEBUG_INFO);

  if ((status = et_station_create(id_from, &stat_from, from_station, sconfig)) < ET_OK) {
    if (status == ET_ERROR_EXISTS) {
      /* my_stat contains pointer to existing station */;
      printf("%s: station already exists, will attach to it\n", argv[0]);



      /* get id and attach to existing station (must be created by 'et_start' */
      if((status = et_station_name_to_id(id_from, &stat_from, from_station)) < ET_OK)
      {
        printf("%s: error in station_name_to_id\n", argv[0]);
        goto error;
      }




    }
    else {
      printf("%s: error in station creation\n", argv[0]);
      goto error;
    }
  }

  et_station_config_destroy(sconfig);
printf("13\n");fflush(stdout);





  /* */
  if (et_station_attach(id_from, stat_from, &att_from) < 0) {
    printf("%s: error in station attach\n", argv[0]);
    goto error;
  }
printf("14\n");fflush(stdout);

  if (et_station_attach(id_to, ET_GRANDCENTRAL, &att_to) < 0) {
    printf("%s: error in station attach\n", argv[0]);
    goto error;
  }
printf("15\n");fflush(stdout);

  et_bridge_config_init(&bconfig);



  /*et_bridge_config_setfunc(bconfig, et_bridge_CODAswap);*/


/*
printf("=====================================\n");
printf("=====================================\n");
printf("=====================================\n");
printf("setting swap function 'et_bridge_BOS'\n");
printf("=====================================\n");
printf("=====================================\n");
printf("=====================================\n");
  et_bridge_config_setfunc(bconfig, et_bridge_BOS);
*/



#ifdef CODASSIPC

  ssarg.project    = project;
  ssarg.session    = session;
  ssarg.rate_rec   = 0.0; 
  ssarg.rate_proc  = 0.0; 
  ssarg.nev_rec    = 0;
  ssarg.nev_proc   = 0;
  ssarg.et_ok      = 1; /* 1 if connected, 0 if not; we'll always set it 1 */
  ssarg.force_exit = 0;
  ssarg.last_time  = time(NULL);

  /*  set ipc parameters, connect to server, etc. */
  ipc_set_application(ssarg.project);
  ipc_set_quit_callback(ssipc_quit_callback);
  ipc_set_user_status_poll_callback(ssipc_status_poll_callback);
  ipc_set_control_message_callback(ssipc_control_message_callback);


  /* construct unique name */
  printf("EB >%s<\n",getenv("CLON_EB"));
  printf("ER >%s<\n",getenv("CLON_ER"));
  printf("DAQ >%s<\n",getenv("CLON_DAQ"));
  if( !strcmp(from_node,getenv("CLON_EB")) )
  {
    strcpy(from_node_ssipc,"eb");
    ssarg.data_flow = (char *)"et:ET2ER->et";
  }
  else if( !strcmp(from_node,getenv("CLON_ER")) )
  {
    strcpy(from_node_ssipc,"er");
    ssarg.data_flow = (char *)"et:ET2ET10->et";
  }
  else if( !strcmp(from_node,getenv("CLON_DAQ")) )
  {
    strcpy(from_node_ssipc,"daq");
    ssarg.data_flow = (char *)"et:ET2ETxx->et";
  }
  else
  {
    strcpy(from_node_ssipc,from_node);
    ssarg.data_flow = (char *)"et:ET2ETxx->et";
  }



  /* 'unique_name' must be defined outside; if it is defined
  as 'et2et_' we'll expand it and make it unique, otherwise
  we'll just use whatever predefined */
  if( !strcmp((char *)unique_name,"et2et_") )
  {
    strcpy((char *)uniquename,(char *)unique_name);
    strcat((char *)uniquename,from_node_ssipc);
    strcat((char *)uniquename,(char *)"_");
    strcat((char *)uniquename,to_node);
    strcat((char *)uniquename,(char *)"_");
    strcat((char *)uniquename,(char *)session);
  }
  else
  {
    strcpy((char *)uniquename,(char *)unique_name);
  }

  ssarg.unique_name = uniquename;
  printf("unique name >%s<\n",uniquename);




  /*set unique name*/
  status = ipc_init(ssarg.unique_name,uniquename); /* <=21 char */

  if(status < 0)
  {
    printf("\net_2_et: Unable to connect to server...probably duplicate unique name\n"
	 "   ...check for another %s connected using ipc_info\n\n",ssarg.unique_name);
    exit(1);
  }
  TipcSrvSubjectSetSubscribe((T_STR)"evt_system",TRUE);
  TipcSrvFlush();


  /* start ssipc_thread */
  nevents = 0;
  if(pthread_create(&sid, NULL, ssipc_thread, (void *)&ssarg) != 0)
  {
    printf("et_2_et: pthread_create(0x%08x,...) failure\n",sid);
    return(-1);
  }
  else
  {
    printf("et_2_et: pthread_create(0x%08x,...) done\n",sid);
  }
#endif


  /* infinite loop */
  while(status == ET_OK)
  {
    status = et_events_bridge(id_from, id_to, att_from, att_to,
			      bconfig, NUMEVENTS, &ntransferred);

#ifdef CODASSIPC
    /* fill 'ssarg' structure */
    ssarg.nev_rec   += ntransferred;
    ssarg.nev_proc  += ntransferred;

    pthread_mutex_lock(&mutex_nevents);

    /* just sum here, will normalize to time interval before sending it out */
    nevents += ntransferred;
    /*printf("1: nevents = %d\n",nevents);*/

    pthread_mutex_unlock(&mutex_nevents);

    if(ssarg.force_exit) break;
#endif
  }


#ifdef CODASSIPC
  ipc_close();
#endif

  et_bridge_config_destroy(bconfig);

  et_forcedclose(id_from);
  et_forcedclose(id_to);
  exit(0);

error:
  exit(1);
}


/************************************************************/
/*              separate thread to handle signals           */
static void *
signal_thread (void *arg)
{
  sigset_t   signal_set;
  int        sig_number;
 
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  
  /* Not necessary to clean up as ET system will do it */
  sigwait(&signal_set, &sig_number);
  printf("et_2_et: got a control-C, exiting\n");

#ifdef CODASSIPC
  force_exit = 1; /* tell smartsocket thread to exit */
  while(force_exit) sleep(1); /* wait for smartsocket thread to exit */
#endif

  exit(1);
}





