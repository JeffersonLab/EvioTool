package org.jlab.coda.jevio;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * This holds an evio block header, also known as a physical record header.
 * Unfortunately, in versions 1, 2 & 3, evio files impose an anachronistic
 * block structure. The complication that arises is that logical records
 * (events) will sometimes cross physical record boundaries.
 *
 *
 * <code><pre>
 * ####################################
 * Evio block header, versions 1,2 & 3:
 * ####################################
 *
 * MSB(31)                          LSB(0)
 * <---  32 bits ------------------------>
 * _______________________________________
 * |            Block Length             |
 * |_____________________________________|
 * |            Block Number             |
 * |_____________________________________|
 * |          Header Length = 8          |
 * |_____________________________________|
 * |               Start                 |
 * |_____________________________________|
 * |                End                  |
 * |_____________________________________|
 * |              Version                |
 * |_____________________________________|
 * |             Reserved 1              |
 * |_____________________________________|
 * |            Magic Number             |
 * |_____________________________________|
 *
 *
 *      Block Length  = number of ints in block (including this one).
 *                      This is fixed for versions 1-3, generally at 8192 (32768 bytes)
 *      Block Number  = id number (starting at 0)
 *      Header Length = number of ints in this header (always 8)
 *      Start         = offset to first event header in block relative to start of block
 *      End           = # of valid words (header + data) in block (normally = block size)
 *      Version       = evio format version
 *      Reserved 1    = reserved
 *      Magic #       = magic number (0xc0da0100) used to check endianness
 *
 * </pre></code>
 *
 *
 * @author heddle
 *
 */
public class BlockHeaderV2 implements IEvioWriter, IBlockHeader {

	/**
	 * The maximum block size in 32 bit ints in this implementation of evio.
     * There is, in actuality, no limit on size; however, the versions 1-3 C
     * library only used 8192 as the block size.
	 */
	public static final int MAX_BLOCK_SIZE = 32768;

	/**
	 * The block (physical record) size in 32 bit ints.
	 */
	private int size;

	/**
	 * The block number. In a file, this is usually sequential.
	 */
	private int number;

	/**
	 * The block header length. Should be 8 in all cases, so getting this correct constitutes a check.
	 */
	private int headerLength;

	/**
     * Offset (in ints, relative to start of block) to the start of the first event (logical record) that begins in this
     * block. For the first event it will just be = 8, the size of the block header. For subsequent physical records it
     * will generally not be 8. Note that a logical record (event) that spans three blocks (physical records) will have
     * <code>start = 0</code>.
     *
	 */
	private int start;

	/**
	 * The number of valid words (header + data) in the block (physical record.) This is normally the same as the block
	 * size, except for the last block (physical record) in the file. <br>
	 * NOTE: for evio files, even if end < size (blocksize) for the last block (physical record), the data behind it
	 * will be padded with zeroes so that the file size is an integer multiple of the block size.
	 */
	private int end;

	/**
	 * The evio version.
	 */
	private int version;

	/**
	 * First reserved word. Sometimes this is used to indicate the ordinal number of the last event that starts
	 * within this block--but that is not mandated. In that case, if the previous block had a value of
	 * reserved1 = 6 and this block has a value of 9, then this block contains the end of event 6, all of events
	 * 7 and 8, and the start of event 9--unless it ends exactly on the end of event 8.<br>
	 */
	private int reserved1;

	/**
	 * This is the magic word: 0xc0da0100 (formerly reserved2). Used to check endianness.
	 */
	private int magicNumber;

    /** This is the byte order of the data being read. */
    private ByteOrder byteOrder;

	/**
	 * This is not part of the block header proper. It is a position in a memory buffer of the start of the block
	 * (physical record). It is kept for convenience.
	 */
	private long bufferStartingPosition = -1L;

    /**
	 * Get the size of the block (physical record).
	 *
	 * @return the size of the block (physical record) in ints.
	 */
	public int getSize() {
		return size;
	}

	/**
	 * Null constructor initializes all fields to zero.
	 */
	public BlockHeaderV2() {
		size = 0;
		number = 0;
		headerLength = 0;
		start = 0;
		end = 0;
		version = 0;
		reserved1 = 0;
		magicNumber = 0;
	}

