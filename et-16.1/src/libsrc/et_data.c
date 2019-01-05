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
 *      Routines dealing with the encoding and decoding of ET system
 *information used in their monitoring.
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "et_private.h"
#include "et_data.h"

/* prototypes */
static int test_mutex(pthread_mutex_t *mp);
static char *et_data_getsys(et_sysdata *sysdata, char *buffer);
static char *et_data_getstat(et_statdata *statdata, int count, char *buffer);
static char *et_data_getatt(et_attdata *attdata, int count, char *buffer);
static char *et_data_getproc(et_procdata *procdata, int count, char *buffer);
static int   etr_data_gethistogram(et_sys_id id, int hist[], int size);

/******************************************************
 * Test mutex to is if its locked or unlocked.
 ******************************************************/
static int test_mutex(pthread_mutex_t *mp)
{
  int status;

  status = pthread_mutex_trylock(mp);
  if (status == 0) {
    pthread_mutex_unlock(mp);
    return ET_MUTEX_UNLOCKED;
  }
  else if (status == EBUSY) {
    return ET_MUTEX_LOCKED;
  }
  return ET_ERROR;
}

/*****************************************************/
/*              Histogram Data                       */
/*****************************************************/
int et_data_gethistogram(et_sys_id id, int hist[], int size)
{
  et_id *etid = (et_id *) id;
  int  i, *p = etid->histogram;

  if (hist == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_gethistogram, bad value for \"hist\" arg\n");
    }
    return ET_ERROR;
  }
  if (size < 1) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_gethistogram, bad value for \"size\" arg\n");
    }
    return ET_ERROR;
  }

  if (etid->locality == ET_REMOTE) {
    return etr_data_gethistogram(id, hist, size);
  }

  if (size > etid->nevents + 1) {
    size = etid->nevents + 1;
  }

  for (i=0; i < size; i++,p++) {
    hist[i] = *p;
  }

  return ET_OK;
}

/******************************************************
 * Receive histogram data sent over the network by ET system.
 ******************************************************/
static int etr_data_gethistogram(et_sys_id id, int hist[], int size)
{
  et_id *etid = (et_id *) id;
  int   i, err, com, sockfd = etid->sockfd;

  com = htonl(ET_NET_SYS_HIST);
  if (etNetTcpWrite(sockfd, (void *) &com, sizeof(com)) != sizeof(com)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etr_data_gethistogram, write error\n");
    }
    return ET_ERROR_WRITE;
  }

  /* read routine's return error value */
  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etr_data_gethistogram, read error\n");
    }
    return ET_ERROR_READ;
  }
  err = ntohl((uint32_t)err);
  if (err != ET_OK) {
    return err;
  }

  /* read histogram data */
  if (etNetTcpRead(sockfd, (void *) hist,(int)(sizeof(int)*(etid->nevents+1))) !=
      sizeof(int)*(etid->nevents+1)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "etr_data_gethistogram, read error\n");
    }
    return ET_ERROR_READ;
  }

  for (i=0; i < size; i++) {
    hist[i] = ntohl((uint32_t)hist[i]);
  }

  return ET_OK;
}


/******************************************************
 * Get ET system data for sending over the network by
 * server.
 ******************************************************/
