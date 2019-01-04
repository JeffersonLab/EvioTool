/*-----------------------------------------------------------------------------
 * Copyright (c) 2012  Jefferson Science Associates
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: timmer@jlab.org  Tel: (757) 249-7100  Fax: (757) 269-6248
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *  Event I/O test program
 *  
 * Author:  Carl Timmer, Data Acquisition Group
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <netinet/tcp.h> /* TCP_NODELAY def */
#include "evio.h"

#if defined sun || defined linux || defined VXWORKS
#  define socklen_t int
#endif

#define LISTENQ 10
#define SA struct sockaddr


#define MIN(a,b) (a<b)? a : b

/* prototypes */
static int  tcp_listen(unsigned short port, int size);
static int  Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);

static char *dictionary =
        "<xmlDict>\n"
        "  <dictEntry name=\"TAG1_NUM1\" tag=\"1\" num=\"1\"/>\n"
        "</xmlDict>\n";


static int     noDelay    = 1;
static int     serverPort = 22333;
static int     bufferSize = 8192;
static int     receiveBufferSize = 4*8192;
static char    *host = "localhost";
static int     recvFd;


static void *receiverThread(void *arg) {
    int listenfd=0, handle, status, nevents, nwords;
    int i;
    uint32_t dictLen, buffer[2048], *ip;
    char *dict;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    
printf("Receiver thread: listen on server socket\n");

    /* open a listening socket */
    listenfd = tcp_listen(serverPort, bufferSize);
    if (listenfd < 0) {
        printf("Error starting listening socket\n");
        exit(1);
    }

    /* wait for connection to client */
printf("Receiver thread: accepting\n");
    recvFd = Accept(listenfd, (SA *) &cliaddr, &len);
    if (recvFd < 0) {
        printf("Error receiving client TCP connection\n");
        exit(1);
    }
    
printf("Receiver thread: got client ... \n");
    status = evOpenSocket(recvFd, "r", &handle);
printf ("Receiver thread: Opened socket, status = %#x\n", status);

     status = evGetDictionary(handle, &dict, &dictLen);
printf ("Receiver thread: get dictionary, status = %#x\n\n", status);

     if (dictionary != NULL) {
         printf("DICTIONARY =\n%s\n", dict);
         free(dict);
     }

     nevents = 0;
    
     while (1) {

//printf("Receiver thread: Waiting on evRead\n");
         if ((status = evRead(handle, buffer, 2048)) != S_SUCCESS) {
             printf("Receiver thread: Done reading\n");
             break;
         }
         
         nevents++;
        
         printf("Receiver thread: Read event #%d,  len = %d data words\n", nevents, buffer[0] - 1);

         ip = buffer;
         nwords = buffer[0] + 1;
        
         printf("      Header words\n");
         printf("        %#10.8x\n", *ip++);
         printf("        %#10.8x\n\n", *ip++);
         printf("      Data words\n");
   
         nwords -= 2;
        
         for (; nwords > 0; nwords-=4) {
             for (i = MIN(nwords,4); i > 0; i--) {
                 printf("        %#10.8x", *ip++);
             }
             printf("\n");
         }
         printf("\n");
     }
    
printf("\nReceiver thread: Last read, status = %x\n", status);
    if (status == EOF) {
        printf("Receiver thread: Last read, reached EOF!\n");
    }
    
    status = evClose(handle);
    printf ("Receiver thread: Closed socket, status = %#x\n\n", status);
    return(0);
}



static int createSendFd() {

    struct sockaddr_in sin;
    int   sendFd, on=1;
    struct hostent  *hp;


    bzero(&sin, sizeof(sin));

    sendFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sendFd < 0) {
        printf("cannot open socket\n");
        exit(1);
    }

    hp = gethostbyname(host);
    if (hp == 0 && (sin.sin_addr.s_addr = inet_addr(host)) == -1) {
        fprintf(stderr, "%s: unknown host\n", host);
        exit(1);
    }

    if (hp != 0) {
        bcopy((const void *)hp->h_addr, (void *) &sin.sin_addr, hp->h_length);
    }

    sin.sin_port = htons(serverPort);
    sin.sin_family = AF_INET;

    if (setsockopt(sendFd, IPPROTO_TCP, TCP_NODELAY,(const char *)  &on, sizeof(on)) < 0) {
        printf("setsockopt TCP_NODELAY failed\n");
        exit(1);
    }

    if (connect(sendFd, (SA *)&sin, sizeof(sin)) < 0) {
        perror("connect");
        printf("connect failed: host %s port %d\n", inet_ntoa(sin.sin_addr),
               ntohs(sin.sin_port));

        exit(1);
    }

    return (sendFd);
}

