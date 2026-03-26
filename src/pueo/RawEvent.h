/****************************************************************************************
*  pueo/RawEvent.h              Raw waveform data
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
*  Barfoo is distributed in the hope that it will be useful, but WITHOUT ANY
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
*  A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
*  You should have received a copy of the GNU General Public License along with
*  Barfoo. If not, see <https://www.gnu.org/licenses/
*
****************************************************************************************/ 


#ifndef PUEO_RAW_EVENT_H
#define PUEO_RAW_EVENT_H

#include "Rtypes.h"
#include "pueo/Conventions.h"

#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif

#include <array>

//!  pueo::RawEvent -- The Raw PUEO Event Data
/*!
  The ROOT implementation of the raw PUEO event data
  \ingroup rootclasses
*/
namespace pueo
{

  class RawEvent
  {
   public:
     RawEvent(){;} ///< Default constructor
#ifdef HAVE_PUEORAWDATA
     RawEvent(const pueo_full_waveforms_t * raw) ///< Constructor from raw type
     : eventNumber(raw->event), runNumber(raw->run)
     {
       static_assert(PUEO_NCHAN == pueo::k::NUM_DIGITIZED_CHANNELS);
     
       for (size_t i = 0; i < PUEO_NCHAN; i++)
       {
         std::copy(raw->wfs[i].data, raw->wfs[i].data+pueo::k::NUM_SAMPLES, data[i].begin());
       }
     }
#endif

     ULong_t eventNumber = 0; ///< Event number

     Int_t runNumber = 0;   ///< Run number

     std::array<std::array<Short_t, pueo::k::NUM_SAMPLES>, pueo::k::NUM_DIGITIZED_CHANNELS> data;

    ClassDefNV(RawEvent,3);
  };

}

#endif 
