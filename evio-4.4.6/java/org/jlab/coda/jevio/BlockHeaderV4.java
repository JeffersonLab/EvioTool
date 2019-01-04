package org.jlab.coda.jevio;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.BitSet;

/**
 * This holds an evio block header, also known as a physical record header.
 * Unfortunately, in versions 1, 2 & 3, evio files impose an anachronistic
 * block structure. The complication that arises is that logical records
 * (events) will sometimes cross physical record boundaries. This block structure
 * is changed in version 4 so that blocks only contain integral numbers of events.
 * The information stored in this block header has also changed.
 *
 *
 * <code><pre>
 * ################################
 * Evio block header, version 4:
 * ################################
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
 * |             Event Count             |
 * |_____________________________________|
 * |             reserved 1              |
 * |_____________________________________|
 * |          Bit info & Version         |
 * |_____________________________________|
 * |             reserved 2              |
 * |_____________________________________|
 * |             Magic Int               |
 * |_____________________________________|
 *
 *
 *      Block Length       = number of ints in block (including this one).
 *      Block Number       = id number (starting at 1)
 *      Header Length      = number of ints in this header (8)
 *      Event Count        = number of events in this block (always an integral #).
 *                           NOTE: this value should not be used to parse the following
 *                           events since the first block may have a dictionary whose
 *                           presence is not included in this count.
 *      Reserved 1         = If bits 2-5 in bit info are RocRaw (1), then (in the first block)
 *                           this contains the CODA id of the source
 *      Bit info & Version = Lowest 8 bits are the version number (4).
 *                           Upper 24 bits contain bit info.
 *                           If a dictionary is included as the first event, bit #9 is set (=1)
 *                           If a last block, bit #10 is set (=1)
 *      Magic Int          = magic number (0xc0da0100) used to check endianness
 *
 *
 *
 * Bit info has the following bits defined (bit #s start with 1):
 *   Bit  9     = true if dictionary is included (relevant for first block only)
 *
 *   Bit  10    = true if this block is the last block in file or network transmission
 *
 *   Bits 11-14 = type of events following (ROC Raw = 0, Physics = 1, PartialPhysics = 2,
 *                DisentangledPhysics = 3, User = 4, Control = 5, Prestart = 6, Go = 7,
 *                Pause = 8, End = 9, Other = 15).
 *
 *                This is useful ONLY for the CODA online use of evio.
 *                That's because only a single CODA event type is placed into
 *                a single ET buffer. That ET buffer then is parsed by an EvioReader or
 *                EvioCompactReader object. Thus all events will be of a single CODA type.
 *
 *
 *
 * </pre></code>
 *
 *
 * @author heddle
 * @author timmer
 *
 */
public class BlockHeaderV4 implements Cloneable, IEvioWriter, IBlockHeader {

    /** The minimum & expected block header size in 32 bit ints. */
    public static final int HEADER_SIZE = 8;

    /** Dictionary presence is 9th bit in version/info word */
    public static final int EV_DICTIONARY_MASK = 0x100;

    /** "Last block" is 10th bit in version/info word */
    public static final int EV_LASTBLOCK_MASK  = 0x200;

    /** Position of word for size of block in 32-bit words. */
    public static final int EV_BLOCKSIZE = 0;
    /** Position of word for block number, starting at 1. */
    public static final int EV_BLOCKNUM = 1;
    /** Position of word for size of header in 32-bit words (=8). */
    public static final int EV_HEADERSIZE = 2;
    /** Position of word for number of events in block. */
    public static final int EV_COUNT = 3;
    /** Position of word for reserved. */
    public static final int EV_RESERVED1 = 4;
    /** Position of word for version of file format. */
    public static final int EV_VERSION = 5;
    /** Position of word for reserved. */
    public static final int EV_RESERVED2 = 6;
    /** Position of word for magic number for endianness tracking. */
    public static final int EV_MAGIC = 7;




	/** The block (physical record) size in 32 bit ints. */
	private int size;

	/** The block number. In a file, this is usually sequential. */
	private int number;

