package org.jlab.coda.jevio;

import org.jlab.coda.jevio.EvioReader.ReadStatus;

import java.io.IOException;


/**
 * A set of static functions that test evio files. It also has some other diagnostic methods for getting counts.
 * 
 * @author heddle
 * 
 */
public class EvioFileTest {

	/**
	 * This enum is used for file testing. <br>
	 * PASS indicates that a given test passed. <br>
	 * FAIL indicates that a given test failed.
	 */
	public static enum TestResult {
		PASS, FAIL
	}

	/**
	 * Get a total count of the number of physical records.
	 * 
	 * @param reader reader of the file to be processed.
	 * @return the total count of blocks (physical records.)
	 */
	public static int totalBlockCount(EvioReader reader) {
		ReadStatus status = EvioReader.ReadStatus.SUCCESS;
		int count = 0;

        try {
            reader.rewind();
            while (status == ReadStatus.SUCCESS) {
                status = reader.nextBlockHeader();
                if (status == ReadStatus.SUCCESS) {
                    reader.position((int)reader.getCurrentBlockHeader().nextBufferStartingPosition());
                    count++;
                }
            }
            reader.rewind();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        System.out.println("total block count: " + count);
        return count;
	}


	/**
	 * Tests whether we can look through the file and find all the block headers.
	 * 
     * @param reader reader of the file to be processed.
	 * @return the result of this test, either PASS or FAIL.
	 */
	public static TestResult readAllBlockHeadersTest(EvioReader reader) {
        int blockCount = 0;
        TestResult result;
		ReadStatus status = ReadStatus.SUCCESS;

        try {
            reader.rewind();

            while (status == ReadStatus.SUCCESS) {
                status = reader.nextBlockHeader();
                if (status == ReadStatus.SUCCESS) {
                    reader.position((int)reader.getCurrentBlockHeader().nextBufferStartingPosition());
                    blockCount++;
                }
            }

            // should have read to the end of the file
            if (status == ReadStatus.END_OF_FILE) {
                System.out.println("Total blocks read: " + blockCount);
                result = TestResult.PASS;
            }
            else {
                result = TestResult.FAIL;
            }

            reader.rewind();
        }
        catch (Exception e) {
            e.printStackTrace();
            result = TestResult.FAIL;
        }

        System.out.println("readAllBlockHeadersTest: " + result);
        return result;
    }


	/**
	 * Tests whether we can look through the file read all the events.
	 * 
     * @param reader reader of the file to be processed.
	 * @return the result of this test, either <code>TestResult.PASS</code> or <code>TestResult.FAIL</code>.
	 */
	public static TestResult readAllEventsTest(EvioReader reader) {
        int count = 0;
		EvioEvent event;
		TestResult result;

		try {
		    // store current file position
		    long oldPosition = reader.position();

		    reader.rewind();
			while ((event = reader.nextEvent()) != null) {
				count++;
				BaseStructureHeader header = event.getHeader();

				System.out.println(count + ")  size: " + header.getLength() + " type: " + header.getDataTypeName()
						+ " \"" + event.getDescription() + "\"");
			}

            // restore file position
            reader.position(oldPosition);
            result = TestResult.PASS;
		}
		catch (Exception e) {
			e.printStackTrace();
            result = TestResult.FAIL;
		}

        System.out.println("readAllBlockHeadersTest: " + result);
		return result;
	}


	/**
	 * Tests whether we can parse events from the file.
	 * 
     * @param reader reader of the file to be processed.
	 * @param num the number to parse. Will try to parse this many (unless it runs out.) Use -1 to parse all events.
	 *            Note: if <code>num</code> is greater than the number of events in the file, it doesn't constitute an
	 *            error.
	 * @return the result of this test, either <code>TestResult.PASS</code> or <code>TestResult.FAIL</code>.
	 */
	public static TestResult parseEventsTest(EvioReader reader, int num) {
		// store current file position

		if (num < 0) {
			num = Integer.MAX_VALUE;
		}

        EvioEvent event;
        int count = 0;
        TestResult result = TestResult.PASS;

        try {
            long oldPosition = reader.position();
            reader.rewind();

            try {
                while ((count < num) && ((event = reader.nextEvent()) != null)) {
                    reader.parseEvent(event);
                    count++;
                }
            }
            catch (EvioException e) {
                e.printStackTrace();
                result = TestResult.FAIL;
            }

            // restore file position
            reader.position(oldPosition);
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        System.out.println("parseEventsTest parsed: " + count + " events");
        System.out.println("parseEventsTest result: " + result);

        return result;
	}

}
