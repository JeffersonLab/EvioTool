package org.jlab.coda.jevio;

public interface IEvioProgressListener {

	/**
	 * Something is listenening for progress, e.g. the number of events that has been written to XML.
	 * @param num the current number,
	 * @param total the total number, i.e, we have completed num out of total.
	 */
	public void completed(int num, int total);
}
