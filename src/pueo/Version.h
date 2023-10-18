
/****************************************************************************************
*  pueo/Version.h              The PUEO versioneer
*
*  This file is inherited from ANITA, where multiple payload versions existed.
*  This is not yet a problem for PUEO, but we might as well keep the machinery out of a sense of optimistc. 
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

#ifndef _PUEO_VERSION_H
#define _PUEO_VERSION_H


/** The default PUEO version may be overriden at compile time. */ 
#ifndef DEFAULT_PUEO_VERSION
#define DEFAULT_PUEO_VERSION 1 
#endif 


 /** Define this macro so other code can act accordingly*/ 
#define MULTIVERSION_PUEO 1


namespace pueo
{
  namespace version 
  {

  /** Set the ANITA Version to use for e.g. calibration.
     *  If eventReaderRoot is compiled with THREADSAFE_PUEO_VERSION,
     *  it is safe to use different PUEO versions in different threads 
     */ 

    void set(int v); 

    /** Retrieve the current ANITA Version*/ 
    int get(); 


    /** Utility function to retrieve the anita version from the unix time. Used
     * when processing data etc. */ 
    int getVersionFromUnixTime(unsigned time); 

    /** Sets the version from unix time  (equivalent to calling set(getVersionFromUnixTime(t)) )
     *
     * returns 0 if successful, 1 if time is suspicious
     * */ 
    int setVersionFromUnixTime(unsigned time); 


  } 
}



#endif 
