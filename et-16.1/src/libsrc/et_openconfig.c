/*----------------------------------------------------------------------------*
 *  Copyright (c) 2002        Southeastern Universities Research Association, *
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
 *      Routines for defining a configuration for opening ET systems.
 *
 *----------------------------------------------------------------------------*/


#include <stdio.h>
#include <string.h>

#include "et_private.h"

/*---------------------------------------------------*/
/*          OPEN SYSTEM CONFIGURATION                */
/*---------------------------------------------------*/

/**
 * @defgroup openConfig Open config
 *
 * These routines configure the options necessary to open an ET system.
 *
 * @{
 */

/**
 * This routine initializes a configuration used to open an ET system.
 *
 * This MUST be done prior to setting any configuration parameters or all setting
 * routines will return an error.
 *
 * @param sconfig   pointer to an open configuration variable
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if arg is NULL or failure to allocate memory for configuration data storage.
 */
int et_open_config_init(et_openconfig *sconfig) {

    et_open_config *sc;

    sc = (et_open_config *) calloc(1, sizeof(et_open_config));
    if (sconfig == NULL || sc == NULL) {
        return ET_ERROR;
    }

    /* default configuration for a station */
    sc->wait             = ET_OPEN_NOWAIT;
    sc->cast             = ET_BROADCAST;
    sc->ttl              = ET_MULTICAST_TTL;
    sc->mode             = ET_HOST_AS_LOCAL;
    sc->debug_default    = ET_DEBUG_ERROR;
    sc->udpport          = ET_UDP_PORT;
    sc->serverport       = ET_SERVER_PORT;
    sc->tcpSendBufSize   = 0;  /* use operating system default */
    sc->tcpRecvBufSize   = 0;  /* use operating system default */
    sc->tcpNoDelay       = 1;  /* on */
    sc->timeout.tv_sec   = 0;
    sc->timeout.tv_nsec  = 0;
    strcpy(sc->host, ET_HOST_LOCAL);
    sc->policy           = ET_POLICY_FIRST; /* use first response to broad/multicast */
    sc->mcastaddrs.count = 0;

    /* find our local subnets' broadcast addresses */
    if (etNetGetBroadcastAddrs(&sc->bcastaddrs, NULL) == ET_ERROR) {
        sc->bcastaddrs = NULL;
    }

    if (etNetGetNetworkInfo(&sc->netinfo, NULL) != ET_OK) {
        sc->netinfo = NULL;
        fprintf(stderr, "et_open_config_init: error in etNetGetNetworkInfo\n");
    }

    sc->init = ET_STRUCT_OK;

    *sconfig = (et_openconfig) sc;
    return ET_OK;
}

/**
 * This routine frees the memory allocated when an open configuration is initialized
 * by @ref et_open_config_init.
 *
 * @param sconfig   open configuration
 *
 * @returns @ref ET_OK
 */
int et_open_config_destroy(et_openconfig sconfig) {

    et_open_config *sc = (et_open_config *)sconfig;

    if (sc == NULL) return ET_OK;

    /* first, free network info (linked list) */
    etNetFreeIpAddrs(sc->netinfo);

    /* next, free broadcast info (linked list) */
    etNetFreeAddrList(sc->bcastaddrs);

    free(sc);
    return ET_OK;
}

/**
 * This routine sets the means to wait for a valid ET system to be detected.
 *
 * Setting val to @ref ET_OPEN_WAIT makes @ref et_open block by waiting until the given ET system
 * is fully functioning or a set time period has passed before returning. Setting val
 * to @ref ET_OPEN_NOWAIT makes et_open return immediately after determining whether the
 * ET system is alive or not. The default is ET_OPEN_NOWAIT.
 *
 * If the ET system is local, both ET_OPEN_WAIT and ET_OPEN_NOWAIT mean et_open will quickly
 * send a UDP broad/multicast packet to see if the system responds. If it does not, it tries
 * to detect a heartbeat which necessitates waiting at least one heartbeat. However, if the
 * system is remote, broad/multicasting to find its location may take up to several seconds.
 * Usually, if the ET system is up and running, this will only take a fraction of a second.
 * If a direct remote connection is being made, it is only tried once in the ET_OPEN_NOWAIT mode,
 * but it tries repeatedly at 10Hz until the set timeout in the ET_OPEN_WAIT mode.
 *
 * @param sconfig   open configuration.
 * @param val       ET_OPEN_WAIT or ET_OPEN_NOWAIT.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is not ET_OPEN_WAIT or ET_OPEN_NOWAIT.
 */
