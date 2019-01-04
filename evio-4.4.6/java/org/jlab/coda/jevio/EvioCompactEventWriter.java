package org.jlab.coda.jevio;

import java.io.*;
import java.nio.*;

/**
 * This class is used to collect multiple buffers, each containing the bytes
 * of a single evio event (<b>NOT</b> a full evio version 4 formatted buffer).
 * It can write the accumulated events into a properly formatted evio version 4
 * file or buffer.
 * It is theoretically thread-safe.
 * It is designed to be fast and does <b>NOT</b> do a deserialization
 * on the buffers collected.<p>
 *
 * When this class was originally written, although much of the code was the same as
 * the {@link EventWriter} class, it was different enough not to be able to easily merge
 * the two. However, since then things have changed. It is now rewritten to be a wrapper
 * around EventWriter. Maintain this class only for backward compatibility.
 *
 * @author timmer
 * @deprecated use EvioWriter instead (warning: the constructor arguments are slightly
 *             different in EvioWriter).
 */
public class EvioCompactEventWriter {

    /** Wrap this object and use it to do everything. */
    private EventWriter writer;


    /**
     * Create an <code>EvioCompactEventWriter</code> object for writing events to a file.
     * It receives buffers containing evio event data (just the event data and <b>not</b> a
     * full evio 4 format) and places them into an internal buffer in full evio 4 format.
     * It writes out the contents of its internal buffer into a file whenever that buffer
     * becomes full. If the split arg is > 0, when the file size reaches the given limit,
     * it is closed and another created. If the file already exists, its contents will be
     * overwritten unless the "overWriteOK" argument is <code>false</code> in
     * which case an exception will be thrown. If the file doesn't exist,
     * it will be created. Byte order defaults to big endian if arg is null.
     * File writing can be terminated by calling {@link #close()} which will flush all
     * remaining data to the file, close it, and disable this object from any more
     * writing.
     *
     * @param baseName      base file name used to generate complete file name (may not be null)
     * @param directory     directory in which file is to be placed
     * @param runNumber     number of the CODA run, used in naming files
     * @param split         if < 1, do not split file, write to only one file of unlimited size.
     *                      Else this is max size (in units of integral # of buffers of maxBuffersize
     *                      bytes each) to make a file before closing it and starting writing another.
     * @param bufferSize    number of bytes to make the internal buffer which will
     *                      be storing events before writing them to a file.
     * @param byteOrder     the byte order in which to write the file.
     * @param xmlDictionary dictionary in xml format or null if none.
     *
     * @throws EvioException if baseName is null;
     *                       if bufferSize too small;
     *                       if file exists but user requested no over-writing;
     *
     */
    public EvioCompactEventWriter(String baseName, String directory, int runNumber, int split,
                                  int bufferSize, ByteOrder byteOrder,
                                  String xmlDictionary)
            throws EvioException {

        // Block size maximum is 100MB or 100k events, whichever is limit is reached first
        writer = new EventWriter(baseName, directory, null, runNumber, split,
                                 EventWriter.DEFAULT_BLOCK_SIZE, // ints
                                 EventWriter.DEFAULT_BLOCK_COUNT,
                                 bufferSize, byteOrder, xmlDictionary,
                                 null, false, false);


        }


