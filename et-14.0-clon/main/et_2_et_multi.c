
/* email from Carl Timmer <timmer@jlab.org>        11/05/04 11:07

Hi Serguei,

I wrote an ET program to take any number of parallel streams and merge 
them into one.
It works on my local machine. To work for you, it'll require a few small
changes. like ET system names, station name, and the routine to find the
event number from the event must be changed to work with real HallB events.

I've attached it. If you're interested, we can go over it together.
Carl
*/



/*----------------------------------------------------------------------------*
 *  Copyright (c) 2004        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *
 *    1-Nov-2004
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12H        *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      ET system example of multiple inputs from a single ET system put
 *      to a sorted single output (producing) stream of another ET system.
 *
 *      There is a single, fundamental problem with taking multiple, independent
 *      inputs and ordering them into 1 stream. If all the events are not
 *      numbered in some way, there is no guarantee they will retain the 
 *      original order when put into a single stream.
 *
 *      Unfortunately, this is what happens in CODA as non-physics or control
 *      events are not numbered. Thus, if a chunk of events is obtained with
 *      no physics or numbered events among them, then it is possible that the
 *      chunk of events will end up in an order different from the original.
 *
 *----------------------------------------------------------------------------*/
 
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#ifdef sun
#include <thread.h>
#endif
#include "et.h"

#define CHUNK_SIZE    200
#define BUFFER_SIZE    10
#define INPUT_STREAMS   5
/* #define NUM_EVENTS    500000 */


/*
 * Structure to hold an array of events and associated info:
 * 1) the circularBuffer this array came from,
 * 2) not = 0 if no numbered events in array,
 * 3) the total number of events in the array,
 * 4) the event number of the first numbered (countable) event,
 * 5) the event number of the last numbered event,
 * 6) the array of events,
 * 7) pointer to the next chunk
 */ 
typedef struct etChunk_t {
    int bufferId;
    int noNumberedEvents;
    int count;
    int firstEventNum;
    int lastEventNum;
    et_event *events[CHUNK_SIZE];
    struct etChunk_t *next;    
} etChunk;



/*
 * Structure to hold an array of et event chunks and associated info:
 * 1)  buffer id number,
 * 2)  number of chunks in buffer,
 * 3)  number of full chunks in buffer,
 * 4)  pointer to head of filled event chunks,
 * 5)  pointer to tail of filled event chunks,
 * 6)  pointer to head of empty event chunks,
 * 7)  pointer to tail of empty event chunks,
 * 8)  mutex for changing structure data,
 * 9)  condition variable for waiting on and signaling of filled chunk available
 * 10) condition variable for waiting on and signaling of empty chunk available
 */ 
typedef struct circularBuffer_t {
    int      id;
    int      size;
    int      fullCount;
    etChunk *filledHead;
    etChunk *filledTail;
    etChunk *emptyHead;
    etChunk *emptyTail;
    pthread_mutex_t  mutex;
    pthread_cond_t   filledChunk;    
    pthread_cond_t   emptiedChunk;    
} circularBuffer;


/*
 * Structure to pass as an argument to event read and getNew threads:
 * 1) id of ET system
 * 2) id of ET station to get from
 * 3) ET station attachment
 * 4) size of new events in bytes
 * 5) circular buffer
 * 6) length of time to wait for a chunk before timing out
 */
typedef struct threadArg_t {
    et_sys_id        systemId;
    et_stat_id       stationId;
    et_att_id        attach;
    int              size;
    circularBuffer  *buffer;
    struct timespec *timeout;
} threadArg;


/* prototypes */
void  shellSort(int n, int a[], int b[]);
void *signalThread (void *arg);
void *getEventsThread(void *arg);
void *getNewEventsThread(void *arg);
int   initCircularBuffer(circularBuffer *buf, int id, int size);
int   getEmptyChunk(circularBuffer *buf, etChunk **chunk, struct timespec *timeout);
int   getFilledChunk(circularBuffer *buf, etChunk **chunk, struct timespec *timeout);
int   putEmptyChunk(circularBuffer *buf, etChunk *chunk);
int   putFilledChunk(circularBuffer *buf, etChunk *chunk);
int   getEventNumber(et_event *pe, int *evNum);
void  getFirstAndLastEventNumbers(etChunk *chunk);
void  copyEvent(et_event *src, et_event *dst);
struct timespec getAbsoluteTime(struct timespec *deltaTime);



