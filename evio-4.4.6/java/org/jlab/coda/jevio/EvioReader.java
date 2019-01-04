package org.jlab.coda.jevio;

import javax.xml.stream.FactoryConfigurationError;
import javax.xml.stream.XMLOutputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamWriter;
import java.io.*;
import java.nio.*;
import java.nio.channels.FileChannel;

/**
 * This is a class of interest to the user. It is used to read an evio version 4 or earlier
 * format file or buffer. Create an <code>EvioReader</code> object corresponding to an event
 * file or file-formatted buffer, and from this class you can test it
 * for consistency and, more importantly, you can call {@link #parseNextEvent} or
 * {@link #parseEvent(int)} to get new events and to stream the embedded structures
 * to an IEvioListener.<p>
 *
 * The streaming effect of parsing an event is that the parser will read the event and hand off structures,
 * such as banks, to any IEvioListeners. For those familiar with XML, the event is processed SAX-like.
 * It is up to the listener to decide what to do with the structures.
 * <p>
 *
 * As an alternative to stream processing, after an event is parsed, the user can use the events treeModel
 * for access to the structures. For those familiar with XML, the event is processed DOM-like.
 * <p>
 *
 * @author heddle
 * @author timmer
 *
 */
public class EvioReader {

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

	/**
	 * This <code>enum</code> denotes the status of a write.<br>
	 * SUCCESS indicates a successful write. <br>
	 * CANNOT_OPEN_FILE indicates that we cannot write because the destination file cannot be opened.<br>
	 * EVIO_EXCEPTION indicates that an EvioException was thrown during a write.<br>
	 * UNKNOWN_ERROR indicates that an unrecoverable error has occurred.
	 */
	public static enum WriteStatus {
		SUCCESS, CANNOT_OPEN_FILE, EVIO_EXCEPTION, UNKNOWN_ERROR
	}

    /**  Offset to get magic number from start of file. */
    private static final int MAGIC_OFFSET = 28;

    /** Offset to get version number from start of file. */
    private static final int VERSION_OFFSET = 20;

    /** Offset to get block size from start of block. */
    private static final int BLOCK_SIZE_OFFSET = 0;

    /** Mask to get version number from 6th int in block. */
    private static final int VERSION_MASK = 0xff;

    /** Root element tag for XML file */
    private static final String ROOT_ELEMENT = "evio-data";




    /** When doing a sequential read, used to assign a transient
     * number [1..n] to events as they are being read. */
    private int eventNumber = 0;

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

    /** Size of the first block in bytes. */
    private int firstBlockSize;

    /**
     * This is the number of blocks in the file including the empty block at the
     * end of the version 4 files. It is not computed unless asked for,
     * and if asked for it is computed and cached in this variable.
     */
    private int blockCount = -1;

	/** The current block header for evio versions 1-3. */
    private BlockHeaderV2 blockHeader2 = new BlockHeaderV2();

    /** The current block header for evio version 4. */
    private BlockHeaderV4 blockHeader4 = new BlockHeaderV4();

    /** Reference to current block header, any version, through interface. */
    private IBlockHeader blockHeader;

    /** Reference to first block header if version 4. */
    private IBlockHeader firstBlockHeader;

    /** Block number expected when reading. Used to check sequence of blocks. */
    private int blockNumberExpected = 1;

    /** If true, throw an exception if block numbers are out of sequence. */
    private boolean checkBlockNumberSequence;

    /** Is this the last block in the file or buffer? */
    private boolean lastBlock;

    /**
     * Version 4 files may have an xml format dictionary in the
     * first event of the first block.
     */
    private String dictionaryXML;

    /** The buffer being read. */
    private ByteBuffer byteBuffer;

    /** Parser object for this file/buffer. */
    private EventParser parser;

    /** Initial position of buffer or mappedByteBuffer when reading a file. */
    private int initialPosition;

    //------------------------
    // File specific members
    //------------------------

    /** Use this object to handle files > 2.1 GBytes but still use memory mapping. */
    private MappedMemoryHandler mappedMemoryHandler;

    /** The buffer containing an entire block when in sequential reading mode. */
    private ByteBuffer blockBuffer;

    /** Absolute path of the underlying file. */
    private String path;

    /** File object. */
    private File file;

    /** File size in bytes. */
    private long fileSize;

    /** File channel used to read data and access file position. */
    private FileChannel fileChannel;

    /** Data stream used to read data. */
    private DataInputStream dataStream;

    /** Do we need to swap data from file? */
    private boolean swap;

    /**
     * Read this file sequentially and not using a memory mapped buffer.
     * If the file being read > 2.1 GBytes, then this is always true.
     */
    private boolean sequentialRead;


    //------------------------
    // EvioReader's state
    //------------------------

    /** Is this object currently closed? */
    private boolean closed;

    /**
     * This class stores the state of this reader so it can be recovered
     * after a state-changing method has been called -- like {@link #rewind()}.
     */
    private class ReaderState {
        private boolean lastBlock;
        private int eventNumber;
        private long filePosition;
        private int byteBufferLimit;
        private int byteBufferPosition;
        private int blockNumberExpected;
        private BlockHeaderV2 blockHeader2;
        private BlockHeaderV4 blockHeader4;
    }


    /**
     * This method saves the current state of this EvioReader object.
     * @return the current state of this EvioReader object.
     */
    private ReaderState getState() {
        ReaderState currentState = new ReaderState();
        currentState.lastBlock   = lastBlock;
        currentState.eventNumber = eventNumber;
        currentState.blockNumberExpected = blockNumberExpected;

        if (sequentialRead) {
            try {
                currentState.filePosition = fileChannel.position();
                currentState.byteBufferLimit = byteBuffer.limit();
                currentState.byteBufferPosition = byteBuffer.position();
            }
            catch (IOException e) {/* should be OK */}
        }
        else {
            if (byteBuffer != null) {
                currentState.byteBufferLimit = byteBuffer.limit();
                currentState.byteBufferPosition = byteBuffer.position();
            }
        }

        if (evioVersion > 3) {
            currentState.blockHeader4 = (BlockHeaderV4)blockHeader4.clone();
        }
        else {
            currentState.blockHeader2 = (BlockHeaderV2)blockHeader2.clone();
        }

        return currentState;
    }


    /**
     * This method restores a previously saved state of this EvioReader object.
     * @param state a previously stored state of this EvioReader object.
     */
    private void restoreState(ReaderState state) {
        lastBlock   = state.lastBlock;
        eventNumber = state.eventNumber;
        blockNumberExpected = state.blockNumberExpected;

        if (sequentialRead) {
            try {
                fileChannel.position(state.filePosition);
                byteBuffer.limit(state.byteBufferLimit);
                byteBuffer.position(state.byteBufferPosition);
            }
            catch (IOException e) {/* should be OK */}
        }
        else {
            if (byteBuffer != null) {
                byteBuffer.position(state.byteBufferPosition);
            }
        }

        if (evioVersion > 3) {
            blockHeader = blockHeader4 = state.blockHeader4;
        }
        else {
            blockHeader = blockHeader2 = state.blockHeader2;
        }
    }


    /**
     * This method prints out a portion of a given ByteBuffer object
     * in hex representation of ints.
     *
     * @param buf buffer to be printed out
     * @param lenInInts length of data in ints to be printed
     */
    private void printBuffer(ByteBuffer buf, int lenInInts) {
        IntBuffer ibuf = buf.asIntBuffer();
        lenInInts = lenInInts > ibuf.capacity() ? ibuf.capacity() : lenInInts;
        for (int i=0; i < lenInInts; i++) {
            System.out.println("  Buf(" + i + ") = 0x" + Integer.toHexString(ibuf.get(i)));
        }
    }

