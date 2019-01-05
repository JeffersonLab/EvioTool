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
 *      Routines dealing with stations.
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>

#include "et_private.h"

/* prototypes */
static void station_remove(et_id *etid, et_stat_id stat_id);
static int  station_insert(et_id *etid, et_station *ps,
                           int position, int parallelposition);
static int  station_find(et_id *etid, et_station *ps,
                         int *position, int *parallelposition);


/*****************************************************
 * Remove a station from its place in the linked lists
 * of active and idle stations.
 *****************************************************/
static void station_remove(et_id *etid, et_stat_id stat_id)
{
  et_system  *sys = etid->sys;
  et_station *ps = etid->grandcentral + stat_id;
  et_station *previous, *next, *nextparallel;
    
  /* The only tricky part in removing a station is to remember that it may not
   * be in the main linked list if it is a parallel station. 
   */
  
  if (ps->config.flow_mode == ET_STATION_PARALLEL) {
    /* If it is the first station in a single group of parallel stations ... */
    if (ps->prev > -1) {
      /* If it's the only parallel station, remove it as you would any other */
      if (ps->nextparallel < 0) {
        /* previous station in linked list */
        previous = etid->grandcentral + ps->prev;
        /* if we're the tail ... */
        if (sys->stat_tail == stat_id) {
          sys->stat_tail = previous->num;
        }
        /* else adjust the next station in the list */
        else {
          next = etid->grandcentral + ps->next;
          next->prev = previous->num;
        }
        previous->next = ps->next;
      }
      
      /* Else if there's more than 1 parallel station in this group,
       * make the next parallel station take this one's place in the
       * main linked list.
       */
      else {
        previous     = etid->grandcentral + ps->prev;
        nextparallel = etid->grandcentral + ps->nextparallel;
        nextparallel->prev = ps->prev;
        nextparallel->next = ps->next;
        nextparallel->prevparallel = -1;
        /* if the station being removed is the tail ... */
        if (sys->stat_tail == stat_id) {
          sys->stat_tail = nextparallel->num;
        }
        /* else adjust the next station in the list */
        else {
          next = etid->grandcentral + ps->next;
          next->prev = ps->nextparallel;
        }
        previous->next = ps->nextparallel;
      }
    }
    
    /* Else it's not the first of the parallel stations ... */
    else {
      previous = etid->grandcentral + ps->prevparallel;
      /* If we're NOT the end of the parallel list, take care of the next guy */
      if (ps->nextparallel > -1) {
        next = etid->grandcentral + ps->nextparallel;
        next->prevparallel = previous->num;
      }
      previous->nextparallel = ps->nextparallel;
      /* If we're the last round-robin station to receive an event,
       * transfer that distinction to the previous station. This
       * adjustment is not necessary if ps is head of the parallel
       * stations since that station get the first event by default.
       */
      if (ps->waslast == 1) {
        previous->waslast = 1;
      }
    }
  }
  
  /* else if it is NOT a parallel station ... */
  else {  
    /* previous station in linked list */
    previous = etid->grandcentral + ps->prev;
    /* if we're the tail ... */
    if (sys->stat_tail == stat_id) {
      sys->stat_tail = previous->num;
    }
    /* else adjust the next station in the list */
    else {
      next = etid->grandcentral + ps->next;
      next->prev = previous->num;
    }
    previous->next = ps->next;
    
  }
    
  return;
}


/*****************************************************
 * Insert a station into its place in the linked lists
 * of active and idle stations.
 *****************************************************/
static int station_insert(et_id *etid, et_station *ps,
                           int position, int parallelposition)
{
  int            counter;
  et_station     *pnext=NULL, *parallelStation, *pstat, *previous;
  et_stat_id     next=0, nextparallel=0;
  et_system      *sys = etid->sys;
  
  /* if GRAND_CENTRAL is only existing station, add this station to end */
  if (sys->nstations < 3) {
    previous = etid->grandcentral;
    ps->prev = previous->num;
    sys->stat_tail = ps->num;
    previous->next = ps->num;
  }
  /* else, put the station in the desired position (2 or more stations exist) */
  else {
    counter = 1;

    while (1) {
      if (counter == 1) {
        previous = etid->grandcentral;
      }
      else {
        previous = etid->grandcentral + next;
      }
      next = previous->next;
      if (next > -1) {
        pnext = etid->grandcentral + next;
      }
/* printf("next = %d, pnext = %p\n", next, pnext); */
  
      /* if we reached the end of the linked list, put station on the end */
      if (next < 0) {
        ps->prev = previous->num;
        sys->stat_tail = ps->num;
        previous->next = ps->num;
        break;
      }
      
      /* else if this is the position to insert the new station ... */
      else if (counter++ == position) {
        
        /* If the station in "position" and this station are both parallel ... */
        if ((ps->config.flow_mode    == ET_STATION_PARALLEL) &&
            (pnext->config.flow_mode == ET_STATION_PARALLEL) &&
            (parallelposition != ET_NEWHEAD))  {
          
          /* If these 2 stations have incompatible definitions or we're trying to place
           * a parallel station in the first (already taken) spot of its group ... */
          if (et_station_compare_parallel(etid, &pnext->config, &ps->config) == 0) {
            if (etid->debug >= ET_DEBUG_ERROR) {
              et_logmsg("ERROR", "station_insert, trying to add incompatible parallel station\n");
            }
            return ET_ERROR;
          }
          else if (parallelposition == 0) {
            if (etid->debug >= ET_DEBUG_ERROR) {
              et_logmsg("ERROR", "station_insert, trying to add parallel station to head of existing parallel group\n");
            }
            return ET_ERROR;
          }

          /* Add this parallel station in the "parallelposition" slot in the
           * parallel linked list or to the end if parallelposition = ET_END.
           */
          counter = 1;
          while (1) {
            if (counter == 1) {
              parallelStation = pnext;
            }
            else {
              parallelStation = etid->grandcentral + nextparallel;
            }
            nextparallel = parallelStation->nextparallel;
            
            if ((counter++ == parallelposition) || (nextparallel < 0)) {
              ps->nextparallel = nextparallel;
              ps->prevparallel = parallelStation->num;
              if (nextparallel > -1) {
                pstat = etid->grandcentral + nextparallel;
                pstat->prevparallel = ps->num;
              }
               parallelStation->nextparallel = ps->num;
              return ET_OK;
            }
          }
        }
        else {
          ps->next = next;
          ps->prev = previous->num;
          pnext->prev = ps->num;
          previous->next = ps->num;
          break;
        }
        
      } /* if right place to insert station */
    } /* while(true) */
  } /* else if more than 1 existing station */

  return ET_OK;
}

/*****************************************************
 * Find the position a station occupies in the linked lists
 * of active and idle stations.
 *****************************************************/
static int station_find(et_id *etid, et_station *ps,
                           int *position, int *parallelposition)
{
  int            currentPosition, currentParallelPosition;
  et_station     *pnext, *parallelStation, *previous;
  et_stat_id     next=0, nextparallel=0;
  et_system      *sys = etid->sys;
  
  /* GRAND_CENTRAL is always easy to find */
  if (ps == etid->grandcentral) {
    *position = 0;
    *parallelposition = 0;
    return ET_OK;
  }
  
  /* Only GRAND_CENTRAL exists ... */
  if (sys->nstations < 2) {
    return ET_ERROR;
  }
  
  currentPosition = 1;

  while (1) {
    if (currentPosition == 1) {
      previous = etid->grandcentral;
    }
    else {
      previous = etid->grandcentral + next;
    }
    
    if ((next = previous->next) < 0) {
      return ET_ERROR;
    }
    pnext = etid->grandcentral + next;

    if (pnext == ps) {
      *position = currentPosition;
      *parallelposition = 0;
      return ET_OK;
    }

    /* If the station is parallel, check out it linked list ... */
    if (pnext->config.flow_mode == ET_STATION_PARALLEL)  {
      currentParallelPosition = 1;
      while (1) {
        if (currentParallelPosition == 1) {
          parallelStation = pnext;
        }
        else {
          parallelStation = etid->grandcentral + nextparallel;
        }
        
        if ((nextparallel = parallelStation->nextparallel) < 0) {
          break;
        }
        pnext = etid->grandcentral + nextparallel;
        
        if (pnext == ps) {
          *position = currentPosition;
          *parallelposition = currentParallelPosition;
          return ET_OK;
        }
        currentParallelPosition++;
      }
    }

    currentPosition++;
  }
}


