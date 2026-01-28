/****************************************************************************************
*  pueo/UsefulEvent.h              More useful waveform data
*  
* 
*  Cosmin Deaconu <cozzyd@kicp.uchicago.edu.edu>    
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


#ifndef PUEO_USEFUL_EVENT_H
#define PUEO_USEFUL_EVENT_H

#include <TObject.h>
#include "pueo/RawEvent.h"
#include "pueo/Conventions.h" 

#include <array>

class TGraph; 


namespace pueo 
{
  class RawHeader; 
  class UsefulEvent : public RawEvent
  {

    public: 
      UsefulEvent(const RawEvent & event, const RawHeader & header); 
      UsefulEvent() { ; }
      virtual ~UsefulEvent() { ; }

      TGraph *makeGraph(size_t chanIndex) const;
      TGraph *makeGraph(int ant, pol::pol_t pol) const; 
      TGraph *makeGraph(ring::ring_t ring, int phi, pol::pol_t pol) const; 
      TGraph *makeGraph(int surf, int chan) const; 

      std::array< std::array<double, pueo::k::MAX_NUMBER_SAMPLES>, pueo::k::NUM_RF_CHANNELS> volts;
      std::array<double, k::NUM_RF_CHANNELS> t0;
      std::array<double, k::NUM_RF_CHANNELS> dt; 
      double t(size_t chan, size_t i) const { return t0[chan] + i * dt[chan]; }


    ClassDef(UsefulEvent,2); 
  }; 
}


#endif


