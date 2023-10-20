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
     RawHeader(); ///< Default constructor

     Int_t           run; ///< Run number, assigned on ground
     UInt_t          realTime; ///< unixTime of readout
     UInt_t          payloadTime; ///< unixTime of readout
     UInt_t          payloadTimeUs; ///< sub second time of readout
     UInt_t          gpsSubTime; ///< sub second time from GPS (if matched)
     ULong_t         eventNumber; ///< Software event number

     UChar_t         priority; ///< Queue (lower 4-bits) and priority (upper 4-bits)

     UInt_t          phiTrigMask[k::NUM_POLS]; ///< 24-bit phi mask (from TURF)
     UInt_t          flags; 

     //Encoded Prioritizer stuff
     UChar_t peakThetaBin;
     UShort_t imagePeak;
     UShort_t coherentSumPeak;

     UInt_t        trigType; /// set pueo::trigger namespace in Conventions
     UInt_t        trigNum; ///< Trigger number (since last clear all)
     UInt_t        trigTime; ///< Trigger time in TURF clock ticks
     UInt_t        c3poNum; ///< Number of TURF clock ticks between GPS pulse per seconds
     UShort_t      ppsNum; ///< Number of GPS PPS since last clear all


     UShort_t        deadTime;
     UChar_t         bufferDepth; // Buffer depth
     UInt_t          triggerTime; ///< Trigger time from TURF converted to unixTime
     UInt_t          triggerTimeNs; ///< Trigger time in ns from TURF
     Int_t           goodTimeFlag; ///< 1 is good trigger time, 0 is bad trigger time


     // Trigger info
     Int_t  triggeringSector;  // for MI
     Int_t  triggeringBeam; // for MI
     UInt_t beamPower; // for MI
     UInt_t triggerPattern; // for Nadir or LF

     int isInPhiMask(int phi, pol::pol_t=pol::kVertical) const; ///< Returns 1 if given phi-pol is in mask


    ClassDef(RawHeader,1);

  };
}


#endif
