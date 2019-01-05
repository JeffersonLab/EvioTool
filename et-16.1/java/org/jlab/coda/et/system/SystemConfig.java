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

package org.jlab.coda.et.system;

import java.lang.*;
import java.util.*;
import java.net.*;
import org.jlab.coda.et.exception.*;
import org.jlab.coda.et.EtConstants;

/**
 * This class defines a configuration for the creation of an ET system.
 *
 * @author Carl Timmer
 */

public class SystemConfig {

    /** Total number of events. */
    private int numEvents;

    /** Size of the "normal" event in bytes. This is the memory allocated to each
     *  event upon starting up the ET system. */
    private int eventSize;

    /** Number of events in each group. Used with multiple producers who want to
     * guarantee available events for each producer. */
    private int[] groups;

    /** Maximum number of station. */
    private int stationsMax;

    /** Maximum number of attachments. */
    private int attachmentsMax;

    /**
     *  Debug level. This may have values of {@link org.jlab.coda.et.EtConstants#debugNone} meaning
     *  print nothing, {@link org.jlab.coda.et.EtConstants#debugSevere} meaning print only the
     *  severest errors, {@link org.jlab.coda.et.EtConstants#debugError} meaning print all errors,
     *  {@link org.jlab.coda.et.EtConstants#debugWarn} meaning print all errors and warnings, and
     *  finally {@link org.jlab.coda.et.EtConstants#debugInfo} meaning print all errors, warnings,
     *  and informative messages.
     */
    private int debug;

    /** ET server's TCP send buffer size in bytes. */
    private int tcpSendBufSize;

    /** ET server's TCP receive buffer size in bytes. */
    private int tcpRecvBufSize;

    /**
     * ET server's socket's no-delay setting.
     * <code>True</code> if no delay, else <code>false</code>.
     */
    private boolean noDelay;

    /** UDP port number for thread responding to users' broad/multicasts looking for the
     *  ET system. */
    private int udpPort;

    /** TCP port number for the thread establishing connections with users,
     *  otherwise referred to as the ET server thread. */
    private int serverPort;

    /** Set of all multicast addresses to listen on (in String form). */
    private HashSet<InetAddress> multicastAddrs;


    /**
     * Constructor that creates a new SystemConfig object using default parameters.
     * The default parameters are:
     *      number of events          = {@link org.jlab.coda.et.EtConstants#defaultNumEvents},
     *      event size                = {@link org.jlab.coda.et.EtConstants#defaultEventSize},
     *      max number of stations    = {@link org.jlab.coda.et.EtConstants#defaultStationsMax},
     *      max number of attachments = {@link org.jlab.coda.et.EtConstants#defaultAttsMax},
     *      debug level               = {@link org.jlab.coda.et.EtConstants#debugError},
     *      udp port                  = {@link org.jlab.coda.et.EtConstants#udpPort},
     *      server (tcp) port         = {@link org.jlab.coda.et.EtConstants#serverPort}, and
     */
    public SystemConfig () {
        numEvents       = EtConstants.defaultNumEvents;
        eventSize       = EtConstants.defaultEventSize;
        stationsMax     = EtConstants.defaultStationsMax;
        attachmentsMax  = EtConstants.defaultAttsMax;
        debug           = EtConstants.debugError;
        udpPort         = EtConstants.udpPort;
        serverPort      = EtConstants.serverPort;
        multicastAddrs  = new HashSet<InetAddress>(10);
        // by default there is one group with all events in it
        groups          = new int[1];
        groups[0]       = numEvents;
    }

    /** Constructor that creates a new SystemConfig object from an existing one. */
    public SystemConfig (SystemConfig config) {
        numEvents       = config.numEvents;
        eventSize       = config.eventSize;
        stationsMax     = config.stationsMax;
        attachmentsMax  = config.attachmentsMax;
        debug           = config.debug;
        udpPort         = config.udpPort;
        tcpRecvBufSize  = config.tcpRecvBufSize;
        tcpSendBufSize  = config.tcpSendBufSize;
        noDelay         = config.noDelay;
        serverPort      = config.serverPort;
        multicastAddrs  = new HashSet<InetAddress>(config.multicastAddrs);
        groups          = config.groups.clone();
    }


