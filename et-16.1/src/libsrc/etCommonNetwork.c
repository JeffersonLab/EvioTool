/*----------------------------------------------------------------------------*
 *  Copyright (c) 2004        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, #10           *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      Routines dealing with network communications.
 *      These routines are made independent of both cMsg and ET
 *           but are used by each with a little preprocessor
 *           help. Since a program may link with both cMsg & ET
 *           libraries, rename these routines using the preprocessor
 *           to avoid naming conflicts.
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <netdb.h>       /* herrno */
#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <sys/ioctl.h>   /* find broacast addr */


#include <strings.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/utsname.h>
#include <inttypes.h>
#include <arpa/inet.h>   /* htonl stuff */
#include <netinet/in.h>
#ifdef __APPLE__
#include <ifaddrs.h>
#endif



extern int h_errno;

#include "etCommonNetwork.h"

/* set debug level for network stuff here */
int etDebug = 0;


/* mutex to protect non-reentrant gethostbyname in Linux since the
 * gethostbyname_r is so buggy. */
static pthread_mutex_t getHostByNameMutex = PTHREAD_MUTEX_INITIALIZER;



/**
 * Routine for byte swapping of 64-bit integer.
 * 
 * @param n 64-bit integer to be swapped
 * @returns swapped 64-bit integer
 */
uint64_t NTOH64(uint64_t n) {
    uint64_t h;
    uint64_t tmp = ntohl(n & 0x00000000ffffffff);
    h = ntohl(n >> 32);
    h |= tmp << 32;
    return h;
}


/**
 * This routine creates a TCP listening socket for a server.
 *
 * @param nonblocking != 0 if creating nonblocking socket (boolean)
 * @param port        port to listen on
 * @param sendBufSize size of socket's send buffer in bytes (<= 0 means use default)
 * @param rcvBufSize  size of socket's receive buffer in bytes (<= 0 means use default)
 * @param noDelay     0 if socket TCP_NODELAY is off, else it's on
 * @param listenFd    pointer to file descriptor which get filled in
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT if listenFd is NULL or port < 1024
 * @returns ET_ERROR_SOCKET/CMSG_SOCKET_ERROR if socket could not be created or socket options could not be set.
 *
 */
int codanetTcpListen(int nonblocking, unsigned short port,
                     int sendBufSize, int rcvBufSize,
                     int noDelay, int *listenFd)
{
    int                 listenfd, err, val;
    const int           on=1;
    struct sockaddr_in  servaddr;

    if (listenFd == NULL || port < 1024) {
        if (codanetDebug >= CODA_DEBUG_ERROR)
            fprintf(stderr, "%sTcpListen: null \"listenFd\" or bad port arg(s)\n", codanetStr);
        return(CODA_BAD_ARGUMENT);
    }
  
    err = listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (err < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpListen: socket error\n", codanetStr);
        return(CODA_SOCKET_ERROR);
    }

    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);
  
    /* don't wait for messages to cue up, send any message immediately */
    if (noDelay) {
        err = setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, (char*) &on, sizeof(on));
        if (err < 0) {
            close(listenfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpListen: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }
   
    /* reuse this port after program quits */
    err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));
    if (err < 0) {
        close(listenfd);
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpListen: setsockopt error\n", codanetStr);
        return(CODA_SOCKET_ERROR);
    }
  
    /* send periodic (every 2 hrs.) signal to see if socket's other end is alive */
    err = setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (char*) &on, sizeof(on));
    if (err < 0) {
        close(listenfd);
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpListen: setsockopt error\n", codanetStr);
        return(CODA_SOCKET_ERROR);
    }
  
    /* set send buffer size unless default specified by a value <= 0 */
    if (sendBufSize > 0) {
        err = setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, (char*) &sendBufSize, sizeof(sendBufSize));
        if (err < 0) {
            close(listenfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpListen: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }
  
    /* set receive buffer size unless default specified by a value <= 0  */
    if (rcvBufSize > 0) {
        err = setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (char*) &rcvBufSize, sizeof(rcvBufSize));
        if (err < 0) {
            close(listenfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpListen: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }

    /* make this socket non-blocking if desired */
    if (nonblocking) {
        val = fcntl(listenfd, F_GETFL, 0);
        if (val > -1) {
            fcntl(listenfd, F_SETFL, val | O_NONBLOCK);
        }
    }

    /* don't let anyone else have this port */
    err = bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
    if (err < 0) {
        close(listenfd);
        /* Don't print error as it happens often with no effect. */
        return(CODA_SOCKET_ERROR);
    }
  
    /* tell system you're waiting for others to connect to this socket */
    err = listen(listenfd, LISTENQ);
    if (err < 0) {
        close(listenfd);
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpListen: listen error\n", codanetStr);
        return(CODA_SOCKET_ERROR);
    }
  
    if (listenFd != NULL) *listenFd = listenfd;
    return(CODA_OK);
}


/**
 * Start with startingPort & keeping trying different port numbers until
 * one is found that is free for listening on. Then create listening socket
 * at the port. Try 1500 port numbers.
 *
 * @param nonblocking  != 0 if creating nonblocking socket (boolean)
 * @param startingPort port number at which to start looking for available port to listen on
 * @param sendBufSize  size of socket's send buffer in bytes (<= 0 means use default)
 * @param rcvBufSize   size of socket's receive buffer in bytes (<= 0 means use default)
 * @param noDelay      0 if socket TCP_NODELAY is off, else it's on
 * @param port         pointer filled in with listening port number selected
 * @param fd           pointer filled in with file descriptor of listening socket
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET_ERROR_SOCKET/CMSG_SOCKET_ERROR if socket could not be created or socket options could not be set.
 *
 */
int codanetGetListeningSocket(int nonblocking, unsigned short startingPort,
                              int sendBufSize, int rcvBufSize,
                              int noDelay, int *finalPort, int *fd) {
                               
    unsigned short  i, port=startingPort, trylimit=1500;
    int listenFd = 0;
      
    /* for a limited number of times */
    for (i=0; i < trylimit; i++) {
        /* try to listen on a port */
        if (codanetTcpListen(nonblocking, port, sendBufSize, rcvBufSize, noDelay, &listenFd) != CODA_OK) {
            if (codanetDebug >= CODA_DEBUG_WARN) {
                fprintf(stderr, "%sGetListeningPort: tried but could not listen on port %hu\n", codanetStr, port);
            }
            port++;
            if (port < 1025) port = 1025;
            continue;
        }
        break;
    }

    if (listenFd < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sServerListeningThread: ports %hu thru %hu busy\n",
                    codanetStr, startingPort, (unsigned short) (startingPort+trylimit-1));
        }
        return(CODA_SOCKET_ERROR);
    }

    if (codanetDebug >= CODA_DEBUG_INFO) {
        fprintf(stderr, "%sServerListeningThread: listening on port %hu\n", codanetStr, port);
    }

    if (finalPort != NULL) *finalPort = port;
    if (fd != NULL) *fd = listenFd;

    return(CODA_OK);
}


/*-------------------------------------------------------------------*/

/** 
 *  (12/4/06)     Default tcp buffer size, bytes
 *  Platform         send      recv
 * --------------------------------------------
 *  Linux java       43690      8192
 *  Linux 2.4,2.6    87380     16384
 *  Solaris 10       49152     49152
 *  
 */




static void connect_w_to(void) { 
    int res, soc, on=1, off=0;
    struct sockaddr_in addr;
    long arg;
    fd_set myset;
    struct timeval tv;
    int valopt;
    socklen_t lon;

    // Create socket
    soc = socket(AF_INET, SOCK_STREAM, 0);
    if (soc < 0) {
        fprintf(stderr, "Error creating socket (%d %s)\n", errno, strerror(errno));
        exit(0);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(2000);
    addr.sin_addr.s_addr = inet_addr("192.168.0.1");

    // Set non-blocking 
    if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) {
        fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(0);
    }
    arg |= O_NONBLOCK;
    if( fcntl(soc, F_SETFL, arg) < 0) {
        fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(0);
    }
    if (ioctl(soc, FIONBIO, &on) < 0) {
        fprintf(stderr, "Error ioctl(..., FIONBIO) (%s)\n", strerror(errno));
        exit(0);
    }
    
    // Trying to connect with timeout 
    res = connect(soc, (struct sockaddr *)&addr, sizeof(addr));
    if (res < 0) {
        if (errno == EINPROGRESS) {
            fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
            do {
                tv.tv_sec = 15;
                tv.tv_usec = 0;
                FD_ZERO(&myset);
                FD_SET(soc, &myset);
                res = select(soc+1, NULL, &myset, NULL, &tv);
                if (res < 0 && errno != EINTR) {
                    fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
                    exit(0);
                }
                else if (res > 0) {
                    // Socket selected for write 
                    lon = sizeof(int);
                    if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) {
                        fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
                        exit(0);
                    }
                    
                    // Check the value returned... 
                    if (valopt) {
                        fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt)
                               );
                        exit(0);
                    }
                    break;
                }
                else {
                    fprintf(stderr, "Timeout in select() - Cancelling!\n");
                    exit(0);
                }
            } while (1);
        }
        else {
            fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
            exit(0);
        }
    }
    
    // Set to blocking mode again... 
    if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) {
        fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(0);
    }
    arg &= (~O_NONBLOCK);
    if( fcntl(soc, F_SETFL, arg) < 0) {
        fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(0);
    }
    
    if (ioctl(soc, FIONBIO, &off) < 0) {
        fprintf(stderr, "Error ioctl(..., FIONBIO) (%s)\n", strerror(errno));
        exit(0);
    }
}


/**
 * This routine is a replacement for connect() with an additional timeout
 * argument. Assumes sockfd is for a nonblocking socket.
 *
 * @param sockfd  socket file descriptor of nonblocking socket
 * @param pAddr   pointer to sockaddr struct
 * @param addrlen length of pAddr structure in bytes
 * @param tv      time to wait before timing out
 * 
 * @returns  1 for a valid connection
 * @returns  0 for a timeout
 * @returns -1 for an error
 *
 */
static int connectWithTimeout(int sockfd, struct sockaddr *pAddr, socklen_t addrlen, struct timeval *tv) {
    int result;
    fd_set myset;
    int valopt;
    socklen_t lon;
    struct timeval timeout;

    result = connect(sockfd, pAddr, addrlen);
    
    if (result < 0) {
        if (errno == EINPROGRESS) {
            if (codanetDebug >= CODA_DEBUG_INFO) {
                fprintf(stderr, "connectWithTimeout: EINPROGRESS in connect() - selecting\n");
            }

            FD_ZERO(&myset);
            FD_SET(sockfd, &myset);
            
            /* select irritatingly sets the timeout values to be 0 in Linux, so use copy of tv */
            timeout.tv_sec  = tv->tv_sec;
            timeout.tv_usec = tv->tv_usec;

            result = select(sockfd+1, NULL, &myset, NULL, &timeout);

            if (result < 0 && errno != EINTR) {
                if (codanetDebug >= CODA_DEBUG_ERROR) {
                    fprintf(stderr, "connectWithTimeout: error connecting %d - %s\n", errno, strerror(errno));
                }
                return -1;
            }
            else if (result > 0) {
                // Socket selected for write
                lon = sizeof(int);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) {
                    if (codanetDebug >= CODA_DEBUG_ERROR) {
                        fprintf(stderr, "connectWithTimeout: error in getsockopt() %d - %s\n", errno, strerror(errno));
                    }
                    return -1;
                }
                    
                // Check the value returned...
                if (valopt) {
                    if (codanetDebug >= CODA_DEBUG_ERROR) {
                        fprintf(stderr, "connectWithTimeout: error in delayed connection() %d - %s\n", valopt, strerror(valopt));
                    }
                    return -1;
                }
            }
            else {
                if (codanetDebug >= CODA_DEBUG_INFO) {
                    fprintf(stderr, "connectWithTimeout: timeout in select() - Cancelling!\n");
                }
                return 0;
            }
        }
        else {
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "connectWithTimeout: error connecting %d - %s\n", errno, strerror(errno));
            }
            return -1;
        }
    }

    return 1;
}



/**
 * This routine makes a TCP connection to a server with an
 * optional timeout.
 *
 * @param ip_address  name of host to connect to (may be dotted-decimal)
 * @param port        port to connect to
 * @param sendBufSize size of socket's send buffer in bytes
 * @param rcvBufSize  size of socket's receive buffer in bytes
 * @param noDelay     0 if socket TCP_NODELAY is off, else it's on
 * @param timeout     pointer to struct containing timeout for connection to be made
 * @param fd          pointer which gets filled in with file descriptor
 * @param localPort   pointer which gets filled in with local (ephemeral) port number
 *
 * @returns ET/CMSG_OK                          if successful
 * @returns ET_ERROR_TIMEOUT/CMSG_TIMEOUT       if connection attempt timed out
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT   if ip_adress or fd args are NULL
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY   if out of memory
 * @returns ET_ERROR_SOCKET/CMSG_SOCKET_ERROR   if socket could not be created or socket options could not be set.
 * @returns ET_ERROR_NETWORK/CMSG_NETWORK_ERROR if host name could not be resolved or could not connect
 */
