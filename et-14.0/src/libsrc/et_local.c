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
 *      Routines for local implementations of opening ET systems and dealing
 *	with heartbeats.
 *
 *----------------------------------------------------------------------------*/
 

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef sun
#include <thread.h>
#endif

#include "et_private.h"
#include "et_network.h"

/* Time intervals to wait in seconds for threads to start */
#define ET_WAIT_HEARTBEAT  10
#define ET_WAIT_HEARTMON   10

/* prototypes */
static void *et_heartbeat(void *arg);
static void *et_heartmonitor(void *arg);
static int   et_start_heartbeat(et_id *id);
static int   et_start_heartmonitor(et_id *id);
static void  et_stop_heartbeat(et_id *id);
static void  et_stop_heartmonitor(et_id *id);

/******************************************************/
int etl_open(et_sys_id *id, const char *filename, et_openconfig openconfig)
{     
  et_open_config *config = (et_open_config *) openconfig;
  et_mem   etInfo;
  et_id   *etid;
  pid_t    my_pid;
  struct timespec heartbeat;
  int      i, err, status, my_index, systemType, byteOrder, correctEndian=0;
  char     buffer[20], *pSharedMem;
#ifdef sun
  int      con;
#endif
  
  /* system id */
  etid = (et_id *) *id;
  
  /* attach to mapped memory */
  err = et_mem_attach(filename, (void **)&pSharedMem, &etInfo);
  if (err != ET_OK) {
      if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "etl_open: cannot attach to ET system file\n");
      }
      return err;
  }

  /* size of mapped memory */
  etid->memsize = (size_t) etInfo.totalSize;
  etid->pmap    = (void *)      (pSharedMem);
  etid->sys     = (et_system *) (pSharedMem + ET_INITIAL_SHARED_MEM_DATA_BYTES);

  if (etid->nselects != etid->sys->nselects) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etl_open: ET system & user have incompatible values for ET_STATION_SELECT_INTS\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR;
  }
  
  etid->stats     = (et_station *) (etid->sys    + 1);
  etid->histogram = (int *)        (etid->stats  + etid->sys->config.nstations);
  etid->events    = (et_event *)   (etid->histogram + (etid->sys->config.nevents + 1));
  etid->data      = (char *)       (etid->events + etid->sys->config.nevents);
  etid->grandcentral = etid->stats;

  /* Here comes the tricky part, we must translate pointers stored
   * in the mapped memory to the local address space. Find the offset.
   * Subtracting 2 ptrs gives an integer value of the number of objects
   * between them of pointer type ("char" in this case).
   */
  etid->offset = (ptrdiff_t) ((char *)etid->pmap - (char *)etid->sys->pmap);
  
  /* At this point we have access to ET system information. Before doing
   * anything else, check to see if this process has already opened the
   * ET system previously. If it has, quit and return an error. There
   * is no need for a process to map the memory into its space more
   * than once. This check will only work on Linux if this thread
   * was the one that previously opened the ET system as Linux idiotically
   * assigns different pid's for different threads.
   */
  my_pid = getpid();
  for (i=0; i < etid->sys->config.nprocesses; i++) {
    if ((etid->sys->proc[i].num > -1) &&
        (etid->sys->proc[i].pid == my_pid)) {
      /* already have ET system opened */
      if (etid->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "etl_open: each process can open an ET system only once!\n");
      }
      munmap(etid->pmap, etid->memsize);
      return ET_ERROR;
    }
  }

  /* Take care of 64 / 32 bit issues */
  etid->bit64 = ET_GET_BIT64(etid->sys->bitInfo);
  /* if we're 64 bit ... */
#ifdef _LP64
  /* Cannot connect to 32 bit ET system if we're 64 bits */
  if (etid->bit64 == 0) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etl_open: ET system is 32 bit and this program is 64 bit!\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR;    
  }
  /* if we're 32 bit ... */
#else
  /* Cannot connect to 64 bit ET system if we're 32 bits */
  if (etid->bit64) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etl_open: ET system is 64 bit and this program is 32 bit!\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR;    
  }
