#define STRINGIFY(X) _STRINGIFY(X)
#define _STRINGIFY(X) #X
#define CONCAT(X,Y) _CONCAT(X,Y)
#define _CONCAT(X,Y) X##Y

#define PUEO_NCHAN 208 // only temporary!!!!

#define ONEDIM  // empty string for one dimension
#define FULLWAVEFORM_FIELDS(LiterallyAnX)\
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
X(pueo_time_t,     ONEDIM,         readout_time_utc_secs)\
X(pueo_waveform_t, [PUEO_NCHAN],   wfs)


void tmp_xmacro_toyscript(){
#define X(VARTYPE, VARDIM, VARNAME) VARTYPE VARNAME VARDIM; 
FULLWAVEFORM_FIELDS(X)
#undef X
}
