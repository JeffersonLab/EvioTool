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

#ifndef __et_private_h
#define __et_private_h

#if  !(defined VXWORKS || defined sun)
#include <stdint.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <pthread.h>

#include "etCommonNetwork.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The data-type model for this architecture is set
 * by solaris to _LP64 or _ILP32. Linux defines 
 * __WORDSIZE to be 64 or 32. vxworks seems to do
 * nothing so define that as ILP32.
 */
#ifdef linux
  #if __WORDSIZE == 64
    #define _LP64 1
  #else
    #define _ILP32 1
  #endif
#elif defined VXWORKS
  #define _ILP32 1
#endif

/* Version Number of this ET software package release */
#define ET_VERSION 14      /* treated as an int */
#define ET_VERSION_MINOR 0 /* treated as an int */

/* Language of ET software package - implementation 
 * varies a bit from language to language.
 */
#define ET_LANG_C     0      /* C    */
#define ET_LANG_CPP   1      /* C++  */
#define ET_LANG_JAVA  2      /* Java */

/* Type of ET system (shared memory layout) */
#define ET_SYSTEM_TYPE_C     1      /* C    */
#define ET_SYSTEM_TYPE_JAVA  2      /* Java */

/*
 * String length of dotted-decimal, ip address string
 * Some systems - but not all - define INET_ADDRSTRLEN
 * ("ddd.ddd.ddd.ddd\0" = 16)
 */
#define ET_IPADDRSTRLEN 16

/*
 * MAXHOSTNAMELEN is defined to be 256 on Solaris and is the max length
 * of the host name so we add one for the terminator. On Linux the
 * situation is less clear but 257 appears to be the max (whether that
 * includes termination is not clear).
 * We need it to be uniform across all platforms since we transfer
 * this info across the network. Define it to be 256 for everyone.
 */
#define ET_MAXHOSTNAMELEN 256


/* *****************************************************************
 *      items to handle multiple network addresses or names        *
 * *****************************************************************/
/* max # of network addresses/names per host we'll examine */
#define ET_MAXADDRESSES 10

/* ***************************************
 * times for heart beating & monitoring *
 * ***************************************/
/* 1.6 sec */
#define ET_MON_SEC    1
#define ET_MON_NSEC   600000000
/* 0.5 sec */
#define ET_BEAT_SEC   0
#define ET_BEAT_NSEC  500000000

/* max value for heartbeats */
#define ET_HBMODULO UINT_MAX

/* # of extra POSIX threads ET client/producer will start */
#define ET_EXTRA_THREADS 2

/*
 * Maximum # of user attachments/processes on the system.
 * This is used only to allocate space in the system structure
 * for attachment/process housekeeping. The actual maximum #s of
 * attachments & processes are passed in a et_sys_config structure
 * to et_system_start.
 *
 * The number of attachments is the number of "et_station_attach"
 * calls successfully returned - each providing an access to events
 * from a certain station. A unix process may make several attachments
 * to one or more stations on one or more ET systems.
 *
 * The number of processes is just the number of "et_open" calls
 * successfully returned on the same host as the ET system. The
 * system must be one which can allow many processes to share the
 * ET mutexes, thus allowing full access to the shared memory. On
 * Linux this is not possible, meaning that the local user does NOT
 * have the proper access to the shared memory.
 * 
 * Since a unix process may only call et_open once, as things now stand,
 * the number of processes is a count of the number of unix processes
 * on Solaris opened locally.
 */
#define ET_ATTACHMENTS_MAX 110
#define ET_PROCESSES_MAX   ET_ATTACHMENTS_MAX

/* status of an attachment to an ET system */
#define ET_ATT_UNUSED 0         /* attachment # not in use */
#define ET_ATT_ACTIVE 1         /* attached to a station */

/* values to tell attachment to return prematurely from ET routine */
#define ET_ATT_CONTINUE 0       /* situation normal */
#define ET_ATT_QUIT     1       /* return immediately */

/* values telling whether an attachment is blocked in a read or not */
#define ET_ATT_UNBLOCKED 0      /* not blocked */
#define ET_ATT_BLOCKED   1      /* blocked */

/* values telling whether an attachment is in simulated sleep mode or not */
#define ET_ATT_NOSLEEP 0        /* not in sleep mode */
#define ET_ATT_SLEEP   1        /* simultated sleep mode */

/* status of a process in regards to an ET system */
#define ET_PROC_CLOSED 0        /* closed to ET systems - no access */
#define ET_PROC_OPEN   1        /* opened to ET system  - access to mapped mem */

/* what a process thinks of its ET system's status */
#define ET_PROC_ETDEAD 0        /* ET system it is connected to is dead */
#define ET_PROC_ETOK   1        /* ET system it is connected to is OK   */

/* certain structures need to be initialized before use */
#define ET_STRUCT_NEW 0         /* struct is newly created and not yet initialized */
#define ET_STRUCT_OK  1         /* struct is initialized and ready for use */

/* values to tell thread to self-destruct or to stick around */
#define ET_THREAD_KEEP 0        /* keep thread around */
#define ET_THREAD_KILL 1        /* remove thread */

/* tells if operating sys can share pthread mutexes between processes */
#define ET_MUTEX_SHARE   0      /* can share mutexes */
#define ET_MUTEX_NOSHARE 1      /* cannot share */

/* is mutex locked or not */
#define ET_MUTEX_UNLOCKED 0
#define ET_MUTEX_LOCKED   1

/* Size in bytes of the data stored at the beginning of the shared memory.
 * These data give general info necessary for Java (and C clients too) to
 * mmap this file properly.*/
#define ET_INITIAL_SHARED_MEM_DATA_BYTES 64

/*
 * STRUCTURES for the STATIONs:
 * --------------------------
 *  structures          purpose
 * ____________        ________________
 *  et_stat_config      station behavior parameters
 *  et_stat_data        station information
 *  et_event            single event
 *  et_list             list of events
 *  et_station          processes can "attach" to station
 *                      to get and put events
 */


/** For event selection function */
typedef int (*ET_SELECT_FUNCPTR) (void *, et_stat_id, et_event *);


