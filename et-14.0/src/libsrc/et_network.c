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
 *      Routines dealing with network communications and byte swapping
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#ifdef VXWORKS

#include <vxWorks.h>
#include <taskLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <ioLib.h>
#include <time.h>
#include <net/uio.h>
#include <net/if_dl.h>

#else

#include <strings.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/utsname.h>
#ifdef __APPLE__
#include <ifaddrs.h>
#endif

#endif

#ifdef sun
#include <sys/filio.h>
#endif
#include "et_network.h"


/**
 * Routine to do a 64 bit swap.
 * @param n 64 bit integer to swap
 * @return 64 bit swapped integer
 */
uint64_t et_ntoh64(uint64_t n) {
    uint64_t h;
    uint64_t tmp = ntohl(n & 0x00000000ffffffff);
    h = ntohl(n >> 32);
    h |= tmp << 32;
    return h;
}
 

static void freeIpAddrs(char **ipAddrs, int count) {
    int i;

    if (count > 0) {
        for (i=0; i < count; i++) {
            free(ipAddrs[i]);
        }
        free(ipAddrs);
    }
}


/**
 * This routine removes an element from a linked list of et_response structures.
 *
 * @param firstAnswer  first element of linked list
 * @param removeAnswer element to remove
 * @return first element of new list;
 *         NULL if nothing left or firstAnswer arg is NULL
 */
static et_response *removeResponseFromList(et_response *firstAnswer, et_response *removeAnswer) {
    et_response *prev=NULL, *next=NULL, *answer=firstAnswer;

    if (firstAnswer == NULL || removeAnswer == NULL) return firstAnswer;

    while (answer != removeAnswer) {
        prev   = answer;
        answer = answer->next;
        if (answer == NULL) {
            /* removeAnswer is not in the list */
            return firstAnswer;
        }
    }

    /* If we're removing the first element ... */
    if (firstAnswer == removeAnswer) {
        next = firstAnswer->next;
        firstAnswer->next = NULL;
        return next;
    }
    
    prev->next = removeAnswer->next;
    removeAnswer->next = NULL;
    return firstAnswer;    
}


/**
 * Routine to free the memory of a et_response structure.
 * @param answer et_response structure to be freed.
 */
void et_freeAnswers(et_response *answer) {
    int i;
    et_response *next;
  
    while (answer != NULL) {
        next = answer->next;
        if (answer->ipaddrs != NULL) {
            for (i=0; i<answer->addrCount; i++) {
                free(answer->ipaddrs[i]);
            }
            free(answer->ipaddrs);
        }
        free(answer->addrs);
        free(answer);
        answer = next;
    }
}


/**
 * This routine takes all the dotted-decimal IP addresses contained in the
 * response arg and returns them in a list ordered so that the addresses on
 * the same subnets as the IP addresses in the netinfo arg are first in the
 * list. This allows an ET client, who is broad/multicasting to find an ET
 * system, to try and make a TCP connection with an address on a local subnet
 * first. All the elements of the returned linked-list need to be freed by the
 * caller.
 *
 * @param response pointer to structure containing ET system's response to a
 *                 broad/multicast which contains all the IP addresses of its host.
 * @param netinfo  pointer to structure containing all local network information
 *
 * @return a linked list of IP addresses in dotted-decimal format with all the
 *         IP addresses in the response arg ordered so that those on the same
 *         subnets as the local host are first. Return NULL if an arg is NULL
 *         or error. If successful, all the elements of the returned linked-list
 *         need to be freed by the caller.
 */
codaIpList *et_orderIpAddrs(et_response *response, codaIpAddr *netinfo) {
    
    int i, err, sameSubnet, firstTime=1;
    char *ipAddress;
    codaIpList *listItem, *lastItem, *firstItem = NULL;
    codaIpAddr *local;


    for (i=0; i < response->addrCount; i++) {
        ipAddress = response->ipaddrs[i];
        local = netinfo;
        sameSubnet = 0;
/*printf("got response address %s\n", ipAddress);*/

        /* Compare with local subnets */
        while (local != NULL) {
            if (local->addr == NULL) break;
/*
printf("ET ip = %s, local ip = %s, local netmask bin = 0x%x\n",
                   ipAddress, local->addr,
                   ntohl(local->netmask.sin_addr.s_addr));
*/

            err = codanetOnSameSubnet2(ipAddress, local->addr,
                                       ntohl(local->netmask.sin_addr.s_addr),
                                       &sameSubnet);
            if (err != CODA_OK) {
                codanetFreeAddrList(firstItem);
                return NULL;
            }

            if (sameSubnet) {
                /* quit if we found a match */
                break;
            }

            local = local->next;
        }

        listItem = (codaIpList *) calloc(1, sizeof(codaIpList));
        if (listItem == NULL) {
            codanetFreeAddrList(firstItem);
            return NULL;
        }

        if (firstTime) {
            lastItem = firstItem = listItem;
            firstTime = 0;
        }

        strncpy(listItem->addr, ipAddress, CODA_IPADDRSTRLEN);

        if (sameSubnet) {
/*printf("same subnet, head of list\n");*/
            /* Put it at the head of the list */
            if (listItem != firstItem) {
                listItem->next = firstItem;
                firstItem = listItem;
            }
        }
        else {
/*printf("diff subnet, end of list\n");*/
            /* Put it at the end of the list */
            if (listItem != lastItem) {
                lastItem->next = listItem;
                lastItem = listItem;
            }
        }
    }

    return firstItem;
}


