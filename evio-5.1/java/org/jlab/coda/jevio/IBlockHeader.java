package org.jlab.coda.jevio;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Make a common interface for different versions of the BlockHeader.
 *
 * @author timmer
 */
public interface IBlockHeader {
    /**
     * The magic number, should be the value of <code>magicNumber</code>.
     */
    public static final int MAGIC_NUMBER = 0xc0da0100;

    /**
	 * Get the size of the block (physical record).
	 *
	 * @return the size of the block (physical record) in ints.
	 */
    int getSize();

    /**
	 * Get the block number for this block (physical record).
     * In a file, this is usually sequential.
     *
     * @return the block number for this block (physical record).
     */
    int getNumber();

    /**
	 * Get the block header length, in ints. This should be 8.
     *
     * @return the block header length. This should be 8.
     */
    int getHeaderLength();

    /**
     * Get the evio version of the block (physical record) header.
     *
     * @return the evio version of the block (physical record) header.
     */
    int getVersion();

    /**
	 * Get the magic number the block (physical record) header which should be 0xc0da0100.
     *
     * @return the magic number in the block (physical record).
     */
    int getMagicNumber();

    /**
	 * Get the byte order of the data being read.
     *
     * @return the byte order of the data being read.
     */
    ByteOrder getByteOrder();

    /**
     * Get the position in the buffer (in bytes) of this block's last data word.<br>
     *
     * @return the position in the buffer (in bytes) of this block's last data word.
     */
    long getBufferEndingPosition();

    /**
     * Get the starting position in the buffer (in bytes) from which this header was read--if that happened.<br>
     * This is not part of the block header proper. It is a position in a memory buffer of the start of the block
     * (physical record). It is kept for convenience. It is up to the reader to set it.
     *
     * @return the starting position in the buffer (in bytes) from which this header was read--if that happened.
     */
    long getBufferStartingPosition();

    /**
	 * Set the starting position in the buffer (in bytes) from which this header was read--if that happened.<br>
     * This is not part of the block header proper. It is a position in a memory buffer of the start of the block
     * (physical record). It is kept for convenience. It is up to the reader to set it.
     *
     * @param bufferStartingPosition the starting position in the buffer from which this header was read--if that
     *            happened.
     */
    void setBufferStartingPosition(long bufferStartingPosition);

    /**
	 * Determines where the start of the next block (physical record) header in some buffer is located (in bytes).
     * This assumes the start position has been maintained by the object performing the buffer read.
     *
     * @return the start of the next block (physical record) header in some buffer is located (in bytes).
     */
    long nextBufferStartingPosition();

    /**
	 * Determines where the start of the first event (logical record) in this block (physical record) is located
     * (in bytes). This assumes the start position has been maintained by the object performing the buffer read.
     *
     * @return where the start of the first event (logical record) in this block (physical record) is located
     *         (in bytes). Returns 0 if start is 0, signaling that this entire physical record is part of a
     *         logical record that spans at least three physical records.
     */
    long firstEventStartingPosition();

    /**
	 * Gives the bytes remaining in this block (physical record) given a buffer position. The position is an absolute
     * position in a byte buffer. This assumes that the absolute position in <code>bufferStartingPosition</code> is
     * being maintained properly by the reader. No block is longer than 2.1GB - 31 bits of length. This is for
     * practical reasons - so a block can be read into a single byte array.
     *
     * @param position the absolute current position is a byte buffer.
     * @return the number of bytes remaining in this block (physical record.)
     * @throws EvioException
     */
    int bytesRemaining(long position) throws EvioException;

    /**
     * Is this block's first event is an evio dictionary?
     *
     * @return <code>true</code> if this block's first event is an evio dictionary, else <code>false</code>
     */
    public boolean hasDictionary();

    /**
     * Is this the last block in the file or being sent over the network?
     *
     * @return <code>true</code> if this is the last block in the file or being sent
     *         over the network, else <code>false</code>
     */
    public boolean isLastBlock();

    /**
	 * Write myself out a byte buffer. This write is relative--i.e., it uses the current position of the buffer.
     *
     * @param byteBuffer the byteBuffer to write to.
     * @return the number of bytes written, which for a BlockHeader is 32.
     */
    int write(ByteBuffer byteBuffer);

    /**
     * Obtain a string representation of the block (physical record) header.
     *
     * @return a string representation of the block (physical record) header.
     */
    @Override
    public String toString();
}
