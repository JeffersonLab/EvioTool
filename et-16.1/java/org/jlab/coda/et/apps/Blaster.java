/*----------------------------------------------------------------------------*
 *  Copyright (c) 2001        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*/

package org.jlab.coda.et.apps;


import org.jlab.coda.et.*;
import org.jlab.coda.et.enums.Mode;

import java.awt.image.DataBuffer;
import java.lang.Exception;
import java.lang.NumberFormatException;
import java.lang.String;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * This class is an example of an event producer designed to blast data
 * at the highest possible rate to the ET system.
 *
 * @author Carl Timmer
 */
public class Blaster {

    public Blaster() {
    }


    private static void usage() {
        System.out.println("\nUsage: java Blaster -f <ET name> -host <ET host> [-h] [-v] [-c <chunk size>]\n" +
                "                     [-s <event size>] [-g <group>] [-p <ET server port>] [-i <interface address>]\n" +
                "                     [-rb <buffer size>] [-sb <buffer size>] [-nd]\n\n" +
                "       -host  ET system's host\n" +
                "       -f     ET system's (memory-mapped file) name\n" +
                "       -h     help\n" +
                "       -v     verbose output\n" +
                "       -c     number of events in one get/put array\n" +
                "       -s     event size in bytes\n" +
                "       -g     group from which to get new events (1,2,...)\n" +
                "       -p     ET server port\n" +
                "       -i     outgoing network interface IP address (dot-decimal)\n\n" +
                "       -rb    TCP receive buffer size (bytes)\n" +
                "       -sb    TCP send buffer size (bytes)\n" +
                "       -nd    turn on TCP no-delay\n" +
                "        This consumer works by making a direct connection to the\n" +
                "        ET system's server port.\n");
    }


    public static void main(String[] args) {

        String etName = null, host = null, netInterface = null;
        int port = EtConstants.serverPort;
        int group = 1;
        int size  = 32;
        int chunk = 1;
        int recvBufSize = 0;
        int sendBufSize = 0;
        boolean noDelay = false;
        boolean verbose = false;

        for (int i = 0; i < args.length; i++) {
            if (args[i].equalsIgnoreCase("-f")) {
                etName = args[++i];
            }
            else if (args[i].equalsIgnoreCase("-i")) {
                netInterface = args[++i];
            }
            else if (args[i].equalsIgnoreCase("-host")) {
                host = args[++i];
            }
            else if (args[i].equalsIgnoreCase("-p")) {
                try {
                    port = Integer.parseInt(args[++i]);
                    if ((port < 1024) || (port > 65535)) {
                        System.out.println("Port number must be between 1024 and 65535.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper port number.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-c")) {
                try {
                    chunk = Integer.parseInt(args[++i]);
                    if ((chunk < 1) || (chunk > 1000)) {
                        System.out.println("Chunk size may be 1 - 1000.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper chunk size.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-s")) {
                try {
                    size = Integer.parseInt(args[++i]);
                    if (size < 1) {
                        System.out.println("Size needs to be positive int.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper size.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-g")) {
                try {
                    group = Integer.parseInt(args[++i]);
                    if ((group < 1) || (group > 10)) {
                        System.out.println("Group number must be between 0 and 10.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper group number.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-rb")) {
                try {
                    recvBufSize = Integer.parseInt(args[++i]);
                    if (recvBufSize < 1) {
                        System.out.println("recv buffer must be > 0.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper recv buffer size.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-sb")) {
                try {
                    sendBufSize = Integer.parseInt(args[++i]);
                    if (sendBufSize < 1) {
                        System.out.println("send buffer must be > 0.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper send buffer size.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-nd")) {
                noDelay = true;
            }
            else if (args[i].equalsIgnoreCase("-v")) {
                verbose = true;
            }
            else {
                usage();
                return;
            }
        }

        if (host == null || etName == null) {
            usage();
            return;
        }

        try {
            // Make a direct connection to ET system's tcp server
            EtSystemOpenConfig config = new EtSystemOpenConfig(etName, host, port);
            config.setConnectRemotely(true);

            if (noDelay)              config.setNoDelay(noDelay);
            if (recvBufSize > 0)      config.setTcpRecvBufSize(recvBufSize);
            if (sendBufSize > 0)      config.setTcpSendBufSize(sendBufSize);
            if (netInterface != null) config.setNetworkInterface(netInterface);

            // create ET system object with verbose debugging output
            EtSystem sys = new EtSystem(config);
            if (verbose) sys.setDebug(EtConstants.debugInfo);
            sys.open();

            // get GRAND_CENTRAL station object
            EtStation gc = sys.stationNameToObject("GRAND_CENTRAL");

            // attach to GRAND_CENTRAL
            EtAttachment att = sys.attach(gc);

            // array of events
            EtEvent[] mevs;
            EtEventImpl realEvent;

            // data to put in new events
            byte[] data = new byte[size];
            ByteBuffer byteBuffer = ByteBuffer.wrap(data);

            long   eventCount=0L, byteCount=0L;
            long   t1, t2, time, totalT=0L, totalEventCount=0L, totalByteCount=0L;
            double eventRate, avgEventRate, byteRate, avgByteRate;


            // keep track of time for event rate calculations
            t1 = System.currentTimeMillis();

            while (true) {
                // get array of new events without allocating memory for data
                mevs = sys.newEvents(att, Mode.SLEEP, true, 0, chunk, size, group);

                // example of how to manipulate events
                for (EtEvent mev : mevs) {
                    // A little bit of trickery to set the data buffer
                    // without having to create objects and allocate
                    // memory each time.
                    realEvent = (EtEventImpl) mev;
                    realEvent.setDataBuffer(byteBuffer);

                    // set data length
                    mev.setLength(size);
                }

                // put events back into ET system
                sys.putEvents(att, mevs);
                eventCount += mevs.length;
                byteCount  += mevs.length*(size + 52) + 20; // 52 is event overhead, 20 is ET call overhead

                // calculate the event rate
                t2 = System.currentTimeMillis();
                time = t2 - t1;

                if (time > 5000) {
                    eventRate = 1000.0 * eventCount / time;
                    byteRate  = 1000.0 * byteCount  / time;

                    totalEventCount += eventCount;
                    totalByteCount  += byteCount;

                    totalT += time;

                    avgEventRate = 1000.0 * totalEventCount / totalT;
                    avgByteRate  = 1000.0 * totalByteCount  / totalT;

                    System.out.printf("evRate: %3.4g Hz,  %3.4g avg;  byteRate: %3.4g bytes/sec,  %3.4g avg\n",
                                       eventRate, avgEventRate, byteRate, avgByteRate);

                    eventCount = 0;
                    byteCount  = 0;

                    t1 = System.currentTimeMillis();
                }
            }
        }
        catch (Exception ex) {
            System.out.println("Error using ET system as producer");
            ex.printStackTrace();
        }
    }
    
}
