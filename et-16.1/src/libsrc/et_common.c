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
 *      Routines for initializations of processes,
 *	stations, events, eventlists, statistics & system ids. Also routines
 *	for opening ET systems and dealing with heartbeats.
 *
 *----------------------------------------------------------------------------*/
 

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "et_private.h"
#include "et_network.h"

/*****************************************************/
/*             ERROR REPORTING                       */
/*****************************************************/

#ifndef WITH_DALOGMSG
void et_logmsg (char *sev, char *fmt, ...)
{
  va_list ap;
  char temp[500];
  
  va_start(ap, fmt);
  vsprintf(temp, fmt, ap);

  printf("et %s: %s", sev, temp);
  va_end(ap);
}
#endif

/*****************************************************/
/*             ET STARTUP & SHUTDOWN                 */
/*****************************************************/

/**
 * This routine determines whether we are looking for the ET system locally,
 * locally on some non-pthread-mutex-sharing operating system, or remotely.
 *
 * @param filename    name of ET system file.
 * @param openconfig  ET system open configuration.
 *
 * @returns @ref ET_REMOTE         if looking for a remote ET system.
 * @returns @ref ET_LOCAL          if looking for a local ET system.
 * @returns @ref ET_LOCAL_NOSHARE  if looking for a local ET system without pthread mutex sharing.
 */
int et_findlocality(const char *filename, et_openconfig openconfig) {

    char ethost[ET_IPADDRSTRLEN];
    et_open_config *config = (et_open_config *) openconfig;

    /* if local client opens ET system as remote (thru server) ...
     * This option is for those applications (such as system
     * monitoring) that only want to communication through
     * an ET system's server and not map its shared mem.
     */
    if (config->mode == ET_HOST_AS_REMOTE) {
        return ET_REMOTE;
    }
    /* else if ET system host name is unknown and remote ... */
    else if (strcmp(config->host, ET_HOST_REMOTE) == 0) {
        return ET_REMOTE;
    }
    /* else if ET system is on local host */
    else if ((strcmp(config->host, ET_HOST_LOCAL) == 0) ||
             (strcmp(config->host, "localhost")   == 0))  {
        /* if local operating system can share pthread mutexes ... */
        if (et_sharedmutex() == ET_MUTEX_SHARE) {
            return ET_LOCAL;
        }
        else {
            return ET_LOCAL_NOSHARE;
        }
    }
    /* else if ET system host name is unknown and maybe anywhere ... */
    else if (strcmp(config->host, ET_HOST_ANYWHERE) == 0) {
        int err, port, isLocal;
        uint32_t inetaddr;
        struct timeval waittime;

        waittime.tv_sec  = 0;
        waittime.tv_usec = 10000; /* 0.1 sec */

        /* send only 1 broad/multicast with a 0.1 sec wait */
        err = et_findserver2(filename, ethost, &port, &inetaddr, NULL, config, 1, &waittime, ET_DEBUG_NONE);
        if ((err == ET_ERROR) || (err == ET_ERROR_TIMEOUT)) {
            et_logmsg("ERROR", "et_findlocality, cannot find ET system\n");
            return err;
        }
        else if (err == ET_ERROR_TOOMANY) {
            /* many systems responded */
            et_logmsg("ERROR", "et_findlocality, multiple ET systems responded\n");
            return err;
        }

        etNetNodeIsLocal(ethost, &isLocal);
        if (isLocal) {
            if (et_sharedmutex() == ET_MUTEX_SHARE) {
                return ET_LOCAL;
            }
            else {
                return ET_LOCAL_NOSHARE;
            }
        }

        return ET_REMOTE;
    }
    /* else ET system host name is given ... */
    else {
        int isLocal;
        etNetNodeIsLocal(config->host, &isLocal);
        if (isLocal) {
            if (et_sharedmutex() == ET_MUTEX_SHARE) {
                return ET_LOCAL;
            }
            else {
                return ET_LOCAL_NOSHARE;
            }
        }

        return ET_REMOTE;
    }

    return ET_REMOTE;
}


/**
 * @addtogroup general General
 *
 * These routines are called by a programmer wishing to open, close, kill, or interact
 * in a general way with an ET system.
 *
 * @{
 */