    // public gets


    /** Get the total number of events.
     *  @return total number of events */
    public int getNumEvents() {return numEvents;}

    /** Get the size of the normal events in bytes.
     *  @return size of normal events in bytes */
    public int getEventSize() {return eventSize;}

    /** Get the array of how many events in each group.
     *  @return array of how many events in each group */
    public int[] getGroups() {return groups.clone();}

    /** Get the maximum number of stations.
     *  @return maximum number of stations */
    public int getStationsMax() {return stationsMax;}

    /** Get the maximum number of attachments.
     *  @return maximum number of attachments */
    public int getAttachmentsMax() {return attachmentsMax;}

    /** Get the debug level.
     *  @return debug level */
    public int getDebug() {return debug;}

    /** Get the TCP receive buffer size in bytes.
     *  @return TCP receive buffer size in bytes */
    public int getTcpSendBufSize() {
        return tcpSendBufSize;
    }

    /** Get the TCP send buffer size in bytes.
     *  @return TCP send buffer size in bytes */
    public int getTcpRecvBufSize() {
        return tcpRecvBufSize;
    }

    /** Get the TCP no-delay setting.
     *  @return TCP no-delay setting */
    public boolean isNoDelay() {
        return noDelay;
    }

    /** Get the udp port number.
     *  @return udp port number */
    public int getUdpPort() {return udpPort;}

    /** Get the tcp server port number.
     *  @return tcp server port number */
    public int getServerPort() {return serverPort;}

    /** Get the set of multicast addresses.
     *  @return set of multicast addresses */
    public Set<InetAddress> getMulticastAddrs() {return new HashSet<InetAddress>(multicastAddrs);}

    /** Get the multicast addresses as a String array.
     *  @return multicast addresses as a String array */
    public String[] getMulticastStrings() {
        if (multicastAddrs == null) {
            return null;
        }
        int index = 0;
        String[] addrs = new String[multicastAddrs.size()];
        for (InetAddress addr : multicastAddrs) {
            addrs[index++] = addr.getHostAddress();
        }
        return addrs;
    }


    // public adds, removes


    /**
     * Adds a multicast address to the set.
     * @param mCastAddr multicast address
     * @throws EtException
     *     if the argument is not a multicast address
     */
    public void addMulticastAddr(String mCastAddr) throws EtException {
        InetAddress addr;
        try {addr = InetAddress.getByName(mCastAddr);}
        catch (UnknownHostException ex) {
            throw new EtException("not a multicast address");
        }

        if (!addr.isMulticastAddress()) {
            throw new EtException("not a multicast address");
        }
        multicastAddrs.add(addr);
        return;
    }


    /**
     * Removes a multicast address from the set.
     * @param addr multicast address
     */
    public void removeMulticastAddr(String addr) {
        InetAddress ad;
        try {ad = InetAddress.getByName(addr);}
        catch (UnknownHostException ex) {
            return;
        }
        multicastAddrs.remove(ad);
        return;
    }


  // public sets


    /**
     * Set the total number of events.
     * @param num total number of events
     * @throws EtException
     *     if the argument is less than 1
     */
    public void setNumEvents(int num) throws EtException {
        if (num < 1) {
            throw new EtException("must have 1 or more events");
        }
        numEvents = num;
        if (groups.length ==1) groups[0] = num;
    }


    /**
     * Set the event size in bytes.
     * @param size event size in bytes
     * @throws EtException
     *     if the argument is less than 1 byte
     */
    public void setEventSize(int size) throws EtException {
        if (size < 1) {
            throw new EtException("events must have at least one byte");
        }
        eventSize = size;
    }


