/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      Routines for TCP and UDP "servers". Threads that listen on UDP sockets
 *        for users trying to find ET systems. Threads for establishing TCP
 *        communications with remote users.
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "et_private.h"
#include "et_network.h"
#include "et_data.h"

/* This struct contains info passed to each thread serving a network ET client connection. */
typedef struct et_threadinfo_t {
    int    connfd;        /* socket connection's fd */
    int    endian;        /* endian of server */
    int    endian_client; /* endian of client */
    int    iov_max;       /* max iov structs for writev */
    int    bit64;         /* is client 64 bit? (1-y, 0-n) */
    et_id  *id;           /* for passing info on system running server */
} et_threadinfo;

/* prototypes */
static void *et_client_thread(void *arg);
static void  et_command_loop(et_threadinfo *info);


/**
 * This routine implements a thread which is listening on all addresses for a UDP
 * packet looking to find this ET system.
 *
 * @param arg pointer to et_netthread structure for passing in info.
 */
void *et_cast_thread(void *arg) {

    et_netthread       *threadarg = (et_netthread *) arg;
    et_sys_config      *config = threadarg->config;
    et_id              *etid   = threadarg->id;
    pthread_attr_t     attr;
    char               uname[ET_MAXHOSTNAMELEN];
    codaIpInfo         *pinfo = config->netinfo.ipinfo;
    int                cast = ET_MULTICAST; /* not used anymore */
    int                ipAddrCount = config->netinfo.count;
    int                i=0, version, sockfd, nbytes, length, err;
    int                magicInts[3], debug=0;
    uint32_t           k, len, netint;
    size_t             bufsize;
    char               *outbuf, *pbuf, inbuf[ET_FILENAME_LENGTH+1+5*sizeof(int)];
    char               filename[ET_FILENAME_LENGTH];
    socklen_t          slen;
    struct sockaddr_in cliaddr;

    etid->debug = ET_DEBUG_INFO;

    /* get thread attribute ready */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* get uname of this host */
    memset(uname, 0, ET_MAXHOSTNAMELEN);
    if (etNetGetUname(uname, ET_MAXHOSTNAMELEN) != ET_OK) {
        strcpy(uname, "..."); /* nothing will match this */
    }

    /* send signal to main thread that this thread has started */
    etid->race = -1;

    /* Setup socket for receiving udp packets on any local address
     * and on the given multicast addresses. */
    err = etNetUdpReceiveAll((unsigned short)config->port, config->mcastaddrs.addr,
                             config->mcastaddrs.count, &sockfd);
    if (err != ET_OK || sockfd < 0) {
        if (etid->debug >= ET_DEBUG_SEVERE) {
            et_logmsg("SEVERE", "et_cast_thread: problem opening socket\n");
        }
        exit(1);
    }
    /* Store socket fd so it can be closed elsewhere. */
    etid->sys->udpFd = sockfd;

printf("Listening on port %d\n", config->port);

    /* Prepare output buffer we send in answer to inquiries:
     *
     *  (0)  ET magic numbers (3 ints)
     *  (1)  ET version #
     *  (2)  port of tcp server thread (not udp config->port)
     *  (3)  ET_BROADCAST or ET_MULTICAST (int) (not used now)
     *  (4)  length of next string
     *  (5)    broadcast address (dotted-dec) if broadcast received or
     *         multicast address (dotted-dec) if multicast received
     *         (see int #3) (not used now)
     *  (6)  length of next string
     *  (7)    hostname given by "uname" (used as a general
     *         identifier of this host no matter which interface is used)
     *  (8)  length of next string
     *  (9)    canonical name of host
     *  (10) number of IP addresses
     *  Loop over # of addresses
     *   |    (11)   32bit, net-byte ordered IPv4 address assoc with following address
     *   |    (12)   length of next string (dot-decimal addr)
     *   V    (13)   dot-decimal IPv4 address
     *
     *  The following was added to allow easy matching of subnets.
     *  Although item (14) it is the same as item (10), it's repeated
     *  here as a precaution. If, for a client, 14 does not have the same
     *  value as 10, then the ET system must be of an older variety without
     *  the following data. Thus, things are backwards compatible.
     *
     *  (14) number of broadcast addresses
     *  Loop over # of addresses
     *   |    (15)   length of next string (broadcast addr)
     *   V    (16)   dotted-decimal broadcast address
     *               corresponding to same order in previous loop
     *
     *  All known IP addresses & their corresponding broadcast addresses
     *  are sent here both in numerical & dotted-decimal forms.
     *
     */

    /* find length of necessary buffer */
    bufsize = sizeof(magicInts) + 8*sizeof(int) + strlen(uname) + strlen(pinfo[0].canon) + 3 /* 3 NULLs */;

    /* look through the list of all IP addresses (and related data) */
    for (i=0; i < ipAddrCount; i++) {
        bufsize += 3*sizeof(int) + strlen(pinfo[i].addr) + 1 + strlen(pinfo[i].broadcast) + 1;
    }

    /* allocate packet's buffer */
    if (debug) printf("et_listen_thread: malloc bufsize = %d\n", (int)bufsize);
    if ( (pbuf = outbuf = (char *) malloc(bufsize)) == NULL ) {
        if (etid->debug >= ET_DEBUG_SEVERE) {
            et_logmsg("SEVERE", "et_listen_thread: cannot allocate memory\n");
        }
        exit(1);
    }

    /* ******************** */
    /* put data into buffer */
    /* ******************** */

    /* 0) magic numbers */
    magicInts[0] = htonl(ET_MAGIC_INT1);
    magicInts[1] = htonl(ET_MAGIC_INT2);
    magicInts[2] = htonl(ET_MAGIC_INT3);
    memcpy(pbuf, magicInts, sizeof(magicInts));
    pbuf += sizeof(magicInts);

    /* 1) ET version */
    k = htonl((uint32_t)etid->version);
    memcpy(pbuf, &k, sizeof(k));
    pbuf += sizeof(k);

    /* 2) TCP server port */
    k = htonl((uint32_t)etid->sys->port);
    memcpy(pbuf, &k, sizeof(k));
    pbuf += sizeof(k);

    /* 3) received broadcast or multicast packet? (not used at the receiving end) */
    k = htonl((uint32_t)cast);
    memcpy(pbuf, &k, sizeof(k));
    pbuf += sizeof(k);

    /* 4 & 5) broadcast or multicast dot-decimal address (not used at the receiving end) */
    /* Just send len = 0 and no IP address */
    k = 0;
    memcpy(pbuf, &k, sizeof(k));
    pbuf += sizeof(k);

    /*
    memcpy(pbuf, listenaddr, len);
    pbuf += len;
     */

    /* 6 & 7) uname */
    len = (uint32_t) strlen(uname)+1;
    k = htonl(len);
    memcpy(pbuf, &k, sizeof(k));
    pbuf += sizeof(k);

    memcpy(pbuf, uname, len);
    pbuf += len;

    /* 8 & 9) canonical name if there is one */
    len = (uint32_t) strlen(pinfo[0].canon)+1;
    if (len > 1) {
        k = htonl(len);
        memcpy(pbuf, &k, sizeof(k));
        pbuf += sizeof(k);

        memcpy(pbuf, pinfo[0].canon, len);
        pbuf += len;
    }
    else {
        k = 0;
        memcpy(pbuf, &k, sizeof(k));
        pbuf += sizeof(k);
    }

    /* 10) number of addresses to follow */
    k = htonl((uint32_t) ipAddrCount);
    memcpy(pbuf, &k, sizeof(k));
    pbuf += sizeof(k);

    /* look through the list of all IP addresses (and related data) */
    for (i=0; i < ipAddrCount; i++) {
        /* 11) 32 bit IP address (already network byte ordered) */
        netint = (uint32_t) pinfo[i].saddr.sin_addr.s_addr;
        memcpy(pbuf, &netint, sizeof(netint));
        pbuf += sizeof(netint);

        /* 12 & 13) IP address (dotted-decimal), length first */
        len = (uint32_t) strlen(pinfo[i].addr)+1;
        k = htonl(len);
        memcpy(pbuf, &k, sizeof(k));
        pbuf += sizeof(k);
        if (debug)
            printf("et_listen_thread: will send to cli, addr = %u, len = %d, addr = %s\n", netint, len, pinfo[i].addr);

        memcpy(pbuf, pinfo[i].addr, len);
        pbuf += len;
    }

    /* 14) number of addresses to follow (allows other end to be backwards compatible with this addition) */
    k = htonl((uint32_t) ipAddrCount);
    memcpy(pbuf, &k, sizeof(k));
    pbuf += sizeof(k);

    /* look through the list of all IP addresses (and related data) */
    for (i=0; i < ipAddrCount; i++) {
        /* 15 & 16) IP address (dotted-decimal), length first */
        len = (uint32_t) strlen(pinfo[i].broadcast)+1;
        k = htonl(len);
        memcpy(pbuf, &k, sizeof(k));
        pbuf += sizeof(k);
        if (debug)
            printf("et_listen_thread: will send to cli, len = %d, broad addr = %s\n", len, pinfo[i].broadcast);

        memcpy(pbuf, pinfo[i].broadcast, len);
        pbuf += len;
    }

    /* release memory allocated in et_cast_thread */
    /* free(arg); */

    while (1) {
        slen = sizeof(cliaddr);

        /* read incoming data */
        nbytes = recvfrom(sockfd, (void *) inbuf, ET_FILENAME_LENGTH+4,
                          0, (SA *) &cliaddr, &slen);
        if (nbytes < 0) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_listen_thread: error in recvfrom\n");
            }
            /* try listening for another packet */
            continue;
        }

        /* decode the data:
         * (1) ET magic numbers (3 ints),
         * (2) ET version #,
         * (3) length of string,
         * (4) ET file name
         */
        pbuf = inbuf;

        /* read & check magic numbers */
        memcpy(magicInts, pbuf, sizeof(magicInts));
        pbuf += sizeof(magicInts);
        if (ntohl(magicInts[0]) != ET_MAGIC_INT1 ||
            ntohl(magicInts[1]) != ET_MAGIC_INT2 ||
            ntohl(magicInts[2]) != ET_MAGIC_INT3)  {
            /* wrong magic numbers, listen for next packet */
            continue;
        }
