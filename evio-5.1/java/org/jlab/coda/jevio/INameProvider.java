package org.jlab.coda.jevio;

/**
 * This interface must be implemented by dictionary readers. For example, a dictionary reader that parses standard CODA
 * dictionary plain text files, or a dictionary reader that processes the xml dictionary Maurizio uses for GEMC.
 * 
 * @author heddle
 * 
 */
public interface INameProvider {

    /**
     * A string used to indicate that no name can be determined.
     */
    public static String NO_NAME_STRING = "???";

	/**
	 * Returns the pretty name of some evio structure. Typically this is involve
	 * the use of the "tag" and, if present, "num" fields. There may also be a hierarchical
	 * dependence. 
	 * 
	 * @param structure the structure to find the name of.
	 * @return a descriptive name, e.g., "Edep".
	 */
	public String getName(BaseStructure structure);
}