int et_data_sys(et_id *id, struct iovec *iov)
{
  uint32_t   i, events_owned=0, totalints=0;
  uint32_t   ifcount, mcount, totalstringlen=0;
  uint32_t   ints[28+2*ET_MAXADDRESSES], len[2*ET_MAXADDRESSES + 1];
  char       *buffer, *pbuf;
  size_t     buffersize;
  et_event   *pe;

  /* values which can change */
  ints[0] = htonl((uint32_t)id->alive);
  ints[1] = htonl((uint32_t)id->sys->heartbeat);
  ints[2] = htonl((uint32_t)id->sys->ntemps);
  ints[3] = htonl((uint32_t)id->sys->nstations);
  ints[4] = htonl((uint32_t)id->sys->nattachments);
  ints[5] = htonl((uint32_t)id->sys->nprocesses);

  /* find out how many events the system owns */
  pe = id->events;
  for (i=0; i < id->sys->config.nevents; i++) {
    if (pe->owner == -1) {
      events_owned++;
    }
    pe++;
  }
  ints[6] = htonl(events_owned);

  /* find out if mutexes are locked */
  ints[7] = htonl((uint32_t)test_mutex(&id->sys->mutex));
  ints[8] = htonl((uint32_t)test_mutex(&id->sys->stat_mutex));
  ints[9] = htonl((uint32_t)test_mutex(&id->sys->statadd_mutex));

  /* values which do NOT change */
  ints[10] = htonl((uint32_t)id->endian);
  ints[11] = htonl((uint32_t)id->share);
  ints[12] = htonl((uint32_t)id->sys->mainpid);
  ints[13] = htonl((uint32_t)id->sys->nselects);
  ints[14] = htonl((uint32_t)id->sys->config.nevents);
  ints[15] = htonl(ET_HIGHINT(id->sys->config.event_size));
  ints[16] = htonl(ET_LOWINT (id->sys->config.event_size));
  ints[17] = htonl((uint32_t)id->bit64);
  
  ints[18] = htonl((uint32_t)id->sys->config.ntemps);
  ints[19] = htonl((uint32_t)id->sys->config.nstations);
  ints[20] = htonl((uint32_t)id->sys->config.nattachments);
  ints[21] = htonl((uint32_t)id->sys->config.nprocesses);

  /* tcp port to ET server */
  ints[22] = htonl((uint32_t)id->sys->port);
  /* udp port direct/broadcasting to find ET server */
  ints[23] = htonl((uint32_t)id->sys->config.port);
  /* udp port multicasting to find ET server (same as above in C version) */
  ints[24] = htonl((uint32_t)id->sys->config.port);

  /* # of interfaces and multicast addresses */
  ifcount  = (uint32_t) id->sys->config.netinfo.count;
  mcount   = (uint32_t) id->sys->config.mcastaddrs.count;
  ints[25] = htonl(ifcount);
  ints[26] = htonl(mcount);
  totalints = 27;

  /* length of interface address strings */
  for (i=0; i < ifcount; i++) {
    len[i] = (uint32_t) strlen(id->sys->config.netinfo.ipinfo[i].addr) + 1;
    ints[totalints++] = htonl(len[i]);
    totalstringlen += len[i];
  }

  /* length of multicast address strings */
  for (i=0; i < mcount; i++) {
    len[i+ifcount] = (uint32_t) strlen(id->sys->config.mcastaddrs.addr[i]) + 1;
    ints[totalints++] = htonl(len[i+ifcount]);
    totalstringlen += len[i+ifcount];
  }

  /* length ET file name */
  len[ifcount+mcount] = (uint32_t) strlen(id->sys->config.filename) + 1;
  ints[totalints++] = htonl((uint32_t)(len[ifcount+mcount]));
  totalstringlen += len[ifcount+mcount];

  /* make buffer */
  buffersize = totalints*sizeof(int) + totalstringlen;
  if ( (buffer = pbuf = (char *) malloc(buffersize)) == NULL) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_sys, cannot allocate memory\n");
    }
    return ET_ERROR;
  }

  /* copy integer data into buffer */
  memcpy((void *)pbuf, (void *) ints, totalints*sizeof(int));
  pbuf += totalints*sizeof(int);

  /* copy strings into buffer */
  for (i=0; i < ifcount; i++) {
    memcpy((void *)pbuf, (void *) id->sys->config.netinfo.ipinfo[i].addr, len[i]);
    pbuf += len[i];
  }
  for (i=0; i < mcount; i++) {
    memcpy((void *)pbuf, (void *) id->sys->config.mcastaddrs.addr[i], len[i+ifcount]);
    pbuf += len[i+ifcount];
  }
  memcpy((void *)pbuf, (void *) id->sys->config.filename, len[ifcount+mcount]);

  /* fill in iov */
  iov->iov_base = (void *) buffer;
  iov->iov_len  = totalints*sizeof(int) + totalstringlen;

  return ET_OK;
}


/******************************************************
 * Get ET station data for sending over the network by
 * server.
 ******************************************************/
