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
 *      Wrapper routines for remote clients that don't
 *	need all the system or local-client stuff. A few routines (those
 *	independent of being remote or local) are copied from this file and
 *	that to mimimize the library size.
 *
 *----------------------------------------------------------------------------*/
 

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "et_private.h"
#include "et_network.h"

/******************************************
 *     MUTEX STUFF (used to make network
 *     communications thread-safe.)
 ******************************************/
/******************************************************/
void et_tcp_lock(et_id *id)
{
  int status;

  status = pthread_mutex_lock(&id->mutex);
  if (status != 0) {
    err_abort(status, "Failed tcp lock");
  }
}

/******************************************************/
void et_tcp_unlock(et_id *id)
{
  int status;

  status = pthread_mutex_unlock(&id->mutex);
  if (status != 0) {
    err_abort(status, "Failed tcp unlock");
  }
}

/******************************
 *     EVENT STUFF
 ******************************/
/******************************************************/
int et_event_new(et_sys_id id, et_att_id att, et_event **pe,
		 int mode, struct timespec *deltatime, size_t size)
{
    return etr_event_new(id, att, pe, mode, deltatime, size);
}

/******************************************************/
int et_events_new(et_sys_id id, et_att_id att, et_event *pe[],
		 int mode, struct timespec *deltatime,
		 size_t size, int num, int *nread)
{
    return etr_events_new(id, att, pe, mode, deltatime, size, num, nread);
}

/******************************************************/
int et_event_get(et_sys_id id, et_att_id att, et_event **pe,
		 int mode, struct timespec *deltatime)
{
    return etr_event_get(id, att, pe, mode, deltatime);
}

/******************************************************/
int et_events_get(et_sys_id id, et_att_id att, et_event *pe[],
		  int mode, struct timespec *deltatime, int num, int *nread)
{
    return etr_events_get(id, att, pe, mode, deltatime, num, nread);
}


/******************************************************/
int et_event_put(et_sys_id id, et_att_id att, et_event *pe)
{ 
    return etr_event_put(id, att, pe);
}

/******************************************************/
int et_events_put(et_sys_id id, et_att_id att, et_event *pe[], int num)
{
    return etr_events_put(id, att, pe, num);
}

/******************************************************/
int et_event_dump(et_sys_id id, et_att_id att, et_event *pe)
{ 
    return etr_event_dump(id, att, pe);
}