int codanetTcpConnectTimeout(const char *ip_address, unsigned short port,
                             int sendBufSize, int rcvBufSize,
                             int noDelay, struct timeval *timeout,
                             int *fd, int *localPort)
{
    int                 res, sockfd, err=0;
    const int           on=1, off=0;
    struct sockaddr_in  servaddr;
    int                 status;
    struct in_addr      **pptr;
    struct hostent      *hp;
    int h_errnop        = 0;

    if (ip_address == NULL || fd == NULL || timeout == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnectTimeout: null argument(s)\n", codanetStr);
        return(CODA_BAD_ARGUMENT);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sTcpConnectTimeout: socket error, %s\n", codanetStr, strerror(errno));
        }
        return(CODA_SOCKET_ERROR);
    }

    /* don't wait for messages to cue up, send any message immediately */
    if (noDelay) {
        err = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &on, sizeof(on));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnectTimeout: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }

    /* set send buffer size unless default specified by a value <= 0 */
    if (sendBufSize > 0) {
        err = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &sendBufSize, sizeof(sendBufSize));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnectTimeout: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }

    /* set receive buffer size unless default specified by a value <= 0  */
    if (rcvBufSize > 0) {
        err = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &rcvBufSize, sizeof(rcvBufSize));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnectTimeout: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }
	
    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);

    /* make socket nonblocking so we can implement a timeout */
    if (ioctl(sockfd, FIONBIO, &on) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnectTimeout: ioctl error\n", codanetStr);
        return(CODA_SOCKET_ERROR);
    }

    /*
     * There seem to be serious bugs with Linux implementation of
     * gethostbyname_r. See:
     * http://curl.haxx.se/mail/lib-2003-10/0201.html
     * http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6369541
     *
     * Sooo, let's use the non-reentrant version and simply protect
     * with our own mutex.
     */
 
    /* make gethostbyname thread-safe */
    status = pthread_mutex_lock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Lock gethostbyname Mutex");
    }
   
    if ((hp = gethostbyname(ip_address)) == NULL) {
        status = pthread_mutex_unlock(&getHostByNameMutex);
        if (status != 0) {
        coda_err_abort(status, "Unlock gethostbyname Mutex");
        }
        close(sockfd);
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnectTimeout: hostname error - %s\n",
                                                      codanetStr, codanetHstrerror(h_errnop));
        return(CODA_NETWORK_ERROR);
    }
    pptr = (struct in_addr **) hp->h_addr_list;

    for ( ; *pptr != NULL; pptr++) {
        memcpy(&servaddr.sin_addr, *pptr, sizeof(struct in_addr));
        res = connectWithTimeout(sockfd, (SA *) &servaddr, sizeof(servaddr), timeout);
        if (res < 0) {
            err = CODA_ERROR;
            if (codanetDebug >= CODA_DEBUG_WARN) {
                fprintf(stderr, "%sTcpConnectTimeout: error attempting to connect to server, %s\n", codanetStr, strerror(errno));
            }
        }
        else if (res == 0) {
            err = CODA_TIMEOUT;
            if (codanetDebug >= CODA_DEBUG_INFO) {
                fprintf(stderr, "%sTcpConnectTimeout: timed out attempting to connect to server\n", codanetStr);
            }
        }
        else {
            err = CODA_OK;
            if (codanetDebug >= CODA_DEBUG_INFO) {
                fprintf(stderr, "%sTcpConnectTimeout: connected to server\n", codanetStr);
            }
            break;
        }
    }

    /* if there's no error, find & return the local port number of this socket */
    if (err != -1 && localPort != NULL) {
        socklen_t len;
        struct sockaddr_in ss;
      
        len = sizeof(ss);
        if (getsockname(sockfd, (SA *) &ss, &len) == 0) {
            *localPort = (int) ntohs(ss.sin_port);
        }
        else {
            *localPort = 0;
        }
    }
   
    status = pthread_mutex_unlock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Unlock gethostbyname Mutex");
    }

    if (err != CODA_OK) {
        close(sockfd);
        return(err);
    }
  
    /* return socket to blocking mode */
    if (ioctl(sockfd, FIONBIO, &off) < 0) {
        close(sockfd);
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sTcpConnectTimeout: ioctl error\n", codanetStr);
        }
        return(CODA_SOCKET_ERROR);
    }

    if (fd != NULL)  *fd = sockfd;
    return(CODA_OK);
}


/**
 * This routine makes a TCP connection to a server using a dotted-decimal IP
 * address. It will block up to the user-specified timeout or the default timeout
 * of the TCP connect() function, whichever is shorter. The point is to avoid
 * the default timeout on Linux of over 3 minutes and on Solaris of 4 minutes
 * if no server present on the other end.
 *
 * @param ip_address  name of host to connect to (may be dotted-decimal)
 * @param interface   interface (dotted-decimal ip address) to connect through
 * @param port        port to connect to
 * @param sendBufSize size of socket's send buffer in bytes
 * @param rcvBufSize  size of socket's receive buffer in bytes
 * @param noDelay     0 if socket TCP_NODELAY is off, else it's on
 * @param timeout     pointer to struct containing timeout for connection to be made
 * @param fd          pointer which gets filled in with file descriptor
 * @param localPort   pointer which gets filled in with local (ephemeral) port number
 *
 * @returns ET/CMSG_OK                          if successful
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT   if ip_adress or fd args are NULL
 * @returns ET_ERROR_SOCKET/CMSG_SOCKET_ERROR   if socket could not be created or socket options could not be set.
 * @returns ET_ERROR_NETWORK/CMSG_NETWORK_ERROR if host name could not be resolved or could not connect
 *
 */
int codanetTcpConnectTimeout2(const char *ip_address, const char *interface, unsigned short port,
                              int sendBufSize, int rcvBufSize, int noDelay, struct timeval *timeout,
                              int *fd, int *localPort)
{
    int                 sockfd, err=0, isDottedDecimal=0;
    const int           on=1, off=0;
    struct sockaddr_in  servaddr;
    uint32_t            inetaddr;


    if (ip_address == NULL || fd == NULL || timeout == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sTcpConnectTimeout2: null argument(s)\n", codanetStr);
        }
        return(CODA_BAD_ARGUMENT);
    }

    /* Check to see if ip_address is in dotted-decimal form. If not, return error. */
    isDottedDecimal = codanetIsDottedDecimal(ip_address, NULL);
    if (!isDottedDecimal) return CODA_ERROR;
    
    if (inet_pton(AF_INET, ip_address, &inetaddr) < 1) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sTcpConnectTimeout2: unknown address for host %s\n", codanetStr, ip_address);
        }
        return(CODA_NETWORK_ERROR);
    }
    
    /* create socket and set some options */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sTcpConnectTimeout2: socket error, %s\n", codanetStr, strerror(errno));
        }
        return(CODA_SOCKET_ERROR);
    }
    
    /* don't wait for messages to cue up, send any message immediately */
    if (noDelay) {
        err = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &on, sizeof(on));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sTcpConnectTimeout2: setsockopt error\n", codanetStr);
            }
            return(CODA_SOCKET_ERROR);
        }
    }
  
    /* set send buffer size unless default specified by a value <= 0 */
    if (sendBufSize > 0) {
        err = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &sendBufSize, sizeof(sendBufSize));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sTcpConnectTimeout2: setsockopt error\n", codanetStr);
            }
            return(CODA_SOCKET_ERROR);
        }
    }
  
    /* set receive buffer size unless default specified by a value <= 0  */
    if (rcvBufSize > 0) {
        err = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &rcvBufSize, sizeof(rcvBufSize));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sTcpConnectTimeout2: setsockopt error\n", codanetStr);
            }
            return(CODA_SOCKET_ERROR);
        }
    }
    
    /* set the outgoing network interface */
    if (interface != NULL && strlen(interface) > 0) {
        err = codanetSetInterface(sockfd, interface);
        if (err != CODA_OK) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sTcpConnectTimeout2: error choosing network interface\n", codanetStr);
            }
            return(CODA_SOCKET_ERROR);
        }
    }

    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);
    servaddr.sin_addr.s_addr = inetaddr;


    /* make socket nonblocking so we can implement a timeout */
    if (ioctl(sockfd, FIONBIO, &on) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sTcpConnectTimeout2: ioctl error\n", codanetStr);
        }
        return(CODA_SOCKET_ERROR);
    }

    err = connectWithTimeout(sockfd, (SA *) &servaddr, sizeof(servaddr), timeout);
    if (err < 0) {
        close(sockfd);
        if (codanetDebug >= CODA_DEBUG_WARN) {
            fprintf(stderr, "%sTcpConnectTimeout2: error attempting to connect to server, %s\n", codanetStr, strerror(errno));
        }
        return(CODA_ERROR);
    }
    else if (err == 0) {
        close(sockfd);
        if (codanetDebug >= CODA_DEBUG_INFO) {
            fprintf(stderr, "%sTcpConnectTimeout2: timed out attempting to connect to server\n", codanetStr);
        }
        return(CODA_TIMEOUT);
    }
    else if (codanetDebug >= CODA_DEBUG_INFO) {
        fprintf(stderr, "%sTcpConnectTimeout2: connected to server\n", codanetStr);
    }


    /* since there's no error, find & return the local port number of this socket */
    if (localPort != NULL) {
        socklen_t len;
        struct sockaddr_in ss;
      
        len = sizeof(ss);
        if (getsockname(sockfd, (SA *) &ss, &len) == 0) {
            *localPort = (int) ntohs(ss.sin_port);
        }
        else {
            *localPort = 0;
        }
    }

    /* return socket to blocking mode */
    if (ioctl(sockfd, FIONBIO, &off) < 0) {
        close(sockfd);
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sTcpConnectTimeout2: ioctl error\n", codanetStr);
        }
        return(CODA_SOCKET_ERROR);
    }

    if (fd != NULL)  *fd = sockfd;
    return(CODA_OK);
}


/**
 * This routine makes a TCP connection to a server.
 *
 * @param ip_address  name of host to connect to (may be dotted-decimal)
 * @param interface   interface (dotted-decimal ip address) to connect through
 * @param port        port to connect to
 * @param sendBufSize size of socket's send buffer in bytes
 * @param rcvBufSize  size of socket's receive buffer in bytes
 * @param noDelay     0 if socket TCP_NODELAY is off, else it's on
 * @param fd          pointer which gets filled in with file descriptor
 * @param localPort   pointer which gets filled in with local (ephemeral) port number
 *
 * @returns ET/CMSG_OK                          if successful
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT   if ip_adress or fd args are NULL
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY   if out of memory
 * @returns ET_ERROR_SOCKET/CMSG_SOCKET_ERROR   if socket could not be created or socket options could not be set.
 * @returns ET_ERROR_NETWORK/CMSG_NETWORK_ERROR if host name could not be resolved or could not connect
 *
 */
int codanetTcpConnect(const char *ip_address, const char *interface, unsigned short port,
                      int sendBufSize, int rcvBufSize, int noDelay, int *fd, int *localPort)
{
    int                 sockfd, err=0, isDottedDecimal=0;
    const int           on=1;
    struct sockaddr_in  servaddr;
    int                 status;
    struct in_addr      **pptr;
    struct hostent      *hp;
    int h_errnop        = 0;

    if (ip_address == NULL || fd == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect: null argument(s)\n", codanetStr);
        return(CODA_BAD_ARGUMENT);
    }

    /* Check to see if ip_address is in dotted-decimal form. If so, use different
     * routine to process that address than if it were a name. */
    isDottedDecimal = codanetIsDottedDecimal(ip_address, NULL);
    if (isDottedDecimal) {
        uint32_t inetaddr;
        if (inet_pton(AF_INET, ip_address, &inetaddr) < 1) {
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sTcpConnect: unknown address for host %s\n", codanetStr, ip_address);
            }
            return(CODA_NETWORK_ERROR);
        }
        return codanetTcpConnect2(inetaddr, interface, port, sendBufSize, rcvBufSize, noDelay, fd, localPort);
    }

    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sTcpConnect: socket error, %s\n", codanetStr, strerror(errno));
        }
        return(CODA_SOCKET_ERROR);
    }

    /* don't wait for messages to cue up, send any message immediately */
    if (noDelay) {
        err = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &on, sizeof(on));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }

    /* set send buffer size unless default specified by a value <= 0 */
    if (sendBufSize > 0) {
        err = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &sendBufSize, sizeof(sendBufSize));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }

    /* set receive buffer size unless default specified by a value <= 0  */
    if (rcvBufSize > 0) {
        err = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &rcvBufSize, sizeof(rcvBufSize));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }

    /* set the outgoing network interface */
    if (interface != NULL && strlen(interface) > 0) {
        err = codanetSetInterface(sockfd, interface);
        if (err != CODA_OK) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sTcpConnect: error choosing network interface\n", codanetStr);
            }
            return(CODA_SOCKET_ERROR);
        }
    }

    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);

    /*
     * There seem to be serious bugs with Linux implementation of
     * gethostbyname_r. See:
     * http://curl.haxx.se/mail/lib-2003-10/0201.html
     * http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6369541
     *
     * Sooo, let's use the non-reentrant version and simply protect
     * with our own mutex.
    */
 
    /* make gethostbyname thread-safe */
    status = pthread_mutex_lock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Lock gethostbyname Mutex");
    }
   
    if ((hp = gethostbyname(ip_address)) == NULL) {
        status = pthread_mutex_unlock(&getHostByNameMutex);
        if (status != 0) {
            coda_err_abort(status, "Unlock gethostbyname Mutex");
        }
        close(sockfd);
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect: hostname error - %s\n",
        codanetStr, codanetHstrerror(h_errnop));
        return(CODA_NETWORK_ERROR);
    }
    pptr = (struct in_addr **) hp->h_addr_list;

    for ( ; *pptr != NULL; pptr++) {
        memcpy(&servaddr.sin_addr, *pptr, sizeof(struct in_addr));
        if ((err = connect(sockfd, (SA *) &servaddr, sizeof(servaddr))) < 0) {
            if (codanetDebug >= CODA_DEBUG_WARN) {
                fprintf(stderr, "%sTcpConnect: error attempting to connect to server, %s\n", codanetStr, strerror(errno));
            }
        }
        else {
            if (codanetDebug >= CODA_DEBUG_INFO) {
                fprintf(stderr, "%sTcpConnect: connected to server\n", codanetStr);
            }
            break;
        }
    }

    /* if there's no error, find & return the local port number of this socket */
    if (err != -1 && localPort != NULL) {
        socklen_t len;
        struct sockaddr_in ss;
      
        len = sizeof(ss);
        if (getsockname(sockfd, (SA *) &ss, &len) == 0) {
            *localPort = (int) ntohs(ss.sin_port);
        }
        else {
            *localPort = 0;
        }
    }
   
    status = pthread_mutex_unlock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Unlock gethostbyname Mutex");
    }

    if (err == -1) {
        close(sockfd);
        if (codanetDebug >= CODA_DEBUG_ERROR)
            fprintf(stderr, "%sTcpConnect: socket connect error\n", codanetStr);
        return(CODA_NETWORK_ERROR);
    }
  
    if (fd != NULL)  *fd = sockfd;
    return(CODA_OK);
}


