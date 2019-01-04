package org.jlab.coda.jevio;

import java.util.EventListener;

/**
 * In SAX like behavior, implementors will listen for structures encountered when an event is parsed.
 * 
 * @author heddle
 * 
 */
public interface IEvioListener extends EventListener {

	/**
	 * Called when a structure is read while parsing an event.
	 * 
	 * NOTE: the user should NOT modify the arguments.
	 * 
	 * @param topStructure the evio structure at the top of the search/parse
	 * @param structure the full structure, including header
	 */
	public void gotStructure(BaseStructure topStructure, IEvioStructure structure);
    
    /**
     * Starting to parse a new evio structure.
     * @param structure the evio structure in question.
     */
    public void startEventParse(BaseStructure structure);

    /**
     * Done parsing a new evio structure.
     * @param structure the evio structure in question.
     */
    public void endEventParse(BaseStructure structure);

}
