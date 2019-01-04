package org.jlab.coda.jevio;

import java.io.*;
import java.nio.*;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * This class is used to read an evio version 4 formatted file or buffer
 * and extract specific evio containers (bank, seg, or tagseg)
 * with actual data in them given a tag/num pair. It is theoretically thread-safe.
 * It is designed to be fast and does <b>NOT</b> do a full deserialization
 * on each event examined.<p>
 *
 * @author timmer
 */
public class EvioCompactReader {

    /**
	 * This <code>enum</code> denotes the status of a read. <br>
	 * SUCCESS indicates a successful read. <br>
	 * END_OF_FILE indicates that we cannot read because an END_OF_FILE has occurred. Technically this means that what
	 * ever we are trying to read is larger than the buffer's unread bytes.<br>
	 * EVIO_EXCEPTION indicates that an EvioException was thrown during a read, possibly due to out of range values,
	 * such as a negative start position.<br>
	 * UNKNOWN_ERROR indicates that an unrecoverable error has occurred.
	 */
	public static enum ReadStatus {
		SUCCESS, END_OF_FILE, EVIO_EXCEPTION, UNKNOWN_ERROR
	}


    /**  Offset to get magic number from start of file. */
    private static final int MAGIC_OFFSET = 28;

    /** Offset to get version number from start of file. */
    private static final int VERSION_OFFSET = 20;

    /** Offset to get block size from start of block. */
    private static final int BLOCK_SIZE_OFFSET = 0;

    /** Offset to get block number from start of block. */
    private static final int BLOCK_NUMBER = 4;

    /** Offset to get block size from start of block. */
    private static final int BLOCK_HEADER_SIZE_OFFSET = 8;

    /** Offset to get block size from start of block. */
    private static final int BLOCK_EVENT_COUNT = 12;

    /** Offset to get block size from start of block. */
    private static final int BLOCK_RESERVED_1 = 16;

    /** Mask to get version number from 6th int in block. */
    private static final int VERSION_MASK = 0xff;

    /** Stores info of all the (top-level) events. */
    public final ArrayList<EvioNode> eventNodes = new ArrayList<EvioNode>(1000);

    /** Store info of all block headers. */
    private final HashMap<Integer, BlockNode> blockNodes = new HashMap<Integer, BlockNode>(20);


    /**
     * This is the number of events in the file. It is not computed unless asked for,
     * and if asked for it is computed and cached in this variable.
     */
    private int eventCount = -1;

    /** Evio version number (1-4). Obtain this by reading first block header. */
    private int evioVersion;

    /**
     * Endianness of the data being read, either
     * {@link java.nio.ByteOrder#BIG_ENDIAN} or
     * {@link java.nio.ByteOrder#LITTLE_ENDIAN}.
     */
    private ByteOrder byteOrder;

    /** When reading a file, this is the byte order of the evio data in the file. */
    private ByteOrder fileByteOrder;

    /**
     * This is the number of blocks in the file including the empty block at the
     * end of the version 4 files. It is not computed unless asked for,
     * and if asked for it is computed and cached in this variable.
     */
    private int blockCount = -1;

    /** Size of the first block header in 32-bit words. Used to read dictionary. */
    private int firstBlockHeaderWords;

    /** The current block header. */
    private BlockHeaderV4 blockHeader = new BlockHeaderV4();

    /** Are we reading a file or buffer? */
    private boolean isFile;

    /** Does the file/buffer have a dictionary? */
    private boolean hasDictionary;

    /**
     * Version 4 files may have an xml format dictionary in the
     * first event of the first block.
     */
    private String dictionaryXML;

    /** Dictionary object created from dictionaryXML string. */
    private EvioXMLDictionary dictionary;

    /** The buffer being read. */
    private ByteBuffer byteBuffer;

    /** Object containing buffer and its parsed information. */
    BufferNode bufferNode;

    /** Initial position of buffer (mappedByteBuffer if reading a file). */
    private int initialPosition;

    /** How much of the buffer being read is valid evio data (in 32bit words)?
     *  The valid data begins at initialPosition and ends after this length.*/
    private int validDataWords;

    /** Is this object currently closed? */
    private boolean closed;

    /** Skip things like keeping track of blocks for the sake of speed? */
    private boolean fast = true;


    //------------------------
    // File specific members
    //------------------------

    /**
     * The buffer representing a map of the input file which is also
     * accessed through {@link #byteBuffer}.
     */
    private MappedByteBuffer mappedByteBuffer;

    /** Absolute path of the underlying file. */
    private String path;

    /** File size in bytes. */
    private long fileSize;



    //------------------------
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
     * Constructor for reading an event file.
     *
     * @param path the full path to the file that contains events.
     *             For writing event files, use an <code>EventWriter</code> object.
     * @see org.jlab.coda.jevio.EventWriter
     * @throws java.io.IOException   if read failure
     * @throws org.jlab.coda.jevio.EvioException if file arg is null
     */
    public EvioCompactReader(String path) throws EvioException, IOException {
        this(new File(path));
    }



