/*
 *  evTestHandles
 *
 *  test how many evOpen handles can be created without error.
 *
 *  timmer, 24-Jan-2014
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include "evio.h"


int main (int argc, char **argv) {

    int i, handle, status, maxloop = 11;
    uint32_t *buf, bufLen;

    
    if(argc < 2) {
        printf("\nusage:\n    evTestHandles filename openTries\n\n");
        exit(-1);
    }
  
    if(argc>=3) maxloop=atoi(argv[2]);

    /* open file */
    for (i=0; i < maxloop; i++) {
        printf("Try # %d\n", (i+1));
        if ( (status = evOpen(argv[1],"r", &handle)) != S_SUCCESS) {
            printf("Unable to open file %s, status = %d, at try # %d\n",argv[1],status, (i+1));
            exit(-1);
        }

        status = evReadAlloc(handle, &buf, &bufLen);
        printf("    Event len = %d words\n", buf[0]);

        free(buf);
    }
  
    exit(0);
}


/*------------------------------------------------------------------------------*/
