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
 *      Header file for ET system
 *
 *----------------------------------------------------------------------------*/

#ifndef _ET_H_
#define _ET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>


#define INET_ATON_ERR   0
#include <inttypes.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*********************************************************************/
/*            The following default values may be changed            */
/*********************************************************************/
/* station stuff */
#define ET_STATION_CUE      10	 /**< Max number of events queued in nonblocking station. */
#define ET_STATION_PRESCALE 1	 /**< Default prescale of blocking stations. */

/* system stuff */
#define ET_SYSTEM_EVENTS    300	 /**<  Default total number of events in ET system. */
#define ET_SYSTEM_NTEMPS    300	 /**<  Default max number of temp events in ET system. */
#define ET_SYSTEM_ESIZE     1000 /**<  Default size of normal events in bytes. */
#define ET_SYSTEM_NSTATS    200	 /**<  Default max number of stations. */

/* network stuff */
#define ET_UDP_PORT        11111 /**<  Default ET system UDP broad/multicast/directed-udp port. */
#define ET_SERVER_PORT     11111 /**<  Default ET system TCP server port. */
#define ET_MULTICAST_PORT  11111 /**<  @deprecated Previous default ET system UDP multicast port kept for backwards compatibility. */
#define ET_BROADCAST_PORT  11111 /**<  @deprecated Previous default ET system UDP broadcast port kept for backwards compatibility. */

#define ET_MULTICAST_ADDR  "239.200.0.0"    /**< Default multicast address to use in finding ET system. */
    
#define ET_EVENT_GROUPS_MAX 200 /**< Default max number of groups events are divided into. */

/*********************************************************************/
/*                              WARNING                              */
/* Changing ET_STATION_SELECT_INTS requires a full recompile of all  */
/* ET libraries (local & remote) whose users interact in any way.    */
/*********************************************************************/

/** Number of control integers associated with each station. */
#define ET_STATION_SELECT_INTS 6

/*********************************************************************/
/*             DO NOT CHANGE ANYTHING BELOW THIS LINE !!!            */
/*********************************************************************/

/** Default multicast time-to-live value (# of router hops permitted). */
#define ET_MULTICAST_TTL   32

/* ET server is on host that is ... */
#define ET_HOST_LOCAL      ".local"      /**< ET host is local. */
#define ET_HOST_REMOTE     ".remote"     /**< ET host is remote. */
#define ET_HOST_ANYWHERE   ".anywhere"   /**< ET host is either local or remote. */

/* Treat local ET server as local or remote? */
#define ET_HOST_AS_LOCAL   1  /**< For communication with local ET system, use shared memory. */
#define ET_HOST_AS_REMOTE  0  /**< For communication with local ET system, use sockets. */

/* Policy to pick a single responder from many after broad/multicast */
#define ET_POLICY_FIRST  0   /**< Pick first responder when broad/multicasting to find ET system. */
#define ET_POLICY_LOCAL  1   /**< Pick local responder, else the first when broad/multicasting to find ET system. */
#define ET_POLICY_ERROR  2   /**< Return an error if more than one responder when broad/multicasting to find ET system. */

/* Name lengths */
#define ET_FILENAME_LENGTH  100 /**< Max length of ET system file name + 1. */
#define ET_FUNCNAME_LENGTH  48  /**< Max length of user's event selection func name + 1. */
#define ET_STATNAME_LENGTH  48  /**< Max length of station name + 1. */
/** Max length of temp event file name + 1, leaves room for _temp<#######>. */
#define ET_TEMPNAME_LENGTH  (ET_FILENAME_LENGTH+12)

/*----------------*/
/* Station stuff. */
/*----------------*/

#define ET_GRANDCENTRAL 0  /**< Station id of GrandCentral station. */

/* Station status. */
#define ET_STATION_UNUSED   0            /**< Station structure is unused. */
#define ET_STATION_CREATING 1            /**< Station is in the middle of being created. */
#define ET_STATION_IDLE     2            /**< Station exists but has no attachments. */
#define ET_STATION_ACTIVE   3            /**< Station exists and has at least one attachment. */

/* How many user process can attach */
#define ET_STATION_USER_MULTI  0         /**< Multiple attachments are allowed on a single station. */
#define ET_STATION_USER_SINGLE 1         /**< Only one attachment is allowed on a single station. */