/**
 * This routine is used with etr_open to find an ET system's tcp host & port.
 * Tries only twice to send broad/multicast packets. Waits 0.1 second for the
 * first response and 1.1 second for the second.
 *
 * @param etname    ET system file name
 * @param ethost    returns name of ET system's host
 * @param port      returns ET system's TCP port
 * @param inetaddr  returns host address as network-byte-ordered, 32-bit unsigned int
 * @param config    configuration passed to et_open
 * @param allETinfo returns a pointer to single structure with all of the ET system's
 *                  response information;
 *                  may be null if all relevant info given in ethost, port, and inetaddr args
 * 
 * @return ET_OK            if successful
 * @return ET_ERROR         failure in select statement
 * @return ET_ERROR_BADARG  NULL arg for ethost, port, or inetaddr
 * @return ET_ERROR_NOMEM   cannot allocate memory
 * @return ET_ERROR_NETWORK error find IP addresses or resolving host name;
 *                          error converting dotted-decimal IP addr to binary;
 *                          error create socket; failure reading socket
 * @return ET_ERROR_SOCKET  error setting socket option
 * @return ET_ERROR_TOOMANY if multiple ET systems responded when policy is to return an error when
 *                          more than one responds.
 * @return ET_ERROR_TIMEOUT if no responses received in allotted time
 */
int et_findserver(const char *etname, char *ethost, int *port,
                  uint32_t *inetaddr, et_open_config *config, et_response **allETinfo)
{
    struct timeval waittime;
    /* wait 0.1 seconds before calling select the first time */
    waittime.tv_sec  = 0;
    waittime.tv_usec = 100000;
  
    return et_findserver2(etname, ethost, port, inetaddr, allETinfo, config, 2, &waittime);
}


/**
 * This routine is used to quickly see if an ET system
 * is alive by trying to get a response from its UDP
 * broad/multicast thread. This is used only when the
 * ET system is local.
 *
 * @param etname name of ET system to test
 * @return 1 if a response was received, else 0
 */
int et_responds(const char *etname)
{
    int   port;
    char  ethost[ET_MAXHOSTNAMELEN];
    uint32_t inetaddr;
    et_openconfig  openconfig; /* opaque structure */
    et_open_config *config;    /* real structure   */
  
    /* by default we'll broadcast to uname subnet */
    et_open_config_init(&openconfig);
    config = (et_open_config *) openconfig;
  
    /* make sure we contact a LOCAL et system of this name! */
    strcpy(config->host, ET_HOST_LOCAL);
  
    /* send only 1 broadcast with a default maximum .01 sec wait */
    if (et_findserver2(etname, ethost, &port, &inetaddr, NULL, config, 1, NULL) == ET_OK) {
        /* got a response */
        return 1;
    }
  
    /* no response */
    return 0;
}


/**
 * To talk to an ET system on another computer, we need
 * to find the TCP port number the server thread of that
 * ET system is listening on. There are a number of ways to
 * do this.<p>
 *
 * The remote client (who is running this routine) either
 * says he doesn't know where the ET is (config->host = 
 * ET_HOST_REMOTE or ET_HOST_ANYWHERE), or he specifies
 * the host that the ET system is running on.<p>
 *
 * If we don't know the host name, we can broadcast and/or
 * multicast (UDP) to addresses which the ET system is
 * listening on (each address on a separate thread). In
 * this broad/multicast, we send the name of the ET system
 * that we want to find. The system which receives the
 * packet and has the same name, responds with quite a bit of information
 * including its TCP port #, uname, canonical host name, and
 * all of its IP addresses in dotted-decimal form. This
 * info is sent from each interface or broad/multicast address
 * that received the packet.<p>
 *
 * Notice that if broad/multicasting to an unknown host was successful,
 * only 1 structure in passed back in the allETinfo arg even though it
 * is a linked list.<p>
 * 
 * If we know the hostname, however, we send a udp packet
 * to ET systems listening on that host and UDP port. The
 * proper system should be the only one to respond with its
 * server TCP port.<p>
 *
 * When a server responds to a broad/multicast and we've
 * specified ET_HOST_REMOTE, this routine must see whether
 * the server is really on a remote host and not the local one.
 * To make this determination, the server has also sent
 * his host name obtained by running "uname". That is compared
 * to the result of running "uname" on this host.
 *
 * @param etname    ET system file name
 * @param ethost    returns name of ET system's host
 * @param port      returns ET system's TCP port
 * @param inetaddr  returns host address as network-byte-ordered, 32-bit unsigned int
 * @param allETinfo returns a pointer to structure (linked list) with all of the ET system's
 *                  response information;
 *                  may be null if all relevant info given in ethost, port, and inetaddr args
 * @param config    configuration passed to et_open
 * @param trys      max # of times to broadcast UDP packet
 * @param waittime  wait time for response to broad/multicast
 *
 * @return ET_OK            if successful
 * @return ET_ERROR         failure in select statement
 * @return ET_ERROR_BADARG  NULL arg for ethost, port, or inetaddr
 * @return ET_ERROR_NOMEM   cannot allocate memory
 * @return ET_ERROR_NETWORK error find IP addresses or resolving host name;
 *                          error converting dotted-decimal IP addr to binary;
 *                          error create socket; failure reading socket
 * @return ET_ERROR_SOCKET  error setting socket option
 * @return ET_ERROR_TOOMANY if multiple ET systems responded when policy is to return an error when
 *                          more than one responds.
 * @return ET_ERROR_TIMEOUT if no responses received in allotted time
 */
