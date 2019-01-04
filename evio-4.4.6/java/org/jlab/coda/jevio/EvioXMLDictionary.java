/*
 * Copyright (c) 2015, Jefferson Science Associates
 *
 * Thomas Jefferson National Accelerator Facility
 * Data Acquisition Group
 *
 * 12000, Jefferson Ave, Newport News, VA 23606
 * Phone : (757)-269-7100
 *
 */

package org.jlab.coda.jevio;

import java.io.File;
import java.io.IOException;
import java.io.ByteArrayInputStream;
import java.io.StringWriter;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;



/**
 * This was developed to read the xml dictionary that Maurizio uses for GEMC.
 * It implements INameProvider, just like all other dictionary readers.
 *
 * An assumption in the following class is that each unique tag/num pair
 * corresponds to an equally unique name. In other words, 2 different
 * tag/numm pairs cannot have the same name.
 *
 * Entries with only a tag value and no num are allowed. They will match
 * a tag/num pair if no exact match exists but the tag matches. For such
 * entries, no additional existence of type, format, or description is allowed.
 * 
 * @author heddle
 * @author timmer
 */
@SuppressWarnings("serial")
public class EvioXMLDictionary implements INameProvider {

    /** Element containing entire dictionary. */
    private static String DICT_TOP_LEVEL = "xmlDict";

    /** There is only one type of element which directly defines an entry (strange name). */
    private static String ENTRY = "xmldumpDictEntry";

    /**
     * New, alternate, shortened form of ENTRY.
     * @since 4.0
     */
    private static String ENTRY_ALT = "dictEntry";

    /**
     * Hierarchical container element.
     * @since 4.0
     */
    private static String ENTRY_BANK = "bank";

    /**
     * Hierarchical leaf element.
     * @since 4.0
     */
    private static String ENTRY_LEAF = "leaf";

    /**
     * Description element.
     * @since 4.0
     */
    private static String DESCRIPTION = "description";

    /**
     * The "format" attribute string.
     * @since 4.0
     */
    private static String FORMAT = "format";

    /**
     * The "type" attribute string.
     * @since 4.0
     */
    private static String TYPE = "type";

	/** The "name" attribute string. */
	private static String NAME = "name";

	/** The "tag" attribute string. */
	private static String TAG = "tag";

    /** The "num" attribute string. */
    private static String NUM = "num";



    /**
     * The character used to separate hierarchical parts of names.
     * @since 4.0
     */
    private String delimiter = ".";

    /**
     * This is the heart of the dictionary in which a key is composed of a tag/num
     * pair and other entry data and its corresponding value is a name.
     * Using a hashmap ensures entries are unique.
     * @since 4.0
     */
    private LinkedHashMap<EvioDictionaryEntry,String> dictMap =
            new LinkedHashMap<EvioDictionaryEntry,String>(100);

    /**
     * Some dictionary entries have only a tag and no num. These entries
     * are used only for pretty printing. It matches a tag num pair if
     * there is no exact match in dictMap, but does match a tag in this map.
     * @since 4.1
     */
    private LinkedHashMap<Integer,String> tagOnlyMap = new LinkedHashMap<Integer,String>(100);

    /**
     * This is a hashmap in which the key is a name and the value is composed
     * of a corresponding tag/num pair and other entry data.
     * It's the reverse of the dictMap mapping.
     * @since 4.0
     */
    private LinkedHashMap<String,EvioDictionaryEntry> reverseMap =
            new LinkedHashMap<String,EvioDictionaryEntry>(100);

    /**
     * Top level xml Node object of xml DOM representation of dictionary.
     * @since 4.0
     */
    private Node topLevelDoc;

    /**
     * Keep a copy of the string representation around so toString() only does hard work once.
     * @since 4.1
     */
    private String stringRepresentation;

    /**
     * Keep a copy of the XML string around so toXML() only does hard work once.
     * @since 4.1
     */
    private String xmlRepresentation;



    /**
     * Create an EvioXMLDictionary from an xml file.
     *
     * @param file file containing xml.
     */
    public EvioXMLDictionary(File file) {
        this(getDomObject(file));
    }