    /**
     * Creates a BlockHeader for evio versions 1-3 format. Only the <code>block size</code>
     * and <code>block number</code> are provided. The other six words, which can be
     * modified by setters, are initialized to these values:<br>
     *<ul>
     *<li><code>headerLength</code> is initialized to 8<br>
     *<li><code>start</code> is initialized to 8<br>
     *<li><code>end</code> is initialized to <code>size</code><br>
     *<li><code>version</code> is initialized to 2<br>
     *<li><code>reserved1</code> is initialized to 0<br>
     *<li><code>magicNumber</code> is initialized to <code>MAGIC_NUMBER</code><br>
     *</ul>
     * @param size the size of the block in ints.
     * @param number the block number--usually sequential.
     */
    public BlockHeaderV2(int size, int number) {
        this.size = size;
        this.number = number;
        headerLength = 8;
        start = 8;
        end = size;
        version = 2;
        reserved1 = 0;
        magicNumber = MAGIC_NUMBER;
    }


    /**
     * This copy constructor creates an evio version 1-3 BlockHeader
     * from another object of this class.
     * @param blkHeader block header object to copy
     */
    public BlockHeaderV2(BlockHeaderV2 blkHeader) {
        if (blkHeader == null) {
            return;
        }
        size         = blkHeader.size;
        number       = blkHeader.number;
        headerLength = blkHeader.headerLength;
        version      = blkHeader.version;
        end          = blkHeader.end;
        start        = blkHeader.start;
        reserved1    = blkHeader.reserved1;
        magicNumber  = blkHeader.magicNumber;
        bufferStartingPosition = blkHeader.bufferStartingPosition;
    }

    /*** {@inheritDoc}  */
    public Object clone() {
        return new BlockHeaderV2(this);
    }

    /**
     * Gets whether this block's first event is an evio dictionary.
     * This is not implemented in evio versions 1-3. Just return false.
     *
     * @return always returns false for evio versions 1-3
     */
    public boolean hasDictionary() {
        return false;
    }

    /**
     * Is this the last block in the file or being sent over the network?
     * This is not implemented in evio versions 1-3. Just return false.
     *
     * @return always returns false for evio versions 1-3
     */
    public boolean isLastBlock() {
        return false;
    }


	/**
	 * Set the size of the block (physical record). Some trivial checking is done.
	 *
	 * @param size the new value for the size, in ints.
	 * @throws org.jlab.coda.jevio.EvioException
	 */
	public void setSize(int size) throws EvioException {
        if ((size < 8) || (size > MAX_BLOCK_SIZE)) {
			throw new EvioException(String.format("Bad value for size in block (physical record) header: %d", size));
		}

        // I'm not sure why this restriction is in here - timmer
        if ((size % 256) != 0) {
            throw new EvioException(String.format(
                    "Bad value for size in block (physical record) header: %d (must be multiple of 256 ints)", size));
        }
        this.size = size;
	}


    /**
     * Get the starting position of the block (physical record.). This is the offset (in ints, relative to start of
     * block) to the start of the first event (logical record) that begins in this block. For the first event it will
     * just be = 8, the size of the block header. For subsequent blocks it will generally not be 8. Note that a
     * an event that spans three blocks (physical records) will have <code>start = 0</code>.
     *
     * NOTE: a logical record (event) that spans three blocks (physical records) will have <code>start = 0</code>.
     *
     * @return the starting position of the block (physical record.)
     */
    public int getStart() {
        return start;
    }

    /**
     * Set the starting position of the block (physical record.). This is the offset (in ints, relative to start of
     * block) to the start of the first event (logical record) that begins in this block. For the first event it will
     * just be = 8, the size of the block header. For subsequent blocks it will generally not be 8. Some trivial
     * checking is done. Note that an event that spans three blocks (physical records) will have
     * <code>start = 0</code>.
     *
     * NOTE: a logical record (event) that spans three blocks (physical records) will have <code>start = 0</code>.
     *
     * @param start the new value for the start.
     * @throws EvioException
     */
    public void setStart(int start) throws EvioException {
        if ((start < 0) || (start > MAX_BLOCK_SIZE)) {
            throw new EvioException(String.format("Bad value for start in block (physical record) header: %d", start));
        }
        this.start = start;
    }

