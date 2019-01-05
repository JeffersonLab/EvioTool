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
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*/

package org.jlab.coda.et;

import java.lang.*;
import java.util.*;
import java.net.*;
import org.jlab.coda.et.exception.*;

/**
 * This class defines a set of configuration parameters used to open an ET system.
 *
 * @author Carl Timmer
 */
public class EtSystemOpenConfig {

    /** Broadcast address. */
    static public final String broadcastIP = "255.255.255.255";


    /** ET system name. */
    private String name;

    /** Either ET system host name or destination of broadcasts and multicasts. */
    private String host;

    /** Network interface used for actually connecting to ET system. */
    private String networkInterface;

    /** Are we broadcasting to find ET system? */
    private boolean broadcasting;

    /** Broadcast addresses. */
    private HashSet<String>  broadcastAddrs;

    /** Multicast addresses. */
    private HashSet<String>  multicastAddrs;

    /**
     * If true, only connect to ET systems with sockets (remotely). If false, try
     * opening any local C-based ET systems using mapped memory and JNI interface.
     */
    private boolean connectRemotely;

    /**
     * Means used to contact an ET system over network. The possible values are
     * @link Constants#broadcast} for broadcasting, {@link EtConstants#multicast}
     * for multicasting, {@link EtConstants#direct} for connecting directly to the
     * ET tcp server, and {@link EtConstants#broadAndMulticast} for using both
     * broadcasting and multicasting.
     */
    private int networkContactMethod;

    /** UDP port number for broad/multicasting or sending udp packets to known hosts. */
    private int udpPort;

    /** TCP server port number of the ET system. */
    private int tcpPort;

    /** Time-to-live value for multicasting. */
    private int ttl;

    /**
     * Policy on what to do about multiple responding ET systems to a broadcast
     * or multicast. The possible values are {@link EtConstants#policyFirst} which
     * chooses the first ET system to respond, {@link EtConstants#policyLocal}
     * which chooses the first local ET system to respond and if none then the
     * first response, or {@link EtConstants#policyError} which throws an
     * EtTooManyException exception.
     */
    private int responsePolicy;

    /** If no ET system is available, how many milliseconds do we wait while trying to open it? */
    private long waitTime;

    /** TCP send buffer size in bytes. */
    private int tcpSendBufSize;

    /** TCP receive buffer size in bytes. */
    private int tcpRecvBufSize;

    /**
     * TCP socket's no-delay setting.
     * <code>True</code> if no delay, else <code>false</code>.
     */
    private boolean noDelay;


    /**
     * No arg constructor. Will broadcast to local subnet addresses,
     * but ET name and host need to be set explicitly.
     */
    public EtSystemOpenConfig() {
        // some default values
        broadcasting = true;
        connectRemotely = false;
        networkContactMethod = EtConstants.broadcast;
        udpPort = EtConstants.udpPort;
        tcpPort = EtConstants.serverPort;
        ttl = 32;
        responsePolicy = EtConstants.policyFirst;
        multicastAddrs = new HashSet<String>(10);
        broadcastAddrs = new HashSet<String>(EtUtils.getAllBroadcastAddresses());
    }


