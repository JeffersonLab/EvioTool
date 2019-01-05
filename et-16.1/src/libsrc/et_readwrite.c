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
 *      Routines to read & write events in shared memory, deal with mutexes,
 *        and repair messed up event lists in shared memory.
 *
 *----------------------------------------------------------------------------*/
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>

#include "et_private.h"

/* prototypes */
#if 0
static int et_llist_put(et_id *id, et_list *pl, et_event **pe, int num);
#endif
static int et_repair_outputlist(et_id *id, et_stat_id stat_id);
static int et_repair_inputlist(et_id *id, et_stat_id stat_id);
static int et_repair_gcinputlist(et_id *id);
static int et_restore(et_id *id, et_list *pl, et_event **pe,
                      int num, int *numwritten);
static int et_restore_in(et_id *id, et_station *ps, et_event **pe,
                         int num, int *numwritten);

/*******************************************************/  
/* Locking & Unlocking stations, system & linked lists */
/*******************************************************/

void et_station_lock(et_system *sys)
{
  int status;
  
  status = pthread_mutex_lock(&sys->stat_mutex);
  if (status != 0) {
    err_abort(status, "Failed station lock");
  }
}

void et_station_unlock(et_system *sys)
{
  int status;
  
  status = pthread_mutex_unlock(&sys->stat_mutex);
  if (status != 0) {
    err_abort(status, "Failed station unlock");
  }
}
/*******************************************************/  

void et_llist_lock(et_list *pl)
{
  int status;
  
  status = pthread_mutex_lock(&pl->mutex);
  if (status != 0) {
    err_abort(status, "Failed llist lock");
  }
}

void et_llist_unlock(et_list *pl)
{
  int status;
  
  status = pthread_mutex_unlock(&pl->mutex);
  if (status != 0) {
    err_abort(status, "Failed llist unlock");
  }
}

/*******************************************************/  
void et_system_lock(et_system *sys)
{
  int status;
  
  status = pthread_mutex_lock(&sys->mutex);
  if (status != 0) {
    err_abort(status, "Failed system lock");
  }
}

void et_system_unlock(et_system *sys)
{
  int status;
  
  status = pthread_mutex_unlock(&sys->mutex);
  if (status != 0) {
    err_abort(status, "Failed system unlock");
  }
}
/*******************************************************/  

void et_transfer_lock(et_station *ps)
{
  int status;
  
  status = pthread_mutex_lock(&ps->mutex);
  if (status != 0) {
    err_abort(status, "Failed transfer lock");
  }
}

void et_transfer_unlock(et_station *ps)
{
  int status;
  
  status = pthread_mutex_unlock(&ps->mutex);
  if (status != 0) {
    err_abort(status, "Failed transfer unlock");
  }
}
/*******************************************************/  

void et_transfer_lock_all(et_id *id)
{
  int i;
  et_station *ps = id->stats;
  
  for (i=0; i < id->sys->config.nstations; i++) {
    et_transfer_lock(ps);
    ps++;
  }
}

void et_transfer_unlock_all(et_id *id)
{
  int i;
  et_station *ps = id->stats;
  
  for (i=0; i < id->sys->config.nstations; i++) {
    et_transfer_unlock(ps);
    ps++;
  }
}

/*******************************************************/  
void et_tcp_lock(et_id *id)
{
    int status;
  
    status = pthread_mutex_lock(&id->mutex);
    if (status != 0) {
        err_abort(status, "Failed tcp lock");
    }
}

void et_tcp_unlock(et_id *id)
{
    int status;
  
    status = pthread_mutex_unlock(&id->mutex);
    if (status != 0) {
        err_abort(status, "Failed tcp unlock");
    }
}
/*******************************************************/  

void et_memRead_lock(et_id *id)
{
#ifndef NO_RW_LOCK
   int status;
  
    status = pthread_rwlock_rdlock(&id->sharedMemlock);
    if (status != 0) {
        err_abort(status, "Failed mem read lock");
    }
#endif
}

void et_memWrite_lock(et_id *id)
{
#ifndef NO_RW_LOCK
    int status;
  
    status = pthread_rwlock_wrlock(&id->sharedMemlock);
    if (status != 0) {
        err_abort(status, "Failed mem write lock");
    }
#endif
}

void et_mem_unlock(et_id *id)
{
#ifndef NO_RW_LOCK
    int status;
  
    status = pthread_rwlock_unlock(&id->sharedMemlock);
    if (status != 0) {
        err_abort(status, "Failed mem unlock");
    }
#endif
}

/*****************************************************/
int et_mutex_locked(pthread_mutex_t *pmutex)
/* Keep trying to lock it over 2 second. */
{
  int status, i;
  struct timespec timeout;
  
  timeout.tv_sec  = 0;
  timeout.tv_nsec = 50000000; /* 0.05sec = 20Hz */
  
  for (i=0; i < 40 ; i++) {
    status = pthread_mutex_trylock(pmutex);
    if (status == EBUSY) {
      nanosleep(&timeout, NULL);
      continue;
    } 
    else if (status !=0) {
      err_abort(status, "Mutex test trylock");
    }
    
    status = pthread_mutex_unlock(pmutex);
    if (status != 0) {
      err_abort(status, "Mutex test unlock");
    }
    /*printf("  UNLOCKED\n");*/
    return ET_MUTEX_UNLOCKED;
  }
  
  /*printf("  LOCKED\n");*/
  return ET_MUTEX_LOCKED;
}

/*****************************************************
 * The next 2 functions are only called by the ET system
 * and can use pointers stored in mapped memory.
 *
 * See the note with "et_fix_mutexes" as to the repairing
 * of locked mutexes by unlocking or reinitialization.
 *****************************************************/
int et_repair_station(et_id *id, et_stat_id stat_id)
{
  int status;
  et_list *pl;
  et_station *ps = id->stats + stat_id;
#ifdef MUTEX_INIT
  pthread_mutexattr_t mattr;

  if (id->share == ET_MUTEX_SHARE) {
    /* set attribute for mutex multiprocess sharing */
    status = pthread_mutexattr_init(&mattr);
    if(status != 0) {
      err_abort(status, "et_init_mem_sys");
    }

    status = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    if(status != 0) {
      err_abort(status, "et_init_mem_sys");
    }
  }   
#endif

  /*
   * First try input list. If it's locked, then the
   * read operation died in the middle of its execution
   * and we need to repair the input list.
   */
  pl = &ps->list_in;
  if (et_mutex_locked(&pl->mutex) == ET_MUTEX_LOCKED) {
    if (id->debug >= ET_DEBUG_WARN) {
      et_logmsg("WARN", "et_repair_station, input list locked\n");
    }
    status = et_repair_inputlist(id, stat_id);
    if (status == ET_ERROR) {
      return status;
    }
    et_llist_unlock(pl);
#ifdef MUTEX_INIT
    if (id->share == ET_MUTEX_SHARE) {
      pthread_mutex_init(&pl->mutex, &mattr);
    }
    else {
      pthread_mutex_init(&pl->mutex, NULL);
    }
#endif
    return ET_OK;
  }
 
  /* next try output list */
  pl = &ps->list_out;
  if (et_mutex_locked(&pl->mutex) == ET_MUTEX_LOCKED) {
    if (id->debug >= ET_DEBUG_WARN) {
      et_logmsg("WARN", "et_repair_station, output list locked\n");
    }
    status = et_repair_outputlist(id, stat_id);
    if (status == ET_ERROR) {
      return status;
    }
    et_llist_unlock(pl);
#ifdef MUTEX_INIT
    if (id->share == ET_MUTEX_SHARE) {
      pthread_mutex_init(&pl->mutex, &mattr);
    }
    else {
      pthread_mutex_init(&pl->mutex, NULL);
    }
#endif
   return ET_OK;
  }
  
#ifdef MUTEX_INIT
  if (id->share == ET_MUTEX_SHARE) {
    /* Release resources */
    status = pthread_mutexattr_destroy(&mattr);
    if(status != 0) {
      err_abort(status, "Mutex attr destroy");
    }
  }
#endif

  return ET_OK;
}

/*****************************************************
 * Repair GrandCentral station from either an et_event(s)_dump
 * or et_event(s)_new operation. Repairing a "dump" is a bit
 * backward from the normal station repair, in that normally
 * you read from the input list and write to the output list.
 * In a dump, you write to the input list; hence the need
 * for a slightly different repair function.
 *****************************************************/
int et_repair_gc(et_id *id)
{
  int status;
  et_list *pl;
  et_station *ps = id->stats;
#ifdef MUTEX_INIT
  pthread_mutexattr_t mattr;

  if (id->share == ET_MUTEX_SHARE) {
    /* set attribute for mutex multiprocess sharing */
    status = pthread_mutexattr_init(&mattr);
    if(status != 0) {
      err_abort(status, "et_init_mem_sys");
    }

    status = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    if(status != 0) {
      err_abort(status, "et_init_mem_sys");
    }
  } 
#endif

  /* if it's locked, then the DUMP/WRITE died in the middle */
  pl = &ps->list_in;
  if (et_mutex_locked(&pl->mutex) == ET_MUTEX_LOCKED) {
    if (id->debug >= ET_DEBUG_WARN) {
      et_logmsg("WARN", "et_repair_gc, input list locked\n");
    }
    
    /* which is it, dead from dumping events or getting new ones? */
    if (ps->fix.in.call == ET_FIX_DUMP) {
      status = et_repair_gcinputlist(id);
      if (id->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_repair_gc, fix GC's input list from a call to et_event(s)_dump\n");
      }
    }
    else {
      status = et_repair_inputlist(id, ET_GRANDCENTRAL);
      if (id->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_repair_gc, fix GC's input list from a call to et_event(s)_new\n");
      }
    }
    
    if (status == ET_ERROR) {
      return status;
    }
    et_llist_unlock(pl);
#ifdef MUTEX_INIT
    if (id->share == ET_MUTEX_SHARE) {
      pthread_mutex_init(&pl->mutex, &mattr);
    }
    else {
      pthread_mutex_init(&pl->mutex, NULL);
    }
#endif
    return ET_OK;
  }
 
#ifdef MUTEX_INIT
  if (id->share == ET_MUTEX_SHARE) {
    /* Release resources */
    status = pthread_mutexattr_destroy(&mattr);
    if(status != 0) {
      err_abort(status, "Mutex attr destroy");
    }
  }
#endif

  return ET_OK;
}

