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


/**
 * This class defines an attachment to a station of an ET system for use by the
 * ET system itself and not the user. Attachments are used to keep track of
 * event ownership and as places to conveniently keep some statistics and other
 * information on the "getting" of events. Attachments can only be created by an
 * ET system's {@link SystemCreate#attach(int)} method.
 *
 * @author Carl Timmer
 */

public class AttachmentLocal {

    // TODO: keep a list or set of events we currently have out?

    /** Unique id number. */
    private int id;

    /** Process id number for attachments written in C language. */
    private int pid;

    /** Name of the host the attachment is residing on. */
    private String host;

    /** IP address of the network interface the attachment is sending data through. */
    private String ipAddress;

    /** Station the attachment is associated with. */
    private StationLocal station;

    /** Number of events put by a user into the attachment. */
    private long eventsPut;

    /** Number of events gotten by a user from the attachment. */
    private long eventsGet;

    /** Number of events dumped (recycled by returning to GRAND_CENTRAL station)
     *  by a user through the attachment. */
    private long eventsDump;

    /** Number of new events gotten by a user from the attachment. */
    private long eventsMake;

    /** Flag telling whether the attachment is blocked waiting to read events
     *  from a station that has no events.  */
    private boolean waiting;

    /**
     * Flag telling whether the attachment is currently in the sleep mode of
     * getEvents or newEvents. Since the implementation of this mode is
     * done by using the timed wait mode, it occasionally happens that the
     * attachment is told to wake up while it is not actually in getEvents or
     * newEvents. If this flag is true, the command to wake up will go ahead
     * and set "wakeUp" to true - even if "waiting" is false.
     */
    private volatile boolean sleepMode;

    /** Flag telling the attachment blocked on a read to wake up or return. */
    private volatile boolean wakeUp;


    /**
     * Constructor. Attachments are only created by an ET system's
     * {@link SystemCreate#attach(int)} method.
     */
    AttachmentLocal() {
        id         = -1;
        pid        = -1;
    }


    /**
     * Gets the attachment id number.
     * @return attachment id number
     */
    public int getId() {
        return id;
    }

    /**
     * Sets the attachment id number.
     * @param id attachment id number
     */
    public void setId(int id) {
        this.id = id;
    }

    /**
     * Gets the process id number for C clients.
     * @return the process id number for C clients
     */
    public int getPid() {
        return pid;
    }

    /**
     * Set the process id number.
     * @param pid he process id number
     */
    public void setPid(int pid) {
        this.pid = pid;
    }

    /**
     * Get the host the attachment is residing on.
     * @return host the attachment is residing on
     */
    public String getHost() {
        return host;
    }

    /**
     * Set the host the attachment is residing on.
     * @param host host the attachment is residing on
     */
    public void setHost(String host) {
        this.host = host;
    }

    /**
     * Get the IP address of the network interface the attachment is sending data through.
     * @return IP address of the network interface the attachment is sending data through.
     */
    public String getIpAddress() {
        return ipAddress;
    }

    /**
     * Set the IP address of the network interface the attachment is sending data through.
     * @param ipAddress IP address of the network interface the attachment is sending data through.
     */
    public void setIpAddress(String ipAddress) {
        this.ipAddress = ipAddress;
    }

    /**
     * Is the attachment blocked waiting to read events from a station that has no events?
     * @return <code>true</code> if attachment is blocked waiting to read events from a station that has no events
     */
    public boolean isWaiting() {
        return waiting;
    }

    /**
     * Set if attachment is blocked waiting to read events from a station that has no events
     * @param waiting is attachment blocked waiting to read events from a station that has no events?
     */
    public void setWaiting(boolean waiting) {
        this.waiting = waiting;
    }

    /**
     * Is this attachment to wake up or return during blocking read?
     * @return <code>true</code> if this attachment is to wake up or return during blocking read
     */
    public boolean isWakeUp() {
        return wakeUp;
    }

    /**
     * Set the flag to wake up or return during blocking read.
     * @param wakeUp flag to wake up or return during blocking read
     */
    public void setWakeUp(boolean wakeUp) {
        this.wakeUp = wakeUp;
    }

    /**
     * Is this attachment currently in the sleep mode of getEvents or newEvents?
     * @return <code>true</code> if this attachment is currently in the sleep mode of getEvents or newEvents
     */
    public boolean isSleepMode() {
        return sleepMode;
    }

    /**
     * Set the flag to be in the sleep mode of getEvents or newEvents.
     * @param sleepMode sleep mode of getEvents or newEvents
     */
    public void setSleepMode(boolean sleepMode) {
        this.sleepMode = sleepMode;
    }

    /**
     * Get the station this attachment is associated with.
     * @return station this attachment is associated with
     */
    public StationLocal getStation() {
        return station;
    }

    /**
     * Set the station this attachment is associated with.
     * @param station station this attachment is associated with
     */
    public void setStation(StationLocal station) {
        this.station = station;
    }

    /**
     * Get the number of events put by a user into this attachment.
     * @return number of events put by a user into this attachment
     */
    public long getEventsPut() {
        return eventsPut;
    }

    /**
     * Get the number of events put by a user into this attachment.
     * @param eventsPut number of events put by a user into this attachment
     */
    public void setEventsPut(long eventsPut) {
        this.eventsPut = eventsPut;
    }

    /**
     * Get the number of events gotten by a user from this attachment.
     * @return number of events gotten by a user from this attachment
     */
    public long getEventsGet() {
        return eventsGet;
    }

    /**
     * Set the number of events gotten by a user from this attachment
     * @param eventsGet number of events gotten by a user from this attachment
     */
    public void setEventsGet(long eventsGet) {
        this.eventsGet = eventsGet;
    }

    /**
     * Get the number of events dumped (recycled by returning to GRAND_CENTRAL station)
     * by a user through this attachment.
     * @return number of events dumped by a user through this attachment
     */
    public long getEventsDump() {
        return eventsDump;
    }

    /**
     * Set the number of events dumped (recycled by returning to GRAND_CENTRAL station)
     * by a user through this attachment.
     * @param eventsDump number of events dumped by a user through this attachment.
     */
    public void setEventsDump(long eventsDump) {
        this.eventsDump = eventsDump;
    }

    /**
     * Get the number of new events gotten by a user from this attachment.
     * @return number of new events gotten by a user from this attachment
     */
    public long getEventsMake() {
        return eventsMake;
    }

    /**
     * Set the number of new events gotten by a user from this attachment.
     * @param eventsMake number of new events gotten by a user from this attachment
     */
    public void setEventsMake(long eventsMake) {
        this.eventsMake = eventsMake;
    }


}



