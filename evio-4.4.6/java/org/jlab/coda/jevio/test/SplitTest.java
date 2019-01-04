package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Test program for file splitting.
 * @author timmer
 * Date: Aug 29, 2013
 */
public class SplitTest {


    // xml dictionary
    static String xmlDictionary =
                    "<xmlDict>\n" +
                    "  <dictEntry name=\"TAG1_NUM1\" tag=\"1\"  num=\"1\"/>\n" +
                    "</xmlDict>";


    /** Creates events of size  2 + 2 + dataWordSize ints. */
    static EvioEvent createEvent(int dataWordSize) {
        // Top level event
        EvioEvent event = null;

        try {
            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(1, DataType.BANK, 1);
            event = builder.getEvent();

            // bank of int
            EvioBank bankInts = new EvioBank(2, DataType.INT32, 2);

            // create int data
            int[] data = new int[dataWordSize];
            for (int i=0; i < dataWordSize; i++) {
                data[i] = i;
            }
            bankInts.appendIntData(data);
            builder.addChild(event, bankInts);
        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        return event;
    }



    /** Look at split names. */
    public static void main(String args[]) {

        try {
            String baseName = "my_$(FILE_ENV)_%s_run_%d_.dat_stuff";
            String runType = "RunType";
            int runNumber = 2;
            long split = 0;

            // Split file number starts at 0
            int splitCount = 0;

            // The following may not be backwards compatible.
            // Make substitutions in the baseName to create the base file name.
            StringBuilder builder = new StringBuilder(100);
            int specifierCount = Utilities.generateBaseFileName(baseName, runType, builder);
            String baseFileName = builder.toString();
            // Also create the first file's name with more substitutions
            String fileName = Utilities.generateFileName(baseFileName, specifierCount,
                    runNumber, split, splitCount++);
            System.out.println("EventWriter const: filename = " + fileName);
            System.out.println("                   basename = " + baseName);
        }
        catch (EvioException e) {
            e.printStackTrace();
        }


    }


    /** For WRITING a local file. */
    public static void main2(String args[]) {

        String fileName = "/daqfs/home/timmer/coda/evio-4.1/my$(FILE_ENV)run_%d_.dat_%4d";
        String fileName2 = "/daqfs/home/timmer/coda/evio-4.1/myFILE";
        //String xmlDictionary = null;

        EvioEvent littleEvent  = createEvent(4);
        EvioEvent biggestEvent = createEvent(16);

        boolean append = false;

        int runNumber = 72;


        int split = 2*240;
        int blockSizeMax  = 80/4 + 2*32/4; //= 16;
        int blockCountMax = 2;
        int bufferSize = 80 + 5*32; //= 4*24;
//        int bufferSize = 80 + 32 + 32 + 96 + 32 + 32 + 32; //4*48;

        try {
            EventWriter writer = new EventWriter(fileName, null, "runType", runNumber, split,
                    blockSizeMax, blockCountMax, bufferSize,
                    ByteOrder.BIG_ENDIAN, xmlDictionary, null, true, append);


//            ByteBuffer buf = ByteBuffer.allocateDirect(bufferSize);
//
//            EventWriter writer = new EventWriter(buf, blockSizeMax, blockCountMax,
//                                                 xmlDictionary, null, 0, append);

            for (int i=0; i < 20; i++) {
                // Write event to file
                writer.writeEvent(littleEvent);
            }

            // All done writing
            writer.close();

//            buf.flip();
//            Utilities.bufferToFile(fileName2, buf, true);

        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }




}
