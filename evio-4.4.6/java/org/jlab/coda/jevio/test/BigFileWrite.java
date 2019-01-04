package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;

/**
 * Test program.
 * First used to write a big (6GB) file and now used to test the
 * RAID for hall D.
 *
 * @author timmer
 * Date: Oct 7, 2010
 */
public class BigFileWrite {

    private boolean debug, writeEvioFile, forceToDisk, random;
    private int bufferBytes = 15000;    // Write data in 15k byte chunks by default
    private long fileBytes = 2000000000; // 2GB file by default
    private int repeat = 3;
    String filename;


    /** Constructor. */
    BigFileWrite(String[] args) {
        decodeCommandLine(args);
    }


    /**
     * Method to decode the command line used to start this application.
     * @param args command line arguments
     */
    private void decodeCommandLine(String[] args) {

        // loop over all args
        for (int i = 0; i < args.length; i++) {

            if (args[i].equalsIgnoreCase("-h")) {
                usage();
                System.exit(-1);
            }
            else if (args[i].equalsIgnoreCase("-r")) {
                try {
                    repeat = Integer.parseInt(args[i+1]);
                }
                catch (NumberFormatException e) {/* default to 3 */}
                i++;
            }
            else if (args[i].equalsIgnoreCase("-file")) {
                filename = args[i+1];
                i++;
            }
            else if (args[i].equalsIgnoreCase("-fsize")) {
                try {
                    fileBytes = Long.parseLong(args[i + 1]);
                }
                catch (NumberFormatException e) {/* default to 2G bytes */}
                i++;
            }
            else if (args[i].equalsIgnoreCase("-bsize")) {
                try {
                    bufferBytes = Integer.parseInt(args[i+1]);
                }
                catch (NumberFormatException e) {/* default to 15k bytes */}
                i++;
            }
            else if (args[i].equalsIgnoreCase("-debug")) {
                debug = true;
            }
            else if (args[i].equalsIgnoreCase("-raf")) {
                random = true;
            }
            else if (args[i].equalsIgnoreCase("-force")) {
                forceToDisk = true;
            }
            else if (args[i].equalsIgnoreCase("-evio")) {
                writeEvioFile = true;
            }
            else {
                usage();
                System.exit(-1);
            }
        }

        return;
    }

    /** Method to print out correct program command line usage. */
    private static void usage() {
        System.out.println("\nUsage:\n\n" +
            "   java BigFileWrite\n" +
            "        [-u <UDL>]             set UDL to connect to cMsg\n" +
            "        [-file <filename>]     name of file to write\n" +
            "        [-fsize <file size>]   final size of file to write (bytes)\n" +
            "        [-bsize <buf size>]    size of buffer to write (bytes)\n" +
            "        [-r <repeat>]          repeat file write this many times, track avg rate\n" +
            "        [-force]               force every write to physical disk (each buffer written)\n" +
            "        [-evio]                write evio data (if not given, just write empty buffer)\n" +
            "        [-debug]               turn on printout\n" +
            "        [-h]                   print this help\n");
    }


    /**
     * Run as a stand-alone application.
     */
    public static void main(String[] args) {
        BigFileWrite receiver = new BigFileWrite(args);
        receiver.run();
    }


