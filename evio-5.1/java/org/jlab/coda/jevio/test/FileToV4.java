package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.File;

/**
 * Test program.
 * @author timmer
 * Date: Oct 21, 2013
 */
public class FileToV4 {


    /** For WRITING a local file. */
    public static void main(String args[]) {

        // files for input & output
        String readFileName  = "/daqfs/home/timmer/coda/jevio-4.3/testdata/evioV2format.ev";
        String writeFileName = "/daqfs/home/timmer/coda/jevio-4.3/testdata/evioV4format.ev";

        EvioEvent event=null;


        System.out.println("\nTRY READING");
        int evCount;
        File fileIn = new File(readFileName);
        System.out.println("read ev file: " + readFileName + " size: " + fileIn.length());
        try {
            EventWriter writer = new EventWriter(writeFileName);

            EvioReader fileReader = new EvioReader(readFileName, false, true);
            evCount = fileReader.getEventCount();
            System.out.println("\nnum ev = " + evCount);


            long t2, t1 = System.currentTimeMillis();

            while ( (event = fileReader.parseNextEvent()) != null) {
                writer.writeEvent(event);
            }

            t2 = System.currentTimeMillis();
            System.out.println("Sequential Time = " + (t2-t1) + " milliseconds");
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        System.out.println("DONE COPYING file to v4");
    }


}