/* Can block for event flow or not */
#define ET_STATION_NONBLOCKING 0         /**< Events bypass station when input queue is full. */
#define ET_STATION_BLOCKING    1         /**< Station accepts all events (except if prescaling > 1 or
                                              if selection rule rejects them). */

/* Criterion for event selection */
#define ET_STATION_SELECT_ALL      1     /**< Select all events into station. */
#define ET_STATION_SELECT_MATCH    2     /**< Select events using predefined selection rule. */
#define ET_STATION_SELECT_USER     3     /**< Select event with user-supplied selection rule. */
#define ET_STATION_SELECT_RROBIN   4     /**< For parallel stations, select events with round-robin distribution. */
#define ET_STATION_SELECT_EQUALCUE 5     /**< For parallel stations, select events such that all input queues
                                              have the same number of events. */

/* How to restore events in dead user process */
#define ET_STATION_RESTORE_OUT 0	    /**< Restore recovered events to output list. */
#define ET_STATION_RESTORE_IN  1	    /**< Restore recovered events to input list. */
#define ET_STATION_RESTORE_GC  2        /**< Restore recovered events to GrandCentral input list. */
#define ET_STATION_RESTORE_REDIST 3     /**< Redistribute recovered events to input of all parallel stations,
                                             along with all events left in station's input list. */

/* How events flow through a (group of) station(s) */
#define ET_STATION_SERIAL        0      /**< normal flow of all events through all stations. */
#define ET_STATION_PARALLEL      1      /**< station is part of a group of stations in which
                                             events are flowing in parallel. */
#define ET_STATION_PARALLEL_HEAD 2      /**< station is head of a group of stations in which
                                             events are flowing in parallel. Only used when
                                             gathering monitoring data of stations (not for config). */

/**
 * @defgroup errors Errors
 *
 * Listing of all the error codes and routines dealing with errors.
 *
 * @{
 */

#define ET_OK              0   /**< No error. */
#define ET_ERROR          -1   /**< General error. */
#define ET_ERROR_TOOMANY  -2   /**< Too many somethings (stations, attachments, temp events, ET system responses) exist. */
#define ET_ERROR_EXISTS   -3   /**< ET system file or station already exists. */
#define ET_ERROR_WAKEUP   -4   /**< Sleeping routine woken up by {@link et_wakeup_attachment()} or {@link et_wakeup_all()}. */
#define ET_ERROR_TIMEOUT  -5   /**< Timed out. */
#define ET_ERROR_EMPTY    -6   /**< No events available in async mode. */
#define ET_ERROR_BUSY     -7   /**< Resource is busy. */
#define ET_ERROR_DEAD     -8   /**< ET system is dead. */
#define ET_ERROR_READ     -9   /**< Network read error */
#define ET_ERROR_WRITE    -10  /**< Network write error, */
#define ET_ERROR_REMOTE   -11  /**< Cannot allocate memory in remote client. */
#define ET_ERROR_NOREMOTE -12  /**< (Currently not used). */
#define ET_ERROR_TOOBIG   -13  /**< Client is 32 bits & server is 64 (or vice versa) and event is too big for one. */
#define ET_ERROR_NOMEM    -14  /**< Cannot allocate memory. */
#define ET_ERROR_BADARG   -15  /**< Bad argument given to function. */
#define ET_ERROR_SOCKET   -16  /**< Socket option could not be set. */
#define ET_ERROR_NETWORK  -17  /**< Host name or address could not be resolved, or cannot connect. */
#define ET_ERROR_CLOSED   -18  /**< ET system has been closed by client. */
#define ET_ERROR_JAVASYS  -19  /**< C code trying to open Java-based ET system file locally. */
/** @} */

/* Debug levels */
#define ET_DEBUG_NONE   0   /**< No debug output. */
#define ET_DEBUG_SEVERE 1   /**< Debug output for severe errors only. */
#define ET_DEBUG_ERROR  2   /**< Debug output for all errors. */
#define ET_DEBUG_WARN   3   /**< Debug output for warnings and  all errors. */
#define ET_DEBUG_INFO   4   /**< Debug output for warnings, errors, and general information. */

/* Event priority */
#define ET_LOW  0           /**< Event is of normal priority. */
#define ET_HIGH 1           /**< Event is of high priority which means it get placed at the top of each
                                 station in/output list (but below other high priority events). */
