/*----------------------------------------------------------------------------*
 *  Copyright (c) 2001        Southeastern Universities Research Association, *
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
 *      Attachment routines
 *
 *----------------------------------------------------------------------------*/

#include "et_private.h"

/**
 * @defgroup attach Attachments
 *
 * These routines are used handle attachments to stations.
 *
 * @{
 */

/**
 * This routine wakes up a specific attachment waiting to read events.
 *
 * @param id    ET system id.
 * @param att   attachment id.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if bad arg.
 * @returns @ref ET_ERROR_CLOSED  if ET system has been closed,
 * @returns @ref ET_ERROR_WRITE   if network error writing,
 */
int et_wakeup_attachment(et_sys_id id, et_att_id att) {

    int status;
    et_id      *etid = (et_id *) id;
    et_stat_id  stat_id;
    et_station *ps;
    et_list    *pl, *pl_gc;

    if (etid == NULL || att < 0) {
        if (etid == NULL || etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_wakeup_attachment, bad argument\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_wakeup_attachment(id, att);
    }

    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_wakeup_attachment, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    stat_id = etid->sys->attach[att].stat;
    ps = etid->grandcentral + stat_id;
    pl = &ps->list_in;
    pl_gc = &etid->grandcentral->list_in;

    if (att >= etid->sys->config.nattachments) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_wakeup_attachment, bad argument\n");
        }
        return ET_ERROR;
    }

    /* Only tell things to wake up if they are sleeping, otherwise
     * after the NEXT read, it will quit as if being woken up! */
    if ((etid->sys->attach[att].blocked == ET_ATT_UNBLOCKED) &&
        (etid->sys->attach[att].sleep   == ET_ATT_NOSLEEP)) {

        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_WARN) {
            et_logmsg("WARN", "et_wakeup_attachment, attachment is NOT blocked so not sending wakeup signal\n");
        }
        return ET_OK;
    }

    if (etid->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_wakeup_attachment, waking up attachment %d\n", att);
    }
    /* attachment may be waiting on a "get", so wake own station in list */
    etid->sys->attach[att].quit = ET_ATT_QUIT;
    status = pthread_cond_broadcast(&pl->cread);
    if (status != 0) {
        err_abort(status, "Wakeup readers");
    }
    /* attachment may be waiting on a "new", so wake GrandCentral's in list */
    status = pthread_cond_broadcast(&pl_gc->cread);
    if (status != 0) {
        err_abort(status, "Wakeup readers");
    }

    et_mem_unlock(etid);
    return ET_OK;
}
 

/**
 * This routine wakes up all attachments waiting to read events.
 *
 * @param id    ET system id.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if bad arg.
 * @returns @ref ET_ERROR_CLOSED  if ET system has been closed,
 * @returns @ref ET_ERROR_WRITE   if network error writing,
 */
