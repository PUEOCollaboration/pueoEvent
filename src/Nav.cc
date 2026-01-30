/****************************************************************************************
*  GeomTool.cc            Implementation of the PUEO Navigation classes
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



#include "pueo/Nav.h" 

ClassImp(pueo::nav::Position); 
ClassImp(pueo::nav::Attitude); 
ClassImp(pueo::nav::Sat); 
ClassImp(pueo::nav::Sats); 

#ifdef HAVE_PUEORAWDATA

pueo::nav::Attitude::Attitude(const pueo_nav_att *att)
  : source(att->source),
  realTime(att->gps_time.utc_secs),
  nSats(att->nsats),
  readoutTime(att->gps_time.utc_secs),
  readoutTimeNsecs(att->gps_time.utc_nsecs),
  latitude(att->lat),
  longitude(att->lon),
  altitude(att->alt),
  heading(att->heading),
  pitch(att->pitch),
  roll(att->roll),
  headingSigma(att->heading_sigma),
  pitchSigma(att->pitch_sigma),
  rollSigma(att->roll_sigma),
  vdop(att->vdop),
  hdop(att->hdop),
  flag(att->flags),
  temperature(att->temperature)
{
  for (size_t i = 0; i < antennaCurrents.size(); i++) antennaCurrents[i] = att->antenna_currents[i];
}


#endif