#define ET_PRIORITY_MASK 1	/**< Bit mask used to send priority bits over network. */

/** Event owner is the ET system (not an attachment or user-process). */
#define ET_SYS -1
/** When creating a new station, put at end of a linked list. */
#define ET_END -1
/** When creating a new parallel station, put at head of a
 * new group of parallel stations and in the main linked list of stations. */
#define ET_NEWHEAD -2

/* Event is temporary or not */
#define ET_EVENT_TEMP   1    /**< Event is temporary (bigger than normal events with
                                  shared memory especially allocated for it alone). */
#define ET_EVENT_NORMAL 0    /**< Normal ET system event existing in main shared memory. */

/* Event is new (obtained thru et_event(s)_new) or not */
#define ET_EVENT_NEW  1      /**< Event obtained through calling @ref et_event_new, @ref et_events_new,
                                  @ref et_event_new_group, or @ref et_events_new_group. */
#define ET_EVENT_USED 0      /**< Event obtained through calling @ref et_event_get or @ref et_events_get. */

/* Event get/new routines' wait mode flags */
#define ET_SLEEP  0          /**< When getting events, wait until at least one is available before returning. */
#define ET_TIMED  1          /**< When getting events, wait until either an event is available or the given time
                                  has expired before returning. */
#define ET_ASYNC  2          /**< When getting events, return immediately if no event is available. */
#define ET_WAIT_MASK  3	     /**< Bit mask for mode's wait info. */
/* event get/new routines' event flow flags */
#define ET_MODIFY 4          /**< Remote user will modify event. */
#define ET_MODIFY_HEADER 8   /**< Remote user will modify only event header (metadata). */
#define ET_DUMP          16  /**< If remote get and not modifying event, server dumps (not puts) events
                                  automatically back into ET. */
#define ET_NOALLOC       32	 /**< Remote user will not allocate memory but use existing buffer. */

/* et_open waiting modes */
#define ET_OPEN_NOWAIT 0    /**< Do not wait for the ET system to appear when trying to open it. */
#define ET_OPEN_WAIT   1    /**< Wait for the ET system to appear when trying to open it. */

/* Are we remote or local? */
#define ET_REMOTE        0  /**< ET system is on a remote host. */
#define ET_LOCAL         1  /**< ET system is on the local host. */
#define ET_LOCAL_NOSHARE 2  /**< ET system is on the local host, but it is not possible to share pthread mutexes. */

/* Use broadcast, multicast, both or direct connection to find server port */
#define ET_MULTICAST 0          /**< Use multicasting to find ET system. */
#define ET_BROADCAST 1          /**< Use broadcasting to find ET system. */
#define ET_DIRECT    2          /**< Connect directly to ET system TCP server by specifying host & port explicitly. */
#define ET_BROADANDMULTICAST 3  /**< Use broad and multicasting to find ET system. */

/* Options for et_open_config_add(remove)broadcast */
#define ET_SUBNET_ALL  ".subnetsAll"     /**< Use all local subnets. */

/* Status of data in an event */
#define ET_DATA_OK               0  /**< Event data is OK. */
#define ET_DATA_CORRUPT          1  /**< Event data is corrupt (currently not used). */
#define ET_DATA_POSSIBLY_CORRUPT 2  /**< Event data is possibly corrupt as event was recovered from user crash. */
#define ET_DATA_MASK		 0x30	/**< Bit mask for network handling of data status. */
#define ET_DATA_SHIFT		 4	    /**< Bit shift for network handling of data status. */

/* Endian values */
#define ET_ENDIAN_BIG      0	/**< Big endian. */
#define ET_ENDIAN_LITTLE   1	/**< Little endian. */
#define ET_ENDIAN_LOCAL    2	/**< Same endian as local host. */
#define ET_ENDIAN_NOTLOCAL 3	/**< Opposite endian as local host. */
#define ET_ENDIAN_SWITCH   4	/**< Switch recorded value of data's endian. */

/* do we swap data or not? */
#define ET_NOSWAP 0  /**< Event data endian is same as local host. */
#define ET_SWAP   1  /**< Event data must be swapped to be consistent with local host endian. */

/* Macros for swapping ints of various sizes */

