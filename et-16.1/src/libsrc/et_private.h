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

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <pthread.h>

#include "etCommonNetwork.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The data-type model for Linux defines  __WORDSIZE to be 64 or 32. */
#ifdef linux
  #if __WORDSIZE == 64
    #define _LP64 1
  #else
    #define _ILP32 1
  #endif
#endif

#define ET_VERSION 16           /**< Major version number. */
#define ET_VERSION_MINOR 1      /**< Minor version number. */

#define ET_LANG_C     0         /**< C language version of ET software. */
#define ET_LANG_CPP   1         /**< C++ language version of ET software.  */
#define ET_LANG_JAVA  2         /**< Java language version of ET software. */

#define ET_SYSTEM_TYPE_C     1  /**< ET system implemented through C language library. */
#define ET_SYSTEM_TYPE_JAVA  2  /**< ET system implemented through Java language library. */

#define ET_IPADDRSTRLEN 16      /**< Max string length of dotted-decimal ip address. */

/** Max length of a host name including the terminating char. */
#define ET_MAXHOSTNAMELEN 256

/** Max number of network addresses/names per host we'll examine. */
#define ET_MAXADDRESSES 10

/****************************************
 * times for heart beating & monitoring *
 ****************************************/
/* 2 min */
#define ET_MON_SEC    120         /**< Seconds between monitoring heartbeat for disconnecting from unresponsive clients. */
#define ET_MON_NSEC     0         /**< Nanoseconds between monitoring heartbeat for disconnecting from unresponsive clients. */

/* 0.5 sec */
#define ET_BEAT_SEC   0           /**< Seconds between heartbeat (integer increment). */
#define ET_BEAT_NSEC  500000000   /**< Nanoseconds between heartbeat (integer increment). */

/* 2 sec */
#define ET_IS_ALIVE_SEC   2       /**< Seconds between monitoring heartbeat for seeing if ET alive. */
#define ET_IS_ALIVE_NSEC  0       /**< Nanoseconds between monitoring heartbeat for seeing if ET alive. */

/* 2.5 sec */
#define ET_CLOSE_SEC   2          /**< Seconds to wait before shutting down ET system. */
#define ET_CLOSE_NSEC  500000000  /**< Nanoseconds to wait before shutting down ET system. */

/** Max value for heartbeat. */
#define ET_HBMODULO UINT_MAX

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
#define ET_ATTACHMENTS_MAX 100                 /**< Maximum number of attachments allowed on the system. */
#define ET_PROCESSES_MAX   ET_ATTACHMENTS_MAX  /**< Maximum number of local processes allowed to open the system. */

/* status of an attachment to an ET system */
#define ET_ATT_UNUSED 0         /**< Attachment number not in use. */
#define ET_ATT_ACTIVE 1         /**< Attachment is active (attached to a station). */

/* values to tell attachment to return prematurely from ET routine */
#define ET_ATT_CONTINUE 0       /**< Attachment is normal. */
#define ET_ATT_QUIT     1       /**< Attachment must return immediately from ET API call. */

/* values telling whether an attachment is blocked in a read or not */
#define ET_ATT_UNBLOCKED 0      /**< Attachment is not blocked on a read. */
#define ET_ATT_BLOCKED   1      /**< Attachment is blocked on a read. */

/* values telling whether an attachment is in simulated sleep mode or not */
#define ET_ATT_NOSLEEP 0        /**< Attachment is not in sleep mode. */
#define ET_ATT_SLEEP   1        /**< Attachment is in simulated sleep mode */

/* status of a process in regards to an ET system */
#define ET_PROC_CLOSED 0        /**< Process has closed ET systems (no access to mapped memory). */
#define ET_PROC_OPEN   1        /**< Process has opened ET system (has access to mapped memory). */

/* what a process thinks of its ET system's status */
#define ET_PROC_ETDEAD 0        /**< Process thinks ET system it is connected to is dead. */
#define ET_PROC_ETOK   1        /**< Process thinks ET system it is connected to is OK.   */

/* certain structures need to be initialized before use */
#define ET_STRUCT_NEW 0         /**< Structure is newly created and not yet initialized. */
#define ET_STRUCT_OK  1         /**< Structure is initialized and ready for use. */

/* values to tell thread to self-destruct or to stick around */
#define ET_THREAD_KEEP 0        /**< Keep thread around. */
#define ET_THREAD_KILL 1        /**< Kill thread. */

