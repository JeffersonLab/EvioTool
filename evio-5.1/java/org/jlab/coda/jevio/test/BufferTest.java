package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.util.Arrays;
import java.util.List;

/**
 * Test program.
 * @author timmer
 * Date: Oct 7, 2010
 */
public class BufferTest {


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

    /** One block header with 1 event. */
    static int data2[] = {
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
        0x00000001,
        0x00000001
    };

    /** One block header with extra int, no events. */
    static int data3[] = {
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

    /** 2 block headers, both empty & each with extra int. */
    static int data4[] = {
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



    /** For testing only */
    public static void main(String args[]) {

        try {

            // Evio event = bank of int data
            EventBuilder eb = new EventBuilder(1, DataType.INT32, 1);
            EvioEvent ev = eb.getEvent();
            int[] dat = new int[] {1, 2, 3, 4};
            ev.appendIntData(dat);

            // create writer with max block size of 256 ints
            EventWriter evWriter = new EventWriter(ByteBuffer.allocate(64), 256, 20, null, null);
            evWriter.close();

            // create buffer to write to of size 274 ints (> 8 + 244 + 8 + 6 + 8)
            // which should hold 3 block headers and both events
            ByteBuffer buffer = ByteBuffer.allocate(4*274);
            buffer.clear();
            evWriter.setBuffer(buffer);

            // write first event - 244 ints long  (with block header = 252 ints)
            evWriter.writeEvent(ev);

            // this should leave room for 4 ints before writer starts a new block header -
            // not enough for the next event which will be 2 header ints + 4 data ints
            // for a total size of 6 ints
            EvioEvent ev2 = new EvioEvent(2, DataType.INT32, 2);
            int[] dat2 = new int[] {2, 2, 2, 2};
            ev2.appendIntData(dat2);

            // write second event - 6 ints long. This should force a second block header.
            evWriter.writeEvent(ev2);
            evWriter.close();

            // Lets's read what we've written
            buffer.flip();

            EvioReader reader = new EvioReader(buffer);
            long evCount = reader.getEventCount();
            System.out.println("Read buffer, got " + evCount + " events & " +
                    reader.getBlockCount() + " blocks\n");

            EvioEvent event;
            while ( (event = reader.parseNextEvent()) != null) {
                System.out.println("Event = " + event.toXML());
                System.out.println("\nevent count = " + reader.getEventCount() + "\n");
            }

        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /** Create writer, close, change buffer, write event, read it. */
    public static void main1(String args[]) {

        try {
            // Evio event = bank of double data
            EventBuilder eb = new EventBuilder(1, DataType.DOUBLE64, 1);
            EvioEvent ev = eb.getEvent();
            double[] da = new double[] {1., 2., 3.};
            ev.appendDoubleData(da);

            EventWriter evWriter = new EventWriter(ByteBuffer.allocate(64), 550000, 200, null, null);
            evWriter.close();

            ByteBuffer buffer = ByteBuffer.allocate(4000);

            for (int j=0; j < 2; j++) {
                buffer.clear();
                evWriter.setBuffer(buffer);
                evWriter.writeEvent(ev);
                evWriter.close();
                buffer.flip();

                EvioReader reader = new EvioReader(buffer);
                long evCount = reader.getEventCount();
                System.out.println("Read buffer, got " + evCount + " number of events\n");

                EvioEvent event;
                while ( (event = reader.parseNextEvent()) != null) {
                   System.out.println("Event = " + event.toXML());
                    System.out.println("\nevent count = " + reader.getEventCount() + "\n");
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    /** Append event to buffer of events. Change buffer and repeat. */
    public static void main2(String args[]) {

        try {

            // Create event to append
            EventBuilder eventBuilder = new EventBuilder(1, DataType.INT32, 1);
            EvioEvent ev = eventBuilder.getEvent();
            ev.appendIntData(new int[]  {4,5,6});

            // Start with a buffer
            byte[] be  = ByteDataTransformer.toBytes(data2, ByteOrder.BIG_ENDIAN);
            // If we are to write into this array we need to expand it first
            byte[] bee = Arrays.copyOf(be, be.length + 4*100);
            ByteBuffer buf = ByteBuffer.wrap(bee);

            EventWriter writer = new EventWriter(buf, 1000000, 3, null, null, true);

System.out.println("Main: writer blockNum = " + writer.getBlockNumber());

            writer.writeEvent(ev);
            writer.close();

System.out.println("Main: (before flip) buf limit = " + buf.limit() +
                   ", pos = " + buf.position() +
                   ", cap = " + buf.capacity());
            buf.flip();

System.out.println("Main: (after flip) buf limit = " + buf.limit() +
                   ", pos = " + buf.position() +
                   ", cap = " + buf.capacity());

            // Read appended-to buffer
            IntBuffer iBuf = buf.asIntBuffer();
            while(iBuf.hasRemaining()) {
                System.out.println("0x" + Integer.toHexString(iBuf.get()));
            }


            // Use a different buffer now but the same writer
            be  = ByteDataTransformer.toBytes(data3, ByteOrder.BIG_ENDIAN);
            // If we are to write into this array we need to expand it first
            bee = Arrays.copyOf(be, be.length + 4*100);
            buf = ByteBuffer.wrap(bee);

            writer.setBuffer(buf);
            writer.writeEvent(ev);
            writer.close();

System.out.println("Main: (before flip) buf limit = " + buf.limit() +
                   ", pos = " + buf.position() +
                   ", cap = " + buf.capacity());
            buf.flip();

System.out.println("Main: (after flip) buf limit = " + buf.limit() +
                   ", pos = " + buf.position() +
                   ", cap = " + buf.capacity());


            iBuf = buf.asIntBuffer();
            while(iBuf.hasRemaining()) {
                System.out.println("0x" + Integer.toHexString(iBuf.get()));
            }


        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }



    /** Append event to buffer of events */
    public static void main3(String args[]) {

        // xml dictionary
        String dictionary =
                "<xmlDict>\n" +
                        "  <xmldumpDictEntry name=\"bank of ints\"   tag=\"1\"   num=\"1\"/>\n" +
                "</xmlDict>";


        try {
            byte[] be  = ByteDataTransformer.toBytes(data1, ByteOrder.BIG_ENDIAN);
            // If we are to write into this array we need to expand it first
            byte[] bee = Arrays.copyOf(be, be.length + 4*100);
            ByteBuffer buf = ByteBuffer.wrap(bee);
//            public EventWriter(ByteBuffer buf, int blockSizeMax, int blockCountMax,
//                               String xmlDictionary, BitSet bitInfo,
//                               boolean append) throws EvioException {

            EventWriter writer = new EventWriter(buf, 1000000, 3, null, null, true);

            int eventsWritten = writer.getEventsWritten();

            System.out.println("Main: read buffer for writing, already contains " + eventsWritten + " events\n");


            // data
            int[] intData = new int[]  {4,5,6};

            // event - bank of banks
            EventBuilder eventBuilder = new EventBuilder(1, DataType.INT32, 1);
            EvioEvent ev = eventBuilder.getEvent();
            ev.appendIntData(intData);
System.out.println("Main: writer blockNum = " + writer.getBlockNumber());

            writer.writeEvent(ev);
            writer.close();

System.out.println("Main: (before flip) buf limit = " + buf.limit() +
                   ", pos = " + buf.position() +
                   ", cap = " + buf.capacity());
            buf.flip();

System.out.println("Main: (after flip) buf limit = " + buf.limit() +
                   ", pos = " + buf.position() +
                   ", cap = " + buf.capacity());

            EvioReader reader = new EvioReader(buf);
            long evCount = reader.getEventCount();

            String dict = reader.getDictionaryXML();
            if (dict == null) {
                System.out.println("Main: ain't got no dictionary!");
            }
            else {
                System.out.println("Main: dict-> \n" + dict);
            }

System.out.println("Main: read file, got " + evCount + " number of events\n");

            EvioEvent event;
            while ( (event = reader.parseNextEvent()) != null) {
               System.out.println("Main: event = " + event.toXML());
                System.out.println("\nMain: event count = " + reader.getEventCount() + "\n");
            }

        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }


    /** Testing for padding bug in which zero values overwrite beginning of data array
     * instead of end for CHAR8 & UCHAR8. Found and fixed. */
    public static void main4(String args[]) {

        try {

            // Evio event = bank of double data
            EventBuilder eb = new EventBuilder(1, DataType.CHAR8, 1);
            EvioEvent ev = eb.getEvent();
            byte[] da = new byte[] {1,2,3,4,5,6,7};
            ev.appendByteData(da);

            ByteBuffer buffer = ByteBuffer.allocate(100);
            EventWriter evWriter = new EventWriter(buffer, 550000, 200, null, null);
            evWriter.writeEvent(ev);
            evWriter.close();

            buffer.flip();

            EvioReader reader = new EvioReader(buffer);
            long evCount = reader.getEventCount();
            System.out.println("Read file, got " + evCount + " number of events\n");

            EvioEvent event;
            while ( (event = reader.parseNextEvent()) != null) {
                System.out.println("Event = " + event.toXML());
                System.out.println("\nevent count = " + reader.getEventCount() + "\n");
            }

        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }



}
