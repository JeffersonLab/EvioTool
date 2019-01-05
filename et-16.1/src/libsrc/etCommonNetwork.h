/*----------------------------------------------------------------------------*
 *  Copyright (c) 2004        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-6B         *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      Header for coda routines dealing with network communications
 *
 *----------------------------------------------------------------------------*/
 
#ifndef __etCommonNetwork_h
#define __etCommonNetwork_h


/*#include <netinet/ip.h>*/  /* IPTOS_LOWDELAY defn */
#include <netinet/in.h>	 /* sockaddr_in{}, sockaddr_storage{} and other Internet defns */
#include <netinet/tcp.h> /* TCP_NODELAY def */
#include <net/if.h>	     /* find broacast addr */
#include <sys/time.h>    /* struct timeval */


/* cmsg or et definitions here */
#include "et.h"


#ifdef	__cplusplus
extern "C" {
#endif

//extern char *codanetStr;

/* set debug level for network stuff here */
extern int etDebug;

/* rename variables & routines to avoid naming conflicts w/ cMsg network lib */
#define   codanetStr                    "etNet"
#define   CODA_OK                       ET_OK
#define   CODA_ERROR                    ET_ERROR
#define   CODA_TIMEOUT                  ET_ERROR_TIMEOUT
#define   CODA_BAD_ARGUMENT             ET_ERROR_BADARG
#define   CODA_NETWORK_ERROR            ET_ERROR_NETWORK
#define   CODA_SOCKET_ERROR             ET_ERROR_SOCKET
#define   CODA_OUT_OF_MEMORY            ET_ERROR_NOMEM

#define   CODA_DEBUG_NONE               0
#define   CODA_DEBUG_INFO               1
#define   CODA_DEBUG_WARN               1
#define   CODA_DEBUG_ERROR              1
#define   CODA_DEBUG_SEVERE             1

#define   CODA_ENDIAN_BIG               ET_ENDIAN_BIG
#define   CODA_ENDIAN_LITTLE            ET_ENDIAN_LITTLE

#define   coda_err_abort                err_abort

#define   codanetDebug                  etDebug

#define   codanetAccept                 etNetAccept
#define   codanetTcpListen              etNetTcpListen
#define   codanetTcpConnect             etNetTcpConnect
#define   codanetTcpConnect2            etNetTcpConnect2
#define   codanetTcpConnectTimeout      etNetTcpConnectTimeout
#define   codanetTcpConnectTimeout2     etNetTcpConnectTimeout2
#define   codanetGetListeningSocket     etNetGetListeningSocket
#define   codanetUdpReceive             etNetUdpReceive
#define   codanetUdpReceiveAll          etNetUdpReceiveAll

#define   codanetTcpRead                etNetTcpRead
#define   codanetTcpRead3iNB            etNetTcpRead3iNB
#define   codanetTcpWrite               etNetTcpWrite
#define   codanetTcpWritev              etNetTcpWritev

#define   codanetIsDottedDecimal        etNetIsDottedDecimal
#define   codanetOnSameSubnet           etNetOnSameSubnet
#define   codanetOnSameSubnet2          etNetOnSameSubnet2
#define   codanetLocalHost              etNetLocalHost
#define   codanetLocalAddress           etNetLocalAddress
#define   codanetLocalByteOrder         etNetLocalByteOrder
#define   codanetLocalSocketAddress     etNetLocalSocketAddress

#define   codanetNodeSame               etNetNodeSame
#define   codanetNodeIsLocal            etNetNodeIsLocal
#define   codanetGetUname               etNetGetUname
#define   codanetIsLinux                etNetIsLinux
#define   codanetSetInterface           etNetSetInterface

#define   codanetHstrerror              etNetHstrerror
#define   codanetStringToNumericIPaddr  etNetStringToNumericIPaddr

#define   codanetGetInterfaceInfo          etNetGetInterfaceInfo
#define   codanetFreeInterfaceInfo         etNetFreeInterfaceInfo
#define   codanetFreeIpAddrs               etNetFreeIpAddrs
#define   codanetGetNetworkInfo            etNetGetNetworkInfo
#define   codanetFreeAddrList              etNetFreeAddrList
#define   codanetGetBroadcastAddrs         etNetGetBroadcastAddrs
#define   codanetGetIpAddrs                etNetGetIpAddrs
#define   codanetOrderIpAddrs              etNetOrderIpAddrs
#define   codanetMcastSetIf                etNetMcastSetIf
#define   codanetGetIfNames                etNetGetIfNames
#define   codanetGetMatchingLocalIpAddress etNetGetMatchingLocalIpAddress
#define   codanetGetBroadcastAddress       etNetGetBroadcastAddress
    


/* convenient network definitions (from Richard Stevens ) */
#define SA                  struct sockaddr
#define LISTENQ             10
#define INET_ATON_ERR       0



/*****************************************************************************
 * The following is taken from R. Stevens book, UNIX Network Programming, 3rd ed.
 * Chapter 17. Used for finding network info. Yes, we stole the code.
 * However, we also added the subnet mask to it.
 */
#define IFI_NAME    16  /* same as IFNAMSIZ in <net/if.h> */
#define IFI_HADDR    8  /* allow for 64-bit EUI-64 in future */
#define IFI_ALIAS    1  /* ifi_addr is an alias */

struct ifi_info {
    char    ifi_name[IFI_NAME];   /* interface name, null terminated */
    u_char  ifi_haddr[IFI_HADDR]; /* hardware address */
    u_short ifi_hlen;             /* # bytes in hardware address: 0, 6, 8 */
    short   ifi_flags;            /* IFF_xxx constants from <net/if.h> */
    short   ifi_myflags;          /* our own IFI_xxx flags */
    struct sockaddr  *ifi_addr;   /* primary address */
    struct sockaddr  *ifi_brdaddr;/* broadcast address */
    struct sockaddr  *ifi_dstaddr;/* destination address */
    struct sockaddr  *ifi_netmask;/* subnet mask */
    struct ifi_info  *ifi_next;   /* next of these structures */
};
/*****************************************************************************/


/* *****************************************************************
 *      items to handle multiple network addresses or names        *
 * *****************************************************************/
/* max # of network addresses/names per host we'll examine */
#define CODA_MAXADDRESSES 10
/*
 * String length of dotted-decimal, ip address string
 * Some systems - but not all - define INET_ADDRSTRLEN
 * ("ddd.ddd.ddd.ddd\0" = 16)
 */
#define CODA_IPADDRSTRLEN 16

/*
 * MAXHOSTNAMELEN is defined to be 256 on Solaris and is the max length
 * of the host name so we add one for the terminator. On Linux the
 * situation is less clear but 257 appears to be the max (whether that
 * includes termination is not clear).
 * We need it to be uniform across all platforms since we transfer
 * this info across the network. Define it to be 256 for everyone.
 */
#define CODA_MAXHOSTNAMELEN 256


/** Structure to handle dotted-decimal IP addresses in a linked list. */
typedef struct codaIpList_t {
    char                addr[CODA_IPADDRSTRLEN];  /**< Single dotted-decimal address for IP address. */
    char                bAddr[CODA_IPADDRSTRLEN]; /**< Single dotted-decimal address for broadcast address. */
    struct codaIpList_t *next;                    /**< Next item in linked list. */
} codaIpList;

/** Structure to handle multiple dotted-decimal IP addresses. */
typedef struct codaDotDecIpAddrs_t {
    int       count;                                      /**< Number of valid addresses in this structure. */
    char      addr[CODA_MAXADDRESSES][CODA_IPADDRSTRLEN]; /**< Array of addresses. */
    pthread_t tid[CODA_MAXADDRESSES];                     /**< Array of pthread thread ids. */
} codaDotDecIpAddrs;


/**
 * This structure holds a single IP address (dotted-decimal
 * and binary forms) along with its canonical name, name aliases,
 * broadcast address, and a place to store a thread id.
 */
typedef struct codaIpAddr_t {
    int    aliasCount;                   /**< Number of aliases stored in this structure. */
    char   **aliases;                    /**< Array of alias strings. */
    char   addr[CODA_IPADDRSTRLEN];      /**< IP address in dotted-decimal form. */
    char   canon[CODA_MAXHOSTNAMELEN];   /**< Canonical name of host associated with addr. */
    char   broadcast[CODA_IPADDRSTRLEN]; /**< Broadcast address in dotted-decimal form. */
    struct sockaddr_in saddr;            /**< Binary form of IP address. */
    struct sockaddr_in netmask;          /**< Binary form of subnet mask. */
    struct codaIpAddr_t *next;           /**< Next item in linked list. */
} codaIpAddr;

/**
 * This structure holds a single IP address (dotted-decimal
 * and binary forms) along with its canonical name, name aliases,
 * broadcast address, and a place to store a thread id. This
 * form is for use with shared memory since it is of fixed size.
 */
typedef struct codaIpInfo_t {
    int    aliasCount;                   /**< Number of aliases stored in this item. */
    char   addr[CODA_IPADDRSTRLEN];      /**< IP address in dotted-decimal form. */
    char   canon[CODA_MAXHOSTNAMELEN];   /**< Canonical name of host associated with addr. */
    char   broadcast[CODA_IPADDRSTRLEN]; /**< Broadcast address in dotted-decimal form. */
    char   aliases[CODA_MAXADDRESSES][CODA_MAXHOSTNAMELEN]; /**< Array of alias strings. */
    struct sockaddr_in saddr;            /**< Binary form of IP address (net byte order). */
    struct sockaddr_in netmask;          /**< Binary form of subnet mask. */
} codaIpInfo;

/**
 * This structure stores an array of codaIpInfo structures in order to represent
 * all the network interface data of a computer.
 */
typedef struct codaNetInfo_t {
    int       count;                      /**< Number of valid array items. */
    codaIpInfo ipinfo[CODA_MAXADDRESSES]; /**< Array of structures each of which holds a
    single IP address and its related data. */
} codaNetInfo;


    
/* routine prototypes */
extern int   codanetTcpListen(int nonblocking, unsigned short port, int sendBufSize,
                              int rcvBufSize, int noDelay, int *listenFd);
extern int   codanetTcpConnect(const char *ip_address, const char *interface, unsigned short port,
                               int sendBufSize, int rcvBufSize, int noDelay, int *fd, int *localPort);
extern int   codanetTcpConnect2(uint32_t inetaddr, const char *interface, unsigned short port,
                                int sendBufSize, int rcvBufSize, int noDelay, int *fd, int *localPort);
extern int   codanetTcpConnectTimeout(const char *ip_address, unsigned short port,
                                      int sendBufSize, int rcvBufSize,
                                      int noDelay, struct timeval *timeout,
                                      int *fd, int *localPort);
extern int   codanetTcpConnectTimeout2(const char *ip_address, const char *interface, unsigned short port,
                                       int sendBufSize, int rcvBufSize, int noDelay, struct timeval *timeout,
                                       int *fd, int *localPort);
extern int   codanetGetListeningSocket(int nonblocking, unsigned short startingPort,
                                       int sendBufSize, int rcvBufSize, int noDelay,
                                       int *finalPort, int *fd);
extern int   codanetAccept(int fd, struct sockaddr *sa, socklen_t *salenptr);
extern int   codanetUdpReceive(unsigned short port, const char *address, int multicast, int *fd);
extern int   codanetUdpReceiveAll(unsigned short port, char multicastAddrs[][CODA_IPADDRSTRLEN], int addrCount, int *fd);

extern int   codanetTcpRead(int fd, void *vptr, int n);
extern int   codanetTcpRead3iNB(int fd, int *i1, int *i2, int *i3);
extern int   codanetTcpWrite(int fd, const void *vptr, int n);
extern int   codanetTcpWritev(int fd, struct iovec iov[], int nbufs, int iov_max);

extern int   codanetLocalHost(char *host, int length);
extern int   codanetLocalAddress(char *address);
extern int   codanetLocalByteOrder(int *endian);
extern int   codanetLocalSocketAddress(int sockfd, char *ipAddress);

extern int   codanetIsDottedDecimal(const char *ipAddress, int *decimals);
extern int   codanetOnSameSubnet(const char *ipAddress1, const char *ipAddress2,
                                 const char *subnetMask, int *sameSubnet);
extern int   codanetOnSameSubnet2(const char *ipAddress1, const char *ipAddress2,
                                  uint32_t subnetMask, int *sameSubnet);
extern int   codanetNodeSame(const char *node1, const char *node2, int *same);
extern int   codanetNodeIsLocal(const char *host, int *isLocal);
extern int   codanetGetUname(char *host, int length);
extern int   codanetIsLinux(int *isLinux);
extern int   codanetSetInterface(int fd, const char *ip_address);

extern int   codanetStringToNumericIPaddr(const char *ip_address, struct sockaddr_in *addr);
extern const char *codanetHstrerror(int err);

extern struct ifi_info *codanetGetInterfaceInfo(int family, int doaliases);
extern void  codanetFreeInterfaceInfo(struct ifi_info *ifihead);
extern void  codanetFreeIpAddrs(codaIpAddr *ipaddr);
extern int   codanetGetNetworkInfo(codaIpAddr **ipaddrs, codaNetInfo *info);
extern void  codanetFreeAddrList(codaIpList *addr);
extern int   codanetGetBroadcastAddrs(codaIpList **addrs, codaDotDecIpAddrs *bcaddrs);
extern int   codanetGetIpAddrs(char ***ipAddrs, int *count, char *host);
extern int   codanetMcastSetIf(int sockfd, const char *ifname, uint32_t ifindex);
extern int   codanetGetIfNames(char ***ifNames, int *count);
extern codaIpList *codanetOrderIpAddrs(codaIpList *ipList, codaIpAddr *netinfo,
                                       char* preferredSubnet, int* noSubnetMatch);
extern int   codanetGetMatchingLocalIpAddress(char *ip, char **matchingIp);
extern int   codanetGetBroadcastAddress(char *ip, char **broadcastIp);
    

    

#ifdef	__cplusplus
}
#endif

#endif
