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

package org.jlab.coda.et;

import org.jlab.coda.et.exception.EtException;
import org.jlab.coda.et.enums.Modify;
import org.jlab.coda.et.enums.Priority;
import org.jlab.coda.et.enums.DataStatus;
import org.jlab.coda.et.enums.Age;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Interface used to define methods necessary to be an Event.
 */
public interface EtEvent {

    /**
     * Initialize an event's fields. Called for an event each time it passes
     * through GRAND_CENTRAL station.
     */
    public void init();


    // getters


    /**
     * Gets the event's id number.
     * @return event's id number.
     */
    public int getId();

    /**
     * Gets the age of the event, either {@link Age#NEW} if a new event obtained through
     * calling {@link EtSystem#newEvents(EtAttachment, org.jlab.coda.et.enums.Mode, int, int, int)}
     * or {@link Age#USED} if obtained through calling
     * {@link EtSystem#getEvents(EtAttachment, org.jlab.coda.et.enums.Mode, org.jlab.coda.et.enums.Modify, int, int)}.
     *
     * @return age of the event.
     */
    public Age getAge();

    /**
     * Gets the group the event belongs to (1, 2, ...) if ET system events are divided into groups.
     * If not, group = 1. Used so some producers don't hog events from others.
     *
     * @return the group the event belongs to.
     */
    public int getGroup();

    /**
     * Gets the event's priority, either high {@link Priority#HIGH} or low {@link Priority#LOW}.
     * Low priority is normal while high priority events get placed at the front of stations'
     * input and output event lists.
     * 
     * @return event's priority.
     */
    public Priority getPriority();

    /**
     * Gets the size of the data buffer in bytes.
     * @return size of the data buffer in bytes
     */
    public int getMemSize();

    /**
     * Gets the length of the data in bytes.
     * @return length of the data in bytes.
     */
    public int getLength();

    /**
     * Gets the status of the data (set by the system), which can be OK {@link DataStatus#OK},
     * corrupted {@link DataStatus#CORRUPT}, or possibly corrupted
     * {@link DataStatus#POSSIBLYCORRUPT}. Data is OK by default, it is never labeled
     * as corrupt but can be labeled as possible corrupt if the process owning an
     * event crashes and the system recovers it.
     *
     * @return status of the data.
     */
    public DataStatus getDataStatus();

    /**
     * Gets the event's modify value when receiving it over the network.
     * This specifies whether the user wants to read the event only, will modify only
     * the event header (everything except the data), or will modify the data and/or header.
     * Modifying the data and/or header is {@link Modify#ANYTHING}, modifying only the header
     * is {@link Modify#HEADER}, else the default assumed, {@link Modify#NOTHING},
     * is that nothing is modified resulting in this event being put back into
     * the ET system (by remote server) immediately upon being copied and that copy
     * sent to the user.
     *
     * @return event's modify value.
     */
    public Modify getModify();

    /**
     * Gets the event's control array.
     * This is an array of integers which can be used for any purpose by the user.
     * 
     * @return event's control array.
     */
    public int[] getControl();

    /**
     * Gets the data array which is backing the event's data buffer if there is one.
     * Any changes to the array will be reflected in the buffer.
     * If there is no backing data array, as is the case when a user connects
     * to a local, C-based ET system (memory mapped buffer) then an exception
     * is thrown.
     *
     * @return data array which is backing the event's data buffer
     * @throws UnsupportedOperationException if there is no backing array
     */
    public byte[] getData() throws UnsupportedOperationException;

    /**
     * Gets the event's data buffer.
     * @return event's data buffer.
     */
    public ByteBuffer getDataBuffer();

    /**
     * Gets the attachment id of the attachment which owns or got the event.
     * If it's owned by the system its value is {@link EtConstants#system}.
     *
     * @return id of owning attachment or {@link EtConstants#system} if system owns it
     */
    public int getOwner();

    /**
     * Gets the event data's byte order.
     * @return event data's byte order
     */
    public ByteOrder getByteOrder();

    /**
     * Gets the raw byte order data (0x04030201 or 0x01020304).
     * @return raw byte order data (0x04030201 or 0x01020304).
     */
    public int getRawByteOrder();

    /**
     * Gets whether this event is a temporary event or not.
     * For java-based ET systems this is always false. If this event represents
     * an event in a local, C-based ET system (through JNI), this can be true if its
     * memory is larger than that of the normal events necessitating the temporary
     * allocation of another chunk of shared memory just for this event.
     *
     * @return {@code true} if this events represents a local, C-based ET system temp event,
     *         else {@code false}.
     */
    public boolean isTemp();


    // setters


    /**
     * Sets the event's priority, either high {@link Priority#HIGH} or low {@link Priority#LOW}.
     * Low priority is normal while high priority events get placed at the front of stations'
     * input and output event lists.
     *
     * @param pri event priority
     */
    public void setPriority(Priority pri);

    /**
     * Sets the event's data length in bytes.
     *
     * @param len data length
     * @throws EtException if length is less than zero
     */
    public void setLength(int len) throws EtException;

    /**
     * Sets the event's control array by copying it into the event.
     *
     * @param con control array
     * @throws EtException if control array has the wrong number of elements
     */
    public void setControl(int[] con) throws EtException;

    /**
     * Set the event data's byte order.
     * @param order data's byte order
     */
    void setByteOrder(ByteOrder order);

    /**
     * Set the event data's byte order by using values consistent with C-based ET systems,
     * {@link EtConstants#endianBig}, {@link EtConstants#endianLittle}, {@link EtConstants#endianLocal},
     * {@link EtConstants#endianNotLocal}, or {@link EtConstants#endianSwitch}.
     *
     * @param endian endian value
     * @throws EtException if argument is a bad value
     */
    void setByteOrder(int endian) throws EtException;

    /**
     * Set the event data's byte order as big with 0x04030201 or
     * as little with 0x01020304.
     *
     * @param byteOrder data's byte order as big with 0x04030201 or
     *                  as little with 0x01020304.
     */
    void setRawByteOrder(int byteOrder) throws EtException;


    // miscellaneous

    
    /**
     * Tells caller if the event data needs to be swapped in order to be the
     * same byte order as the local JVM.
     *
     * @return <code>true</code> if swapping is needed, otherwise <code>false</code>
     */
    public boolean needToSwap();
}