/**
 * This routine makes a TCP connection to a server.
 *
 * @param inetaddr    binary numeric address of host to connect to
 * @param interface   interface (dotted-decimal ip address) to connect through
 * @param port        port to connect to
 * @param sendBufSize size of socket's send buffer in bytes
 * @param rcvBufSize  size of socket's receive buffer in bytes
 * @param noDelay     0 if socket TCP_NODELAY is off, else it's on
 * @param fd          pointer which gets filled in with file descriptor
 * @param localPort   pointer which gets filled in with local (ephemeral) port number
 *
 * @returns ET/CMSG_OK                          if successful
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT   if ip_adress or fd args are NULL
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY   if out of memory
 * @returns ET_ERROR_SOCKET/CMSG_SOCKET_ERROR   if socket could not be created or socket options could not be set.
 * @returns ET_ERROR_NETWORK/CMSG_NETWORK_ERROR if host name could not be resolved or could not connect
 *
 */
int codanetTcpConnect2(uint32_t inetaddr, const char *interface, unsigned short port,
                       int sendBufSize, int rcvBufSize, int noDelay, int *fd, int *localPort)
{
    int                 sockfd, err=0;
    const int           on=1;
    struct sockaddr_in  servaddr;

  
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR)
            fprintf(stderr, "%sTcpConnect2: socket error, %s\n", codanetStr, strerror(errno));
        return(CODA_SOCKET_ERROR);
    }
    
    /* don't wait for messages to cue up, send any message immediately */
    if (noDelay) {
        err = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &on, sizeof(on));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect2: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }
  
    /* set send buffer size unless default specified by a value <= 0 */
    if (sendBufSize > 0) {
        err = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &sendBufSize, sizeof(sendBufSize));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect2: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }
  
    /* set receive buffer size unless default specified by a value <= 0  */
    if (rcvBufSize > 0) {
        err = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &rcvBufSize, sizeof(rcvBufSize));
        if (err < 0) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect2: setsockopt error\n", codanetStr);
            return(CODA_SOCKET_ERROR);
        }
    }
    
    /* set the outgoing network interface */
    if (interface != NULL && strlen(interface) > 0) {
        err = codanetSetInterface(sockfd, interface);
        if (err != CODA_OK) {
            close(sockfd);
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sTcpConnect2: error choosing network interface\n", codanetStr);
            }
            return(CODA_SOCKET_ERROR);
        }
    }

    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);
    servaddr.sin_addr.s_addr = inetaddr;

    if ((err = connect(sockfd, (SA *) &servaddr, sizeof(servaddr))) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect2: error attempting to connect to server\n",
                                                      codanetStr);
    }
  
    /* if there's no error, find & return the local port number of this socket */
    if (err != -1 && localPort != NULL) {
        socklen_t len;
        struct sockaddr_in ss;
      
        len = sizeof(ss);
        if (getsockname(sockfd, (SA *) &ss, &len) == 0) {
            *localPort = (int) ntohs(ss.sin_port);
        }
        else {
            *localPort = 0;
        }
    }

    if (err == -1) {
        close(sockfd);
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sTcpConnect2: socket connect error, %s\n",
                                                      codanetStr, strerror(errno));
        return(CODA_NETWORK_ERROR);
    }
  
    if (fd != NULL)  *fd = sockfd;
    return(CODA_OK);
}


/**
 * Function to take a string IP address, either an alphabetic host name such as
 * mycomputer.jlab.org or one in presentation format such as 129.57.120.113,
 * and convert it to binary numeric format and place it in a sockaddr_in
 * structure.
 *
 * @param ip_address string IP address of a host
 * @param addr pointer to struct holding the binary numeric value of the host
 *
 * @returns ET/CMSG_OK                          if successful
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT   if ip_address is null
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY   if out of memory
 * @returns ET_ERROR_NETWORK/CMSG_NETWORK_ERROR if the numeric address could not be obtained/resolved
 */
int codanetStringToNumericIPaddr(const char *ip_address, struct sockaddr_in *addr)
{
    int isDottedDecimal=0, status;
    struct in_addr      **pptr;
    struct hostent      *hp;

    if (ip_address == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sStringToNumericIPaddr: null argument\n", codanetStr);
        return(CODA_BAD_ARGUMENT);
    }
  	
    /*
     * Check to see if ip_address is in dotted-decimal form. If so, use different
     * routines to process that address than if it were a name.
     */
    isDottedDecimal = codanetIsDottedDecimal(ip_address, NULL);

    if (isDottedDecimal) {
        if (inet_pton(AF_INET, ip_address, &addr->sin_addr) < 1) {
            return(CODA_NETWORK_ERROR);
        }
        return(CODA_OK);
    }

    /*
     * There seem to be serious bugs with Linux implementation of
     * gethostbyname_r. See:
     * http://curl.haxx.se/mail/lib-2003-10/0201.html
     * http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6369541
     * 
     * Sooo, let's use the non-reentrant version and simply protect
     * with our own mutex.
     */
 
    /* make gethostbyname thread-safe */
    status = pthread_mutex_lock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Lock gethostbyname Mutex");
    }
   
    if ((hp = gethostbyname(ip_address)) == NULL) {
        status = pthread_mutex_unlock(&getHostByNameMutex);
        if (status != 0) {
            coda_err_abort(status, "Unlock gethostbyname Mutex");
        }
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sStringToNumericIPaddr: hostname error - %s\n", codanetStr, codanetHstrerror(h_errno));
        }
        return(CODA_NETWORK_ERROR);
    }
  
    pptr = (struct in_addr **) hp->h_addr_list;

    for ( ; *pptr != NULL; pptr++) {
        memcpy(&addr->sin_addr, *pptr, sizeof(struct in_addr));
        break;
    }
   
    status = pthread_mutex_unlock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Unlock gethostbyname Mutex");
    }

    return(CODA_OK);
}


/**
 * This routine chooses a particular network interface for a TCP socket
 * by having the caller provide a dotted-decimal ip address. Port is set
 * to an ephemeral port.
 *
 * @param fd file descriptor for TCP socket
 * @param ip_address IP address in dotted-decimal form
 *
 * @returns ET/CMSG_OK                          if successful
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT   if ip_address is null
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY   if out of memory
 * @returns ET_ERROR_NETWORK/CMSG_NETWORK_ERROR if the numeric address could not be obtained/resolved
 */
int codanetSetInterface(int fd, const char *ip_address) {
    int err;
    struct sockaddr_in netAddr;
    
    memset(&netAddr, 0, sizeof(struct sockaddr_in));
    
    err = codanetStringToNumericIPaddr(ip_address, &netAddr);
    if (err != CODA_OK) {
        return err;
    }
    netAddr.sin_family = AF_INET; /* ipv4 */
    netAddr.sin_port = 0;         /* choose ephemeral port # */

    err = bind(fd, (struct sockaddr *) &netAddr, sizeof(netAddr));
    if (err != 0) perror("error in codanetSetInterface: ");
    return err;
}


/**
 * This routine is a convenient wrapper for the accept function
 * which returns a file descriptor of a socket from a client
 * connecting to this server.
 * 
 * @param fd   file descriptor of server's listening socket
 * @param sa pointer to struct to be filled with address of peer (connecting) socket
 * @param salenptr pointer to struct is a value-result
                   argument: it should initially contain the size of the structure pointed
                   to by sa; on return it will contain the actual length (in  bytes)  of
                   the address returned
 *
 * @returns file descriptor of connecting socket else -1 for error (errno is set)
 */
int codanetAccept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int  n;

  again:
    if ((n = accept(fd, sa, salenptr)) < 0) {

#ifdef	EPROTO
        if (errno == EPROTO || errno == ECONNABORTED) {
#else
        if (errno == ECONNABORTED) {
#endif
            goto again;
        }
        else {
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sAccept: errno = %d, err = %s\n", codanetStr, errno, strerror(errno));
            }
        }
    }
    
    return(n);
}


/**
 * This routine is a convenient wrapper for the writev function
 * which writes multiple buffers sequentially over a single file descriptor.
 * Will write all data unless error occurs.
 *
 * @param fd      file descriptor
 * @param iov     array of structs containing pointers to buffers to write
 * @param nbufs   size of iov array
 * @param iov_max maxmimum number iof iov structs to use in one writev call
 *
 * @returns total number of bytes written, else -1 if error (errno is set)
 */
int codanetTcpWritev(int fd, struct iovec iov[], int nbufs, int iov_max)
{
    struct iovec *iovp;
    int n_write, n_sent, nbytes, cc, i;
  
    /* determine total # of bytes to be sent */
    nbytes = 0;
    for (i=0; i < nbufs; i++) {
        nbytes += iov[i].iov_len;
    }
  
    n_sent = 0;
    while (n_sent < nbufs) {  
        n_write = ((nbufs - n_sent) >= iov_max)?iov_max:(nbufs - n_sent);
        iovp     = &iov[n_sent];
        n_sent  += n_write;
      
      retry:
        if ( (cc = writev(fd, iovp, n_write)) == -1) {
            if (errno == EWOULDBLOCK) {
                if (codanetDebug >= CODA_DEBUG_WARN) {
                    fprintf(stderr,"%sTcpWritev gives EWOULDBLOCK\n", codanetStr);
                }
                goto retry;
            }
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr,"%sTcpWritev(%d,,%d) = writev(%d,,%d) = %d\n",
                        codanetStr, fd, nbufs, fd, n_write, cc);
            }
            perror("xxxNetTcpWritev");
            return(-1);
        }
    }
    
    return(nbytes);
}


/**
 * This routine is a convenient wrapper for the write function
 * which writes a given number of bytes from a single buffer
 * over a single file descriptor. Will write all data unless
 * error occurs.
 *
 * @param fd      file descriptor
 * @param vptr    pointer to buffers to write
 * @param n       number of bytes to write
 *
 * @returns total number of bytes written, else -1 if error (errno is set)
 */