/*****************************************************
 * The next 3 functions, et_repair_outputlist,
 * et_repair_inputlist, and et_repair_gcinputlist are
 * carefully written to work together with the 
 * et_station_read/write/dump functions.
 * Any changes to one set may require changes in the
 * other. The et_station_read/write functions are designed
 * so that if they crash ANYWHERE, the repair functions
 * will be able to return a station to a fully operational,
 * self-consistent state. The repair routines are only
 * run when the read/write routines die with a mutex
 * locked, so no need to worry about mutexes.
 *
 * These functions are only called by the ET system
 * and can use pointers stored in mapped memory.
 *****************************************************/
static int et_repair_outputlist(et_id *id, et_stat_id stat_id)
{
  int i, status, cnt, num, start, error=ET_ERROR, count=0, lasthigh=0;
  et_event   *pe, *pe_last_USR;
  et_station *ps = id->stats + stat_id;
  et_list    *pl = &ps->list_out;
  
  start = ps->fix.out.start;
  cnt   = ps->fix.out.cnt;
  num   = ps->fix.out.num;

  if (id->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_repair_outputlist, cnt = %d, num = %d\n", cnt, num);
    et_logmsg("INFO", "et_repair_outputlist, firstevent = %p, lastevent = %p\n", pl->firstevent, pl->lastevent);
  }
  
  if (start == 0) {
    /* Either nothing or everything was written to output list */  
    if (id->debug >= ET_DEBUG_INFO) {
      if (pl->cnt == 0 || pl->cnt == cnt) {
        et_logmsg("INFO", "et_repair_outputlist, nothing written, nothing to repair\n");
      }
      else {
        et_logmsg("INFO", "et_repair_outputlist, everything or nothing written, no repairs\n");
      }
    }
    return ET_OK;
  }
  else if (pl->cnt == cnt + num) {
    /* everything was completely written to output list */  
    ps->fix.out.start = 0;
    if (id->debug >= ET_DEBUG_INFO) {
      et_logmsg("INFO", "et_repair_outputlist, nothing to repair, write complete\n");
    }
    return ET_OK;
  }
  /* else if nothing written to input list ... */  
  else if ((pl->cnt == 0) && (pl->firstevent == NULL)) {
    ps->fix.out.start = 0;
    if (id->debug >= ET_DEBUG_INFO) {
      et_logmsg("INFO", "et_repair_outputlist, nothing written, nothing to repair\n");
    }
    return ET_OK;
  }
  else if ((pl->cnt == 0) && (pl->firstevent != NULL)) {
    /* make sure the very first event written is also last */
    pl->lastevent = pl->firstevent;
  }
  
  pe = ET_PEVENT2USR(pl->firstevent, id->offset);
  pe_last_USR = ET_PEVENT2USR(pl->lastevent, id->offset);

  /*
   * The last event to be written may have only partially completed
   * the process. Make sure it's owner is set, the list's count is
   * right, and record the number of high priority events.
   */
  for (i=0; i < cnt+num ; i++) {
/*printf("et_repair_outputlist: pe = %p, owner = %d, pri = %d, i = %d\n", pe, pe->owner, pe->priority, i);*/
    pe->owner = -1;
    count++;
    if (pe->priority == ET_HIGH) {
      lasthigh++;
    }
    if (pe == pe_last_USR) {
/*printf("et_repair_outputlist: last pe = %p\n", pe);*/
      error = ET_OK;
      break;
    }
    pe = ET_PEVENT2USR(pe->next, id->offset);
  }
  
  pl->cnt       = count;
  pl->lasthigh  = lasthigh;
  ps->fix.out.start = 0;
  
  /* If the list is repaired and a bunch of events are still in it,
   * then we must signal the conductor to move them as they may get
   * stuck in certain circumstances.
   */
  status = pthread_cond_signal(&pl->cread);
  if (status != 0) {
    err_abort(status, "signal event here");
  }
  
  if (id->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_repair_outputlist, pl->cnt = %d, pl->lasthigh = %d\n", pl->cnt, pl->lasthigh);
  }

  return error;  
}

/*****************************************************/
static int et_repair_inputlist(et_id *id, et_stat_id stat_id)
{
  int        cnt, num, count=0;
  et_event   *pe, *first, *pe_last_USR;
  et_station *ps = id->stats + stat_id;
  et_list    *pl = &ps->list_in;

  first = ps->fix.in.first; /* ptr to event in ET process' space */
  cnt   = ps->fix.in.cnt;
  num   = ps->fix.in.num;
 
  if (first == NULL) {
    /* Either nothing or everything was read from input list */  
    if (id->debug >= ET_DEBUG_INFO) {
      if (pl->cnt == cnt) {
        et_logmsg("INFO", "et_repair_inputlist, nothing written, nothing to repair\n");
      }
      else if (pl->cnt == 0) {
        et_logmsg("INFO", "et_repair_inputlist, everything written, nothing to repair\n");
      }
      else {
        et_logmsg("INFO", "et_repair_inputlist, everything or nothing written, no repairs\n");
      }
    }
    return ET_OK;
  }
  else if (pl->cnt == cnt - num) {
    /* everything was completely read from input list */  
    ps->fix.in.first = NULL;
    if (id->debug >= ET_DEBUG_INFO) {
      et_logmsg("INFO", "et_repair_inputlist, nothing to repair, write complete\n");
    }
    return ET_OK;
  }

  pl->firstevent = first;
  pe = ET_PEVENT2USR(first, id->offset);
  pe_last_USR = ET_PEVENT2USR(pl->lastevent, id->offset);

  do {
    count++;
    pe->owner = ET_SYS;
    if (pe == pe_last_USR) {
      break;
    }
    pe = ET_PEVENT2USR(pe->next, id->offset);
  } while (1);
  
  if (id->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_repair_inputlist, initial cnt = %d, new count = %d\n", cnt, count);
  }
  
  pl->cnt = count;
  ps->fix.in.first = NULL;
  return ET_OK;  
}

/*****************************************************/
static int et_repair_gcinputlist(et_id *id)
{
  int i, cnt, num, start, error=ET_ERROR, count=0;
  et_event   *pe, *pe_last_USR;
  et_station *ps = id->stats;
  et_list    *pl = &ps->list_in;
  uint64_t   eventsin;
  
  start    = ps->fix.in.start;
  cnt      = ps->fix.in.cnt;
  num      = ps->fix.in.num;
  eventsin = ps->fix.in.eventsin;
  
  if (id->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_repair_gcinputlist, fix.cnt = %d, fix.num = %d\n", cnt, num);
    et_logmsg("INFO", "et_repair_gcinputlist, firstevent = %p, lastevent = %p\n", pl->firstevent, pl->lastevent);
  }
  
  /* high and low priority do NOT matter in GC input list */
  
  if (start == 0) {
    /* Either nothing or everything was written to input list */  
    if (id->debug >= ET_DEBUG_INFO) {
      if (pl->cnt == 0 || pl->cnt == cnt) {
        et_logmsg("INFO", "et_repair_gcinputlist, nothing written, nothing to repair\n");
      }
      else {
        et_logmsg("INFO", "et_repair_gcinputlist, everything or nothing written, no repairs\n");
      }
    }
    return ET_OK;
  }
  /* else if everything was completely written to input list ... */  
  else if (pl->cnt == cnt + num) {
    ps->fix.in.start = 0;
    if (id->debug >= ET_DEBUG_INFO) {
      et_logmsg("INFO", "et_repair_gcinputlist, write complete, nothing to repair\n");
    }
    return ET_OK;
  }
  /* else if nothing written to input list ... */  
  else if ((pl->cnt == 0) && (pl->firstevent == NULL)) {
    ps->fix.in.start = 0;
    if (id->debug >= ET_DEBUG_INFO) {
      et_logmsg("INFO", "et_repair_gcinputlist, nothing written, nothing to repair\n");
    }
    return ET_OK;
  }
  else if ((pl->cnt == 0) && (pl->firstevent != NULL)) {
    /* make sure the very first event written is also last */
    pl->lastevent = pl->firstevent;
  }
  
  pe = ET_PEVENT2USR(pl->firstevent, id->offset);
  pe_last_USR = ET_PEVENT2USR(pl->lastevent, id->offset);

  /*
   * The last event to be written may have only partially completed
   * the process. Make sure owner is set, the list's count is right.
   */
  for (i=0; i < cnt+num ; i++) {
    pe->owner = ET_SYS;
    count++;
    if (pe == pe_last_USR) {
      error = ET_OK;
      break;
    }
    pe = ET_PEVENT2USR(pe->next, id->offset);
  }
  
  pl->cnt = count;
  /* if pl->events_in did not get updated, do it now */
  if (eventsin == pl->events_in) {
    pl->events_in = pl->events_in + count - cnt;
  }
  ps->fix.in.start = 0;
  
  if (id->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_repair_gcinputlist, pl->cnt = %d\n", pl->cnt);
  }

  return error;  
}

/*****************************************************
 * et_station_(n)write routines are for writing
 * events into a station's output list.
 * et_station_(n)read routines are for reading
 * events from a station's intput list.
 *****************************************************/
