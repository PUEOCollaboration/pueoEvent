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

       Int_t           run; 
       UInt_t          realTime; ///<Time from the GPS unit
       UInt_t          timeOfDay; ///<in ms since the start of the day
       UInt_t          payloadTime;
       UInt_t          payloadTimeUs;
       UShort_t        nSatsVis; 
       UShort_t        nSatsTracked; 
       Float_t         latitude; ///< In degrees
       Float_t         longitude; ///<In degrees
       Float_t         altitude; ///<In metres, WGS84
       Float_t         course; 
       Float_t         groundVelocity; //m/s
       Float_t         vertVelocity; //m/s
       Float_t         pdop; //< 
       Float_t         hdop; //< 
       Float_t         tdop; //< 
       Int_t           flag;
       
      ClassDef(Position,1);
    };

    // position AND attitude
    class Attitude: public TObject
    {
     public:
       Attitude() {;}
       virtual ~Attitude(){;}

       Int_t           run; 
       UInt_t          realTime; ///<Time from the GPS unit
       UInt_t          timeOfDay; ///<in ms since the start of the day
       UShort_t       nSatsVis; 
       UShort_t        nSatsTracked; 
       UInt_t          payloadTime;
       UInt_t          payloadTimeUs;
       Float_t         latitude; ///< In degrees
       Float_t         longitude; ///<In degrees
       Float_t         altitude; ///<In metres, WGS84
       Float_t         heading; ///<In degrees
       Float_t         pitch; ///<In degreess
       Float_t         roll; ///<In degreess
       Float_t         mrms; 
       Float_t         brms; 
       Int_t           flag; 
       
      ClassDef(Attitude,1);
    };

    class Sat:  public TObject
    {
      int id; // constellation + prn or soething
      int8_t elevation;
      uint8_t azimuth; 
      uint8_t snr; 
      uint8_t used : 1; 
      ClassDef(Sat,1);
    };

    class Sats : public TObject
    {
      public:
        Sats() {; }
        virtual ~Sats() {;}
        Int_t           run; 
        UInt_t          realTime; ///<Time from the GPS unit
        UInt_t          payloadTime;
        UInt_t          payloadTimeUs;
        std::vector<Sat> sats; 

       ClassDef(Sats,1); 
    }; 
  }
}


#endif //ADU5PAT_H