int codanetTcpWrite(int fd, const void *vptr, int n)
{
    int		nleft;
    int		nwritten;
    const char	*ptr;

    ptr = (char *) vptr;
    nleft = n;
  
    while (nleft > 0) {
        if ( (nwritten = write(fd, (char*)ptr, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0;		/* and call write() again */
            }
            else {
                return(nwritten);	/* error */
            }
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}


/**
 * This routine is a convenient wrapper for the read function
 * which read a given number of bytes into a single buffer
 * over a single file descriptor. Will read all data unless
 * error occurs.
 *
 * @param fd      file descriptor
 * @param vptr    pointer to buffers to read into
 * @param n       number of bytes to read
 *
 * @returns total number of bytes read, errno is set if error
 */
int codanetTcpRead(int fd, void *vptr, int n)
{
    int	nleft;
    int	nread;
    char	*ptr;

    ptr = (char *) vptr;
    nleft = n;
  
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            /*
            if (errno == EINTR)            fprintf(stderr, "call interrupted\n");
            else if (errno == EAGAIN)      fprintf(stderr, "non-blocking return, or socket timeout\n");
            else if (errno == EWOULDBLOCK) fprintf(stderr, "nonblocking return\n");
            else if (errno == EIO)         fprintf(stderr, "I/O error\n");
            else if (errno == EISDIR)      fprintf(stderr, "fd refers to directory\n");
            else if (errno == EBADF)       fprintf(stderr, "fd not valid or not open for reading\n");
            else if (errno == EINVAL)      fprintf(stderr, "fd not suitable for reading\n");
            else if (errno == EFAULT)      fprintf(stderr, "buffer is outside address space\n");
            else {perror("xxxNetTcpRead");}
            */
            if (errno == EINTR) {
                nread = 0;		/* and call read() again */
            }
            else {
                return(nread);
            }
        }
        else if (nread == 0) {
            break;			/* EOF */
        }
    
        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);		/* return >= 0 */
}


/**
 * This routine is used only in ET system.
 * Reads 3 ints from a nonblocking socket which are returned.
 * If they cannot be read within .1 sec, forget about it
 * since the client is probably not a valid one. Takes
 * care of converting network to host byte order.
 *
 * @param fd file descriptor fof nonblocking socket rom which to read
 * @param i1 pointer which gets filled with first int read
 * @param i2 pointer which gets filled with second int read
 * @param i3 pointer which gets filled with third int read
 * 
 * @returns ET/CMSG_OK    if successful
 * @returns ET/CMSG_ERROR if error
 */
int codanetTcpRead3iNB(int fd, int *i1, int *i2, int *i3)
{
    const int numBytes = 12;
    size_t  nleft = numBytes;
    ssize_t nread;
    char    buf[numBytes]; /* byte buffer */
    int     bufIndex = 0, loops = 10;
    struct timespec wait = {0, 10000000}; /* .01 sec */

    
    while (nleft > 0 && loops-- > 0) {

        if ( (nread = read(fd, &buf[bufIndex], nleft)) < 0) {
           
            if (errno != EWOULDBLOCK) {
                return(CODA_ERROR);   /* error reading */
            }
            
            /* try again after a .01 sec delay */
            nanosleep(&wait,NULL);
            continue;
            
        } else if (nread == 0) {
            return(CODA_ERROR);       /* EOF, but not done reading yet */
        }
    
        nleft -= nread;
        bufIndex += nread;
    }

    /* if we timed out ... */
    if (nleft > 0) {
        return(CODA_ERROR);
    }

    /* This takes care of converting network byte order */
    if (i1 != NULL) *i1 = ((buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8)  | buf[3]);
    if (i2 != NULL) *i2 = ((buf[4]<<24) | (buf[5]<<16) | (buf[6]<<8)  | buf[7]);
    if (i3 != NULL) *i3 = ((buf[8]<<24) | (buf[9]<<16) | (buf[10]<<8) | buf[11]);
    
    return(CODA_OK);
}


/**
 * This routine gets the byte order of the local host.
 *
 * @param pointer to int which gets filled with byte order
 *                (CMSG/ET_ENDIAN_BIG or CMSG/ET_ENDIAN_LITTLE)
 *
 * @returns ET/CMSG_OK    if successful
 * @returns ET/CMSG_ERROR if error
 */
int codanetLocalByteOrder(int *endian)
{
  union {
    short  s;
    char   c[sizeof(short)];
  } un;

  un.s = 0x0102;
  if (sizeof(short) == 2) {
    if (un.c[0] == 1 && un.c[1] == 2) {
      *endian = CODA_ENDIAN_BIG;
      return(CODA_OK);
    }
    else if (un.c[0] == 2 && un.c[1] == 1) {
      *endian = CODA_ENDIAN_LITTLE;
      return(CODA_OK);
    }
    else {
      if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sByteOrder: unknown endian\n", codanetStr);
      return(CODA_ERROR);
    }
  }
  else {
    if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sByteOrder: sizeof(short) = %u\n", codanetStr, (uint32_t)sizeof(short));
    return(CODA_ERROR);
  }
}


/**
 * This routine returns the local IP address associated with a socket.
 * Used only for IPv4 TCP sockets.
 *
 * @param sockfd socket file descriptor
 * @param ipAddress array of at least 16 characters
 *
 * @returns ET/CMSG_OK    if successful
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT if any arg has a bad value
 * @returns ET/CMSG_ERROR if error
 */
int codanetLocalSocketAddress(int sockfd, char *ipAddress)
{
    char *ip;
    struct sockaddr_in *sa;

    struct sockaddr_storage ss;
    socklen_t len = sizeof(ss);

    if (sockfd < 0 || ipAddress == NULL) return(CODA_BAD_ARGUMENT);
    
    if (getsockname(sockfd, (SA *) &ss, &len) < 0) {
        return (CODA_ERROR);
    }

    if (ss.ss_family == AF_INET) {
        sa = (struct sockaddr_in *) &ss;
        ip = inet_ntoa(sa->sin_addr);
        strncpy(ipAddress, ip, CODA_IPADDRSTRLEN-1);
    }
    else {
        return (CODA_ERROR);
    }

    return(CODA_OK);
}


/**
 * This routine tells whether the given ip address is in dot-decimal notation or not.
 *
 * @param ipAddress ip address in string form
 * @param decimals  pointer to array of 4 ints which gets filled with ip address ints if not NULL
 *                  and address is in dotted decimal form (left most first)
 *
 * @returns 1    if address is in dot decimal format
 * @returns 0    if address is <b>NOT</b> in dot decimal format
 */
int codanetIsDottedDecimal(const char *ipAddress, int *decimals)
{
    int i[4], j, err, isDottedDecimal = 0;

    if (ipAddress == NULL) return(0);
    
    err = sscanf(ipAddress, "%d.%d.%d.%d", &i[0], &i[1], &i[2], &i[3]);
    if (err == 4) {
        isDottedDecimal = 1;
        for (j=0; j < 4; j++) {
            if (i[j] < 0 || i[j] > 255) {
                isDottedDecimal = 0;
                break;
            }
        }
    }

    if (isDottedDecimal && decimals != NULL) {
        for (j=0; j < 4; j++) {
            *(decimals++) = i[j];
        }
    }

    return(isDottedDecimal);
}


/**
 * This routine tells whether the 2 given IP addresses in dot-decimal notation
 * are on the same subnet or not given a subnet mask in dot-decimal notation,
 *
 * @param ipAddress1 first  IP address in dot-decimal notation
 * @param ipAddress2 second IP address in dot-decimal notation
 * @param subnetMask subnet mask in dot-decimal notation for subnet of interest
 * @param sameSubnet pointer to int gets filled with 1 if addresses on same subnet, else 0
 *
 * @returns ET/CMSG_OK    if successful
 * @returns ET/CMSG_ERROR if args are NULL or not in dot-decimal form
 */
int codanetOnSameSubnet(const char *ipAddress1, const char *ipAddress2,
                        const char *subnetMask, int *sameSubnet)
{
    int msk[4];
    uint32_t mask;

    if (!codanetIsDottedDecimal(subnetMask, msk)) {
        return(CODA_ERROR);
    }

    mask  = (uint32_t) ((msk[0] << 24) | (msk[1] << 16) | (msk[2] << 8) | msk[3]);

    return codanetOnSameSubnet2(ipAddress1, ipAddress2, mask, sameSubnet);
}


/**
 * This routine tells whether the 2 given IP addresses in dot-decimal notation
 * are on the same subnet or not given a subnet mask in 32 bit binary form
 * (local byte order).
 *
 * @param ipAddress1 first  IP address in dot-decimal notation
 * @param ipAddress2 second IP address in dot-decimal notation
 * @param subnetMask subnet mask as LOCAL-byte-ordered 32 bit int
 * @param sameSubnet pointer to int gets filled with 1 if addresses on same subnet, else 0
 *
 * @returns ET/CMSG_OK    if successful
 * @returns ET/CMSG_ERROR if args are NULL or not in dot-decimal form
 */
int codanetOnSameSubnet2(const char *ipAddress1, const char *ipAddress2,
                         uint32_t subnetMask, int *sameSubnet)
{
    int ip1[4], ip2[4];
    uint32_t addr1, addr2;

    if (!codanetIsDottedDecimal(ipAddress1, ip1) ||
        !codanetIsDottedDecimal(ipAddress2, ip2) ||
        sameSubnet == NULL) {

        return(CODA_ERROR);
    }
    
    addr1 = (uint32_t) ((ip1[0] << 24) | (ip1[1] << 16) | (ip1[2] << 8) | ip1[3]);
    addr2 = (uint32_t) ((ip2[0] << 24) | (ip2[1] << 16) | (ip2[2] << 8) | ip2[3]);

    if ((addr1 & subnetMask) == (addr2 & subnetMask)) {
        *sameSubnet = 1;
    }
    else {
        *sameSubnet = 0;
    }

    return(CODA_OK);
}


/**
 * Routine that translates integer error into string.
 * 
 * @param err integer error
 * @returns error string
 */
const char *codanetHstrerror(int err)
{
    if (err == 0)
	    return("no error");

    if (err == HOST_NOT_FOUND)
	    return("Unknown host");

    if (err == TRY_AGAIN)
	    return("Temporary error on name server - try again later");

    if (err == NO_RECOVERY)
	    return("Unrecoverable name server error");

    if (err == NO_DATA)
    return("No address associated with name");

    return("unknown error");
}


/**
 * This routine finds out if the two given nodes are the same machine.
 * Comparison of names is unreliable as there can be more than one name
 * for a domain (as in cebaf.gov & jlab.org) or more that one name
 * for any host. Do a complete comparison by comparing the binary
 * address values.
 *
 * @param node1 first node
 * @param node2 second node
 * @param same pointer filled in with 1 if same, else 0
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if error finding host
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT if any arg is NULL
 */
int codanetNodeSame(const char *node1, const char *node2, int *same)
{
    struct hostent *hptr;
    char **pptr;
    /* save up to maxAddresses ip addresses for each node */
    int maxAddresses = 10;
    struct in_addr   node1addrs[maxAddresses], node2addrs[maxAddresses];
    int              n1addrs=0, n2addrs=0, i, j;
  
    if ((node1 == NULL) || (node2 == NULL) || same == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sNodeSame: bad argument(s)\n", codanetStr);
        return CODA_BAD_ARGUMENT;
    }
  
    /* do a quick check of name against name, it may work */
    if (strcmp(node1, node2) == 0) {
        *same = 1;
        return CODA_OK;
    }
   
    /* Since gethostbyname uses static data and gethostbyname_r
     * is buggy on linux, do things the hard way and save
     * the results into arrays so we don't overwrite data.
     */
   
    if ( (hptr = gethostbyname(node1)) == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sNodeSame: gethostbyname error\n", codanetStr);
        return CODA_ERROR;
    }
    for (pptr = hptr->h_addr_list; *pptr != NULL; pptr++) {
        node1addrs[n1addrs++] = *((struct in_addr *) *pptr);
        if (n1addrs > maxAddresses-1) break;
    }

    if ( (hptr = gethostbyname(node2)) == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sNodeSame: gethostbyname error\n", codanetStr);
        return CODA_ERROR;
    }
    for (pptr = hptr->h_addr_list; *pptr != NULL; pptr++) {
        node2addrs[n2addrs++] = *((struct in_addr *) *pptr);
        if (n2addrs > maxAddresses-1) break;
    }

    /* look through addresses for a match */
    for (i=0; i < n1addrs; i++) {
        for (j=0; j < n2addrs; j++) {
            if (node1addrs[i].s_addr == node2addrs[j].s_addr) {
                *same = 1;
                return CODA_OK;
             }
        }
    }
  
    *same = 0;
    return CODA_OK;
}


/**
 * This routine finds out if the given host is the same as the
 * local host or not.
 *
 * @param host hostname
 * @param isLocal pointer filled in with 1 if local host, else 0
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if could not find the host
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT if either argument is NULL
 */
int codanetNodeIsLocal(const char *host, int *isLocal)
{
    struct utsname myname;
    int status, same=0;
    int debugTemp, debug=0;
  
    debugTemp = codanetDebug;
    if (debug) codanetDebug = CODA_DEBUG_INFO;
  
    if (host == NULL || isLocal == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sNodeIsLocal: bad argument(s)\n", codanetStr);
        codanetDebug = debugTemp;
        return CODA_BAD_ARGUMENT;
    }

    /* find out the name of the machine we're on */
    if (uname(&myname) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sNodeIsLocal: cannot find hostname\n", codanetStr);
        codanetDebug = debugTemp;
        return CODA_ERROR;
    }

    if ( (status = codanetNodeSame(host, myname.nodename, &same)) != CODA_OK) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sNodeIsLocal: error in codanetNodeSame\n", codanetStr);
        codanetDebug = debugTemp;
        return status;
    }
  
    codanetDebug = debugTemp;
  
    if (same)
        *isLocal = 1;
    else
        *isLocal = 0;
  
    return CODA_OK;
}


/**
 * This routine returns the host name returned by "uname".
 *
 * @param host    pointer filled in with name of local host from uname
 * @param length  number of bytes in host arg
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if could not find the host name
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT if host argument is NULL or length < 2
 */
int codanetGetUname(char *host, int length)
{
    struct utsname myname;
  
    if (host == NULL || length < 2) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sGetUname: bad argument(s)\n", codanetStr);
        return CODA_BAD_ARGUMENT;
    }

    /* find out the name of the machine we're on */
    if (uname(&myname) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sGetUname: cannot find uname\n", codanetStr);
        return CODA_ERROR;
    }

    /* return the null-teminated uname */
    strncpy(host, myname.nodename, (size_t)length);
    host[length-1] = '\0';
  
    return CODA_OK;
}


/**
 * This routine returns the fully-qualified canonical host name of this host.
 * If not available, return host given by uname's node name.
 *
 * @param host    pointer filled in with fully-qualified canonical host name
 * @param length  number of bytes in host arg
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if could not find the host name
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT if host argument is NULL or length < 2
 */
int codanetLocalHost(char *host, int length)
{
    struct utsname myname;
    struct hostent *hptr;
    int status;
  
    if (host == NULL || length < 2) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sLocalHost: bad argument\n", codanetStr);
        return(CODA_BAD_ARGUMENT);
    }

    /* find out the name of the machine we're on */
    if (uname(&myname) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sLocalHost: cannot find hostname\n", codanetStr);
        return(CODA_ERROR);
    }
  
    /* make gethostbyname thread-safe */
    status = pthread_mutex_lock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Lock gethostbyname Mutex");
    }

    if ( (hptr = gethostbyname(myname.nodename)) == NULL) {
        /* return the null-teminated uname node name */
        strncpy(host, myname.nodename, (size_t)length);
        host[length-1] = '\0';
    }
    else {
        /* return the null-teminated canonical name */
        strncpy(host, hptr->h_name, (size_t)length);
        host[length-1] = '\0';
    }
  
    status = pthread_mutex_unlock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Unlock gethostbyname Mutex");
    }

    return(CODA_OK);
}


/**
 * This routine returns the default dotted-decimal address of this host.
 * Note: there must be 16 bytes available in host arg.
 *
 * @param host pointer filled in with the default dotted-decimal address of this host
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if could not find the host address
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT if host argument is NULL
 */
int codanetLocalAddress(char *address)
{
    struct utsname myname;
    struct hostent *hptr;
    char           **pptr, *val;
    int            status;
  
    if (address == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sLocalAddress: bad argument\n", codanetStr);
        return(CODA_BAD_ARGUMENT);
    }

    /* find out the name of the machine we're on */
    if (uname(&myname) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sLocalAddress: cannot find hostname\n", codanetStr);
        return(CODA_ERROR);
    }
  
    /* make gethostbyname thread-safe */
    status = pthread_mutex_lock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Lock gethostbyname Mutex");
    }

    if ( (hptr = gethostbyname(myname.nodename)) == NULL) {
        status = pthread_mutex_unlock(&getHostByNameMutex);
        if (status != 0) {
            coda_err_abort(status, "Unlock gethostbyname Mutex");
        }
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sLocalAddress: cannot find hostname\n", codanetStr);
        return(CODA_ERROR);
    }

    /* find address from hostent structure */
    pptr = hptr->h_addr_list;
    val  = inet_ntoa(*((struct in_addr *) *pptr));
  
    /* return the null-teminated dotted-decimal address */
    if (val == NULL) {
        status = pthread_mutex_unlock(&getHostByNameMutex);
        if (status != 0) {
            coda_err_abort(status, "Unlock gethostbyname Mutex");
        }
        return(CODA_ERROR);
    }
    strncpy(address, val, 16);
    address[15] = '\0';

    status = pthread_mutex_unlock(&getHostByNameMutex);
    if (status != 0) {
        coda_err_abort(status, "Unlock gethostbyname Mutex");
    }
  
    return(CODA_OK);
}


