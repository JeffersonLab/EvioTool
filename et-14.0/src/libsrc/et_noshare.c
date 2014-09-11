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
 *      Routines for operating systems like Linux that cannot share mutexes
 *	between different Unix processes.
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "et_private.h"
#include "et_network.h"

/*
 * For a Linux local open, need to map the ET system memory AND
 * open up a network connection to the server which the ET system
 * is running. Thus, the server handles all the mutex and condition
 * variable stuff, and the application can read the data right
 * from the mapped memory. Voila, a local ET system on Linux.
 */
int etn_open(et_sys_id *id, const char *filename, et_openconfig openconfig)
{     
  et_open_config *config = (et_open_config *) openconfig;
  et_mem etInfo;
  et_id *etid;
  struct timespec heartbeat;
  int status, sockfd, length, bufsize, version, nselects;
  int systemType, byteOrder, correctEndian=0;
  int err=ET_OK, transfer[8], incoming[9];
  char *buf, *pbuf, *pSharedMem;
  char  buffer[20];
  
  /* system id */
  etid = (et_id *) *id;
    
  /* attach to mapped memory */
  err = et_mem_attach(filename, (void **)&pSharedMem, &etInfo);
  if (err != ET_OK) {
      if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "etn_open: cannot attach to ET system file\n");
      }
      return err;
  }

  /* size of mapped memory */
  etid->memsize = (size_t) etInfo.totalSize;
  etid->pmap    = (void *)       (pSharedMem);
  etid->sys     = (et_system *)  (pSharedMem + ET_INITIAL_SHARED_MEM_DATA_BYTES);
  
  if (etid->nselects != etid->sys->nselects) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open: ET system & user have incompatible values for ET_STATION_SELECT_INTS\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR;
  }
  
  etid->stats     = (et_station *) (etid->sys    + 1);
  etid->histogram = (int *)        (etid->stats  + etid->sys->config.nstations);
  etid->events    = (et_event *)   (etid->histogram + (etid->sys->config.nevents + 1));
  etid->data      = (char *)       (etid->events + etid->sys->config.nevents);
  etid->grandcentral = etid->stats;
  etid->offset = (ptrdiff_t) ((char *)etid->pmap - (char *)etid->sys->pmap);
  
  /* Take care of 64 / 32 bit issues */
  etid->bit64 = ET_GET_BIT64(etid->sys->bitInfo);
  /* if we're 64 bit ... */
#ifdef _LP64
  /* Cannot connect to 32 bit ET system if we're 64 bits */
  if (etid->bit64 == 0) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open: ET system is 32 bit and this program is 64 bit!\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR;    
  }
  /* if we're 32 bit ... */
#else
  /* Cannot connect to 64 bit ET system if we're 32 bits */
  if (etid->bit64) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open: ET system is 64 bit and this program is 32 bit!\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR;    
  }
#endif

  /* Add data to id - locality, port & host of ET system */
  etid->locality = ET_LOCAL_NOSHARE;
  etid->port     = etid->sys->port;
  strcpy(etid->ethost, etid->sys->host);
  
  if (etid->debug >= ET_DEBUG_INFO) {
    et_logmsg("INFO", "etn_open, offset   : val = %d\n", etid->offset);
    et_logmsg("INFO", "etn_open, ET map   : ptr = %p\n", etid->pmap);
    et_logmsg("INFO", "etn_open, ET sys   : ptr = %p\n", etid->sys);
    et_logmsg("INFO", "etn_open, ET stats : ptr = %p\n", etid->stats);
    et_logmsg("INFO", "etn_open, ET histo : ptr = %p\n", etid->histogram);
    et_logmsg("INFO", "etn_open, ET events: ptr = %p\n", etid->events);
    et_logmsg("INFO", "etn_open, ET data  : ptr = %p\n", etid->data);
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
      et_logmsg("ERROR", "etn_open: ET system is not active\n");
    }
    munmap(etid->pmap, etid->memsize);
    return status;
  }
     
  /***************************/
  /* Now the network portion */
  /***************************/
  
  if (etid->sys->port <= 0) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open: bad value for port\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR_REMOTE;
  }
  
  /* find client's iov_max value */
