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
 *      Routines dealing with station configuration.
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "et_private.h"

/**
 * @defgroup statConfig Station config
 *
 * These routines configure the options necessary to create a station.
 *
 * @{
 */


/*****************************************************/
/*            STATION CONFIGURATION                  */
/*****************************************************/

/**
 * This routine initializes a configuration used to create a station in the ET system.
 *
 * This MUST be done prior to setting any configuration parameters or all setting
 * routines will return an error.
 *
 * @param sconfig   pointer to an station configuration variable
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if it fails to allocate memory for configuration data storage.
 */
int et_station_config_init(et_statconfig* sconfig) {

    int i;
    et_stat_config *sc;

    sc = (et_stat_config *) malloc(sizeof(et_stat_config));
    if (sc == NULL) {
        return ET_ERROR;
    }

    /* default configuration for a station */
    sc->flow_mode    = ET_STATION_SERIAL;
    sc->restore_mode = ET_STATION_RESTORE_OUT;
    sc->user_mode    = ET_STATION_USER_MULTI;
    sc->select_mode  = ET_STATION_SELECT_ALL;
    sc->block_mode   = ET_STATION_BLOCKING;
    *sc->fname        = '\0';
    *sc->lib          = '\0';
    *sc->classs       = '\0';
    sc->cue          =  ET_STATION_CUE;
    sc->prescale     =  ET_STATION_PRESCALE;
    for (i=0 ; i< ET_STATION_SELECT_INTS ; i++) {
        sc->select[i]  = -1;
    }
    sc->init         =  ET_STRUCT_OK;

    *sconfig = (et_statconfig) sc;
    return ET_OK;
}

/**
 * This routine frees the memory allocated when a configuration is initialized
 * by @ref et_station_config_init().
 *
 * @param sconfig   station configuration
 *
 * @returns @ref ET_OK
 */
int et_station_config_destroy(et_statconfig sconfig) {

    if (sconfig != NULL) {
        free((et_stat_config *) sconfig);
    }
    return ET_OK;
}

/**
 * This routine sets a station configuration's block mode.
 *
 * Determines whether all events pass through the station (@ref ET_STATION_BLOCKING)
 * which is the default or whether only a fixed size cue of events is filled while
 * all others by-pass the station (@ref ET_STATION_NONBLOCKING).
 *
 * @param sconfig   station configuration.
 * @param val       ET_STATION_BLOCKING or ET_STATION_NONBLOCKING.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or was not initialized;
 *                          if val is not ET_STATION_BLOCKING or ET_STATION_NONBLOCKING.
 */
int et_station_config_setblock(et_statconfig sconfig, int val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_STATION_NONBLOCKING) &&
        (val != ET_STATION_BLOCKING))     {
        return ET_ERROR;
    }

    sc->block_mode = val;
    return ET_OK;
}

/**
 * This routine gets a station configuration's block mode.
 *
 * @param sconfig   station configuration.
 * @param val       int pointer which gets filled with either @ref ET_STATION_BLOCKING or
 *                  @ref ET_STATION_NONBLOCKING.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setblock()
 */
int et_station_config_getblock(et_statconfig sconfig, int *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->block_mode;
    return ET_OK;
}

/**
 * This routine sets a station configuration's flow mode.
 *
 * Determines whether this station is treated independently of all other stations
 * and every event is examined for possible placement into it (@ref ET_STATION_SERIAL)
 * which is the default, or whether this station is grouped together with other stations into a
 * group of parallel stations (@ref ET_STATION_PARALLEL). Within a single group of parallel
 * stations, events may be distributed in a round-robin or load-distributed manner.
 *
 * @param sconfig   station configuration.
 * @param val       ET_STATION_SERIAL or ET_STATION_PARALLEL.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or was not initialized;
 *                          if val is not ET_STATION_BLOCKING or ET_STATION_NONBLOCKING.
 */
int et_station_config_setflow(et_statconfig sconfig, int val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_STATION_SERIAL)   &&
        (val != ET_STATION_PARALLEL))    {
        return ET_ERROR;
    }

    sc->flow_mode = val;
    return ET_OK;
}

/**
 * This routine gets a station configuration's flow mode.
 *
 * @param sconfig   station configuration.
 * @param val       int pointer which gets filled with either ET_STATION_SERIAL or ET_STATION_PARALLEL.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setflow()
 */
int et_station_config_getflow(et_statconfig sconfig, int *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->flow_mode;
    return ET_OK;
}