int et_station_write(et_id *id, et_stat_id stat_id, et_event *pe)
{
  et_station *ps = id->stats + stat_id;
  et_list *pl = &ps->list_out;
  et_event *pe_ET, *pe_last_USR=NULL, *pe_lasthigh=NULL;
  int i, status;
  
  et_llist_lock(pl);

  if (pl->cnt >= id->sys->config.nevents) {
    et_llist_unlock(pl); 
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_station_write, output list cnt is too high (%d)\n", pl->cnt);
    }
    return ET_ERROR;
  }
  
  /* ptr to event represented in ET's process space */
  pe_ET = ET_PEVENT2ET(pe, id->offset);
  
  /* info for fixing broken output list */
  if (pl->cnt == 0) {
    pl->firstevent = NULL;
  }
  ps->fix.out.cnt = pl->cnt;
  ps->fix.out.num = 1;
  ps->fix.out.start++;

  /* if no events in list ... */
  if (pl->cnt == 0) {
    pl->firstevent = pe_ET;
    pl->lastevent  = pe_ET;
    if (pe->priority == ET_HIGH) {
      /* pl->lasthigh was set to 0 by conductor read (like pl->cnt) */
      pl->lasthigh = 1;
    }
  }
  /* if adding low priority item to list of 1 or more ... */
  else if (pe->priority == ET_LOW) {
     /* last event in user's process space */
     pe_last_USR = ET_PEVENT2USR(pl->lastevent, id->offset);
     pe_last_USR->next = pe_ET;
     pl->lastevent = pe_ET;
  }
  /* if adding high priority item to list of 1 or more ... */
  else {
    /* if high priority items exist, find last high pri event */
    if (pl->lasthigh != 0) {
      pe_lasthigh = ET_PEVENT2USR(pl->firstevent, id->offset);
      for (i=1; i < pl->lasthigh ; i++) {
        pe_lasthigh = ET_PEVENT2USR(pe_lasthigh->next, id->offset);
      }
      /* last event in user's process space */
      pe_last_USR = ET_PEVENT2USR(pl->lastevent, id->offset);
    }
    /* if no high priority items exist ... */
    if (pe_lasthigh == NULL) {
      pe->next = pl->firstevent;
      pl->firstevent = pe_ET;
    }
    /* if only high priority items exist ... */
    else if (pe_lasthigh == pe_last_USR) {
      pe_last_USR->next = pe_ET;
      pl->lastevent = pe_ET;
    }
    /* if high & low priority items exist ... */
    else {
      pe->next = pe_lasthigh->next;
      pe_lasthigh->next = pe_ET;
    }
    pl->lasthigh++;
  }
  
  pe->owner = ET_SYS;
  pl->cnt++;
/* printf("stw, pe=%p, pl=%p, cnt=%d\n", pe, pl, pl->cnt);*/
  ps->fix.out.start--;
  et_llist_unlock(pl);

  /* signal conductor thread that new event is here */
  status = pthread_cond_signal(&pl->cread);
  if (status != 0) {
    err_abort(status, "signal event here");
  }

  return ET_OK;
}


/*****************************************************/
int et_station_read(et_id *id, et_stat_id stat_id, et_event **pe, int mode, et_att_id att, struct timespec *time)
/*
 * Reads the next entry from the station's input linked list.
 * Return code ET_ERROR        : error
 *             ET_ERROR_TIMEOUT: timeout
 *             ET_ERROR_EMPTY  : no events in list
 *             ET_ERROR_BUSY   : async, inputlist is busy
 *             ET_ERROR_WAKEUP : wakeup and return
 *             ET_OK           : success
 */
{
  et_system  *sys = id->sys;
  et_station *ps  = id->stats + stat_id;
  et_list    *pl  = &ps->list_in;
  int status;
  
  if (mode == ET_SLEEP) {
    et_llist_lock(pl);
    /* if getting new event(s), histogram # of events in input list */
    if (stat_id == ET_GRANDCENTRAL) {
      *(id->histogram + pl->cnt) += 1 ;
    }
    while (pl->cnt < 1) {
      sys->attach[att].blocked = ET_ATT_BLOCKED;
      status = pthread_cond_wait(&pl->cread, &pl->mutex);
      sys->attach[att].blocked = ET_ATT_UNBLOCKED;
      if (status != 0) {
        err_abort(status, "Failed llist read wait");
      }
      if (sys->attach[att].quit == ET_ATT_QUIT) {
        if (id->debug >= ET_DEBUG_WARN) {
          et_logmsg("WARN", "et_station_read, quitting\n");
        }
        et_llist_unlock(pl);
        sys->attach[att].quit = ET_ATT_CONTINUE;
        return ET_ERROR_WAKEUP;
      }
    }
  }
  else if (mode == ET_TIMED) {
    et_llist_lock(pl);
    if (stat_id == ET_GRANDCENTRAL) {
      *(id->histogram + pl->cnt) += 1 ;
    }
    while (pl->cnt < 1) {
      sys->attach[att].blocked = ET_ATT_BLOCKED;
      status = pthread_cond_timedwait(&pl->cread, &pl->mutex, time);
      sys->attach[att].blocked = ET_ATT_UNBLOCKED;
      if (status == ETIMEDOUT) {
        et_llist_unlock(pl);
        return ET_ERROR_TIMEOUT;
      }
      else if (status != 0) {
        /* Most likely a bad value for the time.
         * Don't abort, simply return an error.
         */
        if (id->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "et_station_read, pthread_cond_timedwait error\n");
        }
        return ET_ERROR;
      }
      if (sys->attach[att].quit == ET_ATT_QUIT) {
        if (id->debug >= ET_DEBUG_WARN) {
          et_logmsg("WARN", "et_station_read, quitting\n");
        }
        et_llist_unlock(pl);
        sys->attach[att].quit = ET_ATT_CONTINUE;
        return ET_ERROR_WAKEUP;
      }
    }
  }
  else if (mode == ET_ASYNC) {
    status = pthread_mutex_trylock(&pl->mutex);
    if (status == EBUSY) {
      return ET_ERROR_BUSY;
    }
    else if (status !=0) {
      err_abort(status, "Failed llist trylock");
    }
    if (sys->attach[att].quit == ET_ATT_QUIT) {
      if (id->debug >= ET_DEBUG_WARN) {
        et_logmsg("WARN", "et_station_read, quitting\n");
      }
      et_llist_unlock(pl);
      sys->attach[att].quit = ET_ATT_CONTINUE;
      return ET_ERROR_WAKEUP;
    }
    if (stat_id == ET_GRANDCENTRAL) {
      *(id->histogram + pl->cnt) += 1 ;
    }
    if (pl->cnt < 1) {
      et_llist_unlock(pl);
      return ET_ERROR_EMPTY;
    }
  }
  else {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_read, bad mode argument\n");
    }
    return ET_ERROR;
  }
  
  /* info for fixing broken input list */
  ps->fix.in.cnt   = pl->cnt;
  ps->fix.in.num   = 1;
  ps->fix.in.call  = ET_FIX_READ;
  /*
   * pl->firstevent cannot be NULL since we can only
   * be reading if there are events in the list.
   */
  ps->fix.in.first = pl->firstevent;
  
  *pe = ET_PEVENT2USR(pl->firstevent, id->offset);
/*printf("et_station_read: pe = %p, pl->firstevent = %p\n", *pe,  pl->firstevent);*/
  pl->firstevent = (*pe)->next;
  (*pe)->owner = att;
  pl->cnt--;
  ps->fix.in.first = NULL;
 
  et_llist_unlock(pl);
  return ET_OK;
}

/*************************************************************************/
int et_station_nwrite(et_id *id, et_stat_id stat_id, et_event *pe[], int num)
{
  et_station *ps = id->stats + stat_id;
  et_list    *pl = &ps->list_out;
  et_event *pe_ET, *pe_last_USR=NULL, *pe_lasthigh=NULL;
  int i, status, num_in=0;
  
  if (num <= 0) {
    return ET_OK;
  }
  
  et_llist_lock(pl);

  if (num > (id->sys->config.nevents - pl->cnt)) {
    et_llist_unlock(pl);
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_station_nwrite, output list cnt is too high (%d)\n", pl->cnt);
    }
    return ET_ERROR;
  }

  /* info for fixing broken output list */
  if (pl->cnt == 0) {
    pl->firstevent = NULL;
  }
  ps->fix.out.cnt = pl->cnt;
  ps->fix.out.num = num;
  ps->fix.out.start++;
  
  if (pl->cnt == 0) {
    /* ptr to event represented in ET's process space */
    pe_ET = ET_PEVENT2ET(pe[0], id->offset);
    pl->firstevent = pe_ET;
    pl->lastevent  = pe_ET;
/*printf("et_station_nwrite: pe[0] = %p, pe[0]_ET = %p\n", pe[0], pe_ET);*/
    pe[0]->owner   = ET_SYS;
    if (pe[0]->priority == ET_HIGH) {
      /* pl->lasthigh was set to 0 by conductor read */
      pe_lasthigh  = pe[0];
      pl->lasthigh = 1;
    }
    pl->cnt++;
    num_in++;
  }
  else if (pl->lasthigh != 0) {
    /* find last ET_HIGH pri event */
    pe_lasthigh = ET_PEVENT2USR(pl->firstevent, id->offset);
    for (i=1; i < pl->lasthigh ; i++) {
      pe_lasthigh = ET_PEVENT2USR(pe_lasthigh->next, id->offset);
    }
  }
  pe_last_USR = ET_PEVENT2USR(pl->lastevent, id->offset);
  
  for (i=num_in; i < num ; i++) {
    pe_ET = ET_PEVENT2ET(pe[i], id->offset);
    
    /* if ET_LOW pri event, add to end ... */
    if (pe[i]->priority == ET_LOW) {
      pe_last_USR->next = pe_ET;
      pl->lastevent = pe_ET;
      pe_last_USR = pe[i];
/*printf("et_station_nwrite: pe[i] = %p, pe[i]_ET = %p\n", pe[i], pe_ET);*/
    }
    /* if ET_HIGH pri event ... */
    else {
      /* if no high priority items exist ... */
      if (pe_lasthigh == NULL) {
        pe[i]->next    = pl->firstevent;
        pl->firstevent = pe_ET;
      }
      /* if only high priority items exist ... */
      else if (pe_lasthigh == pe_last_USR) {
        pe_last_USR->next = pe_ET;
        pl->lastevent = pe_ET;
        pe_last_USR = pe[i];
      }
      /* if high & low priority items exist ... */
      else {
        pe[i]->next = pe_lasthigh->next;
        pe_lasthigh->next = pe_ET;
      }
      pe_lasthigh = pe[i];
      pl->lasthigh++;
    }
    pe[i]->owner = ET_SYS;
    pl->cnt++;
  }
    
  ps->fix.out.start--;
  et_llist_unlock(pl);

  /* signal conductor thread that new events are here */
  status = pthread_cond_signal(&pl->cread);
  if (status != 0) {
    err_abort(status, "signal event here");
  }
  
  return ET_OK;
}


