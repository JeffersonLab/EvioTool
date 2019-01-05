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

package org.jlab.coda.et;

import java.lang.*;
import java.nio.ByteOrder;
import java.util.*;
import java.io.*;
import java.net.*;
import java.nio.ByteBuffer;

import org.jlab.coda.et.data.*;
import org.jlab.coda.et.exception.*;
import org.jlab.coda.et.enums.Modify;
import org.jlab.coda.et.enums.Mode;
import org.jlab.coda.et.enums.Priority;
import org.jlab.coda.et.enums.DataStatus;

// TODO: if IO exception occurs, open is not set to false, must catch it and call close()
// TODO: so open is set to false. Then try open again.
/**
 * This class implements an object which allows a user to interact with an ET
 * system. It is not the ET system itself, but rather a proxy which communicates
 * over the network or through JNI with the real ET system.
 *
 * @author Carl Timmer
 */

public class EtSystem {

    /** Object to specify how to open the ET system of interest. */
    private EtSystemOpenConfig openConfig;

    /** Object used to connect to a real ET system. */
    private EtSystemOpen sys;

    /** Flag telling whether the real ET system is currently opened or not. */
    private boolean open;

    /** Debug level. */
    private int debug;

    /** Tcp socket connected to ET system's server. */
    private Socket sock;

    /** Flag specifying whether the ET system process is Java based or not. */
    private boolean isJava;

    /** Data input stream built on top of the socket's input stream (with an
     *  intervening buffered input stream). */
    private DataInputStream in;

    /** Data output stream built on top of the socket's output stream (with an
     *  intervening buffered output stream). */
    private DataOutputStream out;



    /**
     * Construct a new EtSystem object.
     *
     * @param config EtSystemOpenConfig object to specify how to open the ET
     *               system of interest (copy is stored & used)
     * @param debug  debug level (e.g. {@link EtConstants#debugInfo})
     * @throws EtException if config is null or not self-consistent
     */
    public EtSystem(EtSystemOpenConfig config, int debug) throws EtException {

        if (config == null) {
            throw new EtException("Invalid arg");
        }

        openConfig = new EtSystemOpenConfig(config);

        if (!openConfig.selfConsistent()) {
            throw new EtException("system open configuration is not self-consistent");
        }

        sys = new EtSystemOpen(openConfig);

        if ((debug != EtConstants.debugNone)   &&
            (debug != EtConstants.debugSevere) &&
            (debug != EtConstants.debugError)  &&
            (debug != EtConstants.debugWarn)   &&
            (debug != EtConstants.debugInfo))    {

            this.debug = EtConstants.debugError;
        }
        else {
            this.debug = debug;
        }

        try {
            sys.setDebug(debug);
        }
        catch (EtException e) { /* never happen */ }

        //open();
    }

    /**
     * Construct a new EtSystem object. Debug level set to print only errors.
     *
     * @param config EtSystemOpenConfig object to specify how to open the ET
     *               system of interest (copy is stored & used)
     * @throws EtException if config is not self-consistent
     */
    public EtSystem(EtSystemOpenConfig config) throws EtException {

        this(config, EtConstants.debugError);
    }

    /**
     * Construct a new EtSystem object. Not meant for general use.
     * Use one of the other constructors.
     *
     * @param sys   EtSystemOpen object to specify a connection to the ET
     *              system of interest
     * @param debug debug level (e.g. {@link EtConstants#debugInfo})
     *
     * @throws IOException
     *     if problems with network communications
     * @throws UnknownHostException
     *     if the host address(es) is(are) unknown
     * @throws EtException
     *     if arg is null;
     *     if the responding ET system has the wrong name, runs a different
     *     version of ET, or has a different value for
     *     {@link EtConstants#stationSelectInts}
     * @throws EtTooManyException
     *     if there were more than one valid response when policy is set to
     *     {@link EtConstants#policyError} and we are looking either
     *     remotely or anywhere for the ET system.
     */
    public EtSystem(EtSystemOpen sys, int debug)  throws
            IOException, EtException, EtTooManyException {

        if (sys == null) {
            throw new EtException("Invalid arg");
        }

        this.sys   = sys;
        openConfig = sys.getConfig();

        if ((debug != EtConstants.debugNone)   &&
            (debug != EtConstants.debugSevere) &&
            (debug != EtConstants.debugError)  &&
            (debug != EtConstants.debugWarn)   &&
            (debug != EtConstants.debugInfo))    {

            this.debug = EtConstants.debugError;
        }
        else {
            this.debug = debug;
        }

        if (sys.isConnected()) {
            if (sys.getLanguage() == EtConstants.langJava) {isJava = true;}

            // buffer communication streams for efficiency
            sock = sys.getSocket();

            if (openConfig.getTcpRecvBufSize() > 0) {
                in = new DataInputStream(new BufferedInputStream(sock.getInputStream(), openConfig.getTcpRecvBufSize()));
            }
            else {
                in = new DataInputStream( new BufferedInputStream( sock.getInputStream(), sock.getReceiveBufferSize()));
            }

            if (openConfig.getTcpSendBufSize() > 0) {
                out  = new DataOutputStream(new BufferedOutputStream(sock.getOutputStream(), openConfig.getTcpSendBufSize()));
            }
            else {
                out  = new DataOutputStream(new BufferedOutputStream(sock.getOutputStream(), sock.getSendBufferSize()));
            }

            open = true;
        }
        else {
            open();
        }

    }


    // Local getters & setters

    
    /**
     * Get the data input stream to talk to ET system server.
     * @return data input stream to talk to ET system server
     */
    public DataInputStream getInputStream() {
        return in;
    }

    /**
     * Get the data output stream to receive from the ET system server.
     * @return data output stream to receive from the ET system server
     */
    public DataOutputStream getOutputStream() {
        return out;
    }

    /**
     * Gets whether the jni library is being used for local access to ET file.
     * @return whether the jni library is being used for local access to ET file.
     */
    public boolean usingJniLibrary() {
        return sys.usingJniLibrary();
    }

    /**
     * Gets the debug output level.
     * @return debug output level
     */
    public int getDebug() {
        return debug;
    }

    /**
     * Sets the debug output level. Must be either {@link EtConstants#debugNone},
     * {@link EtConstants#debugSevere}, {@link EtConstants#debugError},
     * {@link EtConstants#debugWarn}, or {@link EtConstants#debugInfo}.
     *
     * @param val debug level
     * @throws EtException if bad argument value
     */
    public void setDebug(int val) throws EtException {
        if ((val != EtConstants.debugNone)   &&
            (val != EtConstants.debugSevere) &&
            (val != EtConstants.debugError)  &&
            (val != EtConstants.debugWarn)   &&
            (val != EtConstants.debugInfo)) {
            throw new EtException("bad debug argument");
        }
        debug = val;
        try {
            sys.setDebug(debug);
        }
        catch (EtException e) { /* never happen */ }
    }

    /**
     * Gets a copy of the configuration object used to specify how to open the ET system.
     * @return copy of the configuration object used to specify how to open the ET system.
     */
    public EtSystemOpenConfig getConfig() {
        return new EtSystemOpenConfig(openConfig);
    }


    /**
     * Open the ET system and set up buffered communication.
     *
     * @throws IOException
     *     if problems with network communications
     * @throws UnknownHostException
     *     if the host address(es) is(are) unknown
     * @throws EtException
     *     if the responding ET system has the wrong name, runs a different
     *     version of ET, or has a different value for
     *     {@link EtConstants#stationSelectInts}
     * @throws EtTooManyException
     *     if there were more than one valid response when policy is set to
     *     {@link EtConstants#policyError} and we are looking either
     *     remotely or anywhere for the ET system.
     */
    synchronized public void open() throws IOException, EtException, EtTooManyException {

        if (open) {
            return;
        }

        try {
            sys.connect();
        }
        catch (EtTooManyException ex) {
            if (debug >= EtConstants.debugError) {
                int count = 1;
                System.out.println("The following hosts responded:");
                for (Map.Entry<ArrayList<String>[],Integer> entry : sys.getResponders().entrySet()) {
                    System.out.println("  host #" + (count++) + " at port " + entry.getValue());
                    ArrayList<String> addrList = entry.getKey()[0];
                    for (String s : addrList) {
                        System.out.println("    " + s);
                    }
                    System.out.println();
                }
            }
            throw ex;
        }

        if (sys.getLanguage() == EtConstants.langJava) {isJava = true;}

        sock = sys.getSocket();

        // buffer communication streams for efficiency
        if (openConfig.getTcpRecvBufSize() > 0) {
            in = new DataInputStream(new BufferedInputStream(sock.getInputStream(), openConfig.getTcpRecvBufSize()));
        }
        else {
            in = new DataInputStream( new BufferedInputStream( sock.getInputStream(),  sock.getReceiveBufferSize()));
        }

        if (openConfig.getTcpSendBufSize() > 0) {
            out  = new DataOutputStream(new BufferedOutputStream(sock.getOutputStream(), openConfig.getTcpSendBufSize()));
        }
        else {
            out  = new DataOutputStream(new BufferedOutputStream(sock.getOutputStream(), sock.getSendBufferSize()));
        }

        open = true;
    }


