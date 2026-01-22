/****************************************************************************************
*  pueo/TruthEvent.h             MC Truth 
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


#ifndef PUEO_TRUTH_EVENT_H
#define PUEO_TRUTH_EVENT_H

#include <TObject.h>
#include <TVector3.h>
#include "Conventions.h" 

namespace pueo 
{

  class TruthEvent : public TObject 
  {
    public: 
      TruthEvent() { ; }
      virtual ~TruthEvent() {; }

      Int_t run = 0; //< Run number;
      UInt_t          realTime=0;               ///< unixTime of readout
      UInt_t          eventNumber=0;            ///< Software event number


      TVector3        balloonPos;             ///< Balloon position (icemc Cartesian coordinates) 
      TVector3        balloonDir;             ///< Balloon direction (icemc Cartesian coordinateS)
      TVector3        nuPos;                  ///< Neutrino position
      TVector3        nuDir;                  ///< Neutrino direction
      TVector3        rfExit;                 ///< Position of rf exit
      Double_t        nuMom = 0;              ///< Neutrino momentum
      Int_t           nu_pdg = 0;             ///< Neutrino PDG code
      TVector3        polarization;           ///< 
      TVector3        poynting;               ///< 
      Double_t        payloadPhi = -999;      ///< Phi of signal in payload coordinates (degrees)
      Double_t        payloadTheta = -999;    ///< Theta of signal in payload coordinates (degrees)
      Double_t        sourceLon = 0;              ///< RF position when leaving the ice: Longitude (using icemc model)
      Double_t        sourceLat = 0;          ///< RF position when leaving the ice: Latitude  (using icemc model)
      Double_t        sourceAlt = 0;          ///< RF position when leaving the ice: Altitude  (using icemc model)
      Double_t        weight = 0;             ///< Weight assigned by icemc


      std::vector<double> signal[k::NUM_DIGITZED_CHANNELS];  ///<Noise-less signal

    ClassDef(TruthEvent,1); 
  }; 
}


#endif