/*************************************************************************/
int et_station_nwriteNoPri(et_id *id, et_stat_id stat_id, et_event *pe[], int num)
{
  et_station *ps = id->stats + stat_id;
  et_list    *pl = &ps->list_out;
  et_event *pe_ET, *pe_last_USR=NULL;
  int i, status, num_in=0;
  
  if (num <= 0) {
    return ET_OK;
  }
  
  et_llist_lock(pl);

  if (num > (id->sys->config.nevents - pl->cnt)) {
    et_llist_unlock(pl);
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_station_nwrite, output list cnt is too high (%d)\n", pl->cnt);
    }
    return ET_ERROR;
  }

  /* info for fixing broken output list */
  if (pl->cnt == 0) {
    pl->firstevent = NULL;
  }
  ps->fix.out.cnt = pl->cnt;
  ps->fix.out.num = num;
  ps->fix.out.start++;
    
  if (pl->cnt == 0) {
    /* ptr to event represented in ET's process space */
    pe_ET = ET_PEVENT2ET(pe[0], id->offset);
    pl->firstevent = pe_ET;
    pl->lastevent  = pe_ET;
/*printf("et_station_nwrite: pe[0] = %p, pe[0]_ET = %p\n", pe[0], pe_ET);*/
    pe[0]->owner   = ET_SYS;
    pl->cnt++;
    num_in++;
  }
  
  pe_last_USR = ET_PEVENT2USR(pl->lastevent, id->offset);
  
  for (i=num_in; i < num ; i++) {
    pe_ET = ET_PEVENT2ET(pe[i], id->offset);
    
    /* if ET_LOW pri event, add to end ... */
    pe_last_USR->next = pe_ET;
    pl->lastevent = pe_ET;
    pe_last_USR = pe[i];
/*printf("et_station_nwrite: pe[i] = %p, pe[i]_ET = %p\n", pe[i], pe_ET);*/
    pe[i]->owner = ET_SYS;
    pl->cnt++;
  }
    
  ps->fix.out.start--;
  et_llist_unlock(pl);

  /* signal conductor thread that new events are here */
  status = pthread_cond_signal(&pl->cread);
  if (status != 0) {
    err_abort(status, "signal event here");
  }
  
  return ET_OK;
}


/**
 * This routine reads a number of entries from a station's input list.
 * 
 * @return ET_OK            for success
 * @return ET_ERROR         for error
 * @return ET_ERROR_TIMEOUT for timeout
 * @return ET_ERROR_EMPTY   for no events in list
 * @return ET_ERROR_BUSY    for async, inputlist is busy
 * @return ET_ERROR_WAKEUP  for wakeup and return
 */
int et_station_nread(et_id *id, et_stat_id stat_id, et_event *pe[], int mode,
                     et_att_id att, struct timespec *time, int num, int *nread)
{
  et_system *sys = id->sys;
  et_station *ps = id->stats + stat_id;
  et_list    *pl = &ps->list_in;
  int i, status;
  
  *nread = 0;
    
  if (num < 1) {
    return ET_OK;
  }
  
  if (mode == ET_SLEEP) {
    et_llist_lock(pl);
    /* if getting new event(s), histogram # of events in input list */
    if (stat_id == ET_GRANDCENTRAL) {
      *(id->histogram + pl->cnt) += 1 ;
    }
    while (pl->cnt < 1) {
        sys->attach[att].blocked = ET_ATT_BLOCKED;
        status = pthread_cond_wait(&pl->cread, &pl->mutex);
        sys->attach[att].blocked = ET_ATT_UNBLOCKED;
        if (status != 0) {
            err_abort(status, "Failed llist read wait");
        }
        if (sys->attach[att].quit == ET_ATT_QUIT) {
            if (id->debug >= ET_DEBUG_WARN) {
                et_logmsg("WARN", "et_station_nread, quitting\n");
            }
            et_llist_unlock(pl);
            sys->attach[att].quit = ET_ATT_CONTINUE;
            return ET_ERROR_WAKEUP;
        }
    }
  }
  else if (mode == ET_TIMED) {
      et_llist_lock(pl);
      if (stat_id == ET_GRANDCENTRAL) {
          *(id->histogram + pl->cnt) += 1 ;
      }
      while (pl->cnt < 1) {
          sys->attach[att].blocked = ET_ATT_BLOCKED;
          status = pthread_cond_timedwait(&pl->cread, &pl->mutex, time);
          sys->attach[att].blocked = ET_ATT_UNBLOCKED;
          if (status == ETIMEDOUT) {
              et_llist_unlock(pl);
              return ET_ERROR_TIMEOUT;
          }
          else if (status != 0) {
              /* Most likely a bad value for the time.
               * Don't abort, simply return an error.
               */
              if (id->debug >= ET_DEBUG_ERROR) {
                  et_logmsg("ERROR", "et_station_nread, pthread_cond_timedwait error\n");
              }
              return ET_ERROR;
          }
          if (sys->attach[att].quit == ET_ATT_QUIT) {
              if (id->debug >= ET_DEBUG_WARN) {
                  et_logmsg("WARN", "et_station_nread, quitting\n");
              }
              et_llist_unlock(pl);
              sys->attach[att].quit = ET_ATT_CONTINUE;
              return ET_ERROR_WAKEUP;
          }
      }
  }
  else if (mode == ET_ASYNC) {
      status = pthread_mutex_trylock(&pl->mutex);
      if (status == EBUSY) {
          return ET_ERROR_BUSY;
      }
      else if (status !=0) {
          err_abort(status, "Failed llist trylock");
      }
      if (sys->attach[att].quit == ET_ATT_QUIT) {
          if (id->debug >= ET_DEBUG_WARN) {
              et_logmsg("WARN", "et_station_nread, quitting\n");
          }
          et_llist_unlock(pl);
          sys->attach[att].quit = ET_ATT_CONTINUE;
          return ET_ERROR_WAKEUP;
      }
      if (stat_id == ET_GRANDCENTRAL) {
          *(id->histogram + pl->cnt) += 1 ;
      }
      if (pl->cnt < 1) {
          et_llist_unlock(pl);
          return ET_ERROR_EMPTY;
      }
  }
  else {
      if (id->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "et_station_nread, bad mode argument\n");
      }
      return ET_ERROR;
  }

  if (num > pl->cnt) {
      num = pl->cnt;
  }

  /* info for fixing broken input list */
  ps->fix.in.num   = num;
  ps->fix.in.cnt   = pl->cnt;
  ps->fix.in.call  = ET_FIX_READ;
  ps->fix.in.first = pl->firstevent;

  pe[0] = ET_PEVENT2USR(pl->firstevent, id->offset);
  pe[0]->owner = att;
  for(i=1; i < num ; i++) {
    pe[i] = ET_PEVENT2USR(pe[i-1]->next, id->offset);
    pe[i]->owner = att;
  }     
  pl->firstevent = pe[num-1]->next;
  pl->cnt -= num;
  ps->fix.in.first = NULL;
  
  et_llist_unlock(pl);

  *nread = num;
  return ET_OK;
}


/**
 * This routine reads a number of entries from a station's input list.
 * It only reads events belonging to the given group number.
 * 
 * @return ET_OK            for success
 * @return ET_ERROR         for error
 * @return ET_ERROR_TIMEOUT for timeout
 * @return ET_ERROR_EMPTY   for no events in list
 * @return ET_ERROR_BUSY    for async, inputlist is busy
 * @return ET_ERROR_WAKEUP  for wakeup and return
 */
