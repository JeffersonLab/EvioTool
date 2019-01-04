package org.jlab.coda.jevio;

import java.nio.*;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.util.*;
import java.io.UnsupportedEncodingException;
import java.io.StringWriter;

import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamWriter;
import javax.xml.stream.XMLOutputFactory;

/**
 * This is the base class for all evio structures: Banks, Segments, and TagSegments.
 * It implements <code>MutableTreeNode</code> because a tree representation of
 * events is created when a new event is parsed.<p>
 * Note that using an EventBuilder for the same event in more than one thread
 * can cause problems. For example the boolean lengthsUpToDate in this class
 * would need to be volatile.
 * 
 * @author heddle
 * @author timmer - add byte order tracking, make setAllHeaderLengths more efficient
 *
 */
public abstract class BaseStructure implements Cloneable, IEvioStructure, MutableTreeNode, IEvioWriter {

	/**
	 * Holds the header of the bank.
	 */
	protected BaseStructureHeader header;

    /**
     * The raw data of the structure.
     * Storing data as raw bytes limits the # of data elements to
     * Integer.MAX_VALUE/size_of_data_type_in_bytes.
     */
    protected byte rawBytes[];

    /**
     * Used if raw data should be interpreted as chars.
     * The reason rawBytes is not used directly is because
     * it may be padded and it may not, depending on whether
     * this object was created by EvioReader or by EventBuilder, etc., etc.
     * We don't want to return rawBytes when a user calls getByteData() if there
     * are padding bytes in it.
     */
    protected byte charData[];

	/**
	 * Used if raw data should be interpreted as ints.
	 */
	protected int intData[];

	/**
	 * Used if raw data should be interpreted as longs.
	 */
	protected long longData[];

	/**
	 * Used if raw data should be interpreted as shorts.
	 */
	protected short shortData[];

	/**
	 * Used if raw data should be interpreted as doubles.
	 */
	protected double doubleData[];

    /**
     * Used if raw data should be interpreted as doubles.
     */
    protected float floatData[];

    /**
     * Used if raw data should be interpreted as composite type.
     */
    protected CompositeData[] compositeData;

    /**
     * Used if raw data should be interpreted as a string.
     */
    protected StringBuilder stringData;

    /**
     * Used if raw data should be interpreted as a string.
     */
    protected ArrayList<String> stringsList;

    /**
     * Keep track of end of the last string added to stringData (including null but not padding).
     */
    protected int stringEnd;

    /**
     * The number of stored data items like number of banks, ints, floats, etc.
     * (not the size in ints or bytes). Some items may be padded such as shorts
     * and bytes. This will tell the meaningful number of such data items.
     * In the case of containers, returns number of bytes not in header.
     */
    protected int numberDataItems;

    /** Bytes with which to pad short and byte data. */
    private static byte[] padValues = {0,0,0};

    /** Number of bytes to pad short and byte data. */
    private static int[]  padCount  = {0,3,2,1};

    /**
     * Give the XML output proper indentation.
     */
    protected String xmlIndent = "";

    /**
     * Name of this object as an XML element.
     */
    protected String xmlElementName = "unknown";   //TODO: get rid of this init val

    /**
     * Name of this object's contents as an XML attribute if it is a structure type.
     */
    protected String xmlContentAttributeName = "unknown32";  //TODO: get rid of this init val

    /**
     * Endianness of the raw data if appropriate,
     * either {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}.
     */
    protected ByteOrder byteOrder;

	/**
	 * The parent of the structure. If it is an "event", the parent is null. This is used for creating trees for a
	 * variety of purposes (not necessarily graphical.)
	 */
	private BaseStructure parent;

	/**
	 * Holds the children of this structure. This is used for creating trees for a variety of purposes
     * (not necessarily graphical.)
	 */
	protected ArrayList<BaseStructure> children;

    /**
     * Keep track of whether header length data is up-to-date or not.
     */
    protected boolean lengthsUpToDate;

    /**
     * Is this structure a leaf? Leaves are structures with no children.
     */
    protected boolean isLeaf = true;


    /**
	 * Constructor using a provided header
	 * 
	 * @param header the header to use.
	 * @see BaseStructureHeader
	 */
	public BaseStructure(BaseStructureHeader header) {
		this.header = header;
        // by default we're big endian since this is java
        byteOrder = ByteOrder.BIG_ENDIAN;
        setXmlNames();
	}

    /**
     * This method does a partial copy and is designed to help convert
     * between banks, segments,and tagsegments in the {@link StructureTransformer}
     * class (hence the name "transfrom").
     * It copies all the data from another BaseStructure object.
     * Children are <b>not</b> copied in the deep clone way,
     * but their references are added to this structure.
     * It does <b>not</b> copy header data or the parent either.
     *
     * @param structure BaseStructure from which to copy data.
     */
    public void transform(BaseStructure structure) {

        if (structure == null) return;

        // reinitialize this base structure first
        rawBytes      = null;
        charData      = null;
        intData       = null;
        longData      = null;
        shortData     = null;
        doubleData    = null;
        floatData     = null;
        compositeData = null;
        stringData    = null;
        stringsList   = null;
        stringEnd     = 0;

        if (children != null) {
            children.clear();
        }
        else {
            children = new ArrayList<BaseStructure>(10);
        }

        // copy over some stuff from other structure
        isLeaf = structure.isLeaf;
        lengthsUpToDate = structure.lengthsUpToDate;
        byteOrder = structure.byteOrder;
        numberDataItems = structure.numberDataItems;

        xmlIndent = structure.xmlIndent;
        xmlElementName = structure.xmlElementName;
        xmlContentAttributeName =  structure.xmlContentAttributeName;

        if (structure.getRawBytes() != null) {
            setRawBytes(structure.getRawBytes().clone());
        }

        DataType type = structure.getHeader().getDataType();

        switch (type)  {
            case SHORT16:
            case USHORT16:
                if (structure.shortData != null) {
                    shortData = structure.shortData.clone();
                }
                break;

            case INT32:
            case UINT32:
                if (structure.intData != null) {
                    intData = structure.intData.clone();
                }
                break;

            case LONG64:
            case ULONG64:
                if (structure.longData != null) {
                    longData = structure.longData.clone();
                }
                break;

            case FLOAT32:
                if (structure.floatData != null) {
                    floatData = structure.floatData.clone();
                }
                break;

            case DOUBLE64:
                if (structure.doubleData != null) {
                    doubleData = structure.doubleData.clone();
                }
                break;

            case CHAR8:
            case UCHAR8:
                if (structure.charData != null) {
                    charData = structure.charData.clone();
                }
                break;

            case CHARSTAR8:
                if (structure.stringsList != null) {
                    stringsList = new ArrayList<String>(structure.stringsList.size());
                    stringsList.addAll(structure.stringsList);
                    stringData = new StringBuilder(structure.stringData);
                    stringEnd = structure.stringEnd;
                }
                break;

            case COMPOSITE:
                if (structure.compositeData != null) {
                    int len = structure.compositeData.length;
                    compositeData = new CompositeData[len];
                    for (int i=0; i < len; i++) {
                        compositeData[i] = (CompositeData) structure.compositeData[i].clone();
                    }
                }
                break;

            case ALSOBANK:
            case ALSOSEGMENT:
            case BANK:
            case SEGMENT:
            case TAGSEGMENT:
                for (BaseStructure kid : structure.children) {
                    children.add(kid);
                }
                break;

            default:
        }
    }

//    static public BaseStructure copy(BaseStructure structure) {
//        return (BaseStructure)structure.clone();
//    }

    /**
     * Clone this object. First call the Object class's clone() method
     * which creates a bitwise copy. Then clone all the mutable objects
     * so that this method does a safe (not deep) clone. This means all
     * children get cloned as well. Remind me again why anyone would
     * want to clone their bratty kids?
     */
    public Object clone() {
        try {
            // "bs" is a bitwise copy. Do a deep clone of all object references.
            BaseStructure bs = (BaseStructure)super.clone();

            // Clone the header
            bs.header = (BaseStructureHeader) header.clone();

            // Clone raw bytes
            if (rawBytes != null) {
                bs.rawBytes = rawBytes.clone();
            }

            // Clone data
            switch (header.getDataType())  {
                case SHORT16:
                case USHORT16:
                    if (shortData != null) {
                        bs.shortData = shortData.clone();
                    }
                    break;

                case INT32:
                case UINT32:
                    if (intData != null) {
                        bs.intData = intData.clone();
                    }
                    break;

                case LONG64:
                case ULONG64:
                    if (longData != null) {
                        bs.longData = longData.clone();
                    }
                    break;

                case FLOAT32:
                    if (floatData != null) {
                        bs.floatData = floatData.clone();
                    }
                    break;

                case DOUBLE64:
                    if (doubleData != null) {
                        bs.doubleData = doubleData.clone();
                    }
                    break;

                case CHAR8:
                case UCHAR8:
                    if (charData != null) {
                        bs.charData = charData.clone();
                    }
                    break;

                case CHARSTAR8:
                    if (stringsList != null) {
                        bs.stringsList = new ArrayList<String>(stringsList.size());
                        bs.stringsList.addAll(stringsList);
                        bs.stringData = new StringBuilder(stringData);
                        bs.stringEnd  = stringEnd;
                    }
                    break;

                case COMPOSITE:
                    if (compositeData != null) {
                        int len = compositeData.length;
                        bs.compositeData = new CompositeData[len];
                        for (int i=0; i < len; i++) {
                            bs.compositeData[i] = (CompositeData) compositeData[i].clone();
                        }
                    }
                    break;

                case ALSOBANK:
                case ALSOSEGMENT:
                case BANK:
                case SEGMENT:
                case TAGSEGMENT:
                    // Create kid storage since we're a container type
                    bs.children = new ArrayList<BaseStructure>(10);

                    // Clone kids
                    for (BaseStructure kid : children) {
                        bs.children.add((BaseStructure)kid.clone());
                    }
                    break;

                default:
            }

            // Do NOT clone the parent, just keep it as a reference.

            return bs;
        }
        catch (CloneNotSupportedException e) {
            return null;
        }
    }

    /** Clear all existing data from a non-container structure. */
    private void clearData() {

        if (header.getDataType().isStructure()) return;

        rawBytes        = null;
        charData        = null;
        intData         = null;
        longData        = null;
        shortData       = null;
        doubleData      = null;
        floatData       = null;
        compositeData   = null;
        stringData      = null;
        stringsList     = null;
        stringEnd       = 0;
        numberDataItems = 0;
    }

