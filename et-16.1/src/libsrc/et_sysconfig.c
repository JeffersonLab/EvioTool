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

#include "et_private.h"

/*****************************************************
 * Routines for setting and getting system configuration
 * parameters. Used to create systems..
 *****************************************************/

/**
 * @addtogroup Developer Developer routines
 *
 * These routines are for developers.
 *
 * @{
 */

/**
 * @defgroup systemConfig System config
 *
 * These routines are for configuring an ET system.
 *
 * @{
 */


/**
 * This routine initializes a configuration used to define an ET system.
 *
 * This MUST be done prior to setting any configuration parameters or all setting
 * routines will return an error.
 *
 * @param sconfig   pointer to system configuration variable.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if arg is NULL or failure to allocate memory for configuration data storage.
 * @returns @ref ET_ERROR_NOMEM  if there is not enough memory allocated to store network info.
 *                               Recompile code with bigger value of ET_MAXADDRESSES.
 */
int et_system_config_init(et_sysconfig* sconfig) {

    et_sys_config *sc;

    sc = (et_sys_config *) calloc(1, sizeof(et_sys_config));
    if (sconfig == NULL || sc == NULL) {
        return ET_ERROR;
    }

    /* default configuration for a system */
    sc->nevents          = ET_SYSTEM_EVENTS;
    sc->event_size       = ET_SYSTEM_ESIZE;
    sc->ntemps           = ET_SYSTEM_NTEMPS;
    sc->nstations        = ET_SYSTEM_NSTATS;
    sc->nprocesses       = ET_PROCESSES_MAX;
    sc->nattachments     = ET_ATTACHMENTS_MAX;
    sc->port             = ET_UDP_PORT;
    sc->serverport       = ET_SERVER_PORT;
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


/**
 * This routine frees the memory allocated when a system configuration is initialized
 * by @ref et_system_config_init.
 *
 * @param sconfig   system configuration.
 *
 * @returns @ref ET_OK.
 */
int et_system_config_destroy(et_sysconfig sconfig) {

    et_sys_config *sc = (et_sys_config *)sconfig;

    if (sc == NULL) return ET_OK;

    free(sc);
    return ET_OK;
}


/**
 * This routine sets the configuration's total number of events.
 *
 * @param sconfig   system configuration.
 * @param val       any positive integer.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 1.
 */
int et_system_config_setevents(et_sysconfig sconfig, int val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
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


/**
 * This routine gets the configuration's total number of events.
 *
 * @param sconfig   system configuration.
 * @param val       int pointer which gets filled with number of events.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_system_config_getevents(et_sysconfig sconfig, int *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = sc->nevents;
    return ET_OK;
}


/**
 * This routine sets the configuration's event size in bytes.
 *
 * @param sconfig   system configuration.
 * @param val       any positive integer within the bounds of available memory.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 1.
 */
int et_system_config_setsize(et_sysconfig sconfig, size_t val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 1) {
        return ET_ERROR;
    }

    sc->event_size = val;
    return ET_OK;
}


/**
 * This routine gets the configuration's event size in bytes.
 *
 * @param sconfig   system configuration.
 * @param val       pointer which gets filled with event size in bytes.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_system_config_getsize(et_sysconfig sconfig, size_t *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = (size_t) sc->event_size;
    return ET_OK;
}


/**
 * This routine sets the configuration's maximum number of temporary events.
 * The max number of temp events must not be greater than the total number of events.
 * Temp events are created when a user calls @ref et_events_new() with a larger memory
 * size than the normal events created in the main shared memory.
 * Their creation is expensive and the ET system operates
 * much faster by increasing the size of normal events in order to avoid
 * creating any temp events at all.
 *
 * @param sconfig   system configuration.
 * @param val       any positive integer <= total number of events.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 1 or > total number of events.
 */
int et_system_config_settemps(et_sysconfig sconfig, int val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 1 || val > sc->nevents) {
        return ET_ERROR;
    }

    sc->ntemps = val;
    return ET_OK;
}


/**
 * This routine gets the configuration's maximum number of temporary events.
 *
 * @param sconfig   system configuration.
 * @param val       int pointer which gets filled with the maximum number of temporary events.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 *
 * @see et_system_config_settemps.
 */
int et_system_config_gettemps(et_sysconfig sconfig, int *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = sc->ntemps;
    return ET_OK;
}


