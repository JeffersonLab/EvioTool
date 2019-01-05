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
 *      Routines for transferring events between 2 ET systems
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>

#include "et_private.h"


static int localET_2_localET(et_sys_id id_from, et_sys_id id_to,
		                     et_att_id att_from, et_att_id att_to,
                             et_bridge_config *config,
                             int num, int *ntransferred);

static int remoteET_2_ET(et_sys_id id_from, et_sys_id id_to,
                         et_att_id att_from, et_att_id att_to,
                         et_bridge_config *config,
                         int num, int *ntransferred);

static int ET_2_remoteET(et_sys_id id_from, et_sys_id id_to,
                         et_att_id att_from, et_att_id att_to,
                         et_bridge_config *config,
                         int num, int *ntransferred);

/*****************************************************/
/*               BRIDGE CONFIGURATION                */
/*****************************************************/

/**
 * @defgroup bridge Bridge
 *
 *  These routines are used both to configure a bridge between 2 ET systems and to
 *  actually run the bridge.
 *
 * @{
 */

/**
 * This routine initializes a configuration used to establish a bridge
 * between 2 ET systems.
 *
 * This MUST be done prior to setting any configuration parameters or all setting
 * routines will return an error.
 *
 * @param config   pointer to an bridge configuration variable
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if arg is NULL or failure to allocate memory for configuration data storage.
 */
int et_bridge_config_init(et_bridgeconfig *config) {

    et_bridge_config *bc;

    bc = (et_bridge_config *) malloc(sizeof(et_bridge_config));
    if (config == NULL || bc == NULL) {
        return ET_ERROR;
    }

    /* default configuration for a station */
    bc->mode_from            = ET_SLEEP;
    bc->mode_to              = ET_SLEEP;
    bc->chunk_from           = 100;
    bc->chunk_to             = 100;
    bc->timeout_from.tv_sec  = 0;
    bc->timeout_from.tv_nsec = 0;
    bc->timeout_to.tv_sec    = 0;
    bc->timeout_to.tv_nsec   = 0;
    bc->func                 = NULL;
    bc->init                 = ET_STRUCT_OK;

    *config = (et_bridgeconfig) bc;
    return ET_OK;
}


/**
 * This routine frees the memory allocated when a bridge configuration is initialized
 * by @ref et_bridge_config_init.
 *
 * @param sconfig   bridge configuration.
 *
 * @returns @ref ET_OK.
 */
int et_bridge_config_destroy(et_bridgeconfig sconfig) {

    if (sconfig == NULL) return ET_OK;
    free((et_bridge_config *) sconfig);
    return ET_OK;
}


/**
 * This routine sets the mode of getting events from the "from" ET system.
 *
 * @param sconfig    bridge configuration.
 * @param val        is set to either @ref ET_SLEEP, @ref ET_TIMED, or @ref ET_ASYNC and determines
 *                   the mode of getting events from the "from" ET system.
 *                   The default is ET_SLEEP.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if config is NULL or not initialized;
 *                          if val is not ET_SLEEP, ET_TIMED, or ET_ASYNC.
 */
int et_bridge_config_setmodefrom(et_bridgeconfig config, int val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_SLEEP) &&
        (val != ET_TIMED) &&
        (val != ET_ASYNC))  {
        return ET_ERROR;
    }

    bc->mode_from = val;
    return ET_OK;
}


/**
 * This routine gets the mode of getting events from the "from" ET system.
 *
 * @param sconfig    bridge configuration.
 * @param val        int pointer that gets filled with either @ref ET_SLEEP, @ref ET_TIMED,
 *                   or @ref ET_ASYNC.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or config not initialized.
 */
int et_bridge_config_getmodefrom(et_bridgeconfig config, int *val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (val == NULL || bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = bc->mode_from;
    return ET_OK;
}


/**
 * This routine sets the mode of getting new events from the "to" ET system.
 *
 * @param sconfig    bridge configuration.
 * @param val        is set to either @ref ET_SLEEP, @ref ET_TIMED, or @ref ET_ASYNC and determines
 *                   the mode of getting new events from the "to" ET system.
 *                   The default is ET_SLEEP.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if config is NULL or not initialized;
 *                          if val is not ET_SLEEP, ET_TIMED, or ET_ASYNC.
 */
int et_bridge_config_setmodeto(et_bridgeconfig config, int val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_SLEEP) &&
        (val != ET_TIMED) &&
        (val != ET_ASYNC))  {
        return ET_ERROR;
    }

    bc->mode_to = val;
    return ET_OK;
}


/**
 * This routine gets the mode of getting new events from the "to" ET system.
 *
 * @param sconfig    bridge configuration.
 * @param val        int pointer that gets filled with either @ref ET_SLEEP, @ref ET_TIMED,
 *                   or @ref ET_ASYNC.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or config not initialized.
 */