    /**
     * Constructor for reading an event file.
     *
     * @param file the file that contains events.
     *
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if file arg is null; file is too large;
     */
    public EvioCompactReader(File file) throws EvioException, IOException {
        if (file == null) {
            throw new EvioException("File arg is null");
        }

        FileInputStream fileInputStream = new FileInputStream(file);
        path = file.getAbsolutePath();
        FileChannel fileChannel = fileInputStream.getChannel();
        fileSize = fileChannel.size();

        // Is the file byte size > max int value?
        // If so we cannot use a memory mapped file.
        if (fileSize > Integer.MAX_VALUE) {
            throw new EvioException("file too large (must be < 2.1475GB)");
        }

        mapFile(fileChannel);
        fileChannel.close(); // this object is no longer needed since we have the map

        initialPosition = 0;

        // Read first block header and find the file's endianness & evio version #.
        // If there's a dictionary, read that too.
        if (readFirstHeader() != ReadStatus.SUCCESS) {
            throw new IOException("Failed reading first block header/dictionary");
        }

        // Generate a table of all event positions in buffer for random access.
        generateEventPositionTable();

        isFile = true;
    }


    /**
     * Constructor for reading a buffer.
     *
     * @param byteBuffer the buffer that contains events.

     * @see EventWriter
     * @throws EvioException if buffer arg is null;
     *                       failure to read first block header
     */
    public EvioCompactReader(ByteBuffer byteBuffer) throws EvioException {

        if (byteBuffer == null) {
            throw new EvioException("Buffer arg is null");
        }

        initialPosition = byteBuffer.position();
        this.byteBuffer = byteBuffer;

        // Read first block header and find the file's endianness & evio version #.
        // If there's a dictionary, read that too.
        if (readFirstHeader() != ReadStatus.SUCCESS) {
            throw new EvioException("Failed reading first block header/dictionary");
        }

        // Generate a table of all event positions in buffer for random access.
        generateEventPositionTable();
    }

    /**
     * This method can be used to avoid creating additional EvioCompactReader
     * objects by reusing this one with another buffer. The method
     * {@link #close()} is called before anything else.
     *
     * @param buf ByteBuffer to be read
     * @throws EvioException if arg is null;
     *                       if failure to read first block header
     */
    public void setBuffer(ByteBuffer buf) throws EvioException {
        setBuffer(buf, true);
    }

    /**
     * This method can be used to avoid creating additional EvioCompactReader
     * objects by reusing this one with another buffer. The method
     * {@link #close()} is called before anything else.
     *
     * @param buf ByteBuffer to be read
     * @param fast ignore things like tracking block info for the sake of speed
     * @throws EvioException if arg is null;
     *                       if failure to read first block header
     */
    synchronized void setBuffer(ByteBuffer buf, boolean fast) throws EvioException {

        if (buf == null) {
            throw new EvioException("arg is null");
        }

        close();

        this.fast = fast;

        if (!fast) {
            blockNodes.clear();
        }
        eventNodes.clear();

        blockCount      = -1;
        eventCount      = -1;
        dictionaryXML   = null;
        initialPosition = buf.position();
        this.byteBuffer = buf;

        if (readFirstHeader() != ReadStatus.SUCCESS) {
            throw new EvioException("Failed reading first block header/dictionary");
        }

        generateEventPositionTable();
        closed = false;
    }

    /**
     * Has {@link #close()} been called (without reopening by calling
     * {@link #setBuffer(ByteBuffer)})?
     *
     * @return {@code true} if this object closed, else {@code false}.
     */
    public synchronized boolean isClosed() {return closed;}

    /**
     * Get the byte order of the file/buffer being read.
     * @return byte order of the file/buffer being read.
     */
    public ByteOrder getByteOrder() {return byteOrder;}

    /**
     * Get the evio version number.
     * @return evio version number.
     */
    public int getEvioVersion() {
        return evioVersion;
    }

    /**
      * Get the path to the file.
      * @return path to the file
      */
     public String getPath() {
         return path;
     }

    /**
     * When reading a file, this method's return value
     * is the byte order of the evio data in the file.
     * @return byte order of the evio data in the file.
     */
    public ByteOrder getFileByteOrder() {
        return fileByteOrder;
    }

    /**
     * Get the XML format dictionary is there is one.
     * @throws EvioException if object closed and dictionary still unread
     * @return XML format dictionary, else null.
     */
    public synchronized String getDictionaryXML() throws EvioException {
        if (dictionaryXML != null) return dictionaryXML;

        if (closed) {
            throw new EvioException("object closed");
        }

        // Is there a dictionary? If so, read it now if we haven't already.
        if (hasDictionary) {
            try {
                readDictionary();
            }
            catch (Exception e) {}
        }

        return dictionaryXML;
    }

     /**
      * Get the evio dictionary if is there is one.
      * @throws EvioException if object closed and dictionary still unread
      * @return evio dictionary if exists, else null.
      */
     public synchronized EvioXMLDictionary getDictionary() throws EvioException {
         if (dictionary != null) return dictionary;

         if (closed) {
             throw new EvioException("object closed");
         }

         // Is there a dictionary? If so, read it now if we haven't already.
         if (hasDictionary) {
             try {
                 if (dictionaryXML == null) {
                     readDictionary();
                 }
                 dictionary = new EvioXMLDictionary(dictionaryXML);
             }
             catch (Exception e) {}
         }

         return dictionary;
     }

    /**
     * Does this evio file have an associated XML dictionary?
     *
     * @return <code>true</code> if this evio file has an associated XML dictionary,
     *         else <code>false</code>
     */
    public boolean hasDictionary() {
         return hasDictionary;
    }

	/**
	 * Maps the file into memory. The data are not actually loaded in memory-- subsequent reads will read
	 * random-access-like from the file.
	 *
     * @param inputChannel the input channel.
     * @throws IOException if file cannot be opened
	 */
	private void mapFile(FileChannel inputChannel) throws IOException {
		long sz = inputChannel.size();
		mappedByteBuffer = inputChannel.map(FileChannel.MapMode.READ_ONLY, 0L, sz);
        byteBuffer = mappedByteBuffer;
	}

