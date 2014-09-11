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
 *      Routines for user to get, put, make, dump, & manipulate events
 *      in the ET system.
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>

#include "et_private.h"
#include "et_network.h"

/******************************************************/
/* Make a temporary buffer for the extra large event
 * size = bytes needed for data only
 */
static int et_event_make(et_id *id, et_event *pe, size_t size)
{
    et_system *sys = id->sys;
    void *pdata;
    int   ntemps;

    /* grab system mutex */
    et_system_lock(id->sys);

    /* too many temp events already exist. */
    if (sys->ntemps >= sys->config.ntemps) {
        ntemps = sys->ntemps;
        et_system_unlock(id->sys);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_make, too many (%d) temp events\n",
                      ntemps);
        }
        return ET_ERROR_TOOMANY;
    }
    sys->ntemps++;
  
    /* release system mutex */
    et_system_unlock(id->sys);

    /* Get mem "on the fly" */
    if ((pdata = et_temp_create(pe->filename, size)) == NULL) {
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_make, cannot allocate temp event mem\n");
        }
        return ET_ERROR;
    }
  
    /* put new buffer's location/size in event struct */
    et_init_event_(pe);
    pe->temp    = ET_EVENT_TEMP;
    pe->pdata   = pdata;
    pe->length  = size;
    pe->memsize = size;

    return ET_OK;
}


/******************************************************/
/*    attachment requesting event of certain size     */
int et_event_new(et_sys_id id, et_att_id att, et_event **pe,
                 int mode, struct timespec *deltatime, size_t size)
{
    int num_try=0, try_max, status, wait;
    struct timespec waitforme, abs_time;
    et_id     *etid = (et_id *) id;
    et_system *sys = etid->sys;
  
    /* check for default value of group != 0 */
    if (etid->group) {
        return et_event_new_group(id, att, pe, mode, deltatime,
                                  size, etid->group);
    }

    if ((att < 0) || (size < 0) || (pe == NULL)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_new, bad argument(s)\n");
        }
        return ET_ERROR;
    }
  
    /* make sure mode is wait value of ET_SLEEP, ET_TIMED, or ET_ASYNC */
    wait = mode & ET_WAIT_MASK;
    if ((wait != ET_SLEEP) && (wait != ET_TIMED) && (wait != ET_ASYNC)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_new, improper value for mode\n");
        }
        return ET_ERROR;
    }

    if (wait == ET_TIMED && deltatime == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_new, specify a time for ET_TIMED mode\n");
        }
        return ET_ERROR;
    }
  
    if (etid->locality == ET_REMOTE) {
        return etr_event_new(id, att, pe, mode, deltatime, size);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_event_new(id, att, pe, wait, deltatime, size);
    }
  
    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }

    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_new, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }
  
    if (sys->attach[att].status != ET_ATT_ACTIVE) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_new, attachment #%d is not active\n", att);
        }
        return ET_ERROR;
    }
  
    if (wait != ET_TIMED) {
        /* gets next Grand central input list event */
        if ((status = et_station_read(etid, ET_GRANDCENTRAL, pe, wait, att, NULL)) != ET_OK) {
            et_mem_unlock(etid);
            if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_event_new, cannot read event\n");
            }
            /* If status == ET_ERROR_WAKEUP, it may have been woken up by the
             * thread monitoring the ET system heartbeat discovering that the
             * ET system was dead. In that case return ET_ERROR_DEAD.
             */
            if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                return ET_ERROR_DEAD;
            }
            return status;
        }
    }
    else {
        /* translate a delta time into an absolute time */
        int nsec_total;
    
#if defined linux || defined __APPLE__
        struct timeval now;
        gettimeofday(&now, NULL);
        nsec_total = deltatime->tv_nsec + 1000*now.tv_usec;
#else
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        nsec_total = deltatime->tv_nsec + now.tv_nsec;
#endif
    
        if (nsec_total >= 1000000000) {
            abs_time.tv_nsec = nsec_total - 1000000000;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec + 1;
        }
        else {
            abs_time.tv_nsec = nsec_total;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec;
        }
    
        /* gets next Grand central input list event */
        if ((status = et_station_read(etid, ET_GRANDCENTRAL, pe, wait, att, &abs_time)) != ET_OK) {
            et_mem_unlock(etid);
            if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_event_new, cannot read event\n");
            }
            if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                return ET_ERROR_DEAD;
            }
            return status;
        }
    }
    
    /* make a temporary buffer for extra large event */
    if (size > sys->config.event_size) {
        /* try for 30 seconds, if necessary, to get a temp event */
        waitforme.tv_sec  = 0;
        if (sys->hz < 2) {
            waitforme.tv_nsec = 10000000; /* 10 millisec */
        }
        else {
            waitforme.tv_nsec = 1000000000/sys->hz;
        }
        try_max = (sys->hz)*30;

        try_again:
        if ((status = et_event_make(etid, *pe, size)) != ET_OK) {
            if (status == ET_ERROR_TOOMANY) {
                if (etid->debug >= ET_DEBUG_WARN) {
                    et_logmsg("WARN", "et_event_new, too many temp events\n");
                }

                /* wait until more temp events available, then try again */
                while (sys->ntemps >= sys->config.ntemps && num_try < try_max) {
                    num_try++;
                    nanosleep(&waitforme, NULL);
                }
        
                if (etid->debug >= ET_DEBUG_WARN) {
                    et_logmsg("WARN", "et_event_new, num_try = %d\n", num_try);
                }
                
                if (num_try >= try_max) {
                    et_mem_unlock(etid);
                    if (etid->debug >= ET_DEBUG_ERROR) {
                        et_logmsg("ERROR", "et_event_new, too many trys to get temp event, status = %d\n", status);
                    }
                    return status;
                }
                num_try=0;
                goto try_again;
            }
            else {
                et_mem_unlock(etid);
                return status;
            }
        }
    }
    else {
        /* init & set data pointer to own data space */
        et_init_event_(*pe);
        (*pe)->pdata   = ET_PDATA2USR((*pe)->data, etid->offset);
        (*pe)->length  = size;
        (*pe)->memsize = sys->config.event_size;
    }

    /* keep track of # of new events made by this attachment */
    sys->attach[att].events_make++;

    /* release mutex */
    et_mem_unlock(etid);

    return ET_OK;
}


