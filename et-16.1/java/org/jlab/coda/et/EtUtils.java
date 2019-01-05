/*----------------------------------------------------------------------------*
 *  Copyright (c) 2010        Jefferson Science Associates,                   *
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

package org.jlab.coda.et;

import org.jlab.coda.et.exception.EtException;

import java.net.*;
import java.nio.ByteOrder;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Collection of methods to help manipulate bytes in arrays.
 * @author timmer
 */
public class EtUtils {

    /**
     * Turn short into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data short to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @param dest array in which to store returned bytes
     * @param off offset into dest array where returned bytes are placed
     */
    public static void shortToBytes(short data, ByteOrder byteOrder, byte[] dest, int off) {

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            dest[off  ] = (byte)(data >>> 8);
            dest[off+1] = (byte)(data      );
        }
        else {
            dest[off  ] = (byte)(data      );
            dest[off+1] = (byte)(data >>> 8);
        }
    }


    /**
      * Turn int into byte array.
      * Avoids creation of new byte array with each call.
      *
      * @param data int to convert
      * @param byteOrder byte order of returned bytes (big endian if null)
      * @param dest array in which to store returned bytes
      * @param off offset into dest array where returned bytes are placed
      */
     public static void intToBytes(int data, ByteOrder byteOrder, byte[] dest, int off) {

         if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
             dest[off  ] = (byte)(data >> 24);
             dest[off+1] = (byte)(data >> 16);
             dest[off+2] = (byte)(data >>  8);
             dest[off+3] = (byte)(data      );
         }
         else {
             dest[off  ] = (byte)(data      );
             dest[off+1] = (byte)(data >>  8);
             dest[off+2] = (byte)(data >> 16);
             dest[off+3] = (byte)(data >> 24);
         }
     }

     /**
      * Turn long into byte array.
      * Avoids creation of new byte array with each call.
      *
      * @param data long to convert
      * @param byteOrder byte order of returned bytes (big endian if null)
      * @param dest array in which to store returned bytes
      * @param off offset into dest array where returned bytes are placed
      */
     public static void longToBytes(long data, ByteOrder byteOrder, byte[] dest, int off) {

         if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
             dest[off  ] = (byte)(data >> 56);
             dest[off+1] = (byte)(data >> 48);
             dest[off+2] = (byte)(data >> 40);
             dest[off+3] = (byte)(data >> 32);
             dest[off+4] = (byte)(data >> 24);
             dest[off+5] = (byte)(data >> 16);
             dest[off+6] = (byte)(data >>  8);
             dest[off+7] = (byte)(data      );
         }
         else {
             dest[off  ] = (byte)(data      );
             dest[off+1] = (byte)(data >>  8);
             dest[off+2] = (byte)(data >> 16);
             dest[off+3] = (byte)(data >> 24);
             dest[off+4] = (byte)(data >> 32);
             dest[off+5] = (byte)(data >> 40);
             dest[off+6] = (byte)(data >> 48);
             dest[off+7] = (byte)(data >> 56);
         }
     }

    /**
     * Copies a short value into 2 bytes of a byte array.
     * @param shortVal short value
     * @param b byte array
     * @param off offset into the byte array
     */
    public static void shortToBytes(short shortVal, byte[] b, int off) {
        shortToBytes(shortVal, null, b, off);
    }

    /**
     * Copies an integer value into 4 bytes of a byte array.
     * @param intVal integer value
     * @param b byte array
     * @param off offset into the byte array
     */
    public static void intToBytes(int intVal, byte[] b, int off) {
        intToBytes(intVal, null, b, off);
    }

    /**
     * Copies an long (64 bit) value into 8 bytes of a byte array.
     * @param longVal long value
     * @param b byte array
     * @param off offset into the byte array
     */
    public static void longToBytes(long longVal, byte[] b, int off) {
        longToBytes(longVal, null, b, off);
    }


    /**
     * Turn section of byte array into a short.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param off offset into data array
     * @return short converted from byte array
     */
    public static short bytesToShort(byte[] data, ByteOrder byteOrder, int off) {

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (short)(
                (0xff & data[  off]) << 8 |
                (0xff & data[1+off])
            );
        }
        else {
            return (short)(
                (0xff & data[  off]) |
                (0xff & data[1+off] << 8)
            );
        }
    }

    /**
     * Turn section of byte array into an int.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param off offset into data array
     * @return int converted from byte array
     */
    public static int bytesToInt(byte[] data, ByteOrder byteOrder, int off) {

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (
                (0xff & data[  off]) << 24 |
                (0xff & data[1+off]) << 16 |
                (0xff & data[2+off]) <<  8 |
                (0xff & data[3+off])
            );
        }
        else {
            return (
                (0xff & data[  off])       |
                (0xff & data[1+off]) <<  8 |
                (0xff & data[2+off]) << 16 |
                (0xff & data[3+off]) << 24
            );
        }
    }

    /**
     * Turn section of byte array into a long.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param off offset into data array
     * @return long converted from byte array
     */
    public static long bytesToLong(byte[] data, ByteOrder byteOrder, int off) {

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (
                // Convert to longs before shift because digits
                // are lost with ints beyond the 32-bit limit
                (long)(0xff & data[  off]) << 56 |
                (long)(0xff & data[1+off]) << 48 |
                (long)(0xff & data[2+off]) << 40 |
                (long)(0xff & data[3+off]) << 32 |
                (long)(0xff & data[4+off]) << 24 |
                (long)(0xff & data[5+off]) << 16 |
                (long)(0xff & data[6+off]) <<  8 |
                (long)(0xff & data[7+off])
            );
        }
        else {
            return (
                (long)(0xff & data[  off])       |
                (long)(0xff & data[1+off]) <<  8 |
                (long)(0xff & data[2+off]) << 16 |
                (long)(0xff & data[3+off]) << 24 |
                (long)(0xff & data[4+off]) << 32 |
                (long)(0xff & data[5+off]) << 40 |
                (long)(0xff & data[6+off]) << 48 |
                (long)(0xff & data[7+off]) << 56
            );
        }
    }

    /**
     * Converts 2 bytes of a byte array into a short.
     * @param b byte array
     * @param off offset into the byte array (0 = start at first element)
     * @return short value
     */
    public static short bytesToShort(byte[] b, int off) {
        return bytesToShort(b, null, off);
    }

    /**
     * Converts 4 bytes of a byte array into an integer.
     *
     * @param b   byte array
     * @param off offset into the byte array (0 = start at first element)
     * @return integer value
     */
    public static int bytesToInt(byte[] b, int off) {
        return bytesToInt(b, null, off);
    }

    /**
     * Converts 8 bytes of a byte array into a long.
     * @param b byte array
     * @param off offset into the byte array (0 = start at first element)
     * @return long value
     */
    public static long bytesToLong(byte[] b, int off) {
        return bytesToLong(b, null, off);
    }

    /**
     * Swaps 4 bytes of a byte array in place.
     * @param b byte array
     * @param off offset into the byte array
     */
    public static void swapArrayInt(byte[] b, int off) {
        byte b1, b2, b3, b4;
        b1 = b[off];
        b2 = b[off+1];
        b3 = b[off+2];
        b4 = b[off+3];
        b[off+3] = b1;
        b[off+2] = b2;
        b[off+1] = b3;
        b[off]   = b4;
    }

    /**
     * Swaps 2 bytes of a byte array in place.
     * @param b byte array
     * @param off offset into the byte array
     */
    public static void swapArrayShort(byte[] b, int off) {
        byte b1, b2;
        b1 = b[off];
        b2 = b[off+1];
        b[off+1] = b1;
        b[off]   = b2;
    }


    /**
     * Is the given list of IP addresses identical to those of the local host?
     *
     * @param addrs list of String (dot decimal) addresses to test
     * @return <code>true</code> if host is local, else <code>false</code>
     * @throws UnknownHostException if host cannot be resolved
     */
    public static boolean isHostLocal(ArrayList<String> addrs) throws UnknownHostException {
        if (addrs == null || addrs.size() < 1) return false;

        Collection<String> localHostIpAddrs = getAllIpAddresses();

        // Compare to see if 1 address matches.
        // If so, host is the local machine.
        for (String localIP : localHostIpAddrs) {
            // Don't compare loopbacks!
            if (localIP.equals("127.0.0.1")) continue;

            for (String ip : addrs) {
                if (localIP.equals(ip)) {
                    return true;
                }
            }
        }

        return false;
    }


    /**
      * Determine whether a given host name refers to the local host.
      * @param hostName host name that is checked to see if its local or not
      */
     public static final boolean isHostLocal(String hostName) {
        if (hostName == null || hostName.length() < 1) return false;
        if (hostName.equals("127.0.0.1")) return true;

        try {
            // Get all hostName's IP addresses
            InetAddress[] hostAddrs = InetAddress.getAllByName(hostName);
            ArrayList<String> hostAddrList = new ArrayList<String>();
            for (InetAddress addr : hostAddrs) {
                hostAddrList.add(addr.getHostAddress());
            }

            return isHostLocal(hostAddrList);
        }
        catch (UnknownHostException e) {}

        return false;
     }


    /**
     * Determine whether two given host names refers to the same host.
     * @param hostName1 host name that is checked to see if it is the same as the other arg or not.
     * @param hostName2 host name that is checked to see if it is the same as the other arg or not.
     */
     public static final boolean isHostSame(String hostName1, String hostName2) {
        // Arg check
        if (hostName1 == null || hostName1.length() < 1) return false;
        if (hostName2 == null || hostName2.length() < 1) return false;

        try {
            // Compare all know IP addresses against each other

            // Get all hostName1's IP addresses
            InetAddress[] hostAddrs1  = InetAddress.getAllByName(hostName1);

            // Get all hostName2's IP addresses
            InetAddress[] hostAddrs2  = InetAddress.getAllByName(hostName2);

            // See if any 2 addresses are identical
            for (InetAddress lAddr : hostAddrs1) {
                // Don't compare loopbacks!
                if (lAddr.getHostAddress().equals("127.0.0.1")) continue;

                for (InetAddress hAddr : hostAddrs2) {
                    if (lAddr.equals(hAddr)) return true;
                }
            }
        }
        catch (UnknownHostException e) {}

        return false;
     }


    /**
     * Get all local IP addresses in a list in dotted-decimal form.
     * The first IP address in the list is the one associated with
     * the canonical host name.
     * @return list of all local IP addresses in dotted-decimal form.
     */
    public static Collection<String> getAllIpAddresses() {
        // Set of our IP addresses in order
        LinkedHashSet<String> set = new LinkedHashSet<String>();

        try {
            // Start with IP addr associated with canonical host name (if any)
            String canonicalIP = InetAddress.getByName(InetAddress.getLocalHost().
                                                getCanonicalHostName()).getHostAddress();

            if (canonicalIP != null) {
//System.out.println("getAllIpAddresses: add canonical IP = " + canonicalIP);
                set.add(canonicalIP);
            }
        }
        catch (UnknownHostException e) {}

        try {
            Enumeration<NetworkInterface> enumer = NetworkInterface.getNetworkInterfaces();
            while (enumer.hasMoreElements()) {
                NetworkInterface ni = enumer.nextElement();
                if (ni.isUp() && !ni.isLoopback()) {
                    List<InterfaceAddress> inAddrs = ni.getInterfaceAddresses();
                    for (InterfaceAddress ifAddr : inAddrs) {
                        InetAddress addr = ifAddr.getAddress();

                        // skip IPv6 addresses (IPv4 addr has 4 bytes)
                        if (addr.getAddress().length != 4) continue;

                        set.add(addr.getHostAddress());
//System.out.println("getAllIpAddresses: add ip address = " + addr.getHostAddress());
                    }
                }
            }
        }
        catch (SocketException e) {
            e.printStackTrace();
        }

        return set;
    }


    /**
     * Get all local IP broadcast addresses in a list in dotted-decimal form.
     * This only makes sense for IPv4.
     * @return list of all local IP broadcast addresses in dotted-decimal form.
     */
    public static List<String> getAllBroadcastAddresses() {

        // List of our IP addresses
        LinkedList<String> ipList = new LinkedList<String>();

        try {
            Enumeration<NetworkInterface> enumer = NetworkInterface.getNetworkInterfaces();
            while (enumer.hasMoreElements()) {
                NetworkInterface ni = enumer.nextElement();
                if (ni.isUp() && !ni.isLoopback()) {
                    List<InterfaceAddress> inAddrs = ni.getInterfaceAddresses();
                    for (InterfaceAddress ifAddr : inAddrs) {
                        Inet4Address bAddr;
                        try { bAddr = (Inet4Address)ifAddr.getBroadcast(); }
                        catch (ClassCastException e) {
                            // probably IPv6 so ignore
                            continue;
                        }
                        if (bAddr == null) continue;
                        ipList.add(bAddr.getHostAddress());
                    }
                }
            }
        }
        catch (SocketException e) {
            e.printStackTrace();
        }

        // Try this if nothing else works
        if (ipList.size() < 1) {
            ipList.add("255.255.255.255");
        }

        return ipList;
    }


    /**
     * Given an IP address as an argument, this method will return that if it matches
     * one of the local IP addresses. Given a broadcast or subnet address it will
     * return a local IP address on that subnet. If there are no matches,
     * null is returned.
     *
     * @param ip IP or subnet address in dotted-decimal format
     * @return matching local address on the same subnet;
     *         else null if no match
     * @throws EtException if arg not in dotted-decimal format
     */
    public static String getMatchingLocalIpAddress(String ip) throws EtException {
        if (ip == null) {
            return null;
        }

        byte[] ipBytes = isDottedDecimal(ip);
        if (ipBytes == null) {
            throw new EtException("arg is not in dot-decimal (textual presentation) format");
        }

        // Iterate through list of local addresses
        try {
            Enumeration<NetworkInterface> enumer = NetworkInterface.getNetworkInterfaces();

            // For each network interface ...
            while (enumer.hasMoreElements()) {
                NetworkInterface ni = enumer.nextElement();
                if (!ni.isUp() || ni.isLoopback()) {
                    continue;
                }

                // List of IPs associated with this interface
                List<InterfaceAddress> ifAddrs = ni.getInterfaceAddresses();

                for (InterfaceAddress ifAddr : ifAddrs) {
                    Inet4Address addrv4;
                    try { addrv4 = (Inet4Address)ifAddr.getAddress(); }
                    catch (ClassCastException e) {
                        // probably IPv6 so ignore
                        continue;
                    }

                    // If this matches an actual local IP address, return it
                    if (ip.equals(addrv4.getHostAddress()))  {
//System.out.println("getMatchingLocalIpAddress: this is a local address");
                        return ip;
                    }

                    // If this matches a broadcast/subnet address, return
                    // an actual local IP address on this subnet.
                    if (ip.equals(ifAddr.getBroadcast().getHostAddress())) {
//System.out.println("getMatchingLocalIpAddress: broadcast addr, use this IP on that subnet: " +
//                           addrv4.getHostAddress());
                        return addrv4.getHostAddress();
                    }
                }
            }
        }
        catch (SocketException e) {}

        // no match
        return null;
    }


    /**
     * Given a local IP address as an argument, this method will return its
     * broadcast or subnet address. If it already is a broadcast address,
     * that is returned. If address is not local or none can be found,
     * null is returned.
     *
     * @param ip IP or subnet address in dotted-decimal format
     * @return ip's subnet address; null if it's not local or cannot be found
     * @throws EtException if arg is null or not in dotted-decimal format
     */
    public static String getBroadcastAddress2(String ip) throws EtException {
        if (ip == null) {
            throw new EtException("arg is null");
        }

        byte[] ipBytes = isDottedDecimal(ip);
        if (ipBytes == null) {
            throw new EtException("arg is not in dot-decimal (textual presentation) format");
        }

 //       InetAddress addr = InetAddress.getByName(ip);
    //    Dns.GetHostName()
        return null;
    }


    /**
     * Given a local IP address as an argument, this method will return its
     * broadcast or subnet address. If it already is a broadcast address,
     * that is returned. If address is not local or none can be found,
     * null is returned.
     *
     * @param ip IP or subnet address in dotted-decimal format
     * @return ip's subnet address; null if it's not local or cannot be found
     * @throws EtException if arg is null or not in dotted-decimal format
     */
    public static String getBroadcastAddress(String ip) throws EtException {
        if (ip == null) {
            throw new EtException("arg is null");
        }

        byte[] ipBytes = isDottedDecimal(ip);
        if (ipBytes == null) {
            throw new EtException("arg is not in dot-decimal (textual presentation) format");
        }

        // Iterate through list of local addresses
        try {
            Enumeration<NetworkInterface> enumer = NetworkInterface.getNetworkInterfaces();

            // For each network interface ...
            while (enumer.hasMoreElements()) {
                NetworkInterface ni = enumer.nextElement();
                if (!ni.isUp() || ni.isLoopback()) {
                    continue;
                }

                // List of IPs associated with this interface
                List<InterfaceAddress> ifAddrs = ni.getInterfaceAddresses();

                for (InterfaceAddress ifAddr : ifAddrs) {
                    Inet4Address addrv4;
                    try { addrv4 = (Inet4Address)ifAddr.getAddress(); }
                    catch (ClassCastException e) {
                        // probably IPv6 so ignore
                        continue;
                    }
//System.out.println("getMatchingLocalIpAddress: ip (" + ip + ") =? local addr(" +
//addrv4.getHostAddress() + ")");

                    // If this matches an actual local IP address,
                    // return its broadcast address
                    if (ip.equals(addrv4.getHostAddress()))  {
//System.out.println("getMatchingLocalIpAddress: ==== this is a local address");
                        return ifAddr.getBroadcast().getHostAddress();
                    }

//System.out.println("getMatchingLocalIpAddress: ip (" + ip + ") =? broad addr(" +
//                    ifAddr.getBroadcast().getHostAddress() + ")");
                    // If this matches a broadcast/subnet address, return it as is.
                    if (ip.equals(ifAddr.getBroadcast().getHostAddress())) {
//System.out.println("getMatchingLocalIpAddress: ==== broadcast addr: " + ip);
                        return ip;
                    }
                }
            }
        }
        catch (SocketException e) {}

        // no match
        return null;
    }



    /**
     * Takes a list of dotted-decimal formatted IP address strings and orders them
     * so that those on local subnets are first and others come last.
     * This only works for IPv4.
     *
     * @param ipAddresses list of addresses to order; null if arg is null
     * @return ordered list of given IP addresses in dotted-decimal form with those
     *         on local subnets listed first.
     */
    public static List<String> orderIPAddresses(List<String> ipAddresses) {

        if (ipAddresses == null) {
            return null;
        }

        // Eliminate duplicates in arg
        HashSet<String> ipSet = new HashSet<String>(ipAddresses);

        // List of our local IP info - need local subnet masks
        LinkedList<String> ipList = new LinkedList<String>();

        // iterate through argument list of addresses
        outerLoop:
        for (String ip : ipSet) {
            try {
                // iterate through local list of addresses
                Enumeration<NetworkInterface> enumer = NetworkInterface.getNetworkInterfaces();
                while (enumer.hasMoreElements()) {
                    NetworkInterface ni = enumer.nextElement();
                    if (!ni.isUp() || ni.isLoopback()) {
                        continue;
                    }
                    List<InterfaceAddress> inAddrs = ni.getInterfaceAddresses();

                    for (InterfaceAddress ifAddr : inAddrs) {
                        Inet4Address addrv4;
                        try { addrv4 = (Inet4Address)ifAddr.getAddress(); }
                        catch (ClassCastException e) {
                            // probably IPv6 so ignore
                            continue;
                        }

                        // Turn net prefix len into integer subnet mask
                        short prefixLength = ifAddr.getNetworkPrefixLength();
                        int subnetMask = 0;
                        for (int j = 0; j < prefixLength; ++j) {
                            subnetMask |= (1 << 31-j);
                        }
                        String localIP = addrv4.getHostAddress();
                        try {
                            // If local and arg addresses are on the same subnet,
                            // we gotta match to put at the head of the list.
                            if (onSameSubnet(ip, localIP, subnetMask)) {
                                ipList.addFirst(ip);
//System.out.println("Add " + ip + " to list top");
                                continue outerLoop;
                            }
                        }
                        // try the next address
                        catch (EtException e) {}
                    }
                }
            }
            catch (SocketException e) {
                continue;
            }

            // This arg address is not on any of the local subnets,
            // so put it at the end of the list.
//System.out.println("Add " + ip + " to list bottom");
            ipList.addLast(ip);
        }

        return ipList;
    }


    /**
     * Takes a list of dotted-decimal formatted IP address strings and their corresponding
     * broadcast addresses and orders them so that those on the given local subnet are first,
     * those on other local subnets are next, and all others come last.
     * This only works for IPv4.
     *
     * @param ipAddresses      list of addresses to order; null if arg is null
     * @param broadAddresses   list of broadcast addresses - each associated w/ corresponding
     *                         ip address
     * @param preferredAddress if not null, it is the preferred subnet(broadcast) address
     *                         used in sorting so that addresses on the given list which
     *                         exist on the preferred subnet are first on the returned list.
     *                         If a local, non-subnet ip address is given, it will be
     *                         converted internally to the subnet it is on.
     * @return ordered list of given IP addresses in dotted-decimal form with those
     *         on the given local subnet listed first, those on other local subnets listed
     *         next, and all others last.
     */
    public static List<String> orderIPAddresses(List<String> ipAddresses,
                                                List<String> broadAddresses,
                                                String preferredAddress) {

        if (ipAddresses == null) {
            return null;
        }

//System.out.println("orderIPAddresses: IN");

        // List of all IP addrs, ordered
        LinkedList<String> ipList = new LinkedList<String>();

        // List of IP addrs on preferred subnet
        LinkedList<String> preferred = new LinkedList<String>();

        // Convert the preferred address into a local subnet address
        String prefSubnet = null;
        try {
            // Will be null if not local
            prefSubnet = getBroadcastAddress(preferredAddress);
        }
        catch (EtException e) {
            // preferredAddresses is null or not in dotted-decimal format, so ignore
        }

//System.out.println("orderIPAddresses: preferred subnet: " + prefSubnet);
//        if (broadAddresses != null) {
//            System.out.println("broadcastAddress NOT NULL:");
//            for (String s : broadAddresses)
//            System.out.println(s);
//        }

        // Iterate through argument list of addresses
        outerLoop:
        for (int i=0; i < ipAddresses.size(); i++) {

            // If the remote IP addresses are accompanied by their subnet addresses ...
            if (broadAddresses != null && broadAddresses.size() > i) {
                String ipSubNet = broadAddresses.get(i);

                try {
                    // Iterate through list of local addresses and see if its
                    // broadcast address matches the remote broadcast address.
                    // If so, put it in preferred list.
                    Enumeration<NetworkInterface> enumer = NetworkInterface.getNetworkInterfaces();
                    while (enumer.hasMoreElements()) {
                        NetworkInterface ni = enumer.nextElement();
                        if (!ni.isUp() || ni.isLoopback()) {
                            continue;
                        }
                        List<InterfaceAddress> ifAddrs = ni.getInterfaceAddresses();

                        for (InterfaceAddress ifAddr : ifAddrs) {
                            Inet4Address addrv4;
                            try { addrv4 = (Inet4Address)ifAddr.getAddress(); }
                            catch (ClassCastException e) {
                                // probably IPv6 so ignore
                                continue;
                            }

                            String localBroadcastAddr = ifAddr.getBroadcast().getHostAddress();

                            // If the 2 are on the same subnet as the preferred address,
                            // place it on the preferred list.
                            if (prefSubnet != null &&
                                prefSubnet.equals(ipSubNet) &&
                                prefSubnet.equals(localBroadcastAddr)) {

//System.out.println("orderIPAddresses: ip " + ipAddresses.get(i) + " on preferred subnet: " + prefSubnet);
                                preferred.add(ipAddresses.get(i));
                                continue outerLoop;
                            }
                            // If the 2 are on the same subnet but not on the preferred address,
                            // place it first on the regular list.
                            else if (localBroadcastAddr.equals(ipSubNet)) {
//System.out.println("orderIPAddresses: ip " + ipAddresses.get(i) + " on local subnet: " + localBroadcastAddr);
                                ipList.addFirst(ipAddresses.get(i));
                                continue outerLoop;
                            }
                        }
                    }
                }
                catch (SocketException e) {}
            }

            // This address is not on the preferred or any of the
            // local subnets, so put it at the end of the list.
//System.out.println("Add " + ip + " to list bottom");
            ipList.addLast(ipAddresses.get(i));
        }
//System.out.println("\n");

        // Add any preferred addresses to top of list
        ipList.addAll(0, preferred);

        return ipList;
    }


    /**
     * This method tells whether the 2 given IP addresses in dot-decimal notation
     * are on the same subnet or not given a subnet mask in integer form
     * (local byte order). This only works for IPv4.
     *
     * @param ipAddress1 first  IP address in dot-decimal notation
     * @param ipAddress2 second IP address in dot-decimal notation
     * @param subnetMask subnet mask as LOCAL-byte-ordered 32 bit int
     *
     * @returns {@code true} if on same subnet, else {@code false}.
     */
    static boolean onSameSubnet(String ipAddress1, String ipAddress2, int subnetMask)
                                    throws EtException
    {
        if (ipAddress1 == null || ipAddress2 == null) {
            throw new EtException("null arg(s)");
        }

        byte[] ipBytes1 = isDottedDecimal(ipAddress1);
        byte[] ipBytes2 = isDottedDecimal(ipAddress2);

        if (ipBytes1 == null || ipBytes2 == null)  {
            throw new EtException("one or both IP address args are not dot-decimal format");
        }

        int addr1 = (ipBytes1[0] << 24) | (ipBytes1[1] << 16) | (ipBytes1[2] << 8) | ipBytes1[3];
        int addr2 = (ipBytes2[0] << 24) | (ipBytes2[1] << 16) | (ipBytes2[2] << 8) | ipBytes2[3];

        return (addr1 & subnetMask) == (addr2 & subnetMask);
    }


    /**
     * This method tells whether the 2 given IP addresses (one in dot-decimal
     * notation and the other in raw form) are on the same subnet or not given
     * a subnet mask in integer form (local byte order). This only works for IPv4.
     *
     * @param ipAddress1 first  IP address in dot-decimal notation
     * @param ipAddress2 second IP address in raw form (net byte order
     *                   meaning highest order byte in element 0)
     * @param subnetMask subnet mask as LOCAL-byte-ordered 32 bit int
     *
     * @returns {@code true} if on same subnet, else {@code false}.
     */
    static boolean onSameSubnet(String ipAddress1, byte[] ipAddress2, int subnetMask)
                                    throws EtException
    {
        if (ipAddress1 == null || ipAddress2 == null) {
            throw new EtException("null arg(s)");
        }

        byte[] ipBytes1 = isDottedDecimal(ipAddress1);

        if (ipBytes1 == null)  {
            throw new EtException("first IP address arg is not dot-decimal format");
        }

        int addr1 = (ipBytes1[0]   << 24) | (ipBytes1[1]   << 16) | (ipBytes1[2]   << 8) | ipBytes1[3];
        int addr2 = (ipAddress2[0] << 24) | (ipAddress2[1] << 16) | (ipAddress2[2] << 8) | ipAddress2[3];

        return (addr1 & subnetMask) == (addr2 & subnetMask);
    }


    /**
     * This method tells whether the given IPv4 address is in dot-decimal notation or not.
     * If it is, then the returned bytes are the address in numeric form, else null
     * is returned. This only works for IPv4.
     *
     * @param ipAddress IPV4 address
     * @returns IPV4 address in numeric form if arg is valid dot-decimal address, else null
     *          (in network byte order with highest order byte in element 0).
     */
    public static byte[] isDottedDecimal(String ipAddress)
    {
        if (ipAddress == null) return null;

        String IP_ADDRESS = "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})";
        Pattern pattern = Pattern.compile(IP_ADDRESS);
        Matcher matcher = pattern.matcher(ipAddress);
        if (!matcher.matches()) return null;

        int[] hostInts = new int[4];
        for (int i = 1; i <= 4; ++i) {
//System.out.println("   group(" + i + ") = " + matcher.group(i));
            hostInts[i-1] = Integer.parseInt(matcher.group(i));
            if (hostInts[i-1] > 255 || hostInts[i-1] < 0) return null;
        }

        // Change ints to bytes
        byte[] hostBytes = new byte[4];
        hostBytes[0] = (byte) hostInts[0];
        hostBytes[1] = (byte) hostInts[1];
        hostBytes[2] = (byte) hostInts[2];
        hostBytes[3] = (byte) hostInts[3];

        return hostBytes;
     }

}
