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


import java.lang.*;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashSet;

import org.jlab.coda.et.*;
import org.jlab.coda.et.enums.Mode;

/**
 * This class is an example of an event producer for an ET system.
 *
 * @author Carl Timmer
 */
public class Producer {

    public Producer() {
    }


    private static void usage() {
        System.out.println("\nUsage: java Producer -f <et name>\n" +
                "                      [-h] [-v] [-r] [-m] [-b] [-nd] [-blast]\n" +
                "                      [-host <ET host>] [-w <big endian? 0/1>]\n" +
                "                      [-s <event size>] [-c <chunk size>] [-g <group>]\n" +
                "                      [-d <delay>] [-p <ET port>]\n" +
                "                      [-i <interface address>] [-a <mcast addr>]\n" +
                "                      [-rb <buf size>] [-sb <buf size>]\n\n" +

                "       -f     ET system's (memory-mapped file) name\n" +
                "       -host  ET system's host if direct connection (default to local)\n" +
                "       -h     help\n" +
                "       -v     verbose output\n\n" +

                "       -s     event size in bytes\n" +
                "       -c     number of events in one get/put array\n" +
                "       -g     group from which to get new events (1,2,...)\n" +
                "       -d     delay in millisec between each round of getting and putting events\n\n" +

                "       -p     ET port (TCP for direct, UDP for broad/multicast)\n" +
                "       -r     act as remote (TCP) client even if ET system is local\n" +
                "       -w     write data (1 sequential int per event), 1 = Java (big) endian, 0 else\n" +
                "       -blast if remote, use external data buf (no mem allocation),\n" +
                "              do not write data (overrides -w)\n\n" +

                "       -i     outgoing network interface address (dot-decimal)\n" +
                "       -a     multicast address(es) (dot-decimal), may use multiple times\n" +
                "       -m     multicast to find ET (use default address if -a unused)\n" +
                "       -b     broadcast to find ET\n\n" +

                "       -rb    TCP receive buffer size (bytes)\n" +
                "       -sb    TCP send    buffer size (bytes)\n" +
                "       -nd    use TCP_NODELAY option\n\n" +

                "        This producer works by making a direct connection to the\n" +
                "        ET system's server port and host unless at least one multicast address\n" +
                "        is specified with -a, the -m option is used, or the -b option is used\n" +
                "        in which case multi/broadcasting used to find the ET system.\n" +
                "        If multi/broadcasting fails, look locally to find the ET system.\n" +
                "        This program gets new events from the system and puts them back.\n\n");
    }