/** Macro to swap endian of 64 bit arg. */
#define ET_SWAP64(x) ( (((x) >> 56) & 0x00000000000000FFL) | \
                       (((x) >> 40) & 0x000000000000FF00L) | \
                       (((x) >> 24) & 0x0000000000FF0000L) | \
                       (((x) >> 8)  & 0x00000000FF000000L) | \
                       (((x) << 8)  & 0x000000FF00000000L) | \
                       (((x) << 24) & 0x0000FF0000000000L) | \
                       (((x) << 40) & 0x00FF000000000000L) | \
                       (((x) << 56) & 0xFF00000000000000L) )

/** Macro to swap endian of 32 bit arg. */
#define ET_SWAP32(x) ( (((x) >> 24) & 0x000000FF) | \
                       (((x) >> 8)  & 0x0000FF00) | \
                       (((x) << 8)  & 0x00FF0000) | \
                       (((x) << 24) & 0xFF000000) )

/** Macro to swap endian of 16 bit arg. */
#define ET_SWAP16(x) ( (((x) >> 8) & 0x00FF) | \
                       (((x) << 8) & 0xFF00) )


/** Macro to exit program with unrecoverable error (see "Programming with POSIX threads' by Butenhof). */
#define err_abort(code,text) do { \
    fprintf (stderr, "%s at \"%s\":%d: %s\n", \
        text, __FILE__, __LINE__, strerror (code)); \
    exit (-1); \
} while (0)


/* STRUCTURES */

/**
 * This structure defines an <b>Event</b> which exists in shared memory, holds data,
 * and gets passed from station to station.
 */
typedef struct et_event_t {
  struct et_event_t *next;  /**< Next event in linked list. */
  void    *tempdata;        /**< For MacOS or non mutex-sharing systems, a temp event
                                 gotten by the server thread needs its data pointer stored
                                 while the user maps the temp file and puts that pointer
                                 in pdata field (when converting the pdata pointer back to
                                 a value the ET system uses, so the server thread can
                                 unmap it from memory, grab & restore the stored value). */
  void    *pdata;           /**< Set to either <b>data</b> field pointer (shared mem),
 *                               or to temp event buffer. */
  char    *data;            /**< Pointer to shared memory data. */
  uint64_t length;          /**< Size of actual data in bytes. */
  uint64_t memsize;         /**< Total size of available data memory in bytes. */

  /* for remote events */
  uint64_t pointer;        /**< For remote events, pointer to this event in the
                                server's space (used for writing of modified events). */
  int      modify;         /**< When "getting" events from a remote client, this flag tells
                                server whether this event will be modified with the
                                intention of sending it back to the server (@ref ET_MODIFY) or
                                whether only the header will be modified and returned
                                (@ref ET_MODIFY_HEADER) or whether there'll be no modification
                                of the event (0). */
  /* ----------------- */

  int     priority;        /**< Event priority, either @ref ET_HIGH or @ref ET_LOW. */
  int     owner;           /**< Id of attachment that owns this event, else the system (-1). */
  int     temp;            /**< @ref ET_EVENT_TEMP if temporary event, else @ref ET_EVENT_NORMAL. */
  int     age;             /**< @ref ET_EVENT_NEW if it's a new event, else @ref ET_EVENT_USED
                                (new events always go to GrandCentral if user crashes). */
  int     datastatus;      /**< @ref ET_DATA_OK normally, @ref ET_DATA_CORRUPT if data corrupt,
                                @ref ET_DATA_POSSIBLY_CORRUPT if data questionable. */
  int     byteorder;       /**< Use to track the data's endianness (set to 0x04030201). */
  int     group;           /**< Group number, events are divided into groups for limiting
                                the number of events producers have access to. */
  int     place;           /**< Place number of event's relative position in shared mem starting
                                with 0 (lowest memory position) and ending with (# of events - 1). */
  int     control[ET_STATION_SELECT_INTS];  /**< Array of ints to select on for entry into station. */
  char    filename[ET_TEMPNAME_LENGTH];     /**< Filename of temp event. */
} et_event;

/** Used to define a data-swapping function. */
typedef int (*ET_SWAP_FUNCPTR) (et_event *, et_event*, int, int);

