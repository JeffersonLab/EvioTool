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

char *dictionary =
"<xmlDict>\n"
"  <xmldumpDictEntry name=\"Tag1-Num1\"   tag=\"1\"   num=\"1\"/>\n"
"  <xmldumpDictEntry name=\"Tag2-Num2\"   tag=\"2\"   num=\"2\"/>\n"
"  <xmldumpDictEntry name=\"Tag3-Num3\"   tag=\"3\"   num=\"3\"/>\n"
"  <xmldumpDictEntry name=\"Tag4-Num4\"   tag=\"4\"   num=\"4\"/>\n"
"  <xmldumpDictEntry name=\"Tag5-Num5\"   tag=\"5\"   num=\"5\"/>\n"
// "  <xmldumpDictEntry name=\"Tag6-Num6\"   tag=\"6\"   num=\"6\"/>\n"
"  <xmldumpDictEntry name=\"Tag7-Num7\"   tag=\"7\"   num=\"7\"/>\n"
"  <xmldumpDictEntry name=\"Tag8-Num8\"   tag=\"8\"   num=\"8\"/>\n"
"</xmlDict>\n";

static void printBuffer(uint32_t *p, int len) {
    int i;
    printf("\nBUFFER:\n");
    for (i=0; i < len; i++) {
        printf("%d   0x%x\n", i, *(p++));
    }
    printf("\n");
}


int main()
{
    int handle, status, maxEvBlk = 2, bufLen;
    uint32_t bufLenBytes, bank1[3], bank2[3], bank3[3];
    char eventBuffer[4 * ((4*8) + (5*3))];
    bufLen = (4*8) + (5*3);
    bufLenBytes = 4 * bufLen;
    memset(eventBuffer, 0, bufLenBytes);

    // create 3 little banks
    // length of 2 ints
    bank1[0] = bank2[0] = bank3[0] = 2;
    // tag = 1,2,3, banks contains ints, num = 1,2,3 
    bank1[1] = 1 << 16 | 0x1 << 8 | 1;
    bank2[1] = 2 << 16 | 0x1 << 8 | 2;
    bank3[1] = 3 << 16 | 0x1 << 8 | 3;
    // data = 1,2,3
    bank1[2] = 1;
    bank2[2] = 2;
    bank3[2] = 3;
    


    printf("\nEvent I/O tests to BUFFER  ...\n");
    status = evOpenBuffer(eventBuffer, bufLen, "w", &handle);
    printf ("    Opened buffer, status = %#x\n", status);

//    status = evWriteDictionary(handle, dictionary);
//    printf ("    Write dictionary to buffer, status = %#x\n\n", status);

    status = evIoctl(handle, "N", (void *) (&maxEvBlk));
    printf ("    Changed max events/block to %d, status = %#x\n", maxEvBlk, status);

    status = evWrite(handle, bank1);
    printf ("    Wrote event #1 to buffer, status = %#x\n",status);
    if (status != S_SUCCESS) {
        printf("Error writing, %s, quit\n", evPerror(status));
        exit(1);
    }

    status = evGetBufferLength(handle, &bufLenBytes);
    printf ("    Wrote %u bytes to buffer\n", bufLenBytes);
    
printBuffer((uint32_t *) eventBuffer, bufLenBytes/4);

    status = evWrite(handle, bank2);
    printf ("    Wrote event #2 to buffer, status = %#x\n",status);
    if (status != S_SUCCESS) {
        printf("Error writing, %s, quit\n", evPerror(status));
        exit(1);
    }

    status = evGetBufferLength(handle, &bufLenBytes);
    printf ("    Wrote %u bytes to buffer\n", bufLenBytes);

printBuffer((uint32_t *) eventBuffer, bufLenBytes/4);

    status = evWrite(handle, bank3);
    printf ("    Wrote event #3 to buffer, status = %#x\n",status);
    if (status != S_SUCCESS) {
        printf("Error writing, %s, quit\n", evPerror(status));
        exit(1);
    }

    status = evGetBufferLength(handle, &bufLenBytes);
    printf ("    Wrote %u bytes to buffer\n", bufLenBytes);

printBuffer((uint32_t *) eventBuffer, bufLenBytes/4);

    status = evWrite(handle, bank1);
    printf ("    Wrote event #1 to buffer, status = %#x\n",status);
    if (status != S_SUCCESS) {
        printf("Error writing, %s, quit\n", evPerror(status));
        exit(1);
    }

    status = evGetBufferLength(handle, &bufLenBytes);
    printf ("    Wrote %u bytes to buffer\n", bufLenBytes);
    
printBuffer((uint32_t *) eventBuffer, bufLenBytes/4);

    status = evWrite(handle, bank2);
    printf ("    Wrote event #2 to buffer, status = %#x\n",status);
    if (status != S_SUCCESS) {
        printf("Error writing, %s, quit\n", evPerror(status));
        exit(1);
    }

    status = evGetBufferLength(handle, &bufLenBytes);
    printf ("    Wrote %u bytes to buffer\n", bufLenBytes);

    status = evClose(handle);
    printf ("    \"Closed\" buffer, status = %#x\n\n", status);

printBuffer((uint32_t *) eventBuffer, bufLen);

}
