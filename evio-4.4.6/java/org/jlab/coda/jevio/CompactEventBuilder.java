/*
 * Copyright (c) 2014, Jefferson Science Associates
 *
 * Thomas Jefferson National Accelerator Facility
 * Data Acquisition Group
 *
 * 12000, Jefferson Ave, Newport News, VA 23606
 * Phone : (757)-269-7100
 *
 */

package org.jlab.coda.jevio;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.*;
import java.nio.ByteBuffer;
import java.util.ArrayList;

/**
 * This class is used for creating events
 * and their substructures while minimizing use of Java objects.
 * Evio format data of a single event is written directly,
 * and sequentially, into a buffer. The buffer contains
 * only the single event, not the full, evio file format.<p>
 *
 * The methods of this class are not synchronized so it is NOT
 * threadsafe. This is done for speed. The buffer retrieved by
 * {@link #getBuffer()} is ready to read.
 *
 * @author timmer
 * (Feb 6 2014)
 */
public class CompactEventBuilder {

    /** Buffer in which to write. */
    private ByteBuffer buffer;

    /** Byte array which backs the buffer. */
    private byte[] array;

    /** Current writing position in the buffer. */
    private int position;

    /** Byte order of the buffer, convenience variable. */
    private ByteOrder order;

    /** Two alternate ways of writing data, one which is direct to the
     *  backing array and the other using ByteBuffer object methods. */
    private boolean useByteBuffer = false;

    /** Did this object create the byte buffer? */
    private boolean createdBuffer = false;

    /** When writing to buffer, generate EvioNode objects as evio
     *  structures are being created. */
    private boolean generateNodes = false;

    /** If {@link #generateNodes} is {@code true}, then store
     *  generated node objects in this list (in buffer order. */
    private ArrayList<EvioNode> nodes;

    /** Maximum allowed evio structure levels in buffer. */
    private static final int MAX_LEVELS = 100;

    /** Number of bytes to pad short and byte data. */
    private static final int[] padCount = {0,3,2,1};


    /** This class stores information about an evio structure
     * (bank, segment, or tagsegment) which allows us to write
     * a length or padding in that structure in the buffer.
     */
    private class StructureContent {
        /** Starting position of this structure in the buffer. */
        int pos;
        /** Keep track of amount of primitive data written for finding padding.
         *  Can be either bytes or shorts. */
        int dataLen;
        /** Padding for byte and short data. */
        int padding;
        /** Type of evio structure this is. */
        DataType type;
        /** Type of evio structures or data contained. */
        DataType dataType;


        StructureContent(int pos, DataType type, DataType dataType) {
            this.pos = pos;
            this.type = type;
            this.dataType = dataType;
        }
    }

//    /** The top of the stack is parent of the current structure
//     *  and the next one down is the parent of the parent, etc. */
//    private LinkedBlockingDeque<StructureContent> stack;

    /** The top (first element) of the stack is parent of the current structure
     *  and the next one down is the parent of the parent, etc.
     *  Actually, the first (0th) element is unused, and things start with the
     *  second element. */
    private StructureContent[] stackArray;

    /** Each element of the array is the total length of all evio
     *  data in a structure including the full length of that
     *  structure's header. There is one length for each level
     *  of evio structures with the 0th element being the top
     *  structure (or event) level. */
    private int[] totalLengths;

    /** Current evio structure being created. */
    private StructureContent currentStructure;

    /** Level of the evio structure currently being created.
     *  Starts at 0 for the event bank. */
    private int currentLevel;


    /**
     * This is the constructor to use for building a new event
     * (just the event in a buffer, not the full evio file format).
     * A buffer is created in this constructor.
     *
     * @param bufferSize size of byte buffer (in bytes) to create.
     * @param order      byte order of created buffer.
     *
     * @throws EvioException if arg is null;
     *                       if bufferSize arg too small
     */
    public CompactEventBuilder(int bufferSize, ByteOrder order)
                    throws EvioException {
        this(bufferSize, order, false);
   	}


    /**
     * This is the constructor to use for building a new event
     * (just the event in a buffer, not the full evio file format).
     * A buffer is created in this constructor.
     *
     * @param bufferSize size of byte buffer (in bytes) to create.
     * @param order      byte order of created buffer.
     * @param generateNodes generate and store an EvioNode object
     *                      for each evio structure created.
     *
     * @throws EvioException if arg is null;
     *                       if bufferSize arg too small
     */
    public CompactEventBuilder(int bufferSize, ByteOrder order, boolean generateNodes)
                    throws EvioException {

        if (order == null) {
            throw new EvioException("null arg(s)");
        }

        if (bufferSize < 8) {
            throw new EvioException("bufferSize arg too small");
        }

        // Create buffer
        array = new byte[bufferSize];
        buffer = ByteBuffer.wrap(array);
        buffer.order(order);
        this.order = order;
        this.generateNodes = generateNodes;
        position = 0;

        // Init variables
        nodes = null;
        currentLevel = -1;
        createdBuffer = true;
        currentStructure = null;
        totalLengths = new int[MAX_LEVELS];
        stackArray = new StructureContent[MAX_LEVELS];
   	}


    /**
     * This is the constructor to use for building a new event
     * (just the event in a buffer, not the full evio file format)
     * with a user-given buffer.
     *
     * @param buffer the byte buffer to write into.
     *
     * @throws EvioException if arg is null;
     *                       if buffer is too small
     */
    public CompactEventBuilder(ByteBuffer buffer)
                    throws EvioException {
        this(buffer,false);
   	}


    /**
     * This is the constructor to use for building a new event
     * (just the event in a buffer, not the full evio file format)
     * with a user-given buffer.
     *
     * @param buffer the byte buffer to write into.
     * @param generateNodes generate and store an EvioNode object
     *                      for each evio structure created.
     *
     * @throws EvioException if arg is null;
     *                       if buffer is too small
     */
    public CompactEventBuilder(ByteBuffer buffer, boolean generateNodes)
                    throws EvioException {

        setBuffer(buffer, generateNodes);
   	}