/* TYPES */
typedef void *et_sys_id;        /**< ET system id. */
typedef void *et_statconfig;    /**< ET station configuration. */
typedef void *et_sysconfig;     /**< ET system configuration. */
typedef void *et_openconfig;    /**< ET open configuration. */
typedef void *et_bridgeconfig;  /**< ET bridge id. */
typedef int   et_stat_id;       /**< ET station id. */
typedef int   et_proc_id;       /**< ET process id. */
typedef int   et_att_id;        /**< ET attachment id. */

/***********************/
/* Extern declarations */
/***********************/

/* event functions */
extern int  et_event_new(et_sys_id id, et_att_id att, et_event **pe,  
                         int mode, struct timespec *deltatime, size_t size);
extern int  et_events_new(et_sys_id id, et_att_id att, et_event *pe[], 
                          int mode, struct timespec *deltatime, size_t size, 
                          int num, int *nread);
extern int  et_event_new_group(et_sys_id id, et_att_id att, et_event **pe,
                               int mode, struct timespec *deltatime,
                               size_t size, int group);
extern int  et_events_new_group(et_sys_id id, et_att_id att, et_event *pe[],
                                int mode, struct timespec *deltatime,
                                size_t size, int num, int group, int *nread);

extern int  et_event_get(et_sys_id id, et_att_id att, et_event **pe,  
                         int mode, struct timespec *deltatime);
extern int  et_events_get(et_sys_id id, et_att_id att, et_event *pe[], 
                          int mode, struct timespec *deltatime, 
                          int num, int *nread);

extern int  et_event_put(et_sys_id id, et_att_id att, et_event *pe);
extern int  et_events_put(et_sys_id id, et_att_id att, et_event *pe[], 
                          int num);
			   
extern int  et_event_dump(et_sys_id id, et_att_id att, et_event *pe);
extern int  et_events_dump(et_sys_id id, et_att_id att, et_event *pe[],
			   int num);

extern int  et_event_getgroup(et_event *pe, int *grp);

extern int  et_event_setpriority(et_event *pe, int pri);
extern int  et_event_getpriority(et_event *pe, int *pri);

extern int  et_event_setlength(et_event *pe, size_t len);
extern int  et_event_getlength(et_event *pe, size_t *len);

extern int  et_event_setcontrol(et_event *pe, int con[], int num);
extern int  et_event_getcontrol(et_event *pe, int con[]);

extern int  et_event_setdatastatus(et_event *pe, int datastatus);
extern int  et_event_getdatastatus(et_event *pe, int *datastatus);

extern int  et_event_setendian(et_event *pe, int endian);
extern int  et_event_getendian(et_event *pe, int *endian);

extern int  et_event_getdata(et_event *pe, void **data);
extern int  et_event_setdatabuffer(et_sys_id id, et_event *pe, void *data);
extern int  et_event_needtoswap(et_event *pe, int *swap);

/* system functions */
extern int  et_open(et_sys_id* id, const char *filename, et_openconfig openconfig);
extern int  et_close(et_sys_id id);
extern int  et_forcedclose(et_sys_id id);
extern int  et_kill(et_sys_id id);
extern int  et_system_start(et_sys_id* id, et_sysconfig sconfig);
extern int  et_system_close(et_sys_id id);
extern int  et_alive(et_sys_id id);
extern int  et_wait_for_alive(et_sys_id id);
extern char* et_perror(int err);

/* attachment functions */
extern int  et_wakeup_attachment(et_sys_id id, et_att_id att);
extern int  et_wakeup_all(et_sys_id id, et_stat_id stat_id);
extern int  et_attach_geteventsput(et_sys_id id, et_att_id att_id,
                                   uint64_t *events);
extern int  et_attach_geteventsget(et_sys_id id, et_att_id att_id,
                                   uint64_t *events);
extern int  et_attach_geteventsdump(et_sys_id id, et_att_id att_id,
                                    uint64_t *events);
extern int  et_attach_geteventsmake(et_sys_id id, et_att_id att_id,
                                    uint64_t *events);

/* station manipulation */
extern int  et_station_create_at(et_sys_id id, et_stat_id *stat_id,
                  const char *stat_name, et_statconfig sconfig,
		  int position, int parallelposition);
extern int  et_station_create(et_sys_id id, et_stat_id *stat_id,
                  const char *stat_name, et_statconfig sconfig);
extern int  et_station_remove(et_sys_id id, et_stat_id stat_id);
extern int  et_station_attach(et_sys_id id, et_stat_id stat_id,
                              et_att_id *att);