    /** Close the ET system. */
    synchronized public void close() {

        if (!open) {
            return;
        }

        // if communication with ET system fails, we've already been "closed"
        try {
            // Are we using JNI? If so, close the ET system it opened.
            if (sys.usingJniLibrary()) {
//System.out.println("   Close et sys JNI object");
                sys.getJni().close();
            }
//            else {
//System.out.println("   Do NOT close et sys JNI object since NO local shared memory");
//            }

            out.writeInt(EtConstants.netClose);  // close and forcedclose do the same thing in java
            out.flush();
        }
        catch (IOException ex) {
            if (debug >= EtConstants.debugError) {
                System.out.println("network communication error");
            }
        }
        finally {
            try {
                in.close();
                out.close();
                sys.disconnect(); // does sock.close()
            }
            catch (IOException ex) { /* ignore exception */ }
        }

        open = false;
    }


    /**
     * Kill the ET system.
     * @throws IOException if problems with network communications
     * @throws EtClosedException if the ET system is closed
     */
    synchronized public void kill() throws IOException, EtClosedException {

        if (!open) {
            throw new EtClosedException("ET system is closed");
        }

        // If communication with ET system fails, we've already been "closed"
        // and cannot, therefore, kill the ET system.
        try {
            // Are we using JNI? If so, close the ET system it opened.
            if (sys.usingJniLibrary()) {
//System.out.println("   Close et sys JNI object");
                sys.getJni().close();
            }
//            else {
//System.out.println("   Do NOT close et sys JNI object since NO local shared memory");
//            }

            out.writeInt(EtConstants.netKill);
            out.flush();
        }
        finally {
            try {
                in.close();
                out.close();
                sys.disconnect(); // does sock.close()
            }
            catch (IOException ex) { /* ignore exception */ }
        }

        open = false;
    }


    /**
     * Is the ET system alive and are we connected to it?
     *
     *  @return <code>true</code> if the ET system is alive and we're connected to it,
     *          otherwise  <code>false</code>
     */
    synchronized public boolean alive() {
        if (!open) {
            return false;
        }

        int alive;
        // If ET system is NOT alive, or if ET system was killed and restarted
        // (breaking tcp connection), we'll get a read or write error.
        try {
            out.writeInt(EtConstants.netAlive);
            out.flush();
            alive = in.readInt();
        }
        catch (IOException ex) {
            if (debug >= EtConstants.debugError) {
                System.out.println("network communication error");
            }
            return false;
        }

        return (alive == 1);
    }


    /**
     * Wake up an attachment that is waiting to read events from a station's empty input list.
     *
     * @param att attachment to wake up
     *
     * @throws IOException
     *      if problems with network communications
     * @throws EtException
     *      if arg is null;
     *      if the attachment object is invalid
     * @throws EtClosedException
   *     if the ET system is closed
     */
    synchronized public void wakeUpAttachment(EtAttachment att)
            throws IOException, EtException, EtClosedException {
        if (!open) {
            throw new EtClosedException ("Not connected to ET system");
        }

        if (att == null || !att.isUsable() || att.getSys() != this) {
            throw new EtException("Invalid attachment");
        }

        out.writeInt(EtConstants.netWakeAtt);
        out.writeInt(att.getId());
        out.flush();
    }


    /**
     * Wake up all attachments waiting to read events from a station's
     * empty input list.
     *
     * @param station station whose attachments are to wake up
     *
     * @throws IOException
     *      if problems with network communications
     * @throws EtException
     *      if arg is null;
     *      if the station object is invalid
     * @throws EtClosedException
     *     if the ET system is closed
     */
    synchronized public void wakeUpAll(EtStation station)
            throws IOException, EtException, EtClosedException {
        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (station == null || !station.isUsable() || station.getSys() != this) {
            throw new EtException("Invalid station");
        }

        out.writeInt(EtConstants.netWakeAll);
        out.writeInt(station.getId());
        out.flush();
    }


    //****************************************************
    //                      STATIONS                     *
    //****************************************************

    
    /**
     * Checks a station configuration for self-consistency.
     *
     * @param config station configuration
     *
     * @throws EtException
     *     if arg is null;
     *     if the station configuration is not self-consistent
     */
    private void configCheck(EtStationConfig config) throws EtException {

        if (config == null) {
            throw new EtException("Invalid arg");
        }

        // USER mode means specifing a class
        if ((config.getSelectMode()  == EtConstants.stationSelectUser) &&
            (config.getSelectClass() == null)) {

            throw new EtException("station config needs a select class name");
        }

        // Must be parallel, block, not prescale, and not restore to input list if rrobin or equal cue
        if (((config.getSelectMode()  == EtConstants.stationSelectRRobin) ||
             (config.getSelectMode()  == EtConstants.stationSelectEqualCue)) &&
            ((config.getFlowMode()    == EtConstants.stationSerial) ||
             (config.getBlockMode()   == EtConstants.stationNonBlocking) ||
             (config.getRestoreMode() == EtConstants.stationRestoreIn)  ||
             (config.getPrescale()    != 1))) {

            throw new EtException("if flowMode = rrobin/equalcue, station must be parallel, nonBlocking, prescale=1, & not restoreIn");
        }

        // If redistributing restored events, must be a parallel station
        if ((config.getRestoreMode() == EtConstants.stationRestoreRedist) &&
            (config.getFlowMode()    != EtConstants.stationParallel)) {

            throw new EtException("if restoreMode = restoreRedist, station must be parallel");
        }

        if (config.getCue() > sys.getNumEvents()) {
            config.setCue(sys.getNumEvents());
        }
    }


    /**
     * Creates a new station placed at the end of the ordered list of stations.
     * If the station is added to a group of parallel stations,
     * it is placed at the end of the list of parallel stations.
     *
     * @param config  station configuration
     * @param name    station name
     *
     * @return new station object
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if not connected to ET system;
     *     if the select method's class cannot be loaded;
     *     if the position is less than 1 (GRAND_CENTRAL's spot);
     *     if the name is GRAND_CENTRAL (already taken);
     *     if the configuration's cue size is too big;
     *     if the configuration needs a select class name
     *     if the configuration inconsistent
     *     if trying to add incompatible parallel station;
     *     if trying to add parallel station to head of existing parallel group;
     * @throws EtExistsException
     *     if the station already exists but with a different configuration
     * @throws EtTooManyException
     *     if the maximum number of stations has been created already
     */
    public EtStation createStation(EtStationConfig config, String name)
            throws IOException, EtDeadException, EtClosedException, EtException,
                   EtExistsException, EtTooManyException {

        return createStation(config, name, EtConstants.end, EtConstants.end);
    }


    /**
     * Creates a new station at a specified position in the ordered list of
     * stations. If the station is added to a group of parallel stations,
     * it is placed at the end of the list of parallel stations.
     *
     * @param config   station configuration
     * @param name     station name
     * @param position position in the linked list to put the station.
     *
     * @return new station object
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if not connected to ET system;
     *     if the select method's class cannot be loaded;
     *     if the position is less than 1 (GRAND_CENTRAL's spot);
     *     if the name is GRAND_CENTRAL (already taken);
     *     if the configuration's cue size is too big;
     *     if the configuration needs a select class name
     *     if the configuration inconsistent
     *     if trying to add incompatible parallel station;
     *     if trying to add parallel station to head of existing parallel group;
     * @throws EtExistsException
     *     if the station already exists but with a different configuration
     * @throws EtTooManyException
     *     if the maximum number of stations has been created already
     */
    public EtStation createStation(EtStationConfig config, String name, int position)
            throws IOException, EtDeadException, EtClosedException, EtException,
                   EtExistsException, EtTooManyException {
        return createStation(config, name, position, EtConstants.end);
    }


    /**
     * Creates a new station at a specified position in the ordered list of
     * stations and in a specified position in an ordered list of parallel
     * stations if it is a parallel station.
     *
     * @param config     station configuration
     * @param name       station name
     * @param position   position in the main list to put the station.
     * @param parallelPosition   position in the list of parallel
     *                           stations to put the station.
     *
     * @return new station object
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     *     if the select method's class cannot be loaded;
     *     if the position is less than 1 (GRAND_CENTRAL's spot);
     *     if the name is GRAND_CENTRAL (already taken);
     *     if the name is too long;
     *     if the configuration's cue size is too big;
     *     if the configuration needs a select class name;
     *     if the configuration inconsistent
     *     if trying to add incompatible parallel station;
     *     if trying to add parallel station to head of existing parallel group;
     * @throws EtExistsException
     *     if the station already exists but with a different configuration
     * @throws EtTooManyException
     *     if the maximum number of stations has been created already
     */
    synchronized public EtStation createStation(EtStationConfig config, String name,
                                              int position, int parallelPosition)
            throws IOException, EtDeadException, EtClosedException, EtException,
                   EtExistsException, EtTooManyException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (name == null || config == null) {
            throw new EtException("Invalid arg");
        }

        // cannot create GrandCentral
        if (name.equals("GRAND_CENTRAL")) {
            throw new EtException("Cannot create GRAND_CENTRAL station");
        }

        // cannot create GrandCentral
        if (name.length() > EtConstants.stationNameLenMax) {
            throw new EtException("Station name too long (> " + EtConstants.stationNameLenMax + " chars)");
        }

