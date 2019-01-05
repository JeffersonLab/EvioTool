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

package org.jlab.coda.et;

import java.lang.*;
import java.io.*;
import org.jlab.coda.et.exception.*;

/**
 * This class defines a station for the ET system user.
 *
 * @author Carl Timmer
 */

public class EtStation {

    /** Unique id number. */
    private int id;

    /** Name of the station. */
    private String name;

    /** User's ET system object. */
    private EtSystem sys;

    /** Flag telling whether this station object is usable or the station it
     *  represents has been removed. Set by the user's ET system object. */
    private boolean usable;

    // userMode = attachmentLimit; 0 = multiple attachments, 1 = single attachment, 2 = 2 attachments, etc...


    /**
     * Creates a station object. Done by the ET system object only.
     *
     * @param name station name
     * @param id   station id number
     * @param sys  user's ET system object
     */
    EtStation(String name, int id, EtSystem sys) {
        this.id = id;
        this.sys = sys;
        this.name = name;
    }


    // public sets


    /**
     * Sets whether this station object is usable or the station it represents has been removed.
     * @param usable <code>true</code> if station object is usable,
     *               <code>false</code> if station it represents has been removed
     */
    void setUsable(boolean usable) {
        this.usable = usable;
    }


    // public gets


    /** Gets the station name.
     * @return station name */
    public String getName() {return name;}

    /** Gets the station id.
     * @return station id */
    public int getId() {return id;}

    /** Gets the ET system object.
     * @return ET system object */
    public EtSystem getSys() {return sys;}

    /** Tells if this station object is usable.
     * @return <code>true</code> if station object is usable and <code>false</code> otherwise */
    public boolean isUsable() {return usable;}

    /**
     * Gets the station's select array used for filtering events.
     *
     * @return array of select integers
     * @throws EtException
     *     if the station has been removed or cannot be found
     * @see EtStationConfig#select
     */
    public int[] getSelectWords() throws IOException, EtException {
        if (!usable) {throw new EtException("station has been removed");}

        int err;
        int[] select = new int[EtConstants.stationSelectInts];

        synchronized(sys) {
            sys.getOutputStream().writeInt(EtConstants.netStatGSw);
            sys.getOutputStream().writeInt(id);
            sys.getOutputStream().flush();

            err = sys.getInputStream().readInt();
            for (int i=0; i < select.length; i++) {
                select[i] = sys.getInputStream().readInt();
            }
        }

        if (err != EtConstants.ok) {
            throw new EtException("cannot find station");
        }

        return select;
    }

