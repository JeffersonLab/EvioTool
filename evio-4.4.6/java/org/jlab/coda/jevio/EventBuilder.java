package org.jlab.coda.jevio;

import java.io.File;
import java.io.IOException;

/**
 * This class is used for creating and manipulating events. One constructor is convenient for creating new events while
 * another is useful for manipulating existing events. You can create a new EventBuilder for each event being handled,
 * However, in many cases one can use the same EventBuilder for all events by calling the setEvent method.
 * The only reason a singleton pattern was not used was to allow for the possibility that events will be built or
 * manipulated on multiple threads.
 * @author heddle
 *
 */
public class EventBuilder {

	/**
	 * The event being built
	 */
	private EvioEvent event;
	
	
	/**
	 * This is the constructor to use for an EventBuilder object that will operate on a new, empty event.
	 * @param tag the tag for the event header (which is just a bank header).
	 * @param dataType the data type for the event object--which again is just the type for the outer most
	 * bank. Often an event is a bank of banks, so typically this will be DataType.BANK, or 0xe (14).
	 * @param num often an ordinal enumeration.
	 */
	public EventBuilder(int tag, DataType dataType, int num) {
		//create an event with the correct header data. 
		event = new EvioEvent(tag, dataType, num);
	}
	
	/**
	 * This is the constructor to use when you want to manipulate an existing event.
	 * @param event the event to manipulate.
	 */
	public EventBuilder(EvioEvent event) {
		this.event = event;
	}
	
	/**
	 * This goes through the event recursively, and makes sure all the length fields
	 * in the headers are properly set.
     * @throws EvioException if the length of containing event is too large (> Integer.MAX_VALULE),
	 */
	public void setAllHeaderLengths() {
        try {
            event.setAllHeaderLengths();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }
    }
		
	/**
	 * This clears all the data fields in a structure, but not the parent or the children. This keeps the
	 * existing tree structure intact. To remove a structure (and, consequently, all its descendants) from the
	 * tree, use <code>remove</code>
	 * @param structure the segment to clear.
	 */
	public void clearData(BaseStructure structure) {
		if (structure != null) {
			structure.rawBytes    = null;
			structure.doubleData  = null;
			structure.floatData   = null;
			structure.intData     = null;
			structure.longData    = null;
            structure.shortData   = null;
            structure.charData    = null;
            structure.stringData  = null;
            structure.stringsList = null;
            structure.stringEnd   = 0;
            structure.numberDataItems = 0;
		}
	}
	
	/**
	 * Add a child to a parent structure.
     *
	 * @param parent the parent structure.
	 * @param child the child structure.
	 * @throws EvioException if parent or child is null, child has wrong byte order,
     *                       is wrong structure type, or parent is not a container
	 */
	public void addChild(BaseStructure parent, BaseStructure child) throws EvioException {
		
		if (child == null || parent == null) {
			throw new EvioException("Null child or parent arg.");
		}

        if (child.getByteOrder() != event.getByteOrder()) {
            throw new EvioException("Attempt to add child with opposite byte order.");
        }
		
		// the child must be consistent with the data type of the parent. For example, if the child
		// is a BANK, then the data type of the parent must be BANK.
		DataType parentDataType = parent.header.getDataType();
		String errStr;
		if (parentDataType.isStructure()) {
			switch(parentDataType) {
			case BANK: case ALSOBANK:
				if (child.getStructureType() != StructureType.BANK) {
					errStr = "Type mismatch in addChild. Parent content type: " + parentDataType + 
					" child type: " + child.getStructureType();
					throw new EvioException(errStr);
				}
				break;
				
			case SEGMENT: case ALSOSEGMENT:
				if (child.getStructureType() != StructureType.SEGMENT) {
					errStr = "Type mismatch in addChild. Parent content type: " + parentDataType + 
					" child type: " + child.getStructureType();
					throw new EvioException(errStr);
				}
				break;
				
			case TAGSEGMENT:// case ALSOTAGSEGMENT:
				if (child.getStructureType() != StructureType.TAGSEGMENT) {
					errStr = "Type mismatch in addChild. Parent content type: " + parentDataType + 
					" child type: " + child.getStructureType();
					throw new EvioException(errStr);
				}
				break;

            default:
            }
		}
		else { //parent is not a container--it is expecting to hold primitives and cannot have children
			errStr = "Type mismatch in addChild. Parent content type: " + parentDataType + 
			" cannot have children.";
			throw new EvioException(errStr);
		}
		

		parent.insert(child);
        child.setParent(parent);
		setAllHeaderLengths();
	}
	
