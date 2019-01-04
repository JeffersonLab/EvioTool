package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import javax.swing.tree.DefaultTreeModel;
import java.io.File;
import java.io.IOException;
import java.nio.*;
import java.util.Arrays;
import java.util.List;

/**
 * Created by IntelliJ IDEA.
 * User: timmer
 * Date: 4/12/11
 * Time: 10:21 AM
 * To change this template use File | Settings | File Templates.
 */
public class Tester {

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

    public static long getUINT32Value(ByteBuffer buf, int index)
    {
        if (buf != null) {
            IntBuffer ibuf = buf.asIntBuffer();

        	if (index >= 0 && index < ibuf.limit()) {
                System.out.println(" Long = " + ((long) buf.asIntBuffer().get(index)) );
                return ( 0xFFFFFFFFL &  (long) buf.asIntBuffer().get(index) );
        	}
        }
		return -999L;
    }


    public static long try1(ByteBuffer buf, int index) {
        if (buf != null) {
            if (index >= 0 && index < buf.asIntBuffer().limit()) {
                System.out.println(" long = " + ((long) buf.getInt(index*4)) );
                return ( 0xFFFFFFFFL &  (long) buf.getInt(index*4) );
            }
        }
        return -999L;
    }


    // Test to see how hex is interpreted
    public static void main(String args[]) {
        try {
            boolean badEntry = false;
            String tagStr = "0xffd1";
            int tag = Integer.decode(tagStr);
            if (tag < 0) badEntry = true;
System.out.println("Tag, int = " + tag + ", str = " + tagStr + ", bad entry = " + badEntry);

        } catch (NumberFormatException e) {
            e.printStackTrace();
        }
    }

    // Test to see when code chokes adding data to a bank
    public static void main11(String args[]) {
        try {
            int[]  a = new int[] {1, 2, 0xffffffff, 0x88888888};
            byte[] b = ByteDataTransformer.toBytes(a,ByteOrder.BIG_ENDIAN);
            ByteBuffer buf = ByteBuffer.wrap(b);

            for (int i=0; i < 4; i++) {
                System.out.println("i = " + i + " -> 0x" + Long.toHexString(getUINT32Value(buf, i)));
                System.out.println("i = " + i + " -> 0x" + Long.toHexString(try1(buf, i)));

                System.out.println("");
            }

        }
        catch (EvioException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }

    }

    // Test to see when code chokes adding data to a bank
    public static void main22(String args[]) {
        byte[] a = new byte[16];
        Arrays.fill(a, 0, 4,  (byte)1);
        Arrays.fill(a, 4, 8,  (byte)2);
        Arrays.fill(a, 8, 16, (byte)3);

        ByteBuffer buf = ByteBuffer.wrap(a);
        buf.position(4).limit(8);
        byte[] aa = buf.array();
        System.out.println("aa array has len = " + aa.length);

        ByteBuffer slBuf = buf.slice();
        System.out.println("slBuf array has pos = " + slBuf.position() + ", limit = " + slBuf.limit() +
        ", cap = " + slBuf.capacity());
        byte[] slaa = slBuf.array();
        System.out.println("slaa array has len = " + slaa.length);
        for (byte b : slaa) {
            System.out.println(" " + b);
        }



    }

   // Test to see when code chokes adding data to a bank
    public static void main66(String args[]) {
        try {
            EvioEvent bank = new EvioEvent(1, DataType.BANK,  1);
            EvioBank ibank = new EvioBank (2, DataType.INT32, 2);
            EvioBank bbank = new EvioBank (3, DataType.INT32, 3);
            EvioBank lbank = new EvioBank (4, DataType.LONG64, 4);
            //int[] data     = new int[2500000];
           // int[] data3    = new int[2147483646];
            int[] data4    = new int[1];
            int[] data2    = new int[1];
            long[] data5    = new long[268435456];
            // Keep adding to event
            EventBuilder builder = new EventBuilder(bank);
            builder.appendLongData(lbank, data5);
            //builder.appendIntData(bbank, data3);
            //builder.appendIntData(bbank, data4);
            //ibank.appendIntData();
            //ibank.setIntData(data2);
            for (int i=0; i < 2000; i++) {
                System.out.print(i + " ");
                    builder.addChild(bank, ibank);
            }
           // builder.appendIntData(ibank, data);
            //ibank.appendIntData(data);

        }
        catch (EvioException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }


    }

