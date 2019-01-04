package org.jlab.coda.jevio;

import java.io.File;

/**
 * A Factory class for generating an appropriate INameProvider. It makes its decisions based on the dictionary file that
 * it gets handed in its only public method, <code>createNameProvider</code>. For example, if it is given an xml file
 * (based on a ".xml" extension) it guesses that you want a reader that can handle the xml diction file developed for
 * GEMC.
 * 
 * @author heddle
 * 
 */
public class NameProviderFactory {

	/**
	 * Creates a NameProvider based on the file name.
	 * 
	 * @param file dictionary file.
	 * @return a NameProvider appropriate for the fileName.
	 */
	public static INameProvider createNameProvider(File file) {
        INameProvider provider = null;

        if (file != null) {
            // only handle xml files for now
            String filename =  file.getName();
            if (filename.endsWith(".xml")  ||
                filename.endsWith(".dict") ||
                filename.endsWith(".txt"))   {

                return new EvioXMLDictionary(file);
            }
            // TODO provide provider for standard CODA dictionaries
        }

        return provider;
    }


    /**
     * Creates a NameProvider based on the file name.
     *
     * @param xmlString xml dictionary string.
     * @return a NameProvider appropriate for the fileName.
     */
    public static INameProvider createNameProvider(String xmlString) {
        INameProvider provider = null;

        if (xmlString != null) {
            return new EvioXMLDictionary(xmlString);
            // TODO provide provider for standard CODA dictionaries
        }

        return provider;
    }

}
