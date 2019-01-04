package org.jlab.coda.jevio;

import javax.swing.tree.DefaultTreeModel;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamWriter;

/**
 * An event is really just the outer, primary bank. That is, the first structure in an event (aka logical record, aka
 * buffer) must be a bank, probably a bank of banks.
 * 
 * The Event trivially extends bank, just so there can be a distinct <code>EvioEvent</code> class, for clarity.
 * 
 * @author heddle
 * 
 */
public class EvioEvent extends EvioBank {
		
	/** The XML record tag for an event. */
	public static final String ELEMENT_NAME = "event";

	/**
	 * As the event is parsed, it is parsed into a tree.
     * Note that this is only used when parsing.
	 */
	protected DefaultTreeModel treeModel;
	
	/**
	 * This is not the "num" field from the header, but rather a number [1..] indicating
	 * which event this was in the event file from which it was read. 
	 */
	private int eventNumber;

    /** There may be a dictionary in xml associated with this event. Or there may not. */
    private String dictionaryXML;

    /** Has this been parsed yet or not? */
    protected boolean parsed;
    


    /**
	 * Explicit null constructor for evio event.
	 */
	public EvioEvent() {
	}
	
    /**
     * Constructor using a provided BankHeader.
     *
     * @param bankHeader the header to use.
     * @see BankHeader
     */
    public EvioEvent(BankHeader bankHeader) {
        super(bankHeader);
    }

	/**
	 * This is a general constructor to use for an EvioEvent.
     *
	 * @param tag the tag for the event header (which is just a bank header).
	 * @param dataType the (enum) data type for the content of the bank.
	 * @param num sometimes, but not necessarily, an ordinal enumeration.
	 */
	public EvioEvent(int tag, DataType dataType, int num) {
		this(new BankHeader(tag, dataType, num));
	}

    /**
     * Deep clone of BaseStructure does most of the work.
     * Leave EvioBank's attachment as a reference. Only
     * thing we need to do get rid of any cloned treeModel.
     */
    public Object clone() {
        EvioEvent ev = (EvioEvent) super.clone();

        // A treeModel is only created & used if this event was parsed into existence.
        // To avoid keeping this unused and transient object, just get rid of it.
        treeModel = null;

        return ev;
    }

//    /**
//     * Method to used to help reproduce an EvioEvent's treeModel object.
//     *
//     * @param model   Model to add items to
//     * @param parent  parent TreeNode object
//     * @param child   child TreeNode object
//     * @param index   index at which to place child in parent
//     */
//    static private void addChildToTree(DefaultTreeModel model, BaseStructure parent,
//                                BaseStructure child, int index) {
//        // See if the child has children
//        Vector<BaseStructure> kids = child.getChildren();
//
//        // If not, we're a leaf so add to tree model
//        if (kids == null || kids.size() < 1) {
//            model.insertNodeInto(parent, child, index);   // (parent, child, index)
//            return;
//        }
//
//        // If we have kids, add each to model recursively
//        int i=0;
//        for (BaseStructure kid : kids) {
//            addChildToTree(model, child, kid, i++);
//        }
//    }

    /**
     * Is there an XML dictionary associated with this event?
     * @return <code>true</code> if there is an XML dictionary associated with this event,
     *         else <code>false</code>
     */
    public boolean hasDictionaryXML() {
        return dictionaryXML != null;
    }

    /**
     * Get the XML dictionary associated with this event if there is one.
     * @return the XML dictionary associated with this event if there is one, else null
     */
    public String getDictionaryXML() {
        return dictionaryXML;
    }

    /**
     * Set the XML dictionary associated with this event.
     * @param dictionaryXML the XML dictionary associated with this event.
     */
    public void setDictionaryXML(String dictionaryXML) {
        this.dictionaryXML = dictionaryXML;
    }

//	/**
//	 * Create a string representation of the event.
//	 * @return a string representation of the event.
//	 */
//	@Override
//	public  String toStringOrig() {
//
//		String description = getDescription();
//		if (INameProvider.NO_NAME_STRING.equals(description)) {
//			description = null;
//		}
//
//		String s = "<Event> " + " len (ints): " + header.length + " " + " tag: " + header.tag + " num: " + header.number;
//        if (header.dataType.isStructure()) {
//            s += " content: " + xmlContentAttributeName;
//        }
//		if (description != null) {
//			s += " [" + description + "]";
//		}
//		return s;
//	}

    /**
     * Obtain a string representation of the event.
     * @return a string representation of the event.
     */
    public String toString() {

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
            sb.append("<Event> has ");
            sb.append(header.getDataType());
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
            sb.append("  dataLen=0");
        }
        else {
            sb.append("  dataLen=");
            sb.append(rawBytes.length/4);
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
     * Get the tree model representing this event. This would have been generated
     * as the event was being parsed.
     * @return the tree model representing this event.
     */
    public DefaultTreeModel getTreeModel() {
        return treeModel;
    }

	/**
	 * Inserts a child structure into the event's JTree. This is called when the event is being parsed,
	 * and when an event is being created.
	 * @param child the child structure being added to the tree.
	 * @param parent the parent structure of the new child.
	 */
	public void insert(BaseStructure child, BaseStructure parent) {
		treeModel.insertNodeInto(child, parent, parent.getChildCount());
	}
	
    /**
     * This returns a number [1..] indicating which event this was in the event file from which
     * it was read. It is not the "num" field from the header.
     * @return a number [1..] indicating which event this was in the event file from which
     * it was read.
     */
    public int getEventNumber() {
        return eventNumber;
    }

	/**
	 * This sets a number [1..] indicating which event this was in the event file from which
	 * it was read. It is not the "num" field from the header.
	 * @param eventNumber a number [1..] indicating which event this was in the event file from which
	 * it was read.
	 */
	public void setEventNumber(int eventNumber) {
		this.eventNumber = eventNumber;
	}

	/**
	 * Write this event structure out as an XML record.
     * Integers written in decimal.
	 * @param xmlWriter the writer used to write the events.
	 */
	@Override
	public void toXML(XMLStreamWriter xmlWriter) {
        toXML(xmlWriter, false);
	}

    /**
     * Write this event structure out as an XML record.
     * @param xmlWriter the writer used to write the events.
     * @param hex       if true, ints get displayed in hexadecimal
     */
    @Override
    public void toXML(XMLStreamWriter xmlWriter, boolean hex) {

        try {
            int totalLen = header.length + 1;
            increaseXmlIndent();
            xmlWriter.writeCharacters(xmlIndent);
            xmlWriter.writeComment(" Buffer " + getEventNumber() + " contains " + totalLen + " words (" + 4*totalLen + " bytes)");
            commonXMLStart(xmlWriter);
            xmlWriter.writeAttribute("format", "evio");
            xmlWriter.writeAttribute("count", "" + getEventNumber());
            if (header.dataType.isStructure()) {
                xmlWriter.writeAttribute("content", xmlContentAttributeName);
            }
            xmlWriter.writeAttribute("data_type", String.format("0x%x", header.dataType.getValue()));
            xmlWriter.writeAttribute("tag", "" + header.tag);
            xmlWriter.writeAttribute("num", "" + header.number);
            xmlWriter.writeAttribute("length", "" + header.length);
            increaseXmlIndent();
            commonXMLDataWrite(xmlWriter, hex);
            decreaseXmlIndent();
            commonXMLClose(xmlWriter, hex);
            xmlWriter.writeCharacters("\n");
            decreaseXmlIndent();
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