    public static void main55(String args[]) {
        byte[] data = new byte[] {(byte)1, (byte)2};
        int off=0;
        // treat the high bit improperly (shift first, then mask, high byte disappears)
        short s1 = (short) ((0xff & data[  off]) | (0xff & data[1+off] << 8));
        //proper method
        short s2 = (short) ((0xff & data[  off]) | (0xff & data[1+off]) << 8);
        String i1 = Integer.toHexString(ByteDataTransformer.shortBitsToInt(s1));
        String i2 = Integer.toHexString(ByteDataTransformer.shortBitsToInt(s2));
        System.out.println("littleE s1 = 0x"+ i1 + ", s2 = 0x" + i2);
    }

    public static void main33(String args[]) {
        EvioEvent ev   = new EvioEvent(1, DataType.INT32,  1);
        EvioBank bank  = new EvioBank (2, DataType.BANK,  2);
        EvioBank ibank = new EvioBank (3, DataType.INT32, 3);
        int[] data     = new int[] {1,2,3,4,5};

        // Build a bigger event tree
//        EventBuilder builder = new EventBuilder(ev);
//        try {
//            builder.addChild(ev, bank);
//            builder.addChild(bank, ibank);
//        }
//        catch (EvioException e) {}
//        ibank.setIntData(data);

        try {
            ev.setDictionaryXML("blah blah blah");
            ev.appendIntData(data);
        }
        catch (EvioException e) {}
        System.out.println("Now create the clone ...");

        EvioEvent evClone = (EvioEvent)ev.clone();
//        EvioBank  bkClone = (EvioBank)bank.clone();

//        DefaultTreeModel treeModel = ev.getTreeModel();
//        System.out.println("Tree model object for original event:\n" + treeModel);
//        System.out.println("Tree for original event:\n" + treeModel.toString());

        int[] data1 = evClone.getIntData();
//        int[] data2 = bkClone.getIntData();

        if (data1 != null) {
            for (int i : data1) {
                System.out.println("event i = " + i);
            }
        }
        else {
            System.out.println("event int data is NULL !!!");
        }

        String dict = evClone.getDictionaryXML();
        if (dict != null) {
            System.out.println("dictionary = \n" + dict);
        }
        else {
            System.out.println("event dictionary is NULL !!!");
        }


        //int len = ((BankHeader)evClone.getHeader()).getHeaderLength();
        //int len = evClone.getHeader().getHeaderLength();
        int len = ev.getHeader().getHeaderLength();
        System.out.println("header length = " + len);

        System.out.println("Change ev tag from 1 to 66");
        ev.getHeader().setTag(66);
        System.out.println("\nev header = " + ev.getHeader().toString());
        System.out.println("\nclone header = " + evClone.getHeader().toString());



//        if (data2 != null) {
//            for (int i : data2) {
//                System.out.println("bank i = " + i);
//            }
//        }
//        else {
//            System.out.println("bank int data is NULL !!!");
//        }
    }