/* tells if operating sys can share pthread mutexes between processes */
#define ET_MUTEX_SHARE   0      /**< Operating system can share pthread mutex between different local processes. */
#define ET_MUTEX_NOSHARE 1      /**< Operating system <b>cannot</b> share pthread mutex between different local processes. */

/* is pthread mutex locked or not */
#define ET_MUTEX_UNLOCKED 0    /**< Pthread mutex is unlocked. */
#define ET_MUTEX_LOCKED   1    /**< Pthread mutex is locked. */

/** Size in bytes of the data stored at the beginning of the shared memory
 * (these data give general info necessary for Java and C clients to
 * handle the ET system file properly). */
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


/** Defines event selection function. */
typedef int (*ET_SELECT_FUNCPTR) (void *, et_stat_id, et_event *);


/** Structure to hold parameters used to configure a station. */
typedef struct et_stat_config_t {
  int  init;           /**< @ref ET_STRUCT_OK if structure properly initialized,
 *                          else @ref ET_STRUCT_NEW. */
  int  flow_mode;      /**< @ref ET_STATION_PARALLEL if station part of a group of stations through
                            which events flow in parallel, or @ref ET_STATION_SERIAL if events
                            flow through each station (default). */
  int  user_mode;      /**< Number of attachment allowed, @ref ET_STATION_USER_MULTI for any number,
                            or @ref ET_STATION_USER_SINGLE for only 1. */
  int  restore_mode;   /**< if process dies, events it read but didn't write can be sent to:
                            1) station's ouput with @ref ET_STATION_RESTORE_OUT,
                            2) station's input with @ref ET_STATION_RESTORE_IN,
                            3) grand_central station's input with @ref ET_STATION_RESTORE_GC, or
                            4) previous station's output list for redistribution among a group of
                               parallel stations with @ref ET_STATION_RESTORE_REDIST.*/
  int  block_mode;     /**< @ref ET_STATION_BLOCKING for accepting every event which meets its
                            condition, or @ref ET_STATION_NONBLOCKING for accepting only enough to fill
                            a limited queue and allowing everything else to continue downstream. */
  int  prescale;       /**< For blocking stations only, accept only every Nth normally accepted
                            event. */
  int  cue;            /**< For nonblocking stations only, max number of events accepted into input list. */
  int  select_mode;    /**< @ref ET_STATION_SELECT_ALL for accepting every event,
                            @ref ET_STATION_SELECT_MATCH for accepting events whose control array must
                            match station's select array by predefined rules,
                            @ref ET_STATION_SELECT_USER for accepting events by a user-supplied library
                            function,
                            @ref ET_STATION_SELECT_RROBIN for accepting events by using a round robin
                            distribution of events to parallel stations, and
                            @ref ET_STATION_SELECT_EQUALCUE for accepting events by distributing an
                            equal number of events in each queue of a single group of parallel stations. */
  int  select[ET_STATION_SELECT_INTS]; /**< Array of ints for use in event selection. */
  char fname[ET_FUNCNAME_LENGTH];      /**< Name of user-defined event selection routine (for C-based ET system). */
  char lib[ET_FILENAME_LENGTH];        /**< Name of shared library containing user-defined event selection routine
                                            (for C-based ET system). */
  char classs[ET_FILENAME_LENGTH];     /**< Name of JAVA class containing method to implement event selection
                                            (for Java-based ET system). */
} et_stat_config;


/** Structure to hold the current state of a station. */
typedef struct et_stat_data_t {
  int   status;                    /**< Station's state may be @ref ET_STATION_ACTIVE for a station accepting events,
                                        @ref ET_STATION_IDLE for a station with no attachments and therefore accepting
                                        no events,
                                        @ref ET_STATION_CREATING for a station in the process of being created, or
                                        @ref ET_STATION_UNUSED for a station structure in shared memory that is unused. */
  int   pid_create;                /**< Process id of process that created the station. */
  int   nattachments;              /**< Number of attachments to this station. */
  int   att[ET_ATTACHMENTS_MAX];   /**< Array in which the unique id# of an attachment is the index and the value is
                                        id# if attached and -1 otherwise. */
  void *lib_handle;                /**< Handle for the opened shared library of a user-defined event selection routine. */
  ET_SELECT_FUNCPTR func;          /**< Pointer to user-defined event selection routine. */
} et_stat_data;