    /**
     * Create an EvioXMLDictionary from an xml file.
     *
     * @param file file containing xml.
     * @param delimiter character used to separate hierarchical parts of names.
     */
    public EvioXMLDictionary(File file, String delimiter) {
        this(getDomObject(file), delimiter);
    }

    /**
     * Create an EvioXMLDictionary from an xml string.
     *
     * @param xmlString string containing xml.
     */
    public EvioXMLDictionary(String xmlString) {
        this(getDomObject(xmlString));
    }

    /**
     * Create an EvioXMLDictionary from an xml string.
     *
     * @param xmlString string containing xml.
     * @param delimiter character used to separate hierarchical parts of names.
     */
    public EvioXMLDictionary(String xmlString, String delimiter) {
        this(getDomObject(xmlString), delimiter);
    }

    /**
     * Create an EvioXMLDictionary from an xml Document object.
	 *
	 * @param domDocument DOM object representing xml dictionary.
	 */
	public EvioXMLDictionary(Document domDocument) {
        this(domDocument, null);
    }

    /**
     * Create an EvioXMLDictionary from an xml Document object.
     *
     * @param domDocument DOM object representing xml dictionary.
     * @param delimiter character used to separate hierarchical parts of names.
     */
    public EvioXMLDictionary(Document domDocument, String delimiter) {

        if (domDocument == null) return;
        if (delimiter   != null) this.delimiter = delimiter;

        // Find top level of dictionary inside xml file being parsed (case sensitive!)
        NodeList list = domDocument.getElementsByTagName(DICT_TOP_LEVEL);

        // If no dictionary, just return
        if (list.getLength() < 1) return;

        // Otherwise, pick the first one
        Node topNode = list.item(0);
        topLevelDoc = topNode;

        // Look at all the children, if any
        NodeList kidList = topNode.getChildNodes();
        if (kidList.getLength() < 1) return;

        int tag, num;
        boolean badEntry;
        String name, tagStr, numStr, type, format, description;

        // Pick out elements that are both old & new direct entry elements
        for (int index = 0; index < kidList.getLength(); index++) {
            Node node = kidList.item(index);
            if (node == null) continue;

            // Only looking for "xmldumpDictEntry" and "dictEntry" nodes
            if (!node.getNodeName().equalsIgnoreCase(ENTRY) &&
                !node.getNodeName().equalsIgnoreCase(ENTRY_ALT)) {
                continue;
            }
            if (node.hasAttributes()) {
                tag = num = 0;
                badEntry = false;
                name = numStr = tagStr = type = format = description = null;

                NamedNodeMap map = node.getAttributes();

                // Get the name
                Node attrNode = map.getNamedItem(NAME);
                if (attrNode != null) {
                    name = attrNode.getNodeValue();
                }

                // Get the tag
                attrNode = map.getNamedItem(TAG);
                if (attrNode != null) {
                    tagStr = attrNode.getNodeValue();
                    try {
                        tag = Integer.decode(tagStr);
                        if (tag < 0) badEntry = true;
//System.out.println("Tag, dec = " + tag);
                    }
                    catch (NumberFormatException e) {
                        badEntry = true;
                    }
                }

                // Get the num
                attrNode = map.getNamedItem(NUM);
                if (attrNode != null) {
                    numStr = attrNode.getNodeValue();
                    try {
                        num = Integer.decode(numStr);
                        if (num < 0) badEntry = true;
//System.out.println("Num, dec = " + num);
                    }
                    catch (NumberFormatException e) {
                        badEntry = true;
                    }
                }

                // Get the type, if any
                attrNode = map.getNamedItem(TYPE);
                if (attrNode != null) {
                    type = attrNode.getNodeValue();
                }

                // Look for description node (xml element) as child of entry node
                NodeList children = node.getChildNodes();
                if (children.getLength() > 0) {

                    // Pick out first description element only
                    for (int i = 0; i < children.getLength(); i++) {
                        Node childNode = children.item(i);
                        if (childNode == null) continue;

                        // Only looking for "description" node
                        if (!childNode.getNodeName().equalsIgnoreCase(DESCRIPTION)) {
                            continue;
                        }

                        description = childNode.getTextContent();
//System.out.println("FOUND DESCRIPTION: = " + description);

                        // See if there's a format attribute
                        if (childNode.hasAttributes()) {
                            map = childNode.getAttributes();

                            // Get the format
                            attrNode = map.getNamedItem(FORMAT);
                            if (attrNode != null) {
                                format = attrNode.getNodeValue();
//System.out.println("FOUND FORMAT: = " + format);
                            }
                        }
                        break;
                    }
                }

                // Skip meaningless entries
                if (name == null || tagStr == null || badEntry) {
                    System.out.println("IGNORING badly formatted dictionary entry 1: name = " + name);
                    continue;
                }

                if (numStr == null && (type != null || description != null || format != null)) {
                    System.out.println("IGNORING badly formatted dictionary entry 2: name = " + name);
                    continue;
                }

                // If no num defined, put in different hashmap
                if (numStr == null) {
                    // Only consider it if not already in tag only map ...
                    if (!tagOnlyMap.containsKey(tag)) {
                        // See if this tag exists among the regular entries as well
                        boolean forgetIt = false;
                        for (EvioDictionaryEntry data : dictMap.keySet()) {
                            if (data.getTag() == tag) {
                                forgetIt = true;
                                break;
                            }
                        }

                        if (!forgetIt) {
                            tagOnlyMap.put(tag, name);
                        }
                        else {
                            System.out.println("IGNORING duplicate dictionary entry: name = " + name);
                        }
                    }
                    else {
                        System.out.println("IGNORING duplicate dictionary entry: name = " + name);
                    }
                }
                else {
                    // Transform tag/num pair into single object
                    EvioDictionaryEntry key = new EvioDictionaryEntry(tag, num, type, description, format);

                    // Only add to dictionary if both name and tag/num pair are unique
                    if (!dictMap.containsKey(key) && !reverseMap.containsKey(name))  {
                        dictMap.put(key, name);
                        reverseMap.put(name, key);
                    }
                    else {
                        System.out.println("IGNORING duplicate dictionary entry: name = " + name);
                    }
                }
            }
        }

        // Look at the (new) hierarchical entry elements,
        // recursively, and add all existing entries.
        addHierachicalDictEntries(kidList, null);

	} // end Constructor