	/**
	 * Get the ending position of the block (physical record.) This is the number of valid words (header + data) in the
	 * block (physical record.) This is normally the same as the block size, except for the last block (physical record)
	 * in the file.<br>
	 * NOTE: for evio files, even if end < size (blocksize) for the last block (physical record), the data behind it
	 * will be padded with zeroes so that the file size is an integer multiple of the block size.
	 *
	 * @return the ending position of the block (physical record.)
	 */
	public int getEnd() {
		return end;
	}

	/**
	 * Set the ending position of the block (physical record.) This is the number of valid words (header + data) in the
	 * block (physical record.) This is normally the same as the block size, except for the last block (physical record)
	 * in the file. Some trivial checking is done.<br>
	 * NOTE: for evio files, even if end < size (blocksize) for the last block (physical record), the data behind it
	 * will be padded with zeroes so that the file size is an integer multiple of the block size.
	 *
	 * @param end the new value for the end.
	 * @throws EvioException
	 */
	public void setEnd(int end) throws EvioException {
		if ((end < 8) || (end > MAX_BLOCK_SIZE)) {
			throw new EvioException(String.format("Bad value for end in block (physical record) header: %d", end));
		}
		this.end = end;
	}

	/**
	 * Get the block number for this block (physical record). In a file, this is usually sequential.
	 *
	 * @return the block number for this block (physical record).
	 */
	public int getNumber() {
		return number;
	}

	/**
	 * Set the block number for this block (physical record). In a file, this is usually sequential. This is not
	 * checked.
	 *
	 * @param number the number of the block (physical record).
	 */
	public void setNumber(int number) {
		this.number = number;
	}

	/**
	 * Get the block header length, in ints. This should be 8.
	 *
	 * @return the block header length. This should be 8.
	 */
	public int getHeaderLength() {
		return headerLength;
	}

	/**
	 * Set the block header length, in ints. This should be 8. However, since this is usually read as part of reading
	 * the physical record header, it is a good check to have a setter rather than just fix its value at 8.
	 *
	 * param headerLength the new block header length. This should be 8.
	 *
	 * @throws EvioException if headerLength is not 8.
	 */
	public void setHeaderLength(int headerLength) throws EvioException {
		if (headerLength != 8) {
			String message = "Bad Block (Physical Record) Header Length: " + headerLength;
			throw new EvioException(message);
		}
		this.headerLength = headerLength;
	}

	/**
	 * Get the evio version of the block (physical record) header.
	 *
	 * @return the evio version of the block (physical record) header.
	 */
	public int getVersion() {
		return version;
	}

    /**
     * Sets the evio version. Should be 1, 2 or 3 but no check is performed here.
     *
     * @param version the evio version of evio.
     */
    public void setVersion(int version) {
        this.version = version;
    }

	/**
	 * Get the first reserved word in the block (physical record) header. Used in evio versions 1-3 only.
	 *
	 * @return the first reserved word in the block (physical record). Used in evio versions 1-3 only.
	 */
	public int getReserved1() {
		return reserved1;
	}

	/**
	 * Sets the value of reserved 1.
	 *
	 * @param reserved1 the value for reserved1.
	 */
	public void setReserved1(int reserved1) {
		this.reserved1 = reserved1;
	}

	/**
	 * Get the magic number the block (physical record) header which should be 0xc0da0100.
     * Formerly this location in the header was called "reserved2".
	 *
	 * @return the magic number in the block (physical record).
	 */
	public int getMagicNumber() {
		return magicNumber;
	}

	/**
	 * Sets the value of magicNumber. This should match the constant MAGIC_NUMBER.
     * If it doesn't, some obvious possibilities: <br>
	 * 1) The evio data (perhaps from a file) is screwed up.<br>
	 * 2) The reading algorithm is screwed up. <br>
	 * 3) The endianess is not being handled properly.
	 *
	 * @param magicNumber the new value for magic number.
	 * @throws EvioException
	 */
	public void setMagicNumber(int magicNumber) throws EvioException {
		if (magicNumber != MAGIC_NUMBER) {
			throw new EvioException(String.format("Value for magicNumber %8x does not match MAGIC_NUMBER 0xc0da0100.",
                                                  magicNumber));
		}
		this.magicNumber = magicNumber;
	}

    /** {@inheritDoc} */
    public ByteOrder getByteOrder() {return byteOrder;}

