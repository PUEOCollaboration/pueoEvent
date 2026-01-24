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
#include "pueo/rawdata.h"

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
  RawHeader(const  pueo_full_waveforms_t * fwf) :
  run(fwf->run),
  realTime(fwf->readout_time.utc_secs),
  payloadTime(fwf->readout_time.utc_secs), // note: actually not sure about this
  payloadTimeUs(fwf->readout_time.utc_secs * 1e3), // nor this
  gpsSubTime(),
  eventNumber(),
  priority(),
  flags(),
  peakThetaBin(),
  imagePeak(),
  coherentSumPeak(),
  trigType(),
  trigNum(),
  trigTime(),
  c3poNum(),
  ppsNum(),
  deadTime(),
  bufferDepth(),
  triggerTime(),
  triggerTimeNs(),
  goodTimeFlag(),
  triggeringSector(),
  triggeringBeam(),
  beamPower(),
  triggerPattern()
  {
    phiTrigMask[0] = 0;
    phiTrigMask[1] = 0;
  }

  Int_t           run = 0 ; ///< Run number, assigned on ground
  UInt_t          realTime = 0 ; ///< unixTime of readout
  UInt_t          payloadTime = 0 ; ///< unixTime of readout
  UInt_t          payloadTimeUs = 0 ; ///< sub second time of readout
  UInt_t          gpsSubTime = 0 ; ///< sub second time from GPS (if matched)
  ULong_t         eventNumber = 0 ; ///< Software event number

  UChar_t         priority = 0 ; ///< Queue (lower 4-bits) and priority (upper 4-bits)

  UInt_t          phiTrigMask[k::NUM_POLS] = {0}; ///< 24-bit phi mask (from TURF)
  UInt_t          flags = 0; 

  //Encoded Prioritizer stuff
  UChar_t         peakThetaBin = 0;
  UShort_t        imagePeak = 0;
  UShort_t        coherentSumPeak = 0;

  UInt_t          trigType = 0; /// set pueo::trigger namespace in Conventions
  UInt_t          trigNum = 0; ///< Trigger number (since last clear all)
  UInt_t          trigTime = 0; ///< Trigger time in TURF clock ticks
  UInt_t          c3poNum = 0; ///< Number of TURF clock ticks between GPS pulse per seconds
  UShort_t        ppsNum = 0; ///< Number of GPS PPS since last clear all


  UShort_t        deadTime = 0;
  UChar_t         bufferDepth = 0; // Buffer depth
  UInt_t          triggerTime = 0; ///< Trigger time from TURF converted to unixTime
  UInt_t          triggerTimeNs = 0; ///< Trigger time in ns from TURF
  Int_t           goodTimeFlag = 0; ///< 1 is good trigger time, 0 is bad trigger time


  // Trigger info
  Int_t  triggeringSector = 0;  // for MI
  Int_t  triggeringBeam = 0; // for MI
  UInt_t beamPower = 0; // for MI
  UInt_t triggerPattern = 0; // for Nadir or LF

  int isInPhiMask(int phi, pol::pol_t=pol::kVertical) const; ///< Returns 1 if given phi-pol is in mask


  ClassDef(RawHeader,1);

};
}


#endif