        // check value of position
        if (position != EtConstants.end && position < 1) {
            throw new EtException("Bad value for position");
        }

        // check value of parallel position
        if ((parallelPosition != EtConstants.end) &&
            (parallelPosition != EtConstants.newHead) &&
            (parallelPosition  < 0)) {
            throw new EtException("Bad value for parallel position");
        }

        // check station configuration for self consistency
        configCheck(config);

        // command
        out.writeInt(EtConstants.netStatCrAt);

        // station configuration
        out.writeInt(EtConstants.structOk); // not used in Java
        out.writeInt(config.getFlowMode());
        out.writeInt(config.getUserMode());
        out.writeInt(config.getRestoreMode());
        out.writeInt(config.getBlockMode());
        out.writeInt(config.getPrescale());
        out.writeInt(config.getCue());
        out.writeInt(config.getSelectMode());
        int[] select = config.getSelect();
        for (int i=0; i < EtConstants.stationSelectInts; i++) {
            out.writeInt(select[i]);
        }

        int functionLength = 0; // no function
        if (config.getSelectFunction() != null) {
            functionLength = config.getSelectFunction().length() + 1;
        }
        out.writeInt(functionLength);

        int libraryLength = 0; // no lib
        if (config.getSelectLibrary() != null) {
            libraryLength = config.getSelectLibrary().length() + 1;
        }
        out.writeInt(libraryLength);

        int classLength = 0; // no class
        if (config.getSelectClass() != null) {
            classLength = config.getSelectClass().length() + 1;
        }
        out.writeInt(classLength);

        // station name and position
        int nameLength = name.length() + 1;
        out.writeInt(nameLength);
        out.writeInt(position);
        out.writeInt(parallelPosition);

        // write string(s)
        try {
            if (functionLength > 0) {
                out.write(config.getSelectFunction().getBytes("ASCII"));
                out.writeByte(0);
            }
            if (libraryLength > 0) {
                out.write(config.getSelectLibrary().getBytes("ASCII"));
                out.writeByte(0);
            }
            if (classLength > 0) {
                out.write(config.getSelectClass().getBytes("ASCII"));
                out.writeByte(0);
            }
            out.write(name.getBytes("ASCII"));
            out.writeByte(0);
        }
        catch (UnsupportedEncodingException ex) { /* never happen */ }

        out.flush();

        int err = in.readInt();
        int statId = in.readInt();

        if (err == EtConstants.errorDead) {
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            throw new EtClosedException("ET is closed");
        }
        else if (err ==  EtConstants.errorTooMany) {
            throw new EtTooManyException("Maximum number of stations already created");
        }
        else if (err == EtConstants.errorExists) {
            throw new EtExistsException("Station already exists with different definition");
        }
        else if (err < EtConstants.ok) {
            throw new EtException("Trying to add incompatible parallel station, or\n" +
                    "trying to add parallel station to head of existing parallel group, or\n" +
                    "cannot load select class");
        }

        // create station
        EtStation station = new EtStation(name, statId, this);
        station.setUsable(true);
        if (debug >= EtConstants.debugInfo) {
            System.out.println("Creating station " + name + " is done");
        }
        
