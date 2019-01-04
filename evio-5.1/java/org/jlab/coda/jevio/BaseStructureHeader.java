package org.jlab.coda.jevio;

import java.nio.ByteOrder;

/**
 * This the  header for the base structure (<code>BaseStructure</code>). It does not contain the raw data, just the
 * header. The three headers for the actual structures found in evio (BANK, SEGMENT, and TAGSEMENT) all extend this.
 * 
 * @author heddle
 * 
 */
public abstract class BaseStructureHeader implements Cloneable, IEvioWriter {

    /**
     * The length of the structure in ints. This never includes the
     * first header word itself (which contains the length).
     */
    protected int length;

	/**
	 * The structure tag.
	 */
	protected int tag;

    /**
     * The data type of the structure.
     */
    protected DataType dataType;

    /**
     * The amount of padding bytes when storing short or byte data.
     * Allowed value is 0, 1, 2, or 3 (0,2 for shorts and 0-3 for bytes)
     * and is stored in the upper 2 bits of the dataType when written out.
     */
    protected int padding;

	/**
	 * The number represents an unsigned byte. Only Banks have a number
     * field in their header, so this is only relevant for Banks.
	 */
	protected int number;

    /**
	 * Null constructor.
	 */
	public BaseStructureHeader() {
	}
	
	/**
	 * Constructor
	 * @param tag the tag for the header.
	 * @param dataType the enum data type for the content of the structure.
	 */
	public BaseStructureHeader(int tag, DataType dataType) {
        this(tag, dataType, 0);
	}

	/**
	 * Constructor
	 * @param tag the tag for the header.
	 * @param dataType the data type for the content of the structure.
	 * @param num sometimes, but not necessarily, an ordinal enumeration.
	 */
	public BaseStructureHeader(int tag, DataType dataType, int num) {
		this.tag = tag;
		this.dataType = dataType;
		setNumber(num);
	}


    /**
     * Clone this object by passing responsibility to the Object class
     * which creates a bitwise copy which is fine since all fields are
     * ints or enums.
     */
    public Object clone() {
        try {
            return super.clone();
        }
        catch (CloneNotSupportedException e) {
            return null;
        }
    }


	/**
	 * Get the number. Only Banks have a number field in their header, so this is only relevant for Banks.
	 * 
	 * @return the number.
	 */
	public int getNumber() {
		return number;
	}

	/**
	 * Set the number. Only Banks have a number field in their header, so this is only relevant for Banks.
	 * 
	 * @param number the number.
	 */
	public void setNumber(int number) {
		//num is really an unsigned byte
		if (number < 0) {
			number += 256;
		}
		this.number = number & 0xff;
	}

	/**
	 * Get the data type for the structure.
	 * 
	 * @return the data type for the structure.
	 */
	public int getDataTypeValue() {
		return dataType.getValue();
	}

    /**
     * Set the numeric data type for the structure.
     *
     * @param dataType the dataTtype for the structure.
     */
    void setDataType(int dataType) {
        this.dataType = DataType.getDataType(dataType);
    }

    /**
     * Set the numeric data type for the structure.
     *
     * @param dataType the dataTtype for the structure.
     */
    public void setDataType(DataType dataType) {
        this.dataType = dataType;
    }

    /**
     * Returns the data type for data stored in this structure as a <code>DataType</code> enum.
     *
     * @return the data type for data stored in this structure as a <code>DataType</code> enum.
     * @see DataType
     */
    public DataType getDataType() {
        return dataType;
    }

    /**
     * Returns the data type as a string.
     *
     * @return the data type as a string.
     */
    public String getDataTypeName() {
        return dataType.name();
    }

    /**
     * Get the amount of padding bytes when storing short or byte data.
     * Value is 0, 1, 2, or 3 for bytes and 0 or 2 for shorts.
     * @return number of padding bytes
     */
    public int getPadding() {
        return padding;
    }

    /**
     * Set the amount of padding bytes when storing short or byte data.
     * Allowed value is 0, 1, 2, or 3 for bytes and 0 or 2 for shorts.
     * @param padding amount of padding bytes when storing short or byte data (0-3).
     */
    protected void setPadding(int padding) {
        this.padding = padding;
    }

    /**
     * Get the length of the structure in ints, not counting the length word.
     *
     * @return Get the length of the structure in ints (not counting the length word).
     */
    public int getLength() {
        return length;
    }

    /**
     * Set the length of the structure in ints, not counting the length word.
     *
     * @param length the length of the structure in ints, not counting the length word.
     */
    public void setLength(int length) {
        this.length = length;
    }

    /**
     * Get the length of the structure's header in ints. This includes the first header word itself
     * (which contains the length) and in the case of banks, it also includes the second header word.
     *
     * @return Get the length of the structure's header in ints.
     */
    public abstract int getHeaderLength();

    /**
     * Write myself out as a byte array of evio format data
     * into the given byte array in the specified byte order.
     */
    protected abstract void toArray(byte[] bArray, int offset, ByteOrder order);

	/**
	 * Get the structure tag.
	 * 
	 * @return the structure tag.
	 */
	public int getTag() {
		return tag;
	}

	/**
	 * Set the structure tag.
	 * 
	 * @param tag the structure tag.
	 */
	public void setTag(int tag) {
		this.tag = tag;
	}

	/**
	 * Obtain a string representation of the structure header.
	 * 
	 * @return a string representation of the structure header.
	 */
	@Override
	public String toString() {
		StringBuffer sb = new StringBuffer(512);
		sb.append(String.format("structure length: %d\n", length));
		sb.append(String.format("data type:   %s\n", getDataTypeName()));
        sb.append(String.format("tag:         %d\n", tag));
        sb.append(String.format("padding:     %d\n", padding));
		return sb.toString();
	}
	
	/**
	 * Convenience method to return the byte value of an integer. Although
	 * the parameter is an Integer, use "autoboxing" to pass in a primitive. I.e.,
	 * byteValue(3) works just fine.
	 * @param integer the integer whose byte value is needed. Can pass in a primitive int.
	 * @return the byte value of the integer.
	 */
	public byte byteValue(Integer integer) {
		return integer.byteValue();
	}
	
	/**
	 * Convenience method to return the short value of an integer. Although
	 * the parameter is an Integer, use "autoboxing" to pass in a primitive. I.e.,
	 * shortValue(3345) works just fine.
	 * @param integer the integer whose short value is needed. Can pass in a primitive int.
	 * @return the short value of the integer.
	 */
	public short shortValue(Integer integer) {
		return integer.shortValue();
	}


}