    /**
     * Create an <code>EvioCompactEventWriter</code> object for writing events to a file.
     * It receives buffers containing evio event data (just the event data and <b>not</b> a
     * full evio 4 format) and places them into an internal buffer in full evio 4 format.
     * It writes out the contents of its internal buffer into a file whenever that buffer
     * becomes full. If the split arg is > 0, when the file size reaches the given limit,
     * it is closed and another created. If the file already exists, its contents will be
     * overwritten unless the "overWriteOK" argument is <code>false</code> in
     * which case an exception will be thrown. If the file doesn't exist,
     * it will be created. Byte order defaults to big endian if arg is null.
     * File writing can be terminated by calling {@link #close()} which will flush all
     * remaining data to the file, close it, and disable this object from any more
     * writing.<p>
     * Maintain this constructor for backwards compatibility.
     *
     * @param baseName      base file name used to generate complete file name (may not be null)
     * @param directory     directory in which file is to be placed
     * @param runNumber     number of the CODA run, used in naming files
     * @param split         if < 1, do not split file, write to only one file of unlimited size.
     *                      Else this is max size in bytes (+/- blockSizeMax) to make a file
     *                      before closing it and starting writing another.
     * @param blockSizeMax  the max blocksize in bytes which must be >= 400 B and <= 4 MB.
     *                      The size of the block will not be larger than this size
     *                      unless a single event itself is larger.
     * @param blockCountMax the max number of events in a single block which must be
     *                      >= {@link EventWriter#MIN_BLOCK_COUNT} and
     *                      <= {@link EventWriter#MAX_BLOCK_COUNT}.
     * @param bufferSize    number of bytes to make the internal buffer which will
     *                      be storing events before writing them to a file. Must be at least
     *                      blockSizeMax. If not, it is set to that.
     * @param byteOrder     the byte order in which to write the file.
     * @param xmlDictionary dictionary in xml format or null if none.
     * @param overWriteOK   if <code>false</code> and the file already exists,
     *                      an exception is thrown rather than overwriting it.
     *
     * @throws EvioException if baseName is null;
     *                       if blockSizeMax or blockCountMax exceed limits;
     *                       if bufferSize too small;
     *                       if file exists but user requested no over-writing;
     */
    public EvioCompactEventWriter(String baseName, String directory, int runNumber, int split,
                                  int blockSizeMax, int blockCountMax, int bufferSize,
                                  ByteOrder byteOrder, String xmlDictionary,
                                  boolean overWriteOK)
            throws EvioException {

        writer = new EventWriter(baseName, directory, null, runNumber, split,
                blockSizeMax/4, blockCountMax,
                bufferSize, byteOrder, xmlDictionary,
                null, overWriteOK, false);
    }


    /**
     * Create an <code>EvioCompactEventWriter</code> object for writing events to a file.
     * It receives buffers containing evio event data (just the event data and <b>not</b> a
     * full evio 4 format) and places them into an internal buffer in full evio 4 format.
     * It writes out the contents of its internal buffer into a file whenever that buffer
     * becomes full. If the split arg is > 0, when the file size reaches the given limit,
     * it is closed and another created. If the file already exists, its contents will be
     * overwritten unless the "overWriteOK" argument is <code>false</code> in
     * which case an exception will be thrown. If the file doesn't exist,
     * it will be created. Byte order defaults to big endian if arg is null.
     * File writing can be terminated by calling {@link #close()} which will flush all
     * remaining data to the file, close it, and disable this object from any more
     * writing.
     *
     * @param baseName      base file name used to generate complete file name (may not be null)
     * @param directory     directory in which file is to be placed
     * @param runType       name of run type configuration to be used in naming files
     * @param runNumber     number of the CODA run, used in naming files
     * @param split         if < 1, do not split file, write to only one file of unlimited size.
     *                      Else this is max size in bytes (+/- blockSizeMax) to make a file
     *                      before closing it and starting writing another.
     * @param blockSizeMax  the max blocksize in bytes which must be >= 400 B and <= 4 MB.
     *                      The size of the block will not be larger than this size
     *                      unless a single event itself is larger.
     * @param blockCountMax the max number of events in a single block which must be
     *                      >= {@link EventWriter#MIN_BLOCK_COUNT} and
     *                      <= {@link EventWriter#MAX_BLOCK_COUNT}.
     * @param bufferSize    number of bytes to make the internal buffer which will
     *                      be storing events before writing them to a file. Must be at least
     *                      blockSizeMax. If not, it is set to that.
     * @param byteOrder     the byte order in which to write the file.
     * @param xmlDictionary dictionary in xml format or null if none.
     * @param overWriteOK   if <code>false</code> and the file already exists,
     *                      an exception is thrown rather than overwriting it.
     *
     * @throws EvioException if baseName is null;
     *                       if blockSizeMax or blockCountMax exceed limits;
     *                       if bufferSize too small;
     *                       if file exists but user requested no over-writing;
     */
    public EvioCompactEventWriter(String baseName, String directory, String runType,
                                  int runNumber, int split,
                                  int blockSizeMax, int blockCountMax, int bufferSize,
                                  ByteOrder byteOrder, String xmlDictionary,
                                  boolean overWriteOK)
                    throws EvioException {

        writer = new EventWriter(baseName, directory, runType, runNumber, split,
                                 blockSizeMax/4, blockCountMax,
                                 bufferSize, byteOrder, xmlDictionary,
                                 null, overWriteOK, false);
    }