    /**
     * Get the memory mapped buffer corresponding to the event file.
     * @return the memory mapped buffer corresponding to the event file.
     */
    public MappedByteBuffer getMappedByteBuffer() {
        return mappedByteBuffer;
    }

    /**
     * Get the byte buffer being read directly or corresponding to the event file.
     * @return the byte buffer being read directly or corresponding to the event file.
     */
    public ByteBuffer getByteBuffer() {
        return byteBuffer;
    }

    /**
     * Get the size of the file being read, in bytes.
     * For small files, obtain the file size using the memory mapped buffer's capacity.
     * @return the file size in bytes
     */
    public long fileSize() {
        return fileSize;
    }


    /**
     * Get the EvioNode object associated with a particular event number.
     * @param eventNumber number of event (place in file/buffer) starting at 1.
     * @return  EvioNode object associated with a particular event number,
     *          or null if there is none.
     */
    public EvioNode getEvent(int eventNumber) {
        try {
            return eventNodes.get(eventNumber - 1);
        }
        catch (IndexOutOfBoundsException e) { }
        return null;
    }


    /**
     * Get the EvioNode object associated with a particular event number
     * which has been scanned so all substructures are contained in the
     * node.allNodes list.
     * @param eventNumber number of event (place in file/buffer) starting at 1.
     * @return  EvioNode object associated with a particular event number,
     *
     *         or null if there is none.
     */
    public EvioNode getScannedEvent(int eventNumber) {
        try {
            return scanStructure(eventNumber);
        }
        catch (IndexOutOfBoundsException e) { }
        return null;
    }


    /**
     * Generate a table (ArrayList) of positions of events in file/buffer.
     * This method does <b>not</b> affect the byteBuffer position, eventNumber,
     * or lastBlock values. Uses only absolute gets so byteBuffer position
     * does not change. Called only in constructors and
     * {@link #setBuffer(java.nio.ByteBuffer)}.
     *
     * @throws EvioException if bytes not in evio format
     */
    private void generateEventPositionTable() throws EvioException {

        int      byteInfo, byteLen, blockHdrSize, blockSize, blockEventCount, magicNum;
        boolean  firstBlock=true, hasDictionary=false;
        //boolean  curLastBlock=false;

//        long t2, t1 = System.currentTimeMillis();

        bufferNode = new BufferNode(byteBuffer);

        // Start at the beginning of byteBuffer without changing
        // its current position. Do this with absolute gets.
        int position  = initialPosition;
        int bytesLeft = byteBuffer.limit() - position;

        // Keep track of the # of blocks, events, and valid words in file/buffer
        blockCount = 0;
        eventCount = 0;
        validDataWords = 0;
        BlockNode blockNode=null, previousBlockNode=null;

        try {

            while (bytesLeft > 0) {
                // Need enough data to at least read 1 block header (32 bytes)
                if (bytesLeft < 32) {
                    throw new EvioException("Bad evio format: extra " + bytesLeft +
                                                    " bytes at file end");
                }

                // Swapping is taken care of
                blockSize       = byteBuffer.getInt(position);
                byteInfo        = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_VERSION);
                blockHdrSize    = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_HEADERSIZE);
                blockEventCount = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_COUNT);
                magicNum        = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_MAGIC);

//                System.out.println("    genEvTablePos: blk ev count = " + blockEventCount +
//                                           ", blockSize = " + blockSize +
//                                           ", blockHdrSize = " + blockHdrSize +
//                                           ", byteInfo  = " + byteInfo);

                // If magic # is not right, file is not in proper format
                if (magicNum != BlockHeaderV4.MAGIC_NUMBER) {
                    throw new EvioException("Bad evio format: block header magic # incorrect");
                }

                if (blockSize < 8 || blockHdrSize < 8) {
                    throw new EvioException("Bad evio format (block: len = " +
                                                    blockSize + ", blk header len = " + blockHdrSize + ")" );
                }

                // Check to see if the whole block is there
                if (4*blockSize > bytesLeft) {
//System.out.println("    4*blockSize = " + (4*blockSize) + " >? bytesLeft = " + bytesLeft +
//                   ", pos = " + position);
                    throw new EvioException("Bad evio format: not enough data to read block");
                }

                if (!fast) {
                    // File is now positioned before block header.
                    // Look at block header to get info.
                    blockNode = new BlockNode();

                    blockNode.pos = position;
                    blockNode.len   = blockSize;
                    blockNode.count = blockEventCount;

                    blockNodes.put(blockCount, blockNode);
                    bufferNode.blockNodes.add(blockNode);

                    blockNode.place = blockCount++;

                    // Make linked list of blocks
                    if (previousBlockNode != null) {
                        previousBlockNode.nextBlock = blockNode;
                    }
                    else {
                        previousBlockNode = blockNode;
                    }
                }

                validDataWords += blockSize;
//                curLastBlock    = BlockHeaderV4.isLastBlock(byteInfo);
                if (firstBlock) hasDictionary = BlockHeaderV4.hasDictionary(byteInfo);

                // Hop over block header to events
                position  += 4*blockHdrSize;
                bytesLeft -= 4*blockHdrSize;

