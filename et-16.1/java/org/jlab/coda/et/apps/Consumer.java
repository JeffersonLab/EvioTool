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
import java.util.HashSet;

import org.jlab.coda.et.*;
import org.jlab.coda.et.enums.Mode;
import org.jlab.coda.et.enums.Modify;


/**
 * This class is an example of an event consumer for an ET system.
 *
 * @author Carl Timmer
 */
public class Consumer {

    public Consumer() {}


    private static void usage() {
        System.out.println("\nUsage: java Consumer -f <et name> -s <station name>\n" +
                "                      [-h] [-v] [-nb] [-r] [-m] [-b] [-nd] [-read] [-dump]\n" +
                "                      [-host <ET host>] [-p <ET port>]\n" +
                "                      [-c <chunk size>] [-q <Q size>]\n" +
                "                      [-pos <station pos>] [-ppos <parallel station pos>]\n" +
                "                      [-i <interface address>] [-a <mcast addr>]\n" +
                "                      [-rb <buf size>] [-sb <buf size>]\n\n" +

                "       -f     ET system's (memory-mapped file) name\n" +
                "       -host  ET system's host if direct connection (default to local)\n" +
                "       -s     create station of this name\n" +
                "       -h     help\n\n" +

                "       -v     verbose output (also prints data if reading with -read)\n" +
                "       -read  read data (1 int for each event)\n" +
                "       -dump  dump events back into ET (go directly to GC) instead of put\n" +
                "       -c     number of events in one get/put array\n" +
                "       -r     act as remote (TCP) client even if ET system is local\n" +
                "       -p     port, TCP if direct, else UDP\n\n" +

                "       -nb    make station non-blocking\n" +
                "       -q     queue size if creating non-blocking station\n" +
                "       -pos   position of station (1,2,...)\n" +
                "       -ppos  position of station within a group of parallel stations (-1=end, -2=head)\n\n" +

                "       -i     outgoing network interface address (dot-decimal)\n" +
                "       -a     multicast address(es) (dot-decimal), may use multiple times\n" +
                "       -m     multicast to find ET (use default address if -a unused)\n" +
                "       -b     broadcast to find ET\n\n" +

                "       -rb    TCP receive buffer size (bytes)\n" +
                "       -sb    TCP send    buffer size (bytes)\n" +
                "       -nd    use TCP_NODELAY option\n\n" +

                "        This consumer works by making a direct connection to the\n" +
                "        ET system's server port and host unless at least one multicast address\n" +
                "        is specified with -a, the -m option is used, or the -b option is used\n" +
                "        in which case multi/broadcasting used to find the ET system.\n" +
                "        If multi/broadcasting fails, look locally to find the ET system.\n" +
                "        This program gets all events from the given station and puts them back.\n\n");
    }


