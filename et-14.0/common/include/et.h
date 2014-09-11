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

#if  !(defined VXWORKS || defined sun)
#include <stdint.h>
#endif


#ifdef VXWORKS

#include <vxWorks.h>
#include <time.h>
#include <semLib.h>
#define INET_ATON_ERR   ERROR

#else

#define INET_ATON_ERR   0
#include <inttypes.h>

#endif

#ifdef	__cplusplus
extern "C" {
#endif


/*********************************************************************/
/*            The following default values may be changed            */
/*********************************************************************/
/* station stuff */
#define ET_STATION_CUE      10	 /* max # events cued in nonblocking stat */
#define ET_STATION_PRESCALE 1	 /* prescale of blocking stations */

/* system stuff */
#define ET_SYSTEM_EVENTS    300	 /* total # of events */
#define ET_SYSTEM_NTEMPS    300	 /* max # of temp events */
#define ET_SYSTEM_ESIZE     1000 /* size of normal events in bytes */
#define ET_SYSTEM_NSTATS    200	 /* max # of stations */

/* network stuff */
#define ET_MULTICAST_PORT  11111 /* udp multicast port */
#define ET_BROADCAST_PORT  11111 /* udp broadcast port */
#define ET_SERVER_PORT     11111 /* tcp server port */

#define ET_MULTICAST_ADDR  "239.200.0.0"    /* random multicast addr */
#define ET_BROADCAST_ADDR  "129.57.29.255"  /* jlab daq group subnet */
    
#define ET_EVENT_GROUPS_MAX 500 /* max # of groups events are divided into */

/*********************************************************************/
/*                              WARNING                              */
/* Changing ET_STATION_SELECT_INTS requires a full recompile of all  */
/* ET libraries (local & remote) whose users interact in any way.    */
/*********************************************************************/
/* # of control integers associated with each station */
#define ET_STATION_SELECT_INTS 6

/*********************************************************************/
/*             DO NOT CHANGE ANYTHING BELOW THIS LINE !!!            */
/*********************************************************************/

/* default multicast time-to-live value */
#define ET_MULTICAST_TTL   32

/* ET server is on host that is ... */
#define ET_HOST_LOCAL      ".local"
#define ET_HOST_REMOTE     ".remote"
#define ET_HOST_ANYWHERE   ".anywhere"

/* treat local ET server as local or remote? */
#define ET_HOST_AS_LOCAL   1
#define ET_HOST_AS_REMOTE  0

/* policy to pick a single responder from many after broad/multicast */
#define ET_POLICY_FIRST  0   /* pick first responder */
#define ET_POLICY_LOCAL  1   /* pick local responder first, else first */
#define ET_POLICY_ERROR  2   /* return an error if more than one responder */

/* name lengths */
#define ET_FILENAME_LENGTH  100 /* max length of ET system file name + 1 */
#define ET_FUNCNAME_LENGTH  48  /* max length of user's 
                                   event selection func name + 1 */
#define ET_STATNAME_LENGTH  48  /* max length of station name + 1 */
/* max length of temp event file name + 1, leaves room for _temp<#######> */
#define ET_TEMPNAME_LENGTH  (ET_FILENAME_LENGTH+12)

/* STATION STUFF */
/* et_stat_id of grandcentral station */
#define ET_GRANDCENTRAL 0

/* status */
#define ET_STATION_UNUSED   0
#define ET_STATION_CREATING 1
#define ET_STATION_IDLE     2
#define ET_STATION_ACTIVE   3

/* how many user process can attach */
#define ET_STATION_USER_MULTI  0
#define ET_STATION_USER_SINGLE 1

/* can block for event flow or not */
#define ET_STATION_NONBLOCKING 0
#define ET_STATION_BLOCKING    1

/* criterion for event selection */
#define ET_STATION_SELECT_ALL      1     /* no conditions */
#define ET_STATION_SELECT_MATCH    2     /* matches predefined selection rule */
#define ET_STATION_SELECT_USER     3     /* user supplied selection rule */
#define ET_STATION_SELECT_RROBIN   4     /* round robin distribution of events to 
                                            parallel stations */
#define ET_STATION_SELECT_EQUALCUE 5     /* equal-number-of-events-in-cue distribution
                                            algorithm for parallel stations */

/* how to restore events in dead user process */
#define ET_STATION_RESTORE_OUT 0	/* to output list */
#define ET_STATION_RESTORE_IN  1	/* to input  list */
#define ET_STATION_RESTORE_GC  2    /* to gc station input list */
#define ET_STATION_RESTORE_REDIST 3 /* redistribute to input of all parallel stations,
                                       along with all events left in station's input list */

/* how events flow through a (group of) station(s) */
#define ET_STATION_SERIAL        0  /* normal flow of all events through all stations */
#define ET_STATION_PARALLEL      1  /* station is part of a group of stations in which
                                       events are flowing in parallel. */
#define ET_STATION_PARALLEL_HEAD 2  /* station is head of a group of stations in which
                                       events are flowing in parallel. Only used when
                                       gathering monitoring data of stations (not for config).*/

/* errors */
#define ET_OK              0
#define ET_ERROR          -1
#define ET_ERROR_TOOMANY  -2
#define ET_ERROR_EXISTS   -3
#define ET_ERROR_WAKEUP   -4
#define ET_ERROR_TIMEOUT  -5
#define ET_ERROR_EMPTY    -6
#define ET_ERROR_BUSY     -7
#define ET_ERROR_DEAD     -8
#define ET_ERROR_READ     -9
#define ET_ERROR_WRITE    -10
#define ET_ERROR_REMOTE   -11
#define ET_ERROR_NOREMOTE -12
#define ET_ERROR_TOOBIG   -13
#define ET_ERROR_NOMEM    -14
#define ET_ERROR_BADARG   -15
#define ET_ERROR_SOCKET   -16
#define ET_ERROR_NETWORK  -17
#define ET_ERROR_CLOSED   -18 /* Changed, Added */

/* debug levels */
#define ET_DEBUG_NONE   0
#define ET_DEBUG_SEVERE 1
#define ET_DEBUG_ERROR  2
#define ET_DEBUG_WARN   3
#define ET_DEBUG_INFO   4

/* event priority */
#define ET_LOW  0
#define ET_HIGH 1
#define ET_PRIORITY_MASK 1	/* used to send bits over network */

/* event owner is attachment # unless owned by system */
#define ET_SYS -1
/* when creating a new station, put at end of linked list */
#define ET_END -1
/* when creating a new parallel station, put at head of a
 * new group of parallel stations and in the main linked list */
#define ET_NEWHEAD -2

/* event is temporary or not */
#define ET_EVENT_TEMP   1
#define ET_EVENT_NORMAL 0

/* event is new (obtained thru et_event(s)_new) or not */
#define ET_EVENT_NEW  1
#define ET_EVENT_USED 0

/* event get/new routines' wait mode flags */
#define ET_SLEEP  0
#define ET_TIMED  1
#define ET_ASYNC  2
#define ET_WAIT_MASK  3		/* mask mode's wait info */
/* event get/new routines' event flow flags */
#define ET_MODIFY 4		/* remote user will modify event */
#define ET_MODIFY_HEADER 8	/* remote user will modify only header */
#define ET_DUMP          16	/* remote user want events automatically dumped (not put) */
/* event get/new routines' memory allocation flags */
#define ET_NOALLOC       32	/* remote user will not allocate mem but use existing buf */

/* et_open waiting modes */
#define ET_OPEN_NOWAIT 0
#define ET_OPEN_WAIT   1

/* are we remote or local? */
#define ET_REMOTE        0
#define ET_LOCAL         1
#define ET_LOCAL_NOSHARE 2

/* use broadcast, multicast, both or direct connection to find server port */
#define ET_MULTICAST 0
#define ET_BROADCAST 1
#define ET_DIRECT    2
#define ET_BROADANDMULTICAST 3

/* options for et_open_config_add(remove)broadcast */
#define ET_SUBNET_ALL  ".subnetsAll"     /* all subnets host is on */

/* status of data in an event */
#define ET_DATA_OK               0
#define ET_DATA_CORRUPT          1
#define ET_DATA_POSSIBLY_CORRUPT 2
#define ET_DATA_MASK		 0x30	/* for network use */
#define ET_DATA_SHIFT		 4	/* for network use */

/* endian values */
#define ET_ENDIAN_BIG      0	/* big endian */
#define ET_ENDIAN_LITTLE   1	/* little endian */
#define ET_ENDIAN_LOCAL    2	/* same endian as local host */
#define ET_ENDIAN_NOTLOCAL 3	/* opposite endian as local host */
#define ET_ENDIAN_SWITCH   4	/* switch recorded value of data's endian */

/* do we swap data or not? */
#define ET_NOSWAP 0
#define ET_SWAP   1

/* macros for swapping ints of various sizes */
#define ET_SWAP64(x) ( (((x) >> 56) & 0x00000000000000FFL) | \
                       (((x) >> 40) & 0x000000000000FF00L) | \
                       (((x) >> 24) & 0x0000000000FF0000L) | \
                       (((x) >> 8)  & 0x00000000FF000000L) | \
                       (((x) << 8)  & 0x000000FF00000000L) | \
                       (((x) << 24) & 0x0000FF0000000000L) | \
                       (((x) << 40) & 0x00FF000000000000L) | \
                       (((x) << 56) & 0xFF00000000000000L) )