int et_open_config_setwait(et_openconfig sconfig, int val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_OPEN_NOWAIT) && (val != ET_OPEN_WAIT)) {
        return ET_ERROR;
    }

    sc->wait = val;
    return ET_OK;
}

/**
 * This routine gets the means being used to wait for a valid ET system to be detected.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer which get filled with either @ref ET_OPEN_WAIT or @ref ET_OPEN_NOWAIT.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getwait(et_openconfig sconfig, int *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->wait;
    return ET_OK;
}

/**
 * This routine sets the method for a remote user to discover the ET system.
 *
 * Setting the method to @ref ET_BROADCAST uses UDP broadcasting (the default),
 * @ref ET_MULTICAST uses UDP multicasting, @ref ET_BROADANDMULTICAST uses both, and
 * @ref ET_DIRECT uses a direct connection to the ET system by specifying the host and
 * server TCP port.
 *
 * To avoid broad/multicasting to find the ET system (actually to find the TCP port number
 * of ET's server), use the ET_DIRECT option which connects to ET directly.
 * A possible reason for wanting to avoid broad/multicasting is if the ET system
 * and user are on different subnets and routers will not pass the UDP packets between them.
 * If the ET_DIRECT option is taken, be aware that @ref et_open_config_sethost must use the ET system's
 * actual host name or "localhost" but must NOT be @ref ET_HOST_LOCAL, @ref ET_HOST_REMOTE,
 * or @ref ET_HOST_ANYWHERE. Use @ref et_open_config_setserverport to specify the TCP port number
 * of the ET server.
 *
 * @param sconfig   open configuration.
 * @param val       ET_MULTICAST, ET_BROADCAST, ET_BROADANDMULTICAST, or ET_DIRECT.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is not ET_MULTICAST, ET_BROADCAST, ET_BROADANDMULTICAST, or ET_DIRECT.
 */
int et_open_config_setcast(et_openconfig sconfig, int val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_MULTICAST) &&
        (val != ET_BROADCAST) &&
        (val != ET_BROADANDMULTICAST) &&
        (val != ET_DIRECT))  {
        return ET_ERROR;
    }

    sc->cast = val;
    return ET_OK;
}

/**
 * This routine gets the method for a remote user to discover the ET system.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer which is filled with @ref ET_BROADCAST, @ref ET_MULTICAST,
 *                  @ref ET_BROADANDMULTICAST, or @ref ET_DIRECT.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getcast(et_openconfig sconfig, int *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->cast;
    return ET_OK;
}

/**
 * This routine sets the "ttl" value for multicasting. This value determines how many "hops"
 * through routers the packet makes.
 *
 * It defaults to a value of 32 which should allow multicast packets to pass through local routers.
 *
 * @param sconfig   open configuration.
 * @param val       0 <= ttl value <= 254.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 0 or > 254.
 */