    /** For testing only */
    public static void main123(String args[]) {

        double freq;
        long t1, t2, deltaT;
        final int LOOPS = 1000000;


        double[] doubleData = new double[128];
        for (int i=0; i < doubleData.length; i++) {
            doubleData[i] = i;
        }

        byte[] rawBytesBE = new byte[0];
        byte[] rawBytesLE = new byte[0];
        try {
            rawBytesBE = ByteDataTransformer.toBytes(doubleData, ByteOrder.BIG_ENDIAN);
            rawBytesLE = ByteDataTransformer.toBytes(doubleData, ByteOrder.LITTLE_ENDIAN);
        }
        catch (EvioException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }
        byte[] rawBytes = rawBytesBE;

        ByteBuffer byteBufferBE = ByteBuffer.wrap(rawBytesBE);
        ByteBuffer byteBufferLE = ByteBuffer.wrap(rawBytesLE).order(ByteOrder.LITTLE_ENDIAN);

        ByteBuffer srcBuffer  = byteBufferBE;
        ByteBuffer destBuffer = ByteBuffer.allocate(srcBuffer.capacity());


        srcBuffer.putDouble(0, 1.1);
        System.out.println("get first double from src = " + srcBuffer.getDouble(0));

        ByteBuffer dup = byteBufferBE.duplicate();
        dup.order(ByteOrder.LITTLE_ENDIAN);
        System.out.println("order of src = " + srcBuffer.order());
        System.out.println("order of dup = " + dup.order());

        System.out.println("get LE first double from dup = " + dup.getDouble(0));
        System.out.println("get first double from src = " + srcBuffer.getDouble(0));
        dup.order(ByteOrder.BIG_ENDIAN);
        System.out.println("get BE first double from dup = " + dup.getDouble(0));


        // using only rawBytes as the starting point ...

        // switch byte order or no backing array

//        // option 1
//        t1 = System.currentTimeMillis();
//        int pos, endPos = 8*doubleData.length;
//
//        for (int j=0; j < LOOPS; j++) {
//            pos = 0;
//            for (; pos < endPos; pos += 8) {
//                byteBufferBE.putLong(pos, Long.reverseBytes(byteBufferBE.getLong(pos)));
//            }
//        }
//        t2 = System.currentTimeMillis();
//        deltaT = t2 - t1; // millisec
//        freq = (double) LOOPS / deltaT * 1000;
//        System.out.printf("loopRate D1: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
//
//        // option 2
//        t1 = System.currentTimeMillis();
//        int position  = srcBuffer.position();
//        int limit = srcBuffer.limit();
//
//        for (int j=0; j < LOOPS; j++) {
//            pos = 0;
//            srcBuffer.limit(endPos).position(pos);
//            LongBuffer db = srcBuffer.asLongBuffer();
//            destBuffer.limit(endPos).position(pos);
//            LongBuffer db2 = destBuffer.asLongBuffer();
//            db2.put(db);
//        }
//        destBuffer.limit(limit).position(position);
//        srcBuffer.limit(limit).position(position);
//
//        t2 = System.currentTimeMillis();
//        deltaT = t2 - t1;
//        freq = (double) LOOPS / deltaT * 1000;
//        System.out.printf("loopRate D2: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
//        //printDoubleBuffer(byteBuffer);

        // keep same endian
    }


