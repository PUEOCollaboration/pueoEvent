#ifndef PUEO_TIMEMARK_H
#define PUEO_TIMEMARK_H
#include "Rtypes.h"

#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif

namespace pueo 
{
class Time
{
public:
  Time(){;}

  ULong64_t utc_secs;
  ULong64_t utc_nsecs; // screw it, it gets 64 bit whether it needs it or not :)
  ClassDefNV(Time, 1);

#ifdef HAVE_PUEORAWDATA
  Time(const pueo_time_t *t):utc_secs(t->utc_secs),utc_nsecs(t->utc_nsecs){;}
#endif
};


class Timemark
{
public:
  Timemark(){;}

  pueo::Time readout_time;
  pueo::Time rising;
  pueo::Time falling;
  UInt_t rise_count;
  UInt_t channel;
  UInt_t flags;
  ClassDefNV(Timemark, 1); // non virual ClassDef, see https://root-forum.cern.ch/t/classdef-variants/44736/6

#ifdef HAVE_PUEORAWDATA
  Timemark(const pueo_timemark_t *tmrk):
    readout_time(&tmrk->readout_time),
    rising(&tmrk->rising),
    falling(&tmrk->falling),
    rise_count(tmrk->rise_count),
    channel(tmrk->channel),
    flags(tmrk->flags)
  {;}
#endif
};
}

#endif