int et_open_config_setTTL(et_openconfig sconfig, int val)
{
  et_open_config *sc = (et_open_config *) sconfig;
  
  if (sc == NULL || sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if ((val < 0) || (val > 254))     {
    return ET_ERROR;
  }
  
  sc->ttl = val;
  return ET_OK;
}

/**
 * This routine gets the "ttl" value for multicasting.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer which is filled with ttl value.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getTTL(et_openconfig sconfig, int *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->ttl;
    return ET_OK;
}

/**
 * This routine sets whether the ET system treats the user running this routine as local
 * (if it is local) or remote even if it is local.
 *
 * Set val to @ref ET_HOST_AS_REMOTE if the local user is to be treated as remote.
 * This means all communication is done through the ET server using sockets.
 * The alternative is to set val to @ref ET_HOST_AS_LOCAL (default) which means local users
 * are treated as local with the ET system memory being mapped into the user's space.
 *
 * @param sconfig   open configuration.
 * @param val       ET_HOST_AS_REMOTE or ET_HOST_AS_LOCAL.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is not ET_HOST_AS_REMOTE or ET_HOST_AS_LOCAL.
 */
int et_open_config_setmode(et_openconfig sconfig, int val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_HOST_AS_LOCAL) &&
        (val != ET_HOST_AS_REMOTE))     {
        return ET_ERROR;
    }

    sc->mode = val;
    return ET_OK;
}

/**
 * This routine gets whether the ET system treats the user running this routine as local
 * (if it is local) or remote even if it is local.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer to be filled with @ref ET_HOST_AS_REMOTE or @ref ET_HOST_AS_LOCAL.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getmode(et_openconfig sconfig, int *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->mode;
    return ET_OK;
}

/**
 * This routine sets the default level of debug output.
 *
 * Set val to @ref ET_DEBUG_NONE for no output, @ref ET_DEBUG_SEVERE for output describing severe errors,
 * @ref ET_DEBUG_ERROR for output describing all errors, @ref ET_DEBUG_WARN for output describing warnings
 * and errors, and @ref ET_DEBUG_INFO for output describing all information, warnings, and errors.
 *
 * @param sconfig   open configuration.
 * @param val       ET_DEBUG_NONE, ET_DEBUG_SEVERE, ET_DEBUG_ERROR, ET_DEBUG_WARN, or ET_DEBUG_INFO.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is not ET_DEBUG_NONE, ET_DEBUG_SEVERE, ET_DEBUG_ERROR,
 *                          ET_DEBUG_WARN, or ET_DEBUG_INFO.
 */
int et_open_config_setdebugdefault(et_openconfig sconfig, int val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_DEBUG_NONE)  && (val != ET_DEBUG_SEVERE) &&
        (val != ET_DEBUG_ERROR) && (val != ET_DEBUG_WARN)   &&
        (val != ET_DEBUG_INFO))  {
        return ET_ERROR;
    }

    sc->debug_default = val;
    return ET_OK;
}

/**
 * This routine gets the default level of debug output.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer which is filled with @ref ET_DEBUG_NONE, @ref ET_DEBUG_SEVERE,
 *                  @ref ET_DEBUG_ERROR, @ref ET_DEBUG_WARN, or @ref ET_DEBUG_INFO.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getdebugdefault(et_openconfig sconfig, int *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->debug_default;
    return ET_OK;
}

/**
 * This routine sets the port number for the UDP broad/multicasting used to discover the ET system.
 *
 * The default is @ref ET_BROADCAST_PORT, but may be set to any number > 1023 and < 65536.
 *
 * @param sconfig   open configuration.
 * @param val       UDP port number.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 1024 and > 65535.
 */
int et_open_config_setport(et_openconfig sconfig, int val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 1024 || val > 65535) {
        return ET_ERROR;
    }

    sc->udpport = val;
    return ET_OK;
}

/**
 * This routine gets the port number for the UDP broad/multicasting used to discover the ET system.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer which gets filled with the UDP port number.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getport(et_openconfig sconfig, int *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->udpport;
    return ET_OK;
}

/**
 * This routine sets the TCP port number for opening an ET system directly without broadcasting or multicasting.
 *
 * The default is @ref ET_SERVER_PORT, but may be set to any number > 1023 and < 65536.
 * To avoid broadcasting or multicasting to find the ET system, use @ref et_open_config_setcast set to the
 * @ref ET_DIRECT option. This does no broad/multicast and connects directly to the ET system.
 * A possible reason for wanting to avoid broad/multicasting is if the  ET system and user are
 * on different subnets and routers will not pass the UDP packets between them.
 *
 * @param sconfig   open configuration.
 * @param val       TCP port number.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 1024 and > 65535.
 */