int et_data_stats(et_id *id, struct iovec *iov)
{
  char           *pbuf, *buffer;
  uint32_t        i, j, natts, size, count, count1=0, count2=0, parallelHead=1;
  uint32_t        ints[25+ET_ATTACHMENTS_MAX+ET_STATION_SELECT_INTS];
  uint32_t        fnamelen, namelen, classlen, liblen;
  size_t          intsize, buffersize, actualsize=0;
  et_station     *ps, *pstat;

  /*
   * Count # of active/idle stations so we can create the buffer.
   * Since we're not grabbing any mutexes, the number of stations may
   * change between now and when the buffer is filled. To accomodate this,
   * don't allow more stations later than we count right now.
   */
  ps = id->grandcentral;
  while (1) {
    count1++;
    /* Count all stations in parallel to this one. */
    if (ps->config.flow_mode == ET_STATION_PARALLEL) {
      pstat = ps;
      while (pstat->nextparallel > -1) {
        pstat = id->grandcentral + pstat->nextparallel;
        count1++;
      }
    }
    if (ps->next < 0) break;
    ps = id->grandcentral + ps->next;
  }

  intsize = sizeof(int);
  buffersize = intsize + count1*(sizeof(ints) + ET_FUNCNAME_LENGTH +
            ET_FILENAME_LENGTH + ET_STATNAME_LENGTH);

  if ( (pbuf = buffer = (char *) malloc(buffersize)) == NULL) {
    if (id->debug >= ET_DEBUG_ERROR) {
       et_logmsg("ERROR", "et_data_stats: cannot allocate memory\n");
    }
    return ET_ERROR;
  }

  /* the first number to send is the # of stations */
  count = htonl(count1);
  memcpy((void *) pbuf, (void *) &count,  intsize);
  pbuf += intsize;

  ps = pstat = id->grandcentral;
  while (1) {
    if (++count2 > count1) {
      break;
    }

    /* ints */
    ints[1] = htonl((uint32_t)ps->num);
    ints[2] = htonl((uint32_t)ps->data.status);
    ints[3] = htonl((uint32_t)test_mutex(&ps->mutex));
    for (natts=0,i=0; i < ET_ATTACHMENTS_MAX; i++) {
      if (ps->data.att[i] < 0) continue;
      ints[4+natts++] = htonl((uint32_t)ps->data.att[i]);
    }
    ints[0] = htonl((uint32_t)natts);
    j = 4 + natts;

    ints[j+0] = htonl((uint32_t)test_mutex(&ps->list_in.mutex));
    ints[j+1] = htonl((uint32_t)ps->list_in.cnt);
    ints[j+2] = htonl(ET_HIGHINT(ps->list_in.events_try));
    ints[j+3] = htonl(ET_LOWINT(ps->list_in.events_try));
    ints[j+4] = htonl(ET_HIGHINT(ps->list_in.events_in));
    ints[j+5] = htonl(ET_LOWINT(ps->list_in.events_in));
    ints[j+6] = htonl((uint32_t)test_mutex(&ps->list_out.mutex));
    ints[j+7] = htonl((uint32_t)ps->list_out.cnt);
    ints[j+8] = htonl(ET_HIGHINT(ps->list_out.events_out));
    ints[j+9] = htonl(ET_LOWINT(ps->list_out.events_out));
   
    /* Add some information for the monitor on the receiving end of this data.
     * Let the monitor know if this is the head of a group of parallel stations
     * or not.
     */
    if (ps->config.flow_mode == ET_STATION_PARALLEL && parallelHead) {
      ints[j+10] = htonl(ET_STATION_PARALLEL_HEAD);
    }
    else {
      ints[j+10] = htonl((uint32_t)ps->config.flow_mode);
    }
    ints[j+11] = htonl((uint32_t)ps->config.user_mode);
    ints[j+12] = htonl((uint32_t)ps->config.restore_mode);
    ints[j+13] = htonl((uint32_t)ps->config.block_mode);
    ints[j+14] = htonl((uint32_t)ps->config.prescale);
    ints[j+15] = htonl((uint32_t)ps->config.cue);
    ints[j+16] = htonl((uint32_t)ps->config.select_mode);
    for (i=0; i < ET_STATION_SELECT_INTS; i++) {
      ints[j+17+i] = htonl((uint32_t)ps->config.select[i]);
    }
    j += 17 + ET_STATION_SELECT_INTS;

    /* length of strings */
    fnamelen = (uint32_t)strlen(ps->config.fname)+1;
    liblen   = (uint32_t)strlen(ps->config.lib)+1;
    classlen = (uint32_t)strlen(ps->config.classs)+1;
    namelen  = (uint32_t)strlen(ps->name)+1;

    ints[j+0] = htonl(fnamelen);
    ints[j+1] = htonl(liblen);
    ints[j+2] = htonl(classlen);
    ints[j+3] = htonl(namelen);

    size = (uint32_t) (intsize*(25 + ET_STATION_SELECT_INTS + natts)) ;

    /* copy into buffer */
    memcpy((void *)pbuf, (void *) ints, size);
    pbuf += size;
    memcpy((void *)pbuf, (void *) ps->config.fname, fnamelen);
    pbuf += fnamelen;
    memcpy((void *)pbuf, (void *) ps->config.lib, liblen);
    pbuf += liblen;
    memcpy((void *)pbuf, (void *) ps->config.classs, classlen);
    pbuf += classlen;
    memcpy((void *)pbuf, (void *) ps->name, namelen);
    pbuf += namelen;

    /* track size of all data stored in buffer */
    actualsize += size + fnamelen + liblen + classlen + namelen;

    /* loop thru all stations in parallel to this one */
    if (ps->config.flow_mode == ET_STATION_PARALLEL) {
      /* next station is not the head */
      parallelHead = 0;
      if (ps->nextparallel > -1) {
        ps = id->grandcentral + ps->nextparallel;
        continue;
      }
      else {
        parallelHead = 1;
      }
    }
    
    if (pstat->next < 0) {
      break;
    }
    ps = pstat = id->grandcentral + pstat->next;
  }

   /* if we sent fewer stations than we estimated at first, correct this */
  if (count2 < count1) {
    count = htonl(count2);
    memcpy((void *) buffer, (void *) &count,  intsize);
  }

  iov->iov_base = (void *) buffer;
  iov->iov_len  = intsize + actualsize;

  return ET_OK;
}