#ifndef __APPLE__
  if ( (etid->iov_max = sysconf(_SC_IOV_MAX)) == -1) {
    /* set it to POSIX minimum by default (it always bombs on Linux) */
    etid->iov_max = ET_IOV_MAX;
  }
#else
	etid->iov_max = ET_IOV_MAX;
#endif

  /* magic numbers */
  transfer[0]  = htonl(ET_MAGIC_INT1);
  transfer[1]  = htonl(ET_MAGIC_INT2);
  transfer[2]  = htonl(ET_MAGIC_INT3);
    
  /* endian */
  transfer[3]  = htonl(etid->endian);

  /* length of ET system name */
  length = strlen(filename)+1;
  transfer[4] = htonl(length);
#ifdef _LP64
  transfer[5] = 1;
#else
  transfer[5] = 0;
#endif
  /* not used */
  transfer[6] = 0;
  transfer[7] = 0;
  
  /* make the network connection */
  err = etNetTcpConnect(etid->sys->host, config->interface,
                        (unsigned short)etid->sys->port, config->tcpSendBufSize,
                        config->tcpRecvBufSize, config->tcpNoDelay, &sockfd, NULL);
  if (err != ET_OK) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open: cannot connect to server\n");
    }
    munmap(etid->pmap, etid->memsize);
    return ET_ERROR_REMOTE;
  }
  etid->sockfd = sockfd;

  /* put everything in one buffer */
  bufsize = sizeof(transfer) + length;
  if ( (pbuf = buf = (char *) malloc(bufsize)) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open, cannot allocate memory\n");
    }
    err = ET_ERROR_REMOTE;
    goto error;
  }
  memcpy(pbuf, transfer, sizeof(transfer));
  pbuf += sizeof(transfer);
  memcpy(pbuf,filename, length);
  
  /* write it to server */
  if (etNetTcpWrite(sockfd, (void *) buf, bufsize) != bufsize) {
    free(buf);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open, write error\n");
    }
    err = ET_ERROR_WRITE;
    goto error;
  }
  free(buf);
  
  /* read the return */
  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open, read error\n");
    }
    err = ET_ERROR_READ;
    goto error;
  }
  err = ntohl(err);
  if (err != ET_OK) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open: found the wrong ET system\n");
    }
    goto error;
  }
  
  if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open, read error\n");
    }
    err = ET_ERROR_READ;
    goto error;
  }
  /* ET's endian */
  etid->systemendian = ntohl(incoming[0]);
  /* ET's total number of events */
  etid->nevents = ntohl(incoming[1]);
  /* ET's max event size */
  etid->esize = ET_64BIT_UINT(ntohl(incoming[2]),ntohl(incoming[3]));
  /* ET's version number */
  version = ntohl(incoming[4]);
  /* ET's number of selection integers */
  nselects = ntohl(incoming[5]);
  /* ET's language */
  etid->lang = ntohl(incoming[6]);
  /* ET's 64 or 32 bit exe.This is here for remote apps in which 32 & 64 bit
   * apps can still talk to eachother. That's not true for us but we still
   * must follow the protocol.
   */
  etid->bit64 = ntohl(incoming[7]);
  
  if (version != etid->version) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open: ET system & user's ET versions are different\n");
    }
    err = ET_ERROR_REMOTE;
    goto error;
  }
  
  if (nselects != etid->nselects) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_open: ET system & user have incompatible values for ET_STATION_SELECT_INTS\n");
    }
    err = ET_ERROR_REMOTE;
    goto error;
  }

  return ET_OK;
  
  error:
    close(sockfd);
    munmap(etid->pmap, etid->memsize);
    return err;
}

