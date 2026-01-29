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

ClassImp(pueo::RawHeader); 


int pueo::RawHeader::isInPhiMask(int phi, pueo::pol::pol_t pol) const
{
  return phiTrigMask[pol] & ( 1 << phi); 
}


pueo::RawHeader:: RawHeader(const pueo_full_waveforms_t * wfs)
  : run(wfs->run), 
  triggerTime(wfs->event_second),
  readoutTime(wfs->readout_time.utc_secs),
  readoutTimeNs(wfs->readout_time.utc_nsecs),
  eventNumber(wfs->event),
  trigTime(wfs->event_time),
  lastPPS(wfs->last_pps),
  lastLastPPS(wfs->llast_pps),
  deadTime(wfs->deadtime_counter),
  deadTimeLastPPS(wfs->deadtime_counter_last_pps),
  deadTimeLastLastPPS(wfs->deadtime_counter_llast_pps)

{
  if (wfs->soft_trigger) trigType |= pueo::trigger::kSoft;
  if (wfs->pps_trigger) trigType |= pueo::trigger::kPPS0;
  if (wfs->ext_trigger) trigType |= pueo::trigger::kExt;
  if (wfs->L2_mask) trigType |= pueo::trigger::kRFMI;

  //TODO convert L2 mask ,L1 mask as needed

}