//System.out.println("    hopped blk hdr, pos = " + position + ", is last blk = " + curLastBlock);

                // Check for a dictionary - the first event in the first block.
                // It's not included in the header block count, but we must take
                // it into account by skipping over it.
                if (firstBlock && hasDictionary) {
                    firstBlock = false;

                    // Get its length - bank's len does not include itself
                    byteLen = 4*(byteBuffer.getInt(position) + 1);

                    // Skip over dictionary
                    position  += byteLen;
                    bytesLeft -= byteLen;
//System.out.println("    hopped dict, pos = " + position);
                }

                // For each event in block, store its location
                for (int i=0; i < blockEventCount; i++) {
                    // Sanity check - must have at least 1 header's amount left
                    if (bytesLeft < 8) {
                        throw new EvioException("Bad evio format: not enough data to read event (bad bank len?)");
                    }

                    EvioNode node = extractEventNode(bufferNode, blockNode,
                                                     position, eventCount + i);
//System.out.println("      event "+i+" in block: pos = " + node.pos +
//                           ", dataPos = " + node.dataPos + ", ev # = " + (eventCount + i + 1));
                    eventNodes.add(node);
                    if (!fast) blockNode.allEventNodes.add(node);

                    // Hop over header + data
                    byteLen = 8 + 4*node.dataLen;
                    position  += byteLen;
                    bytesLeft -= byteLen;

                    if (byteLen < 8 || bytesLeft < 0) {
                        throw new EvioException("Bad evio format: bad bank length");
                    }

//System.out.println("    hopped event " + (i+1) + ", bytesLeft = " + bytesLeft + ", pos = " + position + "\n");
                }

                eventCount += blockEventCount;
            }
        }
        catch (IndexOutOfBoundsException e) {
            bufferNode = null;
            throw new EvioException("Bad evio format", e);
        }

