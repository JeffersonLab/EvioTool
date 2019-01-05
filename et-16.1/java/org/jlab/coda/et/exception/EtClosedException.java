/*----------------------------------------------------------------------------*
 *  Copyright (c) 2013        Jefferson Science Associates,                   *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, Suite 3       *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*/

package org.jlab.coda.et.exception;

/**
 * This class represents an error of an ET system when a user tries to
 * call event put/dump/get/new routines or any station-related routines
 * and the et system has already been closed.
 *
 * @author Carl Timmer
 * @since 14.0
 */

public class EtClosedException extends Exception {

    /**
     * Create an exception indicating a user is trying to
     * call event put/dump/get/new routines or any station-related routines
     * and the et system has already been closed.
     *
     * @param message the detail message. The detail message is saved for
     *        later retrieval by the {@link #getMessage()} method.
     */
    public EtClosedException(String message) {
        super(message);
    }

}
