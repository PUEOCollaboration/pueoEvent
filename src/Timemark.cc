#include "pueo/Timemark.h" 

ClassImp(pueo::Timemark); // CERN ROOT bs, cannot be header only, and I must have this line???
ClassImp(pueo::Time);

#ifdef HAVE_PUEORAWDATA
pueo::Time::Time(const pueo_time_t *t):utc_secs(t->utc_secs),utc_nsecs(t->utc_nsecs){;}

pueo::Timemark::Timemark(const pueo_timemark_t *tmrk):
  readout_time(&tmrk->readout_time),
  rising(&tmrk->rising),
  falling(&tmrk->falling),
  rise_count(tmrk->rise_count),
  channel(tmrk->channel),
  flags(tmrk->flags)
{;}
#endif