/******************************************************
 * The following few routines are taken from R. Stevens book,
 * UNIX Network Programming, Chapter 16. A few changes have
 * been made to simply things.
 * These are used for finding our broadcast addresses and
 * network interface hostnames.
 * Yes, we stole the code.
 ******************************************************/

/*****************************************************/
/* From Stevens chapter 3. Convert network address structure
 * into dotted-decimal IP address string (local static storage).
 */
static char *sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128]; /* Unix domain is largest */

    switch (sa->sa_family) {
        case AF_INET: {
            struct sockaddr_in  *sin = (struct sockaddr_in *) sa;
            if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL) {
                if (codanetDebug >= CODA_DEBUG_ERROR) {
                    fprintf(stderr, "sock_ntop_host: %s\n", strerror(errno));
                }
                return(NULL);
            }
            return(str);
        }

#if defined IPV6
        case AF_INET6: {
            struct sockaddr_in6  *sin6 = (struct sockaddr_in6 *) sa;

            if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL) {
                if (codanetDebug >= CODA_DEBUG_ERROR) {
                    fprintf(stderr, "sock_ntop_host: %s\n", strerror(errno));
                }
                return(NULL);
            }
            return(str);
        }
#endif

        default:
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "sock_ntop_host: unknown AF_xxx: %d, len %d", sa->sa_family, salen);
            }
    }
    return (NULL);
}


/**
 * This is a routine taken from Stevens chapter 21
 * (combined mcast_set_if & sockfd_to_family).
 * It sets a socket to multicast from a given interface.
 *
 * @param sockfd  socket file descriptor from which to multicast
 * @param ifname  interface name (e.g. eth0)
 * @param ifindex the index of a particular interface (starts at 1). This is more
 *                useful with IPv6 in which interfaces are numbered.
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if could not find the interface or set socket's properties
 * @returns ET_ERROR_BADARG/CMSG_BAD_ARGUMENT if host argument is NULL AND ifindex < 1
 */
int codanetMcastSetIf(int sockfd, const char *ifname, uint32_t ifindex) {
    int err;
    struct sockaddr_storage ss;
    socklen_t len = sizeof(ss);

    if (getsockname(sockfd, (SA *) &ss, &len) < 0) {
        return(CODA_ERROR);
    }

    switch (ss.ss_family) {
        case AF_INET: {
            struct in_addr      inaddr;
            struct ifreq        ifreq;

            if (ifindex > 0) {
                if (if_indextoname(ifindex, ifreq.ifr_name) == NULL) {
                    /* i/f index not found */
                    return(CODA_ERROR);
                }
                goto doioctl;
            } else if (ifname != NULL) {
                strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
doioctl:
                if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0) {
                    return(CODA_ERROR);
                }
                memcpy(&inaddr,
                       &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr,
                       sizeof(struct in_addr));
            } else
                inaddr.s_addr = htonl(INADDR_ANY);  /* remove prev. set default */

                err = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF,
                                 &inaddr, sizeof(struct in_addr));
                if (err < 0) return(CODA_SOCKET_ERROR);
                return(CODA_OK);
            }

#ifdef  IPV6
        case AF_INET6: {
            uint32_t   idx;

            if ( (idx = ifindex) < 1) {
                if (ifname == NULL) {
                    /* must supply either index or name */
                    return(CODA_BAD_ARGUMENT);
                }
                if ( (idx = if_nametoindex(ifname)) == 0) {
                    /* i/f name not found */
                    return(CODA_ERROR);
                }
            }
            err = setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &idx, sizeof(idx));
            if (err < 0) return(CODA_SOCKET_ERROR);
            return(CODA_OK);
        }
#endif

        default:
            return(CODA_ERROR);
    }
}


/**
 * This routine gets the network interface information about a computer.
 * Any returned non-NULL pointer must be freed with the routine
 * et/cMsgNetFreeInterfaceInfo().
 *
 * @param family socket family (AF_INET, AF_INET6, AF_LINK)
 * @param doaliases = 1 in order to include information on interfaces' aliases
 *                  (secondary names/addresses) as well
 *
 * @returns NULL if error
 * @returns pointer to ifi_info structure which is head of a linked list of
 *          structures containing network interface information
 */
#ifdef __APPLE__
struct ifi_info *codanetGetInterfaceInfo(int family, int doaliases)
{
    struct ifi_info    *ifi, *ifilast, *ifihead, **ifipnext;
    int                sockfd, len, lastlen, flags, myflags;
    char               *ptr, *buf, lastname[IFNAMSIZ], *cptr;
    struct ifconf      ifc;
    struct ifreq       *ifr, ifrcopy;
    struct sockaddr_in *sinptr;


    {
        struct ifaddrs *ifaddrsp;
        struct ifaddrs *theifaddrs;

        getifaddrs(&ifaddrsp);

        ifi     = NULL;
        ifihead = NULL;
        ifilast = NULL;

        for (theifaddrs = ifaddrsp; theifaddrs->ifa_next != NULL; theifaddrs=theifaddrs->ifa_next) {
            ifr = (struct ifreq *) ptr;

            /* for next one in buffer */
            switch (theifaddrs->ifa_addr->sa_family) {
  #ifdef IPV6
                case AF_INET6:
                    len = sizeof(struct sockaddr_in6);
                    break;
  #endif
                case AF_LINK:
                    len = sizeof(struct sockaddr);
                    continue;
                case AF_INET:
                    len = sizeof(struct sockaddr);
                    break;
                default:
                    len = sizeof(struct sockaddr);
                    continue;

            }

            myflags = 0;
            if ( (cptr = strchr(theifaddrs->ifa_name, ':')) != NULL) {
                *cptr = 0;    /* replace colon will null */
            }

            if (strncmp(lastname, theifaddrs->ifa_name, IFNAMSIZ) == 0) {
                if (doaliases == 0) {
                    /* continue;    already processed this interface */
                }

                myflags = IFI_ALIAS;
            }

            memcpy(lastname, theifaddrs->ifa_name, IFNAMSIZ);

            flags = theifaddrs->ifa_flags;

            /* ignore if interface not up */
            if ((flags & IFF_UP) == 0) {
                continue;
            }

            ifilast = ifi;

            ifi = calloc(1, sizeof(struct ifi_info));

            if (ifihead == NULL) ifihead = ifi;

            if (ifilast != NULL)
                ifilast->ifi_next = ifi;

            ifi->ifi_flags = flags;        /* IFF_xxx values */
            ifi->ifi_myflags = myflags;    /* IFI_xxx values */
            memcpy(ifi->ifi_name, theifaddrs->ifa_name, IFI_NAME);

            ifi->ifi_name[IFI_NAME-1] = '\0';

            sinptr = (struct sockaddr_in *) theifaddrs->ifa_addr;
            if (sinptr != NULL) {
                ifi->ifi_addr = calloc(1, sizeof(struct sockaddr_in));
                memcpy(ifi->ifi_addr, sinptr, sizeof(struct sockaddr_in));
            }

            sinptr = (struct sockaddr_in *) theifaddrs->ifa_broadaddr;

            if (sinptr != NULL) {
                ifi->ifi_brdaddr = calloc(1, sizeof(struct sockaddr));
                memcpy(ifi->ifi_brdaddr, sinptr, sizeof(struct sockaddr));
            }

        }
        freeifaddrs(ifaddrsp);
    }

    return(ifihead);    /* pointer to first structure in linked list */
}

#else
struct ifi_info *codanetGetInterfaceInfo(int family, int doaliases)
{
    struct ifi_info     *ifi, *ifihead, **ifipnext;
    int                 err, sockfd, lastlen, flags, myflags;
    size_t              len;
    char                *ptr, *buf, lastname[IFNAMSIZ], *cptr;
    struct ifconf       ifc;
    struct ifreq        *ifr, ifrcopy;
    struct sockaddr_in  *sinptr;
    struct sockaddr_in6 *sin6ptr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "et_get_ifi_info: socket error, %s.\n", strerror(errno));
        return NULL;
    }

    /* initial buffer size guess */
    len = 10 * sizeof(struct ifreq);
    lastlen = 0;
  
    for ( ; ; ) {
        buf = malloc(len);
        ifc.ifc_len = (int)len;
        ifc.ifc_buf = buf;
        
        if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {

            if (errno != EINVAL || lastlen != 0) {
fprintf(stderr, "et_get_ifi_info: ioctl error\n");
                close(sockfd);
                return NULL;
            }
        }
        else {
            if (ifc.ifc_len == lastlen) {
                /* success, len has not changed */
                break;
            }
            lastlen = ifc.ifc_len;
        }
        len += sizeof(struct ifreq);    /* increment */
        free(buf);
    }
      
    ifihead     = NULL;
    ifipnext    = &ifihead;
    lastname[0] = 0;

    for (ptr = buf; ptr < buf + ifc.ifc_len; ) {
        ifr = (struct ifreq *) ptr;

        switch (ifr->ifr_addr.sa_family) {
  #ifdef IPV6
            case AF_INET6:
                len = sizeof(struct sockaddr_in6);
                break;
  #endif
  #if !defined linux
            case AF_LINK:
                len = sizeof(struct sockaddr_dl);
                break;
  #endif
            case AF_INET:
            default:
                len = sizeof(struct sockaddr);
                break;
        }

        /* for next one in buffer */
        /*printf(" Name= %s  Family =0x%x  len = %d\n",  ifr->ifr_name,ifr->ifr_addr.sa_family,len);*/
        ptr += sizeof(struct ifreq);

        /* ignore if not desired address family */
        if (ifr->ifr_addr.sa_family != family) {
            continue;
        }

        myflags = 0;
        if ( (cptr = strchr(ifr->ifr_name, ':')) != NULL) {
                    *cptr = 0;    /* replace colon with null */
        }
        if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0) {
            if (doaliases == 0) {
                continue;    /* already processed this interface */
            }
            myflags = IFI_ALIAS;
        }
        memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

        ifrcopy = *ifr;
        ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
        flags = ifrcopy.ifr_flags;
        /* ignore if interface not up */
        if ((flags & IFF_UP) == 0) {
         continue;
        }

        ifi = calloc(1, sizeof(struct ifi_info));
        *ifipnext = ifi;               /* prev points to this new one */
        ifipnext  = &ifi->ifi_next;    /* pointer to next one goes here */

        ifi->ifi_flags = (short)flags;        /* IFF_xxx values */
        ifi->ifi_myflags = (short)myflags;    /* IFI_xxx values */
        memcpy(ifi->ifi_name, ifr->ifr_name, IFI_NAME);
        ifi->ifi_name[IFI_NAME-1] = '\0';

        switch (ifr->ifr_addr.sa_family) {
            case AF_INET:
                sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
                if (ifi->ifi_addr == NULL) {
                    ifi->ifi_addr = calloc(1, sizeof(struct sockaddr_in));
                    memcpy(ifi->ifi_addr, sinptr, sizeof(struct sockaddr_in));

                    if (flags & IFF_BROADCAST) {
                        ioctl(sockfd, SIOCGIFBRDADDR, &ifrcopy);
                        sinptr = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
                        ifi->ifi_brdaddr = calloc(1, sizeof(struct sockaddr_in));
/*printf("et_get_ifi_info: Broadcast addr = %s\n", inet_ntoa(sinptr->sin_addr));*/
                        memcpy(ifi->ifi_brdaddr, sinptr, sizeof(struct sockaddr_in));
                    }

                    if (flags & IFF_POINTOPOINT) {
                        ioctl(sockfd, SIOCGIFDSTADDR, &ifrcopy);
                        sinptr = (struct sockaddr_in *) &ifrcopy.ifr_dstaddr;
                        ifi->ifi_dstaddr = calloc(1, sizeof(struct sockaddr_in));
                        memcpy(ifi->ifi_dstaddr, sinptr, sizeof(struct sockaddr_in));
                    }

                    /* Get the subnet mask as well (added by Timmer) */
                    err = ioctl(sockfd, SIOCGIFNETMASK, &ifrcopy);
                    if (err == 0) {
                        sinptr = (struct sockaddr_in *) &ifrcopy.ifr_addr;
                        ifi->ifi_netmask = calloc(1, sizeof(struct sockaddr_in));
/*printf("et_get_ifi_info: Subnet mask = %s\n", inet_ntoa(sinptr->sin_addr));*/
                        memcpy(ifi->ifi_netmask, sinptr, sizeof(struct sockaddr_in));
                    }
                    
                }
                break;

  #ifdef IPV6
            case AF_INET6:
                sin6ptr = (struct sockaddr_in6 *) &ifr->ifr_addr;
                ifi->ifi_addr = calloc(1, sizeof(struct sockaddr_in6));
                memcpy(ifi->ifi_addr, sin6ptr, sizeof(struct sockaddr_in6));
                break;
  #endif
            default:
                break;
        }
    }

    free(buf);
    close(sockfd);
    return(ifihead);        /* pointer to first structure in linked list */
}
#endif


/**
 * This routine frees the network interface information returned
 * by et/cMsgNetGetInterfaceInfo.
 *
 * @param ifihead pointer to network info returned by et/cMsgNetGetInterfaceInfo
 */
void codanetFreeInterfaceInfo(struct ifi_info *ifihead)
{
    struct ifi_info  *ifi, *ifinext;

    for (ifi = ifihead; ifi != NULL; ifi = ifinext) {
        if (ifi->ifi_addr != NULL) {
            free(ifi->ifi_addr);
        }
        if (ifi->ifi_brdaddr != NULL) {
            free(ifi->ifi_brdaddr);
        }
        if (ifi->ifi_dstaddr != NULL) {
            free(ifi->ifi_dstaddr);
        }
        if (ifi->ifi_netmask != NULL) {
            free(ifi->ifi_netmask);
        }
        ifinext = ifi->ifi_next;        /* can't fetch ifi_next after free() */
        free(ifi);                      /* the ifi_info{} itself */
    }
}


