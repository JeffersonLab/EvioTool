package org.jlab.coda.jevio;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.nio.channels.FileChannel;
import java.util.IllegalFormatException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This class contains generally useful methods.
 * @author timmer
 * Date: 6/11/13
 */
public class Utilities {

    /**
     * This method generates part of a file name given a base file name as an argument.<p>
     *
     * The base file name may contain up to 2, C-style integer format specifiers
     * (such as <b>%03d</b>, or <b>%x</b>). If more than 2 are found, an exception
     * will be thrown.
     * If no "0" precedes any integer between the "%" and the "d" or "x" of the format specifier,
     * it will be added automatically in order to avoid spaces in the returned string.
     * In the {@link #generateFileName(String, int, int, long, int)} method, the first
     * occurrence will be substituted with the given runNumber value.
     * If the file is being split, the second will be substituted with the split number.<p>
     *
     * The base file name may contain characters of the form <b>$(ENV_VAR)</b>
     * which will be substituted with the value of the associated environmental
     * variable or a blank string if none is found.<p>
     *
     * Finally, the base file name may contain occurrences of the string "%s"
     * which will be substituted with the value of the runType arg or nothing if
     * the runType is null.
     *
     * @param baseName        file name to start with
     * @param runType         run type/configuration name
     * @param newNameBuilder  object which contains generated base file name
     * @return                number of C-style int format specifiers found in baseName arg
     * @throws EvioException  if baseName arg is improperly formatted;
     *                        if baseName or newNameBuilder arg is null
     */
    public static int generateBaseFileName(String baseName, String runType,
                                           StringBuilder newNameBuilder)
            throws EvioException {

        String baseFileName;

        if (baseName == null || newNameBuilder == null) {
            throw new EvioException("null arg(s)");
        }

        // Replace all %s occurrences with run type
        baseName = (runType == null)?  baseName.replace("%s", "") :
                                       baseName.replace("%s", runType);

        // Scan for environmental variables of the form $(xxx)
        // and substitute the values for them (blank string if not found)
        if (baseName.contains("$(")) {
            Pattern pattern = Pattern.compile("\\$\\((.*?)\\)");
            Matcher matcher = pattern.matcher(baseName);
            StringBuffer result = new StringBuffer(100);

            while (matcher.find()) {
                String envVar = matcher.group(1);
                String envVal = System.getenv(envVar);
                if (envVal == null) envVal = "";
//System.out.println("generateBaseFileName: replacing " + envVar + " with " + envVal);
                matcher.appendReplacement(result, envVal);
            }
            matcher.appendTail(result);
//System.out.println("generateBaseFileName: Resulting string = " + result);
            baseFileName = result.toString();
        }
        else {
            baseFileName = baseName;
        }

        // How many C-style int specifiers are in baseFileName?
        Pattern pattern = Pattern.compile("%(\\d*)([xd])");
        Matcher matcher = pattern.matcher(baseFileName);
        StringBuffer result = new StringBuffer(100);

        int specifierCount = 0;
        while (matcher.find()) {
            String width = matcher.group(1);
            // Make sure any number preceding "x" or "d" starts with a 0
            // or else there will be empty spaces in the file name.
            if (width.length() > 0 && !width.startsWith("0")) {
                String newWidth = "0" + width;
//System.out.println("generateFileName: replacing " + width + " with " + newWidth);
                matcher.appendReplacement(result, "%" + newWidth + matcher.group(2));
            }

            specifierCount++;
        }
        matcher.appendTail(result);
        baseFileName = result.toString();

        if (specifierCount > 2) {
            throw new EvioException("baseName arg is improperly formatted");
        }

        // Return the base file name
        newNameBuilder.delete(0, newNameBuilder.length()).append(baseFileName);

        // Return the number of C-style int format specifiers
        return specifierCount;
    }


