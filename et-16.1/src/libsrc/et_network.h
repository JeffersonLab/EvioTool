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

#include <arpa/inet.h>	 /* htonl stuff */
#include <sys/ioctl.h>   /* find broacast addr */

#include "et_private.h"

#ifdef	__cplusplus
extern "C" {
#endif

//#define socklen_t int

#ifdef linux
  #ifndef _SC_IOV_MAX
   #define _SC_IOV_MAX _SC_T_IOV_MAX
  #endif
#endif

#define	SA            struct sockaddr
#define ET_IOV_MAX    16     /* minimum for POSIX systems */


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
			                struct timeval *waittime, int debug);
                            
extern void  et_freeAnswers(et_response *answer);

extern codaIpList *et_orderIpAddrs(et_response *response, codaIpAddr *netinfo,
                                   char* preferredSubnet);

#ifdef	__cplusplus
}
#endif

#endif