/******************************************************
 * Get ET attachment data for sending over the network by
 * server.
 ******************************************************/
int et_data_atts(et_id *id, struct iovec *iov)
{
  char           *pbuf, *buffer;
  uint32_t        i, j, count, count1=0, count2=0, events_owned;
  uint32_t        ints[18], hostlen, namelen, iplen;
  size_t          intsize, bufsize, actualsize=0;
  et_station     *ps;
  et_event       *pe;

  /*
   * Count # of attachments so we can create the buffer.
   * Since we're not grabbing any mutexes, the number of attachments may
   * change between now and when the buffer is filled. To accomodate this,
   * don't allow more attachments later than we count right now.
   */
  for (i=0; i < id->sys->config.nattachments ; i++) {
    if (id->sys->attach[i].status != ET_ATT_UNUSED) {
      count1++;
    }
  }

  intsize = sizeof(int);
  bufsize = intsize + count1*(sizeof(ints)+ET_MAXHOSTNAMELEN+ET_STATNAME_LENGTH);

  if ( (pbuf = buffer = (char *) malloc(bufsize)) == NULL) {
    if (id->debug >= ET_DEBUG_ERROR) {
       et_logmsg("ERROR", "et_pack_stats: cannot allocate memory\n");
    }
    return ET_ERROR;
  }

  /* the first number to send is the # of attachments */
  count = htonl((uint32_t)count1);
  memcpy((void *) pbuf, (void *) &count,  intsize);
  pbuf += intsize;

  for (i=0; i < id->sys->config.nattachments ; i++) {
    if (id->sys->attach[i].status == ET_ATT_UNUSED) {
      continue;
    }
    if (++count2 > count1) {
      break;
    }

    ps = id->grandcentral + id->sys->attach[i].stat;

    /* ints */
    ints[0] = htonl((uint32_t)id->sys->attach[i].num);
    ints[1] = htonl((uint32_t)id->sys->attach[i].proc);
    ints[2] = htonl((uint32_t)id->sys->attach[i].stat);
    ints[3] = htonl((uint32_t)id->sys->attach[i].pid);
    ints[4] = htonl((uint32_t)id->sys->attach[i].blocked);
    ints[5] = htonl((uint32_t)id->sys->attach[i].quit);

    /* find out how many events the attachment owns */
    pe = id->events;
    events_owned = 0;
    for (j=0; j < id->sys->config.nevents; j++) {
      if (pe->owner == id->sys->attach[i].num) {
        events_owned++;
      }
      pe++;
    }
    ints[6] = htonl((uint32_t)events_owned);

    ints[7]  = htonl(ET_HIGHINT(id->sys->attach[i].events_put));
    ints[8]  = htonl(ET_LOWINT( id->sys->attach[i].events_put));
    ints[9]  = htonl(ET_HIGHINT(id->sys->attach[i].events_get));
    ints[10] = htonl(ET_LOWINT( id->sys->attach[i].events_get));
    ints[11] = htonl(ET_HIGHINT(id->sys->attach[i].events_dump));
    ints[12] = htonl(ET_LOWINT( id->sys->attach[i].events_dump));
    ints[13] = htonl(ET_HIGHINT(id->sys->attach[i].events_make));
    ints[14] = htonl(ET_LOWINT( id->sys->attach[i].events_make));
        
    /* length of strings */
    hostlen  = (uint32_t)strlen(id->sys->attach[i].host)+1;
    namelen  = (uint32_t)strlen(ps->name)+1;
    iplen    = (uint32_t)strlen(id->sys->attach[i].interface)+1;
    ints[15] = htonl(hostlen);
    ints[16] = htonl(namelen);
    ints[17] = htonl(iplen);

    /* copy into buffer */
    memcpy((void *)pbuf, (void *) ints, sizeof(ints));
    pbuf += sizeof(ints);
    memcpy((void *)pbuf, (void *) id->sys->attach[i].host, hostlen);
    pbuf += hostlen;
    memcpy((void *)pbuf, (void *) ps->name, namelen);
    pbuf += namelen;
    memcpy((void *)pbuf, (void *) id->sys->attach[i].interface, iplen);
    pbuf += iplen;

    /* track size of all data stored in buffer */
    actualsize += sizeof(ints) + hostlen + namelen + iplen;
  }

  /* if we sent fewer attachments than we estimated at first, correct this */
  if (count2 < count1) {
    count = htonl(count2);
    memcpy((void *) buffer, (void *) &count,  intsize);
  }

  iov->iov_base = (void *) buffer;
  iov->iov_len  = intsize + actualsize;

  return ET_OK;
}

