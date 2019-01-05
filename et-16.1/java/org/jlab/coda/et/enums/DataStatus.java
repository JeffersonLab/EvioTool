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

package org.jlab.coda.et.enums;

import org.jlab.coda.et.EtConstants;

/**
 * This enum represents the 3 possible states of an event's data's status.
 * OK is default. CORRUPT is never used. POSSIBLYCORRUPT is assigned to events
 * whose owning process crashed and were recovered by the system.
 * 
 * @author timmer
 */
public enum DataStatus {
    /** Data is OK or uncorrupted. */
    OK               (EtConstants.dataOk),
    /** Data is worthless or corrupted. */
    CORRUPT          (EtConstants.dataCorrupt),
    /** Data status is unkown and might be corrupted. */
    POSSIBLYCORRUPT  (EtConstants.dataPossiblyCorrupt);

    private int value;

    private DataStatus(int value) {
        this.value = value;
    }

    /**
     * Get the enum's value.
     * @return the value
     */
    public int getValue() {
        return value;
    }

    /**
     * Obtain the name from the value.
     *
     * @param value the value to match.
     * @return the name, or null.
     */
    public static String getName(int value) {
        DataStatus stats[] = DataStatus.values();
        for (DataStatus s : stats) {
            if (s.value == value) {
                return s.name();
            }
        }
        return null;
    }

    /**
     * Obtain the enum from the value.
     *
     * @param value the value to match.
     * @return the matching enum, or <code>null</code>.
     */
    public static DataStatus getStatus(int value) {
        DataStatus stats[] = DataStatus.values();
        for (DataStatus s : stats) {
            if (s.value == value) {
                return s;
            }
        }
        return null;
    }

}
