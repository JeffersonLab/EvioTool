package org.jlab.coda.jevio;

/**
 * This is a general exception used to indicate a problem in the Jevio package.
 * 
 * @author heddle
 * 
 */
@SuppressWarnings("serial")
public class EvioException extends Exception {

    /**
     * Create an EVIO Exception indicating an error specific to the EVIO system.
     *
     * @param message the detail message. The detail message is saved for
     *                later retrieval by the {@link #getMessage()} method.
     */
    public EvioException(String message) {
        super(message);
    }

    /**
     * Create an EVIO Exception with the specified message and cause.
     *
     * @param  message the detail message. The detail message is saved for
     *         later retrieval by the {@link #getMessage()} method.
     * @param  cause the cause (which is saved for later retrieval by the
     *         {@link #getCause()} method).  (A <tt>null</tt> value is
     *         permitted, and indicates that the cause is nonexistent or
     *         unknown.)
     */
    public EvioException(String message, Throwable cause) {
        super(message, cause);
    }

    /**
     * Create an EVIO Exception with the specified cause.
     *
     * @param  cause the cause (which is saved for later retrieval by the
     *         {@link #getCause()} method).  (A <tt>null</tt> value is
     *         permitted, and indicates that the cause is nonexistent or
     *         unknown.)
     */
    public EvioException(Throwable cause) {
        super(cause);
    }

}
