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

#ifdef VXWORKS
#include <vxWorks.h>
#endif

#include <stdio.h>
#include <string.h>
#include "et_private.h"

/*****************************************************/
/*            STATION CONFIGURATION                  */
/*****************************************************/

int et_station_config_init(et_statconfig* sconfig)
{
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

/*****************************************************/
int et_station_config_destroy(et_statconfig sconfig)
{
  if (sconfig != NULL) {
    free((et_stat_config *) sconfig);
  }
  return ET_OK;
}

/*****************************************************/
int et_station_config_setblock(et_statconfig sconfig, int val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if ((val != ET_STATION_NONBLOCKING) &&
      (val != ET_STATION_BLOCKING))     {
    return ET_ERROR;
  }
  
  sc->block_mode = val;
  return ET_OK;
}

/*****************************************************/
int et_station_config_getblock(et_statconfig sconfig, int *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  *val = sc->block_mode;
  return ET_OK;
}

/*****************************************************/
int et_station_config_setflow(et_statconfig sconfig, int val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  
  if ((val != ET_STATION_SERIAL)   &&
      (val != ET_STATION_PARALLEL))    {
    return ET_ERROR;
  }
  
  sc->flow_mode = val;
  return ET_OK;
}

/*****************************************************/
int et_station_config_getflow(et_statconfig sconfig, int *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  *val = sc->flow_mode;
  return ET_OK;
}

/*****************************************************/
int et_station_config_setselect(et_statconfig sconfig, int val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
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

/*****************************************************/
int et_station_config_getselect(et_statconfig sconfig, int *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  *val = sc->select_mode;
  return ET_OK;
}

/*****************************************************/
int et_station_config_setuser(et_statconfig sconfig, int val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  
  if (val < 0) {
    return ET_ERROR;
  }
  
  sc->user_mode = val;
  return ET_OK;
}

/*****************************************************/
int et_station_config_getuser(et_statconfig sconfig, int *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  *val = sc->user_mode;
  return ET_OK;
}

/*****************************************************/
int et_station_config_setrestore(et_statconfig sconfig, int val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
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

/*****************************************************/
int et_station_config_getrestore(et_statconfig sconfig, int *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  *val = sc->restore_mode;
  return ET_OK;
}

/*****************************************************/
int et_station_config_setcue(et_statconfig sconfig, int val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  
  if (val < 1) {
    return ET_ERROR;
  }
  
  sc->cue = val;
  return ET_OK;
}

/*****************************************************/
int et_station_config_getcue(et_statconfig sconfig, int *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  *val = sc->cue;
  return ET_OK;
}

/*****************************************************/
int et_station_config_setprescale(et_statconfig sconfig, int val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  
  if (val < 1) {
    return ET_ERROR;
  }
  
  sc->prescale = val;
  return ET_OK;
}

/*****************************************************/
int et_station_config_getprescale(et_statconfig sconfig, int *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  *val = sc->prescale;
  return ET_OK;
}

/*****************************************************/
int et_station_config_setselectwords(et_statconfig sconfig, int val[])
{
  int i;
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
  
  for (i=0; i < ET_STATION_SELECT_INTS; i++) {
    sc->select[i] = val[i];
  }
  
  return ET_OK;
}

/*****************************************************/
int et_station_config_getselectwords(et_statconfig sconfig, int val[])
{
  int i;
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  for (i=0; i < ET_STATION_SELECT_INTS; i++) {
    val[i] = sc->select[i];
  }
  
  return ET_OK;
}

/*****************************************************/
int et_station_config_setfunction(et_statconfig sconfig, const char *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
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

/*****************************************************/
int et_station_config_getfunction(et_statconfig sconfig, char *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  strcpy(val, sc->fname);
  return ET_OK;
}

/*****************************************************/
int et_station_config_setlib(et_statconfig sconfig, const char *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
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

/*****************************************************/
int et_station_config_getlib(et_statconfig sconfig, char *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  strcpy(val, sc->lib);
  return ET_OK;
}

/*****************************************************/
int et_station_config_setclass(et_statconfig sconfig, const char *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  if (sc->init != ET_STRUCT_OK) {
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

/*****************************************************/
int et_station_config_getclass(et_statconfig sconfig, char *val)
{
  et_stat_config *sc = (et_stat_config *) sconfig;
  
  strcpy(val, sc->classs);
  return ET_OK;
}

/*****************************************************
 * Check a station's configuration settings
 *****************************************************/
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
      if (id->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "et_station_config_check, station cue must be <= total number of events\n");
      }  
      return ET_ERROR;
    }
  }  
  return ET_OK;
}


/******************************************************
 * Compare all relevant station configuration
 * parameters to see if both configs are compatible
 * enough to belong in the same parallel station group.
 *
 * The first configuration is assumed to be that of a
 * station already in the group (has been successfully
 * run through et_station_parallel_check).
 *
 * Return 1 if compatible, else 0
 ******************************************************/
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