/**
 * This routine sets a station configuration's select mode which determines the algorithm this
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
 * @param sconfig   station configuration.
 * @param val       ET_STATION_SELECT_ALL, ET_STATION_SELECT_USER, ET_STATION_SELECT_MATCH,
 *                  ET_STATION_SELECT_RROBIN, or ET_STATION_SELECT_EQUALCUE.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or was not initialized;
 *                          if val is not ET_STATION_SELECT_ALL, ET_STATION_SELECT_USER,
 *                          ET_STATION_SELECT_MATCH, ET_STATION_SELECT_RROBIN, or ET_STATION_SELECT_EQUALCUE.
 */
int et_station_config_setselect(et_statconfig sconfig, int val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_STATION_SELECT_ALL)      &&
        (val != ET_STATION_SELECT_MATCH)    &&
        (val != ET_STATION_SELECT_USER)     &&
        (val != ET_STATION_SELECT_RROBIN)   &&
        (val != ET_STATION_SELECT_EQUALCUE))  {
        return ET_ERROR;
    }

    sc->select_mode = val;
    return ET_OK;
}

/**
 * This routine gets a station configuration's select mode.
 *
 * @param sconfig   station configuration.
 * @param val       int pointer which gets filled with ET_STATION_SELECT_ALL, ET_STATION_SELECT_USER,
 *                  ET_STATION_SELECT_MATCH, ET_STATION_SELECT_RROBIN, or ET_STATION_SELECT_EQUALCUE.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setselect()
 */
int et_station_config_getselect(et_statconfig sconfig, int *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->select_mode;
    return ET_OK;
}

/**
 * This routine sets a station configuration's user mode which is the
 * maximum number of users which may simultaneously attach to the station.
 *
 * Value may be @ref ET_STATION_USER_SINGLE for only 1 user, or
 * @ref ET_STATION_USER_MULTI for multiple users, or may be set to
 * a specific positive integer for only that many simultaneous attachments.
 *
 * @param sconfig   station configuration.
 * @param val       ET_STATION_USER_MULTI or ET_STATION_USER_SINGLE.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or was not initialized;
 *                          if val is not ET_STATION_USER_MULTI, ET_STATION_USER_SINGLE, or positive int,
 */
int et_station_config_setuser(et_statconfig sconfig, int val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 0) {
        return ET_ERROR;
    }

    sc->user_mode = val;
    return ET_OK;
}

/**
 * This routine gets a station configuration's user mode.
 *
 * @param sconfig   station configuration.
 * @param val       int pointer which gets filled with number of allowed simultaneous attachments
 *                  where 0 (@ref ET_STATION_USER_MULTI) means multiple.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setuser()
 */
int et_station_config_getuser(et_statconfig sconfig, int *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->user_mode;
    return ET_OK;
}

/**
 * This routine sets a station configuration's restore mode which determines
 * where events go when the attachment that owns those events detaches from the station.
 *
 * The value may be @ref ET_STATION_RESTORE_OUT which places events in this station's output list,
 * The value may be @ref ET_STATION_RESTORE_IN which places events in this station's input list -
 * unless there are no more attachments in which case they go to the output list.
 * The value may be @ref ET_STATION_RESTORE_GC which places events in Grand Central's input list.
 * The value may be @ref ET_STATION_RESTORE_REDIST which places events in the previous station's
 * output list for redistribution among a group of parallel stations.
 *
 * @param sconfig   station configuration.
 * @param val       ET_STATION_RESTORE_OUT, ET_STATION_RESTORE_IN, ET_STATION_RESTORE_GC,
 *                  or ET_STATION_RESTORE_REDIST.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or was not initialized;
 *                          if val is not ET_STATION_RESTORE_OUT, ET_STATION_RESTORE_IN,
 *                          ET_STATION_RESTORE_GC, or ET_STATION_RESTORE_REDIST.
 */
int et_station_config_setrestore(et_statconfig sconfig, int val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if ((val != ET_STATION_RESTORE_OUT) &&
        (val != ET_STATION_RESTORE_IN)  &&
        (val != ET_STATION_RESTORE_GC)  &&
        (val != ET_STATION_RESTORE_REDIST))    {
        return ET_ERROR;
    }

    sc->restore_mode = val;
    return ET_OK;
}

/**
 * This routine gets a station configuration's restore mode which determines
 * where events go when the attachment who owns those events detaches from the station.
 *
 * @param sconfig   station configuration.
 * @param val       int pointer which gets filled with ET_STATION_RESTORE_OUT,
 *                  ET_STATION_RESTORE_IN, ET_STATION_RESTORE_GC, or ET_STATION_RESTORE_REDIST.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setrestore()
 */
