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

/**
 * Class to facilitate use of Evio XML dictionary entry data as a key or value in a hash table.
 * {@author timmer} 8/17/15.
 */
public class EvioDictionaryEntry {

    /** Tag value of evio container. */
    private final Integer tag;

    /** Num value of evio container which may be null if not given in xml entry. */
    private final Integer num;

    /** Type of data in evio container. */
    private DataType type;

    /** String used to identify format of data if CompositeData type. */
    private final String format;

    /** String used to describe data if CompositeData type. */
    private final String description;


    /**
     * Constructor.
     * @param tag  tag value of evio container.
     */
    EvioDictionaryEntry(Integer tag) {
        this(tag, null, null, null, null);
    }

    /**
     * Constructor.
     * @param tag  tag value of evio container.
     * @param num  num value of evio container.
     */
    EvioDictionaryEntry(Integer tag, Integer num) {
        this(tag, num, null, null, null);
    }

    /**
     * Constructor.
     * @param tag tag value of evio container.
     * @param num num value of evio container.
     * @param type type of data in evio container which may be (case-independent):
     *      unknown32 {@link DataType#UNKNOWN32},
     *      int32 {@link DataType#INT32},
     *      uint32 {@link DataType#UINT32},
     *      float32{@link DataType#FLOAT32},
     *      double64 {@link DataType#DOUBLE64},
     *      charstar8 {@link DataType#CHARSTAR8},
     *      char8 {@link DataType#CHAR8},
     *      uchar8 {@link DataType#UCHAR8},
     *      short16{@link DataType#SHORT16},
     *      ushort16 {@link DataType#USHORT16},
     *      long64 {@link DataType#LONG64},
     *      ulong64 {@link DataType#ULONG64},
     *      tagsegment {@link DataType#TAGSEGMENT},
     *      segment {@link DataType#SEGMENT}.
     *      alsosegment {@link DataType#ALSOSEGMENT},
     *      bank {@link DataType#BANK},
     *      alsobank {@link DataType#ALSOBANK}, or
     *      composite {@link DataType#COMPOSITE},
     */
    EvioDictionaryEntry(Integer tag, Integer num, String type) {
        this(tag, num, type, null, null);
    }

    /**
     * Constructor containing actual implementation.
     *
     * @param tag tag value of evio container.
     * @param num num value of evio container.
     * @param type type of data in evio container which may be (case-independent):
     *      unknown32 {@link DataType#UNKNOWN32},
     *      int32 {@link DataType#INT32},
     *      uint32 {@link DataType#UINT32},
     *      float32{@link DataType#FLOAT32},
     *      double64 {@link DataType#DOUBLE64},
     *      charstar8 {@link DataType#CHARSTAR8},
     *      char8 {@link DataType#CHAR8},
     *      uchar8 {@link DataType#UCHAR8},
     *      short16{@link DataType#SHORT16},
     *      ushort16 {@link DataType#USHORT16},
     *      long64 {@link DataType#LONG64},
     *      ulong64 {@link DataType#ULONG64},
     *      tagsegment {@link DataType#TAGSEGMENT},
     *      segment {@link DataType#SEGMENT}.
     *      alsosegment {@link DataType#ALSOSEGMENT},
     *      bank {@link DataType#BANK},
     *      alsobank {@link DataType#ALSOBANK}, or
     *      composite {@link DataType#COMPOSITE},
     * @param description description of CompositeData
     * @param format format of CompositeData
     */
    EvioDictionaryEntry(Integer tag, Integer num, String type, String description, String format) {
        this.tag = tag;
        this.num = num;
        this.format = format;
        this.description = description;
        this.type = null;
        if (type != null) {
            try {
                this.type = DataType.valueOf(type.toUpperCase());
            }
            catch (IllegalArgumentException e) {
            }
        }
    }

    /** {@inheritDoc} */
    public final int hashCode() {
        int hashTag = tag.hashCode();
        int hashNum = num.hashCode();

        return (hashTag + hashNum) * hashNum + hashTag;
    }

    /** {@inheritDoc} */
    public final boolean equals(Object other) {
        // Objects equal each other if tag & num are the same
        if (other instanceof EvioDictionaryEntry) {
            EvioDictionaryEntry otherPair = (EvioDictionaryEntry) other;
            boolean match = tag.equals(otherPair.tag);
            if (num == null) {
                if (otherPair.num != null) return false;
            }
            else {
                match &= num.equals(otherPair.num);
            }
            return match;
        }

        return false;
    }

    /** {@inheritDoc} */
    public final String toString() {
        StringBuilder builder = new StringBuilder(80);

        builder.append("(tag=");
        builder.append(tag);

        if (num != null) {
            builder.append(",num =");
            builder.append(num);
        }

        if (type != null) {
            builder.append(",type=");
            builder.append(type);
        }

        builder.append(")");

        return builder.toString();
    }

    /**
     * Get the tag value.
     * @return tag value.
     */
    public final Integer getTag() {return tag;}

    /**
     * Get the num value.
     * @return num value.
     */
    public final Integer getNum() {return num;}

    /**
     * Get the data's type.
     * @return data type object.
     */
    public final DataType getType() {return type;}

    /**
     * Get the CompositeData's format.
     * @return CompositeData's format.
     */
    public final String getFormat() {return format;}

    /**
     * Get the CompositeData's description.
     * @return CompositeData's description.
     */
    public final String getDescription() {return description;}
}