/**
 * et_stat_config: parameters to define a station
 *-----------------------------
 * init           : ET_STRUCT_OK if structure properly initialized, else ET_STRUCT_NEW
 * flow_mode      : ET_STATION_PARALLEL if station part of a group of stations through
 *                : which events flow in parallel instead of through each one.
 *                : ET_STATION_SERIAL is default as events flow through each station.
 * user_mode      : multiple or single process(es) can connect
 * restore_mode   : if process dies, events it read but didn't write can be sent to:
 *                : 1) station's ouput with ET_STATION_RESTORE_OUT,
 *                : 2) station's input with ET_STATION_RESTORE_IN, or
 *                : 3) grand_central station with ET_STATION_RESTORE_GC
 * block_mode     : ET_STATION_BLOCKING means station blocks (accepts every event which
 *                : meets its condition) or
 *                : ET_STATION_NONBLOCKING means it doesn't (keeps some in a cue)
 * prescale       : for blocking,    get every Nth event
 * cue            : for nonblocking, # events desired in input list
 * select_mode    : ET_STATION_SELECT_ALL      - accepts every event (no conditions)
 *                : ET_STATION_SELECT_MATCH    - events' control array must match stations'
 *                :                              select array by predefined rules
 *                : ET_STATION_SELECT_USER     - user supplies lib & func to select events
 *                : ET_STATION_SELECT_RROBIN   - round robin distribution of events to
 *                :                              parallel stations
 *                : ET_STATION_SELECT_EQUALCUE - equal-number-of-events-in-cue distribution
 *                :                              algorithm for parallel stations
 * select         : array of ints for user's event selection
 *
 * for "C" systems, fname & lib are used to dynamically load code
 *
 * fname          : if user is providing a routine to define the
 *                : condition of event selection, this is its name
 * lib            : name of shared lib containing above routine
 *
 * for JAVA systems, a class name is used to dynamically load code
 *
 * classs         : name of JAVA class containing method to define the
 *                : condition of event selection (extra s is NOT misspelling)
 */
typedef struct et_stat_config_t {
  int  init;
  int  flow_mode;
  int  user_mode;
  int  restore_mode;
  int  block_mode;
  int  prescale;
  int  cue;
  int  select_mode;
  int  select[ET_STATION_SELECT_INTS];
  char fname[ET_FUNCNAME_LENGTH];
  char lib[ET_FILENAME_LENGTH];
  char classs[ET_FILENAME_LENGTH];
} et_stat_config;


/**
 * et_stat_data : current state of station
 *-----------------------------
 * status       : active, idle, creating, or unused
 * pid_create   : pid of process that created station
 * nattachments : # of attachments to this station
 * att          : array in which the unique id# of an attachment
 *              : is the index (=id# if attached, -1 otherwise)
 * lib_handle   : handle for opened shared lib for user defined
 *              : event selection routine
 * func         : pointer to user's event selection routine
 */ 
typedef struct et_stat_data_t {
  int   status;
  int   pid_create;
  int   nattachments;
  int   att[ET_ATTACHMENTS_MAX];
  void *lib_handle;
  ET_SELECT_FUNCPTR func;
} et_stat_data;


/**
 * et_list: station's input or output list of events
 *-----------------------------
 * cnt          : # events in list
 * lasthigh     : place in list of last high priority event
 * events_try   : # of events tried to put in (before prescale)
 * events_in;   : # of events actually put in
 * events_out;  : # of events actually sent out
 * firstevent   : pointer to first event in linked list
 * lastevent    : pointer to last  event in linked list
 * mutex        : protects linked list when reading & writing
 * cread        : condition var. to notify reader that events are here
 */ 
typedef struct et_list_t {
  int              cnt;
  int              lasthigh;
  uint64_t         events_try;
  uint64_t         events_in;
  uint64_t         events_out;
  et_event        *firstevent;
  et_event        *lastevent;
  pthread_mutex_t  mutex;
  pthread_cond_t   cread;
} et_list;


#define ET_FIX_READ 0
#define ET_FIX_DUMP 1

/** Struct for fixing input list */
struct et_fixin {
  et_event          *first;
  uint64_t           eventsin;
  int                start;
  int                cnt;
  int                num;
  int                call;
};


/** Struct for fixing output list */
struct et_fixout {
  int                start;
  int                cnt;
  int                num;
};


/**
 * et_fix: struct to fix station's input and output linked lists after crash
 *--------------------------------------------------------------
 * first        : pl->firstevent at start of read (NULL if no damage)
 * eventsin     : value of pl->events_in at start of dump
 * start        : = 1 at start of write (0 at end)
 * cnt          : pl->cnt at start of read/write
 * num          : num of events intended to read/write
 * call         : =ET_FIX_DUMP if fixing after et_station_(n)dump call or
 *              : =ET_FIX_READ if fixing after et_station_(n)read call
 */
struct et_fix {
  struct et_fixin  in;
  struct et_fixout out;
};


/**
 * et_station:
 * Use et_stat_id as a user's handle on a station instead
 * of a pointer to the station itself. It allows
 * a fast calculation of a pointer to station.
 *-----------------------------
 * num            : unique id # (only if station being used)
 *                : = 0 for first station (grandcentral), = 1 for next
 *                : station in mapped memory, etc.
 * conductor      : flag to kill conductor thread when station deleted 
 *                : = ET_THREAD_KILL else ET_THREAD_KEEP
 * next           : next active or idle station in et station chain,
 *                : Not storing this as a pointer makes for an awkward
 *                : linked list, but it survives mapping the shared
 *                : memory to a different spot.
 *                : For last station this is -1.
 * prev           : previous active or idle station in et station chain,
 *                : For first station this is -1.
 * nextparallel   : if this station is in a group of parallel stations,
 *                : this is a "pointer" to the next parallel station.
 *                : For last station this is -1.
 * prevparallel   : if this station is in a group of parallel stations,
 *                : this is a "pointer" to the previous parallel station.
 *                : For first station this is -1.
 * waslast        : flag which =1 if this station was last one to receive
 *                : an event when using the round-robin selection method
 *                : for a parallel group of stations. (else =0).
 * name           : unique name
 * mutex          : mutex used for keeping the linked list of used
 *                : stations in order (for event transfers).
 * fix            : info to repair station's lists after user crash
 * data           : status, info
 * config         : configuration specs
 * list_in        : linked list containing events to read
 * list_out       : linked list containing events to be written
 */
typedef struct et_station_t {
  et_stat_id            num;
  int                   conductor;
  et_stat_id            next;
  et_stat_id            prev;
  et_stat_id            nextparallel;
  et_stat_id            prevparallel;
  int                   waslast;
  char                  name[ET_STATNAME_LENGTH];
  pthread_mutex_t       mutex;
  struct et_fix         fix;
  et_stat_data          data;
  et_stat_config        config;
  et_list               list_in;
  et_list               list_out;
} et_station;