/******************************************************/
int etn_alive(et_sys_id id)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  struct timespec waittime;
  unsigned int newheartbt, oldheartbt;
  int status=1, alive, com;
  
  /* monitor ET system's heartbeat by socket first */  
  com = htonl(ET_NET_ALIVE);
  
  /* If ET system is NOT alive, or if ET system was killed and
   * restarted (breaking tcp connection), we'll get a write error.
   */
  et_tcp_lock(etid);
  if (etNetTcpWrite(sockfd, (void *) &com, sizeof(com)) != sizeof(com)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_alive, write error\n");
    }
    status = ET_ERROR_WRITE;
  }
  if (etNetTcpRead(sockfd, &alive, sizeof(alive)) != sizeof(alive)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_alive, read error\n");
    }
    status = ET_ERROR_READ;
  }
  et_tcp_unlock(etid);

  /* if there's no communication error, return alive */
  if (status > 0) {
    return ntohl(alive);
  }
  /* Socket error, so now monitor hbeat thru shared memory.
   * Monitoring this way takes a couple seconds (1.5X the
   * time for system's heart to beat), so only do it when
   * socket communication fails
   */
  waittime.tv_sec  = 1.5*ET_BEAT_SEC;
  waittime.tv_nsec = 1.5*ET_BEAT_NSEC;
  if (waittime.tv_nsec > 1000000000L) {
    waittime.tv_nsec -= 1000000000L;
    waittime.tv_sec  += 1;
  }
    
  oldheartbt = etid->sys->heartbeat;
  nanosleep(&waittime, NULL);
  newheartbt = etid->sys->heartbeat;
  if (oldheartbt == newheartbt) {
    return 0;
  }  
  return 1;
}

/******************************************************/
int etn_wait_for_alive(et_sys_id id)
{
  struct timespec sleeptime;
  
  sleeptime.tv_sec  = 0;
  sleeptime.tv_nsec = 10000000;

  while (!etn_alive(id)) {
    nanosleep(&sleeptime, NULL);
  }
  return ET_OK;
} 

/******************************************************/
int etn_close(et_sys_id id)
{
  et_id *etid = (et_id *) id;
  
  /* unmap the shared memory */
  if (munmap(etid->pmap, etid->memsize) != 0) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_close, cannot unmap ET memory\n");
    }
    return ET_ERROR_REMOTE;
  }
  
  /* close network connection & free mem in "id" */
  return etr_close(id);
}

/******************************************************/
int etn_forcedclose(et_sys_id id)
{
  et_id *etid = (et_id *) id;
  
  /* unmap the shared memory */
  if (munmap(etid->pmap, etid->memsize) != 0) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_forcedclose, cannot unmap ET memory\n");
    }
    return ET_ERROR_REMOTE;
  }
  
  /* close network connection & free mem in "id" */
  return etr_forcedclose(id);
}

/******************************************************/
int etn_kill(et_sys_id id)
{
    et_id *etid = (et_id *) id;
  
    /* unmap the shared memory */
    if (munmap(etid->pmap, etid->memsize) != 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etn_kill, cannot unmap ET memory\n");
        }
        return ET_ERROR_REMOTE;
    }
  
    /* kill ET system */
    return etr_kill(id);
}

/******************************************************/
/*                   ET EVENT STUFF                   */
/******************************************************/
int etn_event_new(et_sys_id id, et_att_id att, et_event **ev,
		 int mode, struct timespec *deltatime, size_t size)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  int err, transfer[7], incoming[3];
  void *pdata, *p;
  
  /*
   * The command to the server is swapped so make sure it's sent in the
   * right byte order. The rest of the items are not swapped by the code.
   */
  transfer[0] = htonl(ET_NET_EV_NEW_L);
  transfer[1] = att;
  transfer[2] = mode;
  transfer[3] = ET_HIGHINT(size);
  transfer[4] = ET_LOWINT(size);
  transfer[5] = 0;
  transfer[6] = 0;
  if (deltatime) {
    transfer[5] = deltatime->tv_sec;
    transfer[6] = deltatime->tv_nsec;
  }
 
  et_tcp_lock(etid);
  if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_event_new, write error\n");
    }
    return ET_ERROR_WRITE;
  }
 
  if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_event_new, read error\n");
    }
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);

  if ( (err = incoming[0]) != ET_OK) {
    return err;
  }
  
  /* Pointers (may be 64 bits) are in ET system space and must be translated.
   * The following ifdef avoids compiler warnings.
   */