        return station;
    }


    /**
     * Removes an existing station.
     *
     * @param station station object
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     *     if attachments to the station still exist;
     *     if the station is GRAND_CENTRAL (which must always exist);
     *     if the station does not exist;
     *     if the station object is invalid
     */
    synchronized public void removeStation(EtStation station)
            throws IOException, EtDeadException, EtClosedException, EtException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (station == null) {
            throw new EtException("Invalid station");
        }

        // cannot remove GrandCentral
        if (station.getId() == 0) {
            throw new EtException("Cannot remove GRAND_CENTRAL station");
        }

        // station object invalid
        if (!station.isUsable() || station.getSys() != this) {
            throw new EtException("Invalid station");
        }

        out.writeInt(EtConstants.netStatRm);
        out.writeInt(station.getId());
        out.flush();

        int err = in.readInt();
        if (err == EtConstants.errorDead) {
            station.setUsable(false);
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            station.setUsable(false);
            throw new EtClosedException("ET is closed");
        }
        else if (err < EtConstants.ok) {
            throw new EtException("Either no such station exists " +
                                  "or remove all attachments before removing station");
        }

        station.setUsable(false);
    }


  /**
   * Changes the position of a station in the ordered list of stations.
   *
   * @param station   station object
   * @param position  position in the main station list (starting at 0)
   * @param parallelPosition  position in list of parallel stations (starting at 0)
   *
   * @throws IOException
   *     if problems with network communications
   * @throws EtDeadException
   *     if the ET system processes are dead
   * @throws EtClosedException
   *     if the ET system is closed
   * @throws EtException
   *     if arg is null;
   *     if the station does not exist;
   *     if trying to move GRAND_CENTRAL;
   *     if position is < 1 (GRAND_CENTRAL is always first);
   *     if parallelPosition < 0;
   *     if station object is invalid;
   *     if trying to move an incompatible parallel station to an existing group
   *        of parallel stations or to the head of an existing group of parallel
   *        stations.
   */
  synchronized public void setStationPosition(EtStation station, int position,
                                              int parallelPosition)
      throws IOException, EtDeadException, EtClosedException, EtException {

      if (!open) {
          throw new EtClosedException("Not connected to ET system");
      }

      if (station == null) {
          throw new EtException("Invalid station");
      }

      // cannot move GrandCentral
      if (station.getId() == 0) {
          throw new EtException("Cannot move GRAND_CENTRAL station");
      }

      if ((position != EtConstants.end) && (position < 0)) {
          throw new EtException("bad value for position");
      }
      else if (position == 0) {
          throw new EtException("GRAND_CENTRAL station is always first");
      }

      if ((parallelPosition != EtConstants.end) &&
          (parallelPosition != EtConstants.newHead) &&
          (parallelPosition < 0)) {
          throw new EtException("bad value for parallelPosition");
      }

      if (!station.isUsable() || station.getSys() != this) {
          throw new EtException("Invalid station");
      }

      out.writeInt(EtConstants.netStatSPos);
      out.writeInt(station.getId());
      out.writeInt(position);
      out.writeInt(parallelPosition);
      out.flush();

      int err = in.readInt();
      if (err == EtConstants.errorDead) {
          station.setUsable(false);
          throw new EtDeadException("ET is dead");
      }
      else if (err == EtConstants.errorClosed) {
          station.setUsable(false);
          throw new EtClosedException("ET is closed");
      }
      else if (err < EtConstants.ok) {
          station.setUsable(false);
          throw new EtException("station does not exist");
      }
  }


    /**
     * Gets the position of a station in the ordered list of stations.
     *
     * @param station station object
     * @return position of a station in the main linked list of stations
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     *     if the station does not exist;
     *     if station object is invalid
     */
    synchronized public int getStationPosition(EtStation station)
            throws IOException, EtDeadException, EtClosedException, EtException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (station == null || !station.isUsable() || station.getSys() != this) {
            throw new EtException("Invalid station");
        }

        // GrandCentral is always first
        if (station.getId() == 0) {
            return 0;
        }

        out.writeInt(EtConstants.netStatGPos);
        out.writeInt(station.getId());
        out.flush();

        int err = in.readInt();
        int position = in.readInt();
        // skip parallel position info
        in.skipBytes(4);

        if (err == EtConstants.errorDead) {
            station.setUsable(false);
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            station.setUsable(false);
            throw new EtClosedException("ET is closed");
        }
        else if (err < EtConstants.ok) {
            station.setUsable(false);
            throw new EtException("station does not exist");
        }

        return position;
    }


    /**
     * Gets the position of a parallel station in its ordered list of
     * parallel stations.
     *
     * @param station station object
     * @return position of a station in the linked list of stations
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     *     if the station does not exist;
     *     if station object is invalid
     */
    synchronized public int getStationParallelPosition(EtStation station)
        throws IOException, EtDeadException, EtClosedException, EtException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (station == null || !station.isUsable() || station.getSys() != this) {
            throw new EtException("Invalid station");
        }

        // parallel position is 0 for serial stations (like GrandCentral)
        if (station.getId() == 0) {
            return 0;
        }

        out.writeInt(EtConstants.netStatGPos);
        out.writeInt(station.getId());
        out.flush();

        int err = in.readInt();
        // skip main position info
        in.skipBytes(4);
        int pPosition = in.readInt();

        if (err == EtConstants.errorDead) {
            station.setUsable(false);
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            station.setUsable(false);
            throw new EtClosedException("ET is closed");
        }
        else if (err < EtConstants.ok) {
            station.setUsable(false);
            throw new EtException("station does not exist");
        }

        return pPosition;
    }


    /**
     * Create an attachment to a station.
     *
     * @param station station object
     * @return an attachment object
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     *     if the station does not exist or is not in active/idle state;
     *     if station object is invalid
     * @throws EtTooManyException
     *     if no more attachments are allowed to the station and/or ET system
     */
    synchronized public EtAttachment attach(EtStation station)
            throws IOException, EtDeadException, EtClosedException,
                   EtException, EtTooManyException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (station == null || !station.isUsable() || station.getSys() != this) {
            throw new EtException("Invalid station");
        }

        // find name of our host
        String host = "unknown";
        try {host = InetAddress.getLocalHost().getHostName();}
        catch (UnknownHostException ex) { /* host = "unknown" */ }

        // find interface (ip address) socket is using
        String ipAddr = sock.getLocalAddress().getHostAddress();

        out.writeInt(EtConstants.netStatAtt);
        out.writeInt(station.getId());
        out.writeInt(-1); // no pid in Java
        out.writeInt(host.length() + 1);
        out.writeInt(ipAddr.length() + 1);

        // write strings
        try {
            out.write(host.getBytes("ASCII"));
            out.writeByte(0);
            out.write(ipAddr.getBytes("ASCII"));
            out.writeByte(0);
        }
        catch (UnsupportedEncodingException ex) { /* never happen */ }
        out.flush();

        int err = in.readInt();
        int attId = in.readInt();

        if (err == EtConstants.errorDead) {
            station.setUsable(false);
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            station.setUsable(false);
            throw new EtClosedException("ET is closed");
        }
        else if (err ==  EtConstants.errorTooMany) {
            throw new EtTooManyException("no more attachments allowed to station or system");
        }
        else if (err < EtConstants.ok) {
            station.setUsable(false);
            throw new EtException("station does not exist or not idle/active");
        }

        EtAttachment att = new EtAttachment(station, attId, this);
        att.setUsable(true);
        return att;
    }



    /**
     * Remove an attachment from a station.
     *
     * @param att attachment object
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     *     if not attached to station;
     *     if the attachment object is invalid
     */
    synchronized public void detach(EtAttachment att)
            throws IOException, EtDeadException, EtClosedException, EtException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (att == null || !att.isUsable() || att.getSys() != this) {
            throw new EtException("Invalid attachment");
        }

        out.writeInt(EtConstants.netStatDet);
        out.writeInt(att.getId());
        out.flush();
        int err = in.readInt();
        att.setUsable(false);

        if (err == EtConstants.errorDead) {
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            throw new EtClosedException("ET is closed");
        }
        else if (err < EtConstants.ok) {
            throw new EtException("not attached to station");
        }
    }


  //*****************************************************
  //                STATION INFORMATION                 *
  //*****************************************************


    /**
     * Is given attachment attached to a station?
     *
     * @param station  station object
     * @param att      attachment object
     *
     * @return <code>true</code> if an attachment is attached to a station,
     *         and <code>false</code> otherwise
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     *     if the station does not exist;
     *     if station object is invalid;
     *     if attachment object is invalid
     */
    synchronized public boolean stationAttached(EtStation station, EtAttachment att)
            throws IOException, EtDeadException, EtClosedException, EtException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (station == null || !station.isUsable() || station.getSys() != this) {
            throw new EtException("Invalid station");
        }

        if (att == null || !att.isUsable() || att.getSys() != this) {
            throw new EtException("Invalid attachment");
        }

        out.writeInt(EtConstants.netStatIsAt);
        out.writeInt(station.getId());
        out.writeInt(att.getId());
        out.flush();
        int err = in.readInt();

        if (err == EtConstants.errorDead) {
            station.setUsable(false);
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            station.setUsable(false);
            throw new EtClosedException("ET is closed");
        }
        else if (err < EtConstants.ok) {
            station.setUsable(false);
            throw new EtException("station does not exist");
        }

        return (err == 1);
    }


    /**
     * Does given station exist?
     *
     * @param name station name
     *
     * @return <code>true</code> if a station exists, and
     *         <code>false</code> otherwise
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     */
    synchronized public boolean stationExists(String name)
            throws IOException, EtDeadException, EtClosedException, EtException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (name == null) {
            throw new EtException("Invalid station name");
        }

        out.writeInt(EtConstants.netStatEx);
        out.writeInt(name.length()+1);
        try {
            out.write(name.getBytes("ASCII"));
            out.writeByte(0);
        }
        catch (UnsupportedEncodingException ex) {}
        out.flush();
        int err = in.readInt();
        // skip main position info
        in.skipBytes(4);

        if (err == EtConstants.errorDead) {
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            throw new EtClosedException("ET is closed");
        }

        // id is ignored here since we can't return it as a C function can
        return (err == 1);
    }


    /**
     * Gets a station's object representation from its name.
     *
     * @param name station name
     * @return station object, or null if no such station exists
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtException
     *     if arg is null;
     */
    synchronized public EtStation stationNameToObject(String name)
            throws IOException, EtDeadException, EtClosedException, EtException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (name == null) {
            throw new EtException("Invalid station name");
        }

        out.writeInt(EtConstants.netStatEx);
        out.writeInt(name.length()+1);
        try {
            out.write(name.getBytes("ASCII"));
            out.writeByte(0);
        }
        catch (UnsupportedEncodingException ex) { /* never happen */ }
        out.flush();

        int err = in.readInt();
        int statId = in.readInt();
        if (err == 1) {
            EtStation stat = new EtStation(name, statId, this);
            stat.setUsable(true);
            return stat;
        }

        if (err == EtConstants.errorDead) {
            throw new EtDeadException("ET is dead");
        }
        else if (err == EtConstants.errorClosed) {
            throw new EtClosedException("ET is closed");
        }

        return null;
    }


    //*****************************************************
    //                       EVENTS
    //*****************************************************


    /**
     * Get new (unused) events from an ET system.
     *
     * @param att      attachment object
     * @param mode     if there are no new events available, this parameter specifies
     *                 whether to wait for some by sleeping {@link Mode#SLEEP},
     *                 to wait for a set time {@link Mode#TIMED},
     *                 or to return immediately {@link Mode#ASYNC}.
     * @param microSec the number of microseconds to wait if a timed wait is specified
     * @param count    the number of events desired
     * @param size     the size of events in bytes
     *
     * @return an array of new events obtained from ET system. Count may be different from that requested.
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if not connected to ET system;
     *     if arguments have bad values;
     *     if attachment object is invalid;
     *     for other general errors
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtEmptyException
     *     if the mode is asynchronous and the station's input list is empty
     * @throws EtBusyException
     *     if the mode is asynchronous and the station's input list is being used
     *     (the mutex is locked)
     * @throws EtTimeoutException
     *     if the mode is timed wait and the time has expired
     * @throws EtWakeUpException
     *     if the attachment has been commanded to wakeup,
     *     {@link org.jlab.coda.et.system.EventList#wakeUp(org.jlab.coda.et.system.AttachmentLocal)},
     *     {@link org.jlab.coda.et.system.EventList#wakeUpAll}
     */
    public EtEvent[] newEvents(EtAttachment att, Mode mode, int microSec, int count, int size)
            throws EtException, EtDeadException, EtClosedException, EtEmptyException,
                   EtBusyException, EtTimeoutException, EtWakeUpException, IOException {

        return newEvents(att, mode, false, microSec, count, size, 1);
    }


    /**
     * Get new (unused) events from a specified group of such events in an ET system.
     * This method uses JNI to call ET routines in the C library. Event memory is
     * directly accessed shared memory.
     *
     * @param attId    attachment id number
     * @param mode     if there are no new events available, this parameter specifies
     *                 whether to wait for some by sleeping {@link EtConstants#sleep},
     *                 to wait for a set time {@link EtConstants#timed},
     *                 or to return immediately {@link EtConstants#async}.
     * @param sec      the number of seconds to wait if a timed wait is specified
     * @param nsec     the number of nanoseconds to wait if a timed wait is specified
     * @param count    the number of events desired
     * @param size     the size of events in bytes
     * @param group    group number from which to draw new events. Some ET systems have
     *                 unused events divided into groups whose numbering starts at 1.
     *                 For ET system not so divided, all events belong to group 1.
     *
     * @return an array of new events obtained from ET system. Count may be different from that requested.
     *
     * @throws EtException
     *     if arguments have bad values, attachment object is invalid, or other general errors
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if ET system is closed
     * @throws EtEmptyException
     *     if the mode is asynchronous and the station's input list is empty
     * @throws EtBusyException
     *     if the mode is asynchronous and the station's input list is being used
     *     (the mutex is locked)
     * @throws EtTimeoutException
     *     if the mode is timed wait and the time has expired
     * @throws EtWakeUpException
     *     if the attachment has been commanded to wakeup,
     *     {@link org.jlab.coda.et.system.EventList#wakeUp(org.jlab.coda.et.system.AttachmentLocal)},
     *     {@link org.jlab.coda.et.system.EventList#wakeUpAll}
     */
    private EtEvent[] newEventsJNI(int attId, int mode, int sec, int nsec, int count, int size, int group)
            throws EtException, EtDeadException, EtClosedException, EtEmptyException,
                   EtBusyException, EtTimeoutException, EtWakeUpException {

        EtEventImpl[] events = sys.getJni().newEvents(sys.getJni().getLocalEtId(), attId,
                                                      mode, sec, nsec, count, size, group);
        return events;
    }




    /**
     * Get new (unused) events from a specified group of such events in an ET system.
     * Will access local C-based ET systems through JNI/shared memory, but other ET
     * systems through sockets.
     *
     * @param att       attachment object
     * @param mode      if there are no new events available, this parameter specifies
     *                  whether to wait for some by sleeping {@link Mode#SLEEP},
     *                  to wait for a set time {@link Mode#TIMED},
     *                  or to return immediately {@link Mode#ASYNC}.
     * @param noBuffer  if <code>true</code>, the new events are to have no byte array and
     *                  no associated ByteBuffer created. Also the user must supply the
     *                  ByteBuffer using {@link EtEventImpl#setDataBuffer(java.nio.ByteBuffer)}.
     *                  Only used if connecting remotely.
     * @param microSec  the number of microseconds to wait if a timed wait is specified
     * @param count     the number of events desired
     * @param size      the size of events in bytes
     * @param group     group number from which to draw new events. Some ET systems have
     *                  unused events divided into groups whose numbering starts at 1.
     *                  For ET system not so divided, all events belong to group 1.
     *
     * @return an array of new events obtained from ET system. Count may be different from that requested.
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if arguments have bad values;
     *     if attachment object is invalid;
     *     if group number is too high;
     *     for other general errors
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtEmptyException
     *     if the mode is asynchronous and the station's input list is empty
     * @throws EtBusyException
     *     if the mode is asynchronous and the station's input list is being used
     *     (the mutex is locked)
     * @throws EtTimeoutException
     *     if the mode is timed wait and the time has expired
     * @throws EtWakeUpException
     *     if the attachment has been commanded to wakeup,
     *     {@link org.jlab.coda.et.system.EventList#wakeUp(org.jlab.coda.et.system.AttachmentLocal)},
     *     {@link org.jlab.coda.et.system.EventList#wakeUpAll}
     */
    public EtEvent[] newEvents(EtAttachment att, Mode mode, boolean noBuffer,
                                            int microSec, int count, int size, int group)
            throws EtException, EtDeadException, EtClosedException, EtEmptyException,
                   EtBusyException, EtTimeoutException, EtWakeUpException, IOException {

        if (mode == null) {
            throw new EtException("Invalid mode");
        }

        if (att == null || !att.isUsable() || att.getSys() != this) {
            throw new EtException("Invalid attachment");
        }

        if (count == 0) {
            return new EtEvent[0];
        }
        else if (count < 0) {
            throw new EtException("bad count argument");
        }

        if ((microSec < 0) && (mode == Mode.TIMED)) {
            throw new EtException("bad microSec argument");
        }
        else if (size < 1) {
            throw new EtException("bad size argument");
        }
        else if (group < 1) {
            throw new EtException("group number must be > 0");
        }

        int sec  = 0;
        int nsec = 0;
        if (microSec > 0) {
            sec = microSec/1000000;
            nsec = (microSec - sec*1000000) * 1000;
        }

        // Do we get things locally through JNI?
        if (sys.usingJniLibrary()) {
            // Value of "open" valid if synchronized
            synchronized (this) {
                if (!open) {
                    throw new EtClosedException("Not connected to ET system");
                }
            }
            return newEventsJNI(att.getId(), mode.getValue(), sec, nsec, count, size, group);
        }

        // When using the network, do NOT use SLEEP mode because that
        // may block all usage of this API's synchronized methods.
        // Use repeated calls in TIMED mode. In between those calls,
        // allow other synchronized code to run (such as wakeUpAll).
        int iterations = 1;
        int newTimeInterval = 200000;  // (in microsec) wait .2 second intervals for each get
        Mode netMode = mode;
        if (mode == Mode.SLEEP) {
            netMode = Mode.TIMED;
            sec  =  newTimeInterval/1000000;
            nsec = (newTimeInterval - sec*1000000) * 1000;
        }
        // Also, if there is a long time designated for TIMED mode,
        // break it up into repeated smaller time chunks for the
        // reason mentioned above. Don't break it up if timeout <= 1 sec.
        else if (mode == Mode.TIMED && (microSec > 1000000))  {
            sec  =  newTimeInterval/1000000;
            nsec = (newTimeInterval - sec*1000000) * 1000;
            // How many times do we call getEvents() with this new timeout value?
            // It will be an over estimate unless timeout is evenly divisible by .2 seconds.
            iterations = microSec/newTimeInterval;
            if (microSec % newTimeInterval > 0) iterations++;
        }

        byte[] buffer = new byte[36];
        EtUtils.intToBytes(EtConstants.netEvsNewGrp, buffer, 0);
        EtUtils.intToBytes(att.getId(),        buffer, 4);
        EtUtils.intToBytes(netMode.getValue(), buffer, 8);
        EtUtils.longToBytes((long)size,        buffer, 12);
        EtUtils.intToBytes(count,              buffer, 20);
        EtUtils.intToBytes(group,              buffer, 24);
        EtUtils.intToBytes(sec,                buffer, 28);
        EtUtils.intToBytes(nsec,               buffer, 32);

        int numEvents;
        EtEventImpl[] evs;

        boolean wait = false;

        while (true) {

            // Allow other synchronized methods to be called here
            if (wait) {
                try {Thread.sleep(10);}
                catch (InterruptedException e) { }
            }

            synchronized (this) {
                if (!open) {
                    throw new EtClosedException("Not connected to ET system");
                }

                out.write(buffer);
                out.flush();

                // ET system clients are liable to get stuck here if the ET
                // system crashes. So use the 2 second socket timeout to try
                // to read again. If the socket connection has been broken,
                // an IOException will be generated.
                int err;
                while (true) {
                    try {
                        err = in.readInt();
                        break;
                    }
                    // If there's an interrupted ex, socket is OK, try again.
                    catch (InterruptedIOException ex) {
                    }
                }

                if (err < EtConstants.ok) {
                    if (debug >= EtConstants.debugError && err != EtConstants.errorTimeout) {
                        System.out.println("error in ET system (newEvents), err = " + err);
                    }

                    // throw some exceptions
                    if (err == EtConstants.error) {
                        throw new EtException("bad mode value or group # too high" );
                    }
                    else if (err == EtConstants.errorBusy) {
                        throw new EtBusyException("input list is busy");
                    }
                    else if (err == EtConstants.errorEmpty) {
                        throw new EtEmptyException("no events in list");
                    }
                    else if (err == EtConstants.errorWakeUp) {
                        throw new EtWakeUpException("attachment " + att.getId() + " woken up");
                    }
                    else if (err == EtConstants.errorTimeout) {
                        // Only get here if using SLEEP or TIMED modes
                        if (mode == Mode.SLEEP || iterations-- > 0) {
                            // Give other synchronized methods a chance to run
                            wait = true;
// System.out.println("implement sleep with another timed mode call (newEvents)");
                            continue;
                        }
                        if (debug >= EtConstants.debugError) {
                            System.out.println("newEvents timeout");
                        }
                        throw new EtTimeoutException("no events within timeout");
                    }
                }

                // number of events to expect
                numEvents = err;

                // list of events to return
                evs = new EtEventImpl[numEvents];
                buffer = new byte[4*numEvents];
                in.readFully(buffer, 0, 4*numEvents);
            }
            break;
        }

        int index=-4;
        long sizeLimit = (size > sys.getEventSize()) ? (long)size : sys.getEventSize();

        // Java limits array sizes to an integer. Thus we're limited to
        // 2G byte arrays. Essentially Java is a 32 bit system in this
        // regard even though the JVM might be 64 bits.
        // So set limits on the size accordingly.

        // if C ET system we are connected to is 64 bits ...
        if (!isJava && sys.isBit64()) {
            // if events size > ~1G, only allocate what's asked for
            if ((long)numEvents*sys.getEventSize() > Integer.MAX_VALUE/2) {
                sizeLimit = size;
            }
        }

        for (int j=0; j < numEvents; j++) {
            evs[j] = new EtEventImpl(size, (int)sizeLimit, isJava, noBuffer);
            evs[j].setId(EtUtils.bytesToInt(buffer, index+=4));
            evs[j].setModify(Modify.ANYTHING);
            evs[j].setOwner(att.getId());
        }

        return evs;
    }



    /**
     * Get events from an ET system.
     * This method uses JNI to call ET routines in the C library.
     * Each C data pointer is wrapped with a ByteBuffer object in JNI code.
     * (The previous manner - now commented out - is that we found the data
     * on the Java side through memory mapped file.) The data ByteBuffer object in each event
     * has its limit set to the data length (not the buffer's full capacity).
     *
     * @param attId    attachment id number
     * @param mode     if there are no events available, this parameter specifies
     *                 whether to wait for some by sleeping {@link EtConstants#sleep},
     *                 to wait for a set time {@link EtConstants#timed},
     *                 or to return immediately {@link EtConstants#async}.
     * @param sec      the number of seconds to wait if a timed wait is specified
     * @param nsec     the number of nanoseconds to wait if a timed wait is specified
     * @param count    the number of events desired
     *
     * @return an array of events obtained from ET system. Count may be different from that requested.
     *
     * @throws EtException
     *     if arguments have bad values, the attachment's station is
     *     GRAND_CENTRAL, or the attachment object is invalid, or other general errors
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtEmptyException
     *     if the mode is asynchronous and the station's input list is empty
     * @throws EtBusyException
     *     if the mode is asynchronous and the station's input list is being used
     *     (the mutex is locked)
     * @throws EtTimeoutException
     *     if the mode is timed wait and the time has expired
     * @throws EtWakeUpException
     *     if the attachment has been commanded to wakeup,
     *     {@link org.jlab.coda.et.system.EventList#wakeUp(org.jlab.coda.et.system.AttachmentLocal)},
     *     {@link org.jlab.coda.et.system.EventList#wakeUpAll}
     */
    private EtEvent[] getEventsJNI(int attId, int mode, int sec, int nsec, int count)
            throws EtException, EtDeadException, EtClosedException, EtEmptyException,
                   EtBusyException, EtTimeoutException, EtWakeUpException {

        EtEventImpl[] events = sys.getJni().getEvents(sys.getJni().getLocalEtId(),
                                                      attId, mode, sec, nsec, count);
        return events;
    }


    /**
     * Get events from an ET system.
     * Will access local C-based ET systems through JNI/shared memory, but other ET
     * systems through sockets. The data ByteBuffer object in each event
     * has its limit set to the data length (not the buffer's full capacity).<p>
     *
     * NOTE: This method must implement the SLEEP mode using the TIMED mode when employing
     * network communications due to the fact that we may block for extended periods of
     * time will in a synchronized block of code. This would not allow other, vital
     * methods to be called simultaneously (e.g. {@link #wakeUpAll(EtStation)}).
     *
     * @param att      attachment object
     * @param mode     if there are no new events available, this parameter specifies
     *                 whether to wait for some by sleeping {@link Mode#SLEEP},
     *                 to wait for a set time {@link Mode#TIMED},
     *                 or to return immediately {@link Mode#ASYNC}.
     * @param modify   this specifies whether this application plans
     *                 on modifying the data in events obtained {@link Modify#ANYTHING}, or
     *                 only modifying headers {@link Modify#HEADER}. The default assumed ,{@link Modify#NOTHING},
     *                 is that no values are modified resulting in the events being put back into
     *                 the ET system (by remote server) immediately upon being copied and that copy
     *                 sent to this method's caller.
     * @param microSec the number of microseconds to wait if a timed wait is
     *                 specified
     * @param count    the number of events desired
     *
     * @return an array of events obtained from ET system. Count may be different from that requested.
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if arguments have bad values;
     *     if the attachment's station is GRAND_CENTRAL;
     *     if the attachment object is invalid;
     *     for other general errors
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     * @throws EtEmptyException
     *     if the mode is asynchronous and the station's input list is empty
     * @throws EtBusyException
     *     if the mode is asynchronous and the station's input list is being used
     *     (the mutex is locked)
     * @throws EtTimeoutException
     *     if the mode is timed wait and the time has expired
     * @throws EtWakeUpException
     *     if the attachment has been commanded to wakeup,
     *     {@link org.jlab.coda.et.system.EventList#wakeUp(org.jlab.coda.et.system.AttachmentLocal)},
     *     {@link org.jlab.coda.et.system.EventList#wakeUpAll}
     */
    public EtEvent[] getEvents(EtAttachment att, Mode mode, Modify modify, int microSec, int count)
            throws EtException, EtDeadException, EtClosedException, EtEmptyException,
                   EtBusyException, EtTimeoutException, EtWakeUpException, IOException {

        if (att == null|| !att.isUsable() || att.getSys() != this) {
            throw new EtException("Invalid attachment");
        }
        
        if (modify == null) {
            modify = Modify.NOTHING;
        }

        // May not get events from GrandCentral
        if (att.getStation().getId() == 0) {
            throw new EtException("may not get events from GRAND_CENTRAL");
        }

        if (count == 0) {
            return new EtEvent[0];
        }
        else if (count < 0) {
            throw new EtException("bad count argument");
        }

        if (mode == null) {
            throw new EtException("Invalid mode");
        }
        else if ((mode == Mode.TIMED) && (microSec < 0)) {
            throw new EtException("bad microSec argument");
        }

        int sec  = 0;
        int nsec = 0;
        if (microSec > 0) {
            sec = microSec/1000000;
            nsec = (microSec - sec*1000000) * 1000;
        }

        // Do we get things locally through JNI?
        if (sys.usingJniLibrary()) {
            // Value of "open" valid only if synchronized
            synchronized (this) {
                if (!open) {
                    throw new EtClosedException("Not connected to ET system");
                }
                return getEventsJNI(att.getId(), mode.getValue(), sec, nsec, count);
            }
        }

        // When using the network, do NOT use SLEEP mode because that
        // may block all usage of this API's synchronized methods.
        // Use repeated calls in TIMED mode. In between those calls,
        // allow other synchronized code to run (such as wakeUpAll).
        int iterations = 1;
        int newTimeInterval = 200000;  // (in microsec) wait .2 second intervals for each get
        Mode netMode = mode;
        if (mode == Mode.SLEEP) {
            netMode = Mode.TIMED;
            sec  =  newTimeInterval/1000000;
            nsec = (newTimeInterval - sec*1000000) * 1000;
        }
        // Also, if there is a long time designated for TIMED mode,
        // break it up into repeated smaller time chunks for the
        // reason mentioned above. Don't break it up if timeout <= 1 sec.
        else if (mode == Mode.TIMED && (microSec > 1000000))  {
            sec  =  newTimeInterval/1000000;
            nsec = (newTimeInterval - sec*1000000) * 1000;
            // How many times do we call getEvents() with this new timeout value?
            // It will be an over estimate unless timeout is evenly divisible by .2 seconds.
            iterations = microSec/newTimeInterval;
            if (microSec % newTimeInterval > 0) iterations++;
        }

        EtEventImpl[] evs;

        byte[] buffer = new byte[28];
        EtUtils.intToBytes(EtConstants.netEvsGet, buffer, 0);
        EtUtils.intToBytes(att.getId(),         buffer, 4);
        EtUtils.intToBytes(netMode.getValue(),  buffer, 8);
        EtUtils.intToBytes(modify.getValue(),   buffer, 12);
        EtUtils.intToBytes(count,               buffer, 16);
        EtUtils.intToBytes(sec,                 buffer, 20);
        EtUtils.intToBytes(nsec,                buffer, 24);

        boolean wait = false;

        while (true) {

            // Allow other synchronized methods to be called here
            if (wait) {
                try {Thread.sleep(10);}
                catch (InterruptedException e) { }
            }

            // Write over the network
            synchronized (this) {
                if (!open) {
                    throw new EtClosedException("Not connected to ET system");
                }

                out.write(buffer);
                out.flush();

                // ET system clients are liable to get stuck here if the ET
                // system crashes. So use the 2 second socket timeout to try
                // to read again. If the socket connection has been broken,
                // an IOException will be generated.
                int err;
                while (true) {
                    try {
                        err = in.readInt();
                        break;
                    }
                    // If there's an interrupted ex, socket is OK, try again.
                    catch (InterruptedIOException ex) {
                    }
                }

                if (err < EtConstants.ok) {
                    if (debug >= EtConstants.debugError && err != EtConstants.errorTimeout) {
                        System.out.println("error in ET system (getEvents), err = " + err);
                    }

                    if (err == EtConstants.error) {
                        throw new EtException("bad mode value" );
                    }
                    else if (err == EtConstants.errorBusy) {
                        throw new EtBusyException("input list is busy");
                    }
                    else if (err == EtConstants.errorEmpty) {
                        throw new EtEmptyException("no events in list");
                    }
                    else if (err == EtConstants.errorWakeUp) {
                        throw new EtWakeUpException("attachment " + att.getId() + " woken up");
                    }
                    else if (err == EtConstants.errorTimeout) {
                        // Only get here if using SLEEP or TIMED modes
                        if (mode == Mode.SLEEP || iterations-- > 0) {
                            // Give other synchronized methods a chance to run
                            wait = true;
                            continue;
                        }
                        if (debug >= EtConstants.debugError) {
//System.out.println("error in ET system (getEvents), timeout");
                        }
                        throw new EtTimeoutException("no events within timeout");
                    }
                }

                // skip reading total size (long)
                in.skipBytes(8);

                final int selectInts   = EtConstants.stationSelectInts;
                final int dataShift    = EtConstants.dataShift;
                final int dataMask     = EtConstants.dataMask;
                final int priorityMask = EtConstants.priorityMask;

                int numEvents = err;
                evs = new EtEventImpl[numEvents];
                int byteChunk = 4*(9+ EtConstants.stationSelectInts);
                buffer = new byte[byteChunk];
                int index;

                long  length, memSize;
                int   priAndStat;

                for (int j=0; j < numEvents; j++) {
                    in.readFully(buffer, 0, byteChunk);

                    length  = EtUtils.bytesToLong(buffer, 0);
                    memSize = EtUtils.bytesToLong(buffer, 8);

                    // Note that the server will not send events too big for us,
                    // we'll get an error above.

                    // if C ET system we are connected to is 64 bits ...
                    if (!isJava && sys.isBit64()) {
                        // if event size > ~1G, only allocate enough to hold data
                        if (memSize > Integer.MAX_VALUE/2) {
                            memSize = length;
                        }
                    }
                    evs[j] = new EtEventImpl((int)memSize, (int)memSize, isJava, false);
                    evs[j].setLength((int)length);
                    evs[j].getDataBuffer().limit((int)length);
                    priAndStat = EtUtils.bytesToInt(buffer, 16);
                    evs[j].setPriority(Priority.getPriority(priAndStat & priorityMask));
                    evs[j].setDataStatus(DataStatus.getStatus((priAndStat & dataMask) >> dataShift));
                    evs[j].setId(EtUtils.bytesToInt(buffer, 20));
                    // skip unused int here
                    evs[j].setRawByteOrder(EtUtils.bytesToInt(buffer, 28));
                    index = 32;   // skip unused int
                    int[] control = new int[selectInts];
                    for (int i=0; i < selectInts; i++) {
                        control[i] = EtUtils.bytesToInt(buffer, index+=4);
                    }
                    evs[j].setControl(control);
                    evs[j].setModify(modify);
                    evs[j].setOwner(att.getId());

                    in.readFully(evs[j].getData(), 0, (int)length);
                }
            }

            break;
        }

        return evs;
    }


    /**
     * Put events into an ET system.
     * Will access local C-based ET systems through JNI/shared memory, but other ET
     * systems through sockets.
     *
     * @param att        attachment object
     * @param eventList  list of event objects
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if invalid arg(s);
     *     if not connected to ET system;
     *     if events are not owned by this attachment;
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public void putEvents(EtAttachment att, List<EtEvent> eventList)
            throws IOException, EtException, EtDeadException, EtClosedException {

        if (eventList == null) {
            throw new EtException("Invalid event list arg");
        }
        putEvents(att, eventList.toArray(new EtEvent[eventList.size()]));
    }


    /**
     * Put events into an ET system.
     * Will access local C-based ET systems through JNI/shared memory, but other ET
     * systems through sockets.
     *
     * @param att  attachment object
     * @param evs  array of event objects
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if invalid arg(s);
     *     if not connected to ET system;
     *     if events are not owned by this attachment;
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public void putEvents(EtAttachment att, EtEvent[] evs)
            throws IOException, EtException, EtDeadException, EtClosedException {

        if (evs == null) {
            throw new EtException("Invalid event array arg");
        }
        putEvents(att, evs, 0, evs.length);
    }


    /**
     * Put events into an ET system.
     * This method uses JNI to call ET routines in the C library. Since event memory, in
     * this case, is directly accessed shared memory, none of it is copied.
     *
     * @param attId  attachment ID
     * @param evs    array of event objects
     * @param offset offset into array
     * @param length number of array elements to put
     *
     * @throws EtException
     *     if invalid arg(s);
     *     if events are not owned by this attachment;
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     */
    private void putEventsJNI(int attId, EtEvent[] evs, int offset, int length)
            throws EtException, EtDeadException, EtClosedException {

        // C interface has no offset (why did I do that again?), so compensate for that
        EtEventImpl[] events = new EtEventImpl[length];
        for (int i=0; i<length; i++) {
            events[i] = (EtEventImpl) evs[offset + i];
        }

        sys.getJni().putEvents(sys.getJni().getLocalEtId(), attId, events, length);
    }


    /**
     * Put events into an ET system.
     * Will access local C-based ET systems through JNI/shared memory, but other ET
     * systems through sockets.
     *
     * @param att    attachment object
     * @param evs    array of event objects
     * @param offset offset into array
     * @param length number of array elements to put
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if invalid arg(s);
     *     if events are not owned by this attachment;
     *     if null data buffer & whole event's being modified;
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     */
    synchronized public void putEvents(EtAttachment att, EtEvent[] evs, int offset, int length)
            throws IOException, EtException, EtDeadException, EtClosedException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (evs == null) {
            throw new EtException("Invalid event array arg");
        }

        if (offset < 0 || length < 0 || offset + length > evs.length) {
            throw new EtException("Bad offset or length argument(s)");
        }

        if (att == null || !att.isUsable() || att.getSys() != this) {
            throw new EtException("Invalid attachment");
        }

        final int selectInts = EtConstants.stationSelectInts;
        final int dataShift  = EtConstants.dataShift;

        // find out how many events we're sending & total # bytes
        int bytes = 0, numEvents = 0;
        int headerSize = 4*(7+selectInts);

        for (int i=offset; i < offset+length; i++) {
            // each event must be registered as owned by this attachment
            if (evs[i].getOwner() != att.getId()) {
                throw new EtException("may not put event(s), not owner");
            }
            // if modifying header only or header & data ...
            if (evs[i].getModify() != Modify.NOTHING) {
                numEvents++;
                bytes += headerSize;
                // if modifying data as well ...
                if (evs[i].getModify() == Modify.ANYTHING) {
                    bytes += evs[i].getLength();
                }
            }
        }


        // Did we get things locally through JNI?
        if (sys.usingJniLibrary()) {
            putEventsJNI(att.getId(), evs, offset, length);
            return;
        }

        // If nothing was modified, we're done, just return.
        if (numEvents == 0) {
            return;
        }

        int indx;
        int[] control;
        byte[] header = new byte[headerSize];

        out.writeInt(EtConstants.netEvsPut);
        out.writeInt(att.getId());
        out.writeInt(numEvents);
        out.writeLong((long)bytes);