/**
 * This routine frees allocated memory of a linked list containing IP info
 * obtained from calling et/cMsgGetNetworkInfo().
 *
 * @param ipaddr pointer to first element of linked list to be freed
 */
void codanetFreeIpAddrs(codaIpAddr *ipaddr) {
    int i;
    codaIpAddr *next;
  
    while (ipaddr != NULL) {
        next = ipaddr->next;
        if (ipaddr->aliases != NULL) {
            for (i=0; i<ipaddr->aliasCount; i++) {
                free(ipaddr->aliases[i]);
            }
            free(ipaddr->aliases);
        }
        free(ipaddr);
        ipaddr = next;
    }
}


/**
 * This routine finds all IP addresses, their names, subnets, & subnet masks of this host
 * and returns the data in the arguments. Only the first item of the returned linked
 * list or array will have the canonical name and aliases.
 *
 * @param ipaddrs address of pointer to a struct to hold IP data in linked list.
 *                If NULL, nothing is returned here.
 * @param ipinfo  pointer to struct to hold IP data in a fixed size array for
 *                storage in shared memory. This array is CODA_MAXADDRESSES in size.
 *                If NULL, nothing is returned here.
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if error
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY if no more memory
 */
int codanetGetNetworkInfo(codaIpAddr **ipaddrs, codaNetInfo *info)
{
    struct ifi_info   *ifi, *ifihead;
    struct sockaddr   *sa;
    struct hostent    *hptr;
    int               i;
    char              **pptr, *pChar, host[CODA_MAXHOSTNAMELEN];
    codaIpAddr         *ipaddr=NULL, *prev=NULL, *first=NULL;
  
  
    /* get fully qualified canonical hostname of this host */
    codanetLocalHost(host, CODA_MAXHOSTNAMELEN);

    /* look through IPv4 interfaces */
    ifihead = ifi = codanetGetInterfaceInfo(AF_INET, 1);
    if (ifi == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sGetNetworkInfo: cannot find network interface info\n", codanetStr);
        }
        return CODA_ERROR;
    }

    for (;ifi != NULL; ifi = ifi->ifi_next) {
        /* ignore loopback interface */
        if (ifi->ifi_flags & IFF_LOOPBACK) {
            continue;
        }
        
        /* if the interface is up */
        if (ifi->ifi_flags & IFF_UP) {
    
            /* allocate space for IP data */
            ipaddr = (codaIpAddr *)calloc(1, sizeof(codaIpAddr));
            if (ipaddr == NULL) {
                if (first != NULL) codanetFreeIpAddrs(first);
                codanetFreeInterfaceInfo(ifihead);
                if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sGetNetworkInfo: no memory\n", codanetStr);
                return CODA_OUT_OF_MEMORY;
            }
            /* string IP address info structs in linked list */
            if (prev != NULL) {
                prev->next = ipaddr;
            }
            else {
                first = ipaddr;
            }
            prev = ipaddr;
   
            /* if there is an address listed ... */
            if ( (sa = ifi->ifi_addr) != NULL) {
                ipaddr->saddr = *((struct sockaddr_in *) sa);    /* copy it */
                /* copy IP address - only 1 per loop */
                if ((pChar = sock_ntop_host(sa, sizeof(*sa))) != NULL) {
                    strncpy(ipaddr->addr, pChar, CODA_IPADDRSTRLEN-1);
                    if (codanetDebug >= CODA_DEBUG_INFO) {
                        printf("%sGetNetworkInfo address   : %s\n", codanetStr, pChar);
                    }
                }
            }
            
            /* if there is a subnet mask listed ... */
            if ( (sa = ifi->ifi_netmask) != NULL) {
                ipaddr->netmask = *((struct sockaddr_in *) sa);    /* copy it */
            }
            
            /* if the interface is broadcast enabled */
            if ((ifi->ifi_flags & IFF_BROADCAST) > 0) {
                /* if there is a broadcast (subnet) address listed ... */
                if ( (sa = ifi->ifi_brdaddr) != NULL) {
                    if ((pChar = sock_ntop_host(sa, sizeof(*sa))) != NULL) {
                        strncpy(ipaddr->broadcast, pChar, CODA_IPADDRSTRLEN-1);
                        if (codanetDebug >= CODA_DEBUG_INFO) {
                            printf("%sGetNetworkInfo broadcast : %s\n", codanetStr, pChar);
                        }
                    }
                }
            }

        } /* if interface is up */
        
        if (codanetDebug >= CODA_DEBUG_INFO) printf("\n");
        
    } /* for each interface */
  
    /* free memory */
    codanetFreeInterfaceInfo(ifihead);

    /* At this point we have most of the info we can get but lack 2 things: 1) host's canonical name,
     * and 2) a list of name aliases. Both of these can be obtained from gethostbyaddr(). */
    ipaddr = first;
    
    while (ipaddr != NULL) {

        /* try gethostbyaddr for each address until one succeeds */

        hptr = gethostbyaddr((const char *)&ipaddr->saddr.sin_addr,
                              sizeof(struct in_addr), AF_INET);
        /* Occasionally, hptr is NULL since there is no OS data about an address,
         * even though nothing is wrong and the error returned is HOST_NOT_FOUND.
         * Just ignore it and try with the next item. */
        if (hptr == NULL) {
            if (codanetDebug >= CODA_DEBUG_WARN) {
                fprintf(stderr, "%sGetNetworkInfo: error in gethostbyaddr, %s\n", codanetStr, hstrerror(h_errno));
            }
        }

        else {

            /* copy canonical name ... */
            if (hptr->h_name != NULL) {
                strncpy(first->canon, hptr->h_name, CODA_MAXHOSTNAMELEN-1);
                if (codanetDebug >= CODA_DEBUG_INFO) {
                    printf("%sGetNetworkInfo canon name: %s\n", codanetStr, hptr->h_name);
                }
            }

            /* copy aliases, but first count them */
            for (pptr= hptr->h_aliases; *pptr != NULL; pptr++) {
                first->aliasCount++;
            }

            if (first->aliasCount > 0) {
                first->aliases = (char **)calloc((size_t)first->aliasCount, sizeof(char *));
                if (first->aliases == NULL) {
                    codanetFreeIpAddrs(first);
                    codanetFreeInterfaceInfo(ifihead);
                    if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sGetNetworkInfo: no memory\n", codanetStr);
                    return CODA_OUT_OF_MEMORY;
                }
            }

            i=0;
            for (pptr= hptr->h_aliases; *pptr != NULL; pptr++) {
                first->aliases[i] = strdup(*pptr);
                if (first->aliases[i] == NULL) {
                    first->aliasCount = i;
                    codanetFreeIpAddrs(first);
                    codanetFreeInterfaceInfo(ifihead);
                    if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sGetNetworkInfo: no memory\n", codanetStr);
                    return CODA_OUT_OF_MEMORY;
                }
                i++;
                if (codanetDebug >= CODA_DEBUG_INFO) {
                    printf("%sGetNetworkInfo alias #%d  : %s\n", codanetStr, i, *pptr);
                }
            }

            /* need only do this once */
            break;
        }

        ipaddr = ipaddr->next;
    }
  
    /* ************************ */
    /* send back data to caller */
    /* ************************ */
  
    /* copy everything into a fixed-size array of structures for use in shared memory */
    if (info != NULL) {
        int j;
        ipaddr = first;
        i=0;
    
        while (ipaddr != NULL) {
            /* look at no more than CODA_MAXADDRESSES IP addresses */
            if (i >= CODA_MAXADDRESSES) break;
            
            info->ipinfo[i].saddr.sin_addr.s_addr   = ipaddr->saddr.sin_addr.s_addr;
            info->ipinfo[i].netmask.sin_addr.s_addr = ipaddr->netmask.sin_addr.s_addr;
            
            if (ipaddr->addr  != NULL) strcpy(info->ipinfo[i].addr, ipaddr->addr);
            else  info->ipinfo[i].addr[0] = '\0';
            
            if (ipaddr->canon != NULL) strcpy(info->ipinfo[i].canon, ipaddr->canon);
            else  info->ipinfo[i].canon[0] = '\0';
            
            if (ipaddr->broadcast != NULL) strcpy(info->ipinfo[i].broadcast, ipaddr->broadcast);
            else  info->ipinfo[i].broadcast[0] = '\0';
            
            for (j=0; j < ipaddr->aliasCount; j++) {
                /* look at no more than CODA_MAXADDRESSES aliases */
                if (j >= CODA_MAXADDRESSES) break;
                strcpy(info->ipinfo[i].aliases[j], ipaddr->aliases[j]);
            }
            info->ipinfo[i].aliasCount = j;
            
            i++;
            ipaddr = ipaddr->next;
        }
        info->count = i;
    }
  
    if (ipaddrs != NULL) {
        *ipaddrs = first;
    }
    else {
        if (first != NULL) codanetFreeIpAddrs(first);
    }
  
    return CODA_OK;
}


/**
 * This routine frees allocated memory of a linked list containing IP addresses.
 * @param addr pointer to first element of linked list to be freed
` */
void codanetFreeAddrList(codaIpList *addr) {
    codaIpList *next;
    while (addr != NULL) {
        next = addr->next;
        free(addr);
        addr = next;
    }
}


/**
 * This routine finds all broadcast addresses, eliminates duplicates and
 * returns the data in the arguments.
 *
 * @param addrs   address of pointer to a struct to hold broadcast data in linked list.
 *                If NULL, nothing is returned here.
 * @param bcaddrs pointer to struct to hold broadcast data in a fixed size array for storage
 *                in shared memory. This array allows only CODA_MAXADDRESSES number of addresses.
 *                If NULL, nothing is returned here.
 *
 * @returns ET/CMSG_OK if successful
 * @returns ET/CMSG_ERROR if error
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY if no more memory
 */
int codanetGetBroadcastAddrs(codaIpList **addrs, codaDotDecIpAddrs *bcaddrs)
{
    char  *p;
    int    index, count=0, skip;
    struct ifi_info *ifi, *ifihead;
    struct sockaddr *sa;
    codaIpList      *baddr=NULL, *first=NULL, *prev=NULL, *paddr;
  
  
    /* look through IPv4 interfaces */
    ifihead = ifi = codanetGetInterfaceInfo(AF_INET, 1);
    if (ifi == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sGetBroadcastAddrs: cannot find network interface info\n", codanetStr);
        }
        return CODA_ERROR;
    }

    for (;ifi != NULL; ifi = ifi->ifi_next) {
        /* ignore loopback */
        if (ifi->ifi_flags & IFF_LOOPBACK) {
            continue;
        }
    
        /* if the interface is up & broadcast enabled */
        if ( (ifi->ifi_flags & IFF_UP) && ((ifi->ifi_flags & IFF_BROADCAST) > 0) ) {
    
            if ( (sa = ifi->ifi_brdaddr) != NULL) {
                p = sock_ntop_host(sa, sizeof(*sa));
        
                /* check to see if we already got this one */
                skip  = 0;
                index = 0;
                paddr = first;
                while (index++ < count) {
                    if (strcmp(p, paddr->addr) == 0) {
                        skip++;
                        break;
                    }
                    paddr = paddr->next;
                }
                if (skip) continue;
        
                /* allocate space for broadcast data */
                baddr = (codaIpList *)calloc(1, sizeof(codaIpList));
                if (baddr == NULL) {
                    if (first != NULL) codanetFreeAddrList(first);
                    codanetFreeInterfaceInfo(ifihead);
                    if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sGetBroadcastAddrs: no memory\n", codanetStr);
                    return CODA_OUT_OF_MEMORY;
                }

                /* string address structs in linked list */
                if (prev != NULL) {
                    prev->next = baddr;
                }
                else {
                    first = baddr;
                }
                prev = baddr;

                strncpy(baddr->addr, p, CODA_IPADDRSTRLEN-1);
                count++;
                if (codanetDebug >= CODA_DEBUG_INFO) {
                    printf("%sGetBroadcastAddrs broadcast : %s\n", codanetStr, sock_ntop_host(sa, sizeof(*sa)));
                }
            }
        } /* if interface is up */
    } /* for each interface */
    
    if (codanetDebug >= CODA_DEBUG_INFO) printf("\n");
  
    /* free memory */
    codanetFreeInterfaceInfo(ifihead);
  
    /* ************************ */
    /* send back data to caller */
    /* ************************ */
  
    /* copy everything into 1 structure for use in shared memory */
    if (bcaddrs != NULL) {
        paddr = first;
        bcaddrs->count = 0;
    
        while (paddr != NULL) {
            if (bcaddrs->count >= CODA_MAXADDRESSES) break;
            strcpy(bcaddrs->addr[bcaddrs->count++], paddr->addr);
            paddr = paddr->next;
        }
    }
  
    if (addrs != NULL) {
        *addrs = first;
    }
    else {
        if (first != NULL) codanetFreeAddrList(first);
    }
  
    return CODA_OK;
}


/**
 * This routine returns an allocated array of local network interfaces names
 * and the array size in the arguments. To free all allocated memory,
 * free each of ifNames' count elements, then free ifNames itself.
 *
 * @param ifNames pointer which gets filled with allocated array of local interfaces names.
 *                If NULL, nothing is returned here.
 * @param count   number of names returned.
 *                If NULL, nothing is returned here.
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if cannot find network interface info
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY if no more memory
 */