#endif

  /* Add data to id - locality, # of events & size, host & port of ET system */
  etid->locality = ET_LOCAL;
  etid->nevents  = etid->sys->config.nevents;
  etid->esize    = etid->sys->config.event_size;
  etid->port     = etid->sys->port;
  strcpy(etid->ethost, etid->sys->host);
 
  if (etid->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "etl_open, offset   : val = %d\n", etid->offset);
    et_logmsg("INFO", "etl_open, ET map   : ptr = %p\n", etid->pmap);
    et_logmsg("INFO", "etl_open, ET sys   : ptr = %p\n", etid->sys);
    et_logmsg("INFO", "etl_open, ET stats : ptr = %p\n", etid->stats);
    et_logmsg("INFO", "etl_open, ET histo : ptr = %p\n", etid->histogram);
    et_logmsg("INFO", "etl_open, ET events: ptr = %p\n", etid->events);
    et_logmsg("INFO", "etl_open, ET data  : ptr = %p\n", etid->data);
  }
    
  heartbeat.tv_sec  = ET_BEAT_SEC;
  heartbeat.tv_nsec = ET_BEAT_NSEC;
  
  /* wait for ET system to start running */
  if (config->wait == ET_OPEN_WAIT) {
    status = et_wait_for_system(*id, &config->timeout, filename);
  }
  /* wait 1 heartbeat minimum, to see if ET system is running */
  else {
    status = et_wait_for_system(*id, &heartbeat, filename);
  }
  
  if (status != ET_OK) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etl_open: ET system is not active\n");
    }
    munmap(etid->pmap, etid->memsize);
    return status;
  }
  
  /*
   * once ET system is up, find this process a unique indentifier
   * that will also serve as an index into the etid->sys->proc[]
   * array where data concerning it will be stored. We must check
   * to see if there's room for us and other processes haven't used
   * up all the array elements.
   */
  my_index = -1;
  et_system_lock(etid->sys);
  
  for (i=0; i < etid->sys->config.nprocesses; i++) {
    if (etid->sys->proc[i].num == -1) {
      my_index = i;
      if (etid->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "etl_open: proc id = %d\n", i);
      }
      break;
    }
  }
  
  if (my_index == -1) {
    et_system_unlock(etid->sys);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etl_open: cannot add more processes to this ET system\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR;
  }
    
  etid->sys->nprocesses++;
  etid->proc = my_index;
  etid->sys->proc[my_index].num = my_index;
  etid->sys->proc[my_index].pid = my_pid;
  etid->sys->proc[my_index].et_status = ET_PROC_ETOK;
  et_system_unlock(etid->sys);
  
#ifdef sun
    con = thr_getconcurrency();
    if (con < 1) {
      con = 1;
    }
    thr_setconcurrency(con + ET_EXTRA_THREADS);
#endif
  
  if ((status = et_start_heartbeat(etid)) != ET_OK) {
    et_system_lock(etid->sys);
    et_init_process(etid->sys, my_index);
    etid->sys->nprocesses--;
    et_system_unlock(etid->sys);
    munmap(etid->pmap, etid->memsize);
    return status;
  }
  
  if ((status = et_start_heartmonitor(etid)) != ET_OK) {
    et_system_lock(etid->sys);
    et_init_process(etid->sys, my_index);
    etid->sys->nprocesses--;
    et_system_unlock(etid->sys);
    et_stop_heartbeat(etid);
    munmap(etid->pmap, etid->memsize);
    return status;
  }
  
  /*
   * Wait until heartbeat is started before declaring the ET system
   * open. Otherwise the system's heartmonitor may find it's open
   * and kill it because there's no heartbeat.
   */
  et_system_lock(etid->sys);
  etid->sys->proc[my_index].status = ET_PROC_OPEN;
  et_system_unlock(etid->sys);
   
  return ET_OK;
}

/******************************************************
 * A mini version of etl_open that allows one to look
 * at an ET system's shared memory but doesn't open
 * the system for full use. Used for monitoring.
 ******************************************************/