/**
 * et_proc: contains process info
 *----------------------------------------------------------
 * num            : unique index # of this process
 * att            : array in which an element (indexed by the
 *                : unique id# of an attachment owned by this process)
 *                : gives the id# of the attachment and -1 otherwise.
 * nattachments   : # of attachments to an ET system in this process
 * status         : open or connected to ET system (ET_PROC_OPEN),
 *                : or closed/unconnected (ET_PROC_CLOSED)
 * et_status      : knows ET system is dead (ET_PROC_ETDEAD),
 *                : or not (ET_PROC_ETOK)
 * heartbeat      : incremented to tell ET system it's alive
 * pid            : unix process id
 * hbeat_thd_id   : heartbeat thread id
 * hmon_thd_id    : heart monitor thread id
 */
struct et_proc {
  et_proc_id    num;
  et_att_id     att[ET_ATTACHMENTS_MAX];
  int           nattachments;
  int           status;
  int           et_status;
  unsigned int  heartbeat;
  pid_t         pid;
  pthread_t     hbeat_thd_id;
  pthread_t     hmon_thd_id;
};


/**
 * et_attach: contains attachment info
 *----------------------------------------------------------
 * num            : unique index # of this attachment
 * proc           : unique index # of process owning this attachment
 * stat           : unique index # of station we're attached to
 * status         : unused or active (ET_ATT_UNUSED, ET_ATT_ACTIVE)
 * blocked        : = ET_ATT_BLOCKED if blocked waiting to read events,
 *                : else = ET_ATT_UNBLOCKED
 * quit           : = ET_ATT_QUIT means return from ET routine
 *                : = ET_ATT_CONTINUE means things are OK
 * sleep          : if = ET_ATT_SLEEP, then attachment is remote and is sleeping
 *                : when getting events. It is simulated by multiple timed waits.
 *                : This causes trouble when waking attachments and must be
 *                : accounted for.
 *                : if = ET_ATT_NOSLEEP, no simulated remote sleeping mode.
 * events_put     : # of events it puts to   station output list
 * events_get     : # of events it gets from station input  list
 * events_dump    : # of events it dumps back into GrandCentral (recycles)
 * events_make    : # of new events it requests
 * pid            : unix process id# of process that owns attachment
 * host           : hostname running process that owns attachment
 * interface      : ip address of outgoing network interface (IPv4)
 */ 
struct et_attach {
  et_att_id   num;
  et_proc_id  proc;
  et_stat_id  stat;
  int         status;
  int         blocked;
  int         quit;
  int         sleep;
  uint64_t    events_put;
  uint64_t    events_get;
  uint64_t    events_dump;
  uint64_t    events_make;
  pid_t       pid;
  char        host[ET_MAXHOSTNAMELEN];
  char        interface[ET_IPADDRSTRLEN];
};


/**
 * et_sys_config  : Contains all info necessary to configure
 *                : an ET system.
 *----------------------------------------------------------
 * event_size     : event size in bytes
 * init           : =ET_STRUCT_OK if structure properly initialized
 * nevents        : total # of events
 * ntemps         : max # of temporary events allowed (<= nevents)
 * nstations      : max # of stations allowed (including GRAND CENTRAL)
 * nprocesses     : max # of processes allowed to open ET system
 * nattachments   : max # of attachments to stations allowed
 * groupCount     : # of event groups
 * groups         : array in which index is the group number and value is the
 *                : number of events in that group. There are "groupCount"
 *                : number of valid groups.
 * filename       : name of the ET system file
 * port           : broad/multicast port # for udp message
 * serverport     : port # for ET system's tcp server thread
 * tcpSendBufSize : size in bytes of TCP send    buffer of socket connecting to ET client
 * tcpRecvBufSize : size in bytes of TCP receive buffer of socket connecting to ET client
 * tcpNoDelay     : if 0, sockets have TCP_NODELAY option off, else on
 * netinfo        : holds all IP info
 * bcastaddrs     : holds all local subnet broadcast addrs (dotted-decimal)
 * mcastaddrs     : holds all multicast addresses to listen on (dotted-dec)
 */
typedef struct  et_sys_config_t {
  uint64_t        event_size;
  int             init;
  int             nevents;
  int             ntemps;
  int             nstations;
  int             nprocesses;
  int             nattachments;
  int             groupCount;
  int             groups[ET_EVENT_GROUPS_MAX];
  char            filename[ET_FILENAME_LENGTH];
  /* for remote use */
  int               port;
  int               serverport;
  int               tcpSendBufSize;
  int               tcpRecvBufSize;
  int               tcpNoDelay;
  codaNetInfo       netinfo;
  codaDotDecIpAddrs bcastaddrs;
  codaDotDecIpAddrs mcastaddrs;
} et_sys_config;


/* Macros to handle the bitInfo word in et_system structure following */
#define ET_BIT64_MASK 0x1
#define ET_KILL_MASK  0x2
#define ET_GET_BIT64(x)  ((x) & ET_BIT64_MASK)
#define ET_GET_KILL(x)   ((x) & ET_KILL_MASK)
#define ET_SET_BIT64(x)  ((x) | ET_BIT64_MASK)
#define ET_SET_KILL(x)   ((x) | ET_KILL_MASK)


/**
 * et_system: contains all ET system information
 *----------------------------------------------------------
 * The next two items are first in the structure so they are the
 * first items in shared memory. The ET version number must be in
 * this same position for each verion of the ET software so
 * comparisons can be made between versions.
 *
 * version        : version # of this ET software release
 * nselects       : current # of selection ints per station (or control ints
 *                : per event)
 * bitInfo        : Least significant bit = 1 if ET system created by 64 bit executable, else 0.
 *                : Next bit = 1 if ET system is being told to kill itself, else 0
 * asthread       : flag to kill addstat thread (ET_THD_KILL)
 * heartbeat      : increment to indicate I'm alive
 * hz             : system clock rate
 * nstations      : current # of stations idle or active
 * ntemps         : current # of temp events existing
 * nprocesses     : current # of processes in system
 * nattachments   : current # of attachments in system
 * port           : port # for tcp server thread of the ET system
 * stat_head      : head of linked list of used stations (not a pointer)
 * stat_tail      : tail of linked list of used stations (not a pointer)
 *                : Not storing these as pointers makes for an awkward
 *                : linked list, but they survive mapping the shared
 *                : memory to a different spot.
 * mainpid        : unix pid of ET system
 * con_add        : concurrency added to process calling et_system_start
 * pmap           : ptr to mapped mem in ET system's process
 *                : This is here so user-called routines can
 *                : read/write pointers correctly from shared mem
 * mutex          : protect system  data in changes
 * stat_mutex     : protect station data in changes
 * statadd_mutex  : used to add stations one at a time
 * statadd        : cond. var. used to add new stations
 * statdone       : cond. var. used to signal end of station creation
 * tid_hb         : sys heartbeat thread id
 * tid_m          : sys heartmonitor thread id
 * tid_as         : sys add station thread id
 * tid_srv        : ET server tcp thread id
 * tid_mul        : id of thread which spawns broad/multicast listening
 *                : threads
 * host           : host of the ET system
 * proc           : array of info on processes
 * attach         : array of info on attachments
 * config         : parameters used to create ET system
 */ 
