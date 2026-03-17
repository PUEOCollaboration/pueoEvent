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
#ifdef HAVE_PUEORAWDATA
  Time(const pueo_time_t *t):
    utc_secs(t->utc_secs),  
    utc_nsecs(t->utc_nsecs)
  {
    ;
  }
#endif
  ULong64_t utc_secs;
  ULong64_t utc_nsecs; // screw it, it gets 64 bit whether it needs it or not :)
};


class Timemark: public TObject
{
public:
  Timemark(){;}

#ifdef HAVE_PUEORAWDATA
  // Timemark(const pueo_timemark_t *tmrk);
  Timemark(const pueo_timemark_t *tmrk) :
    readout_time(&tmrk->readout_time),
    rising(&tmrk->rising),
    falling(&tmrk->falling),
    rise_count(tmrk->rise_count),
    channel(tmrk->channel),
    flags(tmrk->flags){;}
#endif

  virtual ~Timemark() {;}

  pueo::Time readout_time;
  pueo::Time rising;
  pueo::Time falling;
  UInt_t rise_count;
  UInt_t channel;
  UInt_t flags;

  // CERN ROOT bs
  ClassImp(pueo::hsk::Sensor); 
  ClassDef(Timemark,2);
};
}

#endif
