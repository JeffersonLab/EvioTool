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

package org.jlab.coda.et.data;

import org.jlab.coda.et.EtConstants;
import org.jlab.coda.et.EtUtils;

import java.io.*;

/**
 * This class holds all information about an attachment. It parses
 * the information from a stream of data sent by an ET system.
 *
 * @author Carl Timmer
 */
public class AttachmentData {

    /** Attachment's id number.
     *  @see org.jlab.coda.et.EtAttachment#id
     *  @see org.jlab.coda.et.system.AttachmentLocal#id */
    private int num;

    /** Id number of ET process that created this attachment
     * (only relevant in C-based ET systems). */
    private int proc;

    /** Id number of the attachment's station.
     *  @see org.jlab.coda.et.EtAttachment#station
     *  @see org.jlab.coda.et.system.AttachmentLocal#station */
    private int stat;

    /** Unix process id of the program that created this attachment
     * (only relevant in C-based ET systems).
     *  @see org.jlab.coda.et.system.AttachmentLocal#pid */
    private int pid;

    /** Flag indicating if this attachment is blocked waiting to read events. Its
     *  value is {@link org.jlab.coda.et.EtConstants#attBlocked} if blocked and
     *  {@link org.jlab.coda.et.EtConstants#attUnblocked} otherwise.
     *  This is not boolean for C ET system compatibility.
     *  @see org.jlab.coda.et.system.AttachmentLocal#waiting */
    private int blocked;

    /** Flag indicating if this attachment has been told to quit trying to read
     *  events and return. Its value is {@link org.jlab.coda.et.EtConstants#attQuit} if it has been
     *  told to quit and {@link org.jlab.coda.et.EtConstants#attContinue} otherwise.
     *  This is not boolean for C ET system compatibility.
     *  @see org.jlab.coda.et.system.AttachmentLocal#wakeUp */
    private int quit;

    /** The number of events owned by this attachment */
    private int eventsOwned;

    /** Number of events put back into the station.
     *  @see org.jlab.coda.et.EtAttachment#getEventsPut
     *  @see org.jlab.coda.et.system.AttachmentLocal#eventsPut */
    private long eventsPut;

    /** Number of events gotten from the station.
     *  @see org.jlab.coda.et.EtAttachment#getEventsGet
     *  @see org.jlab.coda.et.system.AttachmentLocal#eventsGet */
    private long eventsGet;

    /** Number of events dumped (recycled by returning to GRAND_CENTRAL) through the station.
     *  @see org.jlab.coda.et.EtAttachment#getEventsDump
     *  @see org.jlab.coda.et.system.AttachmentLocal#eventsDump */
    private long eventsDump;

    /** Number of new events gotten from the station.
     *  @see org.jlab.coda.et.EtAttachment#getEventsMake
     *  @see org.jlab.coda.et.system.AttachmentLocal#eventsMake */
    private long eventsMake;

    /** Name of the host running this attachment.
     *  @see org.jlab.coda.et.system.AttachmentLocal#host */
    private String host;

    /**  Name of the station this attachment is associated with. */
    private String stationName;

    /** IP address of the network interface the attachment is sending data through. */
    private String ipAddress;


    // getters


    /** Get the attachment's id number.
     *  @return attachment's id number
     *  @see org.jlab.coda.et.EtAttachment#id
     *  @see org.jlab.coda.et.system.AttachmentLocal#id */
    public int getId() {return num;}

    /** Get the id number of ET process that created this attachment
     *  (only relevant in C-based ET systems).
     *  @return id number of ET process that created this attachment */
    public int getProc() {return proc;}

    /** Get the id number of the station to which this attachment belongs.
     *  @return id number of station to which this attachment belongs
     *  @see org.jlab.coda.et.EtAttachment#station
     *  @see org.jlab.coda.et.system.AttachmentLocal#station */
    public int getStationId() {return stat;}

    /** Get the Unix process id of the program that created this attachment
     * (only relevant in C-based ET systems).
     *  @return Unix process id of the program that created this attachment
     *  @see org.jlab.coda.et.system.AttachmentLocal#pid */
    public int getPid() {return pid;}