    //------------------------

    /**
     * Constructor for reading an event file.
     *
     * @param path the full path to the file that contains events.
     *             For writing event files, use an <code>EventWriter</code> object.
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if file arg is null
     */
    public EvioReader(String path) throws EvioException, IOException {
        this(new File(path));
    }

    /**
     * Constructor for reading an event file.
     *
     * @param path the full path to the file that contains events.
     *             For writing event files, use an <code>EventWriter</code> object.
     * @param checkBlkNumSeq if <code>true</code> check the block number sequence
     *                       and throw an exception if it is not sequential starting
     *                       with 1
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if file arg is null;
     *                       if first block number != 1 when checkBlkNumSeq arg is true
     */
    public EvioReader(String path, boolean checkBlkNumSeq) throws EvioException, IOException {
        this(new File(path), checkBlkNumSeq);
    }

    /**
     * Constructor for reading an event file.
     *
     * @param file the file that contains events.
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if file arg is null
     */
    public EvioReader(File file) throws EvioException, IOException {
        this(file, false);
    }


    /**
     * Constructor for reading an event file.
     *
     * @param file the file that contains events.
     * @param checkBlkNumSeq if <code>true</code> check the block number sequence
     *                       and throw an exception if it is not sequential starting
     *                       with 1
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if file arg is null;
     *                       if first block number != 1 when checkBlkNumSeq arg is true
     */
    public EvioReader(File file, boolean checkBlkNumSeq)
                                        throws EvioException, IOException {
        this(file, checkBlkNumSeq, false);
    }


    /**
     * Constructor for reading an event file.
     *
     * @param path the full path to the file that contains events.
     *             For writing event files, use an <code>EventWriter</code> object.
     * @param checkBlkNumSeq if <code>true</code> check the block number sequence
     *                       and throw an exception if it is not sequential starting
     *                       with 1
     * @param sequential     if <code>true</code> read the file sequentially and not
     *                       using a memory mapped buffer. If file > 2.1 GB, then reads
     *                       are always sequential
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if file arg is null;
     *                       if first block number != 1 when checkBlkNumSeq arg is true
     */
    public EvioReader(String path, boolean checkBlkNumSeq, boolean sequential)
            throws EvioException, IOException {
        this(new File(path), checkBlkNumSeq, sequential);
    }


    /**
     * Constructor for reading an event file.
     *
     * @param file the file that contains events.
     * @param checkBlkNumSeq if <code>true</code> check the block number sequence
     *                       and throw an exception if it is not sequential starting
     *                       with 1
     * @param sequential     if <code>true</code> read the file sequentially and not
     *                       using a memory mapped buffer. If file > 2.1 GB, then reads
     *                       are always sequential
     *
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if file arg is null;
     *                       if file is too small to have valid evio format data
     *                       if first block number != 1 when checkBlkNumSeq arg is true
     */
    public EvioReader(File file, boolean checkBlkNumSeq, boolean sequential)
                                        throws EvioException, IOException {
        if (file == null) {
            throw new EvioException("File arg is null");
        }
        this.file = file;

        checkBlockNumberSequence = checkBlkNumSeq;
        sequentialRead = sequential;
        initialPosition = 0;

        FileInputStream fileInputStream = new FileInputStream(file);
        path = file.getAbsolutePath();
        fileChannel = fileInputStream.getChannel();
        fileSize = fileChannel.size();

        if (fileSize < 40) {
            throw new EvioException("File too small to have valid evio data");
        }

        // Look at the first block header to get various info like endianness and version.
        // Store it for later reference in blockHeader2,4 and in other variables.
        ByteBuffer headerBuf = fileChannel.map(FileChannel.MapMode.READ_ONLY, 0L, 32L);
        parseFirstHeader(headerBuf);

        // What we do from here depends on the evio format version.
        // If we've got the old version, don't memory map big (> 2.1 GB) files,
        // use sequential reading instead.
        if (evioVersion < 4) {
            // Remember, no dictionaries exist for these early versions

            // Got a big file? If so we cannot use a memory mapped file.
            if (fileSize > Integer.MAX_VALUE) {
                sequentialRead = true;
            }

            if (sequentialRead) {
                // Taken from FileInputStream javadoc:
                //
                // "Reading bytes from this [fileInput] stream will increment the channel's
                //  position.  Changing the channel's position, either explicitly or by
                //  reading, will change this stream's file position".
                //
                // So reading from either the dataStream or fileChannel object will
                // change both positions.

                dataStream = new DataInputStream(fileInputStream);
//System.out.println("Big file or reading sequentially for evio versions 2,3");
                prepareForSequentialRead();
            }
            else {
                byteBuffer = fileChannel.map(FileChannel.MapMode.READ_ONLY, 0L, fileSize);
                byteBuffer.order(byteOrder);
                prepareForBufferRead(byteBuffer);
//System.out.println("Memory Map versions 2,3");
            }
        }
        // For the new version, memory map the file - even the big ones
        else {
            if (sequentialRead) {
System.out.println("Reading sequentially for evio versions 4");
                dataStream = new DataInputStream(fileInputStream);
                prepareForSequentialRead();
            }
            else {
                mappedMemoryHandler = new MappedMemoryHandler(fileChannel, byteOrder);
                if (blockHeader4.hasDictionary()) {
                    ByteBuffer buf = mappedMemoryHandler.getFirstMap();
                    // Jump to the first event
                    prepareForBufferRead(buf);
                    // Dictionary is the first event
                    readDictionary(buf);
                }
            }
        }

        parser = new EventParser();
    }


    /**
     * Constructor for reading a buffer.
     *
     * @param byteBuffer the buffer that contains events.
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if buffer arg is null
     */
    public EvioReader(ByteBuffer byteBuffer) throws EvioException, IOException {
        this(byteBuffer, false);
    }

    /**
     * Constructor for reading a buffer.
     *
     * @param byteBuffer the buffer that contains events.
     * @param checkBlkNumSeq if <code>true</code> check the block number sequence
     *                       and throw an exception if it is not sequential starting
     *                       with 1
     * @see EventWriter
     * @throws IOException   if read failure
     * @throws EvioException if buffer arg is null;
     *                       if first block number != 1 when checkBlkNumSeq arg is true
     */
    public EvioReader(ByteBuffer byteBuffer, boolean checkBlkNumSeq)
                                                    throws EvioException, IOException {

        if (byteBuffer == null) {
            throw new EvioException("Buffer arg is null");
        }

        checkBlockNumberSequence = checkBlkNumSeq;
        this.byteBuffer = byteBuffer.slice(); // remove necessity to track initial position

        // Look at the first block header to get various info like endianness and version.
        // Store it for later reference in blockHeader2,4 and in variables.
        // Position is moved past header.
        parseFirstHeader(byteBuffer);
        // Move position back to beginning
        byteBuffer.position(0);

        // For the latest evio format, generate a table
        // of all event positions in buffer for random access.
        if (evioVersion > 3) {
// System.out.println("EvioReader const: evioVersion = " + evioVersion + ", create mem handler");
            mappedMemoryHandler = new MappedMemoryHandler(byteBuffer);
            if (blockHeader4.hasDictionary()) {
                ByteBuffer buf = mappedMemoryHandler.getFirstMap();
                // Jump to the first event
                prepareForBufferRead(buf);
                // Dictionary is the first event
                readDictionary(buf);
            }
        }
        else {
            // Setting the byte order is only necessary if someone hands
            // this method a buffer in which the byte order is improperly set.
            byteBuffer.order(byteOrder);
            prepareForBufferRead(byteBuffer);
        }

        parser = new EventParser();
    }