typedef struct et_system_t {
  int              version;
  int              nselects;
  int              bitInfo;
  int              asthread;
  unsigned int     heartbeat;
  int              hz;
  int              nstations;
  int              ntemps;
  int              nprocesses;
  int              nattachments;
  int              port;
  et_stat_id       stat_head;
  et_stat_id       stat_tail;
  pid_t            mainpid;
#ifdef sun
  int              con_add;
#endif
  void            *pmap;
  pthread_mutex_t  mutex;
  pthread_mutex_t  stat_mutex;
  pthread_mutex_t  statadd_mutex;
  pthread_cond_t   statadd;
  pthread_cond_t   statdone;
  pthread_t        tid_hb;
  pthread_t        tid_hm;
  pthread_t        tid_as;
  pthread_t        tid_srv;
  pthread_t        tid_mul;
  char             host[ET_MAXHOSTNAMELEN];
  struct et_proc   proc[ET_PROCESSES_MAX];
  struct et_attach attach[ET_ATTACHMENTS_MAX];
  et_sys_config    config;
} et_system;


/**
 * et_open_config : parameters used to open an ET system
 *-----------------------------
 * init          : =ET_STRUCT_OK if structure properly initialized
 * wait          : =ET_OPEN_WAIT or ET_OPEN_NOWAIT depending on whether
 *               :  user wants to wait for ET system to appear or not.
 * cast          : =ET_BROADCAST for users to discover host & port # of
 *               :  ET system server by broadcasting
 *               : =ET_MULTICAST for users to discover host & port # of
 *               :  ET system server by multicasting
 *               : =ET_BROADANDMULTICAST for users to discover host & port # of
 *               :  ET system server by both broad and multicasting
 *               : =ET_DIRECT when users specify host & port # of ET system
 * ttl           : multicast ttl value (sets scope of multicast)
 * mode          : =ET_HOST_AS_REMOTE or ET_HOST_AS_LOCAL depending on
 *               : whether connections to a local ET system are made as if
 *               : the client were remote (use server) or local (shared mem)
 * debug_default : the default debug level.
 * udpport       : port number for broadcast & direct udp messages
 * multiport     : port number for multicast messages
 * serverport    : port number for ET system's tcp server thread
 * policy        : policy to determine which responding ET system to a broad-
 *               : multicast will be chosen as the official respondent.
 *               : = ET_POLICY_ERROR - return error if more than one 
 *               :   responds, else return the single respondent
 *               : = ET_POLICY_FIRST - pick the first to respond
 *               : = ET_POLICY_LOCAL - pick the local system first and
 *               :   if it doesn't respond, the first that does
 * tcpSendBufSize: size in bytes of TCP send    buffer of socket connecting to ET tcp server
 * tcpRecvBufSize: size in bytes of TCP receive buffer of socket connecting to ET tcp server
 * tcpNoDelay    : if 0, socket has TCP_NODELAY option off, else on
 * timeout       : max time to wait for ET system to appear if wait=ET_OPEN_WAIT
 * host          : name of host computer that has ET system. Defaults to 
 *               : local host if unset. If domain not included, assumed local.
 *               : May be in dotted-decimal form. If the host name is not known,
 *               : its value may also be ET_HOST_ANYWHERE for an ET system that
 *               : may be local or remote, ET_HOST_REMOTE for an ET system that's
 *               : remote, or ET_HOST_LOCAL for an ET system that is local.
 * interface     : dotted-decimal ip address specifying the interface
 *               : for network communications with ET system.
 * netinfo       : linked list of structs containing network info
 * bcastaddrs    : linked list of all local subnet broadcast addrs (dotted-decimal)
 * mcastaddrs    : list of all multicast addresses (dotted-dec)
 */
typedef struct et_open_config_t {
  int             init;
  int             wait;
  int             cast;
  int             ttl;
  int             mode;
  int             debug_default;
  int             udpport;
  int             multiport;
  int             serverport;
  int             policy;
  int             tcpSendBufSize;
  int             tcpRecvBufSize;
  int             tcpNoDelay;
  struct timespec timeout;
  char            host[ET_MAXHOSTNAMELEN];
  char            interface[ET_IPADDRSTRLEN];
  codaIpAddr       *netinfo;
  codaIpList       *bcastaddrs;
  codaDotDecIpAddrs mcastaddrs;
} et_open_config;


/**
 * et_id  : Contains pointers to key mem locations, config info,
 *        : status info, node locality and remote node info.
 *        : It is essentially an ET system id. User needs one
 *        : for each ET system in use.
 *----------------------------------------------------------
 * init           : =ET_STRUCT_OK if structure properly initialized
 * lang           : language this ET system was written in:
 *                : ET_LANG_C, ET_LANG_CPP, or ET_LANG_JAVA
 * alive          : is system alive? 1 = yes, 0 = no
 * closed         : has et_close been called? 1 = yes, 0 = no
 * bit64          : 1 if the ET system connected to is a 64 bit executable, else 0
 * proc           : unique process id# for processes connected to an
 *                : ET system. It's an index into data stored in the
 *                : "et_system" struct. For the et_id returned by a
 *                : call to "et_system_start", this id is -1, This is
 *                : because it refers to the system itself and is not
 *                : "connected" to the ET system.
 * race           : flag used to eliminate race conditions
 * cleanup        : flag used to warn certain routines (such as et_station_detach and
 *                : et_restore_events) that they are being executed by the main ET system
 *                : process because the ET system is cleaning up after an ET consumer's crash.
 *                : This is meant to distinguish use by the ET system server when executing
 *                : such routines on behalf of a remote client. This prevents mutexes from
 *                : being grabbed during cleanup and temp events from being unlinked during
 *                : normal usage.
 * debug          : level of desired printed output
 * nevents        : total number of events in ET system
 * group          : default event group for calls to et_event(s)_new
 * version        : version # of this ET software release
 * nselects       : current # of selection ints per station (or control ints
 *                : per event)
 * share          : =ET_MUTEX_SHARE if operating system can share mutexes
 *                : between processes, =ET_MUTEX_NOSHARE otherwise
 * memsize        : total size of shared memory in bytes - used when to unmap
 *                : mmapped file when it's already been deleted.
 * esize          : size in bytes of events in ET system
 * offset         : byte offset between pmap for et & user's processes
 *
 * for REMOTE client use:
 * locality       : =ET_LOCAL if process is on same machine as ET system,
 *                : =ET_REMOTE if process is on another machine
 *                : =ET_LOCAL_NOSHARE if process is on same machine as ET
 *                : but cannot shared mutexes between processes
 * sockfd         : client's socket connection to ET server
 * endian         : endian of client's node (ET_ENDIAN_BIG or
 *                : ET_ENDIAN_LITTLE in et_network.h)
 * systemendian   : endian of ET system's node
 * iov_max        : iovmax of client's node
 * port           : port # for tcp server thread of the ET system
 * ethost         : host of the ET system
 *
 * pmap           : pointer to start of mapped memory
 * sys            : ptr to et_system structure
 * stats          : ptr to start of et_station structures
 * histogram      : ptr to histogram data
 * events         : ptr to start of et_event structures
 * data           : ptr to event data
 * grandcentral   : ptr to grandcentral station
 * mutex          : pthread mutex for thread-safe remote communications
 * sharedMemlock  : pthread read-write lock for preventing access of unmapped
 *                : memory after calling et_close()
 */