/* Main program */
int
main(int argc, char **argv)
{
    int i, status;
    pthread_t tid;
    circularBuffer inBuffer[INPUT_STREAMS], outBuffer;
    etChunk *inChunk, *outChunk;
    /* ET connections */
    et_openconfig openConfig;
    et_statconfig stationConfig;
    char *etIn  = "/tmp/et_sys_clashps";
    char *etOut = "/tmp/et_sys_clashps";
    char *stationName = "ET2ET1";
    et_sys_id   inId[INPUT_STREAMS], outId;
    et_stat_id  inStation;
    et_att_id   inAttach[INPUT_STREAMS], outAttach;
    /* signal handling */
    threadArg inArg, outArg;
    sigset_t  sigblock;
    /* timeouts */
    struct timespec inTimeout, outTimeout;    

    /* timeout for waiting for a chunk from the input ET */
    inTimeout.tv_sec   = 2;
    inTimeout.tv_nsec  = 0;
    
    /* timeout for waiting for a chunk from the output ET */
    outTimeout.tv_sec  = 10;
    outTimeout.tv_nsec = 0;
    
        
    /*************************/
    /* setup signal handling */
    /*************************/
    /* block all signals */
    sigfillset(&sigblock);
    status = pthread_sigmask(SIG_BLOCK, &sigblock, NULL);
    if (status != 0) {
        printf("%s: pthread_sigmask failuren", argv[0]);
        exit(1);
    }

#ifdef sun
    /* prepare to run other threads concurrently */
    thr_setconcurrency(thr_getconcurrency() + INPUT_STREAMS + 1);
#endif

    /* spawn signal handling thread */
    pthread_create(&tid, NULL, signalThread, (void *)NULL);
    

    /* ET open configuration */
    et_open_config_init(&openConfig);

    
    /*************************/
    /* for 1 output stream   */
    /*************************/
                
    /* open event-consuming (output) ET system */

	/*sergey*/
    et_open_config_sethost(openConfig, "clondaq4");

    if (et_open(&outId, etOut, openConfig) != ET_OK) {
        fprintf(stdout, "%s: cannot open output ET system\n", argv[0]);
        exit(-1);
    }
                
    /* attach to GrandCentral station for producing events */
    if (et_station_attach(outId, ET_GRANDCENTRAL, &outAttach) < 0) {
        fprintf(stderr, "%s: error in station attach\n", argv[0]);
        exit(1);
    }

    /* init circular buffer */
    initCircularBuffer(&outBuffer, INPUT_STREAMS, BUFFER_SIZE);
    
    /* Start thread to get new events from the single output ET system */
    outArg.systemId = outId;
    outArg.size     = 128;
    outArg.attach   = outAttach;
    outArg.buffer   = &outBuffer;
    outArg.timeout  = NULL;
    status = pthread_create(&tid, NULL, getNewEventsThread, (void *) &outArg);
    if (status != 0) {
        fprintf(stderr, "Error creating thread");
        fflush(stderr);
        exit(-1);
    }
    
    /*******************************/
    /* for each input stream ...   */
    /*******************************/
                
    for (i=0; i < INPUT_STREAMS; i++) {
    
        /* init circular buffer */
        initCircularBuffer(&inBuffer[i], i, BUFFER_SIZE);
    
        /* open event-producing ET system */

        /*sergey: replace following
        et_open_config_sethost(openConfig, ET_HOST_ANYWHERE);
		*/
        et_open_config_sethost(openConfig, "clondaq5");

        if (et_open(&inId[i], etIn, openConfig) != ET_OK) {
            fprintf(stdout, "%s: cannot open input ET system\n", argv[0]);
            exit(-1);
        }
        
        /* set debug level */
        et_system_setdebug(inId[i], ET_DEBUG_INFO);

        
        /* create station if necessary */
        if (i == 0) {      
            /* configuration of station to get from  */    
            et_station_config_init(&stationConfig);
     
            if ((status = et_station_create(inId[i], &inStation, stationName, stationConfig)) < ET_OK) {
                if (status == ET_ERROR_EXISTS) {
                    /* inStation contains pointer to existing station */;
                    fprintf(stdout, "%s: station already exists, will continue\n", argv[0]);
                }
                else if (status == ET_ERROR_TOOMANY) {
                    fprintf(stderr, "%s: too many stations created\n", argv[0]);
                    exit(-1);
                }
                else {
                    fprintf(stderr, "%s: error in station creation\n", argv[0]);
                    exit(-1);
                }
            }
            
            /* release config memory */
            et_station_config_destroy(stationConfig);
        }

        if (et_station_attach(inId[i], inStation, &inAttach[i]) < 0) {
            fprintf(stderr, "%s: error in station attach\n", argv[0]);
            exit(1);
        }
        
        /* Start threads to read events from an input ET system */
        inArg.systemId  = inId[i];
        inArg.stationId = inStation;
        inArg.attach    = inAttach[i];
        inArg.buffer    = &inBuffer[i];
        inArg.timeout   = NULL;
        status = pthread_create(&tid, NULL, getEventsThread, (void *) &inArg);
        if (status != 0) {
            fprintf(stderr, "Error creating thread");
            fflush(stderr);
            exit(-1);
        }
    }

    et_open_config_destroy(openConfig);

    /* give chance for other threads to start */
    sched_yield();
    
    {
        int lowestEvent, nextEvent, lowestStream, haveNext;
        int eventNums[INPUT_STREAMS];
        int cStreams[INPUT_STREAMS];
        etChunk *lowestChunk, *chunks[INPUT_STREAMS];
        
        /* Grab a filled chunk of events from each input stream (circular buffer) */
        for (i=0; i < INPUT_STREAMS; i++) {
            status = getFilledChunk(&inBuffer[i], &chunks[i], &inTimeout);
            if (status != 0) {
              fprintf(stderr, "Error in getting event chunk 1\n");
              exit(-1);
            }
            eventNums[i] = chunks[i]->firstEventNum;
            cStreams[i]  = i;
        }
        
        /* sort the chunks by first event number */
        shellSort(INPUT_STREAMS, eventNums-1, cStreams-1);
        lowestEvent  = eventNums[0];
        lowestStream = cStreams[0];
        lowestChunk  = chunks[lowestStream];
        haveNext     = 1;
        /* if chunk has at least 1 numbered event, set next expected event */
        if (lowestChunk->noNumberedEvents == 0) {
            nextEvent = lowestChunk->lastEventNum + 1;
        }                
        /* else put the chunk with no numbered events */
        else {
            nextEvent = -1;
        }               
        
        /*
        for (i=0; i < INPUT_STREAMS; i++) {
            printf("eventsNums[%d] = %d, cStreams[%d] = %d\n",
                    i, eventNums[i],i,cStreams[i]);
        }
        */
        
        /*
         * The following algorithm assumes that the event numbers are always
         * going up and never down.
         */
        
top:    while (1) {
            /* look for next chunk to put if we don't know which is next */
            if (!haveNext) {
                for (i=0; i < INPUT_STREAMS; i++) {
/*fprintf(stderr, "look for %d, get %d\n", nextEvent,chunks[i]->firstEventNum );*/
                    if (chunks[i] == NULL) continue;
                    
                    /* if chunk has no numbered events, put it anyway */
                    if (chunks[i]->noNumberedEvents) {
                        lowestStream = i;
                        lowestChunk  = chunks[i];
                        haveNext     = 1;
                        break;
                    }                
                    /* if chunk has is next, put it */
                    else if (chunks[i]->firstEventNum == nextEvent) {
                        nextEvent    = chunks[i]->lastEventNum + 1;
                        lowestEvent  = chunks[i]->firstEventNum;
                        lowestStream = i;
                        lowestChunk  = chunks[i];
                        haveNext     = 1;
                        break;
                    }
                }
            }
            
            /* if we have the next chunk, put it ... */
            if (haveNext) {
                /* get a chunk of new events from the output circular buffer */
                status = getFilledChunk(&outBuffer, &outChunk, &outTimeout);
                if (status == ETIMEDOUT) {
                  fprintf(stderr, "TIMEOUT waiting for new event chunk\n");
                  exit(-1);
                }
                else if (status != 0) {
                  fprintf(stderr, "Bad value for TIMEOUT in getting new event chunk\n");
                  exit(-1);
                }
                
                /* move events from input to output */
                for (i=0; i < lowestChunk->count; i++) {
                    copyEvent(lowestChunk->events[i], outChunk->events[i]);      
                }

                /* put ET events back into their ET systems */
                status = et_events_put(outId, outAttach, outChunk->events,
                                       outChunk->count);
                if (status == ET_OK) {
                  ;
                }
                else {
                  fprintf(stderr, "%s: put error, out et_events_put status = %d\n", argv[0], status);
                  exit(-1);
                }

                status = et_events_put(inId[lowestStream],  inAttach[lowestStream],
                                       lowestChunk->events, lowestChunk->count);
                if (status == ET_OK) {
                  ;
                }
                else {
                  fprintf(stderr, "%s: put error, in et_events_put status = %d\n", argv[0], status);
                  exit(-1);
                }

                /* puts chunks back into their respective buffers */
                putEmptyChunk(&outBuffer, outChunk);
                putEmptyChunk(&inBuffer[lowestStream], lowestChunk);
                
                /* reset certain items */
                chunks[lowestStream] = NULL;
                haveNext = 0;
                
                /* To replace the chunk we just read & emptied from one stream, */
                /* get another filled chunk of events from that same stream.    */
                status = getFilledChunk(&inBuffer[lowestStream], &chunks[lowestStream], &inTimeout);
                if (status == ETIMEDOUT) {
                  fprintf(stderr, "TIMEOUT waiting for event chunk\n");
                }
                else if (status != 0) {
                  fprintf(stderr, "Bad value for TIMEOUT in getting event chunk\n");
                  exit(-1);
                }        
            }
            
            /* if we cannot find the next chunk, then ... */
            else {
                /* There is a break in the sequence of numbers, */
                /* so what do we do now?                        */
                 
                /* If there are stuck streams, try to read from them */
                for (i=0; i < INPUT_STREAMS; i++) {
                    if (chunks[i] == NULL) {
                        status = getFilledChunk(&inBuffer[i], &chunks[i], &inTimeout);
                        /* if we cannot read from this stream, try another */
                        if (status != 0) {
                          fprintf(stderr, "Error in getting event chunk 2\n");
                          continue;
                        }
                        
                        /* info for sorting purposes */
                        eventNums[i] = chunks[i]->firstEventNum;
                        cStreams[i]  = i;
                        
                        /* if we got the next chunk, start at very top again */
                        if (chunks[i]->firstEventNum == nextEvent) {
                            goto top;
                        }
                    }
                    else {
                        /* info for sorting purposes */
                        eventNums[i] = chunks[i]->firstEventNum;
                        cStreams[i]  = i;
                    }               
                }
                
                /* If all streams have been read from and there is still a break, */
                /* continue with the current lowest chunk.                        */                
                shellSort(INPUT_STREAMS, eventNums-1, cStreams-1);
                lowestEvent  = eventNums[0];
                lowestStream = cStreams[0];
                lowestChunk  = chunks[lowestStream];
                haveNext     = 1;
                /* if chunk has at least 1 numbered event set next expected event */
                if (lowestChunk->noNumberedEvents == 0) {
                    nextEvent = lowestChunk->lastEventNum + 1;
                }
                /* else put the chunk with no numbered events */
                else {
                    nextEvent = -1;
                }               
/*fprintf(stderr, "Fix event # break -> %d\n", lowestEvent);*/
            }
        }
    }       
      
    return 0;
}