int et_station_nread_group(et_id *id, et_stat_id stat_id, et_event *pe[], int mode,
                           et_att_id att, struct timespec *time, int num, int group,
                           int *nread)
{
    et_event  *pev, *prev;
    et_system *sys = id->sys;
    et_station *ps = id->stats + stat_id;
    et_list    *pl = &ps->list_in;
    int i, status, index, scanList=1;

    *nread = 0;

    if (num < 1) {
        return ET_OK;
    }

    do {
        if (mode == ET_SLEEP) {
            et_llist_lock(pl);
            /* if getting new event(s), histogram # of events in input list */
            if (stat_id == ET_GRANDCENTRAL && scanList) {
                *(id->histogram + pl->cnt) += 1 ;
            }
            while (pl->cnt < 1 || !scanList) {
                sys->attach[att].blocked = ET_ATT_BLOCKED;
                status = pthread_cond_wait(&pl->cread, &pl->mutex);
                sys->attach[att].blocked = ET_ATT_UNBLOCKED;
                if (status != 0) {
                    err_abort(status, "Failed llist read wait");
                }
                if (sys->attach[att].quit == ET_ATT_QUIT) {
                    if (id->debug >= ET_DEBUG_WARN) {
                        et_logmsg("WARN", "et_station_nread, quitting\n");
                    }
                    et_llist_unlock(pl);
                    sys->attach[att].quit = ET_ATT_CONTINUE;
                    return ET_ERROR_WAKEUP;
                }
                scanList = 1;
            }
        }
        else if (mode == ET_TIMED) {
            et_llist_lock(pl);
            if (stat_id == ET_GRANDCENTRAL && scanList) {
                *(id->histogram + pl->cnt) += 1 ;
            }
            while (pl->cnt < 1 || !scanList) {
                sys->attach[att].blocked = ET_ATT_BLOCKED;
                status = pthread_cond_timedwait(&pl->cread, &pl->mutex, time);
                sys->attach[att].blocked = ET_ATT_UNBLOCKED;
                if (status == ETIMEDOUT) {
                    et_llist_unlock(pl);
                    return ET_ERROR_TIMEOUT;
                }
                else if (status != 0) {
                    /* Most likely a bad value for the time.
                     * Don't abort, simply return an error.
                     */
                    if (id->debug >= ET_DEBUG_ERROR) {
                        et_logmsg("ERROR", "et_station_nread, pthread_cond_timedwait error\n");
                    }
                    return ET_ERROR;
                }
                if (sys->attach[att].quit == ET_ATT_QUIT) {
                    if (id->debug >= ET_DEBUG_WARN) {
                        et_logmsg("WARN", "et_station_nread, quitting\n");
                    }
                    et_llist_unlock(pl);
                    sys->attach[att].quit = ET_ATT_CONTINUE;
                    return ET_ERROR_WAKEUP;
                }
                scanList = 1;
            }
        }
        else if (mode == ET_ASYNC) {
            status = pthread_mutex_trylock(&pl->mutex);
            if (status == EBUSY) {
                return ET_ERROR_BUSY;
            }
            else if (status !=0) {
                err_abort(status, "Failed llist trylock");
            }
            if (sys->attach[att].quit == ET_ATT_QUIT) {
                if (id->debug >= ET_DEBUG_WARN) {
                    et_logmsg("WARN", "et_station_nread, quitting\n");
                }
                et_llist_unlock(pl);
                sys->attach[att].quit = ET_ATT_CONTINUE;
                return ET_ERROR_WAKEUP;
            }
            if (stat_id == ET_GRANDCENTRAL) {
                *(id->histogram + pl->cnt) += 1 ;
            }
            if (pl->cnt < 1) {
                et_llist_unlock(pl);
                return ET_ERROR_EMPTY;
            }
        }
        else {
            if (id->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_station_nread, bad mode argument\n");
            }
            return ET_ERROR;
        }

        if (num > pl->cnt) {
            num = pl->cnt;
        }

        /* info for fixing broken input list */
        ps->fix.in.num   = num;
        ps->fix.in.cnt   = pl->cnt;
        ps->fix.in.call  = ET_FIX_READ;
        ps->fix.in.first = pl->firstevent;

        index = 0;
        prev  = NULL;
        pev   = ET_PEVENT2USR(pl->firstevent, id->offset);

        for (i=0; i < pl->cnt; i++) {
            if (pev->group != group) {
                prev = pev;
                pev = ET_PEVENT2USR(pev->next, id->offset);
                continue;
            }
            pe[index++] = pev;
            pev->owner  = att;
            if (prev == NULL) {
                pl->firstevent = pev->next;
            }
            else {
                prev->next = pev->next;
            }
            /* if we grabbed the last event in the list, reset last item pointer */
            if (pl->lastevent == ET_PEVENT2ET(pev, id->offset)) {
                pl->lastevent = ET_PEVENT2ET(prev, id->offset);
            }
            if (index >= num) break;
            pev = ET_PEVENT2USR(pev->next, id->offset);       
        }

        pl->cnt -= index;
        ps->fix.in.first = NULL;

        et_llist_unlock(pl);
        scanList = 0;

        /* If we got nothing and we're ET_SLEEP or ET_TIMED, then try again */
    } while (index == 0 && mode != ET_ASYNC);

    *nread = index;
    return ET_OK;
}

/*****************************************************
 * et_station_(n)dump routines are for writing
 * events into a GrandCentral station's input list.
 * Notice that event priority is ignored in this case.
 *
 * Normally, conductors handle the transfer of events
 * into a station's input list. In that sense, the next
 * two routines violate ET philosophy by transfering
 * events directly into GrandCentral's input list. One
 * consequence is that these routines must take care of
 * keeping statistics by changing pl->events_in
 * usually done in routines et_llist_write_gc and 
 * et_llist_write.
 *****************************************************/
int et_station_dump(et_id *id, et_event *pe)
{
  et_station *ps = id->stats; /* GrandCentral */
  et_list    *pl = &ps->list_in;
  et_event   *pe_ET, *pe_last;
  int         status;
  
  et_llist_lock(pl);

  if (pl->cnt >= id->sys->config.nevents) {
    et_llist_unlock(pl); 
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_station_dump, output list cnt is too high (%d)\n", pl->cnt);
    }
    return ET_ERROR;
  }
  
  /* ptr to event represented in ET's process space */
  pe_ET = ET_PEVENT2ET(pe, id->offset);
  
  /* info for fixing broken GC input list */
  if (pl->cnt == 0) {
    pl->firstevent = NULL;
  }
  ps->fix.in.eventsin = pl->events_in;
  ps->fix.in.cnt  = pl->cnt;
  ps->fix.in.num  = 1;
  ps->fix.in.call = ET_FIX_DUMP;
  ps->fix.in.start++;

  /* if no events in list ... */
  if (pl->cnt == 0) {
    pl->firstevent = pe_ET;
  }
  else {
    /* last event in user's process space */
    pe_last = ET_PEVENT2USR(pl->lastevent, id->offset);
    pe_last->next = pe_ET;
  }
  pl->lastevent = pe_ET;
  
  pe->owner = ET_SYS;
  pl->cnt++;
  pl->events_in = pl->events_in + 1;
  ps->fix.in.start--;
  et_llist_unlock(pl);

  /* signal that new event is here */
  status = pthread_cond_signal(&pl->cread);
  if (status != 0) {
    err_abort(status, "signal event here");
  }

  return ET_OK;
}

/*************************************************************************/
int et_station_ndump(et_id *id, et_event *pe[], int num)
{
  et_station *ps = id->stats; /* GrandCentral */
  et_list    *pl = &ps->list_in;
  et_event *pe_ET, *pe_last;
  int i, status, num_in=0;
  
  if (num <= 0) {
    return ET_OK;
  }
  
  et_llist_lock(pl);

  if (num > (id->sys->config.nevents - pl->cnt)) {
    et_llist_unlock(pl);
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_station_ndump, output list cnt is too high (%d)\n", pl->cnt);
    }
    return ET_ERROR;
  }

  /* info for fixing broken GC input list */
  if (pl->cnt == 0) {
    pl->firstevent = NULL;
  }
  ps->fix.in.eventsin = pl->events_in;
  ps->fix.in.cnt  = pl->cnt;
  ps->fix.in.num  = num;
  ps->fix.in.call = ET_FIX_DUMP;
  ps->fix.in.start++;
          
  if (pl->cnt == 0) {
    /* ptr to event represented in ET's process space */
    pe_ET = ET_PEVENT2ET(pe[0], id->offset);
    pl->firstevent = pe_ET;
    pl->lastevent  = pe_ET;
    pe[0]->owner   = ET_SYS;
    pl->cnt++;
    num_in++;
  }
  pe_last = ET_PEVENT2USR(pl->lastevent, id->offset);
  
  for (i=num_in; i < num ; i++) {
    pe_ET = ET_PEVENT2ET(pe[i], id->offset);
    pe_last->next = pe_ET;
    pl->lastevent = pe_ET;
    pe_last = pe[i];
    pe[i]->owner = ET_SYS;
    pl->cnt++;
  }
            
  ps->fix.in.start--;
  et_llist_unlock(pl);

  /* signal that new events are here */
  status = pthread_cond_signal(&pl->cread);
  if (status != 0) {
    err_abort(status, "signal event here");
  }
  
  return ET_OK;
}



/*****************************************************
 * et_llist_read/write routines are for use only by the ET
 * system process since all pointer values read from
 * the mapped memory are not translated into local
 * process space. They are used for reading from and
 * writing to station lists, either input or output.
 *****************************************************/
int et_llist_read(et_list *pl, et_event **pe)
{
  int i, cnt = pl->cnt;
  
  if (cnt == 0) {
    /* printf("et_llist_read: nothing to read\n");*/
    return ET_ERROR;
  }
      
  pe[0] = pl->firstevent;
/*printf("et_llist_read: pe[0] = %p, ->next = %p\n", pe[0], pe[0]->next);*/
  
  for (i=1; i < cnt ; i++) {
    pe[i] = pe[i-1]->next;
/*printf("et_llist_read: pe[%d] = %p, ->next = %p\n", i, pe[i], pe[i]->next); */
  } 
  pl->firstevent = NULL;
  pl->cnt = 0;
  pl->lasthigh = 0;
  pl->events_out = pl->events_out + cnt;
 
  return cnt;
}