/** Structure defining a station's input or output list of events. */
typedef struct et_list_t {
  int              cnt;          /**< Number of events in list. */
  int              lasthigh;     /**< Place in list of last high priority event. */
  uint64_t         events_try;   /**< Number of events attempted to be put in (before prescale). */
  uint64_t         events_in;    /**< Number of events actually put in. */
  uint64_t         events_out;   /**< Number of events actually taken out. */
  et_event        *firstevent;   /**< Pointer to first event in linked list. */
  et_event        *lastevent;    /**< pointer to last  event in linked list. */
  pthread_mutex_t  mutex;        /**< Pthread mutex which protects linked list when reading & writing. */
  pthread_cond_t   cread;        /**< Pthread condition variable to notify reader that events are here. */
} et_list;


#define ET_FIX_READ 0  /**< Fixing station's in/output event list after @ref et_events_get or @ref et_events_new. */
#define ET_FIX_DUMP 1  /**< Fixing station's in/output event list after @ref et_events_dump. */

/** Structure for fixing station input list */
struct et_fixin {
  et_event   *first;       /**< Value of et_list->firstevent at start of read (NULL if no damage). */
  uint64_t    eventsin;    /**< Value of et_list->events_in at start of dump. */
  int         start;       /**< Is 1 at start of write and 0 at end. */
  int         cnt;         /**< Value of et_list->cnt at start of read/write. */
  int         num;         /**< Number of events intended to read/write. */
  int         call;        /**< @ref ET_FIX_DUMP if fixing after et_station_(n)dump call, or
                                @ref ET_FIX_READ if fixing after et_station_(n)read call. */
};


/** Structure for fixing station output list.*/
struct et_fixout {
    int         start;       /**< Is 1 at start of write and 0 at end. */
    int         cnt;         /**< Value of et_list->cnt at start of read/write. */
    int         num;         /**< Number of events intended to read/write. */
};


/** Struct to fix station's input and output linked lists after crash. */
struct et_fix {
  struct et_fixin  in;  /**< Structure to fix station input list. */
  struct et_fixout out; /**< Structure to fix station output list. */
};


/** Structure defining a station. */
typedef struct et_station_t {
  et_stat_id            num;              /**< Unique id # of station, 0 for first station
                                               (GrandCentral), = 1 for next station in mapped memory, etc. */
  int                   conductor;        /**< Flag to kill conductor thread: when station deleted
                                               @ref ET_THREAD_KILL else @ref ET_THREAD_KEEP. */
  et_stat_id            next;             /**< Integer specifying next active or idle station in station chain,
                                               (not storing this as a pointer makes for an awkward linked list,
                                               but it survives mapping the shared memory to a different spot),
                                               and for last station this is -1. */
  et_stat_id            prev;             /**< Previous active or idle station in station chain and for first
                                               station this is -1. */
  et_stat_id            nextparallel;     /**< If this station is in a group of parallel stations, this is a
                                               "pointer" (actually and index) to the next parallel station and
                                               for last station this is -1. */
  et_stat_id            prevparallel;     /**< If this station is in a group of parallel stations, this is a
                                               "pointer" (actually an index) to the previous parallel station and
                                               for first station this is -1. */
  int                   waslast;          /**< Flag = 1 if this station was last one to receive an event when
                                               using the round-robin selection method for a parallel group of stations
                                               (else = 0). */
  char                  name[ET_STATNAME_LENGTH];  /**< Unique station name. */
  pthread_mutex_t       mutex;            /**< Pthread mutex used for keeping the linked list of used stations in order
                                               for event transfers. */
  struct et_fix         fix;              /**< Info to repair station's lists after user crash. */
  et_stat_data          data;             /**< Current state of station. */
  et_stat_config        config;           /**< Station configuration. */
  et_list               list_in;          /**< Input list  - a linked list containing events to read. */
  et_list               list_out;         /**< Output list - a linked list containing events to be written. */
} et_station;


/** Structure containing local process info. */
struct et_proc {
  et_proc_id    num;                      /**< Unique index # of this process. */
  et_att_id     att[ET_ATTACHMENTS_MAX];  /**< Array in which an element (indexed by the id# of an attachment owned
                                               by this process) gives the id# of the attachment and -1 otherwise. */
  int           nattachments;             /**< Number of attachments to an ET system in this process. */
  int           status;                   /**< @ref ET_PROC_OPEN if open or connected to ET system, or
                                               @ref ET_PROC_CLOSED if closed/unconnected. */
  int           et_status;                /**< @ref ET_PROC_ETDEAD if ET system is dead, or
                                               @ref ET_PROC_ETOK if ET system is OK. */
  unsigned int  heartbeat;                /**< Heartbeat periodically incremented to tell ET system it's alive. */
  pid_t         pid;                      /**< Unix process id. */
  pthread_t     hbeat_thd_id;             /**< Heartbeat pthread id . */
  pthread_t     hmon_thd_id;              /**< Heart monitor pthread id. */
};