int et_station_config_getrestore(et_statconfig sconfig, int *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->restore_mode;
    return ET_OK;
}

/**
 * This routine sets a station configuration's queue size which is the max number of
 * events allowed in the input list when the station is nonblocking.
 *
 * If the size of the queue exceeds the total number of events in the system,
 * it is set to that limit when the station is created. Take notice that setting the
 * size of the queue to the total # of events will, in essence, change the station
 * into one which blocks (block mode of @refET_STATION_BLOCKING). The reason for
 * this is that all events will now pass through this station.
 *
 * @param sconfig   station configuration.
 * @param val       queue size of any positive integer.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or was not initialized;
 *                          if val is not positive.
 */
int et_station_config_setcue(et_statconfig sconfig, int val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 1) {
        return ET_ERROR;
    }

    sc->cue = val;
    return ET_OK;
}

/**
 * This routine gets a station configuration's queue size which is the max number of
 * events allowed in the input list when the station is nonblocking.
 *
 * @param sconfig   station configuration.
 * @param val       int pointer which gets filled with queue size.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setcue()
 */
int et_station_config_getcue(et_statconfig sconfig, int *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->cue;
    return ET_OK;
}

/**
 * This routine sets a station configuration's prescale value. The prescale is 1 by default
 * and selects every Nth event into the input list out of the normally accepted events.
 *
 * @param sconfig   station configuration.
 * @param val       prescale value of any positive integer.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if sconfig is NULL or was not initialized;
 *                          if val is not positive.
 */
int et_station_config_setprescale(et_statconfig sconfig, int val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val < 1) {
        return ET_ERROR;
    }

    sc->prescale = val;
    return ET_OK;
}

/**
 * This routine gets a station configuration's prescale value.
 *
 * @param sconfig   station configuration.
 * @param val       int pointer which gets filled with prescale value.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setprescale()
 */
int et_station_config_getprescale(et_statconfig sconfig, int *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    *val = sc->prescale;
    return ET_OK;
}

/**
 * This routine sets a station configuration's array of selection integers.
 * These integers are used to filter events out of the station when the station config's
 * select mode is set to @ref ET_STATION_SELECT_MATCH. This may also be true if set to the
 * @ref ET_STATION_SELECT_USER mode but that depends on the user's specific filtering routine.
 *
 * @param sconfig   station configuration.
 * @param val       int array with at least @ref ET_STATION_SELECT_INTS number of elements.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized.
 *
 * @see et_station_config_setselect()
 */
int et_station_config_setselectwords(et_statconfig sconfig, int val[]) {

    int i;
    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    for (i=0; i < ET_STATION_SELECT_INTS; i++) {
        sc->select[i] = val[i];
    }

    return ET_OK;
}

/**
 * This routine gets a station configuration's array of selection integers.
 *
 * @param sconfig   station configuration.
 * @param val       int array with at least @ref ET_STATION_SELECT_INTS number of elements
 *                  which gets filled with selection integers.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setselectwords()
 */
int et_station_config_getselectwords(et_statconfig sconfig, int val[]) {

    int i;
    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    for (i=0; i < ET_STATION_SELECT_INTS; i++) {
        val[i] = sc->select[i];
    }

    return ET_OK;
}

/**
 * This routine sets a station configuration's name of the user-defined
 * function which is loaded from a shared library and used for selecting events.
 * This is used with the select mode value of @ref ET_STATION_SELECT_USER.
 *
 * @param sconfig   station configuration.
 * @param val       name of user-defined function in shared library.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *                          if name is NULL or longer than (@ref ET_FUNCNAME_LENGTH - 1).
 *
 * @see et_station_config_setselect()
 */
int et_station_config_setfunction(et_statconfig sconfig, const char *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val == NULL) {
        return ET_ERROR;
    }

    if (strlen(val) > ET_FUNCNAME_LENGTH - 1) {
        return ET_ERROR;
    }

    strcpy(sc->fname, val);
    return ET_OK;
}

/**
 * This routine gets a station configuration's name of the user-defined
 * function which is loaded from a shared library and used for selecting events
 * (may be NULL).
 *
 * @param sconfig   station configuration.
 * @param val       char pointer which gets filled function name.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setfunction()
 */
int et_station_config_getfunction(et_statconfig sconfig, char *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    strcpy(val, sc->fname);
    return ET_OK;
}