/***********************************************************************/

/* copy every field in one (filled) event into another (new) event */
void copyEvent(et_event *src, et_event *dst) {
    int i, len, val;
    size_t size;
    int con[ET_STATION_SELECT_INTS];
    char *srcData, *dstData;
    
    et_event_getpriority(src, &val);
    et_event_setpriority(dst,  val);
    
    et_event_getlength(src, &size);
    len = size;
    et_event_setlength(dst,  len);

    et_event_getdatastatus(src, &val);
    et_event_setdatastatus(dst,  val);
    
    et_event_getendian(src, &val);
    et_event_setendian(dst,  val);
    
    et_event_getcontrol(src, con);
    et_event_setcontrol(dst, con, ET_STATION_SELECT_INTS);

    et_event_getdata(src, (void **) &srcData);
    et_event_getdata(dst, (void **) &dstData);
    memcpy((void *)dstData, (const void *) srcData, (size_t)len);
}


/***********************************************************************/

/* initialize buffers */
int initCircularBuffer(circularBuffer *buf, int id, int size)
{
    int i;
    etChunk *chunk, *prevChunk, *firstChunk, *lastChunk;
    
    if (size < 2) {
        fprintf(stderr, "Need more than 1 item in a circular buffer.\n");
        return(-1);
    }
    
    buf->id   = id;
    buf->size = size;
    buf->fullCount = 0;
    
    /* for "size" number of chunks ... */
    for (i=0 ; i < size; i++) {
        /* create and initialize one chunk */
        chunk = (etChunk *) malloc(sizeof(etChunk));
        chunk->bufferId = id;
        chunk->noNumberedEvents = 0;
        chunk->count = 0;
        chunk->firstEventNum = 0;
        chunk->lastEventNum  = 0;
        
        /* link chunks into a circle */
        if (chunk == NULL) {
            fprintf(stderr, "OUT OF MEMORY\n");
            fflush(stderr);
            exit(-1);
        }        
        
        if (i == 0) {
            prevChunk  = chunk;
            firstChunk = chunk;
            continue;
        }
        else if (i == size-1) {
            lastChunk = chunk;
            prevChunk->next = chunk;
            chunk->next = firstChunk;
        }
        
        prevChunk->next = chunk;                
        prevChunk = chunk;
    }
    
    /* buffer bookkeeping */
    buf->filledHead = NULL;
    buf->emptyHead  = firstChunk;
    buf->emptyTail  = buf->filledTail = lastChunk;
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->filledChunk, NULL);
    pthread_cond_init(&buf->emptiedChunk, NULL);

    
    return(0);
}


