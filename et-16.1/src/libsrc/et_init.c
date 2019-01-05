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
 *      Routines for initializations & destructions.
 *
 *----------------------------------------------------------------------------*/
 

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "et_private.h"
#include "et_network.h"

/*****************************************************/
/*                ODD BALL ROUTINE                   */
/*****************************************************/
/* This is a general routine that must be used in both regular and
 * remote libraries. Of all the files used in both, this is the best
 * place to put it.
 */
/* Can operating system share pthread mutexes between processes? */
int et_sharedmutex(void) {
    int shared;

#ifdef __APPLE__
  shared = ET_MUTEX_NOSHARE; /* For OS X  5/28/04  D. L. */
#else
  /* Perhaps the operating system can tell us */
  if (sysconf(_SC_THREAD_PROCESS_SHARED) >= 1) {
/*printf("THIS PLATFORM SUPPORTS SHARED MUTEXES ACROSS PROCESSES\n");*/
    shared = ET_MUTEX_SHARE;
  }
  else {
    shared = ET_MUTEX_NOSHARE;
  }
#endif

  return shared;
}

/*****************************************************/
/*                INITIALIZATIONS                    */
/*****************************************************/

void et_init_process(et_system *sys, et_proc_id id)
{
  int i;
  
  sys->proc[id].num          = -1;
  sys->proc[id].nattachments = 0;
  sys->proc[id].et_status    = ET_PROC_ETDEAD;
  sys->proc[id].heartbeat    = 0;
  sys->proc[id].pid          = -1;
  for (i=0 ; i < ET_ATTACHMENTS_MAX; i++) {
    sys->proc[id].att[i]     = -1;
  }
  sys->proc[id].status       = ET_PROC_CLOSED;
}

void et_init_attachment(et_system *sys, et_att_id id)
{
  sys->attach[id].num         = -1;
  sys->attach[id].proc        = -1;
  sys->attach[id].stat        = -1;
  sys->attach[id].blocked     = ET_ATT_UNBLOCKED;
  sys->attach[id].quit        = ET_ATT_CONTINUE;
  sys->attach[id].sleep       = ET_ATT_NOSLEEP;
  sys->attach[id].events_put  = 0ULL;
  sys->attach[id].events_get  = 0ULL;
  sys->attach[id].events_dump = 0ULL;
  sys->attach[id].events_make = 0ULL;
  sys->attach[id].pid         = -1;
  strcpy(sys->attach[id].host, "CANNOT_FIND_HOST");
  *sys->attach[id].interface  = '\0';
  sys->attach[id].status      = ET_ATT_UNUSED;
}

/*****************************************************/
void et_init_station(et_station *ps)
{
  int i;

  /* init all except mutex which is done in et_init_mem_station
   * as it should only be done once.
   */
  ps->num          = -1;
  ps->next         = -1;
  ps->prev         = -1;
  ps->nextparallel = -1;
  ps->prevparallel = -1;
  ps->waslast      =  0;
 *ps->name         = '\0';
  ps->conductor    = ET_THREAD_KEEP;

  ps->config.flow_mode    = ET_STATION_SERIAL;
  ps->config.user_mode    = ET_STATION_USER_MULTI;
  ps->config.restore_mode = ET_STATION_RESTORE_OUT;
  ps->config.block_mode   = ET_STATION_BLOCKING;
  ps->config.select_mode  = ET_STATION_SELECT_ALL;
  ps->config.cue          = ET_STATION_CUE;
  ps->config.prescale     = ET_STATION_PRESCALE;
 *ps->config.fname        = '\0';
 *ps->config.lib          = '\0';
 *ps->config.classs       = '\0';
  for (i=0 ; i< ET_STATION_SELECT_INTS ; i++) {
    ps->config.select[i]  = -1;
  }
  ps->config.init         = ET_STRUCT_OK;

  ps->data.nattachments =  0;
  ps->data.pid_create   = -1;
  ps->data.lib_handle   = NULL;
  for (i=0 ; i < ET_ATTACHMENTS_MAX; i++) {
    ps->data.att[i] = -1;
  }
  
  ps->fix.in.first = NULL;
  ps->fix.in.start = 0;
  ps->fix.in.cnt   = 0;
  ps->fix.in.num   = 0;
  ps->fix.in.call  = ET_FIX_READ;
  ps->fix.in.eventsin = 0ULL;
  
  ps->fix.out.start = 0;
  ps->fix.out.cnt   = 0;
  ps->fix.out.num   = 0;
  
  et_init_llist(&ps->list_in);
  et_init_llist(&ps->list_out);
  
  /* This is done last so error recovery is easier.
   * We'll know settings are "unused" consistent. */
  ps->data.status = ET_STATION_UNUSED;
}

/*****************************************************/
void et_init_llist(et_list *pl)
{
  pl->cnt                = 0;
  pl->lasthigh           = 0;
  pl->events_try         = 0ULL;
  pl->events_in          = 0ULL;
  pl->events_out         = 0ULL;
  pl->firstevent         = NULL;
  pl->lastevent          = NULL;
}

/*****************************************************/
/* 
 * Called directly by et_init_mem_event,
 * etr_event(s)_new(_group). Full initialization.
 */
