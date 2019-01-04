/*
 * This program is for testing the new string manipulation
 * routines which facilitate the splitting and automatic
 * naming of files.
 *
 * author: Carl Timmer
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evio.h"


static uint32_t eventBuffer1[] =
{
    0x00000007,
    0x00011001,
    0x00000005,
    0x00020b02,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00000003
}; /* len = 8 words */

/*
static uint32_t eventBuffer2[] =
{
    0x00000011,
    0x00011001,
    0x0000000f,
    0x00020b02,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00000003,
    0x00000004,
    0x00000005,
    0x00000006,
    0x00000007,
    0x00000008,
    0x00000009,
    0x0000000a,
    0x0000000b,
    0x0000000c,
    0x0000000d
}; *//* len = 18 words */

/*
static uint32_t eventBuffer4[] =
{
    0x00000013,
    0x00011001,
    0x00000011,
    0x00020b02,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00000003,
    0x00000004,
    0x00000005,
    0x00000006,
    0x00000007,
    0x00000008,
    0x00000009,
    0x0000000a,
    0x0000000b,
    0x0000000c,
    0x0000000d,
    0x0000000e,
    0x0000000f
}; */ /* len = 20 words */

/*
static uint32_t eventBuffer3[] =
{
    0x0000000f,
    0x00011001,
    0x0000000d,
    0x00020b02,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00000003,
    0x00000004,
    0x00000005,
    0x00000006,
    0x00000007,
    0x00000008,
    0x00000009,
    0x0000000a,
    0x0000000b
}; *//* len = 16 words */


static char *filename  = "/daqfs/home/timmer/coda/evio-4.1/my$(FILE_ENV)run_%d_.dat_%4d";
static char *filename2 = "/daqfs/home/timmer/coda/evio-4.1/myFile";


/* xml dictionary */
/*
static char *xmlDictionary =
        "<xmlDict>\n"
        "  <bank name=\"My Event\"       tag=\"1\"   num=\"1\">\n"
        "     <bank name=\"Segments\"    tag=\"2\"   num=\"2\">\n"
        "       <leaf name=\"My Shorts\" tag=\"3\"   />\n"
        "     </bank>\n"
        "     <bank name=\"Banks\"       tag=\"1\"   num=\"1\">\n"
        "       <leaf name=\"My chars\"  tag=\"5\"   num=\"5\"/>\n"
        "     </bank>\n"
        "  </bank>\n"
        "  <dictEntry name=\"Last Bank\" tag=\"33\"  num=\"66\"/>\n"
        "  <dictEntry name=\"Test Bank\" tag=\"1\" />\n"
        "</xmlDict>";
*/

static char *xmlDictionary2 =
        "<xmlDict>\n"
        "  <dictEntry name=\"TAG1_NUM1\" tag=\"1\" num=\"1\"/>\n"
        "</xmlDict>\n";