    public static void main(String[] args) {

        int group=1, delay=0, size=32, port=0;
        int chunk=1, recvBufSize=0, sendBufSize=0;
        boolean noDelay=false, verbose=false, remote=false, blast=false;
        boolean bigEndian=true, writeData=false;
        boolean broadcast=false, multicast=false, broadAndMulticast=false;
        HashSet<String> multicastAddrs = new HashSet<String>();
        String outgoingInterface=null, etName=null, host=null;


        for (int i = 0; i < args.length; i++) {
            if (args[i].equalsIgnoreCase("-f")) {
                etName = args[++i];
            }
            else if (args[i].equalsIgnoreCase("-host")) {
                host = args[++i];
            }
            else if (args[i].equalsIgnoreCase("-a")) {
                try {
                    String addr = args[++i];
                    if (InetAddress.getByName(addr).isMulticastAddress()) {
                        multicastAddrs.add(addr);
                        multicast = true;
                    }
                    else {
                        System.out.println("\nignoring improper multicast address\n");
                    }
                }
                catch (UnknownHostException e) {}
            }
            else if (args[i].equalsIgnoreCase("-i")) {
                outgoingInterface = args[++i];
            }
            else if (args[i].equalsIgnoreCase("-nd")) {
                noDelay = true;
            }
            else if (args[i].equalsIgnoreCase("-v")) {
                verbose = true;
            }
            else if (args[i].equalsIgnoreCase("-r")) {
                remote = true;
            }
            else if (args[i].equalsIgnoreCase("-m")) {
                multicast = true;
            }
            else if (args[i].equalsIgnoreCase("-b")) {
                broadcast = true;
            }
            else if (args[i].equalsIgnoreCase("-blast")) {
                blast = true;
            }
            else if (args[i].equalsIgnoreCase("-w")) {
                try {
                    int isBigEndian = Integer.parseInt(args[++i]);
                    bigEndian = isBigEndian != 0;
                    writeData = true;
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper endian value (1=big, else 0).");
                    usage();
                    return;
                }
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
            else if (args[i].equalsIgnoreCase("-rb")) {
                try {
                    recvBufSize = Integer.parseInt(args[++i]);
                    if (recvBufSize < 0) {
                        System.out.println("Receive buf size must be 0 or greater.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper receive buffer size.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-sb")) {
                try {
                    sendBufSize = Integer.parseInt(args[++i]);
                    if (sendBufSize < 0) {
                        System.out.println("Send buf size must be 0 or greater.");
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
            else if (args[i].equalsIgnoreCase("-d")) {
                try {
                    delay = Integer.parseInt(args[++i]);
                    if (delay < 1) {
                        System.out.println("delay must be > 0.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper delay.");
                    usage();
                    return;
                }
            }
            else {
                usage();
                return;
            }
        }

        if (etName == null) {
            usage();
            return;
        }


        try {
            EtSystemOpenConfig config = new EtSystemOpenConfig();

            if (broadcast && multicast) {
                broadAndMulticast = true;
            }

            // if multicasting to find ET
            if (multicast) {
                if (multicastAddrs.size() < 1) {
                    // Use default mcast address if not given on command line
                    config.addMulticastAddr(EtConstants.multicastAddr);
                }
                else {
                    // Add multicast addresses to use
                    for (String mcastAddr : multicastAddrs) {
                        config.addMulticastAddr(mcastAddr);
                    }
                }
            }

            if (broadAndMulticast) {
                System.out.println("Broad and Multicasting");
                if (port == 0) {
                    port = EtConstants.udpPort;
                }
                config.setUdpPort(port);
                config.setNetworkContactMethod(EtConstants.broadAndMulticast);
                config.setHost(EtConstants.hostAnywhere);
            }
            else if (multicast) {
                System.out.println("Multicasting");
                if (port == 0) {
                    port = EtConstants.udpPort;
                }
                config.setUdpPort(port);
                config.setNetworkContactMethod(EtConstants.multicast);
                config.setHost(EtConstants.hostAnywhere);
            }
            else if (broadcast) {
                System.out.println("Broadcasting");
                if (port == 0) {
                    port = EtConstants.udpPort;
                }
                config.setUdpPort(port);
                config.setNetworkContactMethod(EtConstants.broadcast);
                config.setHost(EtConstants.hostAnywhere);
            }
            else {
                if (port == 0) {
                    port = EtConstants.serverPort;
                }
                config.setTcpPort(port);
                config.setNetworkContactMethod(EtConstants.direct);
                if (host == null) {
                    host = EtConstants.hostLocal;
                }
                config.setHost(host);
                System.out.println("Direct connection to " + host);
            }

            // Defaults are to use operating system default buffer sizes and turn off TCP_NODELAY
            config.setNoDelay(noDelay);
            config.setTcpRecvBufSize(recvBufSize);
            config.setTcpSendBufSize(sendBufSize);
            config.setNetworkInterface(outgoingInterface);
            config.setWaitTime(0);
            config.setEtName(etName);
            config.setResponsePolicy(EtConstants.policyError);

            if (remote) {
                System.out.println("Set as remote");
                config.setConnectRemotely(remote);
            }

            // create ET system object with verbose debugging output
            EtSystem sys = new EtSystem(config);
            if (verbose) {
                sys.setDebug(EtConstants.debugInfo);
            }
            sys.open();

            // Make things self-consistent by not taking time to write data if blasting.
            // Blasting flag takes precedence.
            if (blast) {
                writeData = false;
            }

            // Find out if we have a remote connection to the ET system
            // so we know if we can use external data buffer for events
            // for blasting - which is quite a bit faster.
            if (sys.usingJniLibrary()) {
                // Local blasting is just the same as local producing
                blast = false;
                System.out.println("ET is local\n");
            }
            else {
                System.out.println("ET is remote\n");
            }


            // get GRAND_CENTRAL station object
            EtStation gc = sys.stationNameToObject("GRAND_CENTRAL");

            // attach to GRAND_CENTRAL
            EtAttachment att = sys.attach(gc);

            EtEvent[]   mevs;
            EtEventImpl realEvent;
            int         startingVal = 0;
            long        t1, t2, time, totalT=0L, totalCount=0L, count=0L;
            double      rate, avgRate;
            ByteBuffer  fakeDataBuf = ByteBuffer.allocate(size);

            // create control array of correct size
            int[] con = new int[EtConstants.stationSelectInts];
            for (int i=0; i < EtConstants.stationSelectInts; i++) {
                con[i] = i+1;
            }

            // keep track of time for event rate calculations
            t1 = System.currentTimeMillis();

            while (true) {
                // Get array of new events (don't allocate local mem if blasting)
                mevs = sys.newEvents(att, Mode.SLEEP, blast, 0, chunk, size, group);
                count += mevs.length;

                // If blasting data (and remote), don't write anything,
                // just use what's in fixed (fake) buffer.
                if (blast) {
                    for (EtEvent ev : mevs) {
                        realEvent = (EtEventImpl) ev;
                        realEvent.setDataBuffer(fakeDataBuf);

                        // set data length
                        ev.setLength(size);
                    }
                }
                // Write data, set control values here
                else if (writeData) {
                    for (int j = 0; j < mevs.length; j++) {
                        // Put integer (j + startingVal) into data buffer
                        if (bigEndian) {
                            mevs[j].getDataBuffer().putInt(j + startingVal);
                            mevs[j].setByteOrder(ByteOrder.BIG_ENDIAN);
                        }
                        else {
                            mevs[j].getDataBuffer().putInt(Integer.reverseBytes(j + startingVal));
                            mevs[j].setByteOrder(ByteOrder.LITTLE_ENDIAN);
                        }

                        // Set data length to be full buf size even though we only wrote 1 int
                        mevs[j].setLength(size);

                        // Set event's control array
                        mevs[j].setControl(con);
                    }
                    startingVal += mevs.length;
                }
                else {
                    for (EtEvent ev : mevs) {
                        ev.setLength(size);
                    }
                }

                if (delay > 0) Thread.sleep(delay);


                // Put events back into ET system
                sys.putEvents(att, mevs);


                // calculate the event rate
                t2 = System.currentTimeMillis();
                time = t2 - t1;

                if (time > 5000) {
                    // reset things if necessary
                    if ( (totalCount >= (Long.MAX_VALUE - count)) ||
                         (totalT >= (Long.MAX_VALUE - time)) )  {
                        totalT = totalCount = count = 0L;
                        t1 = t2;
                        continue;
                    }

                    rate = 1000.0 * ((double) count) / time;
                    totalCount += count;
                    totalT += time;
                    avgRate = 1000.0 * ((double) totalCount) / totalT;
                    // Event rates
                    System.out.println("Events = " + String.format("%.3g", rate) +
                                       " Hz,    avg = " + String.format("%.3g", avgRate));

                    // Data rates
                    rate    = ((double) count) * size / time;
                    avgRate = ((double) totalCount) * size / totalT;
                    System.out.println("  Data = " + String.format("%.3g", rate) +
                                               " kB/s,  avg = " + String.format("%.3g", avgRate) + "\n");

                    count = 0L;
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