/******************************************************
 * Get ET process data for sending over the network by
 * server.
 ******************************************************/
int et_data_procs(et_id *id, struct iovec *iov)
{
  char           *pbuf, *buffer;
  uint32_t        i, j, count, count1=0, count2=0, natts;
  uint32_t        ints[4+ET_ATTACHMENTS_MAX];
  size_t          intsize, bufsize, size;

  /*
   * Count # of processes so we can create the buffer.
   * Since we're not grabbing any mutexes, the number of processes may
   * change between now and when the buffer is filled. To accomodate this,
   * don't allow more processes later than we count right now.
   */
  for (i=0; i < id->sys->config.nprocesses ; i++) {
    if (id->sys->proc[i].status == ET_PROC_OPEN) {
      count1++;
    }
  }

  intsize = sizeof(int);
  bufsize = intsize + count1*sizeof(ints);

  if ( (pbuf = buffer = (char *) malloc(bufsize)) == NULL) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_procs: cannot allocate memory\n");
    }
    return ET_ERROR;
  }

  /* the first number to send is the # of local user processes */
  count = htonl(count1);
  memcpy((void *) pbuf, (void *) &count, intsize);
  bufsize = intsize; /* now use this to track actual data size */
  pbuf += intsize;
  
  for (i=0; i < id->sys->config.nprocesses ; i++) {
    if (id->sys->proc[i].status != ET_PROC_OPEN) {
      continue;
    }
    if (++count2 > count1) {
      break;
    }

    /* ints */
    ints[1] = htonl((uint32_t)id->sys->proc[i].num);
    ints[2] = htonl((uint32_t)id->sys->proc[i].heartbeat);
    ints[3] = htonl((uint32_t)id->sys->proc[i].pid);
    for (natts=0,j=0; j < ET_ATTACHMENTS_MAX; j++) {
      if (id->sys->proc[i].att[j] < 0) continue;
      ints[4+natts++] = htonl((uint32_t)id->sys->proc[i].att[j]);
    }
    ints[0] = htonl(natts);

    /* copy integers into buffer */
    size = intsize*(4+natts);
    memcpy((void *)pbuf, (void *) ints, size);
    pbuf += size;
    bufsize += size;
  }

  /* if we sent fewer processes than we estimated at first, correct this */
  if (count2 < count1) {
    count = htonl(count2);
    memcpy((void *) buffer, (void *) &count,  intsize);
  }

  iov->iov_base = (void *) buffer;
  iov->iov_len  = bufsize;

  return ET_OK;
}


/******************************************************
 * Fill structure with ET system data sent over the
 * network.
 ******************************************************/