/**
 * Routine to write a specified number of bytes to a TCP socket.
 *
 * @param fd   socket's file descriptor
 * @param vptr pointer to buffer from which bytes are supplied
 * @param n    number of bytes to write
 *
 * @return number of bytes written if successful
 * @return -1 if error and errno is set
 */
static int tcpWrite(int fd, const void *vptr, int n)
{
    int         nleft;
    int         nwritten;
    const char  *ptr;

    ptr = (char *) vptr;
    nleft = n;
  
    while (nleft > 0) {
        if ( (nwritten = write(fd, (char*)ptr, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0;       /* and call write() again */
            }
            else {
                return(nwritten);   /* error */
            }
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}



/* Bank with bank of int (data ranges from 4 to 12 to 14 to 16 words */

static uint32_t evBuf_8[] =
{
    0x00000007,
    0x00011001,
    0x00000005,
    0x00020b02,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00000003
}; /* len = 8 words */

/*
static uint32_t evBuf_16[] =
{
    0x0000000f,
    0x00011001,
    0x0000000d,
    0x00020b02,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00000003,
    0x00000004,
    0x00000005,
    0x00000006,
    0x00000007,
    0x00000008,
    0x00000009,
    0x0000000a,
    0x0000000b
}; */ /* len = 16 words */

static uint32_t evBuf_18[] =
{
    0x00000011,
    0x00011001,
    0x0000000f,
    0x00020b02,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00000003,
    0x00000004,
    0x00000005,
    0x00000006,
    0x00000007,
    0x00000008,
    0x00000009,
    0x0000000a,
    0x0000000b,
    0x0000000c,
    0x0000000d
};/* len = 18 words */

static uint32_t evBuf_20[] =
{
    0x00000013,
    0x00011001,
    0x00000011,
    0x00020b02,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00000003,
    0x00000004,
    0x00000005,
    0x00000006,
    0x00000007,
    0x00000008,
    0x00000009,
    0x0000000a,
    0x0000000b,
    0x0000000c,
    0x0000000d,
    0x0000000e,
    0x0000000f
};/* len = 20 words */


int main()
{
    int i, handle, status, sendFd, maxEvBlk=2;
    pthread_t tid;
    // char stuff[128];


    printf("Try running Receiver thread\n");
      
    /* run receiver thread */
    pthread_create(&tid, NULL, receiverThread, (void *) NULL);

    printf("Sending thd: sleep for 2 seconds\n");
    
    /* give it a chance to start */
    sleep(2);

    /* Create sending socket */
    sendFd = createSendFd();

    printf("Sending thd: socket fd = %d\n\n", sendFd);

    printf("\nSending thd: event I/O tests to socket (%d)\n", sendFd);
    status = evOpenSocket(sendFd, "w", &handle);
    
    printf("Sending thd: sleep for 1 more seconds\n");
    
    /* give it a chance to start */
    sleep(1);
    
    printf ("Sending thd: opened socket, status = %#x\n", status);

    
    status = evIoctl(handle, "N", (void *) (&maxEvBlk));
    printf ("Sending thd: changed max events/block to %d, status = %d\n", maxEvBlk, status);
    

    /* target block size = 8 header + 16 event words */
    i = 40;
    status = evIoctl(handle, "B", &i);
    if (status == S_EVFILE_BADSIZEREQ) {
        printf("Sending thd: bad value for target block size given\n");
        exit(0);
    }
    else if (status != S_SUCCESS) {
        printf("Sending thd: error setting target block size\n");
        exit(0);
    }
    printf ("Sending thd: changed target block size to %d, status = %d\n", i, status);


    /* buffer size = 2-8 headers + 16 event words or */
    
    i = 48;
    status = evIoctl(handle, "W", &i);
    if (status == S_EVFILE_BADSIZEREQ) {
        printf("Sending thd: bad value for buffer size given\n");
        exit(0);
    }
    else if (status != S_SUCCESS) {
        printf("Sending thd: error setting buffer size\n");
        exit(0);
    }
    printf ("Sending thd: changed buffer size to %d, status = %d\n", i, status);
    

    status = evWriteDictionary(handle, dictionary);
    printf ("Sending thd: write dictionary to socket, status = %d\n\n", status);

//    printf ("Sending thd: will write ** MED (16 word) ** ev to socket, status = %d\n",status);
//    status = evWrite(handle, evBuf_16);
    for (i=0; i < 4; i++) {
        printf("\nevtestSock: write little event %d ...\n", (i+1));
        status = evWrite(handle, evBuf_8);
        if (status != S_SUCCESS) {
            printf("Error in evWrite(), status = %x\n", status);
            exit(0);
        }
//        printf("\nEnter to continue\n");
//        gets(stuff);
    }

//    printf ("Sending thd: will write ** LITTLE (8 word) ** ev to socket, status = %d\n",status);
//    status = evWrite(handle, evBuf_8);

    printf ("Sending thd: will write ** BIG (18 word) ** ev to socket, status = %d\n",status);
    status = evWrite(handle, evBuf_18);

    printf ("Sending thd: will write ** HUGE (20 word) ** ev to socket, status = %d\n",status);
    status = evWrite(handle, evBuf_20);

    printf ("Sending thd: Call close()\n");
    status = evClose(handle);
    printf ("Sending thd: closed send socket, status %#x, wait 2 seconds\n\n", status);

    /* Don't exit the program before the receiver thread can do its stuff. */
    sleep(2);

    /* Close socket */
    close(sendFd);

    return(0);
}



/*****************************************************/
static int tcp_listen(unsigned short port, int size) {

    int                 listenfd, err;
    const int           debug=1, on=1;
    struct sockaddr_in  servaddr;

    err = listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (err < 0) {
        if (debug) fprintf(stderr, "tcp_listen: socket error\n");
        return err;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);

    if (noDelay) {
        err = setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, (const void *) &on, sizeof(on));
        if (err < 0) {
            close(listenfd);
            if (debug) fprintf(stderr, "tcp_listen: setsockopt error\n");
            return err;
        }
    }

    /* default TCP buffer = 4x data buffer size */
    err = setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (const void *) &receiveBufferSize, sizeof(receiveBufferSize));
    if (err < 0) {
        close(listenfd);
        if (debug) fprintf(stderr, "tcp_listen: setsockopt error\n");
        return err;
    }

    err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on, sizeof(on));
    if (err < 0) {
        close(listenfd);
        if (debug) fprintf(stderr, "tcp_listen: setsockopt error\n");
        return err;
    }

    err = setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (const void *) &on, sizeof(on));
    if (err < 0) {
        close(listenfd);
        if (debug) fprintf(stderr, "tcp_listen: setsockopt error\n");
        return err;
    }

    err = bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
    if (err < 0) {
        close(listenfd);
        if (debug) fprintf(stderr, "tcp_listen: bind error\n");
        return err;
    }

    err = listen(listenfd, LISTENQ);
    if (err < 0) {
        close(listenfd);
        if (debug) fprintf(stderr, "tcp_listen: listen error\n");
        return err;
    }

    return(listenfd);
}


/************************************************************/
static int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr) {
    int  n;

again:
        if ((n = accept(fd, sa, salenptr)) < 0) {
#ifdef EPROTO
        if (errno == EPROTO || errno == ECONNABORTED) {
#else
        if (errno == ECONNABORTED) {
#endif
            goto again;
        }
        else {
            fprintf(stderr, "Accept: error, errno = %d\n", errno);
        }
        }
        return(n);
        }