typedef struct  et_id_t {
  int              init;
  int              lang;
  int              alive;
  int              closed;
  int              bit64;
  et_proc_id       proc;
  int              race;
  int              cleanup;
  int              debug;
  int              nevents;
  int              group;
  int              version;
  int              nselects;
  int              share;
  size_t           memsize;
  uint64_t         esize;
  ptrdiff_t        offset;
  /* for REMOTE use */
  int              locality;
  int              sockfd;
  int              endian;
  int              systemendian;
  int              iov_max;
  int              port;
  char             ethost[ET_MAXHOSTNAMELEN];
  /******************/
  void            *pmap;
  et_system       *sys;
  et_station      *stats;
  int             *histogram;
  et_event        *events;
  char            *data;
  et_station      *grandcentral;
  pthread_mutex_t  mutex;
#ifndef NO_RW_LOCK
  pthread_rwlock_t sharedMemlock;
#endif
} et_id;


/**
 * et_mem: Contains info stored at front of mapped memory
 *----------------------------------------------------------
 *
 * byteOrder      : should be 0x01020304, if not, byte order
 *                : is reversed from local order.
 * sytemType      : type of local system using the mapped memory.
 *                : Right now there are only 2 types. One is an ET
 *                : system written in C (= ET_SYSTEM_TYPE_C).
 *                : The other is an ET system written in Java
 *                : with a different layout of the shared memory
 *                : (= ET_SYSTEM_TYPE_JAVA).
 * major version  : major version # of this ET software release
 * minor version  : minor version # of this ET software release
 * # select ints  : # station selection integers / event
 * headerByteSize : total size of a header structure in bytes
 * eventByteSize  : total size of a single event's data memory in bytes
 * headerPosition : number of bytes past start of shared memory
 *                : that the headers are stored.
 * dataPosition   : number of bytes past start of shared memory
 *                : that the data are stored.
 * totalsize      : total size of mapped memory (mapped
 *                : memory must be allocated in pages).
 * usedsize       : desired size of mapped memory given as arg
 *                : to et_mem_create.
 */
typedef struct  et_mem_t {
    uint32_t  byteOrder;
    uint32_t  systemType;
    uint32_t  majorVersion;
    uint32_t  minorVersion;
    uint32_t  numSelectInts;
    uint32_t  headerByteSize;
    
    uint64_t  eventByteSize;
    uint64_t  headerPosition;
    uint64_t  dataPosition;
    uint64_t  totalSize;
    uint64_t  usedSize;
} et_mem;


/**
 * Structure for holding an ET system's single response to ET client's broad/multicast
 */
typedef struct et_response_t {
    int   port;                     /**< ET system's TCP server port. */
    int   castType;                 /**< ET_BROADCAST or ET_MULTICAST (what this is a response to). */
    int   addrCount;                /**< Number of addresses. */
    char  uname[ET_MAXHOSTNAMELEN]; /**< Uname of sending host. */
    char  canon[ET_MAXHOSTNAMELEN]; /**< Canonical name of sending host. */
    char  castIP[ET_IPADDRSTRLEN];  /**< Original broad/multicast IP addr. */
    uint32_t *addrs;                /**< Array of 32bit net byte ordered addresses (1 for each addr). */
    char  **ipaddrs;                /**< Array of addresses (dotted-decimal string) of host
                                         (all for multicast, on subnet for broadcast). */
    struct et_response_t *next;        /**< Next response in linked list. */
} et_response;


/****************************
 *       REMOTE STUFF       *
 ****************************/

/* "table" of values representing ET commands for tcp/ip */

/* for operating systems that cannot share mutexes (Linux) */
#define  ET_NET_EV_GET_L        0        /**< et_event_get */
#define  ET_NET_EVS_GET_L       1        /**< et_events_get */
#define  ET_NET_EV_PUT_L        2        /**< et_event_put */
#define  ET_NET_EVS_PUT_L       3        /**< et_events_put */
#define  ET_NET_EV_NEW_L        4        /**< et_event_new */
#define  ET_NET_EVS_NEW_L       5        /**< et_events_new */
#define  ET_NET_EV_DUMP_L       6        /**< et_event_dump */
#define  ET_NET_EVS_DUMP_L      7        /**< et_events_dump */
#define  ET_NET_EVS_NEW_GRP_L   8        /**< et_events_new_group */

/* for fully remote systems */
#define  ET_NET_EV_GET         20        /**< et_event_get */
#define  ET_NET_EVS_GET        21        /**< et_events_get */
#define  ET_NET_EV_PUT         22        /**< et_event_put */
#define  ET_NET_EVS_PUT        23        /**< et_events_put */
#define  ET_NET_EV_NEW         24        /**< et_event_new */
#define  ET_NET_EVS_NEW        25        /**< et_events_new */
#define  ET_NET_EV_DUMP        26        /**< et_event_dump */
#define  ET_NET_EVS_DUMP       27        /**< et_events_dump */
#define  ET_NET_EVS_NEW_GRP    28        /**< et_events_new_group */

#define  ET_NET_EVS_NEW_GRP_JAVA 29
      
#define  ET_NET_ALIVE          40        /**< et_alive */
#define  ET_NET_WAIT           41        /**< et_wait_for_alive */
#define  ET_NET_CLOSE          42        /**< et_close */
#define  ET_NET_FCLOSE         43        /**< et_forcedclose */
#define  ET_NET_WAKE_ATT       44        /**< et_wakeup_attachment */
#define  ET_NET_WAKE_ALL       45        /**< et_wakeup_all */
#define  ET_NET_KILL           46        /**< et_kill */