/** Structure containing attachment info. */
struct et_attach {
  et_att_id   num;          /**< Unique index # of this attachment. */
  et_proc_id  proc;         /**< Unique index # of process owning this attachment. */
  et_stat_id  stat;         /**< Unique index # of station we're attached to. */
  int         status;       /**< @ref ET_ATT_UNUSED is attachment unused or @ref ET_ATT_ACTIVE if active. */
  int         blocked;      /**< @ref ET_ATT_BLOCKED if blocked waiting to read events, else @ref ET_ATT_UNBLOCKED. */
  int         quit;         /**< @ref ET_ATT_QUIT to force return from ET API routine, else
                                 @ref ET_ATT_CONTINUE if everything OK. */
  int         sleep;        /**< @ref ET_ATT_SLEEP if attachment is remote and sleeping when getting events
                                 (sleep is simulated by multiple timed waits otherwise it causes trouble when waking
                                 attachments), else
                                 @ref ET_ATT_NOSLEEP if not in simulated remote sleeping mode. */
  uint64_t    events_put;   /**< Number of events put into station output list. */
  uint64_t    events_get;   /**< Number of events obtained from station input list. */
  uint64_t    events_dump;  /**< Number of events dumped back into GrandCentral's input list (recycles). */
  uint64_t    events_make;  /**< Number of new events requested. */
  pid_t       pid;          /**< Unix process id# of process that owns attachment. */
  char        host[ET_MAXHOSTNAMELEN];      /**< Host running process that owns attachment. */
  char        interface[ET_IPADDRSTRLEN];   /**< Dot-decimal IP address of outgoing network interface. */
};


/** Structure containing all info necessary to configure an ET system. */
typedef struct  et_sys_config_t {
  uint64_t          event_size;       /**< Event size in bytes. */
  int               init;             /**< @ref ET_STRUCT_OK if structure initialized, else @ref ET_STRUCT_NEW. */
  int               nevents;          /**< Total # of events. */
  int               ntemps;           /**< Max number of temporary events allowed (<= nevents). */
  int               nstations;        /**< Max number of stations allowed (including GrandCentral). */
  int               nprocesses;       /**< Max number of local processes allowed to open ET system. */
  int               nattachments;     /**< Max number of attachments to stations allowed. */
  int               groupCount;       /**< Number of event groups. */
  int               groups[ET_EVENT_GROUPS_MAX];  /**< Array in which index is the group number (-1) and value is the
                                                       number of events in that group (there are "groupCount"
                                                       number of valid groups). */
  char              filename[ET_FILENAME_LENGTH]; /**< Name of the ET system file. */

  /* for remote use */
  int               port;             /**< Broad/multicast port # for UDP communication. */
  int               serverport;       /**< Port # for ET system TCP server thread. */
  int               tcpSendBufSize;   /**< TCP send buffer size in bytes of socket connecting to ET client. */
  int               tcpRecvBufSize;   /**< TCP receive buffer size in bytes of socket connecting to ET client.. */
  int               tcpNoDelay;       /**< If 0, sockets to clients have TCP_NODELAY option off, else on. */
  codaNetInfo       netinfo;          /**< All local network info. */
  codaDotDecIpAddrs bcastaddrs;       /**< All local subnet broadcast addresses (dot-decimal). */
  codaDotDecIpAddrs mcastaddrs;       /**< All multicast addresses to listen on (dot-decimal). */
} et_sys_config;


/* Macros to handle the bitInfo word in et_system structure following. */
#define ET_BIT64_MASK 0x1  /**< Bit mask to select bit in bitInfo word of et_system structure identifying ET system's
                                host running 64-bit OS. */
#define ET_KILL_MASK  0x2  /**< Bit mask to select bit in bitInfo word of et_system structure telling ET system to
                                kill itself ASAP. */
#define ET_GET_BIT64(x)  ((x) & ET_BIT64_MASK)  /**< Get the bit in the bitInfo word identifying ET system's
                                                     host as running a 64-bit OS. */
#define ET_GET_KILL(x)   ((x) & ET_KILL_MASK)   /**< Get the bit in the bitInfo word telling ET system to
                                                     kill itself ASAP. */
