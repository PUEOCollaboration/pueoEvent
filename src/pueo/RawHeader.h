/****************************************************************************************
*  pueo/RawHeader.h              The PUEO raw header 
*  
*  Cosmin Deaconu <cozzyd@kicp.uchicago.edu>    
*
*  (C) 2023-, The Payload for Ultrahigh Energy Observations (PUEO) Collaboration
* 
*  This file is part of pueoEvent, the ROOT I/O library for PUEO. 
* 
*  pueoEvent is free software: you can redistribute it and/or modify it under the
*  terms of the GNU General Public License as published by the Free Software
*  Foundation, either version 2 of the License, or (at your option) any later
*  version.
* 
*  Foobar is distributed in the hope that it will be useful, but WITHOUT ANY
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
*  A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
*  You should have received a copy of the GNU General Public License along with
*  Foobar. If not, see <https://www.gnu.org/licenses/
*
****************************************************************************************/ 
#ifndef PUEO_RAW_HEADER_H
#define PUEO_RAW_HEADER_H

#include "pueo/Conventions.h"
#include "TTimeStamp.h"
#include "Rtypes.h"
#include <cstdint>

#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif
//!  pueo::RawHeader -- The Raw PUEO Event Header
/*!
  The ROOT implementation of the raw PUEO event header
  \ingroup rootclasses
*/

namespace pueo
{
class RawHeader
{
public:
  RawHeader():readout_time(0,0),corrected_trigger_time(0,0){;} ///< Default constructor

#ifdef HAVE_PUEORAWDATA
  RawHeader(const pueo_full_waveforms_t * wfs);
#endif
  uint32_t   run = 0; ///< Run number, assigned on ground
  uint32_t   event_number = 0;

  /* An example:

            `last_pps`-------+       +-------- `event_time`
                             ↓       ↓      
     0--------125E6--------250E6-----o--375E6--------500E6--------625E6-------->
     |                               |                                    [TURF clock ticks]
  Run Start                       trigger!
     |                               |                                    [sec]
    111--------112----------113----------114----------115----------116---------->
                             ↑
          `event_second` ----+        

  */
  int32_t    event_second = 0; ///< Originally a uint32_t in libpueorawdata but int32_t is okay before year 2038
                               ///< Value can be wrong prior to post-processing.

  uint32_t   event_time = 0; ///< 32-bit free running TURF clock value very briefly after trigger.
  uint32_t   last_pps = 0; ///< TURF clock value at `event_second`. Value can be wrong prior to post-processing.
  uint32_t   llast_pps = 0; ///< ... one second prior, value can be wrong, won't be corrected
  uint32_t   deadtime_counter = 0; ///< @todo: no idea what these are, need help documenting
  uint32_t   deadtime_counter_last_pps = 0;
  uint32_t   deadtime_counter_llast_pps = 0;
  uint32_t   L2_mask = 0;
  uint32_t   trig_type = 0; ///< soft, pps, or ext trigger (see also pueo::trigger in Conventions.h)
  TTimeStamp readout_time; ///< Time since Unix epoch [sec]
                           ///< Tagged by the flight computer upon packet creation (ie event reception)

  TTimeStamp corrected_trigger_time; ///< correction via post-processing with pueo::Timemark
                                     ///< second: from (corrected) `event_second`
                                     ///< nanosecond: from `event_time` and (corrected) `last_pps`

  // uint32_t  flags = 0; @todo: not popuated
  // uint32_t  phi_trig_mask[k::NUM_POLS] = {0}; ///< 24-bit phi mask (from TURF) @todo: not populated
  // UChar_t   L1_octants[k::NUM_SURF_SLOTS] ={0}; @todo not populated

  // Trigger info
  // int isInPhiMask(int phi, pol::pol_t=pol::kVertical) const; ///< Returns 1 if given phi-pol is in mask

  ClassDefNV(RawHeader,4);
};
}


#endif