/***********************************************************************/

/*
 * Shell's method of sorting from "Numerical Recipes" slightly modified.
 * It is assumed that a and b have indexes from 1 to n. Since the input
 * arrays will start at 0 index, call this routine with something like:
 * shellSort(n, a-1, b-1). Then a[1] as seen in the routine is actually
 * a[0] in the program. Sorting is done on a array.
 */
void shellSort(int n, int a[], int b[]) {
  unsigned long i, j, inc;
  int v, w;
  inc = 1;
  do {
    inc *= 3;
    inc++;
  } while (inc <= n);
  
  do {
    inc /= 3;
    for (i=inc+1; i<=n; i++) {
      v = a[i];
      w = b[i];
      j = i;
      while (a[j-inc] > v) {
        a[j] = a[j-inc];
        b[j] = b[j-inc];
        j -= inc;
        if (j <= inc) break;
      }
      a[j] = v;
      b[j] = w;
    }
  } while (inc > 1);
}


/***********************************************************************/

/* translate a delta time into an absolute time for pthread_cond_wait */
struct timespec getAbsoluteTime(struct timespec *deltaTime) {
    struct timeval now;
    struct timespec absTime;
    long   nsecTotal;
    
    gettimeofday(&now, NULL);
    nsecTotal = deltaTime->tv_nsec + 1000*now.tv_usec;
    if (nsecTotal >= 1000000000L) {
      absTime.tv_nsec = nsecTotal - 1000000000L;
      absTime.tv_sec  = deltaTime->tv_sec + now.tv_sec + 1;
    }
    else {
      absTime.tv_nsec = nsecTotal;
      absTime.tv_sec  = deltaTime->tv_sec + now.tv_sec;
    }
    return absTime;
}