#define ET_SET_BIT64(x)  ((x) | ET_BIT64_MASK)  /**< Set the bit in the bitInfo word identifying ET system's
                                                     host as running a 64-bit OS. */
#define ET_SET_KILL(x)   ((x) | ET_KILL_MASK)   /**< Set the bit in the bitInfo word telling ET system to
                                                     kill itself ASAP. */


/** Structure containing all ET system information. */
typedef struct et_system_t {
  int              version;        /**< Major version number of this ET software release. */
  int              nselects;       /**< Number of selection ints per station (also control ints per event). */
  int              bitInfo;        /**< Least significant bit = 1 if ET system created by 64 bit executable, and
                                        next bit = 1 if ET system is being told to kill itself. */
  int              asthread;       /**<  @ref ET_THREAD_KILL if killing add-station thread, else @ref ET_THREAD_KEEP. */
  unsigned int     heartbeat;      /**< Heartbeat which periodically increments to indicate ET is alive. */
  int              hz;             /**< System clock rate in Hz. */
  int              nstations;      /**< Current number of stations idle or active. */
  int              ntemps;         /**< Current number of temp events. */
  int              nprocesses;     /**< Current number of local processes which have opened system. */
  int              nattachments;   /**< Current number of attachments to stations in system. */
  int              port;           /**< TCP server port. */
  int              tcpFd;          /**< TCP server's listening socket file descriptor. Used to close socket when killed. */
  int              udpFd;          /**< UDP server's listening socket file descriptor. Used to close socket when killed. */
  et_stat_id       stat_head;      /**< Index to head of linked list of used stations (not storing this as a pointer
                                        makes for an awkward linked list, but it survives mapping the shared
                                        memory to a different spot). */
  et_stat_id       stat_tail;      /**< Index to tail of linked list of used stations (not storing this as a pointer
                                        makes for an awkward linked list, but it survives mapping the shared
                                        memory to a different spot). */
  pid_t            mainpid;        /**< Unix pid of ET system. */
  void            *pmap;           /**< Pointer to mapped mem in ET system's process (used for user-called routines to
                                        read/write pointers correctly from shared mem). */
  pthread_mutex_t  mutex;          /**< Pthread mutex to protect system data during changes. */
  pthread_mutex_t  stat_mutex;     /**< Pthread mutex to protect station data during changes. */
  pthread_mutex_t  statadd_mutex;  /**< Pthread mutex used to add stations one at a time. */
  pthread_cond_t   statadd;        /**< Pthread condition variable used to add new stations. */
  pthread_cond_t   statdone;       /**< Pthread condition variable used to signal end of station creation. */
  pthread_t        tid_hb;         /**< System heartbeat thread id. */
  pthread_t        tid_hm;         /**< System heartmonitor thread id. */
  pthread_t        tid_as;         /**< System add station thread id. */
  pthread_t        tid_srv;        /**< ET TCP server thread id. */
  pthread_t        tid_mul;        /**< Id of thread spawning broad/multicast listening threads. */
  char             host[ET_MAXHOSTNAMELEN];     /**< Host of the ET system. */
  struct et_proc   proc[ET_PROCESSES_MAX];      /**< Array of info on processes. */
  struct et_attach attach[ET_ATTACHMENTS_MAX];  /**< Array of info on attachments. */
  et_sys_config    config;        /**< Configuration parameters used to create ET system. */
} et_system;