    /**
     * Most general constructor for creating a new EtSystemOpenConfig object.
     *
     * @param etName ET system name
     * @param hostName may open an ET system on the given host which could be the:
     *                 1) actual ET system's host name,
     *                 2) dotted decimal address of ET system's host, or
     *                 3) general location of ET system such as {@link EtConstants#hostAnywhere},
     *                   {@link EtConstants#hostLocal}, or {@link EtConstants#hostRemote}
     * @param bAddrs collection of broadcast addresses (as Strings) to broadcast on in order to
     *               find ET system. If = null, set to list of all local subnets' broadcast addresses.
     * @param mAddrs collection of multicast addresses (as Strings) to multicast to in order to
     *               find ET system. If = null, set to default which is a list containing only
     *               EtConstants.multicastAddr = 239.200.0.0.
     * @param remoteOnly <code>true</code> if talking to ET system only through sockets
     *                   (as if remote), or <code>false</code> if also using JNI/shared memory
     *                   for talking to local C-based ET systems
     * @param method means used to contact an ET system over the network:
     *               {@link EtConstants#broadcast}, {@link EtConstants#multicast},
     *               {@link EtConstants#direct}, or {@link EtConstants#broadAndMulticast}
     * @param tPort  TCP server port number of the ET system. If = 0, set to default which is
     *               EtConstants.serverPort = 11111.
     * @param uPort  UDP port number for broadcasting or sending udp packets to known hosts.
     *               If = 0, set to default which is EtConstants.udpPort = 11111.
     * @param ttlNum Time-to_live value for multicasting. If = 0, set to default which is 32.
     * @param policy policy on what to do about multiple responding ET systems to
     *               a broadcast or multicast: {@link EtConstants#policyFirst},
     *               {@link EtConstants#policyLocal}, or {@link EtConstants#policyError}
     *
     * @throws EtException
     *     if null, blank, or too-long etName;
     *     if null or blank hostName;
     *     if method value is not valid;
     *     if method is direct and no actual host name was specified;
     *     if port numbers are < 1024 or > 65535 (except if = 0 which uses default value);
     *     if ttl is < 0 or > 254;
     *     if policy value is not valid
     */
    public EtSystemOpenConfig(String etName, String hostName,
                             Collection<String> bAddrs, Collection<String> mAddrs,
                             boolean remoteOnly,
                             int method, int tPort, int uPort,
                             int ttlNum, int policy)
            throws EtException {

        name = etName;
        if (etName == null || etName.equals("")) {
            throw new EtException("Bad ET system name");
        }

        if (etName.length() > EtConstants.fileNameLengthMax) {
            throw new EtException("ET system name too long (> " + EtConstants.fileNameLengthMax + " chars)");
        }

        host = hostName;
        if (host == null || host.equals("")) {
            throw new EtException("Bad host or location name");
        }

        if ((mAddrs == null) || (mAddrs.size() < 1)) {
            multicastAddrs = new HashSet<String>(10);
            multicastAddrs.add(EtConstants.multicastAddr);
        }
        else {
            multicastAddrs = new HashSet<String>(mAddrs);
        }

        connectRemotely = remoteOnly;
        
        if ((method != EtConstants.multicast) &&
            (method != EtConstants.broadcast) &&
            (method != EtConstants.broadAndMulticast) &&
            (method != EtConstants.direct))     {
            throw new EtException("Bad contact method value");
        }
        else {
            networkContactMethod = method;
        }

        // Do we broadcast?
        broadcasting = networkContactMethod == EtConstants.broadcast ||
                       networkContactMethod == EtConstants.broadAndMulticast;

        if ((bAddrs == null) || (bAddrs.size() < 1)) {
            // If we're broadcasting, need some addresses ...
            if (broadcasting) {
                broadcastAddrs = new HashSet<String>(EtUtils.getAllBroadcastAddresses());
            }
            else {
                broadcastAddrs = new HashSet<String>(10);
            }
        }
        else {
            broadcastAddrs = new HashSet<String>(bAddrs);
        }

        // inconsistencies?
        if (networkContactMethod == EtConstants.direct) {
            if (host.equals(EtConstants.hostRemote) ||
                host.equals(EtConstants.hostAnywhere)) {
                throw new EtException("Need to specify an actual host name");
            }
        }

        if ((uPort < 1024) || (uPort > 65535)) {
            if (uPort == 0) {
                uPort = EtConstants.udpPort;
            }
            else {
                throw new EtException("Bad UDP port value");
            }
        }
        udpPort = uPort;

        if ((tPort < 1024) || (tPort > 65535)) {
            if (tPort == 0) {
                tPort = EtConstants.serverPort;
            }
            else {
                throw new EtException("Bad TCP port value");
            }
        }
        tcpPort = tPort;


        if ((ttlNum < 1) || (ttlNum > 254)) {
            if (ttlNum == 0) {
                ttlNum = EtConstants.multicastTTL;
            }
            else {
                throw new EtException("Bad TTL value");
            }
        }
        ttl = ttlNum;


        if ((policy != EtConstants.policyFirst) &&
            (policy != EtConstants.policyLocal) &&
            (policy != EtConstants.policyError))  {
            throw new EtException("Bad policy value");
        }
        responsePolicy = policy;

        if ((host.equals(EtConstants.hostRemote)) &&
            (policy == EtConstants.policyLocal)) {
            // stupid combination of settings
            throw new EtException("Policy value cannot be local if host is remote");
        }
    }