#define ET_SWAP32(x) ( (((x) >> 24) & 0x000000FF) | \
                       (((x) >> 8)  & 0x0000FF00) | \
                       (((x) << 8)  & 0x00FF0000) | \
                       (((x) << 24) & 0xFF000000) )

#define ET_SWAP16(x) ( (((x) >> 8) & 0x00FF) | \
                       (((x) << 8) & 0xFF00) )


/* see "Programming with POSIX threads' by Butenhof */
#define err_abort(code,text) do { \
    fprintf (stderr, "%s at \"%s\":%d: %s\n", \
        text, __FILE__, __LINE__, strerror (code)); \
    exit (-1); \
} while (0)


/* STRUCTURES */

/*
 * et_event:
 *-----------------------------
 * next     : next event in linked list
 * tempdata : for Linux or non mutex-sharing systems, a temp event
 *          : gotten by the server thread needs its data pointer stored
 *          : while the user maps the temp file and puts that pointer
 *          : in ev->pdata. When converting the pdata pointer back to
 *          : a value the ET system uses, so the server thread can
 *          : unmap it from memory, grab & restore the stored value.
 * pdata    : = pointer below (shared mem), or points to temp event buffer
 * data     : pointer to shared memory data
 * length   : size of actual data in bytes
 * memsize  : total size of available data memory in bytes
 * pointer  : for REMOTE events- pointer to this event in the
 *          : server's space. Used for writing of modified events.
 * modify   : when "getting" events from a remote client - flag to tell
 *          : server whether this event will be modified with the
 *          : intention of sending it back to the server (ET_MODIFY) or
 *          : whether only the header will be modified and returned
 *          : (ET_MODIFY_HEADER) or whether there'll be no modification
 *          : of the event (0).
 * priority : ET_HIGH or ET_LOW
 * owner    : # of attachment that owns it, else ET_EVENT_SYS (-1)
 * temp     : =ET_EVENT_TEMP if temporary event, else =ET_EVENT_NORMAL
 * age      : =ET_EVENT_NEW if it's a new event, else =ET_EVENT_USED
 *          : New events always go to GrandCentral if user crashes.
 * datastatus: =ET_DATA_OK normally, =ET_DATA_CORRUPT if data corrupt,
 *          : =ET_DATA_POSSIBLY_CORRUPT if data questionable.
 * byteorder: use to track the data's endianness (set to 0x04030201)
 * group    : group number - events are divided into groups for limiting
 *          : the number of events producers have access to
 * place    : place number of event's relative position in shared mem starting
 *          : with 0 (lowest memory position) and ending with (# of events - 1).
 * control  : array of ints to select on (see et_stat_config->select)
 * filename : filename of temp event
 */
 
