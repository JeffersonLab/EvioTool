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
 * This class represents a general error of an ET system.
 *
 * @author Carl Timmer
 */

public class EtException extends Exception {

    /**
     * Create an exception indicating an error specific to the ET system.
     *
     * @param message the detail message. The detail message is saved for
     *        later retrieval by the {@link #getMessage()} method.
     */
    public EtException(String message) {
        super(message);
    }

    /**
      * Constructs a new exception with the specified detail message and
      * cause.  <p>Note that the detail message associated with
      * <code>cause</code> is <i>not</i> automatically incorporated in
      * this exception's detail message.
      *
      * @param message the detail message (which is saved for later retrieval
      *                by the {@link #getMessage()} method).
      * @param cause   the cause (which is saved for later retrieval by the
      *                {@link #getCause()} method).  (A <tt>null</tt> value is
      *                permitted, and indicates that the cause is nonexistent or
      *                unknown.)
      * @since 14.0
      */
     public EtException(String message, Throwable cause) {
         super(message, cause);
     }
}