#ifdef _LP64
  p   = (void *) ET_64BIT_P(incoming[1],incoming[2]);
#else
  p   = (void *) incoming[2];
#endif
  *ev = ET_PEVENT2USR(p, etid->offset);
  
  /* if not a temp event, data is in mem already mmapped */
  if ((*ev)->temp != ET_EVENT_TEMP) {
    /*(*ev)->pdata = ET_PDATA2USR((*ev)->pdata, etid->offset);*/
    (*ev)->pdata = ET_PDATA2USR((*ev)->data, etid->offset);
  }
  /* else, mmap file so we can get at data */
  else {
    /* store ET system's temp data pointer so it doesn't get lost */
    (*ev)->tempdata = (*ev)->pdata;
    /* attach to temp event mem */
    if ((pdata = et_temp_attach((*ev)->filename, (*ev)->memsize)) == NULL) {
      if (etid->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "etn_event_new, cannot attach to temp event\n");
      }
      return ET_ERROR_REMOTE;
    }
    (*ev)->pdata = pdata;
  }

  return ET_OK;
}

/******************************************************/
int etn_events_new(et_sys_id id, et_att_id att, et_event *evs[],
		 int mode, struct timespec *deltatime,
		 size_t size, int num, int *nread)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  int i, nevents, err, transfer[8];
  void *pdata;
  
  transfer[0] = htonl(ET_NET_EVS_NEW_L);
  transfer[1] = att;
  transfer[2] = mode;
  transfer[3] = ET_HIGHINT(size);
  transfer[4] = ET_LOWINT(size);
  transfer[5] = num;
  transfer[6] = 0;
  transfer[7] = 0;
  
  if (deltatime) {
    transfer[6] = deltatime->tv_sec;
    transfer[7] = deltatime->tv_nsec;
  }
 
  et_tcp_lock(etid);
  if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_new, write error\n");
    }
    return ET_ERROR_WRITE;
  }
 
  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_new, read error\n");
    }
    return ET_ERROR_READ;
  }

  if (err != ET_OK) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_new, error in server\n");
    }
    return err;
  }
  
  nevents = err;
  
  /* The following should be independent of whether 64 or 32 bit code.
   * Take advantage of the fact that in local Linux mode, the sizeof
   * a pointer is the same here in client as it is in server.
   * If it weren't, the client would never have been able to do an
   * et_open which checks to see if it's 64 or 32 bit file.
   */
  size = nevents*sizeof(et_event *);  /* double use of size */
  
  if (etNetTcpRead(sockfd, (void *) evs, size) != size) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_new, read error\n");
    }
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);
  
  /* pointers in ET system space and must be translated */
  for (i=0; i < nevents; i++) {
    evs[i] = ET_PEVENT2USR(evs[i], etid->offset);
    
    /* if not a temp event, data is in mem already mmapped */
    if (evs[i]->temp != ET_EVENT_TEMP) {
      /*evs[i]->pdata = ET_PDATA2USR(evs[i]->pdata, etid->offset);*/
      evs[i]->pdata = ET_PDATA2USR(evs[i]->data, etid->offset);
    }
    /* else, mmap file so we can get at data */
    else {
      /* store ET system's temp data pointer so it doesn't get lost */
      evs[i]->tempdata = evs[i]->pdata;
      /* attach to temp event mem */
      if ((pdata = et_temp_attach(evs[i]->filename, evs[i]->memsize)) == NULL) {
	if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "etn_events_new, cannot attach to temp event\n");
	}
	return ET_ERROR_REMOTE;
      }
      evs[i]->pdata = pdata;
    }
  }
  
  *nread = nevents;
  return ET_OK;
}

