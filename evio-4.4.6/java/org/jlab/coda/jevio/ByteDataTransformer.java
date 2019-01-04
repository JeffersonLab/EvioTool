package org.jlab.coda.jevio;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.*;
import java.util.List;


/**
 * This utility class contains methods for transforming a raw byte array into arrays of
 * other types and vice versa as well as other handy methods.
 * 
 * @author heddle
 * @author timmer
 * @author wolin
 *
 */
public class ByteDataTransformer {

    /**
     * Converts a byte array into an int array.
     * Use {@link #toIntArray(byte[], java.nio.ByteOrder)} for better performance.
     *
     * @param bytes the byte array.
     * @param byteOrder the endianness of the data in the byte array,
     *       {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}.
     * @return the raw bytes converted into an int array.
     * @deprecated
     */
    public static int[] getAsIntArray(byte bytes[], ByteOrder byteOrder) {
        ByteBuffer byteBuffer = ByteBuffer.wrap(bytes).order(byteOrder);
        int intsize = bytes.length / 4;
        int array[] = new int[intsize];
        for (int i = 0; i < intsize; i++) {
            array[i] = byteBuffer.getInt();
        }
        return array;
    }


    /**
     * Converts a byte array into an short array.
     * Use {@link #toShortArray(byte[], java.nio.ByteOrder)} for better performance.
     *
     * @param bytes the byte array.
     * @param byteOrder the endianness of the data in the byte array,
     *       {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}.
     * @return the raw bytes converted into an int array.
     * @deprecated
     */
    public static short[] getAsShortArray(byte bytes[], ByteOrder byteOrder) {
        ByteBuffer byteBuffer = ByteBuffer.wrap(bytes).order(byteOrder);
        int shortsize = bytes.length / 2;
        short array[] = new short[shortsize];
        for (int i = 0; i < shortsize; i++) {
            array[i] = byteBuffer.getShort();
        }
        return array;
    }

	/**
	 * Converts a byte array into an short array while accounting for padding.
     * Use {@link #toShortArray(byte[], int, java.nio.ByteOrder)} for better performance.
	 *
     * @param bytes the byte array.
     * @param padding number of <b>bytes</b> at the end of the byte array to ignore.
     *                Valid values are 0 and mulitples of 2 (though only 0 & 2 are used).
     * @param byteOrder the endianness of the data in the byte array,
     *       {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}.
	 * @return the raw bytes converted into an int array.
     * @deprecated
	 */
	public static short[] getAsShortArray(byte bytes[], int padding, ByteOrder byteOrder) {
		ByteBuffer byteBuffer = ByteBuffer.wrap(bytes).order(byteOrder);
		int shortsize = bytes.length / 2;
        if (padding%2 == 0) {
            shortsize -= padding/2;
        }
		short array[] = new short[shortsize];
		for (int i = 0; i < shortsize; i++) {
			array[i] = byteBuffer.getShort();
		}
		return array;
	}

	
	/**
	 * Converts a byte array into a long array.
     * Use {@link #toLongArray(byte[], java.nio.ByteOrder)} for better performance.
	 *
	 * @param bytes the byte array.
     * @param byteOrder the endianness of the data in the byte array,
     *       {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}.
	 * @return the raw bytes converted into a long array.
     * @deprecated
	 */
	public static long[] getAsLongArray(byte bytes[], ByteOrder byteOrder) {
		ByteBuffer byteBuffer = ByteBuffer.wrap(bytes).order(byteOrder);
		int longsize = bytes.length / 8;
		long array[] = new long[longsize];
		for (int i = 0; i < longsize; i++) {
			array[i] = byteBuffer.getLong();
		}
		return array;
	}

	/**
	 * Converts a byte array into a double array.
     * Use {@link #toDoubleArray(byte[], java.nio.ByteOrder)} for better performance.
	 *
	 * @param bytes the byte array.
     * @param byteOrder the endianness of the data in the byte array,
     *       {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}.
	 * @return the raw bytes converted into a double array.
     * @deprecated
	 */
	public static double[] getAsDoubleArray(byte bytes[], ByteOrder byteOrder) {
		ByteBuffer byteBuffer = ByteBuffer.wrap(bytes).order(byteOrder);
		int doublesize = bytes.length / 8;
		double array[] = new double[doublesize];
		for (int i = 0; i < doublesize; i++) {
			array[i] = byteBuffer.getDouble();
		}
		return array;
	}
	
	/**
	 * Converts a byte array into a float array.
     * Use {@link #toFloatArray(byte[], java.nio.ByteOrder)} for better performance.
	 *
	 * @param bytes the byte array.
     * @param byteOrder the endianness of the data in the byte array,
     *       {@link ByteOrder#BIG_ENDIAN} or {@link ByteOrder#LITTLE_ENDIAN}.
	 * @return the raw bytes converted into a float array.
     * @deprecated
	 */
	public static float[] getAsFloatArray(byte bytes[], ByteOrder byteOrder) {
		ByteBuffer byteBuffer = ByteBuffer.wrap(bytes).order(byteOrder);
		int floatsize = bytes.length / 4;
		float array[] = new float[floatsize];
		for (int i = 0; i < floatsize; i++) {
			array[i] = byteBuffer.getFloat();
		}
		return array;
	}


    /**
     * Copies a 2-byte short value into a 4-byte int while preserving bit pattern.
     * In other words, it treats the short as an unsigned value. The resulting int
     * does NOT undergo sign extension (bits of highest 2 bytes are not all ones),
     * and will not be a negative number.
     *
     * @param shortVal short value considered to be unsigned
     * @return the equivalent int value
     */
    public static final int shortBitsToInt(short shortVal) {
        return (shortVal & 0x0000ffff);
    }


    /**
     * Copies a 1-byte byte value into a 4-byte int while preserving bit pattern.
     * In other words, it treats the byte as an unsigned value. The resulting int
     * does NOT undergo sign extension (bits of highest 3 bytes are not all ones),
     * and will not be a negative number.
     *
     * @param byteVal byte value considered to be unsigned
     * @return the equivalent int value
     */
    public static final int byteBitsToInt(byte byteVal) {
        return (byteVal & 0x000000ff);
    }


    // ==========================
    // Buffer --> primitive array
    // ==========================


    /**
     * Converts an ByteBuffer object into a byte array.
     * Copy is made of the data.
     *
     * @param byteBuffer the buffer to convert.
     * @return the ByteBuffer converted into a byte array.
     */
    public static byte[] toByteArray(ByteBuffer byteBuffer) {
        if (byteBuffer == null) return null;
        int size = byteBuffer.remaining();
        byte array[] = new byte[size];

        // Avoid potential disaster if byteBuffer is a slice.
        // In that case, it's backing array can be bigger than its capacity
        // with no way to tell what the proper offset is. In this case do NOT
        // do the arraycopy.

        if (byteBuffer.hasArray() && (byteBuffer.array().length == byteBuffer.capacity())) {
            System.arraycopy(byteBuffer.array(), byteBuffer.position(),
                             array, 0, byteBuffer.remaining());
        }
        else {
            int pos = byteBuffer.position();
            byteBuffer.get(array, 0, size);
            byteBuffer.position(pos);
        }

        return array;
    }


    /**
     * Converts an ByteBuffer object into an int array.
     *
     * @param byteBuffer the buffer to convert.
     * @return the ByteBuffer converted into an int array.
     */
    public static int[] toIntArray(ByteBuffer byteBuffer) {
        if (byteBuffer == null) return null;

        IntBuffer ibuf = byteBuffer.asIntBuffer();

        int size = ibuf.limit();
        int array[] = new int[size];
        ibuf.get(array, 0, size);
        return array;
    }


    /**
     * Converts an ByteBuffer object into a short array.
     *
     * @param byteBuffer the buffer to convert.
     * @return the ByteBuffer converted into a short array.
     */
    public static short[] toShortArray(ByteBuffer byteBuffer) {
        if (byteBuffer == null) return null;

        ShortBuffer sbuf = byteBuffer.asShortBuffer();

        int size = sbuf.limit();
        short array[] = new short[size];
        sbuf.get(array, 0, size);
        return array;
    }


