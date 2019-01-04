package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Test program.
 * @author timmer
 * Date: Oct 7, 2010
 */
public class FileTest2 {


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


    /** For reading file. */
    public static void main(String args[]) {

        if (true) {
            System.out.println("\nTRY READING");
            int evCount;
            EvioEvent event;
            //String fileName = "/daqfs/home/timmer/coda/jevio-4.3/testdata/evioV2format.ev";
            //String fileName = "/daqfs/home/timmer/coda/jevio-4.3/testdata/evioV4format.ev";
            String fileName = "/daqfs/home/timmer/coda/jevio-4.3/testdata/bigFileV4.ev";
            //String fileName = "/daqfs/home/timmer/coda/jevio-4.3/testdata/bigFileV4_2.ev";
            //String fileName = "/dev/shm/carlTest/bigFileV4_2.ev";
            File fileIn = new File(fileName);
            System.out.println("read ev file: " + fileName + " size: " + fileIn.length());

            try {
                boolean sequential = true;
                boolean checkNumSeq = false;
                //EvioReader fileReader = new EvioReader(fileName, checkNumSeq, sequential);

                ByteBuffer buffie = ByteBuffer.wrap(ByteDataTransformer.toBytes(data1, ByteOrder.BIG_ENDIAN));
                EvioReader fileReader = new EvioReader(buffie);

                evCount = fileReader.getEventCount();
                System.out.println("\nnum ev = " + evCount);
                System.out.println("blocks = " + fileReader.getBlockCount());
                System.out.println("dictionary = " + fileReader.getDictionaryXML() + "\n");


                int counter = 0;

                long t2, t1 = System.currentTimeMillis();

                if (true) {

                    System.out.println("Call rewind() ...");
                    fileReader.rewind();


                    System.out.println("Call toXMLFile() ...");
                    fileReader.toXMLFile("/tmp/XML_Test");

                    System.out.println("Call close() ...");
                    fileReader.close();

                }

                if (false) {
                    // Look at 100 events spread throughout the file
                    int jump = 5000;
                    jump = evCount/100;
                    System.out.println("evCount = " + evCount);

                    for (int i=1; i <= evCount; i += jump) {
                        event = fileReader.parseEvent(i);
                        //event = fileReader.getEvent(i);
                        if (event == null) {
                            System.out.println("\nWe got a NULL event !!!");
                            return;
                        }

                        if (i==1) {
                            System.out.println("event size = " + event.getTotalBytes());
                        }

//                        if ((i-1) % 10000 == 0) {
//                            System.out.println("ev# " + i + " = " + event.getIntData()[0]);
//                        }
                    }

                    t2 = System.currentTimeMillis();
                    System.out.println("Random access Time (getEvent(i)) = " + (t2-t1) +
                            " milliseconds, counter = " + counter);

                    System.out.println("Rewind ...");
                    fileReader.rewind();

                    t1 = System.currentTimeMillis();
                }

                if (false) {
                    counter = 0;
                    System.out.println("evCount = " + evCount);

                    for (int i=1; i <= evCount; i++) {
                        event = fileReader.parseEvent(i);
                        //event = fileReader.getEvent(i);
                        if (event == null) {
                            System.out.println("\nWe got a NULL event !!!");
                            return;
                        }
                        if (i==1) {
                            System.out.println("event size = " + event.getTotalBytes());
                        }
//                        if (i % 10000 == 0) {
//                            System.out.println("Data[0] = " + event.getIntData()[0]);
//                        }
                        counter++;
                    }

                    t2 = System.currentTimeMillis();
                    System.out.println("Random access Time (getEvent(i)) = " + (t2-t1) +
                            " milliseconds, counter = " + counter);

                    System.out.println("Rewind ...");
                    fileReader.rewind();

                    t1 = System.currentTimeMillis();
                }

                if (false) {
                    counter = 0;
                    for (int i=1; i <= evCount; i++) {
                        event = fileReader.parseEvent(i);
                        if (event == null) {
                            System.out.println("\nWe got a NULL event !!!");
                            return;
                        }
                        counter++;
                    }

                    t2 = System.currentTimeMillis();
                    System.out.println("Random access Time (parseEvent(i)) = " + (t2-t1) +
                            " milliseconds, counter = " + counter);

                    System.out.println("Rewind ...");
                    fileReader.rewind();

                    t1 = System.currentTimeMillis();
                }


                if (false) {
                    counter = 0;

                    while ( (event = fileReader.nextEvent()) != null) {
                        counter++;

//                    if (counter++ %10 ==0) {
//                        System.out.println("Event #" + counter + " =\n" + event);
//                        int[] d = event.getIntData();
//                        System.out.println("Data[0] = " + d[0] + ", Data[last] = " + d[d.length-1]);
//                        //            System.out.println("Event = \n" + event.toXML());
//                        //            while ( (event = fileReader.parseNextEvent()) != null) {
//                        //               System.out.println("Event = " + event.toString());
//                        //            }
//                    }
                    }

                    t2 = System.currentTimeMillis();
                    System.out.println("Sequential Time (nextEvent) = " + (t2-t1) +
                            " milliseconds, counter = " + counter);

                    System.out.println("Rewind ...");
                    fileReader.rewind();

                    t1 = System.currentTimeMillis();
                }



                if (false) {
                    counter = 0;

                    while ( (event = fileReader.parseNextEvent()) != null) {
                        counter++;

//                    if (counter++ %10 ==0) {
//                        System.out.println("Event #" + counter + " =\n" + event);
//                        int[] d = event.getIntData();
//                        System.out.println("Data[0] = " + d[0] + ", Data[last] = " + d[d.length-1]);
//                        //            System.out.println("Event = \n" + event.toXML());
//                        //            while ( (event = fileReader.parseNextEvent()) != null) {
//                        //               System.out.println("Event = " + event.toString());
//                        //            }
//                    }
                    }

                    t2 = System.currentTimeMillis();
                    System.out.println("Sequential Time (parseNextEvent) = " + (t2-t1) +
                            " milliseconds, counter = " + counter);

                    System.out.println("Rewind ...");
                    fileReader.rewind();

                    t1 = System.currentTimeMillis();
                }


            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }
        }

        System.out.println("DONE READING");

    }



}
