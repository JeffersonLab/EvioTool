package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Test program.
 * @author timmer
 * Date: Sep 16, 2015
 */
public class FirstEventTest {

    /** For testing first event defined in constructor. */
    public static void main1(String args[]) {

        boolean useBuffer = false, append = false;

        //create an event writer to write out the test events.
        String fileName = "/tmp/firstEventTestJava.ev";
        File file = new File(fileName);
        ByteBuffer myBuf = ByteBuffer.allocate(800);

        // xml dictionary
        String dictionary =
                "<xmlDict>\n" +
                        "  <dictEntry name=\"regular event\" tag=\"1\"   num=\"1\"/>\n" +
                        "  <dictEntry name=\"FIRST EVENT\"   tag=\"2\"   num=\"2\"/>\n" +
                "</xmlDict>";

        // data
//        int[] intData1 = new int[256];
//        for (int i=0; i < 256; i++) {
//            intData1[i] = i+1;
//        }
        int[] intData1 = new int[] {1,2,3,4,5,6,7};
        int[] intData2 = new int[] {8,9,10,11,12,13,14};

        EvioEvent eventFirst, event;

        try {
            // First event - bank of ints
            EventBuilder EB = new EventBuilder(2, DataType.UINT32, 2);
            eventFirst = EB.getEvent();
            eventFirst.appendIntData(intData1);

            // Regular event - bank of ints
            EventBuilder EB2 = new EventBuilder(1, DataType.UINT32, 1);
            event = EB2.getEvent();
            event.appendIntData(intData2);

            if (!useBuffer) {
                //int split = 312;
                int split = 100;
                System.out.println("FirstEventTest: create EventWriter for file");

                EventWriter ER = new EventWriter(fileName, null, "runType", 1, split,
                                                 1000, 4, 0,
                                                 ByteOrder.BIG_ENDIAN, dictionary,
                                                 null, true, false, eventFirst);
                System.out.println("FirstEventTest: write event #1");
                ER.writeEvent(event);
                System.out.println("FirstEventTest: write event #2");
                ER.writeEvent(event);
                System.out.println("FirstEventTest: write event #3");
                ER.writeEvent(event);

                // All done writing
                ER.close();

                if (append) {
                    split = 0;

                    // Append 1 event onto 1st file
                    System.out.println("FirstEventTest: opening first file");
                    ER = new EventWriter(fileName + ".0", null, "runType", 1, split,
                                         1000, 4, 0,
                                         ByteOrder.BIG_ENDIAN, null,
                                         null, true, true, null);

                    System.out.println("FirstEventTest: append event #1");
                    ER.writeEvent(event);
                    // All done appending
                    ER.close();
                }
            }
            else {
                System.out.println("FirstEventTest: create EventWriter for buffer");
                EventWriter ER = new EventWriter(myBuf, 1000, 3, dictionary, null, 0, 1,
                                                 false, eventFirst);

                System.out.println("FirstEventTest: write event #1");
                ER.writeEvent(event);
                System.out.println("FirstEventTest: write event #2");
                ER.writeEvent(event);
                System.out.println("FirstEventTest: write event #3");
                ER.writeEvent(event);

                // All done writing
                ER.close();

                // Write buf to file
                myBuf.flip();
                System.out.println("FirstEventTest: before writing to file, buf pos = " + myBuf.position() +
                                           ", lim = " + myBuf.limit());

                // Does not mess with buf pos or limit
                Utilities.bufferToFile(fileName, myBuf, true, false);


                if (append) {
                    // Append 1 event onto buffer
                    System.out.println("FirstEventTest: opening buffer");

                    // Give us room to write a little more
                    myBuf.limit(myBuf.capacity());

                    System.out.println("FirstEventTest: before appending to buf, buf pos = " + myBuf.position() +
                                                       ", lim = " + myBuf.limit());
                    ER = new EventWriter(myBuf, 1000, 3, null, null, 0, 1, append, eventFirst);

                    System.out.println("FirstEventTest: append event #1");
                    ER.writeEvent(event);
                    // All done appending
                    ER.close();

                    // Write buf to file
                    myBuf.flip();

                    // Write buf to file
                    Utilities.bufferToFile(fileName + ".app", myBuf, true, false);
                }
            }

        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }
    }




    /** For testing first event set by method in writing files. */
    public static void main(String args[]) {

        //create an event writer to write out the test events.
        String fileName = "/tmp/firstEventTestJava.ev";
        ByteBuffer myBuf = ByteBuffer.allocate(800);

        // xml dictionary
        String dictionary =
                "<xmlDict>\n" +
                        "  <dictEntry name=\"regular event\" tag=\"1\"   num=\"1\"/>\n" +
                        "  <dictEntry name=\"FIRST EVENT\"   tag=\"2\"   num=\"2\"/>\n" +
                "</xmlDict>";

        // data
//        int[] intData1 = new int[256];
//        for (int i=0; i < 256; i++) {
//            intData1[i] = i+1;
//        }
        int[] intData1 = new int[] {1,2,3,4,5,6,7};
        int[] intData2 = new int[] {8,9,10,11,12,13,14};

        EvioEvent eventFirst, event;

        try {
            // First event - bank of ints
            EventBuilder EB = new EventBuilder(2, DataType.UINT32, 2);
            eventFirst = EB.getEvent();
            eventFirst.appendIntData(intData1);

            // Regular event - bank of ints
            EventBuilder EB2 = new EventBuilder(1, DataType.UINT32, 1);
            event = EB2.getEvent();
            event.appendIntData(intData2);


            int split = 100;

            // Create buffer writer
            System.out.println("FirstEventTest: create EventWriter for buffer w/o first event");
            EventWriter bufWriter = new EventWriter(myBuf, 1000, 3, dictionary, null, 0, 1,
                                              false, null);
            // Write first event into buf as a regular event
            bufWriter.writeEvent(eventFirst);
            bufWriter.close();

            // Duplicate just-written-buffer (ready to read)
            ByteBuffer dataBuf = bufWriter.getByteBuffer();

            // Change first event into a node by reading it
            EvioCompactReader cReader = new EvioCompactReader(dataBuf);
            EvioNode firstEventNode = cReader.getEvent(1);

            // Create a writer to a file to do this
            System.out.println("FirstEventTest: create EventWriter for file");
            EventWriter ER = new EventWriter(fileName, null, "runType", 1, split,
                                             1000, 4, 0,
                                             ByteOrder.BIG_ENDIAN, dictionary,
                                             null, true, false);

            System.out.println("FirstEventTest: write event #1");
            ER.writeEvent(event);

            // Write the first event as a EvioEvent object
            System.out.println("FirstEventTest: set first event");
            ER.setFirstEvent(eventFirst);

            // Write this first event as a EvioNode object
            //ER.setFirstEvent(firstEventNode);

            System.out.println("FirstEventTest: write event #2");
            ER.writeEvent(event);

            System.out.println("FirstEventTest: write event #3");
            ER.writeEvent(event);

            // All done writing
            ER.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }
    }




}