/**
 * This routine sets a station configuration's name of the shared library
 * which is used for loading a user's function to select events.
 * This is used with the select mode value of @ref ET_STATION_SELECT_USER.
 *
 * @param sconfig   station configuration.
 * @param val       name of shared library.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *                          if name is NUL or longer than (@ref ET_FILENAME_LENGTH - 1).
 *
 * @see et_station_config_setselect()
 */
int et_station_config_setlib(et_statconfig sconfig, const char *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val == NULL) {
        return ET_ERROR;
    }

    if (strlen(val) > ET_FILENAME_LENGTH - 1) {
        return ET_ERROR;
    }

    strcpy(sc->lib, val);
    return ET_OK;
}

/**
 * This routine gets a station configuration's name of the shared library
 * which is used for loading a user's function to select events
 * (may be NULL).
 *
 * @param sconfig   station configuration.
 * @param val       char pointer which gets filled shared library name.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setlib()
 */
int et_station_config_getlib(et_statconfig sconfig, char *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    strcpy(val, sc->lib);
    return ET_OK;
}

/**
 * This routine sets a station configuration's name of the Java class
 * which is used for loading a user's function to select events.
 * This is used with the select mode value of @ref ET_STATION_SELECT_USER.
 * It is only meaningful when creating a station in a Java-based ET system.
 *
 * @param sconfig   station configuration.
 * @param val       name of Java class.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *                          if name is NULL or longer than (@ref ET_FILENAME_LENGTH - 1).
 *
 * @see et_station_config_setselect()
 */
int et_station_config_setclass(et_statconfig sconfig, const char *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK) {
        return ET_ERROR;
    }

    if (val == NULL) {
        return ET_ERROR;
    }

    if (strlen(val) > ET_FILENAME_LENGTH - 1) {
        return ET_ERROR;
    }

    strcpy(sc->classs, val);
    return ET_OK;
}

/**
 * This routine gets a station configuration's name of the Java class
 * which is used for loading a user's function to select events.
 * (may be NULL).
 * It is only meaningful when creating a station in a Java-based ET system.
 *
 * @param sconfig   station configuration.
 * @param val       char pointer which gets filled Java class name.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if either arg is NULL or sconfig not initialized;
 *
 * @see et_station_config_setclass()
 */
int et_station_config_getclass(et_statconfig sconfig, char *val) {

    et_stat_config *sc = (et_stat_config *) sconfig;

    if (sc == NULL || sc->init != ET_STRUCT_OK || val == NULL) {
        return ET_ERROR;
    }

    strcpy(val, sc->classs);
    return ET_OK;
}

/** @} */

/**
 * This routine checks a station's configuration settings for
 * internal inconsistencies or bad values.
 *
 * @param id   et system id.
 * @param sc   station config pointer.
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if there is an internal inconsistency or bad value in the configuration.
 */
