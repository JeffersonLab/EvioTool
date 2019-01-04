package org.jlab.coda.jevio;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * This the the header for an evio tag segment structure (<code>EvioTagSegment</code>). It does not contain the raw data, just the
 * header. 
 * 
 * @author heddle
 * 
 */
public final class TagSegmentHeader extends BaseStructureHeader {
	
	/**
	 * Null constructor.
	 */
	public TagSegmentHeader() {
	}

	/**
	 * Constructor.
	 * @param tag the tag for the tag segment header.
	 * @param dataType the data type for the content of the tag segment.
	 */
	public TagSegmentHeader(int tag, DataType dataType) {
        super(tag, dataType);
	}


    /**
     * {@inheritDoc}
     */
    public int getHeaderLength() {return 1;}

    /**
     * {@inheritDoc}
     */
    protected void toArray(byte[] bArray, int offset, ByteOrder order) {
        short compositeWord = (short) ((tag << 4) | (dataType.getValue() & 0xf));

        try {
            if (order == ByteOrder.BIG_ENDIAN) {
                ByteDataTransformer.toBytes(compositeWord, order, bArray, offset);
                ByteDataTransformer.toBytes((short)length, order, bArray, offset + 2);
            }
            else {
                ByteDataTransformer.toBytes((short)length, order, bArray, offset);
                ByteDataTransformer.toBytes(compositeWord, order, bArray, offset+2);
            }
        }
        catch (EvioException e) {e.printStackTrace();}
    }
    
	/**
	 * Obtain a string representation of the bank header.
	 * 
	 * @return a string representation of the bank header.
	 */
	@Override
	public String toString() {
		StringBuffer sb = new StringBuffer(512);
		sb.append(String.format("tag-seg length: %d\n", length));
		sb.append(String.format("number:         %d\n", number));
		sb.append(String.format("data type:      %s\n", getDataTypeName()));
		sb.append(String.format("tag:            %d\n", tag));
		return sb.toString();
	}
	
	/**
	 * Write myself out a byte buffer. This write is relative--i.e., it uses the current position of the buffer.
	 * 
	 * @param byteBuffer the byteBuffer to write to.
	 * @return the number of bytes written, which for a TagSegmentHeader is 4.
	 */
	public int write(ByteBuffer byteBuffer) {
		short compositeWord = (short) ((tag << 4) | (dataType.getValue() & 0xf));

        if (byteBuffer.order() == ByteOrder.BIG_ENDIAN) {
            byteBuffer.putShort(compositeWord);
            byteBuffer.putShort((short)length);
        }
        else {
            byteBuffer.putShort((short)length);
            byteBuffer.putShort(compositeWord);
        }
		return 4;
	}

}