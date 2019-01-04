/*
 * Copyright (c) 2013, Jefferson Science Associates
 *
 * Thomas Jefferson National Accelerator Facility
 * Data Acquisition Group
 *
 * 12000, Jefferson Ave, Newport News, VA 23606
 * Phone : (757)-269-7100
 *
 */

package org.jlab.coda.jevio;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;

/**
 * This class is used to store relevant info about an evio container
 * (bank, segment, or tag segment), without having
 * to de-serialize it into many objects and arrays.
 *
 * @author timmer
 * Date: 11/13/12
 */
public final class EvioNode implements Cloneable {

    /** Header's length value (32-bit words). */
    int len;
    /** Header's tag value. */
    int tag;
    /** Header's num value. */
    int num;
    /** Header's padding value. */
    int pad;
    /** Position of header in file/buffer in bytes.  */
    int pos;
    /** This node's (evio container's) type. Must be bank, segment, or tag segment. */
    int type;

    /** Length of node's data in 32-bit words. */
    int dataLen;
    /** Position of node's data in file/buffer in bytes. */
    int dataPos;
    /** Type of data stored in node. */
    int dataType;

    /** Does this node represent an event (top-level bank)? */
    boolean isEvent;

    /** Block containing this node. */
    BlockNode blockNode;

    /** ByteBuffer that this node is associated with. */
    BufferNode bufferNode;

    /** List of child nodes ordered according to placement in buffer. */
    ArrayList<EvioNode> childNodes;

    //-------------------------------
    // For event-level node
    //-------------------------------

    /**
     * Place of containing event in file/buffer. First event = 0, second = 1, etc.
     * Useful for converting node to EvioEvent object (de-serializing).
     */
    int place;

    /**
     * If top-level event node, was I scanned and all my banks
     * already placed into a list?
     */
    boolean scanned;

    /** List of all nodes in the event including the top-level object
     *  ordered according to placement in buffer.
     *  Only created at the top-level (with constructor). All lower-level
     *  nodes are created with clone() so all nodes have a reference to
     *  the top-level allNodes object. */
    ArrayList<EvioNode> allNodes;

    //-------------------------------
    // For sub event-level node
    //-------------------------------

    /** Node of event containing this node. Is null if this is an event node. */
    EvioNode eventNode;

    /** Node containing this node. Is null if this is an event node. */
    EvioNode parentNode;

    //----------------------------------
    // Constructors (package accessible)
    //----------------------------------

    /** Constructor when fancy features not needed. */
    EvioNode() {
        // Put this node in list of all nodes (evio banks, segs, or tagsegs)
        // contained in this event.
        allNodes = new ArrayList<EvioNode>(5);
        allNodes.add(this);
    }


    /**
     * Constructor which creates an EvioNode associated with
     * an event (top level) evio container when parsing buffers
     * for evio data.
     *
     * @param pos        position of event in buffer (number of bytes)
     * @param place      containing event's place in buffer (starting at 1)
     * @param bufferNode buffer containing this event
     * @param blockNode  block containing this event
     */
    EvioNode(int pos, int place, BufferNode bufferNode, BlockNode blockNode) {
        this.pos = pos;
        this.place = place;
        this.blockNode = blockNode;
        this.bufferNode = bufferNode;
        // This is an event by definition
        this.isEvent = true;
        // Event is a Bank by definition
        this.type = DataType.BANK.getValue();

        // Put this node in list of all nodes (evio banks, segs, or tagsegs)
        // contained in this event.
        allNodes = new ArrayList<EvioNode>(5);
        allNodes.add(this);
    }


    /**
     * Constructor which creates an EvioNode in the CompactEventBuilder.
     *
     * @param tag        the tag for the event (or bank) header.
   	 * @param num        the num for the event (or bank) header.
     * @param pos        position of event in buffer (bytes).
     * @param dataPos    position of event's data in buffer (bytes.)
     * @param type       the type of this evio structure.
     * @param dataType   the data type contained in this evio event.
     * @param bufferNode buffer containing this event.
     */
    EvioNode(int tag, int num, int pos, int dataPos,
             DataType type, DataType dataType,
             BufferNode bufferNode) {

        this.tag = tag;
        this.num = num;
        this.pos = pos;
        this.dataPos = dataPos;

        this.type = type.getValue();
        this.dataType = dataType.getValue();
        this.bufferNode = bufferNode;
    }


    //-------------------------------
    // Methods
    //-------------------------------

    public Object clone() {
        try {
            EvioNode result = (EvioNode)super.clone();
            // Doing things this way means we don't have to
            // copy references to the node & buffer object.
            // However, we do need a new list ...
            result.childNodes = null;
            return result;
        }
        catch (CloneNotSupportedException ex) {
            return null;    // never invoked
        }
    }

    public String toString() {
        StringBuilder builder = new StringBuilder(100);
        builder.append("tag = ");   builder.append(tag);
        builder.append(", num = "); builder.append(num);
        builder.append(", type = "); builder.append(type);
        builder.append(", dataType = "); builder.append(dataType);
        builder.append(", pos = "); builder.append(pos);
        builder.append(", dataPos = "); builder.append(dataPos);
        builder.append(", len = "); builder.append(len);
        builder.append(", dataLen = "); builder.append(dataLen);

        return builder.toString();
    }