    /** For testing only */
    public static void main2(String args[]) {

        double freq;
        long t1, t2, deltaT;
        final int LOOPS = 1000000;


        if (false) {

            double[] doubleData = new double[128];
            for (int i=0; i < doubleData.length; i++) {
                doubleData[i] = i;
            }

            byte[] rawBytesBE = new byte[0];
            byte[] rawBytesLE = new byte[0];
            try {
                rawBytesBE = ByteDataTransformer.toBytes(doubleData, ByteOrder.BIG_ENDIAN);
                rawBytesLE = ByteDataTransformer.toBytes(doubleData, ByteOrder.LITTLE_ENDIAN);
            }
            catch (EvioException e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            }
            byte[] rawBytes = rawBytesBE;

            ByteBuffer byteBufferBE = ByteBuffer.allocate(1024);
            ByteBuffer byteBufferLE = ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);
            ByteBuffer byteBuffer = byteBufferBE;

            // using only rawBytes as the starting point ...

            // switch byte order or no backing array

            // option 1
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBuffer.clear();
                ByteBuffer bbuf = ByteBuffer.wrap(rawBytes).order(ByteOrder.BIG_ENDIAN);
                int doublesize = rawBytes.length / 8;
                for (int i = 0; i < doublesize; i++) {
                    byteBuffer.putDouble(bbuf.getDouble());
                }
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1; // millisec
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate D1: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printDoubleBuffer(byteBuffer);

            // option 2
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBuffer.clear();
                DoubleBuffer db = byteBuffer.asDoubleBuffer();
                DoubleBuffer rawBuf = ByteBuffer.wrap(rawBytes).order(ByteOrder.BIG_ENDIAN).asDoubleBuffer();
                db.put(rawBuf);
                byteBuffer.position(rawBytes.length);
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate D2: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printDoubleBuffer(byteBuffer);

            // keep same endian


            // option 3
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBuffer.clear();
                System.arraycopy(rawBytes, 0, byteBuffer.array(),
                                 byteBuffer.position(), rawBytes.length);
                byteBuffer.position(rawBytes.length);
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate D3: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printDoubleBuffer(byteBuffer);


            // option 4
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBuffer.clear();
                byteBuffer.put(rawBytes);
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate D4: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printDoubleBuffer(byteBuffer);


            // Using doubleData as the starting point ...

            // option 5
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBuffer.clear();
                DoubleBuffer db = byteBuffer.asDoubleBuffer();
                db.put(doubleData, 0, doubleData.length);
                byteBuffer.position(8*doubleData.length);
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate D5: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printDoubleBuffer(byteBuffer);


            // option 6
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBuffer.clear();
                for (double value : doubleData) {
                    byteBuffer.putDouble(value);
                }
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate D6: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printDoubleBuffer(byteBuffer);

            // option 7

            try {
                t1 = System.currentTimeMillis();
                for (int j=0; j < LOOPS; j++) {
                    byteBuffer.clear();
                    byte[] bytes = ByteDataTransformer.toBytes(doubleData, byteBuffer.order());
                    byteBuffer.put(bytes);
                }
                t2 = System.currentTimeMillis();
                deltaT = t2 - t1;
                freq = (double) LOOPS / deltaT * 1000;
                System.out.printf("loopRate D7: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
                //printDoubleBuffer(byteBuffer);
            }
            catch (EvioException e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            }

            // option 8

            try {
                byte[] bArray = new byte[8*doubleData.length];
                t1 = System.currentTimeMillis();
                for (int j=0; j < LOOPS; j++) {
                    byteBuffer.clear();
                    ByteDataTransformer.toBytes(doubleData, byteBuffer.order(), bArray, 0);
                    byteBuffer.put(bArray);
                }
                t2 = System.currentTimeMillis();
                deltaT = t2 - t1;
                freq = (double) LOOPS / deltaT * 1000;
                System.out.printf("loopRate D8: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
                //printDoubleBuffer(byteBuffer);
            }
            catch (EvioException e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            }


        }


        if (false) {

            int[] intData = new int[256];
            for (int i=0; i < intData.length; i++) {
                intData[i] = i;
            }

            byte[] rawBytesIBE = new byte[0];
            byte[] rawBytesILE = new byte[0];
            try {
                rawBytesIBE = ByteDataTransformer.toBytes(intData, ByteOrder.BIG_ENDIAN);
                rawBytesILE = ByteDataTransformer.toBytes(intData, ByteOrder.LITTLE_ENDIAN);
            }
            catch (EvioException e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            }
            byte[] rawBytesI = rawBytesIBE;

            ByteBuffer byteBufferIBE = ByteBuffer.allocate(1024);
            ByteBuffer byteBufferILE = ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);
            ByteBuffer byteBufferI = byteBufferIBE;

            // using only rawBytes as the starting point ...

            // switch byte order or no backing array

            // option 1
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBufferI.clear();
                ByteBuffer bbuf = ByteBuffer.wrap(rawBytesI).order(ByteOrder.BIG_ENDIAN);
                int size = rawBytesI.length / 4;
                for (int i = 0; i < size; i++) {
                    byteBufferI.putInt(bbuf.getInt());
                }
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1; // millisec
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I1: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);

            // option 2
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBufferI.clear();
                IntBuffer db = byteBufferI.asIntBuffer();
                IntBuffer rawBuf = ByteBuffer.wrap(rawBytesI).order(ByteOrder.BIG_ENDIAN).asIntBuffer();
                db.put(rawBuf);
                byteBufferI.position(4*db.position());
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I2: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);

            // keep same endian


