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

/* example */
int
et_carls_function(et_sys_id id, et_stat_id stat_id, et_event *pe)
{ 
  int select[ET_STATION_SELECT_INTS];
  int control[ET_STATION_SELECT_INTS];
  
  et_station_getselectwords(id, stat_id, select);
  et_event_getcontrol(pe, control);

  /* access event control ints thru control[N] */
  /* access station selection ints thru select[N] */
  
  /* return 0 if it is NOT selected, 1 if it is */
  
  if(control[0] == 17)
  {
    return(1);
  }
  return(0);
}




/* for ET2ET station */
int
et2et_function(et_sys_id id, et_stat_id stat_id, et_event *pe)
{
  int control[ET_STATION_SELECT_INTS];
  int cue, cnt;

  et_event_getcontrol(pe, control);

  /* accept all control events (includes scalers) */
  if(control[0]==0) return(1);
  if(control[0]>15) return(1);

  /* accept random selection of other events */
  et_station_getcue(id,stat_id,&cue);
  et_station_getinputcount(id,stat_id,&cnt);
  if(cnt<cue)
  {
    return(1); /* accept */
  }
  else
  {
    return(0); /* ignore */
  }
}



/* for MON station */
int
et_mon_function(et_sys_id id, et_stat_id stat_id, et_event *pe)
{
  int control[ET_STATION_SELECT_INTS];
  int cue, cnt;

  et_event_getcontrol(pe, control);

  /* accept all control events (includes scalers) */
  if(control[0]==0) return(1);
  if(control[0]>15) return(1);

  /* accept random selection of other events */
  et_station_getcue(id,stat_id,&cue);
  et_station_getinputcount(id,stat_id,&cnt);
  if(cnt<cue)
  {
    return(1); /* accept */
  }
  else
  {
    return(0); /* ignore */
  }
}



/* for CED station */

int
et_ced_function(et_sys_id id, et_stat_id stat_id, et_event *pe)
{
  int control[ET_STATION_SELECT_INTS];
    
  et_event_getcontrol(pe, control);
  /*
  printf("et_ced_function: control[0]=%d\n",control[0]);
  */
  /* select events with type between 1 and 14 */
  if(control[0]>0&&control[0]<15)
  {
    return(1);
  }
  else
  {
    return(0);
  }
}

//--------------------------------------------------------------------------


/* for scaler station...accept only control events (includes scaler events) */
int
et_scaler_function(et_sys_id id, et_stat_id stat_id, et_event *pe)
{    
  int control[ET_STATION_SELECT_INTS];
    
  et_event_getcontrol(pe, control);
    
  if(control[0]==0)
  {
    return(1);
  }
  else
  {
    return(0);
  }
}


//--------------------------------------------------------------------------
