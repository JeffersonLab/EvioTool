package org.jlab.coda.jevio;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.BitSet;

/**
 * Used for demoing the JEvio library.
 * 
 * @author heddle
 * 
 */
public class Demo implements IEvioListener {

    public static void main(String args[]) {
        BitSet bitInfo = new BitSet(24);
        bitInfo.set(3);
        bitInfo.set(5);

        int type=0;
        boolean bitSet;
        for (int i=0; i < 4; i++) {
            bitSet = bitInfo.get(i+2);
            if (bitSet) type |= 1 << i;
        }
        System.out.println("Resulting int = 0x" + Integer.toHexString(type));
    }


	/**
	 * Main program for testing
	 * 
	 * @param args the command line arguments.
	 */
	public static void main2(String args[]) {
		System.out.println("Test of EvioReader class.");

		String cwd = Environment.getInstance().getCurrentWorkingDirectory();

		// assumes we have a directory "testdata" with the test files in the cwd.
//		String testEventFile = cwd + "\\testdata\\dennis.ev";
//		String testEventFile = cwd + "\\testdata\\out.ev";
//		String testDictFile = cwd + "\\testdata\\eviodict.xml";
        String testEventFile = cwd + File.separator + "testdata" + File.separator + "out.ev";
        String testDictFile  = cwd + File.separator + "testdata" + File.separator + "eviodict.xml";


		try {
			File file = new File(testEventFile);
			System.out.println("ev file: " + testEventFile + " size: " + file.length());

			EvioReader reader = new EvioReader(testEventFile);
			
			// uncomment below to test a filter
//			IEvioFilter myFilter = new IEvioFilter() {
//				@Override
//				public boolean accept(StructureType structureType, BaseStructureHeader structureHeader) {
//					DataType dataType = structureHeader.getDataTypeEnum();
//					
//					return (dataType == DataType.DOUBLE64);
//				}
//			};
//			EventParser.getInstance().setEvioFilter(myFilter);

			
			//add myself as a listener
			reader.getParser().addEvioListener(new Demo());

			// create a dictionary, set the global name provider
			NameProvider.setProvider(NameProviderFactory.createNameProvider(new File(testDictFile)));

			// try a block test
			EvioFileTest.readAllBlockHeadersTest(reader);

			// try read all events test
			EvioFileTest.readAllEventsTest(reader);

			// just count the blocks
			EvioFileTest.totalBlockCount(reader);

			// try processing some events
			EvioFileTest.parseEventsTest(reader, 1);
		}
		catch (FileNotFoundException e) {
			e.printStackTrace();
			System.exit(1);
		}
		catch (Exception e) {
			e.printStackTrace();
			System.exit(1);
		}

	}

    public void startEventParse(BaseStructure evioEvent) { }

    public void endEventParse(BaseStructure evioEvent) { }

	/**
	 * This IEvioListener has received a structure as the result of an event being parsed.
	 * @param evioEvent the event being parsed.
	 * @param structure the structure that I received. It is a BANK, SEGEMENT
	 * or TAGSEGMENT.
	 */
	@Override
	public void gotStructure(BaseStructure evioEvent, IEvioStructure structure) {

		BaseStructureHeader header = structure.getHeader();

		System.out.println("------------------");
		System.out.println("" + structure);
		
		switch (header.getDataType()) {
		case DOUBLE64:
			System.out.println("        DOUBLE VALS");
			double doubledata[] = structure.getDoubleData();
			for (double d : doubledata) {
				System.out.println("         " + d);
			}
			break;
			
		case INT32: case UINT32:
			System.out.println("        INT VALS");
			int intdata[] = structure.getIntData();
			for (int i : intdata) {
				System.out.println("         " + i);
			}
			break;
			
		case CHAR8: case UCHAR8:
			System.out.println("        BYTE VALS");
			byte bytedata[] = structure.getByteData();
			for (byte i : bytedata) {
				System.out.println("         " + i);
			}
			break;

        default:
        }

	}

}
