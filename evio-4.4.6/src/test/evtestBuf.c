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
#include "evio.h"

uint32_t *makeEvent();
#define MIN(a,b) (a<b)? a : b

char *dictionary =
"<xmlDict>\n"
"  <xmldumpDictEntry name=\"Tag1-Num1\"   tag=\"1\"   num=\"1\"/>\n"
"  <xmldumpDictEntry name=\"Tag2-Num2\"   tag=\"2\"   num=\"2\"/>\n"
"  <xmldumpDictEntry name=\"Tag3-Num3\"   tag=\"3\"   num=\"3\"/>\n"
"  <xmldumpDictEntry name=\"Tag4-Num4\"   tag=\"4\"   num=\"4\"/>\n"
"  <xmldumpDictEntry name=\"Tag5-Num5\"   tag=\"5\"   num=\"5\"/>\n"
"  <xmldumpDictEntry name=\"Tag6-Num6\"   tag=\"6\"   num=\"6\"/>\n"
"  <xmldumpDictEntry name=\"Tag7-Num7\"   tag=\"7\"   num=\"7\"/>\n"
"  <xmldumpDictEntry name=\"Tag8-Num8\"   tag=\"8\"   num=\"8\"/>\n"
"</xmlDict>\n";


int main()
{
    int handle, status, nevents, nwords;
    int i, maxEvBlk = 2;
    uint32_t *ip, *pBuf, buffer[2048], bufLen, dictLen;
    char eventBuffer[4*4096], *dict;

    memset(eventBuffer, 0, 4*4096);


    printf("\nEvent I/O tests to BUFFER (%p) ...\n", eventBuffer);
    status = evOpenBuffer(eventBuffer, 4096, "w", &handle);
    printf ("    Opened buffer, status = %#x\n", status);

    status = evWriteDictionary(handle, dictionary);
    printf ("    Write dictionary to buffer, status = %#x\n\n", status);

    pBuf = ip = makeEvent();
    
    printf ("    Will write ** SINGLE ** event to buffer, status = %#x\n",status);
    status = evWrite(handle, ip);

    status = evGetBufferLength(handle, &bufLen);
    printf ("    Written %d bytes to buffer (before close)\n\n", bufLen);

    status = evClose(handle);
    printf ("    \"Closed\" buffer, status = %#x\n\n", status);


    status = evGetBufferLength(handle, &bufLen);
    printf ("    Written %d bytes to buffer (after close)\n\n", bufLen);

    printf ("    Will reopen buffer for reading\n");
    status = evOpenBuffer(eventBuffer, 4096, "r", &handle);
    printf ("    Reopened buffer for reading, status = %#x\n\n\n", status);

    status = evGetDictionary(handle, &dict, &dictLen);
    printf ("    get dictionary, status = %#x\n\n", status);

    if (dictionary != NULL) {
        printf("DICTIONARY =\n%s\n", dict);
        free(dict);
    }
    
    nevents = 0;
    
    while ((status = evRead(handle, buffer, 2048)) == S_SUCCESS) {
        nevents++;
        
        printf("    Event #%d,  len = %d data words\n", nevents, buffer[0] - 1);

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
    
    status = evClose(handle);
    printf ("    Closed buffer again, status = %#x\n\n", status);

    ip = pBuf;
    
    printf ("    Try opening multiple.dat\n");
    status = evOpenBuffer(eventBuffer, 4096, "w", &handle);
    printf ("    Opened buffer for multiple writes, status = %#x\n", status);

    status = evIoctl(handle, "N", (void *) (&maxEvBlk));
    printf ("    Changed max events/block to %d, status = %#x\n", maxEvBlk, status);
    
    printf ("    Will write 3 events to buffer\n");
    evWrite(handle,ip);
    evWrite(handle,ip);
    evWrite(handle,ip);

    status = evClose(handle);
    printf ("    Closed buffer, status %#x\n\n", status);

    printf ("    Will reopen buffer for reading\n");
    status = evOpenBuffer(eventBuffer, 4096, "r", &handle);
    printf ("    Reopened buffer for reading, status = %#x\n", status);

    nevents = 0;
    
    while ((status = evRead(handle, buffer, 2048)) == S_SUCCESS) {
        nevents++;
        
        printf("    Event #%d,  len = %d data words\n", nevents, buffer[0] - 1);

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
    
    printf("\n    Last read, status = %x\n", status);
    if (status == EOF) {
        printf("    Last read, reached EOF!\n");
    }
    
    evClose(handle);

    free(pBuf);
}


uint32_t *makeEvent()
{
    uint32_t *bank, *segment;
    short *word;


    bank = (uint32_t *) calloc(1, 11*sizeof(int32_t));
    bank[0] = 10;                    /* event length = 10 */
    bank[1] = 1 << 16 | 0x20 << 8;   /* tag = 1, bank 1 contains segments */

    segment = &(bank[2]);
    segment[0] = 2 << 24 | 0xb << 16 | 2; /* tag = 2, seg 1 has 2 - 32 bit ints, len = 2 */
    segment[1] = 0x1;
    segment[2] = 0x2;
        
    segment += 3;
        
    segment[0] = 3 << 24 | 2 << 22 | 4 << 16 | 2; /* tag = 3, 2 bytes padding, seg 2 has 3 shorts, len = 2 */
    word = (short *) &(segment[1]);
    word[0] = 0x3;
    word[1] = 0x4;
    word[2] = 0x5;

    segment += 3;

    /* HI HO - 2 strings */
    segment[0] =    4 << 24 | 0x3 << 16 | 2; /* tag = 4, seg 3 has 2 strings, len = 2 */
    segment[1] = 0x48 << 24 | 0 << 16   | 0x49 << 8 | 0x48 ;   /* H \0 I H */
    segment[2] =   4  << 24 | 4 << 16   | 0 << 8    | 0x4F ;   /* \4 \4 \0 O */

    return(bank);
}

