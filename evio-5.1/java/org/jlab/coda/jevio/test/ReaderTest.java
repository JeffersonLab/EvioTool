package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * Test program made to work with 2 file produced by the SwapTest (one of the mainN() methods).
 * This test the readers both regular and compact to see if they parse this single event
 * properly.
 *
 * @author timmer
 * Date: Dec 3, 2015
 */
public class ReaderTest {

    // These files were written by SwapTest.java or by hand
    static String compositeFile =  "/home/timmer/evioTestFiles/xmlTests/composite.evio";
    static String regularXmlFile = "/home/timmer/evioTestFiles/xmlTests/regularEvent.xml";
    static String regularFile =    "/home/timmer/evioTestFiles/xmlTests/regularEvent.evio";
    static String regularFile2 =   "/home/timmer/evioTestFiles/xmlTests/regularEvent2.evio";
    static String smallXmlFile =   "/home/timmer/evioTestFiles/xmlTests/regularEventSmall.xml";
    static String swappedFile =    "/home/timmer/evioTestFiles/xmlTests/swappedEvent.evio";


    /** Reading files to get EvioNode objects & print out XML representation. */
    public static void main4(String args[]) {

        try {

            String xml = new String(Files.readAllBytes(Paths.get(regularXmlFile)));
            System.out.println("\nXML:\n" + xml);

            System.out.println("Convert XML to EvioEvent:");
            List<EvioEvent> evList = Utilities.toEvents(xml);

            for (EvioEvent ev : evList) {
                System.out.println("\n\nEvioEvent ---> XML:\n" + ev.toXML());
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    /** Reading files to get EvioNode objects & print out XML representation. */
    public static void main3(String args[]) {

        EvioNode node;
        boolean hex = false;

        try {
            EvioCompactReader fileReader = new EvioCompactReader(compositeFile);


            int evNum = 1;
            while ( (node = fileReader.getScannedEvent(evNum++)) != null) {
                String xml = Utilities.toXML(node, hex);
                System.out.println("\nXML:\n" + xml);


                System.out.println("Convert XML to EvioEvent:");
                List<EvioEvent> evList = Utilities.toEvents(xml);

                System.out.println("\n\nev to XML:\n" + evList.get(0).toXML());

                System.out.println("\n\n");
            }

        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    /** Reading files to get EvioNode objects & print out XML representation. */
    public static void main(String args[]) {

        EvioNode node;
        boolean hex=false, compact=true;

        try {
            if (!compact) {
                EvioReader reader = new EvioReader(regularFile);
                EvioEvent event = reader.parseEvent(1);
                System.out.println("\nXML:\n" + event.toXML(hex));
            }
            else {
                EvioCompactReader fileReader = new EvioCompactReader(regularFile);
                node = fileReader.getScannedEvent(1);
                String xml = Utilities.toXML(node, hex);
                System.out.println("\nXML:\n" + xml);
            }

//            System.out.println("----------------------------------------------------------");
//            System.out.println("----------------------SWAPPED-----------------------------");
//
//            fileReader = new EvioCompactReader(swappedFile);
//
//            node = fileReader.getScannedEvent(1);
//            xml = node.toXML();
//            System.out.println("\nXML:\n" + xml);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }



    /** Reading files to get EvioEvent objects & print out XML representation. */
    public static void main1(String args[]) {

        EvioEvent event;

        try {
            EvioReader fileReader = new EvioReader(regularFile);

            System.out.println("TRY EvioReader.parseEvent(1)");
            event = fileReader.parseEvent(1);
            System.out.println("\nXML:\n" + event.toXML());

            System.out.println("Rewind ...");
            fileReader.rewind();

            System.out.println("TRY EvioReader.parseNextEvent()");

            event = fileReader.parseNextEvent();
            System.out.println("\nXML:\n" + event.toXML());
            fileReader.rewind();

            System.out.println("----------------------------------------------------------");
            System.out.println("----------------------SWAPPED-----------------------------");

            fileReader = new EvioReader(swappedFile);

            System.out.println("TRY EvioReader.parseEvent(1)");
            event = fileReader.parseEvent(1);
            System.out.println("\nXML:\n" + event.toXML());

            System.out.println("Rewind ...");
            fileReader.rewind();

            System.out.println("TRY EvioReader.parseNextEvent()");

            event = fileReader.parseNextEvent();
            System.out.println("\nXML:\n" + event.toXML());
            fileReader.rewind();

        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }
    }





}
