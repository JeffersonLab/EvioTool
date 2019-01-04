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
#include <unistd.h>
#include "evio.h"

#define MIN(a,b) (a<b)? a : b


uint32_t *makeEvent() {

    uint32_t *bank;

    bank = (uint32_t *) calloc(5, sizeof(uint32_t));
    bank[0] = 4;                      /* event length = 4 */
    bank[1] = 1 << 16 | 1 << 8 | 1;   /* tag = 1, bank 1 contains ints, num = 1 */
    bank[2] = 1;
    bank[3] = 2;
    bank[4] = 3;
   
    return(bank);
}

static void printFile(char *fileName) {

    int nBytes, i=0, intdata;
    FILE *file = fopen(fileName,"r");
    if (file == NULL) {
        printf("Cannot open file %s\n", fileName);
        return;
    }
    
    printf("  i      file\n");
    printf("-------------------\n");

    while (1) {
        nBytes = sizeof(intdata)*fread(&intdata, sizeof(intdata), 1, file);
        if (nBytes != sizeof(intdata)) {
            break;
        }
        printf("  %d    %#10.8x\n", i++, intdata);
    }
}


static void createFile(char *fileName) {

    int nBytes, len1=54 /*, len2=13, len3=9, len4=18 */;
    
    /*
       3 block headers (first 2 have 2 extra words each, last has 1 extra word).
       First block has 2 events. Second has 3 events.
       Last is empty final block.
    */
    uint32_t data1[] = {
        0x00000014,
        0x00000001,
        0x0000000A,
        0x00000002,
        0x00000000,
        0x00000004,
        0x00000000,
        0xc0da0100,
        0x00000003,
        0x00000002,

        0x00000004,
        0x00010101,
        0x00000001,
        0x00000001,
        0x00000001,
        
        0x00000004,
        0x00010101,
        0x00000002,
        0x00000002,
        0x00000002,
        
        0x00000019,
        0x00000002,
        0x0000000A,
        0x00000003,
        0x00000000,
        0x00000004,
        0x00000000,
        0xc0da0100,
        0x00000001,
        0x00000002,

        0x00000004,
        0x00010101,
        0x00000003,
        0x00000003,
        0x00000003,
        
        0x00000004,
        0x00010101,
        0x00000004,
        0x00000004,
        0x00000004,
        
        0x00000004,
        0x00010101,
        0x00000005,
        0x00000005,
        0x00000005,
        
        0x00000009,
        0x00000003,
        0x00000009,
        0x00000000,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,
        0x00000003,
    };
    
    /* one block header with 1 event */
    /*
    uint32_t data2[] = {
        0x0000000d,
        0x00000001,
        0x00000008,
        0x00000001,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,
        
        0x00000004,
        0x00010101,
        0x00000001,
        0x00000002,
        0x00000003
    };
    */

    /* one block header with extra int, no events */
    /*
    uint32_t data3[] = {
        0x00000009,
        0x00000001,
        0x00000009,
        0x00000000,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,
        0x00000004,
    };
    */

    /* 2 block headers, both empty & each with extra int */
    /*
    uint32_t data4[] = {
        0x00000009,
        0x00000001,
        0x00000009,
        0x00000000,
        0x00000000,
        0x00000004,
        0x00000000,
        0xc0da0100,
        0x00000004,
        
        0x00000009,
        0x00000002,
        0x00000009,
        0x00000000,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,
        0x00000004,
    };
    */
    
    FILE *file = fopen(fileName,"w");
    if (file == NULL) {
        printf("Cannot open file %s\n", fileName);
        return;
    }

    /* Write data to file */
    nBytes = fwrite((const void *)data1, 1, len1*sizeof(uint32_t), file);
    //nBytes = fwrite((const void *)data2, 1, len2*sizeof(uint32_t), file);
    //nBytes = fwrite((const void *)data3, 1, len3*sizeof(uint32_t), file);
    //nBytes = fwrite((const void *)data4, 1, len4*sizeof(uint32_t), file);
    fclose(file);
}