/******************************************************/
int etn_events_new_group(et_sys_id id, et_att_id att, et_event *evs[],
         int mode, struct timespec *deltatime,
         size_t size, int num, int group, int *nread)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  int i, nevents, err, transfer[9];
  void *pdata;
  
  transfer[0] = htonl(ET_NET_EVS_NEW_GRP_L);
  transfer[1] = att;
  transfer[2] = mode;
  transfer[3] = ET_HIGHINT(size);
  transfer[4] = ET_LOWINT(size);
  transfer[5] = num;
  transfer[6] = group;
  transfer[7] = 0;
  transfer[8] = 0;
  
  if (deltatime) {
    transfer[7] = deltatime->tv_sec;
    transfer[8] = deltatime->tv_nsec;
  }
 
  et_tcp_lock(etid);
  if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_new, write error\n");
    }
    return ET_ERROR_WRITE;
  }
 
  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_new, read error\n");
    }
    return ET_ERROR_READ;
  }

  if (err != ET_OK) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_new, error in server\n");
    }
    return err;
  }
  
  nevents = err;
  
  /* The following should be independent of whether 64 or 32 bit code.
   * Take advantage of the fact that in local Linux mode, the sizeof
   * a pointer is the same here in client as it is in server.
   * If it weren't, the client would never have been able to do an
   * et_open which checks to see if it's 64 or 32 bit file.
   */
  size = nevents*sizeof(et_event *);  /* double use of size */
  
  if (etNetTcpRead(sockfd, (void *) evs, size) != size) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_new, read error\n");
    }
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);
  
  /* pointers in ET system space and must be translated */
  for (i=0; i < nevents; i++) {
    evs[i] = ET_PEVENT2USR(evs[i], etid->offset);
    
    /* if not a temp event, data is in mem already mmapped */
    if (evs[i]->temp != ET_EVENT_TEMP) {
      evs[i]->pdata = ET_PDATA2USR(evs[i]->data, etid->offset);
    }
    /* else, mmap file so we can get at data */
    else {
      /* store ET system's temp data pointer so it doesn't get lost */
      evs[i]->tempdata = evs[i]->pdata;
      /* attach to temp event mem */
      if ((pdata = et_temp_attach(evs[i]->filename, evs[i]->memsize)) == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "etn_events_new, cannot attach to temp event\n");
        }
        return ET_ERROR_REMOTE;
      }
      evs[i]->pdata = pdata;
    }
  }
  
  *nread = nevents;
  return ET_OK;
}

/******************************************************
 * NO SWAPPING IS NECESSARY 
 ******************************************************/
int etn_event_get(et_sys_id id, et_att_id att, et_event **ev,
		 int mode, struct timespec *deltatime)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  int err, transfer[5], incoming[3];
  void *pdata, *p;
 
  /* value of mode is checked and modified in et_event_get */
  

  transfer[0] = htonl(ET_NET_EV_GET_L);
  transfer[1] = att;
  transfer[2] = mode;
  transfer[3] = 0;
  transfer[4] = 0;
  if (deltatime) {
    transfer[3] = deltatime->tv_sec;
    transfer[4] = deltatime->tv_nsec;
  }
 
  et_tcp_lock(etid);
  if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_event_get, write error\n");
    }
    return ET_ERROR_WRITE;
  }
 
  if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_event_get, read error\n");
    }
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);

  if ( (err = incoming[0]) != ET_OK) {
    return err;
  }
  
  /* Pointers (may be 64 bits) are in ET system space and must be translated.
   * The following ifdef avoids compiler warnings.
   */
#ifdef _LP64
  p   = (void *) ET_64BIT_P(incoming[1],incoming[2]);
#else
  p   = (void *) incoming[2];
