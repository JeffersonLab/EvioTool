/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      Routines for ET system configuration
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VXWORKS
#include <strings.h>
#endif

#include "et_private.h"
#include "et_network.h"

/*****************************************************
 * Routines for setting and getting system configuration
 * parameters. Used to create systems..
 *****************************************************/

int et_system_config_init(et_sysconfig* sconfig)
{
  et_sys_config *sc;
  
  sc = (et_sys_config *) calloc(1, sizeof(et_sys_config));
  if (sc == NULL) {
    return ET_ERROR;
  }
  
  /* default configuration for a system */
  sc->nevents          = ET_SYSTEM_EVENTS;
  sc->event_size       = ET_SYSTEM_ESIZE;
  sc->ntemps           = ET_SYSTEM_NTEMPS;
  sc->nstations        = ET_SYSTEM_NSTATS;
  sc->nprocesses       = ET_PROCESSES_MAX;
  sc->nattachments     = ET_ATTACHMENTS_MAX;
  sc->port             = ET_BROADCAST_PORT;
  sc->serverport       = 0;
  sc->tcpSendBufSize   = 0;  /* use operating system default */
  sc->tcpRecvBufSize   = 0;  /* use operating system default */
  sc->tcpNoDelay       = 1;  /* on */
  sc->mcastaddrs.count = 0;
 *sc->filename         = '\0';
  sc->groupCount       = 1;
  memset((void *)sc->groups, 0, ET_EVENT_GROUPS_MAX*sizeof(int));
  sc->groups[0]        = sc->nevents;
  
  /* Find our local subnets' broadcast addresses */
  if (etNetGetBroadcastAddrs(NULL, &sc->bcastaddrs) == ET_ERROR) {
    sc->bcastaddrs.count = 0;
  }
  /* check to see if we have room for the last broadcast addr (255.255.255.255) */
  if (sc->bcastaddrs.count >= ET_MAXADDRESSES) {
      return ET_ERROR_NOMEM;     
  }

  /* Find our local interfaces' addresses and names. */
  if (etNetGetNetworkInfo(NULL, &sc->netinfo) != ET_OK) {
    sc->netinfo.count = 0;
  }
   		  
  sc->init = ET_STRUCT_OK;
  *sconfig = (et_sysconfig) sc;
  
  return ET_OK;
}

/*****************************************************/
int et_system_config_destroy(et_sysconfig sconfig)
{
  et_sys_config *sc = (et_sys_config *)sconfig;
  
  if (sc == NULL) return ET_OK;
  
  free(sc);
  return ET_OK;
}

/*****************************************************/
int et_system_config_setevents(et_sysconfig sconfig, int val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  if (val < 1) {
    return ET_ERROR;
  }
  
  sc->nevents = val;
  
  /* if only 1 group (usual case), put all events into that group */
  if (sc->groupCount == 1) {
      sc->groups[0] = val;
  }
  
  /* number of temp events can't be more than the number of events */
  if (sc->ntemps > val) sc->ntemps = val;
  return ET_OK;
}

/*****************************************************/
int et_system_config_getevents(et_sysconfig sconfig, int *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  *val = sc->nevents;
  return ET_OK;
}

/*****************************************************/
int et_system_config_setsize(et_sysconfig sconfig, size_t val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  if (val < 1) {
    return ET_ERROR;
  }
  
  sc->event_size = val;
  return ET_OK;
}

/*****************************************************/
int et_system_config_getsize(et_sysconfig sconfig, size_t *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  *val = (size_t) sc->event_size;
  return ET_OK;
}

/*****************************************************/
int et_system_config_settemps(et_sysconfig sconfig, int val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  if (val < 1) {
    return ET_ERROR;
  }
  
  sc->ntemps = val;
  return ET_OK;
}

/*****************************************************/
int et_system_config_gettemps(et_sysconfig sconfig, int *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  *val = sc->ntemps;
  return ET_OK;
}

/*****************************************************/
int et_system_config_setstations(et_sysconfig sconfig, int val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  if (val < 1) {
    return ET_ERROR;
  }
  
  sc->nstations = val;
  return ET_OK;
}

