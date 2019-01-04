package org.jlab.coda.jevio;


/**
 * This an enum used to convert data type numerical values to a more meaningful name.
 * For example, the data type with value 0x20 corresponds to the enum BANK.
 * Mostly this is used for printing. In this version of evio, the
 * ALSOTAGSEGMENT (0x40) value was removed from this enum because
 * the upper 2 bits of a byte containing the datatype are now used
 * to store padding data.
 * 
 * @author heddle
 * @author timmer
 *
 */
public enum DataType {

	UNKNOWN32      (0x0), 
	UINT32         (0x1), 
	FLOAT32        (0x2), 
	CHARSTAR8      (0x3), 
	SHORT16        (0x4), 
	USHORT16       (0x5), 
	CHAR8          (0x6), 
	UCHAR8         (0x7), 
	DOUBLE64       (0x8), 
	LONG64         (0x9), 
	ULONG64        (0xa), 
	INT32          (0xb), 
	TAGSEGMENT     (0xc), 
	ALSOSEGMENT    (0xd),
    ALSOBANK       (0xe),
    COMPOSITE      (0xf),
	BANK           (0x10),
	SEGMENT        (0x20),

    //  Remove ALSOTAGSEGMENT (0x40) since it was never
    //  used and we now need that to store padding data.
    //	ALSOTAGSEGMENT (0x40),

    // These 2 types are only used when dealing with COMPOSITE data.
    // They are never transported independently and are stored in integers.
    HOLLERIT       (0x21),
    NVALUE         (0x22);


    /** Each name is associated with a specific evio integer value. */
    private int value;


    /** Fast way to convert integer values into DataType objects. */
    private static DataType[] intToType;


    // Fill array after all enum objects created
    static {
        intToType = new DataType[0x42 + 1];
        for (DataType type : values()) {
            intToType[type.value] = type;
        }
    }


	/**
	 * Obtain the enum from the value.
	 *
	 * @param val the value to match.
	 * @return the matching enum, or <code>null</code>.
	 */
    public static DataType getDataType(int val) {
        if (val > 0x42 || val < 0) return null;
        return intToType[val];
    }


    /**
     * Obtain the name from the value.
     *
     * @param val the value to match.
     * @return the name, or "UNKNOWN".
     */
    public static String getName(int val) {
        if (val > 0x42 || val < 0) return "UNKNOWN";
        DataType type = getDataType(val);
        if (type == null) return "UNKNOWN";
        return type.name();
    }


    /**
     * Return a string which is usually the same as the name of the
     * enumerated value, except in the cases of ALSOSEGMENT and
     * ALSOBANK which return SEGMENT and BANK respectively.
     *
     * @return name of the enumerated type
     */
    public String toString() {
        if      (this == ALSOBANK) return "BANK";
        else if (this == ALSOSEGMENT) return "SEGMENT";
        return super.toString();
    }


    /**
     * Constructor.
     * @param value
     */
	private DataType(int value) {
		this.value = value;
	}


	/**
	 * Get the enum's value.
	 * 
	 * @return the value, e.g., 0xe for a BANK
	 */
	public int getValue() {
		return value;
	}


	/**
	 * Convenience routine to see if "this" data type is a structure (a container.)
	 * @return <code>true</code> if the data type corresponds to one of the structure
	 * types: BANK, SEGMENT, or TAGSEGMENT.
	 */
	public boolean isStructure() {
		switch (this) {
		case BANK:
		case SEGMENT:
		case TAGSEGMENT:
		case ALSOBANK:
		case ALSOSEGMENT:
//		case ALSOTAGSEGMENT:
			return true;
		default:
			return false;
		}
	}

    /**
   	 * Convenience routine to see if the given integer arg represents a data type which
     * is a structure (a container).
   	 * @return <code>true</code> if the data type corresponds to one of the structure
   	 * types: BANK, SEGMENT, or TAGSEGMENT.
   	 */
   	static public boolean isStructure(int dataType) {
   		switch (getDataType(dataType)) {
   		case BANK:
   		case SEGMENT:
   		case TAGSEGMENT:
   		case ALSOBANK:
   		case ALSOSEGMENT:
   			return true;
   		default:
   			return false;
   		}
   	}

}