#endif
  *ev = ET_PEVENT2USR(p, etid->offset);
  
  /* if not a temp event, data is in mem already mmapped */
  if ((*ev)->temp != ET_EVENT_TEMP) {
    (*ev)->pdata = ET_PDATA2USR((*ev)->data, etid->offset);
  }
  /* else, mmap file so we can get at data */
  else {
    /* store ET system's temp data pointer so it doesn't get lost */
    (*ev)->tempdata = (*ev)->pdata;
    /* attach to temp event mem */
    if ((pdata = et_temp_attach((*ev)->filename, (*ev)->memsize)) == NULL) {
      if (etid->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "etn_event_get, cannot attach to temp event\n");
      }
      return ET_ERROR_REMOTE;
    }
    (*ev)->pdata = pdata;
  }
   
  return ET_OK;
}

/******************************************************/
int etn_events_get(et_sys_id id, et_att_id att, et_event *evs[],
		  int mode, struct timespec *deltatime, int num, int *nread)
{
  et_id *etid = (et_id *) id;
  size_t size;
  int sockfd = etid->sockfd;
  int i, nevents, err, transfer[6];
  void *pdata;
  
  /* value of mode is checked and modified in et_events_get */
  
  transfer[0] = htonl(ET_NET_EVS_GET_L);
  transfer[1] = att;
  transfer[2] = mode;
  transfer[3] = num;
  transfer[4] = 0;
  transfer[5] = 0;
  
  if (deltatime) {
    transfer[4] = deltatime->tv_sec;
    transfer[5] = deltatime->tv_nsec;
  }
 
  et_tcp_lock(etid);
  if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_get, write error\n");
    }
    return ET_ERROR_WRITE;
  }
 
  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_get, read error\n");
    }
    return ET_ERROR_READ;
  }

  if (err != ET_OK) {
    et_tcp_unlock(etid);
    return err;
  }
  
  nevents = err;
  size    = nevents*sizeof(et_event *);
  
  if (etNetTcpRead(sockfd, (void *) evs, size) != size) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_get, read error\n");
    }
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);
  
  /* these pointers are in ET system space and must be translated */
  for (i=0; i < nevents; i++) {
    evs[i] = ET_PEVENT2USR(evs[i], etid->offset);
    
    /* if not a temp event, data is in mem already mmapped */
    if (evs[i]->temp != ET_EVENT_TEMP) {
      evs[i]->pdata = ET_PDATA2USR(evs[i]->data, etid->offset);
    }
    /* else, mmap file so we can get at data */
    else {
      /* store ET system's temp data pointer so it doesn't get lost */
      evs[i]->tempdata = evs[i]->pdata;
      /* attach to temp event mem */
      if ((pdata = et_temp_attach(evs[i]->filename, evs[i]->memsize)) == NULL) {
	if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "etn_events_get, cannot attach to temp event\n");
	}
	return ET_ERROR_REMOTE;
      }
      evs[i]->pdata = pdata;
    }
  }
  
  *nread = nevents;
  return ET_OK;
}

/******************************************************/
int etn_event_put(et_sys_id id, et_att_id att, et_event *ev)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  int err=ET_OK, transfer[4];
  et_event *p;

  if (ev->length > ev->memsize) {
      if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "etn_event_put, data length is too large!\n");
      }
      return ET_ERROR;
  }

 /* If temp buffer, unmap it from this process' mem.
   * Restore old values for data pointer.
   */
  if (ev->temp != ET_EVENT_TEMP) {
    ev->pdata = ET_PDATA2ET(ev->pdata, etid->offset);
  }
  else {
    if (munmap(ev->pdata, ev->memsize) != 0) {
      if (etid->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "etn_event_put, error in munmap\n");
      }
      return ET_ERROR_REMOTE;
    }
    ev->pdata = ev->tempdata;
  }
  
  transfer[0] = htonl(ET_NET_EV_PUT_L);
  transfer[1] = att;
  /* translate event pointer back into ET system's space */
  p = ET_PEVENT2ET(ev, etid->offset);
  transfer[2] = ET_HIGHINT((uintptr_t) p);
  transfer[3] = ET_LOWINT((uintptr_t) p);
  
  /* write event */
  et_tcp_lock(etid);
  if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_event_put, write error\n");
    }
    /* undo what was done (not possible for temps as mem already unmapped) */
    if (ev->temp != ET_EVENT_TEMP) {
      ev->pdata = ET_PDATA2USR(ev->pdata, etid->offset);
    }
    return ET_ERROR_WRITE;
  }
  
  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_event_put: read error\n");
    }
    /* undo what was done (not possible for temps as mem already unmapped) */
    if (ev->temp != ET_EVENT_TEMP) {
      ev->pdata = ET_PDATA2USR(ev->pdata, etid->offset);
    }
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);
    
  return err;
}