	/**
	 * A convenience method use instead of "instanceof" to see what type of structure we have. Note: this returns the
	 * type of this structure, not the type of data (the content type) this structure holds.
	 * 
	 * @return the <code>StructureType</code> of this structure.
	 * @see StructureType
	 */
	public abstract StructureType getStructureType();

    /**
     * This is a method that must be filled in by subclasses. Write this structure out as an XML record.
     *
     * @param xmlWriter the writer used to write the events. It is tied to an open file.
     */
    public abstract void toXML(XMLStreamWriter xmlWriter);

	/**
	 * Get the element name for the structure for writing to XML.
	 * 
	 * @return the element name for the structure for writing to XML.
	 */
	public abstract String getXMLElementName();

    /**
     * Write this structure out as an XML format string.
     */
    public String toXML() {

        try {
            StringWriter sWriter = new StringWriter();
            XMLStreamWriter xmlWriter = XMLOutputFactory.newInstance().createXMLStreamWriter(sWriter);

            toXML(xmlWriter);
            return sWriter.toString();
        }
        catch (XMLStreamException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Set the name of this object's XML element and also content type attribute if content is structure type.
     */
    protected void setXmlNames() {

        // is this structure holding a structure type container?
        boolean holdingStructureType = false;

        // type of data held by this container type
        DataType type = header.getDataType();
        if (type == null) return;

        switch (type) {
            case CHAR8:
                xmlElementName = "int8"; break;
            case UCHAR8:
                xmlElementName = "uint8"; break;
            case SHORT16:
                xmlElementName = "int16"; break;
            case USHORT16:
                xmlElementName = "uint16"; break;
            case INT32:
                xmlElementName = "int32"; break;
            case UINT32:
                xmlElementName = "uint32"; break;
            case LONG64:
                xmlElementName = "int64"; break;
            case ULONG64:
                xmlElementName = "uint64"; break;
            case FLOAT32:
                xmlElementName = "float32"; break;
            case DOUBLE64:
                xmlElementName = "float64"; break;
            case CHARSTAR8:
                xmlElementName = "string"; break;
            case COMPOSITE:
                xmlElementName = "composite"; break;

            case UNKNOWN32:
                holdingStructureType = true;
                xmlContentAttributeName = "unknown32";
                break;

            case TAGSEGMENT:
//            case ALSOTAGSEGMENT:
                holdingStructureType = true;
                xmlContentAttributeName = "tagsegment";
                break;

            case SEGMENT:
            case ALSOSEGMENT:
                holdingStructureType = true;
                xmlContentAttributeName = "segment";
                break;

            case BANK:
            case ALSOBANK:
                holdingStructureType = true;
                xmlContentAttributeName = "bank";
                break;

            default:
                xmlElementName = "unknown"; break;
        }

        if (holdingStructureType) {
            // Which container type are we (NOT what are we holding)?
            // Because that determines our element name.
            StructureType structType = getStructureType();
            if (structType == StructureType.UNKNOWN32) {
                xmlElementName = "unknown32";
            }
            else {
                xmlElementName = getXMLElementName();
            }
        }

    }

    /**
     * Set the indentation (string of spaces) for more pleasing XML output.
     * @param s the indentation (string of spaces) for more pleasing XML output
     */
    protected void setXmlIndent(String s) {
        xmlIndent = s;
    }

    /**
     * Increase the indentation (string of spaces) of the XML output.
     */
    protected void increaseXmlIndent() {
        xmlIndent += "   ";
    }

    /**
     * Decrease the indentation (string of spaces) of the XML output.
     */
    protected void decreaseXmlIndent() {
        xmlIndent = xmlIndent.substring(0, xmlIndent.length() - 3);
    }

    /**
     * What is the byte order of this data?
     * @return {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}
     */
    public ByteOrder getByteOrder() {
        return byteOrder;
    }

    /**
     * Set the byte order of this data. This method <b>cannot</b> be used to swap data.
     * It is only used to describe the endianness of the rawdata contained.
     * @param byteOrder {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}
     */
    public void setByteOrder(ByteOrder byteOrder) {
        this.byteOrder = byteOrder;
    }

	/**
	 * Is a byte swap required? This is java and therefore big endian. If data is
     * little endian, then a swap is required.
	 * 
	 * @return <code>true</code> if byte swapping is required (data is little endian).
	 */
	public boolean isSwap() {
		return byteOrder == ByteOrder.LITTLE_ENDIAN;
	}

	/**
	 * Get the description from the name provider (dictionary), if there is one.
	 * 
	 * @return the description from the name provider (dictionary), if there is one. If not, return
	 *         NameProvider.NO_NAME_STRING.
	 */
	public String getDescription() {
		return NameProvider.getName(this);
	}


    /**
     * Obtain a string representation of the structure.
     *
     * @return a string representation of the structure.
     */
    public String toString() {
        StructureType stype = getStructureType();
        DataType dtype = header.getDataType();

        String description = getDescription();
        if (INameProvider.NO_NAME_STRING.equals(description)) {
            description = null;
        }

        StringBuilder sb = new StringBuilder(100);
        if (description != null) {
            sb.append("<html><b>");
            sb.append(description);
            sb.append("</b>");
        }
        else {
            sb.append(stype);
            sb.append(" of ");
            sb.append(dtype);
            sb.append("s:  tag=");
            sb.append(header.tag);
            sb.append("(0x");
            sb.append(Integer.toHexString(header.tag));

            sb.append(")");
            if (this instanceof EvioBank) {
                sb.append("  num=");
                sb.append(header.number);
                sb.append("(0x");
                sb.append(Integer.toHexString(header.number));
                sb.append(")");
            }
        }

        // todo: can be more thorough here for datalen calculation

//        sb.append("  len=");
//        sb.append(header.length);
        if (rawBytes == null) {
            sb.append("  dataLen=" + header.padding);
        }
        else {
            sb.append("  dataLen=");
            sb.append(rawBytes.length/4);
        }

        if (header.padding != 0) {
            sb.append("  pad="+header.padding);
        }

        int numChildren = (children == null) ? 0 : children.size();

        if (numChildren > 0) {
            sb.append("  children=");
            sb.append(numChildren);
        }

        if (description != null) {
            sb.append("</html>");
        }

        return sb.toString();
    }

	/**
	 * This is a method from the IEvioStructure Interface. Return the header for this structure.
	 * 
	 * @return the header for this structure.
	 */
    public BaseStructureHeader getHeader() {
		return header;
	}

    /**
     * Get the number of stored data items like number of banks, ints, floats, etc.
     * (not the size in ints or bytes). Some items may be padded such as shorts
     * and bytes. This will tell the meaningful number of such data items.
     * In the case of containers, returns number of bytes not in header.
     *
     * @return number of stored data items (not size or length),
     *         or number of bytes if container
     */
    public int getNumberDataItems() {
        if (isContainer()) {
            numberDataItems = header.getLength() + 1 - header.getHeaderLength();
        }

        // if the calculation has not already been done ...
        if (numberDataItems < 1) {
            // When parsing a file or byte array, it is not fully unpacked until data
            // is asked for specifically, for example as an int array or a float array.
            // Thus we don't know how many of a certain item (say doubles) there is.
            // But we can figure that out now based on the size of the raw data byte array.
            int divisor = 0;
            int padding = 0;
            DataType type = header.getDataType();

            switch (type) {
                case CHAR8:
                case UCHAR8:
                    padding = header.getPadding();
                    divisor = 1; break;
                case SHORT16:
                case USHORT16:
                    padding = header.getPadding();
                    divisor = 2; break;
                case INT32:
                case UINT32:
                case FLOAT32:
                    divisor = 4; break;
                case LONG64:
                case ULONG64:
                case DOUBLE64:
                    divisor = 8; break;
                case CHARSTAR8:
                    String[] s = getStringData();
                    numberDataItems = s.length;
                    break;
                case COMPOSITE:
                    // For this type, numberDataItems is NOT used to
                    // calculate the data length so we're OK returning
                    // any reasonable value here.
                    numberDataItems = 1;
                    if (compositeData != null) numberDataItems = compositeData.length;
                    break;
                default:
            }

            if (divisor > 0 && rawBytes != null) {
                numberDataItems = (rawBytes.length - padding)/divisor;
            }
        }

        return numberDataItems;
    }

    /**
     * Get the length of this structure in bytes, including the header.
     * @return the length of this structure in bytes, including the header.
     */
    public int getTotalBytes() {
        return 4*(header.getLength() + 1);
    }

	/**
	 * Get the raw data of the structure.
	 * 
	 * @return the raw data of the structure.
	 */
	public byte[] getRawBytes() {
		return rawBytes;
	}

	/**
	 * Set the data for the structure.
	 * 
	 * @param rawBytes the structure raw data.
	 */
	public void setRawBytes(byte[] rawBytes) {
		this.rawBytes = rawBytes;
	}

	/**
	 * This is a method from the IEvioStructure Interface. Gets the raw data as an integer array,
     * if the content type as indicated by the header is appropriate.<p>
     * NOTE: since Java does not have unsigned primitives, both INT32 and UINT32
	 * data types will be returned as int arrays. The application will have to deal
     * with reinterpreting signed ints that are negative as unsigned ints.
	 * 
	 * @return the data as an int array, or <code>null</code> if this makes no sense for the given content type.
	 */
	public int[] getIntData() {

        switch (header.getDataType()) {
            case INT32:
            case UINT32:
                if (intData == null) {
                    if (rawBytes == null) {
                        return null;
                    }
                    try {
                        intData = ByteDataTransformer.toIntArray(rawBytes, byteOrder);
                    }
                    catch (EvioException e) {/* should not happen */}
                }
                return intData;
            default:
                return null;
        }
	}

	/**
	 * This is a method from the IEvioStructure Interface. Gets the raw data as a long array,
     * if the content type as indicated by the header is appropriate.<p>
     * NOTE: since Java does not have unsigned primitives, both LONG64 and ULONG64
     * data types will be returned as long arrays. The application will have to deal
     * with reinterpreting signed longs that are negative as unsigned longs.
	 * 
	 * @return the data as an long array, or <code>null</code> if this makes no sense for the given content type.
	 */
	public long[] getLongData() {

        switch (header.getDataType()) {
            case LONG64:
            case ULONG64:
                if (longData == null) {
                    if (rawBytes == null) {
                        return null;
                    }
                    try {
                        longData = ByteDataTransformer.toLongArray(rawBytes, byteOrder);
                    }
                    catch (EvioException e) {/* should not happen */}
                }
                return longData;
            default:
                return null;
        }
	}

	/**
	 * This is a method from the IEvioStructure Interface. Gets the raw data as a float array,
     * if the content type as indicated by the header is appropriate.
	 * 
	 * @return the data as an double array, or <code>null</code> if this makes no sense for the given contents type.
	 */
	public float[] getFloatData() {

        switch (header.getDataType()) {
            case FLOAT32:
                if (floatData == null) {
                    if (rawBytes == null) {
                        return null;
                    }
                    try {
                        floatData = ByteDataTransformer.toFloatArray(rawBytes, byteOrder);
                    }
                    catch (EvioException e) {/* should not happen */}
                }
                return floatData;
            default:
                return null;
        }
	}

	/**
	 * This is a method from the IEvioStructure Interface. Gets the raw data as a double array,
     * if the content type as indicated by the header is appropriate.
	 * 
	 * @return the data as an double array, or <code>null</code> if this makes no sense for the given content type.
	 */
	public double[] getDoubleData() {

        switch (header.getDataType()) {
            case DOUBLE64:
                if (doubleData == null) {
                    if (rawBytes == null) {
                        return null;
                    }
                    try {
                        doubleData = ByteDataTransformer.toDoubleArray(rawBytes, byteOrder);
                    }
                    catch (EvioException e) {/* should not happen */}
                }
                return doubleData;
            default:
                return null;
        }
	}

	/**
	 * This is a method from the IEvioStructure Interface. Gets the raw data as a short array,
     * if the contents type as indicated by the header is appropriate.<p>
     * NOTE: since Java does not have unsigned primitives, both SHORT16 and USHORT16
     * data types will be returned as short arrays. The application will have to deal
     * with reinterpreting signed shorts that are negative as unsigned shorts.
	 *
	 * @return the data as an short array, or <code>null</code> if this makes no sense for the given contents type.
	 */
	public short[] getShortData() {

        switch (header.getDataType()) {
            case SHORT16:
            case USHORT16:
                if (shortData == null) {
                    if (rawBytes == null) {
                        return null;
                    }
                    try {
                        shortData = ByteDataTransformer.toShortArray(rawBytes, header.getPadding(), byteOrder);
                    }
                    catch (EvioException e) {/* should not happen */}
                }
                return shortData;
            default:
                return null;
        }
	}

    /**
     * This is a method from the IEvioStructure Interface. Gets the composite data as
     * an array of CompositeData objects, if the content type as indicated by the header
     * is appropriate.<p>
     *
     * @return the data as an array of CompositeData objects, or <code>null</code>
     *         if this makes no sense for the given content type.
     * @throws EvioException if the data is internally inconsistent
     */
    public CompositeData[] getCompositeData() throws EvioException {

        switch (header.getDataType()) {
            case COMPOSITE:
                if (compositeData == null) {
                    if (rawBytes == null) {
                        return null;
                    }
                    compositeData = CompositeData.parse(rawBytes, byteOrder);
                }
                return compositeData;
            default:
                return null;
        }
    }

	/**
	 * This is a method from the IEvioStructure Interface. Gets the raw data as an byte array,
     * if the contents type as indicated by the header is appropriate.<p>
     * NOTE: since Java does not have unsigned primitives, CHAR8 and UCHAR8
	 * data types will be returned as byte arrays. The application will have to deal
     * with reinterpreting bytes as characters, if necessary.
	 * 
	 * @return the data as an byte array, or <code>null</code> if this makes no sense for the given contents type.
	 */
	public byte[] getByteData() {

        switch (header.getDataType()) {
// for now, NO access to raw data behind stored strings
//            case CHARSTAR8:
//                // CHARSTAR8 data is already padded as part if its format.
//                // If appendStringData() was called, rawBytes was set so we're OK.
//                return rawBytes;
//
            case CHAR8:
            case UCHAR8:
                if (charData == null) {
                    if (rawBytes == null) {
                        return null;
                    }
                    // Get rid of padding on end, if any.
                    charData = new byte[rawBytes.length - header.getPadding()];
                    System.arraycopy(rawBytes, 0, charData, 0, rawBytes.length - header.getPadding());
                }
                return charData;
            default:
                return null;
        }
	}

    /**
     * This is a method from the IEvioStructure Interface. Gets the raw data (ascii) as an
     * array of String objects, if the contents type as indicated by the header is appropriate.
     * For any other behavior, the user should retrieve the data as a byte array and
     * manipulate it in the exact manner desired.<p>
     * Originally, in evio versions 1, 2 and 3, only one string was stored. Recent changes allow
     * an array of strings to be stored and retrieved. The changes are backwards compatible.
     *
     * The following is true about the string raw data format:
     * <ul>
     * <li>All strings are immediately followed by an ending null (0).
     * <li>All string arrays are further padded/ended with at least one 0x4 valued ASCII
     *     char (up to 4 possible).
     * <li>The presence of 1 to 4 ending 4's distinguishes the recent string array version from
     *     the original, single string version.
     * <li>The original version string may be padded with anything after its ending null.
     * <li>Ending non-printing ascii chars (value < 32, = 127) are not included in the string
     *     since they are there for padding.
     * </ul>
     *
     * @return the data as an array of String objects if DataType is CHARSTAR8, or <code>null</code>
     *         if this makes no sense for the given type.
     *
     */
    public String[] getStringData() {

        switch (header.getDataType()) {
        case CHARSTAR8:

            if (stringsList != null) {
                return stringsList.toArray(new String[stringsList.size()]);
            }

            if (stringData == null && rawBytes == null) {
                return null;
            }

            int stringCount = unpackRawBytesToStrings();
            if (stringCount < 1) return null;

            return stringsList.toArray(new String[stringsList.size()]);

        default:
            return null;
        }
    }


    /**
     * This method returns the number of bytes in a raw
     * evio format of the given string array, not including header.
     *
     * @param strings array of String objects to size
     * @return the number of bytes in a raw evio format of the given string array
     * @return 0 if arg is null or has zero length
     */
    static public int stringsToRawSize(String[] strings) {

        if (strings == null || strings.length < 1) {
            return 0;
        }

        int dataLen = 0;
        for (String s : strings) {
            dataLen += s.length() + 1; // don't forget the null
        }

        // Add any necessary padding to 4 byte boundaries.
        // IMPORTANT: There must be at least one '\004'
        // character at the end. This distinguishes evio
        // string array version from earlier version.
        int[] pads = {4,3,2,1};
        dataLen += pads[dataLen%4];

        return dataLen;
    }


    /**
     * This method transforms an array of Strings into raw evio format data,
     * not including header.
     *
     * @param strings array of String objects to transform
     * @return byte array containing evio format string array
     * @return null if arg is null or has zero length
     */
    static public byte[] stringsToRawBytes(String[] strings) {

        if (strings == null || strings.length < 1) {
            return null;
        }

        // create some storage
        int dataLen = 0;
        for (String s : strings) {
            dataLen += s.length() + 1; // don't forget the null
        }
        dataLen += 4; // allow room for maximum padding of 4's
        StringBuilder stringData = new StringBuilder(dataLen);

        for (String s : strings) {
            // add string
            stringData.append(s);
            // add ending null
            stringData.append('\000');
        }

        // Add any necessary padding to 4 byte boundaries.
        // IMPORTANT: There must be at least one '\004'
        // character at the end. This distinguishes evio
        // string array version from earlier version.
        int[] pads = {4,3,2,1};
        switch (pads[stringData.length()%4]) {
            case 4:
                stringData.append("\004\004\004\004");
                break;
            case 3:
                stringData.append("\004\004\004");
                break;
            case 2:
                stringData.append("\004\004");
                break;
            case 1:
                stringData.append('\004');
            default:
        }

        // set raw data
        try {
            return stringData.toString().getBytes("US-ASCII");
        }
        catch (UnsupportedEncodingException e) { /* never happen */ }

        return null;
    }


    /**
     * This method extracts an array of strings from byte array of raw evio string data.
     *
     * @param rawBytes raw evio string data
     * @param offset offset into raw data array
     * @return array of Strings or null if processing error
     */
    static public String[] unpackRawBytesToStrings(byte[] rawBytes, int offset) {

        if (rawBytes == null || offset < 0) return null;

        int length = rawBytes.length - offset;
        StringBuilder stringData = null;
        try {
            // stringData contains all elements of rawBytes
            stringData = new StringBuilder(new String(rawBytes, offset, length, "US-ASCII"));
        }
        catch (UnsupportedEncodingException e) { /* will never happen */ }

        return stringBuilderToStrings(stringData);
    }


    /**
     * This method extracts an array of strings from buffer containing raw evio string data.
     *
     * @param buffer  buffer containing evio string data
     * @param pos     position of string data in buffer
     * @param length  length of string data in buffer in bytes
     * @return array of Strings or null if processing error
     */
    static public String[] unpackRawBytesToStrings(ByteBuffer buffer, int pos, int length) {

        if (buffer == null || pos < 0 || length < 1) return null;

        ByteBuffer stringBuf = buffer.duplicate();
        stringBuf.limit(pos+length).position(pos);

        StringBuilder stringData = null;
        try {
            Charset charset = Charset.forName("US-ASCII");
            CharsetDecoder decoder = charset.newDecoder();
            stringData = new StringBuilder(decoder.decode(stringBuf));
        }
        catch (CharacterCodingException e) {/* should not happen */}

        return stringBuilderToStrings(stringData);
    }

     // TODO: try this for performance reasons
    /**
     * This method extracts an array of strings from buffer containing raw evio string data.
     *
     * @param buffer  buffer containing evio string data
     * @param pos     position of string data in buffer
     * @param length  length of string data in buffer in bytes
     * @return array of Strings or null if processing error
     */
    static String[] unpackRawBytesToStringsNew(ByteBuffer buffer, int pos, int length) {

        if (buffer == null || pos < 0 || length < 1) return null;

        int oldPos = buffer.position();
        int oldLim = buffer.limit();

        StringBuilder stringData = null;
        try {
            Charset charset = Charset.forName("US-ASCII");
            CharsetDecoder decoder = charset.newDecoder();
            stringData = new StringBuilder(decoder.decode(buffer));
        }
        catch (CharacterCodingException e) {/* should not happen */}

        String[] strs = stringBuilderToStrings(stringData);
        buffer.limit(oldLim).position(oldPos);

        return  strs;
    }


    /**
     * This method extracts an array of strings from StringBuilder containing string data.
     *
     * @param stringData  buffer containing string data
     * @return array of Strings or null if processing error
     */
    static private String[] stringBuilderToStrings(StringBuilder stringData) {

        // Each string is terminated with a null (char val = 0)
        // and in addition, the end is padded by ASCII 4's (char val = 4).
        // However, in the legacy versions of evio, there is only one
        // null-terminated string and anything as padding. To accommodate legacy evio, if
        // there is not an ending ASCII value 4, anything past the first null is ignored.
        // After doing so, split at the nulls. Do not use the String
        // method "split" as any empty trailing strings are unfortunately discarded.

        boolean newVersion = true;
        if (stringData.charAt(stringData.length() - 1) != '\004') {
            newVersion = false;
        }

        char c;
        ArrayList<String> stringsList = new ArrayList<String>(20);
        ArrayList<Integer> nullIndexList = new ArrayList<Integer>(20);

        for (int i=0; i < stringData.length(); i++) {
            c = stringData.charAt(i);
            if ( c == '\000' ) {
                nullIndexList.add(i);
                // only 1 null terminated string originally in evio v2,3
                if (!newVersion) {
                    break;
                }
            }
            // Look for any non-printing/control characters (not including null)
            // and end the string there. Allow whitespace.
            else if ( (c < 32 || c == 127) && !Character.isWhitespace(c)) {
                break;
            }
        }

        // One thing to consider is in the old version, if there is no
        // null at the end, just pretend like there is one.

        int firstIndex=0;
        for (int nullIndex : nullIndexList) {
            stringsList.add(stringData.substring(firstIndex, nullIndex));
            firstIndex = nullIndex + 1;
        }

        if (stringsList.size() < 1) return null;

        String strs[] = new String[stringsList.size()];
        stringsList.toArray(strs);

        return strs;
    }


    /**
     * Extract string data from rawBytes array. Make sure rawBytes is in the
     * proper jevio 4.0 format.
     * @return number of strings extracted from bytes
     */
    private int unpackRawBytesToStrings() {

        if (rawBytes == null || rawBytes.length < 4) return 0;

        try {
            // stringData contains all elements of rawBytes
            stringData = new StringBuilder(new String(rawBytes, "US-ASCII"));
        }
        catch (UnsupportedEncodingException e) { /* will never happen */ }

        // Each string is terminated with a null (char val = 0)
        // and in addition, the end is padded by ASCII 4's (char val = 4).
        // However, in the legacy versions of evio, there is only one
        // null-terminated string and anything as padding. To accommodate legacy evio, if
        // there is not an ending ASCII value 4, anything past the first null is ignored.
        // After doing so, split at the nulls. Do not use the String
        // method "split" as any empty trailing strings are unfortunately discarded.

        boolean newVersion = true;
        if (stringData.charAt(stringData.length() - 1) != '\004') {
            newVersion = false;
        }

        char c;
        stringsList = new ArrayList<String>(20);
        ArrayList<Integer> nullIndexList = new ArrayList<Integer>(20);

        for (int i=0; i < stringData.length(); i++) {
            c = stringData.charAt(i);
            if ( c == '\000' ) {
//System.out.println("  found null at i = " + i);
                nullIndexList.add(i);
                // only 1 null terminated string originally in evio v2,3
                if (!newVersion) {
//System.out.println("  evio version 1-3 string");
                    break;
                }
            }
            // Look for any non-printing/control characters (not including null)
            // and end the string there. Allow whitespace.
            else if ( (c < 32 || c == 127) && !Character.isWhitespace(c)) {
//System.out.println("  found non-printing c = " + (int)c + " at i = " + i);
                break;
            }
        }

        // One thing to consider is in the old version, if there is no
        // null at the end, just pretend like there is one.

//System.out.println("  split into " + nullIndexList.size() + " strings");
        int firstIndex=0;
        for (int nullIndex : nullIndexList) {
            stringsList.add(stringData.substring(firstIndex, nullIndex));
//System.out.println("    add " + stringData.substring(firstIndex, nullIndex));
            firstIndex = nullIndex + 1;
        }

        // set length of everything up to & including last null (not padding)
        stringEnd = firstIndex;
        stringData.setLength(stringEnd);
//System.out.println("    string = " + stringData.toString());

        // convert this old byte representation into the new one
        if (!newVersion) {
            // Add any necessary padding to 4 byte boundaries.
            // IMPORTANT: There must be at least one '\004'
            // character at the end. This distinguishes evio
            // string array version from earlier version.
            int[] pads = {4,3,2,1};
            switch (pads[stringData.length()%4]) {
                case 4:
                    stringData.append("\004\004\004\004");
                    break;
                case 3:
                    stringData.append("\004\004\004");
                    break;
                case 2:
                    stringData.append("\004\004");
                    break;
                case 1:
                    stringData.append('\004');
                default:
            }

            // reset raw data format to new version
            try {
                rawBytes = stringData.toString().getBytes("US-ASCII");
            }
            catch (UnsupportedEncodingException e) { /* will never happen */ }
        }

        return stringsList.size();
    }

    /**
	 * Get the parent of this structure. The outer event bank is the only structure with a <code>null</code>
     * parent (the only orphan). All other structures have non-null parent giving the container in which they
     * were embedded. Part of the <code>MutableTreeNode</code> interface.
	 * 
	 * @return the parent of this structure.
	 */
	public BaseStructure getParent() {
		return parent;
	}

	/**
	 * Get an enumeration of all the children of this structure. If none, it returns a constant,
     * empty Enumeration. Part of the <code>MutableTreeNode</code> interface.
	 */
	public Enumeration<?> children() {
		if (children == null) {
			return DefaultMutableTreeNode.EMPTY_ENUMERATION;
		}
        return Collections.enumeration(children);
	}

	/**
	 * Add a child at the given index.
	 * 
	 * @param child the child to add.
	 * @param index the target index. Part of the <code>MutableTreeNode</code> interface.
	 */
	public void insert(MutableTreeNode child, int index) {
		if (children == null) {
			children = new ArrayList<BaseStructure>(10);
		}
		children.add(index, (BaseStructure) child);
        child.setParent(this);
        isLeaf = false;
        lengthsUpToDate(false);
	}

	/**
	 * Convenience method to add a child at the end of the child list. In this application
	 * where are not concerned with the ordering of the children.
	 * 
	 * @param child the child to add. It will be added to the end of child list.
	 */
	public void insert(MutableTreeNode child) {
		if (children == null) {
			children = new ArrayList<BaseStructure>(10);
		}
		//add to end
		insert(child, children.size());
	}

	/**
	 * Removes the child at index from the receiver. Part of the <code>MutableTreeNode</code> interface.
	 * 
	 * @param index the target index for removal.
	 */
	public void remove(int index) {
        if (children == null) return;
        BaseStructure bs = children.remove(index);
        bs.setParent(null);
        if (children.size() < 1) isLeaf = true;
        lengthsUpToDate(false);
	}

	/**
	 * Removes the child. Part of the <code>MutableTreeNode</code> interface.
	 * 
	 * @param child the child node being removed.
	 */
	public void remove(MutableTreeNode child) {
        if (children == null || child == null) return;
		children.remove(child);
        child.setParent(null);
        if (children.size() < 1) isLeaf = true;
        lengthsUpToDate(false);
	}

	/**
	 * Remove this node from its parent. Part of the <code>MutableTreeNode</code> interface.
	 */
	public void removeFromParent() {
		if (parent != null) parent.remove(this);
	}

	/**
	 * Set the parent for this node. Part of the <code>MutableTreeNode</code> interface.
	 */
	public void setParent(MutableTreeNode parent) {
		this.parent = (BaseStructure) parent;
	}

    /**
     * Visit all the structures in this structure (including the structure itself --
     * which is considered its own descendant).
     * This is similar to listening to the event as it is being parsed,
     * but is done to a complete (already) parsed event.
     *
     * @param listener an listener to notify as each structure is visited.
     */
    public void vistAllStructures(IEvioListener listener) {
        visitAllDescendants(this, listener, null);
    }

    /**
     * Visit all the structures in this structure (including the structure itself --
     * which is considered its own descendant) in a depth first manner.
     *
     * @param listener an listener to notify as each structure is visited.
     * @param filter an optional filter that must "accept" structures before
     *               they are passed to the listener. If <code>null</code>, all
     *               structures are passed. In this way, specific types of
     *               structures can be captured.
     */
    public void vistAllStructures(IEvioListener listener, IEvioFilter filter) {
        visitAllDescendants(this, listener, filter);
    }

    /**
     * Visit all the descendants of a given structure
     * (which is considered a descendant of itself.)
     *
     * @param structure the starting structure.
     * @param listener an listener to notify as each structure is visited.
     * @param filter an optional filter that must "accept" structures before
     *               they are passed to the listener. If <code>null</code>, all
     *               structures are passed. In this way, specific types of
     *               structures can be captured.
     */
    private void visitAllDescendants(BaseStructure structure, IEvioListener listener, IEvioFilter filter) {
        if (listener != null) {
            boolean accept = true;
            if (filter != null) {
                accept = filter.accept(structure.getStructureType(), structure);
            }

            if (accept) {
                listener.gotStructure(this, structure);
            }
        }

        if (!(structure.isLeaf())) {
            for (BaseStructure child : structure.getChildren()) {
                visitAllDescendants(child, listener, filter);
            }
        }
    }

    /**
     * Visit all the descendant structures, and collect those that pass a filter.
     * @param filter the filter that must be passed. If <code>null</code>,
     *               this will return all the structures.
     * @return a collection of all structures that are accepted by a filter.
     */
    public List<BaseStructure> getMatchingStructures(IEvioFilter filter) {
        final Vector<BaseStructure> structures = new Vector<BaseStructure>(25, 10);

        IEvioListener listener = new IEvioListener() {
            public void startEventParse(BaseStructure structure) { }

            public void endEventParse(BaseStructure structure) { }

            public void gotStructure(BaseStructure topStructure, IEvioStructure structure) {
                structures.add((BaseStructure)structure);
            }
        };

        vistAllStructures(listener, filter);

        if (structures.size() == 0) {
            return null;
        }
        return structures;
    }

	/**
	 * This method is not relevant for this implementation.
     * An empty implementation is provided to satisfy the interface.
	 * Part of the <code>MutableTreeNode</code> interface.
	 */
	public void setUserObject(Object arg0) {
	}

	/**
	 * Checks whether children are allowed. Structures of structures can have children.
     * Structures of primitive data can not. Thus is is entirely determined by the content type.
     * Part of the <code>MutableTreeNode</code> interface.
	 * 
	 * @return <code>true</code> if this node does not hold primitive data,
     *         i.e., if it is a structure of structures (a container).
	 */
	public boolean getAllowsChildren() {
		return header.getDataType().isStructure();
	}

	/**
	 * Obtain the child at the given index. Part of the <code>MutableTreeNode</code> interface.
	 * 
	 * @param index the target index.
	 * @return the child at the given index or null if none
	 */
	public TreeNode getChildAt(int index) {
        if (children == null) return null;

        BaseStructure b = null;
        try {
            b = children.get(index);
        }
        catch (ArrayIndexOutOfBoundsException e) { }
        return b;
	}

	/**
	 * Get the count of the number of children. Part of the <code>MutableTreeNode</code> interface.
	 * 
	 * @return the number of children.
	 */
	public int getChildCount() {
		if (children == null) {
			return 0;
		}
		return children.size();
	}

	/**
	 * Get the index of a node. Part of the <code>MutableTreeNode</code> interface.
	 * 
	 * @return the index of the target node or -1 if no such node in tree
	 */
	public int getIndex(TreeNode node) {
        if (children == null || node == null) return -1;
		return children.indexOf(node);
	}

	/**
	 * Checks whether this is a leaf. Leaves are structures with no children.
     * All structures that contain primitive data are leaf structures.
	 * Part of the <code>MutableTreeNode</code> interface.<br>
	 * Note: this means that an empty container, say a Bank of Segments that have no segments, is a leaf.
	 * 
	 * @return <code>true</code> if this is a structure with a primitive data type, i.e., it is not a container
	 *         structure that contains other structures.
	 */
	public boolean isLeaf() {
        return isLeaf;
	}
	
	/**
	 * Checks whether this structure is a container, i.e. a structure of structures.
	 * @return <code>true</code> if this structure is a container. This is the same check as 
	 * <code>getAllowsChildren()</code>.
	 */
	public boolean isContainer() {
		return header.getDataType().isStructure();
	}

	/**
	 * This calls the xml write for all of a structure's children.
     * It is used for a depth-first traversal through the tree
	 * when writing an event to xml.
	 * 
	 * @param xmlWriter the writer used to write the events. It is tied to an open file.
	 */
	protected void childrenToXML(XMLStreamWriter xmlWriter) {
		if (children == null) {
			return;
		}
        
        increaseXmlIndent();

		for (BaseStructure structure : children) {
			structure.setXmlIndent(xmlIndent);
            structure.toXML(xmlWriter);
		}

        decreaseXmlIndent();
	}

	/**
	 * All structures have a common start to their xml writing
	 * 
	 * @param xmlWriter the writer used to write the events. It is tied to an open file.
	 */
	protected void commonXMLStart(XMLStreamWriter xmlWriter) {
		try {
			xmlWriter.writeCharacters("\n");
            xmlWriter.writeCharacters(xmlIndent);
			xmlWriter.writeStartElement(xmlElementName);
		}
		catch (XMLStreamException e) {
			e.printStackTrace();
		}
	}

    /**
     * All structures close their xml writing the same way, by drilling down to embedded children.
     *
     * @param xmlWriter the writer used to write the events. It is tied to an open file.
     */
    protected void commonXMLClose(XMLStreamWriter xmlWriter) {
        childrenToXML(xmlWriter);
        try {
            xmlWriter.writeCharacters("\n");
            xmlWriter.writeCharacters(xmlIndent);
            xmlWriter.writeEndElement();
        }
        catch (XMLStreamException e) {
            e.printStackTrace();
        }
    }

	/**
	 * All structures have a common data write for their xml writing
	 * 
	 * @param xmlWriter the writer used to write the events. It is tied to an open file.
	 */
	protected void commonXMLDataWrite(XMLStreamWriter xmlWriter) {

		// only leafs write data
		if (!isLeaf()) {
			return;
		}

		try {
            String s;
            String indent = String.format("\n%s", xmlIndent);

			BaseStructureHeader header = getHeader();
			switch (header.getDataType()) {
			case DOUBLE64:
				double doubledata[] = getDoubleData();
                for (int i=0; i < doubledata.length; i++) {
                    if (i%2 == 0) {
                        xmlWriter.writeCharacters(indent);
                    }
                    s = String.format("%24.16e  ", doubledata[i]);
                    xmlWriter.writeCharacters(s);
                }
				break;

			case FLOAT32:
				float floatdata[] = getFloatData();
                for (int i=0; i < floatdata.length; i++) {
                    if (i%2 == 0) {
                        xmlWriter.writeCharacters(indent);
                    }
                    s = String.format("%14.7e  ", floatdata[i]);
                    xmlWriter.writeCharacters(s);
                }
				break;

			case LONG64:
			case ULONG64:
				long longdata[] = getLongData();
                for (int i=0; i < longdata.length; i++) {
                    if (i%2 == 0) {
                        xmlWriter.writeCharacters(indent);
                    }
                    s = String.format("%20d  ", longdata[i]);
                    xmlWriter.writeCharacters(s);
                }
				break;

			case INT32:
			case UINT32:
				int intdata[] = getIntData();
                for (int i=0; i < intdata.length; i++) {
                    if (i%5 == 0) {
                        xmlWriter.writeCharacters(indent);
                    }
                    s = String.format("%11d  ", intdata[i]);
                    xmlWriter.writeCharacters(s);
                }
				break;

			case SHORT16:
			case USHORT16:
				short shortdata[] = getShortData();
                for (int i=0; i < shortdata.length; i++) {
                    if (i%5 == 0) {
                        xmlWriter.writeCharacters(indent);
                    }
                    s = String.format("%6d  ", shortdata[i]);
                    xmlWriter.writeCharacters(s);
                }
				break;

			case CHAR8:
			case UCHAR8:
				byte bytedata[] = getByteData();
                for (int i=0; i < bytedata.length; i++) {
                    if (i%5 == 0) {
                        xmlWriter.writeCharacters(indent);
                    }
                    s = String.format("%4d  ", bytedata[i]);
                    xmlWriter.writeCharacters(s);
                }
				break;

			case CHARSTAR8:
                String stringdata[] = getStringData();
                for (String str : stringdata) {
                    xmlWriter.writeCharacters("\n");
                    xmlWriter.writeCData(str);
                }
				break;

            case COMPOSITE:
                if (compositeData == null) {
                    try {
                        getCompositeData();
                    }
                    catch (EvioException e) {
                        return;
                    }
                }

                for (int i=0; i < compositeData.length; i++) {
                    compositeData[i].toXML(xmlWriter, this, true);
                }
                break;

            default:
            }
		}
		catch (XMLStreamException e) {
			e.printStackTrace();
		}

	}

	/**
	 * Compute the dataLength. This is the amount of data needed
     * by a leaf of primitives. For non-leafs (structures of
	 * structures) this returns 0. For data types smaller than an
     * int, e.g. a short, it computes assuming padding to an
	 * integer number of ints. For example, if we are writing a byte
     * array of length 3 or 4, the it would return 1. If
	 * the byte array is 5,6,7 or 8 it would return 2;
     *
     * The problem with the original method was that if the data was
     * still packed (only rawBytes was defined), then the data
     * was unpacked - very inefficient. No need to unpack data
     * just to calculate a length. This method doesn't do that.
	 *
	 * @return the amount of ints needed by a leaf of primitives, else 0
	 */
	protected int dataLength() {

		int datalen = 0;

		// only leafs write data
		if (isLeaf()) {
			BaseStructureHeader header = getHeader();

			switch (header.getDataType()) {
			case DOUBLE64:
            case LONG64:
            case ULONG64:
			    datalen = 2 * getNumberDataItems();
				break;

            case INT32:
            case UINT32:
			case FLOAT32:
			    datalen = getNumberDataItems();
				break;

			case SHORT16:
			case USHORT16:
			    datalen = 1 + (getNumberDataItems() - 1) / 2;
				break;

            case CHAR8:
            case UCHAR8:
                datalen = 1 + (getNumberDataItems() - 1) / 4;
                break;

            case CHARSTAR8: // rawbytes contain ascii, already padded
            case COMPOSITE:
                if (rawBytes != null) {
                    datalen = 1 + (rawBytes.length - 1) / 4;
                }
                break;

            default:
            } // switch
		} // isleaf
		return datalen;

	}

    /**
   	 * Get the children of this structure.
     * Use {@link #getChildrenList()} instead.
   	 *
     * @deprecated child structures are no longer keep in a Vector
   	 * @return the children of this structure.
   	 */
   	public Vector<BaseStructure> getChildren() {
        if (children == null) return null;
        return new Vector<BaseStructure>(children);
    }

    /**
   	 * Get the children of this structure.
   	 *
   	 * @return the children of this structure.
   	 */
   	public List<BaseStructure> getChildrenList() {return children;}

    /**
     * Get whether the lengths of all header fields for this structure
     * and all it descendants are up to date or not.
     *
     * @return whether the lengths of all header fields for this structure
     *         and all it descendants are up to date or not.
     */
    protected boolean lengthsUpToDate() {
        return lengthsUpToDate;
    }

    /**
     * Set whether the lengths of all header fields for this structure
     * and all it descendants are up to date or not.
     *
     * @param lengthsUpToDate are the lengths of all header fields for this structure
     *                        and all it descendants up to date or not.
     */
    protected void lengthsUpToDate(boolean lengthsUpToDate) {
        this.lengthsUpToDate = lengthsUpToDate;

        // propagate back up the tree if lengths have been changed
        if (!lengthsUpToDate) {
            if (parent != null) parent.lengthsUpToDate(false);
        }
    }

	/**
	 * Compute and set length of all header fields for this structure and all its descendants.
     * For writing events, this will be crucial for setting the values in the headers.
	 * 
	 * @return the length that would go in the header field (for a leaf).
	 */
	public int setAllHeaderLengthsSemiOrig() throws EvioException {
        // if length info is current, don't bother to recalculate it
        if (lengthsUpToDate) {
            return header.getLength();
        }

		int datalen;

		if (isLeaf()) {
            // # of 32 bit ints for leaves, 0 for empty containers (also considered leaves)
			datalen = dataLength();
		}
		else {
			datalen = 0;

			if (children == null) {
System.err.println("Non leaf with null children!");
				System.exit(1);
			}
			for (BaseStructure child : children) {
				datalen += child.setAllHeaderLengths();
				datalen++; // for the length word of each child
			}
		}

        datalen += header.getHeaderLength() - 1; // length header word

		// set the datalen for the header
		header.setLength(datalen);
        lengthsUpToDate(true);
		return datalen;
	}

    /**
     * Compute and set length of all header fields for this structure and all its descendants.
     * For writing events, this will be crucial for setting the values in the headers.
     *
     * @return the length that would go in the header field (for a leaf).
     * @throws EvioException if the length is too large (> {@link Integer#MAX_VALUE}),
     */
    public int setAllHeaderLengths() throws EvioException {
        // if length info is current, don't bother to recalculate it
        if (lengthsUpToDate) {
            return header.getLength();
        }

        int datalen, len;

        if (isLeaf()) {
            // # of 32 bit ints for leaves, 0 for empty containers (also considered leaves)
            datalen = dataLength();
        }
        else {
            datalen = 0;

            if (children == null) {
System.err.println("Non leaf with null children!");
                System.exit(1);
            }
            for (BaseStructure child : children) {
                len = child.setAllHeaderLengths();
                // Add this check to make sure structure is not being overfilled
                if (Integer.MAX_VALUE - datalen < len) {
                    throw new EvioException("added data overflowed containing structure");
                }
                datalen += len + 1;  // + 1 for the header length word of each child
            }
        }

        len =  header.getHeaderLength() - 1;  // - 1 for length header word
        if (Integer.MAX_VALUE - datalen < len) {
            throw new EvioException("added data overflowed containing structure");
        }

        datalen += len;
//System.out.println("len = " + datalen);
        // set the datalen for the header
        header.setLength(datalen);
        lengthsUpToDate(true);
        return datalen;
    }

	/**
	 * Write myself out a byte buffer with fastest algorithms I could find.
	 *
	 * @param byteBuffer the byteBuffer to write to.
	 * @return the number of bytes written.
	 */
	public int write(ByteBuffer byteBuffer) {

        int startPos = byteBuffer.position();

		// write the header
		header.write(byteBuffer);

        int curPos = byteBuffer.position();

		if (isLeaf()) {
			//BaseStructureHeader header = getHeader();
			switch (header.getDataType()) {
			case DOUBLE64:
                // if data sent over wire or read from file ...
                if (rawBytes != null) {
                    // if bytes do NOT need swapping, this is 36x faster ...
                    if (byteOrder == byteBuffer.order())  {
                        byteBuffer.put(rawBytes, 0, rawBytes.length);
                    }
                    // else if bytes need swapping ...
                    else {
                        DoubleBuffer db = byteBuffer.asDoubleBuffer();
                        DoubleBuffer rawBuf = ByteBuffer.wrap(rawBytes).order(byteOrder).asDoubleBuffer();
                        db.put(rawBuf);
                        byteBuffer.position(curPos + rawBytes.length);
                    }
                }
                // else if user set data thru API (can't-rely-on / no rawBytes array) ...
				else {
                    DoubleBuffer db = byteBuffer.asDoubleBuffer();
                    db.put(doubleData, 0, doubleData.length);
                    byteBuffer.position(curPos + 8*doubleData.length);
				}
				break;

			case FLOAT32:
                if (rawBytes != null) {
                    if (byteOrder == byteBuffer.order()) {
                        byteBuffer.put(rawBytes, 0, rawBytes.length);
                    }
                    else {
                        FloatBuffer db = byteBuffer.asFloatBuffer();
                        FloatBuffer rawBuf = ByteBuffer.wrap(rawBytes).order(byteOrder).asFloatBuffer();
                        db.put(rawBuf);
                        byteBuffer.position(curPos + rawBytes.length);
                    }
                }
				else {
                    FloatBuffer db = byteBuffer.asFloatBuffer();
                    db.put(floatData, 0, floatData.length);
                    byteBuffer.position(curPos + 4*floatData.length);
				}
				break;

			case LONG64:
			case ULONG64:
                if (rawBytes != null) {
                    if (byteOrder == byteBuffer.order()) {
                        byteBuffer.put(rawBytes, 0, rawBytes.length);
                    }
                    else {
                        LongBuffer db = byteBuffer.asLongBuffer();
                        LongBuffer rawBuf = ByteBuffer.wrap(rawBytes).order(byteOrder).asLongBuffer();
                        db.put(rawBuf);
                        byteBuffer.position(curPos + rawBytes.length);
                    }
                }
				else {
                    LongBuffer db = byteBuffer.asLongBuffer();
                    db.put(longData, 0, longData.length);
                    byteBuffer.position(curPos + 8*longData.length);
				}

				break;

			case INT32:
			case UINT32:
                if (rawBytes != null) {
                    if (byteOrder == byteBuffer.order()) {
                        byteBuffer.put(rawBytes, 0, rawBytes.length);
                    }
                    else {
                        IntBuffer db = byteBuffer.asIntBuffer();
                        IntBuffer rawBuf = ByteBuffer.wrap(rawBytes).order(byteOrder).asIntBuffer();
                        db.put(rawBuf);
                        byteBuffer.position(curPos + rawBytes.length);
                    }
                }
				else {
                    IntBuffer db = byteBuffer.asIntBuffer();
                    db.put(intData, 0, intData.length);
                    byteBuffer.position(curPos + 4*intData.length);
				}
				break;

			case SHORT16:
			case USHORT16:
                if (rawBytes != null) {
                    if (byteOrder == byteBuffer.order()) {
                        byteBuffer.put(rawBytes, 0, rawBytes.length);
                    }
                    else {
                        ShortBuffer db = byteBuffer.asShortBuffer();
                        ShortBuffer rawBuf = ByteBuffer.wrap(rawBytes).order(byteOrder).asShortBuffer();
                        db.put(rawBuf);
                        byteBuffer.position(curPos + rawBytes.length);
                    }
                }
				else {
                    ShortBuffer db = byteBuffer.asShortBuffer();
                    db.put(shortData, 0, shortData.length);
                    byteBuffer.position(curPos + 2*shortData.length);

					// might have to pad to 4 byte boundary
                    if (shortData.length % 2 > 0) {
                        byteBuffer.putShort((short) 0);
                    }
				}
				break;

			case CHAR8:
			case UCHAR8:
                if (rawBytes != null) {
                    byteBuffer.put(rawBytes, 0, rawBytes.length);
                }
				else {
                    byteBuffer.put(charData, 0, charData.length);

					// might have to pad to 4 byte boundary
                    byteBuffer.put(padValues, 0, padCount[charData.length%4]);
				}
				break;

            case CHARSTAR8: // rawbytes contains ascii, already padded
                if (rawBytes != null) {
                    byteBuffer.put(rawBytes, 0, rawBytes.length);
                }
                break;

            case COMPOSITE:
                // compositeData object always has rawBytes defined
                if (rawBytes != null) {
                    if (byteOrder == byteBuffer.order()) {
                        byteBuffer.put(rawBytes, 0, rawBytes.length);
                    }
                    else {
//                        System.out.println("Before swap in writing output:");
//                        int[] intA = ByteDataTransformer.getAsIntArray(rawBytes, ByteOrder.BIG_ENDIAN);
//                        for (int i : intA) {
//                            System.out.println("Ox" + Integer.toHexString(i));
//                        }
                        // swap rawBytes
                        byte[] swappedRaw = new byte[rawBytes.length];
                        try {
                            CompositeData.swapAll(rawBytes, 0, swappedRaw, 0,
                                                  rawBytes.length/4, byteOrder);
                        }
                        catch (EvioException e) { /* never happen */ }
                        // write them to buffer
                        byteBuffer.put(swappedRaw, 0, swappedRaw.length);
//                        System.out.println("Swap in writing output:");
//                        intA = ByteDataTransformer.getAsIntArray(swappedRaw, ByteOrder.LITTLE_ENDIAN);
//                        for (int i : intA) {
//                            System.out.println("Ox" + Integer.toHexString(i));
//                        }
                    }
                }
				break;

            default:

            } // switch

		} // isLeaf
		else if (children != null) {
			for (BaseStructure child : children) {
				child.write(byteBuffer);
			}
		} // not leaf

		return byteBuffer.position() - startPos;
	}


    //----------------------------------------------------------------------
    // Methods to append to exising data if any or to set the data if none.
    //----------------------------------------------------------------------


    /**
     * Appends int data to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param data the int data to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
     */
    public void appendIntData(int data[]) throws EvioException {

        // make sure the structure is set to hold this kind of data
        DataType dataType = header.getDataType();
        if ((dataType != DataType.INT32) && (dataType != DataType.UINT32)) {
            throw new EvioException("Tried to append int data to a structure of type: " + dataType);
        }

        // if no data to append, just cave
        if (data == null) {
            return;
        }

        // if no int data ...
        if (intData == null) {
            // if no raw data, things are easy
            if (rawBytes == null) {
                intData = data;
                numberDataItems = data.length;
            }
            // otherwise expand raw data first, then add int array
            else {
                int size1 = rawBytes.length/4;
                int size2 = data.length;

                // TODO: Will need to revise this if using Java 9+ with long array indeces
                if (Integer.MAX_VALUE - size1 < size2) {
                    throw new EvioException("added data overflowed containing structure");
                }
                intData = new int[size1 + size2];
                // unpack existing raw data
                ByteDataTransformer.toIntArray(rawBytes, byteOrder, intData, 0);
                // append new data
                System.arraycopy(data, 0, intData, size1, size2);
                numberDataItems = size1 + size2;
            }
        }
        else {
            int size1 = intData.length; // existing data
            int size2 = data.length;    // new data to append

            // TODO: Will need to revise this if using Java 9+ with long array indeces
            if (Integer.MAX_VALUE - size1 < size2) {
                throw new EvioException("added data overflowed containing structure");
            }
            intData = Arrays.copyOf(intData, size1 + size2);  // extend existing array
            System.arraycopy(data, 0, intData, size1, size2); // append
            numberDataItems += size2;
        }

        // This is not necessary but results in a tremendous performance
        // boost (10x) when writing events over a high speed network. Allows
        // the write to be a byte array copy.
        // Storing data as raw bytes limits the # of elements to Integer.MAX_VALUE/4.
        rawBytes = ByteDataTransformer.toBytes(intData, byteOrder);

        lengthsUpToDate(false);
        setAllHeaderLengths();
    }

    /**
     * Appends int data to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param byteData the data in ByteBuffer form to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
     */
    public void appendIntData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 4) {
            return;
        }

        appendIntData(ByteDataTransformer.toIntArray(byteData));
    }