            // option 3
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBufferI.clear();
                System.arraycopy(rawBytesI, 0, byteBufferI.array(),
                                 byteBufferI.position(), rawBytesI.length);
                byteBufferI.position(rawBytesI.length);
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I3: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);


            // option 4
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBufferI.clear();
                byteBufferI.put(rawBytesI);
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I4: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);


            // Using doubleData as the starting point ...

            // option 5
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBufferI.clear();
                IntBuffer db = byteBufferI.asIntBuffer();
                db.put(intData, 0, intData.length);
                byteBufferI.position(4*db.position());
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I5: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);


            // option 6
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                byteBufferI.clear();
                for (int value : intData) {
                    byteBufferI.putInt(value);
                }
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I6: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);

            // option 7

            try {
                t1 = System.currentTimeMillis();
                for (int j=0; j < LOOPS; j++) {
                    byteBufferI.clear();
                    byte[] bytes = ByteDataTransformer.toBytes(intData, byteBufferI.order());
                    byteBufferI.put(bytes);
                }
                t2 = System.currentTimeMillis();
                deltaT = t2 - t1;
                freq = (double) LOOPS / deltaT * 1000;
                System.out.printf("loopRate I7: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
                //printIntBuffer(byteBufferI);
            }
            catch (EvioException e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            }

            // option 8  (really slow)
//            t1 = System.currentTimeMillis();
//            for (int j=0; j < LOOPS; j++) {
//                byteBufferI.clear();
//                byte[] bytes = ByteDataTransformer.toBytesStream(intData, byteBufferI.order());
//                byteBufferI.put(bytes);
//            }
//            t2 = System.currentTimeMillis();
//            deltaT = t2 - t1;
//            freq = (double) LOOPS / deltaT * 1000;
//            System.out.printf("loopRate I8: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);
        }


        if (false) {

            try {

                // data
                int[]    ia = new int[100];
                double[] da = new double[50];

                EventBuilder eb = new EventBuilder(1, DataType.BANK, 1);
                // event - bank of banks
                EvioEvent ev = eb.getEvent();


                // bank of ints
                EvioBank ibanks = new EvioBank(3, DataType.INT32, 3);
                t1 = System.currentTimeMillis();
                for (int j=0; j < 2*LOOPS; j++) {
                    ibanks.appendIntData(ia);
                    eb.clearData(ibanks);
                }
                t2 = System.currentTimeMillis();
                deltaT = t2 - t1;
                freq = (double) LOOPS / deltaT * 1000;
                System.out.printf("loopRate I1: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));

                // bank of double
                EvioBank dbanks = new EvioBank(3, DataType.DOUBLE64, 3);
                t1 = System.currentTimeMillis();
                for (int j=0; j < 2*LOOPS; j++) {
                    dbanks.appendDoubleData(da);
                    eb.clearData(dbanks);
                }
                t2 = System.currentTimeMillis();
                deltaT = t2 - t1;
                freq = (double) LOOPS / deltaT * 1000;
                System.out.printf("loopRate I2: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));


                // Evio event with bank of double data
                ev = new EvioEvent(1, DataType.BANK, 1);
                eb.setEvent(ev);
                dbanks = new EvioBank(3, DataType.DOUBLE64, 3);
                dbanks.appendDoubleData(da);
                eb.addChild(ev, dbanks);

                EventWriter evWriter = new EventWriter(ByteBuffer.allocate(32), 550000, 200, null, null);
                evWriter.close();

                ByteBuffer buffer = ByteBuffer.allocate(4000);

                t1 = System.currentTimeMillis();
                for (int j=0; j < 2*LOOPS; j++) {
                    buffer.clear();
                    evWriter.setBuffer(buffer);
                    evWriter.writeEvent(ev);
                    evWriter.close();
                }
                t2 = System.currentTimeMillis();
                deltaT = t2 - t1;
                freq = (double) LOOPS / deltaT * 1000;
                System.out.printf("loopRate I3: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));

                // Evio event with bank of int data
                ev = new EvioEvent(1, DataType.BANK, 1);
                eb.setEvent(ev);
                ibanks = new EvioBank(3, DataType.INT32, 3);
                ibanks.appendIntData(ia);
                eb.addChild(ev, ibanks);


                t1 = System.currentTimeMillis();
                for (int j=0; j < 2*LOOPS; j++) {
                    buffer.clear();
                    evWriter.setBuffer(buffer);
                    evWriter.writeEvent(ev);
                    evWriter.close();
                }
                t2 = System.currentTimeMillis();
                deltaT = t2 - t1;
                freq = (double) LOOPS / deltaT * 1000;
                System.out.printf("loopRate I4: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));

            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }





        }


        // Test for appendIntData bug ... in which an int array with only
        // rawData representation is added to. I'm thinking it overwrites
        // what is there.

        if (false) {

            try {
                // data
                int[]    ia1 = new int[] {1,2,3,4,5};
                int[]    ia2 = new int[] {6,7,8,9,10};
                double[] da = new double[] {1.,2.,3.,4.,5.};

                EventBuilder eb = new EventBuilder(1, DataType.BANK, 1);
                // event - bank of banks
                EvioEvent ev = eb.getEvent();

                // bank of ints
                EvioBank ibanks = new EvioBank(3, DataType.INT32, 3);
                ibanks.appendIntData(ia1);
                eb.addChild(ev, ibanks);
                eb.setAllHeaderLengths();

                // bank of double
//                EvioBank dbanks = new EvioBank(3, DataType.DOUBLE64, 3);
//                dbanks.appendDoubleData(da);
//                eb.addChild(ev, dbanks);

                // write to file
                String file  = "/tmp/out.ev";
                EventWriter eventWriter = new EventWriter(new File(file));
                eventWriter.writeEvent(ev);
                eventWriter.close();

                // read in and print
                System.out.println("Read in from file & print");
                EvioReader fReader = new EvioReader(file);
                EvioEvent evR = fReader.parseNextEvent();
                EvioBank bank = (EvioBank) evR.getChildAt(0);
                int [] iData = bank.getIntData();
                for (int i : iData) {
                    System.out.println(""+i);
                }

                // read in, add, and print
                System.out.println("Read in from file, add more, & print");
                fReader = new EvioReader(file);
                evR = fReader.parseNextEvent();
                bank = (EvioBank) evR.getChildAt(0);
                bank.appendIntData(ia2);
                iData = bank.getIntData();
                for (int i : iData) {
                    System.out.println(""+i);
                }

            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }

        }


        // Test for handling short arrays & padding correctly.

        if (true) {

            try {
                // data
                byte[]   ba1 = new byte[]  {1,2,3,4,5,6,7};
                byte[]   ba2 = new byte[]  {8,9,10,11,12,13,14};
                short[]  sa1 = new short[] {1,2,3,4,5,6,7};
                short[]  sa2 = new short[] {8,9,10,11,12,13,14};
                int[]    ia1 = new int[]   {1,2,3,4,5};
                int[]    ia2 = new int[]   {6,7,8,9,10};
                long[]   la1 = new long[]  {1,2,3,4,5};
                long[]   la2 = new long[]  {6,7,8,9,10};
                float[]  fa1 = new float[] {1,2,3,4,5};
                float[]  fa2 = new float[] {6,7,8,9,10};
                double[] da1 = new double[] {1.,2.,3.,4.,5.};
                double[] da2 = new double[] {6.,7.,8.,9.,10.};

                EventBuilder eb = new EventBuilder(1, DataType.BANK, 1);
                // event - bank of banks
                EvioEvent ev = eb.getEvent();

                // bank of ints
                EvioBank ibanks = new EvioBank(3, DataType.CHARSTAR8, 3);
                ibanks.appendStringData("string 1"); // len + null = 8
                eb.addChild(ev, ibanks);
                eb.setAllHeaderLengths();


                // write to file
                String file  = "/tmp/out.ev";
                EventWriter eventWriter = new EventWriter(new File(file));
                eventWriter.writeEvent(ev);
                eventWriter.close();

                // read in and print
                System.out.println("Read in from file & print");
                EvioReader fReader = new EvioReader(file);
                EvioEvent evR = fReader.parseNextEvent();
                EvioBank bank = (EvioBank) evR.getChildAt(0);
                String [] sData = bank.getStringData();
                for (String i : sData) {
                    System.out.println(""+i);
                }

                // read in, add, and print
                System.out.println("Read in from file, add more, & print");
                fReader = new EvioReader(file);
                evR = fReader.parseNextEvent();
                bank = (EvioBank) evR.getChildAt(0);
                bank.appendStringData("string 2");
                bank.appendStringData("string 3");
                sData = bank.getStringData();
                for (String i : sData) {
                    System.out.println(""+i);
                }

            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }

        }

        // test to see which way is best to convert byte array to int array

        if (false) {

            int[] intData = new int[256];
            for (int i=0; i < intData.length; i++) {
                intData[i] = i;
            }

            double[] doubleData = new double[256];
            for (int i=0; i < doubleData.length; i++) {
                doubleData[i] = i;
            }

            byte[] rawBytesIBE = new byte[0];
            byte[] rawBytesILE = new byte[0];
            try {
                rawBytesIBE = ByteDataTransformer.toBytes(intData, ByteOrder.BIG_ENDIAN);
                rawBytesILE = ByteDataTransformer.toBytes(intData, ByteOrder.LITTLE_ENDIAN);
            }
            catch (EvioException e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            }
            byte[] rawBytesI = rawBytesILE;


            // option 1 (toIntArray)
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                int indx;
                int[] ints = new int[rawBytesI.length / 4];
                for (int i = 0; i < ints.length; i++) {
                    indx = i*4;
                    ints[i] = ByteDataTransformer.toInt(rawBytesI[indx],
                                    rawBytesI[indx+1],
                                    rawBytesI[indx+2],
                                    rawBytesI[indx+3],
                                    ByteOrder.LITTLE_ENDIAN);
                }
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1; // millisec
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I1: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));


            // option 2 (toIntArray)
            t1 = System.currentTimeMillis();
            int[] ii = new int[rawBytesI.length / 4];
            for (int j=0; j < LOOPS; j++) {
                for (int i = 0; i < rawBytesI.length-3; i+=4) {
                    ii[i/4+0] = ByteDataTransformer.toInt(rawBytesI[i],
                                          rawBytesI[i+1],
                                          rawBytesI[i+2],
                                          rawBytesI[i+3],
                                          ByteOrder.LITTLE_ENDIAN);
                }
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1; // millisec
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I2: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));


            // option 3 (getAsIntArray)
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                ByteBuffer byteBuffer = ByteBuffer.wrap(rawBytesI).order(ByteOrder.LITTLE_ENDIAN);
                int intsize = rawBytesI.length / 4;
                int array[] = new int[intsize];
                for (int i = 0; i < intsize; i++) {
                    array[i] = byteBuffer.getInt();
                }
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I3: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);

            // option 3 (getAsIntArray2)
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                ByteBuffer byteBuffer = ByteBuffer.wrap(rawBytesI).order(ByteOrder.LITTLE_ENDIAN);
                IntBuffer ibuf = IntBuffer.wrap(new int[rawBytesI.length/4]);
                byteBuffer.asIntBuffer().put(ibuf);
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate I4: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);


            // option 4 (toDoubleArray)
            t1 = System.currentTimeMillis();
            try {
                for (int j=0; j < LOOPS; j++) {
                    ByteDataTransformer.toDoubleArray(rawBytesI, ByteOrder.BIG_ENDIAN);
                }
            }
            catch (EvioException e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1; // millisec
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate D1: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));


            // option 5 (getAsDoubleArray)
            t1 = System.currentTimeMillis();
            for (int j=0; j < LOOPS; j++) {
                ByteDataTransformer.getAsDoubleArray(rawBytesI, ByteOrder.BIG_ENDIAN);
            }
            t2 = System.currentTimeMillis();
            deltaT = t2 - t1;
            freq = (double) LOOPS / deltaT * 1000;
            System.out.printf("loopRate D2: %2.3g Hz,  time = %2.3g sec\n", freq, (deltaT/1000.));
            //printIntBuffer(byteBufferI);


        }

//        // data
//        short[] shortData1 = new short[] {1,2,3};
//
//        try {
//
//            // event - bank of banks
//            EventBuilder eventBuilder = new EventBuilder(1, DataType.SEGMENT, 1);
//            EvioEvent eventShort = eventBuilder.getEvent();
//
//            // seg of banks
//            EvioSegment segBanks = new EvioSegment(2, DataType.BANK);
//
//            // bank has 3 shorts
//            EvioBank shortBank1 = new EvioBank(3, DataType.SHORT16, 3);
//            shortBank1.appendShortData(shortData1);
//
//            // add short bank to seg of banks
//            eventBuilder.addChild(segBanks, shortBank1);
//
//            // add seg of banks to event
//            eventBuilder.addChild(eventShort, segBanks);
//
//            StructureTransformer st = new StructureTransformer();
//            EvioBank bank = st.transform(segBanks, 55);
//
//            System.out.println("Hey we made it.");
//        }
//        catch (EvioException e) {
//            e.printStackTrace();
//        }
//

    }



}