int et_look(et_sys_id *id, const char *filename)
{     
  et_id   *etid;
  char     buffer[20], *pSharedMem;
  int      err, byteOrder, systemType, correctEndian=0;
  et_mem   etInfo;

  /* argument checking */
  if (filename == NULL) {
      et_logmsg("ERROR", "et_look, bad ET name\n");
      return ET_ERROR;
  }
  else if (strlen(filename) > ET_FILENAME_LENGTH - 1) {
      et_logmsg("ERROR", "et_look, ET name too long\n");
      return ET_ERROR;
  }
  
  /* initialize system id */
  et_id_init(id);
  etid = (et_id *) *id;
      
  /* attach to mapped memory */
  err = et_mem_attach(filename, (void **)&pSharedMem, &etInfo);
  if (err != ET_OK) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_look: cannot attach to ET system file\n");
    }
    et_id_destroy(*id);
    return err;
  }
  
  /* size of mapped memory */
  etid->memsize = (size_t) etInfo.totalSize;
  etid->pmap    = (void *)       (pSharedMem);
  etid->sys     = (et_system *)  (pSharedMem + ET_INITIAL_SHARED_MEM_DATA_BYTES);
  
  /* Stop here and check to see if the ET system version and our
   * version of the ET software is the same or not.
   */
  if (etid->version != etid->sys->version) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_look: ET system & user's ET versions are different\n");
    }
    munmap(etid->pmap, etid->memsize);
    et_id_destroy(*id);
    return ET_ERROR;
  }
  if (etid->nselects != etid->sys->nselects) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_look: ET system & user have incompatible values for ET_STATION_SELECT_INTS\n");
    }
    munmap(etid->pmap, etid->memsize);
    et_id_destroy(*id);
    return ET_ERROR;
  }
  
  etid->stats     = (et_station *) (etid->sys    + 1);
  etid->histogram = (int *)        (etid->stats  + etid->sys->config.nstations);
  etid->events    = (et_event *)   (etid->histogram + (etid->sys->config.nevents + 1));
  etid->data      = (char *)       (etid->events + etid->sys->config.nevents);
  etid->grandcentral = etid->stats;
  etid->offset = (ptrdiff_t) ((char *)etid->pmap - (char *)etid->sys->pmap);
  
  if (etid->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_look, offset   : val = %d\n", etid->offset);
    et_logmsg("INFO", "et_look, ET map   : ptr = %p\n", etid->pmap);
    et_logmsg("INFO", "et_look, ET sys   : ptr = %p\n", etid->sys);
    et_logmsg("INFO", "et_look, ET stats : ptr = %p\n", etid->stats);
    et_logmsg("INFO", "et_look, ET histo : ptr = %p\n", etid->histogram);
    et_logmsg("INFO", "et_look, ET events: ptr = %p\n", etid->events);
    et_logmsg("INFO", "et_look, ET data  : ptr = %p\n", etid->data);
  }
    
  /* Add data to id - locality, # of events & size, host & port of ET system */
  etid->nevents  = etid->sys->config.nevents;
  etid->esize    = etid->sys->config.event_size;
  etid->port     = etid->sys->port;
  strcpy(etid->ethost, etid->sys->host);
  if (etid->share == ET_MUTEX_SHARE) {
    etid->locality = ET_LOCAL;
  }
  else {
    etid->locality = ET_LOCAL_NOSHARE;
  }
     
  return ET_OK;
}

/******************************************************/
int et_unlook(et_sys_id id)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality == ET_REMOTE) {
    return ET_ERROR;
  }
     
  if (munmap(etid->pmap, etid->memsize) != 0) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_unlook, cannot unmap ET memory\n");
    }
  }
  et_id_destroy(id);
 
  return ET_OK;
}

