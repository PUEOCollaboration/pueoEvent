/****************************************************************************************
*  RawEvent.cc            Implementation of the PUEO Raw Event
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


#include "pueo/RawEvent.h" 

ClassImp(pueo::RawEvent);


#ifdef HAVE_PUEORAWDATA

pueo::RawEvent::RawEvent(const pueo_full_waveforms_t * raw)
  : eventNumber(raw->event), runNumber(raw->run)
{
  static_assert(PUEO_NCHAN == pueo::k::NUM_DIGITZED_CHANNELS);

  for (size_t i = 0; i < PUEO_NCHAN; i++)
  {
    std::copy(raw->wfs[i].data, raw->wfs[i].data+pueo::k::NUM_SAMPLES, data[i].begin());
  }
}


#endif

