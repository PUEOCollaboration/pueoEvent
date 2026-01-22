#include <cstdint>
#include "TTree.h"

#define STRINGIFY(X) _STRINGIFY(X)
#define _STRINGIFY(X) #X
#define CONCAT(X,Y) _CONCAT(X,Y)
#define _CONCAT(X,Y) X##Y

// for cern root tree type annotation
#define TYPETAG_uint16_t s
#define TYPETAG_uint8_t b
#define TYPETAG_uint32_t i
#define TYPETAG_float F
#define TYPETAG_bool O


#define PUEO_NCHAN 208 // only temporary!!!!

#define ONEDIM  // empty string for one dimension
#define FULLWAVEFORM_FIELDS \
X(uint32_t,        ONEDIM,         run)\
X(uint32_t,        ONEDIM,         event)\
X(uint32_t,        ONEDIM,         event_second)\
X(uint32_t,        ONEDIM,         event_time)\
X(uint32_t,        ONEDIM,         last_pps)\
X(uint32_t,        ONEDIM,         llast_pps)\
X(uint32_t,        ONEDIM,         deadtime_counter)\
X(uint32_t,        ONEDIM,         deadtime_counter_last_pps)\
X(uint32_t,        ONEDIM,         deadtime_counter_llast_pps)\
X(uint32_t,        ONEDIM,         L2_mask)\
X(uint32_t,        ONEDIM,         soft_trigger)\
X(uint32_t,        ONEDIM,         pps_trigger)\
X(uint32_t,        ONEDIM,         ext_trigger)\
// X(pueo_time_t,     ONEDIM,         readout_time_utc_secs)\
// X(pueo_waveform_t, [PUEO_NCHAN],   wfs)


void tmp_xmacro_toyscript(){

  TTree * fwft = new TTree("fullwaveform", "fullwaveform");

#define X(VARTYPE, VARDIM, VARNAME) \
VARTYPE VARNAME VARDIM = {};\
fwft->Branch(#VARNAME, &VARNAME, STRINGIFY(VARNAME) STRINGIFY(VARDIM) "/" STRINGIFY(CONCAT(TYPETAG_,VARTYPE)));
FULLWAVEFORM_FIELDS
#undef X

  fwft->Print();
}
