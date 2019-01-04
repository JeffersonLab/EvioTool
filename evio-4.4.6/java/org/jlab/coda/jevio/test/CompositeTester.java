package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.util.List;

/**
 * Created by IntelliJ IDEA.
 * User: timmer
 * Date: 4/12/11
 * Time: 10:21 AM
 * To change this template use File | Settings | File Templates.
 */
public class CompositeTester {



    /** For testing only */
    public static void main0(String args[]) {


        int[] bank = new int[24];

        /***********************/
        /* bank of tagsegments */
        /***********************/
        bank[0] = 23;                       // bank length
        bank[1] = 6 << 16 | 0xF << 8 | 3;   // tag = 6, bank contains composite type, num = 3


        // N(I,D,F,2S,8a)
        // first part of composite type (for format) = tagseg (tag & type ignored, len used)
        bank[2]  = 5 << 20 | 0x3 << 16 | 4; // tag = 5, seg has char data, len = 4
        // ASCII chars values in latest evio string (array) format, N(I,D,F,2S,8a) with N=2
        bank[3]  = 0x4E << 24 | 0x28 << 16 | 0x49 << 8 | 0x2C;    // N ( I ,
        bank[4]  = 0x44 << 24 | 0x2C << 16 | 0x46 << 8 | 0x2C;    // D , F ,
        bank[5]  = 0x32 << 24 | 0x53 << 16 | 0x2C << 8 | 0x38 ;   // 2 S , 8
        bank[6]  = 0x61 << 24 | 0x29 << 16 | 0x00 << 8 | 0x04 ;   // a ) \0 \4

        // second part of composite type (for data) = bank (tag, num, type ignored, len used)
        bank[7]  = 16;
        bank[8]  = 6 << 16 | 0xF << 8 | 1;
        bank[9]  = 0x2; // N
        bank[10] = 0x00001111; // I
        // Double
        double d = Math.PI * (-1.e-100);
        long  dl = Double.doubleToLongBits(d);
        bank[11] = (int) (dl >>> 32);    // higher 32 bits
        bank[12] = (int)  dl;            // lower 32 bits
        // Float
        float f = (float)(Math.PI*(-1.e-24));
        int  fi = Float.floatToIntBits(f);
        bank[13] = fi;

        bank[14] = 0x11223344; // 2S

        bank[15]  = 0x48 << 24 | 0x49 << 16 | 0x00 << 8 | 0x48;    // H  I \0  H
        bank[16]  = 0x4F << 24 | 0x00 << 16 | 0x04 << 8 | 0x04;    // 0 \ 0 \4 \4

        // duplicate data
        for (int i=0; i < 7; i++) {
            bank[17+i] = bank[10+i];
        }


        // all composite including headers
        int[] allData = new int[22];
        for (int i=0; i < 22; i++) {
            allData[i] = bank[i+2];
        }



        // analyze format string
        String format = "N(I,D,F,2S,8a)";

        try {
            // change int array into byte array
            byte[] byteArray = ByteDataTransformer.toBytes(allData, ByteOrder.BIG_ENDIAN);

            // wrap bytes in ByteBuffer for ease of printing later
            ByteBuffer buf = ByteBuffer.wrap(byteArray);
            buf.order(ByteOrder.BIG_ENDIAN);

            // swap
System.out.println("Call CompositeData.swapAll()");
            CompositeData.swapAll(byteArray, 0, null, 0, allData.length, ByteOrder.BIG_ENDIAN);

            // print swapped data
            System.out.println("SWAPPED DATA:");
            IntBuffer iBuf = buf.asIntBuffer();
            for (int i=0; i < allData.length; i++) {
                System.out.println("     0x"+Integer.toHexString(iBuf.get(i)));
            }
            System.out.println();



//            // Create composite object
//System.out.println("Call CompositeData()");
//            CompositeData cData2 = new CompositeData(byteArray, ByteOrder.LITTLE_ENDIAN);

//            // print swapped data
//            System.out.println("After making CompositeData object DATA:");
//            buf = ByteBuffer.wrap(byteArray);
//            buf.order(ByteOrder.BIG_ENDIAN);
//            iBuf = buf.asIntBuffer();
//            for (int i=0; i < allData.length; i++) {
//                System.out.println("     0x"+Integer.toHexString(iBuf.get(i)));
//            }
//            System.out.println();


            // swap again
System.out.println("Call CompositeData.swapAll()");
            CompositeData.swapAll(byteArray, 0, null, 0, allData.length, ByteOrder.LITTLE_ENDIAN);

            // print double swapped data
            System.out.println("DOUBLE SWAPPED DATA:");
            for (int i=0; i < allData.length; i++) {
                System.out.println("     0x"+Integer.toHexString(iBuf.get(i)));
            }
            System.out.println();

            // Create composite object
            CompositeData cData = new CompositeData(byteArray, ByteOrder.BIG_ENDIAN);

            // print out general data
            printCompositeDataObject(cData);

            // use alternative method to print out
            cData.index(0);
            System.out.println("\nInt    = 0x" + Integer.toHexString(cData.getInt()));
            System.out.println("Double = " + cData.getDouble());
            System.out.println("Float  = " + cData.getFloat());
            System.out.println("Short  = 0x" + Integer.toHexString(cData.getShort()));
            System.out.println("Short  = 0x" + Integer.toHexString(cData.getShort()));
            String[] strs = cData.getStrings();
            for (String s : strs) System.out.println("String = " + s);

            // use toString() method to print out
//            System.out.println("\ntoXmlString =\n" + cData.toXMLString("     ", true));

        }
        catch (EvioException e) {
            e.printStackTrace();
        }

    }


