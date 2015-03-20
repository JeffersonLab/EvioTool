/*-----------------------------------------------------------------------------
 * Copyright (c) 2013 Jefferson Science Associates,
 *                    Thomas Jefferson National Accelerator Faciltiy,
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * JLAB Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: timmmer@jlab.org  Tel: (757) 269-5130
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *	Event I/O test program
 *	
 * Author:  Carl Timmer, JLAB Data Acquisition Group
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
"</xmlDict>\n";

static uint32_t *makeEvent();


/**
  This program writes to stdout using pipes and can therefore be
  read by evReadPipe.c. It's purpose is to test using pipes with evio.

  The function popen does a fork and exec to execute the cmdstring,
  and returns a standard I/O file pointer. If type is "r", the file
  pointer is connected to the standard output of cmdstring
  (result of fp = popen(cmdstring, "r")):

      parent                  cmdstring (child)
 _______________              _________________
 |             |              |               |
 |          fp | <----------- | stdout        |
 |_____________|              |_______________|

  If type is "w", the file pointer is connected to the standard input
  of cmdstring (result of fp = popen(cmdstring, "w")):

      parent                  cmdstring (child)
 _______________              _________________
 |             |              |               |
 |          fp | -----------> | stdin         |
 |_____________|              |_______________|
 
 This program can be used in 2 ways:
    1) at command line, run: evWritePipe "|evReadPipe"
        This program needs to set filename = "\evReadPipe" internally.
        And evReadPipe needs to set filename = "-" internally (read from stdin).
 
   2) at command line, run: evReadPipe "|evWritePipe"
      This program will need to set filename = "-"  internally (write to stdout).
      And evReadPipe needs to set filename = "|evWritePipe" internally.
      Be sure that this program is NOT printing out anything in this case.
*/


static void printEvent(uint32_t eventNum, uint32_t *buffer) {
    int i, nwords;

    if (buffer == NULL) return;
    nwords = buffer[0] + 1;

    printf("   Event #%u,  len = %u data words\n", eventNum, nwords - 2);
    printf("      Header words\n");
    printf("         %#10.8x\n", *buffer++);
    printf("         %#10.8x\n\n", *buffer++);
    printf("      Data words\n");
   
    nwords -= 2;
        
    for (; nwords > 0; nwords-=4) {
        for (i = MIN(nwords,4); i > 0; i--) {
            printf("         %#10.8x", *buffer++);
        }
        printf("\n");
    }
    printf("\n");
}



int main(int argc, char **argv)
{
    int i, handle, status, debug=0;
    uint32_t *buffer;
    char *filename;


    /* The beginning "|" tells evio to open a pipe and write to that
     * so it goes to the stdin of evReadPipe. */
    filename = "|evReadPipe";
    filename = "-";
    

    if (debug) printf("Write to program = %s\n", filename+1);

    status = evOpen(filename, "w", &handle);
    if (debug) printf ("    Opened file, status = %d\n", status);

    status = evWriteDictionary(handle, dictionary);

    buffer = makeEvent();

    if (debug) printEvent(1, buffer);


    for (i=0; i < 3; i++) {
        status = evWrite(handle, buffer);
        if (debug) printf ("    Wrote event to file status = %d\n",status);
    }

    status = evClose(handle);
    if (debug) printf ("    evWritePipe: closed file, status = %d\n\n", status);
 
    free(buffer);
}


static uint32_t *makeEvent()
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
