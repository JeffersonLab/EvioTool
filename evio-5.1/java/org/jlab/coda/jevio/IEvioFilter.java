package org.jlab.coda.jevio;

/**
 * This interface allows applications to create filters so that they only recieve certain structures
 * when events are being processed. Below is a filter that accepts any structure that has tag = 400.
 * <pre>
 * IEvioFilter myFilter = new IEvioFilter() {
 *     public boolean accept(StructureType structureType, IEvioStructure structure) {
 *         return (structure.getHeader().getTag() == 400);
 *     }
 * };
 * EventParser.getInstance().setEvioFilter(myFilter);
 * </pre>
 * @author heddle
 *
 */
public interface IEvioFilter {

	/**
     * Accept or reject the given structure.
     *
	 * @param structureType the structure type, a <code>StructureType</code> enum, of the structure
	 *                      that was just found, e.g., <code>StructureType.BANK</code>.
	 * @param structure the structure that was just found. From its header the tag, num, length,
     *                  and data type are available. The application can filter based on those
     *                  quantities or on the data itself.
	 * @return <code>true</code> if the structure passes the filter and should be given to the listeners.
	 * @see StructureType
	 */
	public boolean accept(StructureType structureType, IEvioStructure structure);
}