/*printf("et_listen_thread: passed magic number test\n");*/

        /* read & check version number */
        memcpy(&version, pbuf, sizeof(version));
        version = ntohl(version);
        pbuf += sizeof(version);
        if (version !=  etid->version) {
            /* wrong version, listen for next packet */
            continue;
        }

        /* read & check to see if we have a reasonable ET name length. */
        memcpy(&length, pbuf, sizeof(length));
        length = ntohl(length);
        pbuf += sizeof(length);
        if ((length < 1) || (length > ET_FILENAME_LENGTH)) {
            /* not proper format, listen for next packet */
            continue;
        }

        /* read ET name */
        memcpy(filename, pbuf, length);

        if (debug)
            printf("et_listen_thread: received packet on %s for %s\n", uname, filename);

        /* check if the ET system the client wants is ours */
        if (strcmp(filename, etid->sys->config.filename) == 0) {
            /* if we are the one the client is looking for, send a reply */
            sendto(sockfd, (void *) outbuf, bufsize, 0, (SA *) &cliaddr, slen);
        }
    }
}


/************************************************************
 * et_netserver is a server for remote clients and for local
 * clients that cannot access the mutexes from shared memory
 * because the operating system does not support such sharing.
 **************************************************************/
void *et_netserver(void *arg)
{
    et_netthread    *threadarg = (et_netthread *) arg;
    et_sys_config   *config = threadarg->config;
    et_id           *etid   = threadarg->id;
    int             listenfd=0, endian, iov_max, debug=0;
    int             err, flags=0, magicInts[3], port=0;
    struct sockaddr_in cliaddr;
    socklen_t       addrlen, len;
    pthread_t       tid;
    pthread_attr_t  attr;
    /* pointer to information to be passed to threads */
    et_threadinfo   *pinfo, *last=NULL;

    /* find servers's endian value */
    if (etNetLocalByteOrder(&endian) != ET_OK) {
        if (etid->debug >= ET_DEBUG_SEVERE) {
            et_logmsg("SEVERE", "et_netserver: strange byteorder\n");
        }
        exit(1);
    }

    /* find servers's iov_max value */
#ifndef __APPLE__
    if ( (iov_max = (int)sysconf(_SC_IOV_MAX)) < 0) {
        /* set it to POSIX minimum by default (it always bombs on Linux) */
        iov_max = ET_IOV_MAX;
    }
#else
        iov_max = ET_IOV_MAX;
#endif
    /* get thread attribute ready */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* open a listening socket */

    /* if a server port was explicitly specified, use it otherwise use default */
    if (config->serverport > 0) {
        port = config->serverport;
    }
    else {
        port = ET_SERVER_PORT;
    }

    err = etNetTcpListen(0, (unsigned short)port, config->tcpSendBufSize,
                         config->tcpRecvBufSize, config->tcpNoDelay, &listenfd);
    if (err != ET_OK) {
        if (etid->debug >= ET_DEBUG_SEVERE) {
            et_logmsg("SEVERE", "et_netserver: specified port is busy, cannot start server thread\n");
        }
        exit(1);
    }
    /* Store fd so socket can be closed elsewhere. */
    etid->sys->tcpFd = listenfd;

    if (debug)
        printf("TCP server listening on port %d\n", port);

    if (etid->debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "et_netserver: am listening on TCP port %d\n", port);
    }

    if (listenfd < 0) {
        if (etid->debug >= ET_DEBUG_SEVERE) {
            et_logmsg("SEVERE", "et_netserver: all ports busy, cannot start server thread\n");
        }
        exit(1);
    }

    /* save this port # in the shared memory so clients can get to it */
    etid->sys->port = port;

    /* send signal that thread started */
    etid->race = -1;

    /* spawn threads to deal with each client */
    addrlen = sizeof(cliaddr);
    for ( ; ; ) {
        len = addrlen;

        /* allocate argument to pass to thread */
        pinfo = (et_threadinfo *) calloc(1, sizeof(et_threadinfo));
        if (pinfo == NULL) {
            if (etid->debug >= ET_DEBUG_SEVERE) {
                et_logmsg("SEVERE", "et_netserver: cannot allocate memory\n");
            }
            exit(1);
        }
        /* set values to pass on to thread */
        pinfo->endian = endian;
        pinfo->iov_max = iov_max;
        pinfo->id = etid;

        /* wait for connection to client */
        pinfo->connfd = etNetAccept(listenfd, (SA *) &cliaddr, &len);
        if (pinfo->connfd < 0) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_netserver: error accepting client connection\n");
            }
            free(pinfo);
            continue;
        }

        /* Read data from client. Set socket to nonblocking so someone probing
         * it and not writing at least 3 ints worth of data will return an error
         * and we can ignore that "client".
         */
        flags = fcntl(pinfo->connfd, F_GETFL, 0);
        if (flags == -1) flags = 0;

        if ((fcntl(pinfo->connfd, F_SETFL, flags | O_NONBLOCK)) < 0) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_netserver: error in fcntl 1\n");
            }
            close(pinfo->connfd);
            free(pinfo);
            continue;
        }

        /*printf("et_netserver: try to read magic numbers\n");*/
        err = etNetTcpRead3iNB(pinfo->connfd, &magicInts[0], &magicInts[1], &magicInts[2]);
        if (err != 0) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_netserver: ET server being probed by non-ET client or read failure\n");
            }
            close(pinfo->connfd);
            free(pinfo);
            continue;
        }

        /*printf("et_netserver: read magic numbers: 1 -> %0x, 2 -> %0x, 3 -> %0x\n",
          magicInts[0],magicInts[1],magicInts[2]); */

        /* check magic numbers received */
        if (magicInts[0] != ET_MAGIC_INT1 ||
            magicInts[1] != ET_MAGIC_INT2 ||
            magicInts[2] != ET_MAGIC_INT3) {

            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_netserver: magic numbers do NOT match, close client\n");
            }
            close(pinfo->connfd);
            free(pinfo);
            continue;
        }
        /*printf("et_netserver: passed magic number test\n");*/

        /* make socket blocking again */
        flags &= ~O_NONBLOCK;
        fcntl(pinfo->connfd, F_SETFL, flags);
        if (err == -1) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "et_netserver: error in fcntl 2\n");
            }
            close(pinfo->connfd);
            free(pinfo);
            continue;
        }

        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("INFO", "et_netserver: magic numbers do match, accept ET client\n");
        }

        if (debug)
            printf("TCP server got a connection so spawn thread\n");

        /* create thread to deal with client */
        pthread_create(&tid, &attr, et_client_thread, (void *) pinfo);
    }

    pthread_exit(NULL);

}


/************************************************************/
static void *et_client_thread(void *arg)
{
  int  connfd, endian, err, length, bit64;
  uint32_t  outgoing[10], incoming[5];
  char et_name[ET_FILENAME_LENGTH];
  et_threadinfo    info;
  et_id            *etid;

  info   = *((et_threadinfo *) arg);
  connfd = info.connfd;
  etid   = info.id;
  free(arg);

  /* read data from client, extra room for future if necessary */
  if (etNetTcpRead(connfd, incoming, sizeof(incoming)) != sizeof(incoming)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_client_thread: read failure\n");
    }
    err = ET_ERROR_READ;
    goto error;
  }
  endian = ntohl(incoming[0]);
  length = ntohl(incoming[1]);
  bit64  = ntohl(incoming[2]);

  /* store client bits, endian info */
  info.bit64 = bit64;
  info.endian_client = endian;

  /* read ET filename */
  if (etNetTcpRead(connfd, et_name, length) != length) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_client_thread: read failure\n");
    }
    err = ET_ERROR_READ;
    goto error;
  }

  /*
   * Check to see if the ET system that the client is trying to
   * connect to is this one (as it should be).
   */
  if (strcmp(et_name, etid->sys->config.filename) != 0) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_client_thread: Sorry! May only connect to %s\n", etid->sys->config.filename);
    }
    err = ET_ERROR;
    goto error;
  }

  /* send ET system info back to client */
  outgoing[0] = htonl(ET_OK);
  outgoing[1] = htonl((uint32_t)info.endian);
  outgoing[2] = htonl((uint32_t)etid->sys->config.nevents);
  outgoing[3] = htonl(ET_HIGHINT(etid->sys->config.event_size));
  outgoing[4] = htonl(ET_LOWINT(etid->sys->config.event_size));
  outgoing[5] = htonl((uint32_t)etid->version);
  outgoing[6] = htonl((uint32_t)etid->nselects);
  outgoing[7] = htonl((uint32_t)etid->lang);
  outgoing[8] = htonl((uint32_t)etid->bit64);
  /* not used */
  outgoing[9] = 0;

  if (etNetTcpWrite(connfd, (void *) outgoing, sizeof(outgoing)) != sizeof(outgoing)) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_client_thread: write failure\n");
    }
    close(connfd);
    pthread_exit(NULL);
  }

  /* wait for and process client requests */
  et_command_loop(&info);

  /* we are done with connected socket */
  close(connfd);

  /* quit thread */
  pthread_exit(NULL);

  error:
    /* if connection is NOT shut down, send reply */
    if (err == ET_ERROR) {
      err = htonl((uint32_t)err);
      if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
          et_logmsg("ERROR", "et_client_thread: write failure\n");
        }
      }
    }
    close(connfd);
    pthread_exit(NULL);
}

