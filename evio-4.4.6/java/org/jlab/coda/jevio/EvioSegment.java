package org.jlab.coda.jevio;

import javax.xml.stream.XMLStreamWriter;
import javax.xml.stream.XMLStreamException;

/**
 * This holds a CODA Segment structure. Mostly it has a header (a <code>SegementHeader</code>) and the raw data stored as an
 * byte array.
 * 
 * @author heddle
 * 
 */
public class EvioSegment extends BaseStructure {

	/**
	 * The XML record tag for a segment.
	 */
	public static final String ELEMENT_NAME = "segment";

	/**
	 * Null constructor creates an empty SegmentHeader.
	 * 
	 * @see SegmentHeader
	 */
	public EvioSegment() {
		this(new SegmentHeader());
	}

	/**
	 * Constructor using a provided SegmentHeader
	 * 
	 * @param segmentHeader the header to use.
	 * @see SegmentHeader
	 */
	public EvioSegment(SegmentHeader segmentHeader) {
		super(segmentHeader);
	}
	
	/**
	 * This is the general constructor to use for a Segment.
	 * @param tag the tag for the segment header.
	 * @param dataType the (enum) data type for the content of the segment.
	 */
	public EvioSegment(int tag, DataType dataType) {
		this(new SegmentHeader(tag, dataType));
	}

	/**
	 * This implements the abstract method from <code>BaseStructure</code>. It is a convenience method use instead of
	 * "instanceof" to see what type of structure we have. Note: this returns the type of this structure, not the type
	 * of data this structure holds.
	 * 
	 * @return the <code>StructureType</code> of this structure, which is a StructureType.SEGMENT.
	 * @see StructureType
	 */
	@Override
	public StructureType getStructureType() {
		return StructureType.SEGMENT;
	}
	
	/**
	 * Write this segment structure out as an XML record.
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
