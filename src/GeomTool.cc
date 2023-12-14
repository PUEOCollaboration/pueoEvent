/****************************************************************************************
*  GeomTool.cc            Implementation of the PUEO Geometry tool
* 
*  Cosmin Deaconu <cozzyd@kicp.uchicago.edu>, porting pueo::GeomTool form AnitaGeomTool (mostly rjn)
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

#include "pueo/GeomTool.h"
#include "pueo/Version.h" 
#include <assert.h>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <fstream>
#include "TMutex.h" 

#define INCHTOMETER 0.0254
#define R_EARTH 6.378137E6
#define  GEOID_MAX 6.378137E6 // parameters of geoid model
#define  GEOID_MIN 6.356752E6
#define C_LIGHT 299792458 //meters
#define FLATTENING_FACTOR (1./298.257223563)




Double_t deltaRL = 0.0;
Double_t deltaUD = 0.0;



bool pueo::GeomTool::readPositions(int v, const std::string & thesrc) 
{
  (void) v; 
  TString calibDir; 


  const char * src = thesrc == "" ? "default" : thesrc.c_str(); 

  char * calibEnv = getenv("PUEO_CALIB_DIR"); 
  if (calibEnv) 
  {
    calibDir = calibEnv; 
 }

  else 
  {
    char * utilEnv = getenv("PUEO_UTIL_INSTALL_DIR"); 
    if (utilEnv) 
    {
      calibDir.Form("%s/share/pueoCalib", utilEnv); 
    }
    else
    {
      calibDir = "calib/";
    }
  }


  //read in antenna positions, then we'll add in phase_centers
  for (int inst = 0; inst < 2; inst++) 
  {
    TString fname;
    fname.Form("%s/%s/%s_pos.csv", calibEnv, src, inst == 0 ? "horn" : "lf"); 

    std::ifstream f(fname.Data());
    if (!f.is_open()) 
    {
      if (inst == 0) 
      {
        std::cerr << "Could not open " << fname << ". Cannot construct GeomTool!" << std::endl; 
        return false; 
      }
      else
      {
        std::cerr << "Could not open " << fname << " or it doesn't exist. Placing LF antennas at 0's" << std::endl; 
        break; 
      }
    }

    //skip first two lines 
    
    std::string line; 
    getline(f,line); 
    getline(f,line); 

    size_t ant = 0; 
    std::vector<double> vals; 
    vals.reserve(10); 

    while (getline(f,line))
    {
      std::string val; 
      std::stringstream ss(line); 
      vals.clear(); 
      while (getline(ss,val,','))
      {
        vals.push_back(INCHTOMETER * atof(val.c_str())); 
      }

      if (inst==0) 
      {
        xAntHorns[ant] = vals[1]; 
        yAntHorns[ant] = vals[2]; 
        zAntHorns[ant] = vals[3]; 
        rAntHorns[ant] = vals[4]; 
        azCenterAntHorns[ant] = vals[5]; 
        azApertureAntHorns[ant] = vals[6]; 
        elAntHorns[ant] = vals[7]; 
      }
      else
      {
        xAntLF[ant] = vals[1]; 
        yAntLF[ant] = vals[2]; 
        zAntLF[ant] = vals[3]; 
        zAntLF[ant] = vals[4]; 
        azCenterAntLF[ant] = vals[5]; 
        azApertureAntLF[ant] = vals[6]; 
        elAntLF[ant] = vals[7]; 
      }



      for (int ipol = 0; ipol < pol::kNotAPol; ipol++)
      {
        if (inst == 0) 
        {
          xPhaseCenterHorns [ant][ipol] = xAntHorns[ant]; 
          yPhaseCenterHorns [ant][ipol] = yAntHorns[ant]; 
          zPhaseCenterHorns [ant][ipol] = zAntHorns[ant]; 
          rPhaseCenterHorns [ant][ipol] = hypot(xAntHorns[ant], yAntHorns[ant]); 
          azPhaseCenterHorns [ant][ipol] = atan2(yAntHorns[ant], xAntHorns[ant]); 
        }
        else
        {
          xPhaseCenterLF [ant][ipol] = xAntLF[ant]; 
          yPhaseCenterLF [ant][ipol] = yAntLF[ant]; 
          zPhaseCenterLF [ant][ipol] = zAntLF[ant]; 
          rPhaseCenterLF [ant][ipol] = hypot(xAntLF[ant], yAntLF[ant]); 
          azPhaseCenterLF [ant][ipol] = atan2(yAntLF[ant], xAntLF[ant]); 
        }
 
      }

      ant++; 
    }
    if (inst == 0 && ant < k::NUM_HORNS)
    {
      std::cerr << "WARNING: Only read in " << ant << " horn antennas!" << std::endl; 
    }
    if (inst == 1 && ant < k::NANTS_LF)
    {
      std::cerr << "WARNING: Only read in " << ant << " LF antennas!" << std::endl; 
    }
  }

  // add in phase centers, if we have them 

  for (int inst = 0; inst < 2; inst++) 
  {
    TString fname;
    fname.Form("%s/%s/%s_relative_phase_centers.csv", calibEnv, src, inst == 0 ? "horn" : "lf"); 

    std::ifstream f(fname.Data());
    if (!f.is_open()) 
    {
        std::cerr << "Could not open " << fname << " or it doesn't exist. No phase center offsets will be loaded!" << std::endl; 
        continue;
    }

    //skip first line
    std::string line; 
    getline(f,line); 

    while (getline(f,line))
    {
      int ant,ipol; 
      double dr,dphi,dz; 
      sscanf(line.c_str(), "%d %d %lf %lf %lf", &ant,&ipol,&dr, &dphi, &dz); 
      if (ant >=0 && ant < ( inst == 0 ? k::NUM_HORNS : k::NANTS_LF) && ipol >=0 && ipol < pol::kNotAPol)
      {
        double sinp,cosp; 
        sincos(dphi * TMath::DegToRad(), &sinp,&cosp); 
        double dx = dr*cosp;
        double dy = dr*sinp; 

        if (inst == 0) 
        {
          xPhaseCenterHorns[ant][ipol] +=  dx; 
          yPhaseCenterHorns[ant][ipol] +=  dy; 
          zPhaseCenterHorns[ant][ipol] +=  dz; 
          rPhaseCenterHorns[ant][ipol] +=  dr; 
          azPhaseCenterHorns[ant][ipol] +=  dphi;
        }
        else
        {
          xPhaseCenterLF[ant][ipol] +=  dx; 
          yPhaseCenterLF[ant][ipol] +=  dy; 
          zPhaseCenterLF[ant][ipol] +=  dz; 
          rPhaseCenterLF[ant][ipol] +=  dr; 
          azPhaseCenterLF[ant][ipol] +=  dphi ; 
        }; 
      }
      else
      {
        std::cerr << "Invalid line \"" << line << "\" in " << fname << ". Skipping"; 
      }

    }
  }

  return true; 
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
/// Default constructor: defines antenna phase centre locations.
//////////////////////////////////////////////////////////////////////////////////////////////////////
pueo::GeomTool::GeomTool(int v, const std::string & src)
{
  valid = readPositions(v,src); 

  fHeadingRotationAxis.SetXYZ(0.,0.,1.);
  fPitchRotationAxis.SetXYZ(0.,1.,0.);
  fRollRotationAxis=fPitchRotationAxis.Cross(fHeadingRotationAxis);
  aftForeOffsetAngleVertical=TMath::DegToRad()*45;
}




Int_t pueo::GeomTool::getPhiRingPolFromSurfChan(Int_t surf,Int_t chan, Int_t &phi,
						      ring::ring_t &ring,pol::pol_t &pol)
{
  if(surf<0 || surf>=k::ACTIVE_SURFS) return -1;
  if(chan<0 || chan>=k::NUM_CHANS_PER_SURF) return -1;

  // 
  if (surf <  ( k::NANTS_MI / 4)) 
  {
    pol = pol::pol_t(surf / 12); 
    phi = 2*surf + (chan / 4); 
    ring =  ring::ring_t(chan % 4); 
  }
  else if (  surf < (k::NANTS_MI/ 4)  + (k::NANTS_NADIR/4)) 
  {
    ring = ring::kNadirRing; 
    pol = pol::pol_t(chan / 4); 
    phi = 8*(surf-24) + 2 * ( chan %4); 
  }
  else
  {
    pol =  k::MULTIPLEX_LF ?  pol::kNotAPol : pol::pol_t(surf % 2); 
    phi = chan; 
    ring = ring::kLF; 
  }


  return phi;
}




Int_t pueo::GeomTool::getSurfChanAntFromRingPhiPol(ring::ring_t ring,Int_t phi, pol::pol_t pol ,Int_t &surf, Int_t &chan, Int_t &ant) {
  if(phi<0 || phi>=k::NUM_PHI) return -1;
  if (ring < 0 || ring >= ring::kNotARing) return -1; 

  if (ring == ring::kLF)
  {
    surf = k::NUM_HORNS / 4 + pol; 
    chan = phi; //figure tihs out later... 
    ant = k::NUM_HORNS+chan; 
  }
  else if (ring == ring::kNadirRing) 
  {
    assert (k::NANTS_NADIR); 
    surf = k::NANTS_MI / 4 + phi/ 8; 
    chan = 4 * pol + phi /2; 
    ant = 96 + phi /2; 
  }
  else if (ring < ring::kNadirRing) 
  {
    surf = 12 * pol  + phi/2 ;
    chan = ring + 4 * (phi %2); 
    ant = phi * 4 + ring; 
  }


  return surf;
}








//////////////////////////////////////////////////////////////////////////////////////////////////////
/// Generates an instance of pueo::GeomTool, required for non-static functions.
//TODO:  unordered_map is slow and I make a copy... 
//////////////////////////////////////////////////////////////////////////////////////////////////////
const pueo::GeomTool&  pueo::GeomTool::Instance(Int_t v, const std::string &  geometry_source )
{

  if (v < 0 || v >= k::NUM_PUEO) v = 0; 
  if (!v) v = version::get(); 

  static TMutex instance_lock; 

  static const std::string defstring = "default";
  const std::string & p = geometry_source == "" ? defstring : geometry_source; 
  static std::unordered_map<std::string,pueo::GeomTool*> instances[k::NUM_PUEO]; 

  if (!instances[v-1].count(p)) 
  {
    instance_lock.Lock(); 
    //check again while holding lock 
    if (!instances[v-1].count(p))
    {
      instances[v-1][p] = new pueo::GeomTool(v, geometry_source); 
    }
    instance_lock.UnLock(); 
  }

  return *(instances[v-1][p]);
}



Int_t pueo::GeomTool::getChanIndex(Int_t surf, Int_t chan){
  return chan+(k::NUM_CHANS_PER_SURF*surf);
}



Double_t pueo::GeomTool::getPhiDiff(Double_t firstPhi, Double_t secondPhi)
{
   Double_t phiDiff=firstPhi-secondPhi;
   if(TMath::Abs(phiDiff)>TMath::Abs(phiDiff+TMath::TwoPi()))
     phiDiff+=TMath::TwoPi();
   if(TMath::Abs(phiDiff)>TMath::Abs(phiDiff-TMath::TwoPi()))
     phiDiff-=TMath::TwoPi();
   return phiDiff;
}


Int_t pueo::GeomTool::getChanIndexFromRingPhiPol(ring::ring_t ring,
					      Int_t phi,
					      pol::pol_t pol)
{
  int surf,chan,ant; 
  getSurfChanAntFromRingPhiPol(ring,phi,pol,surf,chan,ant); 
  return getChanIndex(surf,chan); 
}


Int_t pueo::GeomTool::getChanIndexFromAntPol(Int_t ant,
					  pol::pol_t pol)
{
  int surf, chan; 
  getSurfChanFromAntPol(ant,pol,surf,chan); 
  return getChanIndex(surf,chan);
}

Int_t pueo::GeomTool::getSurfFromAntPol(Int_t ant, pol::pol_t pol)
{
  int surf,chan;
  return getSurfChanFromAntPol(ant,pol,surf,chan); 
}

Int_t pueo::GeomTool::getSurfChanFromAntPol(Int_t ant, pol::pol_t pol, Int_t & surf, Int_t & chan)
{
  ring::ring_t ring = getRingFromAnt(ant); 
  Int_t phi = getPhiFromAnt(ant); 
  int new_ant; 
  getSurfChanAntFromRingPhiPol(ring,phi,pol, surf,chan, new_ant); 
  assert(new_ant == ant); 
  return surf; 
}


Int_t pueo::GeomTool::getPhiSector(Int_t chanIndex)
{
  int surf, chan; 
  getSurfChanFromChanIndex(chanIndex,surf,chan); 
  int phi; 
  ring::ring_t ring; 
  pol::pol_t pol; 
  getPhiRingPolFromSurfChan(surf,chan,phi,ring,pol); 
  return phi; 
}


Double_t pueo::GeomTool::getDirectionWrtNorth(Int_t phiSector, Double_t heading) const {

  if(phiSector<0 || phiSector>=k::NUM_PHI) return -1.;

  //Copied this change from Amy don't (yet) know if it is correct
  // Float_t direction=(1.*360./NUM_PHI)+heading-(phiSector*360./NUM_PHI);

  ///< Heading increases clockwise but phi-sector numbers increase anticlockwise, hence -ve sign.
  Double_t direction = heading + aftForeOffsetAngleVertical*TMath::RadToDeg() - (phiSector*360./k::NUM_PHI);
  
  if(direction>=360){
    direction -= 360;
  }
  else if(direction < 0){
    direction += 360;
  }

  return direction;

}



Int_t pueo::GeomTool::getSurfChanFromChanIndex(Int_t chanIndex, // input channel index
					    Int_t &surf,Int_t &chan) // output surf and channel
{
  if(chanIndex<0 || chanIndex>=k::NUM_DIGITZED_CHANNELS)
    return -1;

  //  std::cout << "chanIndex is " << chanIndex << "\n";
  chan=chanIndex%k::NUM_CHANS_PER_SURF; // 0 to 8
  surf=(chanIndex-chan)/k::NUM_CHANS_PER_SURF; // 0 to 8

  return surf;

}
Int_t pueo::GeomTool::getAntPolFromChanIndex(Int_t chanIndex,Int_t &ant, pol::pol_t &pol) 
{

  int surf,chan;
  getSurfChanFromChanIndex(chanIndex, surf,chan); 
  int phi; 
  ring::ring_t ring; 
  getPhiRingPolFromSurfChan(surf,chan,phi,ring,pol); 
  ant = getAntFromPhiRing(phi,ring); 
  return ant; 
}


Int_t pueo::GeomTool::getAntPolFromSurfChan(Int_t surf,Int_t chan,Int_t &ant, pol::pol_t &pol) 
{

  int phi; 
  ring::ring_t ring; 
  getPhiRingPolFromSurfChan(surf,chan,phi,ring,pol); 
  ant = getAntFromPhiRing(phi,ring); 
  return ant; 
}

Int_t pueo::GeomTool::getAntOrientation(Int_t ant) {
  (void) ant; return 1; 
}


pueo::ring::ring_t pueo::GeomTool::getRingFromAnt(Int_t ant) {
  if (ant< k::NANTS_MI)
    return ring::ring_t(ant % 4) ;
  else if (ant < k::NANTS_MI + k::NANTS_NADIR) 
    return ring::kNadirRing;
  else if (ant < k::NUM_HORNS + k::NANTS_LF) 
    return ring::kLF;
  else return ring::kNotARing; 


}

pueo::ring::ring_t pueo::GeomTool::getRingAntPolPhiFromSurfChan(Int_t surf, Int_t chan,
						 ring::ring_t &ring,
						 Int_t &ant,
						 pol::pol_t &pol,
						 Int_t &phi)
{
  pueo::GeomTool::getAntPolFromSurfChan(surf,chan,ant,pol);
  ring=pueo::GeomTool::getRingFromAnt(ant);
  phi=pueo::GeomTool::getPhiFromAnt(ant);
  return ring; 
}




//Non static thingies
void pueo::GeomTool::getAntXYZ(Int_t ant, Double_t &x, Double_t &y, Double_t &z,pol::pol_t pol) const
{
  if(ant>=0 && ant<k::NUM_HORNS) {
    x=xPhaseCenterHorns[ant][pol];
    y=yPhaseCenterHorns[ant][pol];
    z=zPhaseCenterHorns[ant][pol];
    //std::cout << "ant " << x << " y " << y << " z " << std::endl;
  }
  else if (ant < k::NUM_HORNS + k::NANTS_LF) 
  {
    x = xPhaseCenterLF[ant-k::NUM_HORNS][pol]; 
    y = yPhaseCenterLF[ant-k::NUM_HORNS][pol]; 
    z = zPhaseCenterLF[ant-k::NUM_HORNS][pol]; 
  }
}

Double_t pueo::GeomTool::getAntZ(Int_t ant, pol::pol_t pol) const {
  double x,y,z; 
  getAntXYZ(ant,x,y,z,pol); 
  return z; 
}

Double_t pueo::GeomTool::getAntR(Int_t ant, pol::pol_t pol) const 
{
  if(ant>=0 && ant<k::NUM_HORNS) 
    return rPhaseCenterHorns[ant][pol]; 
  else if (ant < k::NUM_HORNS + k::NANTS_LF) 
    return rPhaseCenterLF[ant-k::NUM_HORNS][pol]; 
 
  return -1; 
}

Double_t pueo::GeomTool::getAntPhiPosition(Int_t ant, pol::pol_t pol) const{
  if(ant>=0 && ant<k::NUM_HORNS) 
    return azPhaseCenterHorns[ant][pol]; 
  else if (ant < k::NUM_HORNS + k::NANTS_LF) 
    return azPhaseCenterLF[ant-k::NUM_HORNS][pol]; 
  return -1; 
}

Double_t pueo::GeomTool::getAntPhiPositionRelToAftFore(Int_t ant, pol::pol_t pol) const {

  Double_t phi= getAntPhiPosition(ant,pol); 
  if (phi < 0) return -1; 
  phi-=aftForeOffsetAngleVertical;
  if(phi<0)
    phi+=TMath::TwoPi();
  if(phi>TMath::TwoPi())
    phi-=TMath::TwoPi();
  return phi; 
}

//Double_t pueo::GeomTool::getMeanAntPairPhiRelToAftFore(Int_t firstAnt, Int_t secondAnt, pol::pol_t pol) {
//
//   if(firstAnt>=0 && firstAnt<k::NUM_ANTS && secondAnt>=0 && secondAnt<k::NUM_ANTS) {
//      //Calculating the phi of each antenna pair
//      Double_t meanPhi=this->getAntPhiPositionRelToAftFore(firstAnt,pol); 
//      if(TMath::Abs(meanPhi-this->getAntPhiPositionRelToAftFore(secondAnt,pol))<TMath::PiOver2()) {	 
//        meanPhi+=this->getAntPhiPositionRelToAftFore(secondAnt,pol);	 
//        meanPhi*=0.5;       
//      }       
//      else if(TMath::Abs(meanPhi-(this->getAntPhiPositionRelToAftFore(secondAnt,pol)+TMath::TwoPi()))<TMath::PiOver2()) {	 
//        meanPhi+=(this->getAntPhiPositionRelToAftFore(secondAnt,pol)+TMath::TwoPi());	 
//        meanPhi*=0.5;       
//      }       
//      else if(TMath::Abs(meanPhi-(this->getAntPhiPositionRelToAftFore(secondAnt,pol)-TMath::TwoPi()))<TMath::PiOver2()) {	 
//        meanPhi+=(this->getAntPhiPositionRelToAftFore(secondAnt,pol)-TMath::TwoPi());	 
//        meanPhi*=0.5;       
//      }      
//      return meanPhi;
//   }
//   return -999;
//}
//


Int_t pueo::GeomTool::getTopAntNearestPhiWave(Double_t phiWave, pol::pol_t pol) const  {
  if(phiWave<0) phiWave+=TMath::TwoPi();
  if(phiWave>TMath::TwoPi()) phiWave-=TMath::TwoPi();
  Double_t minDiff=TMath::TwoPi();
  Int_t minAnt=0;;
  for(Int_t ant=0;ant<16;ant++) {
    //      std::cout << ant << "\t" << azPhaseCentreFromVerticalHorn[ant][pol] << "\t" << phiPrime << "\t" << TMath::Abs(azPhaseCentreFromVerticalHorn[ant][pol]-phiPrime) << "\n";
    if(TMath::Abs(getAntPhiPositionRelToAftFore(ant,pol)-phiWave)<minDiff) {
      minDiff=TMath::Abs(getAntPhiPositionRelToAftFore(ant,pol)-phiWave);
      minAnt=ant;
    }
    
    if(TMath::Abs((getAntPhiPositionRelToAftFore(ant,pol)-TMath::TwoPi())-phiWave)<minDiff) {
      minDiff=TMath::Abs((getAntPhiPositionRelToAftFore(ant,pol)-TMath::TwoPi())-phiWave);
      minAnt=ant;      
    }

    if(TMath::Abs((getAntPhiPositionRelToAftFore(ant,pol)+TMath::TwoPi())-phiWave)<minDiff) {
      minDiff=TMath::Abs((getAntPhiPositionRelToAftFore(ant,pol)+TMath::TwoPi())-phiWave);
      minAnt=ant;      
    }

  }
  return minAnt;
}

Int_t pueo::GeomTool::getUpperAntNearestPhiWave(Double_t phiWave, pol::pol_t pol) const {
  return getTopAntNearestPhiWave(phiWave,pol);
}


//Non static thingies
void pueo::GeomTool::getAntFaceXYZ(Int_t ant, Double_t &x, Double_t &y, Double_t &z) const
{
  if(ant>=0 && ant<k::NUM_HORNS) {
    x=xAntHorns[ant];
    y=yAntHorns[ant];
    z=zAntHorns[ant];
  }
  else if (ant < k::NUM_HORNS + k::NANTS_LF) 
  {
    x=xAntLF[ant-k::NUM_HORNS];
    y=yAntLF[ant-k::NUM_HORNS];
    z=zAntLF[ant-k::NUM_HORNS];
  }
}

Double_t pueo::GeomTool::getAntFaceZ(Int_t ant) const{
  double x =0,y=0,z=0; 
  getAntFaceXYZ(ant,x,y,z); 
  return z; 
}

Double_t pueo::GeomTool::getAntFaceR(Int_t ant) const{
  if(ant>=0 && ant<k::NUM_HORNS) {
    return rAntHorns[ant];
  }
  else if (ant < k::NUM_HORNS + k::NANTS_LF) 
  {
    return rAntLF[ant-k::NUM_HORNS];
  }
  return 0; 
}

Double_t pueo::GeomTool::getAntFacePhiPosition(Int_t ant) const {
  if(ant>=0 && ant<k::NUM_HORNS) {
    return azCenterAntHorns[ant];
  }
  else if (ant < k::NUM_HORNS + k::NANTS_LF) 
  {
    return azCenterAntLF[ant-k::NUM_HORNS];
  }
  return 0; 
}

Double_t pueo::GeomTool::getAntFacePhiPositionRelToAftFore(Int_t ant) const {
  Double_t phi=getAntFacePhiPosition(ant)-aftForeOffsetAngleVertical;
  if(phi<0)
    phi+=TMath::TwoPi();
  if (phi > TMath::TwoPi())
    phi-=TMath::TwoPi(); 
  return phi;
}

Int_t pueo::GeomTool::getTopAntFaceNearestPhiWave(Double_t phiWave) const {
  Double_t phiPrime=phiWave+aftForeOffsetAngleVertical;
  if(phiPrime>TMath::TwoPi()) 
    phiPrime-=TMath::TwoPi();
  Double_t minDiff=TMath::TwoPi();
  Int_t minAnt=0;;
  for(Int_t phi=0;phi<k::NUM_PHI;phi++) {
    int ant = getAntFromPhiRing(phi, ring::kTopRing);
    if(TMath::Abs(azCenterAntHorns[ant]-phiPrime)<minDiff) {
      minDiff=TMath::Abs(azCenterAntHorns[ant]-phiPrime);
      minAnt=ant;
    }
  }
  return minAnt;
}




Int_t pueo::GeomTool::getPhiFromAnt(Int_t ant)
{
  if(ant<k::NANTS_MI)
    return ant / 4; 

  //nadirs 
  else if (ant < k::NANTS_MI + k::NANTS_NADIR)
    return 2*(ant-k::NANTS_MI); 

  //just number like this 
  else return ant - (k::NUM_HORNS); 
}


Int_t pueo::GeomTool::getAntFromPhiRing(Int_t phi, ring::ring_t ring)
{
  if (ring < ring::kNadirRing) 
  {
    return ring + phi *4; 
  }

  else if (ring == ring::kNadirRing)
  {
    assert(k::NANTS_NADIR); 
    return k::NANTS_MI + phi/2; 
  }
  else return k::NUM_HORNS + phi; 
}


pueo::ring::ring_t pueo::GeomTool::getRingFromChanIndex(Int_t idx) 
{
  int ant; pol::pol_t pol; 
  getAntPolFromChanIndex(idx,ant,pol); 
  return getRingFromAnt(ant); 
}

void pueo::GeomTool::getCartesianCoords(Double_t lat, Double_t lon, Double_t alt, Double_t p[3])
{
  if(lat<0) lat*=-1;
   //Note that x and y are switched to conform with previous standards
   lat*=TMath::DegToRad();
   lon*=TMath::DegToRad();
   //calculate x,y,z coordinates
   Double_t C2=pow(cos(lat)*cos(lat)+(1-FLATTENING_FACTOR)*(1-FLATTENING_FACTOR)*sin(lat)*sin(lat),-0.5);
   Double_t Q2=(1-FLATTENING_FACTOR)*(1-FLATTENING_FACTOR)*C2;
   p[1]=(R_EARTH*C2+alt)*TMath::Cos(lat)*TMath::Cos(lon);
   p[0]=(R_EARTH*C2+alt)*TMath::Cos(lat)*TMath::Sin(lon);
   p[2]=(R_EARTH*Q2+alt)*TMath::Sin(lat);
}

void pueo::GeomTool::getLatLonAltFromCartesian(Double_t p[3], Double_t &lat, Double_t &lon, Double_t &alt)
{
  //Here again x and y are flipped for confusions sake
  Double_t xt=p[1];
  Double_t yt=p[0];
  Double_t zt=p[2]; //And flipped z for a test

  static Double_t cosaeSq=(1-FLATTENING_FACTOR)*(1-FLATTENING_FACTOR);
  Double_t lonVal=TMath::ATan2(yt,xt);
  Double_t xySq=TMath::Sqrt(xt*xt+yt*yt);
  Double_t tanPsit=zt/xySq;
  Double_t latGuess=TMath::ATan(tanPsit/cosaeSq);
  Double_t nextLat=latGuess;
  Double_t geomBot=R_EARTH*R_EARTH*xySq;
  do {
    latGuess=nextLat;
    Double_t N=R_EARTH/TMath::Sqrt(cos(latGuess)*cos(latGuess)+(1-FLATTENING_FACTOR)*(1-FLATTENING_FACTOR)*sin(latGuess)*sin(latGuess));
    Double_t top=(R_EARTH*R_EARTH*zt + (1-cosaeSq)*cosaeSq*TMath::Power(N*TMath::Sin(latGuess),3));
    Double_t bottom=geomBot-(1-cosaeSq)*TMath::Power(N*TMath::Cos(latGuess),3);        
    nextLat=TMath::ATan(top/bottom);
    //    std::cout << latGuess << "\t" << nextLat << "\n";
    
  } while(TMath::Abs(nextLat-latGuess)>0.0001);
  latGuess=nextLat;
  Double_t N=R_EARTH/TMath::Sqrt(cos(latGuess)*cos(latGuess)+(1-FLATTENING_FACTOR)*(1-FLATTENING_FACTOR)*sin(latGuess)*sin(latGuess));
  Double_t height=(xySq/TMath::Cos(nextLat))-N;
  
  lat=latGuess*TMath::RadToDeg();
  lon=lonVal*TMath::RadToDeg();
  alt=height;
  if(lat>0) lat*=-1;
 
}

Double_t pueo::GeomTool::getDistanceToCentreOfEarth(Double_t lat)
{
  Double_t pVec[3];
  getCartesianCoords(lat,0,0,pVec);
//   Double_t cosLat=TMath::Cos(lat);
//   Double_t sinLat=TMath::Sin(lat);
//   Double_t a=R_EARTH;
//   Double_t b=a-FLATTENING_FACTOR*a;
//   Double_t radSq=(a*a*cosLat)*(a*a*cosLat)+(b*b*sinLat)*(b*b*sinLat);
//   radSq/=(a*cosLat)*(a*cosLat)+(b*sinLat)*(b*sinLat);
 //  Double_t cosSqAe=(1-FLATTENING_FACTOR)*(1-FLATTENING_FACTOR);
//   Double_t N=R_EARTH/TMath::Sqrt(cosLat*cosLat+cosSqAe*sinLat*sinLat);
//   Double_t radSq=N*N*(cosLat*cosLat+cosSqAe*cosSqAe*sinLat*sinLat);
  return TMath::Sqrt(pVec[0]*pVec[0]+pVec[1]*pVec[1]+pVec[2]*pVec[2]);
}



