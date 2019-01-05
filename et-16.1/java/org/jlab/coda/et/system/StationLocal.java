/*----------------------------------------------------------------------------*
 *  Copyright (c) 2001        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12H        *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*/

package org.jlab.coda.et.system;

import java.lang.*;
import java.util.*;
import java.util.concurrent.locks.ReentrantLock;

import org.jlab.coda.et.exception.*;
import org.jlab.coda.et.*;

/**
 * This class defines a station for ET system use.
 *
 * @author Carl Timmer
 */

public class StationLocal extends Thread implements EtEventSelectable {

    /** ET system object. */
    private SystemCreate sys;

    /** Unique id number. */
    private int id;

    /** Unique station name. */
    private String name;

    /** Station configuration object. */
    private EtStationConfig config;

    /** Station status. It may have the values {@link org.jlab.coda.et.EtConstants#stationUnused},
     *  {@link org.jlab.coda.et.EtConstants#stationCreating}, {@link org.jlab.coda.et.EtConstants#stationIdle}, and
     *  {@link org.jlab.coda.et.EtConstants#stationActive}. */
    private volatile int status;

    /** Flag telling this station to kill the conductor thread */
    private volatile boolean killConductor;

    /** Flag telling if this station was the last to receive an event
     *  when using the round-robin selection method for a parallel group
     *  of stations. */
    private volatile boolean wasLast;

    /** Lock used to stop insertion/removal of stations when events
     *  are being transferred by the conductor thread and vice versa. */
    private final ReentrantLock stopTransferLock;

    /** If this station is the first in a linked list of parallel stations,
     *  this list contains all the parallel stations in that group.
     *  It's protected by stopTransferLock & {@link SystemCreate#systemLock}. */
    private ArrayList<StationLocal> parallelStations;

    /** Input list of events. */
    private EventList inputList;
    
    /** Output list of events. */
    private EventList outputList;

    /** Set of attachments to this station. */
    private HashSet<AttachmentLocal> attachments;

    /** Predefined event selection method used when the station's select mode
     *  is {@link org.jlab.coda.et.EtConstants#stationSelectMatch}. */
    private EtEventSelectable selector;

    /**
     * Creates a new StationLocal object.
     *
     * @param sys ET system object
     * @param name station name
     * @param config station configuration
     * @param id unique station id number
     * @throws EtException
     *     if the station cannot load the selectClass
     */
    public StationLocal(SystemCreate sys, String name, EtStationConfig config, int id)
            throws EtException {

        this.id          = id;
        this.sys         = sys;
        this.name        = name;
        this.config      = new EtStationConfig(config);
        status           = EtConstants.stationUnused;
        parallelStations = new ArrayList<StationLocal>(20);
        stopTransferLock = new ReentrantLock();

        inputList  = new EventList(sys.getConfig().getNumEvents());
        outputList = new EventList(sys.getConfig().getNumEvents());

        // attachments
        attachments = new HashSet<AttachmentLocal>(EtConstants.attachmentsMax);

        // user event selection routine
        selector = this;
        if (config.getSelectMode() == EtConstants.stationSelectUser) {
            // instantiate object of proper class
            try {
                Object f = Class.forName(config.getSelectClass()).newInstance();
                selector = (EtEventSelectable) f;
            }
            catch (ClassNotFoundException ex) {
                throw new EtException("station cannot load select class " + config.getSelectClass());
            }
            catch (InstantiationException ex) {
                throw new EtException("station cannot instantiate class " + config.getSelectClass());
            }
            catch (IllegalAccessException ex) {
                throw new EtException("station cannot load class " + config.getSelectClass());
            }

            if (sys.getConfig().getDebug() >= EtConstants.debugInfo) {
                System.out.println(name + " loaded select class " + config.getSelectClass());
            }
        }
    }


    /**
     * Gets the station id number. Because this class extends Thread, calling this method
     * getId() would override the method from the Thread class, which is something
     * we do NOT want. So call this method getStationId() instead.
     * @return station id number
     */
    public int getStationId() { return id; }

