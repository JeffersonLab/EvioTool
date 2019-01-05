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

package org.jlab.coda.et.exception;
import java.lang.*;

/**
 * This class represents an error of an ET system when an attachment has been
 * told to wake up from a blocking read.
 *
 * @author Carl Timmer
 */

public class EtWakeUpException extends Exception {

    /**
     * Create an exception indicating an error of an ET system
     * when an attachment has been told to wake up from a blocking read.
     *
     * @param message the detail message. The detail message is saved for
     *        later retrieval by the {@link #getMessage()} method.
     */
    public EtWakeUpException(String message) {
        super(message);
    }

}
