/****************************************************************************************
*  pueo/Hsk.h             PUEO Hsk sensors
*  
*  Housekeeping sensor class
* 
*  Keith McBride <kmcbride@uchicago.edu> based on Cosmin Deaconu's work on the attitude class
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


#ifndef PUEO_HSK_H
#define PUEO_HSK_H

//Includes
#include <TObject.h>

#include <array>
#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif
namespace pueo 
{



  namespace hsk 
  {
    /** @defgroup rootclasses The ROOT Classes
     * These are the ROOT clases that make up the event reader
     */
    // per obj there will be a sensor that has values and time for measurement
    class Sensor: public TObject
    {
     public:
       Sensor() {;}
#ifdef HAVE_PUEORAWDATA
       Sensor(const pueo_sensors_disk_t *hsk, int whichsensor);
#endif
       virtual ~Sensor() {;}
       int which_sensor=0;
       UShort_t sensor_id=0;
       UShort_t time_ms=0;
       UInt_t time_secs=0;
       //union { // expanded to 32 bits 
       Float_t fval;
       Int_t ival;
       UInt_t uval;
       std::string subsys;
       std::string sens_name;
       char typetag;
       char kind_unit;
       //} val;
       ClassDef(Sensor,2);
    };
  }
}


#endif //
