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
 * This enum represents the 2 possible priorities an event can have.
 * Low is normal, but high puts events to the front of station input/output lists.
 * @author timmer
 */
public enum Priority {
    /** Low or normal priority, events take their proper turn. */
    LOW      (EtConstants.low),
    /** High priority, events cut in and move to the front of the list. */
    HIGH     (EtConstants.high);

    private int value;

    private Priority(int value) {
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
        Priority pris[] = Priority.values();
        for (Priority p : pris) {
            if (p.value == value) {
                return p.name();
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
    public static Priority getPriority(int value) {
        Priority pris[] = Priority.values();
        for (Priority p : pris) {
            if (p.value == value) {
                return p;
            }
        }
        return null;
    }

}