int codanetGetIfNames(char ***ifNames, int *count) {

    char   **array;
    int    index=0, numIfs=0;
    struct ifi_info *ifi, *ifihead;

    /* look through IPv4 interfaces - no aliases */
    ifihead = ifi = codanetGetInterfaceInfo(AF_INET, 0);
    if (ifi == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sGetIfNames: cannot find network interface info\n", codanetStr);
        }
        return CODA_ERROR;
    }

    /* first count # of interfaces */
    for (;ifi != NULL; ifi = ifi->ifi_next) {
        /* ignore loopback */
        if (ifi->ifi_flags & IFF_LOOPBACK) {
            continue;
        }
    
        /* if the interface is up */
        if (ifi->ifi_flags & IFF_UP) {
            numIfs++;
        }
    }

    if (numIfs < 1) {
        if (count != NULL)   *count = 0;
        if (ifNames != NULL) *ifNames = NULL;
        codanetFreeInterfaceInfo(ifihead);
        return CODA_OK;
    }

    /* allocate mem for array */
    array = (char **) malloc(numIfs*sizeof(char *));
    if (array == NULL) {
        codanetFreeInterfaceInfo(ifihead);
        return CODA_OUT_OF_MEMORY;
    }
    
    ifi = ifihead;
    for (;ifi != NULL; ifi = ifi->ifi_next) {
        /* ignore loopback */
        if (ifi->ifi_flags & IFF_LOOPBACK) {
            continue;
        }
    
        /* if the interface is up */
        if (ifi->ifi_flags & IFF_UP) {
            array[index++] = strdup(ifi->ifi_name);
        }
    }

    /* ************************ */
    /* send back data to caller */
    /* ************************ */
    if (count != NULL)   *count = numIfs;
    if (ifNames != NULL) *ifNames = array;
        
    /* free memory */
    codanetFreeInterfaceInfo(ifihead);
    
    return CODA_OK;
}


/**
 * Given a dot-decimal IP address as an argument, this method will return that
 * if it matches one of the local IP addresses. Given a broadcast or subnet
 * address it will return a local IP address on that subnet. If there are no
 * matches, NULL is returned. Non-NULL returned string must be freed.
 *
 * @param ip IP or subnet address in dot-decimal format
 * @param matchingIp pointer to string filled in with matching local address
 *                   on the same subnet; else null if no match or no local IP
 *                   addresses found
 *
 * @returns ET/CMSG_OK            if successful
 * @returns ET/CMSG_BAD_ARGUMENT  if either arg is null or ip not in dot-decimal format
 */
int codanetGetMatchingLocalIpAddress(char *ip, char **matchingIp) {
    
    char *pChar;
    int isDottedDecimal = 0;
    struct sockaddr *sa;
    struct ifi_info *ifi, *ifihead;
    
    if (ip == NULL || matchingIp == NULL) {
        return CODA_BAD_ARGUMENT;
    }
    
    /* Check to see if ip arg is in dot-decimal form. If not, return error. */
    isDottedDecimal = codanetIsDottedDecimal(ip, NULL);
    if (!isDottedDecimal) return CODA_BAD_ARGUMENT;
    
    /* Look through IPv4 interfaces + all aliases */
    ifihead = ifi = codanetGetInterfaceInfo(AF_INET, 1);
    if (ifi == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sGetMatchingLocalIpAddress: cannot find network interface info\n", codanetStr);
        }
        return CODA_ERROR;
    }
        
    /* First check to see if it matches a local IP address, if so return it */
    ifi = ifihead;
    for (;ifi != NULL; ifi = ifi->ifi_next) {
        /* ignore loopback or down interface */
        if ((ifi->ifi_flags & IFF_LOOPBACK) || !(ifi->ifi_flags & IFF_UP)) {
            continue;
        }
        
        /* if there is an address listed ... */
        if ( (sa = ifi->ifi_addr) != NULL) {
            pChar = sock_ntop_host(sa, sizeof(*sa));
            
            /* Compare IP addresses. If match, return it */
            if (strcmp(ip, pChar) == 0) {
                *matchingIp = strdup(ip);
                codanetFreeInterfaceInfo(ifihead);
                return CODA_OK;
            }
        }
    }
    
    
    /* Next check to see if it matches a local subnet address,
       if so return an IP on that subnet. */
    ifi = ifihead;
    for (;ifi != NULL; ifi = ifi->ifi_next) {
        /* ignore loopback or down interface */
        if ((ifi->ifi_flags & IFF_LOOPBACK) || !(ifi->ifi_flags & IFF_UP)) {
            continue;
        }
        
        /* if there is a broadcast/subnet address listed ... */
        if ( (sa = ifi->ifi_brdaddr) != NULL) {
            pChar = sock_ntop_host(sa, sizeof(*sa));
            
            /* Compare. If match, return associated IP address */
            if (strcmp(pChar, ip) == 0) {
                
                if ( (sa = ifi->ifi_addr) != NULL) {
                    pChar = sock_ntop_host(sa, sizeof(*sa));
                    *matchingIp = strdup(pChar);
                    codanetFreeInterfaceInfo(ifihead);
                    return CODA_OK;
                }
            }
        }
    }
    
    /* free memory */
    codanetFreeInterfaceInfo(ifihead);
        
    /* no match */
    *matchingIp = NULL;
    return CODA_OK;
}


/**
 * Given a local IP address as an argument, this routine will return its
 * broadcast or subnet address. If it already is a broadcast address,
 * that is returned. If address is not local or none can be found,
 * NULL is returned. Non-NULL returned string must be freed.
 *
 * @param ip IP or subnet address in dot-decimal format
 * @param broadcastIp pointer to string filled in with matching local broadcast/subnet
 *                    address; else NULL if no match or no local IP addresses found
 *
 * @returns ET/CMSG_OK            if successful
 * @returns ET/CMSG_BAD_ARGUMENT  if either arg is null or ip not in dot-decimal format
 */
int codanetGetBroadcastAddress(char *ip, char **broadcastIp) {
    
    char *pChar;
    int isDottedDecimal = 0;
    struct sockaddr *sa;
    struct ifi_info *ifi, *ifihead;
    
    if (ip == NULL || broadcastIp == NULL) {
        return CODA_BAD_ARGUMENT;
    }
    
    /* Check to see if ip arg is in dot-decimal form. If not, return error. */
    isDottedDecimal = codanetIsDottedDecimal(ip, NULL);
    if (!isDottedDecimal) return CODA_BAD_ARGUMENT;
    
    /* Look through IPv4 interfaces + all aliases */
    ifihead = ifi = codanetGetInterfaceInfo(AF_INET, 1);
    if (ifi == NULL) {
        if (codanetDebug >= CODA_DEBUG_ERROR) {
            fprintf(stderr, "%sGetBroadcastAddress: cannot find network interface info\n", codanetStr);
        }
        return CODA_ERROR;
    }
    
    /* First check to see if it matches a local broadcast address, if so return it */
    ifi = ifihead;
    for (;ifi != NULL; ifi = ifi->ifi_next) {
        /* ignore loopback or down interface */
        if ((ifi->ifi_flags & IFF_LOOPBACK) || !(ifi->ifi_flags & IFF_UP)) {
            continue;
        }
        
        /* if there is a broadcast address listed ... */
        if ( (sa = ifi->ifi_brdaddr) != NULL) {
            pChar = sock_ntop_host(sa, sizeof(*sa));
            
            /* Compare IP addresses. If match, return it */
            if (strcmp(ip, pChar) == 0) {
                *broadcastIp = strdup(ip);
                codanetFreeInterfaceInfo(ifihead);
                return CODA_OK;
            }
        }
    }
    
    
    /* Next check to see if it matches a local IP address,
       if so return its associated broadcast address. */
    ifi = ifihead;
    for (;ifi != NULL; ifi = ifi->ifi_next) {
        /* ignore loopback or down interface */
        if ((ifi->ifi_flags & IFF_LOOPBACK) || !(ifi->ifi_flags & IFF_UP)) {
            continue;
        }
        
        /* if there is an IP address listed ... */
        if ( (sa = ifi->ifi_addr) != NULL) {
            pChar = sock_ntop_host(sa, sizeof(*sa));
            
            /* Compare. If match, return associated broadcast address */
            if (strcmp(pChar, ip) == 0) {
                
                if ( (sa = ifi->ifi_brdaddr) != NULL) {
                    pChar = sock_ntop_host(sa, sizeof(*sa));
                    *broadcastIp = strdup(pChar);
                    codanetFreeInterfaceInfo(ifihead);
                    return CODA_OK;
                }
            }
        }
    }
    
    /* free memory */
    codanetFreeInterfaceInfo(ifihead);
    
    /* no match */
    *broadcastIp = NULL;
    return CODA_OK;
}


/**
 * This routine returns an allocated array of dotted-decimal IP addresses
 * and the array size in the arguments. To free all allocated memory,
 * free each of ipAddrs' count elements, then free ipAddrs itself.
 *
 * @param ipAddrs pointer which gets filled with allocated array of dotted-decimal IP addresses.
 *                If NULL, nothing is returned here.
 * @param count   number of addresses returned.
 *                If NULL, nothing is returned here.
 * @param host    if NULL, return addressess of local host, else return addresses of given host.
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if cannot find network interface or host info
 * @returns ET_ERROR_NOMEM/CMSG_OUT_OF_MEMORY if no more memory
 */
int codanetGetIpAddrs(char ***ipAddrs, int *count, char *host) {

    static char str[128];
    char   **array=NULL, *pChar;
    int    err, status, index=0, numIps=0, hostIsLocal=0, h_errnop=0;
    struct sockaddr *sa;
    struct ifi_info *ifi, *ifihead;
    struct in_addr  **pptr;
    struct hostent  *hp;

    
    if (host != NULL) {
        err = codanetNodeIsLocal(host, &hostIsLocal);
        if (err != CODA_OK) {
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sGetIpaddrs: cannot find out if %s is local or not\n", codanetStr, host);
            }
            return CODA_ERROR;
        }
    }
    else {
        hostIsLocal = 1;
    }

    if (!hostIsLocal) {
        /* make gethostbyname thread-safe */
        status = pthread_mutex_lock(&getHostByNameMutex);
        if (status != 0) {
            coda_err_abort(status, "Lock gethostbyname Mutex");
        }

        if ((hp = gethostbyname(host)) == NULL) {
            status = pthread_mutex_unlock(&getHostByNameMutex);
            if (status != 0) {
                coda_err_abort(status, "Unlock gethostbyname Mutex");
            }
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sGetIpaddrs: hostname error - %s\n", codanetStr, codanetHstrerror(h_errnop));
            }
            return(CODA_NETWORK_ERROR);
        }
  
        pptr = (struct in_addr **) hp->h_addr_list;

        if (hp->h_addrtype == AF_INET) {
            /* first count the # of addresses */
            for ( ; *pptr != NULL; pptr++) {
                numIps++;
            }

            /* if none, return */
            if (numIps < 1) {
                if (count != NULL)   *count = 0;
                if (ipAddrs != NULL) *ipAddrs = NULL;
                return CODA_OK;
            }

            /* allocate mem for array */
            array = (char **) malloc(numIps*sizeof(char *));
            if (array == NULL) {
                return CODA_OUT_OF_MEMORY;
            }

            pptr = (struct in_addr **) hp->h_addr_list;
            for ( ; *pptr != NULL; pptr++) {
                if (inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)) != NULL) {
                    array[index++] = strdup(str);
                    if (codanetDebug >= CODA_DEBUG_INFO) {
                        printf("%sGetIpaddrs address : %s\n", codanetStr, str);
                    }
                }
            }
        }

        status = pthread_mutex_unlock(&getHostByNameMutex);
        if (status != 0) {
            coda_err_abort(status, "Unlock gethostbyname Mutex");
        }
    }
    
    /* else the host is local. */
    else {
        /* Look through IPv4 interfaces + all aliases */
        ifihead = ifi = codanetGetInterfaceInfo(AF_INET, 1);
        if (ifi == NULL) {
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sGetIpaddrs: cannot find network interface info\n", codanetStr);
            }
            return CODA_ERROR;
        }

        /* first count # of IP addresses */
        for (;ifi != NULL; ifi = ifi->ifi_next) {
            /* ignore loopback */
            if (ifi->ifi_flags & IFF_LOOPBACK) {
                continue;
            }
    
            /* if the interface is up */
            if (ifi->ifi_flags & IFF_UP) {
                numIps++;
            }
        }

        /* if none, return */
        if (numIps < 1) {
            if (count != NULL)   *count = 0;
            if (ipAddrs != NULL) *ipAddrs = NULL;
            codanetFreeInterfaceInfo(ifihead);
            return CODA_OK;
        }

        /* allocate mem for array */
        array = (char **) malloc(numIps*sizeof(char *));
        if (array == NULL) {
            codanetFreeInterfaceInfo(ifihead);
            return CODA_OUT_OF_MEMORY;
        }
    
        ifi = ifihead;
        for (;ifi != NULL; ifi = ifi->ifi_next) {
            /* ignore loopback */
            if (ifi->ifi_flags & IFF_LOOPBACK) {
                continue;
            }
    
            /* if the interface is up */
            if (ifi->ifi_flags & IFF_UP) {
                /* if there is an address listed ... */
                if ( (sa = ifi->ifi_addr) != NULL) {
                    /* copy IP address */
                    if ((pChar = sock_ntop_host(sa, sizeof(*sa))) != NULL) {
                        array[index++] = strdup(pChar);
                        if (codanetDebug >= CODA_DEBUG_INFO) {
                            printf("%sGetIpaddrs address : %s\n", codanetStr, pChar);
                        }
                    }
                }
            }
        }
        
        /* free memory */
        codanetFreeInterfaceInfo(ifihead);
    }

    /* ************************ */
    /* send back data to caller */
    /* ************************ */
    if (count != NULL)   *count = numIps;
    if (ipAddrs != NULL) *ipAddrs = array;
        
    return CODA_OK;
}