    /**
   	 * Sets the byte order of data being read.
   	 *
   	 * @param byteOrder the new value for data's byte order.
   	 */
   	public void setByteOrder(ByteOrder byteOrder) {
   		this.byteOrder = byteOrder;
   	}

	/**
	 * Obtain a string representation of the block (physical record) header.
	 *
	 * @return a string representation of the block (physical record) header.
	 */
	@Override
	public String toString() {
		StringBuffer sb = new StringBuffer(512);
		sb.append(String.format("block size:    %d\n", size));
		sb.append(String.format("number:        %d\n", number));
		sb.append(String.format("headerLen:     %d\n", headerLength));
		sb.append(String.format("start:         %d\n", start));
		sb.append(String.format("end:           %d\n", end));
		sb.append(String.format("version:       %d\n", version));
		sb.append(String.format("reserved1:     %d\n", reserved1));
		sb.append(String.format("magicNumber:   %8x\n", magicNumber));
		sb.append(String.format(" *buffer start:  %d\n", getBufferStartingPosition()));
		sb.append(String.format(" *next   start:  %d\n", nextBufferStartingPosition()));
		return sb.toString();
	}

    /**
     * Get the position in the buffer (in bytes) of this block's last data word.<br>
     *
     * @return the position in the buffer (in bytes) of this block's last data word.
     */
    public long getBufferEndingPosition() {
        return bufferStartingPosition + 4*end;
    }

    /**
     * Get the starting position in the buffer (in bytes) from which this header was read--if that happened.<br>
     * This is not part of the block header proper. It is a position in a memory buffer of the start of the block
     * (physical record). It is kept for convenience. It is up to the reader to set it.
     *
     * @return the starting position in the buffer (in bytes) from which this header was read--if that happened.
     */
    public long getBufferStartingPosition() {
        return bufferStartingPosition;
    }

	/**
	 * Set the starting position in the buffer (in bytes) from which this header was read--if that happened.<br>
	 * This is not part of the block header proper. It is a position in a memory buffer of the start of the block
	 * (physical record). It is kept for convenience. It is up to the reader to set it.
	 *
	 * @param bufferStartingPosition the starting position in the buffer from which this header was read--if that
	 *            happened.
	 */
	public void setBufferStartingPosition(long bufferStartingPosition) {
		this.bufferStartingPosition = bufferStartingPosition;
	}

	/**
	 * Determines where the start of the next block (physical record) header in some buffer is located (in bytes).
     * This assumes the start position has been maintained by the object performing the buffer read.
	 *
	 * @return the start of the next block (physical record) header in some buffer is located (in bytes).
	 */
	public long nextBufferStartingPosition() {
		return bufferStartingPosition + 4*size;
	}

	/**
	 * Determines where the start of the first event (logical record) in this block (physical record) is located
     * (in bytes). This assumes the start position has been maintained by the object performing the buffer read.
	 *
	 * @return where the start of the first event (logical record) in this block (physical record) is located
     *         (in bytes). Returns 0 if start is 0, signaling that this entire physical record is part of a
     *         logical record that spans at least three physical records.
	 */
	public long firstEventStartingPosition() {
        if (start == 0) {
			return 0L;
		}
		return bufferStartingPosition + 4*start;
	}

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
	public int bytesRemaining(long position) throws EvioException {
		if (position < bufferStartingPosition) {
			throw new EvioException("Provided position is less than buffer starting position.");
		}

		long nextBufferStart = nextBufferStartingPosition();
//System.out.println("bytesRemaining: position = " + position + ", next buffer start = " + nextBufferStart);
		if (position > nextBufferStart) {
			throw new EvioException("Provided position beyond buffer end position.");
		}

		return (int)(nextBufferStart - position);
	}

	/**
	 * Write myself out a byte buffer. This write is relative--i.e., it uses the current position of the buffer.
	 *
	 * @param byteBuffer the byteBuffer to write to.
	 * @return the number of bytes written, which for a BlockHeader is 32.
	 */
	public int write(ByteBuffer byteBuffer) {
		byteBuffer.putInt(size);
		byteBuffer.putInt(number);
		byteBuffer.putInt(headerLength); // should always be 8
		byteBuffer.putInt(start);
		byteBuffer.putInt(end);
		byteBuffer.putInt(version);
		byteBuffer.putInt(reserved1);
		byteBuffer.putInt(magicNumber);
		return 32;
	}
}