	/** The block header length which is always 8. */
	private int headerLength;

    /**
     * Since blocks only contain whole events in this version,
     * this stores the number of events contained in a block.
     */
    private int eventCount;

	/** The evio version which is always 4. */
	private int version;

    /** Value of first reserved word. */
    private int reserved1;

    /** Value of second reserved word. */
    private int reserved2;

    /** Bit information. Bit one: is the first event a dictionary? */
    private BitSet bitInfo = new BitSet(24);

	/** This is the magic word, 0xc0da0100, used to check endianness. */
	private int magicNumber;

    /** This is the byte order of the data being read. */
    private ByteOrder byteOrder;

	/**
	 * This is not part of the block header proper. It is a position in a memory buffer
     * of the start of the block (physical record). It is kept for convenience.
	 */
	private long bufferStartingPosition = -1;

    /**
	 * Get the size of the block (physical record).
	 *
	 * @return the size of the block (physical record) in ints.
	 */
	public int getSize() {
		return size;
	}

	/**
	 * Null constructor initializes all fields to zero, except block# = 1.
	 */
	public BlockHeaderV4() {
		size = 0;
		number = 1;
		headerLength = 0;
		version = 0;
		eventCount = 0;
        reserved1 = 0;
        reserved2 = 0;
		magicNumber = 0;
	}

    /**
     * Creates a BlockHeader for evio version 4 format. Only the <code>block size</code>
     * and <code>block number</code> are provided. The other six words, which can be
     * modified by setters, are initialized to these values:<br>
     *<ul>
     *<li><code>headerLength</code> is initialized to 8<br>
     *<li><code>start</code> is initialized to 8<br>
     *<li><code>end</code> is initialized to <code>size</code><br>
     *<li><code>version</code> is initialized to 4<br>
     *<li><code>bitInfo</code> is initialized to all bits off<br>
     *<li><code>magicNumber</code> is initialized to <code>MAGIC_NUMBER</code><br>
     *</ul>
     * @param size the size of the block in ints.
     * @param number the block number--usually sequential.
     */
    public BlockHeaderV4(int size, int number) {
        this.size = size;
        this.number = number;
        headerLength = 8;
        version = 4;
        eventCount = 0;
        reserved1 = 0;
        reserved2 = 0;
        magicNumber = MAGIC_NUMBER;
    }

    /**
     * This copy constructor creates an evio version 4 BlockHeader
     * from another object of this class.
     * @param blkHeader block header object to copy
     */
    public BlockHeaderV4(BlockHeaderV4 blkHeader) {
        if (blkHeader == null) {
            return;
        }
        size         = blkHeader.size;
        number       = blkHeader.number;
        headerLength = blkHeader.headerLength;
        version      = blkHeader.version;
        eventCount   = blkHeader.eventCount;
        reserved1    = blkHeader.reserved1;
        reserved2    = blkHeader.reserved2;
        magicNumber  = blkHeader.magicNumber;
        bitInfo      = (BitSet)  blkHeader.bitInfo.clone();
        bufferStartingPosition = blkHeader.bufferStartingPosition;
    }

    /*** {@inheritDoc}  */
    public Object clone() {
        return new BlockHeaderV4(this);
    }

	/**
	 * Set the size of the block (physical record). Some trivial checking is done.
	 *
	 * @param size the new value for the size, in ints.
	 * @throws EvioException
	 */
	public void setSize(int size) throws EvioException {
        if (size < 8) {
			throw new EvioException(String.format("Bad value for size in block (physical record) header: %d", size));
		}

        this.size = size;
	}

    /**
     * Get the number of events completely contained in the block.
     * NOTE: There are no partial events, only complete events stored in one block.
     *
     * @return the number of events in the block.
     */
    public int getEventCount() {
        return eventCount;
    }

    /**
     * Set the number of events completely contained in the block.
     * NOTE: There are no partial events, only complete events stored in one block.
     *
     * @param count the new number of events in the block.
     * @throws EvioException
     */
    public void setEventCount(int count) throws EvioException {
        if (count < 0) {
            throw new EvioException(String.format("Bad value for event count in block (physical record) header: %d", count));
        }
        this.eventCount = count;
    }