#define  ET_NET_STAT_ATT       60        /**< et_station_attach */
#define  ET_NET_STAT_DET       61        /**< et_station_detach */
#define  ET_NET_STAT_CRAT      62        /**< et_station_create_at */
#define  ET_NET_STAT_RM        63        /**< et_station_remove */
#define  ET_NET_STAT_SPOS      64        /**< et_station_setposition */
#define  ET_NET_STAT_GPOS      65        /**< et_station_getposition */

#define  ET_NET_STAT_ISAT      80        /**< et_station_isattached */
#define  ET_NET_STAT_EX        81        /**< et_station_exists */
#define  ET_NET_STAT_SSW       82        /**< et_station_setselectwords */
#define  ET_NET_STAT_GSW       83        /**< et_station_getselectwords */
#define  ET_NET_STAT_LIB       84        /**< et_station_getlib */
#define  ET_NET_STAT_FUNC      85        /**< et_station_getfunction */
#define  ET_NET_STAT_CLASS     86        /**< et_station_getclass */
   
#define  ET_NET_STAT_GATTS    100        /**< et_station_getattachments */
#define  ET_NET_STAT_STATUS   101        /**< et_station_getstatus */
#define  ET_NET_STAT_INCNT    102        /**< et_station_getinputcount */
#define  ET_NET_STAT_OUTCNT   103        /**< et_station_getoutputcount */
#define  ET_NET_STAT_GBLOCK   104        /**< et_station_getblock */
#define  ET_NET_STAT_GUSER    105        /**< et_station_getuser */
#define  ET_NET_STAT_GRESTORE 106        /**< et_station_getrestore */
#define  ET_NET_STAT_GPRE     107        /**< et_station_getprescale */
#define  ET_NET_STAT_GCUE     108        /**< et_station_getcue */
#define  ET_NET_STAT_GSELECT  109        /**< et_station_getselect */

#define  ET_NET_STAT_SBLOCK   115        /**< et_station_getblock */
#define  ET_NET_STAT_SUSER    116        /**< et_station_getuser */
#define  ET_NET_STAT_SRESTORE 117        /**< et_station_getrestore */
#define  ET_NET_STAT_SPRE     118        /**< et_station_getprescale */
#define  ET_NET_STAT_SCUE     119        /**< et_station_getcue */

#define  ET_NET_ATT_PUT       130        /**< et_att_getput */
#define  ET_NET_ATT_GET       131        /**< et_att_getget */
#define  ET_NET_ATT_DUMP      132        /**< et_att_getdump */
#define  ET_NET_ATT_MAKE      133        /**< et_att_getmake */

#define  ET_NET_SYS_TMP       150        /**< et_system_gettemps */
#define  ET_NET_SYS_TMPMAX    151        /**< et_system_gettempsmax */
#define  ET_NET_SYS_STAT      152        /**< et_system_getstations */
#define  ET_NET_SYS_STATMAX   153        /**< et_system_getstationsmax */
#define  ET_NET_SYS_PROC      154        /**< et_system_getprocs */
#define  ET_NET_SYS_PROCMAX   155        /**< et_system_getprocsmax */
#define  ET_NET_SYS_ATT       156        /**< et_system_getattachments */
#define  ET_NET_SYS_ATTMAX    157        /**< et_system_getattsmax */
#define  ET_NET_SYS_HBEAT     158        /**< et_system_getheartbeat */
#define  ET_NET_SYS_PID       159        /**< et_system_getpid */
#define  ET_NET_SYS_GRP       160        /**< et_system_getgroup */

#define  ET_NET_SYS_DATA      170        /**< send ET system data */
#define  ET_NET_SYS_HIST      171        /**< send ET histogram data */
#define  ET_NET_SYS_GRPS      172        /**< send size of each event group */


/** Struct for passing data from system to network threads */
typedef struct et_netthread_t {
    int   cast;                     /**< broad or multicast */
    et_id *id;                      /**< system id */
    et_sys_config *config;          /**< system configuration */
    char *listenaddr;               /**< broadcast or multicast address  (dot-decimal) */
    char uname[ET_MAXHOSTNAMELEN];  /**< host obtained with "uname" cmd */
} et_netthread;

/****************************
 *       BRIDGE STUFF       *
 ****************************/
 
/**
 * et_bridge_config : parameters used to bridge ET systems,
 *                  : (transfer events between 2 systems)
 *-----------------------------
 * init           : =ET_STRUCT_OK if structure properly initialized
 * mode_from      : =ET_SLEEP, ET_TIMED, or ET_ASYNC for getting events
 *                : from the "from" ET system
 * mode_to        : =ET_SLEEP, ET_TIMED, or ET_ASYNC for getting new events
 *                : from the "to" ET system into which the "from" events go.
 * chunk_from     : # of events to get from the "from" ET system at one time.
 * chunk_to       : # of new events to get from the "to" ET system at one time.
 * timeout_from   : max time to wait when getting events from the "from" ET system
 * timeout_to     : max time to wait when getting new events from the "to" ET system
 * func           : pointer to function which takes a pointer to an event as
 *                : an argument, swaps the data, and returns ET_ERROR if there's
 *                : a problem & ET_OK if not.
 */
typedef struct et_bridge_config_t {
  int             init;
  int             mode_from;
  int             mode_to;
  int             chunk_from;
  int             chunk_to ;
  struct timespec timeout_from;
  struct timespec timeout_to;
  ET_SWAP_FUNCPTR func;                /* see et.h */
} et_bridge_config;


/****************************
 *    END BRIDGE STUFF      *
 ****************************/

/* macro to get pointer to event from id and event place */
#define ET_P2EVENT(etid, place) ((et_event *)((et_event *)(etid->events) + (place)))

/* macros to switch ptrs from et space to user space & vice versa */
#define ET_PEVENT2USR(p, offset) ((et_event *)((char *)(p) + (offset)))
#define ET_PEVENT2ET(p, offset)  ((et_event *)((char *)(p) - (offset)))
#define ET_PSTAT2USR(p, offset)  ((et_station *)((char *)(p) + (offset)))
#define ET_PSTAT2ET(p, offset)   ((et_station *)((char *)(p) - (offset)))
#define ET_PDATA2USR(p, offset)  ((void *)((char *)(p) + (offset)))
#define ET_PDATA2ET(p, offset)   ((void *)((char *)(p) - (offset)))

