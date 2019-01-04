package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.net.InetAddress;
import java.nio.*;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * Take test programs out of EvioCompactReader.
 * @author timmer
 * Date: Nov 15, 2012
 */
public class CompactReaderAddSubtractTest {


    static EvioEvent createSingleEvent(int tag) {

        // Top level event
        EvioEvent event = null;

        // data
        int[] intData = new int[2];
        Arrays.fill(intData, tag);

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
        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        return event;
    }


    static EvioEvent createComplexEvent(int tag) {

         // Top level event
         EvioEvent event = null;

         // data
        int[] intData = new int[2];
        Arrays.fill(intData, tag);

        int[] intData2 = new int[2];
        Arrays.fill(intData2, tag*10);

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

                 // bank of banks
                 EvioBank bankBanks2 = new EvioBank(tag+3, DataType.BANK, 4);
                 builder.addChild(event, bankBanks2);

                      // bank of ints
                      EvioBank bankInts2 = new EvioBank(tag+4, DataType.INT32, 5);
                      bankInts2.appendIntData(intData2);
                      builder.addChild(bankBanks2, bankInts2);

         }
         catch (EvioException e) {
             e.printStackTrace();
         }

         return event;
     }



    static ByteBuffer createComplexBuffer() {

        // Create a buffer
        ByteBuffer myBuf = ByteBuffer.allocate(32 * 5 + 24);

        try {
            // Create an event writer to write into "myBuf"
            //EventWriter writer = new EventWriter(myBuf);
            EventWriter writer = new EventWriter(myBuf, 1000, 1, null, null);

            EvioEvent ev1 = createComplexEvent(1);
            EvioEvent ev2 = createSingleEvent(100);

            // Write events to buffer
            writer.writeEvent(ev1);
            writer.writeEvent(ev2);

            // All done writing
            writer.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        // Get ready to read
        myBuf.flip();

        return myBuf;
    }


    static ByteBuffer createBuffer() {

        // Create a buffer
        ByteBuffer myBuf = ByteBuffer.allocate(32 * 5);

        try {
            // Create an event writer to write into "myBuf"
            //EventWriter writer = new EventWriter(myBuf);
            EventWriter writer = new EventWriter(myBuf, 1000, 1, null, null);

            EvioEvent ev1 = createSingleEvent(1);
            EvioEvent ev2 = createSingleEvent(100);

            // Write events to buffer
            writer.writeEvent(ev1);
            writer.writeEvent(ev2);

            // All done writing
            writer.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        // Get ready to read
        myBuf.flip();

        return myBuf;
    }


    /** Test for addStructure() */
    public static void main(String args[]) {

        try {

            boolean useFile = false;
            EvioCompactReader reader;


            if (useFile) {
                reader = new EvioCompactReader("/tmp/removeTest.evio");
            }
            else {
                // ready-to-read buf with 2 events
                ByteBuffer buf = createComplexBuffer();
                reader = new EvioCompactReader(buf);
            }

            System.out.println("Using file = " + reader.isFile());
            System.out.println("# of events = " + reader.getEventCount());

            EvioNode node1 = reader.getScannedEvent(1);
            EvioNode node2 = reader.getScannedEvent(2);
            reader.toFile("/tmp/junk1");

            int i=0;
            System.out.println("1st event all:");
            for (EvioNode n : node1.getAllNodes()) {
                i++;
                System.out.println("node " + i + ": " + n);
            }

            System.out.println("1st event children:");
            i=0;
            ArrayList<EvioNode> kids = node1.getChildNodes();
            if (kids != null) {
                for (EvioNode n : kids) {
                    i++;
                    System.out.println("child node " + i + ": " + n);
                }
            }

            i=0;
            System.out.println("2nd event all:");
            for (EvioNode n : node2.getAllNodes()) {
                i++;
                System.out.println("node " + i + ": " + n);
            }

            System.out.println("\nBlock 1: " + node1.blockNode + "\n");
            System.out.println("\nBlock 2: " + node2.blockNode + "\n");

//            if (!useFile) reader.toFile("/tmp/removeTest.evio");

            System.out.println("node 1 has all-node-count = " + node1.getAllNodes().size());

            // Remove 3rd bank structure here (1st bank of ints)
//System.out.println("removing node = " + node1.getAllNodes().get(2));
//            ByteBuffer newBuf = reader.removeStructure(node1.getAllNodes().get(2));
System.out.println("removing node = " + node1.getAllNodes().get(1));
            ByteBuffer newBuf = reader.removeStructure(node1.getAllNodes().get(1));
//System.out.println("removing node = " + node2.getAllNodes().get(2));
//            ByteBuffer newBuf = reader.removeStructure(node2.getAllNodes().get(2));
//System.out.println("Using file (after node removal) = " + reader.isFile());

            System.out.println("node 1 now has all-node-count = " + node1.getAllNodes().size());

            //System.out.println("\n\n ******************* removing event # 2\n");
            //ByteBuffer newBuf = reader.removeEvent(2);
            //reader.removeStructure(node2);

//            EvioCompactReader reader2 = new EvioCompactReader(newBuf);
//
//            node1 = reader2.getScannedEvent(1);
//            node2 = reader2.getScannedEvent(2);

            System.out.println("REMOVE node 1");
            newBuf = reader.removeEvent(1);
            System.out.println("Re-get scanned events 1 & 2");
            node1 = reader.getScannedEvent(1);
            reader.removeStructure(node1.getChildAt(0));
            node2 = reader.getScannedEvent(2);

            reader.toFile("/tmp/junk2");

            i=0;
            if (node1 == null) {
                System.out.println("event 1 = null");
            }
            else {

                if (node1.isObsolete()) {
                    System.out.println("OBSOLETE event 1");
                }
                else {
                    System.out.println("1st event after:");
                    for (EvioNode n : node1.getAllNodes()) {
                        i++;
                        if (n.isObsolete()) {
                            System.out.println("OBSOLETE node " + i + ": " + n);
                        }
                        else {
                            System.out.println("node " + i + ": " + n);
                        }
                    }
                    System.out.println("1st event children after:");
                    i = 0;
                    kids = node1.getChildNodes();
                    if (kids != null) {
                        for (EvioNode n : kids) {
                            i++;
                            System.out.println("child node: " + i + ": " + n);
                        }
                    }
                }
            }


            i=0;
            if (node2 == null) {
                System.out.println("event 2 = null");
            }
            else {
                if (node2.isObsolete()) {
                    System.out.println("OBSOLETE event 2");
                }
                else {
                    System.out.println("2nd event after:");
                    for (EvioNode n : node2.getAllNodes()) {
                        i++;
                        System.out.println("node " + i + ": " + n);
                    }
                }
            }

            if (node1 != null) System.out.println("\nBlock 1 after: " + node1.blockNode + "\n");
            if (node2 != null) System.out.println("\nBlock 2 after: " + node2.blockNode + "\n");


            System.out.println("\n\n\nReanalyze");
            System.out.println("# of events = " + reader.getEventCount());

            // Reanalyze the buffer and compare
            EvioCompactReader reader2 = new EvioCompactReader(newBuf);

            node1 = reader2.getScannedEvent(1);
            node2 = reader2.getScannedEvent(2);

            i=0;
            if (node1 == null) {
                System.out.println("event 1 is NULL");
            }
            else if (node1.isObsolete()) {
                System.out.println("event 1 is obsolete");
            }
            else {
                System.out.println("1st event all:");
                for (EvioNode n : node1.getAllNodes()) {
                    i++;
                    System.out.println("node " + i + ": " + n);
                }

                System.out.println("1st event children:");
                i = 0;
                kids = node1.getChildNodes();
                if (kids != null) {
                    for (EvioNode n : kids) {
                        i++;
                        System.out.println("child node " + i + ": " + n);
                    }
                }
                System.out.println("\nBlock 1: " + node1.blockNode + "\n");
            }

             i=0;
            if (node2 == null) {
                System.out.println("event 2 is NULL");
            }
            else if (node2.isObsolete()) {
                System.out.println("event 2 is obsolete");
            }
            else {
                System.out.println("2nd event all:");
                for (EvioNode n : node2.getAllNodes()) {
                    i++;
                    System.out.println("node " + i + ": " + n);
                }
                System.out.println("\nBlock 2: " + node2.blockNode + "\n");
            }

            // Write to file for viewing
//            Utilities.bufferToFile("/tmp/result2.evio", newBuf, true, false);

        }
        catch (Exception e) {
            e.printStackTrace();
        }

    }



    /** Test for addStructure() */
    public static void main1(String args[]) {

        try {
            // ready-to-read buf with 2 events
            ByteBuffer buf = createBuffer();

            // Create bank in buffer to be inserted at end of 1st event
            ByteBuffer evBuf = ByteBuffer.allocate(32);

            try {
                // Create an event writer to write into "evBuf"  (3 nested banks)
                EvioEvent ev = createSingleEvent(10);
                ev.write(evBuf);
                // Get ready to read
                evBuf.flip();
            }
            catch (Exception e) {
                e.printStackTrace();
            }

//            // Add bank to end of first event
//            Utilities.printBuffer(buf, 0, 32, "event");

            EvioCompactReader reader = new EvioCompactReader(buf);

            EvioNode node1 = reader.getEvent(1);
            EvioNode node2 = reader.getScannedEvent(2);

            int i=0;
            System.out.println("1st event all:");
            for (EvioNode n : node1.getAllNodes()) {
                i++;
                System.out.println("node " + i + ": " + n);
            }

            System.out.println("1st event children:");
            i=0;
            ArrayList<EvioNode> kids = node1.getChildNodes();
            if (kids != null) {
                for (EvioNode n : kids) {
                    i++;
                    System.out.println("child node " + i + ": " + n);
                }
            }

            i=0;
            System.out.println("2nd event all:");
            for (EvioNode n : node2.getAllNodes()) {
                i++;
                System.out.println("node " + i + ": " + n);
            }

            System.out.println("\nBlock 1: " + node1.blockNode + "\n");
            System.out.println("\nBlock 2: " + node2.blockNode + "\n");


            // Add structure here
            ByteBuffer newBuf = reader.addStructure(1, evBuf);

//            EvioCompactReader reader2 = new EvioCompactReader(newBuf);
//
//            node1 = reader2.getScannedEvent(1);
//            node2 = reader2.getScannedEvent(2);

            i=0;
            System.out.println("1st event after:");
            for (EvioNode n : node1.getAllNodes()) {
                i++;
                System.out.println("node " + i + ": " + n);
            }

            System.out.println("1st event children after:");
            i=0;
            kids = node1.getChildNodes();
            if (kids != null) {
                for (EvioNode n : kids) {
                    i++;
                    System.out.println("child node: " + i + ": " + n);
                }
            }

            i=0;
            System.out.println("2nd event after:");
            for (EvioNode n : node2.getAllNodes()) {
                i++;
                System.out.println("node " + i + ": " + n);
            }

            System.out.println("\nBlock 1 after: " + node1.blockNode + "\n");
            System.out.println("\nBlock 2 after: " + node2.blockNode + "\n");

            // Write to file for viewing
//            Utilities.bufferToFile("/tmp/result.evio", newBuf, true, false);

        }
        catch (Exception e) {
            e.printStackTrace();
        }

    }



}
