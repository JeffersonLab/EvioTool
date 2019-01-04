package org.jlab.coda.jevio.test;

import org.jlab.coda.jevio.*;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.Arrays;
import java.util.List;

/**
 * Test program.
 * @author timmer
 * Date: Oct 7, 2010
 */
public class FileTest {



    /** For testing the speed difference in writeEvent algorithms.
     *  Want about 1000 little events/block.  */
    public static void main11(String args[]) {

        // Create an event writer to write out the test events.
        String fileName  = "/daqfs/home/timmer/coda/jevio-4.3/testdata/speedTest.ev";
        File file = new File(fileName);

        // data
        byte[] byteData1 = new byte[499990];

        int num, count = 0;
        long t1=0, t2=0, time, totalT=0, totalCount=0;
        double rate, avgRate;

        try {
            // 1MB max block size, 2 max # events/block
            EventWriter eventWriter = new EventWriter(file, 1000000, 2,
                                                      ByteOrder.BIG_ENDIAN, null, null);

            // event -> bank of bytes
            // each event (including header) is 100 bytes
            EventBuilder eventBuilder = new EventBuilder(1, DataType.CHAR8, 1);
            EvioEvent ev = eventBuilder.getEvent();
            ev.appendByteData(byteData1);

            // keep track of time
            t1 = System.currentTimeMillis();


            for (int j=0; j < 10; j++) {
// 10 MB file with 10 block headers
                for (int i=0; i < 2; i++) {
                    eventWriter.writeEvent(ev);
                    count++;
                }
            }

            // all done writing
            eventWriter.close();

            // calculate the event rate
            t2 = System.currentTimeMillis();
            time = t2 - t1;
            rate = 1000.0 * ((double) count) / time;
            totalCount += count;
            totalT += time;
            avgRate = 1000.0 * ((double) totalCount) / totalT;
            System.out.println("rate = " + String.format("%.3g", rate) +
                                       " Hz,  avg = " + String.format("%.3g", avgRate));
            System.out.println("time = " + (time) + " milliseconds");
            count = 0;
            t1 = System.currentTimeMillis();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }


    }



    /** For WRITING a local file. */
    public static void main(String args[]) {

        String fileName  = "/daqfs/home/timmer/coda/jevio-4.3/testdata/fileTestSmall.ev";
        File file = new File(fileName);

        String xmlDictionary = null;

        // data
        int[]   intData  = new int[]   {0,1,2,3,4,5,6,7,8,9,10,11,12,13};

        // Do we overwrite or append?
        boolean append = false;

        // Do we write to file or buffer?
        boolean useFile = true;

        // Top level event
        EvioEvent event = null;

        try {
            // Create an event writer to write out the test events to file
            EventWriter writer = new EventWriter(file, xmlDictionary, append);


            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(1, DataType.BANK, 1);
            event = builder.getEvent();

            // bank of int
            EvioBank bankInts = new EvioBank(2, DataType.INT32, 2);
            bankInts.appendIntData(intData);
            builder.addChild(event, bankInts);


            for (int i=0; i < 1; i++) {
                // Write event to file
                writer.writeEvent(event);
            }

            // All done writing
            writer.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }

    }



