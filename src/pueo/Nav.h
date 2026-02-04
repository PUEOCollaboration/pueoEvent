/****************************************************************************************
*  pueo/Nav.h             PUEO Nav Storage
*  
*  Navigation storage classes
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


#ifndef PUEO_NAV_H
#define PUEO_NAV_H

//Includes
#include <TObject.h>

#include <array>
#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif
namespace pueo 
{



  namespace nav 
  {



    /** @defgroup rootclasses The ROOT Classes
     * These are the ROOT clases that make up the event reader
     */

    class Position: public TObject
    {
     public:
       Position() {;}
       virtual ~Position() {;}

       Int_t           run = 0; 
       UInt_t          realTime = 0; ///<Time from the GPS unit
       UInt_t          timeOfDay = 0; ///<in ms since the start of the day
       UInt_t          payloadTime = 0;
       UInt_t          payloadTimeUs = 0;
       UShort_t        nSatsVis = 0; 
       UShort_t        nSatsTracked = 0; 
       Float_t         latitude = 0; ///< In degrees
       Float_t         longitude = 0; ///<In degrees
       Float_t         altitude = 0; ///<In metres, WGS84
       Float_t         course = 0; 
       Float_t         groundVelocity = 0; //m/s
       Float_t         vertVelocity = 0; //m/s
       Float_t         pdop = 0; //< 
       Float_t         hdop = 0; //< 
       Float_t         tdop = 0; //< 
       Int_t           flag = 0;
       
      ClassDef(Position,1);
    };

    // position AND attitude
    class Attitude: public TObject
    {
     public:
       Attitude() {;}
#ifdef HAVE_PUEORAWDATA
       Attitude(const pueo_nav_att_t *att);
#endif
       virtual ~Attitude(){;}
       char source;
       ULong_t         realTime = 0; ///<Time from the GPS unit
       UInt_t          realTimeNsecs = 0;
       UShort_t        nSats= 0;
       ULong_t         readoutTime = 0;
       UInt_t          readoutTimeNsecs = 0;
       Float_t         latitude = 0; ///< In degrees
       Float_t         longitude = 0; ///<In degrees
       Float_t         altitude = 0; ///<In metres, WGS84
       Float_t         heading = 0; ///<In degrees
       Float_t         pitch = 0; ///<In degreess
       Float_t         roll = 0; ///<In degreess
       Float_t         headingSigma = -1;
       Float_t         pitchSigma = -1;
       Float_t         rollSigma = -1;
       Float_t         vdop = 0;
       Float_t         hdop = 0;
       Int_t           flag = 0;
       std::array<UShort_t, 3> antennaCurrents;
       Short_t        temperature = 0;
       
      ClassDef(Attitude,2);
    };

    class Sat:  public TObject
    {
      public: 
        Sat() { used=0; } 
        int id =-1; // constellation + prn or soething
        int8_t elevation=0;
        uint8_t azimuth=0; 
        uint8_t snr=0; 
        uint8_t used : 1; 
        ClassDef(Sat,1);
      };

    class Sats : public TObject
    {
      public:
        Sats() {; }
        virtual ~Sats() {;}
        Int_t           run = 0; 
        UInt_t          realTime = 0; ///<Time from the GPS unit
        UInt_t          payloadTime = 0;
        UInt_t          payloadTimeUs = 0;
        std::vector<Sat> sats; 

       ClassDef(Sats,1); 
    }; 
  }
}


#endif //PUEO_NAV_H
