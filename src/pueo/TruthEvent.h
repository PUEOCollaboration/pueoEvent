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
#include "pueo/Conventions.h" 

namespace pueo 
{

  class TruthEvent : public TObject 
  {
    TruthEvent() { ; }
    virtual ~TruthEvent(); 

    Int_t run = 0; //< Run number;
    UInt_t          realTime=0;               ///< unixTime of readout
    UInt_t          eventNumber=0;            ///< Software event number


    TVector3        balloonPos;             ///< Balloon position (icemc Cartesian coordinates) 
    TVector3        balloonDir;             ///< Balloon direction (icemc Cartesian coordinateS)
    TVector3        nuPos;                  ///< Neutrino position
    TVector3        nuDir;                  ///< Neutrino direction
    TVector3        rfExit;                 ///< Position of rf exit
    Double_t        nuMom;                  ///< Neutrino momentum
    Int_t           nu_pdg;                 ///< Neutrino PDG code
    Double_t        e_component;            ///< E comp along polarization
    Double_t        h_component;            ///< H comp along polarization
    Double_t        n_component;            ///< Normal comp along polarization
    Double_t        e_component_k;          ///< Component of e-field along the rx e-plane
    Double_t        h_component_k;          ///< Component of the e-field along the rx h-plane
    Double_t        n_component_k;          ///< Component of the e-field along the normal 
    Double_t        vmmhz_max;              ///< Maximum signal at balloon (V/m/MHz)
    Double_t        payloadPhi;             ///< Phi of signal in payload coordinates (degrees)
    Double_t        payloadTheta;           ///< Theta of signal in payload coordinates (degrees)
    Double_t        sourceLon;              ///< RF position when leaving the ice: Longitude (using icemc model)
    Double_t        sourceLat;              ///< RF position when leaving the ice: Latitude  (using icemc model)
    Double_t        sourceAlt;              ///< RF position when leaving the ice: Altitude  (using icemc model)
    Double_t        weight;                 ///< Weight assigned by icemc


    std::vector<double> signal[k::NUM_DIGITZED_CHANNELS];  ///<Noise-less signal
    std::vector<double> noise[k::NUM_DIGITZED_CHANNELS];   ///<Signal-less noise
    Double_t SNRAtTrigger[k::NUM_DIGITZED_CHANNELS];               ///< Array of SNR at trigger
    Double_t maxSNRAtTriggerV;                                  ///< Max SNR at trigger V-POL
    Double_t maxSNRAtTriggerH;                                  ///< Max SNR at trigger H-POL
    Double_t SNRAtDigitizer[k::NUM_DIGITZED_CHANNELS];             ///< Array of SNR at digitizer
    Double_t maxSNRAtDigitizerV;                                ///< Max SNR at digitizer V-POL
    Double_t maxSNRAtDigitizerH;                                ///< Max SNR at digitizer H-POL
    
   

    ClassDef(TruthEvent,1); 
  }; 
}


#endif