void et_init_event(et_event *pe)
{ int i;
  pe->owner      = ET_SYS;
  pe->next       = NULL;
  pe->tempdata   = NULL;
  pe->pdata      = NULL;
  pe->length     = 0L;
  pe->memsize    = 0L;
  pe->priority   = ET_LOW;
  pe->temp       = ET_EVENT_NORMAL;
  pe->age        = ET_EVENT_NEW;
  pe->datastatus = ET_DATA_OK;
  pe->byteorder  = 0x04030201;
  pe->group      = 0;
  pe->pointer    = 0L;
  pe->modify     = 0;
  for (i=0 ; i < ET_STATION_SELECT_INTS ; i++) {
    pe->control[i] = 0;
  }
}

/*****************************************************/
/* Doesn't change owner or group number. Only called directly by
 * et_event_make, et_event(s)_new(_group).
 */

void et_init_event_(et_event *pe)
{
  int group=pe->group, owner=pe->owner;
  et_init_event(pe);
  pe->owner = owner;
  pe->group = group;
}

/*****************************************************/
void et_init_histogram(et_id *id)
{
  int  i, *p = id->histogram;
  
  for (i=0; i < id->nevents + 1; i++,p++) {
    *p = 0;
  }
}

/*****************************************************/
void et_init_stats_att(et_system *sys, et_att_id id)
{  
  sys->attach[id].events_put  = 0ULL;
  sys->attach[id].events_get  = 0ULL;
  sys->attach[id].events_dump = 0ULL;
  sys->attach[id].events_make = 0ULL;
}

/*****************************************************/
void et_init_stats_allatts(et_system *sys)
{  
  int i;
  
  for (i=0; i < ET_ATTACHMENTS_MAX; i++) {
    et_init_stats_att(sys , (et_att_id) i);
  }
}

/*****************************************************/
void et_init_stats_station(et_station *ps)
{  
  ps->list_in.events_try  = 0ULL;
  ps->list_in.events_out  = 0ULL;
  ps->list_in.events_in   = 0ULL;
  ps->list_out.events_try = 0ULL;
  ps->list_out.events_out = 0ULL;
  ps->list_out.events_in  = 0ULL;
}

/*****************************************************/
void et_init_stats_allstations(et_id *id)
{  
  int i;
  et_station *ps = id->grandcentral;
  
  for (i=0; i < id->sys->config.nstations; i++) {
    et_init_stats_station(ps);
    ps++;
  }
}

/*****************************************************/
void et_init_stats_all(et_id *id)
{
  et_init_stats_allatts(id->sys);
  et_init_stats_allstations(id);
  et_init_histogram(id);
}

/*****************************************************/
/*             ET SYSTEM ID                          */
/*****************************************************/

int et_id_init(et_sys_id *id)
{
  et_id *etid;
  
  etid = (et_id *) malloc(sizeof(et_id));
  if (etid == NULL) {
    et_logmsg("ERROR", "et_id_init, cannot allocate memory\n");
    return ET_ERROR;
  }
  
  etid->lang         = ET_LANG_C;
  etid->alive        = 0;
  etid->closed       = 0;
#ifdef _LP64
  etid->bit64        = ET_BIT64_MASK;
#else
  etid->bit64        = 0;
#endif
  etid->proc         = ET_SYS;
  etid->race         = 0;
  etid->cleanup      = 0;
  etid->debug        = ET_DEBUG_ERROR;
  etid->nevents      = 0;
  etid->group        = 0;
  etid->version      = ET_VERSION;
  etid->nselects     = ET_STATION_SELECT_INTS;
  etid->memsize      = 0;
  etid->esize        = 0;
  etid->offset       = 0;
  
  /* can operating system share pthread mutexes between processes? */
  etid->share        = et_sharedmutex();
  
  /* pointers */
  etid->pmap         = NULL;
  etid->sys          = NULL;
  etid->stats        = NULL;
  etid->events       = NULL;
  etid->data         = NULL;
  etid->grandcentral = NULL;
  
  /* remote stuff */
  etid->locality     = ET_LOCAL;
  etid->sockfd       = -1;
  etid->iov_max      = ET_IOV_MAX;
  etid->port         = 0;
  *etid->ethost      = '\0';
  *etid->localAddr   = '\0';
  /* find endian value */
  if (etNetLocalByteOrder(&etid->endian) != ET_OK) {
    et_logmsg("SEVERE", "et_id_init, strange byteorder\n");
    exit(1);
  }
  /* default is user is on same host as system */
  etid->systemendian = etid->endian;
  
  /* initialize mutex for thread-safe TCP */
  pthread_mutex_init(&etid->mutex, NULL);
  
#ifndef NO_RW_LOCK

#ifndef linux
      /* Up to & including solaris 10 / MacOS 10.7, pthread_rwlockattr_setkind_np is not supported */
      pthread_rwlock_init(&etid->sharedMemlock, NULL);
#else
  {
      /* Give preference to writer lock (to et_close & et_station_detach).
       * Default is to give preference to the reader lock. */
      pthread_rwlockattr_t attr;
      pthread_rwlockattr_init(&attr);
      pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
      /* Isnitialize mutex for protecting shared mem from close() */
      pthread_rwlock_init(&etid->sharedMemlock, &attr);
      pthread_rwlockattr_destroy(&attr);
  }
#endif

#endif
  
  /* sign that we're done with init */
  etid->init = ET_STRUCT_OK;
  
  *id = (et_sys_id) etid;
  return ET_OK;
}

/*****************************************************/
void et_id_destroy(et_sys_id id)
{
  et_id *etid = (et_id *) id;
  
  if (etid != NULL) {
    pthread_mutex_destroy(&etid->mutex);
#ifndef NO_RW_LOCK
    pthread_rwlock_destroy(&etid->sharedMemlock);
#endif
    free(etid);
  }
  
  etid = NULL;
  return;
}

