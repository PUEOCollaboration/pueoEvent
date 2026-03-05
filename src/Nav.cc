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
ClassImp(pueo::nav::SunSensor);
ClassImp(pueo::nav::SunSensors);

#ifdef HAVE_PUEORAWDATA

pueo::nav::Attitude::Attitude(const pueo_nav_att *att)
  : source(att->source),
  realTime(att->gps_time.utc_secs),
  realTimeNsecs(att->gps_time.utc_nsecs),
  nSats(att->nsats),
  readoutTime(att->readout_time.utc_secs),
  readoutTimeNsecs(att->readout_time.utc_nsecs),
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

pueo::nav::SunSensors::SunSensors(const pueo_ss *ss)
  : readoutTime(ss->readout_time.utc_secs),
  readoutTimeNsecs(ss->readout_time.utc_nsecs),
  sequence_number(ss->sequence_number),
  flags(ss->flags)
{
  SS0.x1 = ss->ss[0].x1;
  SS0.x2 = ss->ss[0].x2;
  SS0.y1 = ss->ss[0].y1;
  SS0.y2 = ss->ss[0].y2;
  SS0.tempADS1220 = ss->ss[0].tempADS1220;
  SS0.tempSS = ss->ss[0].tempSS;
  SS1.x1 = ss->ss[1].x1;
  SS1.x2 = ss->ss[1].x2;
  SS1.y1 = ss->ss[1].y1;
  SS1.y2 = ss->ss[1].y2;
  SS1.tempADS1220 = ss->ss[1].tempADS1220;
  SS1.tempSS = ss->ss[1].tempSS;
  SS2.x1 = ss->ss[2].x1;
  SS2.x2 = ss->ss[2].x2;
  SS2.y1 = ss->ss[2].y1;
  SS2.y2 = ss->ss[2].y2;
  SS2.tempADS1220 = ss->ss[2].tempADS1220;
  SS2.tempSS = ss->ss[2].tempSS;
  SS3.x1 = ss->ss[3].x1;
  SS3.x2 = ss->ss[3].x2;
  SS3.y1 = ss->ss[3].y1;
  SS3.y2 = ss->ss[3].y2;
  SS3.tempADS1220 = ss->ss[3].tempADS1220;
  SS3.tempSS = ss->ss[3].tempSS;
  SS4.x1 = ss->ss[4].x1;
  SS4.x2 = ss->ss[4].x2;
  SS4.y1 = ss->ss[4].y1;
  SS4.y2 = ss->ss[4].y2;
  SS4.tempADS1220 = ss->ss[4].tempADS1220;
  SS4.tempSS = ss->ss[4].tempSS;
  SS5.x1 = ss->ss[5].x1;
  SS5.x2 = ss->ss[5].x2;
  SS5.y1 = ss->ss[5].y1;
  SS5.y2 = ss->ss[5].y2;
  SS5.tempADS1220 = ss->ss[5].tempADS1220;
  SS5.tempSS = ss->ss[5].tempSS;
  SS6.x1 = ss->ss[6].x1;
  SS6.x2 = ss->ss[6].x2;
  SS6.y1 = ss->ss[6].y1;
  SS6.y2 = ss->ss[6].y2;
  SS6.tempADS1220 = ss->ss[6].tempADS1220;
  SS6.tempSS = ss->ss[6].tempSS;
  SS7.x1 = ss->ss[7].x1;
  SS7.x2 = ss->ss[7].x2;
  SS7.y1 = ss->ss[7].y1;
  SS7.y2 = ss->ss[7].y2;
  SS7.tempADS1220 = ss->ss[7].tempADS1220;
  SS7.tempSS = ss->ss[7].tempSS;
}


#endif
