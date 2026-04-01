#ifndef PUEO_TIMEMARK_H
#define PUEO_TIMEMARK_H
#include "TTimeStamp.h"
#include <cstdint>

#ifdef HAVE_PUEORAWDATA
#include "pueo/rawdata.h"
#endif

namespace pueo
{

  class Timemark
  {
  public:
    Timemark(): readout_time(0,0), rising(0,0), falling(0,0){;}

    TTimeStamp readout_time;
    TTimeStamp rising;
    TTimeStamp falling;
    uint16_t rise_count=0;
    uint8_t channel=0;
    uint8_t flags=0;

    int32_t run=-999;          ///< not in raw data, but should be identifiable via post processing
    int32_t event_number=-999; ///< not in raw data, but should be identifiable via post processing

    ClassDefNV(Timemark, 3); // non virtual ClassDef, see https://root-forum.cern.ch/t/classdef-variants/44736/6

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