    /**
     * Converts an ByteBuffer object into a long array.
     *
     * @param byteBuffer the buffer to convert.
     * @return the ByteBuffer converted into a long array.
     */
    public static long[] toLongArray(ByteBuffer byteBuffer) {
        if (byteBuffer == null) return null;

        LongBuffer lbuf = byteBuffer.asLongBuffer();

        int size = lbuf.limit();
        long array[] = new long[size];
        lbuf.get(array, 0, size);
        return array;
    }


    /**
     * Converts an ByteBuffer object into a float array.
     *
     * @param byteBuffer the buffer to convert.
     * @return the ByteBuffer converted into a float array.
     */
    public static float[] toFloatArray(ByteBuffer byteBuffer) {
        if (byteBuffer == null) return null;

        FloatBuffer fbuf = byteBuffer.asFloatBuffer();

        int size = fbuf.limit();
        float array[] = new float[size];
        fbuf.get(array, 0, size);
        return array;
    }


    /**
     * Converts an ByteBuffer object into a double array.
     *
     * @param byteBuffer the buffer to convert.
     * @return the ByteBuffer converted into a double array.
     */
    public static double[] toDoubleArray(ByteBuffer byteBuffer) {
        if (byteBuffer == null) return null;

        DoubleBuffer dbuf = byteBuffer.asDoubleBuffer();

        int size = dbuf.limit();
        double array[] = new double[size];
        dbuf.get(array, 0, size);
        return array;
    }


    /**
     * Converts an ByteBuffer object containing evio String data into a String array.
     *
     * @param byteBuffer the buffer to convert.
     * @return the ByteBuffer converted into a String array; null if bad format.
     */
    public static String[] toStringArray(ByteBuffer byteBuffer) {
        if (byteBuffer == null) return null;

        return BaseStructure.unpackRawBytesToStrings(byteBuffer,
                                                     byteBuffer.position(),
                                                     byteBuffer.limit() - byteBuffer.position());
    }


    // =========================
    // primitive type --> byte[]
    // =========================

    /**
     * Copies an integer value into 4 bytes of a byte array.
     * @param intVal integer value
     * @param b byte array
     * @param off offset into the byte array
     */
    public static final void intToBytes(int intVal, byte[] b, int off) {
        b[off  ] = (byte)(intVal >> 24);
        b[off+1] = (byte)(intVal >> 16);
        b[off+2] = (byte)(intVal >>  8);
        b[off+3] = (byte)(intVal      );
    }


    /**
     * Copies a short value into 2 bytes of a byte array.
     * @param val short value
     * @param b byte array
     * @param off offset into the byte array
     */
    public static final void shortToBytes(short val, byte[] b, int off) {
        b[off  ] = (byte)(val >>  8);
        b[off+1] = (byte)(val      );
    }



    /**
     * Turn short into byte array.
     *
     * @param data short to convert
     * @param byteOrder byte order of returned byte array (big endian if null)
     * @return byte array representing short
     */
    public static byte[] toBytes(short data, ByteOrder byteOrder) {
        if (byteOrder == ByteOrder.BIG_ENDIAN) {
            return new byte[] {
                    (byte)(data >>> 8),   // byte cast just truncates data val
                    (byte)(data),
            };
        }
        else {
            return new byte[] {
                    (byte)(data),
                    (byte)(data >>> 8),
            };
        }
    }


    /**
     * Turn short into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data short to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @param dest array in which to store returned bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if dest is null or too small or offset negative
     */
    public static void toBytes(short data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException {

        if (dest == null || dest.length < 2+off || off < 0) {
            throw new EvioException("bad arg(s)");
        }

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            dest[off  ] = (byte)(data >>> 8);
            dest[off+1] = (byte)(data      );
        }
        else {
            dest[off  ] = (byte)(data      );
            dest[off+1] = (byte)(data >>> 8);
        }
    }


