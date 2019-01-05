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
 *      Routines for remote ET system users.
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>

#include "et_private.h"
#include "et_network.h"

static int etr_station_getstuff(et_id *id, et_stat_id stat_id, int cmd, int *stuff, const char *routine);
static int etr_system_getstuff(et_id *id, int cmd, int *stuff, const char *routine);


/*****************************************************/
/*                      ET SYSTEM                    */
/*****************************************************/
int etr_open(et_sys_id *id, const char *et_filename, et_openconfig openconfig)
{
    et_open_config *config = (et_open_config *) openconfig;
    int sockfd=-1, version, nselects;
    int err=ET_OK, openerror=ET_OK, transfer[8], incoming[9];
    int port=0, debug;
    uint32_t  length, bufsize, inetaddr = 0;
    et_id *etid;
    char *buf, *pbuf, *ifBroadcastIP=NULL, *ifRegularIP=NULL, ethost[ET_MAXHOSTNAMELEN];
    et_response *response=NULL;

    double          dstart, dnow, dtimeout;
    struct timespec sleeptime;
    struct timeval  start, now;

    /* system id */
    etid = (et_id *) *id;

    debug = config->debug_default;

    strcpy(ethost, config->host);

    /* keep track of starting time */
    gettimeofday(&start, NULL);
    dstart = start.tv_sec + 1.e-6*(start.tv_usec);

    /* 0.5 sec per sleep (2Hz) */
    sleeptime.tv_sec  = 0;
    sleeptime.tv_nsec = 500000000;

    /* set minimum time to wait for connection to ET */
    /* if timeout == 0, wait "forever" */
    if ((config->timeout.tv_sec == 0) && (config->timeout.tv_nsec == 0)) {
        dtimeout = 1.e9; /* 31 years */
    }
    else {
        dtimeout = config->timeout.tv_sec + 1.e-9*(config->timeout.tv_nsec);
    }

    /* if port# & name of ET server specified */
    if (config->cast == ET_DIRECT) {
        port = config->serverport;
        /* Special case is when host is specified as ET_HOST_LOCAL (.local)
         * when the mode is ET_HOST_AS_REMOTE which means a remote/socket connection
         * is made to the local machine. Handle this case by reassigning the
         * host to be 127.0.0.1 - the loopback address. Or it could be changed
         * to "localhost" which usually resolves to the loopback address. */
        if (strcmp(ethost, ET_HOST_LOCAL) == 0) {
            strcpy(ethost, "127.0.0.1");
        }
    }
    
    /*
     * If the "interface" was set in the open config, we don't know if it
     * was set to a regular IP address or a broadcast/subnet address. We need
     * a regular IP address for binding the sending end of the socket we will
     * create here. We also need the broadcast address for sorting the IP
     * addresses we get back from the ET system when calling et_findserver.
     * We can generate both right now.
     */
    if (config->interface != NULL) {
        /* First find an IP address given either an IP or broadcast address. */
        err = codanetGetMatchingLocalIpAddress(config->interface, &ifRegularIP);
        if (err != CODA_OK) {
            ifRegularIP = NULL;
        }
        
        /* Next find a broadcast address given either an IP or broadcast address. */
        err = codanetGetBroadcastAddress(config->interface, &ifBroadcastIP);
        if (err != CODA_OK) {
            ifBroadcastIP = NULL;
        }
    }
        
    while(1) {
        if (port > 0) {
            /* make the network connection */
            if (inetaddr == 0) {
                if (debug >= ET_DEBUG_INFO) {
                    et_logmsg("INFO", "etr_open: try to connect to host %s on port %d through my %s interface\n",
                              ethost, port, ifRegularIP);
                }

                if (etNetTcpConnect(ethost, ifRegularIP, (unsigned short)port,
                                    config->tcpSendBufSize, config->tcpRecvBufSize,
                                    config->tcpNoDelay, &sockfd, NULL) == ET_OK) {
                    if (debug >= ET_DEBUG_INFO) {
                        et_logmsg("INFO", "          success!\n");
                    }
                    break;
                }
            }
            else {
                if (debug >= ET_DEBUG_INFO) {
                    et_logmsg("INFO",
                              "etr_open: try to connect to address %u (host %s) on port %d through my %s interface\n",
                              inetaddr, ethost, port, ifRegularIP);
                }
                if (etNetTcpConnect2(inetaddr, ifRegularIP, (unsigned short)port,
                                     config->tcpSendBufSize, config->tcpRecvBufSize,
                                     config->tcpNoDelay, &sockfd, NULL) == ET_OK) {
                    if (debug >= ET_DEBUG_INFO) {
                        et_logmsg("INFO", "          success!\n");
                    }
                    break;
                }
            }
        }
        else {
            /* else find port# & name of ET server by broad/multicasting */
            if (debug >= ET_DEBUG_INFO) {
                et_logmsg("INFO", "etr_open: calling et_findserver(file=%s, host=%s)\n", et_filename, ethost);
            }
            if ( (openerror = et_findserver(et_filename, ethost, &port, &inetaddr, config, &response, debug)) == ET_OK) {

                struct timeval timeout = {3,0}; /* 3 second timeout for TCP connection */
                int connected = 0;
                codaIpList *dotDecAddr, *first;
                if (response == NULL) {
                    /* use ethost, port */
                    if (debug >= ET_DEBUG_INFO) {
                        et_logmsg("INFO", "          success, but response is NULL!\n");
                    }
                    continue;
                }
         
                /* First order them so the IP addresses on the
                 * same subnets as this client are tried first. */
                first = dotDecAddr = et_orderIpAddrs(response, config->netinfo, ifBroadcastIP);

                if (debug >= ET_DEBUG_INFO) {
                    et_logmsg("INFO", "etr_open: list of ordered IP addresses:\n");
                    while (first != NULL) {
                        printf("%s\n", first->addr);
                        first = first->next;
                    }
                }

                /* Try the IP addresses sent by ET system, one-by-one. */
                while (dotDecAddr != NULL) {
                    if (debug >= ET_DEBUG_INFO) {
                        et_logmsg("INFO", "etr_open: try host %s on port %d through my %s interface\n",
                                  dotDecAddr->addr, port, ifRegularIP);
                    }
                    err = etNetTcpConnectTimeout2(dotDecAddr->addr, ifRegularIP, (unsigned short)port,
                                                  config->tcpSendBufSize, config->tcpRecvBufSize,
                                                  config->tcpNoDelay, &timeout, &sockfd, NULL);
                    if (err == ET_ERROR_TIMEOUT) {
                        if (debug >= ET_DEBUG_INFO) {
                            et_logmsg("INFO", "etr_open: Timed out, try next IP address\n");
                        }
                    }
            
                    if (!connected && err == ET_OK) {
                        if (debug >= ET_DEBUG_INFO) {
                            et_logmsg("INFO", "etr_open: SUCCESS\n");
                        }
                        strcpy(ethost, dotDecAddr->addr);
                        connected = 1;
                        break;
                    }
                    dotDecAddr = dotDecAddr->next;
                }
                
                et_freeAnswers(response);
                codanetFreeAddrList(dotDecAddr);

                if (connected) break;

                /* failure, try again later */
                port = 0;
            }
        }

        /* if user does NOT want to wait ... */
        if (config->wait == ET_OPEN_NOWAIT) {
            break;
        }

        /* if error not due to ET system not being there */
        if (openerror != ET_ERROR_TIMEOUT) {
            break;
        }

        /* see if the min time has elapsed, if so quit. */
        gettimeofday(&now, NULL);
        dnow = now.tv_sec + 1.e-6*(now.tv_usec);
        if (dtimeout < dnow - dstart) {
            break;
        }
        nanosleep(&sleeptime, NULL);
    }
    
    if (ifRegularIP != NULL) {
        free(ifRegularIP);
    }
    
    if (ifBroadcastIP != NULL) {
        free(ifBroadcastIP);
    }
    
    /* check for errors */
    /*printf("etr_open: port %d sockfd = 0x%x\n",port,sockfd);*/
    if ((port == 0) || (sockfd < 0)) {
        if (openerror == ET_ERROR_TOOMANY) {
            if (debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_open: too many ET systems of that name responded\n");
            }
            return ET_ERROR_TOOMANY;
        }
        else {
            if (debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_open: cannot find or connect to ET system\n");
            }
        }
        return ET_ERROR_REMOTE;
    }

    /* find and store local IP address of socket to ET */
    etNetLocalSocketAddress(sockfd, etid->localAddr);
    if (debug >= ET_DEBUG_INFO) {
        et_logmsg("INFO", "etr_open: connection from %s to %s, port# %d\n", etid->localAddr, ethost, port);
    }

    /* Add data to id - locality, socket, port, and host, of the ET system */
    etid->locality = ET_REMOTE;
    etid->sockfd   = sockfd;
    etid->port     = port;
    strcpy(etid->ethost, ethost);

    /* find client's iov_max value */
#if defined __APPLE__
    etid->iov_max = ET_IOV_MAX;
#else
    if ( (etid->iov_max = (int)sysconf(_SC_IOV_MAX)) == -1) {
        /* set it to POSIX minimum by default (it always bombs on Linux) */
        etid->iov_max = ET_IOV_MAX;
    }
#endif

    /* magic numbers */
    transfer[0]  = htonl(ET_MAGIC_INT1);
    transfer[1]  = htonl(ET_MAGIC_INT2);
    transfer[2]  = htonl(ET_MAGIC_INT3);
    
    /* endian */
    transfer[3]  = htonl((uint32_t)etid->endian);

    /* length of ET system name */
    length = (uint32_t) strlen(et_filename) + 1;
    transfer[4] = htonl(length);
#ifdef _LP64
    transfer[5] = 1;
#else
    transfer[5] = 0;
#endif
    /* not used */
    transfer[6] = 0;
    transfer[7] = 0;

    /* put everything in one buffer, extra room in "transfer" */
    bufsize = (uint32_t)sizeof(transfer) + length;
    if ( (pbuf = buf = (char *) malloc(bufsize)) == NULL) {
        if (debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_open, cannot allocate memory\n");
        }
        err = ET_ERROR_REMOTE;
        goto error;
    }
    memcpy(pbuf, transfer, sizeof(transfer));
    pbuf += sizeof(transfer);
    memcpy(pbuf,et_filename, length);

    /* write it to server */
    if (etNetTcpWrite(sockfd, (void *) buf, bufsize) != bufsize) {
        free(buf);
        if (debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_open, write error\n");
        }
        err = ET_ERROR_WRITE;
        goto error;
    }
    free(buf);

    /* read the return */
    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        if (debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_open, read error 1\n");
        }
        err = ET_ERROR_READ;
        goto error;
    }
    err = ntohl((uint32_t)err);
    if (err != ET_OK) {
        if (debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_open: found the wrong ET system\n");
        }
        goto error;
    }

    if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
        if (debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_open, read error 2\n");
        }
        err = ET_ERROR_READ;
        goto error;
    }
    /* ET's endian */
    etid->systemendian = ntohl((uint32_t)incoming[0]);
    /* ET's total number of events */
    etid->nevents = ntohl((uint32_t)incoming[1]);
    /* ET's max event size */
    etid->esize = ET_64BIT_UINT(ntohl((uint32_t)incoming[2]),ntohl((uint32_t)incoming[3]));
    /* ET's version number */
    version = ntohl((uint32_t)incoming[4]);
    /* ET's number of selection integers */
    nselects = ntohl((uint32_t)incoming[5]);
    /* ET's language */
    etid->lang = ntohl((uint32_t)incoming[6]);
    /* ET's 64 or 32 bit exe. Note, because we communicate thru the server
     * and not thru shared mem, 64 & 32 bit programs can speak to each
     * other. It is possible to have a run-time error if a 64 bit app tries
     * to send large data to a 32 bit ET system, or if a 32 bit app tries to
     * read large data from a 64 bit ET system.
     */
    etid->bit64 = ntohl((uint32_t)incoming[7]);

    if (version != etid->version) {
        if (debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_open, ET system & user use different versions of ET\n");
        }
        err = ET_ERROR_REMOTE;
        goto error;
    }

    if (nselects != etid->nselects) {
        if (debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_open, ET system & user compiled with different ET_STATION_SELECT_INTS\n");
        }
        err = ET_ERROR_REMOTE;
        goto error;
    }

