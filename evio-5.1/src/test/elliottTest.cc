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
 

    
//  example program fills evio buffer with one event

//  ejw, 10-oct-2012



#include <stdio.h>
#include "evioUtil.hxx"
#include "evioBufferChannel.hxx"


using namespace evio;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  
    uint8_t ix[10] = {1,2,3,4,5,6,7,8,9,10};
    int streamBufLen=1000;


    // evio buffer channel
    evioBufferChannel *chan;


    try {

        // create stream buffer
        uint32_t streamBuf[streamBufLen];


        // create and open output buffer channel
        chan = new evioBufferChannel(streamBuf,streamBufLen,"w");
        chan->open();


        // create event tree and add banks
        evioDOMTree event1(1, 0); // tag = 1, num = 0
        event1.addBank(4, 11, ix, 9); // tag, num, array, len


        // write event to stream buffer
        chan->write(event1);


        // close buffer channel
        int streamBufSize = chan->getStreamBufSize();
        chan->close();
        delete(chan);
        printf("Stream buffer size = %d\n", streamBufSize);

    } catch (evioException e) {
        cerr << e.toString() << endl;

    } catch (...) {
        cerr << "?unknown exception" << endl;
    }
  
}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