/******************************************************/
int etn_events_put(et_sys_id id, et_att_id att, et_event *evs[], int num)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  int i, err=ET_OK, transfer[3];
  et_event **events;
  struct iovec iov[2];

  for (i=0; i < num; i++) {
      /* if length bigger than memory size, we got problems  */
      if (evs[i]->length > evs[i]->memsize) {
          if (etid->debug >= ET_DEBUG_ERROR) {
              et_logmsg("ERROR", "etn_events_put, 1 or more data lengths are too large!\n");
          }
          return ET_ERROR;
      }
  }

  /* for translating event pointers back into ET system's space */
  if ( (events = (et_event **) malloc(num*sizeof(et_event *))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_put, cannot allocate memory\n");
    }
    return ET_ERROR_REMOTE;
  }
  
  /* (1) If temp buffer, unmap it from this process' mem.
   * (2) Restore old values for data pointer.
   * (3) Translate event pointers back into ET system's space.
   */
  for (i=0; i < num ; i++) {
    events[i] = ET_PEVENT2ET(evs[i], etid->offset);
    
    if (evs[i]->temp != ET_EVENT_TEMP) {
      evs[i]->pdata = ET_PDATA2ET(evs[i]->pdata, etid->offset);
    }
    else {
      if (munmap(evs[i]->pdata, evs[i]->memsize) != 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "etn_events_put, error in munmap\n");
	}
	free(events);
        return ET_ERROR_REMOTE;
      }
      evs[i]->pdata = evs[i]->tempdata;
    }
  }
  
  
  transfer[0] = htonl(ET_NET_EVS_PUT_L);
  transfer[1] = att;
  transfer[2] = num;
  
  iov[0].iov_base = (void *) transfer;
  iov[0].iov_len  = sizeof(transfer);

  iov[1].iov_base = (void *) events;
  iov[1].iov_len  = num*sizeof(et_event *);

  et_tcp_lock(etid);
  if (etNetTcpWritev(sockfd, iov, 2, 16) == -1) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_put, write error\n");
    }
    
    /* undo what was done (not possible for temps as mem already unmapped) */
    for (i=0; i < num; i++) {
      if (evs[i]->temp != ET_EVENT_TEMP) {
        evs[i]->pdata = ET_PDATA2USR(evs[i]->pdata, etid->offset);
      }
    }
    
    free(events);
    return ET_ERROR_WRITE;
  }

  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_put, read error\n");
    }
    
    /* undo what was done (not possible for temps as mem already unmapped) */
    for (i=0; i < num; i++) {
      if (evs[i]->temp != ET_EVENT_TEMP) {
        evs[i]->pdata = ET_PDATA2USR(evs[i]->pdata, etid->offset);
      }
    }
    
    free(events);
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);

  free(events);
  return err;
}