/*printf("etr_open: We are OK\n");*/
    return ET_OK;

error:
    close (sockfd);
    return err;
}


/*****************************************************/
int etr_close(et_sys_id id)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int com;

    com = htonl(ET_NET_CLOSE);
    /* if communication with ET system fails, we've already been "closed" */
    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) &com, sizeof(com)) != sizeof(com)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_close, write error\n");
        }
        /* return ET_ERROR_WRITE; */
    }

    close(sockfd);
    et_tcp_unlock(etid);
    et_id_destroy(id);

    return ET_OK;
}

/*****************************************************/
int etr_forcedclose(et_sys_id id)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int com;

    com = htonl(ET_NET_FCLOSE);
    /* if communication with ET system fails, we've already been "closed" */
    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) &com, sizeof(com)) != sizeof(com)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_forcedclose, write error\n");
        }
        /* return ET_ERROR_WRITE; */
    }

    close(sockfd);
    et_tcp_unlock(etid);
    et_id_destroy(id);

    return ET_OK;
}

/*****************************************************/
int etr_kill(et_sys_id id)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int com;

    com = htonl(ET_NET_KILL);
    
    /* if communication with ET system fails, we cannot kill it */
    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) &com, sizeof(com)) != sizeof(com)) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_kill, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    close(sockfd);
    et_tcp_unlock(etid);
    et_id_destroy(id);

    return ET_OK;
}

/*****************************************************/
int etr_alive(et_sys_id id)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int alive, com;

    com = htonl(ET_NET_ALIVE);
    /* If ET system is NOT alive, or if ET system was killed and
     * restarted (breaking tcp connection), we'll get a write error
     * so signal ET system as dead.
     */
    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) &com, sizeof(com)) != sizeof(com)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_alive, write error\n");
        }
        return 0;
    }
    if (etNetTcpRead(sockfd, &alive, sizeof(alive)) != sizeof(alive)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_alive, read error\n");
        }
        return 0;
    }

    et_tcp_unlock(etid);
    return ntohl((uint32_t)alive);
}

/*****************************************************/
/* This function behaves differently for a remote application
 * than for a local one. It is impossible to talk to a system
 * if it isn't yet alive or if it was killed and restarted,
 * since the tcp connection will have been nonexistant or destroyed.
 * Waiting and repeatedly trying any communication will be
 * an exercise in futility.
 */
int etr_wait_for_alive(et_sys_id id)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int returned, com;

    com = htonl(ET_NET_WAIT);
    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) &com, sizeof(com)) != sizeof(com)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_wait_for_alive, write error\n");
        }
        return ET_ERROR_WRITE;
    }
    if (etNetTcpRead(sockfd, &returned, sizeof(returned)) != sizeof(returned)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_wait_for_alive, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);
    /* if communication with ET is successful, it's alive */
    return ET_OK;
}

/*****************************************************/
int etr_wakeup_attachment(et_sys_id id, et_att_id att)
{
    et_id *etid = (et_id *) id;
    int transfer[2], sockfd = etid->sockfd;

    transfer[0] = htonl(ET_NET_WAKE_ATT);
    transfer[1] = htonl(att);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_wakeup_attachment, write error\n");
        }
        return ET_ERROR_WRITE;
    }
    et_tcp_unlock(etid);
    /* always return ET_OK, so don't bother to send over network */
    return ET_OK;
}

/*****************************************************/
int etr_wakeup_all(et_sys_id id, et_stat_id stat_id)
{
    et_id *etid = (et_id *) id;
    int transfer[2], sockfd = etid->sockfd;

    transfer[0] = htonl(ET_NET_WAKE_ALL);
    transfer[1] = htonl((uint32_t)stat_id);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_wakeup_all, write error\n");
        }
        return ET_ERROR_WRITE;
    }
    et_tcp_unlock(etid);
    /* always return ET_OK, so don't bother to send over network */
    return ET_OK;
}

/*****************************************************/
/*                     ET STATION                    */
/*****************************************************/
int etr_station_create_at(et_sys_id id, et_stat_id *stat_id,const char *stat_name,
                          et_statconfig sconfig, int position, int parallelposition)
{
    et_id *etid = (et_id *) id;
    int i, err, sockfd = etid->sockfd;
    uint32_t  bufsize, lengthname, lengthfname, lengthlib, lengthclass;
    uint32_t ints[15+ET_STATION_SELECT_INTS], transfer[2];
    et_stat_config *sc = (et_stat_config *) sconfig;
    char *buf, *pbuf;

    /* command */
    ints[0] = htonl(ET_NET_STAT_CRAT);

    /* station configuration data */
    ints[1] = htonl((uint32_t)sc->init);
    ints[2] = htonl((uint32_t)sc->flow_mode);
    ints[3] = htonl((uint32_t)sc->user_mode);
    ints[4] = htonl((uint32_t)sc->restore_mode);
    ints[5] = htonl((uint32_t)sc->block_mode);
    ints[6] = htonl((uint32_t)sc->prescale);
    ints[7] = htonl((uint32_t)sc->cue);
    ints[8] = htonl((uint32_t)sc->select_mode);
    for (i=0 ; i < ET_STATION_SELECT_INTS ; i++) {
        ints[9+i] = htonl((uint32_t)sc->select[i]);
    }
    lengthfname = (uint32_t)strlen(sc->fname)+1;
    lengthlib   = (uint32_t)strlen(sc->lib)+1;
    lengthclass = (uint32_t)strlen(sc->classs)+1;
    ints[9 +ET_STATION_SELECT_INTS] = htonl(lengthfname);
    ints[10+ET_STATION_SELECT_INTS] = htonl(lengthlib);
    ints[11+ET_STATION_SELECT_INTS] = htonl(lengthclass);

    /* station name and position */
    lengthname = (uint32_t)strlen(stat_name)+1;
    ints[12+ET_STATION_SELECT_INTS] = htonl(lengthname);
    ints[13+ET_STATION_SELECT_INTS] = htonl((uint32_t)position);
    ints[14+ET_STATION_SELECT_INTS] = htonl((uint32_t)parallelposition);

    /* send it all in one buffer */
    bufsize = (uint32_t)sizeof(ints) + lengthname + lengthfname +
              lengthlib  + lengthclass;
    if ( (pbuf = buf = (char *) malloc(bufsize)) == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_create_at, cannot allocate memory\n");
        }
        return ET_ERROR_REMOTE;
    }

    memcpy(pbuf, ints, sizeof(ints));
    pbuf += sizeof(ints);
    /* strings come last */
    memcpy(pbuf, sc->fname, lengthfname);
    pbuf += lengthfname;
    memcpy(pbuf, sc->lib, lengthlib);
    pbuf += lengthlib;
    memcpy(pbuf, sc->classs, lengthclass);
    pbuf += lengthclass;
    memcpy(pbuf, stat_name, lengthname);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) buf, bufsize) != bufsize) {
        et_tcp_unlock(etid);
        free(buf);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_create_at, write error\n");
        }
        return ET_ERROR_WRITE;
    }
    free(buf);

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_create_at, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);
    err = ntohl(transfer[0]);

    if ((err == ET_OK) || (err == ET_ERROR_EXISTS)) {
        *stat_id = ntohl(transfer[1]);
    }

    return err;
}

/*****************************************************/
int etr_station_create(et_sys_id id, et_stat_id *stat_id, const char *stat_name,
                       et_statconfig sconfig)
{
    return etr_station_create_at(id, stat_id, stat_name, sconfig, ET_END, ET_END);
}

/*****************************************************/
int etr_station_remove(et_sys_id id, et_stat_id stat_id)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int err, transfer[2];

    transfer[0] = htonl(ET_NET_STAT_RM);
    transfer[1] = htonl((uint32_t)stat_id);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_remove, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_remove, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);

    return ntohl((uint32_t)err);
}


/******************************************************/
int etr_station_attach(et_sys_id id, et_stat_id stat_id, et_att_id *att)
{
    et_id *etid = (et_id *) id;
    int err, sockfd = etid->sockfd;
    uint32_t length, ipLength, bufsize, incoming[2], transfer[5];
    char *buf, *pbuf, host[ET_MAXHOSTNAMELEN], ip[ET_IPADDRSTRLEN];
    
    /* find name of our host */
    if (etNetLocalHost(host, ET_MAXHOSTNAMELEN) != ET_OK) {
        if (etid->debug >= ET_DEBUG_WARN) {
            et_logmsg("WARN", "etr_station_attach: cannot find hostname\n");
        }
        length = 0;
    }
    else {
        length = (uint32_t)strlen(host) + 1;
    }

    /* find the local socket's ip address */
    if (etNetLocalSocketAddress(sockfd, ip) != ET_OK) {
        if (etid->debug >= ET_DEBUG_WARN) {
            et_logmsg("WARN", "etr_station_attach: cannot find socket ip address\n");
        }
        ipLength = 0;
    }
    else {
        ipLength = (uint32_t)strlen(ip) + 1;
    }

    transfer[0] = htonl(ET_NET_STAT_ATT);
    transfer[1] = htonl((uint32_t)stat_id);
    transfer[2] = htonl((uint32_t)getpid());
    transfer[3] = htonl(length);
    transfer[4] = htonl(ipLength);

    /* send it all in one buffer */
    bufsize = (uint32_t)sizeof(transfer) + length + ipLength;
    if ( (pbuf = buf = (char *) malloc(bufsize)) == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_attach: cannot allocate memory\n");
        }
        return ET_ERROR_REMOTE;
    }
    memcpy(pbuf, transfer, sizeof(transfer));
    pbuf += sizeof(transfer);
    memcpy(pbuf, host, length);
    pbuf += length;
    memcpy(pbuf, ip, ipLength);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) buf, bufsize) != bufsize) {
        et_tcp_unlock(etid);
        free(buf);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_attach, write error\n");
        }
        return ET_ERROR_WRITE;
    }
    free(buf);

    if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_attach, read error\n");

        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);
    err = ntohl(incoming[0]);

    if (err == ET_OK) {
        if (att) {
            *att = ntohl(incoming[1]);
        }
    }

    return err;
}