int et_station_config_check(et_id *id, et_stat_config *sc)
{    
  if ((sc->flow_mode != ET_STATION_SERIAL) &&
      (sc->flow_mode != ET_STATION_PARALLEL))    {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_config_check, station flow_mode must be ET_STATION_SERIAL/PARALLEL\n");
    }  
    return ET_ERROR;
  }
  
  if ((sc->restore_mode != ET_STATION_RESTORE_OUT) &&
      (sc->restore_mode != ET_STATION_RESTORE_IN)  &&
      (sc->restore_mode != ET_STATION_RESTORE_GC)  &&
      (sc->restore_mode != ET_STATION_RESTORE_REDIST))   {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_config_check, station restore_mode must be ET_STATION_RESTORE_OUT/IN/GC/REDIST\n");
    }  
    return ET_ERROR;
  }
  
  if ((sc->user_mode < 0) ||
      (sc->user_mode > id->sys->config.nattachments)) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_config_check, station user_mode must be ET_STATION_USER_SINGLE/MULTI or max number of attachments desired (not to exceed ET_ATTACHMENTS_MAX)\n");
    }  
    return ET_ERROR;
  }
  
  if ((sc->select_mode != ET_STATION_SELECT_ALL)      &&
      (sc->select_mode != ET_STATION_SELECT_MATCH)    &&
      (sc->select_mode != ET_STATION_SELECT_USER)     &&
      (sc->select_mode != ET_STATION_SELECT_RROBIN)   &&
      (sc->select_mode != ET_STATION_SELECT_EQUALCUE))   {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_config_check, station select_mode must be ET_STATION_SELECT_ALL/MATCH/USER/RROBIN/EQUALCUE\n");
    }  
    return ET_ERROR;
  }
      
  /* USER mode means specifing a shared lib and function */
  /* smallest name for lib = a.so (4 char)               */
  if (sc->select_mode == ET_STATION_SELECT_USER) {
    if ((strlen(sc->lib) < 4) || (strlen(sc->fname) < 1)) {
      if (id->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "et_station_config_check, SELECT_USER mode requires a shared lib and function\n");
      }
      return ET_ERROR;
    }
  }
  
  /* Must be parallel, block, not prescale, and not restore to input list if rrobin or equal cue */
  if (((sc->select_mode  == ET_STATION_SELECT_RROBIN) ||
       (sc->select_mode  == ET_STATION_SELECT_EQUALCUE)) &&
      ((sc->flow_mode    == ET_STATION_SERIAL) ||
       (sc->block_mode   == ET_STATION_NONBLOCKING) ||
       (sc->restore_mode == ET_STATION_RESTORE_IN)  ||
       (sc->prescale     != 1))) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_config_check, if flow_mode = rrobin/equalcue, station must be parallel, blocking, prescale=1, & not restore-in\n");
    }
    return ET_ERROR;
  }

  /* If redistributing restored events, must be a parallel station */
  if ((sc->restore_mode == ET_STATION_RESTORE_REDIST) &&
      (sc->flow_mode    != ET_STATION_PARALLEL)) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_config_check, if restore_mode = restore-redist, station must be parallel\n");
    }
    return ET_ERROR;
  }

  if ((sc->block_mode != ET_STATION_BLOCKING)    &&
      (sc->block_mode != ET_STATION_NONBLOCKING))  {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_config_check, station block_mode must be ET_STATION_BLOCKING/NONBLOCKING\n");
    }  
    return ET_ERROR;
  }
  
  if (sc->block_mode == ET_STATION_BLOCKING) {
    if (sc->prescale < 1) {
      if (id->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "et_station_config_check, station prescale must be > 0\n");
      }  
      return ET_ERROR;
    }
  }
  else {
    if (sc->cue < 1) {
      if (id->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "et_station_config_check, station cue must be > 0\n");
      }  
      return ET_ERROR;
    }
    else if (sc->cue > id->sys->config.nevents) {
        sc->cue = id->sys->config.nevents;
    }
  }  
  return ET_OK;
}


/**
 * This routine compares all relevant station configuration
 * parameters to see if both configs are compatible
 * enough to belong in the same parallel station group.
 *
 * @param id       et system id.
 * @param group    pointer to config of a station already in a parallel group.
 * @param config   pointer to config of station to add to parallel group.
 *
 * @returns 1   if compatible.
 * @returns 0   if NOT compatible.
 */
int et_station_compare_parallel(et_id *id, et_stat_config *group, et_stat_config *config) {

  /* both must be parallel stations */
  if (group->flow_mode != config->flow_mode) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_compare_parallel, flow_mode must be ET_STATION_PARALLEL\n");
    }
    return 0;
  }

  /* if group is roundrobin or equal-cue, then config must be same */
  if (((group->select_mode == ET_STATION_SELECT_RROBIN) &&
       (config->select_mode != ET_STATION_SELECT_RROBIN)) ||
      ((group->select_mode == ET_STATION_SELECT_EQUALCUE) &&
       (config->select_mode != ET_STATION_SELECT_EQUALCUE))) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_compare_parallel, if group is rrobin/equalcue, station must be same\n");
    }
    return 0;
  }

  /* if group is roundrobin or equal-cue, then config's blocking & prescale must be same */
  if (((group->select_mode == ET_STATION_SELECT_RROBIN) ||
       (group->select_mode == ET_STATION_SELECT_EQUALCUE)) &&
      ((group->block_mode  != config->block_mode) ||
       (group->prescale    != config->prescale))) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_compare_parallel, if group is rrobin/equalcue, station must be blocking & prescale=1\n");
    }
    return 0;
  }
  
  /* if group is NOT roundrobin or equal-cue, then config's cannot be either */
  if (((group->select_mode != ET_STATION_SELECT_RROBIN) &&
       (group->select_mode != ET_STATION_SELECT_EQUALCUE)) &&
      ((config->select_mode == ET_STATION_SELECT_RROBIN) ||
       (config->select_mode == ET_STATION_SELECT_EQUALCUE))) {
    if (id->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_station_compare_parallel, if group is NOT rrobin/equalcue, station must not be either\n");
    }
    return 0;
  }
  
  return 1;
}