int main (int argc, char **argv)
{
    int err, handle, arg;

    /* open file for writing & create a buffer */
    /*
    uint32_t bufLen = 60, buffer[60];;
    err = evOpenBuffer(buffer, bufLen, "w", &handle);
    if (err != S_SUCCESS) {
        printf("Error in evOpen(), err = %x\n", err);
        exit(0);
    }
    */
    
    /* open file for splitting */
    err = evOpen(filename2, "w", &handle);
    if (err != S_SUCCESS) {
        printf("Error in evOpen(), err = %x\n", err);
        exit(0);
    }

    /* events per block max */
    arg = 5;
    evIoctl(handle, "N", &arg);

     /*
    int blockSizeMax  = 80/4 + 4*32/4;
    int bufferSize = 4*60;
     */

    /* target block size words */
    arg = 80/4 + 4*32/4;
    err = evIoctl(handle, "B", &arg);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for target block size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting target block size\n");
        exit(0);
    }
        
    printf("\nsplitTest: write dictionary ...\n");
    err = evWriteDictionary(handle, xmlDictionary2);
    printf ("splitTest: write dictionary, err = %x\n\n", err);
    
    printf("\nsplitTest: write little event ...\n");
    err = evWrite(handle, eventBuffer1);
    if (err != S_SUCCESS) {
        printf("Error in evWrite(), err = %x\n", err);
        exit(0);
    }

    printf("\nsplitTest: close() ...\n");
    err = evClose(handle);
    if (err != S_SUCCESS) {
        printf("Error in evClose(), err = %x\n", err);
    }

  //  evPrintBuffer(buffer, 44, 0);

    
    printf("\nsplitTest: open for appending ...\n");
    /* open file for writing & create a buffer */
    /*
    err = evOpenBuffer(buffer, bufLen, "a", &handle);
    if (err != S_SUCCESS) {
        printf("Error in evOpen(), err = %x\n", err);
        exit(0);
    }
    */
    
    err = evOpen(filename2, "a", &handle);
    if (err != S_SUCCESS) {
        printf("Error in evOpen(), err = %x\n", err);
        exit(0);
    }

    /* events per block max */
    printf("\nsplitTest: set events/blk ...\n");
    arg = 5;
    evIoctl(handle, "N", &arg);

     /*
    int blockSizeMax  = 80/4 + 4*32/4;
    int bufferSize = 4*60;
     */

    /* target block size words */
    printf("\nsplitTest: set block size ...\n");
    arg = 80/4 + 5*32/4;
    err = evIoctl(handle, "B", &arg);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for target block size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting target block size\n");
        exit(0);
    }

    /* buffer size words */
    arg = 80/4 + 6*32/4;
    /*arg = 1000000;*/
    err = evIoctl(handle, "W", &arg);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for buffer size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting buffer size\n");
        exit(0);
    }

    printf("\nsplitTest: write little event ...\n");
    err = evWrite(handle, eventBuffer1);
    if (err != S_SUCCESS) {
        printf("Error in evWrite(), err = %x\n", err);
        exit(0);
    }

    printf("\nsplitTest: close() ...\n");
    err = evClose(handle);
    if (err != S_SUCCESS) {
        printf("Error in evClose(), err = %x\n", err);
    }

  //  evPrintBuffer(buffer, 60, 0);
    
    exit(0);
}


int mainBuffer (int argc, char **argv)
{
    int i, err, handle, arg;
    uint64_t split;
    uint32_t bufLen = 60, buffer[60];

    
    /* open file for writing */
    err = evOpenBuffer((char *)buffer, bufLen, "w", &handle);
    if (err != S_SUCCESS) {
        printf("Error in evOpen(), err = %x\n", err);
        exit(0);
    }


    /* events per block max */
    arg = 5;
    evIoctl(handle, "N", &arg);

     /*
    int blockSizeMax  = 80/4 + 4*32/4;
    int bufferSize = 4*60;
     */

    /* target block size words */
    arg = 80/4 + 4*32/4;
    err = evIoctl(handle, "B", &arg);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for target block size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting target block size\n");
        exit(0);
    }
        
      
    /* buffer size words */
    arg = 80/4 + 5*32/4;
    /*arg = 1000000;*/
    err = evIoctl(handle, "W", &arg);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for buffer size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting buffer size\n");
        exit(0);
    }
    
    
    /* split at x bytes */
    split = 240;
    /*split = 4000032L;*/
    err = evIoctl(handle, "S", &split);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for split size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting split size\n");
        exit(0);
    }

    printf("\nsplitTest: write dictionary ...\n");
    err = evWriteDictionary(handle, xmlDictionary2);
    printf ("splitTest: write dictionary, err = %x\n\n", err);
    

    printf("\nsplitTest: write 8-word events ...\n");
    for (i=0; i < 3; i++) {
        printf("\nsplitTest: write little event %d ...\n", (i+1));
        err = evWrite(handle, eventBuffer1);
        if (err != S_SUCCESS) {
            printf("Error in evWrite(), err = %x\n", err);
            exit(0);
        }
    }

    printf("\nsplitTest: close() ...\n");
    err = evClose(handle);
    if (err != S_SUCCESS) {
        printf("Error in evClose(), err = %x\n", err);
    }

    evPrintBuffer(buffer, 60, 0);
    
    exit(0);
}