int main() {
    int handle, status;
    uint32_t *bank, *bank2, bufLen, blkSize=1024+8, /* blkNum=2, */ buffie[15000];
    
    bank2 = (uint32_t *) calloc(3, sizeof(uint32_t));
    bank2[0] = 2;                      /* event length = 2 */
    bank2[1] = 1 << 16 | 1 << 1 | 1;   /* tag = 1, bank 1 contains unsigned 32-bit ints, num = 1 */

    bank = (uint32_t *) calloc(692, sizeof(uint32_t));
    bank[0] = 691;
    bank[1] = 1 << 16 | 1 << 1 | 1;

    status = evOpenBuffer((char *) buffie, 15000, "w", &handle);
    printf ("    Opened buffer for writing, status = %d\n", status);

    // Set max blk size to its maximum
//    evIoctl(handle, "N", (void *)&blkNum);
    evIoctl(handle, "B", (void *)&blkSize);

    status = evGetBufferLength(handle, &bufLen);
    printf ("    %u bytes to buf before any writes\n", bufLen);

    status = evWrite(handle, bank);

    status = evGetBufferLength(handle, &bufLen);
    printf ("    1:  %u bytes, %u words to buf after write\n", bufLen, bufLen/4);

    status = evWrite(handle, bank);

    status = evGetBufferLength(handle, &bufLen);
    printf ("    2:  %u bytes, %u words to buf after write\n", bufLen, bufLen/4);
    
    status = evWrite(handle, bank);

    status = evGetBufferLength(handle, &bufLen);
    printf ("    3:  %u bytes, %u words to buf after write\n", bufLen, bufLen/4);

    status = evWrite(handle, bank);

    status = evGetBufferLength(handle, &bufLen);
    printf ("    4:  %u bytes, %u words to buf after write\n", bufLen, bufLen/4);

    status = evWrite(handle, bank);

    status = evGetBufferLength(handle, &bufLen);
    printf ("    5:  %u bytes, %u words to buf after write\n", bufLen, bufLen/4);

    status = evWrite(handle, bank);

    status = evGetBufferLength(handle, &bufLen);
    printf ("    6:  %u bytes, %u words to buf after write\n", bufLen, bufLen/4);

    status = evClose(handle);
    printf ("    Closed buffer, status = 0x%x\n\n", status);

    free(bank);
    free(bank2);
    
    return 0;
}

int main11() {
    printFile("./mydata");
    return 0;
}

int main12() {
    int i, handle, status, nevents, nwords;
    const uint32_t *ip;
    uint32_t evCount, *buffer, bufLen;

    createFile("./mydata");
    printFile("./mydata");
    
    /* random access file */
    status = evOpen("./mydata", "r", &handle);
    printf ("\nOpened file for READING\n");
    
    evIoctl(handle, "e", (void *)&evCount);
    
    printf ("    file has %u events\n\n", evCount);

    nevents = 0;
    while ((status = evReadAlloc(handle, &buffer, &bufLen)) == S_SUCCESS) {
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

        free(buffer);
    }
    
    status = evClose(handle);
    printf ("    Closed file for reading, status = %d\n\n", status);
    return 0;
}


int main111() {
    int i, handle, status, nevents, nwords;
    const uint32_t *ip;
    uint32_t evCount, buffer[2048];

    memset(buffer, 0, 2048);

    createFile("./mydata");
    printFile("./mydata");
    
    /* random access file */
    status = evOpen("./mydata", "r", &handle);
    printf ("\nOpened file for READING\n");
    
    evIoctl(handle, "e", (void *)&evCount);
    
    printf ("    file has %u events\n\n", evCount);

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
    printf ("    Closed file for reading, status = %d\n\n", status);
    return 0;
}

int main22() {
    int i, handle, status, nevents, nwords;
    const uint32_t *ip;
    uint32_t evCount, buffer[2048];

    uint32_t data[] = {
        0x0000000f,
        0x00000001,
        0x0000000A,
        0x00000001,
        0x00000000,
        0x00000004,
        0x00000000,
        0xc0da0100,
        0x00000001,
        0x00000002,

        0x00000004,
        0x00010101,
        0x00000001,
        0x00000002,
        0x00000003,
        
        0x0000000f,
        0x00000001,
        0x0000000A,
        0x00000001,
        0x00000000,
        0x00000004,
        0x00000000,
        0xc0da0100,
        0x00000001,
        0x00000002,

        0x00000004,
        0x00010101,
        0x00000001,
        0x00000002,
        0x00000003,
        
        0x00000009,
        0x00000002,
        0x00000009,
        0x00000000,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,
        0x00000003,
    };

    
    status = evOpenBuffer((char *) data, 156, "r", &handle);
    printf ("    Opened buffer for READING, status = %d\n", status);
    
    evIoctl(handle, "e", (void *)&evCount);
    printf ("    file has %u events\n\n", evCount);

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
    printf ("    Closed file for reading, status = %d\n\n", status);
    return 0;
}



