/****************************************************************************************
*  pueo/Daqhsk.h             PUEO Nav Storage
*  
*  Daqhsk storage classes
* 
*  Keith McBride <kmcbride@uchicago.edu>    
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


#ifndef PUEO_DAQHSK_H
#define PUEO_DAQHSK_H

//Includes
#include <TObject.h>
#include "pueo/Conventions.h"
#include <array>
#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif
namespace pueo 
{



  namespace daqhsk 
  {

    class Beam:  public TObject{
      public: 
        Beam() {;}
        UInt_t threshold=0;
        UInt_t pseudothreshold=0;
        UShort_t scaler=0;
        UShort_t pseudoscaler=0;
        Bool_t inMask=0;
        Bool_t scalerBank=0;
        ClassDef(Beam,1);
    };
    class Surf: public TObject{
     public:
       Surf() {;}
       virtual ~Surf() {;}
       // where are the constants like PUEO_NSURF and PUEO_NBEAMS
       std::array<UInt_t,8> agc_scale;// this should be PUEO_NCHAN_PER_SURF
       std::array<UInt_t,8> agc_offset; // this should be PUEO_NCHAN_PER_SURF
       UInt_t readoutTime = 0;
       UInt_t readoutTimeNsecs = 0;
       UInt_t ms_elapsed = 0;
       uint8_t surf_link = 0;
       uint8_t surf_slot = 0;
       std::array<daqhsk::Beam,pueo::k::NUM_BEAMS> Beams; 
       ClassDef(Surf,1);
    };


    /** @defgroup rootclasses The ROOT Classes
     * These are the ROOT clases that make up the event reader
     */
    // per class there will be a surf array that has the surf information and beam array inside it

    // then totally different class for the trigger information and l2 readout time, etc
    class Daqhsk: public TObject
    {
     public:
       Daqhsk() {;}
#ifdef HAVE_PUEORAWDATA
       Daqhsk(const pueo_daq_hsk_t *daqhsk);
#endif
       virtual ~Daqhsk(){;}
       UInt_t l2_readout_time = 0;
       UInt_t l2_readout_timeNsecs = 0;
       std::array<UInt_t,12> H_scalers;
       std::array<UInt_t,12> V_scalers;
       UInt_t scaler_readout_time = 0;
       UInt_t scaler_readout_timeNsecs = 0;
       std::array<std::array<UInt_t,7>,4> turfio_L1_rate;
       UInt_t soft_rate = 0;
       UInt_t pps_rate = 0;
       UInt_t ext_rate = 0;
       UShort_t MIE_total_H = 0;
       UShort_t MIE_total_V = 0;
       UShort_t LF_total_H = 0;
       UShort_t LF_total_V = 0;
       UShort_t aux_total = 0;
       UShort_t global_total = 0;
       UInt_t l2_enable_mask = 0;
       std::array<UInt_t,4> turfio_words_recv;
       UInt_t qwords_sent = 0;
       UInt_t events_sent = 0;
       UInt_t trigger_count = 0;
       UInt_t current_second = 0;
       UInt_t last_pps = 0;
       UInt_t llast_pps = 0;
       UInt_t last_dead = 0;
       UInt_t llast_dead = 0;
       UInt_t panic_count = 0;
       UInt_t occupancy = 0;
       UShort_t ack_count = 0;
       UShort_t latency = 0;
       UShort_t offset = 0;
       UShort_t pps_trig_offset = 0;
       std::array<daqhsk::Surf, pueo::k::NUM_SURF_SLOTS> Surfs;
      ClassDef(Daqhsk,1);
    };
  }
}


#endif //ADU5PAT_H