    /**
     * Sets the station's select array - used for filtering events.
     *
     * @param select array of select integers
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed or cannot be found;
     *                     if wrong size array, or if the station is GRAND_CENTRAL
     * @see EtStationConfig#select
     */
    public void setSelectWords(int[] select) throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }

        if (id == 0) {
            throw new EtException("cannot modify GRAND_CENTRAL station");
        }

        if (select.length != EtConstants.stationSelectInts) {
            throw new EtException("wrong number of elements in select array");
        }

        int err;

        synchronized (sys) {
            sys.getOutputStream().writeInt(EtConstants.netStatSSw);
            sys.getOutputStream().writeInt(id);
            for (int i = 0; i < select.length; i++) {
                sys.getOutputStream().writeInt(select[i]);
            }
            sys.getOutputStream().flush();
            err = sys.getInputStream().readInt();
        }

        if (err != EtConstants.ok) {
            throw new EtException("this station has been removed from ET system");
        }

        return;
    }

    /**
     * This gets "String" station parameter data over the network.
     *
     * @param command coded command to send to the TCP server thread.
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station cannot be found
     */
    private String getStringValue(int command) throws IOException, EtException {
        byte[] buf = null;
        String val = null;
        int err, length = 0;

        synchronized (sys) {
            sys.getOutputStream().writeInt(command);
            sys.getOutputStream().writeInt(id);
            sys.getOutputStream().flush();
            err = sys.getInputStream().readInt();
            length = sys.getInputStream().readInt();

            if (err == EtConstants.ok) {
                buf = new byte[length];
                sys.getInputStream().readFully(buf, 0, length);
            }
        }

        if (err == EtConstants.ok) {
            try {
                val = new String(buf, 0, length - 1, "ASCII");
            }
            catch (UnsupportedEncodingException ex) {
            }
        }
        else {
            if (length == 0) {
                return null;
            }
            throw new EtException("cannot find station");
        }

        return val;
    }

    /**
     * Gets the name of the library containing the station's user-defined select
     * function. This is only relevant for station's on C language ET systems.
     *
     * @return station's user-defined select function library
     * @throws IOException if there are problems with network communication
     * @throws EtException  if the station has been removed
     * @see EtStationConfig#selectLibrary
     */
    public String getSelectLibrary() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getStringValue(EtConstants.netStatLib);
    }

    /**
     * Gets the name of the station's user-defined select function.
     * This is only relevant for station's on C language ET systems.
     *
     * @return station's user-defined select function
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     * @see EtStationConfig#selectFunction
     */
    public String getSelectFunction() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getStringValue(EtConstants.netStatFunc);
    }

    /**
     * Gets the name of the class containing the station's user-defined select
     * method. This is only relevant for station's on Java language ET systems.
     *
     * @return station's user-defined select method class
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     * @see EtStationConfig#selectClass
     */
    public String getSelectClass() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getStringValue(EtConstants.netStatClass);
    }


    /**
     * This gets "integer" station parameter data over the network.
     *
     * @param cmd coded command to send to the TCP server thread.
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station cannot be found
     */
    private int getIntValue(int cmd) throws IOException, EtException {
        int err, val = 0;

        synchronized (sys) {
            sys.getOutputStream().writeInt(cmd);
            sys.getOutputStream().writeInt(id);
            sys.getOutputStream().flush();
            err = sys.getInputStream().readInt();
            val = sys.getInputStream().readInt();
        }
        
        if (err != EtConstants.ok) {
            throw new EtException("this station has been removed from ET system");
        }

        return val;
    }

    /**
     * This sets "integer" station parameter data over the network.
     *
     * @param cmd coded command to send to the TCP server thread.
     * @param val value to set.
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station cannot be found
     */
    private void setIntValue(int cmd, int val) throws IOException, EtException {
        int err;

        synchronized (sys) {
            sys.getOutputStream().writeInt(cmd);
            sys.getOutputStream().writeInt(id);
            sys.getOutputStream().writeInt(val);
            sys.getOutputStream().flush();
            err = sys.getInputStream().readInt();
        }

        if (err != EtConstants.ok) {
            throw new EtException("this station has been removed from ET system");
        }

        return;
  }

    /**
     * Gets the station's number of attachments.
     *
     * @return station's number of attachments
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     */
    public int getNumAttachments() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatGAtts);
    }

    /**
     * Gets the station's status. It may have the values
     * {@link EtConstants#stationUnused}, {@link EtConstants#stationCreating},
     * {@link EtConstants#stationIdle}, and {@link EtConstants#stationActive}.
     *
     * @return station's status
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     */
    public int getStatus() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatStatus);
    }

    /**
     * Gets the number of events in the station's input list.
     *
     * @return number of events in the station's input list
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     */
    public int getInputCount() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatInCnt);
    }

    /**
     * Gets the number of events in the station's output list.
     *
     * @return number of events in the station's output list
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     */
    public int getOutputCount() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatOutCnt);
    }

    /**
     * Gets the station configuration's block mode.
     *
     * @return station's block mode
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     * @see EtStationConfig#blockMode
     */
    public int getBlockMode() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatGBlock);
    }

    /**
     * Sets the station's block mode dynamically.
     *
     * @param mode block mode value
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed, bad mode value, or
     *                     the station is GRAND_CENTRAL
     * @see EtStationConfig#blockMode
     */
    public void setBlockMode(int mode) throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }

        if (id == 0) {
            throw new EtException("cannot modify GRAND_CENTRAL station");
        }

        if ((mode != EtConstants.stationBlocking) &&
            (mode != EtConstants.stationNonBlocking)) {
            throw new EtException("bad block mode value");
        }
        setIntValue(EtConstants.netStatSBlock, mode);
        return;
  }

    /**
     * Gets the station configuration's user mode.
     *
     * @return station's user mode
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     * @see EtStationConfig#userMode
     */
    public int getUserMode() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatGUser);
    }

    /**
     * Sets the station's user mode dynamically.
     *
     * @param mode user mode value
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed, bad mode value, or
     *                     the station is GRAND_CENTRAL
     * @see EtStationConfig#userMode
     */
    public void setUserMode(int mode) throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }

        if (id == 0) {
            throw new EtException("cannot modify GRAND_CENTRAL station");
        }

        if (mode < 0) {
            throw new EtException("bad user mode value");
        }

        setIntValue(EtConstants.netStatSUser, mode);
        return;
    }

    /**
     * Gets the station configuration's restore mode.
     *
     * @return station's restore mode
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     * @see EtStationConfig#restoreMode
     */
    public int getRestoreMode() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatGRestore);
    }

    /**
     * Sets the station's restore mode dynamically.
     *
     * @param mode restore mode value
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed, bad mode value, or
     *                     the station is GRAND_CENTRAL
     * @see EtStationConfig#restoreMode
     */
    public void setRestoreMode(int mode) throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }

        if (id == 0) {
            throw new EtException("cannot modify GRAND_CENTRAL station");
        }

        if ((mode != EtConstants.stationRestoreOut) &&
            (mode != EtConstants.stationRestoreIn) &&
            (mode != EtConstants.stationRestoreGC)) {
            throw new EtException("bad restore mode value");
        }

        setIntValue(EtConstants.netStatSRestore, mode);
        return;
    }

    /**
     * Gets the station configuration's select mode.
     *
     * @return station's select mode
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     * @see EtStationConfig#selectMode
     */
    public int getSelectMode() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatGSelect);
  }

    /**
     * Gets the station configuration's cue.
     *
     * @return station's cue
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     * @see EtStationConfig#cue
     */
    public int getCue() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatGCue);
    }

    /**
     * Sets the station's cue size dynamically.
     *
     * @param cue cue value
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed, bad cue value, or
     *                     the station is GRAND_CENTRAL
     * @see EtStationConfig#cue
     */
    public void setCue(int cue) throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }

        if (id == 0) {
            throw new EtException("cannot modify GRAND_CENTRAL station");
        }

        if (cue < 1) {
            throw new EtException("bad cue value");
        }

        setIntValue(EtConstants.netStatSCue, cue);
        return;
    }

    /**
     * Gets the station configuration's prescale.
     *
     * @return station's prescale
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed
     * @see EtStationConfig#prescale
     */
    public int getPrescale() throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }
        return getIntValue(EtConstants.netStatGPre);
    }

    /**
     * Sets the station's prescale dynamically.
     *
     * @param prescale prescale value
     * @throws IOException if there are problems with network communication
     * @throws EtException if the station has been removed, bad prescale value, or
     *                     the station is GRAND_CENTRAL
     * @see EtStationConfig#prescale
     */
    public void setPrescale(int prescale) throws IOException, EtException {
        if (!usable) {
            throw new EtException("station has been removed");
        }

        if (id == 0) {
            throw new EtException("cannot modify GRAND_CENTRAL station");
        }

        if (prescale < 1) {
            throw new EtException("bad prescale value");
        }
        
        setIntValue(EtConstants.netStatSPre, prescale);
        return;
    }


}
