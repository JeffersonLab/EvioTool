package org.jlab.coda.jevio;

import java.nio.*;
import java.util.*;

/**
 * This class is used to read the bytes of just an evio structure (<b>NOT</b>
 * a full evio version 4 formatted file or buffer). It is also capable of
 * adding a structure to an event.
 * The {@link EvioCompactReader} class is similar but reads files and buffers
 * in the complete evio version 4 format.
 * It is theoretically thread-safe.
 * It is designed to be fast and does <b>NOT</b> do a deserialization
 * on the buffer examined.<p>
 *
 * @author timmer
 */
public class EvioCompactStructureHandler {

    /** Stores structure info. */
    private EvioNode node;

    /** The buffer being read. */
    private ByteBuffer byteBuffer;

    /** The byte order of the buffer being read. */
    private ByteOrder byteOrder;

    /** Initial position of buffer. */
    private int initialPosition;

    /** Is this object currently closed? */
    private boolean closed;


    //------------------------


    /**
     * Constructor for reading an EvioNode object.
     *
     * @param node the node to be analyzed.
     * @throws EvioException if node arg is null;
     *                       if byteBuffer not in proper format;
     */
    public EvioCompactStructureHandler(EvioNode node) throws EvioException {

        if (node == null) {
            throw new EvioException("node arg is null");
        }

        ByteBuffer byteBuffer = node.getBufferNode().getBuffer();

        if (byteBuffer.remaining() < 1) {
            throw new EvioException("Buffer has too little data");
        }
        else if ( (node.getTypeObj() == DataType.BANK || node.getTypeObj() == DataType.ALSOBANK) &&
                   byteBuffer.remaining() < 2 ) {
            throw new EvioException("Buffer has too little data");
        }

        this.node = node;
        this.byteBuffer = byteBuffer;
        initialPosition = byteBuffer.position();
        byteOrder = byteBuffer.order();

        // See if the length given in header is consistent with buffer size
        if (node.len + 1 > byteBuffer.remaining()/4) {
            throw new EvioException("Buffer has too little data");
        }
    }


    /**
     * Constructor for reading a buffer that contains 1 structure only (no block headers).
     *
     * @param byteBuffer the buffer to be read that contains 1 structure only (no block headers).
     * @param type the type of outermost structure contained in buffer, may be {@link DataType#BANK},
     *             {@link DataType#SEGMENT}, {@link DataType#TAGSEGMENT} or equivalent.
     * @throws EvioException if byteBuffer arg is null;
     *                       if type arg is null or is not an evio structure;
     *                       if byteBuffer not in proper format;
     */
    public EvioCompactStructureHandler(ByteBuffer byteBuffer, DataType type) throws EvioException {

        if (byteBuffer == null) {
            throw new EvioException("Buffer arg is null");
        }

        if (type == null || !type.isStructure()) {
            throw new EvioException("type arg is null or is not an evio structure");
        }

        if (byteBuffer.remaining() < 1) {
            throw new EvioException("Buffer has too little data");
        }
        else if ( (type == DataType.BANK || type == DataType.ALSOBANK) &&
                   byteBuffer.remaining() < 2 ) {
            throw new EvioException("Buffer has too little data");
        }

        initialPosition = byteBuffer.position();
        this.byteBuffer = byteBuffer;
        byteOrder = byteBuffer.order();

        // Pull out all header info & place into EvioNode object
        node = extractNode(byteBuffer, null, null, type, initialPosition, 0, false);

        // See if the length given in header is consistent with buffer size
        if (node.len + 1 > byteBuffer.remaining()/4) {
            throw new EvioException("Buffer has too little data");
        }
    }


