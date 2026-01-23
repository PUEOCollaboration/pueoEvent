/****************************************************************************************
*  Conventions.cc           PUEO Convention helpers
*  
*  Constants and such
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

#include "pueo/Conventions.h"
#include <iostream>
#include <cstring>


const char *pueo::ring::asString(pueo::ring::ring_t ring)
{
  switch(ring) {
  case kTopRing: return "Top";
  case kUpperMiddleRing: return "Upper Middle";
  case kLowerMiddleRing: return "Lower Middle";
  case kBottomRing: return "Bottom";
  case kLFTopRing: return "LF Top";
  case kLFUpperMiddleRing: return "LF Upper Middle";
  case kLFLowerMiddleRing: return "LF Lower Middle";
  case kLFBottomRing: return "LF Bottom";
  default:
    return "Unknown";
  }
  return "Unknown";
}


char pueo::pol::asChar(pueo::pol::pol_t pol)
{
  switch(pol) {
  case kVertical: return 'V';
  case kHorizontal: return 'H';
      default:
    return 'U';
  }
  return 'U';
}

// abuse GCC/Clang extension here because it's way more convenient


static const double locations[4][3] = 
{
  [pueo::Locations::WAIS] = { -79.468116, -112.059258, 1779.80},
  [pueo::Locations::LDB] = {-(77 + (51.23017/60)), (167 + (12.16908/60)), 0},
  [pueo::Locations::TAYLOR_DOME] = {  -77.78695, 158.66555, 2395 },
  [pueo::Locations::SOUTH_PLUS_200] = { -81.9457,178.392, 0},
};


Double_t pueo::Locations::getLatitude(pueo::Locations::loc_t where) {return locations[where][0]; }
Double_t pueo::Locations::getLongitude(pueo::Locations::loc_t where) {return locations[where][1]; }
Double_t pueo::Locations::getAltitude(pueo::Locations::loc_t where) {return locations[where][2] ;}