int et_findserver2(const char *etname, char *ethost, int *port, uint32_t *inetaddr,
                   et_response **allETinfo, et_open_config *config, int trys, struct timeval *waittime)
{
    int          i, j, k, l, m, n, err, version, addrCount, castType, gotMatch=0, subnetCount=0, ipAddrCount=0;
    int          length, len_net, lastdelay, maxtrys=6, serverport=0, debug=0, magicInts[3];
    const int    on=1, timeincr[]={0,1,2,3,4,5};
    uint32_t     addr;
    codaIpList   *baddr;
   
    /* encoding & decoding packets */
    char  *pbuf, buffer[4096]; /* should be way more than enough */
    char  outbuf[ET_FILENAME_LENGTH+1+5*sizeof(int)];
    char  localhost[ET_MAXHOSTNAMELEN];
    char  localuname[ET_MAXHOSTNAMELEN];
    char  **ipAddrs;
    
    char  specifiedhost[ET_MAXHOSTNAMELEN];
    char  unqualifiedhost[ET_MAXHOSTNAMELEN];

    int   numresponses=0, remoteresponses=0;
    et_response *answer, *answer_first=NULL, *answer_prev=NULL, *first_remote=NULL;

    /* socket & select stuff */
    struct in_addr     castaddr;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t          len;
    int                sockfd, numsockets=0, biggestsockfd=-1;
    struct timespec    delaytime;
    struct timeval     tv;
    fd_set             rset;
  
    /* structure for holding info to send out a packet on a socket */
    struct senddata {
        int sockfd;
        struct sockaddr_in servaddr;
    };
    struct senddata *send;
   
    /* check args */
    if ((ethost == NULL) || (port == NULL) || (inetaddr == NULL)) {
        fprintf(stderr, "et_findserver: invalid (null) arguments\n");
        return ET_ERROR_BADARG;
    }
  
    /* count # of subnet addrs */
    baddr = config->bcastaddrs;
    while (baddr != NULL) {
        subnetCount++;
        baddr = baddr->next;
    }
  
    /* allocate space to store all our outgoing stuff */
    send = (struct senddata *) calloc(subnetCount + config->mcastaddrs.count,
                                      sizeof(struct senddata));
    if (send == NULL) {
        fprintf(stderr, "et_findserver: cannot allocate memory\n");
        return ET_ERROR_NOMEM;
    } 
  
    /* find local uname */
    if (etNetGetUname(localuname, ET_MAXHOSTNAMELEN) != ET_OK) {
        /* nothing will match this */
        strcpy(localuname, "...");
    }
  
  
    /* If the host is local or we know its name... */
    if ((strcmp(config->host, ET_HOST_REMOTE)   != 0) &&
        (strcmp(config->host, ET_HOST_ANYWHERE) != 0))  {

        /* if it's local, find dot-decimal IP addrs */
        if ((strcmp(config->host, ET_HOST_LOCAL) == 0) ||
            (strcmp(config->host, "localhost")   == 0))  {

            if ((etNetGetIpAddrs(&ipAddrs, &ipAddrCount, NULL) != ET_OK) || ipAddrCount < 1) {
                fprintf(stderr, "et_findserver: cannot find local IP addresses\n");
                return ET_ERROR_NETWORK;
            }
        }
        /* else if we know its name, find dot-decimal IP addrs */
        else {
            if ((etNetGetIpAddrs(&ipAddrs, &ipAddrCount, config->host) != ET_OK) || ipAddrCount < 1) {
                fprintf(stderr, "et_findserver: cannot find IP addresses for %s\n", config->host);
                return ET_ERROR_NETWORK;
            }
        }
    }
    /* else if the host name is not specified, and it's either
     * remote or anywhere out there, broad/multicast to find it */
    else { }
   
    /* We need 1 socket for each subnet if broadcasting */
    if ((config->cast == ET_BROADCAST) ||
        (config->cast == ET_BROADANDMULTICAST)) {
        
        /* for each listed broadcast address ... */
        baddr = config->bcastaddrs;
        while (baddr != NULL) {
            /* put address into net-ordered binary form */
            if (inet_aton(baddr->addr, &castaddr) == INET_ATON_ERR) {
                fprintf(stderr, "et_findserver: inet_aton error net_addr = %s\n",  baddr->addr);
                for (j=0; j<numsockets; j++) {
                    close(send[j].sockfd);
                }
                free(send);
                freeIpAddrs(ipAddrs, ipAddrCount);
                return ET_ERROR_NETWORK;
            }

            if (debug) {
                printf("et_findserver: send broadcast packet to %s on port %d\n", baddr->addr, config->udpport);
            }
            
            /* server's location */
            bzero((void *)&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr   = castaddr;
            servaddr.sin_port   = htons((unsigned short)config->udpport);

            /* create socket */
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd < 0) {
                fprintf(stderr, "et_findserver: socket error\n");
                for (j=0; j<numsockets; j++) {
                    close(send[j].sockfd);
                }
                free(send);
                freeIpAddrs(ipAddrs, ipAddrCount);
                return ET_ERROR_NETWORK;
            }

            /* make this a broadcast socket */
            err = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *) &on, sizeof(on));
            if (err < 0) {
                fprintf(stderr, "et_findserver: setsockopt SO_BROADCAST error\n");
                close(sockfd);
                for (j=0; j<numsockets; j++) {
                    close(send[j].sockfd);
                }
                free(send);
                freeIpAddrs(ipAddrs, ipAddrCount);
                return ET_ERROR_SOCKET;
            }

            /* for sending packet and for select */
            send[numsockets].sockfd   = sockfd;
            send[numsockets].servaddr = servaddr;
            numsockets++;
            if (biggestsockfd < sockfd) biggestsockfd = sockfd;
            FD_SET(sockfd, &rset);

            /* go to next broadcast address */
            baddr = baddr->next;
        }
    }

    /* if also configured for multicast, send that too */
    if ((config->cast == ET_MULTICAST) ||
        (config->cast == ET_BROADANDMULTICAST)) {

        /* for each listed address ... */
        for (i=0; i < config->mcastaddrs.count; i++) {
            /* put address into net-ordered binary form */
            if (inet_aton(config->mcastaddrs.addr[i], &castaddr) == INET_ATON_ERR) {
                fprintf(stderr, "et_findserver: inet_aton error\n");
                for (j=0; j<numsockets; j++) {
                    close(send[j].sockfd);
                }
                free(send);
                freeIpAddrs(ipAddrs, ipAddrCount);
                return ET_ERROR_NETWORK;
            }

            if (debug) {
                printf("et_findserver: send multicast packet to %s on port %d\n",
                       config->mcastaddrs.addr[i], config->multiport);
            }
            
            /* server's location */
            bzero((void *)&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr   = castaddr;
            servaddr.sin_port   = htons((unsigned short)config->multiport);

            /* create socket */
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd < 0) {
                fprintf(stderr, "et_findserver: socket error\n");
                for (j=0; j<numsockets; j++) {
                    close(send[j].sockfd);
                }
                free(send);
                freeIpAddrs(ipAddrs, ipAddrCount);
                return ET_ERROR_NETWORK;
            }

            /* Set the scope of the multicast, but don't bother
             * if ttl = 1 since that's the default.
             */
            if (config->ttl != 1) {
                unsigned char ttl = config->ttl;
          
                err = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &ttl, sizeof(ttl));
                if (err < 0){
                    fprintf(stderr, "et_findserver: setsockopt IP_MULTICAST_TTL error\n");
                    close(sockfd);
                    for (j=0; j<numsockets; j++) {
                        close(send[j].sockfd);
                    }
                    free(send);
                    freeIpAddrs(ipAddrs, ipAddrCount);
                    return ET_ERROR_SOCKET;
                }
            }
        
            /* for sending packet and for select */
            send[numsockets].sockfd   = sockfd;
            send[numsockets].servaddr = servaddr;
            numsockets++;
            if (biggestsockfd < sockfd) biggestsockfd = sockfd;
            FD_SET(sockfd, &rset);
        }
    }

  
    /* time to wait for responses to broad/multicasting */
    if (waittime != NULL) {
        delaytime.tv_sec  = waittime->tv_sec;
        delaytime.tv_nsec = 1000*waittime->tv_usec;
    }
    else {
        delaytime.tv_sec  = 0;
        delaytime.tv_nsec = 0;
    }
    lastdelay = delaytime.tv_sec;
  
    /* make select return after 0.01 seconds */
    tv.tv_sec  = 0;
    tv.tv_usec = 10000; /* 0.01 sec */
  
    /* max # of broadcasts is maxtrys */
    if ((trys > maxtrys) || (trys < 1)) {
        trys = maxtrys;
    }
     
    /* Prepare output buffer that we send to servers:
     * (1) ET magic numbers (3 ints),
     * (2) ET version #,
     * (3) ET file name length (includes null)
     * (4) ET file name
     */
    pbuf = outbuf;

    /* magic numbers */
    magicInts[0] = htonl(ET_MAGIC_INT1);
    magicInts[1] = htonl(ET_MAGIC_INT2);
    magicInts[2] = htonl(ET_MAGIC_INT3);
    memcpy(pbuf, magicInts, sizeof(magicInts));
    pbuf += sizeof(magicInts);

    /* ET major version number */
    version = htonl(ET_VERSION);
    memcpy(pbuf, &version, sizeof(version));
    pbuf   += sizeof(version);
  
    /* length of ET system name string */
    length  = strlen(etname)+1;
    len_net = htonl(length);
    memcpy(pbuf, &len_net, sizeof(len_net));
    pbuf += sizeof(len_net);
  
    /* name of the ET system we want to open */
    memcpy(pbuf, etname, length);
  
    /* zero socket-set for select */
    FD_ZERO(&rset);
  
    for (i=0; i < trys; i++) {
        /* send out packets */
        for (j=0; j<numsockets; j++) {
            sendto(send[j].sockfd, (void *) outbuf, sizeof(outbuf), 0,
                   (SA *) &send[j].servaddr, sizeof(servaddr));
        }
    
        /* Increase waiting time for response each round. */
        delaytime.tv_sec = lastdelay + timeincr[i];
        lastdelay = delaytime.tv_sec;
    
        /* reset "rset" value each time select is called */
        for (j=0; j<numsockets; j++) {
            FD_SET(send[j].sockfd, &rset);
        }
    
        /* Linux reportedly changes tv in "select" so reset it each round */
        tv.tv_sec  = 0;
        tv.tv_usec = 10000; /* 0.01 sec */
        
        /* wait for responses from ET systems */
#ifdef VXWORKS
        taskDelay(lastdelay*60);
#else
        nanosleep(&delaytime, NULL);
#endif

        /* select */
        err = select(biggestsockfd + 1, &rset, NULL, NULL, &tv);
        
        /* if select error ... */
        if (err == -1) {
            fprintf(stderr, "et_findserver: select error\n");
            for (j=0; j<numsockets; j++) {
                close(send[j].sockfd);
            }
            free(send);
            freeIpAddrs(ipAddrs, ipAddrCount);
            return ET_ERROR;
        }
        /* else if select times out, rebroadcast with longer waiting time */
        else if (err == 0) {
            continue;
        }
    
        /* else if we have some response(s) ... */
        else {
            for(j=0; j < numsockets; j++) {
                int bytes;
          
                if (FD_ISSET(send[j].sockfd, &rset) == 0) {
                    /* this socket is not ready for reading */
                    continue;
                }
          
anotherpacket:

                /* get back a packet from a server */
                len = sizeof(cliaddr);
                n = recvfrom(send[j].sockfd, (void *) buffer, sizeof(buffer), 0, (SA *) &cliaddr, &len);

                /* if error ... */
                if (n < 0) {
                    fprintf(stderr, "et_findserver: recvfrom error, %s\n", strerror(errno));
                    for (k=0; k<numsockets; k++) {
                        close(send[k].sockfd);
                    }
                    free(send);
                    freeIpAddrs(ipAddrs, ipAddrCount);
                    /* free up all answers */
                    et_freeAnswers(answer_first);
                    return ET_ERROR_NETWORK;
                }
          
                /* allocate space for single response */
                answer = (et_response *) calloc(1, sizeof(et_response));
                if (answer == NULL) {
                    fprintf(stderr, "et_findserver: out of memory\n");
                    for (k=0; k<numsockets; k++) {
                        close(send[k].sockfd);
                    }
                    free(send);
                    freeIpAddrs(ipAddrs, ipAddrCount);
                    et_freeAnswers(answer_first);
                    return ET_ERROR_NOMEM;
                }
          
                /* string answer structs in linked list */
                if (answer_prev != NULL) {
                    answer_prev->next = answer;
                }
                else {
                    answer_first = answer;
                }

                /* see if there's another packet to be read on this socket */
                bytes = 0;
#ifdef VXWORKS
                ioctl(send[j].sockfd, FIONREAD, (int) &bytes);
#else
                ioctl(send[j].sockfd, FIONREAD, (void *) &bytes);
#endif
          
                /* decode packet from ET system:
                 *
                 * (0)  ET magic numbers (3 ints)
                 * (1)  ET version #
                 * (2)  port of tcp server thread (not udp config->port)
                 * (3)  ET_BROADCAST or ET_MULTICAST (int)
                 * (4)  length of next string
                 * (5)    broadcast address (dotted-dec) if broadcast received or
                 *        multicast address (dotted-dec) if multicast received
                 *        (see int #3)
                 * (6)  length of next string
                 * (7)    hostname given by "uname" (used as a general
                 *        identifier of this host no matter which interface is used)
                 * (8)  length of next string
                 * (9)    canonical name of host
                 * (10) number of IP addresses
                 * (11)   32bit, net-byte ordered IPv4 address assoc with following address
                 * (12)   length of next string
                 * (13)       first dotted-decimal IPv4 address
                 * (14)   32bit, net-byte ordered IPv4 address assoc with following address
                 * (15)   length of next string
                 * (16)       second dotted-decimal IPv4 address ...
                 *
                 * All known IP addresses are sent here both in numerical & dotted-decimal forms.
                 */
           
                pbuf = buffer;
          
                do {
                    /* get ET magic #s */
                    memcpy(magicInts, pbuf, sizeof(magicInts));
                    pbuf += sizeof(magicInts);
              
                    /* if the wrong magic numbers, reject it */
                    if (ntohl(magicInts[0]) != ET_MAGIC_INT1 ||
                        ntohl(magicInts[1]) != ET_MAGIC_INT2 ||
                        ntohl(magicInts[2]) != ET_MAGIC_INT3)  {
                        free(answer);
                        break;
                    }
         
                    /* get ET system's major version # */
                    memcpy(&version, pbuf, sizeof(version));
                    version = ntohl(version);
                    pbuf += sizeof(version);

                    /* if the wrong version ET system is responding, reject it */
                    if (version != ET_VERSION) {
                        free(answer);
                        break;
                    }

                    /* get ET system's TCP port */
                    memcpy(&serverport, pbuf, sizeof(serverport));
                    serverport = ntohl(serverport);
                    if ((serverport < 1) || (serverport > 65536)) {
                        free(answer);
                        break;
                    }
                    answer->port = serverport;
                    pbuf += sizeof(serverport);

                    /* broad or multi cast? might be both for java. */
                    memcpy(&castType, pbuf, sizeof(castType));
                    castType = ntohl(castType);
                    if ((castType != ET_BROADCAST) &&
                        (castType != ET_MULTICAST) &&
                        (castType != ET_BROADANDMULTICAST)) {
                        free(answer);
                        break;
                    }
                    answer->castType = castType;
                    pbuf += sizeof(castType);

                    if (debug) {
                        if (castType == ET_BROADCAST) {
                            printf("et_findserver: reply to broadcast\n");
                        }
                        else if (castType != ET_MULTICAST) {
                            printf("et_findserver: reply to multicast\n");
                        }
                        else if (castType != ET_BROADANDMULTICAST) {
                            printf("et_findserver: reply to broad & multicast\n");
                        }
                        else {
                            printf("et_findserver: reply -> don't know if broad or multi casting\n");
                        }
                    }

                    /* get broad/multicast IP original packet sent to */
                    memcpy(&length, pbuf, sizeof(length));
                    length = ntohl(length);
                    if ((length < 1) || (length > ET_IPADDRSTRLEN)) {
                        free(answer);
                        break;
                    }
                    pbuf += sizeof(length);
                    memcpy(answer->castIP, pbuf, length);
                    pbuf += length;

                    /* get ET system's uname */
                    memcpy(&length, pbuf, sizeof(length));
                    length = ntohl(length);
                    if ((length < 1) || (length > ET_MAXHOSTNAMELEN)) {
                        free(answer);
                        break;
                    }
                    pbuf += sizeof(length);
                    memcpy(answer->uname, pbuf, length);
                    pbuf += length;

                    /* get ET system's canonical name */
                    memcpy(&length, pbuf, sizeof(length));
                    length = ntohl(length);
                    if (length > ET_MAXHOSTNAMELEN) {
                        free(answer);
                        break;
                    }
                    pbuf += sizeof(length);
                    if (length > 0) {
                        memcpy(answer->canon, pbuf, length);
                        pbuf += length;
                    }

                    /* get number of IP addrs */
                    memcpy(&addrCount, pbuf, sizeof(addrCount));
                    addrCount = ntohl(addrCount);
                    if ((addrCount < 0) || (addrCount > 20)) {
                        free(answer);
                        break;
                    }
                    answer->addrCount = addrCount;
                    pbuf += sizeof(addrCount);

                    /* allocate mem - arrays of ints & char *'s */
                    answer->addrs = (uint32_t *)calloc(addrCount, sizeof(uint32_t));
                    answer->ipaddrs =  (char **)calloc(addrCount, sizeof(char *));
                    if (answer->addrs == NULL || answer->ipaddrs == NULL) {
                        for (k=0; k<numsockets; k++) {
                            close(send[k].sockfd);
                        }
                        free(send);
                        freeIpAddrs(ipAddrs, ipAddrCount);
                        free(answer);
                        et_freeAnswers(answer_first);
                        return ET_ERROR_NOMEM;
                    }

                    /* read in all the addrs */
                    for (k=0; k < addrCount; k++) {
                        memcpy(&addr, pbuf, sizeof(addr));
                        /* do not swap the addr as it must be network byte ordered */
                        answer->addrs[k] = addr;
                        pbuf += sizeof(addr);

                        memcpy(&length, pbuf, sizeof(length));
                        length = ntohl(length);
                        if ((length < 1) || (length > ET_MAXHOSTNAMELEN)) {
                            for (l=0; l < k; l++) {
                                free(answer->ipaddrs[l]);
                            }
                            free(answer->ipaddrs);
                            free(answer->addrs);
                            free(answer);
                            break;
                        }
                        pbuf += sizeof(length);

                        answer->ipaddrs[k] = strdup(pbuf); /* ending NULL is sent so we're OK */
                        if (answer->ipaddrs[k] == NULL) {
                            for (l=0; l<numsockets; l++) {
                                close(send[l].sockfd);
                            }
                            free(send);
                            freeIpAddrs(ipAddrs, ipAddrCount);
                            for (l=0; l < k; l++) {
                                free(answer->ipaddrs[l]);
                            }
                            free(answer->ipaddrs);
                            free(answer->addrs);
                            free(answer);
                            et_freeAnswers(answer_first);
                            return ET_ERROR_NOMEM;
                        }
                        answer->ipaddrs[k][length-1] = '\0';
                        pbuf += length;
                    }

                    if (debug) {
                        printf("et_findserver: RESPONSE from %s at %d with cast addr = %s and uname = %s\n",
                               answer->ipaddrs[0], answer->port, answer->castIP, answer->uname);
                    }
                    numresponses++;
            
                } while(0);
          
                /* Now that we have a real answer, make that a part of the list */
                answer_prev = answer;
          
                /* see if there's another packet to be read on this socket */
                if (bytes > 0) {
                    goto anotherpacket;
                }
          
            } /* for each socket (j) */
        
        
            /* If host is local or we know its name. There may be many responses.
             * Look only at the response that matches the host's address.
             */
            if ((strcmp(config->host, ET_HOST_REMOTE) != 0) &&
                (strcmp(config->host, ET_HOST_ANYWHERE) != 0)) {
                /* analyze the answers/responses */
                answer = answer_first;
                while (answer != NULL) {
                    /* The problem here is another ET system of the
                     * same name may respond, but we're interested
                     * only in the one on the specified host.
                     */
                    for (n=0; n < answer->addrCount; n++) {
                        for (m=0; m<ipAddrCount; m++) {
if (debug) printf("et_findserver: comparing %s to incoming %s\n", ipAddrs[m], answer->ipaddrs[n]);
                            if (strcmp(ipAddrs[m], answer->ipaddrs[n]) == 0)  {
                                gotMatch = 1;
                                break;
                            }
                        }
                        if (gotMatch) break;
                    }
            
                    if (gotMatch) {
if (debug) printf("et_findserver: got a match to local or specific: %s\n", answer->ipaddrs[n]);
                        for (k=0; k<numsockets; k++) {
                            close(send[k].sockfd);
                        }
                        free(send);
                        freeIpAddrs(ipAddrs, ipAddrCount);
                        strcpy(ethost, answer->ipaddrs[n]);
                        *port = answer->port;
                        *inetaddr = answer->addrs[n];
                        
                        et_freeAnswers(answer_first);
                        return ET_OK;
                    }
            
                    answer = answer->next;
                }
            }

            /* If the host may be anywhere (local or remote) ...  */
            else if (strcmp(config->host, ET_HOST_ANYWHERE) == 0) {
                /* if our policy is to return an error for more than 1 response ... */
                if ((config->policy == ET_POLICY_ERROR) && (numresponses > 1)) {
                    /* print error message and return */
if (debug) printf("et_findserver: %d responses to broad/multicast -\n", numresponses);
                    j = 1;
                    answer = answer_first;
                    while (answer != NULL) {
                        fprintf(stderr, "  RESPONSE %d from %s at %d\n",
                                j++, answer->ipaddrs[0], answer->port);
                        answer = answer->next;
                    }
                    for (k=0; k<numsockets; k++) {
                        close(send[k].sockfd);
                    }
                    free(send);
                    freeIpAddrs(ipAddrs, ipAddrCount);
                    et_freeAnswers(answer_first);
                    return ET_ERROR_TOOMANY;
                }

                /* analyze the answers/responses */
                j = 0;
                answer = answer_first;
                while (answer != NULL) {
                    /* If our policy is to take the first response do so. If our
                     * policy is ET_POLICY_ERROR, we can also return the first as
                     * we can only be here if there's been just 1 response.
                     */
                    if ((config->policy == ET_POLICY_FIRST) ||
                        (config->policy == ET_POLICY_ERROR))  {
if (debug) printf("et_findserver: got a match to .anywhere (%s), first or error policy\n", answer->ipaddrs[0]);
                        for (k=0; k<numsockets; k++) {
                            close(send[k].sockfd);
                        }
                        free(send);
                        freeIpAddrs(ipAddrs, ipAddrCount);
                        strcpy(ethost, answer->ipaddrs[0]);
                        *port = answer->port;
                        *inetaddr = answer->addrs[0];
                        if (allETinfo != NULL) {
                            /* remove answer from linked list & return the first of new list */
                            answer_first = removeResponseFromList(answer_first, answer);
                            *allETinfo = answer;
                        }
                        /* free the rest of the linked list */
                        et_freeAnswers(answer_first);
                        return ET_OK;
                    }
                    /* else if our policy is to take the first local response ... */
                    else if (config->policy == ET_POLICY_LOCAL) {
                        if (strcmp(localuname, answer->uname) == 0) {
if (debug) printf("et_findserver: got a uname match to .anywhere, local policy\n");
                            for (k=0; k<numsockets; k++) {
                                close(send[k].sockfd);
                            }
                            free(send);
                            freeIpAddrs(ipAddrs, ipAddrCount);
                            strcpy(ethost, answer->ipaddrs[0]);
                            *port = answer->port;
                            *inetaddr = answer->addrs[0];
                            if (allETinfo != NULL) {
                                answer_first = removeResponseFromList(answer_first, answer);
                                *allETinfo = answer;
                            }
                            et_freeAnswers(answer_first);
                            return ET_OK;
                        }
              
                        /* If we went through all responses without finding
                         * a local one, pick the first one we received.
                         */
                        if (++j == numresponses-1) {
if (debug) printf("et_findserver: got a match to .anywhere, nothing local available\n");
                            answer = answer_first;
                            for (k=0; k<numsockets; k++) {
                                close(send[k].sockfd);
                            }
                            free(send);
                            freeIpAddrs(ipAddrs, ipAddrCount);
                            strcpy(ethost, answer->ipaddrs[0]);
                            *port = answer->port;
                            *inetaddr = answer->addrs[0];
                            if (allETinfo != NULL) {
                                answer_first = removeResponseFromList(answer_first, answer);
                                *allETinfo = answer;
                            }
                            et_freeAnswers(answer_first);
                            return ET_OK;
                        }
                    }
                    answer = answer->next;
                }
            }

            /* if user did not specify host name, and it must be remote ... */
            else if (strcmp(config->host, ET_HOST_REMOTE) == 0) {
                /* analyze the answers/responses */
                answer = answer_first;
                while (answer != NULL) {
                    /* The problem here is if we are broadcasting, a local ET
                     * system of the same name may respond, but we're interested
                     * only in the remote systems. Weed out the local system.
                     */
                    if (strcmp(localuname, answer->uname) == 0) {
                        answer = answer->next;
                        continue;
                    }
                    remoteresponses++;
                    if (first_remote == NULL) {
                        first_remote = answer;
                    }
                    /* The ET_POLICY_LOCAL doesn't make any sense here so
                     * default to ET_POLICY_FIRST
                     */
                    if ((config->policy == ET_POLICY_FIRST) ||
                        (config->policy == ET_POLICY_LOCAL))  {
if (debug) printf("et_findserver: got a match to .remote, first or local policy\n");
                        for (k=0; k<numsockets; k++) {
                            close(send[k].sockfd);
                        }
                        free(send);
                        freeIpAddrs(ipAddrs, ipAddrCount);
                        strcpy(ethost, answer->ipaddrs[0]);
                        *port = answer->port;
                        *inetaddr = answer->addrs[0];
                        if (allETinfo != NULL) {
                            answer_first = removeResponseFromList(answer_first, answer);
                            *allETinfo = answer;
                        }
                        et_freeAnswers(answer_first);
                        return ET_OK;
                    }
                    answer = answer->next;
                }
          
                /* If we're here, we do NOT have a first/local policy with at least
                 * 1 remote response.
                 */
          
                if (config->policy == ET_POLICY_ERROR) {
                    if (remoteresponses == 1) {
if (debug) printf("et_findserver: got a match to .remote, error policy\n");
                        for (k=0; k<numsockets; k++) {
                            close(send[k].sockfd);
                        }
                        free(send);
                        freeIpAddrs(ipAddrs, ipAddrCount);
                        strcpy(ethost, first_remote->ipaddrs[0]);
                        *port = first_remote->port;
                        *inetaddr = first_remote->addrs[0];
                        if (allETinfo != NULL) {
                            answer_first = removeResponseFromList(answer_first, first_remote);
                            *allETinfo = first_remote;
                        }
                        et_freeAnswers(answer_first);
                        return ET_OK;
                    }
                    else if (remoteresponses > 1) {
                        /* too many proper responses, return error */
                        fprintf(stderr, "et_findserver: %d responses to broad/multicast -\n",
                                numresponses);
                        j = 0;
                        answer = answer_first;
                        while (answer != NULL) {
                            if (strcmp(localuname, answer->uname) == 0) {
                                answer = answer->next;
                                continue;
                            }
                            fprintf(stderr, "  RESPONSE %d from %s at %d\n",
                                    j++, answer->ipaddrs[0], answer->port);
                            answer = answer->next;
                        }
                        for (k=0; k<numsockets; k++) {
                            close(send[k].sockfd);
                        }
                        free(send);
                        freeIpAddrs(ipAddrs, ipAddrCount);
                        et_freeAnswers(answer_first);
                        return ET_ERROR_TOOMANY;
                    }
                }
            } /* else if host is remote */
        } /* else if we have some response(s) ... */
    } /* try another broadcast */
  
    /* if we're here, we got no response from any matching ET system */
