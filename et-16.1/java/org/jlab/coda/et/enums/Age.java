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
 * This enum indicates whether an event is a new or unused event, obtained by a call to
 * {@link org.jlab.coda.et.EtSystem#newEvents}, or whether it is an existing or used event, obtained by
 * a call to {@link org.jlab.coda.et.EtSystem#getEvents}.
 *  
 * @author timmer
 */
public enum Age {

    /** Existing event with data, obtained through getEvents(). */
    USED     (EtConstants.eventUsed),
    /** New event with no data, obtained through newEvents(). */
    NEW      (EtConstants.eventNew);

    
    private int value;

    private Age(int value) {
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
        Age ages[] = Age.values();
        for (Age a : ages) {
            if (a.value == value) {
                return a.name();
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
    public static Age getAge(int value) {
        Age ages[] = Age.values();
        for (Age a : ages) {
            if (a.value == value) {
                return a;
            }
        }
        return null;
    }
}