/******************************************************/
/*  This routine is called by ET system and by users  */
int etr_station_detach(et_sys_id id, et_att_id att)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int err, transfer[2];

    transfer[0] = htonl(ET_NET_STAT_DET);
    transfer[1] = htonl((uint32_t)att);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_detach, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_detach, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);

    return ntohl((uint32_t)err);
}


/*****************************************************/
int etr_station_setposition(et_sys_id id, et_stat_id stat_id, int position,
                            int parallelposition)
{
    et_id *etid = (et_id *) id;
    int err, sockfd = etid->sockfd;
    uint32_t transfer[4];

    transfer[0] = htonl(ET_NET_STAT_SPOS);
    transfer[1] = htonl((uint32_t)stat_id);
    transfer[2] = htonl((uint32_t)position);
    transfer[2] = htonl((uint32_t)parallelposition);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_setposition, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_setposition, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);

    return ntohl((uint32_t)err);
}


/******************************************************/
int etr_station_getposition(et_sys_id id, et_stat_id stat_id, int *position,
                            int *parallelposition)
{
    et_id *etid = (et_id *) id;
    int err, sockfd = etid->sockfd;
    uint32_t transfer[3];

    transfer[0] = htonl(ET_NET_STAT_GPOS);
    transfer[1] = htonl((uint32_t)stat_id);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, 2*sizeof(int)) != 2*sizeof(int)) {
        et_tcp_unlock(id);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getposition, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getposition, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);
    err = ntohl(transfer[0]);

    if (err == ET_OK) {
        if (position) {
            *position = ntohl(transfer[1]);
        }
        if (parallelposition) {
            *parallelposition = ntohl(transfer[2]);
        }
    }

    return err;
}


/******************************************************/
/*                STATION INFORMATION                 */
/******************************************************/
/* is this att attached to this station ? */
int etr_station_isattached(et_sys_id id, et_stat_id stat_id, et_att_id att)
{
    et_id *etid = (et_id *) id;
    int err, sockfd = etid->sockfd;
    uint32_t transfer[3];

    transfer[0] = htonl(ET_NET_STAT_ISAT);
    transfer[1] = htonl((uint32_t)stat_id);
    transfer[2] = htonl((uint32_t)att);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_isattached, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_isattached: read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);

    return ntohl((uint32_t)err);
}

/******************************************************/
int etr_station_exists(et_sys_id id, et_stat_id *stat_id, const char *stat_name)
{
    et_id *etid = (et_id *) id;
    int err, sockfd = etid->sockfd;
    uint32_t com, length, len, bufsize, transfer[2];
    char *buf, *pbuf;

    length = (uint32_t)strlen(stat_name)+1;
    len    = htonl(length);
    com    = htonl(ET_NET_STAT_EX);

    /* send it all in one buffer */
    bufsize = (uint32_t) (sizeof(com) + sizeof(len)) + length;
    if ( (pbuf = buf = (char *) malloc(bufsize)) == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_exists: cannot allocate memory\n");
        }
        return ET_ERROR_REMOTE;
    }
    memcpy(pbuf, &com, sizeof(com));
    pbuf += sizeof(com);
    memcpy(pbuf, &len, sizeof(len));
    pbuf += sizeof(len);
    memcpy(pbuf, stat_name, length);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) buf, bufsize) != bufsize) {
        et_tcp_unlock(etid);
        free(buf);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_exists, write error\n");
        }
        return ET_ERROR_WRITE;
    }
    free(buf);

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_exists, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);
    err = ntohl(transfer[0]);

    if (err == 1) {
        if (stat_id) {
            *stat_id = ntohl(transfer[1]);
        }
    }

    return err;
}

/******************************************************/
int etr_station_setselectwords (et_sys_id id, et_stat_id stat_id, int select[])
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int i, err;
    uint32_t transfer[2+ET_STATION_SELECT_INTS];

    transfer[0] = htonl(ET_NET_STAT_SSW);
    transfer[1] = htonl((uint32_t)stat_id);
    for (i=0; i < ET_STATION_SELECT_INTS; i++) {
        transfer[i+2] = htonl((uint32_t)select[i]);
    }

    et_tcp_lock(id);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(id);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_setselectwords, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        et_tcp_unlock(id);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_setselectwords, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(id);

    return ntohl((uint32_t)err);
}

/******************************************************/
int etr_station_getselectwords(et_sys_id id, et_stat_id stat_id, int select[])
{
    et_id *etid = (et_id *) id;
    int  i, err, sockfd = etid->sockfd;
    uint32_t send[2], transfer[1+ET_STATION_SELECT_INTS];

    send[0] = htonl(ET_NET_STAT_GSW);
    send[1] = htonl((uint32_t)stat_id);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) send, sizeof(send)) != sizeof(send)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getselectwords, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getselectwords, read error\n");
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);
    err = ntohl(transfer[0]);

    if (err == ET_OK) {
        if (select) {
            for (i=0; i<ET_STATION_SELECT_INTS; i++) {
                select[i] = ntohl(transfer[i+1]);
            }
        }
    }

    return err;
}

/******************************************************/
int etr_station_getlib(et_sys_id id, et_stat_id stat_id, char *lib)
{
    et_id *etid = (et_id *) id;
    int err, len, sockfd = etid->sockfd;
    uint32_t transfer[2];
    char libname[ET_FILENAME_LENGTH];

    transfer[0] = htonl(ET_NET_STAT_LIB);
    transfer[1] = htonl((uint32_t)stat_id);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getlib, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getlib, read error\n");
        }
        return ET_ERROR_READ;
    }
    err = ntohl(transfer[0]);

    if (err == ET_OK) {
        len = ntohl(transfer[1]);
        if (etNetTcpRead(sockfd, (void *) libname, len) != len) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_station_getlib, read error\n");
            }
            return ET_ERROR_READ;
        }
        if (lib) {
            strcpy(lib, libname);
        }
    }
    et_tcp_unlock(etid);

    return err;
}

/******************************************************/
int etr_station_getclass(et_sys_id id, et_stat_id stat_id, char *classs)
{
    et_id *etid = (et_id *) id;
    int err, len, sockfd = etid->sockfd;
    uint32_t transfer[2];
    char classname[ET_FILENAME_LENGTH];

    transfer[0] = htonl(ET_NET_STAT_CLASS);
    transfer[1] = htonl((uint32_t)stat_id);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getclass, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getclass, read error\n");
        }
        return ET_ERROR_READ;
    }
    err = ntohl(transfer[0]);

    if (err == ET_OK) {
        len = ntohl(transfer[1]);
        if (etNetTcpRead(sockfd, (void *) classname, len) != len) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_station_getclass, read error\n");
            }
            return ET_ERROR_READ;
        }
        if (classs) {
            strcpy(classs, classname);
        }
    }
    et_tcp_unlock(etid);

    return err;
}

/******************************************************/
int etr_station_getfunction(et_sys_id id, et_stat_id stat_id, char *function)
{
    et_id *etid = (et_id *) id;
    int err, len, sockfd = etid->sockfd;
    uint32_t transfer[2];
    char fname[ET_FUNCNAME_LENGTH];

    transfer[0] = htonl(ET_NET_STAT_FUNC);
    transfer[1] = htonl((uint32_t)stat_id);

    et_tcp_lock(etid);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getfunction, write error\n");
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_station_getfunction, read error\n");
        }
        return ET_ERROR_READ;
    }
    err = ntohl(transfer[0]);

    if (err == ET_OK) {
        len = ntohl(transfer[1]);
        if (etNetTcpRead(sockfd, (void *) fname, len) != len) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_station_getfunction, read error\n");
            }
            return ET_ERROR_READ;
        }
        if (function) {
            strcpy(function, fname);
        }
    }
    et_tcp_unlock(etid);

    return err;
}


/*****************************************************
 * This gets "int" station parameter data
 *****************************************************/
static int etr_station_getstuff (et_id *id, et_stat_id stat_id, int cmd,
                                 int *stuff, const char *routine)
{
    int err, sockfd = id->sockfd;
    uint32_t transfer[2];

    transfer[0] = htonl((uint32_t)cmd);
    transfer[1] = htonl((uint32_t)stat_id);

    et_tcp_lock(id);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(id);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "%s, write error\n", routine);
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(id);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "%s, read error\n", routine);
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(id);
    err = ntohl(transfer[0]);

    if (err == ET_OK) {
        if (stuff) {
            *stuff = ntohl(transfer[1]);
        }
    }

    return err;
}

/******************************************************/
int etr_station_getattachments(et_sys_id id, et_stat_id stat_id, int *numatts)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_GATTS, numatts, "etr_station_getattachments");
    return err;
}

/******************************************************/
int etr_station_getstatus(et_sys_id id, et_stat_id stat_id, int *status)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_STATUS, status, "etr_station_getstatus");
    return err;
}

/*****************************************************/
int etr_station_getinputcount (et_sys_id id, et_stat_id stat_id, int *cnt)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_INCNT, cnt, "etr_station_getinputcount");
    return err;
}

/*****************************************************/
int etr_station_getoutputcount (et_sys_id id, et_stat_id stat_id, int *cnt)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_OUTCNT, cnt, "etr_station_getoutputcount");
    return err;
}

/*****************************************************/
int etr_station_getblock (et_sys_id id, et_stat_id stat_id, int *block)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_GBLOCK, block, "etr_station_getblock");
    return err;
}

/*****************************************************/
int etr_station_getuser (et_sys_id id, et_stat_id stat_id, int *user)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_GUSER, user, "etr_station_getuser");
    return err;
}

/*****************************************************/
int etr_station_getrestore (et_sys_id id, et_stat_id stat_id, int *restore)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_GRESTORE, restore, "etr_station_getrestore");
    return err;
}

/*****************************************************/
int etr_station_getselect (et_sys_id id, et_stat_id stat_id, int *select)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_GSELECT, select, "etr_station_getselect");
    return err;
}

/*****************************************************/
int etr_station_getcue (et_sys_id id, et_stat_id stat_id, int *cue)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_GCUE, cue, "etr_station_getcue");
    return err;
}

/*****************************************************/
int etr_station_getprescale (et_sys_id id, et_stat_id stat_id, int *prescale)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_station_getstuff(etid, stat_id, ET_NET_STAT_GPRE, prescale, "etr_station_getprescale");
    return err;
}

/*****************************************************
 * This sets "int" station parameter data
 *****************************************************/
static int etr_station_setstuff (et_id *id, et_stat_id stat_id, int cmd,
                                 int value, const char *routine)
{
    int err, sockfd = id->sockfd;
    uint32_t transfer[3];

    transfer[0] = htonl((uint32_t)cmd);
    transfer[1] = htonl((uint32_t)stat_id);
    transfer[2] = htonl((uint32_t)value);

    et_tcp_lock(id);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(id);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "%s, write error\n", routine);
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
        et_tcp_unlock(id);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "%s, read error\n", routine);
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(id);

    return ntohl((uint32_t)err);
}