    /**
     * Set the buffer to be written into.
     * @param buffer buffer to be written into.
     * @throws EvioException if arg is null;
     *                       if buffer is too small
     */
    public void setBuffer(ByteBuffer buffer) throws EvioException {
        setBuffer(buffer, false);
    }


    /**
     * Set the buffer to be written into.
     * @param buffer buffer to be written into.
     * @param generateNodes generate and store an EvioNode object
     *                      for each evio structure created.
     * @throws EvioException if arg is null;
     *                       if buffer is too small
     */
    public void setBuffer(ByteBuffer buffer, boolean generateNodes)
                          throws EvioException {

        if (buffer == null) {
            throw new EvioException("null arg(s)");
        }

        this.generateNodes = generateNodes;

        // Prepare buffer
        this.buffer = buffer;
        buffer.clear();
        order = buffer.order();
        position = 0;

        if (buffer.limit() < 8) {
            throw new EvioException("compact buffer too small");
        }

        // Protect against using the backing array of slices
        if (buffer.hasArray() && buffer.array().length == buffer.capacity()) {
            array = buffer.array();
        }
        else {
            useByteBuffer = true;
        }

        // Init variables
        nodes = null;
        currentLevel = -1;
        createdBuffer = false;
        currentStructure = null;
        totalLengths = new int[MAX_LEVELS];
        stackArray = new StructureContent[MAX_LEVELS];
    }


    /**
     * Get the buffer being written into.
     * The buffer is ready to read.
     * @return buffer being written into.
     */
    public ByteBuffer getBuffer() {
        buffer.limit(position);
        buffer.position(0);
        return buffer;
    }


    /**
     * Get the total number of bytes written into the buffer.
     * @return total number of bytes written into the buffer.
     */
    public int getTotalBytes() {return position;}


    /**
     * Double the buffer size if more room is needed and this object
     * created the buffer in the first place.
     *
     * @return {@code true} if buffer was expanded, else {@code false}.
     */
    private boolean expandBuffer() {
        if (!createdBuffer) return false;

        // Double buffer size
        int newSize = 2 * buffer.capacity();

        // Create new buffer
        byte[] newArray = new byte[newSize];
        ByteBuffer newBuf = ByteBuffer.wrap(newArray);
        newBuf.order(order);

        // Copy data into new buffer
        System.arraycopy(array, 0, newArray, 0, position);
        array  = newArray;
        buffer = newBuf;

        // Update generated EvioNodes to point to new buffer
        if (nodes != null) {
            // TODO: this loses block info and whether or not scanned
            BufferNode newBufNode = new BufferNode(buffer);
            for (EvioNode node : nodes) {
                node.bufferNode = newBufNode;
            }
        }

        return true;
    }


    /**
     * This method adds an evio segment structure to the buffer.
     *
     * @param tag tag of segment header
     * @param dataType type of data to be contained by this segment
     * @return if told to generate nodes in constructor, then return
     *         the node created here; else return null
     * @throws EvioException if containing structure does not hold segments;
     *                       if no room in buffer for segment header;
     *                       if too many nested evio structures;
     *                       if top-level bank has not been added first.
     */
    public EvioNode openSegment(int tag, DataType dataType)
        throws EvioException {

        if (currentStructure == null) {
            throw new EvioException("add a bank (event) first");
        }

        // Segments not allowed if parent holds different type
        if (currentStructure.dataType != DataType.SEGMENT &&
            currentStructure.dataType != DataType.ALSOSEGMENT) {
            throw new EvioException("may NOT add segment type, expecting " +
                                            currentStructure.dataType);
        }

        // Make sure we can use all of the buffer in case external changes
        // were made to it (e.g. by doing buffer.flip() in order to read).
        // All this does is set pos = 0, limit = capacity, it does NOT
        // clear the data. We're keep track of the position to write at
        // in our own variable, "position".
        buffer.clear();

        if (buffer.limit() - position < 4) {
            throw new EvioException("no room in buffer");
        }

        // For now, assume length and padding = 0
        if (useByteBuffer) {
            if (order == ByteOrder.BIG_ENDIAN) {
                buffer.put(position,   (byte)tag);
                buffer.put(position+1, (byte)(dataType.getValue() & 0x3f));
            }
            else {
                buffer.put(position+2, (byte)(dataType.getValue() & 0x3f));
                buffer.put(position+3, (byte)tag);
            }
        }
        else {
            if (order == ByteOrder.BIG_ENDIAN) {
                array[position]   = (byte)tag;
                array[position+1] = (byte)(dataType.getValue() & 0x3f);
            }
            else {
                array[position+2] = (byte)(dataType.getValue() & 0x3f);
                array[position+3] = (byte)tag;
            }
        }

        // Since we're adding a structure, we're going down a level (like push)
        currentLevel++;
        if (currentLevel >= MAX_LEVELS) {
            throw new EvioException("too many nested evio structures, increase MAX_LEVELS");
        }

        // Put current structure onto stack as it's now a parent
//            stack.addFirst(currentStructure);
        stackArray[currentLevel] = currentStructure;

        // We just wrote a segment header so this and all
        // containing levels increase in length by 1.
        addToAllLengths(1);

        // Current structure is the segment we are creating right now
        currentStructure = new StructureContent(position, DataType.SEGMENT, dataType);

        // Do we generate an EvioNode object corresponding to this segment?
        EvioNode node = null;
        if (generateNodes) {
            if (nodes == null) {
                nodes = new ArrayList<EvioNode>(100);
            }

            // This constructor does not have lengths or create allNodes list, blockNode
            node = new EvioNode(tag, 0, position, position+4,
                                         DataType.SEGMENT, dataType,
                                         new BufferNode(buffer));
            nodes.add(node);
        }

        position += 4;

        return node;
    }


