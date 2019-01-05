/*----------------------------------------------------------------------------*
 *  Copyright (c) 2001        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*/

package org.jlab.coda.et;

import java.lang.*;
import java.util.*;
import java.io.Serializable;
import org.jlab.coda.et.exception.*;

/**
 * This class specifies a configuration used to create a new station.
 *
 * @author Carl Timmer
 */

public class EtStationConfig implements Serializable {

    /**
     * Maximum number of events to store in this station's input list when the
     * station is nonblocking. When the input list has reached this limit,
     * additional events flowing through the ET system are passed to the next
     * station in line.
     */
    private int cue;

    /**
     * A value of N means selecting 1 out of every Nth event that meets this
     * station's selection criteria.
     */
    private int prescale;

    /**
     * Determine whether the station is part of a single group of stations
     * through which events flow in parallel or is not. A value of
     * {@link EtConstants#stationParallel} means it is a parallel station,
     * while a value of {@link EtConstants#stationSerial} means it is not.
     */
    private int flowMode;

    /**
     * The maximum number of users permitted to attach to this station. A value
     * of 0 means any number of users may attach. It may be set to
     * {@link EtConstants#stationUserMulti} or {@link EtConstants#stationUserSingle}
     * meaning unlimited users and a single user respectively.
     */
    private int userMode;

    /**
     * Determine the method of dealing with events obtained by a user through an
     * attachment, but whose process has ended before putting the events back
     * into the system. It may have the value {@link EtConstants#stationRestoreIn}
     * which places the events in the station's input list,
     * {@link EtConstants#stationRestoreOut} which places them in the output list,
     * or {@link EtConstants#stationRestoreGC} which places them in GRAND_CENTRAL
     * station.
     */
    private int restoreMode;

    /**
     * Determine whether all events will pass through the station (blocking) or
     * whether events should fill a cue with additional events bypassing the
     * station and going to the next (nonblocking). The permitted values are
     * {@link EtConstants#stationBlocking} and {@link EtConstants#stationNonBlocking}.
     */
    private int blockMode;

    /**
     * Determine the method of filtering events for selection into the station's
     * input list. A value of {@link EtConstants#stationSelectAll} applies no
     * filtering, {@link EtConstants#stationSelectMatch} applies a builtin
     * method for selection
     * ({@link org.jlab.coda.et.system.StationLocal#select(org.jlab.coda.et.system.SystemCreate, org.jlab.coda.et.system.StationLocal, EtEvent)}),
     * and {@link EtConstants#stationSelectUser} allows the user to define a selection
     * method. If the station is part of a single group of parallel stations, a
     * value of {@link EtConstants#stationSelectRRobin} distributes events among the
     * parallel stations using a round robin algorithm. Similarly, if the station
     * is part of a single group of parallel stations, a value of
     * {@link EtConstants#stationSelectEqualCue} distributes events among the
     * parallel stations using an algorithm to keep the cues equal to eachother.
     */
    private int selectMode;

    /**
     * An array of integers used in the builtin selection method and available
     * for any tasks the user desires. Its size is set by
     * {@link EtConstants#stationSelectInts}.
     */
    private int[] select;

    /**
     * Name of user-defined select function in a C library. It may be null. This
     * is only relevant to C language ET systems.
     */
    private String selectFunction;

    /**
     * Name of the C library containing the user-defined select function. It may
     * be null. This is only relevant to C language ET systems.
     */
    private String selectLibrary;

    /**
     * Name of the Java class containing the user-defined select method. It may
     * be null. This is only relevant to Java language ET systems.
     */
    private String selectClass;


    /**
     * Creates a new StationConfig object with default values for everything.
     * The default values are:
     * cue         = {@link EtConstants#defaultStationCue},
     * prescale    = {@link EtConstants#defaultStationPrescale},
     * flowMode    = {@link EtConstants#stationSerial},
     * userMode    = {@link EtConstants#stationUserMulti},
     * restoreMode = {@link EtConstants#stationRestoreOut},
     * blockMode   = {@link EtConstants#stationBlocking},
     * selectMode  = {@link EtConstants#stationSelectAll}, and
     * select      = filled with -1's
     */
    public EtStationConfig() {
        cue         = EtConstants.defaultStationCue;
        prescale    = EtConstants.defaultStationPrescale;
        flowMode    = EtConstants.stationSerial;
        userMode    = EtConstants.stationUserMulti;
        restoreMode = EtConstants.stationRestoreOut;
        blockMode   = EtConstants.stationBlocking;
        selectMode  = EtConstants.stationSelectAll;
        select      = new int[EtConstants.stationSelectInts];
        Arrays.fill(select, -1);
    }