/******************************************************/
int et_events_dump(et_sys_id id, et_att_id att, et_event *pe[], int num)
{
    return etr_events_dump(id, att, pe, num);
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

/******************************************************/ 
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

/*****************************************************
 *   COMMON STUFF
 *****************************************************/
/******************************************************/
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

 
/*****************************************************
 *   NOTE: This file is different from the version in
 *   et_common.c . It's modified to ensure a remote
 *   connection to the ET system.
 *****************************************************/
/******************************************************/
int et_open(et_sys_id *id, const char *filename, et_openconfig openconfig)
{     
  int             status, auto_open=0, err;
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
  
  /* We depart from the original version of et_open since
   * this ET client will attach to an ET system remotely by definition.
   * Force the configuration to make a tcp connection to the ET system,
   * even if it's not remote.
   */
  config->mode = ET_HOST_AS_REMOTE;
  status = etr_open(id, filename, openconfig);
 
  if (status != ET_OK) {
    et_id_destroy(*id);
  }
  
  if (auto_open == 1) {
    et_open_config_destroy(auto_config);
  }

  return status;
}
 

/******************************************************/
int et_close(et_sys_id id)
{
  return etr_close(id);
}

/******************************************************/
int et_forcedclose(et_sys_id id)
{
  return etr_forcedclose(id);
}

/*****************************************************/

int et_alive(et_sys_id id)
{
  return etr_alive(id);
}

/******************************************************/
int et_wait_for_alive(et_sys_id id)
{
  return etr_wait_for_alive(id);
} 

/*****************************************************
 *   STATION STUFF 
 *****************************************************/
/******************************************************/
int et_station_create_at(et_sys_id id, et_stat_id *stat_id, const char *stat_name,
                         et_statconfig sconfig, int position, int parallelposition)
{
    return etr_station_create_at(id, stat_id, stat_name, sconfig, position,
                                 parallelposition);
}

/******************************************************/
int et_station_create(et_sys_id id, et_stat_id *stat_id, const char *stat_name,
                      et_statconfig sconfig)
{
    return etr_station_create_at(id, stat_id, stat_name, sconfig, ET_END, ET_END);
}

/*****************************************************/
int et_station_remove(et_sys_id id, et_stat_id stat_id)
{
    return etr_station_remove(id, stat_id);
}

/******************************************************/
int et_station_attach(et_sys_id id, et_stat_id stat_id, et_att_id *att)
{
    return etr_station_attach(id, stat_id, att);
}

/******************************************************/
int et_station_detach(et_sys_id id, et_att_id att)
{
    return etr_station_detach(id, att);
}

/******************************************************/
int et_station_setposition(et_sys_id id, et_stat_id stat_id, int position,
                           int parallelposition)
{
    return etr_station_setposition(id, stat_id, position, parallelposition);
}

/******************************************************/
int et_station_getposition(et_sys_id id, et_stat_id stat_id, int *position,
                           int *parallelposition)
{
    return etr_station_getposition(id, stat_id, position, parallelposition);
}

/******************************************************/
int et_station_isattached(et_sys_id id, et_stat_id stat_id, et_att_id att)
{
    return etr_station_isattached(id, stat_id, att);
}

/******************************************************/
int et_station_exists(et_sys_id id, et_stat_id *stat_id, const char *stat_name)
{
    return etr_station_exists(id, stat_id, stat_name);
}

/******************************************************/
int et_station_name_to_id(et_sys_id id, et_stat_id *stat_id, const char *stat_name)
{
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

/******************************************************/
int et_station_getattachments(et_sys_id id, et_stat_id stat_id, int *numatts)
{
    return etr_station_getattachments(id, stat_id, numatts);
}

/******************************************************/
int et_station_getstatus(et_sys_id id, et_stat_id stat_id, int *status)
{
    return etr_station_getstatus(id, stat_id, status);
}

/******************************************************/
int et_station_getinputcount(et_sys_id id, et_stat_id stat_id, int *cnt)
{
    return etr_station_getinputcount(id, stat_id, cnt);
}

/******************************************************/
int et_station_getoutputcount(et_sys_id id, et_stat_id stat_id, int *cnt)
{
    return etr_station_getoutputcount(id, stat_id, cnt);
}

/******************************************************/
int et_station_getblock(et_sys_id id, et_stat_id stat_id, int *block)
{
   return etr_station_getblock(id, stat_id, block);
}

/******************************************************/
int et_station_setblock(et_sys_id id, et_stat_id stat_id, int block)
{
    return etr_station_setblock(id, stat_id, block);
}

/******************************************************/
int et_station_getuser(et_sys_id id, et_stat_id stat_id, int *user)
{
    return etr_station_getuser(id, stat_id, user);
}

/******************************************************/
int et_station_setuser(et_sys_id id, et_stat_id stat_id, int user)
{
    return etr_station_setuser(id, stat_id, user);
}

/******************************************************/
int et_station_getrestore(et_sys_id id, et_stat_id stat_id, int *restore)
{
    return etr_station_getrestore(id, stat_id, restore);
}

/******************************************************/
int et_station_setrestore(et_sys_id id, et_stat_id stat_id, int restore)
{
    return etr_station_setrestore(id, stat_id, restore);
}


/******************************************************/
int et_station_getprescale(et_sys_id id, et_stat_id stat_id, int *prescale)
{
    return etr_station_getprescale(id, stat_id, prescale);
}

/******************************************************/
int et_station_setprescale(et_sys_id id, et_stat_id stat_id, int prescale)
{
    return etr_station_setprescale(id, stat_id, prescale);
}

/******************************************************/
int et_station_getcue(et_sys_id id, et_stat_id stat_id, int *cue)
{
    return etr_station_getcue(id, stat_id, cue);
}

/******************************************************/
int et_station_setcue(et_sys_id id, et_stat_id stat_id, int cue)
{
    return etr_station_setcue(id, stat_id, cue);
}

/******************************************************/
int et_station_getselectwords(et_sys_id id, et_stat_id stat_id, int select[])
{
    return etr_station_getselectwords(id, stat_id, select);
}

/******************************************************/
int et_station_setselectwords(et_sys_id id, et_stat_id stat_id, int select[])
{
    return etr_station_setselectwords(id, stat_id, select);
}

/******************************************************/
int et_station_getselect(et_sys_id id, et_stat_id stat_id, int *select)
{
    return etr_station_getselect(id, stat_id, select);
}


/******************************************************/
int et_station_getlib(et_sys_id id, et_stat_id stat_id, char *lib)
{
    return etr_station_getlib(id, stat_id, lib);
}

/******************************************************/
int et_station_getclass(et_sys_id id, et_stat_id stat_id, char *classs)
{
    return etr_station_getclass(id, stat_id, classs);
}

/******************************************************/
int et_station_getfunction(et_sys_id id, et_stat_id stat_id, char *function)
{
    return etr_station_getfunction(id, stat_id, function);
}


/*****************************************************
 *     ATTACHMENT STUFF
 *****************************************************/
/******************************************************/
int et_wakeup_attachment(et_sys_id id, et_att_id att)
{
    return etr_wakeup_attachment(id, att);
}
 
/******************************************************/
/*   Wake up all attachments waiting to read events   */
int et_wakeup_all(et_sys_id id, et_stat_id stat_id)
{
    return etr_wakeup_all(id, stat_id);
}

/******************************************************/
/*       Get number of events put by attachment       */
int et_attach_geteventsput(et_sys_id id, et_att_id att_id,
                           uint64_t *events)
{
    return etr_attach_geteventsput(id, att_id, events);
}

/******************************************************/
/*      Get number of events gotten by attachment     */
int et_attach_geteventsget(et_sys_id id, et_att_id att_id,
                           uint64_t *events)
{
    return etr_attach_geteventsget(id, att_id, events);
}

/******************************************************/
/*     Get number of events dumped by attachment      */
int et_attach_geteventsdump(et_sys_id id, et_att_id att_id,
                            uint64_t *events)
{
    return etr_attach_geteventsdump(id, att_id, events);
}

/******************************************************/
/*      Get number of events made by attachment       */
int et_attach_geteventsmake(et_sys_id id, et_att_id att_id,
                            uint64_t *events)
{
    return etr_attach_geteventsmake(id, att_id, events);
}