    /**
     * Set the number of events in each group. Used with multiple producers who want to
     * guarantee available events for each producer.
     *
     * @param groups array defining number of events in each group
     * @throws EtException
     *     if the groups array is null, has length < 1 or values are not positive ints
     */
    public void setGroups(int[] groups) throws EtException {
        if (groups == null || groups.length < 1) {
            throw new EtException("events must have at least one group");
        }
        for (int num : groups) {
            if (num < 1) {
                throw new EtException("each event group must contain at least one event");
            }
        }

        this.groups = groups.clone();
    }

    /**
     * Set the maximum number of stations.
     * @param num maximum number of stations
     * @throws EtException
     *     if the argument is less than 2
     */
    public void setStationsMax(int num) throws EtException {
        if (num < 2) {
            throw new EtException("must have at least 2 stations");
        }
        stationsMax = num;
    }


    /**
     * Set the maximum number of attachments.
     * @param num maximum number of attachments
     * @throws EtException
     *     if the argument is less than 1
     */
    public void setAttachmentsMax(int num) throws EtException {
        if (num < 1) {
            throw new EtException("must be able to have at least one attachment");
        }
        attachmentsMax = num;
    }


    /**
     * Set the debug level.
     * @param level debug level
     * @throws EtException
     *     if the argument has a bad value
     */
    public void setDebug(int level) throws EtException {
        if ((level != EtConstants.debugNone)   &&
                (level != EtConstants.debugInfo)   &&
                (level != EtConstants.debugWarn)   &&
                (level != EtConstants.debugError)  &&
                (level != EtConstants.debugSevere))  {
            throw new EtException("bad debug value");
        }
        debug = level;
    }


    /**
     * Set the TCP send buffer size in bytes. A value of 0
     * means use the operating system default.
     *
     * @param tcpSendBufSize TCP send buffer size in bytes
     * @throws EtException
     *     if the argument is less than 0
     */
    public void setTcpSendBufSize(int tcpSendBufSize) throws EtException {
        if (tcpSendBufSize < 0) {
            throw new EtException("buffer size must be >= than 0");
        }
        this.tcpSendBufSize = tcpSendBufSize;
    }


    /**
     * Set the TCP receive buffer size in bytes. A value of 0
     * means use the operating system default.
     *
     * @param tcpRecvBufSize TCP receive buffer size in bytes
     * @throws EtException
     *     if the argument is less than 0
     */
    public void setTcpRecvBufSize(int tcpRecvBufSize) throws EtException {
        if (tcpRecvBufSize < 0) {
            throw new EtException("buffer size must be >= than 0");
        }
        this.tcpRecvBufSize = tcpRecvBufSize;
    }


    /**
     * Set the TCP no-delay setting. It is off by default.
     * @param noDelay TCP no-delay setting
     */
    public void setNoDelay(boolean noDelay) {
        this.noDelay = noDelay;
    }


    /**
     * Sets the udp port number.
     * @param port udp port number
     * @throws EtException
     *     if the argument is less than 1024
     */
    public void setUdpPort(int port) throws EtException {
        if (port < 1024) {
            throw new EtException("port number must be greater than 1023");
        }
        udpPort = port;
    }


    /**
     * Sets the tcp server port number.
     * @param port tcp server port number
     * @throws EtException
     *     if the argument is less than 1024
     */
    public void setServerPort(int port) throws EtException {
        if (port < 1024) {
            throw new EtException("port number must be greater than 1023");
        }
        serverPort = port;
    }


    /**
     * Checks configuration settings for consistency.
     * @return true if consistent, else false
     */
    public boolean selfConsistent() {
        // Check to see if the number of events in groups equal the total number of events
        int count = 0;
        for (int i : groups) {
            count += i;
        }
        if (count != numEvents) {
            System.out.println("events in groups != total event number");
            return false;
        }
        return true;
    }
}