int et_open_config_setserverport(et_openconfig sconfig, int val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 1024 || val > 65535) {
        return ET_ERROR;
    }

    sc->serverport = val;
    return ET_OK;
}

/**
 * This routine gets the TCP port number for opening an ET system directly without broadcasting or multicasting.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer which gets filled with the TCP port number.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getserverport(et_openconfig sconfig, int *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->serverport;
    return ET_OK;
}

/**
 * This routine sets the maximum time to wait for a valid ET system to be detected.
 *
 * If @ref et_open_config_setmode was set to @ref ET_OPEN_WAIT, then this routine
 * sets the maximum amount of time to wait for an alive ET system to be detected.
 * If the time is set to zero (the default), an infinite time is indicated.
 *
 * Note that in local systems, @ref et_open waits in integral units of the
 * system's heartbeat time (default of 1/2 second).
 * If broad/multicasting to find a remote ET system, it is possible to take up to several
 * seconds to determine whether the system is alive or not. In this case, the time limit
 * may be exceeded.
 *
 * @param sconfig   open configuration.
 * @param val       time to wait (0 = infinite).
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 */
int et_open_config_settimeout(et_openconfig sconfig, struct timespec val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    sc->timeout = val;
    return ET_OK;
}

/**
 * This routine gets the maximum time to wait for a valid ET system to be detected.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer which gets filled max wait time.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_gettimeout(et_openconfig sconfig, struct timespec *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->timeout;
    return ET_OK;
}

/**
 * This routine adds an IP subnet broadcast address to a list of destinations used in broadcast
 * discovery of the ET system.
 *
 * Broadcasting is only used if @ref et_open_config_setcast is set to @ref ET_BROADCAST or
 * @ref ET_BROADANDMULTICAST.
 *
 * The broadcast address must be in dot-decimal form. If this routine is never called,
 * the list of addresses is automatically set to all the local subnet broadcast addresses.
 * However, if finding these addresses fails, then the list is set to a single entry of
 * @ref ET_BROADCAST_ADDR. This argument may also be set to @ref ET_SUBNET_ALL which
 * specifies all the local subnet broadcast addresses.
 *
 * @param sconfig   open configuration.
 * @param val       broadcast address in dot-decimal form.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL, sconfig not initialized,
 *                          val is not dot-decimal format, or cannot allocate memory.
 */
int et_open_config_addbroadcast(et_openconfig sconfig, const char *val) {

    et_open_config *sc = (et_open_config *) sconfig;
    codaIpList *pBcastAddr, *pLast=NULL;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    /* Put all subnet addresses into the list. */
    if (strcmp(val, ET_SUBNET_ALL) == 0) {
        /* free existing broadcast info */
        etNetFreeAddrList(sc->bcastaddrs);
        if (etNetGetBroadcastAddrs(&sc->bcastaddrs, NULL) == ET_ERROR) {
            sc->bcastaddrs = NULL;
        }
        return ET_OK;
    }

    /* If we're here, user wants to add a specific subnet. */

    /* Must be in dot decimal format. */
    if (!codanetIsDottedDecimal(val, NULL)) {
        fprintf(stderr, "et_open_config_addbroadcast: address must be in dot-decimal form\n");
        return ET_ERROR;
    }

    /* Once here, it's probably an address of the right value.
     * Don't add it to the list if it's already there. */

    /* if nothing on list, add it */
    if (sc->bcastaddrs == NULL) {
        pBcastAddr = (codaIpList *) calloc(1, sizeof(codaIpList));
        if (pBcastAddr == NULL) {
            return ET_ERROR;
        }
        strcpy(pBcastAddr->addr, val);
        sc->bcastaddrs = pBcastAddr;
        return ET_OK;
    }

    pBcastAddr = sc->bcastaddrs;
    while (pBcastAddr != NULL) {
        if (strcmp(val, pBcastAddr->addr) == 0) {
            /* it's already in the list */
            return ET_OK;
        }
        pLast = pBcastAddr;
        pBcastAddr = pBcastAddr->next;
    }

    /* not in list so put on end of list */
    pBcastAddr = (codaIpList *) calloc(1, sizeof(codaIpList));
    if (pBcastAddr == NULL) {
        return ET_ERROR;
    }
    strcpy(pBcastAddr->addr, val);
    pLast->next = pBcastAddr;

    return ET_OK;
}