/* macros to break a 64 int into 2, 32 bit ints & the reverse */
#define ET_64BIT_UINT(hi,lo)       (((uint64_t)(hi) << 32) | ((uint64_t)(lo) & 0x00000000FFFFFFFF))
#define ET_64BIT_INT(hi,lo)         (((int64_t)(hi) << 32) | ((int64_t)(lo)  & 0x00000000FFFFFFFF))
#define ET_64BIT_P(hi,lo) ((void *)(((uint64_t)(hi) << 32) | ((uint64_t)(lo) & 0x00000000FFFFFFFF)))
#define ET_HIGHINT(i)         ((int)(((uint64_t)(i) >> 32) & 0x00000000FFFFFFFF))
#define ET_LOWINT(i)           ((int)((uint64_t)(i) & 0x00000000FFFFFFFF))

/***********************/
/* Extern declarations */
/***********************/

/* mutex related */
extern void et_station_lock(et_system *sys);
extern void et_station_unlock(et_system *sys);

extern void et_llist_lock(et_list *pl);
extern void et_llist_unlock(et_list *pl);

extern void et_system_lock(et_system *sys);
extern void et_system_unlock(et_system *sys);

extern void et_transfer_lock(et_station *ps);
extern void et_transfer_unlock(et_station *ps);
extern void et_transfer_lock_all(et_id *id);
extern void et_transfer_unlock_all(et_id *id);

extern void et_tcp_lock(et_id *id);
extern void et_tcp_unlock(et_id *id);

extern void et_memRead_lock(et_id *id);
extern void et_memWrite_lock(et_id *id);
extern void et_mem_unlock(et_id *id);

extern int  et_mutex_locked(pthread_mutex_t *pmutex);

extern int  et_repair_station(et_id *id, et_stat_id stat_id);
extern int  et_repair_gc(et_id *id);

/* initialization */
extern void et_init_station(et_station *ps);
extern void et_init_llist(et_list *pl);

extern void et_init_event(et_event *pe);
extern void et_init_event_(et_event *pe);

extern void et_init_process(et_system *sys, et_proc_id id);
extern void et_init_attachment(et_system *sys, et_att_id id);

extern void et_init_histogram(et_id *id);
extern void et_init_stats_att(et_system *sys, et_att_id id);
extern void et_init_stats_allatts(et_system *sys);
extern void et_init_stats_station(et_station *ps);
extern void et_init_stats_allstations(et_id *id);
extern void et_init_stats_all(et_id *id);

extern int  et_id_init(et_sys_id* id);
extern void et_id_destroy(et_sys_id id);

/* read and write */
extern int  et_station_write(et_id *id,  et_stat_id stat_id, et_event  *pe);
extern int  et_station_nwrite(et_id *id, et_stat_id stat_id, et_event *pe[], int num);

extern int  et_station_read(et_id *id,  et_stat_id stat_id, et_event **pe, 
                            int _mode, et_att_id att, struct timespec *time);
extern int  et_station_nread(et_id *id, et_stat_id stat_id, et_event *pe[], int mode,
                             et_att_id att, struct timespec *time, int num, int *nread);
extern int  et_station_nread_group(et_id *id, et_stat_id stat_id, et_event *pe[], int mode,
                 et_att_id att, struct timespec *time, int num, int group, int *nread);
extern int  et_station_dump(et_id *id, et_event *pe);
extern int  et_station_ndump(et_id *id, et_event *pe[], int num);

extern int  et_llist_read(et_list *pl, et_event **pe);
extern int  et_llist_write(et_id *id, et_list *pl, et_event **pe, int num);
extern int  et_llist_write_gc(et_id *id, et_event **pe, int num);

extern int  et_restore_events(et_id *id, et_att_id att, et_stat_id stat_id);
extern void et_flush_events(et_id *id, et_att_id att, et_stat_id stat_id);

/* mmap/memory functions */
extern int   et_mem_create(const char *name, size_t memsize, void **pmemory, size_t *totalSize);
extern void *et_mem_write_first_block(char *ptr,
                                      uint32_t headerByteSize, uint64_t eventByteSize,
                                      uint64_t headerPosition, uint64_t dataPosition,
                                      uint64_t totalByteSize,  uint64_t usedByteSize);
extern int   et_mem_attach(const char *name, void **pmemory, et_mem *pInfo);
extern int   et_mem_unmap(const char *name, void *pmem);
extern int   et_mem_remove(const char *name, void *pmem);
extern int   et_mem_size(const char *name, size_t *totalsize, size_t *usedsize);

extern void *et_temp_create(const char *name, size_t size);
extern void *et_temp_attach(const char *name, size_t size);
extern int   et_temp_remove(const char *name, void *pmem, size_t size);

/* remote routines */
extern int  etr_event_new(et_sys_id id, et_att_id att, et_event **ev,
                int mode, struct timespec *deltatime, size_t size);
extern int  etr_events_new(et_sys_id id, et_att_id att, et_event *evs[],
                int mode, struct timespec *deltatime, size_t size, int num, int *nread);
extern int  etr_events_new_group(et_sys_id id, et_att_id att, et_event *evs[],
                   int mode, struct timespec *deltatime,
                   size_t size, int num, int group, int *nread);
extern int  etr_event_get(et_sys_id id, et_att_id att, et_event **ev,
                int wait, struct timespec *deltatime);
extern int  etr_events_get(et_sys_id id, et_att_id att, et_event *evs[],
                int wait, struct timespec *deltatime, int num, int *nread);
extern int  etr_event_put(et_sys_id id, et_att_id att, et_event *ev);
extern int  etr_events_put(et_sys_id id, et_att_id att, et_event *evs[], int num);
extern int  etr_event_dump(et_sys_id id, et_att_id att, et_event *ev);
extern int  etr_events_dump(et_sys_id id, et_att_id att, et_event *evs[], int num);

extern int  etr_open(et_sys_id *id, const char *et_filename, et_openconfig openconfig);
extern int  etr_close(et_sys_id id);
extern int  etr_forcedclose(et_sys_id id);
extern int  etr_kill(et_sys_id id);
extern int  etr_alive(et_sys_id id);
extern int  etr_wait_for_alive(et_sys_id id);
extern int  etr_wakeup_attachment(et_sys_id id, et_att_id att);
extern int  etr_wakeup_all(et_sys_id id, et_stat_id stat_id);

extern int  etr_station_create_at(et_sys_id id, et_stat_id *stat_id,
                 const char *stat_name, et_statconfig sconfig,
                 int position, int parallelposition);
extern int  etr_station_create(et_sys_id id, et_stat_id *stat_id,
                 const char *stat_name, et_statconfig sconfig);
extern int  etr_station_remove(et_sys_id id, et_stat_id stat_id);
extern int  etr_station_attach(et_sys_id id, et_stat_id stat_id, et_att_id *att);
extern int  etr_station_detach(et_sys_id id, et_att_id att);
extern int  etr_station_setposition(et_sys_id id, et_stat_id stat_id, int position,
                                    int parallelposition);