//        t2 = System.currentTimeMillis();
//        System.out.println("Time to scan file = " + (t2-t1) + " milliseconds");
    }


    /**
     * Reads the first block (physical record) header in order to determine
     * characteristics of the file or buffer in question. These things
     * do <b>not</b> need to be examined in subsequent block headers.
     * File or buffer must be evio version 4 or greater.
     * Called only in constructors and setBuffer.
     *
     * @return status of read attempt
     */
    private ReadStatus readFirstHeader() {
        // Get first block header
        int pos = initialPosition;

        // Have enough remaining bytes to read header?
        if (byteBuffer.limit() - pos < 32) {
            byteBuffer.clear();
            return ReadStatus.END_OF_FILE;
        }

        try {
            // Set the byte order to match the file's ordering.

            // Check the magic number for endianness (buffer defaults to big endian)
            byteOrder = byteBuffer.order();

            int magicNumber = byteBuffer.getInt(pos + MAGIC_OFFSET);

            if (magicNumber != IBlockHeader.MAGIC_NUMBER) {

                if (byteOrder == ByteOrder.BIG_ENDIAN) {
                    byteOrder = ByteOrder.LITTLE_ENDIAN;
                }
                else {
                    byteOrder = ByteOrder.BIG_ENDIAN;
                }
//System.out.println("EvioCompactReader: switch endianness to " + byteOrder);
                byteBuffer.order(byteOrder);

                // Reread magic number to make sure things are OK
                magicNumber = byteBuffer.getInt(pos + MAGIC_OFFSET);
                if (magicNumber != IBlockHeader.MAGIC_NUMBER) {
System.out.println("ERROR reread magic # (" + magicNumber + ") & still not right");
                    return ReadStatus.EVIO_EXCEPTION;
                }
            }

            fileByteOrder = byteOrder;

            // Check the version number
            int bitInfo = byteBuffer.getInt(pos + VERSION_OFFSET);
            evioVersion = bitInfo & VERSION_MASK;
            if (evioVersion < 4)  {
System.out.println("EvioCompactReader: unsupported evio version (" + evioVersion + ")");
                return ReadStatus.EVIO_EXCEPTION;
            }

            // Does this file/buffer have a dictionary?
            hasDictionary = BlockHeaderV4.bitInfoHasDictionary(bitInfo);

            // # of words in first block header
            firstBlockHeaderWords = byteBuffer.getInt(pos + BLOCK_HEADER_SIZE_OFFSET);

            // Store first header data
            blockHeader.setSize(byteBuffer.getInt(pos + BLOCK_SIZE_OFFSET));
            blockHeader.setNumber(byteBuffer.getInt(pos + BLOCK_NUMBER));
            blockHeader.setHeaderLength(firstBlockHeaderWords);
            blockHeader.setEventCount(byteBuffer.getInt(pos + BLOCK_EVENT_COUNT));
            blockHeader.setReserved1(byteBuffer.getInt(pos + BLOCK_RESERVED_1));   // used from ROC

            // Use 6th word to set bit info & version
            blockHeader.parseToBitInfo(bitInfo);
            blockHeader.setVersion(evioVersion);
            blockHeader.setReserved2(0);  // not used
            blockHeader.setMagicNumber(magicNumber);
            blockHeader.setByteOrder(byteOrder);
        }
        catch (EvioException a) {
            byteBuffer.clear();
            return ReadStatus.EVIO_EXCEPTION;
        }
        catch (BufferUnderflowException a) {
            System.err.println("ERROR endOfBuffer " + a);
            byteBuffer.clear();
            return ReadStatus.UNKNOWN_ERROR;
        }

        return ReadStatus.SUCCESS;
    }


    /**
     * This returns the FIRST block (physical record) header.
     *
     * @return the first block header.
     */
    public BlockHeaderV4 getFirstBlockHeader() {
        return blockHeader;
    }


    /**
     * This method is only called if the user wants to look at the dictionary.
     * This method is synchronized because of its use of a bulk, relative get method.
     *
     * @throws EvioException if failed read due to bad buffer format
     */
     private void readDictionary() throws EvioException {

         // Where are we?
         int originalPos = byteBuffer.position();
         int pos = initialPosition + 4*firstBlockHeaderWords;

         // How big is the event?
         int length;
         length = byteBuffer.getInt(pos);
         if (length < 1) {
             throw new EvioException("Bad value for dictionary length");
         }
         pos += 4;

         // Since we're only interested in length, read but ignore rest of the header.
         byteBuffer.getInt(pos);
         pos += 4;

         // Get the raw data
         int eventDataSizeBytes = 4*(length - 1);
         byte bytes[] = new byte[eventDataSizeBytes];

         // Read in dictionary data (using a relative get)
         try {
             byteBuffer.position(pos);
             byteBuffer.get(bytes, 0, eventDataSizeBytes);
         }
         catch (Exception e) {
             throw new EvioException("Problems reading buffer");
         }

         // Unpack array into dictionary
         String[] strs = BaseStructure.unpackRawBytesToStrings(bytes, 0);
         if (strs == null) {
             throw new EvioException("Data in bad format");
         }
         dictionaryXML = strs[0];

         byteBuffer.position(originalPos);
     }


    /**
     * This method extracts an EvioNode object representing an
     * evio event (top level evio bank) from a given buffer, a
     * location in the buffer, and a few other things. An EvioNode
     * object represents an evio container - either a bank, segment,
     * or tag segment.
     *
     * @param bufferNode buffer to examine
     * @param blockNode  object holding data about block header
     * @param position   position in buffer
     * @param place      place of event in buffer (starting at 0)
     *
     * @return EvioNode object containing evio event information
     * @throws EvioException if file/buffer too small
     */
    static private EvioNode extractEventNode(BufferNode bufferNode, BlockNode blockNode,
                                             int position, int place)
            throws EvioException {

        // Make sure there is enough data to at least read evio header
        ByteBuffer buffer = bufferNode.buffer;
        if (buffer.remaining() < 8) {
            throw new EvioException("buffer underflow");
        }

        // Store evio event info, without de-serializing, into EvioNode object
        EvioNode node = new EvioNode(position, place, bufferNode, blockNode);

        // Get length of current event
        node.len = buffer.getInt(position);
        // Position of data for a bank
        node.dataPos = position + 8;
        // Len of data for a bank
        node.dataLen = node.len - 1;

        // Make sure there is enough data to read full event
        // even though it is NOT completely read at this time.
        if (buffer.remaining() < 4*(node.len + 1)) {
//System.out.println("ERROR: remaining = " + buffer.remaining() +
//            ", node len bytes = " + ( 4*(node.len + 1)));
            throw new EvioException("buffer underflow");
        }

        // Hop over length word
        position += 4;

        // Read second header word
        if (buffer.order() == ByteOrder.BIG_ENDIAN) {
            node.tag = buffer.getShort(position) & 0xffff;
            position += 2;

            int dt = buffer.get(position++) & 0xff;
            node.dataType = dt & 0x3f;
            node.pad  = dt >>> 6;
            // If only 7th bit set, that can only be the legacy tagsegment type
            // with no padding information - convert it properly.
            if (dt == 0x40) {
                node.dataType = DataType.TAGSEGMENT.getValue();
                node.pad = 0;
            }

            node.num = buffer.get(position) & 0xff;
        }
        else {
            node.num = buffer.get(position++) & 0xff;

            int dt = buffer.get(position++) & 0xff;
            node.dataType = dt & 0x3f;
            node.pad  = dt >>> 6;
            if (dt == 0x40) {
                node.dataType = DataType.TAGSEGMENT.getValue();
                node.pad = 0;
            }

            node.tag = buffer.getShort(position) & 0xffff;
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
            return;
        }
 //System.out.println("scanStructure: scanning evio struct with len = " + node.dataLen);
 //System.out.println("scanStructure: data type of node to be scanned = " + type);

        // Start at beginning position of evio structure being scanned
        int position = node.dataPos;
        // Don't go past the data's end which is (position + length)
        // of evio structure being scanned in bytes.
        int endingPos = position + 4*node.dataLen;
        // Buffer we're using
        ByteBuffer buffer = node.bufferNode.buffer;
//System.out.println("scanStructure: pos = " + position + ", ending pos = " + endingPos +
//", lim = " + buffer.limit() + ", cap = " + buffer.capacity());

        int dt, dataType, dataLen, len, pad, tag, num;

        // Do something different depending on what node contains
        switch (type) {
            case BANK:
            case ALSOBANK:

                // Extract all the banks from this bank of banks.
                // Make allowance for reading header (2 ints).
                while (position <= endingPos - 8) {
//System.out.println("scanStructure: buf is at pos " + buffer.position() +
//                   ", limit =  " + buffer.limit() + ", remaining = " + buffer.remaining() +
//                   ", capacity = " + buffer.capacity());

                    // Read first header word
                    len = buffer.getInt(position);
                    // Len of data (no header) for a bank
                    dataLen = len - 1;
                    position += 4;

                    // Read & parse second header word
                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        tag = buffer.getShort(position) & 0xffff;
                        position += 2;

                        dt = buffer.get(position++) & 0xff;
                        dataType = dt & 0x3f;
                        pad  = dt >>> 6;
                        // If only 7th bit set, that can only be the legacy tagsegment type
                        // with no padding information - convert it properly.
                        if (dt == 0x40) {
                            dataType = DataType.TAGSEGMENT.getValue();
                            pad = 0;
                        }

                        num = buffer.get(position++) & 0xff;
                    }
                    else {
                        num = buffer.get(position++) & 0xff;

                        dt = buffer.get(position++) & 0xff;
                        dataType = dt & 0x3f;
                        pad  = dt >>> 6;
                        if (dt == 0x40) {
                            dataType = DataType.TAGSEGMENT.getValue();
                            pad = 0;
                        }

                        tag = buffer.getShort(position) & 0xffff;
                        position += 2;
                    }

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
                // Make allowance for reading header (1 int).
                while (position <= endingPos - 4) {

                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        tag = buffer.get(position++) & 0xff;
                        dt = buffer.get(position++) & 0xff;
                        dataType = dt & 0x3f;
                        pad = dt >>> 6;
                        if (dt == 0x40) {
                            dataType = DataType.TAGSEGMENT.getValue();
                            pad = 0;
                        }
                        len = buffer.getShort(position) & 0xffff;
                        position += 2;
                    }
                    else {
                        len = buffer.getShort(position) & 0xffff;
                        position += 2;
                        dt = buffer.get(position++) & 0xff;
                        dataType = dt & 0x3f;
                        pad = dt >>> 6;
                        if (dt == 0x40) {
                            dataType = DataType.TAGSEGMENT.getValue();
                            pad = 0;
                        }
                        tag = buffer.get(position++) & 0xff;
                    }

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
                // Make allowance for reading header (1 int).
                while (position <= endingPos - 4) {

                    if (buffer.order() == ByteOrder.BIG_ENDIAN) {
                        int temp = buffer.getShort(position) & 0xffff;
                        position += 2;
                        tag = temp >>> 4;
                        dataType = temp & 0xf;
                        len = buffer.getShort(position) & 0xffff;
                        position += 2;
                    }
                    else {
                        len = buffer.getShort(position) & 0xffff;
                        position += 2;
                        int temp = buffer.getShort(position) & 0xffff;
                        position += 2;
                        tag = temp >>> 4;
                        dataType = temp & 0xf;
                    }

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
     * This method scans the given event number in the buffer.
     * It returns an EvioNode object representing the event.
     * All the event's substructures, as EvioNode objects, are
     * contained in the node.allNodes list (including the event itself).
     *
     * @param eventNumber number of the event to be scanned starting at 1
     * @return the EvioNode object corresponding to the given event number
     */
    private EvioNode scanStructure(int eventNumber) {

        // Node corresponding to event
        EvioNode node = eventNodes.get(eventNumber - 1);

        if (node.scanned) {
            node.clearLists();
        }

        // Do this before actual scan so clone() sets all "scanned" fields
        // of child nodes to "true" as well.
        node.scanned = true;

        scanStructure(node);

        return node;
    }


    /**
     * This method searches the specified event in a file/buffer and
     * returns a list of objects each of which contain information
     * about a single evio structure which matches the given tag and num.
     *
     * @param eventNumber place of event in buffer (starting with 1)
     * @param tag tag to match
     * @param num num to match
     * @return list of EvioNode objects corresponding to matching evio structures
     *         (empty if none found)
     * @throws EvioException if bad arg value(s);
     *                       if object closed
     */
    public synchronized List<EvioNode> searchEvent(int eventNumber, int tag, int num)
                                 throws EvioException {

        // check args
        if (tag < 0 || num < 0 || eventNumber < 1 || eventNumber > eventCount) {
            throw new EvioException("bad arg value(s)");
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        ArrayList<EvioNode> returnList = new ArrayList<EvioNode>(100);

        // Scan the node
        ArrayList<EvioNode> list = scanStructure(eventNumber).allNodes;
//System.out.println("searchEvent: ev# = " + eventNumber + ", list size = " + list.size() +
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
     * This method searches the specified event in a file/buffer and
     * returns a list of objects each of which contain information
     * about a single evio structure which matches the given dictionary
     * entry name.
     *
     * @param  eventNumber place of event in buffer (starting with 1)
     * @param  dictName name of dictionary entry to search for
     * @param  dictionary dictionary to use; if null, use dictionary with file/buffer
     *
     * @return list of EvioNode objects corresponding to matching evio structures
     *         (empty if none found)
     * @throws EvioException if dictName is null;
     *                       if dictName is an invalid dictionary entry;
     *                       if dictionary is null and none provided in file/buffer being read;
     *                       if object closed
     */
    public synchronized List<EvioNode> searchEvent(int eventNumber, String dictName,
                                      EvioXMLDictionary dictionary)
                                 throws EvioException {

        if (dictName == null) {
            throw new EvioException("null dictionary entry name");
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        // If no dictionary is specified, use the one provided with the
        // file/buffer. If that does not exist, throw an exception.
        int tag, num;

        if (dictionary == null && hasDictionary)  {
            dictionary = getDictionary();
        }

        if (dictionary != null) {
            tag = dictionary.getTag(dictName);
            num = dictionary.getNum(dictName);
            if (tag == -1 || num == -1) {
                throw new EvioException("no dictionary entry for " + dictName);
            }
        }
        else {
            throw new EvioException("no dictionary available");
        }

        return searchEvent(eventNumber, tag, num);
    }


    /**
     * This method adds a bank, segment, or tag segment onto the end of an
     * event. It is the responsibility of the caller to make sure that
     * the buffer argument contains valid evio data (only data representing
     * the bank or structure to be added - not in file format with bank
     * header and the like) which is compatible with the type of data
     * structure stored in the given event. To produce such an evio data
     * use {@link EvioBank#write(java.nio.ByteBuffer)},
     * {@link EvioSegment#write(java.nio.ByteBuffer)} or
     * {@link EvioTagSegment#write(java.nio.ByteBuffer)} depending on whether
     * a bank, seg, or tagseg is being added.<p>
     * A note about files here. If the constructor of this reader read in data
     * from a file, it will now switch to using a new, internal buffer which
     * is returned by this method or can be retrieved by calling
     * {@link #getByteBuffer()}. It will <b>not</b> expand the file originally used.
     * A new file can be created by calling either the {@link #toFile(String)} or
     * {@link #toFile(java.io.File)} methods.<p>
     * The given buffer argument must be ready to read with its position and limit
     * defining the limits of the data to copy.
     * This method is synchronized due to the bulk, relative puts.
     *
     * @param eventNumber number of event to which addBuffer is to be added
     * @param addBuffer buffer containing evio data to add (<b>not</b> evio file format,
     *                  i.e. no bank headers)
     * @return a new ByteBuffer object which is created and filled with all the data
     *         including what was just added.
     * @throws EvioException if trying to add to a file;
     *                       if eventNumber < 1;
     *                       if addBuffer is null;
     *                       if addBuffer arg is empty or has non-evio format;
     *                       if addBuffer is opposite endian to current event buffer;
     *                       if added data is not the proper length (i.e. multiple of 4 bytes);
     *                       if the event number does not correspond to an existing event;
     *                       if there is an internal programming error;
     *                       if object closed
     */
    public synchronized ByteBuffer addStructure(int eventNumber, ByteBuffer addBuffer) throws EvioException {

        // If we're adding nothing, then DO nothing
//System.out.println("addStructure: addBuffer = " + addBuffer);
//if (addBuffer != null) System.out.println("   remaining = " + addBuffer.remaining());

        if (addBuffer == null || addBuffer.remaining() < 8) {
            throw new EvioException("null, empty, or non-evio format buffer arg");
        }

        if (addBuffer.order() != byteOrder) {
            throw new EvioException("trying to add wrong endian buffer");
        }

        if (eventNumber < 1) {
            throw new EvioException("event number must be > 0");
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        EvioNode eventNode;
        try {
            eventNode = eventNodes.get(eventNumber - 1);
        }
        catch (IndexOutOfBoundsException e) {
            throw new EvioException("event " + eventNumber + " does not exist", e);
        }

        // Position in byteBuffer just past end of event
        int endPos = eventNode.dataPos + 4*eventNode.dataLen;

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
        DataType eventDataType = DataType.getDataType(eventNode.dataType);

//System.out.println("APPENDING " + appendDataWordLen + " words to buf " + byteBuffer);
        //--------------------------------------------
        // Add new structure to end of specified event
        //--------------------------------------------

        // Create a new buffer
        ByteBuffer newBuffer = ByteBuffer.allocate(4*validDataWords + appendDataLen);
        newBuffer.order(byteOrder);

        // Copy beginning part of existing buffer into new buffer
        byteBuffer.limit(endPos).position(initialPosition);
        newBuffer.put(byteBuffer);

        // Copy new structure into new buffer
        newBuffer.put(addBuffer);

        // Copy ending part of existing buffer into new buffer
        byteBuffer.limit(4*validDataWords + initialPosition).position(endPos);
        newBuffer.put(byteBuffer);

        // Get new buffer ready for reading
        newBuffer.flip();

        // Restore original positions of buffers
        byteBuffer.position(initialPosition);
        addBuffer.position(origAddBufPos);

        //-------------------------------------
        // By inserting a structure, we've definitely changed the positions of all
        // structures that follow. Everything downstream gets shifted by appendDataLen
        // bytes.
        // And, if initialPosition was not 0 (in the new buffer it always is),
        // then ALL nodes need their position members shifted by initialPosition
        // bytes upstream.
        //-------------------------------------
        EvioNode eNode;

        for (int i=0; i < eventCount; i++) {

            eNode = eventNodes.get(i);

            // Deal with the event node unless we did it already above
//System.out.println("Event node " + (i+1) + ", pos = " + eNode.pos + ", dataPos = " + eNode.dataPos);
            eNode.pos     -= initialPosition;
            eNode.dataPos -= initialPosition;
            if (i > eventNode.place) {
                eNode.pos     += appendDataLen;
                eNode.dataPos += appendDataLen;
//System.out.println("      pos -> " + eNode.pos + ", dataPos -> " + eNode.dataPos);
            }
        }

        BlockNode bNode;
        for (int i=0; i < blockCount; i++) {
            bNode = blockNodes.get(i);
//System.out.println("Modify block " + (i+1) + ", pos = " + bNode.pos + ", initPos = " + initialPosition);
            bNode.pos -= initialPosition;
            if (i > eventNode.blockNode.place) {
                bNode.pos += appendDataLen;
//System.out.println("      pos -> " + bNode.pos);
            }
        }

        // At this point all EvioNode objects (including those in
        // user's possession) should be updated.

        // This reader object is NOW using the new buffer
        byteBuffer      = newBuffer;
        initialPosition = newBuffer.position();

        // If we started out by reading a file, now we are using the new buffer.
        if (isFile) {
            isFile = false;
            mappedByteBuffer = null;
        }

        // Position in new byteBuffer of block header
        // (and therefore the length word)
        int blockLenPos = eventNode.blockNode.pos;

        // Position in new byteBuffer of event
        int eventLenPos = eventNode.pos;

        //--------------------------------------------
        // Adjust event and block header sizes in both
        // block/node objects and in new buffer.
        //--------------------------------------------

        // Increase block size
//System.out.println("block object len = " +  eventNode.blockNode.len +
//                   ", set to " + (eventNode.blockNode.len + appendDataWordLen));

        eventNode.blockNode.len += appendDataWordLen;
        newBuffer.putInt(blockLenPos, eventNode.blockNode.len);

        // Increase event size
        eventNode.len     += appendDataWordLen;
        eventNode.dataLen += appendDataWordLen;

        switch (eventDataType) {
            case BANK:
            case ALSOBANK:
//System.out.println("event pos = " + eventLenPos + ", len = " + (eventNode.len - appendDataWordLen) +
//                   ", set to " + (eventNode.len));

                newBuffer.putInt(eventLenPos, eventNode.len);
                break;

            case SEGMENT:
            case ALSOSEGMENT:
            case TAGSEGMENT:
//System.out.println("event SEG/TAGSEG pos = " + eventLenPos + ", len = " + (eventNode.len - appendDataWordLen) +
//                   ", set to " + (eventNode.len));
                if (byteOrder == ByteOrder.BIG_ENDIAN) {
                    newBuffer.putShort(eventLenPos+2, (short)(eventNode.len));
                }
                else {
                    newBuffer.putShort(eventLenPos,   (short)(eventNode.len));
                }
                break;

            default:
                throw new EvioException("internal programming error");
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
     *             view into this reader object's buffer.
     * @throws EvioException if object closed
     * @return ByteBuffer object containing data. Position and limit are
     *         set for reading.
     */
    public synchronized ByteBuffer getData(EvioNode node, boolean copy)
                            throws EvioException {
        return node.getByteData(copy);
    }


    /**
     * Get an evio bank or event in ByteBuffer form.
     * The returned buffer is a view into the data of this reader's buffer.<p>
     * This method is synchronized due to the bulk, relative gets & puts.
     *
     * @param eventNumber number of event of interest
     * @return ByteBuffer object containing bank's/event's bytes. Position and limit are
     *         set for reading.
     * @throws EvioException if eventNumber < 1;
     *                       if the event number does not correspond to an existing event;
     *                       if object closed
     */
    public ByteBuffer getEventBuffer(int eventNumber) throws EvioException {
        return getEventBuffer(eventNumber, false);
    }


    /**
     * Get an evio bank or event in ByteBuffer form.
     * Depending on the copy argument, the returned buffer will either be
     * a copy of or a view into the data of this reader's buffer.<p>
     * This method is synchronized due to the bulk, relative gets & puts.
     *
     * @param eventNumber number of event of interest
     * @param copy if <code>true</code>, then return a copy as opposed to a
     *        view into this reader object's buffer.
     * @return ByteBuffer object containing bank's/event's bytes. Position and limit are
     *         set for reading.
     * @throws EvioException if eventNumber < 1;
     *                       if the event number does not correspond to an existing event;
     *                       if object closed
     */
    public synchronized ByteBuffer getEventBuffer(int eventNumber, boolean copy)
            throws EvioException {

        if (eventNumber < 1) {
            throw new EvioException("event number must be > 0");
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        EvioNode node;
        try {
            node = eventNodes.get(eventNumber - 1);
        }
        catch (IndexOutOfBoundsException e) {
            throw new EvioException("event " + eventNumber + " does not exist");
        }

        return node.getStructureBuffer(copy);
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
     *                       if object closed
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

        return node.getStructureBuffer(copy);
    }



	/**
	 * This only sets the position to its initial value.
	 */
    public synchronized void close() {
        byteBuffer.position(initialPosition);
        closed = true;
	}


    /**
     * This is the number of events in the file. Any dictionary event is <b>not</b>
     * included in the count. In versions 3 and earlier, it is not computed unless
     * asked for, and if asked for it is computed and cached.
     *
     * @return the number of events in the file.
     */
    public int getEventCount() {
        return eventCount;
    }

    /**
     * This is the number of blocks in the file including the empty
     * block usually at the end of version 4 files/buffers.
     * For version 3 files, a block size read from the first block is used
     * to calculate the result.
     * It is not computed unless in random access mode or is
     * asked for, and if asked for it is computed and cached.
     *
     * @return the number of blocks in the file (estimate for version 3 files)
     */
    public int getBlockCount() {
        return blockCount;
    }

    /**
     * Save the internal byte buffer to the given file
     * (overwrites existing file).
     *
     * @param fileName
     * @throws IOException
     * @throws EvioException if fileName arg is null;
     *                       if object closed
     */
    public void toFile(String fileName) throws EvioException, IOException {
        if (fileName == null) {
            throw new EvioException("null fileName arg");
        }
        File f = new File(fileName);
        toFile(f);
    }

    /**
     * Save the internal byte buffer to the given file
     * (overwrites existing file).
     *
     * @param file
     * @throws EvioException if file arg is null;
     *                       if object closed
     * @throws IOException
     */
    public synchronized void toFile(File file) throws EvioException, IOException {
        if (file == null) {
            throw new EvioException("null file arg");
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        // Remember where we were
        int pos = byteBuffer.position();

        // Write the file
        FileOutputStream fos = new FileOutputStream(file);
        FileChannel channel = fos.getChannel();
        channel.write(byteBuffer);
        channel.close();

        // Go back to where we were
        byteBuffer.position(pos);
    }

}