/******************************************************/
/*    attachment requesting events of certain size    */
int et_events_new(et_sys_id id, et_att_id att, et_event *pe[],
                  int mode, struct timespec *deltatime,
                  size_t size, int num, int *nread)
{
    int i, num_try=0, try_max, status, numread, wait;
    struct timespec waitforme, abs_time;
    et_id     *etid = (et_id *) id;
    et_system *sys = etid->sys;
  
    /* check for default value of group != 0 */
    if (etid->group) {
        return et_events_new_group(id, att, pe, mode, deltatime,
                                   size, num, etid->group, nread);
    }

    /* set this to 0 in case we return an error */
    if (nread != NULL) {
        *nread = 0;
    }
  
    if (num == 0) {
        return ET_OK;
    }
  
    if ((att < 0) || (size < 0) || (pe == NULL) || (num < 0)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new, bad argument(s)\n");
        }
        return ET_ERROR;
    }
  
    /* make sure mode is wait value of ET_SLEEP, ET_TIMED, or ET_ASYNC */
    wait = mode & ET_WAIT_MASK;
    if ((wait != ET_SLEEP) && (wait != ET_TIMED) && (wait != ET_ASYNC)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new, improper value for mode\n");
        }
        return ET_ERROR;
    }
    
    if (wait == ET_TIMED && deltatime == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new, specify a time for ET_TIMED mode\n");
        }
        return ET_ERROR;
    }
  
    if (etid->locality == ET_REMOTE) {
        return etr_events_new(id, att, pe, mode, deltatime, size, num, nread);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_events_new(id, att, pe, wait, deltatime, size, num, nread);
    }
  
    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }
  
    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }
  
    /* if asking for temp events, don't ask for more than available */
    if (sys->config.event_size < size) {
        if (sys->config.ntemps < num) {
            num = sys->config.ntemps;
        }
    }
  
    if (sys->attach[att].status != ET_ATT_ACTIVE) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new, attachment #%d is not active\n", att);
        }
        return ET_ERROR;
    }
  
    if (wait != ET_TIMED) {
        /* gets next Grand central input list event */
        if ((status = et_station_nread(etid, ET_GRANDCENTRAL, pe, wait, att, NULL, num, &numread)) != ET_OK) {
            et_mem_unlock(etid);
            if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_events_new, cannot read event\n");
            }
            /* If status == ET_ERROR_WAKEUP, it may have been woken up by the
             * thread monitoring the ET system heartbeat discovering that the
             * ET system was dead. In that case return ET_ERROR_DEAD.
             */
            if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                return ET_ERROR_DEAD;
            }
            return status;
        }
    }
    else {
        /* translate a delta time into an absolute time */
        int nsec_total;
    
#if defined linux || defined __APPLE__
        struct timeval now;
        gettimeofday(&now, NULL);
        nsec_total = deltatime->tv_nsec + 1000*now.tv_usec;
#else
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        nsec_total = deltatime->tv_nsec + now.tv_nsec;
#endif
    
        if (nsec_total >= 1000000000) {
            abs_time.tv_nsec = nsec_total - 1000000000;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec + 1;
        }
        else {
            abs_time.tv_nsec = nsec_total;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec;
        }
    
        /* gets next Grand central input list event */
        if ((status = et_station_nread(etid, ET_GRANDCENTRAL, pe, wait, att, &abs_time, num, &numread)) != ET_OK) {
            et_mem_unlock(etid);
            if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_events_new, cannot read event\n");
            }
            if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                return ET_ERROR_DEAD;
            }
            return status;
        }
    }
  
    /* make a temporary buffer for extra large event */
    if (size > sys->config.event_size) {
        /* try for 30 seconds, if necessary, to get a temp event */
        waitforme.tv_sec  = 0;
        if (sys->hz < 2) {
            waitforme.tv_nsec = 10000000; /* 10 millisec */
        }
        else {
            waitforme.tv_nsec = 1000000000/sys->hz;
        }
        try_max = (sys->hz)*30;

        for (i=0; i < numread ; i++) {
        try_again:
            if ((status = et_event_make(etid, pe[i], size)) != ET_OK) {
                if (status == ET_ERROR_TOOMANY) {
                    if (etid->debug >= ET_DEBUG_WARN) {
                        et_logmsg("WARN", "et_events_new, too many temp events\n");
                    }
        
                    /* wait until more temp events available, then try again */
                    while (sys->ntemps >= sys->config.ntemps && num_try < try_max) {
                        num_try++;
                        nanosleep(&waitforme, NULL);
                    }
        
                    if (etid->debug >= ET_DEBUG_WARN) {
                        et_logmsg("WARN", "et_events_new, num_try = %d\n", num_try);
                    }
        
                    if (num_try >= try_max) {
                        et_mem_unlock(etid);
                        if (etid->debug >= ET_DEBUG_ERROR) {
                            et_logmsg("ERROR", "et_events_new, too many trys to get temp event, status = %d\n", status);
                        }
                        return status;
                    }
                    num_try=0;
                    goto try_again;
                }
                else {
                    et_mem_unlock(etid);
                    return status;
                }
            }
        } /* for each event */
    }
    else {
        for (i=0; i < numread ; i++) {
            /* init & set data pointer to own data space */
            et_init_event_(pe[i]);
            pe[i]->pdata   = ET_PDATA2USR(pe[i]->data, etid->offset);
            pe[i]->length  = size;
            pe[i]->memsize = sys->config.event_size;
        }
    }
  
    /* keep track of # of new events made by this attachment */
    sys->attach[att].events_make += numread;

    /* release mutex */
    et_mem_unlock(etid);

    if (nread != NULL) {
        *nread = numread;
    }
    return ET_OK;
}

