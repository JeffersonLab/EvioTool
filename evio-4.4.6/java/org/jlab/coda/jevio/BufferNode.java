package org.jlab.coda.jevio;

import java.nio.ByteBuffer;
import java.util.ArrayList;

/**
 * This class is used to store relevant info about an
 * evio version 4 format buffer.
 *
 * @author timmer
 * Date: 2/5/14
 */
public final class BufferNode {

    /** ByteBuffer associated with this node. */
    ByteBuffer buffer;

    /** Blocks contained in the buffer. */
    ArrayList<BlockNode> blockNodes;

    /**
     * If top-level event node, was I scanned and all my blocks
     * already placed into lists?
     */
    boolean scanned;


    //----------------------------------
    // Constructor (package accessible)
    //----------------------------------

    /**
     * Constructor which stores buffer and creates list of block nodes.
     * @param buffer buffer with evio version 4 file-format data.
     */
    BufferNode(ByteBuffer buffer) {
        this.buffer = buffer;
        blockNodes = new ArrayList<BlockNode>(1000);
    }

    //-------------------------------
    // Methods
    //-------------------------------

    public void clearLists() {
        blockNodes.clear();
    }

    //-------------------------------
    // Getters
    //-------------------------------

    /**
     * Get the byte buffer.
     * @return byte buffer.
     */
    public ByteBuffer getBuffer() {
        return buffer;
    }


}