/***********************************************************************/
void lockMutex(circularBuffer *buf)
{
  int status;
  
  status = pthread_mutex_lock(&buf->mutex);
  if (status != 0) {
    fprintf(stderr, "Failed mutex lock");
    exit(-1);
  }
}


/***********************************************************************/
void unlockMutex(circularBuffer *buf)
{
  int status;
  
  status = pthread_mutex_unlock(&buf->mutex);
  if (status != 0) {
    fprintf(stderr, "Failed mutex lock");
    exit(-1);
  }
}


/***********************************************************************/

/* get a single, empty chunk structure from a circular buffer */
int getEmptyChunk(circularBuffer *buf, etChunk **chunk, struct timespec *timeout) {
    int status;
    struct timespec t;
    
    lockMutex(buf);
    
    /* if there are no available chunks, wait for one */
    while (buf->emptyHead == NULL) {
        if (timeout == NULL) {
            status = pthread_cond_wait(&buf->emptiedChunk, &buf->mutex);
        }
        else {
            t = getAbsoluteTime(timeout);
            status = pthread_cond_timedwait(&buf->emptiedChunk, &buf->mutex, &t);
        }
        
        if (status == ETIMEDOUT) {
            *chunk = NULL;
            unlockMutex(buf);
            return status;
        }
        else if (status != 0) {
            /* Most likely a bad value for the timeout.
             * Don't abort, simply return an error.
             */
            *chunk = NULL;
            unlockMutex(buf);
            return -1;
        }        
    }
    
    *chunk = buf->emptyHead;
    if (buf->emptyHead == buf->emptyTail) {
        buf->emptyHead = NULL;
        unlockMutex(buf);
        return 0;
    }
    buf->emptyHead = buf->emptyHead->next;
    
    unlockMutex(buf);
    
    return 0;
}