    /**
     * This method can be used to avoid creating additional EvioReader
     * objects by reusing this one with another buffer. The method
     * {@link #close()} is called before anything else.
     *
     * @param buf ByteBuffer to be read
     * @throws IOException   if read failure
     * @throws EvioException if buf is null;
     *                       if first block number != 1 when checkBlkNumSeq arg is true
     */
    public synchronized void setBuffer(ByteBuffer buf) throws EvioException, IOException {

        if (buf == null) {
            throw new EvioException("arg is null");
        }

        close();

        lastBlock           =  false;
        eventNumber         =  0;
        blockCount          = -1;
        eventCount          = -1;
        blockNumberExpected =  1;
        dictionaryXML       =  null;
        initialPosition     =  buf.position();
        byteBuffer          =  buf.slice();
        sequentialRead      = false;

        parseFirstHeader(byteBuffer);
        byteBuffer.position(0);

        if (evioVersion > 3) {
            mappedMemoryHandler = new MappedMemoryHandler(byteBuffer);
            if (blockHeader4.hasDictionary()) {
                ByteBuffer bb = mappedMemoryHandler.getFirstMap();
                // Jump to the first event
                prepareForBufferRead(bb);
                // Dictionary is the first event
                readDictionary(bb);
            }
        }
        else {
            byteBuffer.order(byteOrder);
            prepareForBufferRead(byteBuffer);
        }

        closed = false;
    }

    /**
     * Has {@link #close()} been called (without reopening by calling
     * {@link #setBuffer(java.nio.ByteBuffer)})?
     *
     * @return {@code true} if this object closed, else {@code false}.
     */
    public synchronized boolean isClosed() { return closed; }

    /**
     * Is this reader checking the block number sequence and
     * throwing an exception is it's not sequential and starting with 1?
     * @return <code>true</code> if checking block number sequence, else <code>false</code>
     */
    public boolean checkBlockNumberSequence() { return checkBlockNumberSequence; }

    /**
     * Get the byte order of the file/buffer being read.
     * @return byte order of the file/buffer being read.
     */
    public ByteOrder getByteOrder() { return byteOrder; }

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
     * Get the file/buffer parser.
     * @return file/buffer parser.
     */
    public EventParser getParser() {
        return parser;
    }

    /**
     * Set the file/buffer parser.
     * @param parser file/buffer parser.
     */
    public void setParser(EventParser parser) {
        if (parser != null) {
            this.parser = parser;
        }
    }

     /**
     * Get the XML format dictionary is there is one.
     *
     * @return XML format dictionary, else null.
     */
    public String getDictionaryXML() {
        return dictionaryXML;
    }

    /**
     * Does this evio file have an associated XML dictionary?
     *
     * @return <code>true</code> if this evio file has an associated XML dictionary,
     *         else <code>false</code>
     */
    public boolean hasDictionaryXML() {
        return dictionaryXML != null;
    }

    /**
     * Get the number of events remaining in the file.
     *
     * @return number of events remaining in the file
     * @throws IOException if failed file access
     * @throws EvioException if failed reading from coda v3 file
     */
    public int getNumEventsRemaining() throws IOException, EvioException {
        return getEventCount() - eventNumber;
    }

    /**
     * Get the byte buffer being read directly or corresponding to the event file.
     * Not a very useful method. For files, it works only for evio versions 2,3 and
     * returns the internal buffer containing an evio block if using sequential access
     * (for example files > 2.1 GB). It returns the memory mapped buffer otherwise.
     * For reading buffers it returns the buffer being read.
     * @return the byte buffer being read (in certain cases).
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
     * This returns the FIRST block (physical record) header.
     *
     * @return the first block header.
     */
    public IBlockHeader getFirstBlockHeader() {
        return firstBlockHeader;
    }


    /**
     * Reads 8 words of the first block (physical record) header in order to determine
     * the evio version # and endianness of the file or buffer in question. These things
     * do <b>not</b> need to be examined in subsequent block headers. Called only by
     * synchronized methods or constructors.
     *
     * @throws EvioException if buffer too small, contains invalid data,
     *                       or bad block # sequence
     */
    protected void parseFirstHeader(ByteBuffer headerBuf)
            throws EvioException {

        // Check buffer length
        headerBuf.position(0);
        if (headerBuf.remaining() < 32) {
            throw new EvioException("buffer too small");
        }

        // Get the file's version # and byte order
        byteOrder = headerBuf.order();

        int magicNumber = headerBuf.getInt(MAGIC_OFFSET);

        if (magicNumber != IBlockHeader.MAGIC_NUMBER) {
            swap = true;

            if (byteOrder == ByteOrder.BIG_ENDIAN) {
                byteOrder = ByteOrder.LITTLE_ENDIAN;
            }
            else {
                byteOrder = ByteOrder.BIG_ENDIAN;
            }
            headerBuf.order(byteOrder);

            // Reread magic number to make sure things are OK
            magicNumber = headerBuf.getInt(MAGIC_OFFSET);
            if (magicNumber != IBlockHeader.MAGIC_NUMBER) {
System.out.println("ERROR reread magic # (" + magicNumber + ") & still not right");
                throw new EvioException("bad magic #");
            }
        }

        // Check the version number. This requires peeking ahead 5 ints or 20 bytes.
        evioVersion = headerBuf.getInt(VERSION_OFFSET) & VERSION_MASK;
        if (evioVersion < 1)  {
            throw new EvioException("bad version");
        }
//System.out.println("Evio version# = " + evioVersion);

        if (evioVersion >= 4) {
            blockHeader4.setBufferStartingPosition(0);

//                    int pos = 0;
//System.out.println("BlockHeader v4:");
//System.out.println("   block length  = 0x" + Integer.toHexString(headerBuf.getInt(pos))); pos+=4;
//System.out.println("   block number  = 0x" + Integer.toHexString(headerBuf.getInt(pos))); pos+=4;
//System.out.println("   header length = 0x" + Integer.toHexString(headerBuf.getInt(pos))); pos+=4;
//System.out.println("   event count   = 0x" + Integer.toHexString(headerBuf.getInt(pos))); pos+=8;
//System.out.println("   version       = 0x" + Integer.toHexString(headerBuf.getInt(pos))); pos+=8;
//System.out.println("   magic number  = 0x" + Integer.toHexString(headerBuf.getInt(pos))); pos+=4;
//System.out.println();

            // Read the header data
            blockHeader4.setSize(        headerBuf.getInt());
            blockHeader4.setNumber(      headerBuf.getInt());
            blockHeader4.setHeaderLength(headerBuf.getInt());
            blockHeader4.setEventCount(  headerBuf.getInt());
            blockHeader4.setReserved1(   headerBuf.getInt());

            // Use 6th word to set bit info & version
            blockHeader4.parseToBitInfo(headerBuf.getInt());
            blockHeader4.setVersion(evioVersion);
            lastBlock = blockHeader4.getBitInfo(1);
            blockHeader4.setReserved2(headerBuf.getInt());
            blockHeader4.setMagicNumber(headerBuf.getInt());
            blockHeader4.setByteOrder(byteOrder);
            blockHeader = blockHeader4;
            firstBlockHeader = new BlockHeaderV4(blockHeader4);

            // Deal with non-standard header lengths here
            int headerLenDiff = blockHeader4.getHeaderLength() - BlockHeaderV4.HEADER_SIZE;
            // If too small quit with error since headers have a minimum size
            if (headerLenDiff < 0) {
                throw new EvioException("header size too small");
            }

//System.out.println("BlockHeader v4:");
//System.out.println("   block length  = " + blockHeader4.getSize() + " ints");
//System.out.println("   block number  = " + blockHeader4.getNumber());
//System.out.println("   header length = " + blockHeader4.getHeaderLength() + " ints");
//System.out.println("   event count   = " + blockHeader4.getEventCount());
//System.out.println("   version       = " + blockHeader4.getVersion());
//System.out.println("   has Dict      = " + blockHeader4.getBitInfo(0));
//System.out.println("   is End        = " + lastBlock);
//System.out.println("   magic number  = " + Integer.toHexString(blockHeader4.getMagicNumber()));
//System.out.println();
        }
        else {
            // Cache the starting position
            blockHeader2.setBufferStartingPosition(0);

            // read the header data.
            blockHeader2.setSize(        headerBuf.getInt());
            blockHeader2.setNumber(      headerBuf.getInt());
            blockHeader2.setHeaderLength(headerBuf.getInt());
            blockHeader2.setStart(       headerBuf.getInt());
            blockHeader2.setEnd(         headerBuf.getInt());
            // skip version
            headerBuf.getInt();
            blockHeader2.setVersion(evioVersion);
            blockHeader2.setReserved1(   headerBuf.getInt());
            blockHeader2.setMagicNumber( headerBuf.getInt());
            blockHeader2.setByteOrder(byteOrder);
            blockHeader = blockHeader2;

            firstBlockHeader = new BlockHeaderV2(blockHeader2);
        }

        // Store this for later regurgitation of blockCount
        firstBlockSize = 4*blockHeader.getSize();

        // check block number if so configured
        if (checkBlockNumberSequence) {
            if (blockHeader.getNumber() != blockNumberExpected) {
System.out.println("block # out of sequence, got " + blockHeader.getNumber() +
                   " expecting " + blockNumberExpected);

                throw new EvioException("bad block # sequence");
            }
            blockNumberExpected++;
        }
    }