/******************************************************/
int etl_close(et_sys_id id)
{
  et_id *etid = (et_id *) id;
  int i;
#ifdef sun
  int con;
#endif

  /* Don't allow simultaneous access of shared mem as we're about to unmap it */
  et_memWrite_lock(etid);

  /* Record that fact that close has been called so no more access of shared mem */
  etid->closed = 1;
  
  /* ET system must call et_system_close, not et_close */
  if (etid->proc == ET_SYS) {
    et_mem_unlock(etid);
    if (etid->debug >= ET_DEBUG_WARN) {
      et_logmsg("WARN", "et_close, calling et_system_close instead for ET system process\n");
    }
    return et_system_close(id);
  }
  
  if (et_alive(id)) {
    /* check for this process' attachments to stations */
    for (i=0; i < etid->sys->config.nattachments; i++) {
      if (etid->sys->proc[etid->proc].att[i] != -1) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "et_close, detach from all stations first\n");
        }
        return ET_ERROR;
      }
    }

    et_system_lock(etid->sys);
    etid->sys->nprocesses--;
    et_init_process(etid->sys, etid->proc);
    /* if we crash right here, system mutex is permanently locked */
    et_system_unlock(etid->sys);
  }
  else {
    etid->sys->nprocesses--;
    et_init_process(etid->sys, etid->proc);
  }
  
  #ifdef sun
    con = thr_getconcurrency();
    if (con < ET_EXTRA_THREADS + 1) {
      con = 1;
    }
    else {
      con = con - ET_EXTRA_THREADS;
    }
    thr_setconcurrency(con);
  #endif
  
  et_stop_heartmonitor(etid);
  et_stop_heartbeat(etid);
  
  if (munmap(etid->pmap, etid->memsize) != 0) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_close, cannot unmap ET memory\n");
    }
  }
  
  et_mem_unlock(etid);
  et_id_destroy(id);
 
  return ET_OK;
}

/******************************************************/
int etl_forcedclose(et_sys_id id)
{
  int i, status;
  et_id *etid = (et_id *) id;
 
  /* ET system must call et_system_close, not et_forcedclose */
  if (etid->proc == ET_SYS) {
    if (etid->debug >= ET_DEBUG_WARN) {
      et_logmsg("WARN", "et_forcedclose, calling et_system_close instead for ET system process\n");
    }
    return et_system_close(id);
  }
  
  if (et_alive(id)) {
    /* check for this process' attachments to stations */
    for (i=0; i < etid->sys->config.nattachments; i++) {
      if (etid->sys->proc[etid->proc].att[i] != -1) {
        if (etid->debug >= ET_DEBUG_INFO) {
          et_logmsg("INFO", "et_forcedclose, detach %d\n", i);
        }
        et_station_detach(id, i);
      }
    }
  }
  
  return et_close(id);
}

/******************************************************/
int etl_kill(et_sys_id id)
{
    int i, status;
    et_id *etid = (et_id *) id;

    /* If the ET server got a command from a client to kill itself ... */
    if (etid->proc == ET_SYS) {
        et_system_lock(etid->sys);
        /* Set the magic bit that the hearbeat thread looks for.
         * That thread will kill us. */
        etid->sys->bitInfo = ET_SET_KILL(etid->sys->bitInfo);
        et_system_unlock(etid->sys);
        return ET_OK;
    }
    
    if (et_alive(id)) {
        /* check for this process' attachments to stations */
        for (i=0; i < etid->sys->config.nattachments; i++) {
            if (etid->sys->proc[etid->proc].att[i] != -1) {
                if (etid->debug >= ET_DEBUG_INFO) {
                    et_logmsg("INFO", "et_kill, detach %d\n", i);
                }
                et_station_detach(id, i);
            }
        }
    }
  
    /* Don't allow simultaneous access of shared mem as we're about to unmap it */
    et_memWrite_lock(etid);

    /* Record that fact that close has been called so no more access of shared mem */
    etid->closed = 1;
  
    if (et_alive(id)) {
        et_system_lock(etid->sys);
        etid->sys->bitInfo = ET_SET_KILL(etid->sys->bitInfo);
        etid->sys->nprocesses--;
        et_init_process(etid->sys, etid->proc);
        /* if we crash right here, system mutex is permanently locked */
        et_system_unlock(etid->sys);
    }
    else {
        etid->sys->bitInfo = ET_SET_KILL(etid->sys->bitInfo);
        etid->sys->nprocesses--;
        et_init_process(etid->sys, etid->proc);
    }
  
#ifdef sun
    con = thr_getconcurrency();
    if (con < ET_EXTRA_THREADS + 1) {
        con = 1;
    }
    else {
        con = con - ET_EXTRA_THREADS;
    }
    thr_setconcurrency(con);
#endif
  
    et_stop_heartmonitor(etid);
    et_stop_heartbeat(etid);
  
    if (munmap(etid->pmap, etid->memsize) != 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_close, cannot unmap ET memory\n");
        }
    }
  
    et_mem_unlock(etid);
    et_id_destroy(id);
 
    return ET_OK;
}

