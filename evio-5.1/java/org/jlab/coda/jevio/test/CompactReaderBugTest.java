package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.*;
import java.nio.channels.FileChannel;

/**
 * Take test programs out of EvioCompactReader.
 * @author timmer
 * Date: Nov 15, 2012
 */
public class CompactReaderBugTest {


    /**
     1 block header which has 3 events.
     First is bank with 1 char, second has bank with 2 chars,
     and third has bank with 3 chars.
     */
    static int data1[] = {

            0x00000014,
            0x00000001,
            0x00000008,
            0x00000004,
            0x00000000,
            0x00000204,
            0x00000000,
            0xc0da0100,

            0x00000002,
            0x0001c601,
            0x01020304,

            0x00000002,
            0x00028602,
            0x05060708,

            0x00000002,
            0x00034603,
            0x090a0b0c,

            0x00000002,
            0x00040604,
            0x0e0f1011,
    };


    public static void main1(String args[]) {
        byte[] array = new byte[] {(byte)1,(byte)2,(byte)3,(byte)4};

        ByteBuffer bb1 = ByteBuffer.wrap(array);

        System.out.println("Wrapped array: ");
        for (int i=0; i < array.length; i++) {
            System.out.println("array[" + i + "] = " + array[i]);
        }

        ByteBuffer bbDup = bb1.duplicate();
        bbDup.clear();


        System.out.println("\nDuplicate array: ");
        for (int i=0; i < bbDup.remaining(); i++) {
            System.out.println("array[" + i + "] = " + bbDup.get(i));
        }

        bb1.clear();
        ByteBuffer bbSlice = bb1.slice();
        bbSlice.clear();

        System.out.println("\nSlice array: ");
        for (int i=0; i < bbSlice.remaining(); i++) {
            System.out.println("array[" + i + "] = " + bbSlice.get(i));
        }

     //   bbDup.clear();
        bbDup.limit(3).position(1);
        bbSlice = bbDup.slice();
        bbSlice.clear();

        System.out.println("\nSlice of Duplicate array: ");
        for (int i=0; i <  bbSlice.remaining(); i++) {
            System.out.println("array[" + i + "] = " + bbSlice.get(i));
        }



    }




    public static void main(String args[]) {

        String fileName  = "/tmp/testFile.ev";
        System.out.println("Write file " + fileName);
        File file;


        // Write evio file that has extra words in headers
        try {
            byte[] be  = ByteDataTransformer.toBytes(data1, ByteOrder.BIG_ENDIAN);
            ByteBuffer buf = ByteBuffer.wrap(be);
            file = new File(fileName);
            FileOutputStream fileOutputStream = new FileOutputStream(file);
            FileChannel fileChannel = fileOutputStream.getChannel();
            fileChannel.write(buf);
            fileChannel.close();
        }
        catch (Exception e) {
            e.printStackTrace();
            return;
        }

        System.out.println("\nCompactEvioReader file: " + fileName);

        try {
            byte[] data = ByteDataTransformer.toBytes(data1, ByteOrder.BIG_ENDIAN);
            ByteBuffer buf = ByteBuffer.wrap(data);

            EvioCompactReader reader = new EvioCompactReader(buf);
            int evCount = reader.getEventCount();
            for (int i=0; i < evCount; i++) {
                EvioNode node = reader.getEvent(i+1);
System.out.println("\nEvent " + (i+1) + ", tag = " + node.getTag() +
                   ", type = " + node.getDataTypeObj() + ", num = " + node.getNum() +
                   ", pad = " + node.getPad());

                ByteBuffer bb = node.getByteData(false);
System.out.println("Buf: limit = " + bb.limit() + ", cap = " +
                           bb.capacity() + ", pos = " + bb.position());

//                for (int j=0; j <bb.remaining(); j++) {
//                    System.out.println("data["+j+"] = " + bb.get(bb.position()+ j));
//                }



                byte[] array = ByteDataTransformer.toByteArray(bb);
                for (int j=0; j < array.length; j++) {
                    System.out.println("BDT data["+j+"] = " + array[j]);
                }

            }

        }
        catch (Exception e) {
            e.printStackTrace();
        }


        file.delete();
    }


}