static char *et_data_getsys(et_sysdata *sysdata, char *buffer)
{
  uint32_t i, ints[27], ifcount, mcount, len[2*ET_MAXADDRESSES + 1];

  if ((sysdata == NULL) || (buffer == NULL)) {
    return buffer;
  }

  memcpy((void *)ints, (void *)buffer, sizeof(ints));

  sysdata->alive            = ntohl(ints[0]);
  sysdata->heartbeat        = ntohl(ints[1]);
  sysdata->ntemps           = ntohl(ints[2]);
  sysdata->nstations        = ntohl(ints[3]);
  sysdata->nattachments     = ntohl(ints[4]);
  sysdata->nprocesses       = ntohl(ints[5]);
  sysdata->events_owned     = ntohl(ints[6]);
  sysdata->mutex            = ntohl(ints[7]);
  sysdata->stat_mutex       = ntohl(ints[8]);
  sysdata->statadd_mutex    = ntohl(ints[9]);

  sysdata->endian           = ntohl(ints[10]);
  sysdata->share            = ntohl(ints[11]);
  sysdata->mainpid          = ntohl(ints[12]);
  sysdata->nselects         = ntohl(ints[13]);
  sysdata->nevents          = ntohl(ints[14]);
  sysdata->event_size       = ET_64BIT_UINT(ntohl(ints[15]),ntohl(ints[16]));
  sysdata->bit64            = ntohl(ints[17]);
  
  sysdata->ntemps_max       = ntohl(ints[18]);
  sysdata->nstations_max    = ntohl(ints[19]);
  sysdata->nattachments_max = ntohl(ints[20]);
  sysdata->nprocesses_max   = ntohl(ints[21]);

  sysdata->tcp_port         = ntohl(ints[22]);
  sysdata->udp_port         = ntohl(ints[23]);
  sysdata->multi_port       = ntohl(ints[24]); /* not used */

  /* #s of strings coming up */
  sysdata->ifaddrs.count    = ifcount = ntohl(ints[25]);
  sysdata->mcastaddrs.count = mcount  = ntohl(ints[26]);

  /* read in lengths of strings */
  buffer += sizeof(ints);
  memcpy((void *)len, (void *)buffer, (ifcount+mcount+1)*sizeof(int));
  for (i=0; i<ifcount+mcount+1; i++) {
    len[i] = ntohl(len[i]);
  }
  buffer += (ifcount+mcount+1)*sizeof(int);

  /* read in interface address strings */
  for (i=0; i<ifcount; i++) {
    memcpy((void *)sysdata->ifaddrs.addr[i], (void *)buffer, len[i]);
    buffer += len[i];
  }

  /* read in multicast address strings */
  for (i=0; i<mcount; i++) {
    memcpy((void *)sysdata->mcastaddrs.addr[i], (void *)buffer, len[i+ifcount]);
    buffer += len[i+ifcount];
  }

  memcpy((void *)sysdata->filename,(void *)buffer, len[ifcount+mcount]);
  buffer += len[ifcount+mcount];

  return buffer;
}


/******************************************************
 * Fill structure with ET station data sent over the
 * network.
 ******************************************************/
static char *et_data_getstat(et_statdata *statdata, int count, char *buffer)
{
  int i, j, k, att, natts, len1, len2, len3, len4;
  uint32_t ints[24+ET_ATTACHMENTS_MAX+ET_STATION_SELECT_INTS];
  size_t intsize = sizeof(int);

  if ((count == 0) || (statdata == NULL) || (buffer == NULL)) {
    return buffer;
  }


  for (i=0; i < count; i++) {
    memcpy((void*) &natts, (void *)buffer, intsize);
    natts = ntohl((uint32_t)natts);
    buffer += intsize;
    memcpy((void *)ints, (void *)buffer, intsize*(24+ ET_STATION_SELECT_INTS + natts));

    statdata->num                 = ntohl(ints[0]);
    statdata->status              = ntohl(ints[1]);
    statdata->mutex               = ntohl(ints[2]);
    statdata->nattachments        = natts;
    for (j=0; j < ET_ATTACHMENTS_MAX; j++) {
      statdata->att[j]            = -1;
    }
    for (j=0; j < natts; j++) {
      att = ntohl(ints[3+j]);
      statdata->att[att]          = att;
    }
    k = 3 + natts;

    statdata->inlist_mutex        = ntohl(ints[k+0]);
    statdata->inlist_cnt          = ntohl(ints[k+1]);
    
    statdata->inlist_try = ET_64BIT_UINT(ntohl(ints[k+2]),ntohl(ints[k+3]));
    statdata->inlist_in  = ET_64BIT_UINT(ntohl(ints[k+4]),ntohl(ints[k+5]));

    statdata->outlist_mutex       = ntohl(ints[k+6]);
    statdata->outlist_cnt         = ntohl(ints[k+7]);
    
    statdata->outlist_out = ET_64BIT_UINT(ntohl(ints[k+8]),ntohl(ints[k+9]));

    statdata->flow_mode           = ntohl(ints[k+10]);
    statdata->user_mode           = ntohl(ints[k+11]);
    statdata->restore_mode        = ntohl(ints[k+12]);
    statdata->block_mode          = ntohl(ints[k+13]);
    statdata->prescale            = ntohl(ints[k+14]);
    statdata->cue                 = ntohl(ints[k+15]);
    statdata->select_mode         = ntohl(ints[k+16]);
    for (j=0; j< ET_STATION_SELECT_INTS; j++) {
      statdata->select[j]         = ntohl(ints[k+17+j]);
    }
    k += 17 + ET_STATION_SELECT_INTS;

    /* copy in strings */
    len1 = ntohl(ints[k+0]);
    len2 = ntohl(ints[k+1]);
    len3 = ntohl(ints[k+2]);
    len4 = ntohl(ints[k+3]);
    buffer += (k+4)*intsize;

    memcpy((void *)statdata->fname, (void *)buffer, (size_t)len1);
    buffer += len1;
    memcpy((void *)statdata->lib,(void *)buffer, (size_t)len2);
    buffer += len2;
    memcpy((void *)statdata->classs,(void *)buffer, (size_t)len3);
    buffer += len3;
    memcpy((void *)statdata->name,(void *)buffer, (size_t)len4);
    buffer += len4;

    /* go to the next element in statdata array */
    statdata++;
  }

  return buffer;
}