/*****************************************************/
/* routine for writing to gc - ignore priorities     */
int et_llist_write_gc(et_id *id, et_event **pe, int num)
{
  int i, nevents_max = id->sys->config.nevents;
  et_list *pl = &id->grandcentral->list_in; /* input list of GrandCentral */
  
  if (num > (nevents_max - pl->cnt)) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_llist_write_gc, writing too many events\n");
    }
    return ET_ERROR;
  }
  
  if (pl->cnt == 0) {
    pl->firstevent = pe[0];
  }
  else {
   (pl->lastevent)->next = pe[0];
  }
  
  for (i=1; i < num ; i++) {
    pe[i-1]->next = pe[i];
  }
  
  pl->lastevent = pe[num-1];
  pl->events_in = pl->events_in + num;
  pl->cnt += num;

  return ET_OK;
}

/*****************************************************/
int et_llist_write(et_id *id, et_list *pl, et_event **pe, int num)
{
  et_event *pevent, *pe_lasthigh;
  int i, j=0, num_high=1, high_cnt=0,
      nevents_max = id->sys->config.nevents;

  if (num > (nevents_max - pl->cnt)) {
    num = nevents_max - pl->cnt;
  }
  
  /* all events priority == ET_LOW or no events in list */
  if ((pe[0]->priority == ET_LOW) || (pl->cnt == 0)) {
    if (pl->cnt == 0) {
      pl->firstevent = pe[0];
    }
    else {
     (pl->lastevent)->next = pe[0];
    }
    
    for (i=1; i < num ; i++) {
      pe[i-1]->next = pe[i];
    }
    pl->lastevent = pe[num-1];
  }
  /* if any ET_HIGH pri events (pl->cnt != 0) ... */
  else {
/*printf("lwrite: ET_HIGH pri event\n");*/
    /* find last high priority event already in list */
    pe_lasthigh = NULL;
    pevent = pl->firstevent;
    for (i=0; i < pl->cnt ;i++) {
      if (pevent->priority == ET_HIGH) {
        pe_lasthigh = pevent;
        pevent = pevent->next;
        /* count # of ET_HIGH pri events already in list */
        high_cnt++; 
        continue;
      }
      else {
        break;
      }
    }
/*printf("lwrite: lasthigh = %p\n", pe_lasthigh);*/
    /* add first high pri item after others */
    if (pe_lasthigh != NULL) {
      pe[0]->next = pe_lasthigh->next;
      pe_lasthigh->next = pe[0];
    }
    else {
/*printf("lwrite: lasthigh = first\n");*/
      pe[0]->next = pl->firstevent;
      pl->firstevent = pe[0];
    }

    /* add rest of high pri items */
    for (i=1; i < num ; i++) {
      if (pe[i]->priority != ET_HIGH) {
        break;
      }
      pe[i-1]->next = pe[i];
      pe[i]->next   = pevent;
      num_high++;
    }
    
    /* if only ET_HIGH pri events, set last event */
    if (high_cnt == pl->cnt) {
/*printf("lwrite: pl->cnt = %d, high_cnt = %d\n", pl->cnt, high_cnt);*/
      pl->lastevent = pe[num_high-1];
    }
    
/*printf("lwrite: we add num_high (%d) ET_HIGH pri events\n", num_high);*/
    /* rest are ET_LOW pri, add to end */
    if (num_high < num) {
      for (j=num_high+1; j < num ; j++) {
        pe[j-1]->next = pe[j];
/*printf("-");*/
      }
      (pl->lastevent)->next = pe[num_high];
       pl->lastevent = pe[num-1];
    }
/*printf("lwrite: j = %d\n", j);*/
  }
    
  pl->events_in = pl->events_in + num;
  pl->cnt += num;

  return num;
}

/*****************************************************
 * These are routines for writing to in/output lists from a
 * user's process - takes care of pointer translations.
 * The array of event pointers, pe, must be in the user's
 * process space (as must be pl).
 *****************************************************/
#if 0
static int et_llist_put(et_id *id, et_list *pl, et_event **pe, int num)
{
  int i, nevents_max = id->sys->config.nevents;
  et_event *plast;
  
  if (num > (nevents_max - pl->cnt)) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_llist_put, writing too many events\n");
    }
    return ET_ERROR;
  }

  plast = ET_PEVENT2USR(pl->lastevent, id->offset);

  if (pl->cnt == 0) {
    pl->firstevent = ET_PEVENT2ET(pe[0], id->offset);
  }
  else {
    plast->next = ET_PEVENT2ET(pe[0], id->offset);
  }

  for (i=1; i < num ; i++) {
    pe[i-1]->next = ET_PEVENT2ET(pe[i], id->offset);
  }
  
  pl->lastevent = ET_PEVENT2ET(pe[num-1], id->offset);
  /* pl->events_in = pl->events_in + num; */
  pl->cnt += num;

  return ET_OK;
}
#endif

/*****************************************************
 * This routine is only (meant to be) used in et_restore_events.
 * Et_restore_events generates a list of "lost" events that need
 * to be recovered. This routine does put high priority events first,
 * but does not guarantee that events are in their original order.
 * This routine is used for restoring events back to their
 * station's input list.
 *****************************************************/
static int et_restore_in(et_id *id, et_station *ps, et_event **pe,
                         int num, int *numwritten)
{
  et_event *plasthigh=NULL, *pfirst, *plast, *pfirstlow=NULL;
  int i, j=0, status, num_high=1, high_cnt=0;
  int nevents_max = id->sys->config.nevents;
  et_list *pl = &(ps->list_in);
  
  et_llist_lock(pl);
  
  if (pl->cnt >= nevents_max) {
    et_llist_unlock(pl);
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_llist_restore_in, list already full\n");
    }
    return ET_ERROR;
  }
  
  if (num > (nevents_max - pl->cnt)) {
    num = nevents_max - pl->cnt;
  }
  
  /* find last high & first low priority events already in list */
  if (pl->cnt != 0) {
    plasthigh = NULL;
    pfirstlow = ET_PEVENT2USR(pl->firstevent, id->offset);
    for(i=0; i < pl->cnt ;i++) {
      if (pfirstlow->priority == ET_HIGH) {
        plasthigh = pfirstlow;
        pfirstlow = ET_PEVENT2USR(pfirstlow->next, id->offset);
        /* count # of ET_HIGH pri events already in list */
        high_cnt++; 
        continue;
      }
      else {
        break;
      }
    }
    if (high_cnt == pl->cnt) {
      pfirstlow = NULL;
    }
  }
    
  /* no events currently in list - put them in "as is" */
  if (pl->cnt == 0) {
    for (i=1; i < num ; i++) {
      pe[i-1]->next = ET_PEVENT2ET(pe[i], id->offset);
    }
    pl->firstevent = ET_PEVENT2ET(pe[0], id->offset);
    pl->lastevent  = ET_PEVENT2ET(pe[num-1], id->offset);
  }
  
  /* if only ET_LOW pri events to be added ... */
  else if (pe[0]->priority == ET_LOW) {
    plast  = ET_PEVENT2USR(pl->lastevent, id->offset);
    pfirst = pl->firstevent;

    /* string lows together */
    for (i=1; i < num ; i++) {
      pe[i-1]->next = ET_PEVENT2ET(pe[i], id->offset);
    }
    
    if (pfirstlow == NULL) {
        /* tack 'em on the end since no lows already in list */
        plast->next   = ET_PEVENT2ET(pe[0], id->offset);
        pl->lastevent = ET_PEVENT2ET(pe[num-1], id->offset);
    }
    else {
        /* put 'em before existing lows but after highs */
        if (plasthigh != NULL) {
          /* if highs already exist in list */
          plasthigh->next = ET_PEVENT2ET(pe[0], id->offset);
        }
        else {
          pl->firstevent = ET_PEVENT2ET(pe[0], id->offset);
        }
        pe[num-1]->next = ET_PEVENT2ET(pfirstlow, id->offset);
    }
   
  }
  
  /* if any ET_HIGH pri events (pl->cnt != 0) ... */
  else {
    /*
     * we want to put hi pri items before all existing items and
     * low pri items after hi pri but before other low pri items.
     */
     
    /* put high pri item before all others */
    plast  = ET_PEVENT2USR(pl->lastevent, id->offset);
    pfirst = pl->firstevent;
    pe[0]->next = pl->firstevent;
    pl->firstevent = ET_PEVENT2ET(pe[0], id->offset);

    /* add rest of high pri items */
    for (i=1; i < num ; i++) {
      if (pe[i]->priority != ET_HIGH) {
        break;
      }
      pe[i-1]->next = ET_PEVENT2ET(pe[i], id->offset);
      pe[i]->next   = pfirst;
      num_high++;
    }
   
    /* rest are low pri, add after high */
    if (num_high < num) {
      /* string lows together */
      for (j=num_high+1; j < num ; j++) {
        pe[j-1]->next = ET_PEVENT2ET(pe[j], id->offset);
      }
      if (pfirstlow == NULL) {
        /* tack 'em on the end since no lows already in list */
        plast->next   = ET_PEVENT2ET(pe[num_high], id->offset);
        pl->lastevent = ET_PEVENT2ET(pe[num-1], id->offset);
      }
      else {
        /* put 'em before existing lows but after highs */
        if (plasthigh != NULL) {
          /* if highs already exist in list */
          plasthigh->next = ET_PEVENT2ET(pe[num_high], id->offset);
        }
        else {
          pe[num_high-1]->next = ET_PEVENT2ET(pe[num_high], id->offset);
        }
        pe[num-1]->next = ET_PEVENT2ET(pfirstlow, id->offset);
      }
    }
  }
  
  /* Don't count these additions as its their second time
   * being put into this station's input list - 1/12/2001 Timmer
   */  
  /* pl->events_in = pl->events_in + num;*/
  pl->cnt += num;
  
  et_llist_unlock(pl);
  
  /* signal reader(s) that events are here */
  status = pthread_cond_broadcast(&pl->cread);
  if (status != 0) {
    err_abort(status, "et_llist_restore_in");
  }
  
  if (numwritten != NULL) {
    *numwritten = num;
  }
  
  return ET_OK;
}