    /**
     * This method adds an evio tagsegment structure to the buffer.
     *
     * @param tag tag of tagsegment header
     * @param dataType type of data to be contained by this tagsegment
     * @return if told to generate nodes in constructor, then return
     *         the node created here; else return null
     * @throws EvioException if containing structure does not hold tagsegments;
     *                       if no room in buffer for tagsegment header;
     *                       if too many nested evio structures;
     *                       if top-level bank has not been added first.
     */
    public EvioNode openTagSegment(int tag, DataType dataType)
            throws EvioException {

        if (currentStructure == null) {
            throw new EvioException("add a bank (event) first");
        }

        // Tagsegments not allowed if parent holds different type
        if (currentStructure.dataType != DataType.TAGSEGMENT) {
            throw new EvioException("may NOT add tagsegment type, expecting " +
                                            currentStructure.dataType);
        }

        // Make sure we can use all of the buffer in case external changes
        // were made to it (e.g. by doing buffer.flip() in order to read).
        // All this does is set pos = 0, limit = capacity, it does NOT
        // clear the data. We're keep track of the position to write at
        // in our own variable, "position".
        buffer.clear();

        if (buffer.limit() - position < 4) {
            throw new EvioException("no room in buffer");
        }

        // Because Java automatically sets all members of a new
        // byte array to zero, we don't have to specifically write
        // a length of zero into the new tagsegment header (which is
        // what the length is if no data written).

        // For now, assume length = 0
        short compositeWord = (short) ((tag << 4) | (dataType.getValue() & 0xf));

        if (useByteBuffer) {
            if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                buffer.putShort(position, compositeWord);
            }
            else {
                buffer.putShort(position+2, compositeWord);
            }
        }
        else {
            if (order == ByteOrder.BIG_ENDIAN) {
                array[position]   = (byte) (compositeWord >> 8);
                array[position+1] = (byte)  compositeWord;
            }
            else {
                array[position+2] = (byte)  compositeWord;
                array[position+3] = (byte) (compositeWord >> 8);
            }
        }

        // Since we're adding a structure, we're going down a level (like push)
        currentLevel++;
        if (currentLevel >= MAX_LEVELS) {
            throw new EvioException("too many nested evio structures, increase MAX_LEVELS");
        }

        // Put current structure onto stack as it's now a parent
//            stack.addFirst(currentStructure);
        stackArray[currentLevel] = currentStructure;

        // We just wrote a tagsegment header so this and all
        // containing levels increase in length by 1.
        addToAllLengths(1);

        // Current structure is the tagsegment we are creating right now
        currentStructure = new StructureContent(position, DataType.TAGSEGMENT, dataType);

        // Do we generate an EvioNode object corresponding to this segment?
        EvioNode node = null;
        if (generateNodes) {
            if (nodes == null) {
                nodes = new ArrayList<EvioNode>(100);
            }

            // This constructor does not have lengths or create allNodes list, blockNode
            node = new EvioNode(tag, 0, position, position+4,
                                         DataType.TAGSEGMENT, dataType,
                                         new BufferNode(buffer));
            nodes.add(node);
        }

        position += 4;

        return node;
    }


    /**
     * This method adds an evio bank to the buffer.
     *
     * @param tag tag of bank header
     * @param num num of bank header
     * @param dataType data type to be contained in this bank
     * @return if told to generate nodes in constructor, then return
     *         the node created here; else return null
     * @throws EvioException if containing structure does not hold banks;
     *                       if no room in buffer for bank header;
     *                       if too many nested evio structures;
     */
    public EvioNode openBank(int tag, int num, DataType dataType)
            throws EvioException {

        // Banks not allowed if parent holds different type
        if (currentStructure != null &&
                (currentStructure.dataType != DataType.BANK &&
                 currentStructure.dataType != DataType.ALSOBANK)) {
            throw new EvioException("may NOT add bank type, expecting " +
                                            currentStructure.dataType);
        }

        // Make sure we can use all of the buffer in case external changes
        // were made to it (e.g. by doing buffer.flip() in order to read).
        // All this does is set pos = 0, limit = capacity, it does NOT
        // clear the data. We're keep track of the position to write at
        // in our own variable, "position".
        buffer.clear();

        if (buffer.limit() - position < 8) {
            throw new EvioException("no room in buffer");
        }

        // Write bank header into buffer, assuming padding = 0
        if (useByteBuffer) {
            // Bank w/ no data has len = 1
            buffer.putInt(position, 1);

            if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                buffer.putShort(position + 4, (short)tag);
                buffer.put(position + 6, (byte)(dataType.getValue() & 0x3f));
                buffer.put(position + 7, (byte)num);
            }
            else {
                buffer.put(position + 4, (byte)num);
                buffer.put(position + 5, (byte)(dataType.getValue() & 0x3f));
                buffer.putShort(position + 6, (short)tag);
            }
        }
        else {
            // Bank w/ no data has len = 1
            ByteDataTransformer.toBytes(1, order, array, position);

            if (order == ByteOrder.BIG_ENDIAN) {
                array[position+4] = (byte)(tag >> 8);
                array[position+5] = (byte)tag;
                array[position+6] = (byte)(dataType.getValue() & 0x3f);
                array[position+7] = (byte)num;
            }
            else {
                array[position+4] = (byte)num;
                array[position+5] = (byte)(dataType.getValue() & 0x3f);
                array[position+6] = (byte)tag;
                array[position+7] = (byte)(tag >> 8);
            }
        }

        // Since we're adding a structure, we're going down a level (like push)
        currentLevel++;
        if (currentLevel >= MAX_LEVELS) {
            throw new EvioException("too many nested evio structures, increase MAX_LEVELS");
        }

        // Put current structure, if any, onto stack as it's now a parent
