/************************************************* 
*    Simple test program for opening and reading *
*   an existing CODA event format file.          *
*                                                *
*   Author: David Abbott  CEBAF                  *
*                                                *
*   Arguments to the routine are:                *
*                                                *
*             evt <filename> [nevents]           *
*                                                *
**************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "evio.h"

#define MAXBUFLEN  4096
#define MIN(a,b) (a<b)? a : b


int main (int argc, char **argv)
{
    int i, handle, nevents, status, nWords, version;
    uint32_t *buf, *ip, dLen, bufLen;
    char *dictionary = NULL;


    if (argc != 2) {
        printf("Incorrect number of arguments:\n");
        printf("  usage: %s filename\n", argv[0]);
        exit(-1);
    }

    if ( (status = evOpen(argv[1], "r", &handle)) < 0) {
        printf("Unable to open file %s status = %d\n",argv[1],status);
        exit(-1);
    } else {
        printf("Opened %s for reading, hit <enter> to see each individual event\n\n",argv[1]);
    }

    /* Get evio version # of file */
    status = evIoctl(handle, "v", &version);
    if (status == S_SUCCESS) {
        printf("Evio file version = %d\n", version);
    }

    /* Get a dictionary if there is one */
    status = evGetDictionary(handle, &dictionary, &dLen);
    if (status == S_SUCCESS && dictionary != NULL) {
        printf("Dictionary =\n%s\n\n", dictionary);
        free(dictionary);
    }

    nevents=0;
    while ((status = evReadAlloc(handle, &buf, &bufLen))==0) {
        nevents++;
        printf("    Event #%d,  len = %d words\n", nevents, buf[0]);
        nWords = buf[0] + 1;

        for (ip=buf; nWords > 0; nWords-=4) {
            for (i = MIN(nWords,4); i > 0; i--) {
                printf("      %#10.8x", *ip++);
            }
            printf("\n");
        }
       
        printf("\n");

        free(buf);
        printf("Hit enter for next event\n");
        getc(stdin);
    }

    if ( status == EOF ) {
        printf("Hit end-of-file\n");
    }
    else {
        printf("Error reading file, quit\n");
    }
    evClose(handle);
  
    exit(0);
}