if (debug) printf("et_findserver: no valid response received\n");
    for (k=0; k<numsockets; k++) {
        close(send[k].sockfd);
    }
    free(send);
    freeIpAddrs(ipAddrs, ipAddrCount);
    et_freeAnswers(answer_first);
  
    return ET_ERROR_TIMEOUT;
}


#define DT_BANK    0x10
#define DT_LONG    0x01
#define DT_SHORT   0x05
#define DT_BYTE    0x07
#define DT_CHAR    0x03

static int dtswap[] = {0,2,2,0,1,1,0,0, 3,2,3,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};


/******************************************************
 * This is another version of CODAswap and allows
 * specifying separate source and destination buffers
 * as well as a flag telling whether the data was
 * written from a same or different endian machine
 * (i.e. do we swap bank info or not). In the first
 * case, set same_endian = 1, else 0.
 *
 * This function is deprecated. Use the swapping
 * function contained in the evio library.
 ******************************************************/
int et_CODAswap(int *src, int *dest, int nints, int same_endian)
{
    int   i, j, blen, dtype;
    int   *lp, *lpd;
    short *sp, *spd;

    /* DEPRECATED: This function does NOT do a complete job anymore. */
    return ET_ERROR;

    /* int pointers */
    if (dest == NULL) {
        dest = src;
    }

    i = 0;

    while (i < nints) {
        lp  = &src[i];
        lpd = &dest[i];

        if (same_endian) {
            blen  = src[i] - 1;
            dtype = ((src[i+1])&0xff00)>>8;
        }
        else {
            blen  = ET_SWAP32(src[i]) - 1;
            dtype = ((ET_SWAP32(src[i+1]))&0xff00)>>8;
        }

        *lpd++ = ET_SWAP32(*lp);  /* Swap length */
         lp++;
        *lpd   = ET_SWAP32(*lp);  /* Swap bank header */

        i += 2;

        if(dtype != DT_BANK) {
            switch(dtswap[dtype]) {
                case 0:
                    /* No swap */
                    i += blen;
                break;
        
                case 1:
                    /* short swap */
                    sp  = (short *)&src[i];
                    spd = (short *)&dest[i];
                    for(j=0; j<(blen<<1); j++, sp++) {
                        *spd++ = ET_SWAP16(*sp);
                    }
                    i += blen;
                break;
        
                case 2:
                    /* int swap */
                    lp  = &src[i];
                    lpd = &dest[i];
                    for(j=0; j<blen; j++, lp++) {
                        *lpd++ = ET_SWAP32(*lp);
                    }
                    i += blen;
                break;
        
                case 3:
                    /* double swap */
                    lp  = &src[i];
                    lpd = &dest[i];
                    for(j=0; j<blen; j++, lp++) {
                        *lpd++ = ET_SWAP32(*lp);
                    }
                    i += blen;
                break;
        
                default:
                    /* No swap */
                    i += blen;
            }
        }
    }
    
    return ET_OK;
}