//        if (currentStructure != null) {
//            stack.addFirst(currentStructure);
            stackArray[currentLevel] = currentStructure;
//        }

        // We just wrote a bank header so this and all
        // containing levels increase in length by 2.
        addToAllLengths(2);

        // Current structure is the bank we are creating right now
        currentStructure = new StructureContent(position, DataType.BANK, dataType);

        // Do we generate an EvioNode object corresponding to this segment?
        EvioNode node = null;
        if (generateNodes) {
            if (nodes == null) {
                nodes = new ArrayList<EvioNode>(100);
            }

            // This constructor does not have lengths or create allNodes list, blockNode
            node = new EvioNode(tag, 0, position, position+8,
                                         DataType.BANK, dataType,
                                         new BufferNode(buffer));
            nodes.add(node);
        }

        position += 8;

        return node;
    }


    /**
   	 * This method ends the writing of the current evio structure and
     * makes sure the length and padding fields are properly set.
     * Its parent, if any, then becomes the current structure.
     *
     * @return {@code true} if top structure was reached (is current), else {@code false}.
   	 */
   	public boolean closeStructure() {
        // Cannot go up any further
        if (currentStructure == null) {
            return true;
        }

        // Set the length of current structure since we're done adding to it.
        // The length contained in an evio header does NOT include itself,
        // thus the -1.
        setCurrentHeaderLength(totalLengths[currentLevel] - 1);

        // Set padding if necessary
        if (currentStructure.padding > 0) {
            setCurrentHeaderPadding(currentStructure.padding);
        }

        // Clear out old data
        totalLengths[currentLevel] = 0;

        // The new current structure is ..
//        currentStructure = stack.poll();
//        if (currentStructure != stackArray[currentLevel]) {
//            System.out.println("OOOOPPPPPPPPS");
//            System.exit(-1);
//        }
        currentStructure = stackArray[currentLevel];

        // Go up a level
        currentLevel--;

        // If structure is null, we've reached the top and we're done
        return (currentStructure == null);
    }


    /**
     * This method finishes the event writing by setting all the
     * proper lengths & padding and ends up at the event or top level.
     */
    public void closeAll() {
        while (!closeStructure()) {}
    }


    /**
     * This method adds the given length to the current structure's existing
     * length and all its ancestors' lengths. Nothing is written into the
     * buffer. The lengths are keep in an array for later use in writing to
     * the buffer.
     *
     * @param len length to add
     */
    private void addToAllLengths(int len) {
        // Go up the chain of structures from current structure, inclusive
        for (int i=currentLevel; i >= 0; i--) {
            totalLengths[i] += len;
//System.out.println("Set len at level " + i + " to " + totalLengths[i]);
        }
//        System.out.println("");
    }


    /**
     * This method sets the length of the current evio structure in the buffer.
     * @param len length of current structure
     */
    private void setCurrentHeaderLength(int len) {

        switch (currentStructure.type) {
            case BANK:
            case ALSOBANK:
                if (useByteBuffer) {
                    buffer.putInt(currentStructure.pos, len);
                }
                else {
                    try {
                        ByteDataTransformer.toBytes(len, order, array, currentStructure.pos);
                    }
                    catch (EvioException e) {/* never happen*/}
                }

                return;

            case SEGMENT:
            case ALSOSEGMENT:
            case TAGSEGMENT:
                if (useByteBuffer) {
                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        buffer.putShort(currentStructure.pos+2, (short)len);
                    }
                    else {
                        buffer.putShort(currentStructure.pos, (short)len);
                    }
                }
                else {
                    try {
                        if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                            ByteDataTransformer.toBytes((short)len, order,
                                                        array, currentStructure.pos+2);
                        }
                        else {
                            ByteDataTransformer.toBytes((short)len, order,
                                                        array, currentStructure.pos);
                        }
                    }
                    catch (EvioException e) {/* never happen*/}
                }
                return;

            default:
        }
    }



    /**
     * This method sets the padding of the current evio structure in the buffer.
     * @param padding padding of current structure's data
     */
    private void setCurrentHeaderPadding(int padding) {

        // byte containing padding
        byte b = (byte)((currentStructure.dataType.getValue() & 0x3f) | (padding << 6));

        switch (currentStructure.type) {
            case BANK:
            case ALSOBANK:
                if (useByteBuffer) {
                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        buffer.put(currentStructure.pos+6, b);
                    }
                    else {
                        buffer.put(currentStructure.pos+5, b);
                    }
                }
                else {
                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        array[currentStructure.pos+6] = b;
                    }
                    else {
                        array[currentStructure.pos+5] = b;
                    }
                }

                return;

            case SEGMENT:
            case ALSOSEGMENT:

                if (useByteBuffer) {
                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        buffer.put(currentStructure.pos+1, b);
                    }
                    else {
                        buffer.put(currentStructure.pos+2, b);
                    }
                }
                else {
                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        array[currentStructure.pos+1] = b;
                    }
                    else {
                        array[currentStructure.pos+2] = b;
                    }
                }
                return;

            default:
        }
    }


    /**
     * This method takes the node (which has been scanned and that
     * info stored in that object) and rewrites it into the buffer.
     * This is equivalent to a swap if the node is opposite endian in
     * its original buffer. If the node is not opposite endian,
     * there is no point in calling this method.
     *
     * @param node node whose header data must be written
     */
    private void writeHeader(EvioNode node) {

        switch (currentStructure.type) {

            case BANK:
            case ALSOBANK:

                if (useByteBuffer) {
                    buffer.putInt(position, node.len);
                    if (order == ByteOrder.BIG_ENDIAN) {
                        buffer.putShort(position+4, (short)node.tag);
                        buffer.put     (position+6, (byte)((node.dataType & 0x3f) | (node.pad << 6)));
                        buffer.put     (position+7, (byte)node.num);
                    }
                    else {
                        buffer.put     (position+4, (byte)node.num);
                        buffer.put     (position+5, (byte)((node.dataType & 0x3f) | (node.pad << 6)));
                        buffer.putShort(position+6, (short)node.tag);
                    }
                }
                else {
                    if (order == ByteOrder.BIG_ENDIAN) {
                        array[position]   = (byte)(node.len >> 24);
                        array[position+1] = (byte)(node.len >> 16);
                        array[position+2] = (byte)(node.len >> 8);
                        array[position+3] = (byte) node.len;

                        array[position+4] = (byte)(node.tag >>> 8);
                        array[position+5] = (byte) node.tag;
                        array[position+6] = (byte)((node.dataType & 0x3f) | (node.pad << 6));
                        array[position+7] = (byte) node.num;
                    }
                    else {
                        array[position]   = (byte)(node.len);
                        array[position+1] = (byte)(node.len >> 8);
                        array[position+2] = (byte)(node.len >> 16);
                        array[position+3] = (byte)(node.len >> 24);

                        array[position+4] = (byte) node.num;
                        array[position+5] = (byte)((node.dataType & 0x3f) | (node.pad << 6));
                        array[position+6] = (byte) node.tag;
                        array[position+7] = (byte)(node.tag >>> 8);
                    }
                }

                position += 8;
                break;

            case SEGMENT:
            case ALSOSEGMENT:

                if (useByteBuffer) {
                    if (order == ByteOrder.BIG_ENDIAN) {
                        buffer.put     (position,   (byte)node.tag);
                        buffer.put     (position+1, (byte)((node.dataType & 0x3f) | (node.pad << 6)));
                        buffer.putShort(position+2, (short)node.len);
                    }
                    else {
                        buffer.putShort(position,(short)node.len);
                        buffer.put(position + 1, (byte) ((node.dataType & 0x3f) | (node.pad << 6)));
                        buffer.put(position + 2, (byte) node.tag);
                    }
                }
                else {
                    if (order == ByteOrder.BIG_ENDIAN) {
                        array[position]   = (byte)  node.tag;
                        array[position+1] = (byte)((node.dataType & 0x3f) | (node.pad << 6));
                        array[position+2] = (byte) (node.len >> 8);
                        array[position+3] = (byte)  node.len;
                    }
                    else {
                        array[position]   = (byte)  node.len;
                        array[position+1] = (byte) (node.len >> 8);
                        array[position+2] = (byte)((node.dataType & 0x3f) | (node.pad << 6));
                        array[position+3] = (byte)  node.tag;
                    }
                }

                position += 4;
                break;

            case TAGSEGMENT:

                short compositeWord = (short) ((node.tag << 4) | (node.dataType & 0xf));

                if (useByteBuffer) {
                    if (order == ByteOrder.BIG_ENDIAN) {
                        buffer.putShort(position, compositeWord);
                        buffer.putShort(position+2, (short)node.len);
                    }
                    else {
                        buffer.putShort(position,  (short)node.len);
                        buffer.putShort(position + 2, compositeWord);
                   }
                }
                else {
                    if (order == ByteOrder.BIG_ENDIAN) {
                        array[position]   = (byte) (compositeWord >> 8);
                        array[position+1] = (byte)  compositeWord;
                        array[position+2] = (byte) (node.len >> 8);
                        array[position+3] = (byte)  node.len;
                    }
                    else {
                        array[position]   = (byte)  node.len;
                        array[position+1] = (byte) (node.len >> 8);
                        array[position+2] = (byte)  compositeWord;
                        array[position+3] = (byte) (compositeWord >> 8);
                    }
                }

                position += 4;
                break;

            default:
        }
    }


    /**
     * Take the node object and write its data into buffer in the
     * proper endian while also swapping primitive data if desired.
     *
     * @param node node to be written (is never null)
     * @param swapData do we swap the primitive data or not?
     */
    private void writeNode(EvioNode node, boolean swapData) {

        // Write header in endianness of buffer
        writeHeader(node);

        // Does this node contain other containers?
        if (node.getDataTypeObj().isStructure()) {
            // Iterate through list of children
            ArrayList<EvioNode> kids = node.getChildNodes();
            if (kids == null) return;
            for (EvioNode child : kids) {
                writeNode(child, swapData);
            }
        }
        else {
            ByteBuffer nodeBuf = node.bufferNode.buffer;

            if (swapData) {
                try {
                    ByteDataTransformer.swapData(node.getDataTypeObj(), nodeBuf,
                                                 buffer, node.dataPos, position,
                                                 node.dataLen, false);
                }
                catch (EvioException e) {
                    e.printStackTrace();
                }
            }
            else {
                if (!useByteBuffer && nodeBuf.hasArray() && buffer.hasArray() &&
                        nodeBuf.array().length == nodeBuf.capacity()) {
                    System.arraycopy(nodeBuf.array(), node.dataPos,
                                     array, position, 4*node.dataLen);
                }
                else {
                    ByteBuffer duplicateBuf = nodeBuf.duplicate();
                    duplicateBuf.limit(node.dataPos + 4*node.dataLen).position(node.dataPos);

                    buffer.position(position);
                    buffer.put(duplicateBuf); // this method is relative to position
                    buffer.position(0);       // get ready for external reading
                }
            }

            position += 4*node.dataLen;
        }
    }


    /**
     * Adds the evio structure represented by the EvioNode object
     * into the buffer.
     *
     * @param node the EvioNode object representing the evio structure to data.
     * @throws EvioException if data is null or empty;
     *                       if adding wrong data type to structure;
     *                       if structure not added first;
     *                       if no room in buffer for data.
     */
    public void addEvioNode(EvioNode node) throws EvioException {
        if (node == null) {
            throw new EvioException("no data to add");
        }

        if (currentStructure == null) {
            throw new EvioException("add a bank, segment, or tagsegment first");
        }

        if (currentStructure.dataType != node.getTypeObj()) {
            throw new EvioException("may only add " + currentStructure.dataType + " data");
        }

        // Sets pos = 0, limit = capacity, & does NOT clear data
        buffer.clear();

        int len = node.getTotalBytes();
//System.out.println("addEvioNode: buf lim = " + buffer.limit() +
//                           " - pos = " + position + " = (" + (buffer.limit() - position) +
//                           ") must be >= " + node.getTotalBytes() + " node total bytes");
        if (buffer.limit() - position < len) {
            throw new EvioException("no room in buffer");
        }

        addToAllLengths(len/4);  // # 32-bit words

        ByteBuffer nodeBuf = node.bufferNode.buffer;

        if (nodeBuf.order() == buffer.order()) {
            if (!useByteBuffer && nodeBuf.hasArray() && buffer.hasArray() &&
                    nodeBuf.array().length == nodeBuf.capacity()) {
//System.out.println("addEvioNode: arraycopy node (same endian)");
                System.arraycopy(nodeBuf.array(), node.pos,
                                 array, position, node.getTotalBytes());
            }
            else {
//System.out.println("addEvioNode: less efficient node copy (same endian)");
                // Better performance not to slice/duplicate buffer (5 - 10% faster)
                ByteBuffer duplicateBuf = nodeBuf.duplicate();
                duplicateBuf.limit(node.pos + node.getTotalBytes()).position(node.pos);

                buffer.position(position);
                buffer.put(duplicateBuf); // this method is relative to position
                buffer.position(0);       // get ready for external reading
            }

            position += len;
        }
        else {
            // If node is opposite endian as this buffer,
            // rewrite all evio header data, but leave
            // primitive data alone.
            writeNode(node, false);
        }
    }


    /**
     * Appends byte data to the structure.
     *
     * @param data the byte data to append.
     * @throws EvioException if data is null or empty;
     *                       if adding wrong data type to structure;
     *                       if structure not added first;
     *                       if no room in buffer for data.
     */
    public void addByteData(byte data[]) throws EvioException {
        if (data == null || data.length < 1) {
            throw new EvioException("no data to add");
        }

        if (currentStructure == null) {
            throw new EvioException("add a bank, segment, or tagsegment first");
        }

        if (currentStructure.dataType != DataType.CHAR8  &&
            currentStructure.dataType != DataType.UCHAR8)  {
            throw new EvioException("may only add " + currentStructure.dataType + " data");
        }

        // Sets pos = 0, limit = capacity, & does NOT clear data
        buffer.clear();

        int len = data.length;
        if (buffer.limit() - position < len) {
            throw new EvioException("no room in buffer");
        }

        // Things get tricky if this method is called multiple times in succession.
        // Keep track of how much data we write each time so length and padding
        // are accurate.

        // Last increase in length
        int lastWordLen = (currentStructure.dataLen + 3)/4;

        // If we are adding to existing data,
        // place position before previous padding.
        if (currentStructure.dataLen > 0) {
            position -= currentStructure.padding;
        }

        // Total number of bytes to write & already written
        currentStructure.dataLen += len;

        // New total word len of this data
        int totalWordLen = (currentStructure.dataLen + 3)/4;

        // Increase lengths by the difference
        addToAllLengths(totalWordLen - lastWordLen);

        // Copy the data in one chunk
        if (useByteBuffer) {
            buffer.position(position);
            buffer.put(data);
            buffer.position(0);
        }
        else {
            System.arraycopy(data, 0, array, position, len);
        }

        // Calculate the padding
        currentStructure.padding = padCount[currentStructure.dataLen % 4];

        // Advance buffer position
        position += len + currentStructure.padding;
    }


    /**
     * Appends int data to the structure.
     *
     * @param data the int data to append.
     * @throws EvioException if data is null or empty;
     *                       if adding wrong data type to structure;
     *                       if structure not added first;
     *                       if no room in buffer for data.
     */
    public void addIntData(int data[]) throws EvioException {
        if (data == null || data.length < 1) {
            throw new EvioException("no data to add");
        }

        if (currentStructure == null) {
            throw new EvioException("add a bank, segment, or tagsegment first");
        }

        if (currentStructure.dataType != DataType.INT32  &&
            currentStructure.dataType != DataType.UINT32 &&
            currentStructure.dataType != DataType.UNKNOWN32)  {
            throw new EvioException("may only add " + currentStructure.dataType + " data");
        }

        // Sets pos = 0, limit = capacity, & does NOT clear data
        buffer.clear();

        int len = data.length;
        if (buffer.limit() - position < 4*len) {
            throw new EvioException("no room in buffer");
        }

        addToAllLengths(len);  // # 32-bit words

        if (useByteBuffer) {
            buffer.position(position);
            IntBuffer db = buffer.asIntBuffer();
            db.put(data, 0, len);
            buffer.position(0);
        }
        else {
            int pos = position;
            if (order == ByteOrder.BIG_ENDIAN) {
                for (int aData : data) {
// System.out.println("int aData = " + aData);
                    array[pos++] = (byte) (aData >> 24);
                    array[pos++] = (byte) (aData >> 16);
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData);
                }
            }
            else {
                for (int aData : data) {
                    array[pos++] = (byte) (aData);
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData >> 16);
                    array[pos++] = (byte) (aData >> 24);
                }
            }
        }

        position += 4*len;     // # bytes
    }




    /**
     * Appends short data to the structure.
     *
     * @param data the short data to append.
     * @throws EvioException if data is null or empty;
     *                       if adding wrong data type to structure;
     *                       if structure not added first;
     *                       if no room in buffer for data.
     */
    public void addShortData(short data[]) throws EvioException {
        if (data == null || data.length < 1) {
            throw new EvioException("no data to add");
        }

        if (currentStructure == null) {
            throw new EvioException("add a bank, segment, or tagsegment first");
        }

        if (currentStructure.dataType != DataType.SHORT16  &&
            currentStructure.dataType != DataType.USHORT16)  {
            throw new EvioException("may only add " + currentStructure.dataType + " data");
        }

        // Sets pos = 0, limit = capacity, & does NOT clear data
        buffer.clear();

        int len = data.length;
        if (buffer.limit() - position < 2*len) {
            throw new EvioException("no room in buffer");
        }

        // Things get tricky if this method is called multiple times in succession.
        // Keep track of how much data we write each time so length and padding
        // are accurate.

        // Last increase in length
        int lastWordLen = (currentStructure.dataLen + 1)/2;
//System.out.println("lastWordLen = " + lastWordLen);

        // If we are adding to existing data,
        // place position before previous padding.
        if (currentStructure.dataLen > 0) {
            position -= currentStructure.padding;
//System.out.println("Back up before previous padding of " + currentStructure.padding);
        }

        // Total number of bytes to write & already written
        currentStructure.dataLen += len;
//System.out.println("Total bytes to write = " + currentStructure.dataLen);

        // New total word len of this data
        int totalWordLen = (currentStructure.dataLen + 1)/2;
//System.out.println("Total word len = " + totalWordLen);

        // Increase lengths by the difference
        addToAllLengths(totalWordLen - lastWordLen);

        if (useByteBuffer) {
            buffer.position(position);
            ShortBuffer db = buffer.asShortBuffer();
            db.put(data, 0, len);
            buffer.position(0);
        }
        else {
            int pos = position;
            if (order == ByteOrder.BIG_ENDIAN) {
                for (short aData : data) {
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData);
                }
            }
            else {
                for (short aData : data) {
                    array[pos++] = (byte) (aData);
                    array[pos++] = (byte) (aData >> 8);
                }
            }
        }

        currentStructure.padding = 2*(currentStructure.dataLen % 2);
