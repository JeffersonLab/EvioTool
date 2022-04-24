
/* et2evio.c */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "evio.h"
#include "et.h"

#define ET_EVENT_ARRAY_SIZE 3/*100*/
static char et_name[ET_FILENAME_LENGTH];
static et_stat_id  et_statid;
static et_sys_id   et_sys;
static et_att_id   et_attach;
static int         et_locality, et_init = 0, et_reinit = 0;
static et_event *pe[ET_EVENT_ARRAY_SIZE];
static int PrestartCount, prestartEvent=17, endEvent=20;

/* ET Initialization */    
int
et_initialize(void)
{
  et_statconfig   sconfig;
  et_openconfig   openconfig;
  int             status;
  struct timespec timeout;

  timeout.tv_sec  = 2;
  timeout.tv_nsec = 0;

  /* Normally, initialization is done only once. However, if the ET
   * system dies and is restarted, and we're running on a Linux or
   * Linux-like operating system, then we need to re-initalize in
   * order to reestablish the tcp connection for communication etc.
   * Thus, we must undo some of the previous initialization before
   * we do it again.
   */
  if(et_init > 0)
  {
    /* unmap shared mem, detach attachment, close socket, free et_sys */
    et_forcedclose(et_sys);
  }
  
  printf("er_et_initialize: start ET stuff\n");

  if(et_open_config_init(&openconfig) != ET_OK)
  {
    printf("ERROR: er ET init: cannot allocate mem to open ET system\n");
    return(-1);;
  }
  et_open_config_setwait(openconfig, ET_OPEN_WAIT);
  et_open_config_settimeout(openconfig, timeout);
  if(et_open(&et_sys, et_name, openconfig) != ET_OK)
  {
    printf("ERROR: er ET init: cannot open ET system\n");
    return(-2);;
  }
  et_open_config_destroy(openconfig);

  /* set level of debug output */
  et_system_setdebug(et_sys, ET_DEBUG_ERROR);

  /* where am I relative to the ET system? */
  et_system_getlocality(et_sys, &et_locality);

  et_station_config_init(&sconfig);
  et_station_config_setselect(sconfig,  ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig,   ET_STATION_BLOCKING);
  et_station_config_setuser(sconfig,    ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig,1);

  if((status = et_station_create(et_sys, &et_statid, "ET2PRINT", sconfig)) < 0)
  {
    if (status == ET_ERROR_EXISTS) {
      printf("er ET init: station exists, will attach\n");
    }
    else
    {
      et_close(et_sys);
      et_station_config_destroy(sconfig);
      printf("ERROR: er ET init: cannot create ET station (status = %d)\n",
        status);
      return(-3);
    }
  }
  et_station_config_destroy(sconfig);

  if (et_station_attach(et_sys, et_statid, &et_attach) != ET_OK) {
    et_close(et_sys);
    printf("ERROR: er ET init: cannot attached to ET station\n");
    return(-4);;
  }

  et_init++;
  et_reinit = 0;
  printf("er ET init: ET fully initialized\n");
  return(0);
}

int 
gotControlEvent(et_event **pe, int size)
{
  int i;
    
  for (i=0; i<size; i++)
  {
    if ((pe[i]->control[0] == 17) || (pe[i]->control[0] == 20))
    {
      return(1);
    }
  }
  return(0);
}





#define DEBUG
 

#define MAXEVENTS 2000000

#define MAXBUF 10000000
unsigned int buf[MAXBUF];
unsigned int *bufptr;





