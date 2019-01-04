package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.util.Arrays;
import java.util.LinkedList;

/**
 * Created by IntelliJ IDEA.
 * User: timmer
 * Date: 4/12/11
 * Time: 10:21 AM
 * To change this template use File | Settings | File Templates.
 */
public class SwapTest {

    private static void printDoubleBuffer(ByteBuffer byteBuffer) {
        byteBuffer.flip();
        System.out.println();
        for (int i=0; i < byteBuffer.limit()/8 ; i++) {
            System.out.print(byteBuffer.getDouble() + " ");
            if ((i+1)%8 == 0) System.out.println();
        }
        System.out.println();
    }

    private static void printIntBuffer(ByteBuffer byteBuffer) {
        byteBuffer.flip();
        System.out.println();
        for (int i=0; i < byteBuffer.limit()/4 ; i++) {
            System.out.print(byteBuffer.getInt() + " ");
            if ((i+1)%16 == 0) System.out.println();
        }
        System.out.println();
    }


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


    // data
    static      byte[]   byteData   = new byte[]   {1,2,3};
    static      short[]  shortData  = new short[]  {1,2,3};
    static      int[]    intData    = new int[]    {1,2,3};
    static      long[]   longData   = new long[]   {1,2,3};
    static      float[]  floatData  = new float[]  {1,2,3};
    static      double[] doubleData = new double[] {1.,2.,3.};
    static      String[] stringData = new String[] {"123", "456", "789"};

    static ByteBuffer firstBlockHeader = ByteBuffer.allocate(32);
    static ByteBuffer emptyLastHeader  = ByteBuffer.allocate(32);

    static {
        emptyLastHeader.putInt(0,  8);
        emptyLastHeader.putInt(4,  2);
        emptyLastHeader.putInt(8,  8);
        emptyLastHeader.putInt(12, 0);
        emptyLastHeader.putInt(16, 0);
        emptyLastHeader.putInt(20, 0x204);
        emptyLastHeader.putInt(24, 0);
        emptyLastHeader.putInt(28, 0xc0da0100);
    }


    static void setFirstBlockHeader(int words, int count) {
        firstBlockHeader.putInt(0, words + 8);
        firstBlockHeader.putInt(4,  1);
        firstBlockHeader.putInt(8,  8);
        firstBlockHeader.putInt(12, count);
        firstBlockHeader.putInt(16, 0);
        firstBlockHeader.putInt(20, 4);
        firstBlockHeader.putInt(24, 0);
        firstBlockHeader.putInt(28, 0xc0da0100);
    }


    static EvioEvent createSingleEvent(int tag) {

        // Top level event
        EvioEvent event = null;


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
                EvioBank bankBanks2 = new EvioBank(tag+3, DataType.SEGMENT, 4);
                builder.addChild(event, bankBanks2);

                    // segment of shorts
                    EvioSegment bankShorts = new EvioSegment(tag+4, DataType.SHORT16);
                    bankShorts.appendShortData(shortData);
                    builder.addChild(bankBanks2, bankShorts);

                    // segment of segments
                    EvioSegment bankBanks3 = new EvioSegment(tag+5, DataType.SEGMENT);
                    builder.addChild(bankBanks2, bankBanks3);

                        // segment of doubles
                        EvioSegment bankDoubles = new EvioSegment(tag+6, DataType.DOUBLE64);
                        bankDoubles.appendDoubleData(doubleData);
                        builder.addChild(bankBanks3, bankDoubles);

                // Bank of tag segs
                EvioBank bankBanks4 = new EvioBank(tag+7, DataType.TAGSEGMENT, 8);
                builder.addChild(event, bankBanks4);

                    // tag segment of bytes
                    EvioTagSegment bankBytes = new EvioTagSegment(tag+8, DataType.CHAR8);
                    bankBytes.appendByteData(byteData);
                    builder.addChild(bankBanks4, bankBytes);


        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        return event;

    }

