#ifndef PUEO_TIMEMARK_H
#define PUEO_TIMEMARK_H

#include <TObject.h>

#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif

namespace pueo 
{
class Time
{
public:
  Time(){;}
  virtual ~Time() {;}

  ULong64_t utc_secs;
  ULong64_t utc_nsecs; // screw it, it gets 64 bit whether it needs it or not :)
  ClassDef(Time,2);

#ifdef HAVE_PUEORAWDATA
  Time(const pueo_time_t *t);
#endif
};


class Timemark: public TObject
{
public:
  Timemark(){;}
  virtual ~Timemark() {;}

  pueo::Time readout_time;
  pueo::Time rising;
  pueo::Time falling;
  UInt_t rise_count;
  UInt_t channel;
  UInt_t flags;
  ClassDef(Timemark,2); // CERN ROOT bs

#ifdef HAVE_PUEORAWDATA
  Timemark(const pueo_timemark_t *tmrk);
#endif
};
}

#endif
