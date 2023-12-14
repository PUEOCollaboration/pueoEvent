/****************************************************************************************
*  Version.cc            Implementation of the PUEO Versioning tool
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

#
#include "pueo/Version.h" 
#include "pueo/Conventions.h" 
#include <climits>


#include <stdio.h>
#include <iostream>



/** This is the thread-safe version, where multiple threads may be working
* with different PUEO versions at the same time.  It may have a
* performance penalty. */

#ifdef THREADSAFE_PUEO_VERSION
  static __thread volatile int pueo_version = DEFAULT_PUEO_VERSION;
#else 
  static int volatile pueo_version = DEFAULT_PUEO_VERSION; 
#endif 
 

/** These are the boundaries for setting the PUEO version based on timestamp */ 
static const unsigned version_cutoffs[] = 
{ 
  0,0,    //Just padding
  INT_MAX // hopefully PUEO will fly by 2038 
}; 

static bool firstTime = true;

void pueo::version::set(int v) 
{ 

  if(firstTime || pueo_version!=v){
    std::cerr << "pueoVersion=" << v << std::endl;
  }
  pueo_version = v;
  firstTime = false; // don't print warning on AnitaVersion::get() if we called AnitaVersion::set(v)
} 

int pueo::version::get() 
{
  if(firstTime==true){
    std::cerr << "pueoVersion=" << pueo_version << std::endl;
    firstTime = false;
  }

  return pueo_version;

} 

int pueo::version::setVersionFromUnixTime(unsigned t) 
{
  int v = getVersionFromUnixTime(t); 
  if (v>0) 
  {
    set(v); 
    return 0; 
  }
  else
  {
    fprintf(stderr,"Attempting to set a version based on suspicious timestamp: %u\n", t); 
    return 1; 
  }
}

int pueo::version::getVersionFromUnixTime(unsigned t) 
{
  for (int v = k::NUM_PUEO; v>0; v--)
  {
    if (t > version_cutoffs[v]) 
    {
      return v; 
    }
  }

  return -1; 
}

