/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
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
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      Example routine for user's events selection
 *
 *----------------------------------------------------------------------------*/

#include "et.h"

int et_carls_function(et_sys_id id, et_stat_id stat_id, et_event *pe)
{ 
  int select[ET_STATION_SELECT_INTS],
      control[ET_STATION_SELECT_INTS];
  
  et_station_getselectwords(id, stat_id, select);
  et_event_getcontrol(pe, control);

  /* access event control ints thru control[N] */
  /* access station selection ints thru select[N] */
  
  /* return 0 if it is NOT selected, 1 if it is */
  
  if (control[0] == 17) {
    return 1;
  }
  return 0;
}