/**
 * This routine sets the configuration's maximum number of stations that may be created
 * (including GRAND_CENTRAL).
 *
 * @param sconfig   system configuration.
 * @param val       any positive integer.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 1.
 */
int et_system_config_setstations(et_sysconfig sconfig, int val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 1) {
        return ET_ERROR;
    }

    sc->nstations = val;
    return ET_OK;
}


/**
 * This routine gets the configuration's maximum number of stations that may be created
 * (including GRAND_CENTRAL).
 *
 * @param sconfig   system configuration.
 * @param val       int pointer which gets filled with the maximum number of stations.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_system_config_getstations(et_sysconfig sconfig, int *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = sc->nstations;
    return ET_OK;
}


/**
 * This routine sets the configuration's maximum number of processes that may be created.
 *
 * The number of processes is just the number of @ref et_open calls
 * successfully returned on the same host as the ET system. The
 * system must be one which can allow many processes to share the
 * ET mutexes, thus allowing full access to the shared memory. On
 * MacOS this is not possible, meaning that the local user does NOT
 * have the complete access to the shared memory.
 * This routine sets the limit, but is capped at @ref ET_PROCESSES_MAX which may be
 * changed in et_private.h, requiring a recompile of the ET system library.
 *
 * @param sconfig   system configuration.
 * @param val       any positive integer, if > ET_PROCESSES_MAX,
 *                  then it is set to ET_PROCESSES_MAX.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 1.
 */