    /**
     * Simple example of providing a format string and some data
     * in order to create a CompositeData object.
     */
    public static void main1(String args[]) {

        // Create a CompositeData object ...

        // Format to write an int and a string
        // To get the right format code for the string, use a helper method
        String myString = "string";
        String stringFormat = CompositeData.stringsToFormat(new String[] {myString});

        // Put the 2 formats together
        String format = "I," + stringFormat;

        System.out.println("format = " + format);

        // Now create some data
        CompositeData.Data myData = new CompositeData.Data();
        myData.addInt(2);
        myData.addString(myString);

        // Create CompositeData object
        CompositeData cData = null;
        try {
            cData = new CompositeData(format, 1, myData, 0 ,0);
        }
        catch (EvioException e) {
            e.printStackTrace();
            System.exit(-1);
        }

        // Print it out
        printCompositeDataObject(cData);
    }


    /**
     * More complicated example of providing a format string and some data
     * in order to create a CompositeData object.
     */
    public static void main2(String args[]) {

        // Create a CompositeData object ...

        // Format to write a N shorts, 1 float, 1 double a total of N times
        String format = "N(NS,F,D)";

        System.out.println("format = " + format);

        // Now create some data
        CompositeData.Data myData = new CompositeData.Data();
        myData.addN(2);
        myData.addN(3);
        myData.addShort(new short[] {1,2,3}); // use array for convenience
        myData.addFloat(1.0F);
        myData.addDouble(Math.PI);
        myData.addN(1);
        myData.addShort((short)4); // use array for convenience
        myData.addFloat(2.0F);
        myData.addDouble(2.*Math.PI);

        // Note: N's do not needed to be added in sync with the other data.
        // In other words, one could do:
        // myData.addN(2);
        // myData.addN(3);
        // myData.addN(1);
        // myData.addShort(new short[] {1,2,3}); ... etc.
        //
        // or
        //
        // myData.addN(new int[] {2,3,1});
        // myData.addShort(new short[] {1,2,3}); ... etc.
        //


        // Create CompositeData object
        CompositeData cData = null;
        try {
            cData = new CompositeData(format, 1, myData, 0 ,0);
        }
        catch (EvioException e) {
            e.printStackTrace();
            System.exit(-1);
        }

        // Print it out
        printCompositeDataObject(cData);

        try {
            EvioEvent ev = new EvioEvent(0, DataType.COMPOSITE, 0);
            ev.appendCompositeData(new CompositeData[] {cData});

            // Write it to this file
            String fileName  = "./composite.dat";

            EventWriter writer = new EventWriter(fileName);
            writer.writeEvent(ev);
            writer.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }


    }