    /** Indicates if this attachment is blocked waiting to read events.
     *  @return <code>true</code> if blocked waiting to read events, else <code>false</code>
     *  @see org.jlab.coda.et.system.AttachmentLocal#waiting */
    public boolean blocked() {return blocked == EtConstants.attBlocked;}

    /** Indicates if this attachment has been told to quit trying to read events and return.
     *  @return <code>true</code> if this attachment has been told to quit trying to read
     *          events and return, else <code>false</code>
     *  @see org.jlab.coda.et.system.AttachmentLocal#wakeUp */
    public boolean quitting() {return quit == EtConstants.attQuit;}


    /** Get the number of events owned by this attachment.
     *  @return number of events owned by this attachment */
    public int getEventsOwned() {return eventsOwned;}

    /** Get the number of events put back into the station.
     *  @return number of events put back into the station
     *  @see org.jlab.coda.et.EtAttachment#getEventsPut
     *  @see org.jlab.coda.et.system.AttachmentLocal#eventsPut */
    public long getEventsPut() {return eventsPut;}

    /** Get the number of events gotten from the station.
     *  @return number of events gotten from the station
     *  @see org.jlab.coda.et.EtAttachment#getEventsGet
     *  @see org.jlab.coda.et.system.AttachmentLocal#eventsGet */
    public long getEventsGet() {return eventsGet;}

    /** Get the number of events dumped (recycled by returning to GRAND_CENTRAL)
     *  through the station.
     *  @return number of events dumped through the station
     *  @see org.jlab.coda.et.EtAttachment#getEventsDump
     *  @see org.jlab.coda.et.system.AttachmentLocal#eventsDump */
    public long getEventsDump() {return eventsDump;}

    /** Get the number of new events gotten from the station.
     *  @return number of new events gotten from the station
     *  @see org.jlab.coda.et.EtAttachment#getEventsMake
     *  @see org.jlab.coda.et.system.AttachmentLocal#eventsMake */
    public long getEventsMake() {return eventsMake;}

    /** Get the name of the host running this attachment.
     *  @return name of the host running this attachment
     *  @see org.jlab.coda.et.system.AttachmentLocal#host */
    public String getHost() {return host;}

    /** Get the name of the station this attachment is associated with.
     *  @return name of the station this attachment is associated with */
    public String getStationName() {return stationName;}

    /** Get the IP address of the network interface the attachment is sending data through.
     *  @return IP address of the network interface the attachment is sending data through. */
    public String getIpAddress() {return ipAddress;}

    /**
     *  Reads the attachment information from an ET system over the network.
     *  @param dis data input stream
     *  @throws IOException if data read error
     */
    public void read(DataInputStream dis) throws IOException {
        byte[] info = new byte[72];
        dis.readFully(info);

        num         = EtUtils.bytesToInt(info,   0);
        proc        = EtUtils.bytesToInt(info,   4);
        stat        = EtUtils.bytesToInt(info,   8);
        pid         = EtUtils.bytesToInt(info,  12);
        blocked     = EtUtils.bytesToInt(info,  16);
        quit        = EtUtils.bytesToInt(info,  20);
        eventsOwned = EtUtils.bytesToInt(info,  24);
        eventsPut   = EtUtils.bytesToLong(info, 28);
        eventsGet   = EtUtils.bytesToLong(info, 36);
        eventsDump  = EtUtils.bytesToLong(info, 44);
        eventsMake  = EtUtils.bytesToLong(info, 52);

        // read strings, lengths first
        int length1 = EtUtils.bytesToInt(info, 60);
        int length2 = EtUtils.bytesToInt(info, 64);
        int length3 = EtUtils.bytesToInt(info, 68);

        if (length1 + length2 + length3 > 72) {
            info = new byte[length1 + length2 + length3];
        }
        dis.readFully(info, 0, length1 + length2 + length3);
        host = new String(info, 0, length1 - 1, "US-ASCII");
        stationName = new String(info, length1, length2 - 1, "US-ASCII");
        ipAddress = new String(info, length1+length2, length3 - 1, "US-ASCII");
    }
}