//        EtUtils.intToBytes(EtConstants.netEvsPut, ByteOrder.BIG_ENDIAN, header, 0);
//        EtUtils.intToBytes(att.getId(), ByteOrder.BIG_ENDIAN, header, 4);
//        EtUtils.intToBytes(numEvents, ByteOrder.BIG_ENDIAN, header, 8);
//        EtUtils.longToBytes((long)bytes, ByteOrder.BIG_ENDIAN, header, 12);
//        out.write(header, 0, 20);


        for (int i=offset; i < offset+length; i++) {
            // send only if modifying an event (data or header) ...
            if (evs[i].getModify() != Modify.NOTHING) {
                EtUtils.intToBytes(evs[i].getId(), ByteOrder.BIG_ENDIAN, header, 0);
                // skip 1 int here
                EtUtils.longToBytes((long) evs[i].getLength(), ByteOrder.BIG_ENDIAN, header, 8);
                EtUtils.intToBytes(evs[i].getPriority().getValue() | evs[i].getDataStatus().getValue() << dataShift,
                                   ByteOrder.BIG_ENDIAN, header, 16);
                EtUtils.intToBytes(evs[i].getRawByteOrder(), ByteOrder.BIG_ENDIAN, header, 20);
                indx = 28;  // skip 1 int here
                control = evs[i].getControl();
                for (int j=0; j < selectInts; j++,indx+=4) {
                    EtUtils.intToBytes(control[j], ByteOrder.BIG_ENDIAN, header, indx);
                }
                // Much Faster to put header data into byte array and write
                // it out once instead of writing each int and long.
                out.write(header);

                // send data only if modifying whole event
                if (evs[i].getModify() == Modify.ANYTHING) {
                    ByteBuffer buf = evs[i].getDataBuffer();
                    if (buf == null) throw new EtException("null data buffer");
                    if (!buf.hasArray()) {
//System.out.println("Memory mapped buffer does NOT have a backing array !!!");
                        for (int j=0; j<evs[i].getLength(); j++) {
                            out.write(buf.get(j));
                        }
                    }
                    else {
                        out.write(buf.array(), 0, evs[i].getLength());
                    }
                }
            }
        }

        out.flush();

        // err should always be = Constants.ok
        // skip reading error
        in.skipBytes(4);
    }




    /**
     * Dump events into an ET system.
     * This method uses JNI to call ET routines in the C library.
     *
     * @param attId  attachment ID
     * @param evs    array of event objects
     * @param offset offset into array
     * @param length number of array elements to put
     *
     * @throws EtException
     *     if invalid arg(s);
     *     if events are not owned by this attachment;
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     */
    private void dumpEventsJNI(int attId, EtEvent[] evs, int offset, int length)
            throws EtException, EtDeadException, EtClosedException {

        // C interface has no offset (why did I do that again?), so compensate for that
        EtEventImpl[] events = new EtEventImpl[length];
        for (int i=0; i<length; i++) {
            events[i] = (EtEventImpl) evs[offset + i];
        }

        sys.getJni().dumpEvents(sys.getJni().getLocalEtId(), attId, events, length);
    }


    /**
     * Dispose of unwanted events in an ET system. The events are recycled and not
     * made available to any other user.
     * Will access local C-based ET systems through JNI/shared memory, but other ET
     * systems through sockets.
     *
     * @param att  attachment object
     * @param evs  array of event objects
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if invalid arg(s);
     *     if not connected to ET system;
     *     if events are not owned by this attachment;
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public void dumpEvents(EtAttachment att, EtEvent[] evs)
            throws IOException, EtException, EtDeadException, EtClosedException {

        if (evs == null) {
            throw new EtException("Invalid event array arg");
        }
        dumpEvents(att, evs, 0, evs.length);
    }



    /**
     * Dispose of unwanted events in an ET system. The events are recycled and not
     * made available to any other user.
     * Will access local C-based ET systems through JNI/shared memory, but other ET
     * systems through sockets.
     *
     * @param att        attachment object
     * @param eventList  list of event objects
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if invalid arg(s);
     *     if not connected to ET system;
     *     if events are not owned by this attachment;
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public void dumpEvents(EtAttachment att, List<EtEvent> eventList)
            throws IOException, EtException, EtDeadException, EtClosedException {

        if (eventList == null) {
            throw new EtException("Invalid event list arg");
        }

        dumpEvents(att, eventList.toArray(new EtEvent[eventList.size()]));
    }


    /**
     * Dispose of unwanted events in an ET system. The events are recycled and not
     * made available to any other user.
     * Will access local C-based ET systems through JNI/shared memory, but other ET
     * systems through sockets.
     *
     * @param att    attachment object
     * @param evs    array of event objects
     * @param offset offset into array
     * @param length number of array elements to put
     *
     * @throws IOException
     *     if problems with network communications
     * @throws EtException
     *     if invalid arg(s);
     *     if events are not owned by this attachment;
     * @throws EtDeadException
     *     if the ET system processes are dead
     * @throws EtClosedException
     *     if the ET system is closed
     */
    synchronized public void dumpEvents(EtAttachment att, EtEvent[] evs, int offset, int length)
            throws IOException, EtException, EtDeadException, EtClosedException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        if (att == null || !att.isUsable() || att.getSys() != this) {
            throw new EtException("Invalid attachment");
        }

        if (evs == null) {
            throw new EtException("Invalid event array arg");
        }

        if (offset < 0 || length < 0 || offset + length > evs.length) {
            throw new EtException("Bad offset or length argument(s)");
        }

        // find out how many we're sending
        int numEvents = 0;
        for (int i=offset; i<offset+length; i++) {
            // each event must be registered as owned by this attachment
            if (evs[i].getOwner() != att.getId()) {
                throw new EtException("may not put event(s), not owner");
            }
            if (evs[i].getModify() != Modify.NOTHING) numEvents++;
        }

        // Did we get things locally through JNI?
        if (sys.usingJniLibrary()) {
            dumpEventsJNI(att.getId(), evs, offset, length);
            return;
        }

        // If nothing was modified, we're done, just return.
        if (numEvents == 0) {
            return;
        }

        out.writeInt(EtConstants.netEvsDump);
        out.writeInt(att.getId());
        out.writeInt(numEvents);

        for (int i=offset; i<offset+length; i++) {
            // send only if modifying an event (data or header) ...
            if (evs[i].getModify() != Modify.NOTHING) {
                out.writeInt(evs[i].getId());
            }
        }
        out.flush();

        // err should always be = Constants.ok
        // skip reading error
        in.skipBytes(4);
    }


    //****************************************
    //                Getters                *
    //****************************************

    
    /**
     * Gets "integer" format ET system data over the network.
     *
     * @param cmd coded command to send to the TCP server thread
     *
     * @return integer value
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    synchronized private int getIntValue(int cmd) throws IOException, EtClosedException {
        if (!open) {
            throw new EtClosedException ("Not connected to ET system");
        }

        out.writeInt(cmd);
        out.flush();
        // err should always be = Constants.ok
        // skip reading error
        in.skipBytes(4);
        // return val (next readInt)
        return in.readInt();
    }


    /**
     * Gets the number of stations in the ET system.
     *
     * @return number of stations in the ET system
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getNumStations() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysStat);
    }


    /**
     * Gets the maximum number of stations allowed in the ET system.
     *
     * @return maximum number of stations allowed in the ET system
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getStationsMax() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysStatMax);
    }


    /**
     * Gets the number of attachments in the ET system.
     *
     * @return number of attachments in the ET system
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getNumAttachments() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysAtt);
    }


    /**
     * Gets the maximum number of attachments allowed in the ET system.
     *
     * @return maximum number of attachments allowed in the ET system
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getAttachmentsMax() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysAttMax);
    }


    /**
     * Gets the number of local processes in the ET system.
     * This is only relevant in C-language, Solaris systems.
     *
     * @return number of processes in the ET system
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getNumProcesses() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysProc);
    }


    /**
     * Gets the maximum number of local processes allowed in the ET system.
     * This is only relevant in C-language, Solaris systems.
     *
     * @return maximum number of processes allowed in the ET system
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getProcessesMax() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysProcMax);
    }


    /**
     * Gets the number of temp events in the ET system.
     * This is only relevant in C-language systems.
     *
     * @return number of temp events in the ET system
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getNumTemps() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysTmp);
    }


    /**
     * Gets the maximum number of temp events allowed in the ET system.
     * This is only relevant in C-language systems.
     *
     * @return maximum number of temp events in the ET system
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getTempsMax() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysTmpMax);
    }


    /**
     * Gets the ET system heartbeat. This is only relevant in
     * C-language systems.
     *
     * @return ET system heartbeat
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getHeartbeat() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysHBeat);
    }


    /**
     * Gets the UNIX pid of the ET system process.
     * This is only relevant in C-language systems.
     *
     * @return UNIX pid of the ET system process
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getPid() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysPid);
    }


    /**
     * Gets the number of groups events are divided into.
     *
     * @return number of groups events are divided into
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int getGroupCount() throws IOException, EtClosedException {
        return getIntValue(EtConstants.netSysGrp);
    }


    /**
     * Gets the array defining groups events are divided into.
     * The index+1 is the group number, the value is the number
     * of events in that group.
     *
     * @return array defining groups events are divided into
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtClosedException
     *     if the ET system is closed
     */
    public int[] getGroups() throws IOException, EtClosedException {
        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        out.writeInt(EtConstants.netSysGrps);
        out.flush();

        int groupSize = in.readInt();
        int[] groups  = new int[groupSize];

        for (int i=0; i < groupSize; i++) {
            groups[i] = in.readInt();
        }
        return groups;
    }


    /**
     * Gets the number of events in the ET system.
     * @return number of events in the ET system
     */
    public int getNumEvents() {
        return sys.getNumEvents();
    }


    /**
     * Gets the "normal" event size in bytes.
     * @return normal event size in bytes
     */
    public long getEventSize() {
        return sys.getEventSize();
    }


    /**
     * Gets the ET system's implementation language.
     * @return ET system's implementation language
     */
    public int getLanguage() {
        return sys.getLanguage();
    }


    /**
     * Gets the local address (dot decimal) used to connect to ET system.
     * @return the local address (dot decimal) used to connect to ET system.
     */
    public String getLocalAddress() {return sys.getLocalAddress();}

    /**
     * Gets the ET system's host name.
     * @return ET system's host name
     */
    public String getHost() {return sys.getHostAddress();}


    /**
     * Gets the tcp server port number.
     * @return tcp server port number
     */
    public int getTcpPort() {
        return sys.getTcpPort();
    }


    /**
     * Gets all information about the ET system.
     *
     * @return object containing ET system information
     *
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtException
     *     if error in data format/protocol
     * @throws EtClosedException
     *     if the ET system is closed
     */
    synchronized public AllData getData() throws EtException, IOException, EtClosedException {

        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }

        AllData data = new AllData();
        out.writeInt(EtConstants.netSysData);
        out.flush();

        // receive error
        int error = in.readInt();
        if (error != EtConstants.ok) {
            throw new EtException("error getting ET system data");
        }

        // receive incoming data

        // skip reading total size
        in.skipBytes(4);

        // read everything at once (4X faster that way),
        // put it into a byte array, and read from that
        // byte[] bytes = new byte[dataSize];
        // ByteArrayInputStream bis = new ByteArrayInputStream(bytes);
        // DataInputStream dis2 = new DataInputStream(bis);
        // in.readFully(bytes);

        // system data
        // data.sysData.read(dis2);
        data.sysData.read(in);