    /** Write big file of about 6GB for testing purposes.  */
    public void run() {

         // variables to track writing byte rate
        int count = 0;
        long t1, t2, time, totalT=0, totalCount=0;
        double rate, avgRate;
        long loops = fileBytes/bufferBytes;

        System.out.println("Write file of size " + ((float)fileBytes) + " bytes (chunks of " + bufferBytes +
                                   ", " + loops +
                                   " times), " + repeat + " times in a row");

        // Buffer with some data in it
        ByteBuffer dataBuf = ByteBuffer.allocate(bufferBytes);
        if (debug) System.out.println("data buf: pos = " + dataBuf.position() + ", cap = " + dataBuf.capacity() +
                                              ", lim = " + dataBuf.limit());

        for (int i=0; i < dataBuf.capacity()/4; i++) {
            dataBuf.putInt(i*4, i);
        }



        if (!writeEvioFile) {

            if (random) {
                // Use data output stream
                try {
                    File f;
                    for (int i=0; i < repeat; i++) {
                        RandomAccessFile raf = new RandomAccessFile(filename, "rw");

                        // keep track of time
                        t1 = System.currentTimeMillis();

                        for (int j=0; j < loops; j++) {
                            raf.write(dataBuf.array(), 0, dataBuf.limit());
                            // Force it to write to physical disk (KILLS PERFORMANCE!!!, 17x slower)
                            if (forceToDisk) {
                                raf.getFD().sync();
                            }
                            dataBuf.clear();
                        }

                        // all done writing
                        raf.close();

                        // all done
                        t2 = System.currentTimeMillis();

                        // calculate the data writing rate
                        time = t2 - t1;
                        rate = (loops * bufferBytes) / time / 1000.; // MBytes/sec
                        totalCount += loops * bufferBytes;
                        totalT += time;
                        avgRate = ((double) totalCount) / totalT / 1000.;
                        System.out.println("rate = " + String.format("%.3g", rate) +
                                                   ",  avg = " + String.format("%.3g", avgRate) + " MB/sec");
                        System.out.println("time = " + (time) + " milliseconds");

                        // clean up
                        f = new File(filename);
                        if (i < repeat-1) f.delete();
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
            else {
                // Use channels
                try {
                    File f;
                    for (int i=0; i < repeat; i++) {
                        FileOutputStream fileOutputStream = new FileOutputStream(filename, false);  // no appending
                        FileChannel fileChannel = fileOutputStream.getChannel();

                        // keep track of time
                        t1 = System.currentTimeMillis();

                        for (int j=0; j < loops; j++) {
                            fileChannel.write(dataBuf);
                            // Force it to write to physical disk (KILLS PERFORMANCE!!!, 17x slower)
                            if (forceToDisk) fileChannel.force(true);
                            dataBuf.clear();
                        }

                        // all done writing
                        fileOutputStream.close();

                        // all done
                        t2 = System.currentTimeMillis();

                        // calculate the data writing rate
                        time = t2 - t1;
                        rate = (loops * bufferBytes) / time / 1000.; // MBytes/sec
                        totalCount += loops * bufferBytes;
                        totalT += time;
                        avgRate = ((double) totalCount) / totalT / 1000.;
                        System.out.println("rate = " + String.format("%.3g", rate) +
                                                   ",  avg = " + String.format("%.3g", avgRate) + " MB/sec");
                        System.out.println("time = " + (time) + " milliseconds");

                        // clean up
                        f = new File(filename);
                        if (i < repeat-1) f.delete();
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                }

            }
        }

        else {

            try {
                File f;
                int tag = 1, num = 2;

                // Create event, in ByteBuffer format, to wrap data
                ByteBuffer buf = ByteBuffer.allocate(bufferBytes + 8);
                CompactEventBuilder builder = new CompactEventBuilder(buf);
                builder.openBank(tag, num, DataType.CHAR8);
                builder.addByteData(dataBuf.array());
                builder.closeAll();

if (debug) System.out.println("buf: pos = " + buf.position() + ", cap = " + buf.capacity() +
                              ", lim = " + buf.limit() + ", order = " + buf.order());

                // Create an event writer to write out the test events.
                EventWriter eventWriter = new EventWriter(filename, false, ByteOrder.BIG_ENDIAN);

                for (int i=0; i < repeat; i++) {
                    // keep track of time
                    t1 = System.currentTimeMillis();

                    for (int j=0; j < loops; j++) {
                        eventWriter.writeEvent(buf, forceToDisk);
                        buf.clear();
                    }

                    // all done writing
                    eventWriter.close();

                    // all done
                    t2 = System.currentTimeMillis();

                    // calculate the data writing rate
                    time = t2 - t1;
                    rate = (loops * (bufferBytes + 8)) / time / 1000.; // MBytes/sec
                    totalCount += loops * (bufferBytes + 8);
                    totalT += time;
                    avgRate = ((double) totalCount) / totalT / 1000.;
                    System.out.println("rate = " + String.format("%.3g", rate) +
                                               ",  avg = " + String.format("%.3g", avgRate) + " MB/sec");
                    System.out.println("time = " + (time) + " milliseconds");

                    // clean up
                    f = new File(filename);
                    if (i < repeat-1) f.delete();

                    eventWriter = new EventWriter(filename, false, ByteOrder.BIG_ENDIAN);
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }


    }




}