int et_wakeup_all(et_sys_id id, et_stat_id stat_id) {

    int i, status;
    et_att_id att;
    et_id *etid = (et_id *) id;
    et_station *ps;
    et_list    *pl, *pl_gc;

    if (etid == NULL || stat_id < 0) {
        if (etid == NULL || etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_wakeup_all, bad argument\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_wakeup_all(id, stat_id);
    }

    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_wakeup_all, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    ps = etid->grandcentral + stat_id;
    pl = &ps->list_in;
    pl_gc = &etid->grandcentral->list_in;

    if (stat_id >= etid->sys->config.nstations) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_wakeup_all, bad argument\n");
        }
        return ET_ERROR;
    }

    for (i=0; i < etid->sys->config.nattachments ; i++) {
        att = ps->data.att[i];
        if (att > -1) {
            if ((etid->sys->attach[att].blocked == ET_ATT_BLOCKED) ||
                (etid->sys->attach[att].sleep == ET_ATT_SLEEP)) {
                etid->sys->attach[att].quit = ET_ATT_QUIT;
                if (etid->debug >= ET_DEBUG_INFO) {
                    et_logmsg("INFO", "et_wakeup_all, waking up attachment %d\n", att);
                }
            }
        }
    }
    /* attachment may be waiting on a "get", so wake own station in list */
    status = pthread_cond_broadcast(&pl->cread);
    if (status != 0) {
        err_abort(status, "Wakeup all readers");
    }
    /* attachment may be waiting on a "new", so wake GrandCentral's in list */
    status = pthread_cond_broadcast(&pl_gc->cread);
    if (status != 0) {
        err_abort(status, "Wakeup all readers");
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the number of events an attachment put by calling @ref et_event_put
 * or @ref et_events_put.
 *
 * @param id        ET system id.
 * @param att_id    attachment id.
 * @param events    pointer to unsigned 64-bit int which gets filled with number of events put.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if bad arg.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead,
 * @returns @ref ET_ERROR_CLOSED  if ET system has been closed,
 * @returns @ref ET_ERROR_READ    if network error reading,
 * @returns @ref ET_ERROR_WRITE   if network error writing,
 */
int et_attach_geteventsput(et_sys_id id, et_att_id att_id, uint64_t *events) {

    et_id *etid = (et_id *) id;

    if (etid->locality == ET_REMOTE) {
        return etr_attach_geteventsput(id, att_id, events);
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
            et_logmsg("ERROR", "et_attach_geteventsput, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((att_id < 0) || (att_id >= etid->sys->config.nattachments)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_attach_geteventsput, bad attachment id\n");
        }
        return ET_ERROR;
    }

    if (events != NULL) {
        *events = etid->sys->attach[att_id].events_put;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the number of events an attachment obtained by calling @ref et_event_get,
 * or @ref et_events_get.
 *
 * @param id        ET system id.
 * @param att_id    attachment id.
 * @param events    pointer to unsigned 64-bit int which gets filled with number of events gotten.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if bad arg.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead,
 * @returns @ref ET_ERROR_CLOSED  if ET system has been closed,
 * @returns @ref ET_ERROR_READ    if network error reading,
 * @returns @ref ET_ERROR_WRITE   if network error writing,
 */
int et_attach_geteventsget(et_sys_id id, et_att_id att_id, uint64_t *events) {

    et_id *etid = (et_id *) id;

    if (etid->locality == ET_REMOTE) {
        return etr_attach_geteventsget(id, att_id, events);
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
            et_logmsg("ERROR", "et_attach_geteventsget, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((att_id < 0) || (att_id >= etid->sys->config.nattachments)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_attach_geteventsget, bad attachment id\n");
        }
        return ET_ERROR;
    }

    if (events != NULL) {
        *events = etid->sys->attach[att_id].events_get;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the number of events an attachment dumped by calling @ref et_event_dump,
 * or @ref et_events_dump.
 *
 * @param id        ET system id.
 * @param att_id    attachment id.
 * @param events    pointer to unsigned 64-bit int which gets filled with number of events dumped.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if bad arg.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead,
 * @returns @ref ET_ERROR_CLOSED  if ET system has been closed,
 * @returns @ref ET_ERROR_READ    if network error reading,
 * @returns @ref ET_ERROR_WRITE   if network error writing,
 */
int et_attach_geteventsdump(et_sys_id id, et_att_id att_id, uint64_t *events) {

    et_id *etid = (et_id *) id;

    if (etid->locality == ET_REMOTE) {
        return etr_attach_geteventsdump(id, att_id, events);
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
            et_logmsg("ERROR", "et_attach_geteventsdump, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((att_id < 0) || (att_id >= etid->sys->config.nattachments)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_attach_geteventsdump, bad attachment id\n");
        }
        return ET_ERROR;
    }

    if (events != NULL) {
        *events = etid->sys->attach[att_id].events_dump;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the number of events an attachment created by calling @ref et_event_new,
 * @ref et_events_new, @ref et_event_new_group, or @ref et_events_new_group.
 *
 * @param id        ET system id.
 * @param att_id    attachment id.
 * @param events    pointer to unsigned 64-bit int which gets filled with number of events made.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if bad arg.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead,
 * @returns @ref ET_ERROR_CLOSED  if ET system has been closed,
 * @returns @ref ET_ERROR_READ    if network error reading,
 * @returns @ref ET_ERROR_WRITE   if network error writing,
 */
int et_attach_geteventsmake(et_sys_id id, et_att_id att_id, uint64_t *events) {

    et_id *etid = (et_id *) id;

    if (etid->locality == ET_REMOTE) {
        return etr_attach_geteventsmake(id, att_id, events);
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
            et_logmsg("ERROR", "et_attach_geteventsmake, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((att_id < 0) || (att_id >= etid->sys->config.nattachments)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_attach_geteventsmake, bad attachment id\n");
        }
        return ET_ERROR;
    }

    if (events != NULL) {
        *events = etid->sys->attach[att_id].events_make;
    }

    et_mem_unlock(etid);
    return ET_OK;
}

/** @} */