    /**
     * Constructor for broadcasting on all subnets. First responder is chosen.
     *
     * @param etName ET system name
     * @param hostName may open an ET system on the given host which could be the:
     *                 1) actual ET system's host name,
     *                 2) dotted decimal address of ET system's host, or
     *                 3) general location of ET system such as {@link EtConstants#hostAnywhere},
     *                   {@link EtConstants#hostLocal}, or {@link EtConstants#hostRemote}
     *
     * @throws EtException
     *     if args are null or blank;
     */
    public EtSystemOpenConfig(String etName, String hostName)
            throws EtException {
        this (etName, hostName, null, null, false, EtConstants.broadcast,
              EtConstants.serverPort, EtConstants.udpPort,
              EtConstants.multicastTTL, EtConstants.policyFirst);
    }


    /**
     * Constructor for broadcasting on all subnets. First responder is chosen.
     *
     * @param etName ET system name
     * @param uPort  UDP port number to broadcasting to.
     *               If = 0, set to default which is EtConstants.udpPort = 11111.
     * @param hostName may open an ET system on the given host which could be the:
     *                 1) actual ET system's host name,
     *                 2) dotted decimal address of ET system's host, or
     *                 3) general location of ET system such as {@link EtConstants#hostAnywhere},
     *                   {@link EtConstants#hostLocal}, or {@link EtConstants#hostRemote}
     *
     * @throws EtException
     *     if no broadcast addresses were specified;
     *     if port number is < 1024 or > 65535
     */
    public EtSystemOpenConfig(String etName, int uPort, String hostName)
            throws EtException {
        this (etName, hostName, null, null, false, EtConstants.broadcast,
              EtConstants.serverPort, uPort,
              EtConstants.multicastTTL, EtConstants.policyFirst);
    }


    /**
     * Constructor for broadcasting on given subnets. First responder is chosen.
     *
     * @param etName ET system name
     * @param uPort  UDP port number to broadcasting to.
     *               If = 0, set to default which is EtConstants.udpPort = 11111.
     * @param hostName may open an ET system on the given host which could be the:
     *                 1) actual ET system's host name,
     *                 2) dotted decimal address of ET system's host, or
     *                 3) general location of ET system such as {@link EtConstants#hostAnywhere},
     *                   {@link EtConstants#hostLocal}, or {@link EtConstants#hostRemote}
     * @param bAddrs collection of broadcast addresses (as Strings) to broadcast on in order to
     *               find ET system. If = null, set to list of all local subnets' broadcast addresses.
     *
     * @throws EtException
     *     if no broadcast addresses were specified;
     *     if port number is < 1024 or > 65535
     */
    public EtSystemOpenConfig(String etName, int uPort, String hostName,
                              Collection<String> bAddrs)
            throws EtException {
        this (etName, hostName, bAddrs, null, false, EtConstants.broadcast,
              EtConstants.serverPort, uPort,
              EtConstants.multicastTTL, EtConstants.policyFirst);
    }