/*****************************************************/
/*                  HEARTBEAT STUFF                  */
/*****************************************************/

int etl_alive(et_sys_id id)
{
  et_id *etid = (et_id *) id;
  return etid->alive;
}

/******************************************************/
int etl_wait_for_alive(et_sys_id id)
{
  struct timespec sleeptime;
  
  sleeptime.tv_sec  = 0;
  sleeptime.tv_nsec = 10000000;

  while (!et_alive(id)) {
    nanosleep(&sleeptime, NULL);
  }
  return ET_OK;
} 

/******************************************************
 * This routine is called either from etl_open or
 * etn_open. Both deal with local ET systems.
 *
 * If timeout == NULL or 0, wait forever
 ******************************************************/
int et_wait_for_system(et_sys_id id, struct timespec *timeout, const char *etname)
{
  int oldheartbt, newheartbt, wait_forever=0, init=1;
  struct timespec sleeptime;
  et_id *etid = (et_id *) id;
  double increment, totalwait = 1.;
  
  /* Before going thru the time of waiting for a heartbeat
   * time period to see if ET is alive, first send a UDP
   * packet to the thread waiting for such. If it's alive,
   * it'll respond. If it doesn't respond, look for the
   * heartbeat.
   */
  if (et_responds(etname) == 1) {
    etid->alive = 1;
    return ET_OK;
  }
  
  sleeptime.tv_sec  = ET_BEAT_SEC;
  sleeptime.tv_nsec = ET_BEAT_NSEC;
  increment = sleeptime.tv_sec + 1.e-9*(sleeptime.tv_nsec);
  
  /* If "timeout" NULL or set to 0, wait indefinitely */
  if ((timeout == NULL) ||
      ((timeout->tv_sec == 0) && (timeout->tv_nsec == 0))) {
    wait_forever = 1;
  }
  else {
    totalwait = timeout->tv_sec  + 1.e-9*(timeout->tv_nsec);
  }
  
/* printf("et_wait_for_system: alive = %d\n", etid->alive); */
  if (etid->alive != 1) {
    /* We need to wait for ET system to start up for very */
    /* first time before our application can do stuff.    */
    oldheartbt = etid->sys->heartbeat;
/* printf("et_wait_for_system: old heartbeat = %d\n", oldheartbt); */
    nanosleep(&sleeptime, NULL);
    if (!wait_forever) {
      totalwait -= increment;
    }
    
    while (1) {
      newheartbt = etid->sys->heartbeat;
/* printf("et_wait_for_system: new heartbeat = %d\n", newheartbt); */
      if (oldheartbt != newheartbt) {
        etid->alive = 1;
        break;
      }
      if (init) {
        if (etid->debug >= ET_DEBUG_INFO) {
          et_logmsg("INFO", "et_wait_for_system, waiting for initial heartbeat\n");
	}
	init--;
      }
      if (totalwait < 0.) {
        if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "et_wait_for_system, done waiting but ET system not alive\n");
	}
	return ET_ERROR_TIMEOUT;
      }
      
      nanosleep(&sleeptime, NULL);
      if (!wait_forever) {
        totalwait -= increment;
      }
    }
  }
  
  if (!init) {
    if (etid->debug >= ET_DEBUG_INFO) {
      et_logmsg("INFO", "et_wait_for_system, system is ready\n");
    }
  }
  
  return ET_OK;
} 