/**
 * @defgroup station Stations
 *
 * These routines handle interactions with stations.
 *
 * @{
 */


/*****************************************************/
/*     STATION CREATE, REMOVE, ATTACH, DETACH        */
/*****************************************************/

/**
 * This routine creates a station provided that it does not already exist and the maximum number
 * of stations has not been reached. The station's status is set to @ref ET_STATION_IDLE. This changes
 * to @ref ET_STATION_ACTIVE when a process attaches to it. The ET system is immediately notified of
 * the new station upon creation and will transfer events in and out as soon as it is active.
 * It is placed at in the linked list of stations in the position given by the last argument
 * or at the end if that argument is larger than the existing number of stations. The station
 * may already exist, but if it has the same configuration, the id of the existing station is
 * returned.
 *
 * @param id                id of the ET system of interest.
 * @param stat_id           pointer to a station id number and returns the value of the newly created station's id.
 * @param stat_name         station name.
 * @param sconfig           station configuration.
 * @param position          position of the newly created station in the linked list of stations.
 *                          It may have the value @ref ET_END which specifies the end of that list.
 *                          It may not have the value of zero which is the first position and
 *                          reserved for @ref ET_GRANDCENTRAL station.
 * @param parallelposition  position of the newly created station in the linked list of 1 group
 *                          of parallel stations. It may have the value ET_END which specifies
 *                          the end of that group. It may also have the value of @ref ET_NEWHEAD which
 *                          specifies that a new group of parallel stations is being created
 *                          and this is its head. However, if the head already exists, it may
 *                          not have the value of zero, which is the first position, so that
 *                          any existing head is never moved.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if error.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_EXISTS  if station already exists and has a different configuration
 *                                (stat_id is still set to the existing station's id).
 * @returns @ref ET_ERROR_TOOMANY if the maximum number of stations already exist.
 * @returns @ref ET_ERROR_REMOTE  for a memory allocation error of a remote user.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
*/
int et_station_create_at(et_sys_id id, et_stat_id *stat_id, const char *stat_name,
                         et_statconfig sconfig, int position, int parallelposition) {

    int            i, status, isGrandCentral=0, this_station=-1;
    et_id          *etid = (et_id *) id;
    et_list        *pl;
    et_station     *ps;
    et_system      *sys = etid->sys;
    et_stat_config *sc;
    et_statconfig  p_auto_station;


    /* name check */
    if (stat_name == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, station name is NULL\n");
        }
        return ET_ERROR;
    }
    else if (strlen(stat_name) > ET_STATNAME_LENGTH - 1) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, station name is too long\n");
        }
        return ET_ERROR;
    }

    /* position check */
    if ((position != ET_END) && (position < 0)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, bad value for position\n");
        }
        return ET_ERROR;
    }

    /* parallelposition check */
    if ((parallelposition != ET_END) && (parallelposition != ET_NEWHEAD) && (parallelposition < 0)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, bad value for parallel position\n");
        }
        return ET_ERROR;
    }

    /* supply configuration if none given */
    if (sconfig == NULL) {
        /* automatically create, use and destroy a default stn. */
        if (et_station_config_init(&p_auto_station) == ET_ERROR) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_station_create: NULL arg for sconfig but cannot use default.\n");
                et_logmsg("ERROR", "                   must be out of memory!\n");
            }
            return ET_ERROR;
        }
        sconfig = p_auto_station;
    } else {
        p_auto_station = NULL;
    }

    sc = (et_stat_config *) sconfig;

    /* check to see if the configuration was properly initialized */
    if (sc->init != ET_STRUCT_OK) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, station configuration not properly initialized\n");
        }
        return ET_ERROR;
    }

    /* if it's GrandCentral ... */
    if (strcmp("GRAND_CENTRAL", stat_name) == 0) {
        isGrandCentral = 1;
    }
    else if (position == 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, only GrandCentral may occupy position 0\n");
        }
        if (p_auto_station) {
            et_station_config_destroy(p_auto_station);
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_create_at(id, stat_id, stat_name, sconfig, position, parallelposition);
    }

    /*
    * Don't create station if ET is dead, but need to make an
    * exception for GRAND_CENTRAL as it must be created before
    * the ET system heartbeat is started.
    */
    if (!et_alive(id) && (isGrandCentral == 0)) {
        if (p_auto_station) {
            et_station_config_destroy(p_auto_station);
        }
        return ET_ERROR_DEAD;
    }

    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    /* Check configuration for self-consistancy. Do it after calling remote routine
     * since it's gotta look at shared memory. */
    if (et_station_config_check(etid, sc) == ET_ERROR) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, station configuration is not self-consistant\n");
        }
        if (p_auto_station) {
            et_station_config_destroy(p_auto_station);
        }
        return ET_ERROR;
    }

    et_station_lock(sys);

    /* see if station already exists, if so, stat_id = existing station */
    if (et_station_exists(id, stat_id, stat_name) == 1) {
        /* If the station we want to create is in the process of being created,
         * then return an error. Else look to see if its definition is the same
         * as the station we want to create. If it is, return ET_OK.
         */
        et_station_getstatus(id, *stat_id, &status);
        if (status != ET_STATION_CREATING) {
            ps = etid->grandcentral + *stat_id;
            /* see if station configurations are identical */
            if ((ps->config.flow_mode    == sc->flow_mode) &&
                (ps->config.user_mode    == sc->user_mode) &&
                (ps->config.block_mode   == sc->block_mode) &&
                (ps->config.select_mode  == sc->select_mode) &&
                (ps->config.restore_mode == sc->restore_mode) &&
                (ps->config.prescale     == sc->prescale) &&
                (ps->config.cue          == sc->cue)) {

                int equal=1;
                for (i=0 ; i < ET_STATION_SELECT_INTS ; i++) {
                    equal = equal && (ps->config.select[i] == sc->select[i]);
                }
                if (ps->config.select_mode == ET_STATION_SELECT_USER) {
                    equal = 0;
                }

                if (equal == 1) {
                    /* station definitions are the same, so return ET_OK */
                    et_station_unlock(sys);
                    et_mem_unlock(etid);
                    if (p_auto_station) {
                        et_station_config_destroy(p_auto_station);
                    }
                    return ET_OK;
                }
            }
        }
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, station \"%s\" already exists and is defined differently, has a user-defined selection function, or is being created\n",stat_name);
        }
        if (p_auto_station) {
            et_station_config_destroy(p_auto_station);
        }
        return ET_ERROR_EXISTS;
    }

    if (sys->nstations >= sys->config.nstations) {
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_create_at, too many stations already, cannot create\n",stat_name);
        }
        if (p_auto_station) {
            et_station_config_destroy(p_auto_station);
        }
        return ET_ERROR_TOOMANY;
    }
    sys->nstations++;

    ps = etid->grandcentral;
    for (i=0 ; i < sys->config.nstations ; i++) {
        /* printf("et_station_create: ps = %p, i=%d, status = %d\n", ps, i, ps->data.status);*/
        if (ps->data.status == ET_STATION_UNUSED) {
            this_station = i;
            break;
        }
        ps++;
    }

    if (this_station == -1) {
        /* this should never happen */
        sys->nstations--;
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_SEVERE) {
            et_logmsg("SEVERE", "et_station_create_at, algorithm problem\n");
        }
        if (p_auto_station) {
            et_station_config_destroy(p_auto_station);
        }
        return ET_ERROR;
    }

    /* set station structure elements */
    et_init_station(ps);
    ps->num = this_station;
    strcpy(ps->name, stat_name);
    ps->config = *sc;
    ps->data.pid_create = getpid();

    /* done with configurations */
    if (p_auto_station) {
        et_station_config_destroy(p_auto_station);
    }

    /*
     * Can set status w/o grabbing transfer mutexes since this station
     * is not in the linked list of active stations and isn't getting
     * or putting events yet.
     */
    ps->data.status = ET_STATION_CREATING;

    if (isGrandCentral) {
        /* sys->stat_head = sys->stat_tail = ET_GRANDCENTRAL; */
        et_station_unlock(sys);
        et_mem_unlock(etid);
        return ET_OK;
    }

    /*
     * Only get addstat thread to spawn conductor thread if we're
     * not grandcentral station (which is created by the function
     * et_grandcentral_station_create).
     */

    /*
     * Grab mutex the station creation thread uses to wake up
     * so it cannot wake up yet.
     */
    status = pthread_mutex_lock(&sys->statadd_mutex);
    if (status != 0) {
        err_abort(status, "Failed add station lock");
    }

    /* Signal station creation thread to add one more */
    status = pthread_cond_signal(&sys->statadd);
    if (status != 0) {
        err_abort(status, "Signal add station");
    }

    /*
     * Release mutex the station creation thread uses to wake up
     * and simultaneously wait for the process to finish. It finishes
     * when the conductor thread is started, which in turn wakes us
     * up with the "statdone" condition variable.
     */
    status = pthread_cond_wait(&sys->statdone, &sys->statadd_mutex);
    if (status != 0) {
        err_abort(status, "Wait for station addition");
    }

    status = pthread_mutex_unlock(&sys->statadd_mutex);
    if (status != 0) {
        err_abort(status, "Failed add station mutex unlock");
    }

    /* if the conductor was NOT successful in starting, abort */
    if (ps->data.status != ET_STATION_IDLE) {
        sys->nstations--;
        et_init_station(ps);
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_SEVERE) {
            et_logmsg("SEVERE", "et_station_create_at, CANNOT start conductor, remove station %s\n", stat_name);
        }
        return ET_ERROR;
    }

    /* mutex protect changes to station linked list / turn off event transfers */
    et_transfer_lock_all(etid);

    /* insert station into linked list(s) */
    if (station_insert(etid, ps, position, parallelposition) != ET_OK) {
        et_transfer_unlock_all(etid);
        /* since we already started the conductor, we have to kill it */
        ps->conductor = ET_THREAD_KILL;
        pl = &ps->list_out;
        status = pthread_cond_signal(&pl->cread);
        if (status != 0) {
            err_abort(status, "Wake up & die");
        }
        ps->data.status = ET_STATION_UNUSED;
        sys->nstations--;
        et_station_unlock(sys);
        et_mem_unlock(etid);
        return ET_ERROR;
    }

    et_transfer_unlock_all(etid);
    et_station_unlock(sys);
    et_mem_unlock(etid);

    /* set value to be returned */
    *stat_id = this_station;

    if (etid->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_station_create_at, created station %s\n",ps->name);
    }
    return ET_OK;
}