/*****************************************************/
int et_system_config_getstations(et_sysconfig sconfig, int *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  *val = sc->nstations;
  return ET_OK;
}

/*****************************************************/
int et_system_config_setprocs(et_sysconfig sconfig, int val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  if (val < 1) {
    return ET_ERROR;
  }
  else if (val > ET_PROCESSES_MAX) {
    val = ET_PROCESSES_MAX;
  }
  
  sc->nprocesses = val;
  return ET_OK;
}

/*****************************************************/
int et_system_config_getprocs(et_sysconfig sconfig, int *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  *val = sc->nprocesses;
  return ET_OK;
}

/*****************************************************/
int et_system_config_setattachments(et_sysconfig sconfig, int val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  if (val < 1) {
    return ET_ERROR;
  }
  else if (val > ET_ATTACHMENTS_MAX) {
    val = ET_ATTACHMENTS_MAX;
  }
  
  sc->nattachments = val;
  return ET_OK;
}

/*****************************************************/
int et_system_config_getattachments(et_sysconfig sconfig, int *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  *val = sc->nattachments;
  return ET_OK;
}

/*****************************************************/
int et_system_config_setport(et_sysconfig sconfig, int val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if (val < 1024 || val > 65535) {
    return ET_ERROR;
  }
  
  sc->port = val;
  return ET_OK;
}

/*****************************************************/
int et_system_config_getport(et_sysconfig sconfig, int *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  *val = sc->port;
  return ET_OK;
}

/*****************************************************/
int et_system_config_setserverport(et_sysconfig sconfig, int val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if (val < 1024 || val > 65535) {
    return ET_ERROR;
  }
  
  sc->serverport = val;
  return ET_OK;
}

/*****************************************************/
int et_system_config_getserverport(et_sysconfig sconfig, int *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  *val = sc->serverport;
  return ET_OK;
}

/*****************************************************/
int et_system_config_setgroups(et_sysconfig sconfig, int groups[], int size)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  int i;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if (size == 1) {
      for (i=0; i < ET_EVENT_GROUPS_MAX; i++) {
          sc->groups[i] = 0;
      }
      sc->groupCount = 1;
      return ET_OK;
  }
  
  if (groups == NULL || size > ET_EVENT_GROUPS_MAX || size < 1) {
    return ET_ERROR;
  }
  
  for (i=0; i < size; i++) {
      if (groups[i] < 1) return ET_ERROR;
      sc->groups[i] = groups[i];
  }
  
  for (i=size; i < ET_EVENT_GROUPS_MAX; i++) {
      sc->groups[i] = 0;
  }

  sc->groupCount = size;
  return ET_OK;
}

/*****************************************************/
int et_system_config_settcp(et_sysconfig sconfig, int rBufSize, int sBufSize, int noDelay)
{
    et_sys_config *sc = (et_sys_config *) sconfig;
  
    if (sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }
   
    if (rBufSize < 0 || sBufSize < 0) {
        return ET_ERROR;
    }

    if (noDelay < 0) noDelay = 1;

    sc->tcpRecvBufSize = rBufSize;
    sc->tcpSendBufSize = sBufSize;
    sc->tcpNoDelay = noDelay;

    return ET_OK;
}

/*****************************************************/
int et_system_config_gettcp(et_sysconfig sconfig, int *rBufSize, int *sBufSize, int *noDelay)
{
    et_sys_config *sc = (et_sys_config *) sconfig;
  
    if (rBufSize != NULL) {
        *rBufSize = sc->tcpRecvBufSize;
    }
    
    if (sBufSize != NULL) {
        *sBufSize = sc->tcpSendBufSize;
    }
    
    if (noDelay != NULL) {
        *noDelay = sc->tcpNoDelay;
    }

    return ET_OK;
}


/*****************************************************
 * This routine together with et_system_config_removemulticast
 * replaces et_system_config_setaddress.
 ****************************************************/
