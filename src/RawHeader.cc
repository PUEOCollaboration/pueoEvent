/****************************************************************************************
*  RawHeader.cc            Implementation of the PUEO Raw Header
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

#include "pueo/RawHeader.h" 

// int pueo::RawHeader::isInPhiMask(int phi, pueo::pol::pol_t pol) const
// {
//   return phi_trig_mask[pol] & ( 1 << phi); 
// }

#ifdef HAVE_PUEORAWDATA
pueo::RawHeader:: RawHeader(const pueo_full_waveforms_t * wfs):
  run(wfs->run), 
  event_number(wfs->event),
  event_second(wfs->event_second),
  event_time(wfs->event_time),
  last_pps(wfs->last_pps),
  llast_pps(wfs->llast_pps),
  deadtime_counter(wfs->deadtime_counter),
  deadtime_counter_last_pps(wfs->deadtime_counter_last_pps),
  deadtime_counter_llast_pps(wfs->deadtime_counter_llast_pps),
  L2_mask(wfs->L2_mask),
  readout_time((time_t)wfs->readout_time.utc_secs, wfs->readout_time.utc_nsecs),
  corrected_trigger_time(0,0) // cannot know this without post-processing, so initialize to year 1970
{
  if (wfs->soft_trigger) trig_type |= pueo::trigger::kSoft;
  if (wfs->pps_trigger) trig_type |= pueo::trigger::kPPS0;
  if (wfs->ext_trigger) trig_type |= pueo::trigger::kExt;
  if (wfs->L2_mask) trig_type |= pueo::trigger::kRFMI;

  //TODO convert L2 mask ,L1 mask as needed
}
#endif
