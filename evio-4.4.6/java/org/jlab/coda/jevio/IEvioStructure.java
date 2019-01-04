package org.jlab.coda.jevio;

/**
 * This interface is implemented by classes representing the basic evio structures: banks, segments, and tagsegments.
 * Note there is not "getNum" methods, because not all structures have num fields in their header, only BANKs do.
 * 
 * @author heddle
 * 
 */
public interface IEvioStructure {

	/**
	 * Returns the header for this structure
	 * 
	 * @return the <code>BaseStructureHeader</code> for this structure.
	 */
	public BaseStructureHeader getHeader();

	/**
	 * Return the StructureType for this structure.
	 * 
	 * @return the StructureType for this structure.
	 * @see StructureType
	 */
	public StructureType getStructureType();
	
	/**
	 * Gets the raw data as an integer array, if the type as indicated by the
	 * header is appropriate.
	 * NOTE: since Java does not have unsigned primitives, both INT32 and UINT32 data types will be
	 * returned as int arrays. The application will have to deal with reinterpreting signed ints that are
	 * negative as unsigned ints
	 * @return the data as an int array, or <code>null</code> if this makes no sense for the given type.
	 */
	public int[] getIntData();
	
	/**
	 * Gets the raw data as a double array, if the type as indicated by the
	 * header is appropriate.
	 * @return the data as an double array, or <code>null</code> if this makes no sense for the given type.
	 */
	public double[] getDoubleData();
	
	/**
	 * Gets the raw data as an byte array, if the type as indicated by the
	 * header is appropriate.
	 * NOTE: since Java does not have unsigned primitives, CHAR8 and UCHAR8 data types will be
	 * returned as byte arrays. The application will have to deal with reinterpreting bytes as characters,
	 * if necessary.
	 * @return the data as an byte array, or <code>null</code> if this makes no sense for the given type.
	 */
	public byte[] getByteData();
	
    /**
     * Gets the raw data as an array of String objects, if the type as indicated by the
     * header is appropriate.
     * @return the data as an array of String objects, or <code>null</code> if this makes no sense for the given type.
     * (The only DataType it makes sense for is CHARSTAR8.)
     */
    public String[] getStringData();

	/**
	 * Gets the raw data as a long array, if the type as indicated by the
	 * header is appropriate.
	 * NOTE: since Java does not have unsigned primitives, both LONG64 and ULONG64 data types will be
	 * returned as long arrays. The application will have to deal with reinterpreting signed longs that are
	 * negative as unsigned longs.
	 * @return the data as an long array, or <code>null</code> if this makes no sense for the given type.
	 */
	public long[] getLongData();
	
	/**
	 * Gets the raw data as a float array, if the type as indicated by the
	 * header is appropriate.
	 * @return the data as an double array, or <code>null</code> if this makes no sense for the given type.
	 */
	public float[] getFloatData();
	
	/**
	 * Gets the raw data as a short array, if the type as indicated by the
	 * header is appropriate.
     * NOTE: since Java does not have unsigned primitives, both SHORT16 and USHORT16
     * data types will be returned as short arrays. The application will have to deal
     * with reinterpreting signed shorts that are negative as unsigned shorts.

	 * @return the data as an short array, or <code>null</code> if this makes no sense for the given type.
	 */
	public short[] getShortData();
	
    /**
     * Gets the raw data as an array of CompositeData objects, if the type as indicated
     * by the header is appropriate.<p>
     *
     * @return the data as an array of CompositeData objects,
     *         or <code>null</code> if this makes no sense for the given type.
     * @throws EvioException if the data is internally inconsistent
     */
    public CompositeData[] getCompositeData() throws EvioException;

	/**
	 * Get the description from the name provider (dictionary), if there is one.
	 * @return the description from the name provider (dictionary), if there is one. If not, return
	 *         NameProvider.NO_NAME_STRING.
	 */
	public String getDescription();



}