/**
 * This routine creates a station provided that it does not already exist and the maximum number
 * of stations has not been reached. The station's status is set to @ref ET_STATION_IDLE. This changes
 * to @ref ET_STATION_ACTIVE when a process attaches to it. The ET system is immediately notified of
 * the new station upon creation and will transfer events in and out as soon as it is active.
 * It is placed at the end of the linked list of stations. The station
 * may already exist, but if it has the same configuration, the id of the existing station is
 * returned.
 *
 * @param id                id of the ET system.
 * @param stat_id           pointer to a station id number and returns the value of the newly created station's id.
 * @param stat_name         station name.
 * @param sconfig           station configuration.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if error.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_EXISTS  if station already exists and has a different configuration
 *                                (stat_id is still set to the existing station's id).
 * @returns @ref ET_ERROR_TOOMANY if the maximum number of stations already exist.
 * @returns @ref ET_ERROR_REMOTE  for a memory allocation error of a remote user.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_create(et_sys_id id, et_stat_id *stat_id, const char *stat_name,
                      et_statconfig sconfig) {
    return et_station_create_at(id, stat_id, stat_name, sconfig, ET_END, ET_END);
}

/**
 * This routine deletes a station provided it is not GRAND_CENTRAL and provided there are
 * no attached processes. The station's status is set to @ref ET_STATION_UNUSED.
 *
 * @param id       id of the ET system.
 * @param stat_id  station id.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if error.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_remove(et_sys_id id, et_stat_id stat_id) {

    int         status;
    et_list    *pl;
    et_id      *etid = (et_id *) id;
    et_system  *sys = etid->sys;
    et_station *ps = etid->grandcentral + stat_id;
    struct timespec waitforme;

    if (stat_id < 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_remove, bad station id\n");
        }
        return ET_ERROR;
    }
    else if (stat_id == ET_GRANDCENTRAL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_remove, may not remove grandcentral station\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_remove(id, stat_id);
    }

    /* time to wait for remaining events to transfer out of station */
    waitforme.tv_sec  = 0;
    waitforme.tv_nsec = 500000000; /* 0.5 sec */

    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }

    /* Protection from (local) et_close() unmapping shared memory */
    et_memRead_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_remove, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if (stat_id >= sys->config.nstations) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_remove, bad station id\n");
        }
        return ET_ERROR;
    }

    /* grab mutex since attaching process may activate station */
    et_station_lock(sys);

    /* only remove if no attached processes */
    if (ps->data.nattachments > 0) {
        et_station_unlock(sys);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_remove, can't remove %s - still have attachments\n", ps->name);
        }
        et_mem_unlock(etid);
        return ET_ERROR;
    }
    /* since no attached processes, station is idle with no events */

    /* stop all transfers of events by conductors */
    et_transfer_lock_all(etid);

    /* take the station out of the linked lists */
    station_remove(etid, stat_id);

    /*
     * Kill the conductor thread associated with this station,
     * but first check for events still in output list. This could
     * happen if this routine is called before, after or while the
     * conductor is getting its last batch of events but before it
     * could grab the transfer mutex. It's possible for "cnt" be either
     * zero or nonzero depending on whether the conductor finished reading
     * the events or not. In either case, give it a chance to finish.
     */

    /* allow conductor to work */
    et_transfer_unlock(ps);
    /* yield processor to another thread */
    sched_yield();
    pl = &ps->list_out;
    /* if there are still events in output list ... */
    if (pl->cnt > 0) {
        if (etid->debug >= ET_DEBUG_INFO) {
            et_logmsg("INFO", "et_station_remove, station has %d events left\n", pl->cnt);
        }
        /* Give the conductor (which has been/is being woken up) time to
         * transfer its events down the chain before setting ps->conductor to
         * ET_THREAD_KILL - thereby losing the events from the system forever.
         */
        nanosleep(&waitforme, NULL);
    }
    et_transfer_lock(ps);

    /* wake up conductor thread , it'll kill itself */
    ps->conductor = ET_THREAD_KILL;
    status = pthread_cond_signal(&pl->cread);
    if (status != 0) {
        err_abort(status, "Wake up & die");
    }

    ps->data.status = ET_STATION_UNUSED;
    et_transfer_unlock_all(etid);

    sys->nstations--;
    et_station_unlock(sys);
    et_mem_unlock(etid);

    if (etid->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_station_remove, ps = %p, status = ET_STATION_UNUSED\n", ps);
    }

    return ET_OK;
}

