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

//Includes
#include <TObject.h>
#include "pueo/Conventions.h"

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
  class RawHeader: public TObject
  {
     public:
     RawHeader() {;} ///< Default constructor

#ifdef HAVE_PUEORAWDATA
     RawHeader(const pueo_full_waveforms_t * wfs);
#endif
     Int_t           run = 0 ; ///< Run number, assigned on ground
     UInt_t          triggerTime= 0 ; ///< from the DAQ, may be correct or not
     UInt_t          triggerTimeNs= 0;
     UInt_t          readoutTime = 0 ; ///< unixTime of readout
     UInt_t          readoutTimeNs= 0 ; ///< sub second time of readout
     ULong_t         eventNumber = 0 ; ///< Software event number
     UInt_t          L2Mask = {0};
     UInt_t          phiTrigMask[k::NUM_POLS] = {0}; ///< 24-bit phi mask (from TURF)
     UInt_t          flags = 0; 


     UInt_t        trigType = 0; /// set pueo::trigger namespace in Conventions
     UInt_t        trigTime = 0; ///< Trigger time in TURF clock ticks
     UInt_t        lastPPS = 0; ///< Number of TURF clock ticks between GPS pulse per seconds
     UInt_t        lastLastPPS = 0; ///< Number of TURF clock ticks between GPS pulse per seconds


     UShort_t        deadTime = 0;
     UShort_t        deadTimeLastPPS = 0;
     UShort_t        deadTimeLastLastPPS = 0;

     UChar_t         L1Octants[k::NUM_SURF_SLOTS] = 0;

     // Trigger info
     int isInPhiMask(int phi, pol::pol_t=pol::kVertical) const; ///< Returns 1 if given phi-pol is in mask


    ClassDef(RawHeader,2);

  };
}


#endif