/******************************************************/
int etn_event_dump(et_sys_id id, et_att_id att, et_event *ev)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  int err=ET_OK, transfer[4];
  et_event *p;  

  /* If temp buffer, unmap it from this process' mem.
   * Restore old values for data pointer.
   */
  if (ev->temp != ET_EVENT_TEMP) {
    ev->pdata = ET_PDATA2ET(ev->pdata, etid->offset);
  }
  else {
    if (munmap(ev->pdata, ev->memsize) != 0) {
      if (etid->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "etn_event_dump, error in munmap\n");
      }
      return ET_ERROR_REMOTE;
    }
    ev->pdata = ev->tempdata;
  }

  transfer[0] = htonl(ET_NET_EV_DUMP_L);
  transfer[1] = att;
  /* translate event pointer back into ET system's space */
  p = ET_PEVENT2ET(ev, etid->offset);
  transfer[2] = ET_HIGHINT((uintptr_t) p);
  transfer[3] = ET_LOWINT((uintptr_t) p);
  

  /* write event */
  et_tcp_lock(etid);
  if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_event_dump, write error\n");
    }
    /* undo what was done (not possible for temps as mem already unmapped) */
    if (ev->temp != ET_EVENT_TEMP) {
      ev->pdata = ET_PDATA2USR(ev->pdata, etid->offset);
    }
    return ET_ERROR_WRITE;
  }
  
  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_event_dump: read error\n");
    }
    /* undo what was done (not possible for temps as mem already unmapped) */
    if (ev->temp != ET_EVENT_TEMP) {
      ev->pdata = ET_PDATA2USR(ev->pdata, etid->offset);
    }
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);
    
  return err;
}

/******************************************************/
int etn_events_dump(et_sys_id id, et_att_id att, et_event *evs[], int num)
{
  et_id *etid = (et_id *) id;
  int sockfd = etid->sockfd;
  int i, err=ET_OK, transfer[3];
  et_event **events;
  struct iovec iov[2];

  /* translate pointers back into ET system's space */
  if ( (events = (et_event **) malloc(num*sizeof(et_event *))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_dump, cannot allocate memory\n");
    }
    return ET_ERROR_REMOTE;
  }
  
  /* (1) If temp buffer, unmap it from this process' mem.
   * (2) Restore old values for data pointer.
   * (3) Translate event pointers back into ET system's space.
   */
  for (i=0; i < num ; i++) {
    events[i] = ET_PEVENT2ET(evs[i], etid->offset);
    
    if (evs[i]->temp != ET_EVENT_TEMP) {
      evs[i]->pdata = ET_PDATA2ET(evs[i]->pdata, etid->offset);
    }
    else {
      if (munmap(evs[i]->pdata, evs[i]->memsize) != 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "etn_events_dump, error in munmap\n");
	}
	free(events);
        return ET_ERROR_REMOTE;
      }
      evs[i]->pdata = evs[i]->tempdata;
    }
  }
  
  
  transfer[0] = htonl(ET_NET_EVS_DUMP_L);
  transfer[1] = att;
  transfer[2] = num;
  
  iov[0].iov_base = (void *) transfer;
  iov[0].iov_len  = sizeof(transfer);

  iov[1].iov_base = (void *) events;
  iov[1].iov_len  = num*sizeof(et_event *);

  et_tcp_lock(etid);
  if (etNetTcpWritev(sockfd, iov, 2, 16) == -1) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_dump, write error\n");
    }
    
    /* undo what was done (not possible for temps as mem already unmapped) */
    for (i=0; i < num; i++) {
      if (evs[i]->temp != ET_EVENT_TEMP) {
        evs[i]->pdata = ET_PDATA2USR(evs[i]->pdata, etid->offset);
      }
    }
    
    free(events);
    return ET_ERROR_WRITE;
  }

  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    et_tcp_unlock(etid);
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etn_events_dump, read error\n");
    }
    
    /* undo what was done (not possible for temps as mem already unmapped) */
    for (i=0; i < num; i++) {
      if (evs[i]->temp != ET_EVENT_TEMP) {
        evs[i]->pdata = ET_PDATA2USR(evs[i]->pdata, etid->offset);
      }
    }
    
    free(events);
    return ET_ERROR_READ;
  }
  et_tcp_unlock(etid);

  free(events);
  return err;
}