/** Structure holding all configuration parameters used to open an ET system. */
typedef struct et_open_config_t {
    int             init;            /**< @ref ET_STRUCT_OK if structure initialized, else @ref ET_STRUCT_NEW. */
    int             wait;            /**< @ref ET_OPEN_WAIT if user wants to wait for ET system to appear, else
                                          @ref ET_OPEN_NOWAIT. */
    int             cast;            /**< @ref ET_BROADCAST for users to discover host & port # of ET system server
                                          by broadcasting,
                                          @ref ET_MULTICAST for users to discover host & port # of ET system server
                                          by multicasting,
                                          @ref ET_BROADANDMULTICAST for users to discover host & port # of ET system
                                          server by both broad and multicasting, or
                                          @ref ET_DIRECT when users specify host & TCP port # of ET system. */
    int             ttl;             /**< Multicast ttl value (number of router hops permitted). */
    int             mode;            /**< @ref ET_HOST_AS_REMOTE if connection to a local ET system is made as if
                                          the client were remote, or
                                          @ref ET_HOST_AS_LOCAL if shared memory is to be used if local. */
    int             debug_default;   /**< Default debug output level which may be @ref ET_DEBUG_NONE,
                                          @ref ET_DEBUG_SEVERE, @ref ET_DEBUG_ERROR, @ref ET_DEBUG_WARN, or
                                          @ref ET_DEBUG_INFO. */
    int             udpport;         /**< Port number for broadcast, multicast & direct UDP communication. */
    int             serverport;      /**< Port number for ET system's TCP server thread. */
    int             policy;          /**< Policy to determine which responding ET system to a broad/ulticast will be
                                          chosen as the official respondent: 1) @ref ET_POLICY_ERROR - return error if
                                          more than one response, else return the single respondent, 2)
                                          @ref ET_POLICY_FIRST - pick the first to respond, 3) @ref ET_POLICY_LOCAL -
                                          pick the local system first and if it doesn't respond, the first that does. */
    int             tcpSendBufSize;  /**< TCP send buffer size in bytes of socket connecting to ET TCP server. */
    int             tcpRecvBufSize;  /**< TCP receive buffer size in bytes of socket connecting to ET TCP server. */
    int             tcpNoDelay;      /**< If 0, sockets to system have TCP_NODELAY option off, else on. */
    struct timespec timeout;         /**< Max time to wait for ET system to appear if wait = @ref ET_OPEN_WAIT. */
    char            host[ET_MAXHOSTNAMELEN];    /**< Host of ET system which defaults to local host if unset and
                                                     may be @ref ET_HOST_ANYWHERE for an ET system that may be local
                                                     or remote, @ref ET_HOST_REMOTE for an ET system that's remote, or
                                                     @ref ET_HOST_LOCAL for an ET system that is local
                                                     (may be in dot-decimal form). */
    char            interface[ET_IPADDRSTRLEN]; /**< Dot-decimal IP address specifying the network interface. */
    codaIpAddr       *netinfo;       /**< Linked list of structs containing all network info. */
    codaIpList       *bcastaddrs;    /**< Linked list of all local subnet broadcast addresses (dot-decimal). */
    codaDotDecIpAddrs mcastaddrs;    /**< All multicast addresses (dot-decimal). */
} et_open_config;


/**
 * Structure defining an ET system user id (one needed for each system in use) which contains pointers to
 * key mem locations, config info, status info, node locality and remote node info.
 */