/************************************************************/
static void et_command_loop(et_threadinfo *info)
{
  int i, connfd, command, err, iov_max, bit64, status;
  int *histogram=NULL, *header=NULL, attaches[ET_ATTACHMENTS_MAX];
  size_t nevents_max;
  et_event     **events = NULL;
  struct iovec *iov;
  et_id        *etid = info->id;
  et_sys_id     id = (et_sys_id) info->id;
  uint32_t     *ints32 = NULL;

  connfd      = info->connfd;
  iov_max     = info->iov_max;
  bit64       = info->bit64;
  nevents_max = (size_t ) etid->sys->config.nevents;

  /*
   * Keep track of all the attachments this client makes
   * as they may need to be detached if the client dies
   * without cleanly disconnecting itself. Detaching
   * takes care of all events that were sent to clients
   * as events to be modified, but were never put back.
   */
  for (i=0; i < ET_ATTACHMENTS_MAX; i++) {
    attaches[i] = -1;
  }

  /*
   * Allocate things ahead of time so it doesn't need
   * to be done again with each remote command.
   */
  if ( (iov = (struct iovec *) calloc((2*nevents_max)+1, sizeof(struct iovec))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_command_loop: cannot allocate memory\n");
    }
    return;
  }

  if ( (header = (int *) calloc(nevents_max, (9+ET_STATION_SELECT_INTS)*sizeof(int))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_command_loop: cannot allocate memory\n");
    }
    free(iov);
    return;
  }

  /* allocate channels for each event + zero events + 1 more for err */
  if ( (histogram = (int *) calloc(nevents_max+2, sizeof(int))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_command_loop: cannot allocate memory\n");
    }
    free(iov); free(header);
    return;
  }

  if ( (events = (et_event **) calloc(nevents_max, sizeof(et_event *))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_command_loop: cannot allocate memory\n");
    }
    free(iov); free(header); free(histogram);
    return;
  }

  /* Need to send 32bit ints (events' place) for transfer */
  if ( (ints32 = (uint32_t *) calloc(nevents_max, sizeof(uint32_t))) == NULL) {
    if (etid->debug >= ET_DEBUG_ERROR) {
      et_logmsg("ERROR", "et_command_loop: cannot allocate memory\n");
    }
    free(iov); free(header); free(histogram); free(events);
    return;
  }

  /* The Command Loop ... */
  while (1) {

    /* first, read the remote command */
    if (etNetTcpRead(connfd, &command, sizeof(command)) != sizeof(command)) {
      /*if (etid->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "et_command_loop: error reading command\n");
      }*/
      goto end;
    }
    command = ntohl((uint32_t)command);
    /* Since there are so many commands, break up things up a bit */
    if (command < ET_NET_STAT_GATTS) {
      /* Notice that for the commands ending in "_L", which is the local
       * Linux stuff, no swapping (ntohl or htonl) is necessary since it's
       * all local communication.
       */

      switch (command) {

        case  ET_NET_EV_GET_L:
        {
          et_att_id  att;
          int wait, incoming[4], transfer[3];
          struct timespec deltatime;
          et_event *event = NULL;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          att  = incoming[0];
          wait = incoming[1];

          if (wait == ET_TIMED) {
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              /* before going further, check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
              }
              else {
                  deltatime.tv_sec  = incoming[2];
                  deltatime.tv_nsec = incoming[3];
                  err = et_event_get(id, att, &event, wait, &deltatime);
              }
          }
          else if (wait == ET_SLEEP) {
            /* There's a problem if we have a remote client that is waiting
              * for another event by sleeping and the events stop flowing. In
             * that case, the client can be killed and the ET system does NOT
             * know about it. Since this thread will be stuck in et_event_get,
             * it will not immediately detect the break in the socket - at least
             * not until event start flowing again. To circumvent this, implement
             * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
             * detection of broken socket between calls to et_event_get.
             */
            struct timeval timeout;
            fd_set myset;
            etid->sys->attach[att].sleep = ET_ATT_SLEEP;
            FD_ZERO(&myset);
            err = ET_ERROR_TIMEOUT;

            /* 3 second timed wait */
            deltatime.tv_sec  = 3;
            deltatime.tv_nsec = 0;

            while (err == ET_ERROR_TIMEOUT) {
              /* Linux modifies timeout, so reset each round */
              timeout.tv_sec  = 0;
              timeout.tv_usec = 0;
              FD_SET(connfd, &myset);

              /* before waiting 3 sec in "get", check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
                  break;
              }
              
              err = et_event_get(id, att, &event, ET_TIMED, &deltatime);

              /* Async call to select to see if this socket is still open.
               * Ready to read only if socket error in this case.
               */
              if (err == ET_ERROR_TIMEOUT) {
                if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                  goto end;
                }
              }
              else {
                etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
              }
              
            }
          }
          else {
            err = et_event_get(id, att, &event, wait, NULL);
          }

          transfer[0] = err;
          transfer[1] = ET_HIGHINT((uintptr_t) event);
          transfer[2] = ET_LOWINT((uintptr_t) event);

          if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EVS_GET_L:
        {
          et_att_id  att;
          int wait, num, nevents, incoming[5];
          struct iovec iovv[2];
          struct timespec deltatime;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }
          att  = incoming[0];
          wait = incoming[1];
          num  = incoming[2];

          if (wait == ET_TIMED) {
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              /* before going further, check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
              }
              else {
                  deltatime.tv_sec  = incoming[3];
                  deltatime.tv_nsec = incoming[4];
                  err = et_events_get(id, att, events, wait, &deltatime, num, &nevents);
              }
          }
          else if (wait == ET_SLEEP) {
            /* There's a problem if we have a Linux client that is waiting
             * for another event by sleeping and the events stop flowing. In
             * that case, the client can be killed and the ET system does NOT
             * know about it. Since this thread will be stuck in et_events_get,
             * it will not immediately detect the break in the socket - at least
             * not until event start flowing again. To circumvent this, implement
             * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
             * detection of broken socket between calls to et_events_get.
             */
            struct timeval timeout;
            fd_set myset;
            etid->sys->attach[att].sleep = ET_ATT_SLEEP;
            FD_ZERO(&myset);
            err = ET_ERROR_TIMEOUT;

            /* 3 second timed wait */
            deltatime.tv_sec  = 3;
            deltatime.tv_nsec = 0;

            while (err == ET_ERROR_TIMEOUT) {
              /* Linux modifies timeout, so reset each round */
              timeout.tv_sec  = 0;
              timeout.tv_usec = 0;
              FD_SET(connfd, &myset);

              /* before waiting 3 sec in "get", check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
                  break;
              }
              
              err = et_events_get(id, att, events, ET_TIMED, &deltatime, num, &nevents);

              /* Async call to select to see if this socket is still open.
               * Ready to read only if socket error in this case.
               */
              if (err == ET_ERROR_TIMEOUT) {
                if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                  goto end;
                }
              }
              else {
                etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
              }
            }
          }
          else {
            err = et_events_get(id, att, events, wait, NULL, num, &nevents);
          }

          if (err < 0) {
            if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
              goto end;
            }
            break;
          }

          iovv[0].iov_base = (void *) &nevents;
          iovv[0].iov_len  = sizeof(nevents);
          iovv[1].iov_base = (void *) events;
          iovv[1].iov_len  = nevents*sizeof(et_event *);

          if (etNetTcpWritev(connfd, iovv, 2, iov_max) == -1) {
            goto end;
          }
        }
        break;

        case  ET_NET_EV_PUT_L:
        {
          int incoming[3];
          et_event    *pe;
          et_att_id   att;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          att = incoming[0];
  /* Pointers (may be 64 bits) are in ET system space and must be translated.
   * The following ifdef avoids compiler warnings.
   */
#ifdef _LP64
          pe  = (et_event *) (ET_64BIT_P(incoming[1],incoming[2]));
#else
          pe  = (et_event *) incoming[2];
#endif
          
          err = et_event_put(id, att, pe);

          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EVS_PUT_L:
        {
          int       nevents, incoming[2];
          size_t    len;
          et_att_id att;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }
          att     = incoming[0];
          nevents = incoming[1];
          len = nevents*sizeof(et_event *);

          if (etNetTcpRead(connfd, (void *) events, (int)len) != len) {
            goto end;
          }

          err = et_events_put(id, att, events, nevents);

          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EV_NEW_L:
        {
          et_att_id  att;
          int mode, incoming[6], transfer[3];
          size_t size;
          struct timespec deltatime;
          et_event *event = NULL;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          att  = incoming[0];
          mode = incoming[1];
          size = ET_64BIT_UINT(incoming[2], incoming[3]);

          if (mode == ET_TIMED) {
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              /* before going further, check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
              }
              else {
                  deltatime.tv_sec  = incoming[4];
                  deltatime.tv_nsec = incoming[5];
                  err = et_event_new(id, att, &event, mode, &deltatime, size);
              }
          }
          else if (mode == ET_SLEEP) {
            /* There's a problem if we have a remote client that is waiting
             * for a new event by sleeping and the events stop flowing. In
             * that case, the client can be killed and the ET system does NOT
             * know about it. Since this thread will be stuck in et_event_new,
             * it will not immediately detect the break in the socket - at least
             * not until event start flowing again. To circumvent this, implement
             * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
             * detection of broken socket between calls to et_event_new.
             */
            struct timeval timeout;
            fd_set myset;
            etid->sys->attach[att].sleep = ET_ATT_SLEEP;
            FD_ZERO(&myset);
            err = ET_ERROR_TIMEOUT;

            /* 3 second timed wait */
            deltatime.tv_sec  = 3;
            deltatime.tv_nsec = 0;

            while (err == ET_ERROR_TIMEOUT) {
              /* Linux modifies timeout, so reset each round */
              timeout.tv_sec  = 0;
              timeout.tv_usec = 0;
              FD_SET(connfd, &myset);

              /* before waiting 3 sec in "get", check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
                  break;
              }
              
              err = et_event_new(id, att, &event, ET_TIMED, &deltatime, size);

              /* Async call to select to see if this socket is still open.
               * Ready to read only if socket error in this case.
               */
              if (err == ET_ERROR_TIMEOUT) {
                if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                  goto end;
                }
              }
              else {
                etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
              }
            }
          }
          else {
            err = et_event_new(id, att, &event, mode, NULL, size);
          }

          transfer[0] = err;
          
          if (err == ET_OK) {
              transfer[1] = ET_HIGHINT((uintptr_t) event);
              transfer[2] = ET_LOWINT((uintptr_t) event);
          }
          else {
              transfer[1] = 0;
              transfer[2] = 0;
          }

          if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EVS_NEW_L:
        {
          et_att_id  att;
          int mode, nevents, num, incoming[7];
          size_t size;
          struct iovec iovv[2];
          struct timespec deltatime;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          att  = incoming[0];
          mode = incoming[1];
          size = ET_64BIT_UINT(incoming[2], incoming[3]);
          num  = incoming[4];

          if (mode == ET_TIMED) {
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              /* before going further, check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
              }
              else {
                  deltatime.tv_sec  = incoming[5];
                  deltatime.tv_nsec = incoming[6];
                  err = et_events_new(id, att, events, mode, &deltatime, size, num, &nevents);
              }
          }
          else if (mode == ET_SLEEP) {
              /* There's a problem if we have a remote client that is waiting
               * for a new event by sleeping and the events stop flowing. In
               * that case, the client can be killed and the ET system does NOT
               * know about it. Since this thread will be stuck in et_events_new,
               * it will not immediately detect the break in the socket - at least
               * not until event start flowing again. To circumvent this, implement
               * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
               * detection of broken socket between calls to et_events_new.
               */
              struct timeval timeout;
              fd_set myset;
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              FD_ZERO(&myset);
              err = ET_ERROR_TIMEOUT;

              /* 3 second timed wait */
              deltatime.tv_sec  = 3;
              deltatime.tv_nsec = 0;

              while (err == ET_ERROR_TIMEOUT) {
                  /* Linux modifies timeout, so reset each round */
                  timeout.tv_sec  = 0;
                  timeout.tv_usec = 0;
                  FD_SET(connfd, &myset);

                  /* before waiting 3 sec in "get", check here if we're to wake up */
                  if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                      etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                      etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                      err = ET_ERROR_WAKEUP;
                      break;
                  }

                  err = et_events_new(id, att, events, ET_TIMED, &deltatime, size, num, &nevents);

                  /* Async call to select to see if this socket is still open.
                   * Ready to read only if socket error in this case.
                   */
                  if (err == ET_ERROR_TIMEOUT) {
                      if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                          goto end;
                      }
                  }
                  else {
                      etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                      etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  }
              }
          }
          else {
              err = et_events_new(id, att, events, mode, NULL, size, num, &nevents);
          }

          if (err < 0) {
              if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
                  goto end;
              }
              break;
          }

          iovv[0].iov_base = (void *) &nevents;
          iovv[0].iov_len  = sizeof(nevents);
          iovv[1].iov_base = (void *) events;
          iovv[1].iov_len  = nevents*sizeof(et_event *);

          if (etNetTcpWritev(connfd, iovv, 2, iov_max) == -1) {
              goto end;
          }
        }
        break;

        case  ET_NET_EVS_NEW_GRP_L:
        {
          et_att_id  att;
          int mode, nevents, num, group, incoming[8];
          size_t size;
          struct iovec iovv[2];
          struct timespec deltatime;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          att   = incoming[0];
          mode  = incoming[1];
          size  = ET_64BIT_UINT(incoming[2], incoming[3]);
          num   = incoming[4];
          group = incoming[5];

          if (mode == ET_TIMED) {
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              /* before going further, check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
              }
              else {
                  deltatime.tv_sec  = incoming[6];
                  deltatime.tv_nsec = incoming[7];
                  err = et_events_new_group(id, att, events, mode, &deltatime,
                                            size, num, group, &nevents);
              }
          }
          else if (mode == ET_SLEEP) {
              struct timeval timeout;
              fd_set myset;
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              FD_ZERO(&myset);
              err = ET_ERROR_TIMEOUT;

              /* 3 second timed wait */
              deltatime.tv_sec  = 3;
              deltatime.tv_nsec = 0;

              while (err == ET_ERROR_TIMEOUT) {
                  /* Linux modifies timeout, so reset each round */
                  timeout.tv_sec  = 0;
                  timeout.tv_usec = 0;
                  FD_SET(connfd, &myset);

                  /* before waiting 3 sec in "get", check here if we're to wake up */
                  if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                      etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                      etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                      err = ET_ERROR_WAKEUP;
                      break;
                  }

                  err = et_events_new_group(id, att, events, ET_TIMED, &deltatime,
                          size, num, group, &nevents);

                  /* Async call to select to see if this socket is still open.
                   * Ready to read only if socket error in this case.
                   */
                  if (err == ET_ERROR_TIMEOUT) {
                      if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                          goto end;
                      }
                  }
                  else {
                      etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                      etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  }
              }
          }
          else {
              err = et_events_new_group(id, att, events, mode, NULL,
                      size, num, group, &nevents);
          }

          if (err < 0) {
              if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
                  goto end;
              }
              break;
          }

          iovv[0].iov_base = (void *) &nevents;
          iovv[0].iov_len  = sizeof(nevents);
          iovv[1].iov_base = (void *) events;
          iovv[1].iov_len  = nevents*sizeof(et_event *);

          if (etNetTcpWritev(connfd, iovv, 2, iov_max) == -1) {
              goto end;
          }
        }
        break;

        case  ET_NET_EV_DUMP_L:
        {
          int incoming[3];
          et_event    *pe;
          et_att_id   att;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          att = incoming[0];
          /* Pointers (may be 64 bits) are in ET system space and must be translated.
           * The following ifdef avoids compiler warnings.
           */
#ifdef _LP64
          pe  = (et_event *) (ET_64BIT_P(incoming[1],incoming[2]));
#else
          pe  = (et_event *) incoming[2];
#endif

          err = et_event_dump(id, att, pe);

          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EVS_DUMP_L:
        {
          int       nevents, incoming[2];
          size_t    len;
          et_att_id att;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }
          att     = incoming[0];
          nevents = incoming[1];
          len = nevents*sizeof(et_event *);

          if (etNetTcpRead(connfd, (void *) events, (int)len) != len) {
            goto end;
          }

          err = et_events_dump(id, att, events, nevents);

          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EV_GET:
        {
          et_att_id  att;
          int wait, modify, dumpEvents;
          uint32_t incoming[5], hheader[10+ET_STATION_SELECT_INTS];
          struct timespec deltatime;
          struct iovec iovv[2];
          et_event *event = NULL;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }
          att    = ntohl(incoming[0]);
          wait   = ntohl(incoming[1]);
          modify = ntohl(incoming[2]);
          
          /* Do we put or dump events back into ET system? */
          dumpEvents = modify & ET_DUMP;
          /* Are we going to modify the event or its header remotely? */
          modify &= (ET_MODIFY | ET_MODIFY_HEADER);
          
          if (wait == ET_TIMED) {
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              /* before going further, check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
              }
              else {
                deltatime.tv_sec  = ntohl(incoming[3]);
                deltatime.tv_nsec = ntohl(incoming[4]);
                err = et_event_get(id, att, &event, wait, &deltatime);
              }
          }
          else if (wait == ET_SLEEP) {
            /* There's a problem if we have a remote client that is waiting
             * for another event by sleeping and the events stop flowing. In
             * that case, the client can be killed and the ET system does NOT
             * know about it. Since this thread will be stuck in et_event_get,
             * it will not immediately detect the break in the socket - at least
             * not until events start flowing again. To circumvent this, implement
             * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
             * detection of broken socket between calls to et_event_get.
             */
            struct timeval timeout;
            fd_set myset;
            etid->sys->attach[att].sleep = ET_ATT_SLEEP;
            FD_ZERO(&myset);
            err = ET_ERROR_TIMEOUT;

            /* 3 second timed wait */
            deltatime.tv_sec  = 3;
            deltatime.tv_nsec = 0;

            while (err == ET_ERROR_TIMEOUT) {
              /* Linux modifies timeout, so reset each round */
              timeout.tv_sec  = 0;
              timeout.tv_usec = 0;
              FD_SET(connfd, &myset);

              /* before waiting 3 sec in "get", check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
                  break;
              }
              
              err = et_event_get(id, att, &event, ET_TIMED, &deltatime);

              /* Async call to select to see if this socket is still open.
               * Ready to read only if socket error in this case.
               */
              if (err == ET_ERROR_TIMEOUT) {
                if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                  goto end;
                }
              }
              else {
                etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
              }
            }
          }
          else {
            err = et_event_get(id, att, &event, wait, NULL);
          }
          
          hheader[0] = htonl((uint32_t)err);
          if (err < ET_OK) {
            if (etNetTcpWrite(connfd, (void *) hheader, sizeof(hheader[0])) != sizeof(hheader[0])) {
              goto end;
            }
            break;
          }
          
          /* if we're 64 bit & client is 32 bit, don't send event that's too big */