    /**
     * Get the map in which the key is the entry name and the value is an object
     * containing its data (tag, num, type, etc.).
     * @return  map in which the key is the entry name and the value is an object
     *          containing its data (tag, num, type, etc.).
     */
    public Map<String, EvioDictionaryEntry> getMap() {
        return Collections.unmodifiableMap(reverseMap);
    }


    /**
     * Takes a list of the children of an xml node, selects the new
     * hierachical elements and converts them into a number of dictionary
     * entries which are added to this object.
     * This method acts recursively since any node may contain children.
     *
     * @param kidList a list of the children of an xml node.
     */
    private void addHierachicalDictEntries(NodeList kidList, String parentName) {

        if (kidList == null || kidList.getLength() < 1) return;

        int tag, num;
        boolean isLeaf, badEntry;
        String name, tagStr, numStr, type, format, description;

        for (int i = 0; i < kidList.getLength(); i++) {

            Node node = kidList.item(i);
            if (node == null) continue;

            isLeaf = node.getNodeName().equalsIgnoreCase(ENTRY_LEAF);

            // Only looking for "bank" and "leaf" nodes
            if (!node.getNodeName().equalsIgnoreCase(ENTRY_BANK) && !isLeaf) {
                continue;
            }

            if (node.hasAttributes()) {
                tag = num = 0;
                badEntry = false;
                name = numStr = tagStr = type = format = description = null;

                NamedNodeMap map = node.getAttributes();

                // Get the name
                Node attrNode = map.getNamedItem(NAME);
                if (attrNode != null) {
                    name = attrNode.getNodeValue();
                }

                // Get the tag
                Node tagNode = map.getNamedItem(TAG);
                if (tagNode != null) {
                    tagStr = tagNode.getNodeValue();
                    try {
                        tag = Integer.decode(tagStr);
                        if (tag < 0) badEntry = true;
                    }
                    catch (NumberFormatException e) {
                        badEntry = true;
                    }
                }

                // Get the num
                Node numNode = map.getNamedItem(NUM);
                if (numNode != null) {
                    numStr = numNode.getNodeValue();
                    try {
                        num = Integer.decode(numStr);
                        if (num < 0) badEntry = true;
                    }
                    catch (NumberFormatException e) {
                        badEntry = true;
                    }
                }

                // Get the type, if any
                attrNode = map.getNamedItem(TYPE);
                if (attrNode != null) {
                    type = attrNode.getNodeValue();
                }

                // Look for description node (xml element) as child of entry node
                NodeList children = node.getChildNodes();
                if (children.getLength() > 0) {

                    // Pick out first description element only
                    for (int j = 0; j < children.getLength(); j++) {
                        Node childNode = children.item(j);
                        if (childNode == null) continue;

                        // Only looking for "description" node
                        if (!childNode.getNodeName().equalsIgnoreCase(DESCRIPTION)) {
                            continue;
                        }

                        description = childNode.getTextContent();
//System.out.println("FOUND DESCRIPTION: = " + description);

                        // See if there's a format attribute
                        if (childNode.hasAttributes()) {
                            map = childNode.getAttributes();

                            // Get the format
                            attrNode = map.getNamedItem(FORMAT);
                            if (attrNode != null) {
                                format = attrNode.getNodeValue();
//System.out.println("FOUND FORMAT: = " + format);
                            }
                        }
                        break;
                    }
                }

                // Skip meaningless entries
                if (name == null || tagStr == null || badEntry) {
                    System.out.println("IGNORING badly formatted dictionary entry 3: name = " + name);
                    continue;
                }

                if (numStr == null && (type != null || description != null || format != null)) {
                    System.out.println("IGNORING badly formatted dictionary entry 4: name = " + name);
                    continue;
                }

                // Create hierarchical name
                if (parentName != null) name = parentName + delimiter + name;

                // If no num defined, put in different hashmap
                if (numStr == null) {
                    // Only consider it if not already in tag only map ...
                    if (!tagOnlyMap.containsKey(tag)) {
                        // See if this tag exists among the regular entries as well
                        boolean forgetIt = false;
                        for (EvioDictionaryEntry data : dictMap.keySet()) {
                            if (data.getTag() == tag) {
                                forgetIt = true;
                                break;
                            }
                        }

                        if (!forgetIt) {
                            tagOnlyMap.put(tag, name);
                        }
                        else {
                            System.out.println("IGNORING duplicate dictionary entry: name = " + name);
                        }
                    }
                    else {
                        System.out.println("IGNORING duplicate dictionary entry: name = " + name);
                    }
                }
                else {
                    // Transform tag/num pair into single object
                    EvioDictionaryEntry key = new EvioDictionaryEntry(tag, num, type, description, format);

                    // Only add to dictionary if both name and tag/num pair are unique
                    if (!dictMap.containsKey(key) && !reverseMap.containsKey(name))  {
                        dictMap.put(key, name);
                        reverseMap.put(name, key);
                    }
                    else {
                        System.out.println("IGNORING duplicate dictionary entry: name = " + name);
                    }
                }

                // Look at this node's children recursively but skip a leaf's kids
                if (!isLeaf) {
                    addHierachicalDictEntries(node.getChildNodes(), name);
                }
                else if (node.hasChildNodes()) {
                    System.out.println("IGNORING children of \"leaf\" element " + name);
                }
            }
        }
    }