    /**
     * Creates a new StationConfig object from an existing one.
     * @param config config to copy
     */
    public EtStationConfig(EtStationConfig config) {
        cue            = config.cue;
        prescale       = config.prescale;
        flowMode       = config.flowMode;
        userMode       = config.userMode;
        restoreMode    = config.restoreMode;
        blockMode      = config.blockMode;
        selectMode     = config.selectMode;
        select         = (int[]) config.select.clone();
        selectFunction = config.selectFunction;
        selectLibrary  = config.selectLibrary;
        selectClass    = config.selectClass;
    }


    /**
     * Checks to see if station configurations are compatible when adding
     * a parallel station to an existing group of parallel stations.
     *
     * @param group  station configuration of head of existing group of parallel stations
     * @param config configuration of station seeking to be added to the group
     */
    public static boolean compatibleParallelConfigs(EtStationConfig group, EtStationConfig config) {

        // both must be parallel stations
        if ((group.flowMode  != EtConstants.stationParallel) ||
            (config.flowMode != EtConstants.stationParallel))  {
            return false;
        }

        // if group is roundrobin or equal-cue, then config must be same
        if (((group.selectMode  == EtConstants.stationSelectRRobin) &&
             (config.selectMode != EtConstants.stationSelectRRobin)) ||
            ((group.selectMode  == EtConstants.stationSelectEqualCue) &&
             (config.selectMode != EtConstants.stationSelectEqualCue))) {
            return false;
        }

        // If group is roundrobin or equal-cue, then config's blocking & prescale must be same.
        // BlockMode is forced to be blocking and prescale is forced to be 1
        // in the method EtSystem.configCheck.
        if (((group.selectMode == EtConstants.stationSelectRRobin) ||
             (group.selectMode == EtConstants.stationSelectEqualCue)) &&
            ((group.blockMode  != config.blockMode) ||
             (group.prescale   != config.prescale))) {
            return false;
        }

        // if group is NOT roundrobin or equal-cue, then config's cannot be either
        if (((group.selectMode  != EtConstants.stationSelectRRobin) &&
             (group.selectMode  != EtConstants.stationSelectEqualCue)) &&
            ((config.selectMode == EtConstants.stationSelectRRobin) ||
             (config.selectMode == EtConstants.stationSelectEqualCue))) {
            return false;
        }

        return true;
    }


    // public gets


    /** Gets the cue size.
     * @return cue size */
    public int getCue() {return cue;}

    /** Gets the prescale value.
     * @return prescale value */
    public int getPrescale() {return prescale;}

    /** Gets the flow mode.
     * @return flow mode */
    public int getFlowMode() {return flowMode;}

    /** Gets the user mode.
     * @return user mode */
    public int getUserMode() {return userMode;}

    /** Gets the restore mode.
     * @return restore mode */
    public int getRestoreMode() {return restoreMode;}

    /** Gets the block mode.
     * @return block mode */
    public int getBlockMode() {return blockMode;}

    /** Gets the select mode.
     * @return select mode */
    public int getSelectMode() {return selectMode;}

    /** Gets a copy of the select integer array.
     * @return copy of select integer array */
    public int[] getSelect() {return select.clone();}

    /** Gets the user-defined select function name.
     * @return selection function name */
    public String getSelectFunction() {return selectFunction;}

    /** Gets the name of the library containing the user-defined select function.
     * @return library name */
    public String getSelectLibrary() {return selectLibrary;}

    /** Gets the name of the class containing the user-defined select method.
     * @return class name */
    public String getSelectClass() {return selectClass;}


  // public sets