    /**
     * This method can be used to avoid creating additional EvioCompactEventReader
     * objects by reusing this one with another buffer. The method
     * {@link #close()} is called before anything else.
     *
     * @param buf the buffer to be read that contains 1 structure only (no block headers).
     * @param type the type of outermost structure contained in buffer, may be {@link DataType#BANK},
     *             {@link DataType#SEGMENT}, {@link DataType#TAGSEGMENT} or equivalent.
     * @throws EvioException if buf arg is null;
     *                       if type arg is null or is not an evio structure;
     *                       if buf not in proper format;
     */
    public synchronized void setBuffer(ByteBuffer buf, DataType type) throws EvioException {

        if (buf == null) {
            throw new EvioException("Buffer arg is null");
        }

        if (type == null || !type.isStructure()) {
            throw new EvioException("type arg is null or is not an evio structure");
        }

        if (byteBuffer.remaining() < 1) {
            throw new EvioException("Buffer has too little data");
        }
        else if ( (type == DataType.BANK || type == DataType.ALSOBANK) &&
                   byteBuffer.remaining() < 2 ) {
            throw new EvioException("Buffer has too little data");
        }

        close();

        initialPosition  = buf.position();
        byteBuffer       = buf;
        byteOrder        = byteBuffer.order();

        // Pull out all header info & place into EvioNode object
        node = extractNode(byteBuffer, null, null, type, initialPosition, 0, false);

        // See if the length given in header is consistent with buffer size
        if (node.len + 1 > byteBuffer.remaining()/4) {
            throw new EvioException("Buffer has too little data");
        }

        closed = false;
    }


    /**
     * Has {@link #close()} been called (without reopening by calling
     * {@link #setBuffer(java.nio.ByteBuffer, DataType)})?
     *
     * @return {@code true} if this object closed, else {@code false}.
     */
    public synchronized boolean isClosed() {return closed;}


    /**
     * Get the byte buffer being read.
     * @return the byte buffer being read.
     */
    public ByteBuffer getByteBuffer() {
        return byteBuffer;
    }


    /**
     * Get the EvioNode object associated with the structure.
     * @return EvioNode object associated with the structure.
     */
    public EvioNode getStructure() {
        return node;
    }

    /**
     * Get the EvioNode object associated with a particular event number
     * which has been scanned so all substructures are contained in the
     * node.allNodes list.
     * @return  EvioNode object associated with a particular event number,
     *
     *         or null if there is none.
     */
    public EvioNode getScannedStructure() {
        scanStructure(node);
        return node;
    }


    /**
     * This method extracts an EvioNode object from a given buffer, a
     * location in the buffer, and a few other things. An EvioNode
     * object represents an evio container - either a bank, segment,
     * or tag segment.
     *
     * @param buffer    buffer to examine
     * @param blockNode object holding data about block header
     * @param eventNode object holding data about containing event (null if isEvent = true)
     * @param type      type of evio structure to extract
     * @param position  position in buffer
     * @param place     place of event in buffer (starting at 0)
     * @param isEvent   is the node being extracted an event (top-level bank)?
     *
     * @return          EvioNode object containing evio structure information
     * @throws          EvioException if file/buffer not in evio format
     */
    private EvioNode extractNode(ByteBuffer buffer, BlockNode blockNode,
                                 EvioNode eventNode, DataType type,
                                 int position, int place, boolean isEvent)
            throws EvioException {

        // Store current evio info without de-serializing
        EvioNode node  = new EvioNode();
        node.pos        = position;
        node.place      = place;      // Which # event from beginning am I?
        node.blockNode  = blockNode;  // Block associated with this event, always null
        node.eventNode  = eventNode;
        node.isEvent    = isEvent;
        node.type       = type.getValue();
        node.bufferNode = new BufferNode(buffer);

        try {

            switch (type) {

                case BANK:
                case ALSOBANK:

                    // Get length of current event
                    node.len = buffer.getInt(position);
                    // Position of data for a bank
                    node.dataPos = position + 8;
                    // Len of data for a bank
                    node.dataLen = node.len - 1;

                    // Hop over length word
                    position += 4;

                    // Read second header word
                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        node.tag = buffer.getShort(position) & 0x0000ffff;
                        position += 2;

                        int dt = buffer.get(position++) & 0x000000ff;
                        node.dataType = dt & 0x3f;
                        node.pad  = dt >>> 6;
                        // If only 7th bit set, that can only be the legacy tagsegment type
                        // with no padding information - convert it properly.
                        if (dt == 0x40) {
                            node.dataType = DataType.TAGSEGMENT.getValue();
                            node.pad = 0;
                        }

                        node.num = buffer.get(position) & 0x000000ff;
                    }
                    else {
                        node.num = buffer.get(position++) & 0x000000ff;

                        int dt = buffer.get(position++) & 0x000000ff;
                        node.dataType = dt & 0x3f;
                        node.pad  = dt >>> 6;
                        if (dt == 0x40) {
                            node.dataType = DataType.TAGSEGMENT.getValue();
                            node.pad = 0;
                        }

                        node.tag = buffer.getShort(position) & 0x0000ffff;
                    }
                    break;

                case SEGMENT:
                case ALSOSEGMENT:

                    node.dataPos = position + 4;

                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        node.tag = buffer.get(position++) & 0x000000ff;

                        int dt = buffer.get(position++) & 0x000000ff;
                        node.dataType = dt & 0x3f;
                        node.pad = dt >>> 6;
                        if (dt == 0x40) {
                            node.dataType = DataType.TAGSEGMENT.getValue();
                            node.pad = 0;
                        }

                        node.len = buffer.getShort(position) & 0x0000ffff;
                    }
                    else {
                        node.len = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                        int dt = buffer.get(position++) & 0x000000ff;
                        node.dataType = dt & 0x3f;
                        node.pad = dt >>> 6;
                        if (dt == 0x40) {
                            node.dataType = DataType.TAGSEGMENT.getValue();
                            node.pad = 0;
                        }
                        node.tag = buffer.get(position) & 0x000000ff;
                    }

                    node.dataLen = node.len;

                    break;

                case TAGSEGMENT:

                    node.dataPos = position + 4;

                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        int temp = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                        node.tag = temp >>> 4;
                        node.dataType = temp & 0xF;
                        node.len = buffer.getShort(position) & 0x0000ffff;
                    }
                    else {
                        node.len = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                        int temp = buffer.getShort(position) & 0x0000ffff;
                        node.tag = temp >>> 4;
                        node.dataType = temp & 0xF;
                    }