extern int  et_station_detach(et_sys_id id, et_att_id att);
extern int  et_station_setposition(et_sys_id id, et_stat_id stat_id, int position,
                                   int parallelposition);
extern int  et_station_getposition(et_sys_id id, et_stat_id stat_id, int *position,
                                   int *parallelposition);

/***************************************************************/
/* routines to store and read station config parameters used  */
/* to define a station upon its creation.                     */
/***************************************************************/
extern int et_station_config_init(et_statconfig* sconfig);
extern int et_station_config_destroy(et_statconfig sconfig);

extern int et_station_config_setblock(et_statconfig sconfig, int val);
extern int et_station_config_getblock(et_statconfig sconfig, int *val);

extern int et_station_config_setflow(et_statconfig sconfig, int val);
extern int et_station_config_getflow(et_statconfig sconfig, int *val);

extern int et_station_config_setselect(et_statconfig sconfig, int val);
extern int et_station_config_getselect(et_statconfig sconfig, int *val);

extern int et_station_config_setuser(et_statconfig sconfig, int val);
extern int et_station_config_getuser(et_statconfig sconfig, int *val);

extern int et_station_config_setrestore(et_statconfig sconfig, int val);
extern int et_station_config_getrestore(et_statconfig sconfig, int *val);

extern int et_station_config_setcue(et_statconfig sconfig, int val);
extern int et_station_config_getcue(et_statconfig sconfig, int *val);

extern int et_station_config_setprescale(et_statconfig sconfig, int val);
extern int et_station_config_getprescale(et_statconfig sconfig, int *val);

extern int et_station_config_setselectwords(et_statconfig sconfig, int val[]);
extern int et_station_config_getselectwords(et_statconfig sconfig, int val[]);

extern int et_station_config_setfunction(et_statconfig sconfig, const char *val);
extern int et_station_config_getfunction(et_statconfig sconfig, char *val);

extern int et_station_config_setlib(et_statconfig sconfig, const char *val);
extern int et_station_config_getlib(et_statconfig sconfig, char *val);

extern int et_station_config_setclass(et_statconfig sconfig, const char *val);
extern int et_station_config_getclass(et_statconfig sconfig, char *val);

/**************************************************************/
/*     routines to get & set existing station's parameters    */
/**************************************************************/
extern int et_station_isattached(et_sys_id id, et_stat_id stat_id, et_att_id att);
extern int et_station_exists(et_sys_id id, et_stat_id *stat_id, const char *stat_name);
extern int et_station_name_to_id(et_sys_id id, et_stat_id *stat_id, const char *stat_name);

extern int et_station_getattachments(et_sys_id id, et_stat_id stat_id, int *numatts);
extern int et_station_getstatus(et_sys_id id, et_stat_id stat_id, int *status);
extern int et_station_getinputcount(et_sys_id id, et_stat_id stat_id, int *cnt);
extern int et_station_getoutputcount(et_sys_id id, et_stat_id stat_id, int *cnt);
extern int et_station_getselect(et_sys_id id, et_stat_id stat_id, int *select);
extern int et_station_getlib(et_sys_id id, et_stat_id stat_id, char *lib);
extern int et_station_getclass(et_sys_id id, et_stat_id stat_id, char *classs);
extern int et_station_getfunction(et_sys_id id, et_stat_id stat_id, 
                                  char *function);

extern int et_station_getblock(et_sys_id id, et_stat_id stat_id, int *block);
extern int et_station_setblock(et_sys_id id, et_stat_id stat_id, int  block);

extern int et_station_getrestore(et_sys_id id, et_stat_id stat_id, int *restore);
extern int et_station_setrestore(et_sys_id id, et_stat_id stat_id, int  restore);

extern int et_station_getuser(et_sys_id id, et_stat_id stat_id, int *user);
extern int et_station_setuser(et_sys_id id, et_stat_id stat_id, int  user);

extern int et_station_getprescale(et_sys_id id, et_stat_id stat_id,int *prescale);
extern int et_station_setprescale(et_sys_id id, et_stat_id stat_id,int  prescale);

extern int et_station_getcue(et_sys_id id, et_stat_id stat_id, int *cue);
extern int et_station_setcue(et_sys_id id, et_stat_id stat_id, int  cue);