	/**
	 * Appends short data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param data the short data to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
	 */
	public void appendShortData(short data[]) throws EvioException {
		
		DataType dataType = header.getDataType();
		if ((dataType != DataType.SHORT16) && (dataType != DataType.USHORT16)) {
			throw new EvioException("Tried to append short data to a structure of type: " + dataType);
		}
		
		if (data == null) {
			return;
		}
		
		if (shortData == null) {
            if (rawBytes == null) {
                shortData = data;
                numberDataItems = data.length;
            }
            else {
                int size1 = (rawBytes.length - header.getPadding())/2;
                int size2 = data.length;

                if (Integer.MAX_VALUE - size1 < size2) {
                    throw new EvioException("added data overflowed containing structure");
                }
                shortData = new short[size1 + size2];
                ByteDataTransformer.toShortArray(rawBytes, header.getPadding(),
                                                 byteOrder, shortData, 0);
                System.arraycopy(data, 0, shortData, size1, size2);
                numberDataItems = size1 + size2;
            }
		}
		else {
			int size1 = shortData.length;
			int size2 = data.length;

            if (Integer.MAX_VALUE - size1 < size2) {
                throw new EvioException("added data overflowed containing structure");
            }
			shortData = Arrays.copyOf(shortData, size1 + size2);
			System.arraycopy(data, 0, shortData, size1, size2);
            numberDataItems += size2;
		}

        // If odd # of shorts, there are 2 bytes of padding.
        if (numberDataItems%2 != 0) {
            header.setPadding(2);

            if (Integer.MAX_VALUE - 2*numberDataItems < 2) {
                throw new EvioException("added data overflowed containing structure");
            }

            // raw bytes must include padding
            rawBytes = new byte[2*numberDataItems + 2];
            ByteDataTransformer.toBytes(shortData, byteOrder, rawBytes, 0);
        }
        else {
            header.setPadding(0);
            rawBytes = ByteDataTransformer.toBytes(shortData, byteOrder);
        }

        lengthsUpToDate(false);
		setAllHeaderLengths();
	}