/**
 * This routine opens an ET system for client use.
 *
 * Given an ET system on the same host, this routine will map the system's shared memory into the
 * user's space. It also starts up a thread to produce a heartbeat and a second thread to monitor
 * the ET system's heartbeat. If the ET system is remote, a network connection is made to it.<p>
 *
 * The ET system is implemented as a single memory mapped file of the name, filename.
 * This routine should only be called once, before all other ET routines are used, or after a
 * system has been closed with a call to @ref et_close or @ref et_forcedclose. A successful return
 * from this routine assures connection to an ET system which is up and running.
 * <b><i>IT IS CRUCIAL THAT THE USER GET A RETURN VALUE OF "ET_OK" IF THE USER WANTS AN ET SYSTEM
 * GUARANTEED TO FUNCTION.</i></b><p>
 *
 * The user may open an ET system on a remote host. ET decides whether the user is on the same
 * as or a different machine than the system. If the determination is made that the user is on
 * another computer, then network connections are made to that system.<p>
 *
 * @param id          pointer to ET system id which gets filled in if ET system successfully opened.
 * @param filename    name of ET system file.
 * @param openconfig  ET system open configuration.
 *
 * @returns @ref ET_OK             if successful
 * @returns @ref ET_ERROR          if bad arg, ET name too long, cannot initialize id,
 *                                 creating/using shared memory, incompatible values for @ref ET_STATION_SELECT_INTS,
 *                                 or ET system is 32 bit and this program is 64 bit,
 * @returns @ref ET_ERROR_TIMEOUT if the ET system is still not active before the routine returns.
 * @returns @ref ET_ERROR_TOOMANY  if broad/multicasting and too many responses
 * @returns @ref ET_ERROR_REMOTE   if broad/multicasting and cannot find/connect to ET system or
 *                                 cannot allocate memory or ET system & user use different versions of ET, or
 *                                 the host has a strange byte order.
 * @returns @ref ET_ERROR_READ     if network read error.
 * @returns @ref ET_ERROR_WRITE    if network write error.
 */
int et_open(et_sys_id *id, const char *filename, et_openconfig openconfig) {

    int             status, auto_open=0, err, locality;
    et_open_config *config;
    et_openconfig   auto_config = NULL;
    int             def_debug;

    if (openconfig == NULL) {
        auto_open = 1;
        if (et_open_config_init(&auto_config) == ET_ERROR) {
            et_logmsg("ERROR", "et_open, null arg for openconfig, cannot use default\n");
            return ET_ERROR;
        }
        openconfig = auto_config;
    }

    config = (et_open_config *) openconfig;

    err = ET_OK;
    /* argument checking */
    if ((filename == NULL) || (config->init != ET_STRUCT_OK)) {
        et_logmsg("ERROR", "et_open, bad argument\n");
        err = ET_ERROR;
    }
    else if (strlen(filename) > ET_FILENAME_LENGTH - 1) {
        et_logmsg("ERROR", "et_open, ET name too long\n");
        err = ET_ERROR;
    }

    if (err != ET_OK) {
        if (auto_open == 1) {
            et_open_config_destroy(auto_config);
        }
        return err;
    }

    /* initialize id */
    if (et_id_init(id) != ET_OK) {
        et_logmsg("ERROR", "et_open, cannot initialize id\n");
        return ET_ERROR;
    }

    if (et_open_config_getdebugdefault(openconfig, &def_debug) != ET_OK) {
        def_debug = ET_DEBUG_ERROR;
    }
    et_system_setdebug(*id, def_debug);


    /* Decide whether we are looking for the ET system locally,
     * locally on some non-mutex-sharing operating
     * system, remotely, or anywhere.
     */
    locality = et_findlocality(filename, openconfig);
    /* if host is local ... */
    if (locality == ET_LOCAL) {
        status = etl_open(id, filename, openconfig);
        /* If this is a Java-based ET sys, try opening it as remote client. */
        if (status == ET_ERROR_JAVASYS) {
            et_logmsg("ERROR", "et_open: cannot open Java ET file, try as remote client\n");
            status = etr_open(id, filename, openconfig);
        }
    }
        /* else if host is remote ... */
    else if (locality == ET_REMOTE) {
        status = etr_open(id, filename, openconfig);
    }
        /* else if host is local on Linux ... */
    else if (locality == ET_LOCAL_NOSHARE) {
        status = etn_open(id, filename, openconfig);
    }
        /* else if too many systems responded and we have return error policy ... */
    else if ((locality == ET_ERROR_TOOMANY) && (config->policy == ET_POLICY_ERROR)) {
        if (auto_open == 1) {
            et_open_config_destroy(auto_config);
        }
        et_logmsg("ERROR", "et_open: too many ET systems of that name responded\n");
        return ET_ERROR;
    }
        /* else did not find ET system by broad/multicasting. In
         * this case, try to open a local system first then remote.
         */
    else {
        int shared = et_sharedmutex();
        /* if local operating system can share pthread mutexes ... */
        if (shared == ET_MUTEX_SHARE) {
            status = etl_open(id, filename, openconfig);
        }
        else {
            status = etn_open(id, filename, openconfig);
        }

        if (status != ET_OK) {
            status = etr_open(id, filename, openconfig);
        }
    }

    if (status != ET_OK) {
        et_id_destroy(*id);
    }

    if (auto_open == 1) {
        et_open_config_destroy(auto_config);
    }

    return status;
}


