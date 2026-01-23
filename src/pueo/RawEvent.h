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
*  Foobar is distributed in the hope that it will be useful, but WITHOUT ANY
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
*  A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
*  You should have received a copy of the GNU General Public License along with
*  Foobar. If not, see <https://www.gnu.org/licenses/
*
****************************************************************************************/ 


#ifndef PUEO_RAW_EVENT_H
#define PUEO_RAW_EVENT_H

//Includes
#include <TObject.h>
#include "pueo/Conventions.h"
#include "stdint.h"
#include "pueo/rawdata.h"

//!  pue::RawEevent -- The Raw PUEO Event Data
/*!
  The ROOT implementation of the raw PUEO event data
  \ingroup rootclasses
*/
namespace pueo
{

class RawEvent: public TObject
{
public:
  RawEvent(){;} ///< Default constructor

  RawEvent(const pueo_full_waveforms_t * fwf){
    eventNumber = fwf->event;
    runNumber = fwf->run;
    for (std::size_t i_ch=0; i_ch<k::NUM_DIGITIZED_CHANNELS; ++i_ch){
      data[i_ch].assign(fwf->wfs[i_ch].data, fwf->wfs[i_ch].data+fwf->wfs[i_ch].length);
    }
  }
  virtual ~RawEvent() {;} ///< Destructor

  ULong_t eventNumber = 0; ///< Event number from software

  Int_t runNumber = 0;   ///< Run number from software

  std::vector<Short_t> data[k::NUM_DIGITIZED_CHANNELS]; 

  ClassDef(RawEvent,1);
};

}

#endif 
