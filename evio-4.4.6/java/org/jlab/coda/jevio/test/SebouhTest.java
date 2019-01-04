package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.CountDownLatch;

/**
 * Run multithreaded scaling test similar to Sebouh's.
 * @author timmer
 * Date: Oct 7, 2010
 */
public class SebouhTest extends Thread {

    static CountDownLatch endingLatch;
    static CountDownLatch startingLatch;
    static CountDownLatch readyToStartLatch;

    public static void main(String arg[]) throws EvioException, IOException, InterruptedException{
        int n = (arg.length > 0) ? Integer.parseInt(arg[0]) : Runtime.getRuntime().availableProcessors();

        EvioEvent ev = generateEvent();

        System.out.println(n + " threads");
        SebouhTest[] threads = new SebouhTest[n];
        for (int i = 0; i < n; i++){
            threads[i] = new SebouhTest(ev);
        }

        endingLatch       = new CountDownLatch(n);
        startingLatch     = new CountDownLatch(1);
        readyToStartLatch = new CountDownLatch(n);

        System.out.println();

        for (int i = 0; i < n; i++) {
            threads[i].start();
        }

        // wait to make sure all threads are up before starting
        readyToStartLatch.await();

        // start all threads simultaneously
        startingLatch.countDown();

        // wait until everyone is finished before calculating the average
        endingLatch.await();

        double avgTime  = 0.;
        long iterations = 0L;

        for (int i = 0; i < n; i++) {
            avgTime += threads[i].getTotalTime();
            iterations += threads[i].getIterations();
        }
        avgTime /= iterations;
        System.out.println(String.format("Average time = %6.6f", avgTime) + " millisec");
    }

    // Stats
    double totalTime;
    long iterations;

    // Use only 1 each of these object, then
    // reuse to avoid unnecessary object creations
    EvioEvent event;
    ByteBuffer buffer;
    EvioReader reader;
    EventWriter evWriter;


	public SebouhTest(EvioEvent ev) {
        this.event = ev;
        int bufSize = ev.getTotalBytes() + 2*4*8 + 1024;


        buffer = ByteBuffer.allocate(bufSize);
    }

    long getIterations() {return iterations;}
    double getTotalTime() {return totalTime;}

	@Override
	public void run() {

        long tStart=0L, tEnd=0L;

        try {
            readyToStartLatch.countDown();
            startingLatch.await();

		    tStart = System.nanoTime();
		    /*
		        int k = 0;

		        for(long i = 0; i<1000000000L; i++){
			        k+= i*i;
		    }*/


            for(int i = 0; i < 50000; i++){
                evioToBytes();
                bytesToEvio();
                iterations++;
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }


        tEnd = System.nanoTime();
        double time = (tEnd - tStart)/1000000.;
        totalTime += time;

		System.out.println(totalTime + " totaltime in ms");
        endingLatch.countDown();
	}



    public static EvioEvent generateEvent() {

        EvioEvent ev = null;

        try {

            // Evio event = bank of banks
            EventBuilder eb = new EventBuilder(500, DataType.BANK, 0);
            ev = eb.getEvent();

                // bank to contain banks of ints
                EvioBank ibanks = new EvioBank(500, DataType.BANK, 100);
                eb.addChild(ev, ibanks);

                    // 1st bank of ints
                    int[] idata = new int[10];
                    EvioBank ibanks1 = new EvioBank(500, DataType.INT32, 23);
                    ibanks1.appendIntData(idata);
                    eb.addChild(ibanks, ibanks1);

                    // 2nd bank of ints
                    EvioBank ibanks2 = new EvioBank(500, DataType.INT32, 24);
                    ibanks2.appendIntData(idata);
                    eb.addChild(ibanks, ibanks2);

                    // 3rd bank of ints
                    EvioBank ibanks3 = new EvioBank(500, DataType.INT32, 24);
                    ibanks3.appendIntData(idata);
                    eb.addChild(ibanks, ibanks3);

                    // 4th bank of ints
                    EvioBank ibanks4 = new EvioBank(500, DataType.INT32, 24);
                    ibanks4.appendIntData(idata);
                    eb.addChild(ibanks, ibanks4);

                // bank to contain banks of doubles
                EvioBank dbanks = new EvioBank(500, DataType.BANK, 200);
                eb.addChild(ev, dbanks);

                    // add 1st bank of doubles
                    double[] darray = new double[10];
                    EvioBank dbanks1 = new EvioBank(3, DataType.DOUBLE64, 1);
                    dbanks1.appendDoubleData(darray);
                    eb.addChild(dbanks, dbanks1);

                    // add 2nd bank of doubles
                    EvioBank dbanks2 = new EvioBank(3, DataType.DOUBLE64, 8);
                    dbanks2.appendDoubleData(darray);
                    eb.addChild(dbanks, dbanks2);

        }
        catch (Exception e) {
            e.printStackTrace();
        }

        return ev;
    }

    static void printBuf(ByteBuffer buf) {
        System.out.println("BUFFER:");
        int lim = buf.limit();
        int pos = buf.position();
        int cap = buf.capacity();
        int max = lim > 4*20 ? 20 : lim/4;
        for (int i=0; i < max; i++) {
            System.out.println("0x" + Integer.toHexString(buf.getInt(i)));
        }
        System.out.println("\n\n");

    }

    public void evioToBytes() {

        try {
            // Create writer
            buffer.clear();
            if (evWriter == null) {
                evWriter = new EventWriter(buffer);
            }
            else {
                evWriter.setBuffer(buffer);
            }
            // Write event
            evWriter.writeEvent(event);
            evWriter.close();
            buffer.flip();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

    }

    public EvioEvent bytesToEvio() {

        EvioEvent ev = null;

        try {
            if (reader == null) {
                reader = new EvioReader(buffer);
            }
            else {
                reader.setBuffer(buffer);
            }
            EventParser p = reader.getParser();
            p.setNotificationActive(false);

            ev = reader.parseNextEvent();
//System.out.println("parsed ev has " + ev.getChildCount() + " kids");
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        return ev;
    }


    public static ByteBuffer evioToBytesOrig(EvioEvent ev) {

        // Buffer will need to hold event + 2 block headers
        // (1 beginning and 1 @ end) plus a little extra just in case
        int bufSize = ev.getTotalBytes() + 2*4*8 + 1024;

        ByteBuffer buf = null;

        try {
            // Create writer
            buf = ByteBuffer.allocate(bufSize);
            EventWriter evWriter = new EventWriter(buf);

            // Write event
            evWriter.writeEvent(ev);
            evWriter.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        return buf;
    }

    public static EvioEvent bytesToEvioOrig(ByteBuffer buf) {

        EvioEvent ev = null;

        try {
            EvioReader reader = new EvioReader(buf);
            ev = reader.parseNextEvent();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        return ev;
    }


}