	/**
	 * Get the block number for this block (physical record).
     * In a file, this is usually sequential, starting at 1.
	 *
	 * @return the block number for this block (physical record).
	 */
	public int getNumber() {
		return number;
	}

	/**
	 * Set the block number for this block (physical record).
     * In a file, this is usually sequential starting at 1.
     * This is not checked.
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
	 * Set the block header length, in ints. Although technically speaking this value
     * is variable, it should be 8.
	 *
	 * param headerLength the new block header length. This should be 8.
	 */
	public void setHeaderLength(int headerLength) {
		if (headerLength != HEADER_SIZE) {
            System.out.println("Warning: Block Header Length = " + headerLength);
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
     * Sets the evio version. Should be 4 but no check is performed here, see
     * {@link EvioReader#nextBlockHeader()}.
     *
     * @param version the evio version of evio.
     */
    public void setVersion(int version) {
        this.version = version;
    }

    /**
     * Does this integer indicate that there is an evio dictionary
     * (assuming it's the header's sixth word)?
     *
     * @return <code>true</code> if this int's indicates an evio dictionary, else <code>false</code>
     */
    static public boolean hasDictionary(int i) {
        return ((i & EV_DICTIONARY_MASK) > 0);
    }

    /**
     * Is this block's first event is an evio dictionary?
     *
     * @return <code>true</code> if this block's first event is an evio dictionary, else <code>false</code>
     */
    public boolean hasDictionary() {
        return bitInfo.get(0);
    }

    /**
     * Is this the last block in the file or being sent over the network?
     *
     * @return <code>true</code> if this is the last block in the file or being sent
     *         over the network, else <code>false</code>
     */
    public boolean isLastBlock() {
        return bitInfo.get(1);
    }

    /**
     * Does this integer indicate that this is the last block
     * (assuming it's the header's sixth word)?
     *
     * @return <code>true</code> if this int's indicates the last block, else <code>false</code>
     */
    static public boolean isLastBlock(int i) {
        return ((i & EV_LASTBLOCK_MASK) > 0);
    }

    /**
     * Set the bit in the given arg which indicates this is the last block.
     * @param i integer in which to set the last-block bit
     * @return  arg with last-block bit set
     */
    static public int setLastBlockBit(int i)   {
        return (i |= EV_LASTBLOCK_MASK);
    }

    /**
     * Clear the last-block bit in the given arg to indicate it is NOT the last block.
     * @param i integer in which to clear the last-block bit
     * @return arg with last-block bit cleared
     */
    static public int clearLastBlockBit(int i) {
        return (i &= ~EV_LASTBLOCK_MASK);
    }


    /**
     * Get the value of bits 2-5. It represents the type of event being sent.
     *
     * @return bits 2-5 as an integer, representing event type.
     */
    public int getEventType() {
        int type=0;
        boolean bitSet;
        for (int i=0; i < 4; i++) {
            bitSet = bitInfo.get(i+2);
            if (bitSet) type |= 1 << i;
        }
        return type;
    }

    /**
     * Gets a copy of all stored bit information.
     *
     * @return copy of BitSet containing all stored bit information.
     */
    public BitSet getBitInfo() {
        return (BitSet)bitInfo.clone();
    }

    /**
     * Gets the value of a particular bit in the bitInfo field.
     *
     * @param bitIndex index of bit to get
     * @return BitSet containing all stored bit information.
     */
    public boolean getBitInfo(int bitIndex) {
        if (bitIndex < 0 || bitIndex > 23) {
            return false;
        }
        return bitInfo.get(bitIndex);
    }

    /**
     * Sets a particular bit in the bitInfo field.
     *
     * @param bitIndex index of bit to change
     * @param value value to set bit to
     */
    public void setBit(int bitIndex, boolean value) {
        if (bitIndex < 0 || bitIndex > 23) {
            return;
        }
        bitInfo.set(bitIndex, value);
    }

    /**
     * Sets the right bits in bit set (2-5 when starting at 0)
     * to hold 4 bits of the given type value. Useful when
     * generating a bitset for use with {@link EventWriter}
     * constructor.
     *
     * @param bSet Bitset containing all bits to be set
     * @param type event type as int
     */
    static public void setEventType(BitSet bSet, int type) {
        if (type < 0) type = 0;
        else if (type > 15) type = 15;

        if (bSet.size() < 6) return;

        for (int i=2; i < 6; i++) {
            bSet.set(i, ((type >>> i-2) & 0x1) > 0);
        }
    }

    /**
     * Calculates the sixth word of this header which has the version number
     * in the lowest 8 bits and the bit info in the highest 24 bits.
     *
     * @return sixth word of this header.
     */
    public int getSixthWord() {
        int v = version & 0xff;

        for (int i=0; i < bitInfo.length(); i++) {
            if (bitInfo.get(i)) {
                v |= (0x1 << (8+i));
            }
        }

        return v;
    }

    /**
     * Calculates the sixth word of this header which has the version number (4)
     * in the lowest 8 bits and the set in the upper 24 bits.
     *
     * @param set Bitset containing all bits to be set
     * @return generated sixth word of this header.
     */
    static public int generateSixthWord(BitSet set) {
        int v = 4; // version

        for (int i=0; i < set.length(); i++) {
            if (i > 23) {
                break;
            }
            if (set.get(i)) {
                v |= (0x1 << (8+i));
            }
        }

        return v;
    }

    /**
     * Calculates the sixth word of this header which has the version number (4)
     * in the lowest 8 bits and the set in the upper 24 bits. The arg isDictionary
     * is set in the 9th bit and isEnd is set in the 10th bit.
     *
     * @param bSet Bitset containing all bits to be set
     * @param hasDictionary does this block include an evio xml dictionary as the first event?
     * @param isEnd is this the last block of a file or a buffer?
     * @return generated sixth word of this header.
     */
    static public int generateSixthWord(BitSet bSet, boolean hasDictionary, boolean isEnd) {
        int v = 4; // version

        for (int i=0; i < bSet.length(); i++) {
            if (i > 23) {
                break;
            }
            if (bSet.get(i)) {
                v |= (0x1 << (8+i));
            }
        }

        v =  hasDictionary ? (v | 0x100) : v;
        v =  isEnd ? (v | 0x200) : v;

        return v;
    }

    /**
     * Calculates the sixth word of this header which has the version number
     * in the lowest 8 bits. The arg hasDictionary
     * is set in the 9th bit and isEnd is set in the 10th bit. Four bits of an int
     * (event type) are set in bits 11-14.
     *
     * @param version evio version number
     * @param hasDictionary does this block include an evio xml dictionary as the first event?
     * @param isEnd is this the last block of a file or a buffer?
     * @param eventType 4 bit type of events header is containing
     * @return generated sixth word of this header.
     */
    static public int generateSixthWord(int version, boolean hasDictionary,
                                        boolean isEnd, int eventType) {

        return generateSixthWord(null, version, hasDictionary, isEnd, eventType);
    }


    /**
      * Calculates the sixth word of this header which has the version number (4)
      * in the lowest 8 bits and the set in the upper 24 bits. The arg isDictionary
      * is set in the 9th bit and isEnd is set in the 10th bit. Four bits of an int
      * (event type) are set in bits 11-14.
      *
      * @param bSet Bitset containing all bits to be set
      * @param version evio version number
      * @param hasDictionary does this block include an evio xml dictionary as the first event?
      * @param isEnd is this the last block of a file or a buffer?
      * @param eventType 4 bit type of events header is containing
      * @return generated sixth word of this header.
      */
     static public int generateSixthWord(BitSet bSet, int version,
                                         boolean hasDictionary,
                                         boolean isEnd, int eventType) {
         int v = version; // version

         if (bSet != null) {
             for (int i=0; i < bSet.length(); i++) {
                 if (i > 23) {
                     break;
                 }
                 if (bSet.get(i)) {
                     v |= (0x1 << (8+i));
                 }
             }
         }

         v =  hasDictionary ? (v | 0x100) : v;
         v =  isEnd ? (v | 0x200) : v;
         v |= ((eventType & 0xf) << 10);

         return v;
     }


    /**
     * Parses the argument into the bit info fields.
     * This ignores the version in the lowest 8 bits.
     *
     * @param word integer to parse into bit info fields
     */
    public void parseToBitInfo(int word) throws EvioException {
        for (int i=0; i < 24; i++) {
            bitInfo.set(i, ((word >>> 8+i) & 0x1) > 0);
        }
    }

    /**
     * Parses the argument into the bit info fields and looks to see
     * if the the "has dictionary" bit is set.
     *
     * @param word integer to parse into bit info fields
     * @return <code>true</code> if word's "has dictionary" bit is set
     */
    public static boolean bitInfoHasDictionary(int word) {
        BitSet bitInfo = new BitSet(24);
        for (int i=0; i < 24; i++) {
            bitInfo.set(i, ((word >>> 8+i) & 0x1) > 0);
        }
        return bitInfo.get(0);
    }

    /**
     * Get the first reserved word.
     * @return the first reserved word.
     */
    public int getReserved1() {
        return reserved1;
    }

    /**
     * Sets the value of first reserved word.
     * @param reserved1 the value for first reserved word.
     */
    public void setReserved1(int reserved1) {
        this.reserved1 = reserved1;
    }

    /**
     * Get the second reserved word.
     * @return the second reserved word.
     */
    public int getReserved2() {
        return reserved2;
    }

    /**
     * Sets the value of second reserved word.
     * @param reserved2 the value for second reserved word.
     */
    public void setReserved2(int reserved2) {
        this.reserved2 = reserved2;
    }

	/**
	 * Get the magic number the block (physical record) header which should be 0xc0da0100.
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
		sb.append(String.format("block size:     %d\n", size));
		sb.append(String.format("number:         %d\n", number));
		sb.append(String.format("headerLen:      %d\n", headerLength));
        sb.append(String.format("event count:    %d\n", eventCount));
        sb.append(String.format("reserved 1:     %d\n", reserved1));
        sb.append(String.format("bit info:       %s\n", bitInfo));
        sb.append(String.format("has dictionary: %b\n", hasDictionary()));
        sb.append(String.format("version:        %d\n", version));
		sb.append(String.format("magicNumber:    %8x\n", magicNumber));
		sb.append(String.format(" *buffer start: %d\n", getBufferStartingPosition()));
		sb.append(String.format(" *next   start: %d\n", nextBufferStartingPosition()));
		return sb.toString();
	}

    /**
     * Get the position in the buffer (in bytes) of this block's last data word.<br>
     *
     * @return the position in the buffer (in bytes) of this block's last data word.
     */
    public long getBufferEndingPosition() {
        return bufferStartingPosition + 4*size;
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
        return getBufferEndingPosition();
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
		// first data word is start of first event
        return bufferStartingPosition + 4*headerLength;
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
		if (position > nextBufferStart) {
			throw new EvioException("Provided position beyond buffer end position.");
		}

		return (int)(nextBufferStart - position);
	}

	/**
	 * Write myself out a byte buffer. This write is relative--i.e.,
     * it uses the current position of the buffer.
	 *
	 * @param byteBuffer the byteBuffer to write to.
	 * @return the number of bytes written, which for a BlockHeader is 32.
	 */
	public int write(ByteBuffer byteBuffer) {
		byteBuffer.putInt(size);
		byteBuffer.putInt(number);
		byteBuffer.putInt(headerLength); // should always be 8
		byteBuffer.putInt(eventCount);
        byteBuffer.putInt(0);            // unused
		byteBuffer.putInt(getSixthWord());
		byteBuffer.putInt(0);            // unused
		byteBuffer.putInt(magicNumber);
		return 32;
	}
}