/*****************************************************
 * This routine is only (meant to be) used in et_restore_events.
 * Et_restore_events generates a list of "lost" events that need
 * to be recovered. This routine does put high priority events first,
 * but does not guarantee that events are in their original order.
 * This routine is used for restoring events back a station's
 * input or output list.
 *****************************************************/
static int et_restore(et_id *id, et_list *pl, et_event **pe,
                      int num, int *numwritten)
{
  et_event *plasthigh=NULL, *pfirst, *plast, *pfirstlow=NULL;
  int i, j=0, status, num_high=1, high_cnt=0;
  int nevents_max = id->sys->config.nevents;
  
  et_llist_lock(pl);
  
  if (pl->cnt >= nevents_max) {
    et_llist_unlock(pl);
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_llist_restore_in, list already full\n");
    }
    return ET_ERROR;
  }
  
  if (num > (nevents_max - pl->cnt)) {
    num = nevents_max - pl->cnt;
  }
  
  /* find last high & first low priority events already in list */
  if (pl->cnt != 0) {
    plasthigh = NULL;
    pfirstlow = ET_PEVENT2USR(pl->firstevent, id->offset);
    for(i=0; i < pl->cnt ;i++) {
      if (pfirstlow->priority == ET_HIGH) {
        plasthigh = pfirstlow;
        pfirstlow = ET_PEVENT2USR(pfirstlow->next, id->offset);
        /* count # of ET_HIGH pri events already in list */
        high_cnt++; 
        continue;
      }
      else {
        break;
      }
    }
    if (high_cnt == pl->cnt) {
      pfirstlow = NULL;
    }
  }
  
  /* change the owner to the system */
  for (i=0; i < num ; i++) {
    pe[i]->owner = ET_SYS;
  }  
    
  /* no events currently in list - put them in "as is" */
  if (pl->cnt == 0) {
    for (i=1; i < num ; i++) {
      pe[i-1]->next = ET_PEVENT2ET(pe[i], id->offset);
    }
    pl->firstevent = ET_PEVENT2ET(pe[0], id->offset);
    pl->lastevent  = ET_PEVENT2ET(pe[num-1], id->offset);
  }
  
  /* if only ET_LOW pri events to be added ... */
  else if (pe[0]->priority == ET_LOW) {
    plast  = ET_PEVENT2USR(pl->lastevent, id->offset);
    pfirst = pl->firstevent;

    /* string lows together */
    for (i=1; i < num ; i++) {
      pe[i-1]->next = ET_PEVENT2ET(pe[i], id->offset);
    }
    
    if (pfirstlow == NULL) {
        /* tack 'em on the end since no lows already in list */
        plast->next   = ET_PEVENT2ET(pe[0], id->offset);
        pl->lastevent = ET_PEVENT2ET(pe[num-1], id->offset);
    }
    else {
        /* put 'em before existing lows but after highs */
        if (plasthigh != NULL) {
          /* if highs already exist in list */
          plasthigh->next = ET_PEVENT2ET(pe[0], id->offset);
        }
        else {
          pl->firstevent = ET_PEVENT2ET(pe[0], id->offset);
        }
        pe[num-1]->next = ET_PEVENT2ET(pfirstlow, id->offset);
    }
   
  }
  
  /* if any ET_HIGH pri events (pl->cnt != 0) ... */
  else {
    /*
     * we want to put hi pri items before all existing items and
     * low pri items after hi pri but before other low pri items.
     */
     
    /* put high pri item before all others */
    plast  = ET_PEVENT2USR(pl->lastevent, id->offset);
    pfirst = pl->firstevent;
    pe[0]->next = pl->firstevent;
    pl->firstevent = ET_PEVENT2ET(pe[0], id->offset);

    /* add rest of high pri items */
    for (i=1; i < num ; i++) {
      if (pe[i]->priority != ET_HIGH) {
        break;
      }
      pe[i-1]->next = ET_PEVENT2ET(pe[i], id->offset);
      pe[i]->next   = pfirst;
      num_high++;
    }
   
    /* rest are low pri, add after high */
    if (num_high < num) {
      /* string lows together */
      for (j=num_high+1; j < num ; j++) {
        pe[j-1]->next = ET_PEVENT2ET(pe[j], id->offset);
      }
      if (pfirstlow == NULL) {
        /* tack 'em on the end since no lows already in list */
        plast->next   = ET_PEVENT2ET(pe[num_high], id->offset);
        pl->lastevent = ET_PEVENT2ET(pe[num-1], id->offset);
      }
      else {
        /* put 'em before existing lows but after highs */
        if (plasthigh != NULL) {
          /* if highs already exist in list */
          plasthigh->next = ET_PEVENT2ET(pe[num_high], id->offset);
        }
        else {
          pe[num_high-1]->next = ET_PEVENT2ET(pe[num_high], id->offset);
        }
        pe[num-1]->next = ET_PEVENT2ET(pfirstlow, id->offset);
      }
    }
  }
  
  /* Don't count these additions as its their second time
   * being put into this station's list - 1/12/2001 Timmer
   */  
  /* pl->events_in = pl->events_in + num;*/
  pl->cnt += num;
  
  et_llist_unlock(pl);
  
  /* signal reader(s) that events are here */
  status = pthread_cond_broadcast(&pl->cread);
  if (status != 0) {
    err_abort(status, "et_llist_restore_in");
  }
  
  if (numwritten != NULL) {
    *numwritten = num;
  }
  
  return ET_OK;
}

/*****************************************************
 * Move events from station's list_in to list_out. This
 * routine is only called when no processes are attached
 * to the station being flushed.
 *
 * Since this routine is called by both the ET system
 * and user processes, it is implemented with calls to
 * et_station_nread/nwrite. This insures that all proper
 * pointer translations from mem mapped area are done.
 * It's also possible to use these routines because we are
 * only reading from the in list and writing to the out list.
 *****************************************************/
void et_flush_events(et_id *id, et_att_id att, et_stat_id stat_id)
{
  int numread, status, nevents_max = id->sys->config.nevents;
  et_station *ps = id->stats + stat_id;
  et_event **pe;

  if (stat_id == ET_GRANDCENTRAL) {
    return;
  }

  /* calloc arrays since we don't know how big they are at compile time */
  if ( (pe = (et_event **) calloc((size_t) nevents_max, sizeof(et_event *))) == NULL) {
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_flush_events, no memory left\n");
    }
    return;
  }
  
  /* Read in the events from the input list of this station.
   * Use value of attachment (att) that is in the process of detaching.
   * Thus, if a crash occurs in the middle of this routine, the events
   * will later be recovered by the ET system's call to et_restore_events.
   */
  status = et_station_nread(id, stat_id, pe, ET_ASYNC, att, NULL, nevents_max, &numread);
  if (status < 0) {
      if ((status != ET_ERROR_EMPTY) && (id->debug >= ET_DEBUG_ERROR)) {
          et_logmsg("ERROR", "et_flush_events, cannot read events from input list\n");
      }
      free(pe);
      return;
  }
  else {
      if (id->debug >= ET_DEBUG_INFO) {
          et_logmsg("INFO", "et_flush_events, read %d events\n", numread);
      }
  }

  /* If we're dealing with parallel stations redistributing events while restoring,
   * move them from the station's input list to the previous station's output list,
   * else move events from the station's input list to its output list.
   */
  if (ps->config.restore_mode == ET_STATION_RESTORE_REDIST) {
      et_station *firstParallel, *prev;
      et_list *pl;

      /* Find the previous station by finding the first parallel station
       * in this group. The station before that in the main linked list
       * will our "prev" station.
       */
      firstParallel = ps;
      while (firstParallel->prevparallel > -1) {
          firstParallel = id->grandcentral + firstParallel->prevparallel;
      }
      prev = id->grandcentral + firstParallel->prev;

      /* get its output list */
      pl = &prev->list_out;

      /* Just write events into the previous station's output list.
       * Statistics of this station may get messed up a bit if
       * events end up coming thru a second time. Oh well.
       * 4/21/2008 - Timmer
       */
      status = et_restore(id, pl, pe, numread, NULL);
      if (status < 0) {
          if (id->debug >= ET_DEBUG_ERROR) {
              et_logmsg("ERROR", "et_flush_events, cannot write events to output list\n");
          }
      }
      else {
          if (id->debug >= ET_DEBUG_INFO) {
              et_logmsg("INFO", "et_flush_events, wrote %d events to %s's output list\n",
                      numread, prev->name);
          }
      }
  }
  else {
      status = et_station_nwrite(id, stat_id, pe, numread);
      if (status < 0) {
          if (id->debug >= ET_DEBUG_ERROR) {
              et_logmsg("ERROR", "et_flush_events, cannot write events to output list\n");
          }
      }
      else {
          if (id->debug >= ET_DEBUG_INFO) {
              et_logmsg("INFO", "et_flush_events, wrote %d events\n", numread);
          }
      }
  }

  
  free(pe);
  return;
}