    /** For WRITING a local file. */
    public static void mainww(String args[]) {

        // String fileName  = "./myData.ev";
        String fileName  = "/daqfs/home/timmer/coda/jevio-4.3/testdata/fileTestSmall.ev";
        File file = new File(fileName);
        ByteBuffer myBuf = null;

        // xml dictionary
        String xmlDictionary =
            "<xmlDict>\n" +
            "  <bank name=\"My Event\"       tag=\"1\"   num=\"1\">\n" +
            "     <bank name=\"Segments\"    tag=\"2\"   num=\"2\">\n" +
            "       <leaf name=\"My Shorts\" tag=\"3\"   />\n" +
            "     </bank>\n" +
            "     <bank name=\"Banks\"       tag=\"1\"   num=\"1\">\n" +
            "       <leaf name=\"My chars\"  tag=\"5\"   num=\"5\"/>\n" +
            "     </bank>\n" +
            "  </bank>\n" +
            "  <dictEntry name=\"Last Bank\" tag=\"33\"  num=\"66\"/>\n" +
            "  <dictEntry name=\"Test Bank\" tag=\"1\" />\n" +
            "</xmlDict>";
        xmlDictionary = null;
        // data
        byte[]  byteData1 = new byte[]  {1,2,3,4,5};
        int[]   intData1  = new int[]   {4,5,6};
        int[]   intData2  = new int[]   {7,8,9};
        short[] shortData = new short[] {11,22,33};
        double[] doubleData = new double[] {1.1,2.2,3.3};

        // Do we overwrite or append?
        boolean append = false;

        // Do we write to file or buffer?
        boolean useFile = true;

        // Top level event
        EvioEvent event = null;

        try {
            // Create an event writer to write out the test events to file
            EventWriter writer;

            if (useFile) {
                writer = new EventWriter(file, xmlDictionary, append);
            }
            else {
                // Create an event writer to write to buffer
                myBuf = ByteBuffer.allocate(10000);
                myBuf.order(ByteOrder.LITTLE_ENDIAN);
                writer = new EventWriter(myBuf, xmlDictionary, append);
            }

//            // Build event (bank of banks) without an EventBuilder
//            event = new EvioEvent(1, DataType.BANK, 1);
//
//            // bank of segments
//            EvioBank bankSegs = new EvioBank(2, DataType.SEGMENT, 2);
//            event.insert(bankSegs);
//
//            // segment of 3 shorts
//            EvioSegment segShorts = new EvioSegment(3, DataType.SHORT16);
//            segShorts.setShortData(shortData);
//            bankSegs.insert(segShorts);
//            bankSegs.remove(segShorts);
//
//            // another bank of banks
//            EvioBank bankBanks = new EvioBank(4, DataType.BANK, 4);
//            event.insert(bankBanks);
//
//            // bank of chars
//            EvioBank charBank = new EvioBank(5, DataType.CHAR8, 5);
//            charBank.setByteData(byteData1);
//            bankBanks.insert(charBank);
//
//            event.setAllHeaderLengths();

            // Build event (bank of banks) with EventBuilder object
            EventBuilder builder = new EventBuilder(1, DataType.BANK, 1);
            event = builder.getEvent();

            // bank of doubles
            EvioBank bankDoubles = new EvioBank(1, DataType.DOUBLE64, 2);
            bankDoubles.appendDoubleData(doubleData);
            builder.addChild(event, bankDoubles);

            // bank of segments
            EvioBank bankSegs = new EvioBank(2, DataType.SEGMENT, 2);
            builder.addChild(event, bankSegs);

            // segment of 3 shorts
            EvioSegment segShorts = new EvioSegment(3, DataType.SHORT16);
            segShorts.appendShortData(shortData);
            builder.addChild(bankSegs, segShorts);
            //builder.remove(segShorts);

            // another bank of banks
            EvioBank bankBanks = new EvioBank(4, DataType.BANK, 4);
            builder.addChild(event, bankBanks);

            // bank of chars
            EvioBank charBank = new EvioBank(5, DataType.CHAR8, 5);
            charBank.appendByteData(byteData1);
            builder.addChild(bankBanks, charBank);

            // event - ban
            // of banks
            EvioEvent lastEvent = new EvioEvent(33, DataType.INT32, 66);
            // Call this BEFORE appending data!
            lastEvent.setByteOrder(ByteOrder.LITTLE_ENDIAN);
            lastEvent.appendIntData(intData1);
            lastEvent.setIntData(intData2);
            lastEvent.appendIntData(intData2);

            for (int i=0; i < 1; i++) {
                // Write event to file
                writer.writeEvent(event);

                // Write last event to file
                //writer.writeEvent(lastEvent);

                System.out.println("Block # = " + writer.getBlockNumber());

                // How much room do I have left in the buffer now?
                if (!useFile) {
                    System.out.println("Buffer has " + myBuf.remaining() + " bytes left");
                }
            }

            // All done writing
            writer.close();


            // Transform segments into banks in 2 different ways
//            EvioBank segBank1 = StructureTransformer.transform(segShorts, 10);
//            StructureTransformer T = new StructureTransformer();
//            EvioBank segBank2 = T.transform(segShorts, 10);

            if (xmlDictionary != null) {
                EvioXMLDictionary dict = new EvioXMLDictionary(xmlDictionary);
                NameProvider.setProvider(dict);
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }

        boolean readingFile = false;

        if (readingFile) {
            try {
                EvioReader evioReader;
                if (useFile) {
                    System.out.println("read ev file: " + fileName + ", size: " + file.length());
                    evioReader = new EvioReader(fileName);
                }
                else {
                    myBuf.flip();
                    evioReader = new EvioReader(myBuf);
                }
                EventParser parser = evioReader.getParser();

                IEvioListener listener = new IEvioListener() {
                    public void gotStructure(BaseStructure topStructure, IEvioStructure structure) {
                        System.out.println("Parsed structure of type " + structure.getStructureType());
                    }

                    public void startEventParse(BaseStructure structure) {
                        System.out.println("Starting event parse");
                    }

                    public void endEventParse(BaseStructure structure) {
                        System.out.println("Ended event parse");
                    }
                };

                parser.addEvioListener(listener);

                // Get any existing dictionary (should be the same as "xmlDictionary")
                String xmlDictString = evioReader.getDictionaryXML();
                EvioXMLDictionary dictionary = null;

                if (xmlDictString == null) {
                    System.out.println("Ain't got no dictionary!");
                }
                else {
                    // Create dictionary object from xml string
                    dictionary = new EvioXMLDictionary(xmlDictString);
                    System.out.println("Got a dictionary:\n" + dictionary.toString());
                }

                // How many events in the file?
                int evCount = evioReader.getEventCount();
                System.out.println("Read file, got " + evCount + " events:\n");

                // Use the "random access" capability to look at last event (starts at 1)
                EvioEvent ev = evioReader.parseEvent(evCount);
                System.out.println("Last event = " + ev.toString());

                // Print out any data in the last event.
                //
                // In the writing example, the data for this event was set to
                // be little endian so we need to read it in that way too
                ev.setByteOrder(ByteOrder.LITTLE_ENDIAN);
                int[] intData = ev.getIntData();
                if (intData != null) {
                    for (int i=0; i < intData.length; i++) {
                        System.out.println("intData[" + i + "] = " + intData[i]);
                    }
                }

                // Use the dictionary
                if (dictionary != null) {
                    String eventName = dictionary.getName(ev);
                    System.out.println("Name of last event = " + eventName);
                }

                // Use sequential access to events
                while ( (ev = evioReader.parseNextEvent()) != null) {
                    System.out.println("Event = " + ev.toString());
                }

                // Go back to the beginning of file/buffer
                evioReader.rewind();

                // Search for banks/segs/tagsegs with a particular tag & num pair of values
                int tag=1, num=1;
                List<BaseStructure> list = StructureFinder.getMatchingBanks(
                                                (ev = evioReader.parseNextEvent()), tag, num);
                System.out.println("Event = " + ev.toString());
                for (BaseStructure s : list) {
                    if (dictionary == null)
                        System.out.println("Evio structure named \"" + s.toString() + "\" has tag=1 & num=1");
                    else
                        System.out.println("Evio structure named \"" + dictionary.getName(s) + "\" has tag=1 & num=1");
                }

                // ------------------------------------------------------------------
                // Search for banks/segs/tagsegs with a custom set of search criteria
                // ------------------------------------------------------------------

                // This filter selects Segment structures that have odd numbered tags.
                class myEvioFilter implements IEvioFilter {
                    public boolean accept(StructureType structureType, IEvioStructure struct) {
                        return (structureType == StructureType.SEGMENT &&
                                (struct.getHeader().getTag() % 2 == 1));
                    }
                };

                myEvioFilter filter = new myEvioFilter();
                list = StructureFinder.getMatchingStructures(event, filter);
                if (list != null) {
                    System.out.println("list size = " + list.size());
                    for (BaseStructure s : list) {
                        if (dictionary == null)
                            System.out.println("Evio structure named \"" + s.toString() + "\" has tag=1 & num=1");
                        else
                            System.out.println("Evio structure named " + dictionary.getName(s) +
                                           " is a segment with an odd numbered tag");
                    }
                }

                // ------------------------------------------------------------------
            }
            catch (EvioException e) {
                e.printStackTrace();
            }
            catch (IOException e) {
                e.printStackTrace();
            }
        }
    }


    /** For testing only */
    public static void main9(String args[]) {

        //create an event writer to write out the test events.
        String fileName  = "/daqfs/home/timmer/coda/jevio-4.3/testdata/BigOut2.ev";
        File file = new File(fileName);

        ByteBuffer myBuf = ByteBuffer.allocate(800);

        // xml dictionary
        String dictionary =
                "<xmlDict>\n" +
                        "  <xmldumpDictEntry name=\"bank\"           tag=\"1\"   num=\"1\"/>\n" +
                        "  <xmldumpDictEntry name=\"bank of shorts\" tag=\"2\"   num=\"2\"/>\n" +
                        "  <xmldumpDictEntry name=\"shorts pad0\"    tag=\"3\"   num=\"3\"/>\n" +
                        "  <xmldumpDictEntry name=\"shorts pad2\"    tag=\"4\"   num=\"4\"/>\n" +
                        "  <xmldumpDictEntry name=\"bank of chars\"  tag=\"5\"   num=\"5\"/>\n" +
                "</xmlDict>";

        // data
        short[] shortData1  = new short[] {11,22};
        short[] shortData2  = new short[] {11,22,33};

        EvioEvent event = null;

        try {
            //EventWriter eventWriterNew = new EventWriter(myBuf, 100, 3, dictionary, null);
            EventWriter eventWriterNew = new EventWriter(file, 1000, 3, ByteOrder.BIG_ENDIAN, dictionary, null);

            // event - bank of banks
            EventBuilder eventBuilder2 = new EventBuilder(1, DataType.BANK, 1);
            event = eventBuilder2.getEvent();

            // bank of short banks
            EvioBank bankBanks = new EvioBank(2, DataType.BANK, 2);

            // 3 shorts
            EvioBank shortBank1 = new EvioBank(3, DataType.SHORT16, 3);
            shortBank1.appendShortData(shortData1);
            eventBuilder2.addChild(bankBanks, shortBank1);

            EvioBank shortBank2 = new EvioBank(3, DataType.SHORT16, 3);
            shortBank2.appendShortData(shortData2);
            eventBuilder2.addChild(bankBanks, shortBank2);

            eventBuilder2.addChild(event, bankBanks);
            eventWriterNew.writeEvent(event);

            // all done writing
            eventWriterNew.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (EvioException e) {
            e.printStackTrace();
        }


//        // parse bytes
//        class myListener implements IEvioListener {
//
//            public void startEventParse(EvioEvent evioEvent) { }
//
//            public void endEventParse(EvioEvent evioEvent) { }
//
//            public void gotStructure(EvioEvent evioEvent, IEvioStructure structure) {
//
//                BaseStructureHeader header = structure.getHeader();
//
//                System.out.println("------------------");
//                System.out.println("" + structure);
//
//                switch (header.getDataTypeEnum()) {
//                    case FLOAT32:
//                        System.out.println("        FLOAT VALS");
//                        float floatdata[] = structure.getFloatData();
//                        for (float f : floatdata) {
//                            System.out.println("         " + f);
//                        }
//                        break;
//
//                    case DOUBLE64:
//                        System.out.println("        DOUBLE VALS");
//                        double doubledata[] = structure.getDoubleData();
//                        for (double d : doubledata) {
//                            System.out.println("         " + d);
//                        }
//                        break;
//
//                    case SHORT16:
//                        System.out.println("        SHORT VALS");
//                        short shortdata[] = structure.getShortData();
//                        for (short i : shortdata) {
//                            System.out.println("        0x" + Integer.toHexString(i));
//                        }
//                        break;
//
//                    case INT32:
//                    case UINT32:
//                        System.out.println("        INT VALS");
//                        int intdata[] = structure.getIntData();
//                        for (int i : intdata) {
//                            System.out.println("        0x" + Integer.toHexString(i));
//                        }
//                        break;
//
//                    case LONG64:
//                        System.out.println("        LONG VALS");
//                        long longdata[] = structure.getLongData();
//                        for (long i : longdata) {
//                            System.out.println("        0x" + Long.toHexString(i));
//                        }
//                        break;
//
//                    case CHAR8:
//                    case UCHAR8:
//                        System.out.println("        BYTE VALS");
//                        byte bytedata[] = structure.getByteData();
//                        for (byte i : bytedata) {
//                            System.out.println("         " + i);
//                        }
//                        break;
//
//                    case CHARSTAR8:
//                        System.out.println("        STRING VALS");
//                        String stringdata[] = structure.getStringData();
//                        for (String str : stringdata) {
//                            System.out.println("         " + str);
//                        }
//                        break;
//                }
//            }
//        }


         // print event
        System.out.println("Event = \n" + event.toXML());

         //now read it back in

         File fileIn = new File(fileName);
         System.out.println("read ev file: " + fileName + " size: " + fileIn.length());

         try {
             EvioReader fileReader = new EvioReader(fileName);
             System.out.println("\nnum ev = " + fileReader.getEventCount());
             System.out.println("dictionary = " + fileReader.getDictionaryXML() + "\n");



             event = fileReader.parseNextEvent();
             if (event == null) {
                 System.out.println("We got a NULL event !!!");
                 return;
             }
             System.out.println("Event = \n" + event.toXML());
             while ( (event = fileReader.parseNextEvent()) != null) {
                System.out.println("Event = " + event.toString());
             }
         }
         catch (IOException e) {
             e.printStackTrace();
         }
         catch (EvioException e) {
             e.printStackTrace();
         }
     }


    /**
       3 block headers (first 2 have 2 extra words each, last has 1 extra word).
       First block has 2 events. Second has 3 events.
       Last is empty final block.
    */
    static int data1[] = {
        0x00000014,
        0x00000001,
        0x0000000A,
        0x00000002,
        0x00000000,
        0x00000004,
        0x00000000,
        0xc0da0100,
        0x00000003,
        0x00000002,

        0x00000004,
        0x00010101,
        0x00000001,
        0x00000001,
        0x00000001,

        0x00000004,
        0x00010101,
        0x00000002,
        0x00000002,
        0x00000002,

        0x00000019,
        0x00000002,
        0x0000000A,
        0x00000003,
        0x00000000,
        0x00000004,
        0x00000000,
        0xc0da0100,
        0x00000001,
        0x00000002,

        0x00000004,
        0x00010101,
        0x00000003,
        0x00000003,
        0x00000003,

        0x00000004,
        0x00010101,
        0x00000004,
        0x00000004,
        0x00000004,

        0x00000004,
        0x00010101,
        0x00000005,
        0x00000005,
        0x00000005,

        0x00000009,
        0x00000003,
        0x00000009,
        0x00000000,
        0x00000000,
        0x00000204,
        0x00000000,
        0xc0da0100,
        0x00000003,
    };


    /** For writing out a 5GByte file. */
    public static void main22(String args[]) {
        // xml dictionary
        String dictionary =
                "<xmlDict>\n" +
                        "\t<xmldumpDictEntry name=\"bank\"           tag=\"1\"   num=\"1\"/>\n" +
                        "\t<xmldumpDictEntry name=\"bank of shorts\" tag=\"2\"   num=\"2\"/>\n" +
                        "\t<xmldumpDictEntry name=\"shorts pad0\"    tag=\"3\"   num=\"3\"/>\n" +
                        "\t<xmldumpDictEntry name=\"shorts pad2\"    tag=\"4\"   num=\"4\"/>\n" +
                        "\t<xmldumpDictEntry name=\"bank of chars\"  tag=\"5\"   num=\"5\"/>\n" +
                "</xmlDict>";


        // write evio file that has extra words in headers
        if (false) {
            try {
                byte[] be  = ByteDataTransformer.toBytes(data1, ByteOrder.BIG_ENDIAN);
                ByteBuffer buf = ByteBuffer.wrap(be);
                String fileName  = "/local/scratch/HeaderFile.ev";
                File file = new File(fileName);
                FileOutputStream fileOutputStream = new FileOutputStream(file);
                FileChannel fileChannel = fileOutputStream.getChannel();
                fileChannel.write(buf);
                fileChannel.close();
            }
            catch (Exception e) {
                e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
            }

        }

        //create an event writer to write out the test events.
        String fileName  = "/local/scratch/gagik.evio";
        //String fileName  = "/local/scratch/BigFile3GBig.ev";
        //String fileName  = "/local/scratch/BigFile1GLittle.ev";
        //String fileName  = "/local/scratch/LittleFile.ev";
        //String fileName  = "/local/scratch/HeaderFile.ev";
        File file = new File(fileName);
        EvioEvent event;

        //large file with big events (10MB)
        if (false) {
            // data
            int[] intData  = new int[2500000];
            Arrays.fill(intData, 23);


            try {
                EventWriter eventWriterNew = new EventWriter(file, 1000, 3, ByteOrder.BIG_ENDIAN, null, null);

                // event - bank of banks
                EventBuilder eventBuilder2 = new EventBuilder(1, DataType.INT32, 1);
                event = eventBuilder2.getEvent();
                event.appendIntData(intData);

                // 300 ev/file * 10MB/ev = 3GB/file
                for (int i=0; i < 300; i++) {
                    eventWriterNew.writeEvent(event);
                    System.out.print(".");
                }
                System.out.println("\nDONE");

                // all done writing
                eventWriterNew.close();
            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }
        }

        // large file with small events (1kB)
        if (false) {
            // data
            int[] intData  = new int[250];
            Arrays.fill(intData, 25);


            try {
                EventWriter eventWriterNew = new EventWriter(file, 10000000, 100000, ByteOrder.BIG_ENDIAN, null, null);

                // event - bank of banks
                EventBuilder eventBuilder2 = new EventBuilder(1, DataType.INT32, 1);
                event = eventBuilder2.getEvent();
                event.appendIntData(intData);

                // 3000000 ev/file * 1kB/ev = 3GB/file
                for (int i=0; i < 3000000; i++) {
                    eventWriterNew.writeEvent(event);
                    if (i%100000== 0) System.out.print(i+" ");
                }
                System.out.println("\nDONE");

                // all done writing
                eventWriterNew.close();
            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }
        }

        // 1G file with small events (1kB)
        if (false) {
            // data
            int[] intData  = new int[63];
            Arrays.fill(intData, 25);


            try {
                EventWriter eventWriterNew = new EventWriter(file, 10000000, 6535, ByteOrder.BIG_ENDIAN, dictionary, null);

                // event - bank of banks
                EventBuilder eventBuilder2 = new EventBuilder(1, DataType.INT32, 1);
                event = eventBuilder2.getEvent();
                event.appendIntData(intData);

                // 4000000 ev/file * 260/ev = 1GB/file
                for (int i=0; i < 4000000; i++) {
                    eventWriterNew.writeEvent(event);
                    if (i%200000== 0) System.out.print(i+" ");
                }
                System.out.println("\nDONE");

                // all done writing
                eventWriterNew.close();
            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }
        }

        // small file
        if (false) {
            // data
            int[] intData  = new int[1];


            try {
                EventWriter eventWriterNew = new EventWriter(file, 1000, 3, ByteOrder.BIG_ENDIAN,
                                                             dictionary, null);

                // event - bank of banks
                EventBuilder eventBuilder2 = new EventBuilder(1, DataType.INT32, 1);
                event = eventBuilder2.getEvent();
                event.appendIntData(intData);

                // 300 ev/file * 4bytes/ev = 1.2kB/file
                for (int i=0; i < 300; i++) {
                    eventWriterNew.writeEvent(event);
                    System.out.print(".");
                }
                System.out.println("\nDONE");

                // all done writing
                eventWriterNew.close();
            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }
        }

        if (true) {
            System.out.println("\nTRY READING");
            int evCount;
            File fileIn = new File(fileName);
            System.out.println("read ev file: " + fileName + " size: " + fileIn.length());
            try {
                EvioReader fileReader = new EvioReader(fileName, false, true);
                evCount = fileReader.getEventCount();
                System.out.println("\nnum ev = " + evCount);
                System.out.println("blocks = " + fileReader.getBlockCount());
                System.out.println("dictionary = " + fileReader.getDictionaryXML() + "\n");


                int counter = 0;

                long t2, t1 = System.currentTimeMillis();

                while ( (event = fileReader.parseNextEvent()) != null) {
                    if (event == null) {
                        System.out.println("We got a NULL event !!!");
                        return;
                    }

//                    if (counter++ %10 ==0) {
//                        System.out.println("Event #" + counter + " =\n" + event);
//                        int[] d = event.getIntData();
//                        System.out.println("Data[0] = " + d[0] + ", Data[last] = " + d[d.length-1]);
//                        //            System.out.println("Event = \n" + event.toXML());
//                        //            while ( (event = fileReader.parseNextEvent()) != null) {
//                        //               System.out.println("Event = " + event.toString());
//                        //            }
//                    }
                }

                t2 = System.currentTimeMillis();
                System.out.println("Sequential Time = " + (t2-t1) + " milliseconds");




               fileReader.rewind();

                t1 = System.currentTimeMillis();

                for (int i=1; i <= evCount; i++) {
                    event = fileReader.parseEvent(i);
                    if (event == null) {
                        System.out.println("We got a NULL event !!!");
                        return;
                    }

//                    if (i %10 == 0) {
//                        System.out.println("Event #" + i + " =\n" + event);
//                        int[] d = event.getIntData();
//                        System.out.println("Data[0] = " + d[0] + ", Data[last] = " + d[d.length-1]);
//                    }

                }

                t2 = System.currentTimeMillis();
                System.out.println("Random access Time = " + (t2-t1) + " milliseconds");



            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }
        }


        if (false) {
            System.out.println("\nTRY READING");

            File fileIn = new File(fileName);
            System.out.println("read ev file: " + fileName + " size: " + fileIn.length());
            try {
                EvioReader fileReader = new EvioReader(fileName);
                System.out.println("\nev count= " + fileReader.getEventCount());
                System.out.println("dictionary = " + fileReader.getDictionaryXML() + "\n");


                System.out.println("ev count = " + fileReader.getEventCount());

            }
            catch (IOException e) {
                e.printStackTrace();
            }
            catch (EvioException e) {
                e.printStackTrace();
            }
        }


        System.out.println("DONE READING");

    }



}
