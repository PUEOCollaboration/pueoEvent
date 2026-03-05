/****************************************************************************************
*  pueo/Hsk.h            Implementation of the PUEO hsk sensor class
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


#include "pueo/Hsk.h" 

ClassImp(pueo::hsk::Sensor); 

#ifdef HAVE_PUEORAWDATA
pueo::hsk::Sensor::Sensor(const pueo_sensors_disk_t *hsk,int whichsensor) :
  which_sensor(whichsensor),
  sensor_id(hsk->sensors[whichsensor].sensor_id),
  time_ms(hsk->sensors[whichsensor].time_ms),
  time_secs(hsk->sensors[whichsensor].time_secs),
  fval((Float_t) hsk->sensors[whichsensor].val.fval),
  ival((Int_t) hsk->sensors[whichsensor].val.ival),
  uval((UInt_t) hsk->sensors[whichsensor].val.uval),
  subsys(pueo_sensor_id_get_subsystem(hsk->sensors[whichsensor].sensor_id)),
  sens_name(pueo_sensor_id_get_name(hsk->sensors[whichsensor].sensor_id)),
  typetag(pueo_sensor_id_get_type_tag(hsk->sensors[whichsensor].sensor_id)),
  kind_unit(pueo_sensor_id_get_kind(hsk->sensors[whichsensor].sensor_id))
{
    // Implementation to initialize Hsk from pueo_sensor_disk_t
}


#endif