    /**
     * Gets the station name. Because this class extends Thread, calling this method
     * getName() would override the method from the Thread class, which is something
     * we do NOT want. So call this method getStationName() instead.
     * @return station name
     */
    public String getStationName() { return name; }

    /**
     * Get object containing list of input events.
     * @return object containing list of input events
     */
    public EventList getInputList() { return inputList; }

    /**
     * Get object containing list of utput events.
     * @return object containing list of output events
     */
    public EventList getOutputList() { return outputList; }

    /**
     * Get lock object used to add and remove stations from the station linked list
     * while blocking the moving of events.
     * @return lock object
     */
    public ReentrantLock getStopTransferLock() { return stopTransferLock; }

    /**
     * Get the station configuration.
     * @return tation configuration
     */
    public EtStationConfig getConfig() { return config; }

    /**
     * Get the linked list of parallel stations.
     * @return linked list of parallel stations
     */
    public ArrayList<StationLocal> getParallelStations() { return parallelStations; }

    /**
     * Get the station status which may be one of the following values: {@link org.jlab.coda.et.EtConstants#stationUnused },
     * {@link org.jlab.coda.et.EtConstants#stationCreating}, {@link org.jlab.coda.et.EtConstants#stationIdle}, or  {@link org.jlab.coda.et.EtConstants#stationActive}.
     * @return station status
     */
    public int getStatus() { return status; }

    /**
     * Set the station status which may be one of the following values: {@link org.jlab.coda.et.EtConstants#stationUnused },
     * {@link org.jlab.coda.et.EtConstants#stationCreating}, {@link org.jlab.coda.et.EtConstants#stationIdle}, or {@link org.jlab.coda.et.EtConstants#stationActive}.
     * Since the user does not ever call this method, forget any argument checks.
     * @param status station status
     */
    public void setStatus(int status) { this.status = status; }

    /**
     * Get the set of attachments to this station.
     * @return set of attachments to this station
     */
    public HashSet<AttachmentLocal> getAttachments() { return attachments; }

    /**
     * Is the conductor thread scheduled to be terminated?
     * @return <code>true</code> if conductor thread scheduled to be terminated, else <code>false</code>
     */
    public boolean isKillConductor() { return killConductor; }

    /** Schedule conductor thread to be terminated. */
    public void killConductor() { killConductor = true; }


    /**
     * Method to dynamically set a station's blocking mode.
     * @param mode blocking mode value
     */
    void setBlockMode(int mode) {
        if (config.getBlockMode() == mode) return;
        synchronized(sys.getStationLock()) {
            synchronized(inputList) {
                try {
                    config.setBlockMode(mode);
                }
                catch (EtException e) { /* should not happen. */  }
            }
        }
    }


    /**
     * Method to dynamically set a station's cue.
     * @param cue cue value
     */
    void setCue(int cue) {
        if (config.getCue() == cue) return;
        synchronized(sys.getStationLock()) {
            synchronized(inputList) {
                try {
                    config.setCue(cue);
                }
                catch (EtException e) { /* should not happen. */  }
            }
        }
    }


    /**
     * Method to dynamically set a station's prescale.
     * @param prescale prescale value
     */
    void setPrescale(int prescale) {
        if (config.getPrescale() == prescale) return;
        synchronized(sys.getStationLock()) {
            synchronized(inputList) {
                try {
                    config.setPrescale(prescale);
                }
                catch (EtException e) { /* should not happen. */  }
            }
        }
    }


    /**
     * Method to dynamically set a station's select integers.
     * @param select array of selection integers
     */
    void setSelectWords(int[] select) {
        if (config.getSelect() == select) return;
        synchronized(sys.getStationLock()) {
            synchronized(inputList) {
                try {
                    config.setSelect(select.clone());
                }
                catch (EtException e) { /* should not happen. */  }
            }
        }
    }


    /**
     * Method to dynamically set a station's user mode.
     * @param mode user mode value
     */
    void setUserMode(int mode) {
        if (config.getUserMode() == mode) return;
        synchronized(sys.getStationLock()) {
            try {
                config.setUserMode(mode);
            }
            catch (EtException e) { /* should not happen. */  }
        }
    }