extern int  etr_station_getposition(et_sys_id id, et_stat_id stat_id, int *position,
                                    int *parallelposition);
extern int  etr_station_isattached(et_sys_id id, et_stat_id stat_id, et_att_id att);
extern int  etr_station_exists(et_sys_id id, et_stat_id *stat_id, const char *stat_name);

extern int  etr_station_getattachments(et_sys_id id, et_stat_id stat_id, int *numatts);
extern int  etr_station_getstatus(et_sys_id id, et_stat_id stat_id, int *status);
extern int  etr_station_getstatus(et_sys_id id, et_stat_id stat_id, int *status);
extern int  etr_station_getinputcount(et_sys_id id, et_stat_id stat_id, int *cnt);
extern int  etr_station_getoutputcount(et_sys_id id, et_stat_id stat_id, int *cnt);
extern int  etr_station_getselect(et_sys_id id, et_stat_id stat_id, int *select);
extern int  etr_station_getlib(et_sys_id id, et_stat_id stat_id, char *lib);
extern int  etr_station_getclass(et_sys_id id, et_stat_id stat_id, char *classs);
extern int  etr_station_getfunction(et_sys_id id, et_stat_id stat_id, char *function);

extern int  etr_station_getblock(et_sys_id id, et_stat_id stat_id, int *block);
extern int  etr_station_setblock(et_sys_id id, et_stat_id stat_id, int  block);
extern int  etr_station_getuser(et_sys_id id, et_stat_id stat_id, int *user);
extern int  etr_station_setuser(et_sys_id id, et_stat_id stat_id, int  user);
extern int  etr_station_getrestore(et_sys_id id, et_stat_id stat_id, int *restore);
extern int  etr_station_setrestore(et_sys_id id, et_stat_id stat_id, int  restore);
extern int  etr_station_getprescale(et_sys_id id, et_stat_id stat_id, int *prescale);
extern int  etr_station_setprescale(et_sys_id id, et_stat_id stat_id, int  prescale);
extern int  etr_station_getcue(et_sys_id id, et_stat_id stat_id, int *cue);
extern int  etr_station_setcue(et_sys_id id, et_stat_id stat_id, int  cue);
extern int  etr_station_getselectwords(et_sys_id id, et_stat_id stat_id, int select[]);
extern int  etr_station_setselectwords(et_sys_id id, et_stat_id stat_id, int select[]);

extern int  etr_system_gettemps (et_sys_id id, int *temps);
extern int  etr_system_gettempsmax (et_sys_id id, int *tempsmax);
extern int  etr_system_getstations (et_sys_id id, int *stations);
extern int  etr_system_getstationsmax (et_sys_id id, int *stationsmax);
extern int  etr_system_getprocs (et_sys_id id, int *procs);
extern int  etr_system_getprocsmax (et_sys_id id, int *procsmax);
extern int  etr_system_getattachments (et_sys_id id, int *atts);
extern int  etr_system_getattsmax (et_sys_id id, int *attsmax);
extern int  etr_system_getheartbeat (et_sys_id id, int *heartbeat);
extern int  etr_system_getpid (et_sys_id id, int *pid);
extern int  etr_system_getgroupcount (et_sys_id id, int *groupCnt);
/* end remote routines */

/* attachment rountines */
extern int etr_attach_geteventsput(et_sys_id id, et_att_id att_id,
                                   uint64_t *events);
extern int etr_attach_geteventsget(et_sys_id id, et_att_id att_id,
                                   uint64_t *events);
extern int etr_attach_geteventsdump(et_sys_id id, et_att_id att_id,
                                    uint64_t *events);
extern int etr_attach_geteventsmake(et_sys_id id, et_att_id att_id,
                                    uint64_t *events);
/* end of attachment routines */

/* local no-share (linux) routines */
extern int  etn_open(et_sys_id *id, const char *filename, et_openconfig openconfig);
extern int  etn_close(et_sys_id id);
extern int  etn_forcedclose(et_sys_id id);
extern int  etn_kill(et_sys_id id);
extern int  etn_alive(et_sys_id id);
extern int  etn_wait_for_alive(et_sys_id id);
extern int  etn_event_new(et_sys_id id, et_att_id att, et_event **ev,
                 int mode, struct timespec *deltatime, size_t size);
extern int  etn_events_new(et_sys_id id, et_att_id att, et_event *evs[],
                 int mode, struct timespec *deltatime, size_t size, int num, int *nread);
extern int  etn_events_new_group(et_sys_id id, et_att_id att, et_event *evs[],
                 int mode, struct timespec *deltatime,
                 size_t size, int num, int group, int *nread);
extern int  etn_event_get(et_sys_id id, et_att_id att, et_event **ev,
                 int mode, struct timespec *deltatime);
extern int  etn_events_get(et_sys_id id, et_att_id att, et_event *evs[],
                 int mode, struct timespec *deltatime, int num, int *nread);
extern int  etn_event_put(et_sys_id id, et_att_id att, et_event *ev);
extern int  etn_events_put(et_sys_id id, et_att_id att, et_event *evs[], int num);
extern int  etn_event_dump(et_sys_id id, et_att_id att, et_event *ev);
extern int  etn_events_dump(et_sys_id id, et_att_id att, et_event *evs[], int num);
/* end of local linux routines */

/* local routines (Sun) */
extern int  etl_open(et_sys_id *id, const char *filename, et_openconfig openconfig);
extern int  etl_close(et_sys_id id);
extern int  etl_forcedclose(et_sys_id id);
extern int  etl_alive(et_sys_id id);
extern int  etl_wait_for_alive(et_sys_id id);
extern int  et_wait_for_system(et_sys_id id, struct timespec *timeout,
                                const char *etname);
extern int  et_look(et_sys_id *id, const char *filename);
extern int  et_unlook(et_sys_id id);
/* end local routines */

/* server/network routines */
extern void *et_cast_thread(void *arg);
extern void *et_netserver(void *arg);
extern int   et_findserver(const char *etname, char *ethost, int *port,
                           uint32_t *inetaddr, et_open_config *config, et_response **allETinfo);
extern int   et_responds(const char *etname);
extern int   et_sharedmutex(void);
extern int   et_findlocality(const char *filename, et_openconfig openconfig);
/* end server/network routines */
 
/* station configuration checks */
extern int  et_station_config_check(et_id *id, et_stat_config *sc);
extern int  et_station_compare_parallel(et_id *id, et_stat_config *group,
            et_stat_config *config);
/* end station configuration checks */

/* error logging */
extern void et_logmsg (char *sev, char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