extern int et_station_getselectwords(et_sys_id id, et_stat_id stat_id, int select[]);
extern int et_station_setselectwords(et_sys_id id, et_stat_id stat_id, int select[]);


/*************************************************************/
/* routines to store and read system config parameters used  */
/* to define a system upon its creation.                     */
/*************************************************************/
extern int et_system_config_init(et_sysconfig *sconfig);
extern int et_system_config_destroy(et_sysconfig sconfig);

extern int et_system_config_setevents(et_sysconfig sconfig, int val);
extern int et_system_config_getevents(et_sysconfig sconfig, int *val);

extern int et_system_config_setsize(et_sysconfig sconfig, size_t val);
extern int et_system_config_getsize(et_sysconfig sconfig, size_t *val);

extern int et_system_config_settemps(et_sysconfig sconfig, int val);
extern int et_system_config_gettemps(et_sysconfig sconfig, int *val);

extern int et_system_config_setstations(et_sysconfig sconfig, int val);
extern int et_system_config_getstations(et_sysconfig sconfig, int *val);

extern int et_system_config_setprocs(et_sysconfig sconfig, int val);
extern int et_system_config_getprocs(et_sysconfig sconfig, int *val);

extern int et_system_config_setattachments(et_sysconfig sconfig, int val);
extern int et_system_config_getattachments(et_sysconfig sconfig, int *val);

extern int et_system_config_setgroups(et_sysconfig sconfig, int groups[], int size);

extern int et_system_config_setfile(et_sysconfig sconfig, const char *val);
extern int et_system_config_getfile(et_sysconfig sconfig, char *val);

/* does nothing, only here for backwards compatibility
extern int et_system_config_setcast(et_sysconfig sconfig, int val);
extern int et_system_config_getcast(et_sysconfig sconfig, int *val);
*/

extern int et_system_config_setport(et_sysconfig sconfig, int val);
extern int et_system_config_getport(et_sysconfig sconfig, int *val);

extern int et_system_config_setserverport(et_sysconfig sconfig, int val);
extern int et_system_config_getserverport(et_sysconfig sconfig, int *val);

/* deprecated, use ...addmulticast and ...removemulticast instead
extern int et_system_config_setaddress(et_sysconfig sconfig, const char *val);
extern int et_system_config_getaddress(et_sysconfig sconfig, char *val);
*/

extern int et_system_config_addmulticast(et_sysconfig sconfig, const char *val);
extern int et_system_config_removemulticast(et_sysconfig sconfig, const char *val);

extern int et_system_config_settcp(et_sysconfig sconfig, int rBufSize, int sBufSize, int noDelay);
extern int et_system_config_gettcp(et_sysconfig sconfig, int *rBufSize, int *sBufSize, int *noDelay);

/*************************************************************/
/*      routines to store & read system information          */
/*************************************************************/
extern int et_system_setgroup(et_sys_id id, int  group);
extern int et_system_getgroup(et_sys_id id, int *group);
extern int et_system_setdebug(et_sys_id id, int  debug);
extern int et_system_getdebug(et_sys_id id, int *debug);
extern int et_system_getlocality(et_sys_id id, int *locality);
extern int et_system_getnumevents(et_sys_id id, int *numevents);
extern int et_system_geteventsize(et_sys_id id, size_t *eventsize);
extern int et_system_gettempsmax(et_sys_id id, int *tempsmax);
extern int et_system_getstationsmax(et_sys_id id, int *stationsmax);
extern int et_system_getprocsmax(et_sys_id id, int *procsmax);
extern int et_system_getattsmax(et_sys_id id, int *attsmax);
extern int et_system_getheartbeat(et_sys_id id, int *heartbeat);
extern int et_system_getpid(et_sys_id id, pid_t *pid);
extern int et_system_getprocs(et_sys_id id, int *procs);
extern int et_system_getattachments(et_sys_id id, int *atts);
extern int et_system_getstations(et_sys_id id, int *stations);
extern int et_system_gettemps(et_sys_id id, int *temps);
extern int et_system_getserverport(et_sys_id id, int *port);
extern int et_system_gethost(et_sys_id id, char *host);
extern int et_system_getlocaladdress(et_sys_id id, char *address);

/*************************************************************/
/* routines to store and read system config parameters used  */
/* to open an ET system                                      */
/*************************************************************/
extern int et_open_config_init(et_openconfig *sconfig);
extern int et_open_config_destroy(et_openconfig sconfig);

