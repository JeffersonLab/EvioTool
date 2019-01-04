package org.jlab.coda.jevio;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.ArrayList;

/**
 * This is a class designed to handle access of evio version 4 format files
 * with size greater than 2.1 GBytes.
 * Currently the largest size memory map that Java can handle
 * is Integer.MAX_VALUE which limits the use of memory mapping to looking at
 * files that size or smaller. This class circumvents this limit by viewing
 * large files as a collection of multiple memory maps.<p>
 *
 * The problem is that we do <b>NOT</b> want an evio event which is split between
 * 2 different memory maps. This is addressed by scanning a memory map of the
 * largest size to find the last complete evio block it contains. The next memory
 * map then starts at the following block.<p>
 *
 * Just a note about synchronization. This object is <b>NOT</b> threadsafe but it
 * doesn't have to be since its use in {@link EvioReader} is synchronized and
 * therefore there should be not be thread unsafe.
 *
 * @author timmer
 */
public class MappedMemoryHandler {

    /** Number of evio blocks in file/buffer. */
    private int blockCount;

    /** Number of evio event in file/buffer. */
    private int eventCount;

    /** Number of memory maps needed to fully map file. */
    private int regionCount;

    /** List containing each event's location (map & position). */
    private ArrayList<int[]> eventPositions = new ArrayList<int[]>(20000);

    /** List containing each event's memory map. */
    private ArrayList<ByteBuffer> regions = new ArrayList<ByteBuffer>(20);

    /** Was there an evio format error in the file? */
    private boolean evioErrorCondition;


    /**
     * Is there an evio error in the file/buffer.
     * @return {@code true} if error, else {@code false}
     */
    public boolean isEvioErrorCondition() {
        return evioErrorCondition;
    }

    /**
     * Get the number of evio events in the file/buffer.
     * @return number of evio events in the file/buffer.
     */
    public int getEventCount() {return eventCount;}

    /**
     * Get the number of evio blocks in the file/buffer.
     * @return number of evio blocks in the file/buffer.
     */
    public int getBlockCount() {return blockCount;}

    /**
     * Get the number of memory maps used to fully map file.
     * @return number of memory maps used to fully map file.
     */
    public int getMapCount() {return regionCount;}

    /**
     * Get the first memory map - used to map the beginning of the file.
     * @return first memory map - used to map the beginning of the file.
     */
    public ByteBuffer getFirstMap() {return regions.get(0);}


    /**
     * Get the ByteBuffer object containing the event of interest.
     * Position in the buffer is properly set to start reading.
     *
     * @param eventNumber number of the desired event (starting at 0)
     * @return the ByteBuffer object containing the event of interest.
     */
    public ByteBuffer getByteBuffer(int eventNumber) {
        int[] evData = eventPositions.get(eventNumber);
        ByteBuffer buf = regions.get(evData[0]);
        // Even though the position is set here, it's not a problem as
        // long as only 1 thread is using this object.
        buf.position(evData[1]);
        return buf;
    }


    /**
     * Constructor. File format errors are flagged, but no exception is thrown.
     *
     * @param channel file's file channel object
     * @param byteOrder byte order of the data
     * @throws IOException   if could not map file
     * @throws EvioException if bad file format
     */
    public MappedMemoryHandler (FileChannel channel, ByteOrder byteOrder) throws IOException {

        // Divide the memory into chunks or regions
        long remainingSize = channel.size();
        long sz, bytesUsed, prevBytesUsed=-1L, offset = 0L;
        boolean smallFile = remainingSize <= Integer.MAX_VALUE;

        regionCount = 0;
        ByteBuffer memoryMapBuf;

//        long t2, t1 = System.currentTimeMillis();

        while (remainingSize > 0) {
            // Need enough data to at least read 1 block header (32 bytes)
            if (remainingSize < 32) {
System.out.println("MappedMemoryHandler: bad evio format, extra "+ remainingSize + " bytes at file end");
                evioErrorCondition = true;
                return;
            }

            // Don't read more than max allowed
            sz = Math.min(remainingSize, Integer.MAX_VALUE);
//System.out.println("MMH constr: sz = " + sz + ", offset = " + offset);

            memoryMapBuf = channel.map(FileChannel.MapMode.READ_ONLY, offset, sz);
            memoryMapBuf.order(byteOrder);
            // The problem here is that we do NOT want an event which is split between
            // 2 different memory maps. For convenience, find an ending boundary exactly
            // at the end of an evio block. We don't remap the memory, but only use up
            // to and including the last full block of what was just mapped. The next
            // map will start at the following block.
            try {
                bytesUsed = generateEventPositions(memoryMapBuf, regionCount);
            }
            catch (EvioException e) {
System.out.println("MappedMemoryHandler: bad evio format, stop parsing file");
                regions.add(memoryMapBuf);
                regionCount++;
                evioErrorCondition = true;
                return;
            }
//System.out.println("  bytesUsed in map = " + bytesUsed + ", prev used = " + prevBytesUsed);

            // If there is a corrupted file in which the last events are not written,
            // it ends in the middle of the last block. This algorithm will try to
            // read that block again and fail. If it's a small file, all data should
            // have been mapped. Catch this corrupted file.
            if ((smallFile && bytesUsed < remainingSize) ||
                (bytesUsed == prevBytesUsed)) {
System.out.println("MappedMemoryHandler: bad evio format, likely last block not completely written, bytesUsed = "
                           + bytesUsed + ", prev = " + prevBytesUsed);
				regions.add(memoryMapBuf);
				regionCount++;
                evioErrorCondition = true;
                return;
            }
            prevBytesUsed = bytesUsed;
//System.out.println("  eventCount = " + eventCount);
//System.out.println("  blockCount = " + blockCount);

            // Store the map
            regions.add(memoryMapBuf);
            regionCount++;

            offset += bytesUsed;
            remainingSize -= bytesUsed;
        }

//        t2 = System.currentTimeMillis();
//        System.out.println("Time to scan file = " + (t2-t1) + " milliseconds");

    }