int et_bridge_config_getmodeto(et_bridgeconfig config, int *val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (val == NULL || bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = bc->mode_to;
    return ET_OK;
}


/**
 * This routine sets the maximum number of events to get from the "from" ET system
 * in a single call to @ref et_events_get - the number of events to get in one chunk.
 *
 * @param sconfig    bridge configuration.
 * @param val        chunk size is any int > 0 with default being 100.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if config is NULL or not initialized;
 *                          if val is < 1.
 */
int et_bridge_config_setchunkfrom(et_bridgeconfig config, int val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (val < 1 || bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    bc->chunk_from = val;
    return ET_OK;
}


/**
 * This routine gets the maximum number of events to get from the "from" ET system
 * in a single call to @ref et_events_get - the number of events to get in one chunk.
 *
 * @param sconfig    bridge configuration.
 * @param val        int pointer that gets filled with chunk size.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or config not initialized.
 */
int et_bridge_config_getchunkfrom(et_bridgeconfig config, int *val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (val == NULL || bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = bc->chunk_from;
    return ET_OK;
}


/**
 * This routine sets the maximum number of new events to get from the "to" ET system
 * in a single call to @ref et_events_new - the number of events to get in one chunk.
 *
 * @param sconfig    bridge configuration.
 * @param val        chunk size is any int > 0 with default being 100.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if config is NULL or not initialized;
 *                          if val is < 1.
 */
int et_bridge_config_setchunkto(et_bridgeconfig config, int val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (val < 1 || bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    bc->chunk_to = val;
    return ET_OK;
}


/**
 * This routine gets the maximum number of new events to get from the "to" ET system
 * in a single call to @ref et_events_new - the number of events to get in one chunk.
 *
 * @param sconfig    bridge configuration.
 * @param val        int pointer that gets filled with chunk size.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or config not initialized.
 */
int et_bridge_config_getchunkto(et_bridgeconfig config, int *val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (val == NULL || bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = bc->chunk_to;
    return ET_OK;
}


/**
 * This routine sets the time to wait for the "from" ET system during all @ref et_events_get
 * calls when the mode is set to ET_TIMED.
 *
 * @param sconfig    bridge configuration.
 * @param val        time to wait for events from the "from" ET system (default = 0 sec).
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if config is NULL or not initialized.
 */
int et_bridge_config_settimeoutfrom(et_bridgeconfig config, struct timespec val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    bc->timeout_from = val;
    return ET_OK;
}


/**
 * This routine gets the time to wait for the "from" ET system during all @ref et_events_get
 * calls when the mode is set to ET_TIMED.
 *
 * @param sconfig    bridge configuration.
 * @param val        pointer that gets filled with time to wait for events from the "from" ET system.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or config not initialized.
 */
int et_bridge_config_gettimeoutfrom(et_bridgeconfig config, struct timespec *val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (val == NULL || bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = bc->timeout_from;
    return ET_OK;
}


/**
 * This routine sets the time to wait for the "to" ET system during all @ref et_events_new
 * calls when the mode is set to ET_TIMED.
 *
 * @param sconfig    bridge configuration.
 * @param val        time to wait for new events from the "to" ET system (default = 0 sec).
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if config is NULL or not initialized.
 */
int et_bridge_config_settimeoutto(et_bridgeconfig config, struct timespec val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    bc->timeout_to = val;
    return ET_OK;
}


/**
 * This routine gets the time to wait for the "to" ET system during all @ref et_events_new
 * calls when the mode is set to ET_TIMED.
 *
 * @param sconfig    bridge configuration.
 * @param val        pointer that gets filled with time to wait for new events from the "to" ET system.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or config not initialized.
 */
int et_bridge_config_gettimeoutto(et_bridgeconfig config, struct timespec *val) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (val == NULL || bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    *val = bc->timeout_to;
    return ET_OK;
}


/**
 * This routine sets the function used to automatically swap data from one endian
 * to another when bridging events between two ET systems.
 *
 * The function must be of the form: int func(et_event *src, et_event *dest, int bytes, int same_endian)
 * and must return ET_OK if successful, else ET_ERROR. The arguments consists of: src which is a pointer
 * to the event whose data is to be swapped, dest which is a pointer to the event where the swapped data
 * goes, bytes which tells the length of the data in bytes, and same_endian which is a flag equalling
 * one if the machine and the data are of the same endian and zero otherwise. This function must be
 * able to work with src and dest being the same event. With this as a prototype, the user can write
 * a routine which swaps data in the appropriate manner. Notice that the first two arguments are
 * pointers to events and not data buffers. This allows the writer of such a routine to have access
 * to any of the event's header information. In general, such functions should NOT call
 * @ref et_event_setendian in order to change the registered endian value of the data. This is already
 * taken care of in @ref et_events_bridge.
 *
 * @param sconfig    bridge configuration.
 * @param val        swapping function pointer.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if config is NULL or not initialized.
 */
int et_bridge_config_setfunc(et_bridgeconfig config, ET_SWAP_FUNCPTR func) {

    et_bridge_config *bc = (et_bridge_config *) config;

    if (bc == NULL || bc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    bc->func = func;
    return ET_OK;
}


/*****************************************************/
/*                 BRIDGE ROUTINES                   */
/*****************************************************/

/**
 * This routine transfers events between two ET systems in which events are copied from the "from" ET system
 * and placed into the "to" ET system.
 *
 * A function may be provided to swap the data during the transfer.
 * For the best performance, the process calling this routine should be on the same machine as either
 * the "from" or "to" ET systems. Some experimentation is in order to determine which of the two machines
 * will run the bridging faster. The author's experience suggests that placing the process on the machine
 * with the most processors or computing power will probably give the best results.
 *
 * @param id_from        ET system id from which the events are copied.
 * @param id_to          ET system id into which the events are placed.
 * @param att_from       attachment to a station on the "from" ET system.
 * @param att_to         attachment to a station on the "to" ET system (usually GrandCentral).
 * @param bconfig        configuration of the remaining transfer parameters.
 * @param num            total number of events desired to be transferred.
 * @param ntransferred   int pointer that gets filled with the total number of events that were
 *                       actually transferred at the routine's return.
 *
 * @returns @ref ET_OK             if successful.
 * @returns @ref ET_ERROR          if error.
 * @returns @ref ET_ERROR_REMOTE   for a memory allocation error of a remote user.
 * @returns @ref ET_ERROR_READ     for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE    for a remote user's network write error.
 * @returns @ref ET_ERROR_DEAD     if ET system is dead.
 * @returns @ref ET_ERROR_WAKEUP   if told to stop sleeping while trying to get an event.
 * @returns @ref ET_ERROR_TIMEOUT  if timeout on @ref ET_TIMED option.
 * @returns @ref ET_ERROR_BUSY     if cannot get access to events due to activity of other processes
 *                                 when in @ref ET_ASYNC mode.
 * @returns @ref ET_ERROR_EMPTY    if no events available in @ref ET_ASYNC mode.
 */
int et_events_bridge(et_sys_id id_from, et_sys_id id_to,
                     et_att_id att_from, et_att_id att_to,
                     et_bridgeconfig bconfig,
                     int num, int *ntransferred) {

    et_id *idfrom = (et_id *) id_from, *idto = (et_id *) id_to;
    et_bridge_config *config;
    et_bridgeconfig   default_config = NULL;
    int status, auto_config=0;

    *ntransferred = 0;

    /* The program calling this routine has already opened 2 ET
     * systems. Therefore, both must have been compatible with
     * the ET lib used to compile this program and are then
     * compatible with eachother as well.
     */

    /* if no configuration is given, use defaults */
    if (bconfig == NULL) {
        auto_config = 1;
        if (et_bridge_config_init(&default_config) == ET_ERROR) {
            if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
                et_logmsg("ERROR", "et_events_bridge, null arg for \"bconfig\", cannot use default\n");
            }
            return ET_ERROR;
        }
        bconfig = default_config;
    }
    config = (et_bridge_config *) bconfig;

    /* if we have a local ET to local ET transfer ... */
    if ((idfrom->locality != ET_REMOTE) && (idto->locality != ET_REMOTE)) {
        status = localET_2_localET(id_from, id_to, att_from, att_to,
                                   config, num, ntransferred);
    }
    /* else if getting events from remote ET and sending to local ET ... */
    else if ((idfrom->locality == ET_REMOTE) && (idto->locality != ET_REMOTE)) {
        status = remoteET_2_ET(id_from, id_to, att_from, att_to,
                               config, num, ntransferred);
    }
        /* else if getting events from local ET and sending to remote ET or
         * else going from remote to remote systems.
         *
         * If we have a remote ET to remote ET transfer, we
         * can use either ET_2_remoteET or remoteET_2_ET.
         */
    else {
        status = ET_2_remoteET(id_from, id_to, att_from, att_to,
                               config, num, ntransferred);
    }

    if (auto_config) {
        et_bridge_config_destroy(default_config);
    }

    return status;
}

/** @} */

/******************************************************/
static int localET_2_localET(et_sys_id id_from, et_sys_id id_to,
		                     et_att_id att_from, et_att_id att_to,
		                     et_bridge_config *config,
		                     int num, int *ntransferred)
{
  et_id *idfrom = (et_id *) id_from, *idto = (et_id *) id_to;
  et_event **get, **put, **dump;
  int i, j, k, status=ET_OK;
  int swap=0, byteordershouldbe=0, same_endian;
  int num_2get, num_read=0, num_new=0, num_dump=0, num_new_prev=0;
  int total_put=0, total_read=0, total_new=0;

  /* never deal with more the total num of events in the "from" ET system */
  size_t num_limit = ((size_t)idfrom->nevents) & 0xffffffff;
  
  /* allocate arrays to hold the event pointers */
  if ((get = (et_event **) calloc(num_limit, sizeof(et_event *))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, l2l, cannot allocate memory\n");
    }
    return ET_ERROR;
  }
  
  if ((dump = (et_event **) calloc(num_limit, sizeof(et_event *))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, l2l, cannot allocate memory\n");
    }
    free(get);
    return ET_ERROR;
  }
  
  if ((put = (et_event **) calloc(num_limit, sizeof(et_event *))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, l2l, cannot allocate memory\n");
    }
    free(get); free(dump);
    return ET_ERROR;
  }
  
  /* swap the data if desired */
  if (config->func != NULL) {
    swap = 1;
    /* event's byteorder should = "byteordershouldbe" to avoid swap */
    byteordershouldbe = (idto->endian == idto->systemendian) ? 0x04030201 : 0x01020304;
  }
  
  while (total_read < num) {    
    /* first, get events from the "from" ET system */
    num_2get = (num - total_read < config->chunk_from) ? (num - total_read) : config->chunk_from;
    status = et_events_get(id_from, att_from, get, config->mode_from,
			               &config->timeout_from, num_2get, &num_read);
    if (status != ET_OK) {
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
	    et_logmsg("ERROR", "et_events_bridge, error (status = %d) getting events from \"from\" ET system\n",
			      status);
      }
      goto end;
    }
    total_read += num_read;
    
    /* init */
    num_new_prev = total_new = 0;
    
    /* Take those events we read from the "from" ET system and
     * copy them into new events from the "to" ET system.
     */
    while (total_new < num_read) {
      /* If the normal event size in "from" is bigger than the
       * normal event size in "to", perhaps we should NOT ask for a whole
       * bunch as each new one will be a temp event. It may be that
       * the data contained in a particular event may still be
       * smaller than the "to" event size. In this case, it may be
       * best to get them one-by-one to avoid any unneccesary mem
       * mapping of files. Do some tests here.
       */
      
      /* get new events from the "to" ET system */
      num_2get = (num_read - total_new < config->chunk_to) ? (num_read - total_new) : config->chunk_to;
      status = et_events_new(id_to, att_to, put, config->mode_to, &config->timeout_to,
                             idfrom->esize, num_2get, &num_new);
      if (status != ET_OK) {
        if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
          et_logmsg("ERROR", "et_events_bridge, error (status = %d) getting new events from \"to\" ET system\n",
                    status);
        }
      
        /* put back those read in last et_events_get call */
        et_events_put(id_from, att_from, get, num_read);
        goto end;
      }
      
      total_new += num_new;
      num_dump = 0;
      
      for (i=0; i < num_new; i++) {
        /* If data is larger than new event's memory size,
         * make a larger temp event and use it instead.
         * Dump the original new event that was too small.
         */
        j = i + num_new_prev;
        if (get[j]->length > put[i]->memsize) {
          dump[num_dump++] = put[i];
          status = et_event_new(id_to, att_to, &put[i], config->mode_to,
                                &config->timeout_to, get[j]->length);
          if (status != ET_OK) {
            if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
              et_logmsg("ERROR", "et_events_bridge, error (status = %d) getting a large event from \"to\" ET system\n",
                        status);
            }
            /* put back those read in last et_events_get call */
            et_events_put(id_from, att_from, get, num_read);
            /* put new events into the "to" ET system */
            if (et_events_put(id_to, att_to, put, i) == ET_OK) {
              /* we succeeded in transferring some events, record this */
              total_put += i;
            }
            /* dump the rest of the new events read in last et_events_new */
            et_events_dump(id_to, att_to, &put[i], num_new-i);
            /* dump other unwanted new events into the "to" ET system */
            et_events_dump(id_to, att_to, dump, --num_dump);
        
            goto end;
          }
        }
    
        /* copy relevant event data */
        put[i]->length    = get[j]->length;
        put[i]->priority  = get[j]->priority;
        put[i]->byteorder = get[j]->byteorder;
        for (k=0; k < idto->nselects; k++) {
          put[i]->control[k] = get[j]->control[k];
        }
    
        /* if not swapping data, just copy it */
        if (!swap) {
          memcpy(put[i]->pdata, get[j]->pdata, get[j]->length);
        }
        /* swap the data only if necessary */
        else {
          /* if event's byteorder is different than "to" system's, swap */
          if (put[i]->byteorder != byteordershouldbe) {
            /* event's data was written on same endian machine as this host? */
            same_endian = (put[i]->byteorder == 0x04030201) ? 1 : 0;

            if ((*config->func) (get[j], put[i], (int)(get[j]->length), same_endian) != ET_OK) {
              /* put back those read in last et_events_get call */
              et_events_put(id_from, att_from, get, num_read);
              /* put new events into the "to" ET system */
              if (et_events_put(id_to, att_to, put, i) == ET_OK) {
               /* we succeeded in transferring some events, record this */
                total_put += i;
              }
              /* dump the rest of the new events read in last et_events_new */
              et_events_dump(id_to, att_to, &put[i], num_new-i);
              /* dump other unwanted new events into the "to" ET system */
              et_events_dump(id_to, att_to, dump, num_dump);
              goto end;
            }
        
            put[i]->byteorder = ET_SWAP32(put[i]->byteorder);
          }
          else {
            memcpy(put[i]->pdata, get[j]->pdata, get[j]->length);
          }
        }
      }
      
      if (num_new) {
        /* put new events into the "to" ET system */
        status = et_events_put(id_to, att_to, put, num_new);
        if (status != ET_OK) {
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, error (status = %d) putting new events to \"to\" ET system\n",
                      status);
          }
          /* dump unused events into the "to" ET system */
          et_events_dump(id_to, att_to, dump, num_dump);
          /* put back those read in last et_events_get call */
          et_events_put(id_from, att_from, get, num_read);
          goto end;
        }
        total_put += num_new;
      }
      
      if (num_dump) {
        /* dump unused events into the "to" ET system */
        status = et_events_dump(id_to, att_to, dump, num_dump);
        if (status != ET_OK) {
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, error (status = %d) dumping unused events in \"to\" ET system\n",
                      status);
          }
          /* put back those read in last et_events_get call */
          et_events_put(id_from, att_from, get, num_read);
          goto end;
        }
      }
      
      num_new_prev = num_new;
    }
    
    /* put back events from the "from" ET system */
    status = et_events_put(id_from, att_from, get, num_read);
    if (status != ET_OK) {
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
        et_logmsg("ERROR", "et_events_bridge, error (status = %d) putting events into \"from\" ET system\n",
                  status);
      }
      goto end;
    }
  }
  
  