/******************************************************/
/*    attachment requesting event of certain size     */
int et_event_new_group(et_sys_id id, et_att_id att, et_event **pe,
                       int mode, struct timespec *deltatime, size_t size, int group)
{
    int nread;
    return et_events_new_group(id, att, pe, mode, deltatime, size, 1, group, &nread);
}

/******************************************************/
/*    attachment requesting events of certain size    */
int et_events_new_group(et_sys_id id, et_att_id att, et_event *pe[],
                        int mode, struct timespec *deltatime,
                        size_t size, int num, int group, int *nread)
{
    int i, num_try=0, try_max, status, numread, wait;
    struct timespec waitforme, abs_time;
    et_id     *etid = (et_id *) id;
    et_system *sys = etid->sys;

    /* set this to 0 in case we return an error */
    if (nread != NULL) {
        *nread = 0;
    }

    if (num == 0) {
        return ET_OK;
    }

    if ((att < 0) || (size < 0) || (pe == NULL) || (num < 0) || (group < 1)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new_group, bad argument(s)\n");
        }
        return ET_ERROR;
    }

    /* make sure mode is wait value of ET_SLEEP, ET_TIMED, or ET_ASYNC */
    wait = mode & ET_WAIT_MASK;
    if ((wait != ET_SLEEP) && (wait != ET_TIMED) && (wait != ET_ASYNC)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new_group, improper value for mode\n");
        }
        return ET_ERROR;
    }

    if (wait == ET_TIMED && deltatime == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new_group, specify a time for ET_TIMED mode\n");
        }
        return ET_ERROR;
    }
  
    if (etid->locality == ET_REMOTE) {
        return etr_events_new_group(id, att, pe, mode, deltatime, size, num, group, nread);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_events_new_group(id, att, pe, wait, deltatime, size, num, group, nread);
    }
    
    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }

    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new_group, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }
  
    /* check to see if value of group is meaningful (since accessing shared mem, place here) */
    if (group > etid->sys->config.groupCount) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new_group, group arg too large\n");
        }
        return ET_ERROR;
    }
    
    /* if asking for temp events, don't ask for more than available */
    if (sys->config.event_size < size) {
        if (sys->config.ntemps < num) {
            num = sys->config.ntemps;
        }
    }

    if (sys->attach[att].status != ET_ATT_ACTIVE) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_new_group, attachment #%d is not active\n", att);
        }
        return ET_ERROR;
    }

    if (wait != ET_TIMED) {
        /* gets next Grand central input list event */
        if ((status = et_station_nread_group(etid, ET_GRANDCENTRAL, pe, wait, att,
             NULL, num, group, &numread)) != ET_OK) {
                 et_mem_unlock(etid);
                 if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                     et_logmsg("ERROR", "et_events_new_group, cannot read event\n");
                 }
                 /* If status == ET_ERROR_WAKEUP, it may have been woken up by the
                  * thread monitoring the ET system heartbeat discovering that the
                  * ET system was dead. In that case return ET_ERROR_DEAD.
                  */
                 if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                     return ET_ERROR_DEAD;
                 }
                 return status;
             }
    }
    else {
        /* translate a delta time into an absolute time */
        int nsec_total;

#if defined linux || defined __APPLE__
        struct timeval now;
        gettimeofday(&now, NULL);
        nsec_total = deltatime->tv_nsec + 1000*now.tv_usec;
#else
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        nsec_total = deltatime->tv_nsec + now.tv_nsec;
#endif

        if (nsec_total >= 1000000000) {
            abs_time.tv_nsec = nsec_total - 1000000000;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec + 1;
        }
        else {
            abs_time.tv_nsec = nsec_total;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec;
        }

        /* gets next Grand central input list event */
        if ((status = et_station_nread_group(etid, ET_GRANDCENTRAL, pe, wait, att,
             &abs_time, num, group, &numread)) != ET_OK) {
                 et_mem_unlock(etid);
                 if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                     et_logmsg("ERROR", "et_events_new_group, cannot read event\n");
                 }
                 if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                     return ET_ERROR_DEAD;
                 }
                 return status;
             }
    }

    /* make a temporary buffer for extra large event */
    if (size > sys->config.event_size) {
        /* try for 30 seconds, if necessary, to get a temp event */
        waitforme.tv_sec  = 0;
        if (sys->hz < 2) {
            waitforme.tv_nsec = 10000000; /* 10 millisec */
        }
        else {
            waitforme.tv_nsec = 1000000000/sys->hz;
        }
        try_max = (sys->hz)*30;

        for (i=0; i < numread ; i++) {
        try_again:
            if ((status = et_event_make(etid, pe[i], size)) != ET_OK) {
                if (status == ET_ERROR_TOOMANY) {
                    if (etid->debug >= ET_DEBUG_WARN) {
                        et_logmsg("WARN", "et_events_new_group, too many temp events\n");
                    }

                    /* wait until more temp events available, then try again */
                    while (sys->ntemps >= sys->config.ntemps && num_try < try_max) {
                        num_try++;
                        nanosleep(&waitforme, NULL);
                    }

                    if (etid->debug >= ET_DEBUG_WARN) {
                        et_logmsg("WARN", "et_events_new_group, num_try = %d\n", num_try);
                    }

                    if (num_try >= try_max) {
                        et_mem_unlock(etid);
                        if (etid->debug >= ET_DEBUG_ERROR) {
                            et_logmsg("ERROR", "et_events_new_group, too many trys to get temp event, status = %d\n", status);
                        }
                        return status;
                    }
                    num_try=0;
                    goto try_again;
                }
                else {
                    et_mem_unlock(etid);
                    return status;
                }
            }
        } /* for each event */
    }
    else {
        for (i=0; i < numread ; i++) {
            /* init & set data pointer to own data space */
            et_init_event_(pe[i]);
            pe[i]->pdata   = ET_PDATA2USR(pe[i]->data, etid->offset);
            pe[i]->length  = size;
            pe[i]->memsize = sys->config.event_size;
        }
    }

    /* keep track of # of new events made by this attachment */
    sys->attach[att].events_make += numread;

    /* release mutex */
    et_mem_unlock(etid);

    if (nread != NULL) {
        *nread = numread;
    }
    return ET_OK;
}


