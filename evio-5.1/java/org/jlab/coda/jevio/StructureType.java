package org.jlab.coda.jevio;

/**
 * This an enum used to convert structure type numerical values to a more meaningful name. For example, the structure
 * type with value 0xe corresponds to the enum BANK. Mostly this is used for printing.
 * 
 * @author heddle
 * @author timmer
 *
 */
public enum StructureType {

	UNKNOWN32(0x0), 
	TAGSEGMENT(0xc), 
	SEGMENT(0xd), 
	BANK(0xe);

	private int value;

    /** Fast way to convert integer values into StructureType objects. */
    private static StructureType[] intToType;


    // Fill array after all enum objects created
    static {
        intToType = new StructureType[0xe + 1];
        for (StructureType type : values()) {
            intToType[type.value] = type;
        }
    }


    private StructureType(int value) {
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
     * Obtain the name from the value.
     *
     * @param val the value to match.
     * @return the name, or "UNKNOWN".
     */
    public static String getName(int val) {
        if (val > 0xe || val < 0) return "UNKNOWN";
        StructureType type = getStructureType(val);
        if (type == null) return "UNKNOWN";
        return type.name();
    }

    /**
     * Obtain the enum from the value.
     *
     * @param val the value to match.
     * @return the matching enum, or <code>null</code>.
     */
    public static StructureType getStructureType(int val) {
        if (val > 0xe || val < 0) return null;
        return intToType[val];
    }
}