/**
 * This routine removes an IP subnet broadcast address from a list of destinations used in broadcast
 * discovery of the ET system.
 *
 * Broadcasting is only used if @ref et_open_config_setcast is set to @ref ET_BROADCAST or
 * @ref ET_BROADANDMULTICAST.
 *
 * The subnet broadcast IP address must be in dotted-decimal form. If there is no such address
 * on the list, it is ignored. This argument may also be set to @ref ET_SUBNET_ALL which specifies
 * all the local subnet broadcast addresses.
 *
 * @param sconfig   open configuration.
 * @param val       broadcast address in dot-decimal form.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL, sconfig not initialized,
 *                          val is not dot-decimal format.
 */
int et_open_config_removebroadcast(et_openconfig sconfig, const char *val) {

    et_open_config *sc = (et_open_config *) sconfig;
    codaIpList *pBcastAddr, *pPrev=NULL;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    /* if nothing on list, we're done */
    if (sc->bcastaddrs == NULL) {
        return ET_OK;
    }

    /* Remove all subnet addresses from the list. */
    if (strcmp(val, ET_SUBNET_ALL) == 0) {
        /* free existing broadcast info */
        etNetFreeAddrList(sc->bcastaddrs);
        sc->bcastaddrs = NULL;
        return ET_OK;
    }

    /* If we're here, user wants to remove a specific subnet. */

    /* Must be in dot decimal format. */
    if (!codanetIsDottedDecimal(val, NULL)) {
        fprintf(stderr, "et_open_config_removebroadcast: address must be in dot-decimal form\n");
        return ET_ERROR;
    }

    pBcastAddr = sc->bcastaddrs;
    while (pBcastAddr != NULL) {
        /* if it's in the list ... */
        if (strcmp(val, pBcastAddr->addr) == 0) {
            /* if we're at the head ... */
            if (pPrev == NULL) {
                /* next is now the head */
                sc->bcastaddrs = pBcastAddr->next;
            }
            else {
                pPrev->next = pBcastAddr->next;
            }

            /* free what we are removing */
            free(pBcastAddr);

            break;
        }

        pPrev = pBcastAddr;
        pBcastAddr = pBcastAddr->next;
    }

    return ET_OK;
}

/**
 * This routine adds an IP multicast address to a list of destinations used in multicast
 * discovery of the ET system.
 *
 * Multicasting is only used if @ref et_open_config_setcast is set to @ref ET_MULTICAST or
 * @ref ET_BROADANDMULTICAST.
 *
 * The multicast address must be in dot-decimal form. There can be at most @ref ET_MAXADDRESSES
 * addresses in the list. Duplicate entries are not added to the list.
 *
 * @param sconfig   open configuration.
 * @param val       multicast address in dot-decimal form.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL, sconfig not initialized,
 *                          val is not in valid multicast format, or list is full.
 */