/******************************************************/
/*          get event from station input list         */

int et_event_get(et_sys_id id, et_att_id att, et_event **pe,
                 int mode, struct timespec *deltatime)
{
    int        status, wait;
    void       *pdata;
    struct timespec abs_time;
    et_id      *etid = (et_id *) id;
    et_system  *sys  = etid->sys;
    et_stat_id  stat_id;

    if ((att < 0) || (pe == NULL)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_get, bad argument(s)\n");
        }
        return ET_ERROR;
    }
   
    /* make sure mode is wait value of ET_SLEEP, ET_TIMED, or ET_ASYNC */
    wait = mode & ET_WAIT_MASK;
    if ((wait != ET_SLEEP) && (wait != ET_TIMED) && (wait != ET_ASYNC)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_get, improper value for mode\n");
        }
        return ET_ERROR;
    }
  
    if (wait == ET_TIMED && deltatime == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_get, specify a time for ET_TIMED mode\n");
        }
        return ET_ERROR;
    }
  
    if (etid->locality == ET_REMOTE) {
        return etr_event_get(id, att, pe, mode, deltatime);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_event_get(id, att, pe, wait, deltatime);
    }
  
    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }
  
    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_get, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }
  
    if (sys->attach[att].status != ET_ATT_ACTIVE) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_get, attachment #%d is not active\n", att);
        }
        return ET_ERROR;
    }
  
    stat_id = sys->attach[att].stat;
  
    if (stat_id == (et_stat_id) ET_GRANDCENTRAL) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_get, may not get event from grandcentral station\n");
        }
        return ET_ERROR;
    }
  
    if (wait != ET_TIMED) {
        /* gets next input list event */
        if ((status = et_station_read(etid, stat_id, pe, wait, att, NULL)) != ET_OK) {
            et_mem_unlock(etid);
            if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_event_get, cannot read event\n");
            }
            /* If status == ET_ERROR_WAKEUP, it may have been woken up by the
             * thread monitoring the ET system heartbeat discovering that the
             * ET system was dead. In that case return ET_ERROR_DEAD.
             */
            if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                return ET_ERROR_DEAD;
            }
            return status;
        }
    }
    else {
        /* translate a delta time into an absolute time */
        int nsec_total;
    
#if defined linux || defined __APPLE__
        struct timeval now;
        gettimeofday(&now, NULL);
        nsec_total = deltatime->tv_nsec + 1000*now.tv_usec;
#else
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        nsec_total = deltatime->tv_nsec + now.tv_nsec;
#endif

        if (nsec_total >= 1000000000) {
            abs_time.tv_nsec = nsec_total - 1000000000;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec + 1;
        }
        else {
            abs_time.tv_nsec = nsec_total;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec;
        }
       
        /* gets next input list event */
        if ((status = et_station_read(etid, stat_id, pe, wait, att, &abs_time)) != ET_OK) {
            et_mem_unlock(etid);
            if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_event_get, cannot read event\n");
            }
            if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                return ET_ERROR_DEAD;
            }
            return status;
        }
    }
  
    /* if NOT temp buffer, translate data pointer */
    if ((*pe)->temp != ET_EVENT_TEMP) {
        (*pe)->pdata = ET_PDATA2USR((*pe)->data, etid->offset);
    }
    /* if temp buffer, map it into this process' mem */
    else {
        /* attach to temp event mem */
        if ((pdata = et_temp_attach((*pe)->filename, (*pe)->memsize)) == NULL) {
            et_mem_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_event_get, cannot attach to temp event\n");
            }
            return ET_ERROR;
        }
        (*pe)->pdata = pdata;
    }

    /* keep track of # of events gotten by this attachment */
    sys->attach[att].events_get++;

    /* release mutex */
    et_mem_unlock(etid);

    return ET_OK;
}