//System.out.println("getData:  read in system data");

        // station data
        int count = in.readInt();
//System.out.println("getData:  # of stations = " + count);
        data.statData = new StationData[count];
        for (int i=0; i < count; i++) {
            data.statData[i] = new StationData();
            data.statData[i].read(in);
        }
//System.out.println("getData:  read in station data");

        // attachment data
        count = in.readInt();
//System.out.println("getData:  # of attachments = " + count);
        data.attData = new AttachmentData[count];
        for (int i=0; i < count; i++) {
            data.attData[i] = new AttachmentData();
            data.attData[i].read(in);
        }
//System.out.println("getData:  read in attachment data");

        // process data
        count = in.readInt();
//System.out.println("getData:  # of processes = " + count);
        data.procData = new ProcessData[count];
        for (int i=0; i < count; i++) {
            data.procData[i] = new ProcessData();
            data.procData[i].read(in);
        }
//System.out.println("getData:  read in process data");

        return data;
    }


    /**
     * Gets histogram containing data showing how many events in GRAND_CENTRAL's
     * input list when new events are requested by users. This feature is not
     * available on Java ET systems.
     *
     * @return integer array containing histogram
     * 
     * @throws IOException
     *     if there are problems with network communication
     * @throws EtException
     *     if error in data format/protocol
     * @throws EtClosedException
     *     if the ET system is closed
     */
    synchronized public int[] getHistogram() throws IOException, EtException, EtClosedException {
        if (!open) {
            throw new EtClosedException("Not connected to ET system");
        }
        
        out.writeInt(EtConstants.netSysHist);
        out.flush();

        // receive error code
        if (in.readInt() != EtConstants.ok) {
            throw new EtException("cannot get histogram");
        }

        byte[] data = new byte[4*(sys.getNumEvents()+1)];
        int[]  hist = new int[sys.getNumEvents()+1];

        in.readFully(data);
        for (int i=0; i < sys.getNumEvents()+1; i++) {
            hist[i] = EtUtils.bytesToInt(data, i*4);
        }
        return hist;
    }
}