/**
 * This routine will attach the user to a station. This means that the user is free to read
 * and write events from that station or to request new events. It returns a unique
 * attachment id in the second argument which is to be used in all transactions with the
 * station.
 * When a user process attaches to a station, it is marked as an active station,
 * which means it will start receiving events. To remove an attachment, call the
 * routine @ref et_station_detach().
 *
 * @param id       id of the ET system.
 * @param stat_id  station id.
 * @param att      pointer to a attachment id which gets filled in by this routine.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if error.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 * @returns @ref ET_ERROR_TOOMANY if the existing number of attachments is already equal
 *                                to the station or system limit.
 * @returns @ref ET_ERROR_REMOTE  for a memory allocation error of a remote user
 */
int et_station_attach(et_sys_id id, et_stat_id stat_id, et_att_id *att) {

    int       i, my_index = -1, unlockit=0;
    et_id    *etid = (et_id *) id;
    et_system *sys = etid->sys;
    et_station *ps = etid->grandcentral + stat_id;

    if (stat_id < 0 || att == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_attach, bad id or att arg\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_attach(id, stat_id, att);
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
            et_logmsg("ERROR", "et_station_attach, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if (stat_id >= sys->config.nstations) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_attach, bad station id\n");
        }
        return ET_ERROR;
    }

    /* read/modify station data */
    et_station_lock(sys);

    /* attaching process to active or idle station ? */
    if ((ps->data.status != ET_STATION_ACTIVE) && (ps->data.status != ET_STATION_IDLE)) {
        et_station_unlock(sys);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_attach, station %s is not active or idle\n", ps->name);
        }
        et_mem_unlock(etid);
        return ET_ERROR;
    }

    /* limit # of station's attachments */
    if ((ps->config.user_mode > 0) &&
        (ps->config.user_mode <= ps->data.nattachments)) {
        et_station_unlock(sys);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_attach, too many attachments to station %s\n", ps->name);
        }
        et_mem_unlock(etid);
        return ET_ERROR_TOOMANY;
    }

    /* modify system data, now we have TWO mutexes */
    et_system_lock(sys);

    /* don't exceed max # of attachments to system as a whole */
    if (sys->nattachments >= sys->config.nattachments) {
        et_system_unlock(sys);
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_attach, too many attachments to ET system\n");
        }
        return ET_ERROR_TOOMANY;
    }

    for (i=0; i < sys->config.nattachments ; i++) {
        if (sys->attach[i].num == -1) {
            my_index = i;
            break;
        }
    }

    if (my_index < 0) {
        et_system_unlock(sys);
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_SEVERE) {
            et_logmsg("SEVERE", "et_station_attach, algorithm problem\n");
        }
        return ET_ERROR;
    }

    sys->nattachments++;
    et_init_attachment(sys, my_index);
    sys->attach[my_index].num    = my_index;
    sys->attach[my_index].proc   = etid->proc;
    sys->attach[my_index].stat   = stat_id;
    sys->attach[my_index].status = ET_ATT_ACTIVE;
    /*
     * Record pid & host this attachment belongs to. If this routine is
     * being executed by the ET system server thread, it will overwrite
     * these values with the true (remote) values.
     */
    sys->attach[my_index].pid    = getpid();
    if (etNetLocalHost(sys->attach[my_index].host, ET_MAXHOSTNAMELEN) != ET_OK) {
        if (etid->debug >= ET_DEBUG_WARN) {
            et_logmsg("WARN", "et_station_attach: cannot find hostname\n");
        }
    }
    /* While remote attachments are tracked, "proc" refers to only local
     * user processes (on Solaris), and remote processes are ignored.
     * In addition, the system process is also ignored and its
     * etid->proc = ET_SYS. So make sure we take account of that here.
     */
    if (etid->proc != ET_SYS) {
        sys->proc[etid->proc].nattachments++;
        sys->proc[etid->proc].att[my_index] = my_index;
    }
    et_system_unlock(sys);

    ps->data.nattachments++;
    ps->data.att[my_index] = my_index;
    /* Stopping event transfer to add an attachment is necessary for
     * parallel stations to be activated safely.
     */
    if ((ps->config.flow_mode == ET_STATION_PARALLEL) &&
        (ps->data.status != ET_STATION_ACTIVE)) {
        unlockit = 1;
        et_transfer_lock_all(etid);
    }
    ps->data.status = ET_STATION_ACTIVE;
    if (unlockit) {
        et_transfer_unlock_all(etid);
    }
    et_station_unlock(sys);
    et_mem_unlock(etid);

    *att = my_index;

    if (etid->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_station_attach, done\n");
    }
    return ET_OK;
}


/**
 * This routine will detach the user from a station. This means that the user is
 * no longer able to read or write events from that station. It undoes what
 * @ref et_station_attach() does.
 *
 * If this routine detaches the last attachment to a station, it marks the station as idle.
 * In other words, the station stops receiving events since no one is there to read them.
 * All events remaining in the station's input list (after the detachment) will be moved
 * to the output list and sent to other stations. One must detach all attachments to a
 * station before the station can be removed.
 *
 * @param id       id of the ET system.
 * @param att      attachment id.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if bad attachment id or not attached to a station.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_detach(et_sys_id id, et_att_id att) {

    et_id      *etid = (et_id *) id;
    et_system  *sys = etid->sys;
    et_station *ps;
    et_stat_id  stat_id;

    if ((att < 0) || (att >= ET_ATTACHMENTS_MAX)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_detach, bad attachment id\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_detach(id, att);
    }

    if (!et_alive(id)) {
        return ET_ERROR_DEAD;
    }

    /* Protection from (local) et_close() unmapping shared memory.
     * Make it grab the write as opposed to read lock as the effect of detaching
     * is similar to the effect of closing on et_events_get/put/new/dump. */
    et_memWrite_lock(etid);

    /* Has caller already called et_close()? */
    if (etid->closed) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_dettach, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if (att >= sys->config.nattachments) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_detach, bad attachment id\n");
        }
        return ET_ERROR;
    }

    /* don't lock if doing ET system repair/cleanup since already locked */
    if (etid->cleanup != 1) {
        et_station_lock(sys);
    }

    /* see which station we're attached to */
    if (etid->sys->attach[att].stat < 0) {
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_detach, not attached any station!\n");
        }
        return ET_ERROR;
    }
    stat_id = etid->sys->attach[att].stat;
    ps = etid->grandcentral + stat_id;

    /* now we have 2 mutexes */
    if (etid->cleanup != 1) {
        et_system_lock(sys);
    }

    /* set .num first so if we crash after this, the et_fix_attachments
     * routine will not falsely think that we crashed in
     * et_station_attach
     */
    sys->attach[att].num = -1;
    sys->attach[att].status = ET_ATT_UNUSED;
    sys->nattachments--;

    /* if this is station's (not gc's) last attachment, mark station inactive */
    if ((ps->data.nattachments == 1) && (stat_id != ET_GRANDCENTRAL)) {
        /* to change station's status, grab mutexes for event transfer */
        /* this may not be necessary, 1/11/2001 */
        if (etid->cleanup != 1) {
            et_transfer_lock_all(etid);
        }
        ps->data.status = ET_STATION_IDLE;
        if (etid->cleanup != 1) {
            et_transfer_unlock_all(etid);
        }
        et_flush_events(etid, att, stat_id);
        if (etid->debug >= ET_DEBUG_INFO) {
            et_logmsg("INFO", "et_station_detach, make station \"%s\" (%p) idle\n", ps->name, ps);
        }
    }

    /* Events read but never written are returned to system.    */
    /* Can transfer events here without grabbing transfer locks */
    /* since only GC or this station is involved and they will  */
    /* both be active (or idle) at this point.                  */
    if (et_restore_events(etid, att, stat_id) != ET_OK) {
        if (etid->debug >= ET_DEBUG_WARN) {
            et_logmsg("WARN", "et_station_detach, error recovering detached process's events, some events may be permanently lost!\n");
        }
    }

    /* remove attachment's id from station's list of attachments */
    ps->data.nattachments--;
    ps->data.att[att] = -1;

    /* next block is only valid for users or ET server, but NOT ET system cleanup */
    if (etid->cleanup != 1) {
        /* ET server has no proc data stored */
        if (etid->proc != ET_SYS) {
            sys->proc[etid->proc].nattachments--;
            sys->proc[etid->proc].att[att] = -1;
        }
        et_system_unlock(sys);
        et_station_unlock(sys);
    }

    et_mem_unlock(etid);

    if (etid->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_station_detach, done\n");
    }
    return ET_OK;
}