    /**
     * Constructor for multicasting. First responder is chosen.
     *
     * @param etName ET system name
     * @param hostName may open an ET system on the given host which could be the:
     *                 1) actual ET system's host name,
     *                 2) dotted decimal address of ET system's host, or
     *                 3) general location of ET system such as {@link EtConstants#hostAnywhere},
     *                   {@link EtConstants#hostLocal}, or {@link EtConstants#hostRemote}
     * @param mAddrs collection of multicast addresses (as Strings) to multicast to in order to
     *               find ET system. If = null, set to default which is a list containing only
     *               EtConstants.multicastAddr = 239.200.0.0.
     * @param mPort  Port number to multicast to. If = 0, set to default which is
     *               EtConstants.udpPort = 11111.
     * @param ttlNum Time-to_live value for multicasting. If = 0, set to default which is 32.
     *
     * @throws EtException
     *     if no multicast addresses were specified;
     *     if port number is < 1024 or > 65535;
     *     if ttl is < 0 or > 254
     */
    public EtSystemOpenConfig(String etName, String hostName,
                             Collection<String> mAddrs, int mPort, int ttlNum)
            throws EtException {
        this (etName, hostName, null, mAddrs, false, EtConstants.multicast,
              EtConstants.serverPort, mPort,
              ttlNum, EtConstants.policyFirst);
    }


  /**
     * Constructor for connecting directly to tcp server. First responder is
     * chosen.
     *
     * @param etName ET system name
     * @param hostName ET system host name
     * @param tPort  TCP server port number of the ET system. If = 0, set to default which is
     *               EtConstants.serverPort = 11111.
     *
     * @throws EtException
     *     if no actual host name was specified;
     *     if port number is < 1024 or > 65535
     */
    public EtSystemOpenConfig(String etName, String hostName, int tPort)
            throws EtException {
        this (etName, hostName, null, null, false, EtConstants.direct,
              tPort, EtConstants.udpPort,
              EtConstants.multicastTTL, EtConstants.policyFirst);
    }


    /**
     * Constructor to create a new EtSystemOpenConfig object from another.
     * @param config EtSystemOpenConfig object from which to create a new configuration
     */
    public EtSystemOpenConfig(EtSystemOpenConfig config) {
        name                 = config.name;
        host                 = config.host;
        broadcastAddrs       = config.getBroadcastAddrs();
        multicastAddrs       = config.getMulticastAddrs();
        networkContactMethod = config.networkContactMethod;
        connectRemotely      = config.connectRemotely;
        udpPort              = config.udpPort;
        tcpPort              = config.tcpPort;
        ttl                  = config.ttl;
        responsePolicy       = config.responsePolicy;
        waitTime             = config.waitTime;
        networkInterface     = config.networkInterface;
        tcpRecvBufSize       = config.tcpRecvBufSize;
        tcpSendBufSize       = config.tcpSendBufSize;
        noDelay              = config.noDelay;
    }


    /**
     * Method to create a string representation of this object
     * @return  string representation of this object
     */
    public String toString() {
        StringBuilder builder = new StringBuilder(300);
        builder.append("      name = ");    builder.append(name);
        builder.append("\n      host = ");  builder.append(host);
        builder.append("\n   version = ");  builder.append(EtConstants.version);
        builder.append("\n  tcp port = ");  builder.append(tcpPort);
        builder.append("\n  udp port = ");  builder.append(udpPort);
        builder.append("\ntcp  nodelay = ");  builder.append(noDelay);
        builder.append("\ntcp recv buf = ");  builder.append(tcpRecvBufSize);
        builder.append("\ntcp send buf = ");  builder.append(tcpSendBufSize);
        builder.append("\nwait time (ms) = ");  builder.append(waitTime);
        builder.append("\nconnect remotely = ");  builder.append(connectRemotely);
        builder.append("\nnetwork IF = ");  builder.append(networkInterface);
        builder.append("\nttl = ");  builder.append(ttl);

        builder.append("\nmcast addrs = ");
        for (String s : multicastAddrs) {
            builder.append(s); builder.append(", ");
        }

        builder.append("\nbcast addrs = ");
        for (String s : broadcastAddrs) {
            builder.append(s); builder.append(", ");
        }

        builder.append("\nnetwork contact = ");
        switch (networkContactMethod) {
            case  EtConstants.broadcast:
                builder.append("broadcast");
                break;
            case  EtConstants.multicast:
                builder.append("multicast");
                break;

            case  EtConstants.direct:
                builder.append("direct");
                break;

            case  EtConstants.broadAndMulticast:
                builder.append("broad & multicast");
                break;

            default:
        }

        builder.append("\nmultiple responses policy = ");

        switch (networkContactMethod) {
            case  EtConstants.policyFirst:
                builder.append("choose first");
                break;
            case  EtConstants.policyLocal:
                builder.append("choose local");
                break;

            case  EtConstants.policyError:
                builder.append("throw exception");
                break;

            default:
        }

        return builder.toString();
    }


