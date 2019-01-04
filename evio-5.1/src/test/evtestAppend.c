/*-----------------------------------------------------------------------------
 * Copyright (c) 2015  Jefferson Science Associates
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: timmer@jlab.org  Tel: (757) 249-7100  Fax: (757) 269-6248
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *  Event I/O test program
 *  
 * Author:  Carl Timmer, Data Acquisition Group
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include "evio.h"


int main()
{
    int handle, status, nevents, nwords, i;
    uint32_t  evBuf[16], *ip;

    status = evOpen("/tmp/fileTestSmallBigEndian.ev", "a", &handle);
    nevents = 0;

    /* bank  header */
    evBuf[0]  = 0x0000000f;        evBuf[1]  = 0x00020b02;

    /* data */
    evBuf[2]  = 0x0000000c;        evBuf[3]  = 0x0000000d;
    evBuf[4]  = 0x0000000e;        evBuf[5]  = 0x0000000f;
    evBuf[6]  = 0x00000010;        evBuf[7]  = 0x00000011;
    evBuf[8]  = 0x00000012;        evBuf[9]  = 0x00000013;
    evBuf[10] = 0x00000014;        evBuf[11] = 0x00000015;
    evBuf[12] = 0x00000016;        evBuf[13] = 0x00000017;
    evBuf[14] = 0x00000018;        evBuf[15] = 0x00000019;


    status = evWrite(handle, evBuf);
    printf ("    Write file, status = %d\n", status);
    status = evClose(handle);
    printf ("    Closed file, status = %d\n\n", status);
}