    /**
     * Constructor.
     * @param evioBuf buffer to analyze
     */
    public MappedMemoryHandler (ByteBuffer evioBuf) {
        regionCount = 1;

        try {
            // Generate position info
            generateEventPositions(evioBuf, 0);
        }
        catch (EvioException e) {
            evioErrorCondition = true;
        }

        // Store the map
        regions.add(evioBuf);
    }


    /**
     * Generate a table (ArrayList) of positions of events in file/buffer.
     * This method does <b>not</b> affect the byteBuffer position, eventNumber,
     * or lastBlock values. Only called if there are at least 32 bytes available.
     * Valid only in versions 4 and later.
     *
     * @param byteBuffer   buffer to analyze
     * @param regionNumber number of the memory map (starting at 0)
     *
     * @return the number of bytes representing all the full blocks
     *         contained in the given byte buffer.
     * @throws EvioException if bad file format
     */
    private long generateEventPositions(ByteBuffer byteBuffer, int regionNumber)
            throws EvioException {


        int      blockSize, blockHdrSize, blockEventCount, magicNum;
        int      byteInfo, byteLen, bytesLeft, position;
        boolean  firstBlock=true, hasDictionary=false;
//        boolean  curLastBlock;

        // Start at the beginning of byteBuffer
        position  = 0;
        bytesLeft = byteBuffer.limit();

        while (bytesLeft > 0) {
            // Check to see if enough data to read block header.
            // If not return the amount of memory we've used/read.
            if (bytesLeft < 32) {
//System.out.println("return, not enough to read header");
                return position;
            }

            // File is now positioned before block header.
            // Look at block header to get info.  Swapping is taken care of
            byteInfo        = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_VERSION);
            blockSize       = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_BLOCKSIZE);
            blockHdrSize    = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_HEADERSIZE);
            blockEventCount = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_COUNT);
            magicNum        = byteBuffer.getInt(position + 4*BlockHeaderV4.EV_MAGIC);

//            System.out.println("    genEvTablePos: region "+ regionNumber + ", pos " + position +
//                               ", blk ev count = " + blockEventCount +
//                               ", blockSize = " + blockSize +
//                               ", blockHdrSize = " + blockHdrSize +
//                               ", byteInfo  = 0x" + Integer.toHexString(byteInfo) +
//                               ", magic #   = 0x" + Integer.toHexString(magicNum)
//            );

            // If magic # is not right, file is not in proper format
            if (magicNum != BlockHeaderV4.MAGIC_NUMBER) {
                throw new EvioException("Bad evio format: block header magic # incorrect");
            }

            // Check lengths in block header
            if (blockSize < 8 || blockHdrSize < 8) {
                throw new EvioException("Bad evio format: (block: total len = " +
                                                blockSize + ", header len = " + blockHdrSize + ")" );
            }

            // Check to see if the whole block is within the mapped memory.
            // If not return the amount of memory we've used/read.
            if (4*blockSize > bytesLeft) {
//System.out.println("    4*blockSize = " + (4*blockSize) + " >? bytesLeft = " + bytesLeft +
//                   ", pos = " + position);
//System.out.println("return, not enough to read all block data");
                return position;
            }

            blockCount++;
//            eventCount  += blockEventCount;
//            curLastBlock = BlockHeaderV4.isLastBlock(byteInfo);
            if (regionNumber == 0 && firstBlock) {
                hasDictionary = BlockHeaderV4.hasDictionary(byteInfo);
            }

//            System.out.println("    genEvTablePos: blk count = " + blockCount +
//                               ", total ev count = " + eventCount +
//                               "\n                   firstBlock = " + firstBlock +
//                               ", isLastBlock = " + curLastBlock +
//                               ", hasDict  = " + hasDictionary +
//                               ", pos = " + position);

            // Hop over block header to data
            position  += 4*blockHdrSize;
            bytesLeft -= 4*blockHdrSize;

//System.out.println("    hopped blk hdr, bytesLeft = " + bytesLeft + ", pos = " + position);

            // Check for a dictionary - the first event in the first block.
            // It's not included in the header block count, but we must take
            // it into account by skipping over it.
            if (regionNumber == 0 && firstBlock && hasDictionary) {
                firstBlock = false;

                // Get its length - bank's len does not include itself
                byteLen = 4*(byteBuffer.getInt(position) + 1);

                if (byteLen < 4) {
                    throw new EvioException("Bad evio format: bad bank length");
                }

                // Skip over dictionary
                position  += byteLen;
                bytesLeft -= byteLen;
//System.out.println("    hopped dict, dic bytes = " + byteLen + ", bytesLeft = " + bytesLeft +
//                   ", pos = " + position);
            }

            // For each event in block, store its location
            for (int i=0; i < blockEventCount; i++) {
                // Sanity check - must have at least 1 header's amount left
                if (bytesLeft < 8) {
                    throw new EvioException("Bad evio format: not enough data to read event (bad bank len?)");
                }

                // Get length of current event (including full header)
                byteLen = 4*(byteBuffer.getInt(position) + 1);
                bytesLeft -= byteLen;

                if (byteLen < 4 || bytesLeft < 0) {
                    throw new EvioException("Bad evio format: bad bank length");
                }

                // Store current position
                eventPositions.add(new int[] {regionNumber, position});
                eventCount++;

                position += byteLen;
//System.out.println("    hopped event " + (i+1) + ", bytesLeft = " + bytesLeft + ", pos = " + position + "\n");
            }
        }

        return position;
    }



}