/**
 * This routine closes an open ET system.
 *
 * Given a local ET system that has been opened with a call to @ref et_open, this routine will stop
 * all ET-related threads and unmap the system's memory from the user's space making it inaccessible.
 * It also frees memory allocated in et_open to create the system's id. For a remote user, all this
 * routine does is close the connection between the user and ET system as well as free the memory
 * allocated in creating the system's id.
 *
 * This routine should only be called once for a particular ET system after the associated call
 * to et_open. In addition, all attachments of the process calling this routine must be detached
 * first or an error will be returned.
 *
 * @param id          ET system id.
 *
 * @returns @ref ET_OK             if successful
 * @returns @ref ET_ERROR          if bad arg, not detached from all stations.
 * @returns @ref ET_ERROR_REMOTE   for a local user on a non-mutex-sharing system (MacOS),
 *                                 if cannot unmap shared memory
 */
int et_close(et_sys_id id) {

    et_id *etid = (et_id *) id;

    if (id == NULL) return ET_ERROR;

    if (etid->locality == ET_REMOTE) {
        return etr_close(id);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_close(id);
    }
    return etl_close(id);
}


/**
 * This routine closes an open ET system but differs from @ref et_close in that it
 * automatically closes all attachments.
 *
 * Given a local ET system that has been opened with a call to @ref et_open, this routine will stop
 * all ET-related threads and unmap the system's memory from the user's space making it inaccessible.
 * For a remote user, this routine closes the connection between the user and ET system. But before
 * it does any of this, it detaches all attachments belonging to the process calling it. It also
 * frees memory allocated in et_open to create the system's id.
 *
 * @param id          ET system id.
 *
 * @returns @ref ET_OK             if successful
 * @returns @ref ET_ERROR          if bad arg.
 * @returns @ref ET_ERROR_REMOTE   for a local user on a non-mutex-sharing system (MacOS),
 *                                 if cannot unmap shared memory
 */
int et_forcedclose(et_sys_id id) {

    et_id *etid = (et_id *) id;

    if (id == NULL) return ET_ERROR;

    if (etid->locality == ET_REMOTE) {
        return etr_forcedclose(id);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_forcedclose(id);
    }
    return etl_forcedclose(id);
}


/**
 * This routine kills an open ET system.
 *
 * Given a local ET system that has been opened with @ref et_open, this routine will do the same as
 * @ref et_forcedclose for the caller and will cause the ET system to delete its file and kill
 * itself. For a remote user it does the equivalent of an @ref et_close for the caller while telling
 * the ET system to delete its file and kill itself.
 *
 * @param id          ET system id.
 *
 * @returns @ref ET_OK             if successful, or if local client
 * @returns @ref ET_ERROR          if bad arg.
 * @returns @ref ET_ERROR_WRITE    if remote client and network writing error
 * @returns @ref ET_ERROR_REMOTE   for a local user on a non-mutex-sharing system (MacOS),
 *                                 if cannot unmap shared memory
 */
int et_kill(et_sys_id id)
{
    et_id *etid = (et_id *) id;

    if (id == NULL) return ET_ERROR;

    if (etid->locality == ET_REMOTE) {
        return etr_kill(id);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_kill(id);
    }
    return etl_kill(id);
}

/*****************************************************/
/*                  HEARTBEAT STUFF                  */
/*****************************************************/

/**
 * This routine tells if an open ET system is alive (heartbeat is going).
 *
 * This routine behaves differently depending on whether it is run locally or remotely.
 * If the user is running it locally, a thread of the user's process is constantly checking
 * to see if the ET system is alive and provides a valid return value to et_alive when last
 * it was monitored (up to three heartbeats ago). If the user is on a remote node, the ET
 * system's server thread is contacted. If that communication succeeds, then the ET system
 * is alive by definition, otherwise it is dead.
 *
 * @param id     ET system id.
 *
 * @returns 1    if ET system is alive.
 * @returns 0    if ET system is dead or bad arg.
 */