#ifdef _LP64
          if (!bit64 && event->length > UINT32_MAX/5) {
            
            /* we got an event but it's too big so put/dump it back */
            if (dumpEvents) {
              et_event_dump(id, att, event);
            }
            else {
              et_event_put(id, att, event);
            }
            
            err = htonl((uint32_t)ET_ERROR_TOOBIG);
            if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
              goto end;
            }
            
            break;
          }
#endif
          
          /* keep track of how this event is to be modified */
          event->modify = modify;

          hheader[1] = htonl(ET_HIGHINT(event->length));
          hheader[2] = htonl(ET_LOWINT(event->length));
          hheader[3] = htonl(ET_HIGHINT(event->memsize));
          hheader[4] = htonl(ET_LOWINT(event->memsize));
          /* send the priority & datastatus together and save space */
          hheader[5] = htonl((uint32_t) (event->priority | event->datastatus << ET_DATA_SHIFT));
          /* send an index into shared memory and NOT a pointer - for local Java clients */
          hheader[6] = htonl((uint32_t)event->place);
          hheader[7] = 0; /* not used */
          hheader[8] = (uint32_t) event->byteorder;
          hheader[9] = 0; /* not used */
          for (i=0; i < ET_STATION_SELECT_INTS; i++) {
            hheader[i+10] = htonl((uint32_t)event->control[i]);
          }

          iovv[0].iov_base = (void *) hheader;
          iovv[0].iov_len  = sizeof(hheader);
          iovv[1].iov_base = event->pdata;
          iovv[1].iov_len  = event->length;

          /* write data */
          if (etNetTcpWritev(connfd, iovv, 2, iov_max) == -1) {
            goto end;
          }

          if (modify == 0) {
            if (dumpEvents) {
              et_event_dump(id, att, event);
            }
            else {
              et_event_put(id, att, event);
            }
          }
        }
        break;

        case  ET_NET_EVS_GET:
        {
          et_att_id  att;
          int j, wait, num, modify, nevents=0, index, dumpEvents;
          size_t size, headersize;
#ifdef _LP64
          uint64_t lengthSum = 0ULL;
#endif
          uint32_t incoming[6], outgoing[3];
          struct timespec deltatime;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }
          att    = ntohl(incoming[0]);
          wait   = ntohl(incoming[1]);
          modify = ntohl(incoming[2]);
          num    = ntohl(incoming[3]);
          /* Do we put or dump events back into ET system? */
          dumpEvents = modify & ET_DUMP;
          /* Are we going to modify the event or its header remotely? */
          modify &= (ET_MODIFY | ET_MODIFY_HEADER);
          
          if (wait == ET_TIMED) {
              etid->sys->attach[att].sleep = ET_ATT_SLEEP;
              /* before going further, check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
              }
              else {
                deltatime.tv_sec  = ntohl(incoming[4]);
                deltatime.tv_nsec = ntohl(incoming[5]);
                err = et_events_get(id, att, events, wait, &deltatime, num, &nevents);
              }
          }
          else if (wait == ET_SLEEP) {
            /* There's a problem if we have a remote client that is waiting
             * for another event by sleeping and the events stop flowing. In
             * that case, the client can be killed and the ET system does NOT
             * know about it. Since this thread will be stuck in et_events_get,
             * it will not immediately detect the break in the socket - at least
             * not until event start flowing again. To circumvent this, implement
             * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
             * detection of broken socket between calls to et_events_get.
             */
            struct timeval timeout;
            fd_set myset;
            etid->sys->attach[att].sleep = ET_ATT_SLEEP;
            FD_ZERO(&myset);
            err = ET_ERROR_TIMEOUT;

            /* 3 second timed wait */
            deltatime.tv_sec  = 3;
            deltatime.tv_nsec = 0;

            while (err == ET_ERROR_TIMEOUT) {
              /* Linux modifies timeout, so reset each round */
              timeout.tv_sec  = 0;
              timeout.tv_usec = 0;
              FD_SET(connfd, &myset);
              
              /* before waiting 3 sec in "get", check here if we're to wake up */
              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
                  break;
              }
              
              err = et_events_get(id, att, events, ET_TIMED, &deltatime, num, &nevents);

              /* Async call to select to see if this socket is still open.
               * Ready to read only if socket error in this case.
               */
              if (err == ET_ERROR_TIMEOUT) {
                if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                  goto end;
                }
              }
              else {
                etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
              }
            }
          }
          else {
            err = et_events_get(id, att, events, wait, NULL, num, &nevents);
          }

          if (err != ET_OK) {
            err = htonl((uint32_t)err);
            if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
              goto end;
            }
            break;
          }

         /* if we're 64 bit & client is 32 bit, don't send event that's too big */