/**
 * This routine sets a station's position in the linked list of active and idle stations
 * and its position within a group of parallel stations if it is one.
 *
 * @param id                id of the ET system.
 * @param stat_id           station id number. GRAND_CENTRAL station's position may not be changed.
 * @param position          position in the linked list of stations and must be greater than 0 as the
 *                          first position (0) is reserved for GRAND_CENTRAL station. It may be @ref ET_END
 *                          to put it at the end of the list.
 * @param parallelposition  position within the group of a linked list of stations if it is one.
 *                          It may be @ref ET_NEWHEAD to make it the head of a new group of parallel stations.
 *                          It may be ET_END to put it at the end.  If the position parameter refers to an
 *                          existing parallel station, this may be any value > 0 as it may not take the place
 *                          of an existing head. Its configuration must also be compatible with existing
 *                          parallel stations in the group.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is GRAND_CENTRAL, if bad stat_id, or bad position arguments values
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_setposition(et_sys_id id, et_stat_id stat_id, int position, int parallelposition) {

    int        err, currentPosition, currentParallelPosition;
    et_id      *etid = (et_id *) id;
    et_system  *sys  = etid->sys;
    et_station *ps   = etid->grandcentral + stat_id;

    if (stat_id < 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setposition, bad station id\n");
        }
        return ET_ERROR;
    }

    /* may not move grandcentral */
    if (stat_id == ET_GRANDCENTRAL) {
        if (position == 0) {
            return ET_OK;
        }
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setposition, may not change GrandCentral's position\n");
        }
        return ET_ERROR;
    }

    if ((position != ET_END) && (position < 0)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setposition, bad position number\n");
        }
        return ET_ERROR;
    }
    else if (position == 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setposition, only GrandCentral may occupy position 0\n");
        }
        return ET_ERROR;
    }

    /* parallelposition check */
    if ((parallelposition != ET_END) && (parallelposition != ET_NEWHEAD) && (parallelposition < 0)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setposition, bad value for parallel position\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_setposition(id, stat_id, position, parallelposition);
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
            et_logmsg("ERROR", "et_station_setposition, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if (stat_id >= sys->config.nstations) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setposition, bad station id\n");
        }
        return ET_ERROR;
    }

    /* grab station lock otherwise station status may change */
    et_station_lock(sys);

    /* check to see if this station is even in the linked list */
    if ((ps->data.status != ET_STATION_IDLE) &&
        (ps->data.status != ET_STATION_ACTIVE)) {
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setposition, station not defined or being created\n");
        }
        return ET_ERROR;
    }

    /* stop event transfers by conductors, allow modifying linked list */
    et_transfer_lock_all(etid);

    /* find station's current place */
    station_find(etid, ps, &currentPosition, &currentParallelPosition);

    /* remove station from its current place */
    station_remove(etid, stat_id);

    /* insert the station into its new place */
    err = station_insert(etid, ps, position, parallelposition);

    /* If there's an error, try to put it back in its original place */
    if (err != ET_OK) {
        station_insert(etid, ps, currentPosition, currentParallelPosition);
    }

    et_transfer_unlock_all(etid);
    et_station_unlock(sys);
    et_mem_unlock(etid);

    return err;
}


/**
 * This routine gets a station's position in the linked list of active and idle stations
 * and its position within a group of parallel stations if it is one.
 *
 * @param id                id of the ET system.
 * @param stat_id           station id number. GRAND_CENTRAL station's position may not be changed.
 * @param position          pointer to int which gets filled with its position in the linked list of stations.
 * @param parallelposition  pointer to int which gets filled with its position within the group parallel stations.
 *                          if it is one.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getposition(et_sys_id id, et_stat_id stat_id, int *position, int *parallelposition) {

    int        err;
    et_id      *etid = (et_id *) id;
    et_system  *sys  = etid->sys;
    et_station *ps   = etid->grandcentral + stat_id;

    if (stat_id < 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getposition, bad station id\n");
        }
        return ET_ERROR;
    }

    /* grandcentral is always at position 0 */
    if (stat_id == ET_GRANDCENTRAL) {
        if (position != NULL) {
            *position = 0;
        }
        return ET_OK;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_getposition(id, stat_id, position, parallelposition);
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
            et_logmsg("ERROR", "et_station_getposition, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if (stat_id >= sys->config.nstations) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getposition, bad station id\n");
        }
        return ET_ERROR;
    }

    /* grab station lock otherwise station status may change */
    et_station_lock(sys);

    /* check to see if this station is even in the linked list */
    if ((ps->data.status != ET_STATION_IDLE) &&
        (ps->data.status != ET_STATION_ACTIVE)) {
        et_station_unlock(sys);
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getposition, station not defined or being created\n");
        }
        return ET_ERROR;
    }

    /* stop event transfers by conductors, access unmodified linked list */
    et_transfer_lock_all(etid);

    /* find station's current position */
    err = station_find(etid, ps, position, parallelposition);

    et_transfer_unlock_all(etid);
    et_station_unlock(sys);
    et_mem_unlock(etid);

    return err;
}


/*---------------------------------------------------*/
/*                STATION INFORMATION                */
/*---------------------------------------------------*/


/**
 * Is an attachment attached to a particular station?
 *
 * @param id       ET system id.
 * @param stat_id  station id.
 * @param att      attachment id.
 *
 * @returns 1                     if attached.
 * @returns 0                     if not attached.
 * @returns @ref ET_ERROR         for bad argument(s).
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_isattached(et_sys_id id, et_stat_id stat_id, et_att_id att) {

    et_id      *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_isattached(id, stat_id, att);
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
            et_logmsg("ERROR", "et_station_isattached, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_isattached, bad station id\n");
        }
        return ET_ERROR;
    }

    if ((att < 0) || (att >= etid->sys->config.nattachments)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_isattached, bad attachment id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.att[att] == att) {
        et_mem_unlock(etid);
        return 1;
    }

    et_mem_unlock(etid);
    return 0;
}


/**
 * This routine tells whether a station exists or not, given the name of a station,
 * If it does, it gives its id.
 *
 * @param id         ET system id.
 * @param stat_id    pointer to station id which gets filled if station exists.
 * @param stat_name  station name.
 *
 * @returns 1                     if station exists.
 * @returns 0                     if station does not exist.
 * @returns @ref ET_ERROR         stat_name argument is NULL.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 * @returns @ref ET_ERROR_REMOTE  for a memory allocation error of a remote user
 */