    /**
     * Determine if vital parameters are set and all settings are self consistent.
     * @return <code>true</code> if setting are self consistent, else <code>false</code>
     */
    public boolean selfConsistent() {
        if (name == null || name.equals("") ||
            host == null || host.equals(""))  {
            return false;
        }

        if (name.length() > EtConstants.fileNameLengthMax) {
            return false;
        }

        if (networkContactMethod == EtConstants.direct) {
            if (host.equals(EtConstants.hostRemote) ||
                host.equals(EtConstants.hostAnywhere)) {
                return false;
            }
        }

        // stupid combination of settings
        if ((host.equals(EtConstants.hostRemote)) &&
            (responsePolicy == EtConstants.policyLocal)) {
            return false;
        }

        return true;
    }


    // Getters


    /** Gets the ET system name.
     *  @return ET system name */
    public String getEtName() {return name;}

    /** Gets the ET system host name or general location of ET system.
     *  @return ET system host name or general location of ET system */
    public String getHost() {return host;}

    /** Gets multicast addresses.
     *  @return multicast addresses */
    public HashSet<String> getBroadcastAddrs() {return (HashSet<String>) broadcastAddrs.clone();}

    /** Gets multicast addresses.
     *  @return multicast addresses */
    public HashSet<String> getMulticastAddrs() {return (HashSet<String>) multicastAddrs.clone();}

    /** Gets the means used to contact an ET system.
     *  @return means used to contact an ET system */
    public int getNetworkContactMethod() {return networkContactMethod;}

    /** Gets policy on what to do about multiple responding ET systems to a
     *  broadcast or multicast.
     *  @return policy on what to do about multiple responding ET systems to a broadcast or multicast */
    public int getResponsePolicy() {return responsePolicy;}

    /** Gets UDP port number for broad/multicasting or sending udp packets to known hosts.
     *  @return UDP port number for broad/multicast or sending udp packets to known hosts */
    public int getUdpPort() {return udpPort;}

    /** Gets TCP server port number of the ET system.
     *  @return TCP server port number of the ET system */
    public int getTcpPort() {return tcpPort;}

    /** Gets time-to-live value for multicasting.
     *  @return time-to-live value for multicasting */
    public int getTTL() {return ttl;}

    /** Gets the number of multicast addresses.
     *  @return the number of multicast addresses */
    public int getNumMulticastAddrs() {return multicastAddrs.size();}

    /** Are we broadcasting to find ET system?
     *  @return boolean indicating wether we are broadcasting to find ET system */
    public boolean isBroadcasting() {return broadcasting;}

    /** Set true if we're broadcasting to find ET system. */
    public void broadcasting(boolean on) {broadcasting = on;}

    /** Are we going to connect to an ET system remotely only (=true), or will
     *  we also try to use memory mapping and JNI with local C-based ET systems?
     *  @return <code>true</code> if connecting to ET system remotely only, else <code>false</code> */
    public boolean isConnectRemotely() {return connectRemotely;}

    /** If no ET system is available, the number of milliseconds we wait while trying to open it.
     *  @return the number of milliseconds we wait while trying to open ET system */
    public long getWaitTime() {return waitTime;}

    /** Get the network interface used for connecting to the ET system.
     *  @return the network interface used for connecting to the ET system or null if none specified. */
    public String getNetworkInterface() {
        return networkInterface;
    }

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


    // Setters


    /** Sets the ET system name.
     *  @param etName ET system name  */
    public void setEtName(String etName) {name = etName;}