	/**
	 * This removes a structure (and all its descendants) from the tree.
	 * @param child the child structure to remove.
	 * @throws EvioException
	 */
	public void remove(BaseStructure child) throws EvioException {
		if (child == null) {
			throw new EvioException("Attempt to remove null child.");
		}
		
		BaseStructure parent = child.getParent(); 
		
		//the only orphan structure is the event itself, which cannot be removed.
		if (parent == null) {
			throw new EvioException("Attempt to remove root node, i.e., the event. Don't remove an event. Just discard it.");
		}
				
		child.removeFromParent();
        child.setParent(null);
		setAllHeaderLengths();
	}


	/**
	 * Set int data to the structure. If the structure has data, it is overwritten
     * even if the existing data is of a different type.
	 * @param structure the structure to receive the data.
	 * @param data the int data to write.
	 * @throws EvioException if structure arg is null
	 */
	public void setIntData(BaseStructure structure, int data[]) throws EvioException {
		if (structure == null) {
			throw new EvioException("Tried to set int data to a null structure.");
		}
		structure.setIntData(data);
		setAllHeaderLengths();
	}


    /**
     * Set short data to the structure. If the structure has data, it is overwritten
     * even if the existing data is of a different type.
     * @param structure the structure to receive the data.
     * @param data the short data to write.
     * @throws EvioException if structure arg is null
     */
    public void setShortData(BaseStructure structure, short data[]) throws EvioException {
        if (structure == null) {
            throw new EvioException("Tried to set short data to a null structure.");
        }
        structure.setShortData(data);
        setAllHeaderLengths();
    }


    /**
     * Set long data to the structure. If the structure has data, it is overwritten
     * even if the existing data is of a different type.
     * @param structure the structure to receive the data.
     * @param data the long data to write.
     * @throws EvioException if structure arg is null
     */
    public void setLongData(BaseStructure structure, long data[]) throws EvioException {
        if (structure == null) {
            throw new EvioException("Tried to set long data to a null structure.");
        }
        structure.setLongData(data);
        setAllHeaderLengths();
    }


    /**
     * Set byte data to the structure. If the structure has data, it is overwritten
     * even if the existing data is of a different type.
     * @param structure the structure to receive the data.
     * @param data the byte data to write.
     * @throws EvioException if structure arg is null
     */
    public void setByteData(BaseStructure structure, byte data[]) throws EvioException {
        if (structure == null) {
            throw new EvioException("Tried to set byte data to a null structure.");
        }
        structure.setByteData(data);
        setAllHeaderLengths();
    }


    /**
     * Set float data to the structure. If the structure has data, it is overwritten
     * even if the existing data is of a different type.
     * @param structure the structure to receive the data.
     * @param data the float data to write.
     * @throws EvioException if structure arg is null
     */
    public void setFloatData(BaseStructure structure, float data[]) throws EvioException {
        if (structure == null) {
            throw new EvioException("Tried to set float data to a null structure.");
        }
        structure.setFloatData(data);
        setAllHeaderLengths();
    }


    /**
     * Set double data to the structure. If the structure has data, it is overwritten
     * even if the existing data is of a different type.
     * @param structure the structure to receive the data.
     * @param data the double data to write.
     * @throws EvioException if structure arg is null
     */
    public void setDoubleData(BaseStructure structure, double data[]) throws EvioException {
        if (structure == null) {
            throw new EvioException("Tried to set double data to a null structure.");
        }
        structure.setDoubleData(data);
        setAllHeaderLengths();
    }


    /**
     * Set string data to the structure. If the structure has data, it is overwritten
     * even if the existing data is of a different type.
     * @param structure the structure to receive the data.
     * @param data the string data to write.
     * @throws EvioException if structure arg is null
     */
    public void setStringData(BaseStructure structure, String data[]) throws EvioException {
        if (structure == null) {
            throw new EvioException("Tried to set string data to a null structure.");
        }
        structure.setStringData(data);
        setAllHeaderLengths();
    }


