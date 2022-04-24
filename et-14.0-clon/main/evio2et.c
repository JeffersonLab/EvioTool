/* evio2et.c reads evio file and injects events into ET system */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "et.h"
#include "evio.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))

/* recent versions of linux put float.h (and DBL_MAX) in a strange place */
#define DOUBLE_MAX   1.7976931348623157E+308

#define NUMLOOPS 1000
#define CHUNK 100
#define DBL_MAX   1.e+100

#define EVIO_SWAP64(x) ( (((x) >> 56) & 0x00000000000000FFL) | \
                         (((x) >> 40) & 0x000000000000FF00L) | \
                         (((x) >> 24) & 0x0000000000FF0000L) | \
                         (((x) >> 8)  & 0x00000000FF000000L) | \
                         (((x) << 8)  & 0x000000FF00000000L) | \
                         (((x) << 24) & 0x0000FF0000000000L) | \
                         (((x) << 40) & 0x00FF000000000000L) | \
                         (((x) << 56) & 0xFF00000000000000L) )

#define EVIO_SWAP32(x) ( (((x) >> 24) & 0x000000FF) | \
                         (((x) >> 8)  & 0x0000FF00) | \
                         (((x) << 8)  & 0x00FF0000) | \
                         (((x) << 24) & 0xFF000000) )

#define EVIO_SWAP16(x) ( (((x) >> 8) & 0x00FF) | \
                         (((x) << 8) & 0xFF00) )

/* prototype */
static void * signal_thread (void *arg);

#define MAXBUF 500000
static int maxbufbytes = MAXBUF*4;;
static unsigned int buf[MAXBUF];