/**
* This routine takes a list of items, each item being a dot-decimal
* formatted IP address AND its corresponding broadcast address,
* and orders the list so that IP addresses on the preferred local subnet are first,
* those on other local subnets are next, and all others come last.
* This only works for IPv4.<p>
*
* All the elements of the returned linked-list need to be freed by the
* caller by calling {@link codanetFreeAddrList(codaIpList *addr)} once on the list head.
*
* @param ipList   pointer to a linked-list of structures, each containing an IP address and
*                 its corresponding broadcast address.
* @param netinfo  pointer to structure containing all local network information
*                 of interfaces that are "up"
* @param preferredSubnet the subnet whose IP address(es) will be first on the
*                        returned list
* @param noSubnetMatch pointer to int filled with 1 (true) if the preferred subnet
*                      is defined AND (the ipList arg has no item on that subnet OR
*                      the local host has no active network interface on that subnet).
*                      Else it will be filled with 0 (false).
*
* @return a linked list of IP addresses in dot-decimal format with all the
*         IP addresses in the response arg ordered so that those on the
*         preferred subnet are first, those on other local subnets are next,
*         and all others come last. If successful, the returned linked-list
*         need to be freed by the caller.
*         Returns NULL if ipList arg is NULL or no addresses contained in it
*/
codaIpList *codanetOrderIpAddrs(codaIpList *ipList, codaIpAddr *netinfo,
                                char* preferredSubnet, int* noSubnetMatch) {

    int addrLen, bCastAddrLen, onSameSubnet, onPreferredSubnet, preferredCount=0;
    char *ipAddress, *bcastAddress;
    codaIpList *listItem, *lastItem=NULL, *lastPrefItem=NULL, *firstItem = NULL, *firstPrefItem = NULL;
    codaIpAddr *local;

    if (ipList == NULL) return NULL;

    while (ipList != NULL) {
        ipAddress = ipList->addr;
        bcastAddress = ipList->bAddr;
        local = netinfo;
        onSameSubnet = 0;
        onPreferredSubnet = 0;
        /* length of addr strings in given list */
        addrLen = strlen(ipAddress);
        bCastAddrLen = strlen(bcastAddress);

        /*printf("codanetOrderIpAddrs: got list address %s\n", ipAddress);*/

        /* Compare with local subnets */
        while (local != NULL) {
            if (local->broadcast == NULL || bCastAddrLen < 7 || bCastAddrLen > 15) {
                /* Comparison cannot be done since no subnet info available,
                 * either locally or in provided list. Put at bottom of list.
                 * A dot-decimal IP address must be at least 7 chars long but no longer than 15. */
                break;
            }

/*printf("codanetOrderIpAddrs: ET ip = %s, bcast = %s, local bcast = %s\n",
            ipAddress, bcastAddress, local->broadcast);*/

            if (strcmp(local->broadcast, bcastAddress) == 0) {
                onSameSubnet = 1;
/*printf("codanetOrderIpAddrs: on SAME subnet\n");*/
                if (preferredSubnet != NULL && strcmp(preferredSubnet, bcastAddress) == 0) {
                    onPreferredSubnet = 1;
                    preferredCount++;
/*printf("codanetOrderIpAddrs: on PREFFERED subnet\n");*/
                }
                break;
            }

            local = local->next;
        }

        /* If we have no IP address, the provided list is faulty.
         * Forget about it and move to next item. */
        if (addrLen < 7 || addrLen > 15) {
            ipList = ipList->next;
            continue;
        }

        /* Create item to be in returned list */
        listItem = (codaIpList *) calloc(1, sizeof(codaIpList));
        if (listItem == NULL) {
            codanetFreeAddrList(firstItem);
            return NULL;
        }
        strncpy(listItem->addr, ipAddress, CODA_IPADDRSTRLEN);
        if (bCastAddrLen > 6 && bCastAddrLen < 16) {
            strncpy(listItem->bAddr, bcastAddress, CODA_IPADDRSTRLEN);
        }
        /*------------------------------------------------------------*/

        if (onPreferredSubnet) {
            if (firstPrefItem == NULL) {
                lastPrefItem = firstPrefItem = listItem;
                ipList = ipList->next;
                continue;
            }
        }
        else if (firstItem == NULL) {
            lastItem = firstItem = listItem;
            ipList = ipList->next;
            continue;
        }

        if (onPreferredSubnet) {
            /*printf("et_orderIpAddrs: pref subnet, head of list\n");*/
            /* Put it at head of preferred item list */
            listItem->next = firstPrefItem;
            firstPrefItem = listItem;
        }
        else if (onSameSubnet) {
            /*printf("et_orderIpAddrs: same subnet, head of list\n");*/
            /* Put it at the head of the regular list */
            listItem->next = firstItem;
            firstItem = listItem;
        }
        else {
            /*printf("et_orderIpAddrs: diff subnet, end of list\n");*/
            /* Put it at the end of the regular list */
            lastItem->next = listItem;
            lastItem = listItem;
        }

        ipList = ipList->next;
    }

    /* Tell caller, if preferred subnet defined, match or no match */
    if (noSubnetMatch != NULL) {
        if (preferredSubnet != NULL && preferredCount < 1) {
            *noSubnetMatch = 1;
        }
        else {
            *noSubnetMatch = 0;
        }
    }

    /* Now put preferred list at head of regular list */

    /* No lists at all  */
    if (firstPrefItem == NULL && firstItem == NULL) {
        return NULL;
    }
        /* No list combining needed here */
    else if (firstPrefItem != NULL && firstItem == NULL) {
        printf("et_orderIpAddrs: only items in preferred subnet list\n");
        return firstPrefItem;
    }
        /* No list combining needed here either */
    else if (firstPrefItem == NULL && firstItem != NULL) {
        return firstItem;
    }

    /* Combine lists */
    lastPrefItem->next = firstItem;

    return firstPrefItem;
}



/**
 * This routine finds out if the operating system is Linux or not.
 * 
 * @param isLinux pointer to int which gets filled with 1 if Linux, else 0
 * 
 * @returns ET/CMSG_OK    if successful
 * @returns ET/CMSG_ERROR if cannot find system info
 */
int codanetIsLinux(int *isLinux)
{
    struct utsname mysystem;

    /* find out the operating system of the machine we're on */
    if (uname(&mysystem) < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sIsLinux: cannot find system name\n", codanetStr);
        return CODA_ERROR;
    }

    if (strcasecmp(mysystem.sysname, "linux") == 0) {
        if (isLinux != NULL) *isLinux = 1;
    }
    else {
        if (isLinux != NULL) *isLinux = 0;
    }

    return CODA_OK;
}


/**
 * This routine is used in the ET system to create a server's udp
 * socket to receive udp packets.
 *
 * @param port      listening udp port number
 * @param address   listening ip address
 * @param multicast != 0 if listening for multicast address, else 0
 * @param fd        pointer to int which gets filled with listening socket's file descriptor
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if cannot find network interface info
 * @returns ET_ERROR_SOCKET/CMSG_SOCKET_ERROR if socket could not be created or socket options could not be set.
 */
int codanetUdpReceive(unsigned short port, const char *address, int multicast, int *fd)
{
    int                err, sockfd;
    const int          on = 1;
    struct in_addr     castaddr;
    struct sockaddr_in servaddr;

    /* put broad/multicast address into net-ordered binary form */
    if (inet_aton(address, &castaddr) == INET_ATON_ERR) {
        fprintf(stderr, "%sUdpReceive: inet_aton error\n", codanetStr);
        return CODA_ERROR;
    }

    bzero((void *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr   = castaddr;
    /*servaddr.sin_addr.s_addr = htonl(INADDR_ANY);*/
    servaddr.sin_port   = htons(port);

    /* create socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sUdpReceive: socket error\n", codanetStr);
        return CODA_SOCKET_ERROR;
    }
  
    /* allow multiple copies of this to run on same host */
    err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on));
    if (err < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sUdpReceive: setsockopt error\n", codanetStr);
        return CODA_SOCKET_ERROR;
    }
  
    /* add to multicast group */
    if (multicast) {
        struct ifi_info  *ifi, *ifihead;
        struct ip_mreq    mreq;
        struct sockaddr   *sa;

        memcpy(&mreq.imr_multiaddr, &castaddr, sizeof(struct in_addr));

        /* look through all IPv4 interfaces */
        ifihead = ifi = codanetGetInterfaceInfo(AF_INET, 0);
        if (ifi == NULL) {
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sGetNetworkInfo: cannot find network interface info\n", codanetStr);
            }
            return CODA_ERROR;
        }

        for (;ifi != NULL; ifi = ifi->ifi_next) {
            /* ignore loopback interface */
            if (ifi->ifi_flags & IFF_LOOPBACK) {
                continue;
            }
            /* if the interface is up */
            if (ifi->ifi_flags & IFF_UP) {
                /* if there is an address listed ... */
                if ( (sa = ifi->ifi_addr) != NULL) {
                    /* accept multicast over this interface */
/*printf("%sUdpReceive: joining %s on interface %s on port %hu\n", codanetStr, address, ifi->ifi_name, port);*/
                    memcpy(&mreq.imr_interface,
                            &((struct sockaddr_in *) sa)->sin_addr,
                            sizeof(struct in_addr));

                    err = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &mreq, sizeof(mreq));
                    if (err < 0) {
                        perror("codaNetUdpReceive: ");
                        codanetFreeInterfaceInfo(ifihead);
                        if (codanetDebug >= CODA_DEBUG_ERROR) {
                            fprintf(stderr, "%sUdpReceive: setsockopt IP_ADD_MEMBERSHIP error\n", codanetStr);
                        }
                        return CODA_SOCKET_ERROR;
                    }
                }
            }
        }

        /* free memory */
        codanetFreeInterfaceInfo(ifihead);
    }
    
    /* only allow packets to this port & address to be received */
      
    err = bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
    if (err < 0) {
        char errnostr[255];
        sprintf(errnostr,"err=%d ",errno);
        perror(errnostr);
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sUdpReceive: bind error\n", codanetStr);
        return CODA_SOCKET_ERROR;
    }
    
    if (fd != NULL) *fd = sockfd;

    return CODA_OK;
}


/**
 * This routine is used in the ET system to create a server's udp
 * socket to receive udp packets.
 *
 * @param port           listening udp port number
 * @param multicastAddrs array of multicast addresses to listen for
 * @param addrCount      number of multicast addresses in array
 * @param fd             pointer to int which gets filled with listening socket's file descriptor
 *
 * @returns ET/CMSG_OK                        if successful
 * @returns ET/CMSG_ERROR                     if cannot find network interface info
 * @returns ET_ERROR_SOCKET/CMSG_SOCKET_ERROR if socket could not be created or socket options could not be set.
 */
int codanetUdpReceiveAll(unsigned short port, char multicastAddrs[][CODA_IPADDRSTRLEN], int addrCount, int *fd) {

    int                i, err, sockfd;
    const int          on = 1;
    struct in_addr     castaddr;
    struct sockaddr_in servaddr;

    /* Accept packets arriving at all addresses */
    bzero((void *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    /* Create socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sUdpReceive: socket error\n", codanetStr);
        return CODA_SOCKET_ERROR;
    }

    /* Allow multiple copies of this to run on same host */
    err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on));
    if (err < 0) {
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sUdpReceive: setsockopt error\n", codanetStr);
        return CODA_SOCKET_ERROR;
    }

    printf("%sUdpReceive: addrCount = %d\n",codanetStr, addrCount);
    /* Add each multicast address to multicast group on each network interface */
    for (i=0; i < addrCount; i++) {

        struct ifi_info  *ifi, *ifihead;
        struct ip_mreq    mreq;
        struct sockaddr   *sa;
        printf("%sUdpReceive: mcast addr = %s\n",codanetStr, multicastAddrs[i]);

        /* Put multicast address into net-ordered binary form */
        if (inet_aton(multicastAddrs[i], &castaddr) == INET_ATON_ERR) {
            fprintf(stderr, "%sUdpReceive: inet_aton error\n", codanetStr);
            return CODA_ERROR;
        }

        memcpy(&mreq.imr_multiaddr, &castaddr, sizeof(struct in_addr));

        /* Look through all IPv4 interfaces */
        ifihead = ifi = codanetGetInterfaceInfo(AF_INET, 0);
        if (ifi == NULL) {
            if (codanetDebug >= CODA_DEBUG_ERROR) {
                fprintf(stderr, "%sGetNetworkInfo: cannot find network interface info\n", codanetStr);
            }
            return CODA_ERROR;
        }

        for (;ifi != NULL; ifi = ifi->ifi_next) {
            /* Do NOT ignore loopback interface for now */

            /* If the interface is up */
            if (ifi->ifi_flags & IFF_UP) {
                /* if there is an address listed ... */
                if ( (sa = ifi->ifi_addr) != NULL) {
                    /* Accept multicast over this interface */
printf("%sUdpReceive: joining %s on interface %s on port %hu\n", codanetStr, multicastAddrs[i], ifi->ifi_name, port);
                    memcpy(&mreq.imr_interface,
                           &((struct sockaddr_in *) sa)->sin_addr,
                           sizeof(struct in_addr));

                    err = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &mreq, sizeof(mreq));
                    if (err < 0) {
                        perror("codaNetUdpReceive: ");
                        codanetFreeInterfaceInfo(ifihead);
                        if (codanetDebug >= CODA_DEBUG_ERROR) {
                            fprintf(stderr, "%sUdpReceive: setsockopt IP_ADD_MEMBERSHIP error\n", codanetStr);
                        }
                        return CODA_SOCKET_ERROR;
                    }
                }
            }
        }

        /* free memory */
        codanetFreeInterfaceInfo(ifihead);
    }

    /* Only allow packets to this port & address to be received */

    err = bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
    if (err < 0) {
        char errnostr[255];
        sprintf(errnostr,"err=%d ",errno);
        perror(errnostr);
        if (codanetDebug >= CODA_DEBUG_ERROR) fprintf(stderr, "%sUdpReceive: bind error\n", codanetStr);
        return CODA_SOCKET_ERROR;
    }

    if (fd != NULL) *fd = sockfd;

    return CODA_OK;
}