/******************************************************/
int etr_station_setblock(et_sys_id id, et_stat_id stat_id, int block)
{
    et_id *etid = (et_id *) id;
    return etr_station_setstuff(etid, stat_id, ET_NET_STAT_SBLOCK,
                                block, "etr_station_setblock");
}

/******************************************************/
int etr_station_setuser(et_sys_id id, et_stat_id stat_id, int user)
{
    et_id *etid = (et_id *) id;
    return etr_station_setstuff(etid, stat_id, ET_NET_STAT_SUSER,
                                user, "etr_station_setuser");
}

/******************************************************/
int etr_station_setrestore(et_sys_id id, et_stat_id stat_id, int restore)
{
    et_id *etid = (et_id *) id;
    return etr_station_setstuff(etid, stat_id, ET_NET_STAT_SRESTORE,
                                restore, "etr_station_setrestore");
}

/******************************************************/
int etr_station_setprescale(et_sys_id id, et_stat_id stat_id, int prescale)
{
    et_id *etid = (et_id *) id;
    return etr_station_setstuff(etid, stat_id, ET_NET_STAT_SPRE,
                                prescale, "etr_station_setprescale");
}

/******************************************************/
int etr_station_setcue(et_sys_id id, et_stat_id stat_id, int cue)
{
    et_id *etid = (et_id *) id;
    return etr_station_setstuff(etid, stat_id, ET_NET_STAT_SCUE,
                                cue, "etr_station_setcue");
}


/*****************************************************
 * This gets "int" system parameter data
 *****************************************************/
static int etr_system_getstuff (et_id *id, int cmd, int *stuff, const char *routine)
{
    int err, sockfd = id->sockfd;
    uint32_t transfer[2];

    transfer[0] = htonl((uint32_t)cmd);

    et_tcp_lock(id);
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(int)) != sizeof(int)) {
        et_tcp_unlock(id);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "%s, write error\n", routine);
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(id);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "%s, read error\n", routine);
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(id);
    err = ntohl(transfer[0]);

    if (err == ET_OK) {
        if (stuff) {
            *stuff = ntohl(transfer[1]);
        }
    }

    return err;
}

/*****************************************************/
int etr_system_gettemps (et_sys_id id, int *temps)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_TMP, temps, "etr_system_gettemps");
    return err;
}

/*****************************************************/
int etr_system_gettempsmax (et_sys_id id, int *tempsmax)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_TMPMAX, tempsmax, "etr_system_gettempsmax");
    return err;
}

/*****************************************************/
int etr_system_getstations (et_sys_id id, int *stations)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_STAT, stations, "etr_system_getstations");
    return err;
}

/*****************************************************/
int etr_system_getstationsmax (et_sys_id id, int *stationsmax)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_STATMAX, stationsmax, "etr_system_getstationsmax");
    return err;
}

/*****************************************************/
int etr_system_getprocs (et_sys_id id, int *procs)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_PROC, procs, "etr_system_getprocs");
    return err;
}

/*****************************************************/
int etr_system_getprocsmax (et_sys_id id, int *procsmax)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_PROCMAX, procsmax, "etr_system_getprocsmax");
    return err;
}

/*****************************************************/
int etr_system_getattachments (et_sys_id id, int *atts)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_ATT, atts, "etr_system_getattachments");
    return err;
}

/*****************************************************/
int etr_system_getattsmax (et_sys_id id, int *attsmax)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_ATTMAX, attsmax, "etr_system_getattsmax");
    return err;
}

/*****************************************************/
int etr_system_getheartbeat (et_sys_id id, int *heartbeat)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_HBEAT, heartbeat, "etr_system_getheartbeat");
    return err;
}

/*****************************************************/
int etr_system_getpid (et_sys_id id, int *pid)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_PID, pid, "etr_system_getpid");
    return err;
}

/*****************************************************/
int etr_system_getgroupcount (et_sys_id id, int *groupCnt)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_system_getstuff(etid, ET_NET_SYS_GRP, groupCnt, "etr_system_getgroupcount");
    return err;
}

/*****************************************************
 * This gets 64 bit integer attachment data
 *****************************************************/
static int etr_attach_getstuff (et_id *id, et_att_id att_id, int cmd,
                                uint64_t *stuff,
                                const char *routine)
{
    int err, sockfd = id->sockfd;
    uint32_t transfer[3];

    transfer[0] = htonl((uint32_t)cmd);
    transfer[1] = htonl((uint32_t)att_id);

    et_tcp_lock(id);
    if (etNetTcpWrite(sockfd, (void *) transfer, 2*sizeof(int)) != 2*sizeof(int)) {
        et_tcp_unlock(id);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "%s, write error\n", routine);
        }
        return ET_ERROR_WRITE;
    }

    if (etNetTcpRead(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
        et_tcp_unlock(id);
        if (id->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "%s, read error\n", routine);
        }
        return ET_ERROR_READ;
    }
    et_tcp_unlock(id);
    err = ntohl(transfer[0]);

    if (err == ET_OK) {
        if (stuff != NULL) {
            *stuff = ET_64BIT_UINT(ntohl(transfer[1]),ntohl(transfer[2]));
        }
    }

    return err;
}

/*****************************************************/
int etr_attach_geteventsput(et_sys_id id, et_att_id att_id,
                            uint64_t *events)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_attach_getstuff(etid, att_id, ET_NET_ATT_PUT,
                              events, "etr_attach_geteventsput");
    return err;
}

/*****************************************************/
int etr_attach_geteventsget(et_sys_id id, et_att_id att_id,
                            uint64_t *events)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_attach_getstuff(etid, att_id, ET_NET_ATT_GET,
                              events, "etr_attach_geteventsget");
    return err;
}

/*****************************************************/
int etr_attach_geteventsdump(et_sys_id id, et_att_id att_id,
                             uint64_t *events)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_attach_getstuff(etid, att_id, ET_NET_ATT_DUMP,
                              events, "etr_attach_geteventsdump");
    return err;
}

/*****************************************************/
int etr_attach_geteventsmake(et_sys_id id, et_att_id att_id,
                             uint64_t *events)
{
    int err;
    et_id *etid = (et_id *) id;

    err = etr_attach_getstuff(etid, att_id, ET_NET_ATT_MAKE,
                              events, "etr_attach_geteventsmake");
    return err;
}


/******************************************************/
/*                   ET EVENT STUFF                   */
/******************************************************/

/**
 * @addtogroup eventFields Single event
 * @{
 */


/**
 * This routine set an event's data buffer for a remote user.
 *
 * This routine is a little different than other et_event_get/set routines as it must only
 * be used in a remote application. Use this in conjunction with the mode flag of @ref ET_NOALLOC
 * in @ref et_events_new. This allows the user to avoid allocating event data memory, but instead
 * to supply a buffer. This buffer is given by the "data" argument of this routine.
 * If the user does a "put" of an event having called this routine to set its buffer,
 * that user-supplied buffer is not freed.
 *
 * @param id
 * @param pe
 * @parem data
 *
 * @returns @ref ET_OK      if successful.
 * @returns @ref ET_ERROR   if user is not remote or data arg is NULL.
 */
int et_event_setdatabuffer(et_sys_id id, et_event *pe, void *data) {

    et_id *etid = (et_id *) id;

    if (etid->locality != ET_REMOTE) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_setdatabuffer, user must be remote to use this routine\n");
        }
        return ET_ERROR;
    }
    if (data == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "et_event_setdatabuffer, data argument cannot be null\n");
        }
        return ET_ERROR;
    }
    pe->pdata = data;
    return ET_OK;
}

/** @} */

/******************************************************/
int etr_event_new(et_sys_id id, et_att_id att, et_event **ev,
                  int mode, struct timespec *deltatime, size_t size)
{
    int      err, wait, netWait, noalloc, place, delay=0;
    uint32_t transfer[7], incoming[3];
    size_t   eventsize;
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    et_event *newevent;
    
    /* How many times we call etr_event_new with an adjusted deltatime value */
    int iterations = 1;
    /* deltatime in microseconds */
    long microSec = 0L;
    /* Duration of each etr_event_new in microsec (.2 sec) */
    int newTimeInterval = 200000;
    /* Time to wait outside mutex locked code for each etr_event_new */
    struct timespec waitTime = {0, 10000000}; /* .01 sec */
    /* Holds deltatime in sleep mode and when it needs changing in timed mode */
    struct timespec newDeltaTime = {0, 0};

    /* Pick out wait & no-allocate parts of mode.
     * Value of wait is checked in et_event_new. */
    netWait = wait = mode & ET_WAIT_MASK;

    if (wait == ET_TIMED) {
        /* timespec to int */
        microSec = deltatime->tv_sec*1000000 + deltatime->tv_nsec/1000;
    }

    /* When using the network, do NOT use SLEEP mode because that
     * may block all usage of this API's network communication methods.
     * Use repeated calls in TIMED mode. In between those calls,
     * allow other mutex-grabbing code to run (such as wakeUpAll). */
    if (wait == ET_SLEEP) {
        netWait = ET_TIMED;
        newDeltaTime.tv_sec = 0;
        newDeltaTime.tv_nsec = 200000000;  /* 0.2 sec timeout */
        deltatime = &newDeltaTime;
    }
    /* Also, if there is a long time designated for TIMED mode,
     * break it up into repeated smaller time chunks for the
     * reason mentioned above. Don't break it up if timeout <= 1 sec. */
    else if (wait == ET_TIMED && (microSec > 1000000))  {
        /* Newly set duration of each get */
        newDeltaTime.tv_sec  =  newTimeInterval/1000000;
        newDeltaTime.tv_nsec = (newTimeInterval - newDeltaTime.tv_sec*1000000) * 1000;
        deltatime = &newDeltaTime;
        /* How many times do we call etr_event_new with this new timeout value?
         * It will be an over estimate unless timeout is evenly divisible by .2 seconds. */
        iterations = (int) microSec/newTimeInterval;
        if (microSec % newTimeInterval > 0) iterations++;
    }

    /* Do not allocate memory. Use buffer to be designated later. */
    noalloc = mode & ET_NOALLOC;

    transfer[0] = htonl(ET_NET_EV_NEW);
    transfer[1] = htonl((uint32_t)att);
    transfer[2] = htonl((uint32_t)netWait);
    transfer[3] = htonl(ET_HIGHINT(size));
    transfer[4] = htonl(ET_LOWINT(size));
    transfer[5] = 0;
    transfer[6] = 0;
    if (deltatime) {
        transfer[5] = htonl((uint32_t)deltatime->tv_sec);
        transfer[6] = htonl((uint32_t)deltatime->tv_nsec);
    }


    while (1) {
        /* Give other routines that write over the network a chance to run */
        if (delay) nanosleep(&waitTime, NULL);

        et_tcp_lock(etid);
        if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_new, write error\n");
            }
            return ET_ERROR_WRITE;
        }

        /* get back a place for later use */
        if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_new, read error\n");
            }
            return ET_ERROR_READ;
        }
        et_tcp_unlock(etid);
        
        err   = ntohl(incoming[0]);
        place = ntohl(incoming[1]); /* incoming[2] not used */
        
        if (err == ET_ERROR_TIMEOUT) {
            /* Only get here if using SLEEP or TIMED modes */
            if (wait == ET_SLEEP || iterations-- > 0) {
                /* Give other routines that write over the network a chance to run */
                delay = 1;
                continue;
            }
            return err;
        }
        else if (err != ET_OK) {
            return err;
        }

        break;
    }
    
    /* allocate memory for an event */
    if ((newevent = (et_event *) malloc(sizeof(et_event))) == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_event_new, cannot allocate memory\n");
        }
        return ET_ERROR_REMOTE;
    }

    /* initialize it */
    et_init_event(newevent);

    /* Allocate memory for event data. The default event size, even for
     * the remote client, is the standard size defined by the ET system.
     * If the remote client is interested in a larger size, then that
     * request is granted.In this manner, the remote user
     * asking for an event has at least the amount of memory to work
     * with that was originially specified on the ET system. The remote
     * client behaves just as a local one does.
     *
     * There's a problem if the ET sys is 64 bit and this app is 32 bit.
     * If the events on the ET sys are enormous, make sure we don't try
     * to allocate more than a reasonable size. In this case, only
     * allocate what was asked for.
     */