/******************************************************
 * Fill structure with ET attachment data sent over the
 * network.
 ******************************************************/
static char *et_data_getatt(et_attdata *attdata, int count, char *buffer)
{
  uint32_t i, ints[18], len1, len2, len3;

  if ((count == 0) || (attdata == NULL) || (buffer == NULL)) {
    return buffer;
  }

  for (i=0; i < count; i++) {
    memcpy((void *)ints, (void *)buffer, sizeof(ints));

    attdata->num     = ntohl(ints[0]);
    attdata->proc    = ntohl(ints[1]);
    attdata->stat    = ntohl(ints[2]);
    attdata->pid     = ntohl(ints[3]);
    attdata->blocked = ntohl(ints[4]);
    attdata->quit    = ntohl(ints[5]);
    attdata->owned   = ntohl(ints[6]);

    attdata->events_put  = ET_64BIT_UINT(ntohl(ints[7]), ntohl(ints[8]));
    attdata->events_get  = ET_64BIT_UINT(ntohl(ints[9]), ntohl(ints[10]));
    attdata->events_dump = ET_64BIT_UINT(ntohl(ints[11]),ntohl(ints[12]));
    attdata->events_make = ET_64BIT_UINT(ntohl(ints[13]),ntohl(ints[14]));

    /* copy in strings */
    len1 = ntohl(ints[15]);
    len2 = ntohl(ints[16]);
    len3 = ntohl(ints[17]);
    buffer += sizeof(ints);

    memcpy((void *)attdata->host, (void *)buffer, len1);
    buffer += len1;
    memcpy((void *)attdata->station,(void *)buffer, len2);
    buffer += len2;
    memcpy((void *)attdata->interface,(void *)buffer, len3);
    buffer += len3;

    /* go to the next element in attdata array */
    attdata++;
  }

  return buffer;
}


/******************************************************
 * Fill structure with ET process data sent over the
 * network.
 ******************************************************/
static char *et_data_getproc(et_procdata *procdata, int count, char *buffer)
{
  uint32_t i, j, att, natts, ints[3+ET_ATTACHMENTS_MAX];
  size_t   intsize = sizeof(int);

  if ((count == 0) || (procdata == NULL) || (buffer == NULL)) {
    return buffer;
  }

  for (i=0; i < count; i++) {
    memcpy((void*) &natts, (void *)buffer, intsize);
    natts = ntohl(natts);
    buffer += intsize;
    memcpy((void *)ints, (void *)buffer, intsize*(3 + natts));

    procdata->num          = ntohl(ints[0]);
    procdata->heartbeat    = ntohl(ints[1]);
    procdata->pid          = ntohl(ints[2]);
    procdata->nattachments = natts;
    for (j=0; j < ET_ATTACHMENTS_MAX; j++) {
      procdata->att[j] = -1;
    }
    for (j=0; j < natts; j++) {
      att = ntohl(ints[3+j]);
      procdata->att[att] = att;
    }
    buffer += intsize*(3 + natts);

    /* go to the next element in procdata array */
    procdata++;
  }

  return buffer;
}