int et_alive(et_sys_id id) {

    et_id *etid = (et_id *) id;

    if (id == NULL) return 0;

    if (etid->locality == ET_REMOTE) {
        return etr_alive(id);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_alive(id);
    }
    return etl_alive(id);
}


/**
 * This routine waits until given ET system is alive.
 *
 * @param id     ET system id.
 *
 * @returns @ref ET_OK             if successful and ET is alive.
 * @returns @ref ET_ERROR_WRITE    if remote client and network writing error.
 * @returns @ref ET_ERROR_READ     if remote client and network reading error.
 */
int et_wait_for_alive(et_sys_id id) {

    et_id *etid = (et_id *) id;

    if (etid->locality == ET_REMOTE) {
        return etr_wait_for_alive(id);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_wait_for_alive(id);
    }
    return etl_wait_for_alive(id);
}

/** @} */

/**
 * @addtogroup errors Errors
 *
 * @{
 */

/**
 * This routine returns a string describing the given error condition.
 * The returned string is a static char array. This means it is not
 * thread-safe and will be overwritten on subsequent calls.
 *
 * @param error error condition
 *
 * @returns error string
 */
char *et_perror(int error) {

    static char temp[256];

    switch(error) {

        case ET_OK:
            sprintf(temp, "ET_OK:  action completed successfully\n");
            break;

        case ET_ERROR:
            sprintf(temp, "ET_ERROR:  generic error\n");
            break;

        case ET_ERROR_TOOMANY:
            sprintf(temp, "ET_ERROR_TOOMANY:  too many items already exist\n");
            break;

        case ET_ERROR_EXISTS:
            sprintf(temp, "ET_ERROR_EXISTS:  already exists\n");
            break;

        case ET_ERROR_WAKEUP:
            sprintf(temp, "ET_ERROR_WAKEUP:  sleeping routine woken up by command\n");
            break;

        case ET_ERROR_TIMEOUT:
            sprintf(temp, "ET_ERROR_TIMEOUT:  timed out\n");
            break;

        case ET_ERROR_EMPTY:
            sprintf(temp, "ET_ERROR_EMPTY:  no events available in async mode\n");
            break;

        case ET_ERROR_BUSY:
            sprintf(temp, "ET_ERROR_BUSY:  ET system is busy in async mode\n");
            break;

        case ET_ERROR_DEAD:
            sprintf(temp, "ET_ERROR_DEAD:  ET system is dead\n");
            break;

        case ET_ERROR_READ:
            sprintf(temp, "ET_ERROR_READ:  read error\n");
            break;

        case ET_ERROR_WRITE:
            sprintf(temp, "ET_ERROR_WRITE:  write error\n");
            break;

        case ET_ERROR_REMOTE:
            sprintf(temp, "ET_ERROR_REMOTE:  cannot allocate memory in remote application (not server)\n");
            break;

        case ET_ERROR_NOREMOTE:
            sprintf(temp, "ET_ERROR_NOREMOTE:  (currently not used)\n");
            break;

        case ET_ERROR_TOOBIG:
            sprintf(temp, "ET_ERROR_TOOBIG:  client is 32 bits & server is 64 (or vice versa) and event is too big for one\n");
            break;

        case ET_ERROR_NOMEM:
            sprintf(temp, "ET_ERROR_NOMEM:  cannot allocate memory\n");
            break;

        case ET_ERROR_BADARG:
            sprintf(temp, "ET_ERROR_BADARG:  bad arg given to function\n");
            break;

        case ET_ERROR_SOCKET:
            sprintf(temp, "ET_ERROR_SOCKET:  socket option could not be set\n");
            break;

        case ET_ERROR_NETWORK:
            sprintf(temp, "ET_ERROR_NETWORK:  host name or address could not be resolved, or cannot connect\n");
            break;

        case ET_ERROR_CLOSED:
            sprintf(temp, "ET_ERROR_CLOSED:  et_close() as been called, id is now invalid\n");
            break;

        case ET_ERROR_JAVASYS:
            sprintf(temp, "ET_ERROR_CLOSED:  C code trying to open Java-based ET system file locally\n");
            break;

        default:
            sprintf(temp, "?et_perror...no such error: %d\n",error);
            break;
    }

    return(temp);
}

/** @} */