#ifndef _LP64

    if (etid->bit64) {
        /* if event size > ~1G, only allocate what's asked for */
        if (etid->esize > UINT32_MAX/5) {
            eventsize = size;
        }
        else {
            eventsize = (size_t)etid->esize;
        }
    }
    else {
        eventsize = (size_t)etid->esize;
    }
#else
    eventsize = (size_t)etid->esize;
#endif

    if (size > etid->esize) {
        eventsize = size;
        newevent->temp = ET_EVENT_TEMP;
    }

    /* if allocating memory as is normally the case ... */
    if (noalloc == 0) {
        if ((newevent->pdata = malloc(eventsize)) == NULL) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_new, cannot allocate memory\n");
            }
            free(newevent);
            return ET_ERROR_REMOTE;
        }
    }
    /* else if user supplying a buffer ... */
    else {
        /* Take an element of the et_event structure that is unused
         * in a remote setting and use it to record the fact that
         * the user is supplying the data buffer. This way when a
         * "put" is done, ET will not try to free the memory.
         */
        newevent->owner = ET_NOALLOC;
    }
    newevent->length  = size;
    newevent->memsize = eventsize;
    newevent->place   = place;
    newevent->modify  = ET_MODIFY;

    *ev = newevent;

    return ET_OK;
}

/******************************************************/
int etr_events_new(et_sys_id id, et_att_id att, et_event *evs[],
                   int mode, struct timespec *deltatime,
                   size_t size, int num, int *nread)
{
    int i, j, err, temp, nevents, wait, netWait, delay=0, noalloc;
    uint32_t transfer[8];
    size_t eventsize;
    uint32_t *places;
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    et_event **newevents;

    /* How many times we call et_events_new with an adjusted deltatime value */
    int iterations = 1;
    /* deltatime in microseconds */
    long microSec = 0L;
    /* Duration of each et_events_new in microsec (.2 sec) */
    int newTimeInterval = 200000;
    /* Time to wait outside mutex locked code for each et_events_new */
    struct timespec waitTime = {0, 10000000}; /* .01 sec */
    /* Holds deltatime in sleep mode and when it needs changing in timed mode */
    struct timespec newDeltaTime = {0, 0};
    
    
    /* Allocate array of event pointers - store new events here
     * until copied to evs[] when all danger of error is past.
     */
    if ((newevents = (et_event **) calloc(num, sizeof(et_event *))) == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_events_new, cannot allocate memory\n");
        }
        return ET_ERROR_REMOTE;
    }

    /* Pick out wait & no-allocate parts of mode.
     * Value of wait is checked in et_events_new. */
    netWait = wait = mode & ET_WAIT_MASK;

    if (wait == ET_TIMED) {
        /* timespec to int */
        microSec = deltatime->tv_sec*1000000 + deltatime->tv_nsec/1000;
    }

    /* When using the network, do NOT use SLEEP mode because that
     * may block all usage of this API's network communication methods.
     * Use repeated calls in TIMED mode. In between those calls,
     * allow other mutex-grabbing code to run (such as wakeUpAll). */
    if (wait == ET_SLEEP) {
        netWait = ET_TIMED;
        newDeltaTime.tv_sec = 0;
        newDeltaTime.tv_nsec = 200000000;  /* 0.2 sec timeout */
        deltatime = &newDeltaTime;
    }
    /* Also, if there is a long time designated for TIMED mode,
     * break it up into repeated smaller time chunks for the
     * reason mentioned above. Don't break it up if timeout <= 1 sec. */
    else if (wait == ET_TIMED && (microSec > 1000000))  {
        /* Newly set duration of each new */
        newDeltaTime.tv_sec  =  newTimeInterval/1000000;
        newDeltaTime.tv_nsec = (newTimeInterval - newDeltaTime.tv_sec*1000000) * 1000;
        deltatime = &newDeltaTime;
        /* How many times do we call et_events_new with this new timeout value?
        * It will be an over estimate unless timeout is evenly divisible by .2 seconds. */
        iterations = (int)microSec/newTimeInterval;
        if (microSec % newTimeInterval > 0) iterations++;
    }
    
    /* Do not allocate memory. Use buffer to be designated later. */
    noalloc = mode & ET_NOALLOC;

    transfer[0] = htonl(ET_NET_EVS_NEW);
    transfer[1] = htonl((uint32_t)att);
    transfer[2] = htonl((uint32_t)netWait);
    transfer[3] = htonl(ET_HIGHINT(size));
    transfer[4] = htonl(ET_LOWINT(size));
    transfer[5] = htonl((uint32_t)num);
    transfer[6] = 0;
    transfer[7] = 0;

    if (deltatime) {
        transfer[6] = htonl((uint32_t)deltatime->tv_sec);
        transfer[7] = htonl((uint32_t)deltatime->tv_nsec);
    }

    while (1) {
        /* Give other routines that write over the network a chance to run */
        if (delay) nanosleep(&waitTime, NULL);
        
        et_tcp_lock(etid);
        if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_new, write error\n");
            }
            free(newevents);
            return ET_ERROR_WRITE;
        }
        /*printf("etr_events_new: sent transfer array, will read err\n");*/
    
        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_new, read error\n");
            }
            free(newevents);
            return ET_ERROR_READ;
        }
    
        err = ntohl((uint32_t)err);
        
        if (err == ET_ERROR_TIMEOUT) {
            /* Only get here if using SLEEP or TIMED modes */
            et_tcp_unlock(etid);
            if (wait == ET_SLEEP || iterations-- > 0) {
                /* Give other routines that write over the network a chance to run */
                delay = 1;
                continue;
            }
            free(newevents);
            return err;
        }
        else if (err < 0) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_new, error in server\n");
            }
            free(newevents);
            return err;
        }

        break;
    }
    
    /* number of events to expect */
    nevents = err;

    /* allocate memory for event places */
    if ((places = (uint32_t *) calloc((size_t) nevents, sizeof(uint32_t))) == NULL) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_events_new, cannot allocate memory\n");
        }
        free(newevents);
        return ET_ERROR_REMOTE;
    }

    /* read array of event pointers */
    if (etNetTcpRead(sockfd, (void *) places, (int) (nevents*sizeof(uint32_t))) !=
            nevents*sizeof(uint32_t) ) {

        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_events_new, read error\n");
        }
        free(places);
        free(newevents);
        return ET_ERROR_READ;
    }
    
    et_tcp_unlock(etid);

#ifndef _LP64
    if (etid->bit64) {
        /* if events size > ~1G, only allocate what's asked for */
        if (num*etid->esize > UINT32_MAX/5) {
            eventsize = size;
        }
        else {
            eventsize = (size_t)etid->esize;
        }
    }
    else {
        eventsize = (size_t)etid->esize;
    }
#else
    eventsize = (size_t)etid->esize;
#endif

    /* set size of an event's data memory */
    temp = ET_EVENT_NORMAL;
    if (size > etid->esize) {
        eventsize = size;
        temp = ET_EVENT_TEMP;
    }

    for (i=0; i < nevents; i++) {
        /* allocate memory for event */
        if ((newevents[i] = (et_event *) malloc(sizeof(et_event))) == NULL) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_new, cannot allocate memory\n");
            }
            err = ET_ERROR_REMOTE;
            break;
        }
        /* initialize new event */
        et_init_event(newevents[i]);

        /* if allocating memory for event data as is normally the case ... */
        if (noalloc == 0) {
            if ((newevents[i]->pdata = malloc(eventsize)) == NULL) {
                if (etid->debug >= ET_DEBUG_ERROR) {
                    et_logmsg("ERROR", "etr_events_new, cannot allocate memory\n");
                }
                free(newevents[i]);
                err = ET_ERROR_REMOTE;
                break;
            }
        }
        /* else if user supplying a buffer ... */
        else {
            /* Take an element of the et_event structure that is unused
             * in a remote setting and use it to record the fact that
             * the user is supplying the data buffer. This way when a
             * "put" is done, ET will not try to free the memory.
             */
            newevents[i]->owner = ET_NOALLOC;
        }
        newevents[i]->length  = size;
        newevents[i]->memsize = eventsize;
        newevents[i]->modify  = ET_MODIFY;
        newevents[i]->place   = ntohl(places[i]);
        newevents[i]->temp    = temp;
    }

    /* if error in above for loop ... */
    if (err < 0) {
        /* free up all allocated memory */
        for (j=0; j < i; j++) {
            if (noalloc == 0) {
                free(newevents[j]->pdata);
            }
            free(newevents[j]);
        }
        free(places);
        free(newevents);
        return err;
    }

    /* now that all is OK, copy into user's array of event pointers */
    for (i=0; i < nevents; i++) {
        evs[i] = newevents[i];
    }
    if (nread != NULL) {
        *nread = nevents;
    }
    free(places);
    free(newevents);

    return ET_OK;
}

