#ifndef PUEO_TIMEMARK_H
#define PUEO_TIMEMARK_H
#include "Rtypes.h"
#include "TTimeStamp.h"

#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif

namespace pueo
{

  class Timemark
  {
  public:
    Timemark(){;}

    TTimeStamp readout_time;
    TTimeStamp rising;
    TTimeStamp falling;
    UInt_t rise_count;
    UInt_t channel;
    UInt_t flags;

    ClassDefNV(Timemark, 2); // non virual ClassDef, see https://root-forum.cern.ch/t/classdef-variants/44736/6

#ifdef HAVE_PUEORAWDATA
    Timemark(const pueo_timemark_t *tmrk):
      readout_time((time_t) tmrk->readout_time.utc_secs, tmrk->readout_time.utc_nsecs),
      rising((time_t) tmrk->rising.utc_secs, tmrk->rising.utc_nsecs),
      falling((time_t) tmrk->falling.utc_secs, tmrk->falling.utc_nsecs),
      rise_count(tmrk->rise_count),
      channel(tmrk->channel),
      flags(tmrk->flags)
    {;}
#endif
  };
}

#endif
