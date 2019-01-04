package org.jlab.coda.jevio;

import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * This is a utility class employing a singleton to obtain environment information, such as user name, home directory,
 * OS name, etc.
 * 
 * @author heddle
 * 
 */
public class Environment {

	/**
	 * Singleton instantiation.
	 */
	private static Environment environment;

	/**
	 * User's home directory.
	 */

	private String homeDirectory;

	/**
	 * Current working directory
	 */

	private String currentWorkingDirectory;

	/**
	 * User name
	 */

	private String userName;

	/**
	 * Operating System Name
	 */

	private String osName;

	/**
	 * Temporary Directory
	 */

	private String tempDirectory;

	/**
	 * Class path.
	 */
	private String classPath;

	/**
	 * The host name.
	 */
	private String hostName;

	/**
	 * Private constructor for the singleton.
	 */
	private Environment() {
		homeDirectory = getProperty("user.home");
		currentWorkingDirectory = getProperty("user.dir");
		userName = getProperty("user.name");
		osName = getProperty("os.name");
		tempDirectory = getProperty("java.io.tmpdir");
		classPath = getProperty("java.class.path");

		try {
			InetAddress addr = InetAddress.getLocalHost();

			// Get hostname
			hostName = addr.getHostName();
		}
		catch (UnknownHostException e) {
			hostName = "???";
		}

	}

	/**
	 * Accessor for the singleton.
	 * 
	 * @return the singleton <code>Environment</code> object.
	 */
	public static Environment getInstance() {
		if (environment == null) {
			environment = new Environment();
		}
		return environment;
	}

	/**
	 * Convenience routine for getting a system property.
	 * 
	 * @param keyName the key name of the property
	 * @return the property, or <code>null</null>.
	 */
	private String getProperty(String keyName) {
		try {
			return System.getProperty(keyName);
		}
		catch (Exception e) {
			return null;
		}
	}

	/**
	 * Get the current class path.
	 * 
	 * @return the current class path.
	 */
	public String getClassPath() {
		return classPath;
	}

	/**
	 * Get the current working directory.
	 * 
	 * @return the current WorkingDirectory.
	 */
	public String getCurrentWorkingDirectory() {
		return currentWorkingDirectory;
	}

	/**
	 * Get the user's home directory.
	 * 
	 * @return the home directory.
	 */
	public String getHomeDirectory() {
		return homeDirectory;
	}

	/**
	 * Get the operating system name.
	 * 
	 * @return the operating system name.
	 */
	public String getOsName() {
		return osName;
	}

	/**
	 * Get the temp directory.
	 * 
	 * @return the temp directory.
	 */
	public String getTempDirectory() {
		return tempDirectory;
	}

	/**
	 * Get the user name.
	 * 
	 * @return the userName.
	 */
	public String getUserName() {
		return userName;
	}

	/**
	 * Get the host name.
	 * 
	 * @return the host name.
	 */
	public String getHostName() {
		return hostName;
	}

	/**
	 * Convert to a string for diagnostics
	 * 
	 * @return The string
	 */

	@Override
	public String toString() {
		StringBuffer sb = new StringBuffer(1024);
		sb.append("Environment: \n");
		sb.append("Host Name: " + getHostName() + "\n");
		sb.append("User Name: " + getUserName() + "\n");
		sb.append("Temp Directory: " + getTempDirectory() + "\n");
		sb.append("OS Name: " + getOsName() + "\n");
		sb.append("Home Directory: " + getHomeDirectory() + "\n");
		sb.append("Current Working Directory: " + getCurrentWorkingDirectory() + "\n");
		sb.append("Class Path: " + getClassPath() + "\n");
		return sb.toString();
	}

	/**
	 * Main program used for testing only.
	 * 
	 * @param args
	 */
	public static void main(String[] args) {
		Environment env = Environment.getInstance();
		System.out.println(env.toString());
	}

}