int main4() {
    int i, handle, status, nevents, nwords;
    const uint32_t *ip;
    uint32_t evCount, len = 0L, bufLen;
    uint32_t **pTable;

    createFile("./mydata");
    printFile("./mydata");
    
    /* random access file */
    status = evOpen("./mydata", "ra", &handle);
    printf ("\nOpened file for RANDOM ACCESS\n");
    evIoctl(handle, "e", (void *)&evCount);
    
    printf ("    file has %u events\n\n", evCount);

    status = evGetRandomAccessTable(handle, &pTable, &len);
    printf("  i      pointers\n");
    printf("-------------------\n");
    for (i=0; i < len; i++) {
        printf("  %d    %p\n", i, pTable[i]);
    }
    printf("-------------------\n\n");

    nevents = 1;
    while ((status = evReadRandom(handle, &ip, &bufLen, nevents++)) == S_SUCCESS) {
        nwords = ip[0] + 1;
        printf("    Event #%d, len = %d words\n", (nevents - 1), nwords);


        printf("      Header words\n");
        printf("        %#10.8x\n",   *ip++);
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

    free(pTable);
    evClose(handle);

    exit(1);

    ip = makeEvent();

    /* Append an identical 7 events */
    status = evOpen("./mydata", "a", &handle);
    printf ("    Reopened file for APPENDING, status = 0x%x\n", status);
//    evIoctl(handle, "e", (void *)&evCount);
//    printf ("    file BEFORE has %u events\n\n", evCount);
    status = evWrite(handle, ip);
    evIoctl(handle, "e", (void *)&evCount);
    printf ("    file AFTER append has %u events\n\n", evCount);
    printf ("    appended another event, status = 0x%x\n", status);
    status = evClose(handle);
    printf ("    Closed file for appending, status = 0x%x\n\n", status);

    printFile("./mydata");
    
    /* Count events in file when opening in regular read mode */
    status = evOpen("./mydata", "r", &handle);
    evIoctl(handle, "e", (void *)&evCount);
    printf ("    file has %u events\n\n", evCount);
    status = evClose(handle);
}



int main1() {
    int i, handle, status, nevents, nwords;
    const uint32_t *ip = NULL;
    uint32_t **pTable;
    uint32_t evCount, len = 0L, bufLen;

    char *dictionary =
            "<xmlDict>\n"
            "  <xmldumpDictEntry name=\"Tag1-Num1\"   tag=\"1\"   num=\"1\"/>\n"
            "  <xmldumpDictEntry name=\"Tag2-Num2\"   tag=\"2\"   num=\"2\"/>\n"
            "  <xmldumpDictEntry name=\"Tag3-Num3\"   tag=\"3\"   num=\"3\"/>\n"
            "</xmlDict>\n";

    printf("Open file for writing of dictionary & event\n");
    status = evOpen("./mydata", "w", &handle);
    printf ("    Opened file, status = %#x\n", status);

    evIoctl(handle, "e", (void *)&evCount);
    printf ("    file AFTER open has %u events\n\n", evCount);


    status = evWriteDictionary(handle, dictionary);
    printf ("    Wrote dictionary to file, status = %s\n", evPerror(status));

    status = evWriteDictionary(handle, dictionary);
    printf ("    Wrote dictionaryII to file, status = %s\n", evPerror(status));

    status = evIoctl(handle, "e", (void *)&evCount);
    printf ("    file AFTER writng dictionary has %u events, status = %s\n\n", evCount, evPerror(status));
//     ip = makeEvent();
//     status = evWrite(handle, ip);
//     printf ("    Wrote event to file, status = %#x\n\n", status);
//     status = evWrite(handle, ip);
//     printf ("    Wrote event to file, status = %#x\n\n", status);

    status = evClose(handle);
    printf ("    Closed file, status = %#x\n\n", status);


    /* read file */
    status = evOpen("./mydata", "r", &handle);
    printf ("\nOpened file for READING\n");
    evIoctl(handle, "e", (void *)&evCount);
    printf ("    file AFTER open has %u events\n\n", evCount);
    status = evClose(handle);
    printf ("    Closed file, status = %#x\n\n", status);

    printFile("./mydata");
    
    /* random access file */
    status = evOpen("./mydata", "ra", &handle);
    printf ("\nOpened file for RANDOM ACCESS\n");
    evIoctl(handle, "e", (void *)&evCount);
    
    printf ("    file has %u events\n\n", evCount);

    status = evGetRandomAccessTable(handle, &pTable, &len) ;
    printf("  i      pointers\n");
    printf("-------------------\n");
    for (i=0; i < len; i++) {
        printf("  %d    %p\n", i, pTable[i]);
    }
    printf("-------------------\n\n");


    nevents = 1;
    while ((status = evReadRandom(handle, &ip, &bufLen, nevents++)) == S_SUCCESS) {
        nwords = ip[0] + 1;
        printf("    Event #%d, len = %d words\n", (nevents - 1), nwords);


        printf("      Header words\n");
        printf("        %#10.8x\n",   *ip++);
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

    free(pTable);
    evClose(handle);
    return 0;
}

int main2() {
    
    int handle, status, nevents, nwords;
    uint32_t *ip, *pBuf, buffer[2048];
    int i;

    memset(buffer, 0, 2048);

    printf("\nEvent I/O tests...\nTest buffer first:\n");
    
    status = evOpenBuffer((char *) buffer, 2048, "w", &handle);
    printf ("    Opened buffer for writing, status = %d\n", status);

    i = 1;
    evIoctl(handle, "n", (void *)&i);

    pBuf = ip = makeEvent();

    status = evWrite(handle, ip);
    /*
    printf("  i      buffer\n");
    printf("-------------------\n");
    for (i=0; i < 25; i++) {
    printf("  %d    %#10.8x\n", i, buffer[i]);
}
    */

    status = evWrite(handle, ip);

    printf("  i      buffer\n");
    printf("-------------------\n");
    for (i=0; i < 42; i++) {
        printf("  %d    %#10.8x\n", i, buffer[i]);
    }

    status = evClose(handle);
    printf ("    Closed buffer, status = %d\n\n", status);


    status = evOpenBuffer((char *) buffer, 2048, "a", &handle);
    printf ("    Opened buffer for APPENDING, status = %d\n", status);

    i = 1;
    evIoctl(handle, "n", (void *)&i);
    status = evWrite(handle, ip);

    printf("  i      buffer\n");
    printf("-------------------\n");
    for (i=0; i < 50; i++) {
        printf("  %d    %#10.8x\n", i, buffer[i]);
    }

    status = evClose(handle);
    printf ("    Closed buffer, status = %d\n\n", status);
    printf ("    Call printFile()\n");

    printFile("./mydata");

    
    if (0) {
        printf("Test file next:\n");
        status = evOpen("./mydata", "w", &handle);
        printf ("    Opened file for writing, status = %d\n", status);

        i = 1;
        evIoctl(handle, "n", (void *)&i);

        status = evWrite(handle, ip);
//    status = evWrite(handle, ip);
        status = evClose(handle);
        printf ("    Closed file for writing, status = %d\n\n", status);


        status = evOpen("./mydata", "r", &handle);
        printf ("    Opened file for reading, status = %d\n", status);

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
        printf ("    Closed file for reading, status = %d\n\n", status);


        /* Append an identical event */
        status = evOpen("./mydata", "a", &handle);
        printf ("    Reopened file for APPENDING, status = 0x%x\n", status);
        status = evWrite(handle, ip);
        printf ("    appended another event, status = 0x%x\n", status);
        status = evClose(handle);
        printf ("    Closed file for appending, status = 0x%x\n\n", status);


        /* Append another identical event */
        status = evOpen("./mydata", "a", &handle);
        printf ("    Reopened file for APPENDING, status = 0x%x\n", status);
        status = evWrite(handle, ip);
        printf ("    appended another event, status = 0x%x\n", status);
        status = evClose(handle);
        printf ("    Closed file for appending, status = 0x%x\n\n", status);

        /* Append yet another identical event */
        status = evOpen("./mydata", "a", &handle);
        printf ("    Reopened file for APPENDING, status = 0x%x\n", status);
        status = evWrite(handle, ip);
        printf ("    appended another event, status = 0x%x\n", status);
        status = evClose(handle);
        printf ("    Closed file for appending, status = 0x%x\n\n", status);

    

        status = evOpen("./mydata", "r", &handle);
        printf ("    Opened file for reading AGAIN, status = 0x%x\n", status);
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
        printf ("    Closed file for reading, status = %d\n\n", status);

    }

    free(pBuf);
    return 0;
}
