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


/**
 * This class holds all the information about an ET system. This includes
 * information about attachments, stations, processes, and the system
 * level.
 *
 * @author Carl Timmer
 */
public class AllData {

    /**
     * Creates the bare minimum needed to hold the information of an ET system.
     * Information about stations, attachments, and processes are added by
     * other methods.
     *
     * @see org.jlab.coda.et.EtSystem#getData
     */
    public AllData() {
        sysData = new SystemData();
    }

    /** Holds system information. */
    public SystemData sysData;

    /** Holds station information. */
    public StationData statData[];

    /** Holds attachment information. */
    public AttachmentData attData[];

    /**
     * Hold process information. There are no processes (local processes which
     * have direct access to the ET shared memory) in Java language ET.
     */
    public ProcessData procData[];
}