    /** Sets the ET system host name or broad/multicast destination.
     *  May be dotted decimal format. May also be {@link EtConstants#hostAnywhere},
     *  {@link EtConstants#hostLocal}/"localhost", or {@link EtConstants#hostRemote}.
     *  @param hostName system host name or broad/multicast destination */
    public void setHost(String hostName) {host = hostName;}

    /** Removes a multicast address from the set.
     *  @param addr multicast address to be removed */
    public void removeMulticastAddr(String addr) {multicastAddrs.remove(addr);}

    /** Sets whether we going to connect to an ET system remotely only (=true), or whether
     *  we will try to use memory mapping and JNI with local C-based ET systems.
     *  @param connectRemotely <code>true</code> if connecting to ET system remotely only, else <code>false</code> */
    public void setConnectRemotely(boolean connectRemotely) {this.connectRemotely = connectRemotely;}

    /** If no ET system is available, set the number of milliseconds we wait while trying to open it.
     *  @param waitTime  the number of milliseconds we wait while trying to open ET system */
    public void setWaitTime(long waitTime) {
        if (waitTime < 0) {
            waitTime = 0;
        }
        this.waitTime = waitTime;
    }



    /**
     *  Adds a broadcast address to the set.
     *
     *  @param addr broadcast address to be added
     *  @throws EtException if the address is not a broadcast address
     */
    public void addBroadcastAddr(String addr) throws EtException {
        try {InetAddress.getByName(addr);}
        catch (UnknownHostException ex) {
            throw new EtException("not a broadcast address");
        }

        broadcastAddrs.add(addr);
    }


    /**
     *  Sets the collection of broadcast addresses.
     *
     *  @param addrs collection of broadcast addresses (as Strings) or null
     *  @throws EtException if one of the addresses is not a broadcast address
     */
    public void setBroadcastAddrs(Collection<String> addrs) throws EtException {
        if (addrs == null) {
            broadcastAddrs = new HashSet<String>(10);
            return;
        }

        for (String addr : addrs) {
            try {InetAddress.getByName(addr);}
            catch (UnknownHostException ex) {
                throw new EtException("not a broadcast address");
            }
        }
        broadcastAddrs = new HashSet<String>(addrs);
    }


    /**
     *  Adds a multicast address to the set.
     *
     *  @param addr multicast address to be added
     *  @throws EtException if the address is not a multicast address
     */
    public void addMulticastAddr(String addr) throws EtException {
        InetAddress inetAddr;
        try {inetAddr = InetAddress.getByName(addr);}
        catch (UnknownHostException ex) {
            throw new EtException("not a multicast address");
        }

        if (!inetAddr.isMulticastAddress()) {
            throw new EtException("not a multicast address");
        }
        multicastAddrs.add(addr);
    }


    /**
     *  Sets the collection of multicast addresses.
     *
     *  @param addrs collection of multicast addresses (as Strings) or null
     *  @throws EtException if one of the addresses is not a multicast address
     */
    public void setMulticastAddrs(Collection<String> addrs) throws EtException {
        if (addrs == null) {
            multicastAddrs = new HashSet<String>(10);
            return;
        }

        InetAddress inetAddr;
        for (String addr : addrs) {
            try {inetAddr = InetAddress.getByName(addr);}
            catch (UnknownHostException ex) {
                throw new EtException("not a broadcast address");
            }
            if (!inetAddr.isMulticastAddress()) {
                throw new EtException(addr + " is not a multicast address");
            }
        }
        multicastAddrs = new HashSet<String>(addrs);
    }


    /**
     *  Sets the means or method of contacting an ET system. Its values may be
     *  {@link EtConstants#broadcast} for broadcasting, {@link EtConstants#multicast}
     *  for multicasting, {@link EtConstants#direct} for connecting directly to the
     *  ET tcp server, and {@link EtConstants#broadAndMulticast} for using both
     *  broadcasting and multicasting.
     *
     *  @param method means or method of contacting an ET system
     *  @throws EtException if the argument has a bad value
     */
    public void setNetworkContactMethod(int method) throws EtException {
        if ((method != EtConstants.multicast) &&
            (method != EtConstants.broadcast) &&
            (method != EtConstants.broadAndMulticast) &&
            (method != EtConstants.direct))     {
            throw new EtException("bad contact method value");
        }
        networkContactMethod = method;
    }