typedef struct et_event_t {
  struct et_event_t *next;
  void    *tempdata;
  void    *pdata;
  char    *data;
  uint64_t length;
  uint64_t memsize;
  /* for remote events */
  uint64_t pointer;
  int      modify;
  /* ***************** */
  int     priority;
  int     owner;
  int     temp;
  int     age;
  int     datastatus;
  int     byteorder;
  int     group;
  int     place;
  int     control[ET_STATION_SELECT_INTS];
  char    filename[ET_TEMPNAME_LENGTH];
} et_event;

/* for data swapping function */
typedef int (*ET_SWAP_FUNCPTR) (et_event *, et_event*, int, int);

/* TYPES */
typedef void *et_sys_id;
typedef void *et_statconfig;
typedef void *et_sysconfig;
typedef void *et_openconfig;
typedef void *et_bridgeconfig;
typedef int   et_stat_id;
typedef int   et_proc_id;
typedef int   et_att_id;

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

/* swap CODA data functions */
extern int  et_event_CODAswap(et_event *pe);

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

/* does nothing, only here for backwards compatibility */
extern int et_system_config_setcast(et_sysconfig sconfig, int val);
extern int et_system_config_getcast(et_sysconfig sconfig, int *val);

extern int et_system_config_setport(et_sysconfig sconfig, int val);
extern int et_system_config_getport(et_sysconfig sconfig, int *val);

extern int et_system_config_setserverport(et_sysconfig sconfig, int val);
extern int et_system_config_getserverport(et_sysconfig sconfig, int *val);

/* deprecated, use ...addmulticast and ...removemulticast instead */
extern int et_system_config_setaddress(et_sysconfig sconfig, const char *val);
extern int et_system_config_getaddress(et_sysconfig sconfig, char *val);

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

extern int et_open_config_setmultiport(et_openconfig sconfig, int val);
extern int et_open_config_getmultiport(et_openconfig sconfig, int *val);

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
extern int et_bridge_CODAswap(et_event *src_ev, et_event *dest_ev, int bytes, int same_endian);

extern int et_events_bridge(et_sys_id id_from, et_sys_id id_to,
		     et_att_id att_from, et_att_id att_to,
		     et_bridgeconfig bconfig,
		     int num, int *ntransferred);

#ifdef	__cplusplus
}
#endif

#endif