#ifdef _LP64
          if (!bit64) {
            for (i=0; i < nevents; i++) {
              lengthSum += events[i]->length;
              
              if (lengthSum > UINT32_MAX/5) {
                
                 /* we got some events but they're too big so put/dump 'em back */
                if (dumpEvents) {
                  et_events_dump(id, att, events, nevents);
                }
                else {
                  et_events_put(id, att, events, nevents);
                }

                err = htonl((uint32_t)ET_ERROR_TOOBIG);
                if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
                  goto end;
                }
                
                break;
              }
            }
            if (err != ET_OK) break;
          }
#endif
          
          headersize = sizeof(int)*(9+ET_STATION_SELECT_INTS);
          size = nevents*headersize;
          for (j=0; j < nevents; j++) {
            size += events[j]->length;
          }

          outgoing[0] = htonl((uint32_t)nevents);
          outgoing[1] = htonl(ET_HIGHINT(size));
          outgoing[2] = htonl(ET_LOWINT(size));
          iov[0].iov_base = (void *) outgoing;
          iov[0].iov_len  = sizeof(outgoing);
          index=0;

          for (i=0; i < nevents; i++) {
            /* keep track of how this event is to be modified */
            events[i]->modify = modify;
            header[index]   = htonl(ET_HIGHINT(events[i]->length));
            header[index+1] = htonl(ET_LOWINT(events[i]->length));
            
            header[index+2] = htonl(ET_HIGHINT(events[i]->memsize));
            header[index+3] = htonl(ET_LOWINT(events[i]->memsize));
            header[index+4] = htonl((uint32_t) (events[i]->priority | events[i]->datastatus << ET_DATA_SHIFT));
/*printf("Event %d: len = %llu, memsize = %llu\n", i, events[i]->length,  events[i]->memsize);*/
                                    
/*printf("Get %p, high = %x, low = %x\n", events[i],
ET_HIGHINT((uintptr_t)events[i]), ET_LOWINT((uintptr_t)events[i]));
*/
            /* send an index into shared memory and NOT a pointer - for local Java clients */
            header[index+5] = htonl((uint32_t)events[i]->place);
            header[index+6] = 0; /* not used */
            header[index+7] = events[i]->byteorder;
            header[index+8] = 0; /* not used */
            for (j=0; j < ET_STATION_SELECT_INTS; j++) {
              header[index+9+j] = htonl((uint32_t)events[i]->control[j]);
            }
            iov[2*i+1].iov_base = (void *) &header[index];
            iov[2*i+1].iov_len  = headersize;
            iov[2*i+2].iov_base = events[i]->pdata;
            iov[2*i+2].iov_len  = events[i]->length;
            index += (9+ET_STATION_SELECT_INTS);
          }

          if (etNetTcpWritev(connfd, iov, 2*nevents+1, iov_max) == -1) {
            goto end;
          }

          if (modify == 0) {
            if (dumpEvents) {
              et_events_dump(id, att, events, nevents);
            }
            else {
              et_events_put(id, att, events, nevents);
            }
          }
        }
        break;

        case  ET_NET_EV_PUT:
        {
          uint32_t incoming[8+ET_STATION_SELECT_INTS];
          et_event    *pe;
          et_att_id   att;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }
          
          att              = ntohl(incoming[0]);
          /* Pointers (may be 64 bits) are in ET system space and must be translated.
           * The following ifdef avoids compiler warnings.
           */
/*#ifdef _LP64
          pe  = (et_event *) ET_64BIT_P(ntohl(incoming[1]),ntohl(incoming[2]));
#else
          pe  = (et_event *) ntohl(incoming[2]);
#endif*/
          pe               = ET_P2EVENT(etid, ntohl(incoming[1])); /* incoming[2] is NOT used */
          pe->length       = ET_64BIT_UINT(ntohl(incoming[3]),ntohl(incoming[4]));
          pe->priority     = ntohl(incoming[5]) & ET_PRIORITY_MASK;
          pe->datastatus   =(ntohl(incoming[5]) & ET_DATA_MASK) >> ET_DATA_SHIFT;
          pe->byteorder    = incoming[6];
          for (i=0; i < ET_STATION_SELECT_INTS; i++) {
            pe->control[i] = ntohl(incoming[i+8]);
          }
          
          /* only read data if modifying everything */
          if (pe->modify == ET_MODIFY) {
            if (etNetTcpRead(connfd, pe->pdata, (int) pe->length) != pe->length) {
              goto end;
            }
          }

          err = et_event_put(id, att, pe);

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EVS_PUT:
        {
          int j, nevents;
          uint32_t incoming[4], hheader[7+ET_STATION_SELECT_INTS];
          /*uint64_t size;*/
          size_t len;
          et_att_id  att;

/*printf("etr_events_put: read incoming array\n");*/
          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }
          att     = ntohl(incoming[0]);
          nevents = ntohl(incoming[1]);
/*          size    = ET_64BIT_UINT(ntohl(incoming[2]),ntohl(incoming[3]));
printf("etr_events_put: att = %d, nevents = %d, size = %lu\n", att, nevents, size);*/

          for (i=0; i < nevents; i++) {
/*printf("etr_events_put: i = %d, read in header next, %d ints\n", i, 7+ET_STATION_SELECT_INTS);*/
            if (etNetTcpRead(connfd, (void *)hheader, sizeof(hheader)) != sizeof(hheader)) {
              goto end;
            }

          /* Pointers (may be 64 bits) are in ET system space and must be translated.
           * The following ifdef avoids compiler warnings.
           */
/*#ifdef _LP64
            events[i] = (et_event *) ET_64BIT_P(ntohl(hheader[0]),ntohl(hheader[1]));
#else
            events[i] = (et_event *) ntohl(hheader[1]);
#endif*/
            events[i]              = ET_P2EVENT(etid, ntohl(hheader[0])); /* hheader[1] is NOT used */
/*printf("etr_events_put: pointer = %p\n", events[i]);*/
            events[i]->length      = ET_64BIT_UINT(ntohl(hheader[2]),ntohl(hheader[3]));
            len                    = (size_t)events[i]->length;
/*printf("etr_events_put: len = %d\n", (int)len); */
            events[i]->priority    = ntohl(hheader[4]) & ET_PRIORITY_MASK;
            events[i]->datastatus  =(ntohl(hheader[4]) & ET_DATA_MASK) >> ET_DATA_SHIFT;
            events[i]->byteorder   = hheader[5];
            for (j=0; j < ET_STATION_SELECT_INTS; j++) {
              events[i]->control[j] = ntohl(hheader[j+7]);
            }
            /* only read data if modifying everything */
            if (events[i]->modify == ET_MODIFY) {
              if (etNetTcpRead(connfd, events[i]->pdata, (int)len) != len) {
                goto end;
              }
/*printf("etr_events_put: read in data next = %d\n", ET_SWAP32(*((int *) (events[i]->pdata))) );*/
            }
          }
/*printf("etr_events_put: put event for real\n");*/
          err = et_events_put(id, att, events, nevents);

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EV_NEW:
        {
          et_att_id att;
          int mode;
          uint32_t incoming[6], transfer[3];
          uint64_t size;
          struct timespec deltatime;
          et_event *pe = NULL;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          att  = ntohl(incoming[0]);
          mode = ntohl(incoming[1]);
          size = ET_64BIT_UINT(ntohl(incoming[2]), ntohl(incoming[3]));
          
#ifndef _LP64
          /* if we're 32 bit, an 64 bit app may ask for event which is too big */
          if (bit64 && size > UINT32_MAX/5) {
            transfer[0] = htonl(ET_ERROR_TOOBIG);
            transfer[1] = 0;
            transfer[2] = 0;

            if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
              goto end;
            }
            break;
          }
#endif
                  
          if (mode == ET_TIMED) {
            etid->sys->attach[att].sleep = ET_ATT_SLEEP;
            /* before going further, check here if we're to wake up */
            if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                err = ET_ERROR_WAKEUP;
            }
            else {
                deltatime.tv_sec  = ntohl(incoming[4]);
                deltatime.tv_nsec = ntohl(incoming[5]);
                err = et_event_new(id, att, &pe, mode, &deltatime, size);
            }
          }
          else if (mode == ET_SLEEP) {
            /* There's a problem if we have a remote client that is waiting
             * for a new event by sleeping and the events stop flowing. In
             * that case, the client can be killed and the ET system does NOT
             * know about it. Since this thread will be stuck in et_event_new,
             * it will not immediately detect the break in the socket - at least
             * not until events start flowing again. To circumvent this, implement
             * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
             * detection of broken socket between calls to et_event_new.
             */
            struct timeval timeout;
            fd_set myset;
            etid->sys->attach[att].sleep = ET_ATT_SLEEP;
            FD_ZERO(&myset);
            err = ET_ERROR_TIMEOUT;

            /* 3 second timed wait */
            deltatime.tv_sec  = 3;
            deltatime.tv_nsec = 0;

            while (err == ET_ERROR_TIMEOUT) {
              /* Linux modifies timeout, so reset each round */
              timeout.tv_sec  = 0;
              timeout.tv_usec = 0;
              FD_SET(connfd, &myset);

              if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                  etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                  etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                  err = ET_ERROR_WAKEUP;
                  break;
              }
              
              err = et_event_new(id, att, &pe, ET_TIMED, &deltatime, size);

              /* Async call to select to see if this socket is still open.
               * Ready to read only if socket error in this case.
               */
              if (err == ET_ERROR_TIMEOUT) {
                if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                  goto end;
                }
              }
              else {
                etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
              }
            }
          }
          else {
            err = et_event_new(id, att, &pe, mode, NULL, size);
          }
          
          /* keep track of how this event is to be modified */
          if (err == ET_OK) {
              pe->modify = ET_MODIFY;
              /* send an index into shared memory and NOT a pointer - for local Java clients */
              transfer[1] = htonl((uint32_t)pe->place);
          }
          else {
              transfer[1] = 0;
          }
          
          transfer[0] = htonl((uint32_t)err);
          transfer[2] = 0; /* not used */

          if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
        }

        break;

        case  ET_NET_EVS_NEW:
        {
            et_att_id  att;
            int mode, nevents=0, num, nevents_net;
            uint32_t incoming[7];
            uint64_t size;
            struct timespec deltatime;
            struct iovec iovv[2];

            if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
                goto end;
            }

            att  = ntohl(incoming[0]);
            mode = ntohl(incoming[1]);
            size = ET_64BIT_UINT(ntohl(incoming[2]), ntohl(incoming[3]));
            num  = ntohl(incoming[4]);