int
main(int argc, char **argv)
{
  FILE *fd = NULL;
  int bco1[256], bco2[256], bco3[256], bco4[256], nbco1, nbco2, nbco3, nbco4, diff, diff1, diff2;
  int nfile, nevents, etstart, etstop, len;
  char filename[1024];
  int fd_evio, status, ifpga, nchannels, tdcref;
  unsigned long long *b64, timestamp, timestamp_old;
  unsigned int *b32;
  unsigned short *b16;
  unsigned char *b08;
  int trig,chan,fpga,apv,hybrid;
  int i1, type, timestamp_flag;
  float f1,f2;
  unsigned int word;
  int iet, maxevents;
  int handle1, buflen, recl;
  size_t size;

  int nr,sec,strip,nl,ncol,nrow,i,j, k, ii,jj,kk,l,l1,l2,ichan,nn,mm,iev,nbytes,ind1;
  char title[128], *ch;
  char HBOOKfilename[256], chrunnum[32];
  int runnum;
  int nwpawc,lun,lrec,istat,icycle,idn,nbins,nbins1,igood,offset;
  float x1,x2,y1,y2,ww,tmpx,tmpy,ttt,ref;
  /*
  int goodevent, icedev;
  */
  if(argc != 2 && argc != 3)
  {
    printf("Usage: et2print <et_filename> [<output_evio_filename>]\n");
    exit(1);
  }



  /* check if et file exist */
  fd = fopen(argv[1],"r");
  if(fd!=NULL)
  {
    fclose(fd);
    strncpy(et_name,argv[1],ET_FILENAME_LENGTH);
    printf("attach to ET system >%s<\n",et_name);
  }
  else
  {
    printf("ET system >%s< does not exist - exit\n",argv[1]);
    exit(0);
  }
  /*
  if (!et_alive(et_sys))
  {
    printf("ERROR: not attached to ET system\n");
    et_reinit = 1;
    exit(0);
  }
  */
  if(et_initialize() != 0)
  {
    printf("ERROR: cannot initalize ET system\n");
    exit(0);
  }


  /* open output evio file if specified */
  fd_evio = 0;
  if(argc == 3)
  {
    status = evOpen(argv[2],"w",&fd_evio);
    if (status)
    {
      char *errstr = strerror(status);
      printf("ERROR: Unable to open event file - %s : %s\n",
        argv[2],errstr);
      exit(0);
    }
    else
    {
      printf("evOpen(\"%s\",\"w\",%d)\n",argv[2],fd_evio);
      recl = 2047;
    }
  }





  runnum = 1; /* temporary fake it, must extract from et */
  printf("run number is %d\n",runnum);





iev = 0;
nfile = 0;
while(1)
{


  {
    status = et_events_get(et_sys, et_attach, pe, ET_SLEEP,
                            NULL, ET_EVENT_ARRAY_SIZE, &nevents);

    printf("INFO: et_events_get() returns %d, nevents=%d\n",status,nevents);

    /* if no events or error ... */
    if ((nevents < 1) || (status != ET_OK))
    {
      /* if status == ET_ERROR_EMPTY or ET_ERROR_BUSY, no reinit is necessary */
      
      /* will wake up with ET_ERROR_WAKEUP only in threaded code */
      if (status == ET_ERROR_WAKEUP)
      {
        printf("status = ET_ERROR_WAKEUP\n");
      }
      else if (status == ET_ERROR_DEAD)
      {
        printf("status = ET_ERROR_DEAD\n");
        et_reinit = 1;
      }
      else if (status == ET_ERROR)
      {
        printf("error in et_events_get, status = ET_ERROR \n");
        et_reinit = 1;
      }
    }
    else /* if we got events */
    {
      /* by default (no control events) write everything */
      etstart = 0;
      etstop  = nevents - 1;

	  printf("1\n");

      /* if we got control event(s) */
      if (gotControlEvent(pe, nevents))
      {
        /* scan for prestart and end events */
        for (i=0; i<nevents; i++)
        {
	      if (pe[i]->control[0] == prestartEvent)
          {
	        printf("Got Prestart Event!!\n");
	        /* look for first prestart */
	        if (PrestartCount == 0)
            {
	          /* ignore events before first prestart */
	          etstart = i;
	          if (i != 0)
              {
	            printf("ignoring %d events before prestart\n",i);
	          }
	        }
            PrestartCount++;
	      }
	      else if (pe[i]->control[0] == endEvent)
          {
	        /* ignore events after last end event & quit */
            printf("Got End event\n");
            etstop = i;
	      }
        }
      }
	  printf("2\n");
	}
    maxevents = iev + etstop; 
    iet = 0;
    printf("iev=%d, etstop=%d maxevents=%d\n",iev,etstop,maxevents);
  }

  timestamp_old = 0;



/*by-pass prestart-end logic*/
maxevents = nevents;
iet=0;


  while(iet/*iev*/<maxevents)
  {
    iev ++;

    if(!(iev%10000)) printf("\n\n\nEvent %d\n\n",iev);
    printf("\n\n\nEvent %d, iet=%d, maxevents=%d\n\n",iev,iet,maxevents);


      if(iet >= maxevents)
	  {
        printf("ERROR: iet=%d, maxevents=%d\n",iet,maxevents);
        exit(0);
	  }


      et_event_getlength(pe[iet], &size); /*get event length from et*/
      len = size;
	  /*if(len==2388)*/ printf("event length=%d\n",len);


#if 0

	  /*following 2 lines are working, but 'official' is to use  evOpenBuffer()
      bufptr = (unsigned int *)pe[iet]->pdata;
      bufptr += 8;
	  */
      status = evOpenBuffer(pe[iet]->pdata, MAXBUF, "r", &handle1);
      if(status!=0) {printf("evOpenBuffer returns %d\n",status);exit(1);}




      /* in following, you can ether copy from et, or work directly in et */

	  /*actual copy from et
      status = evRead(handle1, buf, MAXBUF);
      if(status!=0) {printf("evRead returns %d\n",status);exit(1);}
	  bufptr = buf;
      */

	  /*get pointer to et buffer (no coping) */
      status = evReadNoCopy(handle1, &bufptr, &buflen);
      if(status!=0) {printf("evReadNoCopy returns %d\n",status);exit(1);}



	  

	  /*
	  printf("buf: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			 bufptr[0],bufptr[1],bufptr[2],bufptr[3],bufptr[4],
			 bufptr[5],bufptr[6],bufptr[7],bufptr[8],bufptr[9]);
	  */
goto a123;
      status = evOpenBuffer(pe[iet]->pdata, MAXBUF, "w", &handle1); /*open 'buffer' in et*/
      if(status!=0) printf("evOpenBuffer returns %d\n",status);
      status = evWrite(handle1, buf); /*write event to the 'buffer'*/
      if(status!=0) printf("evWrite returns %d\n",status);
      evGetBufferLength(handle1,&len); /*get 'buffer' length*/
	  /*printf("len2=%d\n",len);*/
      status = evClose(handle1); /*close 'buffer'*/
      if(status!=0) printf("evClose returns %d\n",status);
      et_event_setlength(pe[iet], len); /*update event length in et*/
a123:


#endif

      iet ++;


	  if(fd_evio)
	  {
        status = evWrite(fd_evio, bufptr);
        if(status!=0)
        {
          printf("evWrite returns %d\n",status);
          exit(0);
        }
	  }


#if 0
      status = evClose(handle1);
      if(status!=0) {printf("evClose returns %d\n",status);exit(1);}
#endif


  } /*while*/



  {
    /* put et events back into system */
    status = et_events_put(et_sys, et_attach, pe, nevents);            
    if (status != ET_OK)
    {
	  printf("error in et_events_put, status = %i \n",status);
      et_reinit = 1;
    }
  }

  if(iev>=MAXEVENTS)
  {
    break;
  }

} /*while*/

  if(fd_evio)
  {
    evClose(fd_evio);
  }


  exit(0);
}
