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

import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.HashSet;

import org.jlab.coda.et.data.*;
import org.jlab.coda.et.exception.*;
import org.jlab.coda.et.EtConstants;
import org.jlab.coda.et.EtSystemOpenConfig;
import org.jlab.coda.et.EtSystem;

/**
 * This class implements a monitor of an ET system. It opens the system,
 * receives all relevant data over a tcp connection, and prints it out.
 *
 * @author Carl Timmer
 */

public class EtMonitor {

    static int  period = 3; // seconds
    static long prevGcOut;

    public EtMonitor() {
    }


    private static void usage() {
        System.out.println("\nUsage: java EtMonitor -f <ET name> [-h] [-r] [-m] [-b]\n" +
                                   "                         [-host <ET host>][-t <period (sec)>]\n" +
                                   "                         [-p <ET port>] [-a <mcast addr>]\n\n" +

                                   "       -f     ET system's (memory-mapped file) name\n" +
                                   "       -host  ET system's host if direct connection (default to local)\n" +
                                   "       -h     help\n" +
                                   "       -t     period in seconds between updates\n" +
                                   "       -r     act as remote (TCP) client even if ET system is local\n\n" +

                                   "       -p     ET port (TCP for direct, UDP for broad/multicast)\n" +
                                   "       -a     multicast address(es) (dot-decimal), may use multiple times\n" +
                                   "       -m     multicast to find ET (use default address if -a unused)\n" +
                                   "       -b     broadcast to find ET\n\n" +

                                   "       This program display the current status of the ET system.\n");
    }


