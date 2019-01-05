package org.jlab.coda.et;

import org.jlab.coda.et.exception.*;

import java.util.concurrent.locks.ReentrantLock;
import java.util.HashMap;


/**
 * This class handles all calls to native methods which, in turn,
 * makes calls to the C ET library routines to get, new, put, and dump events.
 */
class EtJniAccess {

    /** Serialize access to classMap and creation of these objects. */
    static ReentrantLock classLock = new ReentrantLock();

    /** Store EtJniAccess objects here since we only want to create 1 object per ET system. */
    static HashMap<String, EtJniAccess> classMap = new HashMap<String, EtJniAccess>(10);

    /** Has the jni library been loaded? */
    static boolean jniLibLoaded;


    /**
     * Get an instance of this object for a particular ET system. Only one EtJniAccess object
     * is created for a particular ET system.
     *
     * @param etName name of ET system to open
     * @return object of this type to use for interaction with local, C-based ET system
     * @throws EtException        for failure to load the jni library;
     *                            for any failure to open ET system except timeout
     * @throws EtTimeoutException for failure to open ET system within the specified time limit
     */
    static EtJniAccess getInstance(String etName) throws EtException, EtTimeoutException {
        try {

            classLock.lock();

            // Only want to do this once
            if (!jniLibLoaded) {
                try {
                    System.loadLibrary("et_jni");
                }
                catch (Error e) {
                    // If the library cannot be found, we can still
                    // continue to use the ET system with sockets -
                    // just not locally with the C library.
//System.out.println("\nERROR LOADIN JNI LIB !!!!\n");
                    throw new EtException("Error loading libet_jni.so");
                }
            }

            jniLibLoaded = true;

            // See if we've already opened the ET system being asked for, if so, return that
            if (classMap.containsKey(etName)) {
//System.out.println("USE ALREADY EXISTING ET JNI OBJECT for et -> " + etName);
                EtJniAccess jni = classMap.get(etName);
                jni.numberOpens++;
//System.out.println("numberOpens = " + jni.numberOpens);
                return jni;
            }

            EtJniAccess jni = new EtJniAccess();
            jni.openLocalEtSystem(etName);
            jni.etSystemName = etName;
            jni.numberOpens = 1;
//System.out.println("CREATING ET JNI OBJECT for et -> " + etName);
//System.out.println("numberOpens = " + jni.numberOpens);
            classMap.put(etName, jni);

            return jni;
        }
        finally {
            classLock.unlock();
        }
    }

    private int numberOpens;

    /** Place to store id (pointer) returned from et_open in C code. */
    private long localEtId;

    /** Store the name of the ET system. */
    private String etSystemName;

    /**
     * Create EtJniAccess objects with the {@link #getInstance(String)} method.
     */
    private EtJniAccess() {}


    /**
     * Get the et id.
     * @return et id
     */
    long getLocalEtId() {
        return localEtId;
    }


    /**
     * Set the et id. Used inside native method {@link #openLocalEtSystem(String)}.
     * @param id et id
     */
    private void setLocalEtId(long id) {
        localEtId = id;
    }


    /**
     * Open a local, C-based ET system and store it's id in {@link #localEtId}.
     * This only needs to be done once per local system even though many connections
     * to the ET server may be desired.
     *
     * @param etName name of ET system to open
     *
     * @throws EtException        for any failure to open ET system except timeout
     * @throws EtTimeoutException for failure to open ET system within the specified time limit
     */
    private native void openLocalEtSystem(String etName)
            throws EtException, EtTimeoutException;


    /**
     * Close the local, C-based ET system that we previously opened.
     */
    void close() {
        try {
            classLock.lock();
            numberOpens--;
//System.out.println("close: numberOpens = " + numberOpens);
            if (numberOpens < 1) {
                classMap.remove(etSystemName);
//System.out.println("close: really close local ET system");
                closeLocalEtSystem(localEtId);
            }
        }
        finally {
            classLock.unlock();
        }
    }


    /**
     * Close the local, C-based ET system that we previously opened.
     *
     * @param etId ET system id
     */
    private native void closeLocalEtSystem(long etId);