    /**
     *  Sets the policy on what to do about multiple responding ET systems to a
     *  broadcast or multicast. The possible values are
     *  {@link EtConstants#policyFirst} which chooses the first ET system to
     *  respond, {@link EtConstants#policyLocal} which chooses the first local ET
     *  system to respond and if none then the first response, or
     *  {@link EtConstants#policyError} which throws an EtTooManyException
     *  exception.
     *
     *  @param policy policy on what to do about multiple responding ET systems
     *  @throws EtException
     *     if the argument has a bad value or if the policy says to choose a local
     *     ET system but the host is set to chose a remote system.
     */
    public void setResponsePolicy(int policy) throws EtException {
        if ((policy != EtConstants.policyFirst) &&
            (policy != EtConstants.policyLocal) &&
            (policy != EtConstants.policyError))  {
            throw new EtException("bad policy value");
        }
        
        if ((host.equals(EtConstants.hostRemote)) &&
            (policy == EtConstants.policyLocal)) {
            // stupid combination of settings
            throw new EtException("policy value cannot be local if host is remote");
        }
        responsePolicy = policy;
    }


    /**
     *  Sets the UDP port number for broad/multicasting and sending udp packets to known hosts.
     *
     *  @param port UDP port number for broad/multicasting and sending udp packets to known hosts;
     *              set to 0 for default = {@link EtConstants#udpPort}.

     *  @throws EtException if the port number is < 1024 or > 65535
     */
    public void setUdpPort(int port) throws EtException {
        if (port == 0) {
            udpPort = EtConstants.udpPort;
            return;
        }
        if ((port < 1024) || (port > 65535)) {
            throw new EtException("bad UDP port value");
        }
        udpPort = port;
    }


    /**
     *  Sets the TCP server port number of the ET system.
     *
     *  @param port TCP server port number of the ET system;
     *              set to 0 for default = {@link EtConstants#serverPort}.

     *  @throws EtException if the port number is < 1024 or > 65535
     */
    public void setTcpPort(int port) throws EtException {
        if (port == 0) {
            tcpPort = EtConstants.serverPort;
            return;
        }
        else if ((port < 1024) || (port > 65535)) {
            throw new EtException("bad TCP port value");
        }
        tcpPort = port;
    }


    /**
     *  Sets the time-to-live value for multicasting.
     *
     *  @param ttlNum time-to-live value for multicasting (-1 < ttl < 255);
     *                set to 0 for default = {@link EtConstants#multicastTTL}.
     *  @throws EtException if the port number is < 0 or > 254
     */
    public void setTTL(int ttlNum) throws EtException {
        if (ttlNum == 0) {
            ttl = EtConstants.multicastTTL;
            return;
        }
        else if ((ttlNum < 0) || (ttlNum > 254)) {
            throw new EtException("bad TTL value");
        }
        ttl = ttlNum;
    }

    /**
     * Set the network interface (in dotted-decimal format) used for connecting to the ET system.
     * The interface may be specified by giving a specific IP address or a broadcast (subnet)
     * address. If it is a broadcast address, it will convert that to the specific IP associated
     * with it in other parts of the code.
     *
     * @param networkInterface the network interface used for connecting to the ET system,
     *                         or null if none specified. Address may be either a specific
     *                         or broadcast IP dotted-decimal.
     * @throws EtException
     *    if the interface is not in dotted-decimal format or is an invalid IP address.
     */
    public void setNetworkInterface(String networkInterface) throws EtException {
        if (networkInterface != null) {
            // Check for dot-decimal format
            byte[] addr = EtUtils.isDottedDecimal(networkInterface);
            if (addr == null) {
                throw new EtException("not in dot-decimal (presentation) format");
            }

            try {InetAddress.getByName(networkInterface);}
            catch (UnknownHostException ex) {
                throw new EtException("not a valid network interface");
            }
        }
        this.networkInterface = networkInterface;
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


}