                    node.dataLen = node.len;

                    break;

                default:
                    throw new EvioException("File/buffer bad format");
            }
        }
        catch (BufferUnderflowException a) {
            throw new EvioException("File/buffer bad format");
        }

        return node;
    }


    /**
     * This method recursively stores, in the given list, all the information
     * about an evio structure's children found in the given ByteBuffer object.
     * It uses absolute gets so buffer's position does <b>not</b> change.
     *
     * @param node node being scanned
     */
    static private void scanStructure(EvioNode node) {

        // Type of evio structure being scanned
        DataType type = node.getDataTypeObj();

        // If node does not contain containers, return since we can't drill any further down
        if (!type.isStructure()) {
//System.out.println("scanStructure: NODE is not a structure, return");
            return;
        }
 //System.out.println("scanStructure: scanning evio struct with len = " + node.dataLen);
 //System.out.println("scanStructure: data type of node to be scanned = " + type);

        // Start at beginning position of evio structure being scanned
        int position = node.dataPos;
        // Length of evio structure being scanned in bytes
        int dataBytes = 4*node.dataLen;
        // Don't go past the data's end
        int endingPos = position + dataBytes;
        // Buffer we're using
        ByteBuffer buffer = node.bufferNode.buffer;

        int dt, dataType, dataLen, len, pad, tag, num;

        // Do something different depending on what node contains
        switch (type) {
            case BANK:
            case ALSOBANK:

                // Extract all the banks from this bank of banks.
                while (position < endingPos) {
//System.out.println("scanStructure: start at pos " + position + " < end " + endingPos + " ?");

//System.out.println("scanStructure: buf is at pos " + buffer.position() +
//                   ", limit =  " + buffer.limit() + ", remaining = " + buffer.remaining() +
//                   ", capacity = " + buffer.capacity());

                    // Read first header word
                    len = buffer.getInt(position);
                    dataLen = len - 1; // Len of data (no header) for a bank
                    position += 4;

                    // Read & parse second header word
                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        tag = buffer.getShort(position) & 0x0000ffff;
                        position += 2;

                        dt = buffer.get(position++) & 0x000000ff;
                        dataType = dt & 0x3f;
                        pad  = dt >>> 6;
                        // If only 7th bit set, that can only be the legacy tagsegment type
                        // with no padding information - convert it properly.
                        if (dt == 0x40) {
                            dataType = DataType.TAGSEGMENT.getValue();
                            pad = 0;
                        }

                        num = buffer.get(position++) & 0x000000ff;
                    }
                    else {
                        num = buffer.get(position++) & 0x000000ff;

                        dt = buffer.get(position++) & 0x000000ff;
                        dataType = dt & 0x3f;
                        pad  = dt >>> 6;
                        if (dt == 0x40) {
                            dataType = DataType.TAGSEGMENT.getValue();
                            pad = 0;
                        }

                        tag = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                    }
//System.out.println("found tag/num = " + tag + ", " + num);

                    // Cloning is a fast copy that eliminates the need
                    // for setting stuff that's the same as the parent.
                    EvioNode kidNode = (EvioNode)node.clone();

                    kidNode.len  = len;
                    kidNode.pos  = position - 8;
                    kidNode.type = DataType.BANK.getValue();  // This is a bank

                    kidNode.dataLen  = dataLen;
                    kidNode.dataPos  = position;
                    kidNode.dataType = dataType;

                    kidNode.pad = pad;
                    kidNode.tag = tag;
                    kidNode.num = num;

                    // Create the tree structure
                    kidNode.isEvent = false;
                    kidNode.parentNode = node;

                    // Add this to list of children and to list of all nodes in the event
                    node.addChild(kidNode);
//System.out.println("scanStructure: kid bank, scanned val = " + kidNode.scanned);

//System.out.println("scanStructure: kid bank at pos = " + kidNode.pos +
//                    " with type " +  DataType.getDataType(dataType) + ", tag/num = " + kidNode.tag +
//                    "/" + kidNode.num + ", list size = " + node.eventNode.allNodes.size());

                    // Only scan through this child if it's a container
                    if (DataType.isStructure(dataType)) {
                        scanStructure(kidNode);
                    }

                    // Set position to start of next header (hop over kid's data)
                    position += 4*dataLen;
                }

                break; // structure contains banks

            case SEGMENT:
            case ALSOSEGMENT:

                // Extract all the segments from this bank of segments.
                while (position < endingPos) {

                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        tag = buffer.get(position++) & 0x000000ff;
                        dt = buffer.get(position++) & 0x000000ff;
                        dataType = dt & 0x3f;
                        pad = dt >>> 6;
                        if (dt == 0x40) {
                            dataType = DataType.TAGSEGMENT.getValue();
                            pad = 0;
                        }
                        len = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                    }
                    else {
                        len = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                        dt = buffer.get(position++) & 0x000000ff;
                        dataType = dt & 0x3f;
                        pad = dt >>> 6;
                        if (dt == 0x40) {
                            dataType = DataType.TAGSEGMENT.getValue();
                            pad = 0;
                        }
                        tag = buffer.get(position++) & 0x000000ff;
                    }
//System.out.println("found tag/num = " + tag + ", 0");

                    EvioNode kidNode = (EvioNode)node.clone();

                    kidNode.len  = len;
                    kidNode.pos  = position - 4;
                    kidNode.type = DataType.SEGMENT.getValue();  // This is a segment

                    kidNode.dataLen  = len;
                    kidNode.dataPos  = position;
                    kidNode.dataType = dataType;

                    kidNode.pad = pad;
                    kidNode.tag = tag;
                    kidNode.num = 0;

                    kidNode.isEvent = false;
                    kidNode.parentNode = node;

                    node.addChild(kidNode);
//System.out.println("scanStructure: kid bank, scanned val = " + kidNode.scanned);

// System.out.println("scanStructure: kid seg at pos = " + kidNode.pos +
//                    " with type " +  DataType.getDataType(dataType) + ", tag/num = " + kidNode.tag +
//                    "/" + kidNode.num + ", list size = " + node.eventNode.allNodes.size());
                    if (DataType.isStructure(dataType)) {
                        scanStructure(kidNode);
                    }

                    position += 4*len;
                }

                break; // structure contains segments

            case TAGSEGMENT:

                // Extract all the tag segments from this bank of tag segments.
                while (position < endingPos) {

                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        int temp = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                        tag = temp >>> 4;
                        dataType = temp & 0xF;
                        len = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                    }
                    else {
                        len = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                        int temp = buffer.getShort(position) & 0x0000ffff;
                        position += 2;
                        tag = temp >>> 4;
                        dataType = temp & 0xF;
                    }
