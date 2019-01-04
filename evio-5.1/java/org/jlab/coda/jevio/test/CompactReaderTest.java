package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.*;
import java.nio.channels.FileChannel;
import java.util.Arrays;
import java.util.BitSet;
import java.util.List;
import java.util.Scanner;

/**
 * Take test programs out of EvioCompactReader.
 * @author timmer
 * Date: Nov 15, 2012
 */
public class CompactReaderTest {

    /**
     * This method prints out a portion of a given ByteBuffer object
     * in hex representation of ints.
     *
     * @param buf buffer to be printed out
     * @param lenInInts length of data in ints to be printed
     */
    private static void printBuffer(ByteBuffer buf, int lenInInts) {
        IntBuffer ibuf = buf.asIntBuffer();
        lenInInts = lenInInts > ibuf.capacity() ? ibuf.capacity() : lenInInts;
        for (int i=0; i < lenInInts; i++) {
            System.out.println("  Buf(" + i + ") = 0x" + Integer.toHexString(ibuf.get(i)));
        }
    }



    static EvioEvent createSingleEvent(int tag) {

        // Top level event
        EvioEvent event = null;

        // data  (1500 bytes)
        int[] intData = new int[2];
        Arrays.fill(intData, 1);

        byte[] byteData = new byte[9];
        Arrays.fill(byteData, (byte)2);

        short[] shortData = new short[3];
        Arrays.fill(shortData, (short)3);

        double[] doubleData = new double[1];
        Arrays.fill(doubleData, 4.);

        try {

            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(tag, DataType.BANK, 1);
            event = builder.getEvent();

                // bank of banks
                EvioBank bankBanks = new EvioBank(tag+1, DataType.BANK, 2);
                builder.addChild(event, bankBanks);

                    // bank of ints
                    EvioBank bankInts = new EvioBank(tag+2, DataType.INT32, 3);
                    bankInts.appendIntData(intData);
                    builder.addChild(bankBanks, bankInts);

                // Bank of segs
                EvioBank bankBanks2 = new EvioBank(tag+3, DataType.BANK, 4);
                builder.addChild(event, bankBanks2);

                    // segment of shorts
                    EvioBank bankShorts = new EvioBank(tag+4, DataType.SHORT16, 5);
                    bankShorts.appendShortData(shortData);
                    builder.addChild(bankBanks2, bankShorts);

                    // segment of segments
                    EvioBank bankBanks3 = new EvioBank(tag+5, DataType.BANK, 6);
                    builder.addChild(bankBanks2, bankBanks3);

                        // segment of doubles
                        EvioBank bankDoubles = new EvioBank(tag+6, DataType.DOUBLE64, 7);
                        bankDoubles.appendDoubleData(doubleData);
                        builder.addChild(bankBanks3, bankDoubles);

                // Bank of tag segs
                EvioBank bankBanks4 = new EvioBank(tag+7, DataType.BANK, 8);
                builder.addChild(event, bankBanks4);

                    // tag segment of bytes
                    EvioBank bankBytes = new EvioBank(tag+8, DataType.CHAR8, 9);
                    bankBytes.appendByteData(byteData);
                    builder.addChild(bankBanks4, bankBytes);


        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        return event;

    }


    static EvioEvent createSimpleSingleEvent(int tag) {

        // Top level event
        EvioEvent event = null;

        // data
        int[] intData = new int[21];
        Arrays.fill(intData, 1);

        try {
            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(tag, DataType.BANK, 1);
            event = builder.getEvent();

            // bank of ints
            EvioBank bankInts = new EvioBank(tag+1, DataType.INT32, 2);
            bankInts.appendIntData(intData);
            builder.addChild(event, bankInts);

        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        return event;

    }




    static EvioEvent createSingleEvent1(int tag) {

        // Top level event
        EvioEvent event = null;

        // data  (1500 bytes)
        int[] intData = new int[4];
        Arrays.fill(intData, 1);

        byte[] byteData = new byte[4];
        Arrays.fill(byteData, (byte)2);

        short[] shortData = new short[4];
        Arrays.fill(shortData, (short)3);

        double[] doubleData = new double[5];
        Arrays.fill(doubleData, 4.);

        try {

            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(tag, DataType.BANK, 1);
            event = builder.getEvent();

                // bank of banks
                EvioBank bankBanks = new EvioBank(tag+1, DataType.BANK, 2);
                builder.addChild(event, bankBanks);

                    // bank of ints
                    EvioBank bankInts = new EvioBank(tag+2, DataType.INT32, 3);
                    bankInts.appendIntData(intData);
                    builder.addChild(bankBanks, bankInts);

                // Bank of segs
                EvioBank bankSegs = new EvioBank(tag+3, DataType.SEGMENT, 1);
                builder.addChild(event, bankSegs);

                    // segment of shorts
                    EvioSegment segShorts = new EvioSegment(tag+4, DataType.SHORT16);
                    segShorts.appendShortData(shortData);
                    builder.addChild(bankSegs, segShorts);

                    // segment of segments
                    EvioSegment segSegs = new EvioSegment(tag+5, DataType.SEGMENT);
                    builder.addChild(bankSegs, segSegs);

                        // segment of doubles
                        EvioSegment segDoubles = new EvioSegment(tag+6, DataType.DOUBLE64);
                        segDoubles.appendDoubleData(doubleData);
                        builder.addChild(segSegs, segDoubles);

                // Bank of tag segs
                EvioBank bankTagsegs = new EvioBank(tag+7, DataType.TAGSEGMENT, 1);
                builder.addChild(event, bankTagsegs);

                    // tag segment of bytes
                    EvioTagSegment tagBytes = new EvioTagSegment(tag+8, DataType.CHAR8);
                    tagBytes.appendByteData(byteData);
                    builder.addChild(bankTagsegs, tagBytes);


        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        return event;

    }

    static EvioEvent createSingleEvent2(int tag) {

        // Top level event
        EvioEvent event = null;

        // data  (1500 bytes)
        int[] intData = new int[1];
        Arrays.fill(intData, 1);

        byte[] byteData = new byte[4];
        Arrays.fill(byteData, (byte)2);

        short[] shortData = new short[2];
        Arrays.fill(shortData, (short)3);

        double[] doubleData = new double[1];
        Arrays.fill(doubleData, 4.);

        try {

            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(tag, DataType.SEGMENT, 1);
            event = builder.getEvent();


                    // segment of shorts
                    EvioSegment segShorts = new EvioSegment(tag+4, DataType.SHORT16);
                    segShorts.appendShortData(shortData);
                    builder.addChild(event, segShorts);


                    // segment of doubles
                    EvioSegment segDoubles = new EvioSegment(tag+6, DataType.DOUBLE64);
                    segDoubles.appendDoubleData(doubleData);
                    builder.addChild(event, segDoubles);


//                // Bank of tag segs
//                EvioBank bankTagsegs = new EvioBank(tag+7, DataType.TAGSEGMENT, 1);
//                builder.addChild(event, bankTagsegs);
//
//                    // tag segment of bytes
//                    EvioTagSegment tagBytes = new EvioTagSegment(tag+8, DataType.CHAR8);
//                    tagBytes.appendByteData(byteData);
//                    builder.addChild(bankTagsegs, tagBytes);


        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        return event;

    }





    static ByteBuffer createSingleSegment(int tag) {

        EvioSegment seg = new EvioSegment(tag, DataType.INT32);

        int[] intData = new int[10];
        Arrays.fill(intData, 456);

        try {
            seg.appendIntData(intData);
        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        ByteBuffer bb = ByteBuffer.allocate(4*12);
        seg.write(bb);
        bb.flip();

        return bb;
    }



    static ByteBuffer createSingleBank(int tag, int num) {

         EvioEvent bank = createSingleEvent1(tag);
         int byteSize = bank.getTotalBytes();

         ByteBuffer bb = ByteBuffer.allocate(byteSize);
         bank.write(bb);
         bb.flip();

         return bb;
     }

    static ByteBuffer createSingleBank2(int tag, int num) {

         EvioBank bank = new EvioBank(tag, DataType.INT32, num);

         int[] intData = new int[10];
         Arrays.fill(intData, 123);

         try {
             bank.appendIntData(intData);
         }
         catch (EvioException e) {
             e.printStackTrace();
         }

         ByteBuffer bb = ByteBuffer.allocate(4*12);
         bank.write(bb);
         bb.flip();

         return bb;
     }


    static ByteBuffer createBuffer(int eventCount) {

        String xmlDict = null;

        String xmlDict2 =
                "<xmlDict attr='junk'>"  +
                    "<dictEntry name='T7N0'    tag= '7'    num = '0' />"  +
                    "<dictEntry name='T3N3'    tag= '3'    num = '3' />"  +
                    "<dictEntry name='T66N99'  tag= '66'   num = '99' />"  +
                "</xmlDict>";

        // Create a buffer
        ByteBuffer myBuf = ByteBuffer.allocate(1000 * eventCount);
//        myBuf.position(100);

        try {
            // Create an event writer to write into "myBuf" with 10000 ints or 100 events/block max
            EventWriter writer = new EventWriter(myBuf, 10000, 100, xmlDict, null);

//            EvioEvent ev = createSingleEvent(1);
            EvioEvent ev = createSimpleSingleEvent(1);

            for (int i=0; i < eventCount; i++) {
                // Write event to buffer
                writer.writeEvent(ev);
            }

            // All done writing
            writer.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        // Get ready to read
        myBuf.flip();
//        myBuf.position(100);

        return myBuf;
    }


    static void printBytes(ByteBuffer buf) {
        for (int i=buf.position(); i < buf.capacity(); i++) {
            System.out.println("byte = " + buf.get(i));
        }
        System.out.println("\n");
    }

    /** For reading a local file/buffer, take events and put them into
     *  a EvioCompactEventWriter and write a file with it. */
    public static void main5(String args[]) {

        ByteBuffer buf = ByteBuffer.allocate(4);
        buf.order(ByteOrder.BIG_ENDIAN);

        buf.putInt(0, 666);
        printBytes(buf);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        int readVal = buf.getInt(0);
        System.out.println("read value = " + readVal);
        printBytes(buf);

    }

    /** For reading a local buffer, take events and put them into
     *  an EventWriter and write a buffer with it. */
    public static void main_buffer(String args[]) {

        // Top level events
        EvioEvent event1, event2, event3;

        // data
        int[] intDataHuge  = new int[1248];
        int[] intDataBig   = new int[200];
        int[] intDataSmall = new int[ 10];
        Arrays.fill(intDataBig, 1);
        Arrays.fill(intDataHuge, 2);
        Arrays.fill(intDataSmall, 3);

        try {
            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(5, DataType.BANK, 5);
            event3 = builder.getEvent();
            // bank of ints
            EvioBank bankInts = new EvioBank(6, DataType.INT32, 6);
            bankInts.appendIntData(intDataHuge);
            builder.addChild(event3, bankInts);


            EventBuilder builder1 = new EventBuilder(1, DataType.BANK, 1);
            event1 = builder1.getEvent();
            bankInts = new EvioBank(2, DataType.INT32, 2);
            bankInts.appendIntData(intDataBig);
            builder1.addChild(event1, bankInts);


            EventBuilder builder2 = new EventBuilder(3, DataType.BANK, 3);
            event2 = builder2.getEvent();
            bankInts = new EvioBank(4, DataType.INT32, 4);
            bankInts.appendIntData(intDataSmall);
            builder2.addChild(event2, bankInts);


//            private EventWriter(ByteBuffer buf, int blockSizeMax, int blockCountMax,
//                                String xmlDictionary, BitSet bitInfo, int reserved1,
//                                boolean append) throws EvioException {


            if (false) {
                ByteBuffer buffie = ByteBuffer.allocate(7664);
                EventWriter writer = new EventWriter(buffie, 300, 1000, null, null, 0, false);

                long t2, t1 = System.currentTimeMillis();

                for (int i=0; i < 15; i++)  {
                    writer.writeEvent(event2); // little
                }

                writer.writeEvent(event1);  // med

                for (int i=0; i < 15; i++)  {
                    writer.writeEvent(event2);  // little
                }

                writer.writeEvent(event3);  // big

                writer.close();
                t2 = System.currentTimeMillis();
                System.out.println("time (ms) = " + (t2 - t1));

                buffie.flip();

                Utilities.bufferToFile("/dev/shm/carlTest/file", buffie, true, false);
            }
            else {
                long t2, t1 = System.currentTimeMillis();

                ByteBuffer buffie = ByteBuffer.allocate(8560);
                EventWriter writer = new EventWriter(buffie, 300, 1000, null, null, 0, false);
                writer.writeEvent(event2);  // little
                writer.close();

                for (int i=0; i < 14; i++)  {
                    writer = new EventWriter(buffie, 300, 1000, null, null, 0, true);
                    writer.writeEvent(event2); // little
                    writer.close();
                }

                writer = new EventWriter(buffie, 300, 1000, null, null, 0, true);
                writer.writeEvent(event1);  // med
                writer.close();

                for (int i=0; i < 15; i++)  {
                    writer = new EventWriter(buffie, 300, 1000, null, null, 0, true);
                    writer.writeEvent(event2); // little
                    writer.close();
                }

                writer = new EventWriter(buffie, 300, 1000, null, null, 0, true);
                writer.writeEvent(event3);  // big
                writer.close();

                t2 = System.currentTimeMillis();
                System.out.println("time (ms) = " + (t2 - t1));

                buffie.flip();

                Utilities.bufferToFile("/dev/shm/carlTest/file", buffie, true, false);
            }

        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

    }



    /** For reading a local file, take events and put them into
     *  an EventWriter and write a file with it. */
    public static void main44(String args[]) {

        // Top level events
        EvioEvent event1, event2, event3;

        // data
        int[] intDataHuge  = new int[1248];
        int[] intDataBig   = new int[200];
        int[] intDataSmall = new int[ 10];
        Arrays.fill(intDataBig, 1);
        Arrays.fill(intDataHuge, 2);
        Arrays.fill(intDataSmall, 3);

        try {
            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(5, DataType.BANK, 5);
            event3 = builder.getEvent();
            // bank of ints
            EvioBank bankInts = new EvioBank(6, DataType.INT32, 6);
            bankInts.appendIntData(intDataHuge);
            builder.addChild(event3, bankInts);


            EventBuilder builder1 = new EventBuilder(1, DataType.BANK, 1);
            event1 = builder1.getEvent();
            bankInts = new EvioBank(2, DataType.INT32, 2);
            bankInts.appendIntData(intDataBig);
            builder1.addChild(event1, bankInts);


            EventBuilder builder2 = new EventBuilder(3, DataType.BANK, 3);
            event2 = builder2.getEvent();
            bankInts = new EvioBank(4, DataType.INT32, 4);
            bankInts.appendIntData(intDataSmall);
            builder2.addChild(event2, bankInts);


//            public EventWriter(String baseName, String directory, String runType,
//                               int runNumber, int split,
//                               int blockSizeMax (ints), int blockCountMax, int bufferSize (bytes),
//                               ByteOrder byteOrder, String xmlDictionary,
//                               BitSet bitInfo, boolean overWriteOK, boolean append)
//

            if (false) {
                EventWriter writer = new EventWriter(
                        "/dev/shm/carlTest/file",
                        null, null, 0, 1000, 300, 1000, 1200, null, null, null, true, false);

                long t2, t1 = System.currentTimeMillis();

                for (int i=0; i < 15; i++)  {
                    writer.writeEvent(event2); // little
                }

                writer.writeEvent(event1);  // med

                for (int i=0; i < 15; i++)  {
                    writer.writeEvent(event2);  // little
                }

                writer.writeEvent(event3);  // big

                writer.close();
                t2 = System.currentTimeMillis();
                System.out.println("time (ms) = " + (t2 - t1));
            }
            else {
                long t2, t1 = System.currentTimeMillis();

                EventWriter writer = new EventWriter(
                        "/dev/shm/carlTest/file",
                        null, null, 0, 0, 300, 1000, 1200, null, null, null, true, false);

                writer.writeEvent(event2);  // small
                writer.close();


                writer = new EventWriter(
                        "/dev/shm/carlTest/file",
                        null, null, 0, 0, 300, 1000, 1200, null, null, null, false, true);

                writer.writeEvent(event1);  // med
                writer.close();

                writer = new EventWriter(
                        "/dev/shm/carlTest/file",
                        null, null, 0, 0, 300, 1000, 1200, null, null, null, false, true);

                writer.writeEvent(event3);  // big
                writer.close();

                t2 = System.currentTimeMillis();
                System.out.println("time (ms) = " + (t2 - t1));

            }

        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

    }


    /** For reading a local file, take events and put them into
     *  an EventWriter and write a file with it. */
    public static void main_compactFile(String args[]) {

        // Top level events
        EvioEvent event1, event2, event3;

        // data
        int[] intDataHuge  = new int[1248];
        int[] intDataBig   = new int[200];
        int[] intDataSmall = new int[ 10];
        Arrays.fill(intDataBig, 1);
        Arrays.fill(intDataHuge, 2);
        Arrays.fill(intDataSmall, 3);

        try {
            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(5, DataType.BANK, 5);
            event3 = builder.getEvent();
            // bank of ints
            EvioBank bankInts = new EvioBank(6, DataType.INT32, 6);
            bankInts.appendIntData(intDataHuge);
            builder.addChild(event3, bankInts);


            EventBuilder builder1 = new EventBuilder(1, DataType.BANK, 1);
            event1 = builder1.getEvent();
            bankInts = new EvioBank(2, DataType.INT32, 2);
            bankInts.appendIntData(intDataBig);
            builder1.addChild(event1, bankInts);


            EventBuilder builder2 = new EventBuilder(3, DataType.BANK, 3);
            event2 = builder2.getEvent();
            bankInts = new EvioBank(4, DataType.INT32, 4);
            bankInts.appendIntData(intDataSmall);
            builder2.addChild(event2, bankInts);



//            public EvioCompactEventWriter(String baseName, String directory, int runNumber, int split,
//                                         int blockSizeMax, int blockCountMax, int bufferSize,
//                                         ByteOrder byteOrder, String xmlDictionary,
//                                         boolean overWriteOK)


            ByteBuffer evBuf1 = ByteBuffer.allocate(event1.getTotalBytes());
            event1.write(evBuf1);
            evBuf1.flip();

            ByteBuffer evBuf2 = ByteBuffer.allocate(event2.getTotalBytes());
            event2.write(evBuf2);
            evBuf2.flip();

            ByteBuffer evBuf3 = ByteBuffer.allocate(event3.getTotalBytes());
            event3.write(evBuf3);
            evBuf3.flip();


            if (false) {
                EvioCompactEventWriter writer = new EvioCompactEventWriter(
                        "/dev/shm/carlTest/file",
                        null, 0, 1000, 4*300, 1000, 1200, null, null, true);

                long t2, t1 = System.currentTimeMillis();

                for (int i=0; i < 15; i++)  {
                    writer.writeEvent(evBuf2); // little
                    evBuf2.flip();
                }

                writer.writeEvent(evBuf1);  // med
                evBuf1.flip();

                for (int i=0; i < 15; i++)  {
                    writer.writeEvent(evBuf2);  // little
                    evBuf2.flip();
                }

                writer.writeEvent(evBuf3);  // big
                evBuf3.flip();

                writer.close();
                t2 = System.currentTimeMillis();
                System.out.println("time (ms) = " + (t2 - t1));
            }
            else {
                long t2, t1 = System.currentTimeMillis();

                EvioCompactEventWriter writer = new EvioCompactEventWriter(
                        "/dev/shm/carlTest/file",
                        null, 0, 1000, 4*300, 1000, 1200, null, null, true);

                writer.writeEvent(evBuf2);  // small
                evBuf2.flip();
                writer.close();


                writer = new EvioCompactEventWriter(
                        "/dev/shm/carlTest/file",
                        null, 0, 1000, 4*300, 1000, 1200, null, null, true);

                writer.writeEvent(evBuf1);  // med
                evBuf1.flip();
                writer.close();

                writer = new EvioCompactEventWriter(
                        "/dev/shm/carlTest/file",
                        null, 0, 1000, 4*300, 1000, 1200, null, null, true);

                writer.writeEvent(evBuf3);  // big
                evBuf3.flip();
                writer.close();

                t2 = System.currentTimeMillis();
                System.out.println("time (ms) = " + (t2 - t1));

            }

        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

    }



    /** For reading a local file, take events and put them into
     *  an EventWriter and write a file with it. */
    public static void main_compactBuffer(String args[]) {

        // Top level events
        EvioEvent event1, event2, event3;

        // data
        int[] intDataHuge  = new int[1248];
        int[] intDataBig   = new int[200];
        int[] intDataSmall = new int[ 10];
        Arrays.fill(intDataBig, 1);
        Arrays.fill(intDataHuge, 2);
        Arrays.fill(intDataSmall, 3);

        try {
            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(5, DataType.BANK, 5);
            event3 = builder.getEvent();
            // bank of ints
            EvioBank bankInts = new EvioBank(6, DataType.INT32, 6);
            bankInts.appendIntData(intDataHuge);
            builder.addChild(event3, bankInts);


            EventBuilder builder1 = new EventBuilder(1, DataType.BANK, 1);
            event1 = builder1.getEvent();
            bankInts = new EvioBank(2, DataType.INT32, 2);
            bankInts.appendIntData(intDataBig);
            builder1.addChild(event1, bankInts);


            EventBuilder builder2 = new EventBuilder(3, DataType.BANK, 3);
            event2 = builder2.getEvent();
            bankInts = new EvioBank(4, DataType.INT32, 4);
            bankInts.appendIntData(intDataSmall);
            builder2.addChild(event2, bankInts);


//            * @param byteBuffer    buffer into which events are written.
//            * @param blockSizeMax  the max blocksize in bytes which must be >= 400 B and <= 4 MB.
//            *                      The size of the block will not be larger than this size
//            *                      unless a single event itself is larger.
//            * @param blockCountMax the max number of events in a single block which must be
//            *                      >= {@link EventWriter#MIN_BLOCK_COUNT} and <= {@link EventWriter#MAX_BLOCK_COUNT}.
//            * @param xmlDictionary dictionary in xml format or null if none.
//            *
//            * @throws EvioException if blockSizeMax or blockCountMax exceed limits;
//            *                       if buffer arg is null
//            */
//            public EvioCompactEventWriter(ByteBuffer byteBuffer, int blockSizeMax,
//            int blockCountMax, String xmlDictionary)

            ByteBuffer evBuf1 = ByteBuffer.allocate(event1.getTotalBytes());
            event1.write(evBuf1);
            evBuf1.flip();

            ByteBuffer evBuf2 = ByteBuffer.allocate(event2.getTotalBytes());
            event2.write(evBuf2);
            evBuf2.flip();

            ByteBuffer evBuf3 = ByteBuffer.allocate(event3.getTotalBytes());
            event3.write(evBuf3);
            evBuf3.flip();


            if (false) {
                ByteBuffer buffie = ByteBuffer.allocate(7664);
                EvioCompactEventWriter writer =
                        new EvioCompactEventWriter(buffie, 4*300, 1000, null);

                long t2, t1 = System.currentTimeMillis();

                for (int i=0; i < 15; i++)  {
                    writer.writeEvent(evBuf2); // little
                    evBuf2.flip();
                }

                writer.writeEvent(evBuf1);  // med
                evBuf1.flip();

                for (int i=0; i < 15; i++)  {
                    writer.writeEvent(evBuf2);  // little
                    evBuf2.flip();
                }

                writer.writeEvent(evBuf3);  // big
                evBuf3.flip();

                writer.close();
                t2 = System.currentTimeMillis();
                System.out.println("time (ms) = " + (t2 - t1));

                buffie.flip();

                Utilities.bufferToFile("/dev/shm/carlTest/file", buffie, true, false);
            }
            else {
                long t2, t1 = System.currentTimeMillis();

                ByteBuffer buffie = ByteBuffer.allocate(7664);
                EvioCompactEventWriter writer = new EvioCompactEventWriter(buffie, 4*300, 1000, null);

                writer.writeEvent(evBuf2);  // small
                evBuf2.flip();
                writer.close();

                writer = new EvioCompactEventWriter(buffie, 4*300, 1000, null);

                writer.writeEvent(evBuf1);  // med
                evBuf1.flip();
                writer.close();

                writer = new EvioCompactEventWriter(buffie, 4*300, 1000, null);

                writer.writeEvent(evBuf3);  // big
                evBuf3.flip();
                writer.close();

                t2 = System.currentTimeMillis();
                System.out.println("time (ms) = " + (t2 - t1));

                buffie.flip();

                Utilities.bufferToFile("/dev/shm/carlTest/file", buffie, true, false);
            }

        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

    }


    /** For reading a local file/buffer, take events and put them into
     *  a EvioCompactEventWriter and write a file with it. */
    public static void main33(String args[]) {

        // Top level events
        EvioEvent event1, event2, event3;

        // data
        int[] intDataHuge  = new int[1248];
        int[] intDataBig   = new int[200];
        int[] intDataSmall = new int[ 10];
        Arrays.fill(intDataBig, 1);
        Arrays.fill(intDataHuge, 2);
        Arrays.fill(intDataSmall, 3);

        try {
            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(5, DataType.BANK, 5);
            event3 = builder.getEvent();
            // bank of ints
            EvioBank bankInts = new EvioBank(6, DataType.INT32, 6);
            bankInts.appendIntData(intDataHuge);
            builder.addChild(event3, bankInts);


            EventBuilder builder1 = new EventBuilder(1, DataType.BANK, 1);
            event1 = builder1.getEvent();
            bankInts = new EvioBank(2, DataType.INT32, 2);
            bankInts.appendIntData(intDataBig);
            builder1.addChild(event1, bankInts);


            EventBuilder builder2 = new EventBuilder(3, DataType.BANK, 3);
            event2 = builder2.getEvent();
            bankInts = new EvioBank(4, DataType.INT32, 4);
            bankInts.appendIntData(intDataSmall);
            builder2.addChild(event2, bankInts);

//            public EvioCompactEventWriter(String baseName, String directory, int runNumber, int split,
//                                         int blockSizeMax, int blockCountMax, int bufferSize,
//                                         ByteOrder byteOrder, String xmlDictionary,
//                                         boolean overWriteOK)

            EvioCompactEventWriter writer = new EvioCompactEventWriter("file", null, 0, 0,
                    256000, 20000, 256000, null, null, true);

            ByteBuffer evBuf1 = ByteBuffer.allocate(event1.getTotalBytes());
            event1.write(evBuf1);
            evBuf1.flip();

            ByteBuffer evBuf2 = ByteBuffer.allocate(event2.getTotalBytes());
            event2.write(evBuf2);
            evBuf2.flip();

            ByteBuffer evBuf3 = ByteBuffer.allocate(event3.getTotalBytes());
            event3.write(evBuf3);
            evBuf3.flip();

            long t2, t1 = System.currentTimeMillis();
            for (int i=0; i< 200000; i++)  {
                writer.writeEvent(evBuf3);
                evBuf3.flip();
            }
            t2 = System.currentTimeMillis();
            System.out.println("time (ms) = " + (t2 - t1));
        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

    }




    /** For reading a local file/buffer, take events and put them into
     *  a EvioCompactEventWriter and write a file with it. */
    public static void main0(String args[]) {

        int evCount;


        try {
            EvioCompactReader reader = new EvioCompactReader("/daqfs/home/timmer/coda/evioTestFiles/BSTN2.ev");

            // Get each event in the buffer
            evCount = reader.getEventCount();

            Scanner keyboard = new Scanner(System.in);
            keyboard.next().charAt(0);

            for (int i=0; i < evCount; i++) {
                Thread.sleep(1);
                reader.searchEvent(i+1, 10, 1);
            }

            reader.close();

        }
        catch (EvioException e) {
            e.printStackTrace();
            System.exit(-1);
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit(-1);
        }
    }




    /**
       3 block headers (first 2 have 2 extra words each, last has 1 extra word).
       First block has 2 events. Second has 3 events.
       Last is empty final block.
    */
    static int data1[] = {
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

        0x0000000f,
        0x00000002,
        0x0000000A,
        0x00000002,
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
            0x00000003,
            0x00000003,
            0x00000003,
    };




    /** For reading a local file/buffer, take events and put them into
     *  a EvioCompactEventWriter and write a file with it. */
    public static void main(String args[]) {

        int evCount;

        String fileName  = "/tmp/testFile.ev";
        System.out.println("Write file " + fileName);


        // Write evio file that has extra words in headers
        try {
            byte[] be  = ByteDataTransformer.toBytes(data1, ByteOrder.BIG_ENDIAN);
            ByteBuffer buf = ByteBuffer.wrap(be);
            File file = new File(fileName);
            FileOutputStream fileOutputStream = new FileOutputStream(file);
            FileChannel fileChannel = fileOutputStream.getChannel();
            fileChannel.write(buf);
            fileChannel.close();
        }
        catch (Exception e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }


        System.out.println("\nRead file " + fileName);

        File fileIn = new File(fileName);

        System.out.println("\nEvioReader file: " + fileName);
        try {
            EvioReader reader = new EvioReader(fileName);
            System.out.println("\nev count= " + reader.getEventCount());
            System.out.println("dictionary = " + reader.getDictionaryXML() + "\n");


            System.out.println("ev count = " + reader.getEventCount());

        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        System.out.println("\nCompactEvioReader file: " + fileName);
        try {
            EvioCompactReader reader = new EvioCompactReader(fileName);
            System.out.println("\nev count= " + reader.getEventCount());
            System.out.println("dictionary = " + reader.getDictionaryXML() + "\n");


            System.out.println("ev count = " + reader.getEventCount());

        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }


        fileIn.delete();

    }




    /** For WRITING a local file. */
    public static void main00(String args[]) {

        // Create buffer with 5 events
        int count = 5, evCount;
        ByteBuffer myBuf = createBuffer(count);
        System.out.println("created buffer, pos = " + myBuf.position() + ", lim = " + myBuf.limit());

        // Create buffer containing bank to add dynamically
        ByteBuffer addBankBuf = createSingleBank(7, 7);
System.out.println("addBankBuf size = " + addBankBuf.remaining());

        List<EvioNode> returnList;

        try {
            EvioCompactReader reader = new EvioCompactReader(myBuf);
            //EvioCompactReader reader = new EvioCompactReader("/tmp/compactTest");

            EvioNode node = reader.getEvent(4);
System.out.println("event 4 pos = " + node.getPosition());

            // Get each event in the buffer
            evCount = reader.getEventCount();

            returnList = reader.searchEvent(4, 1, 1);
            if (returnList.size() < 1) {
                System.out.println("GOT NOTHING IN SEARCH for ev 4, T/N = 1/1");
            }
            else {
                System.out.println("Found " + returnList.size() + " banks for ev 4, T/N = 1/1");
            }

            returnList = reader.searchEvent(2, 7, 7);
            if (returnList.size() < 1) {
                System.out.println("GOT NOTHING IN SEARCH for ev 2, T/N = 7/7");
            }
            else {
                System.out.println("Found " + returnList.size() + " banks for ev 2, T/N = 7/7");
            }

System.out.println("Insert buffer of " + addBankBuf.remaining() + " bytes");
            reader.addStructure(2, addBankBuf);
System.out.println("Insert another buffer of " + addBankBuf.remaining() + " bytes");
            reader.addStructure(2, addBankBuf);

//System.out.println("write buffer to file /tmp/compactTest");
            //reader.toFile("/tmp/compactTest");

System.out.println("event 4 again, pos = " + node.getPosition());

            returnList = reader.searchEvent(4, 7, 7);
            if (returnList.size() < 1) {
                System.out.println("GOT NOTHING IN SEARCH for ev4, T/N = 7/7");
            }
            else {
                System.out.println("Data for T/n = 7/7 ->");
                EvioNode node70 = returnList.get(0);
                ByteBuffer buf = reader.getData(node70);
                DataType dType = node70.getDataTypeObj();
                System.out.println("Get data of type " + dType);
                if (dType == DataType.DOUBLE64) {
                    System.out.println("Double data =");
                    DoubleBuffer dbuf = buf.asDoubleBuffer();
                    for (int k=0; k < dbuf.limit(); k++) {
                        System.out.println(k + "   " + dbuf.get(k));
                    }
                }
            }
        }
        catch (EvioException e) {
            e.printStackTrace();
            System.exit(-1);
        }
    }




    /** For WRITING a local file. */
    public static void main2(String args[]) {

        // Create buffer with 100 events
        int count = 5, evCount = 0, loops = 100000;
        ByteBuffer myBuf = createBuffer(count);
        System.out.println("created buffer, pos = " + myBuf.position() + ", lim = " + myBuf.limit());

        // Create buffer containing bank to add dynamically
        ByteBuffer bankBuf = createSingleBank(66, 99);
        //ByteBuffer bankBuf = createSingleSegment(66);


        try {

            // keep track of time
            long t1, t2, time;
            double rate;

            List<EvioNode> returnList;
            List<BaseStructure> retList1, retList2;

            t1 = System.currentTimeMillis();


            // Doing things the old-fashioned way ...
            if (false) {
                for (int i=0; i < loops; i++) {
                    // Use the original reader which does a deserialization
                    try {
                        // Define a filter to select structures tag=7 & num=0.
                        class myEvioFilter implements IEvioFilter {
                            int tag, num;
                            myEvioFilter(int tag, int num) {this.tag = tag; this.num = num;}
                            public boolean accept(StructureType type, IEvioStructure struct){
                                return ((struct.getHeader().getTag() == tag) &&
                                        (struct.getHeader().getNumber() == num));
                            }
                        };

                        // Create the defined filter
                        myEvioFilter filter70 = new myEvioFilter(7,0);
                        myEvioFilter filter33 = new myEvioFilter(3,3);

                        // Create reader
                        EvioReader reader = new EvioReader(myBuf);

                        // Get each event in the buffer
                        evCount = reader.getEventCount();
                        for (int j=1; j <= evCount; j++) {
                            EvioEvent ev = reader.parseNextEvent();

                            // Use the filter to search "event"
                            retList1 = StructureFinder.getMatchingStructures(ev, filter70);
                            retList2 = StructureFinder.getMatchingStructures(ev, filter33);
                            if (retList1 != null) {
System.out.println("Return list size for search of tag/num = 7/0 -> " + retList1.size());
                               for (BaseStructure bs : retList1) {
                               }
                            }
                        }
                    }
                    catch (IOException e) {
                        e.printStackTrace();
                        System.exit(-1);
                    }

                }

                // calculate the event rate
                t2 = System.currentTimeMillis();
                time = t2 - t1;
                rate = 1000.0 * ((double) loops*evCount) / time;
                System.out.println("rate = " + String.format("%.3g", rate) + " Hz");
                System.out.println("time = " + (time) + " milliseconds");
            }

            t1 = System.currentTimeMillis();


            // Doing things the new way with EvioCompactReader
            if (true) {
                for (int i=0; i < 1; i++) {
                    try {
                        EvioCompactReader reader = new EvioCompactReader(myBuf);
                        //EvioCompactReader reader = new EvioCompactReader("/tmp/compactTest");
                        if (reader.hasDictionary()) {
                            System.out.println(" *** We Have a dictionary ***");
                        }

                        String xmlDict = reader.getDictionaryXML();
                        if (xmlDict != null) {
                            System.out.println("HEY WE GOTTA DICTIONARY String = \n" + xmlDict);
                        }

                        EvioXMLDictionary dictionary = reader.getDictionary();
                        if (dictionary != null) {
                            System.out.println("HEY WE GOTTA DICTIONARY OBJECT !!!");
                        }


                        // Get each event in the buffer
                        evCount = reader.getEventCount();

                        for (int j=0; j < 1; j++) {

//System.out.println("Insert buffer of " + bankBuf.remaining() + " bytes");
                            reader.addStructure(1, bankBuf);

//System.out.println("write buffer to file /tmp/compactTest");
                            //reader.toFile("/tmp/compactTest");

                            returnList = reader.searchEvent(1, 66, 99);
                            //returnList = reader.searchEvent(1, 66, 0);
                            if (returnList.size() < 1) {
                                System.out.println("GOT NOTHING IN SEARCH for T66N99");
                            }
                            else {
                                EvioNode node70 = returnList.get(0);
System.out.println("Data for T66N99 (ret list = " + returnList.size() + ") ->");
                                ByteBuffer buf = reader.getData(node70);
                                DataType dType = node70.getDataTypeObj();
                                System.out.println("Get data of type " + dType);
                                if (dType == DataType.INT32) {
                                    System.out.println("Int data =");
                                    IntBuffer ibuf = buf.asIntBuffer();
                                    for (int k=0; k < ibuf.limit(); k++) {
                                        System.out.println(k + "   " + ibuf.get(k));
                                    }
                                }
                            }

                            //returnList = reader.searchEvent(j+1, "T7N0", dictionary);
                            returnList = reader.searchEvent(3, 7, 0);
                            if (returnList.size() < 1) {
                                System.out.println("GOT NOTHING IN SEARCH for T7N0");
                            }
                            else {
                                System.out.println("Data for T7N0 ->");
                                EvioNode node70 = returnList.get(0);
                                ByteBuffer buf = reader.getData(node70);
                                DataType dType = node70.getDataTypeObj();
                                System.out.println("Get data of type " + dType);
                                if (dType == DataType.DOUBLE64) {
                                    System.out.println("Double data =");
                                    DoubleBuffer dbuf = buf.asDoubleBuffer();
                                    for (int k=0; k < dbuf.limit(); k++) {
                                        System.out.println(k + "   " + dbuf.get(k));
                                    }
                                }
                            }

                            //returnList = reader.searchEvent(j+1,"T3N3", dictionary);
                            returnList = reader.searchEvent(3, 5, 0);
                            if (returnList.size() < 1) {
                                System.out.println("GOT NOTHING IN SEARCH for T5N0");
                            }
                            else {
                                System.out.println("Data for T5N0 ->");
                                EvioNode node33 = returnList.get(0);
                                ByteBuffer buf = reader.getData(node33);
                                DataType dType = node33.getDataTypeObj();
                                System.out.println("Get data of type " + dType);
                                if (dType == DataType.SHORT16) {
                                    System.out.println("Short data =");
                                    ShortBuffer ibuf = buf.asShortBuffer();
                                    for (short k=0; k < ibuf.limit(); k++) {
                                        System.out.println(k + "   " + ibuf.get(k));
                                    }
                                }
                            }


                            returnList = reader.searchEvent(3, 3, 3);
                            if (returnList.size() < 1) {
                                System.out.println("GOT NOTHING IN SEARCH for T3N3");
                            }
                            else {
                                System.out.println("Data for T3N3 ->");
                                EvioNode node33 = returnList.get(0);
                                ByteBuffer buf = reader.getData(node33);
                                DataType dType = node33.getDataTypeObj();
                                System.out.println("Get data of type " + dType);
                                if (dType == DataType.INT32) {
                                    System.out.println("Int data =");
                                    IntBuffer ibuf = buf.asIntBuffer();
                                    for (int k=0; k < ibuf.limit(); k++) {
                                        System.out.println(k + "   " + ibuf.get(k));
                                    }
                                }
                            }

                        }
                    }
                    catch (EvioException e) {
                        e.printStackTrace();
                        System.exit(-1);
                    }
//                    catch (IOException e) {
//                        e.printStackTrace();
//                        System.exit(-1);
//                    }

                }

                // calculate the event rate
                t2 = System.currentTimeMillis();
                time = t2 - t1;
                rate = 1000.0 * ((double) loops*evCount) / time;
                System.out.println("rate = " + String.format("%.3g", rate) + " Hz");
                System.out.println("time = " + (time) + " milliseconds");

            }

        }
        catch (EvioException e) {
            e.printStackTrace();
        }
    }


}