    public void clearLists() {
        if (childNodes != null) childNodes.clear();

        // Should only be defined if this is an event (isEvent == true)
        if (allNodes != null) {
            allNodes.clear();
            // Remember to add event's node into list
            if (eventNode == null) {
                allNodes.add(this);
            }
            else {
                allNodes.add(eventNode);
            }
        }
    }

    //-------------------------------
    // Setters
    //-------------------------------

    /**
     * Add a child node to the end of the child list and
     * to the list of all events.
     * @param childNode child node to add to the end of the child list.
     */
    synchronized public void addChild(EvioNode childNode) {
        if (childNodes == null) {
            childNodes = new ArrayList<EvioNode>(100);
        }
        childNodes.add(childNode);

        if (allNodes != null) allNodes.add(childNode);
    }



//    /**
//     * Remove a node from this child list.
//     * @param childNode node to remove from child list.
//     */
//    synchronized public void removeChild(EvioNode childNode) {
//        if (childNodes != null) {
//            childNodes.remove(childNode);
//        }
//
//        if (allNodes != null) allNodes.remove(childNode);
//    }

    //-------------------------------
    // Getters
    //-------------------------------

//    /**
//     * Get the object representing the block header.
//     * @return object representing the block header.
//     */
//    public BlockNode getBlockNode() {
//        return blockNode;
//    }

    /**
     * Get the list of all nodes that this node contains,
     * always including itself. This is meaningful only if this
     * node has been scanned, otherwise it contains only itself.
     *
     * @return list of all nodes that this node contains; null if not top-level node
     */
    public ArrayList<EvioNode> getAllNodes() {return allNodes;}

    /**
     * Get the list of all child nodes that this node contains.
     * This is meaningful only if this node has been scanned,
     * otherwise it is null.
     *
     * @return list of all child nodes that this node contains;
     *         null if not scanned or no children
     */
    public ArrayList<EvioNode> getChildNodes() {
        return childNodes;
    }

    /**
     * Get the child node at the given index (starts at 0).
     * This is meaningful only if this node has been scanned,
     * otherwise it is null.
     *
     * @return child node at the given index;
     *         null if not scanned or no child at that index
     */
    public EvioNode getChildAt(int index) {
        if ((childNodes == null) || (childNodes.size() < index+1)) return null;
        return childNodes.get(index);
    }

    /**
     * Get the number all children that this node contains.
     * This is meaningful only if this node has been scanned,
     * otherwise it returns 0.
     *
     * @return number of children that this node contains;
     *         0 if not scanned
     */
    public int getChildCount() {
        if (childNodes == null) return 0;
        return childNodes.size();
    }

    /**
     * Get the object containing the buffer that this node is associated with.
     * @return object containing the buffer that this node is associated with.
     */
    public BufferNode getBufferNode() {
        return bufferNode;
    }

    /**
     * Get the length of this evio structure (not including length word itself)
     * in 32-bit words.
     * @return length of this evio structure (not including length word itself)
     *         in 32-bit words
     */
    public int getLength() {
        return len;
    }

    /**
     * Get the length of this evio structure including entire header in bytes.
     * @return length of this evio structure including entire header in bytes.
     */
    public int getTotalBytes() {
        return 4*dataLen + dataPos - pos;
    }

    /**
     * Get the tag of this evio structure.
     * @return tag of this evio structure
     */
    public int getTag() {
        return tag;
    }

    /**
     * Get the num of this evio structure.
     * Will be zero for tagsegments.
     * @return num of this evio structure
     */
    public int getNum() {
        return num;
    }

    /**
     * Get the padding of this evio structure.
     * Will be zero for segments and tagsegments.
     * @return padding of this evio structure
     */
    public int getPad() {
        return pad;
    }

    /**
     * Get the file/buffer byte position of this evio structure.
     * @return file/buffer byte position of this evio structure
     */
    public int getPosition() {
        return pos;
    }

    /**
     * Get the evio type of this evio structure, not what it contains.
     * Call {@link DataType#getDataType(int)} on the
     * returned value to get the object representation.
     * @return evio type of this evio structure, not what it contains
     */
    public int getType() {return type;}

    /**
     * Get the evio type of this evio structure as an object.
     * @return evio type of this evio structure as an object.
     */
    public DataType getTypeObj() {
        return DataType.getDataType(type);
    }

    /**
     * Get the length of this evio structure's data only (no header words)
     * in 32-bit words.
     * @return length of this evio structure's data only (no header words)
     *         in 32-bit words.
     */
    public int getDataLength() {
        return dataLen;
    }

    /**
     * Get the file/buffer byte position of this evio structure's data.
     * @return file/buffer byte position of this evio structure's data
     */
    public int getDataPosition() {
        return dataPos;
    }