/******************************************************/
int etr_events_new_group(et_sys_id id, et_att_id att, et_event *evs[],
                   int mode, struct timespec *deltatime,
                   size_t size, int num, int group, int *nread)
{
    int i, j, err, temp, nevents, wait, netWait, delay=0, noalloc;
    size_t eventsize;
    uint32_t  transfer[9], *places;
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    et_event **newevents;

    /* How many times we call et_events_new_grp with an adjusted deltatime value */
    int iterations = 1;
    /* deltatime in microseconds */
    long microSec = 0L;
    /* Duration of each et_events_new_grp in microsec (.2 sec) */
    int newTimeInterval = 200000;
    /* Time to wait outside mutex locked code for each et_events_new_grp */
    struct timespec waitTime = {0, 10000000}; /* .01 sec */
    /* Holds deltatime in sleep mode and when it needs changing in timed mode */
    struct timespec newDeltaTime = {0, 0};
    
    
    /* Allocate array of event pointers - store new events here
     * until copied to evs[] when all danger of error is past.
     */
    if ((newevents = (et_event **) calloc((size_t) num, sizeof(et_event *))) == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_events_new_group, cannot allocate memory\n");
        }
        return ET_ERROR_REMOTE;
    }

    /* Pick out wait & no-allocate parts of mode.
     * Value of wait is checked in et_events_new_group. */
    netWait = wait = mode & ET_WAIT_MASK;

    if (wait == ET_TIMED) {
        /* timespec to int */
        microSec = deltatime->tv_sec*1000000 + deltatime->tv_nsec/1000;
    }

    /* When using the network, do NOT use SLEEP mode because that
     * may block all usage of this API's network communication methods.
     * Use repeated calls in TIMED mode. In between those calls,
     * allow other mutex-grabbing code to run (such as wakeUpAll). */
    if (wait == ET_SLEEP) {
        netWait = ET_TIMED;
        newDeltaTime.tv_sec = 0;
        newDeltaTime.tv_nsec = 200000000;  /* 0.2 sec timeout */
        deltatime = &newDeltaTime;
    }
    /* Also, if there is a long time designated for TIMED mode,
     * break it up into repeated smaller time chunks for the
     * reason mentioned above. Don't break it up if timeout <= 1 sec. */
    else if (wait == ET_TIMED && (microSec > 1000000))  {
        /* Newly set duration of each new */
        newDeltaTime.tv_sec  =  newTimeInterval/1000000;
        newDeltaTime.tv_nsec = (newTimeInterval - newDeltaTime.tv_sec*1000000) * 1000;
        deltatime = &newDeltaTime;
        /* How many times do we call et_events_new_grp with this new timeout value?
        * It will be an over estimate unless timeout is evenly divisible by .2 seconds. */
        iterations = (int)microSec/newTimeInterval;
        if (microSec % newTimeInterval > 0) iterations++;
    }

    /* Do not allocate memory. Use buffer to be designated later. */
    noalloc = mode & ET_NOALLOC;

    transfer[0] = htonl(ET_NET_EVS_NEW_GRP);
    transfer[1] = htonl((uint32_t)att);
    transfer[2] = htonl((uint32_t)netWait);
    transfer[3] = htonl(ET_HIGHINT(size));
    transfer[4] = htonl(ET_LOWINT(size));
    transfer[5] = htonl((uint32_t)num);
    transfer[6] = htonl((uint32_t)group);
    transfer[7] = 0;
    transfer[8] = 0;

    if (deltatime) {
        transfer[7] = htonl((uint32_t)deltatime->tv_sec);
        transfer[8] = htonl((uint32_t)deltatime->tv_nsec);
    }

    while (1) {
        /* Give other routines that write over the network a chance to run */
        if (delay) nanosleep(&waitTime, NULL);

        et_tcp_lock(etid);
        if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_new_group, write error\n");
            }
            free(newevents);
            return ET_ERROR_WRITE;
        }

        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_new_group, read error\n");
            }
            free(newevents);
            return ET_ERROR_READ;
        }
    
        err = ntohl((uint32_t)err);

        if (err == ET_ERROR_TIMEOUT) {
            /* Only get here if using SLEEP or TIMED modes */
            et_tcp_unlock(etid);
            if (wait == ET_SLEEP || iterations-- > 0) {
                /* Give other routines that write over the network a chance to run */
                delay = 1;
                continue;
            }
            free(newevents);
            return err;
        }
        else if (err < 0) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_new_group, error in server\n");
            }
            free(newevents);
            return err;
        }
        
        break;
    }

    /* number of events to expect */
    nevents = err;

    /* allocate memory for event pointers */
    if ((places = (uint32_t *) calloc((size_t) nevents, sizeof(uint32_t))) == NULL) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_events_new_group, cannot allocate memory\n");
        }
        free(newevents);
        return ET_ERROR_REMOTE;
    }

    /* read array of event pointers */
    if (etNetTcpRead(sockfd, (void *) places, (int) (nevents*sizeof(uint32_t))) !=
            nevents*sizeof(uint32_t) ) {

        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_events_new_group, read error\n");
        }
        free(places);
        free(newevents);
        return ET_ERROR_READ;
    }
    et_tcp_unlock(etid);

#ifndef _LP64
    if (etid->bit64) {
        /* if events size > ~1G, only allocate what's asked for */
        if (num*etid->esize > UINT32_MAX/5) {
            eventsize = size;
        }
        else {
            eventsize = (size_t)etid->esize;
        }
    }
    else {
        eventsize = (size_t)etid->esize;
    }
#else
    eventsize = (size_t)etid->esize;
#endif

    /* set size of an event's data memory */
    temp = ET_EVENT_NORMAL;
    if (size > etid->esize) {
        eventsize = size;
        temp = ET_EVENT_TEMP;
    }

    for (i=0; i < nevents; i++) {
        /* allocate memory for event */
        if ((newevents[i] = (et_event *) malloc(sizeof(et_event))) == NULL) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_new_group, cannot allocate memory\n");
            }
            err = ET_ERROR_REMOTE;
            break;
        }
        /* initialize new event */
        et_init_event(newevents[i]);

        /* if allocating memory for event data as is normally the case ... */
        if (noalloc == 0) {
            if ((newevents[i]->pdata = malloc(eventsize)) == NULL) {
                if (etid->debug >= ET_DEBUG_ERROR) {
                    et_logmsg("ERROR", "etr_events_new_group, cannot allocate memory\n");
                }
                free(newevents[i]);
                err = ET_ERROR_REMOTE;
                break;
            }
        }
        /* else if user supplying a buffer ... */
        else {
            /* Take an element of the et_event structure that is unused
             * in a remote setting and use it to record the fact that
             * the user is supplying the data buffer. This way when a
             * "put" is done, ET will not try to free the memory.
             */
            newevents[i]->owner = ET_NOALLOC;
        }
        newevents[i]->length  = size;
        newevents[i]->memsize = eventsize;
        newevents[i]->modify  = ET_MODIFY;
        newevents[i]->place   = ntohl(places[i]);
        newevents[i]->temp    = temp;
    }

    /* if error in above for loop ... */
    if (err < 0) {
        /* free up all allocated memory */
        for (j=0; j < i; j++) {
            if (noalloc == 0) {
                free(newevents[j]->pdata);
            }
            free(newevents[j]);
        }
        free(places);
        free(newevents);
        return err;
    }

    /* now that all is OK, copy into user's array of event pointers */
    for (i=0; i < nevents; i++) {
        evs[i] = newevents[i];
    }
    if (nread != NULL) {
        *nread = nevents;
    }
    free(places);
    free(newevents);

    return ET_OK;
}

/******************************************************/
int etr_event_get(et_sys_id id, et_att_id att, et_event **ev,
                  int mode, struct timespec *deltatime)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int i, err, modify, wait, netWait, delay=0;
    uint32_t transfer[6], header[9+ET_STATION_SELECT_INTS];
    uint64_t len;
    size_t eventsize=0;
    et_event *newevent;

    /* How many times we call et_event_get with an adjusted deltatime value */
    int iterations = 1;
    /* deltatime in microseconds */
    long microSec  = 0L;
    /* Duration of each et_event_get in microsec (.2 sec) */
    int newTimeInterval = 200000;
    /* Time to wait outside mutex locked code for each et_event_get */
    struct timespec waitTime = {0, 10000000}; /* .01 sec */
    /* Holds deltatime in sleep mode and when it needs changing in timed mode */
    struct timespec newDeltaTime = {0, 0};
    
    
    /* Pick out wait & no-allocate parts of mode.
     * Value of wait is checked in et_event_get. */
    netWait = wait = mode & ET_WAIT_MASK;

    if (wait == ET_TIMED) {
        /* timespec to int */
        microSec = deltatime->tv_sec*1000000 + deltatime->tv_nsec/1000;
    }

    /* When using the network, do NOT use SLEEP mode because that
     * may block all usage of this API's network communication methods.
     * Use repeated calls in TIMED mode. In between those calls,
     * allow other mutex-grabbing code to run (such as wakeUpAll). */
    if (wait == ET_SLEEP) {
        netWait = ET_TIMED;
        newDeltaTime.tv_sec = 0;
        newDeltaTime.tv_nsec = 200000000;  /* 0.2 sec timeout */
        deltatime = &newDeltaTime;
    }
    /* Also, if there is a long time designated for TIMED mode,
     * break it up into repeated smaller time chunks for the
     * reason mentioned above. Don't break it up if timeout <= 1 sec. */
    else if (wait == ET_TIMED && (microSec > 1000000))  {
        /* Newly set duration of each get */
        newDeltaTime.tv_sec  =  newTimeInterval/1000000;
        newDeltaTime.tv_nsec = (newTimeInterval - newDeltaTime.tv_sec*1000000) * 1000;
        deltatime = &newDeltaTime;
        /* How many times do we call et_event_get with this new timeout value?
         * It will be an over estimate unless timeout is evenly divisible by .2 seconds. */
        iterations = (int)microSec/newTimeInterval;
        if (microSec % newTimeInterval > 0) iterations++;
    }

    /* Modifying the whole event has precedence over modifying
     * only the header should the user specify both.
     */
    modify = mode & ET_MODIFY;
    if (modify == 0) {
        modify = mode & ET_MODIFY_HEADER;
    }

    transfer[0] = htonl(ET_NET_EV_GET);
    transfer[1] = htonl((uint32_t)att);
    transfer[2] = htonl((uint32_t)netWait);
    transfer[3] = htonl((uint32_t) (modify | (mode & ET_DUMP)));
    transfer[4] = 0;
    transfer[5] = 0;
    if (deltatime) {
        transfer[4] = htonl((uint32_t)deltatime->tv_sec);
        transfer[5] = htonl((uint32_t)deltatime->tv_nsec);
    }

    while(1) {
        /* Give other routines that write over the network a chance to run */
        if (delay) nanosleep(&waitTime, NULL);

        et_tcp_lock(etid);
        if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_get, write error\n");
            }
            return ET_ERROR_WRITE;
        }
    
        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_get, read error\n");
            }
            return ET_ERROR_READ;
        }
        
        err = ntohl((uint32_t)err);
        
        if (err == ET_ERROR_TIMEOUT) {
            /* Only get here if using SLEEP or TIMED modes */
            et_tcp_unlock(etid);
            if (wait == ET_SLEEP || iterations-- > 0) {
                /* Give other routines that write over the network a chance to run */
                delay = 1;
                continue;
            }
            return err;
        }
        else if (err != ET_OK) {
            et_tcp_unlock(etid);
            return err;
        }

        break;
    }
    
    // tcp mutex is locked after leaving above while loop

    if (etNetTcpRead(sockfd, (void *) header, sizeof(header)) != sizeof(header)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_event_get, read error\n");
        }
        return ET_ERROR_READ;
    }

    /* allocate memory for an event */
    if ((newevent = (et_event *) malloc(sizeof(et_event))) == NULL) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_event_get, cannot allocate memory\n");
        }
        return ET_ERROR_REMOTE;
    }

    /* initialize new event */
    et_init_event(newevent);

    newevent->length = len = ET_64BIT_UINT(ntohl(header[0]),ntohl(header[1]));
    newevent->memsize      = ET_64BIT_UINT(ntohl(header[2]),ntohl(header[3]));
    newevent->priority     = ntohl(header[4]) & ET_PRIORITY_MASK;
    newevent->datastatus   =(ntohl(header[4]) & ET_DATA_MASK) >> ET_DATA_SHIFT;
    newevent->place        = ntohl(header[5]);  /*ET_64BIT_UINT(ntohl(header[5]),ntohl(header[6]));*/
    newevent->byteorder    = header[7];
    for (i=0; i < ET_STATION_SELECT_INTS; i++) {
        newevent->control[i] = ntohl(header[i+9]);
    }
    newevent->modify = modify;