int mainFile (int argc, char **argv)
{
    int i, err, handle, arg;
    // int evCount = 5;
    uint64_t split;
    uint32_t eventBuffer5[60];
    // char stuff[128];

    eventBuffer5[0] = 0x0000003b;
    eventBuffer5[1] = 0x00011001;
    eventBuffer5[2] = 0x00000039,
    eventBuffer5[3] = 0x00020b02;

    for (i=4; i < 61; i++) {
        eventBuffer5[i] = i-4;
    }

    
    /* open file for splitting */
    err = evOpen(filename, "s", &handle);
    if (err != S_SUCCESS) {
        printf("Error in evOpen(), err = %x\n", err);
        exit(0);
    }


    /* 1 event per block max */
    arg = 5;
    evIoctl(handle, "N", &arg);

     /*
    int split = 4*24;
    int blockSizeMax  = 16;
    int blockCountMax = 10;
    int bufferSize = 4*24; //4*48;


    int split = 240;
    int blockSizeMax  = 80/4 + 4*32/4; //= 16;
    int blockCountMax = 10;
    int bufferSize = 80 + 5*32; //= 4*24;

     */

    /* target block size words */
    arg = 80/4 + 4*32/4;
    err = evIoctl(handle, "B", &arg);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for target block size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting target block size\n");
        exit(0);
    }
    
    /* buffer size words */
    arg = 80/4 + 5*32/4;
    /*arg = 1000000;*/
    err = evIoctl(handle, "W", &arg);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for buffer size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting buffer size\n");
        exit(0);
    }
    
    
    /* split at x bytes */
    split = 240;
    /*split = 4000032L;*/
    err = evIoctl(handle, "S", &split);
    if (err == S_EVFILE_BADSIZEREQ) {
        printf("splitTest: bad value for split size given\n");
        exit(0);
    }
    else if (err != S_SUCCESS) {
        printf("splitTest: error setting split size\n");
        exit(0);
    }

    
    printf("\nsplitTest: write dictionary ...\n");
    err = evWriteDictionary(handle, xmlDictionary2);
    printf ("splitTest: write dictionary, err = %x\n\n", err);
    

    printf("\nsplitTest: write 8-word events ...\n");
    for (i=0; i < 3; i++) {
        printf("\nsplitTest: write little event %d ...\n", (i+1));
        err = evWrite(handle, eventBuffer1);
        if (err != S_SUCCESS) {
            printf("Error in evWrite(), err = %x\n", err);
            exit(0);
        }
        
//        printf("\nEnter to continue\n");
//        gets(stuff);
    }

    /*
    printf("\nsplitTest: write 1 REALLY big event ...\n");
    err = evWrite(handle, eventBuffer5);
    if (err != S_SUCCESS) {
    printf("Error in evWrite(), err = %x\n", err);
    printf("\nEnter to continue\n");
    exit(0);
}
    */

//    printf("\nEnter to continue\n");
//    gets(stuff);



   /*

    for (i=0; i < evCount; i++) {
    printf("\nsplitTest: write little event %d ...\n", (i+1));
    err = evWrite(handle, eventBuffer1);
    if (err != S_SUCCESS) {
    printf("Error in evWrite(), err = %x\n", err);
    exit(0);
}
}
   */
//     printf("\nsplitTest: write 1 big event ...\n");
//     err = evWrite(handle, eventBuffer2);
//     if (err != S_SUCCESS) {
//         printf("Error in evWrite(), err = %x\n", err);
//         exit(0);
//     }

    printf("\nsplitTest: close() ...\n");
    err = evClose(handle);
    if (err != S_SUCCESS) {
        printf("Error in evClose(), err = %x\n", err);
    }

    exit(0);
}

