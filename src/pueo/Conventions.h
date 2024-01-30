/****************************************************************************************
*  pueo/Conventions.h           PUEO Convenentions
*  
*  Constants and such
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

#ifndef PUEO_CONVENTIONS_H
#define PUEO_CONVENTIONS_H


#ifndef ROOT_Rtypes
#if !defined(__CINT__) || defined(__MAKECINT__)
#include "Rtypes.h"
#endif
#endif

#include "pueo/Version.h"


namespace  pueo 
{
  namespace k
  {
    constexpr int NANTS_MI = 96; 
    constexpr int NANTS_NADIR = 0; 
    constexpr int NANTS_LF = 8; 
    constexpr int MULTIPLEX_LF = 0; 
    constexpr int NUM_HORNS = NANTS_MI + NANTS_NADIR; 
    constexpr int NUM_ANTS = NANTS_MI + NANTS_NADIR + NANTS_LF; 
    constexpr int NUM_POLS = 2;
    constexpr int NUM_PUEO = 1; 
    constexpr int NUM_PHI = 24; 
    constexpr int NUM_CHANS_PER_SURF = 8; 
    constexpr int ACTIVE_SURFS = 28; 
    constexpr int NUM_DIGITZED_CHANNELS = NUM_ANTS*NUM_POLS; 
    constexpr int MAX_NUMBER_SAMPLES = 4096; 
  }

//Now some geometry and polarisation considerations

//!  ring::ring_t -- Enumeration for the rings
/*!
 * This matches the convention for map.dat
  \ingroup rootclasses
*/
  namespace ring {
    typedef enum ering {
      kUnknown = 0,
      kTopRing  = 1, ///< The Top Ring,
      kUpperMiddleRing  = 2, ///< The UIpper Middle Ring.
      kLowerMiddleRing = 3,
      kBottomRing = 4, ///< The Bottom Ring.
      kLFTopRing =5,
      kLFUpperMiddleRing =6,
      kLFLowerMiddleRing =7,
      kLFBottomRing =8,
      kNotARing ///< Useful in for loops.
    } ring_t; ///< Ring enumeration

    inline ring_t fromIndex(int idx) { return idx > kUnknown && idx < kNotARing ? ring_t(idx) : kUnknown; }
    inline ring_t fromIdx(int idx) { return fromIndex(idx) ; }
    inline bool isLF(ring_t ring) { return ring >=kTopRing && ring <= kLFBottomRing ; }
    const char *asString(pueo::ring::ring_t ring); ///< Returns the ring as a character string
  }

//!  pol::pol_t -- Enumeration for the two polarisations
/*!
  Really that's all there is to it.
  \ingroup rootclasses
*/
namespace pol {
   typedef enum epol {
     kHorizontal = 0, ///< Horizontal Polarisation
     kVertical   = 1, ///< Vertical Polarisation
     kNotAPol ///< USeful in for loops.
   } pol_t; ///< Polarisation enumeration.
   char asChar(pueo::pol::pol_t pol); ///< Returns the polarisation as a character string.
   inline pol_t fromChar(char p) { return p == 'H' ? kHorizontal : p == 'V' ? kVertical : kNotAPol; }
}

namespace trigger
{
  typedef enum etype : uint32_t 
  {
    kUnknown = 0,
    kRFMI = 1, 
    kExt  = 2, //probably for debugging? 
    kRFLF = 4, 
    kPPS0 = 8 , 
    kPPS1 = 16, 
    kSoft = 32,
    kVPol = 64, 
    kHPol = 128

  } type_t; 

  inline bool isRFTrigger( uint32_t t) 
  {
    uint32_t rf_trigger = kRFMI | kRFLF; 
    return  !!(t & rf_trigger);
  }
}



//
//
/*!
  Things like the calibration antennas and pulsers etc.
  \ingroup rootclasses
*/
namespace Locations {


  Double_t getWaisLatitude();
  Double_t getWaisLongitude();
  Double_t getWaisAltitude();

  Double_t getLDBLatitude();
  Double_t getLDBLongitude();
  Double_t getLDBAltitude();

}

}



#endif //PUEO_CONVENTIONS_H