    /**
     * Appends short data to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param byteData the data in ByteBuffer form to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
     */
    public void appendShortData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 2) {
            return;
        }

        appendShortData(ByteDataTransformer.toShortArray(byteData));
    }

	/**
	 * Appends long data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param data the long data to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
	 */
	public void appendLongData(long data[]) throws EvioException {
		
		DataType dataType = header.getDataType();
		if ((dataType != DataType.LONG64) && (dataType != DataType.ULONG64)) {
			throw new EvioException("Tried to append long data to a structure of type: " + dataType);
		}
		
		if (data == null) {
			return;
		}
		
		if (longData == null) {
            if (rawBytes == null) {
                longData = data;
                numberDataItems = data.length;
            }
            else {
                int size1 = rawBytes.length/8;
                int size2 = data.length;

                if (Integer.MAX_VALUE - size1 < size2) {
                    throw new EvioException("added data overflowed containing structure");
                }
                longData = new long[size1 + size2];
                ByteDataTransformer.toLongArray(rawBytes, byteOrder, longData, 0);
                System.arraycopy(data, 0, longData, size1, size2);
                numberDataItems = size1 + size2;
            }
		}
		else {
			int size1 = longData.length;
			int size2 = data.length;

            if (Integer.MAX_VALUE - size1 < size2) {
                throw new EvioException("added data overflowed containing structure");
            }
			longData = Arrays.copyOf(longData, size1 + size2);
			System.arraycopy(data, 0, longData, size1, size2);
            numberDataItems += size2;
		}

        rawBytes = ByteDataTransformer.toBytes(longData, byteOrder);

        lengthsUpToDate(false);
		setAllHeaderLengths();
	}
	
    /**
     * Appends long data to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param byteData the data in ByteBuffer form to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
     */
    public void appendLongData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 8) {
            return;
        }

        appendLongData(ByteDataTransformer.toLongArray(byteData));
    }

	/**
	 * Appends byte data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param data the byte data to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
	 */
	public void appendByteData(byte data[]) throws EvioException {
		
		DataType dataType = header.getDataType();
		if ((dataType != DataType.CHAR8) && (dataType != DataType.UCHAR8)) {
			throw new EvioException("Tried to append byte data to a structure of type: " + dataType);
		}
		
		if (data == null) {
			return;
		}
		
		if (charData == null) {
            if (rawBytes == null) {
                charData = data;
                numberDataItems = data.length;
            }
            else {
                int size1 = rawBytes.length - header.getPadding();
                int size2 = data.length;

                if (Integer.MAX_VALUE - size1 < size2) {
                    throw new EvioException("added data overflowed containing structure");
                }
                charData = new byte[size1 + size2];
                System.arraycopy(rawBytes, 0, charData, 0, size1);
                System.arraycopy(data, 0, charData, size1, size2);
                numberDataItems = size1 + size2;
            }
		}
		else {
			int size1 = charData.length;
			int size2 = data.length;

            if (Integer.MAX_VALUE - size1 < size2) {
                throw new EvioException("added data overflowed containing structure");
            }
			charData = Arrays.copyOf(charData, size1 + size2);
			System.arraycopy(data, 0, charData, size1, size2);
            numberDataItems += data.length;
		}

        // store necessary padding to 4 byte boundaries.
        int padding = padCount[numberDataItems%4];
        header.setPadding(padding);
//System.out.println("# data items = " + numberDataItems + ", padding = " + padding);

        // Array creation sets everything to zero. Only need to copy in data.
        if (Integer.MAX_VALUE - numberDataItems < padding) {
            throw new EvioException("added data overflowed containing structure");
        }
        rawBytes = new byte[numberDataItems + padding];
        System.arraycopy(charData,  0, rawBytes, 0, numberDataItems);
//        System.arraycopy(padValues, 0, rawBytes, numberDataItems, padding); // unnecessary

        lengthsUpToDate(false);
		setAllHeaderLengths();
	}
	
    /**
     * Appends byte data to the structure. If the structure has no data, then this
     * is the same as setting the data. Data is copied in.
     * @param byteData the data in ByteBuffer form to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
     */
    public void appendByteData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 1) {
            return;
        }

        appendByteData(ByteDataTransformer.toByteArray(byteData));
    }

	/**
	 * Appends float data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param data the float data to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
	 */
	public void appendFloatData(float data[]) throws EvioException {
		
		DataType dataType = header.getDataType();
		if (dataType != DataType.FLOAT32) {
			throw new EvioException("Tried to append float data to a structure of type: " + dataType);
		}
		
		if (data == null) {
			return;
		}
		
		if (floatData == null) {
            if (rawBytes == null) {
                floatData = data;
                numberDataItems = data.length;
            }
            else {
                int size1 = rawBytes.length/4;
                int size2 = data.length;

                if (Integer.MAX_VALUE - size1 < size2) {
                    throw new EvioException("added data overflowed containing structure");
                }
                floatData = new float[size1 + size2];
                ByteDataTransformer.toFloatArray(rawBytes, byteOrder, floatData, 0);
                System.arraycopy(data, 0, floatData, size1, size2);
                numberDataItems = size1 + size2;
            }
		}
		else {
			int size1 = floatData.length;
			int size2 = data.length;

            if (Integer.MAX_VALUE - size1 < size2) {
                throw new EvioException("added data overflowed containing structure");
            }
			floatData = Arrays.copyOf(floatData, size1 + size2);
			System.arraycopy(data, 0, floatData, size1, size2);
            numberDataItems += data.length;
		}

        rawBytes = ByteDataTransformer.toBytes(floatData, byteOrder);

        lengthsUpToDate(false);
		setAllHeaderLengths();
	}

    /**
     * Appends float data to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param byteData the data in ByteBuffer form to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
     */
    public void appendFloatData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 4) {
            return;
        }

        appendFloatData(ByteDataTransformer.toFloatArray(byteData));
    }

    /**
     * Appends string to the structure (as ascii). If the structure has no data, then this
     * is the same as setting the data. Don't worry about checking for size limits since
     * jevio structures will never contain a char array > {@link Integer#MAX_VALUE} in size.
     * @param s the string to append (as ascii), or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type
     */
    public void appendStringData(String s) throws EvioException {
        appendStringData(new String[] {s});
    }

    /**
     * Appends an array of strings to the structure (as ascii).
     * If the structure has no data, then this
     * is the same as setting the data. Don't worry about checking for size limits since
     * jevio structures will never contain a char array > {@link Integer#MAX_VALUE} in size.
     * @param s the strings to append (as ascii), or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type
     */
    public void appendStringData(String[] s) throws EvioException {

        DataType dataType = header.getDataType();
        if (dataType != DataType.CHARSTAR8) {
            throw new EvioException("Tried to append string to a structure of type: " + dataType);
        }

        if (s == null) {
            return;
        }

        // if no existing data ...
        if (stringData == null) {
		    // if no raw data, things are easy
            if (rawBytes == null) {
                // create some storage
                stringsList = new ArrayList<String>(s.length > 20 ? s.length + 20 : 20);
                int len = 3; // max padding
                for (int i=0; i < s.length; i++) {
                    len += s.length + 1;
                }
                stringData = new StringBuilder(len);
                numberDataItems = s.length;
            }
            // otherwise expand raw data first, then add string
            else {
                unpackRawBytesToStrings();
                // remove any existing padding
                stringData.delete(stringEnd, stringData.length());
                numberDataItems = stringsList.size() + s.length;
            }
        }
        else {
            // remove any existing padding
            stringData.delete(stringEnd, stringData.length());
            numberDataItems += s.length;
        }

        for (int i=0; i < s.length; i++) {
            // store string
            stringsList.add(s[i]);

            // add string
            stringData.append(s[i]);

            // add ending null
            stringData.append('\000');
        }

        // mark end of data before adding padding
        stringEnd = stringData.length();

        // Add any necessary padding to 4 byte boundaries.
        // IMPORTANT: There must be at least one '\004'
        // character at the end. This distinguishes evio
        // string array version from earlier version.
        int[] pads = {4,3,2,1};
        switch (pads[stringData.length()%4]) {
            case 4:
                stringData.append("\004\004\004\004");
                break;
            case 3:
                stringData.append("\004\004\004");
                break;
            case 2:
                stringData.append("\004\004");
                break;
            case 1:
                stringData.append('\004');
            default:
        }

        // set raw data
        try {
            rawBytes = stringData.toString().getBytes("US-ASCII");
        }
        catch (UnsupportedEncodingException e) { /* will never happen */ }

        lengthsUpToDate(false);
        setAllHeaderLengths();
    }

    /**
	 * Appends double data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param data the double data to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
	 */
	public void appendDoubleData(double data[]) throws EvioException {
		
		DataType dataType = header.getDataType();
		if (dataType != DataType.DOUBLE64) {
			throw new EvioException("Tried to append double data to a structure of type: " + dataType);
		}
		
		if (data == null) {
			return;
		}
		
		if (doubleData == null) {
            if (rawBytes == null) {
			    doubleData = data;
                numberDataItems = data.length;
            }
            else {
                int size1 = rawBytes.length/8;
                int size2 = data.length;

                if (Integer.MAX_VALUE - size1 < size2) {
                    throw new EvioException("added data overflowed containing structure");
                }
                doubleData = new double[size1 + size2];
                ByteDataTransformer.toDoubleArray(rawBytes, byteOrder, doubleData, 0);
                System.arraycopy(data, 0, doubleData, size1, size2);
                numberDataItems = size1 + size2;
            }
		}
		else {
			int size1 = doubleData.length;
			int size2 = data.length;

            if (Integer.MAX_VALUE - size1 < size2) {
                throw new EvioException("added data overflowed containing structure");
            }
			doubleData = Arrays.copyOf(doubleData, size1 + size2);
			System.arraycopy(data, 0, doubleData, size1, size2);
            numberDataItems += data.length;
		}

        rawBytes = ByteDataTransformer.toBytes(doubleData, byteOrder);

        lengthsUpToDate(false);
		setAllHeaderLengths();
	}

    /**
	 * Appends double data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
     * @param byteData the data in ByteBuffer form to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data has too many elements to store in raw byte array (JVM limit)
	 */
    public void appendDoubleData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 8) {
            return;
        }

        appendDoubleData(ByteDataTransformer.toDoubleArray(byteData));
    }

    /**
     * Appends CompositeData objects to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param data the CompositeData objects to append, or set if there is no existing data.
     * @throws EvioException if adding data to a structure of a different data type;
     *                       if data takes up too much memory to store in raw byte array (JVM limit)
	 */
	public void appendCompositeData(CompositeData data[]) throws EvioException {

		DataType dataType = header.getDataType();
		if (dataType != DataType.COMPOSITE) {
			throw new EvioException("Tried to set composite data in a structure of type: " + dataType);
		}

		if (data == null || data.length < 1) {
			return;
		}

        // Composite data is always in the local (in this case, BIG) endian
        // because if generated in JVM that's true, and if read in, it is
        // swapped to local if necessary. Either case it's big endian.
        if (compositeData == null) {
            if (rawBytes == null) {
                compositeData   = data;
                numberDataItems = data.length;
            }
            else {
                // Decode the raw data we have
                CompositeData[] cdArray = CompositeData.parse(rawBytes, byteOrder);

                // Allocate array to hold everything
                int len1 = cdArray.length, len2 = data.length;
                int totalLen = len1 + len2;

                if (Integer.MAX_VALUE - len1 < len2) {
                    throw new EvioException("added data overflowed containing structure");
                }
                compositeData = new CompositeData[totalLen];

                // Fill with existing object first
                for (int i=0; i < len1; i++) {
                    compositeData[i] = cdArray[i];
                }
                // Append new objects
                for (int i=len1; i < totalLen; i++) {
                    compositeData[i] = cdArray[i];
                }
                numberDataItems = totalLen;
            }
        }
        else {
            int len1 = compositeData.length, len2 = data.length;
            int totalLen = len1 + len2;

            if (Integer.MAX_VALUE - len1 < len2) {
                throw new EvioException("added data overflowed containing structure");
            }

            CompositeData[] cdArray = compositeData;
            compositeData = new CompositeData[totalLen];

            // Fill with existing object first
            for (int i=0; i < len1; i++) {
                compositeData[i] = cdArray[i];
            }
            // Append new objects
            for (int i=len1; i < totalLen; i++) {
                compositeData[i] = cdArray[i];
            }
            numberDataItems = totalLen;
        }

        rawBytes  = CompositeData.generateRawBytes(compositeData);
//        int[] intA = ByteDataTransformer.getAsIntArray(rawBytes, ByteOrder.BIG_ENDIAN);
//        for (int i : intA) {
//            System.out.println("Ox" + Integer.toHexString(i));
//        }
        byteOrder = data[0].getByteOrder();

        lengthsUpToDate(false);
		setAllHeaderLengths();
	}


    //----------------------------------------------------------------------
    // Methods to set the data, erasing what may have been there previously.
    //----------------------------------------------------------------------


    /**
     * Set the data in this structure to the given array of ints.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#UINT32} or {@link DataType#INT32},
     * it will be reset in the header to {@link DataType#INT32}.
     *
     * @param data the int data to set to.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setIntData(int data[]) throws EvioException {

        DataType dataType = header.getDataType();
        if ((dataType != DataType.INT32) && (dataType != DataType.UINT32)) {
            header.setDataType(DataType.INT32);
        }

        clearData();
        appendIntData(data);
    }

    /**
     * Set the data in this structure to the ints contained in the given ByteBuffer.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#UINT32} or {@link DataType#INT32},
     * it will be reset in the header to {@link DataType#INT32}.
     *
     * @param byteData the int data in ByteBuffer form.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setIntData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 4) {
            return;
        }

        setIntData(ByteDataTransformer.toIntArray(byteData));
    }

    /**
     * Set the data in this structure to the given array of shorts.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#USHORT16} or {@link DataType#SHORT16},
     * it will be reset in the header to {@link DataType#SHORT16}.
     *
     * @param data the short data to set to.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setShortData(short data[]) throws EvioException {

        DataType dataType = header.getDataType();
        if ((dataType != DataType.SHORT16) && (dataType != DataType.USHORT16)) {
            header.setDataType(DataType.SHORT16);
        }

        clearData();
        appendShortData(data);
    }

    /**
     * Set the data in this structure to the shorts contained in the given ByteBuffer.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#USHORT16} or {@link DataType#SHORT16},
     * it will be reset in the header to {@link DataType#SHORT16}.
     *
     * @param byteData the short data in ByteBuffer form.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setShortData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 2) {
            return;
        }

        setShortData(ByteDataTransformer.toShortArray(byteData));
    }

    /**
     * Set the data in this structure to the given array of longs.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#ULONG64} or {@link DataType#LONG64},
     * it will be reset in the header to {@link DataType#LONG64}.
     *
     * @param data the long data to set to.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setLongData(long data[]) throws EvioException {

        DataType dataType = header.getDataType();
        if ((dataType != DataType.LONG64) && (dataType != DataType.ULONG64)) {
            header.setDataType(DataType.LONG64);
        }

        clearData();
        appendLongData(data);
    }

    /**
     * Set the data in this structure to the longs contained in the given ByteBuffer.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#ULONG64} or {@link DataType#LONG64},
     * it will be reset in the header to {@link DataType#LONG64}.
     *
     * @param byteData the long data in ByteBuffer form.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setLongData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 4) {
            return;
        }

        setLongData(ByteDataTransformer.toLongArray(byteData));
    }

    /**
     * Set the data in this structure to the given array of bytes.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#UCHAR8} or {@link DataType#CHAR8},
     * it will be reset in the header to {@link DataType#CHAR8}.
     *
     * @param data the byte data to set to.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setByteData(byte data[]) throws EvioException {

        DataType dataType = header.getDataType();
        if ((dataType != DataType.CHAR8) && (dataType != DataType.UCHAR8)) {
            header.setDataType(DataType.CHAR8);
        }

        clearData();
        appendByteData(data);
    }

    /**
     * Set the data in this structure to the bytes contained in the given ByteBuffer.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#UCHAR8} or {@link DataType#CHAR8},
     * it will be reset in the header to {@link DataType#CHAR8}.
     *
     * @param byteData the byte data in ByteBuffer form.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setByteData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 1) {
            return;
        }

        setByteData(ByteDataTransformer.toByteArray(byteData));
    }

    /**
     * Set the data in this structure to the given array of floats.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#FLOAT32}, it will be reset in the
     * header to that type.
     *
     * @param data the float data to set to.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setFloatData(float data[]) throws EvioException {

        DataType dataType = header.getDataType();
        if (dataType != DataType.FLOAT32) {
            header.setDataType(DataType.FLOAT32);
        }

        clearData();
        appendFloatData(data);
    }

    /**
     * Set the data in this structure to the floats contained in the given ByteBuffer.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#FLOAT32}, it will be reset in the
     * header to that type.
     *
     * @param byteData the float data in ByteBuffer form.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setFloatData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 1) {
            return;
        }

        setFloatData(ByteDataTransformer.toFloatArray(byteData));
    }

    /**
     * Set the data in this structure to the given array of doubles.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#DOUBLE64}, it will be reset in the
     * header to that type.
     *
     * @param data the double data to set to.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setDoubleData(double data[]) throws EvioException {

        DataType dataType = header.getDataType();
        if (dataType != DataType.DOUBLE64) {
            header.setDataType(DataType.DOUBLE64);
        }

        clearData();
        appendDoubleData(data);
    }

    /**
     * Set the data in this structure to the doubles contained in the given ByteBuffer.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#DOUBLE64}, it will be reset in the
     * header to that type.
     *
     * @param byteData the double data in ByteBuffer form.
     * @throws EvioException if data has too many elements to store in raw byte array (JVM limit)
     */
    public void setDoubleData(ByteBuffer byteData) throws EvioException {

        if (byteData == null || byteData.remaining() < 1) {
            return;
        }

        setDoubleData(ByteDataTransformer.toDoubleArray(byteData));
    }

    /**
     * Set the data in this structure to the given array of Strings.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#CHARSTAR8},it will be reset in the
     * header to that type.
     *
     * @param data the String data to set to.
     */
    public void setStringData(String data[]) {

        DataType dataType = header.getDataType();
        if (dataType != DataType.CHARSTAR8) {
            header.setDataType(DataType.CHARSTAR8);
        }

        clearData();
        try {appendStringData(data);}
        catch (EvioException e) {/* never happen */}
    }

    /**
     * Set the data in this structure to the given array of CompositeData objects.
     * All previously existing data will be gone. If the previous data
     * type was not {@link DataType#COMPOSITE}, it will be reset in the
     * header to that type.
     *
     * @param data the array of CompositeData objects to set to.
     * @throws EvioException if data uses too much memory to store in raw byte array (JVM limit)
     */
    public void setCompositeData(CompositeData data[]) throws EvioException {

        DataType dataType = header.getDataType();
        if (dataType != DataType.COMPOSITE) {
            header.setDataType(DataType.COMPOSITE);
        }

        clearData();
        appendCompositeData(data);
    }

}
