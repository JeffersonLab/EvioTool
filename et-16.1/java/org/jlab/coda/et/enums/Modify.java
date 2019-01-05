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
 * This enum represents the 3 possible modifications a networked user can make to
 * an event. NOTHING means no changes will be made. HEADER means only information
 * in the event header (non-data) will possibly be modified. ANYTHING means that either
 * header or data will possibly be modified. If the user gets events in which NOTHING will
 * be modified, the server sends a copy of the event to the user over the network
 * and immediately puts the originals back into the system. This greatly increases
 * performance.
 *
 * @author timmer
 */
public enum Modify {
    /** Network user will make no changes to data or header (non-data). Event is readonly. */
    NOTHING  (0),
    /** Network user may make changes to data and/or header (non-data). Event is read-write. */
    ANYTHING (EtConstants.modify),
    /** Network user may make changes to header (non-data) only. */
    HEADER   (EtConstants.modifyHeader);

    private int value;

    private Modify(int value) {
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
        Modify mods[] = Modify.values();
        for (Modify m : mods) {
            if (m.value == value) {
                return m.name();
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
    public static Modify getModify(int value) {
        Modify mods[] = Modify.values();
        for (Modify m : mods) {
            if (m.value == value) {
                return m;
            }
        }
        return null;
    }


}