/******************************************************/
/*          get event from station input list         */

int et_events_get(et_sys_id id, et_att_id att, et_event *pe[],
                  int mode, struct timespec *deltatime, int num, int *nread)
{
    int         i, status, numread, wait;
    void       *pdata;
    struct timespec abs_time;
    et_id      *etid = (et_id *) id;
    et_system  *sys  = etid->sys;
    et_stat_id  stat_id;

    if (nread != NULL) {
        *nread = 0;
    }
  
    if (num == 0) {
        return ET_OK;
    }
  
    if ((att < 0) || (pe == NULL) || (num < 0)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_get, bad argument(s)\n");
        }
        return ET_ERROR;
    }
   
    /* make sure mode is wait value of ET_SLEEP, ET_TIMED, or ET_ASYNC */
    wait = mode & ET_WAIT_MASK;
    if ((wait != ET_SLEEP) && (wait != ET_TIMED) && (wait != ET_ASYNC)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_get, improper value for mode\n");
        }
        return ET_ERROR;
    }
  
    if (wait == ET_TIMED && deltatime == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_get, specify a time for ET_TIMED mode\n");
        }
        return ET_ERROR;
    }
  
    if (etid->locality == ET_REMOTE) {
        return etr_events_get(id, att, pe, mode, deltatime, num, nread);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_events_get(id, att, pe, wait, deltatime, num, nread);
    }
  
    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }
  
    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_get, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }
  
    if (sys->attach[att].status != ET_ATT_ACTIVE) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_get, attachment #%d is not active\n", att);
        }
        return ET_ERROR;
    }

    stat_id = sys->attach[att].stat;
  
    if (stat_id == (et_stat_id) ET_GRANDCENTRAL) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_get, may not get events from grandcentral station\n");
        }
        return ET_ERROR;
    }
  
    if (wait != ET_TIMED) {
        /* get input list events */
        if ((status = et_station_nread(etid, stat_id, pe, wait, att, NULL, num, &numread)) != ET_OK) {
            et_mem_unlock(etid);
            if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_events_get, cannot read events\n");
            }
            if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                return ET_ERROR_DEAD;
            }
            return status;
        }
    }
    else {
        /* translate a delta time into an absolute time */
        int nsec_total;
    
#if defined linux || defined __APPLE__
        struct timeval now;
        gettimeofday(&now, NULL);
        nsec_total = deltatime->tv_nsec + 1000*now.tv_usec;
#else
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        nsec_total = deltatime->tv_nsec + now.tv_nsec;
#endif

        if (nsec_total >= 1000000000) {
            abs_time.tv_nsec = nsec_total - 1000000000;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec + 1;
        }
        else {
            abs_time.tv_nsec = nsec_total;
            abs_time.tv_sec  = deltatime->tv_sec + now.tv_sec;
        }
    
        /* get input list events */
        if ((status = et_station_nread(etid, stat_id, pe, wait, att, &abs_time, num, &numread)) != ET_OK) {
            et_mem_unlock(etid);
            if ((status == ET_ERROR) && (etid->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_events_get, cannot read events\n");
            }
            if ((status == ET_ERROR_WAKEUP) && (!et_alive(id))) {
                return ET_ERROR_DEAD;
            }
            return status;
        }
    }
  
    for (i=0; i < numread ; i++) {
        /* if NOT temp buffer, translate data pointer */
        if (pe[i]->temp != ET_EVENT_TEMP) {
            pe[i]->pdata = ET_PDATA2USR(pe[i]->data, etid->offset);
        }
        /* if temp buffer, map it into this process' mem */
        else {
            /* attach to temp event mem */
            if ((pdata = et_temp_attach(pe[i]->filename, pe[i]->memsize)) == NULL) {
                et_mem_unlock(etid);
                if (etid->debug >= ET_DEBUG_ERROR) {
                    et_logmsg("ERROR", "et_events_get, cannot attach to temp event\n");
                }
                return ET_ERROR;
            }
            pe[i]->pdata = pdata;
        }
    }
  
    /* keep track of # of events gotten by this attachment */
    sys->attach[att].events_get += numread;

    /* release mutex */
    et_mem_unlock(etid);

    if (nread != NULL) {
        *nread = numread;
    }
    return ET_OK;
}