int et_station_exists(et_sys_id id, et_stat_id *stat_id, const char *stat_name) {

    int         num;
    et_id      *etid = (et_id *) id;
    et_station *ps = etid->grandcentral;

    /* name check */
    if (stat_name == NULL) {
        return ET_ERROR;
    }

    if (etid->locality == ET_REMOTE) {
        return etr_station_exists(id, stat_id, stat_name);
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
            et_logmsg("ERROR", "et_station_exists, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    for (num=0; num < etid->sys->config.nstations ; num++) {
        if (ps->data.status != ET_STATION_UNUSED) {
            if (strcmp(ps->name, stat_name) == 0) {
                if (stat_id != NULL) {
                    *stat_id = num;
                }
                et_mem_unlock(etid);
                return 1;
            }
        }
        ps++;
    }

    et_mem_unlock(etid);
    return 0;
}


/**
 * This routine gives a station id, given the name of a station,
 *
 * @param id         ET system id.
 * @param stat_id    pointer to station id which gets filled if station exists.
 * @param stat_name  station name.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if no station by that name exists or stat_name is NULL.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 * @returns @ref ET_ERROR_REMOTE  for a memory allocation error of a remote user
 */
int et_station_name_to_id(et_sys_id id, et_stat_id *stat_id, const char *stat_name) {

    int status;

    status = et_station_exists(id, stat_id, stat_name);
    if (status < 0) {
        return status;
    }
    else if (status == 1) {
        return ET_OK;
    }

    return ET_ERROR;
}


/**
 * This routine gets the number of attachments to the given station.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param numatts    int pointer which gets filled with the number of attachments.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getattachments(et_sys_id id, et_stat_id stat_id, int *numatts) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getattachments(id, stat_id, numatts);
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
            et_logmsg("ERROR", "et_station_getattachments, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getattachments, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getattachments, station is unused\n");
        }
        return ET_ERROR;
    }

    if (numatts != NULL) {
        *numatts = ps->data.nattachments;
    }

    et_mem_unlock(etid);
    return ET_OK;
}

/**
 * This routine gets the status of a station which may be one of the following values.
 * <ul>
 * <li>@ref ET_STATION_UNUSED - station structure or id is not being used at all.</li>
 * <li>@ref ET_STATION_CREATING - station is in the process of being created.</li>
 * <li>@ref ET_STATION_ACTIVE - station has been created and has attachments.</li>
 * <li>@ref ET_STATION_IDLE - station has been created but has no attachments.</li>
 * </ul>
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param status     int pointer which gets filled with the station status which may be
 *                   ET_STATION_UNUSED, ET_STATION_CREATING, ET_STATION_IDLE, or ET_STATION_ACTIVE.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getstatus(et_sys_id id, et_stat_id stat_id, int *status) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getstatus(id, stat_id, status);
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
            et_logmsg("ERROR", "et_station_getstatus, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getstatus, bad station id\n");
        }
        return ET_ERROR;
    }


    if (status != NULL) {
        *status = ps->data.status;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the number of events in a station's input list.
 * This number changes rapidly and is likely to be out-of-date immediately.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param cnt        int pointer which gets filled with the number of events in the input list.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getinputcount(et_sys_id id, et_stat_id stat_id, int *cnt) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getinputcount(id, stat_id, cnt);
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
            et_logmsg("ERROR", "et_station_getinputcount, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getinputcount, bad station id\n");
        }
        return ET_ERROR;
    }

    if (cnt != NULL) {
        *cnt = ps->list_in.cnt;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the number of events in a station's output list.
 * This number changes rapidly and is likely to be out-of-date immediately.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param cnt        int pointer which gets filled with the number of events in the output list.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getoutputcount(et_sys_id id, et_stat_id stat_id, int *cnt) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getoutputcount(id, stat_id, cnt);
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
            et_logmsg("ERROR", "et_station_getoutputcount, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getoutputcount, bad station id\n");
        }
        return ET_ERROR;
    }

    if (cnt != NULL) {
        *cnt = ps->list_out.cnt;
    }

    et_mem_unlock(etid);
    return ET_OK;
}

/*----------------------------------------------------*/
/*    STATION INFORMATION & DYNAMIC CONFIGURATION     */
/*----------------------------------------------------*/

/**
 * This routine gets the block mode of a station which may be
 * @ref ET_STATION_BLOCKING or @ref ET_STATION_NONBLOCKING.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param block      int pointer which gets filled with the station block mode which may be
 *                   ET_STATION_BLOCKING, or ET_STATION_NONBLOCKING.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 *
 * @see et_station_setblock()
 */
int et_station_getblock(et_sys_id id, et_stat_id stat_id, int *block) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getblock(id, stat_id, block);
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
            et_logmsg("ERROR", "et_station_getblock, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getblock, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getblock, station is unused\n");
        }
        return ET_ERROR;
    }

    if (block != NULL) {
        *block = ps->config.block_mode;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine sets a station's block mode.
 *
 * Determines whether all events pass through the station (@ref ET_STATION_BLOCKING)
 * or whether only a fixed size cue of events is filled while
 * all others by-pass the station (@ref ET_STATION_NONBLOCKING).
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param block      station block mode which may be
 *                   ET_STATION_BLOCKING, or ET_STATION_NONBLOCKING.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is GRAND_CENTRAL, bad stat_id, or bad block argument value
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_setblock(et_sys_id id, et_stat_id stat_id, int block) {

    et_id      *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (stat_id == ET_GRANDCENTRAL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setblock, may not modify grandcentral station\n");
        }
        return ET_ERROR;
    }

    if ((block != ET_STATION_BLOCKING) &&
        (block != ET_STATION_NONBLOCKING)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setblock, bad block mode value\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_setblock(id, stat_id, block);
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
            et_logmsg("ERROR", "et_station_setblock, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setblock, bad station id\n");
        }
        return ET_ERROR;
    }

    if (((ps->config.select_mode == ET_STATION_SELECT_RROBIN) ||
         (ps->config.select_mode == ET_STATION_SELECT_EQUALCUE)) &&
        (block ==  ET_STATION_NONBLOCKING)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setblock, cannot set rrobin or equalcue station to nonblocking\n");
        }
        return ET_ERROR;
    }

    et_station_lock(etid->sys);
    /* et_transfer_lock_all(etid); */
    ps->config.block_mode = block;
    /* et_transfer_unlock_all(etid); */
    et_station_unlock(etid->sys);
    et_mem_unlock(etid);

    return ET_OK;
}


/**
 * This routine gets a station's user mode which is the maximum number of
 * simultaneous attachments allowed with 0 meaning multiple.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param user       int pointer which gets filled with max number of allowed simultaneous attachments
 *                   where 0 (@ref ET_STATION_USER_MULTI) means multiple.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 *
 * @see et_station_setuser()
 */
