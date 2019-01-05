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

import java.io.*;

import org.jlab.coda.et.exception.*;

/**
 * This class defines an ET system user's attachment to a station.
 * Attachments can only be created by an ET system's {@link EtSystem#attach(EtStation)}
 * method. Attachments are means of designating the
 * ownership of events and keeping track of events.
 *
 * @author Carl Timmer
 */
public class EtAttachment {

    // TODO: keep a list or set of events we currently have out?

    /** Unique id number. */
    private int id;

    /** ET system the attachment is associated with. */
    private EtSystem sys;

    /** Station the attachment is associated with. */
    private EtStation station;

    /**
     * Flag telling whether this attachment object is usable or the attachment it
     * represents has been detached. Set by the user's ET system object.
     */
    private boolean usable;


    /**
     * Constructor for creating an attachment to a specific ET system and station.
     * Attachments can only be created by an ET system's {@link EtSystem#attach(EtStation)}
     * method.
     *
     * @param station  station object
     * @param id       unique attachment id number
     * @param sys      ET system object
     */
    EtAttachment(EtStation station, int id, EtSystem sys) {
        this.id      = id;
        this.sys     = sys;
        this.station = station;
    }


    // Getters/Setters


    /**
     * Gets the object of the station attached to.
     * @return object of station attached to
     */
    public EtStation getStation() {return station;}

    /**
     * Gets the id number of this attachment.
     * @return id number of this attachment
     */
    public int getId() {return id;}

    /**
     * Tells if this attachment object is usable.
     * @return <code>true</code> if attachment object is usable and <code>false
     * </code> otherwise
     */
    public boolean isUsable() {return usable;}

    /**
     * Sets whether this attachment object is usable or not.
     * @param usable <code>true</code> if this attachment object is usable and <code>false otherwise
     */
    void setUsable(boolean usable) {
        this.usable = usable;
    }

    /**
     * Sets the EtSystemUse object for using the ET system.
     * @return the EtSystemUse object for using the ET system
     */
    public EtSystem getSys() {
        return sys;
    }

    /**
     * Gets the value of an attachment's eventsPut, eventsGet, eventsDump, or
     * eventsMake by network communication with the ET system.
     *
     * @param cmd command number
     * @return value of requested parameter
     * @throws IOException if there are network communication problems
     * @throws EtException if the station no longer exists
     */
    private long getLongValue(int cmd) throws IOException, EtException {
        int  err;
        long val;

        synchronized (sys) {
            sys.getOutputStream().writeInt(cmd);
            sys.getOutputStream().writeInt(id);
            sys.getOutputStream().flush();
            err = sys.getInputStream().readInt();
            val = sys.getInputStream().readLong();
        }

        if (err != EtConstants.ok) {
            throw new EtException("this station has been revmoved from ET system");
        }

        return val;
    }

    /**
     * Gets the number of events put into the ET system by this attachment.
     * @return number of events put into the ET system by this attachment
     */
    public long getEventsPut() throws IOException, EtException {
        return getLongValue(EtConstants.netAttPut);
    }

    /**
     * Gets the number of events gotten from the ET system by this attachment.
     * @return number of events gotten from the ET system by this attachment
     */
    public long getEventsGet() throws IOException, EtException {
        return getLongValue(EtConstants.netAttGet);
    }

    /**
     * Gets the number of events dumped (recycled by returning to GRAND_CENTRAL
     * station) through this attachment.
     *
     * @return number of events dumped into the ET system by this attachment
     */
    public long getEventsDump() throws IOException, EtException {
        return getLongValue(EtConstants.netAttDump);
    }

    /**
     * Gets the number of new events gotten from the ET system by this attachment.
     * @return number of new events gotten from the ET system by this attachment
     */
    public long getEventsMake() throws IOException, EtException {
        return getLongValue(EtConstants.netAttMake);
  }
}