/***********************************************************************/

/* get a single, filled chunk structure from a circular buffer */
int getFilledChunk(circularBuffer *buf, etChunk **chunk, struct timespec *timeout) {
    int status;
    struct timespec t;
    
    lockMutex(buf);
    
    /* if there are no available chunks, wait for one */
    while (buf->filledHead == NULL) {
        if (timeout == NULL) {
            status = pthread_cond_wait(&buf->filledChunk, &buf->mutex);
        }
        else {
            t = getAbsoluteTime(timeout);
            status = pthread_cond_timedwait(&buf->filledChunk, &buf->mutex, &t);
        }
        
        if (status == ETIMEDOUT) {
            *chunk = NULL;
            unlockMutex(buf);
            return status;
        }
        else if (status != 0) {
            /* Most likely a bad value for the timeout.
             * Don't abort, simply return an error.
             */
            *chunk = NULL;
            unlockMutex(buf);
            return -1;
        }        
    }
    *chunk = buf->filledHead;
    if (buf->filledHead == buf->filledTail) {
        buf->filledHead = NULL;
        unlockMutex(buf);
        return 0;
    }
    buf->filledHead = buf->filledHead->next;
    
    unlockMutex(buf);
    
    return 0;
}

/***********************************************************************/

/* put a single, used or empty chunk structure back into its circular buffer */
int putEmptyChunk(circularBuffer *buf, etChunk *chunk) {
    int status;
    
    lockMutex(buf);
        
    buf->emptyTail = chunk;
    if (buf->emptyHead == NULL) {
        buf->emptyHead = chunk;
    }
    buf->fullCount--;
    pthread_cond_signal(&buf->emptiedChunk);
    
    unlockMutex(buf);
    
    return 0;
}


/***********************************************************************/

/* put a single, filled chunk structure back into its circular buffer */
int putFilledChunk(circularBuffer *buf, etChunk *chunk) {
    int status;
    
    lockMutex(buf);
        
    buf->filledTail = chunk;
    if (buf->filledHead == NULL) {
        buf->filledHead = chunk;
    }
    buf->fullCount++;
    pthread_cond_signal(&buf->filledChunk);
    
    unlockMutex(buf);
    
    return 0;
}


/***********************************************************************/

/*
 * Returns whether (1) or not (0) the event is numbered, and fills
 * in the 2nd argument to tell what that number is if appropriate.
 */
int getEventNumber(et_event *pe, int *evNum) {
    
    int *data, len;
    size_t size;
    /*int con[ET_STATION_SELECT_INTS]; */
    /* et_event_getcontrol(pe, con); */
    
    /* need at least an integer's worth in the data buffer */
    et_event_getlength(pe, &size);
    len = size;
    if (len < 4) {
        return 0;
    }    
    et_event_getdata(pe, (void **) &data);
    *evNum = *data;
    return 1;
}


/***********************************************************************/

/*
 * Gets the first and last event numbers for later ordering of chunks 
 * as well as whether the chunk has any numbered events at all.
 */
void getFirstAndLastEventNumbers(etChunk *chunk) {
    int i, err, eventNum, gotFirst=0;
    
    chunk->firstEventNum    = -1;
    chunk->lastEventNum     = -1;
    chunk->noNumberedEvents =  1;
    
    for (i=0; i < chunk->count; i++) {
        err = getEventNumber(chunk->events[i], &eventNum);
        if (err == 0) {
            continue;
        }
        
        if (!gotFirst) {
            chunk->firstEventNum = eventNum;
            gotFirst = 1;
        }
        
        chunk->noNumberedEvents = 0;
        chunk->lastEventNum = eventNum;
    }
}



/***********************************************************************/