    /**
     * Returns the name of a given evio structure.
     *
     * @param structure the structure to find the name of.
     * @return a descriptive name or ??? if none found
     */
    public String getName(BaseStructure structure) {
        int tag = structure.getHeader().getTag();
        int num = structure.getHeader().getNumber();
        return getName(tag, num);
	}


    /**
     * Returns the name associated with the given tag and num.
     *
     * @param tag to find the name of
     * @param num to find the name of
     * @return a descriptive name or ??? if none found
     */
    public String getName(int tag, int num) {
        EvioDictionaryEntry key = new EvioDictionaryEntry(tag, num);

        String name = dictMap.get(key);
        if (name == null) {
            // Check to see if tag matches anything in tagOnlyMap
            name = tagOnlyMap.get(tag);
            if (name == null) {
                return INameProvider.NO_NAME_STRING;
            }
        }

        return name;
	}


    /**
     * Returns the description, if any, associated with the given tag and num.
     *
     * @param tag to find the description of
     * @param num to find the description of
     * @return the description or null if none found
     */
    public String getDescription(int tag, int num) {
        // Need to find the description associated with given tag/num.
        // First create an EntryData object to get associated name.
        // (This works because of overriding the equals() method).
        EvioDictionaryEntry key = new EvioDictionaryEntry(tag, num);

        String name = dictMap.get(key);
        if (name == null) {
            System.out.println("getDescription: no entry for that key (tag/num)");
            return null;
        }

        // Now that we have the name, use that to find the
        // original EntryData object with the description in it.
        EvioDictionaryEntry origKey = reverseMap.get(name);
        if (origKey == null) {
            System.out.println("getDescription: no orig entry for that key (tag/num)");
            return null;
        }

        return origKey.getDescription();
	}