end:
  *ntransferred = total_put;
  free(get); free(put); free(dump);
  return status;
}

/******************************************************/
static int remoteET_2_ET(et_sys_id id_from, et_sys_id id_to,
                         et_att_id att_from, et_att_id att_to,
                         et_bridge_config *config,
                         int num, int *ntransferred)
{
  et_id *idfrom = (et_id *) id_from, *idto = (et_id *) id_to;
  et_event **put, **dump;
  int sockfd = idfrom->sockfd;
  int i=0, k, write_events=0, status=ET_OK, err;
  uint64_t len /*,size*/;
  int swap=0, byteordershouldbe=0, same_endian;
  int num_2get, num_read=0, num_new=0, num_dump=0;
  int total_put=0, total_read=0, total_new=0;
  int transfer[7], incoming[2], header[9+ET_STATION_SELECT_INTS];
  
  /* never deal with more the total num of events in the "from" ET system */
  size_t num_limit = ((size_t)idfrom->nevents) & 0xffffffff;

  /* allocate arrays to hold the event pointers */
  if ((dump = (et_event **) calloc(num_limit, sizeof(et_event *))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, l2l, cannot allocate memory\n");
    }
    return ET_ERROR;
  }
  
  if ((put = (et_event **) calloc(num_limit, sizeof(et_event *))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, l2l, cannot allocate memory\n");
    }
    free(dump);
    return ET_ERROR;
  }
  
  /* swap the data if desired */
  if (config->func != NULL) {
    swap = 1;
    /* event's byteorder should = "byteordershouldbe" to avoid swap */
    byteordershouldbe = (idto->endian == idto->systemendian) ? 0x04030201 : 0x01020304;
  }
  
  et_tcp_lock(idfrom);
  
  while (total_read < num) {        
    /* First, get events from the "from" ET system.
     * We're borrowing code from etr_events_get and
     * modifying the guts.
     */
    num_2get = (num - total_read < config->chunk_from) ? (num - total_read) : config->chunk_from;
    transfer[0] = htonl(ET_NET_EVS_GET);
    transfer[1] = htonl((uint32_t)att_from);
    transfer[2] = htonl((uint32_t)(config->mode_from & ET_WAIT_MASK));
    transfer[3] = 0; /* we are not modifying data */
    transfer[4] = htonl((uint32_t)num_2get);
    transfer[5] = 0;
    transfer[6] = 0;
  
    if (config->mode_from == ET_TIMED) {
      transfer[5] = htonl((uint32_t)config->timeout_from.tv_sec);
      transfer[6] = htonl((uint32_t)config->timeout_from.tv_nsec);
    }
 
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
      et_tcp_unlock(idfrom);
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
        et_logmsg("ERROR", "et_events_bridge, write error\n");
      }
      free(dump); free(put);
      return ET_ERROR_WRITE;
    }
 
    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
      et_tcp_unlock(idfrom);
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
        et_logmsg("ERROR", "et_events_bridge, read error\n");
      }
      free(dump); free(put);
      return ET_ERROR_READ;
    }
    err = ntohl((uint32_t)err);
    
    if (err < 0) {
      et_tcp_unlock(idfrom);
      free(dump); free(put);
      return err;
    }
    
    /* read total size of data to come - in bytes */
    if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
      et_tcp_unlock(idfrom);
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
        et_logmsg("ERROR", "et_events_bridge, read error\n");
      }
      free(dump); free(put);
      return ET_ERROR_READ;
    }
    /*size = ET_64BIT_UINT(ntohl(incoming[0]),ntohl(incoming[1]));*/

    num_read = err;
    total_read += num_read;
    total_new = 0;
    
    /* Now that we know how many events we are about to receive,
     * get the new events from the "to" ET system & fill w/data.
     */
    while (total_new < num_read) {
      num_2get = (num_read - total_new < config->chunk_to) ? (num_read - total_new) : config->chunk_to;
      status = et_events_new(id_to, att_to, put, config->mode_to, &config->timeout_to,
                             idfrom->esize, num_2get, &num_new);
      if (status != ET_OK) {
        if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
          et_logmsg("ERROR", "et_events_bridge, error (status = %d) getting new events from \"to\" ET system\n",
                    status);
          et_logmsg("ERROR", "et_events_bridge, connection to \"from\" ET system will be broken, close & reopen system\n");
        }
        goto end;
      }

      total_new += num_new;
      num_dump = 0;
      
      for (i=0; i < num_new; i++) {
        /* Read in the event's header info */
        if (etNetTcpRead(sockfd, (void *) header, sizeof(header)) != sizeof(header)) {
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, reading event header error\n");
          }
          write_events = 1;
          status = ET_ERROR_READ;
          goto end;
        }
    
        /* data length in bytes */
        len = ET_64BIT_UINT(ntohl((uint32_t)header[0]),ntohl((uint32_t)header[1]));
    
        /* If data is larger than new event's memory size ... */
        if (len > put[i]->memsize) {
          dump[num_dump++] = put[i];
          status = et_event_new(id_to, att_to, &put[i], config->mode_to,
                                &config->timeout_to, len);
          if (status != ET_OK) {
            if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
              et_logmsg("ERROR", "et_events_bridge, error (status = %d) getting a large event from \"to\" ET system\n",
                        status);
              et_logmsg("ERROR", "et_events_bridge, connection to \"from\" ET system will be broken, close & reopen system\n");
            }
            write_events = 1;
            num_dump--;
            goto end;
          }
        }
    
        /* copy/read relevant event data */
        put[i]->length     = len;
        put[i]->priority   = ntohl((uint32_t)header[4]) & ET_PRIORITY_MASK;
        put[i]->datastatus =(ntohl((uint32_t)header[4]) & ET_DATA_MASK) >> ET_DATA_SHIFT;
        put[i]->byteorder  = header[7];
        for (k=0; k < idto->nselects; k++) {
          put[i]->control[k] = ntohl((uint32_t)header[k+9]);
        }
    
        if (etNetTcpRead(sockfd, put[i]->pdata, (int)len) != len) {
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, reading event data error\n");
          }
          write_events = 1;
          status = ET_ERROR_READ;
          goto end;
        }
    
        /* swap the data if desired & necessary */
        if (swap) {
          /* if event's byteorder is different than "to" system's, swap */
          if (put[i]->byteorder != byteordershouldbe) {
            /* event's data was written on same endian machine as this host */
            same_endian = (put[i]->byteorder == 0x04030201) ? 1 : 0;

            if ((*config->func) (put[i], put[i], (int)put[i]->length, same_endian) != ET_OK) {
              write_events = 1;
              status = ET_ERROR;
              goto end;
            }
            put[i]->byteorder = ET_SWAP32(put[i]->byteorder);
          }
        }
      }
            
      if (num_new) {
        /* put new events into the "to" ET system */
        status = et_events_put(id_to, att_to, put, num_new);
        if (status != ET_OK) {
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, error (status = %d) putting new events to \"to\" ET system\n",
                      status);
            et_logmsg("ERROR", "et_events_bridge, connection to \"from\" ET system may be broken, close & reopen system\n");
          }
          et_events_dump(id_to, att_to, dump, num_dump);
          goto end;
        }
        total_put += num_new;
      }
      
      if (num_dump) {
        /* dump unused events into the "to" ET system */
        status = et_events_dump(id_to, att_to, dump, num_dump);
        if (status != ET_OK) {
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, error (status = %d) dumping unused events in \"to\" ET system\n",
                      status);
            et_logmsg("ERROR", "et_events_bridge, connection to \"from\" ET system may be broken, close & reopen system\n");
          }
          goto end;
        }
      }
    }
    /* No need to put remote events back since the "from" ET system
     * server has already put them back, and since we didn't allocate
     * any memory but read them directly into local events, don't need
     * to free any memory.
     */
  }
  et_tcp_unlock(idfrom);
  