#ifndef _LP64
            /* if we're 32 bit, an 64 bit app may ask for events which are too big */
            if (bit64 && num*size > UINT32_MAX/5) {
                err = htonl(ET_ERROR_TOOBIG);
                if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
                    goto end;
                }
                break;
            }
#endif

            if (mode == ET_TIMED) {
                etid->sys->attach[att].sleep = ET_ATT_SLEEP;
                /* before going further, check here if we're to wake up */
                if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                    etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                    etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                    err = ET_ERROR_WAKEUP;
                }
                else {
                    deltatime.tv_sec  = ntohl(incoming[5]);
                    deltatime.tv_nsec = ntohl(incoming[6]);
                    err = et_events_new(id, att, events, mode, &deltatime, size, num, &nevents);
                }
            }
            else if (mode == ET_SLEEP) {
                /* There's a problem if we have a remote client that is waiting
                 * for a new event by sleeping and the events stop flowing. In
                 * that case, the client can be killed and the ET system does NOT
                 * know about it. Since this thread will be stuck in et_events_new,
                 * it will not immediately detect the break in the socket - at least
                 * not until event start flowing again. To circumvent this, implement
                 * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
                 * detection of broken socket between calls to et_events_new.
                 */
                struct timeval timeout;
                fd_set myset;
                etid->sys->attach[att].sleep = ET_ATT_SLEEP;
                FD_ZERO(&myset);
                err = ET_ERROR_TIMEOUT;

                /* 3 second timed wait */
                deltatime.tv_sec  = 3;
                deltatime.tv_nsec = 0;

                while (err == ET_ERROR_TIMEOUT) {
                    /* Linux modifies timeout, so reset each round */
                    timeout.tv_sec  = 0;
                    timeout.tv_usec = 0;
                    FD_SET(connfd, &myset);

                    /* before waiting 3 sec in "get", check here if we're to wake up */
                    if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                        etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                        etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                        err = ET_ERROR_WAKEUP;
                        break;
                    }

                    err = et_events_new(id, att, events, ET_TIMED, &deltatime, size, num, &nevents);

                    /* Async call to select to see if this socket is still open.
                     * Ready to read only if socket error in this case.
                     */
                    if (err == ET_ERROR_TIMEOUT) {
                        if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                            goto end;
                        }
                    }
                    else {
                        etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                        etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                    }
                }
            }
            else {
                err = et_events_new(id, att, events, mode, NULL, size, num, &nevents);
            }

            if (err < 0) {
                err = htonl((uint32_t)err);
                if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
                    goto end;
                }
                break;
            }

            for (i=0; i < nevents; i++) {
                /* keep track of how this event is to be modified */
                events[i]->modify = ET_MODIFY;
                ints32[i] = htonl((uint32_t)events[i]->place);
            }

            nevents_net = htonl((uint32_t)nevents);
            iovv[0].iov_base = (void *) &nevents_net;
            iovv[0].iov_len  = sizeof(nevents_net);
            iovv[1].iov_base = (void *) ints32;
            iovv[1].iov_len  = nevents*sizeof(uint32_t);

            if (etNetTcpWritev(connfd, iovv, 2, iov_max) == -1) {
                goto end;
            }
        }
        break;

        case  ET_NET_EVS_NEW_GRP:
        {
            et_att_id  att;
            int mode, nevents=0, num, group, nevents_net;
            uint32_t incoming[8];
            uint64_t size;
            struct timespec deltatime;
            struct iovec iovv[2];

            if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
                goto end;
            }

            att   = ntohl(incoming[0]);
            mode  = ntohl(incoming[1]);
            size  = ET_64BIT_UINT(ntohl(incoming[2]), ntohl(incoming[3]));
            num   = ntohl(incoming[4]);
            group = ntohl(incoming[5]);

#ifndef _LP64
            /* if we're 32 bit, a 64 bit app may ask for events which are too big */
            if (bit64 && num*size > UINT32_MAX/5) {
                err = htonl(ET_ERROR_TOOBIG);
                if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
                    goto end;
                }
                break;
            }