extern int et_open_config_setwait(et_openconfig sconfig, int val);
extern int et_open_config_getwait(et_openconfig sconfig, int *val);

extern int et_open_config_setcast(et_openconfig sconfig, int val);
extern int et_open_config_getcast(et_openconfig sconfig, int *val);

extern int et_open_config_setTTL(et_openconfig sconfig, int val);
extern int et_open_config_getTTL(et_openconfig sconfig, int *val);

extern int et_open_config_setmode(et_openconfig sconfig, int val);
extern int et_open_config_getmode(et_openconfig sconfig, int *val);

extern int et_open_config_setdebugdefault(et_openconfig sconfig, int val);
extern int et_open_config_getdebugdefault(et_openconfig sconfig, int *val);

extern int et_open_config_setport(et_openconfig sconfig, int val);
extern int et_open_config_getport(et_openconfig sconfig, int *val);

extern int et_open_config_setserverport(et_openconfig sconfig, int val);
extern int et_open_config_getserverport(et_openconfig sconfig, int *val);

extern int et_open_config_settimeout(et_openconfig sconfig, struct timespec val);
extern int et_open_config_gettimeout(et_openconfig sconfig, struct timespec *val);

/* deprecated, use ...addbroadcast, ...addmulticast, etc. instead */
extern int et_open_config_setaddress(et_openconfig sconfig, const char *val);
extern int et_open_config_getaddress(et_openconfig sconfig, char *val);

extern int et_open_config_sethost(et_openconfig sconfig, const char *val);
extern int et_open_config_gethost(et_openconfig sconfig, char *val);

extern int et_open_config_addbroadcast(et_openconfig sconfig, const char *val);
extern int et_open_config_removebroadcast(et_openconfig sconfig, const char *val);

extern int et_open_config_addmulticast(et_openconfig sconfig, const char *val);
extern int et_open_config_removemulticast(et_openconfig sconfig, const char *val);

extern int et_open_config_setpolicy(et_openconfig sconfig, int val);
extern int et_open_config_getpolicy(et_openconfig sconfig, int *val);

extern int et_open_config_setinterface(et_openconfig sconfig, const char *val);
extern int et_open_config_getinterface(et_openconfig sconfig, char *val);

extern int et_open_config_settcp(et_openconfig sconfig, int rBufSize, int sBufSize, int noDelay);
extern int et_open_config_gettcp(et_openconfig sconfig, int *rBufSize, int *sBufSize, int *noDelay);


/*************************************************************
 * routines to store and read system bridge parameters used  *
 * to bridge (transfer events between) 2 ET systems          *
 *************************************************************/
extern int et_bridge_config_init(et_bridgeconfig *config);
extern int et_bridge_config_destroy(et_bridgeconfig sconfig);

extern int et_bridge_config_setmodefrom(et_bridgeconfig config, int val);
extern int et_bridge_config_getmodefrom(et_bridgeconfig config, int *val);

extern int et_bridge_config_setmodeto(et_bridgeconfig config, int val);
extern int et_bridge_config_getmodeto(et_bridgeconfig config, int *val);

extern int et_bridge_config_setchunkfrom(et_bridgeconfig config, int val);
extern int et_bridge_config_getchunkfrom(et_bridgeconfig config, int *val);

extern int et_bridge_config_setchunkto(et_bridgeconfig config, int val);
extern int et_bridge_config_getchunkto(et_bridgeconfig config, int *val);

extern int et_bridge_config_settimeoutfrom(et_bridgeconfig config, struct timespec val);
extern int et_bridge_config_gettimeoutfrom(et_bridgeconfig config, struct timespec *val);

extern int et_bridge_config_settimeoutto(et_bridgeconfig config, struct timespec val);
extern int et_bridge_config_gettimeoutto(et_bridgeconfig config, struct timespec *val);

extern int et_bridge_config_setfunc(et_bridgeconfig config, ET_SWAP_FUNCPTR func);

extern int et_events_bridge(et_sys_id id_from, et_sys_id id_to,
		     et_att_id att_from, et_att_id att_to,
		     et_bridgeconfig bconfig,
		     int num, int *ntransferred);

#ifdef	__cplusplus
}
#endif

#endif