/******************************************************/
static void *et_heartbeat(void *arg)
{
  et_id          *id = (et_id *) arg;
  et_proc_id      me = id->proc;
  struct timespec timeout;
  const int       forever = 1;
  
  timeout.tv_sec  = ET_BEAT_SEC;
  timeout.tv_nsec = ET_BEAT_NSEC;
   
  /* signal to spawning thread that I'm running */
  id->race = -1;
  
  while (forever) {
    id->sys->proc[me].heartbeat = (id->sys->proc[me].heartbeat + 1) % ET_HBMODULO;
    /*printf(" HB%d\n", me); fflush(stdout);*/
    nanosleep(&timeout, NULL);
    pthread_testcancel();
  }
  return (NULL);
}

/******************************************************/
static void *et_heartmonitor(void *arg)
{
  et_id          *id  = (et_id *) arg;
  et_proc_id      me  = id->proc;
  et_system      *sys = id->sys;
  et_station     *ps;
  et_list        *pl;
  et_stat_id      stat;
  struct timespec timeout;
  int             disconnected=0, oldheartbt=-1, newheartbt;
  int             i;
  const int       forever = 1;


  timeout.tv_sec  = ET_MON_SEC;
  timeout.tv_nsec = ET_MON_NSEC;
    
  /* signal to spawning thread that I'm running */
  id->race = -1;
 
  while (forever) {    
    nanosleep(&timeout, NULL);
    pthread_testcancel();
    newheartbt = sys->heartbeat;
/*printf("et_heartmon %d: mine = %d, sys = %d\n", me, sys->proc[me].heartbeat, newheartbt);*/
     
    if (oldheartbt == newheartbt) {
      if (!disconnected) {
        if (id->debug >= ET_DEBUG_WARN) {
          et_logmsg("WARN", "et_heartmon %d, et system is dead - waiting\n", me);
        }
	id->alive = 0;
        sys->proc[me].et_status = ET_PROC_ETDEAD;
	
	/* scan the process' attachments, fix mutexes, unblock read calls */
        for (i=0; i < sys->config.nattachments; i++) {
	  if (sys->proc[me].att[i] != -1) {
            /* check for locked mutex in station's input and output llists */
	    /*ps = ET_PSTAT2USR(sys->attach[att_id].pstat, id->offset);*/
	    stat = sys->attach[i].stat;
	    ps = id->grandcentral + stat;
            pl = &(ps->list_out);
            if (et_mutex_locked(&pl->mutex) == ET_MUTEX_LOCKED) {
              if (id->debug >= ET_DEBUG_INFO) {
                et_logmsg("INFO", "et_heartmon %d, out list locked\n", me);
              }
	      et_llist_unlock(pl);
            }
       
            pl = &(ps->list_in);
            if (et_mutex_locked(&pl->mutex) == ET_MUTEX_LOCKED) {
              if (id->debug >= ET_DEBUG_INFO) {
                et_logmsg("INFO", "et_heartmon %d, in list locked\n", me);
              }
	      et_llist_unlock(pl);
            }
	    /* waking up attachment unblocks any read calls */
            et_wakeup_attachment((et_sys_id) id, i);
	  }
	}
        disconnected = 1;
      }
    }
    else {
      oldheartbt = newheartbt;
      if (disconnected) {
        if (id->debug >= ET_DEBUG_INFO) {
          et_logmsg("INFO", "et_heartmon %d, reconnect!!\n", me);
        }
	sys->proc[me].et_status = ET_PROC_ETOK;
        /* When reconnecting, the mem mapped file in the ET system may be
	 * mapped to a different location in memory than in the previous
	 * system. Thus, we need correct the offset.
	 */
        id->offset = (ptrdiff_t) ((char *)id->pmap - (char *)id->sys->pmap);
        id->alive = 1;
        disconnected = 0;
      }
    }
  }
  return (NULL);
}

