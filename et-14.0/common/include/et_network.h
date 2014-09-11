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
 *      Header for routines dealing with network communications
 *
 *----------------------------------------------------------------------------*/
 
#ifndef __et_network_h
#define __et_network_h

#ifdef VXWORKS
#include <ioLib.h>       /* writev */
#include <inetLib.h>    /* htonl stuff */
#else
#include <arpa/inet.h>	 /* htonl stuff */
#endif

#ifdef sun
#include <sys/sockio.h>  /* find broacast addr */
#else
#include <sys/ioctl.h>   /* find broacast addr */
#endif

#include "et_private.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if defined sun || defined linux || defined VXWORKS || defined __APPLE__
#  define socklen_t int
#endif

#ifdef linux
  #ifndef _SC_IOV_MAX
   #define _SC_IOV_MAX _SC_T_IOV_MAX
  #endif
#endif

#define	SA                  struct sockaddr
#define LISTENQ             10
#define ET_SOCKBUFSIZE      49640  /* multiple of 1460 - ethernet MSS */
#define ET_IOV_MAX          16     /* minimum for POSIX systems */

/* are 2 nodes the same or different? */
#define ET_NODE_SAME 0
#define ET_NODE_DIFF 1

/*
 * Make solaris compatible with Linux. On Solaris,
 * _BIG_ENDIAN  or  _LITTLE_ENDIAN is defined
 * depending on the architecture.
 */
#ifdef sun

  #define __LITTLE_ENDIAN 1234
  #define __BIG_ENDIAN    4321

  #if defined(_BIG_ENDIAN)
    #define __BYTE_ORDER __BIG_ENDIAN
  #else
    #define __BYTE_ORDER __LITTLE_ENDIAN
  #endif

/*
 * On vxworks, _BIG_ENDIAN = 1234 and _LITTLE_ENDIAN = 4321,
 * which is backwards. _BYTE_ORDER is also defined.
 * In types/vxArch.h, these definitions are carefully set
 * to these reversed values. In other header files such as
 * netinet/ip.h & tcp.h, the values are normal (ie
 * _BIG_ENDIAN = 4321). What's this all about?
 */
#elif VXWORKS

  #define __LITTLE_ENDIAN 1234
  #define __BIG_ENDIAN    4321

  #if _BYTE_ORDER == _BIG_ENDIAN
    #define __BYTE_ORDER __BIG_ENDIAN
  #else
    #define __BYTE_ORDER __LITTLE_ENDIAN
  #endif

#endif


/* Byte swapping for 64 bits. */
#define hton64(x) ntoh64(x)
#if __BYTE_ORDER == __BIG_ENDIAN
#define ntoh64(x) x
#else
#define ntoh64(x) et_ntoh64(x)
#endif

/*
 * Ints representing ascii for "ET is Grreat",
 * used to filter out portscanning software.
 */
#define ET_MAGIC_INT1 0x45543269
#define ET_MAGIC_INT2 0x73324772
#define ET_MAGIC_INT3 0x72656174


/* our prototypes */
extern uint64_t et_ntoh64(uint64_t n);

extern int   et_CODAswap(int *src, int *dest, int nints, int same_endian);

extern int   et_findserver2(const char *etname, char *ethost, int *port,
                            uint32_t *inetaddr, et_response **allETinfo,
                            et_open_config *config, int trys,
			                struct timeval *waittime);
                            
extern void  et_freeAnswers(et_response *answer);

extern codaIpList *et_orderIpAddrs(et_response *response, codaIpAddr *netinfo);

#ifdef	__cplusplus
}
#endif

#endif
