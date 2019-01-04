/*-----------------------------------------------------------------------------
 * Copyright (c) 1991,1992 Southeastern Universities Research Association,
 *                         Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: coda@cebaf.gov  Tel: (804) 249-7101  Fax: (804) 249-7363
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *	Event I/O test program
 *	
 * Author:  Chip Watson, CEBAF Data Acquisition Group
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evio.h"

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

uint32_t *makeEvent();
uint32_t *makeEvent2();

int main()
{
    int handle, status, nevents, nwords;
    uint32_t *ip, *pBuf, buffer[2048], swappedBuffer[2048];
    int i, maxEvBlk = 2;


    printf("\nEvent I/O tests...\n");
    status = evOpen("single.dat", "w", &handle);
    printf ("    Opened single.dat, status = %d\n", status);

    status = evWriteDictionary(handle, dictionary);

    pBuf = ip = makeEvent2();

    memset(buffer, 0, 2048);
    memset(swappedBuffer, 0, 2048);
    
    evioswap(pBuf, 0, swappedBuffer);
    evioswap(swappedBuffer, 1, buffer);

    printf("  i      buffer      swapped 1x   swapped 2x\n");
    printf("------------------------------------\n");
    for (i=0; i < 39; i++) {
        printf("  %d    %#10.8x      %#10.8x   %#10.8x\n", i, ip[i], swappedBuffer[i], buffer[i]);
    }

    
    printf ("    Will write single.dat, status = %d\n",status);
    status = evWrite(handle, ip);

    status = evClose(handle);
    printf ("    Closed single.dat, status = %d\n\n", status);

    status = evOpen("single.dat", "r", &handle);
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

    printf ("    Last evRead status = %d\n", status);
   
    
    status = evClose(handle);
    printf ("    Closed single.dat again, status = %d\n\n", status);

    ip = pBuf;
    
    printf ("    Try opening multiple.dat\n");
    status = evOpen("multiple.dat", "w", &handle);
    printf ("    Opened multiple.dat, status = %d\n", status);

    status = evIoctl(handle, "N", (void *) (&maxEvBlk));
    printf ("    Changed max events/block to %d, status = %#x\n", maxEvBlk, status);
    
    printf ("    Will write multiple.dat\n");
    evWrite(handle,ip);
    evWrite(handle,ip);
    evWrite(handle,ip);

    status = evClose(handle);
    printf ("    Closed multiple.dat status %d\n\n", status);

    status = evOpen("multiple.dat", "r", &handle);
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

uint32_t *makeEvent2()
{
    short *shortWord;
    float  data;
    double doubl;
    uint64_t bigInt, *bigWord;
    uint32_t *bank, *segment, *tagseg, *intWord;


    bank = (uint32_t *) calloc(1, 39*sizeof(int32_t));

    /* bank of banks */
    bank[0] = 38;                        /* bank length (not including this int) */
    bank[1] = 1 << 16 | 0x10 << 8 | 1;   /* tag = 1, bank contains banks, num = 1 */

    
    /********************/
    /* bank of segments */
    /********************/
    bank[2] = 10;                        /* bank length (not including this int) */
    bank[3] = 1 << 16 | 0x20 << 8 | 2;   /* tag = 1, bank contains segments, num = 2 */

    /* segment of 32 bit ints */
    segment = &(bank[4]);
    segment[0] = 2 << 24 | 0xb << 16 | 2; /* tag = 2, seg 1 has 2 - 32 bit ints, len = 2 */
    segment[1] = 0x1;
    segment[2] = 0x2;
        
    segment += 3; /* bank[7] */
        
    /* segments of shorts */
    segment[0] = 3 << 24 | 2 << 22 | 4 << 16 | 2; /* tag = 3, 2 bytes padding, seg 2 has 3 shorts, len = 2 */
    shortWord = (short *) &(segment[1]);
    shortWord[0] = 0x3;
    shortWord[1] = 0x4;
    shortWord[2] = 0x5;

    segment += 3; /* bank[10] */

    /* segment of string (HI HO - 2 strings - little endian) */
    segment[0] =    4 << 24 | 0x3 << 16 | 2; /* tag = 4, seg 3 has 2 strings, len = 2 */
    segment[1] = 0x48 << 24 | 0 << 16   | 0x49 << 8 | 0x48 ;   /* H \0 I H */
    segment[2] =   4  << 24 | 4 << 16   | 0 << 8    | 0x4F ;   /* \4 \4 \0 O */

    segment += 3; /* bank[13] */
    

    /***********************/
    /* bank of tagsegments */
    /***********************/
    bank[13] = 25;                       /* bank length */
    bank[14] = 6 << 16 | 0xC << 8 | 3;   /* tag = 6, bank contains tagsegments, num = 3 */

    /* tagsegment of composite type */
    tagseg = &(bank[15]);
    tagseg[0] = 5 << 20 | 0xF << 16 | 17; /* tag = 5, seg has composite data, len = 17 */

    /* first part of composite type (for format) = tagseg (tag & type ignored, len used) */
    //    tagseg[1] = 5 << 20 | 0x3 << 16 | 3; /* tag = 5, seg has char data, len = 3 */
    /* ASCII chars values in latest evio string (array) format, 2(2I,F) */
    //    tagseg[2] = 0x49 << 24 | 0x32 << 16 | 0x28 << 8 | 0x32 ;   /* I 2 ( 2 */
    //    tagseg[3] =   0  << 24 | 0x29 << 16 | 0x46 << 8 | 0x2C ;   /* \0 ) F , */
    //    tagseg[4] =   4  << 24 |    4 << 16 |    4 << 8 |    4 ;   /* \4 \4 \4 \4 */

    /* second part of composite type (for data) = bank (tag, num, type ignored, len used) */
    /*
    intWord = &(bank[20]);
    intWord[0] = 7;
    intWord[1] = 6 << 16 | 0xF << 8 | 1;
    data = 123.456;
    intWord[2] = 0x1111;
    intWord[3] = 0x2222;
    intWord[4] = *(int *)&data;
    intWord[5] = 0x11111111;
    intWord[6] = 0x22222222;
    intWord[7] = *(int *)&data;
    */

    /* first part of composite type (for format) = tagseg (tag & type ignored, len used) */
    //    tagseg[1] = 5 << 20 | 0x3 << 16 | 3; /* tag = 5, seg has char data, len = 3 */
    /* ASCII chars values in latest evio string (array) format, I,N(I,F) */
    //    tagseg[2] = 0x28 << 24 | 0x4E << 16 | 0x2C << 8 | 0x49;    /* ( N , I */
    //    tagseg[3] = 0x29 << 24 | 0x46 << 16 | 0x2C << 8 | 0x49;    /* ) F , I */
    //    tagseg[4] =   4  << 24 |    4 << 16 |    4 << 8 |    0 ;   /* \4 \4 \4 \0 */
    
    /* second part of composite type (for data) = bank (tag, num, type ignored, len used) */
    /*
    intWord = &(bank[20]);
    intWord[0] = 7;
    intWord[1] = 6 << 16 | 0xF << 8 | 1;
    data = 123.456;
    intWord[2] = 0x3333;
    intWord[3] = 0x2;
    intWord[4] = 0x1111;
    intWord[5] = *(int *)&data;
    intWord[6] = 0x2222;
    intWord[7] = *(int *)&data;
    */
    
    /* first part of composite type (for format) = tagseg (tag & type ignored, len used) */
    tagseg[1] = 5 << 20 | 0x3 << 16 | 3; /* tag = 5, seg has char data, len = 3 */
    /* ASCII chars values in latest evio string (array) format, N(N(I,2S)) with N=2 */
    tagseg[2] = 0x28 << 24 | 0x4E << 16 | 0x28 << 8 | 0x4E;    /* ( N ( N */
    tagseg[3] = 0x53 << 24 | 0x32 << 16 | 0x2C << 8 | 0x49;    /* S 2 , I */
    tagseg[4] =   4  << 24 |    0 << 16 | 0x29 << 8 | 0x29 ;   /* \4 \0 ) ) */
    
    /* second part of composite type (for data) = bank (tag, num, type ignored, len used) */
    
    intWord = &(bank[20]);
    intWord[0] = 12;
    intWord[1] = 6 << 16 | 0xF << 8 | 1;
    intWord[2] = 0x2; /* N */
    intWord[3] = 0x2; /* N */
    intWord[4] = 0x00001111;
    intWord[5] = 0x11223344;
    intWord[6] = 0x00002222;
    intWord[7] = 0x55667788;
    
    intWord[8]  = 0x2; /* N */
    intWord[9]  = 0x00003333;
    intWord[10] = 0x00991188;
    intWord[11] = 0x00004444;
    intWord[12] = 0x22773366;
    
    
    /* tagsegment of 64 bit ints, little endian */
    bigInt = 0x0102030405060708L;
    tagseg = &(bank[33]);
    tagseg[0] = 7 << 20 | 0xa << 16 | 2; /* tag = 7, tagseg has 1 - 64 bit uint, len = 2 */
    bigWord = (uint64_t *) &(tagseg[1]);
    bigWord[0] = bigInt;
      
    /* tagsegment of double, little endian */
    doubl = 123.;
    tagseg = &(bank[36]);
    tagseg[0] = 8 << 20 | 0x8 << 16 | 2; /* tag = 8, tagseg has 1 double, len = 2 */
    bigWord = (uint64_t *) &(tagseg[1]);
    bigWord[0] = *(uint64_t *)&doubl;

    return(bank);
}
