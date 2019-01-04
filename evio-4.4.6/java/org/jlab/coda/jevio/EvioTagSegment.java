package org.jlab.coda.jevio;

import javax.xml.stream.XMLStreamWriter;
import javax.xml.stream.XMLStreamException;

/**
 * This holds a CODA TagSegment structure. Mostly it has a header (a <code>TagSegementHeader</code>) and the raw data stored as an
 * byte array.
 * 
 * @author heddle
 * 
 */
public class EvioTagSegment extends BaseStructure {

	/**
	 * The XML record tag for a tag segment.
	 */
	public static final String ELEMENT_NAME = "tagsegment";


	/**
	 * Null constructor creates an empty TagSegmentHeader.
	 * 
	 * @see TagSegmentHeader
	 */
	public EvioTagSegment() {
		this(new TagSegmentHeader());
	}

	/**
	 * Constructor using a provided TagSegmentHeader
	 * 
	 * @param tagSegmentHeader the header to use.
	 * @see TagSegmentHeader
	 */
	public EvioTagSegment(TagSegmentHeader tagSegmentHeader) {
		super(tagSegmentHeader);
	}
	
	/**
	 * This is the general constructor to use for a TagSegment.
	 * @param tag the tag for the tag segment header.
	 * @param dataType the (enum) data type for the content of the tag segment.
	 */
	public EvioTagSegment(int tag, DataType dataType) {
		this(new TagSegmentHeader(tag, dataType));
	}

	/**
	 * This implements the abstract method from <code>BaseStructure</code>. It is a convenience method use instead of
	 * "instanceof" to see what type of structure we have. Note: this returns the type of this structure, not the type
	 * of data this structure holds.
	 * 
	 * @return the <code>StructureType</code> of this structure, which is a StructureType.TAGSEGMENT.
	 * @see StructureType
	 */
	@Override
	public StructureType getStructureType() {
		return StructureType.TAGSEGMENT;
	}
	
	/**
	 * Write this tag segment structure out as an XML record.
	 * @param xmlWriter the writer used to write the events.
	 */
	@Override
	public void toXML(XMLStreamWriter xmlWriter) {
		
        try {
            commonXMLStart(xmlWriter);
            if (header.dataType.isStructure()) {
			    xmlWriter.writeAttribute("content", xmlContentAttributeName);
            }
            xmlWriter.writeAttribute("data_type", String.format("0x%x", header.dataType.getValue()));
            xmlWriter.writeAttribute("tag", "" + header.tag);
            xmlWriter.writeAttribute("length", "" + header.length);
            xmlWriter.writeAttribute("ndata", "" + getNumberDataItems());
            increaseXmlIndent();
            commonXMLDataWrite(xmlWriter);
            decreaseXmlIndent();
            commonXMLClose(xmlWriter);
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

}