    /**
     * Reads the first block header into a buffer and gets that
     * buffer ready for first-time read.
     *
     * @throws IOException if file access problems
     */
    private void prepareForSequentialRead() throws IOException {
        // Create a buffer to hold the entire first block of data.
        // Make this bigger than necessary so we're not constantly reallocating.
        int blkSize = firstBlockHeader.getSize();
        if (blockBuffer == null || blockBuffer.capacity() < 4*blkSize) {
            blockBuffer = ByteBuffer.allocate(4*blkSize + 10000);
            blockBuffer.order(byteOrder);
        }
        blockBuffer.clear().limit(4*blkSize);

        // Read the entire first block of data
        fileChannel.read(blockBuffer);
        // Get it ready for reading
        blockBuffer.flip();
        // Convenience variable
        byteBuffer = blockBuffer;

        // Position buffer properly
        prepareForBufferRead(byteBuffer);
    }


    /**
     * Sets the proper buffer position for first-time read AFTER the first header.
     * @param buffer buffer to prepare
     */
    private void prepareForBufferRead(ByteBuffer buffer) {
        // Position after header
        int pos = 32;
        buffer.position(pos);

        // Deal with non-standard first header length.
        // No non-standard header lengths in evio version 2 & 3 files.
        if (evioVersion < 4) return;

        int headerLenDiff = blockHeader4.getHeaderLength() - BlockHeaderV4.HEADER_SIZE;
        // Hop over any extra header words
        if (headerLenDiff > 0) {
            for (int i=0; i < headerLenDiff; i++) {
//System.out.println("Skip extra header int");
                pos += 4;
                buffer.position(pos);
            }
        }
    }


