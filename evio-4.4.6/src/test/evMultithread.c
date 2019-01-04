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
 *	Event I/O test program
 *	
 * Author:  Carl Timmer, Data Acquisition Group
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "evio.h"

#define MIN(a,b) (a<b)? a : b


/*
   3 block headers (first 2 have 2 extra words each, last has 1 extra word).
   First block has 2 events    pthread_create(&config->bcastaddrs.tid[i], &attr, et_listen_thread, (void *) newarg);
. Second has 3 events.
   Last is empty final block.
*/
static uint32_t data1[] = {
    0x00000014,
    0x00000001,
    0x0000000A,
    0x00000002,
    0x00000000,
    0x00000004,
    0x00000000,
    0xc0da0100,
    0x00000003,
    0x00000002,

    0x00000004,
    0x00010101,
    0x00000001,
    0x00000001,
    0x00000001,
        
    0x00000004,
    0x00010101,
    0x00000002,
    0x00000002,
    0x00000002,
        
    0x00000019,
    0x00000002,
    0x0000000A,
    0x00000003,
    0x00000000,
    0x00000004,
    0x00000000,
    0xc0da0100,
    0x00000001,
    0x00000002,

    0x00000004,
    0x00010101,
    0x00000003,
    0x00000003,
    0x00000003,
        
    0x00000004,
    0x00010101,
    0x00000004,
    0x00000004,
    0x00000004,
        
    0x00000004,
    0x00010101,
    0x00000005,
    0x00000005,
    0x00000005,
        
    0x00000009,
    0x00000003,
    0x00000009,
    0x00000000,
    0x00000000,
    0x00000204,
    0x00000000,
    0xc0da0100,
    0x00000003,
    };
    
/* one block header with 1 event
static uint32_t data2[] = {
        0x0000000d,
        0x00000001,
        0x00000008,
        0x00000001,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,
        
        0x00000004,
        0x00010101,
        0x00000001,
        0x00000002,
        0x00000003
};
*/

static char *dictionary =
"<xmlDict>\n"
        "  <dictEntry name=\"Tag1-Num1\"   tag=\"1\"   num=\"1\"/>\n"
        "  <dictEntry name=\"Tag2-Num2\"   tag=\"2\"   num=\"2\"/>\n"
        "  <dictEntry name=\"Tag3-Num3\"   tag=\"3\"   num=\"3\"/>\n"
        "  <dictEntry name=\"Tag4-Num4\"   tag=\"4\"   num=\"4\"/>\n"
        "  <dictEntry name=\"Tag5-Num5\"   tag=\"5\"   num=\"5\"/>\n"
        "  <dictEntry name=\"Tag6-Num6\"   tag=\"6\"   num=\"6\"/>\n"
        "  <dictEntry name=\"Tag7-Num7\"   tag=\"7\"   num=\"7\"/>\n"
        "  <dictEntry name=\"Tag8-Num8\"   tag=\"8\"   num=\"8\"/>\n"
"</xmlDict>\n";

static void *closeThread(void *arg);
static void *writeThread(void *arg);
static int handle;

int main1() {
    int status;
    int maxEvBlk = 2;
    int *ip, bufLen;
    size_t bufSize = 10000;
    char eventBuffer[4*bufSize];
    pthread_t  tid;
    struct timespec wait = {0, 10000L}; /* .00001 sec */

    memset(eventBuffer, 0, 4*bufSize);


    printf("\nEvent I/O tests to BUFFER (%p) ...\n", eventBuffer);
    status = evOpenBuffer(eventBuffer, bufSize, "w", &handle);

    status = evIoctl(handle, "N", (void *) (&maxEvBlk));
    printf ("    Changed max events/block to %d, status = %#x\n", maxEvBlk, status);

    status = evWriteDictionary(handle, dictionary);
    printf ("    Write dictionary to buffer, status = %#x\n\n", status);

    /* Launch thread to close */
    pthread_create(&tid, NULL, closeThread, (void *)NULL);

    while (1) {
        /* write 1 event to buffer */
        status = evWrite(handle, data1);
        if (status != S_SUCCESS) {
            printf ("    error in write, status = %#x\n\n", status);
            break;
        }
        nanosleep(&wait, NULL);
    }
    return(0);
}

int main() {
    int status;
    int maxEvBlk = 2;
    size_t bufSize = 100000;
    char eventBuffer[4*bufSize];
    pthread_t  tid1, tid2;
    struct timespec wait = {0, 100000L}; /* .0001 sec */

    memset(eventBuffer, 0, 4*bufSize);


    printf("\nEvent I/O tests to BUFFER (%p) ...\n", eventBuffer);
    status = evOpenBuffer(eventBuffer, bufSize, "w", &handle);

    status = evIoctl(handle, "N", (void *) (&maxEvBlk));
    printf ("    Changed max events/block to %d, status = %#x\n", maxEvBlk, status);

    status = evWriteDictionary(handle, dictionary);
    printf ("    Write dictionary to buffer, status = %#x\n\n", status);

    /* Launch thread to close */
    pthread_create(&tid2, NULL, closeThread, (void *)NULL);

    /* Launch thread to write */
    pthread_create(&tid1, NULL, writeThread, (void *)NULL);

    while (1) {
        /* write 1 event to buffer */
        printf ("W1\n");
        status = evWrite(handle, data1);
        if (status != S_SUCCESS) {
            printf ("    error in write, status = %#x\n\n", status);
            break;
        }
        printf ("W1 wait\n");
        sched_yield();
        nanosleep(&wait, NULL);
    }
    return(0);
}

static void *writeThread(void *arg) {
    int status;
    struct timespec wait = {0, 100000L}; /* .0001 sec */
    while (1) {
        /* write 1 event to buffer */
        printf ("W2\n");
        status = evWrite(handle, data1);
        if (status != S_SUCCESS) {
            printf ("    error in write, status = %#x\n\n", status);
            break;
        }
        printf ("W2 wait\n");
        sched_yield();
        nanosleep(&wait, NULL);
    }
    return(0);
}


static void *closeThread(void *arg) {

    int status;
    sched_yield();
//    printf ("    *** Sleep in close thread\n");
//    sleep(1);
    printf ("    *** Call evClose() in close thread\n");
    status = evClose(handle);
    printf ("    \"Closed\" buffer, status = %#x\n\n", status);
    return(0);
}
