package org.jlab.coda.jevio;

/**
 * This class maintains the single global NameProvider. There is no default name provider, so if the application doesn't
 * provide one, the static method <code>getName</code> will always return the constant <code>NO_NAME_STRING</code>,
 * which will be something like "NO_NAME_STRING".
 * 
 * Typically at start up an application will locate a dictionary file, use the <code>NameProviderFactory</code> to
 * create a <code>INameProvider</code>, and then call the static method @link #setProvider(INameProvider).
 * 
 * @author heddle
 * 
 */
public class NameProvider {

	/**
	 * The singleton
	 */
	private static INameProvider provider = null;

	/**
	 * Private constructor prevents anyone from making one of these.
	 */
	private NameProvider() {
	}

    /**
     * Returns true if the provider (dictionary) was set, else false.
     * @return <code>true</code> if the provider (dictionary) was set, else <code>false</code>.
     */
    public static boolean isProviderSet() {
        return provider != null;
    }

    /**
     * Sets the one global (singleton) name provider.
     *
     * @param aProvider the provider to use.
     */
    public static void setProvider(INameProvider aProvider) {
        provider = aProvider;
    }

	/**
	 * Returns the pretty name of some evio structure. Typically this is involve
	 * the use of the "tag" and, if present, "num" fields. There may also be a hierarchical
	 * dependence. 
	 * 
	 * @param structure the structure to find the name of.
	 * @return a descriptive name, e.g., "Edep".
	 */
	public static String getName(BaseStructure structure) {
		if (provider == null) {
			return INameProvider.NO_NAME_STRING;
		}
		return provider.getName(structure);
	}
}