/******************************************************/
/*
 * Called when an attachment got an event from its inlist
 * and wants to put it back into the et system.
 * Notice, we don't check to see if the attachment
 * is idle. That's because it can only become idle
 * by detaching from the station and this automatically
 * takes all events owned by the attachment and puts them
 * back into the system. Putting them back changes the
 * owner to "-1" so any attempt by the idle attachment to
 * put these events back will fail.
 */
int et_event_put(et_sys_id id, et_att_id att, et_event *pe)
{
    int status, ageOrig;
    et_stat_id stat_id;
    et_id      *etid = (et_id *) id;
    et_system  *sys  = etid->sys;

    if ((att < 0) || (pe == NULL)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_put, bad argument(s)\n");
        }
        return ET_ERROR;
    }
  
    if (etid->locality == ET_REMOTE) {
        return etr_event_put(id, att, pe);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_event_put(id, att, pe);
    }
  
    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }

    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_put, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    /* if length bigger than memory size, we got problems (pe accesses shared mem) */
    if (pe->length > pe->memsize) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_put, data length is too large!\n");
        }
        return ET_ERROR;
    }

    if (pe->owner != att) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_put, not event owner so can't put back\n");
        }
        return ET_ERROR;
    }
    
    /* if temp buffer, unmap it from this process' mem */
    if (pe->temp == ET_EVENT_TEMP) {
        if (munmap(pe->pdata, pe->memsize) != 0) {
            et_mem_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_event_put, error in munmap\n");
            }
            return ET_ERROR;
        }
    }

    stat_id = sys->attach[att].stat;
    ageOrig = pe->age;
    pe->age = ET_EVENT_USED;

    if ((status = et_station_write(etid, stat_id, pe)) != ET_OK) {
        /* undo things in case of error */
        pe->age = ageOrig;
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_put, cannot write event\n");
        }
        return status;
    }
  
    /* keep track of # of events put by this attachment */
    sys->attach[att].events_put++;

    /* release mutex */
    et_mem_unlock(etid);

    return ET_OK;
}


/******************************************************/
int et_events_put(et_sys_id id, et_att_id att, et_event *pe[], int num)
{
    int i, status;
    et_stat_id stat_id;
    et_id      *etid = (et_id *) id;
    et_system  *sys  = etid->sys;
 
    if (num == 0) {
        return ET_OK;
    }
  
    if ((num < 0) || (att < 0) || (pe == NULL)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_put, bad argument(s)\n");
        }
        return ET_ERROR;
    }
   
    if (etid->locality == ET_REMOTE) {
        return etr_events_put(id, att, pe, num);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_events_put(id, att, pe, num);
    }
  
    if(!et_alive(id)) {
        return ET_ERROR_DEAD;
    }
  
    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_put, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    for (i=0; i < num; i++) {
        /* if length bigger than memory size, we got problems  */
        if (pe[i]->length > pe[i]->memsize) {
            et_mem_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_events_put, 1 or more data lengths are too large!\n");
            }
            return ET_ERROR;
        }
     
        /* If not event owner, can't write.  */
        if (pe[i]->owner != att) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_events_put, attachment (%d) not event owner (%d) so can't put back\n",
                          att, pe[i]->owner);
            }
            et_mem_unlock(etid);
            return ET_ERROR;
        }
    }
        
    for (i=0; i < num ; i++) {
        /* if temp buffer, unmap it from this process' mem */
        if (pe[i]->temp == ET_EVENT_TEMP) {
            /*printf("et_events_put: unmap temp event\n");*/
            if (munmap(pe[i]->pdata, pe[i]->memsize) != 0) {
                et_mem_unlock(etid);
                if (etid->debug >= ET_DEBUG_ERROR) {
                    et_logmsg("ERROR", "et_events_put, error in munmap\n");
                }
                return ET_ERROR;
            }
        }
        /* Store age here temporarily in case it needs to revert if error, modify is not used normally. */
        pe[i]->modify = pe[i]->age;
        pe[i]->age = ET_EVENT_USED;
    }

    stat_id = sys->attach[att].stat;
  
    if ( (status = et_station_nwrite(etid, stat_id, pe, num)) != ET_OK) {
        /* undo things in case of error */
        for (i=0; i < num; i++) {
            pe[i]->age = pe[i]->modify;
            pe[i]->modify = 0;
        }
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_put, cannot write events\n");
        }
        return status;
    }
  
    /* keep track of # of events put by this attachment */
    sys->attach[att].events_put += num;

    /* release mutex */
    et_mem_unlock(etid);

    return ET_OK;
}