    /**
     * Put the given array of events back into the local, C-based ET system.
     *
     * @param etId   ET system id
     * @param attId  attachment id
     * @param evs    array of events
     * @param length number of events to be put (starting at index of 0)
     * 
     * @throws EtException for variety of general errors
     * @throws EtDeadException if ET system is dead
     * @throws EtClosedException if ET system is closed
     */
    native void putEvents(long etId, int attId, EtEventImpl[] evs, int length)
            throws EtException, EtDeadException, EtClosedException;


    /**
     * Dump (dispose of) the given array of unwanted events back into the local, C-based ET system.
     *
     * @param etId   ET system id
     * @param attId  attachment id
     * @param evs    array of event objects
     * @param length number of events to be dumped (starting at index of 0)
     *
     * @throws EtException for variety of general errors
     * @throws EtDeadException if ET system is dead
     * @throws EtClosedException if ET system is closed
     */
    native void dumpEvents(long etId, int attId, EtEventImpl[] evs, int length)
            throws EtException, EtDeadException, EtClosedException;


    /**
     * Get events from the local, C-based ET system.
     *
     * @param etId   ET system id
     * @param attId  attachment id
     * @param mode   if there are no events available, this parameter specifies
     *               whether to wait for some by sleeping {@link EtConstants#sleep},
     *               to wait for a set time {@link EtConstants#timed},
     *               or to return immediately {@link EtConstants#async}.
     * @param sec    the number of seconds to wait if a timed wait is specified
     * @param nsec   the number of nanoseconds to wait if a timed wait is specified
     * @param count  number of events desired. Size may be different from that requested.
     *
     * @return array of events obtained from ET system. Count may be different from that requested.
     *
     * @throws EtException
     *     if general errors
     * @throws EtDeadException
     *     if the ET system process is dead
     * @throws EtClosedException
     *     if ET system is closed
     * @throws EtEmptyException
     *     if the mode is asynchronous and the station's input list is empty
     * @throws EtBusyException
     *     if the mode is asynchronous and the station's input list is being used
     *     (the mutex is locked)
     * @throws EtTimeoutException
     *     if the mode is timed wait and the time has expired
     * @throws EtWakeUpException
     *     if the attachment has been commanded to wakeup,
     *     {@link org.jlab.coda.et.system.EventList#wakeUp(org.jlab.coda.et.system.AttachmentLocal)},
     *     {@link org.jlab.coda.et.system.EventList#wakeUpAll}
     */
    native EtEventImpl[] getEvents(long etId, int attId, int mode, int sec, int nsec, int count)
            throws EtException, EtDeadException, EtClosedException, EtEmptyException,
                   EtBusyException, EtTimeoutException, EtWakeUpException;


    /**
     * Get new (unused) events from a specified group of such events from the local, C-based ET system.
     *
     * @param etId   ET system id
     * @param attId  attachment id
     * @param mode   if there are no new events available, this parameter specifies
     *               whether to wait for some by sleeping {@link EtConstants#sleep},
     *               to wait for a set time {@link EtConstants#timed},
     *               or to return immediately {@link EtConstants#async}.
     * @param sec    the number of seconds to wait if a timed wait is specified
     * @param nsec   the number of nanoseconds to wait if a timed wait is specified
     * @param count  number of events desired
     * @param size   the size in bytes of the events desired
     * @param group  group number from which to draw new events. Some ET systems have
     *               unused events divided into groups whose numbering starts at 1.
     *               For ET system not so divided, all events belong to group 1.
     *
     * @return array of unused events obtained from ET system. Count may be different from that requested.
     *
     * @throws EtException
     *     if general errors
     * @throws EtDeadException
     *     if the ET system process is dead
     * @throws EtClosedException
     *     if ET system is closed
     * @throws EtEmptyException
     *     if the mode is asynchronous and the station's input list is empty
     * @throws EtBusyException
     *     if the mode is asynchronous and the station's input list is being used
     *     (the mutex is locked)
     * @throws EtTimeoutException
     *     if the mode is timed wait and the time has expired
     * @throws EtWakeUpException
     *     if the attachment has been commanded to wakeup,
     *     {@link org.jlab.coda.et.system.EventList#wakeUp(org.jlab.coda.et.system.AttachmentLocal)},
     *     {@link org.jlab.coda.et.system.EventList#wakeUpAll}
     */
    native EtEventImpl[] newEvents(long etId, int attId, int mode, int sec, int nsec, int count, int size, int group)
            throws EtException, EtDeadException, EtClosedException, EtEmptyException,
                   EtBusyException, EtTimeoutException, EtWakeUpException;
}
