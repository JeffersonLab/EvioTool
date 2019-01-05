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

import org.jlab.coda.et.system.StationLocal;
import org.jlab.coda.et.system.SystemCreate;

/**
 * This interface defines a method to use for custom event selection in a station.
 *
 * @author Carl Timmer
 */

public interface EtEventSelectable {

  /**
   * An event selection method must follow this form.
   * @param sys the ET system object
   * @param st the station using a user-defned selection method
   * @param ev event being evaluated for selection
   * @return {@code true} if event is accepted for entrance into station,
   *         else {@code false}.
   */
  public boolean select(SystemCreate sys, StationLocal st, EtEvent ev);

}