//System.out.println("found tag/num = " + tag + ", 0");

                    EvioNode kidNode = (EvioNode)node.clone();

                    kidNode.len  = len;
                    kidNode.pos  = position - 4;
                    kidNode.type = DataType.TAGSEGMENT.getValue();  // This is a tag segment

                    kidNode.dataLen  = len;
                    kidNode.dataPos  = position;
                    kidNode.dataType = dataType;

                    kidNode.pad = 0;
                    kidNode.tag = tag;
                    kidNode.num = 0;

                    kidNode.isEvent = false;
                    kidNode.parentNode = node;

                    node.addChild(kidNode);
//System.out.println("scanStructure: kid bank, scanned val = " + kidNode.scanned);

// System.out.println("scanStructure: kid tagseg at pos = " + kidNode.pos +
//                    " with type " +  DataType.getDataType(dataType) + ", tag/num = " + kidNode.tag +
//                    "/" + kidNode.num + ", list size = " + node.eventNode.allNodes.size());
                   if (DataType.isStructure(dataType)) {
                        scanStructure(kidNode);
                    }

                    position += 4*len;
                }

                break;

            default:
        }
    }

    /**
     * This method prints out a portion of a given ByteBuffer object
     * in hex representation of ints.
     *
     * @param buf buffer to be printed out
     * @param lenInInts length of data in ints to be printed
     */
    private static void printBuffer(ByteBuffer buf, int lenInInts) {
        IntBuffer ibuf = buf.asIntBuffer();
        lenInInts = lenInInts > ibuf.capacity() ? ibuf.capacity() : lenInInts;
        for (int i=0; i < lenInInts; i++) {
            System.out.println("  Buf(" + i + ") = 0x" + Integer.toHexString(ibuf.get(i)));
        }
    }


    /**
     * This method scans the event in the buffer.
     * The results are stored for future reference so the
     * event is only scanned once. It returns a list of EvioNode
     * objects representing evio structures (banks, segs, tagsegs).
     *
     * @return the list of objects (evio structures containing data)
     *         obtained from the scan
     */
    private ArrayList<EvioNode> scanStructure() {
//        if (node.scanned) {
//            node.clearLists();
//        }


        if (!node.scanned) {
            // Do this before actual scan so clone() sets all "scanned" fields
            // of child nodes to "true" as well.
            node.scanned = true;

//System.out.println("scanStructure: not scanned do so now");
            scanStructure(node);
        }
//        else {
//            System.out.println("scanStructure: already scanned, skip it");
//            scanStructure(node);
//        }

        // Return results of this or a previous scan
        return node.allNodes;
    }


    /**
     * This method searches the event and
     * returns a list of objects each of which contain information
     * about a single evio structure which matches the given tag and num.
     *
     * @param tag tag to match
     * @param num num to match
     * @return list of EvioNode objects corresponding to matching evio structures
     *         (empty if none found)
     * @throws EvioException if bad arg value(s);
     *                       if object closed
     */
    public synchronized List<EvioNode> searchStructure(int tag, int num)
                                 throws EvioException {

        // check args
        if (tag < 0 || num < 0) {
            throw new EvioException("bad arg value(s)");
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        ArrayList<EvioNode> returnList = new ArrayList<EvioNode>(100);

        // If event has been previously scanned we get that list,
        // otherwise we scan the node and store the results for future use.
        ArrayList<EvioNode> list = scanStructure();
//System.out.println("searchEvent: list size = " + list.size() +
//" for tag/num = " + tag + "/" + num);

        // Now look for matches in this event
        for (EvioNode enode: list) {
//System.out.println("searchEvent: desired tag = " + tag + " found " + enode.tag);
//System.out.println("           : desired num = " + num + " found " + enode.num);
            if (enode.tag == tag && enode.num == num) {
//System.out.println("           : found node at pos = " + enode.pos + " len = " + enode.len);
                returnList.add(enode);
            }
        }

        return returnList;
    }


    /**
     * This method searches the event and
     * returns a list of objects each of which contain information
     * about a single evio structure which matches the given dictionary
     * entry name.
     *
     * @param  dictName name of dictionary entry to search for
     * @param  dictionary dictionary to use
     * @return list of EvioNode objects corresponding to matching evio structures
     *         (empty if none found)
     * @throws EvioException if either dictName or dictionary arg is null;
     *                       if dictName is an invalid dictionary entry;
     *                       if object closed
     */
    public List<EvioNode> searchStructure(String dictName,
                                          EvioXMLDictionary dictionary)
                                 throws EvioException {

        if (dictName == null || dictionary == null) {
            throw new EvioException("null dictionary and/or entry name");
        }

        int tag = dictionary.getTag(dictName);
        int num = dictionary.getNum(dictName);
        if (tag == -1 || num == -1) {
            throw new EvioException("no dictionary entry for " + dictName);
        }

        return searchStructure(tag, num);
    }


    /**
     * This method adds a bank, segment, or tagsegment onto the end of a
     * structure which contains banks, segments, or tagsegments respectively.
     * It is the responsibility of the caller to make sure that
     * the buffer argument contains valid evio data (only data representing
     * the bank or structure to be added - not in file format with bank
     * header and the like) which is compatible with the type of data
     * structure stored in the given event. This means it is not possible to
     * add to structures which contain only a primitive data type.<p>
     * To produce properly formatted evio data, use
     * {@link EvioBank#write(java.nio.ByteBuffer)},
     * {@link EvioSegment#write(java.nio.ByteBuffer)} or
     * {@link EvioTagSegment#write(java.nio.ByteBuffer)} depending on whether
     * a bank, seg, or tagseg is being added.<p>
     * A note about files here. If the constructor of this reader read in data
     * from a file, it will now switch to using a new, internal buffer which
     * is returned by this method or can be retrieved by calling
     * {@link #getByteBuffer()}. It will <b>not</b> expand the file originally used.
     * The given buffer argument must be ready to read with its position and limit
     * defining the limits of the data to copy.
     * This method is synchronized due to the bulk, relative puts.
     *
     * @param addBuffer buffer containing evio data to add (<b>not</b> evio file format,
     *                  i.e. no bank headers)
     * @return a new ByteBuffer object which is created and filled with all the data
     *         including what was just added.
     * @throws EvioException if addBuffer is null;
     *                       if addBuffer arg is empty or has non-evio format;
     *                       if addBuffer is opposite endian to current event buffer;
     *                       if added data is not the proper length (i.e. multiple of 4 bytes);
     *                       if there is an internal programming error;
     *                       if object closed
     */
    public synchronized ByteBuffer addStructure(ByteBuffer addBuffer) throws EvioException {

        // If we're adding nothing, then DO nothing
//System.out.println("addStructure: addBuffer = " + addBuffer);
//if (addBuffer != null) System.out.println("   remaining = " + addBuffer.remaining());

        if (addBuffer == null || addBuffer.remaining() < 4) {
            throw new EvioException("null, empty, or non-evio format buffer arg");
        }

        if (addBuffer.order() != byteOrder) {
            throw new EvioException("trying to add wrong endian buffer");
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        // Position in byteBuffer just past end of event
        int endPos = node.dataPos + 4* node.dataLen;

        // Original position of buffer being added
        int origAddBufPos = addBuffer.position();

        // How many bytes are we adding?
        int appendDataLen = addBuffer.remaining();

        // Make sure it's a multiple of 4
        if (appendDataLen % 4 != 0) {
            throw new EvioException("data added is not in evio format");
        }

        // Data length in 32-bit words
        int appendDataWordLen = appendDataLen / 4;

        // Event contains structures of this type
        DataType eventDataType = node.getDataTypeObj();

//System.out.println("APPENDING " + appendDataWordLen + " words to buf " + byteBuffer);
        //--------------------------------------------------------
        // Add new structure to end of event (nothing comes after)
        //--------------------------------------------------------

        // Create a new buffer
        ByteBuffer newBuffer = ByteBuffer.allocate(endPos - initialPosition + appendDataLen);
        newBuffer.order(byteOrder);
        // Copy existing buffer into new buffer
        byteBuffer.limit(endPos).position(initialPosition);
        newBuffer.put(byteBuffer);
        // Remember position where we put new data into new buffer
        int newDataPos = newBuffer.position();
        // Copy new structure into new buffer
        newBuffer.put(addBuffer);
        // Get new buffer ready for reading
        newBuffer.flip();
        // Restore original positions of buffers
        byteBuffer.position(initialPosition);
        addBuffer.position(origAddBufPos);

        //-------------------------------------
        // If initialPosition was not 0 (in the new buffer it always is),
        // then ALL nodes need their position members shifted by initialPosition
        // bytes upstream.
        //-------------------------------------
        boolean didEventNode = false;

        if (initialPosition != 0) {
            // If event has been scanned, adjust it and all its sub-nodes
            if (node.allNodes != null) {
//System.out.println("Scanned node " + (i+1) +":");
                for (EvioNode evNode : node.allNodes) {
//System.out.println("      pos = " + evNode.pos + ", dataPos = " + evNode.dataPos);
                    evNode.pos     -= initialPosition;
                    evNode.dataPos -= initialPosition;
                    // remember if the event's node object is in the scanned list
                    if (evNode == node) didEventNode = true;
                }
            }

            // Deal with the event node unless we did it already above.
            // This would happen only if the top level bank was a bank of primitive types.
            if (!didEventNode) {
//System.out.println("Modify node " + (i+1) + ", pos = " + eNode.pos + ", dataPos = " + eNode.dataPos);
                node.pos     -= initialPosition;
                node.dataPos -= initialPosition;
            }
        }

        // At this point all EvioNode objects (including those in
        // user's possession) are updated.

        // This reader object is NOW using the new buffer
        byteBuffer      = newBuffer;
        initialPosition = newBuffer.position();

        // Position in new byteBuffer of event
        int eventLenPos = node.pos;

        //--------------------------------------------
        // Adjust event sizes in both
        // node object and in new buffer.
        //--------------------------------------------

        // Increase event size
        node.len     += appendDataWordLen;
        node.dataLen += appendDataWordLen;

        switch (eventDataType) {
            case BANK:
            case ALSOBANK:
//System.out.println("event pos = " + eventLenPos + ", len = " + (eventNode.len - appendDataWordLen) +
//                   ", set to " + (eventNode.len));

                newBuffer.putInt(eventLenPos, node.len);
                break;

            case SEGMENT:
            case ALSOSEGMENT:
            case TAGSEGMENT:
//System.out.println("event SEG/TAGSEG pos = " + eventLenPos + ", len = " + (eventNode.len - appendDataWordLen) +
//                   ", set to " + (eventNode.len));
                if (byteBuffer.order() == ByteOrder.BIG_ENDIAN) {
                    newBuffer.putShort(eventLenPos+2, (short)(node.len));
                }
                else {
                    newBuffer.putShort(eventLenPos,   (short)(node.len));
                }
                break;

            default:
                throw new EvioException("internal programming error");
        }

        // Scan new structure -if & only if- its containing event has already been scanned.
        if (node.scanned) {
//System.out.println("Extracting new node: at pos = " + newDataPos + ", type = " +  eventDataType);
            // Create a node object from the data we just added to the byteBuffer
            EvioNode newNode = extractNode(newBuffer, node.blockNode, node,
                                           eventDataType, newDataPos, 0, false);

//System.out.println("   new node: pos = " + newNode.pos + ", type = " +  newNode.getDataTypeObj() +
//                        ", data pos = " + newNode.dataPos);

            // Add to the new node to the list
            node.allNodes.add(newNode);

            // Add its kids too
            scanStructure(newNode);
            node.childNodes.addAll(newNode.childNodes);
        }


        return newBuffer;
    }


    /**
     * Get the data associated with an evio structure in ByteBuffer form.
     * The returned buffer is a view into this reader's buffer (no copy done).
     * Changes in one will affect the other.
     *
     * @param node evio structure whose data is to be retrieved
     * @throws EvioException if object closed
     * @return ByteBuffer object containing data. Position and limit are
     *         set for reading.
     */
    public ByteBuffer getData(EvioNode node) throws EvioException {
        return getData(node, false);
    }


    /**
     * Get the data associated with an evio structure in ByteBuffer form.
     * Depending on the copy argument, the returned buffer will either be
     * a copy of or a view into the data of this reader's buffer.<p>
     * This method is synchronized due to the bulk, relative gets & puts.
     *
     * @param node evio structure whose data is to be retrieved
     * @param copy if <code>true</code>, then return a copy as opposed to a
     *        view into this reader object's buffer.
     * @throws EvioException if object closed
     * @return ByteBuffer object containing data. Position and limit are
     *         set for reading.
     */
    public synchronized ByteBuffer getData(EvioNode node, boolean copy)
                                    throws EvioException {
        if (closed) {
            throw new EvioException("object closed");
        }

        int pos = byteBuffer.position();
        int lim = byteBuffer.limit();
        byteBuffer.limit(node.dataPos + 4*node.dataLen - node.pad).position(node.dataPos);

        if (copy) {
            ByteBuffer newBuf = ByteBuffer.allocate(4*node.dataLen - node.pad).order(byteOrder);
            // Relative get and put changes position in both buffers
            newBuf.put(byteBuffer);
            newBuf.flip();
            byteBuffer.limit(lim).position(pos);
            return newBuf;
        }

        ByteBuffer buf = byteBuffer.slice().order(byteOrder);
        byteBuffer.limit(lim).position(pos);
        return buf;
    }


    /**
     * Get an evio structure (bank, seg, or tagseg) in ByteBuffer form.
     * The returned buffer is a view into the data of this reader's buffer.<p>
     * This method is synchronized due to the bulk, relative gets & puts.
     *
     * @param node node object representing evio structure of interest
     * @return ByteBuffer object containing bank's/event's bytes. Position and limit are
     *         set for reading.
     * @throws EvioException if node is null;
     */
    public ByteBuffer getStructureBuffer(EvioNode node) throws EvioException {
        return getStructureBuffer(node, false);
    }


    /**
     * Get an evio structure (bank, seg, or tagseg) in ByteBuffer form.
     * Depending on the copy argument, the returned buffer will either be
     * a copy of or a view into the data of this reader's buffer.<p>
     * This method is synchronized due to the bulk, relative gets & puts.
     *
     * @param node node object representing evio structure of interest
     * @param copy if <code>true</code>, then return a copy as opposed to a
     *        view into this reader object's buffer.
     * @return ByteBuffer object containing structure's bytes. Position and limit are
     *         set for reading.
     * @throws EvioException if node is null;
     *                       if object closed
     */
    public synchronized ByteBuffer getStructureBuffer(EvioNode node, boolean copy)
            throws EvioException {

        if (node == null) {
            throw new EvioException("node arg is null");
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        int pos = byteBuffer.position();
        int lim = byteBuffer.limit();
        byteBuffer.limit(node.dataPos + 4*node.dataLen).position(node.pos);

        if (copy) {
            ByteBuffer newBuf = ByteBuffer.allocate(node.dataPos + 4*node.dataLen - node.pos).order(byteOrder);
            // Relative get and put changes position in both buffers
            newBuf.put(byteBuffer);
            newBuf.flip();
            byteBuffer.limit(lim).position(pos);
            return newBuf;
        }

        ByteBuffer buf = byteBuffer.slice().order(byteOrder);
        byteBuffer.limit(lim).position(pos);
        return buf;
    }


    /**
     * This method returns an unmodifiable list of all
     * evio structures in buffer as EvioNode objects.
     *
     * @throws EvioException if object closed
     * @return unmodifiable list of all evio structures in buffer as EvioNode objects.
     */
    public synchronized List<EvioNode> getNodes() throws EvioException {
        if (closed) {
            throw new EvioException("object closed");
        }
        return Collections.unmodifiableList(scanStructure());
    }


    /**
     * This method returns an unmodifiable list of all
     * evio structures in buffer as EvioNode objects.
     *
     * @throws EvioException if object closed
     * @return unmodifiable list of all evio structures in buffer as EvioNode objects.
     */
    public synchronized List<EvioNode> getChildNodes() throws EvioException {
        if (closed) {
            throw new EvioException("object closed");
        }
        scanStructure();
        return Collections.unmodifiableList(node.childNodes);
    }


	/**
	 * This only sets the position to its initial value.
	 */
    public synchronized void close() {
        byteBuffer.position(initialPosition);
        closed = true;
	}

}