    /**
     * Appends int data to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param structure the structure to receive the data, which is appended.
     * @param data the int data to append, or set if there is no existing data.
     * @throws EvioException
     */
    public void appendIntData(BaseStructure structure, int data[]) throws EvioException {
        if (structure == null) {
            throw new EvioException("Tried to append int data to a null structure.");
        }
        structure.appendIntData(data);
        setAllHeaderLengths();
    }


	/**
	 * Appends short data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param structure the structure to receive the data, which is appended.
	 * @param data the short data to append, or set if there is no existing data.
	 * @throws EvioException
	 */
	public void appendShortData(BaseStructure structure, short data[]) throws EvioException {
		if (structure == null) {
			throw new EvioException("Tried to append short data to a null structure.");
		}
		structure.appendShortData(data);
		setAllHeaderLengths();
	}


	/**
	 * Appends long data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param structure the structure to receive the data, which is appended.
	 * @param data the long data to append, or set if there is no existing data.
	 * @throws EvioException
	 */
	public void appendLongData(BaseStructure structure, long data[]) throws EvioException {
		if (structure == null) {
			throw new EvioException("Tried to append long data to a null structure.");
		}
		structure.appendLongData(data);
		setAllHeaderLengths();
	}


	/**
	 * Appends byte data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param structure the structure to receive the data, which is appended.
	 * @param data the byte data to append, or set if there is no existing data.
	 * @throws EvioException
	 */
	public void appendByteData(BaseStructure structure, byte data[]) throws EvioException {
		if (structure == null) {
			throw new EvioException("Tried to append byte data to a null structure.");
		}
		structure.appendByteData(data);
		setAllHeaderLengths();
	}


	/**
	 * Appends float data to the structure. If the structure has no data, then this
	 * is the same as setting the data.
	 * @param structure the structure to receive the data, which is appended.
	 * @param data the float data to append, or set if there is no existing data.
	 * @throws EvioException
	 */
	public void appendFloatData(BaseStructure structure, float data[]) throws EvioException {
		if (structure == null) {
			throw new EvioException("Tried to append float data to a null structure.");
		}
		structure.appendFloatData(data);
		setAllHeaderLengths();
	}
	

    /**
     * Appends double data to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param structure the structure to receive the data, which is appended.
     * @param data the double data to append, or set if there is no existing data.
     * @throws EvioException
     */
    public void appendDoubleData(BaseStructure structure, double data[]) throws EvioException {
        if (structure == null) {
            throw new EvioException("Tried to append double data to a null structure.");
        }
        structure.appendDoubleData(data);
        setAllHeaderLengths();
    }


    /**
     * Appends string data to the structure. If the structure has no data, then this
     * is the same as setting the data.
     * @param structure the structure to receive the data, which is appended.
     * @param data the string to append, or set if there is no existing data.
     * @throws EvioException
     */
    public void appendStringData(BaseStructure structure, String data) throws EvioException {

        if (structure == null) {
            throw new EvioException("Tried to append String to a null structure.");
        }
        structure.appendStringData(data);
        setAllHeaderLengths();
    }


	/**
	 * Get the underlying event.
	 * @return the underlying event.
	 */
	public EvioEvent getEvent() {
		return event;
	}
	
	/**
	 * Set the underlying event. As far as this event builder is concerned, the
	 * previous underlying event is lost, and all subsequent calls will affect
	 * the newly supplied event.
	 * param the new underlying event.
	 */
	public void setEvent(EvioEvent event) {
		this.event = event;
	}
	