    public static void main(String[] args) {

        int port=0, flowMode = EtConstants.stationSerial;
        int position=1, pposition=0, qSize=0, chunk=1, recvBufSize=0, sendBufSize=0;
        boolean dump=false, readData=false, noDelay=false;
        boolean blocking=true, verbose=false, remote=false;
        boolean broadcast=false, multicast=false, broadAndMulticast=false;
        HashSet<String> multicastAddrs = new HashSet<String>();
        String outgoingInterface=null, etName=null, host=null, statName=null;

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
            else if (args[i].equalsIgnoreCase("-nb")) {
                blocking = false;
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
            else if (args[i].equalsIgnoreCase("-read")) {
                readData = true;
            }
            else if (args[i].equalsIgnoreCase("-dump")) {
                dump = true;
            }
            else if (args[i].equalsIgnoreCase("-m")) {
                multicast = true;
            }
            else if (args[i].equalsIgnoreCase("-b")) {
                broadcast = true;
            }
            else if (args[i].equalsIgnoreCase("-s")) {
                statName = args[++i];
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
            else if (args[i].equalsIgnoreCase("-q")) {
                try {
                    qSize = Integer.parseInt(args[++i]);
                    if (qSize < 1) {
                        System.out.println("Queue size must be > 0.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper queue size number.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-pos")) {
                try {
                    position = Integer.parseInt(args[++i]);
                    if (position < 1) {
                        System.out.println("Position must be > 0.");
                        usage();
                        return;
                    }
                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper position number.");
                    usage();
                    return;
                }
            }
            else if (args[i].equalsIgnoreCase("-ppos")) {
                try {
                    pposition = Integer.parseInt(args[++i]);
                    if (pposition < -2 || pposition == 0) {
                        System.out.println("Parallel position must be > -3 and != 0.");
                        usage();
                        return;
                    }
System.out.println("Flow mode is parallel");
                    flowMode = EtConstants.stationParallel;
                    if      (pposition == -2) pposition = EtConstants.newHead;
                    else if (pposition == -1) pposition = EtConstants.end;

                }
                catch (NumberFormatException ex) {
                    System.out.println("Did not specify a proper parallel position number.");
                    usage();
                    return;
                }
            }
            else {
                usage();
                return;
            }
        }

        if (etName == null || statName == null) {
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
            System.out.println("Connect to ET using local IP addr = " + sys.getLocalAddress());

            // configuration of a new station
            EtStationConfig statConfig = new EtStationConfig();
            statConfig.setFlowMode(flowMode);
            if (!blocking) {
                statConfig.setBlockMode(EtConstants.stationNonBlocking);
                if (qSize > 0) {
                    statConfig.setCue(qSize);
                }
            }

            // create station
            EtStation stat = sys.createStation(statConfig, statName, position, pposition);

            // attach to new station
            EtAttachment att = sys.attach(stat);

            // array of events
            EtEvent[] mevs;

            int    len, num;
            long   t1=0L, t2=0L, time, totalT=0L, count=0L, totalCount=0L, bytes=0L, totalBytes=0L;
            double rate, avgRate;

            // keep track of time
            t1 = System.currentTimeMillis();

            while (true) {

                // get events from ET system
                //mevs = sys.getEvents(att, Mode.TIMED, Modify.ANYTHING, 2000, chunk);
                mevs = sys.getEvents(att, Mode.SLEEP, Modify.ANYTHING, 0, chunk);

                // example of reading & printing event data
                if (readData) {
                    for (EtEvent mev : mevs) {
                        // Get event's data buffer
                        ByteBuffer buf = mev.getDataBuffer();
                        num = buf.getInt(0);
                        len = mev.getLength();
                        bytes += len;
                        totalBytes += len;

                        if (verbose) {
                            System.out.println("    data (len = " + mev.getLength() + ") = " + num);

                            try {
                                // If using byte array you need to watch out for endianness
                                byte[] data = mev.getData();
                                int idata = EtUtils.bytesToInt(data,0);
                                System.out.println("data byte order = " + mev.getByteOrder());
                                if (mev.needToSwap()) {
                                    System.out.println("    data (len = " +len +
                                                               ") needs swapping, swapped int = " + Integer.reverseBytes(idata));
                                }
                                else {
                                    System.out.println("    data (len = " + len +
                                                               ")does NOT need swapping, int = " + idata);
                                }
                            }
                            catch (UnsupportedOperationException e) {
                            }

                            System.out.print("control array = {");
                            int[] con = mev.getControl();
                            for (int j : con) {
                                System.out.print(j + " ");
                            }
                            System.out.println("}");
                        }
                    }
                }
                else {
                    for (EtEvent mev : mevs) {
                        len = mev.getLength();
                        bytes += len;       // +52 for overhead
                        totalBytes += len;  // +52 for overhead
                    }
                    // For more overhead
                    // bytes += 20;
                    // totalBytes += 20;
                }

                // put events back into ET system
                if (!dump) {
                    sys.putEvents(att, mevs);
                }
                else {
                    // dump events back into ET system
                    sys.dumpEvents(att, mevs);
                }
                count += mevs.length;

                // calculate the event rate
                t2 = System.currentTimeMillis();
                time = t2 - t1;

                if (time > 5000) {
                    // reset things if necessary
                    if ( (totalCount >= (Long.MAX_VALUE - count)) ||
                         (totalT >= (Long.MAX_VALUE - time)) )  {
                        bytes = totalBytes = totalT = totalCount = count = 0L;
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
                    rate    = ((double) bytes) / time;
                    avgRate = ((double) totalBytes) / totalT;
                    System.out.println("  Data = " + String.format("%.3g", rate) +
                                               " kB/s,  avg = " + String.format("%.3g", avgRate) + "\n");

                    bytes = count = 0L;
                    t1 = System.currentTimeMillis();
                }
            }
        }
        catch (Exception ex) {
            System.out.println("Error using ET system as consumer");
            ex.printStackTrace();
        }
    }
    
}
