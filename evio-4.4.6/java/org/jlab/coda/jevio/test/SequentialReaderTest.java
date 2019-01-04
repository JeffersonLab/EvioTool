package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.Arrays;
import java.util.List;

/**
 * Test program.
 * @author timmer
 * Date: Apr 14, 2015
 */
public class SequentialReaderTest {

    /** For testing only */
    public static void main(String args[]) {

        int counter = 1;

        String fileName  = "/home/timmer/evioTestFiles/hd_rawdata_002175_000_good.evio";
        File fileIn = new File(fileName);
        System.out.println("read ev file: " + fileName + " size: " + fileIn.length());

        try {
            // Read sequentially
            EvioReader fileReader = new EvioReader(fileName, false, true);

            System.out.println("count events ...");
            int eventCount = fileReader.getEventCount();
            System.out.println("done counting events");

            System.out.println("get ev #1");
            EvioEvent event = fileReader.getEvent(1);

            System.out.println("get ev #" + (eventCount/2));
            event = fileReader.getEvent(eventCount/2);

            System.out.println("get ev #" + eventCount);
            event = fileReader.getEvent(eventCount);

            System.out.println("rewind file");
            fileReader.rewind();

            System.out.println("goto ev #1");
            event = fileReader.gotoEventNumber(1);

            System.out.println("goto ev #" + (eventCount/2));
            event = fileReader.gotoEventNumber(eventCount / 2);

            System.out.println("goto ev #" + eventCount);
            event = fileReader.gotoEventNumber(eventCount);

            System.out.println("rewind file");
            fileReader.rewind();

            System.out.println("parse ev #1");
            event = fileReader.parseEvent(1);

            System.out.println("parse ev #" + (eventCount/2));
            event = fileReader.parseEvent(eventCount / 2);

            System.out.println("parse ev #" + eventCount);
            event = fileReader.parseEvent(eventCount);

            System.out.println("rewind file");
            fileReader.rewind();

            while (fileReader.parseNextEvent() != null) {
                System.out.println("parseNextEvent # " + counter++);
            }

        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


}