	/**
	 * Main program for testing.
	 * @param args ignored command line arguments.
	 */
	public static void main(String args[]) {
		//create an event writer to write out the test events.
		String outfile = "C:\\Documents and Settings\\heddle\\My Documents\\test.ev";
		EventWriter eventWriter = null;
		try {
			eventWriter = new EventWriter(new File(outfile));
		}
		catch (EvioException e) {
			e.printStackTrace();
			System.exit(1);
		}
		
		//count the events we make for testing
		int eventNumber = 1;
		
		//use a tag of 11 for events--for no particular reason
		int tag = 11;
		
		try {
			//first event-- a trivial event containing an array of ints. 
			EventBuilder eventBuilder = new EventBuilder(tag, DataType.INT32, eventNumber++);
			EvioEvent event1 = eventBuilder.getEvent();
			//should end up with int array 1..25,1..10
			eventBuilder.appendIntData(event1, fakeIntArray(25));
			eventBuilder.appendIntData(event1, fakeIntArray(10));
			eventWriter.writeEvent(event1);
			
			//second event, more traditional bank of banks
			eventBuilder = new EventBuilder(tag, DataType.BANK, eventNumber++);
			EvioEvent event2 = eventBuilder.getEvent();
			
			//add a bank of doubles
			EvioBank bank1 = new EvioBank(22, DataType.DOUBLE64, 0);
			eventBuilder.appendDoubleData(bank1, fakeDoubleArray(10));
			eventBuilder.addChild(event2, bank1);
			eventWriter.writeEvent(event2);
			
			//lets modify event2
			event2.getHeader().setNumber(eventNumber++);
			EvioBank bank2 = new EvioBank(33, DataType.BANK, 0);
			eventBuilder.addChild(event2, bank2);

			EvioBank subBank1 = new EvioBank(34, DataType.SHORT16, 1);
			eventBuilder.addChild(bank2, subBank1);
			eventBuilder.appendShortData(subBank1, fakeShortArray(5));
			
						
			
			//now add a bank of segments
			EvioBank subBank2 = new EvioBank(33, DataType.SEGMENT, 0);
			eventBuilder.addChild(bank2, subBank2);
			
			EvioSegment segment1 = new EvioSegment(34, DataType.SHORT16);
			eventBuilder.addChild(subBank2, segment1);
			eventBuilder.appendShortData(segment1, fakeShortArray(7));
			
			EvioSegment segment2 = new EvioSegment(34, DataType.SHORT16);
			eventBuilder.addChild(subBank2, segment2);
			eventBuilder.appendShortData(segment2, fakeShortArray(10));
			
			
			//now add a bank of tag segments
			EvioBank subBank3 = new EvioBank(45, DataType.TAGSEGMENT, 0);
			eventBuilder.addChild(bank2, subBank3);

			EvioTagSegment tagsegment1 = new EvioTagSegment(34, DataType.INT32);
			eventBuilder.addChild(subBank3, tagsegment1);
			eventBuilder.appendIntData(tagsegment1, fakeIntArray(3));
			
			EvioTagSegment tagsegment2 = new EvioTagSegment(34, DataType.CHARSTAR8);
			eventBuilder.addChild(subBank3, tagsegment2);
			eventBuilder.appendStringData(tagsegment2, "This is a string");

			
//			System.err.println("EVENT2: " + event2.getHeader());
//			System.err.println("BANK1: " + bank1.getHeader());
//			System.err.println("BANK2: " + bank2.getHeader());
//			System.err.println("SUBBANK1: " + subBank1.getHeader());
//			System.err.println("SUBBANK2: " + subBank2.getHeader());
//			System.err.println("segment1: " + segment1.getHeader());
//			System.err.println("segment2: " + segment2.getHeader());
//			System.err.println("SUBBANK3: " + subBank3.getHeader());
//			System.err.println("tagseg1: " + tagsegment1.getHeader());
			
			
			//write the event
			eventWriter.writeEvent(event2);		
			
            //all done
            eventWriter.close();

		}
        catch (EvioException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

		System.out.println("Test completed.");
	}
	
	/**
	 * Array of ints, sequential, 1..size, for test purposes.
	 * @param size the size of the array.
	 * @return the fake int array.
	 */
	private static int[] fakeIntArray(int size) {
		int[] array = new int[size];
		for (int i = 0; i < size; i++) {
			array[i] = i+1;
		}
		return array;
	}
	
	/**
	 * Array of shorts, sequential, 1..size, for test purposes.
	 * @param size the size of the array.
	 * @return the fake short array.
	 */
	private static short[] fakeShortArray(int size) {
		short[] array = new short[size];
		for (int i = 0; i < size; i++) {
			array[i] = (short)(i+1);
		}
		return array;
	}
	
	/**
	 * Array of characters for test purposes.
	 * @return the fake char array.
	 */
	private static char[] fakeCharArray() {
		char array[] = {'T','h','i','s',' ','i','s',' ','c','h','a','r',' ', 'd','a','t','a','.'};
		return array;
	}


	
	/**
	 * Array of doubles, sequential, 1..size, for test purposes.
	 * @param size the size of the array.
	 * @return the fake double array.
	 */
	private static double[] fakeDoubleArray(int size) {
		double[] array = new double[size];
		for (int i = 0; i < size; i++) {
			array[i] = i+1;
		}
		return array;
	}

}