/* Thread for reading chunks of events from the input ET system */
void *getEventsThread(void *arg) {
    threadArg *tArg = (threadArg *) arg;
    
    et_sys_id       systemId  = tArg->systemId;
    et_stat_id      stationId = tArg->stationId;
    et_att_id       attach    = tArg->attach;
    circularBuffer *buffer    = tArg->buffer;
    struct timespec *timeout  = tArg->timeout;    
    
    int i, j, status, numRead, totalRead = 0;
    etChunk *chunk;
    
    
    while (1) {      
      /* get next empty chunk from circular buffer */
      status = getEmptyChunk(buffer, &chunk, timeout);
      if (status == ETIMEDOUT) {
        fprintf(stderr, "TIMEOUT waiting for free buffer\n");
        exit(-1);
      }
      else if (status != 0) {
        fprintf(stderr, "Bad value for TIMEOUT in getting free buffer\n");
        exit(-1);
      }        
      
      
      /* get ET events into chunk's array */
      if (timeout == NULL) {
          status = et_events_get(systemId, attach, chunk->events, ET_SLEEP,
                                 timeout, CHUNK_SIZE, &numRead);
      }
      else {
          status = et_events_get(systemId, attach, chunk->events, ET_TIMED,
                                 timeout, CHUNK_SIZE, &numRead);
      }
      
      if (status == ET_OK) {
        ;
      }
      else if (status == ET_ERROR_TIMEOUT) {
        printf("get timeout\n");
        goto end;
      }
      else {
        printf("get error # %d\n", status);
      }
      
      chunk->count = numRead;
      getFirstAndLastEventNumbers(chunk);
      
      /* put filled chunk back in buffer */
      status = putFilledChunk(buffer, chunk);

      totalRead += numRead;
      
      end:
        ;
        /* print something out after having read NUM_EVENTS events */
        /*
        if (totalRead >= NUM_EVENTS) {
          totalRead = 0;
          printf("buffer %d: %d events\n", buffer->id, NUM_EVENTS);
        }
        */
    }

    return (NULL);
}


/***********************************************************************/

/* Thread for reading chunks of new events from the output ET system */
void *getNewEventsThread(void *arg) {
    threadArg *tArg = (threadArg *) arg;
    
    et_sys_id        systemId = tArg->systemId;
    et_att_id        attach   = tArg->attach;
    int              size     = tArg->size;    
    circularBuffer  *buffer   = tArg->buffer;
    struct timespec *timeout  = tArg->timeout;    
    
    int i, j, status, numRead, totalRead = 0;
    etChunk *chunk;
    
    
    while (1) {      
      /* get next empty chunk from circular buffer */
      status = getEmptyChunk(buffer, &chunk, timeout);
      if (status == ETIMEDOUT) {
        fprintf(stderr, "NEW: TIMEOUT waiting for free buffer\n");
        exit(-1);
      }
      else if (status != 0) {
        fprintf(stderr, "NEW: Bad value for TIMEOUT in getting free buffer\n");
        exit(-1);
      }        
      
      
      /* get new ET events into chunk's array */
      if (timeout == NULL) {
          status = et_events_new(systemId, attach, chunk->events, ET_SLEEP,
                                 timeout, size, CHUNK_SIZE, &numRead);
      }
      else {
          status = et_events_new(systemId, attach, chunk->events, ET_TIMED,
                                 timeout, size, CHUNK_SIZE, &numRead);
      }

      if (status == ET_OK) {
        ;
      }
      else if (status == ET_ERROR_TIMEOUT) {
        printf("NEW: get timeout\n");
        goto end;
      }
      else {
        printf("NEW: get error # %d\n", status);
      }
      
      chunk->count = numRead;
      
      /* put filled chunk back in buffer */
      status = putFilledChunk(buffer, chunk);

      totalRead += numRead;
      
      end:
        ;
        /* print something out after having read NUM_EVENTS events */
        /*
        if (totalRead >= NUM_EVENTS) {
          totalRead = 0;
          printf("NEW: buffer %d: %d events\n", buffer->id, NUM_EVENTS);
        }
        */
    }

    return (NULL);
}


/***********************************************************************/

/* thread to handle signals */
void *signalThread (void *arg)
{
  sigset_t        signal_set;
  int             sig_number;
  et_stat_id      stat;
  et_sys_id       sysid;
  et_openconfig   openconfig;
 
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  
  /* Wait for Control-C */
  sigwait(&signal_set, &sig_number);
  
  printf("multiStream: got a control-C, ending program\n");
  
  /* Handle any cleaning up.
   * Remember that all threads are running concurrently with this handler.
   */
  
  exit(1);
}