    /**
     * More complicated example of providing a format string and some data
     * in order to create a CompositeData object.
     */
    public static void main(String args[]) {

        // Create a CompositeData object ...

        // Format to write a N shorts, 1 float, 1 double a total of N times
        String format = "N(I,F)";

System.out.println("format = " + format);

        // Now create some data
        CompositeData.Data myData1 = new CompositeData.Data();
        myData1.addN(1);
        myData1.addInt(1); // use array for convenience
        myData1.addFloat(1.0F);

        // Now create some data
        CompositeData.Data myData2 = new CompositeData.Data();
        myData2.addN(1);
        myData2.addInt(2); // use array for convenience
        myData2.addFloat(2.0F);

        // Now create some data
        CompositeData.Data myData3 = new CompositeData.Data();
        myData3.addN(1);
        myData3.addInt(3); // use array for convenience
        myData3.addFloat(3.0F);


System.out.println("Create composite data objects");

        // Create CompositeData object
        CompositeData[] cData = new CompositeData[3];
        try {
            cData[0] = new CompositeData(format, 1, myData1, 1 ,1);
            cData[1] = new CompositeData(format, 2, myData2, 2 ,2);
            cData[2] = new CompositeData(format, 3, myData3, 3 ,3);
        }
        catch (EvioException e) {
            e.printStackTrace();
            System.exit(-1);
        }

        // Print it out
        System.out.println("Print composite data objects");
        printCompositeDataObject(cData[0]);
        printCompositeDataObject(cData[1]);
        printCompositeDataObject(cData[2]);

        try {
            EvioEvent ev = new EvioEvent(0, DataType.COMPOSITE, 0);
            ev.appendCompositeData(cData);

            // Write it to this file
            String fileName  = "./composite.dat";

System.out.println("WRITE FILE:");
            EventWriter writer = new EventWriter(fileName);
            writer.writeEvent(ev);
            writer.close();

            // Read it from file
System.out.println("READ FILE & PRINT CONTENTS:");
            EvioReader reader = new EvioReader(fileName);
            EvioEvent evR = reader.parseNextEvent();
            BaseStructureHeader h = evR.getHeader();
            System.out.println("event: tag = " + h.getTag() +
                                ", type = " + h.getDataTypeName() + ", len = " + h.getLength());
            if (evR != null) {
                CompositeData[] cDataR = evR.getCompositeData();
                for (CompositeData cd : cDataR) {
                    printCompositeDataObject(cd);
                }
            }

        }
        catch (Exception e) {
            e.printStackTrace();
        }


    }




    /** For testing only */
    public static void main4(String args[]) {

        //create an event writer to write out the test events.
        String fileName  = "/daqfs/home/timmer/coda/evio-4.0.sergey/Linux-x86_64/bin/sample.dat";

        File fileIn = new File(fileName);
        System.out.println("read ev file: " + fileName + " size: " + fileIn.length());

        try {
            EvioReader evioReader = new EvioReader(fileName);
            EvioEvent ev;
            while ( (ev = evioReader.parseNextEvent()) != null) {
                System.out.println("EVENT:\n" + ev.toXML());
            }
        }
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }


    /**
     * Print the data from a CompositeData object in a user-friendly form.
     * @param cData CompositeData object
     */
    public static void printCompositeDataObject(CompositeData cData) {

        // Get lists of data items & their types from composite data object
        List<Object> items = cData.getItems();
        List<DataType> types = cData.getTypes();

        // Use these list to print out data of unknown format
        DataType type;
        int len = items.size();
        for (int i=0; i < len; i++) {
            type =  types.get(i);
            System.out.print(String.format("type = %9s, val = ", type));
            switch (type) {
                case NVALUE:
                case INT32:
                case UINT32:
                case UNKNOWN32:
                    int j = (Integer)items.get(i);
                    System.out.println("0x"+Integer.toHexString(j));
                    break;
                case LONG64:
                case ULONG64:
                    long l = (Long)items.get(i);
                    System.out.println("0x"+Long.toHexString(l));
                    break;
                case SHORT16:
                case USHORT16:
                    short s = (Short)items.get(i);
                    System.out.println("0x"+Integer.toHexString(s));
                    break;
                case CHAR8:
                case UCHAR8:
                    byte b = (Byte)items.get(i);
                    System.out.println("0x"+Integer.toHexString(b));
                    break;
                case FLOAT32:
                    float ff = (Float)items.get(i);
                    System.out.println(""+ff);
                    break;
                case DOUBLE64:
                    double dd = (Double)items.get(i);
                    System.out.println(""+dd);
                    break;
                case CHARSTAR8:
                    String[] strs = (String[])items.get(i);
                    for (String ss : strs) {
                        System.out.print(ss + ", ");
                    }
                    System.out.println();
                    break;
                default:
            }
        }

    }



}
