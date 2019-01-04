package org.jlab.coda.jevio.apps;

import org.jlab.coda.jevio.*;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * This program takes an xml format file containing evio data
 * and saves it into a binary evio format file.
 * Created by timmer on 1/15/16.
 */
public class Xml2evio {

    /** Method to print out correct program command line usage. */
    private static void usage() {

        System.out.println("\nUsage: java Xml2evio -x <xml file>  -f <evio file>\n" +
                             "                     [-v] [-hex] [-d <dictionary file>]\n" +
                             "                     [-max <count>] [-skip <count>]\n\n" +

        "          -h    help\n" +
        "          -v    verbose output\n" +
        "          -x    xml input file name\n" +
        "          -f    evio output file name\n" +
        "          -d    xml dictionary file name\n" +
        "          -hex  display ints in hex with verbose option\n" +
        "          -max  maximum number of events to convert to evio\n" +
        "          -skip number of initial events to skip in xml file\n\n" +

        "          This program takes evio events in an xml file and\n" +
        "          converts it to a binary evio file.\n");
    }


    public Xml2evio() {
    }

    public static void main(String[] args) {
        int max=0, skip=0;
        boolean debug=false, verbose=false, hex=false;
        String xmlFile=null, evioFile=null, dictFile=null;
        EvioXMLDictionary dictionary=null;

        // loop over all args
        for (int i = 0; i < args.length; i++) {
            if (args[i].equalsIgnoreCase("-max")) {
                max = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-skip")) {
                skip = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-hex")) {
                hex = true;
            }
            else if (args[i].equalsIgnoreCase("-f")) {
                evioFile = args[i + 1];
                i++;
            }
            else if (args[i].equalsIgnoreCase("-x")) {
                xmlFile = args[i + 1];
                i++;
            }
            else if (args[i].equalsIgnoreCase("-d")) {
                dictFile = args[i + 1];
                i++;
            }
            else if (args[i].equalsIgnoreCase("-v")) {
                debug = true;
                verbose = true;
            }
            else {
                usage();
                System.exit(-1);
            }
        }

        if (xmlFile == null) {
            System.out.println("No xml file defined");
            usage();
            System.exit(-1);
        }

        if (dictFile != null) {
            // Any failure in the following line results in an empty dictionary
            dictionary = new EvioXMLDictionary(new File(dictFile));
            if (dictionary.size() < 1) {
                dictionary = null;
            }
        }

        try {
            String xml = new String(Files.readAllBytes(Paths.get(xmlFile)));
            List<EvioEvent> evList = Utilities.toEvents(xml, max, skip, dictionary, debug);
            EventWriter writer = null;
            if (evioFile != null) {
                writer = new EventWriter(evioFile);
            }

            for (EvioEvent ev : evList) {
                if (evioFile != null) {
                    writer.writeEvent(ev);
                }

                if (verbose) {
                    System.out.println("Event:\n" + ev.toXML(hex));
                }
            }

            if (evioFile != null) {
                writer.close();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

}