    /**
     * Reads the block (physical record) header. Assumes the mapped buffer or file is positioned
     * at the start of the next block header (physical record.) By the time this is called,
     * the version # and byte order have already been determined. Not necessary to do that
     * for each block header that's read. Called from synchronized method.<br>
     *
     * A Bank header is 8, 32-bit ints. The first int is the size of the block in ints
     * (not counting the length itself, i.e., the number of ints to follow).
     *
     * Most users should have no need for this method, since most applications do not
     * care about the block (physical record) header.
     *
     * @return status of read attempt
     * @throws IOException if file access problems
     */
    protected ReadStatus nextBlockHeader() throws IOException {

        // We already read the last block header
        if (lastBlock) {
            return ReadStatus.END_OF_FILE;
        }

        // Have enough remaining bytes to read header?
        if (sequentialRead) {
            if (fileSize - fileChannel.position() < 32L) {
                return ReadStatus.END_OF_FILE;
            }
        }
        else {
            if (byteBuffer.remaining() < 32) {
                byteBuffer.clear();
                return ReadStatus.END_OF_FILE;
            }
        }
        try {

            if (sequentialRead) {
                // Read len of block
                int blkSize = dataStream.readInt();
                if (swap) blkSize = Integer.reverseBytes(blkSize);
                // Create a buffer to hold the entire first block of data
                if (blockBuffer != null && blockBuffer.capacity() >= 4*blkSize) {
                    blockBuffer.clear();
                    blockBuffer.limit(4*blkSize);
//System.out.println("nextBlockHeader: reset buffer limit -> " + (4 * blkSize));
                }
                else {
                    // Make this bigger than necessary so we're not constantly reallocating
//System.out.println("nextBlockHeader: make the buffer bigger -> 10000 + " + (4*blkSize));
                    blockBuffer = ByteBuffer.allocate(4*blkSize + 10000);
                    blockBuffer.limit(4*blkSize);
                    blockBuffer.order(byteOrder);
                }
                // Read the entire first block of data
                blockBuffer.putInt(blkSize);
                fileChannel.read(blockBuffer);
                blockBuffer.flip();
                // Convenience variable
                byteBuffer = blockBuffer;
                // Now keeping track of pos in this new blockBuffer (should be 0)
                blockHeader.setBufferStartingPosition(blockBuffer.position());
            }
            else {
                // Record starting position
                blockHeader.setBufferStartingPosition(byteBuffer.position());
            }

            if (evioVersion >= 4) {
                // Read the header data.
                blockHeader4.setSize(byteBuffer.getInt());
                blockHeader4.setNumber(byteBuffer.getInt());
                blockHeader4.setHeaderLength(byteBuffer.getInt());
                blockHeader4.setEventCount(byteBuffer.getInt());
                blockHeader4.setReserved1(byteBuffer.getInt());
                // Use 6th word to set bit info
                blockHeader4.parseToBitInfo(byteBuffer.getInt());
                blockHeader4.setVersion(evioVersion);
                lastBlock = blockHeader4.getBitInfo(1);
                blockHeader4.setReserved2(byteBuffer.getInt());
                blockHeader4.setMagicNumber(byteBuffer.getInt());
                blockHeader = blockHeader4;

                // Deal with non-standard header lengths here
                int headerLenDiff = blockHeader4.getHeaderLength() - BlockHeaderV4.HEADER_SIZE;
                // If too small quit with error since headers have a minimum size
                if (headerLenDiff < 0) {
                    return ReadStatus.EVIO_EXCEPTION;
                }
                // If bigger, read extra ints
                else if (headerLenDiff > 0) {
                    for (int i=0; i < headerLenDiff; i++) {
                        byteBuffer.getInt();
                    }
                }
            }
            else if (evioVersion < 4) {
                // read the header data.
                blockHeader2.setSize(byteBuffer.getInt());
                blockHeader2.setNumber(byteBuffer.getInt());
                blockHeader2.setHeaderLength(byteBuffer.getInt());
                blockHeader2.setStart(byteBuffer.getInt());
                blockHeader2.setEnd(byteBuffer.getInt());
                // skip version
                byteBuffer.getInt();
                blockHeader2.setVersion(evioVersion);
                blockHeader2.setReserved1(byteBuffer.getInt());
                blockHeader2.setMagicNumber(byteBuffer.getInt());
                blockHeader = blockHeader2;
            }
            else {
                // bad version # - should never happen
                return ReadStatus.EVIO_EXCEPTION;
            }

            // check block number if so configured
            if (checkBlockNumberSequence) {
                if (blockHeader.getNumber() != blockNumberExpected) {

System.out.println("block # out of sequence, got " + blockHeader.getNumber() +
                   " expecting " + blockNumberExpected);

                    return ReadStatus.EVIO_EXCEPTION;
                }
                blockNumberExpected++;
            }
        }
        catch (EvioException e) {
            e.printStackTrace();
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
     * This method is only called once at the very beginning if buffer is known to have
     * a dictionary. It then reads that dictionary. Only called in versions 4 & up.
     * Position buffer after dictionary. Called from synchronized method or constructor.
     *
     * @since 4.0
     * @param buffer buffer to read to get dictionary
     * @throws EvioException if failed read due to bad buffer format;
     *                       if version 3 or earlier
     */
     private void readDictionary(ByteBuffer buffer) throws EvioException {

         if (evioVersion < 4) {
             throw new EvioException("Unsupported version (" + evioVersion + ")");
         }

         // How many bytes remain in this buffer?
         int bytesRemaining = buffer.remaining();
         if (bytesRemaining < 12) {
             throw new EvioException("Not enough data in buffer");
         }

         // Once here, we are assured the entire next event is in this buffer.
         int length;
         length = buffer.getInt();

         if (length < 1) {
             throw new EvioException("Bad value for dictionary length");
         }
         bytesRemaining -= 4;

         // Since we're only interested in length, read but ignore rest of the header.
         buffer.getInt();
         bytesRemaining -= 4;

         // get the raw data
         int eventDataSizeBytes = 4*(length - 1);
         if (bytesRemaining < eventDataSizeBytes) {
             throw new EvioException("Not enough data in buffer");
         }

         byte bytes[] = new byte[eventDataSizeBytes];

         // Read in dictionary data
         try {
            buffer.get(bytes, 0, eventDataSizeBytes);
         }
         catch (Exception e) {
             throw new EvioException("Problems reading buffer");
         }

         // This is the very first event and must be a dictionary
         String[] strs = BaseStructure.unpackRawBytesToStrings(bytes, 0);
         if (strs == null) {
             throw new EvioException("Data in bad format");
         }
         dictionaryXML = strs[0];
     }

    /**
     * Get the event in the file/buffer at a given index (starting at 1).
     * As useful as this sounds, most applications will probably call
     * {@link #parseNextEvent()} or {@link #parseEvent(int)} instead,
     * since it combines combines getting an event with parsing it.<p>
     *
     * @param  index the event number in a 1,2,..N counting sense, from beginning of file/buffer.
     * @return the event in the file/buffer at the given index or null if none
     * @throws IOException   if failed file access
     * @throws EvioException if failed read due to bad file/buffer format;
     *                       if out of memory;
     *                       if index < 1;
     *                       if object closed
     */
    public EvioEvent getEvent(int index)
            throws IOException, EvioException {

        if (index < 1) {
            throw new EvioException("index arg starts at 1");
        }

        //  Versions 2 & 3 || sequential
        if (sequentialRead || evioVersion < 4) {
            // Do not fully parse events up to final event
            return gotoEventNumber(index, false);
        }

        //  Version 4 and up && non sequential
        return getEventV4(index);
    }


    /**
     * Get the event in the file/buffer at a given index (starting at 1).
     * It is only valid for evio versions 4+.
     * As useful as this sounds, most applications will probably call
     * {@link #parseNextEvent()} or {@link #parseEvent(int)} instead,
     * since it combines combines getting an event with parsing it.
     * Only called if not sequential reading.<p>
     *
     * @param  index the event number in a 1,2,..N counting sense, from beginning of file/buffer.
     * @return the event in the file/buffer at the given index or null if none
     * @throws IOException   if failed file access
     * @throws EvioException if failed read due to bad file/buffer format;
     *                       if out of memory;
     *                       if object closed
     */
    private synchronized EvioEvent getEventV4(int index) throws IOException, EvioException {

        if (index > mappedMemoryHandler.getEventCount()) {
            return null;
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        index--;

        EvioEvent event = new EvioEvent();
        BaseStructureHeader header = event.getHeader();

        int length, eventDataSizeBytes = 0;

        ByteBuffer buf = mappedMemoryHandler.getByteBuffer(index);
        length = buf.getInt();

        if (length < 1) {
            throw new EvioException("Bad file/buffer format");
        }
        header.setLength(length);

        try {
            // Read second header word
            if (byteOrder == ByteOrder.BIG_ENDIAN) {
                // Interested in bit pattern, not negative numbers
                int dt;
                header.setTag(ByteDataTransformer.shortBitsToInt(buf.getShort()));
                dt = ByteDataTransformer.byteBitsToInt(buf.get());

                int type = dt & 0x3f;
                int padding = dt >>> 6;
                // If only 7th bit set, that can only be the legacy tagsegment type
                // with no padding information - convert it properly.
                if (dt == 0x40) {
                    type = DataType.TAGSEGMENT.getValue();
                    padding = 0;
                }
                header.setDataType(type);
                header.setPadding(padding);

                // Once we know what the data type is, let the no-arg constructed
                // event know what type it is holding so xml names are set correctly.
                event.setXmlNames();
                header.setNumber(ByteDataTransformer.byteBitsToInt(buf.get()));
            }
            else {
                int dt;
                header.setNumber(ByteDataTransformer.byteBitsToInt(buf.get()));
                dt = ByteDataTransformer.byteBitsToInt(buf.get());

                int type = dt & 0x3f;
                int padding = dt >>> 6;
                if (dt == 0x40) {
                    type = DataType.TAGSEGMENT.getValue();
                    padding = 0;
                }
                header.setDataType(type);
                header.setPadding(padding);

                event.setXmlNames();
                header.setTag(ByteDataTransformer.shortBitsToInt(buf.getShort()));
            }

            // Read the raw data
            eventDataSizeBytes = 4*(length - 1);
            byte bytes[] = new byte[eventDataSizeBytes];
            buf.get(bytes, 0, eventDataSizeBytes);

            event.setRawBytes(bytes);
            event.setByteOrder(byteOrder);
            event.setEventNumber(++eventNumber);

        }
        catch (OutOfMemoryError e) {
            throw new EvioException("Out Of Memory: (event size = " + eventDataSizeBytes + ")", e);
        }
        catch (Exception e) {
            throw new EvioException("Error", e);
        }

        return event;
    }


	/**
	 * This is a workhorse method. It retrieves the desired event from the file/buffer,
     * and then parses it SAX-like. It will drill down and uncover all structures
     * (banks, segments, and tagsegments) and notify any interested listeners.
	 *
     * @param  index number of event desired, starting at 1, from beginning of file/buffer
	 * @return the parsed event at the given index or null if none
     * @throws IOException if failed file access
     * @throws EvioException if failed read due to bad file/buffer format;
     *                       if out of memory;
     *                       if index < 1;
     *                       if object closed
	 */
	public synchronized EvioEvent parseEvent(int index) throws IOException, EvioException {
		EvioEvent event = getEvent(index);
        if (event != null) parseEvent(event);
		return event;
	}


    /**
     * Get the next event in the file/buffer. As useful as this sounds, most
     * applications will probably call {@link #parseNextEvent()} instead, since
     * it combines getting the next event with parsing the next event.<p>
     *
     * Although this method can get events in versions 4+, it now delegates that
     * to another method. No changes were made to this method from versions 1-3 in order
     * to read the version 4 format as it is subset of versions 1-3 with variable block
     * length.
     *
     * @return the next event in the file.
     *         On error it throws an EvioException.
     *         On end of file, it returns <code>null</code>.
     * @throws IOException   if failed file access
     * @throws EvioException if failed read due to bad buffer format;
     *                       if object closed
     */
    public synchronized EvioEvent nextEvent() throws IOException, EvioException {

        if (!sequentialRead && evioVersion > 3) {
            return getEvent(eventNumber+1);
        }

        if (closed) {
            throw new EvioException("object closed");
        }

        EvioEvent event = new EvioEvent();
        BaseStructureHeader header = event.getHeader();
        long currentPosition = byteBuffer.position();

        // How many bytes remain in this block until we reach the next block header?
        // Must see if we have to deal with crossing physical record boundaries (< version 4).
        int bytesRemaining = blockBytesRemaining();
//System.out.println("nextEvent: pos = " + currentPosition + ", lim = " + byteBuffer.limit() +
//                   ", remaining = " + bytesRemaining);
        if (bytesRemaining < 0) {
            throw new EvioException("Number of block bytes remaining is negative.");
        }

        // Are we exactly at the end of the block (physical record)?
        if (bytesRemaining == 0) {
            ReadStatus status = nextBlockHeader();
            if (status == ReadStatus.SUCCESS) {
                return nextEvent();
            }
            else if (status == ReadStatus.END_OF_FILE) {
                return null;
            }
            else {
                throw new EvioException("Failed reading block header in nextEvent.");
            }
        }
        // Or have we already read in the last event?
        // If jevio versions 1-3, the last block may not be full.
        // Thus bytesRemaining may be > 0, but we may have read
        // in all the existing data. (This should never happen in version 4).
        else if (blockHeader.getBufferEndingPosition() == currentPosition) {
            return null;
        }

        // Version   4: once here, we are assured the entire next event is in this block.
        // Version 1-3: no matter what, we can get the length of the next event.
        int length;
        length = byteBuffer.getInt();
        if (length < 1) {
            throw new EvioException("non-positive length (0x" + Integer.toHexString(length) + ")");
        }

        header.setLength(length);
        bytesRemaining -= 4; // just read in 4 bytes

        // Versions 1-3: if we were unlucky, after reading the length
        //               there are no bytes remaining in this bank.
        // Don't really need the "if (version < 4)" here except for clarity.
        if (evioVersion < 4) {
            if (bytesRemaining == 0) {
                ReadStatus status = nextBlockHeader();
                if (status == ReadStatus.END_OF_FILE) {
                    return null;
                }
                else if (status != ReadStatus.SUCCESS) {
                    throw new EvioException("Failed reading block header in nextEvent.");
                }
                bytesRemaining = blockBytesRemaining();
            }
        }

        // Now should be good to go, except data may cross block boundary.
        // In any case, should be able to read the rest of the header.
        if (byteOrder == ByteOrder.BIG_ENDIAN) {
            // interested in bit pattern, not negative numbers
            header.setTag(ByteDataTransformer.shortBitsToInt(byteBuffer.getShort()));
            int dt = ByteDataTransformer.byteBitsToInt(byteBuffer.get());

            int type = dt & 0x3f;
            int padding = dt >>> 6;
            // If only 7th bit set, that can only be the legacy tagsegment type
            // with no padding information - convert it properly.
            if (dt == 0x40) {
                type = DataType.TAGSEGMENT.getValue();
                padding = 0;
            }
            header.setDataType(type);
            header.setPadding(padding);

            // Once we know what the data type is, let the no-arg constructed
            // event know what type it is holding so xml names are set correctly.
            event.setXmlNames();
            header.setNumber(ByteDataTransformer.byteBitsToInt(byteBuffer.get()));
        }
        else {
            header.setNumber(ByteDataTransformer.byteBitsToInt(byteBuffer.get()));
            int dt = ByteDataTransformer.byteBitsToInt(byteBuffer.get());

            int type = dt & 0x3f;
            int padding = dt >>> 6;
            if (dt == 0x40) {
                type = DataType.TAGSEGMENT.getValue();
                padding = 0;
            }
            header.setDataType(type);
            header.setPadding(padding);

            event.setXmlNames();
            header.setTag(ByteDataTransformer.shortBitsToInt(byteBuffer.getShort()));
        }
        bytesRemaining -= 4; // just read in 4 bytes

        // get the raw data
        int eventDataSizeBytes = 4*(length - 1);

        try {
            byte bytes[] = new byte[eventDataSizeBytes];

            int bytesToGo = eventDataSizeBytes;
            int offset = 0;

            // Don't really need the "if (version < 4)" here except for clarity.
            if (evioVersion < 4) {
                // be in while loop if have to cross block boundary[ies].
                while (bytesToGo > bytesRemaining) {
                    byteBuffer.get(bytes, offset, bytesRemaining);

                    ReadStatus status = nextBlockHeader();
                    if (status == ReadStatus.END_OF_FILE) {
                        return null;
                    }
                    else if (status != ReadStatus.SUCCESS) {
                        throw new EvioException("Failed reading block header after crossing boundary in nextEvent.");
                    }
                    bytesToGo -= bytesRemaining;
                    offset += bytesRemaining;
                    bytesRemaining = blockBytesRemaining();
                }
            }

            // last (perhaps only) read
            byteBuffer.get(bytes, offset, bytesToGo);
            event.setRawBytes(bytes);
            event.setByteOrder(byteOrder); // add this to track endianness, timmer
            // Don't worry about dictionaries here as version must be 1-3
            event.setEventNumber(++eventNumber);
            return event;
        }
        catch (OutOfMemoryError ome) {
            System.out.println("Out Of Memory\n" +
                                       "eventDataSizeBytes = " + eventDataSizeBytes + "\n" +
                                       "bytes Remaining = " + bytesRemaining + "\n" +
                                       "event Count: " + eventCount);
            return null;
        }
        catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }


	/**
	 * This is a workhorse method. It retrieves the next event from the file/buffer,
     * and then parses it SAX-like. It will drill down and uncover all structures
     * (banks, segments, and tagsegments) and notify any interested listeners.
	 *
	 * @return the event that was parsed.
     *         On error it throws an EvioException.
     *         On end of file, it returns <code>null</code>.
     * @throws IOException if failed file access
     * @throws EvioException if read failure or bad format
     *                       if object closed
     */
	public synchronized EvioEvent parseNextEvent() throws IOException, EvioException {
		EvioEvent event = nextEvent();
		if (event != null) {
			parseEvent(event);
		}
		return event;
	}

	/**
	 * This will parse an event, SAX-like. It will drill down and uncover all structures
     * (banks, segments, and tagsegments) and notify any interested listeners.<p>
	 *
	 * As useful as this sounds, most applications will probably call {@link #parseNextEvent()}
     * instead, since it combines combines getting the next event with parsing the next event.<p>
     *
     * This method is only called by synchronized methods and therefore is not synchronized.
	 *
	 * @param evioEvent the event to parse.
	 * @throws EvioException if bad format
	 */
	public void parseEvent(EvioEvent evioEvent) throws EvioException {
        // This method is synchronized too
		parser.parseEvent(evioEvent);
	}

	/**
	 * Get the number of bytes remaining in the current block (physical record).
     * This is used for pathology checks like crossing the block boundary.
     * Called only by {@link #nextEvent()}.
	 *
	 * @return the number of bytes remaining in the current block (physical record).
     */
	private int blockBytesRemaining() {
		try {
            return blockHeader.bytesRemaining(byteBuffer.position());
		}
		catch (EvioException e) {
			e.printStackTrace();
			return -1;
		}
	}

	/**
	 * The equivalent of rewinding the file. What it actually does
     * is set the position of the file/buffer back to where it was
     * after calling the constructor - after the first header.
     * This method, along with the two <code>position()</code> and the
     * <code>close()</code> method, allows applications to treat files
     * in a normal random access manner.
     *
     * @throws IOException   if failed file access or buffer/file read
     * @throws EvioException if object closed
	 */
    public synchronized void rewind() throws IOException, EvioException {
        if (closed) {
            throw new EvioException("object closed");
        }

        if (sequentialRead) {
            fileChannel.position(initialPosition);
            prepareForSequentialRead();
        }
        else if (evioVersion < 4) {
            byteBuffer.position(initialPosition);
            prepareForBufferRead(byteBuffer);
        }

        lastBlock = false;
        eventNumber = 0;
        blockNumberExpected = 1;
        blockHeader = firstBlockHeader;
        blockHeader.setBufferStartingPosition(initialPosition);
	}

	/**
	 * This is equivalent to obtaining the current position in the file.
     * What it actually does is return the position of the buffer. This
     * method, along with the <code>rewind()</code>, <code>position(int)</code>
     * and the <code>close()</code> method, allows applications to treat files
     * in a normal random access manner. Only meaningful to evio versions 1-3
     * and for sequential reading.<p>
	 *
	 * @return the position of the buffer; -1 if version 4+
     * @throws IOException   if error accessing file
     * @throws EvioException if object closed
     */
	public synchronized long position() throws IOException, EvioException {
        if (!sequentialRead && evioVersion > 3) return -1L;

        if (closed) {
            throw new EvioException("object closed");
        }

        if (sequentialRead) {
            return fileChannel.position();
        }
		return byteBuffer.position();
	}

	/**
	 * This method sets the current position in the file or buffer. This
     * method, along with the <code>rewind()</code>, <code>position()</code>
     * and the <code>close()</code> method, allows applications to treat files
     * in a normal random access manner. Only meaningful to evio versions 1-3
     * and for sequential reading.<p>
     *
     * <b>HOWEVER</b>, using this method is not necessary for random access of
     * events and is no longer recommended because it interferes with the sequential
     * reading of events. Therefore it is now deprecated.
     *
	 * @deprecated
	 * @param position the new position of the buffer.
     * @throws IOException   if error accessing file
     * @throws EvioException if object closed
     */
	public synchronized void position(long position) throws IOException, EvioException  {
        if (!sequentialRead && evioVersion > 3) return;

        if (closed) {
            throw new EvioException("object closed");
        }

        if (sequentialRead) {
            fileChannel.position(position);
        }
        else {
            byteBuffer.position((int)position);
        }
	}

	/**
	 * This is closes the file, but for buffers it only sets the position to 0.
     * This method, along with the <code>rewind()</code> and the two
     * <code>position()</code> methods, allows applications to treat files
     * in a normal random access manner.
     *
     * @throws IOException if error accessing file
	 */
    public synchronized void close() throws IOException {
        if (closed) {
            return;
        }

        if (!sequentialRead && evioVersion > 3) {
            if (byteBuffer != null) byteBuffer.position(initialPosition);
            mappedMemoryHandler = null;

            if (fileChannel != null) {
                fileChannel.close();
                fileChannel = null;
            }

            closed = true;
            return;
        }

        if (sequentialRead) {
            fileChannel.close();
            dataStream.close();
        }
        else {
            byteBuffer.position(initialPosition);
        }

        closed = true;
    }

	/**
	 * This returns the current (active) block (physical record) header.
     * Since most users have no interest in physical records, this method
     * should not be used. Mostly it is used by the test programs in the
	 * <code>EvioReaderTest</code> class.
	 *
	 * @return the current block header.
	 */
	public IBlockHeader getCurrentBlockHeader() {
		return blockHeader;
	}

    /**
     * Go to a specific event in the file. The events are numbered 1..N.
     * This number is transient--it is not part of the event as stored in the evio file.
     * In versions 4 and up this is just a wrapper on {@link #getEvent(int)}.
     *
     * @param evNumber the event number in a 1..N counting sense, from the start of the file.
     * @return the specified event in file or null if there's an error or nothing at that event #.
     * @throws IOException if failed file access
     * @throws EvioException if object closed
     */
    public EvioEvent gotoEventNumber(int evNumber) throws IOException, EvioException {
        return gotoEventNumber(evNumber, true);
    }


    /**
     * Go to a specific event in the file. The events are numbered 1..N.
     * This number is transient--it is not part of the event as stored in the evio file.
     * Before version 4, this does the work for {@link #getEvent(int)}.
     *
     * @param  evNumber the event number in a 1,2,..N counting sense, from beginning of file/buffer.
     * @param  parse if {@code true}, parse the desired event
     * @return the specified event in file or null if there's an error or nothing at that event #.
     * @throws IOException if failed file access
     * @throws EvioException if object closed
     */
    private synchronized EvioEvent gotoEventNumber(int evNumber, boolean parse)
            throws IOException, EvioException {

        if (evNumber < 1) {
			return null;
		}

        if (closed) {
            throw new EvioException("object closed");
        }

        if (!sequentialRead && evioVersion > 3) {
            try {
                if (parse) {
                    return parseEvent(evNumber);
                }
                else {
                    return getEvent(evNumber);
                }
            }
            catch (EvioException e) {
                return null;
            }
        }

		rewind();
		EvioEvent event;

		try {
			// get the first evNumber - 1 events without parsing
			for (int i = 1; i < evNumber; i++) {
				event = nextEvent();
				if (event == null) {
					throw new EvioException("Asked to go to event: " + evNumber + ", which is beyond the end of file");
				}
			}
			// get one more event, the evNumber'th event
            if (parse) {
			    return parseNextEvent();
            }
            else {
                return nextEvent();
            }
		}
		catch (EvioException e) {
			e.printStackTrace();
		}
		return null;
	}

	/**
     * Rewrite the file to XML (not including dictionary).
	 *
	 * @param path the path to the XML file.
	 * @return the status of the write.
     * @throws IOException   if failed file access
     * @throws EvioException if object closed
	 */
	public WriteStatus toXMLFile(String path) throws IOException, EvioException {
		return toXMLFile(path, null);
	}

	/**
	 * Rewrite the file to XML (not including dictionary).
	 *
	 * @param path the path to the XML file.
	 * @param progressListener and optional progress listener, can be <code>null</code>.
	 * @return the status of the write.
     * @throws IOException   if failed file access
     * @throws EvioException if object closed
     * @see IEvioProgressListener
	 */
	public synchronized WriteStatus toXMLFile(String path, IEvioProgressListener progressListener)
                throws IOException, EvioException {

        if (closed) {
            throw new EvioException("object closed");
        }

        FileOutputStream fos;

		try {
			fos = new FileOutputStream(path);
		}
		catch (FileNotFoundException e) {
			e.printStackTrace();
			return WriteStatus.CANNOT_OPEN_FILE;
		}

		try {
			XMLStreamWriter xmlWriter = XMLOutputFactory.newInstance().createXMLStreamWriter(fos);
			xmlWriter.writeStartDocument();
			xmlWriter.writeCharacters("\n");
			xmlWriter.writeComment("Event source file: " + path);

			// start the root element
			xmlWriter.writeCharacters("\n");
			xmlWriter.writeStartElement(ROOT_ELEMENT);
			xmlWriter.writeAttribute("numevents", "" + getEventCount());
            xmlWriter.writeCharacters("\n");

            // The difficulty is that this method can be called at
            // any time. So we need to save our state and then restore
            // it when we're done.
            ReaderState state = getState();

            // Go to the beginning
			rewind();

			// now loop through the events
			EvioEvent event;
			try {
				while ((event = parseNextEvent()) != null) {
					event.toXML(xmlWriter);
					// anybody interested in progress?
					if (progressListener != null) {
						progressListener.completed(event.getEventNumber(), getEventCount());
					}
				}
			}
			catch (EvioException e) {
				e.printStackTrace();
				return WriteStatus.UNKNOWN_ERROR;
			}

			// done. Close root element, end the document, and flush.
			xmlWriter.writeEndElement();
			xmlWriter.writeEndDocument();
			xmlWriter.flush();
			xmlWriter.close();

			try {
				fos.close();
			}
			catch (IOException e) {
				e.printStackTrace();
			}

            // Restore our original settings
            restoreState(state);

		}
		catch (XMLStreamException e) {
			e.printStackTrace();
			return WriteStatus.UNKNOWN_ERROR;
		}
        catch (FactoryConfigurationError e) {
            return WriteStatus.UNKNOWN_ERROR;
        }
        catch (EvioException e) {
            return WriteStatus.EVIO_EXCEPTION;
        }

		return WriteStatus.SUCCESS;
	}


    /**
     * This is the number of events in the file. Any dictionary event is <b>not</b>
     * included in the count. In versions 3 and earlier, it is not computed unless
     * asked for, and if asked for it is computed and cached.
     *
     * @return the number of events in the file.
     * @throws IOException   if failed file access
     * @throws EvioException if read failure;
     *                       if object closed
     */
    public synchronized int getEventCount() throws IOException, EvioException {

        if (closed) {
            throw new EvioException("object closed");
        }

        if (!sequentialRead && evioVersion > 3) {
            return mappedMemoryHandler.getEventCount();
        }

        if (eventCount < 0) {
            // The difficulty is that this method can be called at
            // any time. So we need to save our state and then restore
            // it when we're done.
            ReaderState state = getState();

            rewind();
            eventCount = 0;

            while ((nextEvent()) != null) {
                eventCount++;
            }

            // If sequential access to v2 file, then nextEvent() places
            // new data into byteBuffer. Restoring the original state
            // is useless without also restoring/re-reading the data.
            if (sequentialRead) {
                rewind();
                // Go back to original event # & therefore buffer data
                for (int i=1; i < state.eventNumber; i++) {
                    nextEvent();
                }
            }

            // Restore our original settings
            restoreState(state);
        }

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
     * @throws EvioException if object closed
     * @return the number of blocks in the file (estimate for version 3 files)
     */
    public synchronized int getBlockCount() throws EvioException{

        if (closed) {
            throw new EvioException("object closed");
        }

        if (!sequentialRead && evioVersion > 3) {
            return mappedMemoryHandler.getBlockCount();
        }

        if (blockCount < 0) {
            // Although block size is theoretically adjustable, I believe
            // that everyone used 8192 words for the block size in version 3.
            blockCount = (int) (fileSize/firstBlockSize);
        }

        return blockCount;
    }


	/**
	 * Method used for diagnostics. It compares two event files. It checks the following (in order):<br>
	 * <ol>
	 * <li> That neither file is null.
	 * <li> That both files exist.
	 * <li> That neither file is a directory.
	 * <li> That both files can be read.
	 * <li> That both files are the same size.
	 * <li> That both files contain the same number of events.
	 * <li> Finally, that they are the same in a byte-by-byte comparison.
	 * </ol>
     * NOTE: Two files with the same events but different physical record size will be reported as different.
     *       They will fail the same size test.
     * @param evFile1 first file to be compared
     * @param evFile2 second file to be compared
	 * @return <code>true</code> if the files are, byte-by-byte, identical.
	 */
	public static boolean compareEventFiles(File evFile1, File evFile2) {

		if ((evFile1 == null) || (evFile2 == null)) {
			System.out.println("In compareEventFiles, one or both files are null.");
			return false;
		}

		if (!evFile1.exists() || !evFile2.exists()) {
			System.out.println("In compareEventFiles, one or both files do not exist.");
			return false;
		}

		if (evFile1.isDirectory() || evFile2.isDirectory()) {
			System.out.println("In compareEventFiles, one or both files is a directory.");
			return false;
		}

		if (!evFile1.canRead() || !evFile2.canRead()) {
			System.out.println("In compareEventFiles, one or both files cannot be read.");
			return false;
		}

		String name1 = evFile1.getName();
		String name2 = evFile2.getName();

		long size1 = evFile1.length();
		long size2 = evFile2.length();

		if (size1 == size2) {
			System.out.println(name1 + " and " + name2 + " have the same length: " + size1);
		}
		else {
			System.out.println(name1 + " and " + name2 + " have the different lengths.");
			System.out.println(name1 + ": " + size1);
			System.out.println(name2 + ": " + size2);
			return false;
		}

		try {
			EvioReader evioFile1 = new EvioReader(evFile1);
			EvioReader evioFile2 = new EvioReader(evFile2);
			int evCount1 = evioFile1.getEventCount();
			int evCount2 = evioFile2.getEventCount();
			if (evCount1 == evCount2) {
				System.out.println(name1 + " and " + name2 + " have the same #events: " + evCount1);
			}
			else {
				System.out.println(name1 + " and " + name2 + " have the different #events.");
				System.out.println(name1 + ": " + evCount1);
				System.out.println(name2 + ": " + evCount2);
				return false;
			}
		}
        catch (EvioException e) {
            e.printStackTrace();
            return false;
        }
        catch (IOException e) {
            e.printStackTrace();
            return false;
        }


		System.out.print("Byte by byte comparison...");
		System.out.flush();

		int onetenth = (int)(1 + size1/10);

		//now a byte-by-byte comparison
		try {
			FileInputStream fis1 = new FileInputStream(evFile1);
			FileInputStream fis2 = new FileInputStream(evFile1);

			for (int i = 0; i < size1; i++) {
				try {
					int byte1 = fis1.read();
					int byte2 = fis2.read();

					if (byte1 != byte2) {
						System.out.println(name1 + " and " + name2 + " different at byte offset: " + i);
						return false;
					}

					if ((i % onetenth) == 0) {
						System.out.print(".");
						System.out.flush();
					}
				}
				catch (IOException e) {
					e.printStackTrace();
					return false;
				}
			}

			System.out.println("");

			try {
				fis1.close();
				fis2.close();
			}
			catch (IOException e) {
				e.printStackTrace();
			}
		}
		catch (FileNotFoundException e) {
			e.printStackTrace();
		}


		System.out.println("files " + name1 + " and " + evFile2.getPath() + " are identical.");
		return true;
	}

}