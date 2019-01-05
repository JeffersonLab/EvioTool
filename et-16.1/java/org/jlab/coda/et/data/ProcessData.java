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

import java.io.*;

/**
 * This class holds all information about an ET process. It parses
 * the information from a stream of data sent by an ET system. There
 * are no processes in Java ET systems.
 *
 * @author Carl Timmer
 */
public class ProcessData {

    /** Unique id number. */
    private int num;

    /** Heartbeat count. */
    private int heartbeat;

    /** Unix process id. */
    private int pid;

    /** Number of attachments this process created. */
    private int attachments;

    /**
     * An array of attachment id numbers. Only the first "attachments"
     * number of elements are meaningful.
     */
    private int attIds[] = new int[EtConstants.attachmentsMax];


    // get methods


    /**
     * Get the process' unique id number.
     * @return process' unique id number
     */
    public int getId() {
        return num;
    }

    /**
     * Get the heartbeat count.
     * @return heartbeat count
     */
    public int getHeartbeat() {
        return heartbeat;
    }

    /**
     * Get the Unix process id.
     * @return Unix process id
     */
    public int getPid() {
        return pid;
    }

    /**
     * Get the number of attachments this process created.
     * @return number of attachments this process created
     */
    public int getAttachments() {
        return attachments;
    }

    /**
     * Get the array of attachment id numbers.
     * @return array of attachment id numbers
     */
    public int[] getAttachmentIds() {
        return attIds.clone();
    }

    /**
     * Reads the process information from an ET system over the network.
     *
     * @param dis data input stream
     * @throws java.io.IOException if data read error
     */
    public void read(DataInputStream dis) throws IOException {
        attachments = dis.readInt();
        num         = dis.readInt();
        heartbeat   = dis.readInt();
        pid         = dis.readInt();
        for (int i = 0; i < attachments; i++) {
            attIds[i] = dis.readInt();
        }
    }
}