    /**
     * Create an <code>EvioCompactEventWriter</code> object for writing events to the
     * given buffer. Will overwrite any existing data in buffer!
     * It receives buffers containing evio event data (just the event data and <b>not</b> a
     * full evio 4 format) and places them into the given buffer in full evio 4 format.
     * Byte order of events to be written must match the order of the given buffer.
     * Writing can be terminated by calling {@link #close()} which will flush all
     * remaining data to the buffer and disable this object from any more writing.
     *
     * @param byteBuffer    buffer into which events are written.
     * @param blockSizeMax  the max blocksize in bytes which must be >= 400 B and <= 4 MB.
     *                      The size of the block will not be larger than this size
     *                      unless a single event itself is larger.
     * @param blockCountMax the max number of events in a single block which must be
     *                      >= {@link EventWriter#MIN_BLOCK_COUNT} and <= {@link EventWriter#MAX_BLOCK_COUNT}.
     * @param xmlDictionary dictionary in xml format or null if none.
     *
     * @throws EvioException if blockSizeMax or blockCountMax exceed limits;
     *                       if buffer arg is null
     */
    public EvioCompactEventWriter(ByteBuffer byteBuffer, int blockSizeMax,
                                  int blockCountMax, String xmlDictionary)
            throws EvioException {


        writer = new EventWriter(byteBuffer, blockSizeMax/4, blockCountMax, xmlDictionary, null);
    }


    /**
     * Get the number of events written to a file/buffer.
     * Remember that a particular event may not yet be
     * flushed to the file/buffer.
     * If the file being written to is split, the returned
     * value refers to all split files taken together.
     *
     * @return number of events written to a file/buffer.
     */
    public int getEventsWritten() {
        return writer.getEventsWritten();
    }


    /**
     * Set the number with which to start block numbers.
     * This method does nothing if events have already been written.
     * @param startingBlockNumber  the number with which to start block numbers.
     */
    public void setStartingBlockNumber(int startingBlockNumber) {
        writer.setStartingBlockNumber(startingBlockNumber);
    }


    /** This method flushes any remaining data to file and disables this object. */
    public void close() {
        writer.close();
    }


    /**
     * Write an event (bank) to the buffer in evio version 4 format.
     * The given event buffer must contain only the event's data (event header
     * and event data) and must <b>not</b> be in complete evio format.
     * If the buffer is full, it will be flushed to the file if writing to a file.
     * Otherwise an exception will be thrown.
     *
     * @param eventBuffer the event (bank) to write in buffer form
     * @throws EvioException if event is opposite byte order of internal buffer;
     *                       if close() already called;
     *                       if bad eventBuffer format;
     *                       if file could not be opened for writing;
     *                       if file exists but user requested no over-writing;
     *                       if no room when writing to user-given buffer;
     * @throws IOException   if error writing file
     */
    public void writeEvent(ByteBuffer eventBuffer)
            throws EvioException, IOException {

        writer.writeEvent(eventBuffer);
    }


    /**
     * If writing to a file, return null.
     * If writing to a buffer, get a duplicate of the user-given buffer
     * being written into. The buffer's position will be 0.
     * It's limit will be the size of the valid data. Basically, it's
     * ready to be read from. The returned buffer shares data with the
     * original buffer but has separate limit, position, and mark.
     * This will be useful if creating a single buffer to be sent over
     * the network.
     *
     * @return duplicated byte buffer.
     */
    public ByteBuffer getByteBuffer() {
        return writer.getByteBuffer();
    }

}