    /**
     * Sets the station's cue size.
     *
     * @param q cue size
     * @throws EtException if there is a bad cue size value
     */
    public void setCue(int q) throws EtException {
        if (q < 1) {
            throw new EtException("bad cue value");
        }
        cue = q;
    }

    /**
     * Sets the station's prescale value.
     *
     * @param pre prescale value
     * @throws EtException if there is a bad prescale value
     */
    public void setPrescale(int pre) throws EtException {
        if (pre < 1) {
            throw new EtException("bad prescale value");
        }
        prescale = pre;
    }

    /**
     * Sets the station's flow mode value.
     *
     * @param mode flow mode
     * @throws EtException if there is a bad flow mode value
     */
    public void setFlowMode(int mode) throws EtException {
        if ((mode != EtConstants.stationSerial) &&
            (mode != EtConstants.stationParallel)) {
            throw new EtException("bad flow mode value");
        }
        flowMode = mode;
    }

    /**
     * Sets the station's user mode value.
     *
     * @param mode user mode
     * @throws EtException if there is a bad user mode value
     */
    public void setUserMode(int mode) throws EtException {
        if (mode < 0) {
            throw new EtException("bad user mode value");
        }
        userMode = mode;
    }

    /**
     * Sets the station's restore mode value.
     *
     * @param mode restore mode
     * @throws EtException if there is a bad restore mode value
     */
    public void setRestoreMode(int mode) throws EtException {
        if ((mode != EtConstants.stationRestoreOut) &&
            (mode != EtConstants.stationRestoreIn) &&
            (mode != EtConstants.stationRestoreGC) &&
            (mode != EtConstants.stationRestoreRedist)) {
            throw new EtException("bad restore mode value");
        }
        restoreMode = mode;
    }

    /**
     * Sets the station's block mode value.
     *
     * @param mode block mode
     * @throws EtException if there is a bad block mode value
     */
    public void setBlockMode(int mode) throws EtException {
        if ((mode != EtConstants.stationBlocking) &&
            (mode != EtConstants.stationNonBlocking)) {
            throw new EtException("bad block mode value");
        }
        blockMode = mode;
    }

    /**
     * Sets the station's select mode value.
     *
     * @param mode select mode
     * @throws EtException if there is a bad select mode value
     */
    public void setSelectMode(int mode) throws EtException {
        if ((mode != EtConstants.stationSelectAll) &&
            (mode != EtConstants.stationSelectMatch) &&
            (mode != EtConstants.stationSelectUser) &&
            (mode != EtConstants.stationSelectRRobin) &&
            (mode != EtConstants.stationSelectEqualCue)) {
            throw new EtException("bad select mode value");
        }
        selectMode = mode;
  }

    /**
     * Sets the station's select integer array.
     *
     * @param sel select integer array
     * @throws EtException if there are the wrong number of elements in the array
     */
    public void setSelect(int[] sel) throws EtException {
        if (sel.length != EtConstants.stationSelectInts) {
            throw new EtException("wrong number of elements in select array");
        }
        select = (int[]) sel.clone();
    }

    /**
     * Sets the station's user-defined select function.
     *
     * @param func name of the user-defined select function
     * @throws EtException if argument is too long
     */
    public void setSelectFunction(String func) throws EtException {
        if (func.length() > EtConstants.functionNameLenMax) {
            throw new EtException("arg is too long (> " + EtConstants.functionNameLenMax + " chars)");
        }
        selectFunction = func;
    }

    /**
     * Sets the library containing the user-defined select function.
     *
     * @param lib name of the library containing the user-defined select function
     * @throws EtException if argument is too long
     */
    public void setSelectLibrary(String lib) throws EtException {
        if (lib.length() > EtConstants.fileNameLenMax) {
            throw new EtException("arg is too long (> " + EtConstants.fileNameLenMax + " chars)");
        }
        selectLibrary = lib;
    }

    /**
     * Sets the class containing the user-defined select method.
     *
     * @param sClass name of the class containing the user-defined select method
     * @throws EtException if argument is too long
     */
    public void setSelectClass(String sClass) throws EtException {
        if (sClass.length() > EtConstants.fileNameLenMax) {
            throw new EtException("arg is too long (> " + EtConstants.fileNameLenMax + " chars)");
        }
        selectClass = sClass;
    }
}
