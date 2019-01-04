package org.jlab.coda.jevio;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * This the header for an evio bank structure (<code>EvioBank</code>). It does not contain the raw data, just the
 * header. Note: since an "event" is really just the outermost bank, this is also the header for an
 * <code>EvioEvent</code>.
 * 
 * @author heddle
 * 
 */
public final class BankHeader extends BaseStructureHeader {

	/**
	 * Null constructor.
	 */
	public BankHeader() {
	}

	/**
	 * Constructor
	 * @param tag the tag for the bank header.
	 * @param dataType the data type for the content of the bank.
	 * @param num sometimes, but not necessarily, an ordinal enumeration.
	 */
	public BankHeader(int tag, DataType dataType, int num) {
		super(tag, dataType, num);
	}


    /**
     * {@inheritDoc}
     */
    public int getHeaderLength() {return 2;}

    /**
     * {@inheritDoc}
     */
    protected void toArray(byte[] bArray, int offset, ByteOrder order) {
        try {
            // length first
            ByteDataTransformer.toBytes(length, order, bArray, offset);

            if (order == ByteOrder.BIG_ENDIAN) {
                ByteDataTransformer.toBytes((short)tag, order, bArray, offset+4);
                // lowest 6 bits are dataType, upper 2 bits are padding
                bArray[offset+6] = (byte)((dataType.getValue() & 0x3f) | (padding << 6));
                bArray[offset+7] = (byte)number;
            }
            else {
                bArray[offset+4] = (byte)number;
                bArray[offset+5] = (byte)((dataType.getValue() & 0x3f) | (padding << 6));
                ByteDataTransformer.toBytes((short)tag, order, bArray, offset+6);
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
		sb.append(String.format("bank length: %d\n", length));
		sb.append(String.format("number:      %d\n", number));
		sb.append(String.format("data type:   %s\n", getDataTypeName()));
        sb.append(String.format("tag:         %d\n", tag));
        sb.append(String.format("padding:     %d\n", padding));
		return sb.toString();
	}
	
	/**
	 * Write myself out a byte buffer. This write is relative--i.e., it uses the current position of the buffer.
	 * 
	 * @param byteBuffer the byteBuffer to write to.
	 * @return the number of bytes written, which for a BankHeader is 8.
	 */
	public int write(ByteBuffer byteBuffer) {
		byteBuffer.putInt(length);
        
        if (byteBuffer.order() == ByteOrder.BIG_ENDIAN) {
            byteBuffer.putShort((short)tag);
            byteBuffer.put((byte)((dataType.getValue() & 0x3f) | (padding << 6)));
            byteBuffer.put((byte)number);
        }
        else {
            byteBuffer.put((byte)number);
            byteBuffer.put((byte)((dataType.getValue() & 0x3f) | (padding << 6)));
            byteBuffer.putShort((short)tag);
        }

		return 8;
	}

}