    /**
     * Turn short array into byte array.
     *
     * @param data short array to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @return byte array representing short array or null if data is null
     * @throws EvioException if data array has too many elements to convert to a byte array
     */
    public static byte[] toBytes(short[] data, ByteOrder byteOrder) throws EvioException {

        if (data == null) return null;
        if (data.length > Integer.MAX_VALUE/2) {
            throw new EvioException("short array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        byte[] b = new byte[data.length*2];

        ByteBuffer buf = ByteBuffer.wrap(b).order(byteOrder);
        ShortBuffer ib = buf.asShortBuffer();
        ib.put(data, 0, data.length);

        return b;
    }


    /**
      * Turn short array into byte array.
      * Avoids creation of new byte array with each call.
      *
      * @param data short array to convert
      * @param byteOrder byte order of written bytes (big endian if null)
      * @param dest array in which to store written bytes
      * @param off offset into dest array where returned bytes are placed
      * @throws EvioException if data is null, dest is null or too small, or offset negative;
      *                       if data array has too many elements to convert to a byte array
      */
     public static void toBytes(short[] data, ByteOrder byteOrder, byte[] dest, int off)
             throws EvioException {

         if (data == null || dest == null || dest.length < 2*data.length+off || off < 0) {
             throw new EvioException("bad arg(s)");
         }

         if (data.length > Integer.MAX_VALUE/2) {
             throw new EvioException("short array has too many elements to convert to byte array");
         }

         if (byteOrder == null) {
             byteOrder = ByteOrder.BIG_ENDIAN;
         }

         ByteBuffer buf = ByteBuffer.wrap(dest).order(byteOrder);
         buf.position(off);
         ShortBuffer ib = buf.asShortBuffer();
         ib.put(data, 0, data.length);

         return;
     }


    // =========================

    /**
     * Turn int into byte array.
     *
     * @param data int to convert
     * @param byteOrder byte order of returned byte array (big endian if null)
     * @return byte array representing int
     */
    public static byte[] toBytes(int data, ByteOrder byteOrder) {
        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return new byte[] {
                    (byte)(data >> 24),
                    (byte)(data >> 16),
                    (byte)(data >>  8),
                    (byte)(data),
            };
        }
        else {
            return new byte[] {
                    (byte)(data),
                    (byte)(data >>  8),
                    (byte)(data >> 16),
                    (byte)(data >> 24),
            };
        }
    }


    /**
     * Turn int into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data int to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @param dest array in which to store returned bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if dest is null or too small or offset negative
     */
    public static void toBytes(int data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException {

        if (dest == null || dest.length < 4+off || off < 0) {
            throw new EvioException("bad arg(s)");
        }

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            dest[off  ] = (byte)(data >> 24);
            dest[off+1] = (byte)(data >> 16);
            dest[off+2] = (byte)(data >>  8);
            dest[off+3] = (byte)(data      );
        }
        else {
            dest[off  ] = (byte)(data      );
            dest[off+1] = (byte)(data >>  8);
            dest[off+2] = (byte)(data >> 16);
            dest[off+3] = (byte)(data >> 24);
        }
    }


    /**
     * Turn int array into byte array.
     *
     * @param data int array to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @return byte array representing int array or null if data is null
     * @throws EvioException if data array as too many elements to
     *                       convert to a byte array
     */
    public static byte[] toBytes(int[] data, ByteOrder byteOrder) throws EvioException {

        if (data == null) return null;
        if (data.length > Integer.MAX_VALUE/4) {
            throw new EvioException("int array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        byte[] b = new byte[data.length*4];

        ByteBuffer buf = ByteBuffer.wrap(b).order(byteOrder);
        IntBuffer ib = buf.asIntBuffer();
        ib.put(data, 0, data.length);

        return b;
    }


    /**
     * Turn int array into byte array.
     *
     * @param data int array to convert
     * @param offset into int array
     * @param length number of integers to conver to bytes
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @return byte array representing int array or null if data is null
     * @throws EvioException if data array has too many elements to
     *                       convert to a byte array
     */
    public static byte[] toBytes(int[] data, int offset, int length, ByteOrder byteOrder)
                    throws EvioException {

        if (data == null) return null;
        if (offset < 0 || length < 1) return null;
        if (data.length > Integer.MAX_VALUE/4) {
            throw new EvioException("int array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        byte[] b = new byte[length*4];

        ByteBuffer buf = ByteBuffer.wrap(b).order(byteOrder);
        IntBuffer ib = buf.asIntBuffer();
        ib.put(data, offset, length);

        return b;
    }


    /**
     * Turn int array into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data int array to convert
     * @param byteOrder byte order of written bytes (big endian if null)
     * @param dest array in which to store written bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if data is null, dest is null or too small, or offset negative;
     *                       if data array has too many elements to convert to a byte array
     */
    public static void toBytes(int[] data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException{

        if (data == null || dest == null || dest.length < 4*data.length+off || off < 0) {
            throw new EvioException("bad arg(s)");
        }

        if (data.length > Integer.MAX_VALUE/4) {
            throw new EvioException("int array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        ByteBuffer buf = ByteBuffer.wrap(dest).order(byteOrder);
        buf.position(off);
        IntBuffer ib = buf.asIntBuffer();
        ib.put(data, 0, data.length);

        return;
    }


    /**
     * Turn int array into byte array.
     * Uses IO streams to do the work - very inefficient.
     * Keep around for tutorial purposes.
     *
     * @param data int array to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @return byte array representing int array
     * @throws EvioException if data array has too many elements to
     *                       convert to a byte array
     */
    public static byte[] toBytesStream(int[] data, ByteOrder byteOrder) throws EvioException {

        if (data == null) return null;
        if (data.length > Integer.MAX_VALUE/4) {
            throw new EvioException("int array has too many elements to convert to byte array");
        }

        ByteArrayOutputStream baos = new ByteArrayOutputStream(4*data.length);
        DataOutputStream out = new DataOutputStream(baos);

        try {
            for (int i : data) {
                if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
                    out.writeInt(i);
                }
                else {
                    out.writeInt(Integer.reverseBytes(i));
                }
            }
            out.flush();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

        return baos.toByteArray();
    }

    // =========================

    /**
     * Turn long into byte array.
     *
     * @param data long to convert
     * @param byteOrder byte order of returned byte array (big endian if null)
     * @return byte array representing long
     */
    public static byte[] toBytes(long data, ByteOrder byteOrder) {

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return new byte[] {
                    (byte)(data >> 56),
                    (byte)(data >> 48),
                    (byte)(data >> 40),
                    (byte)(data >> 32),
                    (byte)(data >> 24),
                    (byte)(data >> 16),
                    (byte)(data >>  8),
                    (byte)(data      ),
            };
        }
        else {
            return new byte[] {
                    (byte)(data      ),
                    (byte)(data >>  8),
                    (byte)(data >> 16),
                    (byte)(data >> 24),
                    (byte)(data >> 32),
                    (byte)(data >> 40),
                    (byte)(data >> 48),
                    (byte)(data >> 56),
            };
        }
    }


    /**
     * Turn long into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data long to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @param dest array in which to store returned bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if dest is null or too small or offset negative
     */
    public static void toBytes(long data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException {

        if (dest == null || dest.length < 8+off || off < 0) {
            throw new EvioException("bad arg(s)");
        }

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            dest[off  ] = (byte)(data >> 56);
            dest[off+1] = (byte)(data >> 48);
            dest[off+2] = (byte)(data >> 40);
            dest[off+3] = (byte)(data >> 32);
            dest[off+4] = (byte)(data >> 24);
            dest[off+5] = (byte)(data >> 16);
            dest[off+6] = (byte)(data >>  8);
            dest[off+7] = (byte)(data      );
        }
        else {
            dest[off  ] = (byte)(data      );
            dest[off+1] = (byte)(data >>  8);
            dest[off+2] = (byte)(data >> 16);
            dest[off+3] = (byte)(data >> 24);
            dest[off+4] = (byte)(data >> 32);
            dest[off+5] = (byte)(data >> 40);
            dest[off+6] = (byte)(data >> 48);
            dest[off+7] = (byte)(data >> 56);
        }
    }


    /**
     * Turn long array into byte array.
     *
     * @param data long array to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @return byte array representing long array or null if data is null
     * @throws EvioException if data array has too many elements to
     *                       convert to a byte array
     */
    public static byte[] toBytes(long[] data, ByteOrder byteOrder) throws EvioException {

        if (data == null) return null;
        if (data.length > Integer.MAX_VALUE/8) {
            throw new EvioException("long array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        byte[] b = new byte[data.length*8];

        ByteBuffer buf = ByteBuffer.wrap(b).order(byteOrder);
        LongBuffer ib = buf.asLongBuffer();
        ib.put(data, 0, data.length);

        return b;
    }


    /**
     * Turn long array into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data long array to convert
     * @param byteOrder byte order of written bytes (big endian if null)
     * @param dest array in which to store written bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if data is null, dest is null or too small, or offset negative;
     *                       if data array has too many elements to convert to a byte array
     */
    public static void toBytes(long[] data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException{

        if (data == null || dest == null || dest.length < 8*data.length+off || off < 0) {
            throw new EvioException("bad arg(s)");
        }

        if (data.length > Integer.MAX_VALUE/8) {
            throw new EvioException("long array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        ByteBuffer buf = ByteBuffer.wrap(dest).order(byteOrder);
        buf.position(off);
        LongBuffer ib = buf.asLongBuffer();
        ib.put(data, 0, data.length);

        return;
    }


    // =========================

    /**
     * Turn float into byte array.
     *
     * @param data float to convert
     * @param byteOrder byte order of returned byte array (big endian if null)
     * @return byte array representing float
     */
    public static byte[] toBytes(float data, ByteOrder byteOrder) {
        return toBytes(Float.floatToRawIntBits(data), byteOrder);
    }

    /**
     * Turn float into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data float to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @param dest array in which to store returned bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if dest is null or too small or offset negative
     */
    public static void toBytes(float data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException {

        toBytes(Float.floatToRawIntBits(data), byteOrder, dest, off);
    }

    /**
     * Turn float array into byte array.
     *
     * @param data float array to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @return byte array representing float array or null if data is null
     * @throws EvioException if data array has too many elements to
     *                       convert to a byte array
     */
    public static byte[] toBytes(float[] data, ByteOrder byteOrder) throws EvioException {

        if (data == null) return null;
        if (data.length > Integer.MAX_VALUE/4) {
            throw new EvioException("float array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        byte[] b = new byte[data.length*4];

        ByteBuffer buf = ByteBuffer.wrap(b).order(byteOrder);
        FloatBuffer ib = buf.asFloatBuffer();
        ib.put(data, 0, data.length);

        return b;
    }

    /**
     * Turn float array into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data float array to convert
     * @param byteOrder byte order of written bytes (big endian if null)
     * @param dest array in which to store written bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if data is null, dest is null or too small, or offset negative;
     *                       if data array has too many elements to convert to a byte array
     */
    public static void toBytes(float[] data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException{

        if (data == null || dest == null || dest.length < 4*data.length+off || off < 0) {
            throw new EvioException("bad arg(s)");
        }

        if (data.length > Integer.MAX_VALUE/4) {
            throw new EvioException("float array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        ByteBuffer buf = ByteBuffer.wrap(dest).order(byteOrder);
        buf.position(off);
        FloatBuffer ib = buf.asFloatBuffer();
        ib.put(data, 0, data.length);

        return;
    }

    // =========================

    /**
     * Turn double into byte array.
     *
     * @param data double to convert
     * @param byteOrder byte order of returned byte array (big endian if null)
     * @return byte array representing double
     */
    public static byte[] toBytes(double data, ByteOrder byteOrder) {
        return toBytes(Double.doubleToRawLongBits(data), byteOrder);
    }

    /**
     * Turn double into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data double to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @param dest array in which to store returned bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if dest is null or too small or offset negative
     */
    public static void toBytes(double data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException {

        toBytes(Double.doubleToRawLongBits(data), byteOrder, dest, off);
    }

    /**
     * Turn double array into byte array.
     *
     * @param data double array to convert
     * @param byteOrder byte order of returned bytes (big endian if null)
     * @return byte array representing double array or null if data is null
     * @throws EvioException if data array has too many elements to
     *                       convert to a byte array
     */
    public static byte[] toBytes(double[] data, ByteOrder byteOrder) throws EvioException {

        if (data == null) return null;
        if (data.length > Integer.MAX_VALUE/8) {
            throw new EvioException("double array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        byte[] b = new byte[data.length*8];

        ByteBuffer buf = ByteBuffer.wrap(b).order(byteOrder);
        DoubleBuffer ib = buf.asDoubleBuffer();
        ib.put(data, 0, data.length);

        return b;
    }

    /**
     * Turn double array into byte array.
     * Avoids creation of new byte array with each call.
     *
     * @param data double array to convert
     * @param byteOrder byte order of written bytes (big endian if null)
     * @param dest array in which to store written bytes
     * @param off offset into dest array where returned bytes are placed
     * @throws EvioException if data is null, dest is null or too small, or offset negative;
     *                       if data array has too many elements to convert to a byte array
     */
    public static void toBytes(double[] data, ByteOrder byteOrder, byte[] dest, int off)
            throws EvioException{

        if (data == null || dest == null || dest.length < 8*data.length+off || off < 0) {
            throw new EvioException("bad arg(s)");
        }

        if (data.length > Integer.MAX_VALUE/8) {
            throw new EvioException("double array has too many elements to convert to byte array");
        }

        if (byteOrder == null) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        }

        ByteBuffer buf = ByteBuffer.wrap(dest).order(byteOrder);
        buf.position(off);
        DoubleBuffer ib = buf.asDoubleBuffer();
        ib.put(data, 0, data.length);

        return;
    }

    // =========================
    // byte[] --> primitive type
    // =========================

    /**
     * Turn section of byte array into a short.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param off offset into data array
     * @return short converted from byte array
     * @throws EvioException if data is null or wrong size, or off is negative
     */
    public static short toShort(byte[] data, ByteOrder byteOrder, int off) throws EvioException {
        if (data == null || data.length < 2+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (short)(
                (0xff & data[  off]) << 8 |
                (0xff & data[1+off])
            );
        }
        else {
            return (short)(
                (0xff & data[  off]) |
                (0xff & data[1+off]) << 8
            );
        }
    }

    /**
     * Turn 2 bytes into a short.
     *
     * @param b1 1st byte
     * @param b2 2nd byte
     * @param byteOrder if big endian, 1st byte is most significant (big endian if null)
     * @return short converted from byte array
     */
    public static short toShort(byte b1, byte b2, ByteOrder byteOrder) {

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (short)(
                (0xff & b1) << 8 |
                (0xff & b2)
            );
        }
        else {
            return (short)(
                (0xff & b1) |
                (0xff & b2) << 8
            );
        }
    }

    /**
     * Turn byte array into a short array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @return short array converted from byte array
     * @throws EvioException if data is null or odd size
     */
    public static short[] toShortArray(byte[] data, ByteOrder byteOrder) throws EvioException {
        if (data == null || data.length % 2 != 0) {
            throw new EvioException("bad data arg");
        }

        short[] sa = new short[data.length / 2];
        for (int i=0; i < sa.length; i++) {
            sa[i] = toShort(data[(i*2)],
                            data[(i*2)+1],
                            byteOrder);
        }
        return sa;
    }

    /**
     * Turn byte array into a short array while taking padding into account.
     *
     * @param data byte array to convert
     * @param padding number of <b>bytes</b> at the end of the byte array to ignore.
     *                Valid values are 0 and 2
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @return short array converted from byte array
     * @throws EvioException if data is null or odd size; padding not 0 or 2
     */
    public static short[] toShortArray(byte[] data, int padding, ByteOrder byteOrder) throws EvioException {
        if (data == null || data.length % 2 != 0 || (padding != 0 && padding != 2)) {
            throw new EvioException("bad data arg");
        }

        short[] sa = new short[(data.length - padding)/2];
        for (int i=0; i < sa.length; i++) {
            sa[i] = toShort(data[(i*2)],
                            data[(i*2)+1],
                            byteOrder);
        }
        return sa;
    }

    /**
     * Turn byte array into a short array while taking padding into account.
     *
     * @param data byte array to convert
     * @param padding number of <b>bytes</b> at the end of the byte array to ignore.
     *                Valid values are 0 and 2
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param dest array in which to write converted bytes
     * @param off offset into dest array
     * @throws EvioException if data is null or wrong size; dest is null, wrong size, or off < 0
     */
    public static void toShortArray(byte[] data, int padding, ByteOrder byteOrder, short[] dest, int off)
            throws EvioException {

        if (data == null || data.length % 2 != 0 || (padding != 0 && padding != 2) ||
            dest == null || 2*dest.length < data.length-padding+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        //int len = data.length - padding;
System.out.println("toShortArray: padding = " + padding + ", data len = " + data.length);
        for (int i = 0; i < data.length - padding - 1; i+=2) {
            dest[i/2+off] = toShort(data[i],
                                    data[i+1],
                                    byteOrder);
        }
    }

    /**
     * Turn byte array into an short array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param dest array in which to write converted bytes
     * @param off offset into dest array
     * @throws EvioException if data is null or wrong size; dest is null, wrong size, or off < 0
     */
    public static void toShortArray(byte[] data, ByteOrder byteOrder, short[] dest, int off)
            throws EvioException {

        if (data == null || data.length % 2 != 0 ||
            dest == null || 2*dest.length < data.length+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        for (int i = 0; i < data.length-1; i+=2) {
            dest[i/2+off] = toShort(data[i],
                                    data[i + 1],
                                    byteOrder);
        }
    }

	    // =========================

    /**
     * Turn section of byte array into an int.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param off offset into data array
     * @return int converted from byte array
     * @throws EvioException if data is null or wrong size, or off is negative
     */
    public static int toInt(byte[] data, ByteOrder byteOrder, int off) throws EvioException {
        if (data == null || data.length < 4+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (
                (0xff & data[  off]) << 24 |
                (0xff & data[1+off]) << 16 |
                (0xff & data[2+off]) <<  8 |
                (0xff & data[3+off])
            );
        }
        else {
            return (
                (0xff & data[  off])       |
                (0xff & data[1+off]) <<  8 |
                (0xff & data[2+off]) << 16 |
                (0xff & data[3+off]) << 24
            );
        }
    }

    /**
     * Turn 4 bytes into an int.
     *
     * @param b1 1st byte
     * @param b2 2nd byte
     * @param b3 3rd byte
     * @param b4 4th byte
     * @param byteOrder if big endian, 1st byte is most significant &
     *                  4th is least (big endian if null)
     * @return int converted from byte array
     */
    public static int toInt(byte b1, byte b2, byte b3, byte b4, ByteOrder byteOrder) {

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (
                (0xff & b1) << 24 |
                (0xff & b2) << 16 |
                (0xff & b3) <<  8 |
                (0xff & b4)
            );
        }
        else {
            return (
                (0xff & b1)       |
                (0xff & b2) <<  8 |
                (0xff & b3) << 16 |
                (0xff & b4) << 24
            );
        }
    }

    /**
     * Turn byte array into an int array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @return int array converted from byte array
     * @throws EvioException if data is null or wrong size
     */
    public static int[] toIntArray(byte[] data, ByteOrder byteOrder) throws EvioException {
        if (data == null || data.length % 4 != 0) {
            throw new EvioException("bad data arg");
        }

        int indx;
        int[] ints = new int[data.length / 4];
        for (int i = 0; i < ints.length; i++) {
            indx = i*4;
            ints[i] = toInt(data[indx],
                            data[indx+1],
                            data[indx+2],
                            data[indx+3],
                            byteOrder);
        }
        return ints;
    }

    /**
     * Turn byte array into an int array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param dest array in which to write converted bytes
     * @param off offset into dest array
     * @throws EvioException if data is null or wrong size; dest is null, wrong size, or off < 0
     */
    public static void toIntArray(byte[] data, ByteOrder byteOrder, int[] dest, int off)
            throws EvioException {

        if (data == null || data.length % 4 != 0 ||
            dest == null || 4*dest.length < data.length+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        for (int i = 0; i < data.length-3; i+=4) {
            dest[i/4+off] = toInt(data[i],
                                  data[i+1],
                                  data[i+2],
                                  data[i+3],
                                  byteOrder);
        }
    }

    // =========================

    /**
     * Turn section of byte array into a long.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param off offset into data array
     * @return long converted from byte array
     * @throws EvioException if data is null or wrong size, or off is negative
     */
    public static long toLong(byte[] data, ByteOrder byteOrder, int off) throws EvioException {
        if (data == null || data.length < 8+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (
                // Convert to longs before shift because digits
                // are lost with ints beyond the 32-bit limit
                (long)(0xff & data[  off]) << 56 |
                (long)(0xff & data[1+off]) << 48 |
                (long)(0xff & data[2+off]) << 40 |
                (long)(0xff & data[3+off]) << 32 |
                (long)(0xff & data[4+off]) << 24 |
                (long)(0xff & data[5+off]) << 16 |
                (long)(0xff & data[6+off]) <<  8 |
                (long)(0xff & data[7+off])
            );
        }
        else {
            return (
                (long)(0xff & data[  off])       |
                (long)(0xff & data[1+off]) <<  8 |
                (long)(0xff & data[2+off]) << 16 |
                (long)(0xff & data[3+off]) << 24 |
                (long)(0xff & data[4+off]) << 32 |
                (long)(0xff & data[5+off]) << 40 |
                (long)(0xff & data[6+off]) << 48 |
                (long)(0xff & data[7+off]) << 56
            );
        }
    }


    /**
     * Turn 8 bytes into a long.
     *
     * @param b1 1st byte
     * @param b2 2nd byte
     * @param b3 3rd byte
     * @param b4 4th byte
     * @param b5 5th byte
     * @param b6 6th byte
     * @param b7 7th byte
     * @param b8 8th byte
     * @param byteOrder if big endian, 1st byte is most significant &
     *                  8th least (big endian if null)
     * @return long converted from byte array
     */
    public static long toLong(byte b1, byte b2, byte b3, byte b4,
                              byte b5, byte b6, byte b7, byte b8, ByteOrder byteOrder) {

        if (byteOrder == null || byteOrder == ByteOrder.BIG_ENDIAN) {
            return (
                (long)(0xff & b1) << 56 |
                (long)(0xff & b2) << 48 |
                (long)(0xff & b3) << 40 |
                (long)(0xff & b4) << 32 |
                (long)(0xff & b5) << 24 |
                (long)(0xff & b6) << 16 |
                (long)(0xff & b7) <<  8 |
                (long)(0xff & b8)
            );
        }
        else {
            return (
                (long)(0xff & b1)       |
                (long)(0xff & b2) <<  8 |
                (long)(0xff & b3) << 16 |
                (long)(0xff & b4) << 24 |
                (long)(0xff & b5) << 32 |
                (long)(0xff & b6) << 40 |
                (long)(0xff & b7) << 48 |
                (long)(0xff & b8) << 56
            );
        }
    }


    /**
     * Turn byte array into a long array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @return long array converted from byte array
     * @throws EvioException if data is null or wrong size
     */
    public static long[] toLongArray(byte[] data, ByteOrder byteOrder) throws EvioException {
        if (data == null || data.length % 8 != 0) {
            throw new EvioException("bad data arg");
        }

        int indx;
        long[] lngs = new long[data.length / 8];
        for (int i = 0; i < lngs.length; i++) {
            indx = i*8;
            lngs[i] = toLong(data[indx],
                             data[indx+1],
                             data[indx+2],
                             data[indx+3],
                             data[indx+4],
                             data[indx+5],
                             data[indx+6],
                             data[indx+7],
                             byteOrder);
        }
        return lngs;
    }

    /**
     * Turn byte array into an long array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param dest array in which to write converted bytes
     * @param off offset into dest array
     * @throws EvioException if data is null or wrong size; dest is null, wrong size, or off < 0
     */
    public static void toLongArray(byte[] data, ByteOrder byteOrder, long[] dest, int off)
            throws EvioException {

        if (data == null || data.length % 8 != 0 ||
            dest == null || 8*dest.length < data.length+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        for (int i = 0; i < data.length-7; i+=8) {
            dest[i/8+off] = toLong(data[i],
                                   data[i + 1],
                                   data[i + 2],
                                   data[i + 3],
                                   data[i + 4],
                                   data[i + 5],
                                   data[i + 6],
                                   data[i + 7],
                                   byteOrder);
        }
    }

    // =========================

    /**
     * Turn section of byte array into a float.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param off offset into data array
     * @return float converted from byte array
     * @throws EvioException if data is null or wrong size, or off is negative
     */
    public static float toFloat(byte[] data, ByteOrder byteOrder, int off) throws EvioException {
        return Float.intBitsToFloat(toInt(data, byteOrder, off));
    }

    /**
     * Turn 4 bytes into a float.
     *
     * @param b1 1st byte
     * @param b2 2nd byte
     * @param b3 3rd byte
     * @param b4 4th byte
     * @param byteOrder if big endian, 1st byte is most significant &
     *                  4th least (big endian if null)
     * @return float converted from byte array
     */
    public static float toFloat(byte b1, byte b2, byte b3, byte b4, ByteOrder byteOrder) {
        return Float.intBitsToFloat(toInt(b1,b2,b3,b4, byteOrder));
    }

    /**
     * Turn byte array into a float array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @return float array converted from byte array
     * @throws EvioException if data is null or wrong size
     */
    public static float[] toFloatArray(byte[] data, ByteOrder byteOrder) throws EvioException {
        if (data == null || data.length % 4 != 0) {
            throw new EvioException("bad data arg");
        }

        int indx;
        float[] flts = new float[data.length / 4];
        for (int i = 0; i < flts.length; i++) {
            indx = i*4;
            flts[i] = toFloat(data[indx],
                              data[indx+1],
                              data[indx+2],
                              data[indx+3],
                              byteOrder
                             );
        }
        return flts;
    }

    /**
     * Turn byte array into an float array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param dest array in which to write converted bytes
     * @param off offset into dest array
     * @throws EvioException if data is null or wrong size; dest is null, wrong size, or off < 0
     */
    public static void toFloatArray(byte[] data, ByteOrder byteOrder, float[] dest, int off)
            throws EvioException {

        if (data == null || data.length % 4 != 0 ||
            dest == null || 4*dest.length < data.length+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        for (int i = 0; i < data.length-3; i+=4) {
            dest[i/4+off] = toFloat(data[i],
                                    data[i + 1],
                                    data[i + 2],
                                    data[i + 3],
                                    byteOrder);
        }
    }

    // =========================

    /**
     * Turn section of byte array into a double.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param off offset into data array
     * @return double converted from byte array
     * @throws EvioException if data is null or wrong size, or off is negative
     */
    public static double toDouble(byte[] data, ByteOrder byteOrder, int off) throws EvioException {
        return Double.longBitsToDouble(toLong(data, byteOrder, off));
    }

    /**
     * Turn 8 bytes into a double.
     *
     * @param b1 1st byte
     * @param b2 2nd byte
     * @param b3 3rd byte
     * @param b4 4th byte
     * @param b5 5th byte
     * @param b6 6th byte
     * @param b7 7th byte
     * @param b8 8th byte
     * @param byteOrder if big endian, 1st byte is most significant &
     *                  8th least (big endian if null)
     * @return double converted from byte array
     */
    public static double toDouble(byte b1, byte b2, byte b3, byte b4,
                                  byte b5, byte b6, byte b7, byte b8, ByteOrder byteOrder) {
        return Double.longBitsToDouble(toLong(b1,b2,b3,b4,b5,b6,b7,b8, byteOrder));
    }

    /**
     * Turn byte array into a double array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @return double array converted from byte array
     * @throws EvioException if data is null or wrong size
     */
    public static double[] toDoubleArray(byte[] data, ByteOrder byteOrder) throws EvioException {
        if (data == null || data.length % 8 != 0) {
            throw new EvioException("bad data arg");
        }

        int indx;
        double[] dbls = new double[data.length / 8];
        for (int i=0; i < dbls.length; i++) {
            indx = i*8;
            dbls[i] = toDouble(data[indx],
                               data[indx+1],
                               data[indx+2],
                               data[indx+3],
                               data[indx+4],
                               data[indx+5],
                               data[indx+6],
                               data[indx+7],
                               byteOrder);
        }
        return dbls;
    }

    /**
     * Turn byte array into an double array.
     *
     * @param data byte array to convert
     * @param byteOrder byte order of supplied bytes (big endian if null)
     * @param dest array in which to write converted bytes
     * @param off offset into dest array
     * @throws EvioException if data is null or wrong size; dest is null, wrong size, or off < 0
     */
    public static void toDoubleArray(byte[] data, ByteOrder byteOrder, double[] dest, int off)
            throws EvioException {

        if (data == null || data.length % 8 != 0 ||
            dest == null || 8*dest.length < data.length+off || off < 0) {
            throw new EvioException("bad data arg");
        }

        for (int i = 0; i < data.length-7; i+=8) {
            dest[i/8+off] = toDouble(data[i],
                                     data[i + 1],
                                     data[i + 2],
                                     data[i + 3],
                                     data[i + 4],
                                     data[i + 5],
                                     data[i + 6],
                                     data[i + 7],
                                     byteOrder);
        }
    }


    // =========================
    // Swapping Evio Data
    // =========================


    /**
     * This method swaps the byte order of an entire evio event or bank.
     * The byte order of the swapped buffer will be opposite to the byte order
     * of the the source buffer argument. If the swap is done in place, the
     * byte order of the source buffer will be switched upon completion and
     * the destPos arg will be set equal to the srcPos arg.
     * A ByteBuffer's current byte order can be found by calling
     * {@link java.nio.ByteBuffer#order()}.<p>
     *
     * The data to be swapped must <b>not</b> be in the evio file format (with
     * block headers). Data must only consist of bytes representing a single event/bank.
     * Position and limit of neither buffer is changed.
     *
     * @param srcBuffer  buffer containing event to swap.
     * @param destBuffer buffer in which to placed the swapped event.
     *                   If null, or identical to srcBuffer, the data is swapped in place.
     * @param srcPos     position in srcBuffer to start reading event
     * @param destPos    position in destBuffer to start writing swapped event
     *
     * @throws EvioException if srcBuffer arg is null;
     *                       if any buffer position is not zero
     */
    public static void swapEvent(ByteBuffer srcBuffer, ByteBuffer destBuffer,
                                 int srcPos, int destPos) throws EvioException {

        swapEvent(srcBuffer, destBuffer, srcPos, destPos, null);
    }


    /**
     * This method swaps the byte order of an entire evio event or bank.
     * The byte order of the swapped buffer will be opposite to the byte order
     * of the the source buffer argument. If the swap is done in place, the
     * byte order of the source buffer will be switched upon completion and
     * the destPos arg will be set equal to the srcPos arg.
     * A ByteBuffer's current byte order can be found by calling
     * {@link java.nio.ByteBuffer#order()}.<p>
     *
     * The data to be swapped must <b>not</b> be in the evio file format (with
     * block headers). Data must only consist of bytes representing a single event/bank.
     * Position and limit of neither buffer is changed.
     *
     * @param srcBuffer  buffer containing event to swap.
     * @param destBuffer buffer in which to placed the swapped event.
     *                   If null, or identical to srcBuffer, the data is swapped in place.
     * @param srcPos     position in srcBuffer to start reading event
     * @param destPos    position in destBuffer to start writing swapped event
     * @param nodeList   if not null, generate & store node objects here -
     *                   one for each swapped evio structure in destBuffer.
     *
     * @throws EvioException if srcBuffer arg is null;
     *                       if any buffer position is not zero
     */
    public static void swapEvent(ByteBuffer srcBuffer, ByteBuffer destBuffer,
                                 int srcPos, int destPos, List<EvioNode> nodeList)
            throws EvioException {

        swapEvent(srcBuffer, destBuffer, srcPos, destPos, true, nodeList);
    }


    /**
     * This method swaps the byte order of an entire evio event or bank.
     * The byte order of the swapped buffer will be opposite to the byte order
     * of the the source buffer argument. If the swap is done in place, the
     * byte order of the source buffer will be switched upon completion and
     * the destPos arg will be set equal to the srcPos arg.
     * A ByteBuffer's current byte order can be found by calling
     * {@link java.nio.ByteBuffer#order()}.<p>
     *
     * The data to be swapped must <b>not</b> be in the evio file format (with
     * block headers). Data must only consist of bytes representing a single event/bank.
     * Position and limit of neither buffer is changed.
     *
     * @param srcBuffer  buffer containing event to swap.
     * @param destBuffer buffer in which to placed the swapped event.
     *                   If null, or identical to srcBuffer, the data is swapped in place.
     * @param srcPos     position in srcBuffer to start reading event
     * @param destPos    position in destBuffer to start writing swapped event
     * @param swapData   if false, do NOT swap data, else swap data too
     * @param nodeList   if not null, generate & store node objects here -
     *                   one for each swapped evio structure in destBuffer.
     *
     * @throws EvioException if srcBuffer arg is null;
     *                       if any buffer position is not zero
     */
    public static void swapEvent(ByteBuffer srcBuffer, ByteBuffer destBuffer,
                                 int srcPos, int destPos, boolean swapData,
                                 List<EvioNode> nodeList)
            throws EvioException {

        if (srcBuffer == null) {
            throw new EvioException("Null event in parseEvent.");
        }

        // Find the destination byte order and if it is to be swapped in place
        boolean inPlace = false;
        ByteOrder srcOrder  = srcBuffer.order();
        ByteOrder destOrder = srcOrder == ByteOrder.BIG_ENDIAN ? ByteOrder.LITTLE_ENDIAN :
                                                                 ByteOrder.BIG_ENDIAN;

        if (destBuffer == null || srcBuffer == destBuffer) {
            // Do this so we can treat the source buffer as if it were a
            // completely different destination buffer with its own byte order.
            destBuffer = srcBuffer.duplicate();
            destPos = srcPos;
            inPlace = true;
        }
        destBuffer.order(destOrder);

        // Check position args
        if (srcPos < 0 || srcPos > srcBuffer.capacity() - 8) {
            throw new EvioException("bad value for srcPos arg");
        }

        if (destPos < 0 || destPos > destBuffer.capacity() - 8) {
            throw new EvioException("bad value for destPos arg");
        }

        // Create node
        EvioNode node = new EvioNode();

        // We know events are banks, so start by reading & swapping a bank header
        swapBankHeader(node, srcBuffer, destBuffer, srcPos, destPos);

        // Store all nodes here
        if (nodeList != null) {
            // Set a few special members for an event
            node.eventNode = node;
            node.scanned   = true;
            node.isEvent   = true;
            node.type = DataType.BANK.getValue();
            nodeList.add(node);
        }
//System.out.println("Created top node containing " + node.getDataTypeObj());

        // The event is an evio bank so recursively swap it as such
        swapStructure(node.getDataTypeObj(), srcBuffer, destBuffer,
                      srcPos + 8, destPos + 8, node.dataLen, inPlace, swapData, nodeList);

        if (inPlace) {
            srcBuffer.order(destOrder);
        }
    }


    /**
     * This method reads and swaps an evio bank header.
     * It can also return information about the bank.
     * Position and limit of neither buffer argument is changed.
     *
     * @param node       object in which to store data about the bank
     *                   in destBuffer after swap; may be null
     * @param srcBuffer  buffer containing bank header to be swapped
     * @param destBuffer buffer in which to place swapped bank header
     * @param srcPos     position in srcBuffer to start reading bank header
     * @param destPos    position in destBuffer to start writing swapped bank header
     *
     * @throws EvioException if srcBuffer is not properly formatted;
     *                       if destBuffer is too small to contain swapped data
     */
    static void swapBankHeader(EvioNode node, ByteBuffer srcBuffer,
                               ByteBuffer destBuffer, int srcPos, int destPos)
                throws EvioException {

        try {
            int dt, type, pad, tag, num, length;
            ByteOrder srcOrder = srcBuffer.order();

            if (node != null) node.pos = destPos;

            // Read & swap first bank header word
            length = srcBuffer.getInt(srcPos);
            destBuffer.putInt(destPos, length);
            srcPos  += 4;
            destPos += 4;
//System.out.println("  BH: len = " + length);

            // Read second bank header word
            if (srcOrder == ByteOrder.BIG_ENDIAN) {
                tag = srcBuffer.getShort(srcPos) & 0x0000ffff;
                srcPos += 2;

                dt = srcBuffer.get(srcPos++) & 0x000000ff;
                type = dt & 0x3f;
                pad  = dt >>> 6;
                // If only 7th bit set, that can only be the legacy tagsegment type
                // with no padding information - convert it properly.
                if (dt == 0x40) {
                    type = DataType.TAGSEGMENT.getValue();
                    pad = 0;
                }

                num = srcBuffer.get(srcPos++) & 0x000000ff;
            }
            else {
                num = srcBuffer.get(srcPos++) & 0x000000ff;

                dt = srcBuffer.get(srcPos++) & 0x000000ff;
                type = dt & 0x3f;
                pad  = dt >>> 6;
                if (dt == 0x40) {
                    type = DataType.TAGSEGMENT.getValue();
                    pad = 0;
                }

                tag = srcBuffer.getShort(srcPos) & 0x0000ffff;
                srcPos += 2;
            }
//System.out.println("  BH: type/tag/num/pad = " + type + "/" + tag + "/" + num + "/" + pad);

            // Swap second bank header word
            if (srcOrder == ByteOrder.BIG_ENDIAN) {
                destBuffer.put(destPos++, (byte)num);
                destBuffer.put(destPos++, (byte)dt);
                destBuffer.putShort(destPos, (short)tag);
                destPos += 2;
            }
            else {
                destBuffer.putShort(destPos, (short)tag);
                destPos += 2;
                destBuffer.put(destPos++, (byte)dt);
                destBuffer.put(destPos++, (byte)num);
            }

            if (node != null) {
                node.len = length;
                node.num = num;
                node.tag = tag;
                node.pad = pad;
                node.dataPos = destPos;
                node.dataLen = length - 1;
                node.dataType = type;
            }
        }
        catch (BufferOverflowException e) {
            throw new EvioException("destBuffer too small to hold swapped data");
        }
        catch (BufferUnderflowException e) {
            throw new EvioException("srcBuffer not evio format");
        }
    }


    /**
     * This method reads and swaps an evio segment header.
     * It can also return information about the segment.
     * Position and limit of neither buffer argument is changed.
     *
     * @param node       object in which to store data about the segment
     *                   in destBuffer after swap; may be null
     * @param srcBuffer  buffer containing segment header to be swapped
     * @param destBuffer buffer in which to place swapped segment header
     * @param srcPos     position in srcBuffer to start reading segment header
     * @param destPos    position in destBuffer to start writing swapped segment header
     *
     * @throws EvioException if srcBuffer is not properly formatted;
     *                       if destBuffer is too small to contain swapped data
     */
    static void swapSegmentHeader(EvioNode node, ByteBuffer srcBuffer,
                                  ByteBuffer destBuffer, int srcPos, int destPos)
                throws EvioException {

        try {
            int dt, type, pad, tag, len;
            ByteOrder srcOrder = srcBuffer.order();

            if (node != null) node.pos = destPos;

            // Read segment header word
            if (srcOrder == ByteOrder.BIG_ENDIAN) {
                tag = srcBuffer.get(srcPos++) & 0x000000ff;
                dt  = srcBuffer.get(srcPos++) & 0x000000ff;
                type = dt & 0x3f;
                pad  = dt >>> 6;
                // If only 7th bit set, that can only be the legacy tagsegment type
                // with no padding information - convert it properly.
                if (dt == 0x40) {
                    type = DataType.TAGSEGMENT.getValue();
                    pad = 0;
                }

                len = srcBuffer.getShort(srcPos) & 0x0000ffff;
                srcPos += 2;
            }
            else {
                len = srcBuffer.getShort(srcPos) & 0x0000ffff;
                srcPos += 2;

                dt = srcBuffer.get(srcPos++) & 0x000000ff;
                type = dt & 0x3f;
                pad  = dt >>> 6;
                if (dt == 0x40) {
                    type = DataType.TAGSEGMENT.getValue();
                    pad = 0;
                }

                tag = srcBuffer.get(srcPos++) & 0x000000ff;
            }
//System.out.println("found tag/num = " + tag + ", " + num);

            // Swap segment header word
            if (srcOrder == ByteOrder.BIG_ENDIAN) {
                destBuffer.putShort(destPos, (short)len);
                destPos += 2;
                destBuffer.put(destPos++, (byte)dt);
                destBuffer.put(destPos++, (byte)tag);
            }
            else {
                destBuffer.put(destPos++, (byte)tag);
                destBuffer.put(destPos++, (byte)((type & 0x3f) | (pad << 6)));
                destBuffer.putShort(destPos, (short)len);
                destPos += 2;
            }

            if (node != null) {
                node.len = len;
                node.num = 0;
                node.tag = tag;
                node.pad = pad;
                node.dataPos = destPos;
                node.dataLen = len;
                node.dataType = type;
            }
        }
        catch (BufferOverflowException e) {
            throw new EvioException("destBuffer too small to hold swapped data");
        }
        catch (BufferUnderflowException e) {
            throw new EvioException("srcBuffer not evio format");
        }
    }


    /**
     * This method reads and swaps an evio tagsegment header.
     * It can also return information about the tagsegment.
     * Position and limit of neither buffer argument is changed.
     *
     * @param node       object in which to store data about the tagsegment
     *                   in destBuffer after swap; may be null
     * @param srcBuffer  buffer containing tagsegment header to be swapped
     * @param destBuffer buffer in which to place swapped tagsegment header
     * @param srcPos     position in srcBuffer to start reading tagsegment header
     * @param destPos    position in destBuffer to start writing swapped tagsegment header
     *
     * @throws EvioException if srcBuffer is not properly formatted;
     *                       if destBuffer is too small to contain swapped data
     */
    static void swapTagSegmentHeader(EvioNode node, ByteBuffer srcBuffer,
                                     ByteBuffer destBuffer, int srcPos, int destPos)
                throws EvioException {

        try {
            int type, tag, len, temp;
            ByteOrder srcOrder = srcBuffer.order();

            if (node != null) node.pos = destPos;

            // Read segment header word
            if (srcOrder == ByteOrder.BIG_ENDIAN) {
                temp = srcBuffer.getShort(srcPos) & 0x0000ffff;
                srcPos += 2;

                tag  = temp >>> 4;
                type = temp & 0xf;

                len = srcBuffer.getShort(srcPos) & 0x0000ffff;
                srcPos += 2;
            }
            else {
                len = srcBuffer.getShort(srcPos) & 0x0000ffff;
                srcPos += 2;

                temp = srcBuffer.getShort(srcPos) & 0x0000ffff;
                srcPos += 2;

                tag  = temp >>> 4;
                type = temp & 0xf;
            }
//System.out.println("found tag/num = " + tag + ", " + num);

            // Swap segment header word
            if (srcOrder == ByteOrder.BIG_ENDIAN) {
                destBuffer.putShort(destPos, (short)len);
                destPos += 2;
                destBuffer.putShort(destPos, (short)temp);
                destPos += 2;
            }
            else {
                destBuffer.putShort(destPos, (short)temp);
                destPos += 2;
                destBuffer.putShort(destPos, (short)len);
                destPos += 2;
            }

            if (node != null) {
                node.len = len;
                node.num = 0;
                node.tag = tag;
                node.pad = 0;
                node.dataPos = destPos;
                node.dataLen = len;
                node.dataType = type;
            }
        }
        catch (BufferOverflowException e) {
            throw new EvioException("destBuffer too small to hold swapped data");
        }
        catch (BufferUnderflowException e) {
            throw new EvioException("srcBuffer not evio format");
        }
    }


    /**
     * This method swaps the data of an evio leaf structure. In other words the
     * structure being swapped does not contain evio structures.
     *
     * @param type       type of data being swapped
     * @param srcBuffer  buffer containing data to be swapped
     * @param destBuffer buffer in which to place swapped data
     * @param srcPos     position in srcBuffer to start reading data
     * @param destPos    position in destBuffer to start writing swapped data
     * @param len        length of data in 32 bit words
     * @param inPlace    if true, data is swapped in srcBuffer
     *
     * @throws EvioException if srcBuffer not in evio format;
     *                       if destBuffer too small;
     *                       if bad values for srcPos and/or destPos;
     */
    static void swapData(DataType type, ByteBuffer srcBuffer,
                                 ByteBuffer destBuffer, int srcPos, int destPos,
                                 int len, boolean inPlace)
                throws EvioException {

        // We end here
        int endPos = srcPos + 4*len;

        switch (type) {

            // 64 bit swap
            case LONG64:
            case ULONG64:
            case DOUBLE64:
                // When only swapping, no need to convert to double & back
                for (; srcPos < endPos; srcPos += 8, destPos += 8) {
                    destBuffer.putLong(destPos, srcBuffer.getLong(srcPos));
                }

                break;

            // 32 bit swap
            case INT32:
            case UINT32:
			case FLOAT32:
	            // When only swapping, no need to convert to float & back
                for (; srcPos < endPos; srcPos += 4, destPos += 4) {
                    destBuffer.putInt(destPos, srcBuffer.getInt(srcPos));
                }
				break;

            // 16 bit swap
			case SHORT16:
			case USHORT16:
                for (; srcPos < endPos; srcPos += 2, destPos += 2) {
                    destBuffer.putShort(destPos, srcBuffer.getShort(srcPos));
                }
				break;

            // 8 bit swap - no swap needed, but need to copy if destBuf != srcBuf
			case CHAR8:
			case UCHAR8:
            case CHARSTAR8:
                if (!inPlace) {
                    for (; srcPos < endPos; srcPos++, destPos++) {
                        destBuffer.put(destPos, srcBuffer.get(srcPos));
                    }
                }
                break;

            // new composite type
            case COMPOSITE:
                CompositeData.swapAll(srcBuffer, destBuffer, srcPos, destPos, len, inPlace);
                break;

            default:
        }
    }


    /**
     * Swap an evio structure. If it is a structure of structures,
     * such as a bank of banks, swap recursively.
     *
     * @param dataType   the type of evio data contained in structure to swap
     *                   (bank, segment, double, int, ...).
     * @param srcBuffer  buffer containing structure to swap.
     * @param destBuffer buffer in which to place the swapped structure.
     * @param srcPos     position in srcBuffer to start reading structure
     * @param destPos    position in destBuffer to start writing swapped structure
     * @param length     length of structure in 32-bit words
     * @param inPlace    if true, data is swapped in srcBuffer
     * @param swapData   if false, data is NOT swapped, else it is
     * @param nodeList   if not null, store all node objects here -
     *                   one for each swapped evio structure in destBuffer.
     *
     * @throws EvioException
     * @throws EvioException if srcBuffer not in evio format;
     *                       if destBuffer too small;
     *                       if bad values for srcPos and/or destPos;
     */
    static void swapStructure(DataType dataType, ByteBuffer srcBuffer,
                              ByteBuffer destBuffer, int srcPos, int destPos,
                              int length, boolean inPlace, boolean swapData,
                              List<EvioNode> nodeList)
             throws EvioException {

        // If not a structure of structures, swap the data and return - no more recursion.
        if (!dataType.isStructure()) {
            // swap raw data here
            if (swapData) swapData(dataType, srcBuffer, destBuffer, srcPos, destPos, length, inPlace);
//System.out.println("HIT END_OF_LINE");
            return;
        }

        int offset = 0, sPos = srcPos, dPos = destPos;
        EvioNode node = new EvioNode();

        EvioNode firstNode = null;
        if (nodeList != null) {
            firstNode = nodeList.get(0);
            node.scanned   = true;
            node.eventNode = firstNode;
        }

        // change 32-bit words to bytes
        length *= 4;

        // This structure contains banks
        switch (dataType) {
            case BANK:
            case ALSOBANK:
                // Swap all banks contained in this structure
                while (offset < length) {
//System.out.println("bank A: offset = " + offset + ", len = " + length);
                    swapBankHeader(node, srcBuffer, destBuffer, sPos, dPos);
//System.out.println("Create bank node for struct holding " + node.getDataTypeObj());

                    swapStructure(node.getDataTypeObj(), srcBuffer, destBuffer,
                                  sPos+8, dPos+8, node.dataLen, inPlace, swapData, nodeList);

                    // Position offset to start of next header
                    offset += 4 * (node.len + 1); // plus 1 for length word
                    sPos = srcPos  + offset;
                    dPos = destPos + offset;

                    // Store node objects
                    if (nodeList != null) {
                        node.type = DataType.BANK.getValue();
                        nodeList.add(node);
                        if (offset < length) {
                            node = new EvioNode();
                            node.scanned   = true;
                            node.eventNode = firstNode;
                        }
                    }
//System.out.println("bank B: offset = " + offset + ", len = " + length);
                }
                break;

            case SEGMENT:
            case ALSOSEGMENT:
                // extract all the segments from this bank.
                while (offset < length) {
//System.out.println("seg A: offset = " + offset + ", len = " + length);
                    swapSegmentHeader(node, srcBuffer, destBuffer, sPos, dPos);

//System.out.println("Create seg node for struct holding " + node.getDataTypeObj());
                    swapStructure(node.getDataTypeObj(), srcBuffer, destBuffer,
                                  sPos+4, dPos+4, node.dataLen, inPlace, swapData, nodeList);

                    offset += 4 * (node.len + 1);
                    sPos = srcPos  + offset;
                    dPos = destPos + offset;

                    // Store node objects
                    if (nodeList != null) {
                        node.type = DataType.SEGMENT.getValue();
                        nodeList.add(node);
                        if (offset < length) {
                            node = new EvioNode();
                            node.scanned   = true;
                            node.eventNode = firstNode;
                        }
                    }
//System.out.println("seg B: offset = " + offset + ", len = " + length);
                }

                break;

            case TAGSEGMENT:
         // case ALSOTAGSEGMENT:
                // extract all the tag segments from this bank.
                while (offset < length) {
//System.out.println("tagseg A: offset = " + offset + ", len = " + length);

                    swapTagSegmentHeader(node, srcBuffer, destBuffer, sPos, dPos);
//System.out.println("Create tagseg node for struct holding " + node.getDataTypeObj());

                    swapStructure(node.getDataTypeObj(), srcBuffer, destBuffer,
                                  sPos+4, dPos+4, node.dataLen, inPlace, swapData, nodeList);

                    offset += 4 * (node.len + 1);
                    sPos = srcPos  + offset;
                    dPos = destPos + offset;

                    if (nodeList != null) {
                        node.type = DataType.TAGSEGMENT.getValue();
                        nodeList.add(node);
                        if (offset < length) {
                            node = new EvioNode();
                            node.scanned   = true;
                            node.eventNode = firstNode;
                        }
                    }
//System.out.println("tagseg B: offset = " + offset + ", len = " + length);
                }

                break;
            default:
        }

    }




}