end:
  et_tcp_unlock(idfrom);
  if (write_events) {
    /* put what we got so far into the "to" ET system */
    if (et_events_put(id_to, att_to, put, i) == ET_OK) {
      /* we succeeded in transferring some events, record this */
      total_put += i;
    }
    /* dump the rest of the new events read in last et_events_new */
    et_events_dump(id_to, att_to, &put[i], num_new-i);
    /* dump other unwanted new events into the "to" ET system */
    et_events_dump(id_to, att_to, dump, num_dump);
  }
  *ntransferred = total_put;
  free(put); free(dump);
  return status;
}

/******************************************************/
static int ET_2_remoteET(et_sys_id id_from, et_sys_id id_to,
                         et_att_id att_from, et_att_id att_to,
                         et_bridge_config *config,
                         int num, int *ntransferred)
{
  et_id *idfrom = (et_id *) id_from, *idto = (et_id *) id_to;
  et_event **get=NULL, **put=NULL, **dump=NULL;
  int sockfd = idto->sockfd;
  int i, j, k, status=ET_OK, err;
  int swap=0, byteorder, byteordershouldbe=0, same_endian;
  int transfer[5], index=0, iov_bufs=0, *header=NULL;
  int num_2get, num_read=0, num_new=0, num_dump=0, num_new_prev=0;
  int total_put=0, total_read=0, total_new=0;
  uint64_t bytes;
  struct iovec *iov=NULL;
  void *databuf=NULL;
  
  /* This routine can be used to transfer events from one remote ET
   * system to another. In that case, each event transferred causes
   * 2X data size mem to be allocated + 210bytes of header + 60 bytes of
   * various useful arrays. We can only get as many events as the "from"
   * system allows.If it's 1000 events of 5k+210 bytes a piece, then we
   * need 10.2MB - not too bad. However, if the user wants to transfer
   * 10,000,000 events (num argument), then our get,dump,put,etc arrays
   * will take up 600MB! So we need to limit them. We'll never need more
   * than that to deal with the total number of events in the "from" ET system.
   */
  size_t num_limit = ((size_t)idfrom->nevents) & 0xffffffff;
  size_t headersize = (7+ET_STATION_SELECT_INTS)*sizeof(int);
  
  /* allocate arrays to hold the event pointers */
  if ((get = (et_event **) calloc(num_limit, sizeof(et_event *))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, cannot allocate memory\n");
    }
    return ET_ERROR;
  }
  
  if ((dump = (et_event **) calloc(num_limit, sizeof(et_event *))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, cannot allocate memory\n");
    }
    free(get);
    return ET_ERROR;
  }
  
  if ((put = (et_event **) calloc(num_limit, sizeof(et_event *))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, cannot allocate memory\n");
    }
    free(get); free(dump);
    return ET_ERROR;
  }
  
  if ( (iov = (struct iovec *) calloc(2*num_limit+1, sizeof(struct iovec))) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, cannot allocate memory\n");
    }
    free(get); free(dump); free(put);
    return ET_ERROR;
  }
  
  if ( (header = (int *) calloc(num_limit, headersize)) == NULL) {
    if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
      et_logmsg("ERROR", "et_events_bridge, cannot allocate memory\n");
    }
    free(get); free(dump); free(put); free(iov);
    return ET_ERROR;
  }

  /* swap the data if desired */
  if (config->func != NULL) {
    swap = 1;
    byteordershouldbe = (idto->endian == idto->systemendian) ? 0x04030201 : 0x01020304;
  }
  
  transfer[0] = htonl(ET_NET_EVS_PUT);
  transfer[1] = htonl((uint32_t)att_to);
  iov[iov_bufs].iov_base = (void *) transfer;
  iov[iov_bufs].iov_len  = sizeof(transfer);
  iov_bufs++;
    
  while (total_read < num) {    
    /* first, get events from the "from" ET system */
    num_2get = (num - total_read < config->chunk_from) ? (num - total_read) : config->chunk_from;
    status = et_events_get(id_from, att_from, get, config->mode_from,
                           &config->timeout_from, num_2get, &num_read);
    if (status != ET_OK) {
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
        et_logmsg("ERROR", "et_events_bridge, error (status = %d) getting events from \"from\" ET system\n",
                  status);
      }
      goto end;
    }
    total_read += num_read;
    
    /* init vars */
    num_new_prev = total_new = 0;
    
    /* Take those events we read from the "from" ET system and
     * copy them into new events from the "to" ET system.
     */
    while (total_new < num_read) {
      /* get new events from the "to" ET system (remote) */
      num_2get = (num_read - total_new < config->chunk_to) ? (num_read - total_new) : config->chunk_to;
      status = et_events_new(id_to, att_to, put, config->mode_to, &config->timeout_to,
                             idfrom->esize, num_2get, &num_new);
      if (status != ET_OK) {
        if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
          et_logmsg("ERROR", "et_events_bridge, error (status = %d) getting new events from \"to\" ET system\n",
                    status);
        }
        /* put back those read in last et_events_get call */
        et_events_put(id_from, att_from, get, num_read);
        goto end;
      }
      
      total_new += num_new;
      num_dump = 0;
      iov_bufs = 1;
      index = 0;
      bytes = 0ULL;
      
      for (i=0; i < num_new; i++) {
        /* If data is larger than new event's memory size,
         * make a larger temp event and use it instead.
         * Dump the original new event that was too small.
         */
        j = i + num_new_prev;
        if (get[j]->length > put[i]->memsize) {
          dump[num_dump++] = put[i];
          status = et_event_new(id_to, att_to, &put[i], config->mode_to,
                                &config->timeout_to, get[j]->length);
          if (status != ET_OK) {
            if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
              et_logmsg("ERROR", "et_events_bridge, error (status = %d) getting a large event from \"to\" ET system\n",
                        status);
            }
            /* put back those read in last et_events_get call */
            et_events_put(id_from, att_from, get, num_read);
            /* dump the useful new events into the "to" ET system */
            et_events_dump(id_to, att_to, put, num_new);
            /* dump the unwanted new events into the "to" ET system */
            et_events_dump(id_to, att_to, dump, --num_dump);

            goto end;
          }
        }
    
        /* here's where the data usually is */
        databuf   = get[j]->pdata;
        byteorder = get[j]->byteorder;
    
        /* Do NOT copy data into the locally allocated buffers
         * given by a remote et_events_new. Instead, when doing
         * the across-network put, make the pointers to the
         * the local event data buffers arguments to put's
         * "iov" buffers.
         */
     
        /* swap the data if desired & necessary */
        if (swap) {
          /* if event's byteorder is different than "to" system's, swap */
          if (get[j]->byteorder != byteordershouldbe) {
            /* event's data was written on same endian machine as this host */
            same_endian = (get[j]->byteorder == 0x04030201) ? 1 : 0;
            
            /* Use the buffers of the remote "new" events to put swapped
             * data in since we do not want to swap the data in the
             * get[j]->pdata buffer if it is part of the local ET system.
             * We would be changing the original data and not a copy.
             * The "new" event buffers are normally not used.
             */
            databuf = put[i]->pdata;
            
            if ((*config->func) (get[j], put[i], get[j]->length, same_endian) != ET_OK) {
              status = ET_ERROR;
              goto end;
            }
            /* bug, we don't want to swap byte order of original event */
            /* get[j]->byteorder = ET_SWAP32(get[j]->byteorder); */
            byteorder = ET_SWAP32(get[j]->byteorder);
          }
        }
    
        header[index]   = htonl((uint32_t)put[i]->place);
        header[index+1] = 0; /* not used */
        header[index+2] = htonl(ET_HIGHINT(get[j]->length));
        header[index+3] = htonl(ET_LOWINT(get[j]->length));
        header[index+4] = htonl((uint32_t) (get[j]->priority | get[j]->datastatus << ET_DATA_SHIFT));
        header[index+5] = byteorder;
        header[index+6] = 0; /* not used */

        for (k=0; k < ET_STATION_SELECT_INTS; k++) {
          header[index+7+k] = htonl((uint32_t)get[j]->control[k]);
        }
    
        iov[iov_bufs].iov_base = (void *) &header[index];
        iov[iov_bufs].iov_len  = headersize;
        iov_bufs++;
        iov[iov_bufs].iov_base = databuf;
        iov[iov_bufs].iov_len  = get[j]->length;
        iov_bufs++;
        bytes += headersize + get[j]->length;
        index += (7+ET_STATION_SELECT_INTS);
      }
      
      if (num_new) {
        /* put the new events into the "to" ET system */
        transfer[2] = htonl((uint32_t)num_new);
        transfer[3] = htonl(ET_HIGHINT(bytes));
        transfer[4] = htonl(ET_LOWINT(bytes));

        et_tcp_lock(idto);
        if (etNetTcpWritev(sockfd, iov, iov_bufs, 16) == -1) {
          et_tcp_unlock(idto);
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, write error\n");
          }
          /* put back those read in last et_events_get call */
          et_events_put(id_from, att_from, get, num_read);
          /* dump the useful new events (the ones we just tried to write) */
          et_events_dump(id_to, att_to, put, num_new);
          /* dump the unwanted new events into the "to" ET system */
          et_events_dump(id_to, att_to, dump, num_dump);
      
          status = ET_ERROR_WRITE;
          goto end;
        }

        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
          et_tcp_unlock(idto);
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, read error\n");
          }
          /* do NOT try to dump events we just wrote & failed to read the return err */
          /* put back those read in last et_events_get call */
          et_events_put(id_from, att_from, get, num_read);
          /* dump the unwanted new events into the "to" ET system */
          et_events_dump(id_to, att_to, dump, num_dump);
      
          status = ET_ERROR_READ;
          goto end;
        }
        else if ( (status = ntohl((uint32_t)err)) != ET_OK) {
          et_tcp_unlock(idto);
          /* do NOT try to dump events we just wrote & failed */
          /* put back those read in last et_events_get call */
          et_events_put(id_from, att_from, get, num_read);
          /* dump the unwanted new events into the "to" ET system */
          et_events_dump(id_to, att_to, dump, num_dump);
      
          goto end;
        }
        et_tcp_unlock(idto);
        total_put += num_new;
    
        /* free up memory allocated for the remote new events */
        for (i=0; i < num_new; i++) {
         free(put[i]->pdata);
         free(put[i]);
        }
      }
       
      if (num_dump) {
        /* dump unused events into the "to" ET system */
        status = et_events_dump(id_to, att_to, dump, num_dump);
        if (status != ET_OK) {
          if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
            et_logmsg("ERROR", "et_events_bridge, error (status = %d) dumping unused events in \"to\" ET system\n",
                      status);
          }
          /* put back those read in last et_events_get call */
          et_events_put(id_from, att_from, get, num_read);
          goto end;
        }
      }
      
      num_new_prev = num_new;
    }
    
    /* put back events from the "from" ET system */
    status = et_events_put(id_from, att_from, get, num_read);
    if (status != ET_OK) {
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR)) {
        et_logmsg("ERROR", "et_events_bridge, error (status = %d) putting events into \"from\" ET system\n",
                  status);
      }
      goto end;
    }
  }
  
end:
  *ntransferred = total_put;
  free(get); free(dump); free(put); free(iov); free(header);
  return status;
}