#ifndef _LP64

    if (etid->bit64) {
        /* if event size > ~1G don't allocate all that space, only enough to hold data */
        if (newevent->memsize > UINT32_MAX/5) {
            eventsize = len;
        }
        else {
            eventsize = (size_t) newevent->memsize;
        }
    }
    else {
        eventsize = (size_t) newevent->memsize;
    }
#else
    eventsize = (size_t) newevent->memsize;
#endif

    if ((newevent->pdata = malloc(eventsize)) == NULL) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_event_get, cannot allocate memory\n");
        }
        free(newevent);
        return ET_ERROR_REMOTE;
    }

    /* read data */
    if (len > 0) {
        if (etNetTcpRead(sockfd, newevent->pdata, (int)len) != len) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_get, read error\n");
            }
            free(newevent->pdata);
            free(newevent);
            return ET_ERROR_READ;
        }
    }

    et_tcp_unlock(etid);

    *ev = newevent;
    return ET_OK;
}

/******************************************************/
int etr_events_get(et_sys_id id, et_att_id att, et_event *evs[],
                   int mode, struct timespec *deltatime, int num, int *nread)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int i, j, nevents, err, error, modify, wait, netWait, delay=0;
    uint32_t incoming[2], transfer[7], header[9+ET_STATION_SELECT_INTS];
    uint64_t size, len;
    size_t eventsize=0;
    et_event **newevents;

    /* How many times we call et_events_get with an adjusted deltatime value */
    int iterations = 1;
    /* deltatime in microseconds */
    long microSec = 0L;
    /* Duration of each et_events_get in microsec (.2 sec) */
    int newTimeInterval = 200000;
    /* Time to wait outside mutex locked code for each et_events_get */
    struct timespec waitTime = {0, 10000000}; /* .01 sec */
    /* Holds deltatime in sleep mode and when it needs changing in timed mode */
    struct timespec newDeltaTime = {0, 0};
    
    
    /* Allocate array of event pointers - store new events here
     * until copied to evs[] when all danger of error is past.
     */
    if ((newevents = (et_event **) calloc((size_t) num, sizeof(et_event *))) == NULL) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_events_get, cannot allocate memory\n");
        }
        return ET_ERROR_REMOTE;
    }

    /* Pick out wait & no-allocate parts of mode.
     * Value of wait is checked in et_events_get. */
    netWait = wait = mode & ET_WAIT_MASK;

    if (wait == ET_TIMED) {
        /* timespec to int */
        microSec = deltatime->tv_sec*1000000 + deltatime->tv_nsec/1000;
    }
    
    /* When using the network, do NOT use SLEEP mode because that
     * may block all usage of this API's network communication methods.
     * Use repeated calls in TIMED mode. In between those calls,
     * allow other mutex-grabbing code to run (such as wakeUpAll). */
    if (wait == ET_SLEEP) {
        netWait = ET_TIMED;
        newDeltaTime.tv_sec = 0;
        newDeltaTime.tv_nsec = 200000000;  /* 0.2 sec timeout */
        deltatime = &newDeltaTime;
    }
    /* Also, if there is a long time designated for TIMED mode,
     * break it up into repeated smaller time chunks for the
     * reason mentioned above. Don't break it up if timeout <= 1 sec. */
    else if (wait == ET_TIMED && (microSec > 1000000))  {
        /* Newly set duration of each get */
        newDeltaTime.tv_sec  =  newTimeInterval/1000000;
        newDeltaTime.tv_nsec = (newTimeInterval - newDeltaTime.tv_sec*1000000) * 1000;
        deltatime = &newDeltaTime;
        /* How many times do we call et_events_get with this new timeout value?
         * It will be an over estimate unless timeout is evenly divisible by .2 seconds. */
        iterations = (int)microSec/newTimeInterval;
        if (microSec % newTimeInterval > 0) iterations++;
    }
    
    /* Modifying the whole event has precedence over modifying
     * only the header should the user specify both.
     */
    modify = mode & ET_MODIFY;
    if (modify == 0) {
        modify = mode & ET_MODIFY_HEADER;
    }

    transfer[0] = htonl(ET_NET_EVS_GET);
    transfer[1] = htonl((uint32_t)att);
    transfer[2] = htonl((uint32_t)netWait);
    transfer[3] = htonl((uint32_t) (modify | (mode & ET_DUMP)));
    transfer[4] = htonl((uint32_t)num);
    transfer[5] = 0;
    transfer[6] = 0;
    
    if (deltatime) {
        transfer[5] = htonl((uint32_t)deltatime->tv_sec);
        transfer[6] = htonl((uint32_t)deltatime->tv_nsec);
    }

    while (1) {
        /* Give other routines that write over the network a chance to run */
        if (delay) nanosleep(&waitTime, NULL);
        
        et_tcp_lock(etid);
        if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_get, write error\n");
            }
            free(newevents);
            return ET_ERROR_WRITE;
        }
        
        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_get, read error\n");
            }
            free(newevents);
            return ET_ERROR_READ;
        }
        
        err = ntohl((uint32_t)err);

        /* if timeout, try again */
        if (err == ET_ERROR_TIMEOUT) {
            /* Only get here if using SLEEP or TIMED modes */
            et_tcp_unlock(etid);
            if (wait == ET_SLEEP || iterations-- > 0) {
                /* Give other routines that write over the network a chance to run */
                delay = 1;
                continue;
            }
            free(newevents);
            return err;
        }
        /* if another error and not a valid response (# of events), return error */
        else if (err < 0) {
            et_tcp_unlock(etid);
            free(newevents);
            return err;
        }
        
        break;
    }

    // tcp mutex is locked after leaving above while loop
        
    nevents = err;
    
    /* read total size of data to come - in bytes */
    if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming)) {
        et_tcp_unlock(etid);
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_events_get, read error\n");
        }
        free(newevents);
        return ET_ERROR_READ;
    }
    size = ET_64BIT_UINT(ntohl(incoming[0]),ntohl(incoming[1]));

    error = ET_OK;

    for (i=0; i < nevents; i++) {

        if (etNetTcpRead(sockfd, (void *) header, sizeof(header)) != sizeof(header)) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_get, read error\n");
            }
            error = ET_ERROR_READ;
            break;
        }

        /* allocate memory for event */
        if ((newevents[i] = (et_event *) malloc(sizeof(et_event))) == NULL) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_get, cannot allocate memory\n");
            }
            err = ET_ERROR_REMOTE;
            break;
        }

        /* initialize new event */
        et_init_event(newevents[i]);

        newevents[i]->length = len = ET_64BIT_UINT(ntohl(header[0]),ntohl(header[1]));
        newevents[i]->memsize      = ET_64BIT_UINT(ntohl(header[2]),ntohl(header[3]));
        newevents[i]->priority     = ntohl(header[4]) & ET_PRIORITY_MASK;
        newevents[i]->datastatus   =(ntohl(header[4]) & ET_DATA_MASK) >> ET_DATA_SHIFT;
        newevents[i]->place        = ntohl(header[5]);  /*ET_64BIT_UINT(ntohl(header[5]),ntohl(header[6]));*/
        /*printf("Got %llx, high = %x, low = %x\n", newevents[i]->pointer,
                                                  ntohl(header[5]),  ntohl(header[6]));
        */
        /*printf("Event %d: len = %llu, memsize = %llu\n", i, len,  newevents[i]->memsize);*/
        newevents[i]->byteorder    = header[7];
        for (j=0; j < ET_STATION_SELECT_INTS; j++) {
            newevents[i]->control[j] = ntohl(header[j+9]);
        }
        newevents[i]->modify = modify;

#ifndef _LP64
        /* The server will not send events that are too big for us,
         * we'll get an error above.
         */
        if (etid->bit64) {
            /* if event size > ~1G don't allocate all that space, only enough to hold data */
            if (newevents[i]->memsize > UINT32_MAX/5) {
                eventsize = len;
            }
            else {
                eventsize = (size_t) newevents[i]->memsize;
            }
        }
        else {
            eventsize = (size_t) newevents[i]->memsize;
        }
#else
        eventsize = (size_t) newevents[i]->memsize;
#endif

        if ((newevents[i]->pdata = malloc(eventsize)) == NULL) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_get, cannot allocate memory\n");
            }
            free(newevents[i]);
            error = ET_ERROR_REMOTE;
            break;
        }

        if (len > 0) {
            if (etNetTcpRead(sockfd, newevents[i]->pdata, (int)len) != len) {
                if (etid->debug >= ET_DEBUG_ERROR) {
                    et_logmsg("ERROR", "etr_events_get, read error\n");
                }
                free(newevents[i]->pdata);
                free(newevents[i]);
                error = ET_ERROR_READ;
                break;
            }
        }
    }

    et_tcp_unlock(etid);

    if (error != ET_OK) {
        /* free up all allocated memory */
        for (j=0; j < i; j++) {
            free(newevents[j]->pdata);
            free(newevents[j]);
        }
        free(newevents);
        return error;
    }

    /* now that all is OK, copy into user's array of event pointers */
    for (i=0; i < nevents; i++) {
        evs[i] = newevents[i];
    }
    if (nread != NULL) {
        *nread = nevents;
    }
    free(newevents);
    
    return ET_OK;
}