#endif

            if (mode == ET_TIMED) {
                etid->sys->attach[att].sleep = ET_ATT_SLEEP;
                /* before going further, check here if we're to wake up */
                if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                    etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                    etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                    err = ET_ERROR_WAKEUP;
                }
                else {
                    deltatime.tv_sec  = ntohl(incoming[6]);
                    deltatime.tv_nsec = ntohl(incoming[7]);
                    err = et_events_new_group(id, att, events, mode, &deltatime,
                        size, num, group, &nevents);
                }
            }
            else if (mode == ET_SLEEP) {
                /* There's a problem if we have a remote client that is waiting
                 * for a new event by sleeping and the events stop flowing. In
                 * that case, the client can be killed and the ET system does NOT
                 * know about it. Since this thread will be stuck in et_events_new,
                 * it will not immediately detect the break in the socket - at least
                 * not until event start flowing again. To circumvent this, implement
                 * ET_SLEEP by repeats of ET_TIMED every couple seconds to allow
                 * detection of broken socket between calls to et_events_new.
                 */
                struct timeval timeout;
                fd_set myset;
                etid->sys->attach[att].sleep = ET_ATT_SLEEP;
                FD_ZERO(&myset);
                err = ET_ERROR_TIMEOUT;

                /* 3 second timed wait */
                deltatime.tv_sec  = 3;
                deltatime.tv_nsec = 0;

                while (err == ET_ERROR_TIMEOUT) {
                    /* Linux modifies timeout, so reset each round */
                    timeout.tv_sec  = 0;
                    timeout.tv_usec = 0;
                    FD_SET(connfd, &myset);

                    /* before waiting 3 sec in "get", check here if we're to wake up */
                    if (etid->sys->attach[att].quit == ET_ATT_QUIT) {
                        etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                        etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                        err = ET_ERROR_WAKEUP;
                        break;
                    }

                    err = et_events_new_group(id, att, events, ET_TIMED, &deltatime,
                            size, num, group, &nevents);

                    /* Async call to select to see if this socket is still open.
                     * Ready to read only if socket error in this case.
                     */
                    if (err == ET_ERROR_TIMEOUT) {
                        if (select(connfd + 1, &myset, NULL, NULL, &timeout) > 0) {
                            goto end;
                        }
                    }
                    else {
                        etid->sys->attach[att].sleep = ET_ATT_NOSLEEP;
                        etid->sys->attach[att].quit  = ET_ATT_CONTINUE;
                    }
                }
            }
            else {
                err = et_events_new_group(id, att, events, mode, NULL,
                        size, num, group, &nevents);
            }

            if (err < 0) {
                err = htonl((uint32_t)err);
                if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
                    goto end;
                }
                break;
            }

            /* If we're on a 32 bit machine, we need to translate these
             * 32 bit pointers to 64 bit ints and send over the wire in
             * order to be compatible with 64 bit ET systems.
             */
            for (i=0; i < nevents; i++) {
                /* keep track of how this event is to be modified */
                events[i]->modify = ET_MODIFY;
                ints32[i] = htonl((uint32_t)events[i]->place);
            }

            nevents_net = htonl((uint32_t)nevents);
            iovv[0].iov_base = (void *) &nevents_net;
            iovv[0].iov_len  = sizeof(nevents_net);
            iovv[1].iov_base = (void *) ints32;
            iovv[1].iov_len  = nevents*sizeof(uint32_t);

            if (etNetTcpWritev(connfd, iovv, 2, iov_max) == -1) {
                goto end;
            }
        }
        break;

       case  ET_NET_EV_DUMP:
        {
          uint32_t incoming[2];
          et_event    *pe;
          et_att_id   att;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          att = ntohl(incoming[0]);
          pe  = ET_P2EVENT(etid, ntohl(incoming[1]));
          err = et_event_dump(id, att, pe);

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_EVS_DUMP:
        {
          uint32_t nevents, incoming[2];
          et_att_id  att;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }
          att     = ntohl(incoming[0]);
          nevents = ntohl(incoming[1]);

          if (etNetTcpRead(connfd, (void *) ints32, (int) (nevents*sizeof(int))) != nevents*sizeof(int)) {
            goto end;
          }

          for (i=0; i < nevents; i++) {
              events[i] = ET_P2EVENT(etid, ntohl(ints32[i]));
              /*events[i] = (et_event *) ((uintptr_t) ntoh64(ints32[i]));*/
          }

          err = et_events_dump(id, att, events, nevents);

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_ALIVE:
        {
          /* we must be alive by definition as this is in the ET process */
          err = htonl(1);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_WAIT:
        {
          /* We must be alive by definition as this is in the ET process,
           * hence there is no need to call et_wait_for_alive(id)
           */
          err = htonl(ET_OK);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_CLOSE:
        case  ET_NET_FCLOSE:
        {
          /* Don't bother sending a response as the caller doesn't listen
           * for one and just closes the socket anyway. */

          /* detach all attachments */
          for (i=0; i <ET_ATTACHMENTS_MAX ; i++) {
            if (attaches[i] > -1) {
              et_station_detach(id, attaches[i]);
            }
          }

          if (etid->debug >= ET_DEBUG_INFO) {
              et_logmsg("INFO", "et_command_loop: remote client closing\n");
          }

          free(iov); free(header); free(histogram); free(events); free(ints32);
        }
        return;

        case  ET_NET_WAKE_ATT:
        {
          et_att_id  att;

          if (etNetTcpRead(connfd, (void *) &att, sizeof(att)) != sizeof(att)) {
            goto end;
          }
          att = ntohl((uint32_t)att);

/*printf("ET SERVER: got wake-up-all cmd for att id = %d\n", (int)att);*/
          et_wakeup_attachment(id, att);
        }
        break;

        case  ET_NET_WAKE_ALL:
        {
          et_stat_id stat_id;

          if (etNetTcpRead(connfd, (void *) &stat_id, sizeof(stat_id)) != sizeof(stat_id)) {
            goto end;
          }
          stat_id = ntohl((uint32_t)stat_id);
/*printf("ET SERVER: got wake-up-all cmd for station id = %d\n", (int)stat_id);*/

          et_wakeup_all(id, stat_id);
        }
        break;

        case  ET_NET_KILL:
        {
          /* Don't bother sending a response as the caller doesn't listen
           * for one and just closes the socket anyway. */
            et_kill(id);
        }
        return;

        case  ET_NET_STAT_ATT:
        {
          et_stat_id  stat_id;
          et_att_id   att;
          uint32_t length, ipLength, outgoing[2], transfer[4];
          char host[ET_MAXHOSTNAMELEN], interface[ET_IPADDRSTRLEN];
          pid_t pid;

          if (etNetTcpRead(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
          stat_id  = ntohl(transfer[0]);
          pid      = ntohl(transfer[1]);
          length   = ntohl(transfer[2]);
          ipLength = ntohl(transfer[3]);

          if (length > 0) {
              if (etNetTcpRead(connfd, (void *) host, length) != length) {
                  goto end;
              }
          }

          if (ipLength > 0) {
              if (etNetTcpRead(connfd, (void *) interface, ipLength) != ipLength) {
                  goto end;
              }
          }

          err = et_station_attach(id, stat_id, &att);
          /* keep track of all attachments */
          if (err == ET_OK) {
            attaches[att] = att;
            /*
             * Register, in shared memory, that this attachment
             * is to a process "pid" running on "host".
             */
            etid->sys->attach[att].pid = pid;
            if (length > 0) {
                strcpy(etid->sys->attach[att].host, host);
            }
            if (ipLength > 0) {
                strcpy(etid->sys->attach[att].interface, interface);
            }
          }

          outgoing[0] = htonl((uint32_t)err);
          outgoing[1] = htonl((uint32_t)att);
          if (etNetTcpWrite(connfd, (void *) outgoing, sizeof(outgoing)) != sizeof(outgoing)) {
            goto end;
          }

        }
        break;

        case  ET_NET_STAT_DET:
        {
          et_att_id  att;

          if (etNetTcpRead(connfd, (void *) &att, sizeof(att)) != sizeof(att)) {
            goto end;
          }
          att = ntohl((uint32_t)att);

          err = et_station_detach(id, att);
          /* keep track of all detachments */
          if (err == ET_OK) {
            attaches[att] = -1;
          }

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }

        }
        break;


        case  ET_NET_STAT_CRAT:
        {
          et_stat_config sc;
          et_stat_id     stat_id;
          char stat_name[ET_STATNAME_LENGTH];
          uint32_t incoming[14+ET_STATION_SELECT_INTS], transfer[2];
          int position, pposition, lengthname, lengthfname, lengthlib, lengthclass;

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          sc.init         = ntohl(incoming[0]);
          sc.flow_mode    = ntohl(incoming[1]);
          sc.user_mode    = ntohl(incoming[2]);
          sc.restore_mode = ntohl(incoming[3]);
          sc.block_mode   = ntohl(incoming[4]);
          sc.prescale     = ntohl(incoming[5]);
          sc.cue          = ntohl(incoming[6]);
          sc.select_mode  = ntohl(incoming[7]);
          for (i=0 ; i < ET_STATION_SELECT_INTS ; i++) {
            sc.select[i]  = ntohl(incoming[8+i]);
          }
          lengthfname = ntohl(incoming[8 +ET_STATION_SELECT_INTS]);
          lengthlib   = ntohl(incoming[9 +ET_STATION_SELECT_INTS]);
          lengthclass = ntohl(incoming[10+ET_STATION_SELECT_INTS]);
          lengthname  = ntohl(incoming[11+ET_STATION_SELECT_INTS]);
          position    = ntohl(incoming[12+ET_STATION_SELECT_INTS]);
          pposition   = ntohl(incoming[13+ET_STATION_SELECT_INTS]);

          if (etNetTcpRead(connfd, (void *) sc.fname, lengthfname) != lengthfname) {
            goto end;
          }
          if (etNetTcpRead(connfd, (void *) sc.lib, lengthlib) != lengthlib) {
            goto end;
          }
          if (etNetTcpRead(connfd, (void *) sc.classs, lengthclass) != lengthclass) {
            goto end;
          }
          if (etNetTcpRead(connfd, (void *) stat_name, lengthname) != lengthname) {
            goto end;
          }

          err = et_station_create_at(id, &stat_id, stat_name,
                                    (et_statconfig) &sc, position, pposition);

          transfer[0] = htonl((uint32_t)err);
          transfer[1] = htonl((uint32_t)stat_id);
          if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
        }
        break;


        case  ET_NET_STAT_RM:
        {
          et_stat_id stat_id;

          if (etNetTcpRead(connfd, (void *) &stat_id, sizeof(stat_id)) != sizeof(stat_id)) {
            goto end;
          }
          stat_id = ntohl((uint32_t)stat_id);

          err = et_station_remove(id, stat_id);

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
           goto end;
          }
        }
        break;

        case  ET_NET_STAT_SPOS:
        {
          et_stat_id  stat_id;
          int  position, pposition;
          uint32_t  transfer[3];

          if (etNetTcpRead(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
          stat_id   = ntohl(transfer[0]);
          position  = ntohl(transfer[1]);
          pposition = ntohl(transfer[2]);

          err = et_station_setposition(id, stat_id, position, pposition);

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

        case  ET_NET_STAT_GPOS:
        {
          et_stat_id  stat_id;
          int  position, pposition;
          uint32_t transfer[3];

          if (etNetTcpRead(connfd, (void *) &stat_id, sizeof(stat_id)) != sizeof(stat_id)) {
            goto end;
          }
          stat_id = ntohl((uint32_t)stat_id);

          err = et_station_getposition(id, stat_id, &position, &pposition);

          transfer[0] = htonl((uint32_t)err);
          transfer[1] = htonl((uint32_t)position);
          transfer[2] = htonl((uint32_t)pposition);
          if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
        }
        break;

        case  ET_NET_STAT_ISAT:
        {
          et_stat_id  stat_id;
          et_att_id   att;
          uint32_t  transfer[2];

          if (etNetTcpRead(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
          stat_id = ntohl(transfer[0]);
          att     = ntohl(transfer[1]);

          err = et_station_isattached(id, stat_id, att);

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;

       case  ET_NET_STAT_EX:
        {
          et_stat_id  stat_id;
          char stat_name[ET_STATNAME_LENGTH];
          uint32_t  length, transfer[2];

          if (etNetTcpRead(connfd, (void *) &length, sizeof(length)) != sizeof(length)) {
            goto end;
          }
          length = ntohl(length);
          if (etNetTcpRead(connfd, (void *) stat_name, length) != length) {
            goto end;
          }

          err = et_station_exists(id, &stat_id, stat_name);

          transfer[0] = htonl((uint32_t)err);
          transfer[1] = htonl((uint32_t)stat_id);
          if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            goto end;
          }
        }
        break;

        case  ET_NET_STAT_SSW:
        {
          et_stat_id  stat_id;
          int incoming[1+ET_STATION_SELECT_INTS];

          if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            goto end;
          }

          stat_id = ntohl((uint32_t)incoming[0]);
          for (i=0 ; i < ET_STATION_SELECT_INTS; i++) {
            incoming[i] = ntohl((uint32_t)incoming[1+i]);
          }
          
          err = et_station_setselectwords(id, stat_id, incoming);

          err = htonl((uint32_t)err);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        break;


        case  ET_NET_STAT_GSW:
        {
          et_stat_id  stat_id;
          int sw[1+ET_STATION_SELECT_INTS];

          if (etNetTcpRead(connfd, (void *) &stat_id, sizeof(stat_id)) != sizeof(stat_id)) {
            goto end;
          }
          stat_id = ntohl((uint32_t)stat_id);

          err = et_station_getselectwords(id, stat_id, &sw[1]);

          sw[0] = htonl((uint32_t)err);
          for (i=1; i <= ET_STATION_SELECT_INTS; i++) {
            sw[i] = htonl((uint32_t)sw[i]);
          }
          if (etNetTcpWrite(connfd, (void *) sw, sizeof(sw)) != sizeof(sw)) {
            goto end;
          }
        }
        break;

        case  ET_NET_STAT_LIB:
        case  ET_NET_STAT_FUNC:
        case  ET_NET_STAT_CLASS:
        {
          et_stat_id  stat_id;
          size_t len, size;
          int transfer[2];
          /* make sure buf is larger than class, lib or func name + 2 integers
           * (no matter how big ints may get)
           */
          char *name, buf[128+ET_FILENAME_LENGTH];

          if (etNetTcpRead(connfd, (void *) &stat_id, sizeof(stat_id)) != sizeof(stat_id)) {
            goto end;
          }
          stat_id = ntohl((uint32_t)stat_id);

          name = buf + sizeof(transfer);
          if (command == ET_NET_STAT_LIB) {
            err = et_station_getlib(id, stat_id, name);
          }
          else if (command == ET_NET_STAT_FUNC) {
            err = et_station_getfunction(id, stat_id, name);
          }
          else  {
            err = et_station_getclass(id, stat_id, name);
          }

          if (err != ET_OK) {
            transfer[0] = htonl((uint32_t)err);
            transfer[1] = 0;
            if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
              goto end;
            }
          }
          else {
            len = strlen(name) + 1;
            transfer[0] = htonl((uint32_t)err);
            transfer[1] = htonl((uint32_t)len);
            memcpy(buf, transfer, sizeof(transfer));
            size = sizeof(transfer) + len;
            if (etNetTcpWrite(connfd, (void *) buf, (int)size) != size) {
              goto end;
            }
          }
        }
        break;

        default :
         ;
      } /* switch(command) */
    }   /* if (command < ET_NET_STAT_GATTS) */

    /* the following commands get values associated with stations */
    else if (command < ET_NET_STAT_SBLOCK) {

      et_stat_id  stat_id;
      int  val, transfer[2];

      if (etNetTcpRead(connfd, (void *) &stat_id, sizeof(stat_id)) != sizeof(stat_id)) {
        goto end;
      }
      stat_id = ntohl((uint32_t)stat_id);

      switch (command) {
        case  ET_NET_STAT_GATTS:
          err = et_station_getattachments(id, stat_id, &val);
          break;
        case  ET_NET_STAT_STATUS:
          err = et_station_getstatus(id, stat_id, &val);
          break;
        case  ET_NET_STAT_INCNT:
          err = et_station_getinputcount(id, stat_id, &val);
          break;
        case  ET_NET_STAT_OUTCNT:
          err = et_station_getoutputcount(id, stat_id, &val);
          break;
        case  ET_NET_STAT_GBLOCK:
          err = et_station_getblock(id, stat_id, &val);
          break;
        case  ET_NET_STAT_GUSER:
          err = et_station_getuser(id, stat_id, &val);
          break;
        case  ET_NET_STAT_GRESTORE:
          err = et_station_getrestore(id, stat_id, &val);
          break;
        case  ET_NET_STAT_GPRE:
          err = et_station_getprescale(id, stat_id, &val);
          break;
        case  ET_NET_STAT_GCUE:
          err = et_station_getcue(id, stat_id, &val);
          break;
        case  ET_NET_STAT_GSELECT:
          err = et_station_getselect(id, stat_id, &val);
          break;
        default:
          if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_command_loop: bad command value\n");
          }
          goto end;
      }
      
      transfer[0] = htonl((uint32_t)err);
      transfer[1] = htonl((uint32_t)val);
      if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        goto end;
      }
    }
    
    
    /* the following commands set values associated with stations */
    else if (command < ET_NET_ATT_PUT) {

      et_stat_id  stat_id;
      int  val;
      uint32_t incoming[2];

      if (etNetTcpRead(connfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
        goto end;
      }
      stat_id = ntohl(incoming[0]);
      val     = ntohl(incoming[1]);

      switch (command) {
        case  ET_NET_STAT_SBLOCK:
          err = et_station_setblock(id, stat_id, val);
          break;
        case  ET_NET_STAT_SUSER:
          err = et_station_setuser(id, stat_id, val);
          break;
        case  ET_NET_STAT_SRESTORE:
          err = et_station_setrestore(id, stat_id, val);
          break;
        case  ET_NET_STAT_SPRE:
          err = et_station_setprescale(id, stat_id, val);
          break;
        case  ET_NET_STAT_SCUE:
          err = et_station_setcue(id, stat_id, val);
          break;
        default:
          if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_command_loop: bad command value\n");
          }
          goto end;
      }
      
      err = htonl((uint32_t)err);
      if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        goto end;
      }
    }
    
    
    /* the following commands get values associated with attachments */
    else if (command < ET_NET_SYS_TMP) {
      et_att_id  att_id;
      int        transfer[3];
      uint64_t   llevents;

      if (etNetTcpRead(connfd, (void *) &att_id, sizeof(att_id)) != sizeof(att_id)) {
        goto end;
      }
      att_id = ntohl((uint32_t)att_id);

      switch (command) {
        case  ET_NET_ATT_PUT:
          err = et_attach_geteventsput(id, att_id, &llevents);
          break;
        case  ET_NET_ATT_GET:
          err = et_attach_geteventsget(id, att_id, &llevents);
          break;
        case  ET_NET_ATT_DUMP:
          err = et_attach_geteventsdump(id, att_id, &llevents);
          break;
        case  ET_NET_ATT_MAKE:
          err = et_attach_geteventsmake(id, att_id, &llevents);
          break;
        default:
          if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_command_loop: bad command value\n");
          }
          goto end;
      }
      
      transfer[0] = htonl((uint32_t)err);
      transfer[1] = htonl(ET_HIGHINT(llevents));
      transfer[2] = htonl(ET_LOWINT(llevents));
      if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        goto end;
      }
    }


    /* the following commands get values associated with the system */
    else if (command <= ET_NET_SYS_GRP) {

      int  val, transfer[2];
      pid_t pid; /* int in Linux, long in Solaris */

      switch (command) {
        case  ET_NET_SYS_TMP:
          err = et_system_gettemps(id, &val);
          break;
        case  ET_NET_SYS_TMPMAX:
          err = et_system_gettempsmax(id, &val);
          break;
        case  ET_NET_SYS_STAT:
          err = et_system_getstations(id, &val);
          break;
        case  ET_NET_SYS_STATMAX:
          err = et_system_getstationsmax(id, &val);
          break;
        case  ET_NET_SYS_PROC:
          err = et_system_getprocs(id, &val);
          break;
        case  ET_NET_SYS_PROCMAX:
          err = et_system_getprocsmax(id, &val);
          break;
        case  ET_NET_SYS_ATT:
          err = et_system_getattachments(id, &val);
          break;
        case  ET_NET_SYS_ATTMAX:
          err = et_system_getattsmax(id, &val);
          break;
        case  ET_NET_SYS_HBEAT:
          err = et_system_getheartbeat(id, &val);
          break;
        case  ET_NET_SYS_PID:
          err = et_system_getpid(id, &pid);
          val = pid;
          break;
        case  ET_NET_SYS_GRP:
            {et_id *etidd = (et_id *) id;
            val = etidd->sys->config.groupCount;}
            err = ET_OK;
          break;
        default:
          if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_command_loop: bad command value\n");
          }
          goto end;
      }
      
      transfer[0] = htonl((uint32_t)err);
      transfer[1] = htonl((uint32_t)val);
      if (etNetTcpWrite(connfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        goto end;
      }
    }

    /* the following commands distribute data about this ET system over the network */
    else if (command <= ET_NET_SYS_GRPS) {
      if (command == ET_NET_SYS_DATA) {
        struct iovec iovv[5];
        uint32_t  outgoing[2], size;
        
        err = ET_OK;
        
        if (et_data_sys(etid, &iovv[1]) != ET_OK) {
          err = ET_ERROR;
        }
        else if (et_data_stats(etid, &iovv[2]) != ET_OK) {
          err = ET_ERROR;
          free(iovv[1].iov_base);
        }
        else if (et_data_atts(etid, &iovv[3]) != ET_OK) {
          err = ET_ERROR;
          free(iovv[1].iov_base);
          free(iovv[2].iov_base);
        }
        else if (et_data_procs(etid, &iovv[4]) != ET_OK) {
          err = ET_ERROR;
          free(iovv[1].iov_base);
          free(iovv[2].iov_base);
          free(iovv[3].iov_base);
        }

        if (err != ET_OK) {
          err = htonl((uint32_t)ET_ERROR);
          if (etNetTcpWrite(connfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            goto end;
          }
        }
        else {
          /*
           * The first bit of data we send will be the error followed by the
           * total size - in bytes - of the data that is to follow. This
           * allows the receiver to read that first bit of data and then to
           * allocate a buffer of the size necessary to hold all the real data.
           */
          size = (uint32_t) (iovv[1].iov_len + iovv[2].iov_len + iovv[3].iov_len + iovv[4].iov_len);
          outgoing[0] = htonl(ET_OK);
          outgoing[1] = htonl(size);
          iovv[0].iov_base = (void *) outgoing;
          iovv[0].iov_len  = sizeof(outgoing);

          if (etNetTcpWritev(connfd, iovv, 5, iov_max) == -1) {
            free(iovv[1].iov_base);
            free(iovv[2].iov_base);
            free(iovv[3].iov_base);
            free(iovv[4].iov_base);
            goto end;
          }

          /* free buffers allocated in "et_data_ ..." routines */
          free(iovv[1].iov_base);
          free(iovv[2].iov_base);
          free(iovv[3].iov_base);
          free(iovv[4].iov_base);
        }
      }

      /* send histogram data */
      else if (command == ET_NET_SYS_HIST) {
          err = et_data_gethistogram(id, histogram+1, (int) (nevents_max+1));
          /* sneak error code into histogram array for convenience in writing */
          histogram[0] = err;

          if (err != ET_OK) {
              histogram[0] = htonl((uint32_t)histogram[0]);
              if (etNetTcpWrite(connfd, (void *) histogram, sizeof(int)) != sizeof(int)) {
                  goto end;
              }
          }
          else {
              /* swap data */
              for (i=0; i < nevents_max+2; i++) {
                  histogram[i] = htonl((uint32_t)histogram[i]);
              }
              if (etNetTcpWrite(connfd, (void *) histogram, (int) (sizeof(int)*(nevents_max+2))) !=
                  sizeof(int)*(nevents_max+2)) {
                  goto end;
              }
          }
      }

      /* send group data */
      else if (command == ET_NET_SYS_GRPS) {
          et_id *etidd = (et_id *) id;
          int *groups;
          int groupCount = etidd->sys->config.groupCount;

          /* send number of groups to follow */
          i = htonl((uint32_t)groupCount);
          if (etNetTcpWrite(connfd, (void *) &i, sizeof(int)) != sizeof(int)) {
              goto end;
          }

          /* send number in each group */
          groups = (int *) malloc(groupCount*sizeof(int));
          if (groups == NULL) {
              et_logmsg("ERROR", "et_command_loop: cannot allocate memory\n");
              free(iov); free(header); free(histogram); free(events); free(ints32);
              return;
          }
          for (i=0; i < groupCount; i++) {
              groups[i] = htonl((uint32_t)etidd->sys->config.groups[i]);
          }
          if (etNetTcpWrite(connfd, (void *) groups, (int) (groupCount*sizeof(int))) != groupCount*sizeof(int)) {
              goto end;
          }

          free(groups);          
      }

    }

    else {
      if (etid->debug >= ET_DEBUG_ERROR) {
        et_logmsg("ERROR", "et_command_loop: bad command value\n");
      }
      goto end;
    }

  } /* while(1) */

  /* we only end up down here if there's an error */
  end:
    /*
     * The client has crashed, therefore we must detach all
     * attachments or risked stopping the ET system. The client
     * will not be asking for or processing any more events.
     */
    for (i=0; i <ET_ATTACHMENTS_MAX ; i++) {
      if (attaches[i] > -1) {
        et_station_detach(id, attaches[i]);
      }
    }

    if (etid->debug >= ET_DEBUG_WARN) {
        et_logmsg("WARN", "et_command_loop: remote client connection broken\n");
    }

    free(iov); free(header); free(histogram); free(events); free(ints32);
    return;
}