    /**
     * Method to dynamically set a station's restore mode.
     * @param mode restore mode value
     */
    void setRestoreMode(int mode) {
        if (config.getRestoreMode() == mode) return;
        synchronized(sys.getStationLock()) {
            try {
                config.setRestoreMode(mode);
            }
            catch (EtException e) { /* should not happen. */  }
        }
    }


    /**
     * When selectMode equals {@link org.jlab.coda.et.EtConstants#stationSelectMatch}, this
     * becomes the station's selection method.
     * True is returned
     *
     * @param sys ET system object
     * @param stat station object
     * @param ev event object being evaluated
     * @see org.jlab.coda.et.EtEventSelectable
     */
    public boolean select(SystemCreate sys, StationLocal stat, EtEvent ev) {
        boolean result = false;
        int[] select  = stat.config.getSelect();
        int[] control = ev.getControl();

        for (int i=0; i < EtConstants.stationSelectInts ; i++) {
            if (i%2 == 0) {
                result = result || ((select[i] != -1) &&
                        (select[i] == control[i]));
            }
            else {
                result = result || ((select[i] != -1) &&
                        ((select[i] & control[i]) != 0));
            }
        }
        return result;
    }


    /**
     * Shell's method of sorting from "Numerical Recipes" slightly modified.
     * It is assumed that a and b have indexes from 1 to n. Since the input
     * arrays will start at 0 index, put nonsense in the first element.
     *
     * @param n number of array elements to be sorted
     * @param a array to be sorted
     * @param b array to be sorted in the same manner that array a is sorted
     */
    private void shellSort(int n, int[] a, int[] b) {
        int i, j, inc, v, w;
        inc = 1;
        do {
            inc *= 3;
            inc++;
        } while (inc <= n);

        do {
            inc /= 3;
            for (i = inc + 1; i <= n; i++) {
                v = a[i];
                w = b[i];
                j = i;
                while (a[j - inc] > v) {
                    a[j] = a[j - inc];
                    b[j] = b[j - inc];
                    j -= inc;
                    if (j <= inc) break;
                }
                a[j] = v;
                b[j] = w;
            }
        } while (inc > 1);
    }

    
    /**
     * Method to implement thread conducting events between stations. This
     * conductor places all events that go into a single station into one list
     * then writes it. It looks downstream, one station at a time, and repeats
     * the process.
     * It optimizes for cases in which the next station is GRAND_CENTRAL or one
     * which takes all events. In those cases, it dumps everything in that
     * station's input list without bothering to sort or filter it.
     */
    public void run() {
        int count, prescale, available, getListSize, position;
        long listTry;
        EtEventImpl ev;
        boolean writeAll, parallelIsActive, rrobinOrEqualcue;
        StationLocal currentStat, stat, firstActive, startStation;
        List<EtEventImpl> subList;
        ListIterator statIterator, pIterator = null;

        // inputList of next station
        EventList inList;
        // events read from station's outputList
        ArrayList<EtEventImpl> getList = new ArrayList<EtEventImpl>(sys.getConfig().getNumEvents());
        // events to be put into the next station's inputList
        ArrayList<EtEventImpl> putList = new ArrayList<EtEventImpl>(sys.getConfig().getNumEvents());

        // store some constants in stack variables for greater speed
        final int idle = EtConstants.stationIdle;
        final int active = EtConstants.stationActive;
        final int blocking = EtConstants.stationBlocking;
        final int nonBlocking = EtConstants.stationNonBlocking;
        final int selectAll = EtConstants.stationSelectAll;
        final int parallel = EtConstants.stationParallel;

        if (name.equals("GRAND_CENTRAL")) {
            status = active;
        }
        else {
            status = idle;
        }

        while (true) {
            // wait for events
            synchronized (outputList) {
                while (outputList.getEvents().size() < 1) {
                    try {
                        outputList.wait();
                    }
                    catch (InterruptedException ex) {
                    }
                    if (killConductor) {
                        return;
                    }
                }
            }

            // grab all events in station's outputList
            outputList.get(getList);

            // reinit items
            writeAll = false;

            // allow no change to linked list of created stations
            stopTransferLock.lock();

            // find next station in main linked list
            position = sys.getStations().indexOf(this);
            // If we're a parallel station which is NOT the head of its group,
            // find our position in the main linked list
            if (position < 0) {
                position = 1;
                for (ListIterator i = sys.getStations().listIterator(1); i.hasNext();) {
                    stat = (StationLocal) i.next();
                    if (stat.config.getFlowMode() == parallel) {
                        // we've found the group of parallel stations we belong to & our position
                        if (stat.parallelStations.indexOf(this) > -1) {
                            break;
                        }
                    }
                    position++;
                }
            }

            statIterator = sys.getStations().listIterator(position + 1);
            if (statIterator.hasNext()) {
                currentStat = (StationLocal) statIterator.next();
            }
            else {
                // the next station is GrandCentral, put everything in it
                currentStat = sys.getStations().get(0);
                inList = currentStat.inputList;
                synchronized (inList) {
                    inList.putInLow(getList);
                    getList.clear();
                    inList.notifyAll();
                }
                stopTransferLock.unlock();
                continue;
            }

            inList = currentStat.inputList;

            while (getList.size() > 0) {
                parallelIsActive = false;
                rrobinOrEqualcue = false;
                startStation = null;
                firstActive = null;

                // if this is a parallel station ...
                if (currentStat.config.getFlowMode() == EtConstants.stationParallel) {
                    // Are any of the parallel stations active or can we skip the bunch?
                    pIterator = currentStat.parallelStations.listIterator();
                    while (pIterator.hasNext()) {
                        stat = (StationLocal) pIterator.next();
                        if (stat.status == EtConstants.stationActive) {
                            parallelIsActive = true;
                            firstActive = stat;
                            break;
                        }
                    }
                    // At this point pIterator will give the station after firstActive
                    // with the following next().

                    // Which algorithm are we using?
                    if (parallelIsActive &&
                            ((currentStat.config.getSelectMode() == EtConstants.stationSelectRRobin) ||
                                    (currentStat.config.getSelectMode() == EtConstants.stationSelectEqualCue))) {
                        rrobinOrEqualcue = true;
                    }
                }

                // if not rrobin/equalcue & station(s) is(are) active ...
                if (!rrobinOrEqualcue &&
                        (parallelIsActive || (currentStat.status == EtConstants.stationActive))) {

                    if (currentStat.config.getFlowMode() == EtConstants.stationParallel) {
                        // Skip to first active parallel station
                        currentStat = firstActive;
                        inList = currentStat.inputList;
                    }

                    // Loop through all the active parallel stations if necessary.
                    parallelDo:
                    do {
                        // allow no exterior change to inputList
                        synchronized (inList) {
                            // if GrandCentral, put everything into it ...
                            if (currentStat.id == 0) {
                                writeAll = true;
                            }

                            // all events, blocking
                            else if ((currentStat.config.getSelectMode() == selectAll) &&
                                    (currentStat.config.getBlockMode() == blocking)) {

                                // if prescale=1, dump everything into station
                                getListSize = getList.size();
                                if (currentStat.config.getPrescale() == 1) {
                                    writeAll = true;
                                }
                                else {
                                    prescale = currentStat.config.getPrescale();
                                    listTry = inList.getEventsTry();
                                    subList = getList.subList(0, (int) ((listTry + getListSize) / prescale - listTry / prescale));
                                    putList.addAll(subList);
                                    subList.clear();
                                }
                                inList.setEventsTry(inList.getEventsTry() + getListSize);
                            }

                            // all events, nonblocking
                            else if ((currentStat.config.getSelectMode() == selectAll) &&
                                    (currentStat.config.getBlockMode() == nonBlocking)) {
                                if (inList.getEvents().size() < currentStat.config.getCue()) {
                                    count = currentStat.config.getCue() - inList.getEvents().size();
                                    available = getList.size();
                                    subList = getList.subList(0, (count > available) ? available : count);
                                    putList.addAll(subList);
                                    subList.clear();
                                }
                            }

                            //  condition (user or match), blocking
                            else if (currentStat.config.getBlockMode() == blocking) {
                                prescale = currentStat.config.getPrescale();
                                for (ListIterator i = getList.listIterator(); i.hasNext();) {
                                    ev = (EtEventImpl) i.next();
                                    // apply selection method
                                    if (currentStat.selector.select(sys, currentStat, ev)) {
                                        // apply prescale
                                        listTry = inList.getEventsTry();
                                        inList.setEventsTry(listTry + 1);
                                        if ((listTry % prescale) == 0) {
                                            putList.add(ev);
                                            i.remove();
                                        }
                                    }
                                }
                            }

                            // condition (user or match) + nonblocking
                            else if (currentStat.config.getBlockMode() == nonBlocking) {
                                if (inList.getEvents().size() < currentStat.config.getCue()) {
                                    count = currentStat.config.getCue() - inList.getEvents().size();
                                    for (ListIterator i = getList.listIterator(); i.hasNext();) {
                                        ev = (EtEventImpl) i.next();
                                        // apply selection method
                                        if (currentStat.selector.select(sys, currentStat, ev)) {
                                            putList.add(ev);
                                            i.remove();
                                            if (--count < 1) {
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            // if items go in this station ...
                            if ((putList.size() > 0) || (writeAll)) {
                                // if grandcentral
                                if (currentStat.id == 0) {
                                    inList.putInLow(getList);
                                    getList.clear();
                                    writeAll = false;
                                }

                                else {
                                    if (writeAll) {
                                        inList.putAll(getList);
                                        getList.clear();
                                        writeAll = false;
                                    }
                                    else {
                                        inList.putAll(putList);
                                        putList.clear();
                                    }
                                }
                                // signal reader that new events are here
                                inList.notifyAll();
                            } // if items go in this station
                        } // end of inputList synchronization

                        // go to next active parallel station, if there is one
                        if (parallelIsActive) {
                            do {
                                if (pIterator.hasNext()) {
                                    stat = (StationLocal) pIterator.next();
                                    if (stat.status == EtConstants.stationActive) {
                                        currentStat = stat;
                                        inList = currentStat.inputList;
                                        break;
                                    }
                                }
                                else {
                                    break parallelDo;
                                }
                            } while (stat.status != EtConstants.stationActive);
                        }

                        // loop through active parallel stations if necessary
                    } while (parallelIsActive && (getList.size() > 0));

                } // if station active and not rrobin or equalcue

                // Implement the round-robin & equal-cue algorithms for dispensing
                // events to a single group of parallel stations.
                else if (rrobinOrEqualcue && parallelIsActive) {

                    int num, extra, lastEventIndex = 0, eventsAlreadyPut, numActiveStations = 0;
                    int index, numOfEvents, min, eventsToPut, eventsLeft;
                    int eventsPerStation, nextHigherCue, eventsDoledOut, stationsWithSameCue;
                    int[] numEvents;

                    if (currentStat.config.getSelectMode() == EtConstants.stationSelectRRobin) {
                        // Flag to start looking for station that receives first round-robin event
                        boolean startLooking = false;
                        stat = currentStat;
                        pIterator = currentStat.parallelStations.listIterator(1);

                        while (true) {
                            // for each active station ...
                            if (stat.status == EtConstants.stationActive) {
                                if (startLooking) {
                                    // This is the first active station after
                                    // the last station to receive an event.
                                    startStation = stat;
                                    startLooking = false;
                                }
                                numActiveStations++;
                            }

                            // Find last station to receive a round-robin event and start looking
                            // for the next active station to receive the first one.
                            if (stat.wasLast) {
                                stat.wasLast = false;
                                startLooking = true;
                            }

                            // find next station in the parallel linked list
                            if (pIterator.hasNext()) {
                                stat = (StationLocal) pIterator.next();
                            }
                            // else if we're at the end of the list ...
                            else {
                                // If we still haven't found a place to start the round-robin
                                // event dealing, make it the first active station.
                                if (startStation == null) {
                                    startStation = firstActive;
                                }
                                break;
                            }
                        }

                        // Find the number of events going into each station
                        num = getList.size() / numActiveStations;
                        // Find the number of events left over (not enough for another round). */
                        extra = getList.size() % numActiveStations;
                        eventsAlreadyPut = count = 0;
                        numEvents = new int[numActiveStations];

                        // Rearrange events so all those destined for a particular
                        // station are grouped together in the new array.
                        for (int i = 0; i < numActiveStations; i++) {
                            if (i < extra) {
                                numEvents[i] = num + 1;
                                if (i == (extra - 1)) {
                                    lastEventIndex = i;
                                }
                            }
                            else {
                                numEvents[i] = num;
                            }

                            if (extra == 0) {
                                lastEventIndex = numActiveStations - 1;
                            }

                            numOfEvents = numEvents[i];

                            index = i;
                            for (int j = 0; j < numOfEvents; j++) {
                                putList.add(getList.get(index));
                                index += numActiveStations;
                            }
                        }

                        // Place the first event with the station after the one which
                        // received the last event in the previous round.
                        stat = startStation;
                        inList = stat.inputList;
                        count = 0;

                        // set iterator to start with the station following startStation
                        index = currentStat.parallelStations.indexOf(startStation) + 1;
                        pIterator = currentStat.parallelStations.listIterator(index);

                        while (true) {
                            // For each active parallel station ...
                            if (stat.status == EtConstants.stationActive) {
                                // Mark station that got the last event
                                if (count == lastEventIndex) {
                                    stat.wasLast = true;
                                }

                                // Put "eventsToPut" number of events in the next active station
                                eventsToPut = numEvents[count++];

                                if (eventsToPut > 0) {
                                    synchronized (inList) {
                                        subList = putList.subList(eventsAlreadyPut, eventsAlreadyPut + eventsToPut);
                                        inList.putAll(subList);
                                        inList.setEventsTry(inList.getEventsTry() + eventsToPut);
                                        // signal reader that new events are here
                                        inList.notifyAll();
                                    }

                                    eventsAlreadyPut += eventsToPut;
                                }
                            }

                            // Find next active station
                            if (count >= numActiveStations) {
                                break;
                            }
                            else if (pIterator.hasNext()) {
                                stat = (StationLocal) pIterator.next();
                                inList = stat.inputList;
                            }
                            else {
                                // Go back to the first active parallel station
                                stat = firstActive;
                                inList = stat.inputList;
                                index = currentStat.parallelStations.indexOf(stat) + 1;
                                pIterator = currentStat.parallelStations.listIterator(index);
                            }
                        } // while (forever)

                        putList.clear();

                    } // if round-robin

                    // else if equal-cue algorithm ...
                    else {
                        eventsLeft = getList.size();
                        eventsDoledOut = 0;
                        eventsAlreadyPut = 0;
                        stationsWithSameCue = 0;

                        // Array that keeps track of original station order, and
                        // one that contains input list counts.
                        // Give 'em an extra element as the sorting routine
                        // assumes a starting index of 1.
                        int[] place = new int[sys.getConfig().getStationsMax() + 1];
                        int[] inListCount = new int[sys.getConfig().getStationsMax() + 1];
                        for (int i = 1; i <= sys.getConfig().getStationsMax(); i++) {
                            place[i] = i;
                        }

                        stat = firstActive;
                        while (true) {
                            // For each active station ...
                            if (stat.status == EtConstants.stationActive) {
                                // Find total # of events in stations' input lists.
                                // Store this information as it will change and we don't
                                // really want to grab all the input mutexes to make
                                // sure these values don't change.
                                inListCount[numActiveStations + 1] = stat.inputList.getEvents().size();

                                // Total number of active stations
                                numActiveStations++;
                            }

                            // find next station in the parallel linked list
                            if (pIterator.hasNext()) {
                                stat = (StationLocal) pIterator.next();
                            }
                            else {
                                break;
                            }
                        }

                        // Sort the input lists (cues) according to number of events. The "place"
                        // array remembers the place in the presorted array of input lists.
                        // Arrays to be sorted are assumed to have indexes from 1 to n,
                        // so the first element contains nonsense.
                        shellSort(numActiveStations, inListCount, place);

                        // To determine which stations get how many events:
                        // Take the lowest cues, add enough to make them equal
                        // to the next higher cue. Continue doing this until all
                        // are equal. Evenly divide any remaining events.
                        nextHigherCue = 0;
                        min = inListCount[1];
                        numEvents = new int[numActiveStations];

                        while (eventsDoledOut < eventsLeft) {
                            // Find how many cues have the lowest # of events in them
                            stationsWithSameCue = 0;
                            for (int i = 1; i <= numActiveStations; i++) {
                                // Does events in cue + events we've just given it = min?
                                if (min == inListCount[i] + numEvents[place[i] - 1]) {
                                    stationsWithSameCue++;
                                }
                                else {
                                    nextHigherCue = inListCount[i];
                                    break;
                                }
                            }

                            // If all stations have same # of events, or if there are not enough
                            // events to fill each lowest cue to level of the next higher cue,
                            // we spread available events between them all ...
                            if ((stationsWithSameCue == numActiveStations) ||
                                    ((eventsLeft - eventsDoledOut) < ((nextHigherCue - min) * stationsWithSameCue)))
                            {
                                eventsToPut = eventsLeft - eventsDoledOut;
                                eventsPerStation = eventsToPut / stationsWithSameCue;
                                extra = eventsToPut % stationsWithSameCue;
                                count = 0;
                                for (int i = 1; i <= stationsWithSameCue; i++) {
                                    if (count++ < extra) {
                                        numEvents[place[i] - 1] += eventsPerStation + 1;
                                    }
                                    else {
                                        numEvents[place[i] - 1] += eventsPerStation;
                                    }
                                }
                                break;
                            }
                            // Else, fill the lowest cues to the level of the next higher cue
                            // and repeat the cycle.
                            else {
                                eventsPerStation = nextHigherCue - min;
                                for (int i = 1; i <= stationsWithSameCue; i++) {
                                    numEvents[place[i] - 1] += eventsPerStation;
                                }
                                min = nextHigherCue;
                            }
                            eventsDoledOut += eventsPerStation * stationsWithSameCue;
                        }

                        stat = firstActive;
                        count = 0;
                        index = currentStat.parallelStations.indexOf(stat) + 1;
                        pIterator = currentStat.parallelStations.listIterator(index);

                        while (true) {
                            // for each active parallel station ...
                            if (stat.status == EtConstants.stationActive) {

                                if ((eventsToPut = numEvents[count++]) < 1) {
                                    // find next station in the parallel linked list
                                    if (pIterator.hasNext()) {
                                        stat = (StationLocal) pIterator.next();
                                        continue;
                                    }
                                    else {
                                        break;
                                    }
                                }

                                // Put "eventsToPut" number of events in the next active station
                                inList = stat.inputList;
                                synchronized (inList) {
                                    subList = getList.subList(eventsAlreadyPut, eventsAlreadyPut + eventsToPut);
                                    inList.putAll(subList);
                                    inList.setEventsTry(inList.getEventsTry() + eventsToPut);
                                    // signal reader that new events are here
                                    inList.notifyAll();
                                }

                                eventsAlreadyPut += eventsToPut;
                            }

                            // Find next station in the parallel linked list
                            if (pIterator.hasNext()) {
                                stat = (StationLocal) pIterator.next();
                            }
                            else {
                                break;
                            }
                        } // while(true)
                    } // else if equal-cue algorithm

                    getList.clear();

                } // Implement the round-robin & equal-cue algorithms

                if (currentStat.id == 0) {
                    break;
                }

                // find next station
                if (statIterator.hasNext()) {
                    currentStat = (StationLocal) statIterator.next();
                }
                else {
                    currentStat = sys.getStations().get(0);
                }
                inList = currentStat.inputList;

            } // while(getList.size() > 0), events left to put

            // stop transfer unlocked - now changes to stations linked list allowed
            stopTransferLock.unlock();

        } // while(true)

    } // run method


}