//System.out.println("short padding = " + currentStructure.padding);
        // Advance position
        position += 2*len + currentStructure.padding;
//System.out.println("set pos = " + position);
//System.out.println("");
    }


    /**
     * Appends long data to the structure.
     *
     * @param data the long data to append.
     * @throws EvioException if data is null or empty;
     *                       if adding wrong data type to structure;
     *                       if structure not added first;
     *                       if no room in buffer for data.
     */
    public void addLongData(long data[]) throws EvioException {
        if (data == null || data.length < 1) {
            throw new EvioException("no data to add");
        }

        if (currentStructure == null) {
            throw new EvioException("add a bank, segment, or tagsegment first");
        }

        if (currentStructure.dataType != DataType.LONG64  &&
            currentStructure.dataType != DataType.ULONG64)  {
            throw new EvioException("may only add " + currentStructure.dataType + " data");
        }

        // Sets pos = 0, limit = capacity, & does NOT clear data
        buffer.clear();

        int len = data.length;
        if (buffer.limit() - position < 8*len) {
            throw new EvioException("no room in buffer");
        }

        addToAllLengths(2*len);  // # 32-bit words

        if (useByteBuffer) {
            buffer.position(position);
            LongBuffer db = buffer.asLongBuffer();
            db.put(data, 0, len);
            buffer.position(0);
        }
        else {
            int pos = position;
            if (order == ByteOrder.BIG_ENDIAN) {
                for (long aData : data) {
                    array[pos++] = (byte) (aData >> 56);
                    array[pos++] = (byte) (aData >> 48);
                    array[pos++] = (byte) (aData >> 40);
                    array[pos++] = (byte) (aData >> 32);
                    array[pos++] = (byte) (aData >> 24);
                    array[pos++] = (byte) (aData >> 16);
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData);
                }
            }
            else {
                for (long aData : data) {
                    array[pos++] = (byte) (aData);
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData >> 16);
                    array[pos++] = (byte) (aData >> 24);
                    array[pos++] = (byte) (aData >> 32);
                    array[pos++] = (byte) (aData >> 40);
                    array[pos++] = (byte) (aData >> 48);
                    array[pos++] = (byte) (aData >> 56);
                }
            }
        }

        position += 8*len;       // # bytes
    }


    /**
     * Appends float data to the structure.
     *
     * @param data the float data to append.
     * @throws EvioException if data is null or empty;
     *                       if adding wrong data type to structure;
     *                       if structure not added first;
     *                       if no room in buffer for data.
     */
    public void addFloatData(float data[]) throws EvioException {
        if (data == null || data.length < 1) {
            throw new EvioException("no data to add");
        }

        if (currentStructure == null) {
            throw new EvioException("add a bank, segment, or tagsegment first");
        }

        if (currentStructure.dataType != DataType.FLOAT32) {
            throw new EvioException("may only add " + currentStructure.dataType + " data");
        }

        // Sets pos = 0, limit = capacity, & does NOT clear data
        buffer.clear();

        int len = data.length;
        if (buffer.limit() - position < 4*len) {
            throw new EvioException("no room in buffer");
        }

        addToAllLengths(len);  // # 32-bit words

        if (useByteBuffer) {
            buffer.position(position);
            FloatBuffer db = buffer.asFloatBuffer();
            db.put(data, 0, len);
            buffer.position(0);
        }
        else {
            int aData, pos = position;
            if (order == ByteOrder.BIG_ENDIAN) {
                for (float fData : data) {
                    aData = Float.floatToRawIntBits(fData);
                    array[pos++] = (byte) (aData >> 24);
                    array[pos++] = (byte) (aData >> 16);
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData);
                }
            }
            else {
                for (float fData : data) {
                    aData = Float.floatToRawIntBits(fData);
                    array[pos++] = (byte) (aData);
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData >> 16);
                    array[pos++] = (byte) (aData >> 24);
                }
            }
        }

        position += 4*len;     // # bytes
    }




    /**
     * Appends double data to the structure.
     *
     * @param data the double data to append.
     * @throws EvioException if data is null or empty;
     *                       if adding wrong data type to structure;
     *                       if structure not added first;
     *                       if no room in buffer for data.
     */
    public void addDoubleData(double data[]) throws EvioException {
        if (data == null || data.length < 1) {
            throw new EvioException("no data to add");
        }

        if (currentStructure == null) {
            throw new EvioException("add a bank, segment, or tagsegment first");
        }

        if (currentStructure.dataType != DataType.DOUBLE64) {
            throw new EvioException("may only add " + currentStructure.dataType + " data");
        }

        // Sets pos = 0, limit = capacity, & does NOT clear data
        buffer.clear();

        int len = data.length;
        if (buffer.limit() - position < 8*len) {
            throw new EvioException("no room in buffer");
        }

        addToAllLengths(2*len);  // # 32-bit words

        if (useByteBuffer) {
            buffer.position(position);
            DoubleBuffer db = buffer.asDoubleBuffer();
            db.put(data, 0, len);
            buffer.position(0);
        }
        else {
            long aData;
            int pos = position;
            if (order == ByteOrder.BIG_ENDIAN) {
                for (double dData : data) {
                    aData = Double.doubleToRawLongBits(dData);
                    array[pos++] = (byte) (aData >> 56);
                    array[pos++] = (byte) (aData >> 48);
                    array[pos++] = (byte) (aData >> 40);
                    array[pos++] = (byte) (aData >> 32);
                    array[pos++] = (byte) (aData >> 24);
                    array[pos++] = (byte) (aData >> 16);
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData);
                }
            }
            else {
                for (double dData : data) {
                    aData = Double.doubleToRawLongBits(dData);
                    array[pos++] = (byte) (aData);
                    array[pos++] = (byte) (aData >> 8);
                    array[pos++] = (byte) (aData >> 16);
                    array[pos++] = (byte) (aData >> 24);
                    array[pos++] = (byte) (aData >> 32);
                    array[pos++] = (byte) (aData >> 40);
                    array[pos++] = (byte) (aData >> 48);
                    array[pos++] = (byte) (aData >> 56);
                }
            }
        }

        position += 8*len;     // # bytes
    }




    /**
     * Appends string array to the structure.
     *
     * @param strings the strings to append.
     * @throws EvioException if data is null or empty;
     *                       if adding wrong data type to structure;
     *                       if structure not added first;
     *                       if no room in buffer for data.
     */
    public void addStringData(String strings[]) throws EvioException {
        if (strings == null || strings.length < 1) {
            throw new EvioException("no data to add");
        }

        if (currentStructure == null) {
            throw new EvioException("add a bank, segment, or tagsegment first");
        }

        if (currentStructure.dataType != DataType.CHARSTAR8) {
            throw new EvioException("may only add " + currentStructure.dataType + " data");
        }

        // Convert strings into byte array (already padded)
        byte[] data = BaseStructure.stringsToRawBytes(strings);

        // Sets pos = 0, limit = capacity, & does NOT clear data
        buffer.clear();

        int len = data.length;
        if (buffer.limit() - position < len) {
            throw new EvioException("no room in buffer");
        }

        // Things get tricky if this method is called multiple times in succession.
        // In fact, it's too difficult to bother with so just throw an exception.
        if (currentStructure.dataLen > 0) {
            throw new EvioException("addStringData() may only be called once per structure");
        }

        if (useByteBuffer) {
            buffer.position(position);
            buffer.put(data);
            buffer.position(0);
        }
        else {
            System.arraycopy(data, 0, array, position, len);
        }
        currentStructure.dataLen += len;
        addToAllLengths(len/4);

        position += len;
    }


    /**
     * This method writes a file in proper evio format with block header
     * containing the single event constructed by this object. This is
     * useful as a test to see if event is being properly constructed.
     * Use this in conjunction with the event viewer in order to check
     * that format is correct. This only works with array backed buffers.
     *
     * @param filename name of file
     */
    public void toFile(String filename) {
        try {
            File file = new File(filename);
            FileOutputStream fos = new FileOutputStream(file);
            java.io.DataOutputStream dos = new java.io.DataOutputStream(fos);

            boolean writeLastBlock = false;

            // Get buffer ready to read
            buffer.limit(position);
            buffer.position(0);

            // Write beginning 8 word header
            // total length
            int len = buffer.remaining();
//System.out.println("Buf len = " + len + " bytes");
            int len2 = len/4 + 8;
//System.out.println("Tot len = " + len2 + " words");
            // blk #
            int blockNum = 1;
            // header length
            int headerLen = 8;
            // event count
            int evCount = 1;
            // bit & version (last block, evio version 4 */
            int info = 0x204;
            if (writeLastBlock) {
                info = 0x4;
            }

            // endian checker
            int magicNum = IBlockHeader.MAGIC_NUMBER;

            if (order == ByteOrder.LITTLE_ENDIAN) {
                len2      = Integer.reverseBytes(len2);
                blockNum  = Integer.reverseBytes(blockNum);
                headerLen = Integer.reverseBytes(headerLen);
                evCount   = Integer.reverseBytes(evCount);
                info      = Integer.reverseBytes(info);
                magicNum  = Integer.reverseBytes(magicNum);
            }

            // Write the block header

            dos.writeInt(len2);       // len
            dos.writeInt(blockNum);   // block num
            dos.writeInt(headerLen);  // header len
            dos.writeInt(evCount);    // event count
            dos.writeInt(0);          // reserved 1
            dos.writeInt(info);       // bit & version info
            dos.writeInt(0);          // reserved 2
            dos.writeInt(magicNum);   // magic #

            // Now write the event data
            dos.write(array, 0, len);

            dos.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

        // Reset position & limit
        buffer.clear();
    }

}
