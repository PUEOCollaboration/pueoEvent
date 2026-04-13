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
  // Default constructor:
  // Initialize all timestamps to a clearly garbage value (year 1970).
  // Otherwise ROOT will initialize them to the current time and that's not a good obvious garbage value
  RawHeader():
    corrected_readout_time((time_t)0,0),
    corrected_trigger_time((time_t)0,0)
  {;}

#ifdef HAVE_PUEORAWDATA
  RawHeader(const pueo_full_waveforms_t * wfs);
#endif
  uint32_t run = 0; ///< Run number
  uint32_t eventNumber = 0;

  // Definitions of last_pps, event_time, and event_second.
  // For details see DocDB doc-607-v5 (https://pueo.uchicago.edu/DocDB/cgi-bin/ShowDocument?docid=607)
  // note that example values are for illustration purposes only: [sec] is usually very large
  //
  //                `lastPPS`-------+       +-------- `trigTime`, aka `event_time` in libpueorawdata
  //                                ↓       ↓      
  //        0--------125E6--------250E6-----o--375E6--------500E6--------625E6-------->
  //        |                               |                                    [TURF clock ticks]
  //     Run Start                       trigger!
  //        |                               |                                    [sec]
  //       111--------112----------113----------114----------115----------116---------->
  //                                ↑
  //                                +----  `triggerTime`, aka `event_second` in libpueorawdata
  //

  [[deprecated("Not populated during raw data conversion nor corrected during post-processing. DO NOT USE.")]]
  uint32_t triggerTimeNs = 0;///< This was intended to store the subsecond portion of the trigger time.

  // Originally a uint32_t in libpueorawdata but int32_t is okay before year 2038.
  // int32_t for easier arithmetics.
  int32_t  triggerTime = 0;  ///< Number of seconds since Unix epoch (1970 Jan 1 00:00:00)
                             ///< This stores the ORIGINAL RAW DATA VALUE WHICH CAN BE WRONG prior to post-processing.
                             ///< The corrected value is stored in `corrected_trigger_time`.

  uint32_t trigTime = 0;     ///< 32-bit free running TURF clock value very briefly after trigger.

  uint32_t lastPPS = 0;      ///< TURF clock value at the nearest prior second (ie. at `triggerTime`).
                             ///< The variable stores the ORIGINAL RAW DATA VALUE WHICH CAN BE WRONG.
                             ///< The corrected value is stored in `corrected_pps`.

  double   corrected_pps = 0;///< corrected `lastPPS` from post-processing.

  double   clock_frequency = 0; ///< Obtained from post-processing: average pps difference b/w each second.
                                ///< Nominal value is about 125MHz.
                                ///< The subsecond is (trigTime - lastPPS) / clock_frequency
                                ///< The subsecond will be stored in `corrected_trigger_time`

  uint32_t lastLastPPS = 0;  ///< Same as `lastPPS` but one second prior; not corrected.

  uint32_t deadTime = 0;     ///< @todo: no idea what these are, need help documenting
  uint32_t deadTimeLastPPS = 0;
  uint32_t deadTimeLastLastPPS = 0;
  uint32_t L2Mask = 0;

  uint32_t trigType = 0;    ///< soft, pps, or ext trigger (see also pueo::trigger in Conventions.h)

  int32_t  readoutTime = 0;  ///< Number of seconds since Unix epoch (1970 Jan 1 00:00:00)
                             ///< Tagged by the flight computer upon packet creation (ie event reception)
                             ///< This stores the original raw-data value,
                             ///< which can drift and is accurate to only a few seconds.
                             ///< Prefer `corrected_trigger_time` for timing.

  uint32_t readoutTimeNs = 0;///< Subsecond portion of the readout time [nanosec]
                             ///< This stores the original raw-data value, likely not trust-worthy.

  TTimeStamp corrected_readout_time; ///< correction via post-processing (method TBD)

  TTimeStamp corrected_trigger_time; ///< correction via post-processing with pueo::Timemark
                                     ///< second: from (corrected) `triggerTime`
                                     ///< nanosecond: from `trigTime`, `corrected_last_pps` and `clock_frequency`

  uint32_t  flags = 0; /// @todo: not implemented yet (2026Apr2)
  uint8_t   L1_octants[k::NUM_SURF_SLOTS] ={0}; ///< @todo not implemented yet (2026Apr2)
  uint32_t  phiTrigMask[k::NUM_POLS] = {0}; ///< 24-bit phi mask (from TURF) 
                                            ///< @note: not quite the same as L2_mask
                                            ///< @todo: not implemented yet (2026Apr2)

  // Trigger info
  int isInPhiMask(int phi, pol::pol_t=pol::kVertical) const; ///< Returns 1 if given phi-pol is in mask

  ClassDefNV(RawHeader,4);
};
}


#endif