typedef struct  et_id_t {
    int              init;          /**< @ref ET_STRUCT_OK if structure initialized, else @ref ET_STRUCT_NEW. */
    int              lang;          /**< Language this ET system was written in: @ref ET_LANG_C, @ref ET_LANG_CPP, or
                                         @ref ET_LANG_JAVA. */
    int              alive;         /**< Is system alive? 1 = yes, 0 = no. */
    int              closed;        /**< Has @ref et_close been called? 1 = yes, 0 = no. */
    int              bit64;         /**< 1 if the ET system is a 64 bit executable, else 0. */
    et_proc_id       proc;          /**< Unique process id# for process connected to ET system (index into data
                                         stored in the @ref et_system struct) which for a call to @ref et_system_start
                                         is -1 (this is because it refers to the system itself and is not "connected"
                                         to the ET system). */
    int              race;          /**< Flag used to eliminate race conditions. */
    int              cleanup;       /**< Flag used to warn certain routines (such as @ref et_station_detach and
                                         @ref et_restore_events) that they are being executed by the main ET system
                                         process because the ET system is cleaning up after an ET client's crash
                                         (as opposed to being executed by a client) and this prevents mutexes from
                                         being grabbed during cleanup and temp events from being unlinked during
                                         normal usage. */
    int              debug;         /**< Level of desired printed debug output which may be @ref ET_DEBUG_NONE,
                                         @ref ET_DEBUG_SEVERE, @ref ET_DEBUG_ERROR, @ref ET_DEBUG_WARN, or
                                         @ref ET_DEBUG_INFO. */
    int              nevents;       /**< Total number of events in ET system. */
    int              group;         /**< Default event group for calls to @ref et_event_new and @ref et_events_new. */
    int              version;       /**< Major version number of this ET software release. */
    int              nselects;      /**< Number of selection ints per station (or control ints per event). */
    int              share;         /**< @ref ET_MUTEX_SHARE if operating system can share mutexes between local
                                         processes, or @ref ET_MUTEX_NOSHARE for operating systems that cannot - like
                                         MacOS. */
    size_t           memsize;       /**< Total size of shared memory in bytes - used to unmap the mmapped file when
                                         it's already been deleted. */
    uint64_t         esize;         /**< Size in bytes of events in ET system. */
    ptrdiff_t        offset;        /**< Offset between pointers to same shared memory for ET & user's processes
                                         (needed for user process to make sense of pointers in shared memory). */

    /* for REMOTE use */
    int              locality;      /**< @ref ET_LOCAL if process is on same machine as ET system, @ref ET_REMOTE
                                         if process is on another machine, @ref ET_LOCAL_NOSHARE if process is on
                                         same machine as ET but cannot shared mutexes between processes (like MacOS). */
    int              sockfd;        /**< File descriptor of client's TCP socket connection to ET server. */
    int              endian;        /**< Endianness of client's node @ref ET_ENDIAN_BIG or @ref ET_ENDIAN_LITTLE. */
    int              systemendian;  /**< Endianness of ET system's node @ref ET_ENDIAN_BIG or @ref ET_ENDIAN_LITTLE. */
    int              iov_max;       /**< Store the operating system's value iovmax of client's node, default to
                                         @ref ET_IOV_MAX (how many separate buffers can be sent with one writev() call). */
    int              port;          /**< Port number for ET TCP server. */
    char             ethost[ET_MAXHOSTNAMELEN];   /**< Host of the ET system. */
    char             localAddr[ET_IPADDRSTRLEN];  /**< Local dot-decimal address of socket connection to ET. */
    /******************/

    void            *pmap;          /**< Pointer to start of shared (mapped) memory. */
    et_system       *sys;           /**< Pointer to @ref et_system structure in shared memory. */
    et_station      *stats;         /**< Pointer to start of @ref et_station structures in shared memory. */
    int             *histogram;     /**< Pointer to histogram data in shared memory. */
    et_event        *events;        /**< Pointer to start of @ref et_event structures in shared memory. */
    char            *data;          /**< Pointer to start of event data in shared memory. */
    et_station      *grandcentral;  /**< Pointer to grandcentral station in shared memory. */
    pthread_mutex_t  mutex;         /**< Pthread mutex for thread-safe remote communications. */
#ifndef NO_RW_LOCK
    pthread_rwlock_t sharedMemlock; /**< Pthread read-write lock for preventing access of unmapped memory after
                                         calling @ref et_close. */
#endif
} et_id;


/** Structure containing info stored at front of shared or mapped memory. */
typedef struct  et_mem_t {
    uint32_t  byteOrder;       /**< Should be 0x01020304, if not, byte order is reversed from local order. */
    uint32_t  systemType;      /**< Type of local system using the shared memory: @ref ET_SYSTEM_TYPE_C is an ET
                                    system written in C, and @ref ET_SYSTEM_TYPE_JAVA is an ET system written in Java
                                    with a different layout of the shared memory. */
    uint32_t  majorVersion;    /**< Major version number of this ET software release. */
    uint32_t  minorVersion;    /**< Minor version number of this ET software release. */
    uint32_t  numSelectInts;   /**< Number of station selection integers per event. */
    uint32_t  headerByteSize;  /**< Total size of a single event's "header" or metadata structure in bytes. */
    
    uint64_t  eventByteSize;   /**< Total size of a single event's data memory in bytes. */
    uint64_t  headerPosition;  /**< Number of bytes past beginning of shared memory that the headers are stored. */
    uint64_t  dataPosition;    /**< Number of bytes past beginning of shared memory that the data are stored. */
    uint64_t  totalSize;       /**< Total size of shared (mapped) memory (must be allocated in pages). */
    uint64_t  usedSize;        /**< Desired size of shared memory given as arg to @ref et_mem_create . */
} et_mem;