int et_open_config_addmulticast(et_openconfig sconfig, const char *val)
{
    et_open_config *sc = (et_open_config *) sconfig;
    int  i, intVals[4];

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    if ((strlen(val) >= ET_IPADDRSTRLEN) || (strlen(val) < 7)) {
        return ET_ERROR;
    }

    if (!codanetIsDottedDecimal(val, intVals)) {
        fprintf(stderr, "et_open_config_addmulticast: address must be in dot-decimal form\n");
        return ET_ERROR;
    }

    /* The address is in dotted-decimal form. If the first
     * number is between 224-239, its a multicast address.
     */
    if ((intVals[0] < 224) || (intVals[0] > 239)) {
        fprintf(stderr, "et_open_config_addmulticast: bad value for address\n");
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

/**
 * This routine removes a multicast address from a list of destinations used in multicast
 * discovery of the ET system.
 *
 * Multicasting is only used if @ref et_open_config_setcast is set to @ref ET_MULTICAST or
 * @ref ET_BROADANDMULTICAST.
 *
 * The multicast address must be in dot-decimal form. If there is no such address
 * on the list, it is ignored.
 *
 * @param sconfig   open configuration.
 * @param val       multicast address in dot-decimal form.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL, sconfig not initialized,
 *                          val is not in valid multicast format.
 */
int et_open_config_removemulticast(et_openconfig sconfig, const char *val)
{
    et_open_config *sc = (et_open_config *) sconfig;
    size_t len;
    int  firstnumber, i, j;
    char firstint[4];

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
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

/**
 * This routine sets the return policy from calls to @ref et_open.
 *
 * If an et_open generates more than one response from one or more ET systems,
 * this policy can be set to return an error, open the first system to respond,
 * or open the first local system to respond.
 *
 * One ET system may respond multiple times to a single et_open by a user if it is connected
 * to more than one subnet or is listening to multiple multicast addresses. It is also possible that
 * if a user is configured for @ref ET_HOST_REMOTE or @ref ET_HOST_ANYWHERE, several ET systems may
 * have the same name and respond with their port numbers and host names. Thus, the user needs to
 * determine the best way to respond to such situations.
 *
 * The policy can be set to @ref ET_POLICY_ERROR if the user wants et_open to return an error
 * if more than one response is received from its broad or multicast attempt to find an ET system.
 * Set it to @ref ET_POLICY_FIRST if the first responding system is the one to be opened.
 * Else, set it to @ref ET_POLICY_LOCAL if the first responding local system is opened if
 * there is one, and if not, the first responding system.
 *
 * @param sconfig   open configuration.
 * @param val       return policy: ET_POLICY_ERROR, ET_POLICY_FIRST, or ET_POLICY_LOCAL.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is not ET_POLICY_ERROR, ET_POLICY_FIRST, or ET_POLICY_LOCAL.
 */
int et_open_config_setpolicy(et_openconfig sconfig, int val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_POLICY_FIRST) &&
        (val != ET_POLICY_LOCAL) &&
        (val != ET_POLICY_ERROR))  {
        return ET_ERROR;
    }

    sc->policy = val;
    return ET_OK;
}

/**
 * This routine gets the return policy from calls to @ref et_open.
 *
 * @see et_open_config_setpolicy.
 *
 * @param sconfig   open configuration.
 * @param val       int pointer which is filled with ET_POLICY_ERROR, ET_POLICY_FIRST, or ET_POLICY_LOCAL.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getpolicy(et_openconfig sconfig, int *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->policy;
    return ET_OK;
}

/**
 * This routine sets host or computer on which the ET system is running.
 *
 * The host is the name of the computer on which the ET system resides.
 * For opening a local system only, set val to @ref ET_HOST_LOCAL (the default) or
 * "localhost" (including quotes). For opening a system on an unknown remote computer only,
 * set it to @ref ET_HOST_REMOTE. For an unknown host which may be local or remote,
 * set it to @ref ET_HOST_ANYWHERE. Otherwise set val to the name or dotted-decimal
 * IP address of the desired host. If the @ref ET_DIRECT option is taken in
 * @ref et_open_config_setcast, be aware that this routine must use the ET system's
 * actual host name or "localhost" but must NOT be ET_HOST_LOCAL, ET_HOST_REMOTE, or
 * ET_HOST_ANYWHERE.
 *
 * @param sconfig   open configuration.
 * @param val       host name, "localhost", ET_HOST_LOCAL, ET_HOST_REMOTE, or ET_HOST_ANYWHERE.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *                          if val is longer than @ref ET_FUNCNAME_LENGTH - 1.
 */
int et_open_config_sethost(et_openconfig sconfig, const char *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    if (strlen(val) > ET_FUNCNAME_LENGTH-1) {
        return ET_ERROR;
    }

    strcpy(sc->host, val);
    return ET_OK;
}

/**
 * This routine gets the host or computer on which the ET system is running.
 *
 * @see et_open_config_sethost
 *
 * @param sconfig   open configuration.
 * @param val       char pointer which is filled with host name.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_gethost(et_openconfig sconfig, char *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    strcpy(val, sc->host);
    return ET_OK;
}

/**
 * This routine sets the network interface to use when communicating over the network
 * with the ET system.
 *
 * Network communications with the ET system is only used when the ET system host is not local,
 * the mode is @ref ET_HOST_AS_REMOTE, or a Java ET system has been opened. The interface must be
 * specified in dot-decimal form. In addition to being a valid, local IP address, the
 * given address may be a network (subnet) address. If it is such, it will convert that to the
 * specific IP associated with it.
 *
 * @param sconfig   open configuration.
 * @param val       valid local or network dot-decimal IP address
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *                          if val is not in dot-decimal form.
 */
int et_open_config_setinterface(et_openconfig sconfig, const char *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    if (!etNetIsDottedDecimal(val, NULL)) {
        return ET_ERROR;
    }

    if (strlen(val) > ET_IPADDRSTRLEN-1) {
        return ET_ERROR;
    }

    strcpy(sc->interface, val);
    return ET_OK;
}

/**
 * This routine gets the network interface being used to communicate over the network
 * with the ET system.
 *
 * @see et_open_config_setinterface
 *
 * @param sconfig   open configuration.
 * @param val       char pointer which is filled with network IP address (NULL if none).
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_open_config_getinterface(et_openconfig sconfig, char *val) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    if (sc->interface == NULL) {
        *val = '\0';
        return ET_OK;
    }

    strncpy(val, sc->interface, ET_IPADDRSTRLEN-1);
    return ET_OK;
}

/**
 * This routine sets the parameters of any TCP connection to the ET system.
 *
 * These include the TCP receiving buffer size in bytes, the TCP sending buffer size in bytes,
 * and the TCP NODELAY value. If either of the buffer sizes are 0, then the operating system
 * default values are used. Setting NODELAY to 0, turns it off, anything else turns it on
 * (which sends a write immediately over the network without waiting for further writes).
 *
 * @param sconfig    open configuration.
 * @param  rBufSize  TCP receiving buffer size in bytes.
 * @param  sBufSize  TCP sending buffer size in bytes.
 * @param  noDelay   boolean value which is true (non-zero) for no delay of TCP writes, else false (zero).
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if sizes are negative numbers.
 */
int et_open_config_settcp(et_openconfig sconfig, int rBufSize, int sBufSize, int noDelay) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK ) {
        return ET_ERROR;
    }

    if (rBufSize < 0 || sBufSize < 0) {
        return ET_ERROR;
    }

    if (noDelay != 0) noDelay = 1;

    sc->tcpRecvBufSize = rBufSize;
    sc->tcpSendBufSize = sBufSize;
    sc->tcpNoDelay = noDelay;

    return ET_OK;
}

/**
 * This routine gets the parameters of any TCP connection to the ET system.
 *
 * These include the TCP receiving buffer size in bytes, the TCP sending buffer
 * size in bytes, and the TCP NODELAY value.
 *
 * @see et_open_config_settcp
 *
 * @param sconfig   open configuration.
 * @param rBufSize  int pointer that gets filled with the TCP receiving buffer size in bytes.
 * @param sBufSize  int pointer that gets filled with the TCP sending buffer size in bytes.
 * @param noDelay   int pointer that gets filled with a boolean value, which is true (1)
 *                  for no delay of TCP writes, else false (0).
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig arg is NULL or not initialized.
 */
int et_open_config_gettcp(et_openconfig sconfig, int *rBufSize, int *sBufSize, int *noDelay) {

    et_open_config *sc = (et_open_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK ) {
        return ET_ERROR;
    }

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


/** @} */