    /**
     * Get the evio type of the data this evio structure contains.
     * Call {@link DataType#getDataType(int)} on the
     * returned value to get the object representation.
     * @return evio type of the data this evio structure contains
     */
    public int getDataType() {
        return dataType;
    }

    /**
     * Get the evio type of the data this evio structure contains as an object.
     * @return evio type of the data this evio structure contains as an object.
     */
    public DataType getDataTypeObj() {
        return DataType.getDataType(dataType);
    }


    /**
     * If this object represents an event (top-level, evio bank),
     * then returns its number (place in file or buffer) starting
     * with 1. If not, return -1.
     * @return event number if representing an event, else -1
     */
    public int getEventNumber() {
        // TODO: This does not seems right as place defaults to a value of 0!!!
        return (place + 1);
    }


    /**
     * Does this object represent an event?
     * @return <code>true</code> if this object represents and event,
     *         else <code>false</code>
     */
    public boolean isEvent() {
        return isEvent;
    }


    /**
     * Update, in the buffer, the tag of the structure header this object represents.
     * Sometimes it's necessary to go back and change the tag of an evio
     * structure that's already been written. This will do that.
     *
     * @param newTag new tag value
     */
    public void updateTag(int newTag) {

        ByteBuffer buffer = bufferNode.buffer;

        switch (DataType.getDataType(type)) {
            case BANK:
            case ALSOBANK:
                if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                    buffer.putShort(pos+4, (short) newTag);
                }
                else {
                    buffer.putShort(pos+6, (short) newTag);
                }
                return;

            case SEGMENT:
            case ALSOSEGMENT:
                if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                    buffer.put(pos, (byte)newTag);
                }
                else {
                    buffer.put(pos+3, (byte)newTag);
                }
                return;

            case TAGSEGMENT:
                short compositeWord = (short) ((tag << 4) | (dataType & 0xf));
                if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                    buffer.putShort(pos, compositeWord);
                }
                else {
                    buffer.putShort(pos+2, compositeWord);
                }
                return;

            default:
        }
    }


    /**
     * Update, in the buffer, the num of the bank header this object represents.
     * Sometimes it's necessary to go back and change the num of an evio
     * structure that's already been written. This will do that
     * .
     * @param newNum new num value
     */
    public void updateNum(int newNum) {

        ByteBuffer buffer = bufferNode.buffer;

        switch (DataType.getDataType(type)) {
            case BANK:
            case ALSOBANK:
                if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                    buffer.put(pos+7, (byte) newNum);
                }
                else {
                    buffer.put(pos+4, (byte)newNum);
                }
                return;

            default:
        }
    }


    /**
     * Get the data associated with this node in ByteBuffer form.
     * Depending on the copy argument, the returned buffer will either be
     * a copy of or a view into this node's buffer.
     * Position and limit are set for reading.<p>
     * This method is not synchronized.
     *
     * @param copy if <code>true</code>, then return a copy as opposed to a
     *             view into this node's buffer.
     * @return ByteBuffer containing data.
     *         Position and limit are set for reading.
     */
    public ByteBuffer getByteData(boolean copy) {

        // The tricky thing to keep in mind is that the buffer
        // which this node uses may also be used by other nodes.
        // That means setting its limit and position may interfere
        // with other operations being done to it.
        // So even though it is less efficient, use a duplicate of the
        // buffer which gives us our own limit and position.
        ByteOrder order   = bufferNode.buffer.order();
        ByteBuffer buffer = bufferNode.buffer.duplicate().order(order);
        buffer.limit(dataPos + 4*dataLen - pad).position(dataPos);

        if (copy) {
            ByteBuffer newBuf = ByteBuffer.allocate(4*dataLen - pad).order(order);
            newBuf.put(buffer);
            newBuf.flip();
            return newBuf;
        }

        return  buffer.slice().order(order);
    }


    /**
     * Get this node's entire evio structure in ByteBuffer form.
     * Depending on the copy argument, the returned buffer will either be
     * a copy of or a view into the data of this node's buffer.
     * Position and limit are set for reading.<p>
     * This method is not synchronized.
     *
     * @param copy if <code>true</code>, then return a copy as opposed to a
     *        view into this node's buffer.
     * @return ByteBuffer object containing evio structure's bytes.
     *         Position and limit are set for reading.
     */
    public ByteBuffer getStructureBuffer(boolean copy)  {

        // The tricky thing to keep in mind is that the buffer
        // which this node uses may also be used by other nodes.
        // That means setting its limit and position may interfere
        // with other operations being done to it.
        // So even though it is less efficient, use a duplicate of the
        // buffer which gives us our own limit and position.

        ByteOrder order   = bufferNode.buffer.order();
        ByteBuffer buffer = bufferNode.buffer.duplicate().order(order);
        buffer.limit(dataPos + 4*dataLen).position(pos);

        if (copy) {
            ByteBuffer newBuf = ByteBuffer.allocate(getTotalBytes()).order(order);
            newBuf.put(buffer);
            newBuf.flip();
            return newBuf;
        }

        return buffer.slice().order(order);
    }


}
