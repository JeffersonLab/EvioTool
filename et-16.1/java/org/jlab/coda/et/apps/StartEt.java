/*----------------------------------------------------------------------------*
 *  Copyright (c) 2001        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12H        *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*/

package org.jlab.coda.et.apps;

import java.io.File;
import java.lang.*;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.HashSet;

import org.jlab.coda.et.*;
import org.jlab.coda.et.system.*;

/**
 * This class is an example of starting up an ET system.
 *
 * @author Carl Timmer
 * @version 7.0
 */
public class StartEt {

    /** Method to print out correct program command line usage. */
    private static void usage() {

        System.out.println("\nUsage: java StartEt [-h] [-v] [-d] [-f <file>] [-n <events>] [-s <eventSize>]\n" +
                             "                    [-g <groups>] [-a <multicast address>]\n" +
                             "                    [-p <TCP server port>] [-u <UDP port>]\n" +
                             "                    [-rb <buf size>] [-sb <buf size>] [-nd]\n\n" +

        "          -h   help\n" +
        "          -v   verbose output\n" +
        "          -d   deletes an existing file first\n" +
        "          -f   memory-mapped file name\n\n" +

        "          -n   number of events\n" +
        "          -s   event size in bytes\n" +
        "          -g   number of groups to divide events into\n\n" +

        "          -p   TCP server port #\n" +
        "          -u   UDP broadcast port #\n" +
        "          -a   multicast address\n\n" +

        "          -rb  TCP receive buffer size (bytes)\n" +
        "          -sb  TCP send    buffer size (bytes)\n" +
        "          -nd  use TCP_NODELAY option\n\n" +

        "          This program starts up an ET system.\n" +
        "          Listens on 239.200.0.0 by default.\n");
    }


    public StartEt() {
    }

    public static void main(String[] args) {
        int numEvents = 3000, size = 128;
        int serverPort = EtConstants.serverPort;
        int udpPort = EtConstants.udpPort;
        int recvBufSize = 0, sendBufSize = 0;
        int numGroups = 1;
        boolean debug = false;
        boolean noDelay = false;
        boolean deleteFile = false;
        String file = null;
        HashSet<String> multicastAddrs = new HashSet<String>();

        // loop over all args
        for (int i = 0; i < args.length; i++) {
            if (args[i].equalsIgnoreCase("-h")) {
                usage();
                System.exit(-1);
            }
            else if (args[i].equalsIgnoreCase("-n")) {
                numEvents = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-f")) {
                file = args[i + 1];
                i++;
            }
            else if (args[i].equalsIgnoreCase("-p")) {
                serverPort = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-u")) {
                udpPort = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-s")) {
                size = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-rb")) {
                recvBufSize = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-sb")) {
                sendBufSize = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-nd")) {
                noDelay = true;
            }
            else if (args[i].equalsIgnoreCase("-v")) {
                debug = true;
            }
            else if (args[i].equalsIgnoreCase("-g")) {
                numGroups = Integer.parseInt(args[i + 1]);
                i++;
            }
            else if (args[i].equalsIgnoreCase("-d")) {
                deleteFile = true;
            }
            else if (args[i].equalsIgnoreCase("-a")) {
                try {
                    String addr = args[++i];
                    if (InetAddress.getByName(addr).isMulticastAddress()) {
                        multicastAddrs.add(addr);
                    }
                    else {
                        System.out.println("\nignoring improper multicast address\n");
                    }
                }
                catch (UnknownHostException e) {}
            }
            else {
                usage();
                System.exit(-1);
            }
        }

        if (file == null) {
            String et_filename = System.getenv("SESSION");
            if (et_filename == null) {
                System.out.println("No ET file name given and SESSION env variable not defined");
                usage();
                System.exit(-1);
            }

            file = "/tmp/et_sys_" + et_filename;
        }

        // check length of name
        if (file.length() >=  EtConstants.fileNameLengthMax) {
            System.out.println("ET file name is too long");
            usage();
            System.exit(-1);
        }

        if (deleteFile) {
            File f = new File(file);
            f.delete();
        }

        try {
            System.out.println("STARTING ET SYSTEM");

            // ET system configuration object
            SystemConfig config = new SystemConfig();

            // multicast addresses to listen on
            if (multicastAddrs.size() < 1) {
                // listen on default multicast address if nothing else
                config.addMulticastAddr(EtConstants.multicastAddr);
            }
            else {
                for (String maddr : multicastAddrs) {
                    config.addMulticastAddr(maddr);
                }
            }

            // set tcp server port
            config.setServerPort(serverPort);
            // set port for listening for udp packets
            config.setUdpPort(udpPort);
            // set total number of events
            config.setNumEvents(numEvents);
            // set size of events in bytes
            config.setEventSize(size);
            // set tcp receive buffer size in bytes
            if (recvBufSize > 0) {
                config.setTcpRecvBufSize(recvBufSize);
            }
            // set tcp send buffer size in bytes
            if (sendBufSize > 0) {
                config.setTcpSendBufSize(sendBufSize);
            }
            // set tcp no-delay
            if (noDelay) {
                config.setNoDelay(noDelay);
            }
            // set debug level
            if (debug) {
                config.setDebug(EtConstants.debugInfo);
            }

            // divide events into equal groups and any leftovers into another group */
            if (numGroups > 1) {
                int addgroup=0;

                int n = numEvents / numGroups;
                int r = numEvents % numGroups;
                if (r > 0) {
                    addgroup = 1;
                }

                int[] groups = new int[numGroups+addgroup];

                for (int i=0; i < numGroups; i++) {
                    groups[i] = n;
                }

                if (addgroup > 0) {
                    groups[numGroups] = r;
                }

                config.setGroups(groups);
            }

            // create an active ET system
            SystemCreate sys = new SystemCreate(file, config);
        }
        catch (Exception ex) {
            System.out.println("ERROR STARTING ET SYSTEM");
            ex.printStackTrace();
        }

    }
}
