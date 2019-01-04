package org.jlab.coda.jevio.test;


import org.jlab.coda.jevio.*;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.util.Arrays;

/**
 * Test program made to test the method which unpacks a byte array into string data.
 * Also tests how a bank of strings is processed by the EvioReader when data len = 0
 * or len = 1;
 *
 * @author timmer
 * Date: Dec 17, 2015
 */
public class UnpackStringTest {

    /* e, v, i, o, null, 4, 4, 4*/
    static private byte[] good1 = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                              (byte)0,   (byte)4,   (byte)4,   (byte)4};

    /* e, v, i, o, null, H, E, Y, H, O, null, 4*/
    static private byte[] good2 = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                              (byte)0,   (byte)72,  (byte)69,  (byte)89,
                                              (byte)72,  (byte)79,  (byte)0,   (byte)4};

    /* e, v, i, o, null, x, y, z*/
    static private byte[] good3 = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                              (byte)0,   (byte)120, (byte)121, (byte)122};

    /* e, v, i, o, -, x, y, z*/
    static private byte[] bad1 = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                             (byte)45,  (byte)120, (byte)121, (byte)122};

    /* e, v, i, o, 3, 4, 4, 4*/  /* bad format 1 */
    static private byte[] bad2  = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                              (byte)3,   (byte)4,   (byte)4,   (byte)4};

    /* e, v, i, o, 0, 4, 3, 3, 3, 3, 3, 4*/  /* bad format 2 */
    static private byte[] bad3  = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                              (byte)0,   (byte)4,   (byte)3,   (byte)3,
                                              (byte)3,   (byte)3,   (byte)3,   (byte)4};

     /* e, v, i, o, null, H, E, Y, H, O, 3, 4*/  /* bad format 4 */
    static private byte[] bad4  = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                              (byte)0,   (byte)72,  (byte)69,  (byte)89,
                                              (byte)72,  (byte)79,  (byte)3,   (byte)4};

    /* e, v, i, o, null, 4, 3, 4*/  /* bad format 3 */
    static private byte[] bad5 = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                              (byte)0,   (byte)4,   (byte)3,   (byte)4};

    /* whitespace chars - only printing ws chars are 9,10,13*/
    static private byte[] ws = new byte[] {(byte)9,   (byte)10,  (byte)11,  (byte)12,
                                           (byte)13,  (byte)28,  (byte)29,  (byte)30,
                                           (byte)31};

    /* e, v, i, o, ws, ws, ws, ws, ws, ws, ws, ws, null, 4, 4 (test whitespace chars) */
    static private byte[] bad6 = new byte[] {(byte)101, (byte)118, (byte)105, (byte)111,
                                              (byte)9,   (byte)10,  (byte)11,  (byte)12,
                                              (byte)13,  (byte)28,  (byte)29,  (byte)30,
                                              (byte)31,  (byte)0,   (byte)4,   (byte)4};

    /* e, 0, 4*/  /* too small */
    static private byte[] bad7 = new byte[] {(byte)101, (byte)0,   (byte)4};

    /* Full evio file format of bank of strings with 1 string = "a".
    *  This is interpreted properly. */
    static int data1[] = {
        0x0000000b,
        0x00000000,
        0x00000008,
        0x00000001,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,

        0x00000002,
        0x00010302,
        0x61000404,
    };

    /* Full evio file format of bank of strings with all chars = '\4' */
    static int data2[] = {
        0x0000000b,
        0x00000000,
        0x00000008,
        0x00000001,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,

        0x00000002,
        0x00010302,
        0x04040404,
    };

    /* Full evio file format of bank of strings with 2, '\0's and 2, '\4's.
    *  This is interpreted properly as 2 blank strings (each zero length). */
    static int data3[] = {
        0x0000000b,
        0x00000000,
        0x00000008,
        0x00000001,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,

        0x00000002,
        0x00010302,
        0x00000404,
    };




    /** Build event with string data & print out to help construct a buffer we can
     *  experiment with. */
    public static void mainMakeEvioBuffer(String args[]) {

        // Buffer to fill
        ByteBuffer buffer1 = ByteBuffer.allocate(256);
        CompactEventBuilder builder = null;
        int num=2, tag=1;

        try {
            builder = new CompactEventBuilder(buffer1);

            // create top/event level bank of strings
            builder.openBank(tag, num, DataType.CHARSTAR8);
            builder.addStringData(new String[] {"a"});
            builder.closeAll();

            buffer1 = builder.getBuffer();

            ByteBuffer buffer2 = ByteBuffer.allocate(512);
            EventWriter writer = new EventWriter(buffer2);
            writer.writeEvent(buffer1);

            Utilities.printBuffer(buffer2, 0, 20, "Strings");
        }
        catch (Exception e) {
            e.printStackTrace();
        }

    }


    /** Build event with string data & print out to help construct a buffer we can
     *  experiment with. */
    public static void main2(String args[]) {

        try {
            byte[] b = ByteDataTransformer.toBytes(data2, ByteOrder.BIG_ENDIAN);
            ByteBuffer buf = ByteBuffer.wrap(b);

            EvioReader reader = new EvioReader(buf);
            EvioEvent event = reader.getEvent(1);

            String[] strings = event.getStringData();

            if (strings == null) {
                System.out.println("Strings are NULL");
                return;
            }

            System.out.println("Have " + strings.length + " number of strings in bank:");
            for (String s : strings) {
                System.out.println("  string = \"" + s + "\"");
            }

        }
        catch (Exception e) {
            e.printStackTrace();
        }

    }


    /** Test unpackRawBytes method. */
    public static void main(String args[]) {

// Whitespace test
//        for (byte b : ws) {
//            System.out.println(b+"--" + ((char)b) + "--");
//        }

        // To do this test, various methods used below need to be made public temporarily
        // and uncommented below.

        try {
            EvioBank bank = new EvioBank(1, DataType.CHARSTAR8, 2);
            bank.setRawBytes(bad7);
           // bank.unpackRawBytesToStrings();

            //String[] strings = new String[] {"hey", "ho"};
            //bank.appendStringData(strings);

            String[] sa = bank.getStringData();
            int i = 1;
            if (sa != null) {
                System.out.println("Decoded strings:");
                for (String s : sa) {
                    System.out.print("string " + (i++) + ": ");
                    System.out.println(s);
                }
            }
            else {
                System.out.println("No decoded strings");
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    /** Compare performance of unpackRawBytes method between old and new versions. */
    public static void main1(String args[]) {

        byte[] b = new byte[400];
        Arrays.fill(b, 0, 396, (byte)101);
        b[396] = (byte)0;
        b[397] = (byte)4;
        b[398] = (byte)4;
        b[399] = (byte)4;

        EvioBank bank = new EvioBank(1, DataType.CHARSTAR8, 2);
        bank.setRawBytes(b);

        long t2, t1 = System.currentTimeMillis();

        for (int i=0; i < 2000000; i++) {
            // make public first, then uncomment next line
            //bank.unpackRawBytesToStrings();
        }

        t2 = System.currentTimeMillis();

        System.out.println("Time = " + (t2 - t1) + " millisec");

    }



}