    /**
     * Returns the description, if any, associated with the name of a dictionary entry.
     *
     * @param name dictionary name
     * @return description; null if name or is unknown or no description is associated with it
     */
    public String getDescription(String name) {
        EvioDictionaryEntry entry = reverseMap.get(name);
        if (entry == null) {
            return null;
        }

        return entry.getDescription();
	}


    /**
     * Returns the format, if any, associated with the given tag and num.
     *
     * @param tag to find the format of
     * @param num to find the format of
     * @return the format or null if none found
     */
    public String getFormat(int tag, int num) {
        // Need to find the format associated with given tag/num.
        // First create an EntryData object to get associated name.
        // (This works because of overriding the equals() method).
        EvioDictionaryEntry key = new EvioDictionaryEntry(tag, num);

        String name = dictMap.get(key);
        if (name == null) return null;

        // Now that we have the name, use that to find the
        // original EntryData object with the format in it.
        EvioDictionaryEntry origKey = reverseMap.get(name);
        if (origKey == null) {
            return null;
        }

        return origKey.getFormat();
	}


    /**
     * Returns the format, if any, associated with the name of a dictionary entry.
     *
     * @param name dictionary name
     * @return format; null if name or is unknown or no format is associated with it
     */
    public String getFormat(String name) {
        EvioDictionaryEntry data = reverseMap.get(name);
        if (data == null) {
            return null;
        }

        return data.getFormat();
	}


    /**
     * Returns the type, if any, associated with the given tag and num.
     *
     * @param tag to find the type of
     * @param num to find the type of
     * @return the type or null if none found
     */
    public DataType getType(int tag, int num) {
        // Need to find the type associated with given tag/num.
        // First create an EntryData object to get associated name.
        // (This works because of overriding the equals() method).
        EvioDictionaryEntry key = new EvioDictionaryEntry(tag, num);

        String name = dictMap.get(key);
        if (name == null) return null;

        // Now that we have the name, use that to find the
        // original EntryData object with the type in it.
        EvioDictionaryEntry origKey = reverseMap.get(name);
        if (origKey == null) {
            return null;
        }

        return origKey.getType();
	}


    /**
     * Returns the type, if any, associated with the name of a dictionary entry.
     *
     * @param name dictionary name
     * @return type; null if name or is unknown or no type is associated with it
     */
    public DataType getType(String name) {
        EvioDictionaryEntry data = reverseMap.get(name);
        if (data == null) {
            return null;
        }

        return data.getType();
	}


    /**
     * Returns the tag/num pair, in an int array,
     * corresponding to the name of a dictionary entry.
     *
     * @param name dictionary name
     * @return an integer array in which the first element is the tag
     *         and the second is the num; null if name is unknown
     */
    public int[] getTagNum(String name) {
        // Get the tag/num pair
        EvioDictionaryEntry pair = reverseMap.get(name);
        if (pair == null) {
            return null;
        }

        return new int[] {pair.getTag(), pair.getNum()};
	}