    static EvioEvent createSimpleEvent(int tag) {

        // Top level event
        EvioEvent event = null;

        try {
            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(tag, DataType.BANK, 1);
            event = builder.getEvent();

//            // bank of bytes
//            EvioBank bankBytes = new EvioBank(tag+1, DataType.UCHAR8, 2);
//            bankBytes.appendByteData(byteData);
//            builder.addChild(event, bankBytes);
//
//            // bank of shorts
//            EvioBank bankShorts = new EvioBank(tag+2, DataType.USHORT16, 3);
//            bankShorts.appendShortData(shortData);
//            builder.addChild(event, bankShorts);
//
//            // bank of ints
//            EvioBank bankInts = new EvioBank(tag+3, DataType.INT32, 4);
//            bankInts.appendIntData(intData);
//            builder.addChild(event, bankInts);
//
//            // bank of longs
//            EvioBank bankLongs = new EvioBank(tag+4, DataType.LONG64, 5);
//            bankLongs.appendLongData(longData);
//            builder.addChild(event, bankLongs);
//
//            // bank of floats
//            EvioBank bankFloats = new EvioBank(tag+5, DataType.FLOAT32, 6);
//            bankFloats.appendFloatData(floatData);
//            builder.addChild(event, bankFloats);

            // bank of doubles
            EvioBank bankDoubles = new EvioBank(tag+6, DataType.DOUBLE64, 7);
            bankDoubles.appendDoubleData(doubleData);
            builder.addChild(event, bankDoubles);

//            // bank of string array
//            EvioBank bankStrings = new EvioBank(tag+7, DataType.CHARSTAR8, 8);
//            for (int i=0; i < stringData.length; i++) {
//                bankStrings.appendStringData(stringData[i]);
//            }
//            builder.addChild(event, bankStrings);

            // bank of composite data
            CompositeData.Data cData = new CompositeData.Data();
            cData.addShort((short) 1);
            cData.addInt(1);
            cData.addLong(1L);
            cData.addFloat(1);
            cData.addDouble(1.);

            cData.addShort((short) 2);
            cData.addInt(2);
            cData.addLong(2L);
            cData.addFloat(2);
            cData.addDouble(2.);

            CompositeData cd = new CompositeData("S,I,L,F,D", 1, cData, 2, 3);
            System.out.println("CD:\n" +  cd.toString());

            EvioBank bankComposite = new EvioBank(tag+8, DataType.COMPOSITE, 9);
            bankComposite.appendCompositeData(new CompositeData[]{cd});
            builder.addChild(event, bankComposite);

        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        return event;
    }


    /** For testing only */
    public static void main(String args[]) {

        try {
            //vioEvent bank = createSimpleEvent(1);
            EvioEvent bank = createSingleEvent(1);
            int byteSize = bank.getTotalBytes();

            ByteBuffer bb1 = ByteBuffer.allocate(2*byteSize + 2*(32));
            ByteBuffer bb2 = ByteBuffer.allocate(2*byteSize + 2*(32));

            // Write first block header
            setFirstBlockHeader(2 * byteSize / 4, 2);
            bb1.put(firstBlockHeader);
            firstBlockHeader.position(0);

            // Write event
            bank.write(bb1);
            bank.write(bb1);

            // Write last block header
            bb1.put(emptyLastHeader);
            emptyLastHeader.position(0);

            // Get ready to read buffer
            bb1.flip();
            bb2.put(bb1);
            bb1.flip();
            bb2.flip();

//            printBuffer(bb1, 1000);

//            ByteBuffer srcBuffer  = bb;
//            ByteBuffer destBuffer = ByteBuffer.allocate(bb.capacity());

            System.out.println("XML:\n" + bank.toXML());

            LinkedList<EvioNode> list = new LinkedList<EvioNode>();

            ByteDataTransformer.swapEvent(bb1, bb2, 32, 32);
            System.out.println("\n*************\n");
//            bb.order(ByteOrder.BIG_ENDIAN);
            ByteDataTransformer.swapEvent(bb2, bb2, 32, 32, list);

            System.out.println("Node list size = " + list.size());
//
//            for (int i=0; i < list.size(); i++) {
//                DataType typ = DataType.getDataType(list.get(i).getType());
//                System.out.println("node " + i + ": " + typ + " containing " + list.get(i).getDataTypeObj() + ", pos = " +
//                list.get(i).getPosition());
//            }

            // Change byte buffer back into an event
            EvioReader reader = new EvioReader(bb2);
            EvioEvent ev = reader.parseEvent(1);


            System.out.println("\n\n reconstituted XML:\n" + ev.toXML());
//            printBuffer(bb1, 1000);

            IntBuffer ibuf1 = bb1.asIntBuffer();
            IntBuffer ibuf2 = bb2.asIntBuffer();
            int lenInInts = ibuf1.limit() < ibuf1.capacity() ? ibuf1.limit() : ibuf1.capacity();
            System.out.println("ibuf1 limit = " + ibuf1.limit() + ", cap = " + ibuf1.capacity());
            System.out.println("ibuf2 limit = " + ibuf2.limit() + ", cap = " + ibuf2.capacity());
            System.out.println("bb1           bb2\n---------------------------");
            for (int i=0; i < lenInInts; i++) {
                if (ibuf1.get(i) != ibuf2.get(i)) {
                    System.out.println("index " + i + ": 0x" + Integer.toHexString(ibuf1.get(i)) +
                                               " swapped to 0x" +Integer.toHexString(ibuf1.get(i)));
                }
//                System.out.println("0x" + Integer.toHexString(ibuf1.get(i)) +
//                                           "    0x" + Integer.toHexString(ibuf1.get(i)));
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }


    }



}