int et_system_config_setprocs(et_sysconfig sconfig, int val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
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


/**
 * This routine gets the configuration's maximum number of processes that may be created.
 *
 * @param sconfig   system configuration.
 * @param val       int pointer which gets filled with the maximum number of processes.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 *
 * @see et_system_config_setprocs.
 */
int et_system_config_getprocs(et_sysconfig sconfig, int *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = sc->nprocesses;
    return ET_OK;
}


/**
 * This routine sets the configuration's maximum number of attachments that may be created.
 *
 * The number of attachments is the number of local @ref et_station_attach
 * calls successfully returned - each providing an access to events from a
 * certain station.
 * This routine sets the limit, but is capped at @ref ET_ATTACHMENTS_MAX which may be
 * changed in et_private.h, requiring a recompile of the ET system library.
 *
 * @param sconfig   system configuration.
 * @param val       any positive integer, if > ET_ATTACHMENTS_MAX,
 *                  then it is set to ET_ATTACHMENTS_MAX.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is < 1.
 */
int et_system_config_setattachments(et_sysconfig sconfig, int val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
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


/**
 * This routine gets the configuration's maximum number of attachments that may be created.
 *
 * @param sconfig   system configuration.
 * @param val       int pointer which gets filled with the maximum number of attachments.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 *
 * @see et_system_config_setattachments.
 */
int et_system_config_getattachments(et_sysconfig sconfig, int *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = sc->nattachments;
    return ET_OK;
}


/**
 * This routine sets the configuration's UDP listening port used by remote users trying
 * to find the ET system by broad/multicasting.
 *
 * @param sconfig   system configuration.
 * @param val       port number of the broadcast or multicast communications.
 *                  The default is @ref ET_BROADCAST_PORT, defined in et.h.
 *                  It may also be set to any available port number.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val < 1024 or > 65535.
 */
int et_system_config_setport(et_sysconfig sconfig, int val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 1024 || val > 65535) {
        return ET_ERROR;
    }

    sc->port = val;
    return ET_OK;
}


/**
 * This routine gets the configuration's UDP listening port used by remote users trying
 * to find the ET system by broad/multicasting.
 *
 * @param sconfig   system configuration.
 * @param val       int pointer which gets filled with the UDP listening port.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 *
 * @see et_system_config_setport.
 */
int et_system_config_getport(et_sysconfig sconfig, int *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = sc->port;
    return ET_OK;
}


/**
 * This routine sets the configuration's TCP listening port number
 * used to communicate with remote users.
 *
 * If not set here, the port defaults to @ref ET_SERVER_PORT (defined in et.h).
 * The ET system process exits with an error if the port is not available.
 *
 * @param sconfig   system configuration.
 * @param val       port number of the TCP communications which
 *                  may be set to any available port number.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val < 1024 or > 65535.
 */
int et_system_config_setserverport(et_sysconfig sconfig, int val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

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
 * This routine gets the configuration's TCP listening port number
 * used to communicate with remote users.
 *
 * @param sconfig   system configuration.
 * @param val       int pointer which gets filled with the TCP listening port.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 *
 * @see et_system_config_setserverport.
 */
int et_system_config_getserverport(et_sysconfig sconfig, int *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = sc->serverport;
    return ET_OK;
}


/**
 * This routine sets the configuration's number of event groups and how many events
 * are in each group.
 *
 * To prevent ET clients who grab new events from competing with each
 * other for these new events, an ET system can divide events into groups. The given array
 * specifies how many events are in each group. Groups are numbered sequentially from 1
 * (corresponding to the element group[0]). Clients may either call @ref et_system_setgroup to
 * specify which group to get events from when calling @ref et_events_new, or clients may call
 * @ref et_events_new_group to obtain new events from a specific group regardless of any default
 * group set by et_system_setgroup.
 *
 * @param sconfig   system configuration.
 * @param groups    array containing the number of events in each group. The index into the array
 *                  plus one gives the group number. The first <b>size</b> number of elements must be
 *                  positive integers. The sum of the first <b>size</b> elements must equal the total
 *                  number of events in the system.
 * @param size      number of array elements in group to be used.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if groups == NULL or element of group < 1;
 *                          if size > @ref ET_EVENT_GROUPS_MAX or < 1.
 */
int et_system_config_setgroups(et_sysconfig sconfig, int groups[], int size) {

    et_sys_config *sc = (et_sys_config *) sconfig;
    int i;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
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


/**
 * This routine sets the configuration's TCP socket options
 * used when communicating with remote users over sockets.
 *
 * The receive and send buffer sizes in bytes and the TCP_NODELAY
 * values can be set. If a buffer size is set to 0, the operating
 * system default is used.
 *
 * @param sconfig   system configuration.
 * @param rBufSize  TCP socket receive buffer size in bytes.
 * @param sBufSize  TCP socket send buffer size in bytes.
 * @param noDelay   if 0, TCP_NODELAY option is not set, else it is.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if a buffer size < 0.
 */
int et_system_config_settcp(et_sysconfig sconfig, int rBufSize, int sBufSize, int noDelay) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
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
 * This routine gets the configuration's TCP socket options
 * used when communicating with remote users over sockets.
 *
 * @param sconfig   system configuration.
 * @param rBufSize  int pointer which gets filled with the TCP socket receive buffer size in bytes.
 * @param sBufSize  int pointer which gets filled with the TCP socket send buffer size in bytes.
 * @param noDelay   int pointer which gets filled with 1 if TCP_NODELAY option is set, else 0.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized.
 *
 * @see et_system_config_settcp.
 */
int et_system_config_gettcp(et_sysconfig sconfig, int *rBufSize, int *sBufSize, int *noDelay)
{
    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
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


/*----------------------------------------------------------------
 * This routine together with et_system_config_removemulticast
 * replaces et_system_config_setaddress.
 *----------------------------------------------------------------*/

/**
 * This routine adds a multicast address to a list, each address of which the
 * ET system is listening on for UDP packets from users trying to find it.
 * No more than @ref ET_MAXADDRESSES number of multicast addresses can be
 * added. Duplicate entries are not added to the list.
 *
 * @param sconfig   system configuration.
 * @param val       multicast address in dot-decimal form.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is NULL or is not in valid dot-decimal form;
 *                          if ET_MAXADDRESSES number of multicast addresses
 *                          have already been added.
 */
int et_system_config_addmulticast(et_sysconfig sconfig, const char *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;
    size_t len;
    int  firstnumber, i;
    char firstint[4];

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val == NULL) || (strlen(val) >= ET_IPADDRSTRLEN) || (strlen(val) < 7)) {
        return ET_ERROR;
    }

    /* The address is in dotted-decimal form. If the first
     * number is between 224-239, its a multicast address. */
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
     * there's no more room on the list. */
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
 * This routine removes a multicast address from a list, so that the ET system
 * no longer listens on it for UDP packets from users trying to find the system.
 *
 * @param sconfig   system configuration.
 * @param val       multicast address in dot-decimal form.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is NULL or is not in valid dot-decimal form.
 *
 * @see et_system_config_addmulticast.
 */
int et_system_config_removemulticast(et_sysconfig sconfig, const char *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;
    size_t len;
    int  firstnumber, i, j;
    char firstint[4];

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val == NULL) || (strlen(val) >= ET_IPADDRSTRLEN) || (strlen(val) < 7)) {
        return ET_ERROR;
    }

    /* The address is in dotted-decimal form. If the first
     * number is between 224-239, its a multicast address. */
    len = strcspn(val, ".");
    strncpy(firstint, val, len);
    firstint[len] = '\0';
    firstnumber = atoi(firstint);

    if ((firstnumber < 224) || (firstnumber > 239)) {
        fprintf(stderr, "et_open_config_addmulticast: bad value for multicast address\n");
        return ET_ERROR;
    }

    /* Once here, it's probably an address of the right value.
     * Remove it from the list only if it's already there. */
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
 * This routine sets the configuration's ET system file name.
 * The name can be no longer than (@ref ET_FILENAME_LENGTH - 1)
 * characters in length.
 *
 * @param sconfig   system configuration.
 * @param val       ET system file name.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or not initialized;
 *                          if val is NULL or > (ET_FILENAME_LENGTH - 1) chars.
 */
int et_system_config_setfile(et_sysconfig sconfig, const char *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val == NULL || strlen(val) > ET_FILENAME_LENGTH - 1) {
        return ET_ERROR;
    }

    strcpy(sc->filename, val);

    return ET_OK;
}


/**
 * This routine gets the configuration's ET system file name.
 *
 * @param sconfig   system configuration.
 * @param val       pointer which gets filled with the ET file name.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 *
 * @see et_system_config_setfile.
 */
int et_system_config_getfile(et_sysconfig sconfig, char *val) {

    et_sys_config *sc = (et_sys_config *) sconfig;

    if (val == NULL || sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    strcpy(val, sc->filename);
    return ET_OK;
}

/** @} */
/** @} */


/**
 * @defgroup EtSystemUser System
 *
 * These routines are called by the ET system user in order to get or modify
 * parameters of the ET system itself.
 *
 * @{
 */


/*----------------------------------------------------*/
/* Routines for setting/getting system information    */
/* from an existing system.                           */
/*----------------------------------------------------*/


/**
 * This routine sets the default group number of the ET client.
 *
 * When the group is set to 0, calls to @ref et_event_new or @ref et_events_new gets
 * events of any group. When the group number is set to a positive number, the same
 * calls only get events from the given group.
 *
 * @param sconfig   system id.
 * @param val       ET system file name.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if id is NULL or not initialized;
 *                               if group < 0 or > # of groups;
 *                               if remote communication error.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_setgroup(et_sys_id id, int group) {

    et_id *etid = (et_id *) id;
    int err, groupCount;

    if (etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

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


/**
 * This routine gets the default group number of the ET client.
 *
 * @param sconfig   system id.
 * @param group     int pointer which gets filled with the default group number.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or id not initialized.
 *
 * @see et_system_setgroup.
 */
int et_system_getgroup(et_sys_id id, int *group) {

    et_id *etid = (et_id *) id;

    if (group == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *group = etid->group;
    return ET_OK;
}


/**
 * This routine sets the level of debug output for the ET client.
 *
 * The allowed values are:
 * <ul>
 *     <li>@ref ET_DEBUG_NONE  -  no output</li>
 *     <li>@ref ET_DEBUG_SEVERE  -  output only for severe errors</li>
 *     <li>@ref ET_DEBUG_ERROR  -  output for all errors</li>
 *     <li>@ref ET_DEBUG_WARN  -  output for warnings and all errors</li>
 *     <li>@ref ET_DEBUG_INFO  -  output for all information, warnings, and errors</li>
 *</ul>
 *
 * @param id     system id.
 * @param debug  level of debug output which may be one of ET_DEBUG_NONE,
 *               ET_DEBUG_SEVERE, ET_DEBUG_ERROR, ET_DEBUG_WARN, or ET_DEBUG_INFO.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if id is NULL or not initialized;
 *                          if debug is not one of the allowed values.
 */
int et_system_setdebug(et_sys_id id, int debug) {

    et_id *etid = (et_id *) id;

    if (etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((debug != ET_DEBUG_NONE)  && (debug != ET_DEBUG_SEVERE) &&
        (debug != ET_DEBUG_ERROR) && (debug != ET_DEBUG_WARN)   &&
        (debug != ET_DEBUG_INFO))  {
        return ET_ERROR;
    }
    etid->debug = debug;
    return ET_OK;
}


/**
 * This routine gets the level of debug output for the ET client.
 *
 * @param id      system id.
 * @param debug   int pointer which gets filled with the debug output level which may be one of
 *                ET_DEBUG_NONE, ET_DEBUG_SEVERE, ET_DEBUG_ERROR, ET_DEBUG_WARN, or ET_DEBUG_INFO.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or id not initialized.
 *
 * @see et_system_setdebug.
 */
int et_system_getdebug(et_sys_id id, int *debug) {

    et_id *etid = (et_id *) id;

    if (debug == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *debug = etid->debug;
    return ET_OK;
}


/**
 * This routine gets the locality of the ET client.
 *
 * A return value of @ref ET_REMOTE indicates it's on a different host,
 * @ref ET_LOCAL if it's on the same host, or @ref ET_LOCAL_NOSHARE if it's
 * on the same host but unable to share pthread mutexes with the ET system.
 *
 * @param id         system id.
 * @param locality   int pointer which gets filled with the locality value which may be one of
 *                   ET_LOCAL, ET_REMOTE, or ET_LOCAL_NOSHARE.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or id not initialized.
 */
int et_system_getlocality(et_sys_id id, int *locality) {

    et_id *etid = (et_id *) id;

    if (locality == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *locality = etid->locality;
    return ET_OK;
}


/**
 * This routine gets the number of events in the ET system.
 *
 * @param id          system id.
 * @param numevents   int pointer which gets filled with the number of events.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or id not initialized.
 */
int et_system_getnumevents(et_sys_id id, int *numevents) {

    et_id *etid = (et_id *) id;

    if (numevents == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *numevents = etid->nevents;
    return ET_OK;
}


/**
 * This routine gets the size of an event in bytes in the ET system.
 *
 * @param id          system id.
 * @param eventsize   pointer which gets filled with the size of an event.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or id not initialized.
 */
int et_system_geteventsize(et_sys_id id, size_t *eventsize) {

    et_id *etid = (et_id *) id;

    if (eventsize == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *eventsize = (size_t) etid->esize;
    return ET_OK;
}


/**
 * This routine gets the maximum number of temporary events allowed in the ET system.
 * If the system is Java-based, this call is meaningless and returns a value of 0 since
 * there is no shared memory and no temp events.
 *
 * @param id          system id.
 * @param tempsmax    int pointer which gets filled with the maximum number of temporary events
 *                    allowed.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_gettempsmax(et_sys_id id, int *tempsmax) {

    et_id *etid = (et_id *) id;

    if (tempsmax == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_gettempsmax(id, tempsmax);
    }

    *tempsmax = etid->sys->config.ntemps;
    return ET_OK;
}


/**
 * This routine gets the maximum number of stations allowed in the ET system.
 *
 * @param id             system id.
 * @param stationsmax    int pointer which gets filled with the maximum number of stations allowed.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_getstationsmax(et_sys_id id, int *stationsmax) {

    et_id *etid = (et_id *) id;

    if (stationsmax == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_getstationsmax(id, stationsmax);
    }

    *stationsmax = etid->sys->config.nstations;
    return ET_OK;
}


/**
 * This routine gets the maximum number of local processes that are allowed to open the ET system.
 * If the system is Java-based, this call is meaningless and returns a value of 0 since there is
 * no shared memory.
 *
 * @param id             system id.
 * @param procsmax       int pointer which gets filled with the maximum number of local processes
 *                       allowed.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_getprocsmax(et_sys_id id, int *procsmax) {

    et_id *etid = (et_id *) id;

    if (procsmax == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_getprocsmax(id, procsmax);
    }

    *procsmax = etid->sys->config.nprocesses;
    return ET_OK;
}


/**
 * This routine gets the maximum number of attachments that are allowed to be made to the
 * ET system.
 *
 * @param id             system id.
 * @param attsmax        int pointer which gets filled with the maximum number of attachments
 *                       allowed.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_getattsmax(et_sys_id id, int *attsmax) {

    et_id *etid = (et_id *) id;

    if (attsmax == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_getattsmax(id, attsmax);
    }

    *attsmax = etid->sys->config.nattachments;
    return ET_OK;
}


/**
 * This routine gets the current value of the heartbeat of the ET system.
 * For a healthy C-based system, this value changes every heartbeat period (1/2 sec).
 * If the system is Java-based, this call is meaningless and returns a value of 0.
 *
 * @param id             system id.
 * @param heartbeat      int pointer which gets filled with the ET system heartbeat.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_getheartbeat(et_sys_id id, int *heartbeat) {

    et_id *etid = (et_id *) id;

    if (heartbeat == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_getheartbeat(id, heartbeat);
    }

    *heartbeat = etid->sys->heartbeat;
    return ET_OK;
}


/**
 * This routine gets the unix process id of the ET system process.
 * If the ET system is Java-based, this call is meaningless and returns a value of -1.
 *
 * @param id       system id.
 * @param pid      pointer which gets filled with the unix process id of the ET system process.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_getpid(et_sys_id id, pid_t *pid) {

    et_id *etid = (et_id *) id;

    if (pid == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_getpid(id, pid);
    }

    *pid = etid->sys->mainpid;
    return ET_OK;
}


/**
 * This routine gets the number of local processes that have opened the ET system.
 * If the system is Java-based, this call is meaningless and returns a value of 0
 * since there is no shared memory.
 *
 * @param id          system id.
 * @param procs       int pointer which gets filled with the number of local processes that called et_open.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_getprocs(et_sys_id id, int *procs) {

    et_id *etid = (et_id *) id;

    if (procs == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_getprocs(id, procs);
    }

    *procs = etid->sys->nprocesses;
    return ET_OK;
}


/**
 * This routine gets the number of attachments that have been made to the ET system.
 *
 * @param id        system id.
 * @param atts      int pointer which gets filled with the number of attachments.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_getattachments(et_sys_id id, int *atts) {

    et_id *etid = (et_id *) id;

    if (atts == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_getattachments(id, atts);
    }

    *atts = etid->sys->nattachments;
    return ET_OK;
}


/**
 * This routine gets the number of stations in the ET system.
 *
 * @param id          system id.
 * @param stations    int pointer which gets filled with the number of stations.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_getstations(et_sys_id id, int *stations) {

    et_id *etid = (et_id *) id;

    if (stations == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_getstations(id, stations);
    }

    *stations = etid->sys->nstations;
    return ET_OK;
}


/**
 * This routine gets the number of temporary events currently in the ET system.
 * If the system is Java-based, this call is meaningless and returns a value of 0 since
 * there is no shared memory and no temp events.
 *
 * @param id       system id.
 * @param temps    int pointer which gets filled with the number of temporary events.
 *
 * @returns @ref ET_OK           if successful.
 * @returns @ref ET_ERROR        if either arg is NULL or id not initialized.
 * @returns @ref ET_ERROR_READ   if network communication reading problem.
 * @returns @ref ET_ERROR_WRITE  if network communication writing problem.
 */
int et_system_gettemps(et_sys_id id, int *temps) {

    et_id *etid = (et_id *) id;

    if (temps == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_system_gettemps(id, temps);
    }

    *temps = etid->sys->ntemps;
    return ET_OK;
}


/**
 * This routine gets the TCP listening port number of the ET system.
 *
 * @param sconfig   system configuration.
 * @param port      int pointer which gets filled with the system's TCP listening port.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_system_getserverport(et_sys_id id, int *port) {

    et_id *etid = (et_id *) id;

    if (port == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *port = etid->port;
    return ET_OK;
}


/**
 * This routine gets the host of the ET system.
 * To be safe the array should be at least {@ref ET_MAXHOSTNAMELEN} characters long.
 *
 * @param sconfig   system configuration.
 * @param host      pointer which gets filled with the system's host.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_system_gethost(et_sys_id id, char *host) {

    et_id *etid = (et_id *) id;

    if (host == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    strcpy(host, etid->ethost);
    return ET_OK;
}


/**
 * This routine gets the local IP address, if any, used to connect remotely to the ET system.
 * The array must be at least {@ref ET_IPADDRSTRLEN} characters long.
 *
 * @param sconfig   system configuration.
 * @param address   pointer which gets filled with the local IP address used to connect remotely to the ET system.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 */
int et_system_getlocaladdress(et_sys_id id, char *address) {

    et_id *etid = (et_id *) id;

    if (address == NULL || etid == NULL || etid->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    strcpy(address, etid->localAddr);
    return ET_OK;
}


/** @} */
/** @} */