/** Structure for holding an ET system's single response to ET client's broad/multicast. */
typedef struct et_response_t {
    int   port;                     /**< ET system's TCP server port. */
    int   castType;                 /**< @ref ET_BROADCAST or @ref ET_MULTICAST (what this is a response to). */
    int   addrCount;                /**< Number of addresses. */
    char  uname[ET_MAXHOSTNAMELEN]; /**< Uname of sending host. */
    char  canon[ET_MAXHOSTNAMELEN]; /**< Canonical name of sending host. */
    char  castIP[ET_IPADDRSTRLEN];  /**< Original broad/multicast IP addr. */
    uint32_t *addrs;                /**< Array of 32bit net byte ordered addresses (1 for each addr). */
    char  **ipaddrs;                /**< Array of all IP addresses (dot-decimal string) of host. */
    char  **bcastaddrs;             /**< Array of all broadcast addresses (dot-decimal string) of host. */
    struct et_response_t *next;     /**< Next response in linked list. */
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


/** Struct for passing data from system to network threads. */
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
 * Structure to define configuration parameters used to bridge ET systems
 * (transfer events between 2 systems).
 */
typedef struct et_bridge_config_t {
    int             init;          /**< @ref ET_STRUCT_OK if structure initialized, else @ref ET_STRUCT_NEW. */
    int             mode_from;     /**< @ref ET_SLEEP, @ref ET_TIMED, or @ref ET_ASYNC for getting events
                                        from the "from" ET system. */
    int             mode_to;       /**< @ref ET_SLEEP, @ref ET_TIMED, or @ref ET_ASYNC for getting events
                                        from the "to" ET system into which the "from" events go. */
    int             chunk_from;    /**< Number of events to get from the "from" ET system at one time. */
    int             chunk_to;      /**< Number of new events to get from the "to" ET system at one time. */
    struct timespec timeout_from;  /**< Max time to wait when getting events from the "from" ET system. */
    struct timespec timeout_to;    /**< Max time to wait when getting new events from the "to" ET system. */
    ET_SWAP_FUNCPTR func;          /**< Pointer to function which takes a pointer to an event as an argument,
                                        swaps the data, and returns ET_ERROR if there's a problem & ET_OK if not. */
} et_bridge_config;


/****************************
 *    END BRIDGE STUFF      *
 ****************************/

/** Macro to get pointer to event from system id and event place. */
#define ET_P2EVENT(etid, place) ((et_event *)((et_event *)(etid->events) + (place)))

/* Macros to switch ptrs from ET space to user space & vice versa. */

/** Macro to change event pointer from ET system space to user space. */
#define ET_PEVENT2USR(p, offset) ((et_event *)((char *)(p) + (offset)))
/** Macro to change event pointer from user space to ET system space. */
#define ET_PEVENT2ET(p, offset)  ((et_event *)((char *)(p) - (offset)))

/** Macro to change station pointer from ET system space to user space. */
#define ET_PSTAT2USR(p, offset)  ((et_station *)((char *)(p) + (offset)))
/** Macro to change station pointer from user space to ET system space. */
#define ET_PSTAT2ET(p, offset)   ((et_station *)((char *)(p) - (offset)))

/** Macro to change data pointer from ET system space to user space. */
#define ET_PDATA2USR(p, offset)  ((void *)((char *)(p) + (offset)))
/** Macro to change data pointer from user space to ET system space. */
#define ET_PDATA2ET(p, offset)   ((void *)((char *)(p) - (offset)))

/* Macros to break a 64 int into 2, 32 bit ints & the reverse. */

/** Macro to take 2 ints (up to 64 bits each) and change into 1, unsigned 64-bit int
 * (<b>hi</b> becomes the high 32 bits and <b>lo</b> becomes the low  32 bits).*/
#define ET_64BIT_UINT(hi,lo)       (((uint64_t)(hi) << 32) | ((uint64_t)(lo) & 0x00000000FFFFFFFF))

/** Macro to take 2 ints (up to 64 bits each) and change into 1, signed 64-bit int
 * (<b>hi</b> becomes the high 32 bits and <b>lo</b> becomes the low  32 bits).*/
#define ET_64BIT_INT(hi,lo)         (((int64_t)(hi) << 32) | ((int64_t)(lo)  & 0x00000000FFFFFFFF))

/** Macro to take 2 ints (up to 64 bits each) and change into 64-bit void pointer
 * (<b>hi</b> becomes the high 32 bits and <b>lo</b> becomes the low  32 bits).*/
#define ET_64BIT_P(hi,lo) ((void *)(((uint64_t)(hi) << 32) | ((uint64_t)(lo) & 0x00000000FFFFFFFF)))

/** Macro to take a 64-bit unsigned int and return the high 32 bits as an unsigned 32-bit int. */
#define ET_HIGHINT(i)         ((uint32_t)(((uint64_t)(i) >> 32) & 0x00000000FFFFFFFF))

/** Macro to take a 64-bit unsigned int and return the low 32 bits as an unsigned 32-bit int. */
#define ET_LOWINT(i)           ((uint32_t)((uint64_t)(i) & 0x00000000FFFFFFFF))

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
extern void  et_mem_write_first_block(char *ptr,
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
extern int  etl_kill(et_sys_id id);
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
                           uint32_t *inetaddr, et_open_config *config,
                           et_response **allETinfo, int debug);
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