/******************************************************
 * Receive ET data sent over the network by ET system.
 ******************************************************/
int et_data_get(et_sys_id id, et_alldata *alldata)
{
  et_id *etid = (et_id *) id;
  int   err, com, sockfd = etid->sockfd;
  uint32_t   stat_count, att_count, proc_count, bufsize;
  char  *pbuf, *buffer;


  com = htonl(ET_NET_SYS_DATA);
  if (etNetTcpWrite(sockfd, (void *) &com, sizeof(com)) != sizeof(com)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, write error\n");
    }
    return ET_ERROR_WRITE;
  }

  /* read first piece of data which is the error */
  if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, read error\n");
    }
    return ET_ERROR_READ;
  }
  err = ntohl((uint32_t)err);
  if (err != ET_OK) {
    return err;
  }
  
  /* read second piece of data which is size of all the rest of the data */
  if (etNetTcpRead(sockfd, (void *) &bufsize, sizeof(bufsize)) != sizeof(bufsize)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, read error\n");
    }
    return ET_ERROR_READ;
  }

  bufsize = ntohl(bufsize);
  if ( (pbuf = buffer = (char *) malloc(bufsize)) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, cannot allocate read buffer memory\n");
    }
    return ET_ERROR;
  }

  /* read in all the actual data into a single buffer */
  if (etNetTcpRead(sockfd, (void *) pbuf, bufsize) != bufsize) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, read error\n");
    }
    free(buffer);
    return ET_ERROR_READ;
  }

  /* data goes to host byte order and into structures */

  /* Start with system data - allocate structure to be filled with data */
  if ( (alldata->sysdata = (et_sysdata *) malloc(sizeof(et_sysdata))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, cannot allocate system data memory\n");
    }
    free(buffer);
    return ET_ERROR;
  }
  /* unpack data into structure */
  pbuf = et_data_getsys(alldata->sysdata, pbuf);

  /* now do the station data, first read how many stations */
  memcpy((void *) &stat_count, (void *) pbuf, sizeof(int));
  stat_count = ntohl(stat_count);
  alldata->nstations = stat_count;
  pbuf += sizeof(int);

  /* now allocate structures to be filled with data */
  if ( (alldata->statdata =
        (et_statdata *) calloc(stat_count, sizeof(et_statdata))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, cannot allocate station data memory\n");
    }
    free(buffer);
    free(alldata->sysdata);
    return ET_ERROR;
  }
  /* unpack data into structure */
  pbuf = et_data_getstat(alldata->statdata, stat_count, pbuf);

  /* now do the attachment data, first read how many attachments */
  memcpy((void *) &att_count, (void *) pbuf, sizeof(int));
  att_count = ntohl(att_count);
  alldata->natts = att_count;
  pbuf += sizeof(int);

  /* now allocate structures to be filled with data */
  if ( (alldata->attdata =
        (et_attdata *) calloc(att_count, sizeof(et_attdata))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, cannot allocate attachment data memory\n");
    }
    free(buffer);
    free(alldata->sysdata);
    free(alldata->statdata);
    return ET_ERROR;
  }
  /* unpack data into structure */
  pbuf = et_data_getatt(alldata->attdata, att_count, pbuf);

  /* now do the process data, first read how many processes */
  memcpy((void *) &proc_count, (void *) pbuf, sizeof(int));
  proc_count = ntohl(proc_count);
  alldata->nprocs = proc_count;
  pbuf += sizeof(int);

  /* now allocate structures to be filled with data */
  if ( (alldata->procdata =
        (et_procdata *) calloc(proc_count, sizeof(et_procdata))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_data_get, cannot allocate process data memory\n");
    }
    free(buffer);
    free(alldata->sysdata);
    free(alldata->statdata);
    free(alldata->attdata);
    return ET_ERROR;
  }
  /* unpack data into structure */
  pbuf = et_data_getproc(alldata->procdata, proc_count, pbuf);

  /* free data buffer */
  free(buffer);

  return ET_OK;
}


/******************************************************
 * Free memory allocated to store ET system information.
 ******************************************************/
void et_data_free(et_alldata *alldata)
{
  if (alldata == NULL) {
    return;
  }

  free(alldata->sysdata);
  free(alldata->statdata);
  free(alldata->attdata);
  free(alldata->procdata);

  return;
}




