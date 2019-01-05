package org.jlab.coda.et.exception;

/**
 * This class represents an error of an ET system when its processes are dead.
 *
 * @author Carl Timmer
 */
public class EtDeadException extends Exception {

    /**
     * Create an exception indicating an error of an ET system when its processes are dead.
     *
     * @param message the detail message. The detail message is saved for
     *        later retrieval by the {@link #getMessage()} method.
     */
    public EtDeadException(String message) {
        super(message);
    }

}