    /**
     * This method generates a complete file name from the previously determined baseFileName
     * obtained from calling {@link #generateBaseFileName(String, String, StringBuilder)}.
     * If evio data is to be split up into multiple files (split > 0), numbers are used to
     * distinguish between the split files with splitNumber.
     * If baseFileName contains C-style int format specifiers (specifierCount > 0), then
     * the first occurrence will be substituted with the given runNumber value.
     * If the file is being split, the second will be substituted with the splitNumber.
     * If 2 specifiers exist and the file is not being split, no substitutions are made.
     * If no specifier for the splitNumber exists, it is tacked onto the end of the file name.
     *
     * @param baseFileName   file name to use as a basis for the generated file name
     * @param specifierCount number of C-style int format specifiers in baseFileName arg
     * @param runNumber      CODA run number
     * @param split          number of bytes at which to split off evio file
     * @param splitNumber    number of the split file
     *
     * @return generated file name
     *
     * @throws IllegalFormatException if the baseFileName arg contains printing format
     *                                specifiers which are not compatible with integers
     *                                and interfere with formatting
     */
    public static String generateFileName(String baseFileName, int specifierCount,
                                          int runNumber, long split, int splitNumber)
                        throws IllegalFormatException {

        String fileName = baseFileName;

        // If we're splitting files ...
        if (split > 0L) {
            // For no specifiers: tack split # on end of base file name
            if (specifierCount < 1) {
                fileName = baseFileName + "." + splitNumber;
            }
            // For 1 specifier: insert run # at specified location, then tack split # on end
            else if (specifierCount == 1) {
                fileName = String.format(baseFileName, runNumber);
                fileName += "." + splitNumber;
            }
            // For 2 specifiers: insert run # and split # at specified locations
            else {
                fileName = String.format(baseFileName, runNumber, splitNumber);
            }
        }
        // If we're not splitting files ...
        else {
            if (specifierCount == 1) {
                // Insert runNumber
                fileName = String.format(baseFileName, runNumber);
            }
            else if (specifierCount == 2) {
                // First get rid of the extra (last) int format specifier as no split # exists
                Pattern pattern = Pattern.compile("(%\\d*[xd])");
                Matcher matcher = pattern.matcher(fileName);
                StringBuffer result = new StringBuffer(100);

                if (matcher.find()) {
                    // Only look at second int format specifier
                    if (matcher.find()) {
                        matcher.appendReplacement(result, "");
                        matcher.appendTail(result);
                        fileName = result.toString();
//System.out.println("generateFileName: replacing last int specifier =  " + fileName);
                    }
                }

                // Insert runNumber into first specifier
                fileName = String.format(fileName, runNumber);
            }
        }

        return fileName;
    }


    /**
     * This method takes a byte buffer and saves it to the specified file,
     * overwriting whatever is there.
     *
     * @param fileName       name of file to write
     * @param buf            buffer to write to file
     * @param overWriteOK    if {@code true}, OK to overwrite previously existing file
     * @param addBlockHeader if {@code true}, add evio block header for proper evio file format
     * @throws IOException   if trouble writing to file
     * @throws EvioException if file exists but overwriting is not permitted;
     *                       if null arg(s)
     */
    public static void bufferToFile(String fileName, ByteBuffer buf,
                                    boolean overWriteOK, boolean addBlockHeader)
            throws IOException, EvioException{

        if (fileName == null || buf == null) {
            throw new EvioException("null arg(s)");
        }

        File file = new File(fileName);

        // If we can't overwrite and file exists, throw exception
        if (!overWriteOK && (file.exists() && file.isFile())) {
            throw new EvioException("File exists but over-writing not permitted, "
                    + file.getPath());
        }

        int limit    = buf.limit();
        int position = buf.position();

        FileOutputStream fileOutputStream = new FileOutputStream(file);
        FileChannel fileChannel = fileOutputStream.getChannel();

        if (addBlockHeader) {
            ByteBuffer blockHead = ByteBuffer.allocate(32);
            blockHead.order(buf.order());
            blockHead.putInt(8 + (limit - position)/4);   // total len of block in words
            blockHead.putInt(1);                          // block number
            blockHead.putInt(8);                          // header len in words
            blockHead.putInt(1);                          // event count
            blockHead.putInt(0);                          // reserved
            blockHead.putInt(0x204);                      // last block, version 4
            blockHead.putInt(0);                          // reserved
            blockHead.putInt(BlockHeaderV4.MAGIC_NUMBER); // 0xcoda0100
            blockHead.flip();
            fileChannel.write(blockHead);
        }

        fileChannel.write(buf);
        fileChannel.close();
        buf.limit(limit).position(position);
    }


    /**
     * This method takes a byte buffer and prints out the desired number of words
     * from the given position.
     *
     * @param buf            buffer to print out
     * @param position       position of data (bytes) in buffer to start printing
     * @param words          number of 32 bit words to print in hex
     * @param label          a label to print as header
     */
    synchronized public static void printBuffer(ByteBuffer buf, int position, int words, String label) {

        if (buf == null) {
            System.out.println("printBuffer: buf arg is null");
            return;
        }

        int origPos = buf.position();
        buf.position(position);

        if (label != null) System.out.println(label + ":");

        IntBuffer ibuf = buf.asIntBuffer();
        words = words > ibuf.capacity() ? ibuf.capacity() : words;
        for (int i=0; i < words; i++) {
            System.out.println("  Buf(" + i + ") = 0x" + Integer.toHexString(ibuf.get(i)));
        }
        System.out.println();

        buf.position(origPos);
   }

}