/******************************************************/
int et_event_dump(et_sys_id id, et_att_id att, et_event *pe)
{
    int status, temp=0;
    et_id      *etid = (et_id *) id;
    et_system  *sys  = etid->sys;

    if ((att < 0) || (pe == NULL)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_dump, bad argument(s)\n");
        }
        return ET_ERROR;
    }
  
    if (etid->locality == ET_REMOTE) {
        return etr_event_dump(id, att, pe);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_event_dump(id, att, pe);
    }
  
    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }

    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_dump, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if (pe->owner != att) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_dump, not event owner so can't put back\n");
        }
        return ET_ERROR;
    }
  
    /* if temp buffer, unmap it from this process' mem */
    if (pe->temp == ET_EVENT_TEMP) {
        /* Since we're returning the event to GrandCentral, we need to
         * do what et_conductor normally does. That is, we must unlink
         * (as well as unmap) the file and also decrement sys->ntemps.
         */
        if (et_temp_remove(pe->filename, pe->pdata, (size_t) pe->memsize) != ET_OK) {
            et_mem_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_event_dump, error in removing temp mem\n");
            }
            return ET_ERROR;
        }
        temp++;
    }

    if ((status = et_station_dump(etid, pe)) != ET_OK) {
        pe->owner = att;
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_dump, cannot write event\n");
        }
        return status;
    }
  
    if (temp) {
        et_system_lock(sys);
        sys->ntemps--;
        et_system_unlock(sys);
    }
  
    /* keep track of # of events dumped by this attachment */
    sys->attach[att].events_dump++;

    /* release mutex */
    et_mem_unlock(etid);

    return ET_OK;
}


/******************************************************/
int et_events_dump(et_sys_id id, et_att_id att, et_event *pe[], int num)
{
    int i, status, ntemps=0;
    et_id      *etid = (et_id *) id;
    et_system  *sys  = etid->sys;
 
    if (num == 0) {
        return ET_OK;
    }
  
    if ((att < 0) || (pe == NULL) || (num < 0)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_dump, bad argument(s)\n");
        }
        return ET_ERROR;
    }
  
    if (etid->locality == ET_REMOTE) {
        return etr_events_dump(id, att, pe, num);
    }
    else if (etid->locality == ET_LOCAL_NOSHARE) {
        return etn_events_dump(id, att, pe, num);
    }
  
    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }
  
    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_dump, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    for (i=0; i < num; i++) {
        /* If not event owner, can't write.  */
        if (pe[i]->owner != att) {
            et_mem_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_events_dump, not event owner so can't put back\n");
            }
            return ET_ERROR;
        }
    }
      
    for (i=0; i < num; i++) {
        /* if temp buffer, unmap it from this process' mem */
        if (pe[i]->temp == ET_EVENT_TEMP) {
            /* Since we're returning the event to GrandCentral, we need to
             * do what et_conductor normally does. That is, we must unlink
             * (as well as unmap) the file and also decrement sys->ntemps.
             */
            if (et_temp_remove(pe[i]->filename, pe[i]->pdata, (size_t) pe[i]->memsize) != ET_OK) {
                et_mem_unlock(etid);
                if (etid->debug >= ET_DEBUG_ERROR) {
                    et_logmsg("ERROR", "et_event_dump, error in removing temp mem\n");
                }
                return ET_ERROR;
            }
            ntemps++;
        }
    }

    if ( (status = et_station_ndump(etid, pe, num)) != ET_OK) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_events_dump, cannot write events\n");
        }
        return status;
    }
  
    if (ntemps > 0) {
        et_system_lock(sys);
        sys->ntemps -= ntemps;
        et_system_unlock(sys);
    }
  
    /* keep track of # of events dumped by this attachment */
    sys->attach[att].events_dump += num;

    /* release mutex */
    et_mem_unlock(etid);

    return ET_OK;
}


/******************************
 *  ACCESS TO EVENT STRUCTURE
 *
 * Note  concerning pointers to et_event structures:
 *       If an et_close() is done, those pointers
 *       must NOT be used or a seg fault will result.
 *       Let this serve as a warning to all users.
 *
 ******************************/

/******************************************************/
int et_event_getgroup(et_event *pe, int *grp)
{ 
  if (grp == NULL) return ET_ERROR;
  *grp = pe->group;
  return ET_OK;
}

/******************************************************/
int et_event_setpriority(et_event *pe, int pri)
{ 
  if (pri != ET_HIGH && pri != ET_LOW) {
    return ET_ERROR;
  }
  
  pe->priority = pri;
  return ET_OK;
}

