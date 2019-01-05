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
 *      Header for routines dealing with the encoding and decoding of
 *      ET system information.
 *
 *----------------------------------------------------------------------------*/

#ifndef __et_data_h
#define __et_data_h

#include <sys/uio.h>     /* iovec */
#include "et_private.h"

#ifdef    __cplusplus
extern "C" {
#endif

/******************************************************
 * Define structures that will help us organize
 * all ET system information.
 ******************************************************/

/* System Information */
typedef struct et_sysdata_t {
  /* values which can change */
  int alive;            /* is system alive? */
  int heartbeat;        /* heartbeat count */
  int mutex;            /* is system mutex locked? */
  int stat_mutex;       /* is station mutex locked ? */
  int statadd_mutex;    /* is add station mutex locked ? */
  int ntemps;           /* current # of temp events */
  int nstations;        /* cuurent # of stations (idle or active) */
  int nattachments;     /* current # of attachments */
  int nprocesses;       /* current # of processes */
  int events_owned;     /* # of events owned by the system */

  /* values which do NOT change */
  int endian;           /* endian value of host running system */
  int share;            /* can system share mutexes between processes? */
  int mainpid;          /* pid of ET system process */
  int nselects;         /* number of ints in station's select array */
  int nevents;          /* total # of events */
  int bit64;            /* 1 if 64 bit ET system, else 0 */
  uint64_t event_size;  /* size of "normal" events in bytes */
  int ntemps_max;       /* max # of temp events allowed */
  int nstations_max;    /* max # of stations allowed */
  int nattachments_max; /* max # of attachments allowed */
  int nprocesses_max;   /* max # of processes allowed */

  int tcp_port;         /* port of ET server */
  int udp_port;         /* port for udp direct/broadcast to find server */
  int multi_port;       /* port for udp multicast to find server (not used, same as udp_port) */

  codaDotDecIpAddrs ifaddrs;    /* list of interface addresses (dotted-dec) */
  codaDotDecIpAddrs mcastaddrs; /* list of multicast addresses (dotted-dec) */

  char filename[ET_FILENAME_LENGTH]; /* name of ET system file */
} et_sysdata;


/* Station Information */
typedef struct et_statdata_t {
  int num;                      /* unique id of this station */
  int status;                   /* status */
  int mutex;                    /* is event transfer mutex locked? */
  int nattachments;             /* # of attachments to this station */
  int att[ET_ATTACHMENTS_MAX];  /* id #'s of these attachments */

  int      inlist_mutex;  /* input list mutex status */
  int      inlist_cnt;    /* # of events in input list */
  uint64_t inlist_try;    /* # of events that were tried to put in */
  uint64_t inlist_in;     /* # of events put into input list */
  int      outlist_mutex; /* output list mutex status */
  int      outlist_cnt;   /* # of events in output list */
  uint64_t outlist_out;   /* # of events taken out output list */

  int flow_mode;          /* station definition parameters */
  int user_mode;
  int restore_mode;
  int block_mode;
  int prescale;
  int cue;
  int select_mode;
  int select[ET_STATION_SELECT_INTS];

  char fname[ET_FUNCNAME_LENGTH];
  char lib[ET_FILENAME_LENGTH];
  char classs[ET_FILENAME_LENGTH];
  char name[ET_STATNAME_LENGTH];    /* station name */
} et_statdata;


/* Attachment Information */
typedef struct et_attdata_t {
  int num;              /* unique id # */
  int proc;             /* id # of proc that created this attachment */
  int stat;             /* id # of station attached to */
  int pid;              /* pid of unix process that created this att */
  int blocked;          /* is blocked waiting to read events? */
  int quit;             /* has it been told to quit reading & return */
  int owned;            /* # of events owned */

  uint64_t events_put;  /* # of events put back into station */
  uint64_t events_get;  /* # of events read from station */
  uint64_t events_dump; /* # of events dumped */
  uint64_t events_make; /* # of new events made */

  char host[ET_MAXHOSTNAMELEN];     /* name of host running att */
  char station[ET_STATNAME_LENGTH]; /* name of station attached to */
  char interface[ET_IPADDRSTRLEN];  /* IP address of interface data sent through */
} et_attdata;


/* Process Information */
typedef struct et_procdata_t {
  int num;                     /* unique id of process */
  int heartbeat;               /* heartbeat count */
  int pid;                     /* pid of this unix process */
  int nattachments;            /* # of attachment this process created */
  int att[ET_ATTACHMENTS_MAX]; /* id #'s of these attachments */
} et_procdata;


/* System,Station,Attachment & Process Information */
typedef struct et_alldata_t {
  int nstations;
  int natts;
  int nprocs;
  et_sysdata  *sysdata;
  et_statdata *statdata;
  et_attdata  *attdata;
  et_procdata *procdata;
} et_alldata;

/* packing ET system data into buffer routines */
extern int   et_data_sys(et_id *id, struct iovec *iov);
extern int   et_data_atts(et_id *id, struct iovec *iov);
extern int   et_data_procs(et_id *id, struct iovec *iov);
extern int   et_data_stats(et_id *id, struct iovec *iov);
extern int   et_data_get(et_sys_id id, et_alldata *alldata);
extern void  et_data_free(et_alldata *alldata);
extern int   et_data_gethistogram(et_sys_id id, int hist[], int size);


#ifdef    __cplusplus
}
#endif

#endif