int et_system_config_addmulticast(et_sysconfig sconfig, const char *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  size_t len;
  int  firstnumber, i;
  char firstint[4];
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  
  if (val == NULL) {
    return ET_ERROR;
  }
  
  if ((strlen(val) >= ET_IPADDRSTRLEN) || (strlen(val) < 7)) {
    return ET_ERROR;
  }
  
  /* The address is in dotted-decimal form. If the first
   * number is between 224-239, its a multicast address.
   */
  len = strcspn(val, ".");
  strncpy(firstint, val, len);
  firstint[len] = '\0';
  firstnumber = atoi(firstint);
  
  if ((firstnumber < 224) || (firstnumber > 239)) {
    fprintf(stderr, "et_open_config_addmulticast: bad value for multicast address\n");
    return ET_ERROR;
  }
  
  /* Once here, it's probably an address of the right value.
   * Don't add it to the list if it's already there or if
   * there's no more room on the list.
   */
  for (i=0; i < sc->mcastaddrs.count; i++) {
    if (strcmp(val, sc->mcastaddrs.addr[i]) == 0) {
      /* it's already in the list */
      return ET_OK;
    }
  }
  if (sc->mcastaddrs.count == ET_MAXADDRESSES) {
    /* list is already full */
    return ET_ERROR;
  }
  
  strcpy(sc->mcastaddrs.addr[sc->mcastaddrs.count++], val);
  
  return ET_OK;
}

/*****************************************************/
int et_system_config_removemulticast(et_sysconfig sconfig, const char *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  size_t len;
  int  firstnumber, i, j;
  char firstint[4];
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  
  if (val == NULL) {
    return ET_ERROR;
  }
  
  if ((strlen(val) >= ET_IPADDRSTRLEN) || (strlen(val) < 7)) {
    return ET_ERROR;
  }
  
  /* The address is in dotted-decimal form. If the first
   * number is between 224-239, its a multicast address.
   */
  len = strcspn(val, ".");
  strncpy(firstint, val, len);
  firstint[len] = '\0';
  firstnumber = atoi(firstint);
  
  if ((firstnumber < 224) || (firstnumber > 239)) {
    fprintf(stderr, "et_open_config_addmulticast: bad value for multicast address\n");
    return ET_ERROR;
  }
  
  /* Once here, it's probably an address of the right value.
   * Remove it from the list only if it's already there.
   */
  for (i=0; i < sc->mcastaddrs.count; i++) {
    if (strcmp(val, sc->mcastaddrs.addr[i]) == 0) {
      /* It's in the list. Remove it and move later elements up. */
      for (j=i+1; j < sc->mcastaddrs.count; j++) {
        strcpy(sc->mcastaddrs.addr[j-1], sc->mcastaddrs.addr[j]);
      }
      sc->mcastaddrs.count--;
      return ET_OK;
    }
  }
    
  return ET_OK;
}


/*****************************************************/
int et_system_config_setfile(et_sysconfig sconfig, const char *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  if (val == NULL) {
    return ET_ERROR;
  }
  if (strlen(val) > ET_FILENAME_LENGTH - 1) {
    return ET_ERROR;
  }
  
  strcpy(sc->filename, val);

  return ET_OK;
}

/*****************************************************/
int et_system_config_getfile(et_sysconfig sconfig, char *val)
{
  et_sys_config *sc = (et_sys_config *) sconfig;
  
  strcpy(val, sc->filename);
  return ET_OK;
}

/******************************************************/
/* Routines for setting/getting system information    */
/* from an existing system.                           */
/******************************************************/

int et_system_getgroup(et_sys_id id, int *group)
{
  et_id *etid = (et_id *) id;
  
  if (group != NULL) {
     *group = etid->group;
     return ET_OK;
  }
  return ET_ERROR;
} 

/******************************************************/
int et_system_setgroup(et_sys_id id, int group)
{
  et_id *etid = (et_id *) id;
  int err, groupCount;
  
  if (group == 0) {
      etid->group = 0;
      return ET_OK;      
  }
  else if (group < 0)  {
     return ET_ERROR;
  }
  
  if (etid->locality != ET_LOCAL) {
    err = etr_system_getgroupcount(id, &groupCount);
    if (err != ET_OK)
        return err;
  }
  else {
    groupCount = etid->sys->config.groupCount;
  }

  if (group > groupCount)  {
     return ET_ERROR;
  }
  
  etid->group = group;
  return ET_OK;
} 

