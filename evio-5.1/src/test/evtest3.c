/*
 *  evtest3
 *
 *  tests evioswap 
 *
 *  ejw, 20-nov-2003
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include "evio.h"

#define MAXBUFLEN  100000



int main (int argc, char **argv) {

  int i,loop,handle, nevents, status;
  unsigned int buf[MAXBUFLEN];
  unsigned int buf2[MAXBUFLEN];
  unsigned int buf3[MAXBUFLEN];
  unsigned int other[MAXBUFLEN];
  int maxev   = 0;
  int maxloop = 1;
  

  if(argc < 2) {
    printf("\nusage:\n    evtest3 filename [maxloop]\n\n");
    exit(-1);
  }
  if(argc>=3) maxloop=atoi(argv[2]);


  /* open file */
  if ( (status = evOpen(argv[1],"r",&handle)) < 0) {
    printf("Unable to open file %s status = %d\n",argv[1],status);
    exit(-1);
  }


  /* read and swap */
  nevents=0;
  while ((status=evRead(handle,buf,MAXBUFLEN))==0) {
    nevents++;

    /* swap and save */
    for(loop=0; loop<maxloop; loop++) {

      evioswap(buf,0,buf2);  /* buf2 is now other */
      for(i=0;i<buf[0];i++)other[i]=buf2[i];
      
      evioswap(buf2,1,NULL); /* buf2 is now same */
      for(i=0; i<buf[0]; i++) {
	if(buf2[i]!=buf[i]) {
	  printf("?error event %d word %d in: %u same %u (%x)\n",
		 nevents,i,buf[i],buf2[i],buf2[i]);
	}
      }
      
      evioswap(buf2,0,NULL); /* buf2 is now other */
      for(i=0; i<buf[0]; i++) {
	if(buf2[i]!=other[i]) {
	  printf("?error event %d word %d in: %u other %u (%x)\n",
		 nevents,i,buf2[i],other[i],other[i]);
	}
      }
      
      evioswap(buf2,1,buf3); /* buf3 is now same */
    }  /* end loop */
    
    for(i=0; i<buf[0]; i++) {
      if(buf3[i]!=buf[i]) {
	printf("?error event %d word %d in: %u same %u (%x)\n",
	       nevents,i,buf[i],buf3[i],buf3[i]);
      }
    }
    

    if((nevents >= maxev) && (maxev != 0)) break;
  }


  /* done */
  printf("processed %d events\n",nevents);
  evClose(handle);
  exit(0);
}


/*------------------------------------------------------------------------------*/