    public static void main(String[] args) {

        int port=0;
        boolean remote=false, broadcast=false, multicast=false, broadAndMulticast=false;
        HashSet<String> multicastAddrs = new HashSet<String>();
        String etName = null, host = null;

        try {
            for (int i = 0; i < args.length; i++) {
                if (args[i].equalsIgnoreCase("-f")) {
                    etName = args[++i];
                }
                else if (args[i].equalsIgnoreCase("-host")) {
                    host = args[++i];
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
                else if (args[i].equalsIgnoreCase("-t")) {
                    try {
                        period = Integer.parseInt(args[++i]);
                        if (period < 1) {
                            System.out.println("Period must be at least 1 second.");
                            usage();
                            return;
                        }
                    }
                    catch (NumberFormatException ex) {
                        System.out.println("Did not specify a proper period value.");
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

            config.setWaitTime(0);
            config.setEtName(etName);
            config.setResponsePolicy(EtConstants.policyError);

            if (remote) {
                System.out.println("Set as remote");
                config.setConnectRemotely(remote);
            }

            // create ET system object with verbose debugging output
            EtSystem sys = new EtSystem(config);
            sys.open();

            AllData etData = new AllData();

/*
      // timing
      long t1, t2;
      for (int j=0; j < 10; j++) {
        t1 = System.currentTimeMillis();
        for(int i=0; i < 2000; i++) {
          etData = sys.getData();
        }
        t2 = System.currentTimeMillis();
        double rate = 1000.0*2000.0/((double)(t2-t1));
        System.out.println("rate = " + rate + " Hz");
      }
*/

          while (true) {
              try {
                  etData = sys.getData();
                  display(sys, etData);
              }
              catch (EtException ex) {
                  ex.printStackTrace();
                  System.out.print("\n*****************************************\n");
                  System.out.print("*   Error getting data from ET system   *");
                  System.out.print("\n*****************************************\n");
              }
              Thread.sleep(period * 1000);
          }


      }
      catch (IOException ex) {
          System.out.println("Communication error with ET system:");
          ex.printStackTrace();
      }
      catch (Exception ex) {
          System.out.println("ERROR:");
          ex.printStackTrace();
      }

  }


    private static void display(EtSystem sys, AllData data)
  {
    int           end = 499, lang;
    boolean       blocking;
    double	  rate = 0.0;
    StringBuffer  str = new StringBuffer(end+1);

    str.append("  ET SYSTEM - (");
    str.append(data.sysData.getEtName());
    str.append(") (host ");
    str.append(sys.getHost());
    str.append(") (bits ");
    if (data.sysData.isBit64()) {
      str.append("64)\n");
    }
    else {
      str.append("32)\n");
    }
    str.append("              (tcp port ");
    str.append(data.sysData.getTcpPort());
    str.append(") (udp port ");
    str.append(data.sysData.getUdpPort());
    str.append(")\n              (pid ");
    str.append(data.sysData.getMainPid());
    str.append(") (lang ");
    lang = sys.getLanguage();
    if (lang == EtConstants.langJava) {
      str.append("Java) (period ");
    }
    else if (lang == EtConstants.langC) {
      str.append("C) (period ");
    }
    else if (lang == EtConstants.langCpp) {
      str.append("C++) (period ");
    }
    else {
      str.append("unknown) (period ");
    }
    str.append(period);
    str.append(" sec)\n");
    System.out.println(str.toString());
    str.delete(0, end);

    str.append("  STATIC INFO - maximum of:\n");
    str.append("    events(");
    str.append(data.sysData.getEvents());
    str.append("), event size(");
    str.append(data.sysData.getEventSize());
    str.append("), temps(");
    str.append(data.sysData.getTempsMax());
    str.append(")\n");
    str.append("    stations(");
    str.append(data.sysData.getStationsMax());
    str.append("), attaches(");
    str.append(data.sysData.getAttachmentsMax());
    str.append("), procs(");
    str.append(data.sysData.getProcessesMax());
    str.append(")\n");

    if (data.sysData.getInterfaces() > 0) {
      String[] ifAddrs = data.sysData.getInterfaceAddresses();
      str.append("    network interfaces(");
      str.append(ifAddrs.length);
      str.append(")  ");
      for (int i=0; i < ifAddrs.length; i++) {
        str.append(ifAddrs[i]);
        str.append(", ");
      }
      str.append("\n");
    }
    else {
      str.append("    network interfaces(0): none\n");
    }

    if (data.sysData.getMulticasts() > 0) {
      String[] mAddrs = data.sysData.getMulticastAddresses();
      str.append("    multicast addresses(");
      str.append(mAddrs.length);
      str.append(")  ");
      for (int i=0; i < mAddrs.length; i++) {
        str.append(mAddrs[i]);
        str.append(", ");
      }
      str.append("\n");
    }

    str.append("\n  DYNAMIC INFO - currently there are:\n");
    str.append("    processes(");
    str.append(data.sysData.getProcesses());
    str.append("), attachments(");
    str.append(data.sysData.getAttachments());
    str.append("), temps(");
    str.append(data.sysData.getTemps());
    str.append(")\n    stations(");
    str.append(data.sysData.getStations());
    str.append("), hearbeat(");
    str.append(data.sysData.getHeartbeat());
    str.append(")\n");
    System.out.println(str.toString());
    str.delete(0, end);

    str.append("  STATIONS:\n");

    for (int i=0; i < data.statData.length; i++) {
      str.append("    \"");
      str.append(data.statData[i].getName());
      str.append("\" (id = ");
      str.append(data.statData[i].getId());
      str.append(")\n      static info\n");

      if (data.statData[i].getStatus() == EtConstants.stationIdle)
        str.append("        status(IDLE), ");
      else
        str.append("        status(ACTIVE), ");

      if (data.statData[i].getFlowMode() == EtConstants.stationSerial) {
        str.append("flow(SERIAL), ");
      }
      else {
        str.append("flow(PARALLEL), ");
      }

      if (data.statData[i].getBlockMode() == EtConstants.stationBlocking) {
        str.append("blocking(YES), ");
        blocking = true;
      }
      else {
        str.append("blocking(NO), ");
        blocking = false;
      }

      if (data.statData[i].getUserMode() == EtConstants.stationUserMulti) {
        str.append("user(MULTI), ");
      }
      else {
        str.append("user(");
	str.append(data.statData[i].getUserMode());
	str.append("), ");
      }

      if (data.statData[i].getSelectMode() == EtConstants.stationSelectAll)
        str.append("select(ALL)\n");
      else if (data.statData[i].getSelectMode() == EtConstants.stationSelectMatch)
        str.append("select(MATCH)\n");
      else if (data.statData[i].getSelectMode() == EtConstants.stationSelectUser)
        str.append("select(USER)\n");
      else if (data.statData[i].getSelectMode() == EtConstants.stationSelectRRobin)
        str.append("select(RROBIN)\n");
      else
        str.append("select(EQUALCUE)\n");

      if (data.statData[i].getRestoreMode() == EtConstants.stationRestoreOut)
        str.append("        restore(OUT), ");
      else if (data.statData[i].getRestoreMode() == EtConstants.stationRestoreIn)
        str.append("        restore(IN), ");
      else
        str.append("        restore(GC), ");

      str.append("prescale(");
      str.append(data.statData[i].getPrescale());
      str.append("), cue(");
      str.append(data.statData[i].getCue());
      str.append("), ");

      str.append("select words(");
      int[] select = data.statData[i].getSelect();
      for (int j=0; j < select.length; j++) {
          str.append(select[j]);
          str.append(", ");
      }
      str.append(")");

      if (data.statData[i].getSelectMode() == EtConstants.stationSelectUser) {
        str.append("\n        lib = ");
        str.append(data.statData[i].getSelectLibrary());
        str.append(",  function = ");
        str.append(data.statData[i].getSelectFunction());
        str.append(",  class = ");
        str.append(data.statData[i].getSelectClass());
        str.append("");
      }

      System.out.println(str.toString());
      str.delete(0, end);

      // dynamic station info or info on active stations
      if (data.statData[i].getStatus() != EtConstants.stationActive) {
        System.out.println();
        continue;
      }

      str.append("      dynamic info\n");
      str.append("        attachments: total#(");
      int attsTotal = data.statData[i].getAttachments();
      str.append(attsTotal);
      str.append("),  ids(");

      int[] attIds = data.statData[i].getAttachmentIds();
      for (int j=0; j < attsTotal; j++) {
        str.append(attIds[j]);
          str.append(", ");
      }
      str.append(")\n");

      str.append("        input  list: cnt = ");
      str.append(data.statData[i].getInListCount());
      str.append(", events in = ");
      str.append(data.statData[i].getInListIn());

      // if blocking station and not grandcentral ...
      if (blocking && (data.statData[i].getId() != 0)) {
        str.append(", events try = ");
        str.append(data.statData[i].getInListTry());
      }
      str.append("\n");

      str.append("        output list: cnt = ");
      str.append(data.statData[i].getOutListCount());
      str.append(", events out = ");
      str.append(data.statData[i].getOutListOut());
      str.append("\n");

      System.out.println(str.toString());
      str.delete(0, end);

      // keep track of grandcentral data rate
      if (i==0) {
        rate = (data.statData[i].getOutListOut() - prevGcOut)/period;
        prevGcOut = data.statData[i].getOutListOut();
      }
    } // for (int i=0; i < data.statData.length; i++) {


    // user processes
    if (data.procData.length > 0) {
      str.append("  LOCAL USERS:\n");
      for (int i=0; i < data.procData.length; i++) {
        if (data.procData[i].getAttachments() < 1) {
          str.append("    process id# ");
          str.append(data.procData[i].getId());
          str.append(", # attachments(0), ");
        }
        else {
          str.append("    process id# ");
          str.append(data.procData[i].getId());
          str.append(", # attachments(");
          str.append(data.procData[i].getAttachments());
          str.append("), attach ids(");
          int[] atIds = data.procData[i].getAttachmentIds();
          for (int j=0; j < atIds.length; j++) {
            str.append(atIds[j]);
            str.append(", ");
          }
          str.append("), ");
        }
        str.append("pid(");
        str.append(data.procData[i].getPid());
        str.append("), hbeat(");
        str.append(data.procData[i].getHeartbeat());
        str.append(")\n");
      }
      System.out.println(str.toString());
      str.delete(0, end);
    }


    // user attachments
    if (data.attData.length > 0) {
        str.append("  ATTACHMENTS: len = ");
        str.append(data.attData.length);
        str.append("\n");
      for (int i=0; i < data.attData.length; i++) {
        str.append("    att #");
        str.append(data.attData[i].getId());
        str.append(", is at station(");
        str.append(data.attData[i].getStationName());
        str.append(") on host(");
        str.append(data.attData[i].getHost());
        str.append(") at pid(");
        str.append(data.attData[i].getPid());
        str.append(")\n");
        str.append("    proc(");
        str.append(data.attData[i].getProc());
        str.append("), ");
        if (data.attData[i].blocked()) {
          str.append("blocked(YES)");
        }
        else {
          str.append("blocked(NO)");
        }
        if (data.attData[i].quitting()) {
          str.append(", told to quit");
        }
        str.append("\n      events:  make(");
        str.append(data.attData[i].getEventsMake());
        str.append("), get(");
        str.append(data.attData[i].getEventsGet());
        str.append("), put(");
        str.append(data.attData[i].getEventsPut());
        str.append("), dump(");
        str.append(data.attData[i].getEventsDump());
        str.append(")");
        System.out.println(str.toString());
        str.delete(0, end);
      }
    }

    str.append("\n  EVENTS OWNED BY:\n");
    str.append("    system (");
    str.append(data.sysData.getEventsOwned());
    str.append("),");
    for (int i=0; i < data.attData.length; i++) {
      str.append("  att");
      str.append(data.attData[i].getId());
      str.append(" (");
      str.append(data.attData[i].getEventsOwned());
      str.append("),");
      if ((i+1)%6 == 0)
        str.append("\n    ");
    }
    str.append("\n\n");

    // Event rate
    str.append("  EVENT RATE of GC = ");
    str.append(rate);
    str.append(" events/sec\n\n");

    // idle stations
    str.append("  IDLE STATIONS:      ");
    for (int i=0; i < data.statData.length; i++) {
      if (data.statData[i].getStatus() == EtConstants.stationIdle) {
        str.append(data.statData[i].getName());
        str.append(", ");
      }
    }
    str.append("\n");

    // stations linked list
    str.append("  STATION CHAIN:      ");
    for (int i=0; i < data.statData.length; i++) {
      str.append(data.statData[i].getName());
      str.append(", ");
    }
    str.append("\n");


    if (lang != EtConstants.langJava) {
      // mutexes
      str.append("  LOCKED MUTEXES:     ");
      if (data.sysData.getMutex() == EtConstants.mutexLocked)
        str.append("system, ");
      if (data.sysData.getStatMutex() == EtConstants.mutexLocked)
        str.append("station, ");
      if (data.sysData.getStatAddMutex() == EtConstants.mutexLocked)
        str.append("add_station, ");

      for (int i=0; i < data.statData.length; i++) {
        if (data.statData[i].getMutex() == EtConstants.mutexLocked) {
          str.append(data.statData[i].getName());
        }
        if (data.statData[i].getInListMutex() == EtConstants.mutexLocked) {
          str.append(data.statData[i].getName());
          str.append("-in, ");
        }
        if (data.statData[i].getOutListMutex() == EtConstants.mutexLocked) {
          str.append(data.statData[i].getName());
          str.append("-out, ");
        }
      }
      str.append("\n");
    }

    str.append("\n*****************************************\n");
    System.out.println(str.toString());
    str.delete(0, end);

  }




}