/******************************************************/
/* spawn thread to handle heartbeat */
static int et_start_heartbeat(et_id *id)
{ 
  int status, num_try=0, try_max;
  pthread_attr_t attr;
  struct timespec waitforme;
 
  waitforme.tv_sec  = 0;
  if (id->sys->hz < 2) {
    waitforme.tv_nsec = 10000000; /* 10 millisec */
  }
  else {
    waitforme.tv_nsec = 1000000000/id->sys->hz;
  }

  /* wait for ET_WAIT_HEARTBEAT seconds */
  try_max = (id->sys->hz)*ET_WAIT_HEARTBEAT;

  status = pthread_attr_init(&attr);
  if(status != 0) {
    err_abort(status, "Thread attr init");
  }
   
  status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if(status != 0) {
    err_abort(status, "Thread attr init");
  }
  
  status = pthread_create(&id->sys->proc[id->proc].hbeat_thd_id,
                          &attr, et_heartbeat, (void *) id);
  if(status != 0) {
    err_abort(status, "Start heartbeat");
  }
  
  status = pthread_attr_destroy(&attr);
  if(status != 0) {
    err_abort(status, "Thread attr destroy");
  }
  
  /*
   * Wait for heartbeat thread to start before returning,
   * this should eliminate any possible race conditions.
   * Wait for id->race == -1.
   */
  while((id->race != -1)&&(num_try++ < try_max)) {
    nanosleep(&waitforme, NULL);
  }
  id->race = 0;
  
  if (num_try > try_max) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_start_heartbeat, did NOT start\n");
    }
    return ET_ERROR;
  }
  
  if (id->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_start_heartbeat, started\n");
  }
  
  return ET_OK;
}
  
/******************************************************/
/* spawn thread to handle ET death detection */
static int et_start_heartmonitor(et_id *id)
{
  int status, num_try=0, try_max;
  pthread_attr_t attr;
  struct timespec waitforme;
 
  waitforme.tv_sec  = 0;
  if (id->sys->hz < 2) {
    waitforme.tv_nsec = 10000000; /* 10 millisec */
  }
  else {
    waitforme.tv_nsec = 1000000000/id->sys->hz;
  }

  /* wait for ET_WAIT_HEARTMON seconds */
  try_max = (id->sys->hz)*ET_WAIT_HEARTMON;
  
  status = pthread_attr_init(&attr);
  if(status != 0) {
    err_abort(status, "Thread attr init");
  }
  
  status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if(status != 0) {
    err_abort(status, "Thread attr init");
  }
  
  status = pthread_create(&id->sys->proc[id->proc].hmon_thd_id,
                          &attr, et_heartmonitor, (void *) id);
  if(status != 0) {
    err_abort(status, "Start heartbeat monitor");
  }
  
  status = pthread_attr_destroy(&attr);
  if(status != 0) {
    err_abort(status, "Thread attr destroy");
  }
  /*
   * Wait for heartmon thread to start before returning,
   * this should eliminate any possible race conditions.
   * Wait for id->race == -1.
   */
  while((id->race != -1) && (num_try++ < try_max)) {
    nanosleep(&waitforme, NULL);
  }
  id->race = 0;
  
  if (num_try > try_max) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_start_heartmonitor, did NOT start\n");
    }
    return ET_ERROR;
  }
  
  if (id->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "et_start_heartmonitor, started\n");
  }
  
  return ET_OK;
}
  
/******************************************************/
/* cancel heartbeat thread */
static void et_stop_heartbeat(et_id *id)
{
  int status;
  
  status = pthread_cancel(id->sys->proc[id->proc].hbeat_thd_id);
  if (status != 0) {
    err_abort(status, "Cancel heartbeat thread");
  }
  return;
}
  
/******************************************************/
/* cancel heart monitor thread */
static void et_stop_heartmonitor(et_id *id)
{
  int status;
  
  status = pthread_cancel(id->sys->proc[id->proc].hmon_thd_id);
  if (status != 0) {
    err_abort(status, "Cancel heart monitor thread");
  }
  return;
}

