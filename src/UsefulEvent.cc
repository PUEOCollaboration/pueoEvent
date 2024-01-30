/****************************************************************************************
*  UseuflEvent.cc            Implementation of the PUEO Useful Event
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


#include "pueo/UsefulEvent.h" 
#include "pueo/GeomTool.h" 
#include "pueo/RawHeader.h" 

#include "TGraph.h"
#include "TAxis.h" 

ClassImp(pueo::UsefulEvent); 



pueo::UsefulEvent::UsefulEvent(const RawEvent & event, const RawHeader & header) 
  : RawEvent(event)
{
  (void) header; 

  auto geom = GeomTool::Instance(); 
  for (size_t ichan = 0; ichan < k::NUM_DIGITZED_CHANNELS; ichan++) 
  {
    volts[ichan].resize(data[ichan].size()); 
    for (size_t i = 0; i < volts[ichan].size(); i++) 
    {
      volts[ichan][i] = data[ichan][i] *500./2048 ; // TODO: CALIBRATION
    }
    t0[ichan] = 0;//TODO!!!  will likely depend on trigger type or something... 
    dt[ichan] = ring::isLF(geom.getRingFromChanIndex(ichan)) ? 1./1.5 :  1./3; 
  }

}

TGraph * pueo::UsefulEvent::makeGraph(int ant, pol::pol_t pol) const
{
  return makeGraph(GeomTool::Instance().getChanIndexFromAntPol(ant,pol)); 
}

TGraph * pueo::UsefulEvent::makeGraph(ring::ring_t ring, int phi, pol::pol_t pol) const
{
  return makeGraph(GeomTool::Instance().getChanIndexFromRingPhiPol(ring,phi,pol)); 
}

TGraph * pueo::UsefulEvent::makeGraph(int surf, int chan) const
{
  return makeGraph(GeomTool::Instance().getChanIndex(surf,chan)); 
}




TGraph * pueo::UsefulEvent::makeGraph(size_t chanIndex) const
{
  if (chanIndex >= k::NUM_DIGITZED_CHANNELS) return 0; 
  TGraph * g = new TGraph(volts[chanIndex].size()); 
  int ant; 
  pol::pol_t pol; 

  GeomTool::Instance().getAntPolFromChanIndex(chanIndex,ant,pol); 
  for (size_t i = 0; i < volts[chanIndex].size(); i++) 
  {
    g->GetY()[i] = volts[chanIndex][i]; 
    g->GetX()[i] = i * dt[chanIndex] + t0[chanIndex]; 
  }
  g->SetName(Form("ant%d%c", ant, pol::asChar(pol))); 
  g->SetTitle(Form("Antenna %d%c", ant, pol::asChar(pol))); 
  g->GetXaxis()->SetTitle("t [ns]"); 
  g->GetYaxis()->SetTitle("V [mV]"); 
  g->SetBit(TGraph::kIsSortedX); 
  g->SetBit(TGraph::kNotEditable);

  return g; 
} 