    /**
     * Returns the tag corresponding to the name of a dictionary entry.
     *
     * @param name dictionary name
     * @return tag; -1 if name unknown
     */
    public int getTag(String name) {
        // Get the data
        EvioDictionaryEntry data = reverseMap.get(name);
        if (data == null) {
            for (Map.Entry<Integer, String> entry : tagOnlyMap.entrySet()) {
                if (entry.getValue().equals(name)) {
                    return entry.getKey();
                }
            }
            return -1;
        }

        return data.getTag();
	}


    /**
     * Returns the num corresponding to the name of a dictionary entry.
     *
     * @param name dictionary name
     * @return num; -1 if name unknown
     */
    public int getNum(String name) {
        // Get the data
        EvioDictionaryEntry data = reverseMap.get(name);
        if (data == null) {
            return -1;
        }

        return data.getNum();
	}


	/**
	 * Return a dom object corresponding to an xml file.
	 * 
	 * @param file the XML File object
	 * @return the dom object (or <code>null</code>.)
	 */
	private static Document getDomObject(File file) {
		// get the factory
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		Document dom = null;

		try {
			// Using factory get an instance of document builder
			DocumentBuilder db = dbf.newDocumentBuilder();

			// parse using builder to get DOM representation of the XML file
			dom = db.parse(file);
		}
		catch (ParserConfigurationException e) {
			e.printStackTrace();
		}
		catch (SAXException e) {
			e.printStackTrace();
		}
		catch (IOException e) {
			e.printStackTrace();
		}
        catch (Exception e) {
            e.printStackTrace();
        }

		return dom;
	}


    /**
     * Return a dom object corresponding to an xml string.
     *
     * @param xmlString XML string representing dictionary
     * @return the dom object (or <code>null</code>.)
     */
    private static Document getDomObject(String xmlString) {
        // get the factory
        DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
        Document dom = null;

        try {
            // Using factory get an instance of document builder
            DocumentBuilder db = dbf.newDocumentBuilder();

            // parse using builder to get DOM representation of the XML string
            ByteArrayInputStream bais = new ByteArrayInputStream(xmlString.getBytes());
            dom = db.parse(bais);
        }
        catch (ParserConfigurationException e) {
            e.printStackTrace();
        }
        catch (SAXException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        return dom;
    }


    /**
     * Get an xml representation of the dictionary.
     * @return an xml representation of the dictionary.
     */
    public String toXML() {
        return nodeToString(topLevelDoc);
    }


    /**
     * This method converts an xml Node object into an xml string.
     * @return an xml representation of an xml Node object; null if conversion problems
     */
    private String nodeToString(Node node) {
        if (xmlRepresentation != null) return xmlRepresentation;

        StringWriter sw = new StringWriter();
        try {
            TransformerFactory transformerFactory = TransformerFactory.newInstance();
            transformerFactory.setAttribute("indent-number", 3);
            Transformer t = transformerFactory.newTransformer();
            t.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes");
            t.setOutputProperty(OutputKeys.INDENT, "yes");
            t.transform(new DOMSource(node), new StreamResult(sw));
        } catch (TransformerException te) {
            return null;
        }

        xmlRepresentation = sw.toString();
        return xmlRepresentation;
    }


    /**
     * Get a string representation of the dictionary.
     * @return a string representation of the dictionary.
     */
    @Override
    public String toString() {
        if (stringRepresentation != null) return stringRepresentation;

        EvioDictionaryEntry pair;
        StringBuilder sb = new StringBuilder(4096);
        sb.append("-- Dictionary --\n");

        for (String name : reverseMap.keySet()) {
            // Get the tag/num pair
            pair = reverseMap.get(name);
            sb.append(String.format("tag: %-15s num: %-15s name: %s\n", pair.getTag(), pair.getNum(), name));

        }

        stringRepresentation = sb.toString();
        return stringRepresentation;
    }
}