int
main(int argc,char **argv)
{
  int             handle, handle1, len;
  int             i, ii, j, status, nevents_max, buflen, event_type;
  size_t          event_size;
  double          freq=0.0, freq_tot=0.0, freq_avg=0.0, datafreq=0.0, datafreq_tot=0.0, datafreq_avg=0.0;
  int             iterations=1, count, evcount;
  double          datacount;
  et_att_id	  attach1;
  et_sys_id       id;
  et_openconfig   openconfig;
  et_event       **pe;
  struct timespec timeout;
#if defined linux || defined __APPLE__
  struct timeval  t1, t2;
#else
  struct timespec t1, t2;
#endif
  double          time;
  sigset_t        sigblock;
  pthread_t       tid;
  char filename[256], etname[256];
  unsigned int *pdata;

  /* handy data for testing */
  int   numbers[] = {0,1,2,3,4,5,6,7,8,9};
  char   *stuff[] = {"One","Two","Three","Four","Five","Six","Seven","Eight","Nine","Ten"};
  int   control[] = {17,8,-1,-1}; /* 17,8 are arbitrary */
  
  if (argc != 3)
  {
    printf("Usage: %s <evio_filename> <et_filename>\n", argv[0]);
    exit(1);
  }
  else
  {
    strcpy(filename,argv[1]);
    strcpy(etname,argv[2]);
    printf("Using evio input file >%s< and et system file >%s<\n",filename,etname);
  }

  timeout.tv_sec  = 0;
  timeout.tv_nsec = 1;


  /*************************/
  /* setup signal handling */
  /*************************/
  /* block all signals */
  sigfillset(&sigblock);
  status = pthread_sigmask(SIG_BLOCK, &sigblock, NULL);
  if (status != 0) {
    printf("%s: pthread_sigmask failure\n", argv[0]);
    exit(1);
  }

#ifdef sun
  /* prepare to run signal handling thread concurrently */
  thr_setconcurrency(thr_getconcurrency() + 1);
#endif

  /* spawn signal handling thread */
  pthread_create(&tid, NULL, signal_thread, (void *)NULL);
  
restartLinux:
  /* open ET system */
  et_open_config_init(&openconfig);
  if (et_open(&id, etname, openconfig) != ET_OK)
  {
    printf("%s: et_open problems\n", argv[0]);
    exit(1);
  }
  et_open_config_destroy(openconfig);
 
  /* set level of debug output (everything) */
  et_system_setdebug(id, ET_DEBUG_INFO);
  

  /*
   * Now that we have access to an ET system, find out have many
   * events it has and what size they are. Then allocate an array
   * of pointers to use for reading, writing, and modifying these events.
   */
  if (et_system_getnumevents(id, &nevents_max) != ET_OK)
  {
    printf("et_client: ET has died");
    exit(1);
  }
  if (et_system_geteventsize(id, &event_size) != ET_OK)
  {
    printf("et_client: ET has died");
    exit(1);
  }
  printf("ET system info: event_size=%d bytes, the number of events = %d\n",event_size, nevents_max);
  if ( (pe = (et_event **) calloc(nevents_max, sizeof(et_event *))) == NULL)
  {
    printf("et_client: cannot allocate memory");
    exit(1);
  }



  /* attach to grandcentral station */
  if (et_station_attach(id, ET_GRANDCENTRAL, &attach1) < 0)
  {
    printf("%s: error in et_station_attach\n", argv[0]);
    exit(1);
  }
  /*printf("attached to gc, att = %d, pid = %d\n", attach1, getpid());*/


  /* open eviofile */


  if ( (status = evOpen(filename,"r",&handle)) !=0)
  {
	printf("cannot open >%d<, evOpen status %d \n",filename,status);
    fflush(stdout);
    goto error;
  }

  while (et_alive(id))
  {
    /* read time for future statistics calculations */
#if defined linux || defined __APPLE__
    gettimeofday(&t1, NULL);
#else
    clock_gettime(CLOCK_REALTIME, &t1);
#endif

    /* loop NUMLOOPS times before printing out statistics */
    evcount = 0;
    datacount = 0.0;
    for (j=0; j<NUMLOOPS; j++)
    {
      status = et_events_new(id, attach1, pe, ET_SLEEP, NULL, event_size, CHUNK, &count);

      if (status == ET_OK)
      {
        ;
      }
      else if (status == ET_ERROR_DEAD) {
        printf("%s: ET system is dead\n", argv[0]);
        break;
      }
      else if (status == ET_ERROR_TIMEOUT) {
        printf("%s: got timeout\n", argv[0]);
        break;
      }
      else if (status == ET_ERROR_EMPTY) {
        printf("%s: no events\n", argv[0]);
        break;
      }
      else if (status == ET_ERROR_BUSY) {
        printf("%s: grandcentral is busy\n", argv[0]);
        break;
      }
      else if (status == ET_ERROR_WAKEUP) {
        printf("%s: someone told me to wake up\n", argv[0]);
        break;
      }
      else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
        printf("%s: socket communication error\n", argv[0]);
        break;
      }
      else if (status != ET_OK) {
        printf("%s: request error\n", argv[0]);
        goto error;
      }

      /* write data, set priority, set control values here */
      for(i=0; i<count; i++)
      {
        /* the following line will allow et_client modes 3 & 4 to work */
        /* et_event_setcontrol(pe[i], control, 4); */
        et_event_getdata(pe[i], (void **) &pdata);

        /*new et requirement: need to form buffer in certain format by following call*/
        status = evOpenBuffer((char *)pdata, MAXBUF, "w", &handle1);
        if(status!=0) printf("evOpenBuffer returns %d\n",status);



next_event:

		if((status = evRead(handle,buf,maxbufbytes)) == EOF)
		{
          /*printf("reopen data file >%s<\n",filename);*/
          status = evClose(handle);
          status = evOpen(filename,"r",&handle);
		  status=evRead(handle,buf,maxbufbytes);
		}

		/*does not work for evRead yet, will be fixed in new evio release - Sergey 5-nov-2013
        status = evGetBufferLength(handle,&buflen);
		*/
        buflen = buf[0]+1;

        if(status!=0) printf("evGetBufferLength returns %d\n",status);

		/*printf("evGetBufferLength: buffer length = %d bytes\n",buflen*4);*/
        if(buflen > event_size/4)
		{
          printf("ERROR: event size from the file is %d bytes which is bigger then ET system event size %d bytes\n",buflen*4,event_size);
          printf("       Increase the size of ET system event accordingly (parameter '-s' in 'et_start')\n");
          exit(1);
		}

		
		/*ignore special events */
        event_type = (buf[1]>>16)&0xffff; /* actually it is bank tag */
		/*
        for(i=0; i<50; i++) printf("[%2d] 0x%08x\n",i,buf[i]);
		exit(0);
		*/
        if(event_type==17||event_type==18||event_type==20)
		{
          printf("Skip control event type %d\n",event_type);
          goto next_event;
		}




        status = evWrite(handle1, buf);
        if(status!=0) printf("evWrite returns %d\n",status);

        evGetBufferLength(handle1,&len);
		/*printf("len=%d buflen=%d\n",len, buflen);*/
/*sergey: evGetBufferLength() returns len=32, use buflen instead*/
        len = buflen<<2;

        status = evClose(handle1);
        if(status!=0) printf("evClose returns %d\n",status);
		

		/*			
        printf("---> len=%d\n",len);
        printf("2 data(hex): 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			   pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7],pdata[8],pdata[9]);
        printf("2 data(dec): %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d\n\n",
			   pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7],pdata[8],pdata[9]);
		*/

        datacount += (double)len;
        et_event_setlength(pe[i], len);
	  }
      evcount += count;




      /* put events back into the ET system */
      status = et_events_put(id, attach1, pe, count);
      if (status == ET_OK) {
        ;
      }
      else if (status == ET_ERROR_DEAD) {
        printf("%s: ET is dead\n", argv[0]);
        break;
      }
      else if ((status == ET_ERROR_WRITE) || (status == ET_ERROR_READ)) {
        printf("%s: socket communication error\n", argv[0]);
        break;
      }
      else if (status != ET_OK) {
        printf("%s: put error, status = %d\n", argv[0], status);
        goto error;
      }
    } /* for NUMLOOPS */
  
    /* statistics */
#if defined linux || defined __APPLE__
    gettimeofday(&t2, NULL);
    time = (double)(t2.tv_sec - t1.tv_sec) + 1.e-6*(t2.tv_usec - t1.tv_usec);
#else
    clock_gettime(CLOCK_REALTIME, &t2);
    time = (double)(t2.tv_sec - t1.tv_sec) + 1.e-9*(t2.tv_nsec - t1.tv_nsec);
#endif
    freq = evcount / time;
    datafreq = datacount / time;

    if ((DOUBLE_MAX - freq_tot) < freq)
    {
      freq_tot   = 0.0;
      datafreq_tot = 0.0;
	  iterations = 1;
    }
    freq_tot += freq;
    datafreq_tot += datafreq;
    freq_avg = freq_tot/(double)iterations;
    datafreq_avg = datafreq_tot/(double)iterations;
    iterations++;
    printf("%s: event rate %9.1f Hz (%9.1f Hz Avg),    data rate %7.3f MByte/sec (%7.3f MByte/sec Avg)\n",argv[0],
      freq,freq_avg,datafreq/1000000.,datafreq_avg/1000000.);

    /* if ET system is dead, wait here until it comes back */
    if (!et_alive(id))
    {
      status = et_wait_for_alive(id);
	  if (status == ET_OK)
      {
        int locality;
        et_system_getlocality(id, &locality);
        /* if Linux, re-establish connection to ET system since socket broken */
        if (locality == ET_LOCAL_NOSHARE)
        {
          printf("%s: try to reconnect Linux client\n", argv[0]);
	      et_forcedclose(id);
	      goto restartLinux;
	    }
	  }
    }
      
  } /* while(alive) */
    
  
  error:
    printf("%s: ERROR\n", argv[0]);
    exit(0);
}

/************************************************************/
/*              separate thread to handle signals           */
static void *
signal_thread (void *arg)
{
  sigset_t   signal_set;
  int        sig_number;
 
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  
  /* Not necessary to clean up as ET system will do it */
  sigwait(&signal_set, &sig_number);
  printf("Got a control-C, exiting\n");
  exit(1);
}
