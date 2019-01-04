package org.jlab.coda.jevio;

import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamWriter;

/**
 * This holds a CODA Bank structure. Mostly it has a header
 * (a <code>BankHeader</code>) and the raw data stored as an
 * byte array.
 * 
 * @author heddle
 * 
 */
public class EvioBank extends BaseStructure implements Cloneable {

	/**
	 * The XML record tag for a segment.
	 */
	public static final String ELEMENT_NAME = "bank";

    /** For general convenience, allow an object to be attached to an evio bank. */
    transient protected Object attachment;


	/**
	 * Null constructor for a bank.
	 */
	public EvioBank() {
		super(new BankHeader());
	}

	/**
	 * Constructor using a provided BankHeader
	 * 
	 * @param bankHeader the header to use.
	 * @see BankHeader
	 */
	public EvioBank(BankHeader bankHeader) {
		super(bankHeader);
	}
	
	/**
	 * This is the general constructor to use for a Bank.
	 * @param tag the tag for the bank header.
	 * @param dataType the (enum) data type for the content of the bank.
	 * @param num sometimes, but not necessarily, an ordinal enumeration.
	 */
	public EvioBank(int tag, DataType dataType, int num) {
		this(new BankHeader(tag, dataType, num));
	}

    /**
     * Get the attached object.
     * @return the attached object
     */
    public Object getAttachment() {
        return attachment;
    }

    /**
     * Set the attached object.
     * @param attachment object to attach to this bank
     */
    public void setAttachment(Object attachment) {
        this.attachment = attachment;
    }

	/**
	 * This implements the abstract method from <code>BaseStructure</code>. It is a convenience method use instead of
	 * "instanceof" to see what type of structure we have. Note: this returns the type of this structure, not the type
	 * of data this structure holds.
	 * 
	 * @return the <code>StructureType</code> of this structure, which is a StructureType.BANK.
	 * @see StructureType
	 */
	@Override
	public StructureType getStructureType() {
		return StructureType.BANK;
	}
	
	/**
	 * Write this bank structure out as an XML record.
     * Integers written in decimal.
	 * @param xmlWriter the writer used to write the events.
	 */
	@Override
	public void toXML(XMLStreamWriter xmlWriter) {
        toXML(xmlWriter, false);
	}

    /**
	 * Write this bank structure out as an XML record.
     * @param xmlWriter the writer used to write the events.
     * @param hex       if true, ints get displayed in hexadecimal
     */
    @Override
    public void toXML(XMLStreamWriter xmlWriter, boolean hex) {

		try {
			commonXMLStart(xmlWriter);
            if (header.dataType.isStructure()) {
			    xmlWriter.writeAttribute("content", xmlContentAttributeName);
            }
			xmlWriter.writeAttribute("data_type", String.format("0x%x", header.dataType.getValue()));
			xmlWriter.writeAttribute("tag", "" + header.tag);
            xmlWriter.writeAttribute("num", "" + header.number);
            xmlWriter.writeAttribute("length", "" + header.length);
            xmlWriter.writeAttribute("ndata", "" + getNumberDataItems());
            increaseXmlIndent();
			commonXMLDataWrite(xmlWriter, hex);
            decreaseXmlIndent();
			commonXMLClose(xmlWriter, hex);
		}
		catch (XMLStreamException e) {
			e.printStackTrace();
		}
	}

	/**
	 * Get the element name for the bank for writing to XML.
	 * @return the element name for the structure for writing to XML.
	 */
	@Override
	public String getXMLElementName() {
		return ELEMENT_NAME;
	}

    /**
     * Write myself out as a byte array of evio format data
     * in the byte order given by {@link #getByteOrder}.
     * This method is much more efficient than using {@link #write(java.nio.ByteBuffer)}.<p>
     * <b>However, be warned that this method is only useful when a bank has
     * just been read from a file or buffer. Once the user adds data or children
     * to the bank, this method does NOT produce correct results.</b>
     *
     * @return byte array containing evio format data of this bank in currently set byte order
     */
    byte[] toArray() {
        byte[] bArray = new byte[rawBytes.length + header.getHeaderLength()*4];
        // write the header
        header.toArray(bArray, 0, byteOrder);
        // write the rest
        System.arraycopy(rawBytes, 0, bArray, header.getHeaderLength()*4, rawBytes.length);
        return bArray;
    }

    /**
     * Deep clone of BaseStructure does all of the work for us.
     */
    public Object clone() {
        EvioBank bank = (EvioBank) super.clone();
        // Do not include that attachment reference in the clone
        bank.attachment = null;
        return bank;
    }



}