/******************************************************/
int et_system_setdebug(et_sys_id id, int debug)
{
  et_id *etid = (et_id *) id;
  
  if ((debug != ET_DEBUG_NONE)  && (debug != ET_DEBUG_SEVERE) &&
      (debug != ET_DEBUG_ERROR) && (debug != ET_DEBUG_WARN)   &&
      (debug != ET_DEBUG_INFO))  {
     return ET_ERROR;
  }
  etid->debug = debug;
  return ET_OK;
} 

/******************************************************/
int et_system_getdebug(et_sys_id id, int *debug)
{
  et_id *etid = (et_id *) id;
  
  if (debug != NULL) {
     *debug = etid->debug;
     return ET_OK;
  }
  return ET_ERROR;
} 

/******************************************************/

int et_system_getlocality(et_sys_id id, int *locality)
{
  et_id *etid = (et_id *) id;
  
  if (locality != NULL) {
     *locality = etid->locality;
     return ET_OK;
  }
  return ET_ERROR;
} 

/******************************************************/
int et_system_getnumevents(et_sys_id id, int *numevents)
{
  et_id *etid = (et_id *) id;
  
  if (numevents != NULL) {
     *numevents = etid->nevents;
     return ET_OK;
  }
  return ET_ERROR;
} 

/******************************************************/
int et_system_geteventsize(et_sys_id id, size_t *eventsize)
{
  et_id *etid = (et_id *) id;
  
  if (eventsize != NULL) {
     *eventsize = (size_t) etid->esize;
     return ET_OK;
  }
  return ET_ERROR;
} 

/******************************************************/
int et_system_gettempsmax(et_sys_id id, int *tempsmax)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_gettempsmax(id, tempsmax);
  }

  if (tempsmax != NULL) {
     *tempsmax = etid->sys->config.ntemps;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getstationsmax(et_sys_id id, int *stationsmax)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_getstationsmax(id, stationsmax);
  }

  if (stationsmax != NULL) {
     *stationsmax = etid->sys->config.nstations;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getprocsmax(et_sys_id id, int *procsmax)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_getprocsmax(id, procsmax);
  }

  if (procsmax != NULL) {
     *procsmax = etid->sys->config.nprocesses;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getattsmax(et_sys_id id, int *attsmax)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_getattsmax(id, attsmax);
  }

  if (attsmax != NULL) {
     *attsmax = etid->sys->config.nattachments;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getheartbeat(et_sys_id id, int *heartbeat)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_getheartbeat(id, heartbeat);
  }

  if (heartbeat != NULL) {
     *heartbeat = etid->sys->heartbeat;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getpid(et_sys_id id, pid_t *pid)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_getpid(id, (int *) pid);
  }

  if (pid != NULL) {
     *pid = etid->sys->mainpid;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getprocs(et_sys_id id, int *procs)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_getprocs(id, procs);
  }

  if (procs != NULL) {
     *procs = etid->sys->nprocesses;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getattachments(et_sys_id id, int *atts)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_getattachments(id, atts);
  }

  if (atts != NULL) {
     *atts = etid->sys->nattachments;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getstations(et_sys_id id, int *stations)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_getstations(id, stations);
  }

  if (stations != NULL) {
     *stations = etid->sys->nstations;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_gettemps(et_sys_id id, int *temps)
{
  et_id *etid = (et_id *) id;
  
  if (etid->locality != ET_LOCAL) {
    return etr_system_gettemps(id, temps);
  }

  if (temps != NULL) {
     *temps = etid->sys->ntemps;
     return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_getserverport(et_sys_id id, int *port)
{
  et_id *etid = (et_id *) id;

  if (port != NULL) {
    *port = etid->port;
    return ET_OK;
  }
  return ET_ERROR;
}

/******************************************************/
int et_system_gethost(et_sys_id id, char *host)
{
  et_id *etid = (et_id *) id;

  if (host != NULL) {
    strcpy(host, etid->ethost);
    return ET_OK;
  }
  return ET_ERROR;
}