/*****************************************************/
int et_restore_events(et_id *id, et_att_id att, et_stat_id stat_id)
/*
 * If a process crashes or detaches from a station,
 * it may still own events - events that were read
 * but not written. This routine restores these to
 * the system for use by others.
 */
{
  et_station *ps = id->stats + stat_id;
  int   i, j, status=ET_OK,
        num_events, num_temps, num_written = 0, num_new,
        mode = ps->config.restore_mode,
        nevents_max = id->sys->config.nevents;
  et_event  *pe = id->events;
  et_event  **pevent, **ordered, **new;
  
  /* calloc arrays since we don't know how big they are at compile time */
  /* one array for the normal events */
  if ( (pevent = (et_event **) calloc((size_t) nevents_max, sizeof(et_event *))) == NULL) {
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_restore_events, no memory left\n");
    }
    return ET_ERROR;
  }
  /* one array for the new events - those that were obtained by calls to 
   * et_event(s)_new but never put into the system. They may not contain
   * valid data and must be dumped
   */
  if ( (new = (et_event **) calloc((size_t) nevents_max, sizeof(et_event *))) == NULL) {
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_restore_events, no memory left\n");
    }
    free(pevent);
    return ET_ERROR;
  }
  
  /* get arrays of normal & new events belonging to "att" */
  num_events = num_temps = num_new = 0;
  for (i=0 ; i < id->sys->config.nevents ; i++) {
    if (pe->owner == att) {
      /* separate the new from the "used" events */
      if (pe->age == ET_EVENT_NEW) {
        new[num_new++] = pe;
      }
      else {
        pevent[num_events++] = pe;
        pe->owner = ET_SYS;
      }
      if (pe->temp == ET_EVENT_TEMP) {
        num_temps++;
      }
    }
    pe++;
  }
  
  if ((num_events == 0) && (num_new == 0)) {
    free(pevent); free(new);
    if (id->debug >= ET_DEBUG_INFO) {
      et_logmsg("INFO", "et_restore_events, no events found belonging to attachment %d \n", att);
    }
    return ET_OK;
  }
  
  /* calloc array size of number of recovered normal events */
  if ( (ordered = (et_event **) calloc((size_t) num_events, sizeof(et_event *))) == NULL) {
    if (id->debug >= ET_DEBUG_SEVERE) {
      et_logmsg("SEVERE", "et_restore_events, no memory left\n");
    }
    free(pevent); free(new);
    return ET_ERROR;
  }

  /* for convenience, order events so high priority are first in array */
  for (i=0, j=0 ; i < num_events ; i++) {
    if (pevent[i]->priority == ET_HIGH) {
      ordered[j++] = pevent[i];
    }
  }
  /* now put in all the low priority events */
  for (i=0; i < num_events ; i++) {
    if (pevent[i]->priority != ET_HIGH) {
      ordered[j++] = pevent[i];
    }
  }
  
  if (id->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_restore_events, found %d normal, %d new, %d temp events belonging to %d\n", num_events, num_new, num_temps, att);
  }
  
  /*
   * Since this routine is only called by et_station_detach,
   * the system mutex is already locked, otherwise the
   * system will need to be locked when changing id->sys->ntemps.
   */
  
  /* first dump all the new events into GrandCentral */
  if (num_new > 0) {
    for (i=0; i < num_new ; i++) {
      /*
       * Temp events were mapped into the process holding the
       * attachment "att". If that process is calling this routine,
       * we must remove the temp events with "et_temp_remove" which
       * unmaps and then unlinks (deletes) the associated file.
       * However, if this routine is being called by the ET system
       * trying to cleanup after a crashed process, then we must
       * use "unlink" to get rid of the temp files but must not
       * call "et_temp_remove" as it would try to unmap memory
       * that is not mapped. The temp files are unlinked because
       * we return these events back to GrandCentral station.
       */
      if (new[i]->temp == ET_EVENT_TEMP) {
        if (id->cleanup == 1) {
          unlink(new[i]->filename);
        }
        else {
          et_temp_remove(new[i]->filename, new[i]->pdata, (size_t) new[i]->memsize);
        }
        id->sys->ntemps--;
      }
    }
    status = et_station_ndump(id, new, num_new);
    if (id->debug >= ET_DEBUG_INFO) {
      et_logmsg("INFO", "et_restore_events, dumped %d new events\n", num_new);
    }
  }
  
  if (num_events > 0) {
    /* If normal events are to be returned to GrandCentral's input list ... */
    if (mode == ET_STATION_RESTORE_GC || stat_id == ET_GRANDCENTRAL) {
      /* free temporary events' allocated memory (see note above) */
      for (i=0; i < num_events ; i++) {
        if (ordered[i]->temp == ET_EVENT_TEMP) {
          if (id->cleanup == 1) {
            unlink(ordered[i]->filename);
          }
          else {
            et_temp_remove(ordered[i]->filename, ordered[i]->pdata, (size_t) ordered[i]->memsize);
          }
          id->sys->ntemps--;
          if (id->debug >= ET_DEBUG_INFO) {
            et_logmsg("INFO", "et_restore_events, deleting tmp event %s\n",
                      ordered[i]->filename);
          }
        }
      }
      status = et_station_ndump(id, ordered, num_events);   
      if (id->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_restore_events, restored %d events to GrandCentral\n",
                  num_events);
      }
    }

    /*
     * If events are to be returned to station's output list ...
     * Notice that if we are supposed to put things in the station's
     * input list, but we are the last attachement (and in the
     * middle of detaching), then events put into the station's input
     * list will be lost to the system. Place them into the output list.
     */
    else if ((mode == ET_STATION_RESTORE_OUT) ||
             (mode == ET_STATION_RESTORE_IN && ps->data.nattachments == 0)) {
      /* unmap mem of temp events if I'm the user that mapped it */
      if (id->cleanup != 1) {
        for (i=0; i < num_events ; i++) {
          if (ordered[i]->temp == ET_EVENT_TEMP) {
            munmap(ordered[i]->pdata, (size_t) ordered[i]->memsize);
            if (id->debug >= ET_DEBUG_INFO) {
              et_logmsg("INFO", "et_restore_events, unmap tmp event %s\n",
                        ordered[i]->filename);
            }
          }
        }
      }
      /* Mark these events as possibly having corrupt data since 
       * the user that crashed with these events may have been in
       * the middle of modifying them.
       */
      /* When detaching for a remote user, data is NOT corrupted,
       * but that is not taken into account here. It could be done
       * by seeing if attachment's host is remote. 1/12/2001 - Timmer
       */
      for (i=0; i < num_events ; i++) {
        ordered[i]->datastatus = ET_DATA_POSSIBLY_CORRUPT;
      }
      
      status = et_station_nwrite(id, stat_id, ordered, num_events);
      if (id->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_restore_events, restored %d events to %s's output list\n",
                  num_events, ps->name);
      }
    }

    /* If events are to be returned to station's input list ... */
    else if (mode == ET_STATION_RESTORE_IN) {
      /* unmap mem of temp events if I'm the user that mapped it */
      if (id->cleanup != 1) {
        for (i=0; i < num_events ; i++) {
          if (ordered[i]->temp == ET_EVENT_TEMP) {
            munmap(ordered[i]->pdata, (size_t) ordered[i]->memsize);
            if (id->debug >= ET_DEBUG_INFO) {
              et_logmsg("INFO", "et_restore_events, unmap tmp event %s\n",
                        ordered[i]->filename);
            }
          }
        }
      }
      /* Mark these events as possibly having corrupt data since 
       * the user that crashed with these events may have been in
       * the middle of modifying them.
       */
      /* When detaching for a remote user, data is NOT corrupted,
       * but that is not taken into account here. It could be done
       * by seeing if attachment's host is remote. 1/12/2001 - Timmer
       */
      for (i=0; i < num_events ; i++) {
        ordered[i]->datastatus = ET_DATA_POSSIBLY_CORRUPT;
      }
      
      /* There is an easier way to do this. Just write events into
       * the previous station's output list. But then statistics
       * get messed up a bit. One could also check to see if the
       * station was blocking and if so only do the et_station_nwrite.
       * 1/12/2001 - Timmer
       */
      status = et_restore(id, &(ps->list_in), ordered, num_events, &num_written);
      if (id->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_restore_events, restored %d events to %s's input list\n",
                  num_written, ps->name);
      }

      if (num_written < num_events) {
        /* station won't accept all events, put rest into output list */
        if (id->debug >= ET_DEBUG_WARN) {
          et_logmsg("WARN", "et_restore_events, input list won't accept all recovered events\n");
        }
        status = et_station_nwrite(id, stat_id, &ordered[num_written], num_events-num_written);
      }
    }
    
    /* If events are to be redistributed between a set of parallel stations ... */
    else if (mode == ET_STATION_RESTORE_REDIST) {
      
        et_station *firstParallel, *prev;
        et_list *pl;

        /* unmap mem of temp events if I'm the user that mapped it */
        if (id->cleanup != 1) {
            for (i=0; i < num_events ; i++) {
                if (ordered[i]->temp == ET_EVENT_TEMP) {
                    munmap(ordered[i]->pdata, (size_t) ordered[i]->memsize);
                    if (id->debug >= ET_DEBUG_INFO) {
                        et_logmsg("INFO", "et_restore_events, unmap tmp event %s\n",
                                ordered[i]->filename);
                    }
                }
            }
        }
        for (i=0; i < num_events ; i++) {
            ordered[i]->datastatus = ET_DATA_POSSIBLY_CORRUPT;
        }

        /* Find the previous station by finding the first parallel station
         * in this group. The station before that in the main linked list
         * will our "prev" station.
         */
        firstParallel = ps;
        while (firstParallel->prevparallel > -1) {
            firstParallel = id->grandcentral + firstParallel->prevparallel;
        }
        prev = id->grandcentral + firstParallel->prev;

        /* get its output list */
        pl = &prev->list_out;

        /* Just write events into the previous station's output list.
         * Statistics of this station may get messed up a bit if
         * events end up coming thru a second time. Oh well.
         * 4/21/2008 - Timmer
         */
        status = et_restore(id, pl, ordered, num_events, NULL);
        if (id->debug >= ET_DEBUG_INFO) {
            et_logmsg("INFO", "et_restore_events, restored %d events to %s's output list\n",
                    num_written, prev->name);
        }
    }

    else {
      status = ET_ERROR;
    }
  } /* if (num_events > 0) */
  
  /* free up allocated memory */
  free(pevent); free(ordered); free(new);
  
  return status;
}