/******************************************************/
int et_event_getpriority(et_event *pe, int *pri)
{ 
  if (pri == NULL) return ET_ERROR;
  *pri = pe->priority;
  return ET_OK;
}

/******************************************************/
int et_event_setlength(et_event *pe, size_t len)
{ 
  if (len < 0) return ET_ERROR;
  if (len > pe->memsize) return ET_ERROR;
  pe->length = len;
  return ET_OK;
}

/******************************************************/
int et_event_getlength(et_event *pe, size_t *len)
{ 
  if (len == NULL) return ET_ERROR;
  *len = pe->length;
  return ET_OK;
}

/******************************************************/
int et_event_getdata(et_event *pe, void **data)
{ 
  if (data == NULL) return ET_ERROR;
  *data = pe->pdata;
  return ET_OK;
}

/******************************************************/
int et_event_setcontrol(et_event *pe, int con[], int num)
{ 
  int i;
  
  if ((num < 1) || (num > ET_STATION_SELECT_INTS)) {
    return ET_ERROR;
  }
  
  for (i=0; i < num; i++) {
    pe->control[i] = con[i];
  }
  
  return ET_OK;
}

/******************************************************/
int et_event_getcontrol(et_event *pe, int con[])
{ 
  int i;
  
  if (con == NULL) return ET_ERROR;
  
  for (i=0; i < ET_STATION_SELECT_INTS; i++) {
    con[i] = pe->control[i];
  }
  
  return ET_OK;
}

/******************************************************/
int et_event_setdatastatus(et_event *pe, int datastatus)
{ 
  if ((datastatus != ET_DATA_OK) &&
      (datastatus != ET_DATA_CORRUPT) &&
      (datastatus != ET_DATA_POSSIBLY_CORRUPT)) {
    return ET_ERROR;
  }
  
  pe->datastatus = datastatus;
  return ET_OK;
}

/******************************************************/
int et_event_getdatastatus(et_event *pe, int *datastatus)
{ 
  if (datastatus == NULL) return ET_ERROR;
  *datastatus = pe->datastatus;
  return ET_OK;
}

/******************************************************/
int et_event_setendian(et_event *pe, int endian)
{
  int err, myendian;
  
  if ( (err = etNetLocalByteOrder(&myendian)) != ET_OK) {
    return err;
  }
  
  if ((endian != ET_ENDIAN_BIG)      &&
      (endian != ET_ENDIAN_LITTLE)   &&
      (endian != ET_ENDIAN_LOCAL)    &&
      (endian != ET_ENDIAN_NOTLOCAL) &&
      (endian != ET_ENDIAN_SWITCH))     {
    return ET_ERROR;
  }
  
  if ((endian == ET_ENDIAN_BIG) || (endian == ET_ENDIAN_LITTLE)) {
    pe->byteorder = (myendian == endian) ? 0x04030201 : 0x01020304;
  }
  else if (endian == ET_ENDIAN_LOCAL) {
    pe->byteorder = 0x04030201;
  }
  else if (endian == ET_ENDIAN_NOTLOCAL) {
    pe->byteorder = 0x01020304;
  }
  else {
    pe->byteorder = ET_SWAP32(pe->byteorder);
  }

  return ET_OK;
}

/******************************************************/
int et_event_getendian(et_event *pe, int *endian)
{ 
  int err, myendian, notmyendian;
  
  if (endian == NULL) {
    return ET_ERROR;
  }
  
  if ( (err = etNetLocalByteOrder(&myendian)) != ET_OK) {
      return err;
  }
  
  notmyendian = (myendian == ET_ENDIAN_BIG) ? ET_ENDIAN_LITTLE : ET_ENDIAN_BIG;
  *endian = (pe->byteorder == 0x04030201) ? myendian : notmyendian;
  
  return ET_OK;
}

/******************************************************/
int et_event_needtoswap(et_event *pe, int *swap)
{   
  if (swap == NULL) return ET_ERROR;
  *swap = (pe->byteorder == 0x04030201) ? ET_NOSWAP : ET_SWAP;
  
  return ET_OK;
}

/*****************************************************
 * Routine which swaps the data in an event if it's in
 * the CODA format and automatically keeps track of the
 * swap. (No need to call et_event_setendian).
 *
 * This function is deprecated. Use the swapping
 * function contained in the evio library.
 *****************************************************/
 
int et_event_CODAswap(et_event *pe)
{
  int length, same_endian=1;
  length = pe->length/sizeof(int);

  /* DEPRECATED: Swapping is now done in evio library. */
  return ET_ERROR;
  
  /* event's data written on diff endian machine as this host? */
  if (pe->byteorder != 0x04030201) {
    same_endian = 0;
  }
  
  /* swap the data */
  if (et_CODAswap((int *)pe->pdata, NULL, length, same_endian) != ET_OK) {
    return ET_ERROR;
  }
  
  /* must also swap the "endian" element of the header
   * since it's byte order mirrors that of the data
   */
  pe->byteorder = ET_SWAP32(pe->byteorder);
  
  return ET_OK;
}