int et_station_getuser(et_sys_id id, et_stat_id stat_id, int *user) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getuser(id, stat_id, user);
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
            et_logmsg("ERROR", "et_station_getuser, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getuser, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getuser, station is unused\n");
        }
        return ET_ERROR;
    }

    if (user != NULL) {
        *user = ps->config.user_mode;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine sets a station's user mode which is the
 * maximum number of simultaneous attachments allowed to a station.
 *
 * Value may be @ref ET_STATION_USER_SINGLE for only 1 user, or
 * @ref ET_STATION_USER_MULTI for multiple users, or may be set to
 * a specific positive integer for only that many simultaneous attachments.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param user       ET_STATION_USER_MULTI, ET_STATION_USER_SINGLE or a positive integer.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is GRAND_CENTRAL, bad stat_id or bad user argument value,
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_setuser(et_sys_id id, et_stat_id stat_id, int user) {

    et_id      *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (stat_id == ET_GRANDCENTRAL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setuser, may not modify grandcentral station\n");
        }
        return ET_ERROR;
    }

    if (user < 0) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setuser, bad user mode value\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_setuser(id, stat_id, user);
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
            et_logmsg("ERROR", "et_station_setuser, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setuser, bad station id\n");
        }
        return ET_ERROR;
    }

    et_station_lock(etid->sys);
    ps->config.user_mode = user;
    et_station_unlock(etid->sys);
    et_mem_unlock(etid);

    return ET_OK;
}

/**
 * This routine gets a station's restore mode which determines where events
 * go when the attachment that owns those events detaches from a station.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param restore    int pointer which gets filled with @ref ET_STATION_RESTORE_OUT,
 *                   @ref ET_STATION_RESTORE_IN, @ref ET_STATION_RESTORE_GC, or @ref ET_STATION_RESTORE_REDIST.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 *
 * @see et_station_setrestore()
 */
int et_station_getrestore(et_sys_id id, et_stat_id stat_id, int *restore) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getrestore(id, stat_id, restore);
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
            et_logmsg("ERROR", "et_station_getrestore, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getrestore, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getrestore, station is unused\n");
        }
        return ET_ERROR;
    }

    if (restore != NULL) {
        *restore = ps->config.restore_mode;
    }

    et_mem_unlock(etid);
    return ET_OK;
}

/**
 * This routine sets a station's restore mode which determines where events
 * go when the attachment that owns those events detaches from a station.
 *
 * The value may be @ref ET_STATION_RESTORE_OUT which places events in this station's output list,
 * The value may be @ref ET_STATION_RESTORE_IN which places events in this station's input list -
 * unless there are no more attachments in which case they go to the output list.
 * The value may be @ref ET_STATION_RESTORE_GC which places events in Grand Central's input list.
 * The value may be @ref ET_STATION_RESTORE_REDIST which places events in the previous station's
 * output list for redistribution among a group of parallel stations.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param restore    ET_STATION_RESTORE_OUT, ET_STATION_RESTORE_IN, ET_STATION_RESTORE_GC,
 *                   or ET_STATION_RESTORE_REDIST.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is GRAND_CENTRAL, bad stat_id, or bad restore argument value
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_setrestore(et_sys_id id, et_stat_id stat_id, int restore) {

    et_id      *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (stat_id == ET_GRANDCENTRAL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setrestore, may not modify grandcentral station\n");
        }
        return ET_ERROR;
    }

    if ((restore != ET_STATION_RESTORE_OUT) &&
        (restore != ET_STATION_RESTORE_IN)  &&
        (restore != ET_STATION_RESTORE_GC))   {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setrestore, bad restore mode value\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_setrestore(id, stat_id, restore);
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
            et_logmsg("ERROR", "et_station_setrestore, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setrestore, bad station id\n");
        }
        return ET_ERROR;
    }

    if (((ps->config.select_mode == ET_STATION_SELECT_RROBIN) ||
         (ps->config.select_mode == ET_STATION_SELECT_EQUALCUE)) &&
        (restore ==  ET_STATION_RESTORE_IN)) {

        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setblock, cannot set rrobin or equalcue station to restore to input list\n");
        }
        return ET_ERROR;
    }

    et_station_lock(etid->sys);
    ps->config.restore_mode = restore;
    et_station_unlock(etid->sys);
    et_mem_unlock(etid);

    return ET_OK;
}


/**
 * This routine gets a station's prescale value.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param prescale   int pointer which gets filled with the prescale value.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 *
 * @see et_station_setprescale().
 */
int et_station_getprescale(et_sys_id id, et_stat_id stat_id, int *prescale) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getprescale(id, stat_id, prescale);
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
            et_logmsg("ERROR", "et_station_getprescale, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getprescale, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getprescale, station is unused\n");
        }
        return ET_ERROR;
    }

    if (prescale != NULL) {
        *prescale = ps->config.prescale;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine sets a station's prescale value. The prescale is 1 by default
 * and selects every Nth event into the input list out of the normally accepted events.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param prescale   prescale value with is any positive integer.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is GRAND_CENTRAL, bad stat_id or bad user argument value,
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_setprescale(et_sys_id id, et_stat_id stat_id, int prescale) {

    et_id      *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;
    et_list    *pl = &ps->list_in;

    if (stat_id == ET_GRANDCENTRAL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setprescale, may not modify grandcentral station\n");
        }
        return ET_ERROR;
    }

    if (prescale < 1) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setprescale, bad prescale value\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_setprescale(id, stat_id, prescale);
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
            et_logmsg("ERROR", "et_station_setprescale, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setprescale, bad station id\n");
        }
        return ET_ERROR;
    }

    if (((ps->config.select_mode == ET_STATION_SELECT_RROBIN) ||
         (ps->config.select_mode == ET_STATION_SELECT_EQUALCUE)) &&
        (prescale != 1)) {

        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setblock, prescale for rrobin or equalcue station must be 1\n");
        }
        return ET_ERROR;
    }

    et_station_lock(etid->sys);
    et_llist_lock(pl);
    ps->config.prescale = prescale;
    et_llist_unlock(pl);
    et_station_unlock(etid->sys);
    et_mem_unlock(etid);

    return ET_OK;
}


/**
 * This routine gets a station's queue size, the size of the input event list
 * for nonblocking stations.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param cue        int pointer which gets filled with the queue size.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 *
 * @see et_station_setcue().
 */
int et_station_getcue(et_sys_id id, et_stat_id stat_id, int *cue) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getcue(id, stat_id, cue);
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
            et_logmsg("ERROR", "et_station_getcue, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getcue, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getcue, station is unused\n");
        }
        return ET_ERROR;
    }

    if (cue != NULL) {
        *cue = ps->config.cue;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine sets a station's queue size, the size of the input event list
 * for nonblocking stations. If the queue size is set to the total number of
 * events in the system, it effectively changes the behavior of the station
 * to act as if it were blocking.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param cue        integer from 1 up to the total number of events in the system.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is GRAND_CENTRAL, bad stat_id or bad user argument value,
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_setcue(et_sys_id id, et_stat_id stat_id, int cue) {

    et_id      *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;
    et_list    *pl = &ps->list_in;

    if (stat_id == ET_GRANDCENTRAL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setcue, may not modify grandcentral station\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_setcue(id, stat_id, cue);
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
            et_logmsg("ERROR", "et_station_setcue, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setcue, bad station id\n");
        }
        return ET_ERROR;
    }

    if ((cue < 1) || (cue > etid->sys->config.nevents)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setcue, bad cue value\n");
        }
        return ET_ERROR;
    }

    et_station_lock(etid->sys);
    et_llist_lock(pl);
    ps->config.cue = cue;
    et_llist_unlock(pl);
    et_station_unlock(etid->sys);
    et_mem_unlock(etid);

    return ET_OK;
}


/**
 * This routine gets a station's array of selection integers used to select events.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param select     integer array with at least @ref ET_STATION_SELECT_INTS number of
 *                   elements which gets filled in with the station's selection array.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 *
 * @see et_station_setselectwords().
 */