/******************************************************/
int etr_event_put(et_sys_id id, et_att_id att, et_event *ev)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int i, err=ET_OK, iov_bufs;
    uint32_t transfer[9+ET_STATION_SELECT_INTS];
    struct iovec iov[2];

    /* if length bigger than memory size, we got problems  */
    if (ev->length > ev->memsize) {
        if (etid->debug >= ET_DEBUG_ERROR) {
            et_logmsg("ERROR", "etr_event_put, data length is too large!\n");
        }
        return ET_ERROR;
    }

    /* if we're changing an event or writing a new event ... */
    if (ev->modify > 0) {
        /* Do not send back datastatus bits since they
         * only change if this remote client dies.
         */
        transfer[0] = htonl(ET_NET_EV_PUT);
        transfer[1] = htonl((uint32_t)att);
        transfer[2] = htonl((uint32_t)ev->place);
        transfer[3] = 0; /* not used */
        transfer[4] = htonl(ET_HIGHINT(ev->length));
        transfer[5] = htonl(ET_LOWINT(ev->length));
        transfer[6] = htonl((uint32_t) (ev->priority | ev->datastatus << ET_DATA_SHIFT));
        transfer[7] = (uint32_t) ev->byteorder;
        transfer[8] = 0; /* not used */
        for (i=0; i < ET_STATION_SELECT_INTS; i++) {
            transfer[i+9] = htonl((uint32_t)ev->control[i]);
        }

        /* send header if modifying header or whole event */
        iov[0].iov_base = (void *) transfer;
        iov[0].iov_len  = sizeof(transfer);
        iov_bufs = 1;

        /* send data only if modifying whole event */
        if (ev->modify == ET_MODIFY) {
            /* The data pointer might be null if using ET_NOALLOC
             * option in et_event(s)_new and the user never supplied
             * a data buffer.
             */
            if (ev->pdata == NULL) {
                if (etid->debug >= ET_DEBUG_ERROR) {
                    et_logmsg("ERROR", "etr_event_put, bad pointer to data\n");
                }
                return ET_ERROR_REMOTE;
            }
            iov[1].iov_base = ev->pdata;
            iov[1].iov_len  = (size_t)ev->length;
            iov_bufs++;
        }

        /* write data */
        et_tcp_lock(etid);
        if (etNetTcpWritev(sockfd, iov, iov_bufs, 16) == -1) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_put, write error\n");
            }
            return ET_ERROR_WRITE;
        }

        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_put, read error\n");
            }
            free(ev->pdata);
            free(ev);
            return ET_ERROR_READ;
        }
        et_tcp_unlock(etid);
        err = ntohl((uint32_t)err);
    }

    /* If event data buffer was malloced, then free it,
     * if supplied by the user, do not free it. */
    if (ev->owner != ET_NOALLOC) {
        free(ev->pdata);
    }
    free(ev);

    return err;
}

/******************************************************/
int etr_events_put(et_sys_id id, et_att_id att, et_event *evs[], int num)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int i, j, err, iov_init, iov_bufs, nevents;
    int index, headersize, *header = NULL;
    uint32_t transfer[5];
    uint64_t bytes;
    struct iovec *iov = NULL;

    err        = ET_OK;
    nevents    = 0;
    iov_init   = 0;
    iov_bufs   = 0;
    bytes      = 0ULL;
    index      = 0;
    headersize = (7+ET_STATION_SELECT_INTS)*sizeof(int);

    for (i=0; i < num; i++) {
        /* if length bigger than memory size, we got problems  */
        if (evs[i]->length > evs[i]->memsize) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_put, 1 or more data lengths are too large!\n");
            }
            return ET_ERROR;
        }
    }

    for (i=0; i < num; i++) {
        /* if modifying an event ... */
        if (evs[i]->modify > 0) {
            /* if first time thru ... */
            if (iov_init == 0)
            {
                iov_init++;
                index = 0;
                if ( (iov = (struct iovec *) calloc((size_t) (2*num+1), sizeof(struct iovec))) == NULL) {
                    if (etid->debug >= ET_DEBUG_ERROR) {
                        et_logmsg("ERROR", "etr_events_put, cannot allocate memory\n");
                    }
                    return ET_ERROR_REMOTE;
                }
                if ( (header = (int *) calloc((size_t) num, (size_t) headersize)) == NULL) {
                    if (etid->debug >= ET_DEBUG_ERROR) {
                        et_logmsg("ERROR", "etr_events_put, cannot allocate memory\n");
                    }
                    free(iov);
                    return ET_ERROR_REMOTE;
                }
                transfer[0] = htonl(ET_NET_EVS_PUT);
                transfer[1] = htonl((uint32_t)att);
                iov[iov_bufs].iov_base = (void *) transfer;
                iov[iov_bufs].iov_len  = sizeof(transfer);
                iov_bufs++;
            }

            header[index]   = htonl((uint32_t)evs[i]->place);
            header[index+1] = 0; /* not used */
            header[index+2] = htonl(ET_HIGHINT(evs[i]->length));
            header[index+3] = htonl(ET_LOWINT(evs[i]->length));
            header[index+4] = htonl((uint32_t) (evs[i]->priority | evs[i]->datastatus << ET_DATA_SHIFT));
            header[index+5] = evs[i]->byteorder;
            header[index+6] = 0; /* not used */
            for (j=0; j < ET_STATION_SELECT_INTS; j++) {
                header[index+7+j] = htonl((uint32_t)evs[i]->control[j]);
            }

            /*
            printf("etr_events_put:   [0]  = %x\n", ntohl(header[index]));
            printf("etr_events_put:   [1]  = %x\n", ntohl(header[index+1]));
            printf("etr_events_put:   [2]  = %x\n", ntohl(header[index+2]));
            printf("etr_events_put:   [3]  = %x\n", ntohl(header[index+3]));
            printf("etr_events_put:   [4]  = %d\n", ntohl(header[index+4]));
            printf("etr_events_put:   [5]  = 0x%x\n", ntohl(header[index+5]));
            printf("etr_events_put:   [6]  = %d\n", ntohl(header[index+6]));
            printf("etr_events_put:   [7]  = %d\n", ntohl(header[index+7]));
            printf("etr_events_put:   [8]  = %d\n", ntohl(header[index+8]));
            printf("etr_events_put:   [9]  = %d\n", ntohl(header[index+9]));
            printf("etr_events_put:   [10] = %d\n", ntohl(header[index+10]));
            if (ET_STATION_SELECT_INTS >= 6) {
                printf("etr_events_put:   [11] = %d\n", ntohl(header[index+11]));
                printf("etr_events_put:   [12] = %d\n", ntohl(header[index+12]));
            }
            printf("\n\n");
            */
            
            /* send header if modifying header or whole event */
            iov[iov_bufs].iov_base = (void *) &header[index];
            iov[iov_bufs].iov_len  = (size_t) headersize;
            iov_bufs++;
            bytes += headersize;

            /* send data only if modifying whole event */
            if (evs[i]->modify == ET_MODIFY) {
                /* The data pointer might be null if using ET_NOALLOC
                 * option in et_event(s)_new and the user never supplied
                 * a data buffer.
                 */
                if (evs[i]->pdata == NULL) {
                    if (etid->debug >= ET_DEBUG_ERROR)
                    {
                        et_logmsg("ERROR", "etr_events_put, bad pointer to data\n");
                    }
                    free(iov);
                    free(header);
                    return ET_ERROR_REMOTE;
                }
                iov[iov_bufs].iov_base = evs[i]->pdata;
                iov[iov_bufs].iov_len  = (size_t) evs[i]->length;
                iov_bufs++;
                bytes += evs[i]->length;
            }
            nevents++;
            index += (7+ET_STATION_SELECT_INTS);
/*printf("etr_events_put: num events stored in header and iov, %d\n", nevents);*/
        }
    }

    if (nevents > 0) {
        /* send # of events & total # bytes */
        transfer[2] = htonl((uint32_t)nevents);
        transfer[3] = htonl(ET_HIGHINT(bytes));
        transfer[4] = htonl(ET_LOWINT(bytes));

        et_tcp_lock(etid);
        if (etNetTcpWritev(sockfd, iov, iov_bufs, 16) == -1) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_put, write error\n");
            }
            free(iov);
            free(header);
            return ET_ERROR_WRITE;
        }
        free(iov);
        free(header);

        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_put, read error\n");
            }
            err = ET_ERROR_READ;
        }
        else {
            err = ntohl((uint32_t)err);
        }
        et_tcp_unlock(etid);
    }


    for (i=0; i < num; i++) {
        /* if event data buffer was malloced, then free it,
         * if supplied by the user, do not free it. */
        if (evs[i]->owner != ET_NOALLOC) {
            free(evs[i]->pdata);
        }
        free(evs[i]);
    }

    return err;
}

/******************************************************/
/* Dumping an event means we don't have to bother
 * sending any data, just its pointer.
 */
int etr_event_dump(et_sys_id id, et_att_id att, et_event *ev)
{
    et_id *etid = (et_id *) id;
    int err=ET_OK, sockfd = etid->sockfd;
    uint32_t transfer[3];

    /* If we've told the ET system that we're changing an event
     * or writing a new event, send back some info.
     */
    if (ev->modify > 0) {
        transfer[0] = htonl(ET_NET_EV_DUMP);
        transfer[1] = htonl((uint32_t)att);
        transfer[2] = htonl((uint32_t)ev->place);

        /* write data */
        et_tcp_lock(etid);
        if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_dump, write error\n");
            }
            return ET_ERROR_WRITE;
        }

        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_event_dump, read error\n");
            }
            free(ev->pdata);
            free(ev);
            return ET_ERROR_READ;
        }
        et_tcp_unlock(etid);
        err = ntohl((uint32_t)err);
    }

    free(ev->pdata);
    free(ev);

    return err;
}

/******************************************************/
int etr_events_dump(et_sys_id id, et_att_id att, et_event *evs[], int num)
{
    et_id *etid = (et_id *) id;
    int sockfd = etid->sockfd;
    int i, err, iov_init, count, index;
    int *transfer = NULL;

    err      = ET_OK;
    index    = 0;
    count    = 0;
    iov_init = 0;

    for (i=0; i < num; i++) {
        /* if modifying an event ... */
        if (evs[i]->modify > 0) {
            /* if first time thru ... */
            if (iov_init == 0) {
                iov_init++;
                if ( (transfer = (int *) calloc((size_t) (num+3), sizeof(int))) == NULL) {
                    if (etid->debug >= ET_DEBUG_ERROR) {
                        et_logmsg("ERROR", "etr_events_dump, cannot allocate memory\n");
                    }
                    return ET_ERROR_REMOTE;
                }

                transfer[0] = htonl(ET_NET_EVS_DUMP);
                transfer[1] = htonl((uint32_t)att);
                index = 3;
            }

            transfer[index++] = htonl((uint32_t)evs[i]->place);

            count++;
        }
    }

    if (count > 0) {
        transfer[2] = htonl((uint32_t)count);

        et_tcp_lock(etid);
        if (etNetTcpWrite(sockfd, (void *) transfer, (int) ((count+3)*sizeof(int))) !=
                (count+3)*sizeof(int)) {
            et_tcp_unlock(etid);
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_dump, write error\n");
            }
            free(transfer);
            return ET_ERROR_WRITE;
        }

        free(transfer);

        if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err)) {
            if (etid->debug >= ET_DEBUG_ERROR) {
                et_logmsg("ERROR", "etr_events_dump, read error\n");
            }
            err = ET_ERROR_READ;
        }
        else {
            err = ntohl((uint32_t)err);
        }
        et_tcp_unlock(etid);
    }


    for (i=0; i < num; i++) {
        free(evs[i]->pdata);
        free(evs[i]);
    }

    return err;
}
