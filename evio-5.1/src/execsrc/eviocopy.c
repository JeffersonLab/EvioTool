/*
 *  eviocopy.c
 *
 *   extracts evio events and copies them to another evio file
 *
 *   Author: Elliott Wolin, JLab, 12-sep-2001
*/

/* still to do
 * -----------
 *
*/



/* for posix */
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__


/*  misc macros, etc. */
#define MAXEVIOBUF 1000000


/* include files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "evio.h"


/*  misc variables */
static unsigned int buf[MAXEVIOBUF];
static char *input_filename;
static int input_handle;
static char *output_filename;
static int output_handle;
static int nevent         = 0;
static int nwrite         = 0;
static int skip_event     = 0;
static int max_event      = 0;
static int nevok          = 0;
static int evok[100];
static int nnoev          = 0;
static int noev[100];
static int nnonum         = 0;
static int nonum[100];
static int debug          = 0;


/* prototypes */
void decode_command_line(int argc, char **argv);
int user_event_select(unsigned int *buf);

/*--------------------------------------------------------------------------*/


int main (int argc, char **argv) {

  int status;
  // int l;
  

  /* decode command line */
  decode_command_line(argc,argv);


  /* open evio input file */
  if((status=evOpen(input_filename,"r",&input_handle))!=0) {
    printf("\n ?Unable to open input file %s, status=%d\n\n",input_filename,status);
    exit(EXIT_FAILURE);
  }


  /* open evio output file */
  if((status=evOpen(output_filename,"w",&output_handle))!=0) {
    printf("\n ?Unable to open output file %s, status=%d\n\n",output_filename,status);
    exit(EXIT_FAILURE);
  }


  /* debug...need to set large block size ??? */
/*   l=0x8000; */
/*   status=evIoctl(output_handle,"b",(void*)&l); */
/*   if(status!=0) { */
/*     printf("\n ?evIoctl error on output file %s, status=%d\n\n",output_filename,status); */
/*     exit(EXIT_FAILURE); */
/*   } */


  /* loop over events, skip some, copy up to max_event events */
  nevent=0;
  nwrite=0;
  while ((status=evRead(input_handle,buf,MAXEVIOBUF))==0) {
    nevent++;
    if(skip_event>=nevent)continue;
    if(user_event_select(buf)==0)continue;
    nwrite++;
    status=evWrite(output_handle,buf);
    if(status!=0) {
      printf("\n ?evWrite error output file %s, status=%d\n\n",output_filename,status);
      exit(EXIT_FAILURE);
    }
    if( (max_event>0) && (nevent>=max_event+skip_event) )break;
  }


  /* done */
  printf("\n  Read %d events, copied %d events\n\n",nevent,nwrite);
  evClose(output_handle);
  evClose(input_handle);
  exit(EXIT_SUCCESS);
}


/*---------------------------------------------------------------- */


int user_event_select(unsigned int *buf) {

  int i;
  int event_tag = buf[1]>>16;


  if((nevok<=0)&&(nnoev<=0)&&(nnonum<=0)) {
    return(1);

  } else if(nevok>0) {
    for(i=0; i<nevok; i++) if(event_tag==evok[i])return(1);
    return(0);
    
  } else if(nnoev>0) {
    for(i=0; i<nnoev; i++) if(event_tag==noev[i])return(0);
    return(1);

  } else if(nnonum>0) {
    for(i=0; i<nnonum; i++) if(nevent==nonum[i])return(0);
    return(1);
  }

  return(0);
}


/*---------------------------------------------------------------- */


void decode_command_line(int argc, char**argv) {
  
  const char *help = 
    "\nusage:\n\n  eviocopy [-max max_event] [-skip skip_event] \n"
    "           [-ev evtag] [-noev evtag] [-nonum evnum] [-debug] input_filename output_filename\n";
  int i;
    
    
  if(argc<2) {
    printf("%s\n",help);
    exit(EXIT_SUCCESS);
  } 


  /* loop over arguments */
  i=1;
  while (i<argc) {
    if (strncasecmp(argv[i],"-h",2)==0) {
      printf("%s\n",help);
      exit(EXIT_SUCCESS);

    } else if (strncasecmp(argv[i],"-debug",6)==0) {
      debug=1;
      i=i+1;

    } else if (strncasecmp(argv[i],"-max",4)==0) {
      max_event=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-skip",5)==0) {
      skip_event=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-ev",3)==0) {
      if(nevok<(sizeof(evok)/sizeof(int))) {
	evok[nevok++]=atoi(argv[i+1]);
	i=i+2;
      } else {
	printf("?too many ev flags: %s\n",argv[i+1]);
      }

    } else if (strncasecmp(argv[i],"-noev",5)==0) {
      if(nnoev<(sizeof(noev)/sizeof(int))) {
	noev[nnoev++]=atoi(argv[i+1]);
	i=i+2;
      } else {
	printf("?too many noev flags: %s\n",argv[i+1]);
      }

    } else if (strncasecmp(argv[i],"-nonum",6)==0) {
      if(nnonum<(sizeof(nonum)/sizeof(int))) {
	nonum[nnonum++]=atoi(argv[i+1]);
	i=i+2;
      } else {
	printf("?too many nonum flags: %s\n",argv[i+1]);
      }

    } else if (strncasecmp(argv[i],"-",1)==0) {
      printf("\n  ?unknown command line arg: %s\n\n",argv[i]);
      exit(EXIT_FAILURE);

    } else {
      break;
    }
  }
  
  /* last two args better be filenames */
  input_filename=argv[argc-2];
  output_filename=argv[argc-1];

  return;
}


/*---------------------------------------------------------------- */