int et_station_getselectwords(et_sys_id id, et_stat_id stat_id, int select[]) {

    int i;
    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getselectwords(id, stat_id, select);
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
            et_logmsg("ERROR", "et_station_getselectwords, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getselectwords, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getselectwords, station is unused\n");
        }
        return ET_ERROR;
    }

    if (select != NULL) {
        for (i=0; i < ET_STATION_SELECT_INTS; i++) {
            select[i] = ps->config.select[i];
        }
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine sets a station's array of selection integers used to select events.
 * These integers are used to filter events out of the station when the station's
 * select mode is set to @ref ET_STATION_SELECT_MATCH. This may also be true if set to the
 * @ref ET_STATION_SELECT_USER mode but that depends on the user's specific filtering routine.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param select     integer array with at least ET_STATION_SELECT_INTS number of elements.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is GRAND_CENTRAL, bad stat_id or bad user argument value,
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_setselectwords(et_sys_id id, et_stat_id stat_id, int select[]) {

    int         i;
    et_id      *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;
    et_list    *pl = &ps->list_in;


    if (stat_id == ET_GRANDCENTRAL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setselectwords, may not modify grandcentral station\n");
        }
        return ET_ERROR;
    }

    if (etid->locality != ET_LOCAL) {
        return etr_station_setselectwords(id, stat_id, select);
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
            et_logmsg("ERROR", "et_station_setselectwords, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_setselectwords, bad station id\n");
        }
        return ET_ERROR;
    }

    et_station_lock(etid->sys);
    et_llist_lock(pl);
    for (i=0; i < ET_STATION_SELECT_INTS; i++) {
        ps->config.select[i] = select[i];
    }
    et_llist_unlock(pl);
    et_station_unlock(etid->sys);
    et_mem_unlock(etid);

    return ET_OK;
}

/**
 * This routine gets a station's select mode which determines the algorithm this
 * station uses to decide which events to let into its input list.
 *
 * <uL>
 *  <li>The value @ref ET_STATION_SELECT_ALL allows all events in.</li>
 *
 *  <li>The value @ref ET_STATION_SELECT_USER indicates a user-provided function will decide.</li>
 *
 *  <li>The value @ref ET_STATION_SELECT_MATCH uses a built-in algorithm.
 * To understand what this algorithm does, one must realize that each event has an array of
 * integers associated with it - metadata which can be set by the user. Likewise, each
 * station has a corresponding array of integers of the same size associated with it -
 * again, metadata which can be set by the user. The function compares each element of the
 * 2 arrays together and ORs the results. The first elements are checked for equality, the
 * second elements with a bitwise AND, the third for equality, the fourth with a bitwise AND etc., etc.
 * If the result is 1 or true, then the event is accepted into the input list.</li>
 *
 *  <li>The value @ref ET_STATION_SELECT_RROBIN only applies to parallel stations and results in a
 * round-robin distribution of events between all parallel stations in the same group.</li>
 *
 *  <li>The value @ref ET_STATION_SELECT_EQUALCUE only applies to parallel stations and results in a
 * distribution of events between all parallel stations in the same group such that all input lists
 * contain the same number of events (load distribution).</li>
 *</ul>
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param select     int pointer that gets filled with select mode which may be
 *                   ET_STATION_SELECT_ALL, ET_STATION_SELECT_USER, ET_STATION_SELECT_MATCH,
 *                   ET_STATION_SELECT_RROBIN, or ET_STATION_SELECT_EQUALCUE..
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused or bad stat_id argument.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getselect(et_sys_id id, et_stat_id stat_id, int *select) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->locality == ET_REMOTE) {
        return etr_station_getselect(id, stat_id, select);
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
            et_logmsg("ERROR", "et_station_getselect, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getselect, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getselect, station is unused\n");
        }
        return ET_ERROR;
    }

    if (select != NULL) {
        *select = ps->config.select_mode;
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the name of the shared library containing an event
 * selection function provided by the user. Meaningful only if the
 * @ref ET_STATION_SELECT_USER select mode is being used.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param lib        pointer which gets filled with the library name.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused, bad stat_id argument, or select mode
 *                                is not ET_STATION_SELECT_USER.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getlib(et_sys_id id, et_stat_id stat_id, char *lib) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->lang == ET_LANG_JAVA) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getlib, not a C system\n");
        }
        return ET_ERROR;
    }

    if (etid->locality == ET_REMOTE) {
        return etr_station_getlib(id, stat_id, lib);
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
            et_logmsg("ERROR", "et_station_getlib, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getlib, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getlib, station is unused\n");
        }
        return ET_ERROR;
    }

    if (ps->config.select_mode != ET_STATION_SELECT_USER) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getlib, station is not in user select mode\n");
        }
        return ET_ERROR;
    }

    if (lib != NULL) {
        strcpy(lib, ps->config.lib);
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the Java class defining an event
 * selection method provided by the user for Java-based ET systems.
 * Meaningful only if the @ref ET_STATION_SELECT_USER select mode is being used.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param classs     pointer which gets filled with the class name.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused, bad stat_id argument, or select mode
 *                                is not ET_STATION_SELECT_USER.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getclass(et_sys_id id, et_stat_id stat_id, char *classs) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->lang != ET_LANG_JAVA) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getclass, not a JAVA system\n");
        }
        return ET_ERROR;
    }

    if (etid->locality == ET_REMOTE) {
        return etr_station_getclass(id, stat_id, classs);
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
            et_logmsg("ERROR", "et_station_getclass, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getclass, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getclass, station is unused\n");
        }
        return ET_ERROR;
    }

    if (ps->config.select_mode != ET_STATION_SELECT_USER) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getclass, station is not in user select mode\n");
        }
        return ET_ERROR;
    }

    if (classs != NULL) {
        strcpy(classs, ps->config.classs);
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/**
 * This routine gets the name of the selection function in the shared library
 * provided by the user. Meaningful only if the
 * @ref ET_STATION_SELECT_USER select mode is being used.
 *
 * @param id         ET system id.
 * @param stat_id    station id.
 * @param lib        pointer which gets filled with the selection function name.
 *
 * @returns @ref ET_OK            if successful.
 * @returns @ref ET_ERROR         if station is unused, bad stat_id argument, or select mode
 *                                is not ET_STATION_SELECT_USER.
 * @returns @ref ET_ERROR_CLOSED  if et_close already called.
 * @returns @ref ET_ERROR_DEAD    if ET system is dead.
 * @returns @ref ET_ERROR_READ    for a remote user's network read error.
 * @returns @ref ET_ERROR_WRITE   for a remote user's network write error.
 */
int et_station_getfunction(et_sys_id id, et_stat_id stat_id, char *function) {

    et_id *etid = (et_id *) id;
    et_station *ps = etid->grandcentral + stat_id;

    if (etid->lang == ET_LANG_JAVA) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getfunction, not a C system\n");
        }
        return ET_ERROR;
    }

    if (etid->locality == ET_REMOTE) {
        return etr_station_getfunction(id, stat_id, function);
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
            et_logmsg("ERROR", "et_station_getfunction, et id is closed\n");
        }
        return ET_ERROR_CLOSED;
    }

    if ((stat_id < 0) || (stat_id >= etid->sys->config.nstations)) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getfunction, bad station id\n");
        }
        return ET_ERROR;
    }

    if (ps->data.status == ET_STATION_UNUSED) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getfunction, station is unused\n");
        }
        return ET_ERROR;
    }

    if (ps->config.select_mode != ET_STATION_SELECT_USER) {
        et_mem_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_station_getfunction, station is not in user select mode\n");
        }
        return ET_ERROR;
    }

    if (function != NULL) {
        strcpy(function, ps->config.fname);
    }

    et_mem_unlock(etid);
    return ET_OK;
}


/** @} */




